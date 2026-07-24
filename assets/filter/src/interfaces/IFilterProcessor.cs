using OpenCvSharp;
using System;
using System.Collections.Generic;
using System.Text;
using System.Text.Json;

namespace src.interfaces
{
    public interface IFilterProcessor : IDisposable
    {
        void Start(JsonElement config);

        void Update(JsonElement config);

        void Process(Mat frame);
    }
}
