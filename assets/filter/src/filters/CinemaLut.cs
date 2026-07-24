using OpenCvSharp;
using src.interfaces;
using System;
using System.IO;
using System.Text.Json;

public class CinemaLut : IFilterProcessor
{
    private int _lutSize;
    private byte[]? _lutData; // Guardaremos la tabla escalada a bytes (0-255) para máxima velocidad

    private bool _loaded = false;

    public void Start(JsonElement config)
    {
        if (config.TryGetProperty("file", out var fileProp))
        {
            string path = fileProp.GetString()!;
            Task.Run(() =>
            {
                LoadCubeFile(path);
                _loaded = true;
            });
        }
    }

    public void Update(JsonElement config)
    {
        // Podrías cargar un .cube diferente si el usuario cambia de filtro en vivo
        Start(config);
    }

    private void LoadCubeFile(string path)
    {
        if (!File.Exists(path)) return;

        string[] lines = File.ReadAllLines(path);

        // Encontrar el tamaño (LUT_3D_SIZE). Suele ser 16, 17 o 33.
        foreach (var line in lines)
        {
            if (line.StartsWith("LUT_3D_SIZE"))
            {
                _lutSize = int.Parse(line.Split(' ')[1]);
                _lutData = new byte[_lutSize * _lutSize * _lutSize * 3];
                break;
            }
        }

        int index = 0;
        foreach (var line in lines)
        {
            // Ignorar comentarios y metadatos
            if (string.IsNullOrWhiteSpace(line) || line.StartsWith("#") || line.StartsWith("TITLE") || line.StartsWith("LUT"))
                continue;

            string[] rgb = line.Split(' ', StringSplitOptions.RemoveEmptyEntries);
            if (rgb.Length == 3)
            {
                // El .cube guarda floats de 0.0 a 1.0. Los pasamos a bytes de 0 a 255.
                _lutData![index++] = (byte)(float.Parse(rgb[0]) * 255f); // R
                _lutData[index++] = (byte)(float.Parse(rgb[1]) * 255f); // G
                _lutData[index++] = (byte)(float.Parse(rgb[2]) * 255f); // B
            }
        }
    }

    public unsafe void Process(Mat frame)
    {
        if (_lutData == null || frame.Empty() || _loaded == false) return;

        // Anclamos nuestro arreglo LUT a la memoria para accederlo con puntero
        fixed (byte* lutPtr = _lutData)
        {
            byte* framePtr = (byte*)frame.Data;
            int totalPixels = frame.Width * frame.Height;
            int maxIndex = _lutSize - 1;
            float scale = maxIndex / 255f;

            // Iteramos todo el frame. Asumiendo que el buffer de C++ es RGBA (4 canales)
            for (int i = 0; i < totalPixels; i++)
            {
                int p = i * 4; // Índice del pixel actual

                byte r = framePtr[p];     // R (WebRTC en C++ puede mandar el orden distinto, si se ve azul, invierte R y B)
                byte g = framePtr[p + 1]; // G
                byte b = framePtr[p + 2]; // B
                // framePtr[p + 3] es el canal Alpha, no lo tocamos

                // Mapeo Rápido (Nearest Neighbor)
                // Calculamos en qué cajón de la cuadrícula 3D cae el color actual
                int rIdx = (int)(r * scale);
                int gIdx = (int)(g * scale);
                int bIdx = (int)(b * scale);

                // Fórmula matemática para aplanar coordenadas 3D en un arreglo 1D:
                // Índice = (B * size * size) + (G * size) + R
                int lutPos = (bIdx * _lutSize * _lutSize + gIdx * _lutSize + rIdx) * 3;

                // Reemplazamos los colores originales in-place
                framePtr[p] = lutPtr[lutPos];
                framePtr[p + 1] = lutPtr[lutPos + 1];
                framePtr[p + 2] = lutPtr[lutPos + 2];
            }
        }
    }

    public void Dispose()
    {
        _lutData = null; // Liberamos para el Garbage Collector
    }
}