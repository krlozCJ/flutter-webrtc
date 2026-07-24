using OpenCvSharp;
using src.filters;
using src.interfaces;
using System;
using System.Collections.Generic;
using System.Text;
using System.Text.Json;

namespace src
{
    public class FilterManagerOrchestor : IDisposable
    {
        // Usamos un diccionario que mantiene el orden de inserción si es necesario,
        // o simplemente un diccionario normal. La llave es el ID del filtro.
        private readonly Dictionary<string, IFilterProcessor> _filters = new();

        public FilterManagerOrchestor()
        {
        }

        public void ApplyOptions(string jsonOptions)
        {
            if (string.IsNullOrWhiteSpace(jsonOptions)) return;

            using var document = JsonDocument.Parse(jsonOptions);
            var newConfig = document.RootElement.EnumerateObject().ToDictionary(x => x.Name, x => x.Value);

            // 1. Eliminar los filtros que ya no están en el nuevo mapa
            var keysToRemove = _filters.Keys.Except(newConfig.Keys).ToList();
            foreach (var key in keysToRemove)
            {
                _filters[key].Dispose();
                _filters.Remove(key);
            }

            // 2. Actualizar existentes o crear nuevos
            foreach (var kvp in newConfig)
            {
                if (_filters.TryGetValue(kvp.Key, out var existingFilter))
                {
                    existingFilter.Update(kvp.Value);
                }
                else
                {
                    var newFilter = CreateFilterFactory(kvp.Value);
                    if (newFilter != null)
                    {
                        newFilter.Start(kvp.Value);
                        _filters[kvp.Key] = newFilter;
                    }
                }
            }
        }

        public void ProcessFrame(IntPtr buffer, int width, int height)
        {
            if (_filters.Count == 0) return; // Si no hay filtros, no hacemos nada

            // Envolvemos el puntero en un Mat. ¡Cero copias de memoria!
            //using Mat frame = new Mat(height, width, MatType.CV_8UC4, buffer);
            using Mat frame = Mat.FromPixelData(width, height, MatType.CV_8UC4, buffer);

            // Pasamos el MISMO Mat por todos los filtros
            foreach (var filter in _filters.Values)
            {
                filter.Process(frame);
            }
        }

        private IFilterProcessor CreateFilterFactory(JsonElement config)
        {
            // Leemos el tipo de filtro para instanciar la clase correcta
            string type = config.GetProperty("type").GetString()!;
            return type switch
            {
                "beauty" => new BeautyFilter(),
                "color" => new ColorCorrection(),
                "cinema" => new CinemaLut(),
                "privacy" => new PrivacyFilter(),
                _ => new BeautyFilter()
            };
        }

        public void Dispose()
        {
            foreach (var filter in _filters.Values)
            {
                filter.Dispose();
            }
            _filters.Clear();
        }
    }
}
