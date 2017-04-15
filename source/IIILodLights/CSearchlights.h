#ifndef __SEARCHLIGHTS
#define __SEARCHLIGHTS
#include "stdafx.h"
#include "CLODLightManager.h"

class CSearchlights
{
public:
	static void                         (__cdecl drawCustomSpotLightIII)(RwV3D StartPoint, RwV3D EndPoint, float TargetRadius, float baseRadius, float slColorFactor1, char slColorFactor2, float slAlpha = 1.0f);
	static void							RenderHeliSearchLightsIII();
	static void							RenderSearchLightsIII();
};

#endif