using OpenCvSharp;
using src.interfaces;
using System.Text.Json;

public class ColorCorrection : IFilterProcessor
{
    private double _contrast = 1.0;
    private double _brightness = 0.0;
    private double _saturation = 1.0; // 1.0 = normal, >1.0 = más saturado
    private int _temperature = 0;     // Valores positivos = cálido, negativos = frío

    public void Start(JsonElement config) => Update(config);

    public void Update(JsonElement config)
    {
        if (config.TryGetProperty("contrast", out var c)) _contrast = c.GetDouble();
        if (config.TryGetProperty("brightness", out var b)) _brightness = b.GetDouble();
        if (config.TryGetProperty("saturation", out var s)) _saturation = s.GetDouble();
        if (config.TryGetProperty("temperature", out var t)) _temperature = t.GetInt32();
    }

    public void Process(Mat frame)
    {
        // 1. BRILLO Y CONTRASTE
        if (_contrast != 1.0 || _brightness != 0.0)
        {
            frame.ConvertTo(frame, -1, alpha: _contrast, beta: _brightness);
        }

        // 2. TEMPERATURA (Frío / Cálido)
        if (_temperature != 0)
        {
            // Asumiendo formato RGBA: Canal 0 = R, Canal 1 = G, Canal 2 = B, Canal 3 = A
            // Nota: Si WebRTC te manda BGRA, invierte el orden de R y B aquí.
            double redShift = _temperature;
            double blueShift = -_temperature;

            // Sumamos valores al rojo y azul. El canal verde y alpha se quedan en 0.
            Scalar tempShift = new Scalar(redShift, 0, blueShift, 0);

            // Cv2.Add suma el escalar asegurándose de no pasarse de 255 ni bajar de 0 (clipping automático)
            Cv2.Add(frame, tempShift, frame);
        }

        // 3. SATURACIÓN
        if (_saturation != 1.0)
        {
            using Mat rgb = new Mat();
            // Convertimos a HSV
            Cv2.CvtColor(frame, rgb, ColorConversionCodes.RGBA2RGB);

            using Mat hsv = new Mat();
            Cv2.CvtColor(rgb, hsv, ColorConversionCodes.RGB2HSV);

            // Separamos los canales: [0] = H, [1] = S, [2] = V
            Mat[] channels = Cv2.Split(hsv);

            // Multiplicamos solo la Saturación
            channels[1].ConvertTo(channels[1], -1, alpha: _saturation);

            // Volvemos a unir y convertimos de vuelta a RGBA
            Cv2.Merge(channels, hsv);

            Cv2.CvtColor(hsv, rgb, ColorConversionCodes.HSV2BGR);
            Cv2.CvtColor(rgb, frame, ColorConversionCodes.RGB2RGBA);

            // Liberamos memoria de los canales temporales
            foreach (var c in channels) c.Dispose();
        }
    }

    public void Dispose() { }
}