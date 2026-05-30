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

#include "Hooking.Patterns.h"

#include "gameref.hpp"
#include "callbacks.h"

#include "rw.h"

inline bool IsModuleUAL(HMODULE mod)
{
    if (GetProcAddress(mod, "IsUltimateASILoader") != NULL || (GetProcAddress(mod, "DirectInput8Create") != NULL && GetProcAddress(mod, "DirectSoundCreate8") != NULL && GetProcAddress(mod, "InternetOpenA") != NULL))
        return true;
    return false;
}

inline bool IsUALPresent()
{
    for (const auto& entry : std::stacktrace::current())
    {
        HMODULE hModule = NULL;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)entry.native_handle(), &hModule))
        {
            if (IsModuleUAL(hModule))
                return true;
        }
    }
    return false;
}