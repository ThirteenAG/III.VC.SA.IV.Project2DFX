module;

#define NOMINMAX
#include <stdafx.h>

export module Game;

// The game's variables and functions the plugin uses, for the Complete Edition and the older versions of GTA IV and
// Episodes from Liberty City: every lookup tries the Complete Edition's pattern first

export template <size_t count = 1, typename... Args>
hook::pattern find_pattern(Args... args)
{
    hook::pattern pattern;
    ((pattern = hook::pattern(args), !pattern.count_hint(count).empty()) || ...);
    return pattern;
}

export namespace CTimer
{
    float* fTimeStep = nullptr;
    uint32_t* m_snTimeInMilliseconds = nullptr;
    bool* ms_bScriptPause = nullptr;
}

export namespace CClock
{
    uint8_t* ms_nGameClockHours = nullptr;
    uint8_t* ms_nGameClockMinutes = nullptr;
}

export namespace CWeather
{
    int32_t* OldWeatherType = nullptr;
    int32_t* NewWeatherType = nullptr;
    float* InterpolationValue = nullptr;
}

export constexpr uint32_t NUM_HOURS = 11;
export constexpr uint32_t NUM_WEATHERS = 9;

export struct TimeCycleParams
{
    uint32_t mAmbient0Color;
    uint32_t mAmbient1Color;
    uint32_t mDirLightColor;
    float mDirLightMultiplier;
    float mAmbient0Multiplier;
    float mAmbient1Multiplier;
    float mAOStrength;
    float mPedAOStrength;
    float mRimLightingMultiplier;
    float mSkyLightMultiplier;
    float mDirLightSpecMultiplier;
    uint32_t mSkyBottomColorFogDensity;
    uint32_t mSunCore;
    float mCoronaBrightness;
    float mCoronaSize;
    float mDistantCoronaBrightness;
    float mDistantCoronaSize;
    float mFarClip;
    float mFogStart;
    float mDOFStart;
    float mNearDOFBlur;
    float mFarDOFBlur;
    uint32_t mLowCloudsColor;
    uint32_t mBottomCloudsColor;
    uint32_t mWater;
    float mUnused64[7];
    float mWaterReflectionMultiplier;
    float mParticleBrightness;
    float mExposure;
    float mBloomThreshold;
    float mMidGrayValue;
    float mBloomIntensity;
    uint32_t mColorCorrection;
    uint32_t mColorAdd;
    float mDesaturation;
    float mContrast;
    float mGamma;
    float mDesaturationFar;
    float mContrastFar;
    float mGammaFar;
    float mDepthFxNear;
    float mDepthFxFar;
    float mLumMin;
    float mLumMax;
    float mLumDelay;
    int32_t mCloudAlpha;
    float mUnusedD0;
    float mTemperature;
    float mGlobalReflectionMultiplier;
    float mUnusedDC;
    float mSkyColor[3];
    float mUnusedEC;
    float mSkyHorizonColor[3];
    float mUnusedFC;
    float mSkyEastHorizonColor[3];
    float mUnused10C;
    float mCloud1Color[3];
    float mUnknown11C;
    float mSkyHorizonHeight;
    float mSkyHorizonBrightness;
    float mSunAxisX;
    float mSunAxisY;
    float mCloud2Color[3];
    float mUnused13C;
    float mCloud2ShadowStrength;
    float mCloud2Threshold;
    float mCloud2Bias1;
    float mCloud2Scale;
    float mCloudInScatteringRange;
    float mCloud2Bias2;
    float mDetailNoiseScale;
    float mDetailNoiseMultiplier;
    float mCloud2Offset;
    float mCloudWarp;
    float mCloudsFadeOut;
    float mCloud1Bias;
    float mCloud1Detail;
    float mCloud1Threshold;
    float mCloud1Height;
    float mUnused17C;
    float mCloud3Color[3];
    float mUnused18C;
    float mUnknown190;
    float mUnused198[3];
    float mSunColor[3];
    float mUnused1AC;
    float mCloudsBrightness;
    float mDetailNoiseOffset;
    float mStarsBrightness;
    float mVisibleStars;
    float mMoonBrightness;
    float mUnused1C4[3];
    float mMoonColor[3];
    float mUnused1DC;
    float mMoonGlow;
    float mMoonParam3;
    float SunCenterStart;
    float SunCenterEnd;
    float mSunSize;
    float mUnused1F8[3];
    float mUnknown200;
    float mSkyBrightness;
    float mUnused208;
    int32_t mFilmGrain;
};

export struct Timecycle
{
    TimeCycleParams mParams[NUM_HOURS][NUM_WEATHERS];
};

export Timecycle* mTimeCycle = nullptr;

// The hour each of the timecycle's time slots starts at
export constexpr int32_t TimecycleSlotHours[NUM_HOURS] = { 0, 5, 6, 7, 9, 12, 18, 19, 20, 21, 22 };

export int(__cdecl* DrawCorona)(int id, char r, char g, char b, float intensity, void* pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14) = nullptr;

// Script natives, called through the game's table like scripts call them
export namespace Natives
{
    enum : uint32_t
    {
        DOES_TEXT_LABEL_EXIST = 0x2A611607,
        FORCE_TIME_OF_DAY = 0xB9B5070,
        GET_CAM_FAR_CLIP = 0x752643C9,
        GET_CAM_POS = 0x60C22E93,
        GET_ROOT_CAM = 0x75E005F1,
    };

    struct Vector3
    {
        float x, y, z;
    };

    struct Vector4
    {
        float x, y, z, w;
    };

    struct Context
    {
        void* m_pReturn;
        uint32_t m_nArgCount;
        void* m_pArgs;
        uint32_t m_nDataCount;
        Vector3* m_pOriginalData[4];
        Vector4 m_TemporaryData[4];
        uint32_t m_Stack[16];
        uint32_t m_Return[4];

        Context() : m_pReturn(m_Return), m_nArgCount(0), m_pArgs(m_Stack), m_nDataCount(0), m_pOriginalData{}, m_TemporaryData{}, m_Stack{}, m_Return{}
        {
        }

        template <typename T>
        void Push(T value)
        {
            static_assert(sizeof(T) <= sizeof(uint32_t));
            m_Stack[m_nArgCount] = 0;
            *reinterpret_cast<T*>(&m_Stack[m_nArgCount++]) = value;
        }
    };

    using Handler = void(__cdecl*)(Context*);

    uint32_t* ms_dwNativeTableSize = nullptr;
    uint32_t** ms_pNatives = nullptr;
    void* (__stdcall* getNativeAddress)(uint32_t) = nullptr;

    Handler GetHandler(uint32_t hash)
    {
        if (getNativeAddress)
            return reinterpret_cast<Handler>(getNativeAddress(hash));

        if (!ms_pNatives || !ms_dwNativeTableSize || !*ms_dwNativeTableSize || !hash)
            return nullptr;

        auto natives = *ms_pNatives;
        auto size = *ms_dwNativeTableSize;
        auto index = hash % size;
        auto step = hash;
        auto entry = natives[2 * index];
        if (entry != hash)
        {
            while (entry)
            {
                step = (step >> 1) + 1;
                index = (step + index) % size;
                entry = natives[2 * index];
                if (entry == hash)
                    break;
            }
        }

        return entry ? reinterpret_cast<Handler>(natives[2 * index + 1]) : nullptr;
    }

    // Natives run only while scripts run (not while the game is paused); false when it didn't run
    template <uint32_t Hash, typename... Args>
    bool Call(Context& context, Args... args)
    {
        static Handler handler = nullptr;
        if (*CTimer::ms_bScriptPause)
            return false;
        if (!handler)
            handler = GetHandler(Hash);
        if (!handler)
            return false;
        (context.Push(args), ...);
        handler(&context);
        return true;
    }

    bool DoesTextLabelExist(const char* label)
    {
        Context context;
        return Call<DOES_TEXT_LABEL_EXIST>(context, label) && (context.m_Return[0] & 0xFF) != 0;
    }

    // The camera the frame is rendered from: position and far clip; false while scripts don't run
    bool GetRenderedCamera(float& x, float& y, float& z, float& farClip)
    {
        int32_t camera = 0;
        Context root;
        if (!Call<GET_ROOT_CAM>(root, &camera))
            return false;
        Context pos;
        Call<GET_CAM_POS>(pos, camera, &x, &y, &z);
        Context clip;
        Call<GET_CAM_FAR_CLIP>(clip, camera, &farClip);
        return true;
    }
}

// The time of day the frame is rendered at: a script's FORCE_TIME_OF_DAY locks it (m_nLockedHours/Minutes, -1 when
// not locked) without changing the clock, and the timecycle uses the locked time
int32_t* pLockedHours = nullptr;
int32_t* pLockedMinutes = nullptr;

export int RenderedHours()
{
    return *pLockedHours >= 0 && *pLockedHours < 24 ? *pLockedHours : *CClock::ms_nGameClockHours;
}

export int RenderedMinutes()
{
    return *pLockedHours >= 0 && *pLockedHours < 24 && *pLockedMinutes >= 0 && *pLockedMinutes < 60 ? *pLockedMinutes : *CClock::ms_nGameClockMinutes;
}

// Text of the Vice City conversion for GTA IV, which has its own map and lights
export bool HasViceCityStrings()
{
    return Natives::DoesTextLabelExist("COL4_17") && Natives::DoesTextLabelExist("ROK3_1");
}

// Finds what the plugin uses
export void FindGameAddresses()
{
    auto pattern = find_pattern("F3 0F 10 05 ? ? ? ? F3 0F 59 05 ? ? ? ? 8B 43 20 53", "F3 0F 10 05 ? ? ? ? F3 0F 59 44 24 ? 83 C4 04 83 7C 24");
    CTimer::fTimeStep = *pattern.get_first<float*>(4);

    pattern = find_pattern("A1 ? ? ? ? A3 ? ? ? ? EB 3A", "A1 ? ? ? ? 39 05 ? ? ? ? 76 1F");
    CTimer::m_snTimeInMilliseconds = *pattern.get_first<uint32_t*>(1);

    pattern = hook::pattern("0F B6 0D ? ? ? ? 0F B6 C0 0B C1");
    if (!pattern.empty())
        CTimer::ms_bScriptPause = *pattern.get_first<bool*>(15);
    else
        CTimer::ms_bScriptPause = *hook::get_pattern<bool*>("0F B6 0D ? ? ? ? 0F B6 15 ? ? ? ? 33 C0 0B C1", 10);

    pattern = hook::pattern("A3 ? ? ? ? 8B 44 24 ? A3 ? ? ? ? 8B 44 24 ? A3 ? ? ? ? A1");
    if (!pattern.empty())
    {
        CClock::ms_nGameClockHours = *pattern.get_first<uint8_t*>(1);
        CClock::ms_nGameClockMinutes = *pattern.get_first<uint8_t*>(10);
    }
    else
    {
        pattern = hook::pattern("A3 ? ? ? ? 8B 44 24 ? 89 0D ? ? ? ? 8B 0D");
        CClock::ms_nGameClockHours = *pattern.get_first<uint8_t*>(11);
        CClock::ms_nGameClockMinutes = *pattern.get_first<uint8_t*>(23);
    }

    // The setter of the locked time: mov eax, [esp+4]; mov ds:m_nLockedHours, eax; mov eax, [esp+8]; mov
    // ds:m_nLockedMinutes, eax (older versions load both arguments first)
    auto lockedTime = injector::GetBranchDestination(hook::get_pattern("75 04 6A 0C EB 02 6A 16 E8", 8)).as_int();
    pattern = hook::range_pattern(lockedTime, lockedTime + 19, "8B 44 24 04 A3 ? ? ? ? 8B 44 24 08 A3");
    if (!pattern.empty())
    {
        pLockedHours = *pattern.get_first<int32_t*>(5);
        pLockedMinutes = *pattern.get_first<int32_t*>(14);
    }
    else
    {
        pattern = hook::range_pattern(lockedTime, lockedTime + 20, "8B 44 24 04 8B 4C 24 08 A3 ? ? ? ? 89 0D");
        pLockedHours = *pattern.get_first<int32_t*>(9);
        pLockedMinutes = *pattern.get_first<int32_t*>(15);
    }

    pattern = find_pattern("A1 ? ? ? ? 83 C4 08 8B CF 68 ? ? ? ? 89", "A1 ? ? ? ? 80 3F 04");
    CWeather::OldWeatherType = *pattern.get_first<int32_t*>(1);

    pattern = find_pattern("A1 ? ? ? ? 89 46 4C A1", "A1 ? ? ? ? 77 05 A1 ? ? ? ? 80 3F 04");
    CWeather::NewWeatherType = *pattern.get_first<int32_t*>(1);

    CWeather::InterpolationValue = *hook::get_pattern<float*>("F3 0F 10 05 ? ? ? ? 8B 44 24 0C 8B 4C 24 04", 4);

    pattern = find_pattern("05 ? ? ? ? 50 8D 4C 24 60", "05 ? ? ? ? D9 5C 24 04 8D 4C 24 38 D9");
    mTimeCycle = *pattern.get_first<Timecycle*>(1);

    pattern = find_pattern("8B 15 ? ? ? ? 56 8D 72 01", "A1 ? ? ? ? 56 8D 70 01 81 FE");
    DrawCorona = pattern.get_first<int(__cdecl)(int, char, char, char, float, void*, float, float, float, int, float, char, char, int)>(0);

    auto size = find_pattern<2>("8B 35 ? ? ? ? 85 F6 75 06 33 C0 5E C2 04 00 53 57 8B 7C 24 10", "8B 3D ? ? ? ? 85 FF 75 04 33 C0 5F C3");
    auto natives = find_pattern<2>("8B 1D ? ? ? ? 8B CF 8B 04 D3 3B C7 74 19 8D 64 24 00 85 C0", "8B 1D ? ? ? ? 8B CE 8B 04 D3 3B C6 74 17 85 C0");
    Natives::ms_dwNativeTableSize = *size.get_first<uint32_t*>(2);
    Natives::ms_pNatives = *natives.get_first<uint32_t**>(2);

    // The Complete Edition looks natives up through a function
    pattern = hook::pattern("56 8B 35 ? ? ? ? 85 F6 75 06");
    if (pattern.size() >= 2)
        Natives::getNativeAddress = pattern.get(1).get<void* (__stdcall)(uint32_t)>(0);
}

export unsigned int HashStringLowercase(const char* str)
{
    unsigned int hash = 0;
    auto current = str;

    if (*str == '"')
        current = str + 1;

    while (*current)
    {
        char character = *current;

        if (*str == '"' && character == '"')
            break;

        ++current;

        if ((uint8_t)(character - 'A') <= 25)
            character += 32;
        else if (character == '\\')
            character = '/';

        hash = (1025 * (hash + character) >> 6) ^ 1025 * (hash + character);
    }

    return 32769 * (9 * hash ^ (9 * hash >> 11));
}
