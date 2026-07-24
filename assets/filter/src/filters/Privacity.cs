using Microsoft.ML.OnnxRuntime;
using Microsoft.ML.OnnxRuntime.Tensors;
using OpenCvSharp;
using src.interfaces;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;

public class PrivacyFilter : IFilterProcessor
{
    private InferenceSession? _session;
    private string? _inputName;
    private string? _outputName;
    private bool _loaded = false;

    // Tamaño de entrada típico para selfie_segmentation (256x256)
    private const int ModelSize = 256;

    // Arreglo reutilizable para evitar que el Recolector de Basura (GC) trabaje en cada frame
    private float[]? _tensorData;

    private double _blurIntensity = 15.0; // Radio base para el desenfoque

    public void Start(JsonElement config)
    {
        if (!config.TryGetProperty("model_path", out var pathProp))
            throw new ArgumentException("Se requiere la ruta del modelo ONNX.");

        string modelPath = pathProp.GetString()!;
        Task.Run(() =>
        {

        // 1. Configurar ONNX Runtime para usar la GPU a través de DirectML
        SessionOptions options = new SessionOptions();
        options.AppendExecutionProvider_DML(0); // 0 es el índice de la GPU principal
        options.GraphOptimizationLevel = GraphOptimizationLevel.ORT_ENABLE_ALL;

        _session = new InferenceSession(modelPath, options);

        // Obtener los nombres dinámicos de entrada y salida del modelo
        _inputName = _session.InputMetadata.Keys.GetEnumerator().Current ?? _session.InputMetadata.Keys.First();
        _outputName = _session.OutputMetadata.Keys.GetEnumerator().Current ?? _session.OutputMetadata.Keys.First();

        // Preasignar el arreglo para el tensor (Formato NCHW: 1 x 3 x 256 x 256)
        _tensorData = new float[1 * 3 * ModelSize * ModelSize];

        Update(config);
            _loaded = true;
        });
    }

    public void Update(JsonElement config)
    {
        if (config.TryGetProperty("intensity", out var val))
            _blurIntensity = val.GetDouble();
    }

    public unsafe void Process(Mat frame)
    {
        if (_session == null || frame.Empty() || _tensorData == null || _loaded == false) return;

        int width = frame.Width;
        int height = frame.Height;

        // --- PASO 1: PREPARAR EL TENSOR PARA ONNX ---
        using Mat resizedForModel = new Mat();
        Cv2.Resize(frame, resizedForModel, new Size(ModelSize, ModelSize));

        // Convertir RGBA a RGB (el modelo no usa Alpha)
        using Mat rgbModelInput = new Mat();
        Cv2.CvtColor(resizedForModel, rgbModelInput, ColorConversionCodes.RGBA2RGB);

        // Llenar el tensor Data (Formato NCHW estándar en modelos PyTorch/ONNX)
        // Normalizamos los píxeles de 0-255 a 0.0-1.0 (revisa si tu modelo específico requiere -1 a 1)
        byte* rgbPtr = (byte*)rgbModelInput.Data;
        int channelSize = ModelSize * ModelSize;

        for (int i = 0; i < channelSize; i++)
        {
            int p = i * 3;
            _tensorData[i] = rgbPtr[p] / 255f;                   // R (canal 0)
            _tensorData[i + channelSize] = rgbPtr[p + 1] / 255f; // G (canal 1)
            _tensorData[i + channelSize * 2] = rgbPtr[p + 2] / 255f; // B (canal 2)
        }

        // --- PASO 2: INFERENCIA EN GPU ---
        var tensor = new DenseTensor<float>(_tensorData, new[] { 1, 3, ModelSize, ModelSize });
        var inputs = new List<NamedOnnxValue> { NamedOnnxValue.CreateFromTensor(_inputName, tensor) };

        using IDisposableReadOnlyCollection<DisposableNamedOnnxValue> results = _session.Run(inputs);

        // La salida es una máscara de 256x256 con valores flotantes (probabilidad de ser humano)
        var outputTensor = results.First(v => v.Name == _outputName).AsTensor<float>();

        // --- PASO 3: CREAR LA MÁSCARA Y ESCALARLA ---
        // Envolvemos el tensor de salida en un Mat de OpenCV de 1 canal flotante (CV_32FC1)
        fixed (float* maskDataPtr = outputTensor.ToArray())
        {
            //using Mat mask256 = new Mat(ModelSize, ModelSize, MatType.CV_32FC1, (IntPtr)maskDataPtr);
            using Mat mask256 = Mat.FromPixelData(ModelSize, ModelSize, MatType.CV_32FC1, (IntPtr)maskDataPtr);
            using Mat maskFull = new Mat();
            // Escalar la máscara a la resolución original (ej. 1080p)
            Cv2.Resize(mask256, maskFull, new Size(width, height));

            // --- PASO 4: GENERAR EL FONDO BORROSO (Rápido) ---
            using Mat blurredBackground = new Mat();
            using Mat tempSmall = new Mat();

            // Truco: Reducir -> Difuminar -> Agrandar
            Cv2.Resize(frame, tempSmall, new Size(width / 4, height / 4));

            // Asegurar que el radio sea impar para GaussianBlur
            int ksize = (int)_blurIntensity;
            if (ksize % 2 == 0) ksize++;
            if (ksize < 3) ksize = 3;

            Cv2.GaussianBlur(tempSmall, tempSmall, new Size(ksize, ksize), 0);
            Cv2.Resize(tempSmall, blurredBackground, new Size(width, height));

            // --- PASO 5: BLENDING FINAL IN-PLACE ---
            // Sobreescribimos el frame original fusionando los píxeles nítidos y los borrosos
            byte* framePtr = (byte*)frame.Data;
            byte* blurPtr = (byte*)blurredBackground.Data;
            float* maskPtr = (float*)maskFull.Data;

            int totalPixels = width * height;

            for (int i = 0; i < totalPixels; i++)
            {
                int p = i * 4;
                float humanAlpha = maskPtr[i]; // 1.0 = Humano, 0.0 = Fondo
                float bgAlpha = 1.0f - humanAlpha;

                // Mezcla: (Original * humano) + (Borrado * fondo)
                framePtr[p] = (byte)(framePtr[p] * humanAlpha + blurPtr[p] * bgAlpha);     // R
                framePtr[p + 1] = (byte)(framePtr[p + 1] * humanAlpha + blurPtr[p + 1] * bgAlpha); // G
                framePtr[p + 2] = (byte)(framePtr[p + 2] * humanAlpha + blurPtr[p + 2] * bgAlpha); // B
                // framePtr[p + 3] (Alpha) se queda intacto
            }
        }
    }

    public void Dispose()
    {
        _session?.Dispose();
        _tensorData = null;
    }
}