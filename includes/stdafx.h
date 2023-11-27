#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <random>
#include <chrono>
#include <ctime>
#include <stacktrace>

#pragma warning(disable:4201)
#pragma warning(disable:4458)

#include "IniReader.h"

#include "injector\injector.hpp"
#include "injector\assembly.hpp"
#include "injector\calling.hpp"
#include "injector\hooking.hpp"
#include "injector\utility.hpp"

#include "rwsdk\rwcore.h"
#include "rwsdk\rpworld.h"

#include "Maths.h"
#include "General.h"
#include "Camera.h"

template<typename T>
inline T random(T a, T b)
{
    return a + static_cast<T>(rand() * (1.0f / RAND_MAX) * (b - a));
}

template<typename T>
inline T Min(const T& a, const T& b)
{
    return a > b ? b : a;
}

template<typename T>
inline T Max(const T& a, const T& b)
{
    return a > b ? a : b;
}

inline bool IsModuleUAL(HMODULE mod)
{
    if (GetProcAddress(mod, "IsUltimateASILoader") != NULL || (GetProcAddress(mod, "DirectInput8Create") != NULL && GetProcAddress(mod, "DirectSoundCreate8") != NULL && GetProcAddress(mod, "InternetOpenA") != NULL))
        return true;
    return false;
}

inline bool IsUALPresent() {
    for (const auto& entry : std::stacktrace::current()) {
        HMODULE hModule = NULL;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)entry.native_handle(), &hModule)) {
            if (IsModuleUAL(hModule))
                return true;
        }
    }
    return false;
}