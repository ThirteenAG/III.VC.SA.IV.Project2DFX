module;

#include <stdafx.h>

export module TimecycleIV;

export const uint32_t NUM_HOURS = 11;
export const uint32_t NUM_WEATHERS = 9;

export struct TimeCycleParams
{
public:
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

    static int32_t GameTimeToTimecycTimeIndex(const int32_t gameTime)
    {
        const int32_t gameTimeToTimecycTimeIndex[24] = { 0, 0, 0, 0, 0, 1, 2, 3, 3, 4, 4, 4, 5, 5, 5, 5, 5, 5, 6, 7, 8, 9, 10, 10 };
        return gameTimeToTimecycTimeIndex[gameTime];
    }
};

export Timecycle* mTimeCycle = nullptr;

export namespace CWeather
{
    enum eWeatherType : uint32_t
    {
        EXTRASUNNY,
        SUNNY,
        SUNNY_WINDY,
        CLOUDY,
        RAIN,
        DRIZZLE,
        FOGGY,
        LIGHTNING
    };

    eWeatherType* CurrentWeather = nullptr;
}