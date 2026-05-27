module;

export module ComVars;

export
{
    int numCoronas = 25000;
    bool bRenderLodLights;
    float fCoronaRadiusMultiplier;
    bool bSlightlyIncreaseRadiusWithDistance;
    float fCoronaFarClip;
    bool autoFarClip;
    char* szCustomCoronaTexturePath;
    bool bRenderStaticShadowsForLODs;
    bool bIncreasePedsCarsShadowsDrawDistance;
    float fStaticShadowsIntensity, fStaticShadowsDrawDistance;
    float fTrafficLightsShadowsIntensity, fTrafficLightsShadowsDrawDistance;
    bool bRenderSearchlightEffects;
    bool bRenderOnlyDuringFoggyWeather;
    bool bRenderHeliSearchlights;
    int nSmoothEffect;
    float fSearchlightEffectVisibilityFactor;
    bool bEnableDrawDistanceChanger;
    float fMinDrawDistanceOnTheGround, fFactor1, fFactor2, fStaticSunSize;
    bool bAdaptiveDrawDistanceEnabled;
    int nMinFPSValue, nMaxFPSValue;
    float fNewFarClip, fMaxPossibleDrawDistance;
    float fMaxDrawDistanceForNormalObjects, fTimedObjectsDrawDistance, fNeonsDrawDistance, fLODObjectsDrawDistance;
    float fGenericObjectsDrawDistance, fAllNormalObjectsDrawDistance, fVegetationDrawDistance;
    bool bLoadAllBinaryIPLs, bPreloadLODs;
    float fDrawDistance;
    bool bRandomExplosionEffects, bReplaceSmokeTrailWithBulletTrail;
}