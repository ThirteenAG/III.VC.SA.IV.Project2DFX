#ifndef __SEARCHLIGHTS
#define __SEARCHLIGHTS
#include "..\includes\stdafx.h"
#include "..\includes\CLODLightManager.h"

class CSearchlights
{
public:
	static void                         (__cdecl drawCustomSpotLightVC)(RwV3D StartPoint, RwV3D EndPoint, float TargetRadius, float baseRadius, float slColorFactor1, char slColorFactor2, float slAlpha = 1.0f);
	static void							RenderHeliSearchLightsVC();
	static void							RenderSearchLightsVC();
};

#endif