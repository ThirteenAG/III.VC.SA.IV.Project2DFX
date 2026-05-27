module;

#include <stdafx.h>
#include "rw.h"

export module Clock;

export namespace CClock
{
    GameRef<uint8_t> ms_nGameClockHours;
    GameRef<uint8_t> ms_nGameClockMinutes;

    char(__cdecl* GetIsTimeInRange)(char hourA, char hourB) = nullptr;
}
