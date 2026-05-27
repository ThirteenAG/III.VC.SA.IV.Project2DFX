module;
#include <cstdint>

export module ModelInfo;

export namespace CModelInfo
{
    void* (__cdecl* GetModelInfo)(const char* name, int* id) = nullptr;
}