#pragma once
#include <stdint.h>

extern "C" {
    __declspec(dllimport) void* RegisterManager();
    __declspec(dllimport) void ApplyFilter(void* managerPtr, const char* jsonOptions);
    __declspec(dllimport) void ProcessFrame(void* managerPtr, uint8_t* rgbaBuffer, int width, int height);
    __declspec(dllimport) void RemoveManager(void* managerPtr);
}