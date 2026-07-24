using OpenCvSharp;
using src.interfaces;
using System;
using System.Collections.Generic;
using System.Text;
using System.Text.Json;

namespace src.filters
{
    internal class BeautyFilter : IFilterProcessor
    {
        private double _intensity;

        public void Start(JsonElement config) => Update(config);

        public void Update(JsonElement config)
        {
            if (config.TryGetProperty("intensity", out var val))
            {
                _intensity = val.GetDouble();
            }
        }

        public void Process(Mat frame)
        {
            using Mat temp = frame.Clone();
            int d = (int)(_intensity * 10) + 1;
            Cv2.BilateralFilter(temp, frame, d, d * 2, d / 2);
        }

        public void Dispose()
        {
        }
    }
}
