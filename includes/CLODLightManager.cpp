#include "..\includes\stdafx.h"
#include "CLODLightManager.h"

std::map<unsigned int, CLamppostInfo>		FileContent;
std::multimap<unsigned int, CLamppostInfo>  FileContentMMap;
std::vector<CLamppostInfo>                  CLODLightManager::m_Lampposts;
std::map<unsigned int, CLamppostInfo>*		CLODLightManager::pFileContent = &FileContent;
std::multimap<unsigned int, CLamppostInfo>* CLODLightManager::pFileContentMMap = &FileContentMMap;
bool							            CLODLightManager::m_bCatchLamppostsNow;
bool										bIsIVEFLC = false;

bool  CLODLightManager::bRenderLodLights;
float CLODLightManager::fCoronaRadiusMultiplier;
bool  CLODLightManager::bSlightlyIncreaseRadiusWithDistance;
float CLODLightManager::fCoronaFarClip;
bool  CLODLightManager::autoFarClip;
char* CLODLightManager::szCustomCoronaTexturePath;
bool  CLODLightManager::bRenderStaticShadowsForLODs;
bool  CLODLightManager::bIncreasePedsCarsShadowsDrawDistance;
float CLODLightManager::fStaticShadowsIntensity, CLODLightManager::fStaticShadowsDrawDistance;
float CLODLightManager::fTrafficLightsShadowsIntensity, CLODLightManager::fTrafficLightsShadowsDrawDistance;
bool  CLODLightManager::bRenderSearchlightEffects;
bool  CLODLightManager::bRenderOnlyDuringFoggyWeather;
bool  CLODLightManager::bRenderHeliSearchlights;
int   CLODLightManager::nSmoothEffect;
float CLODLightManager::fSearchlightEffectVisibilityFactor;
bool  CLODLightManager::bEnableDrawDistanceChanger;
float CLODLightManager::fMinDrawDistanceOnTheGround, CLODLightManager::fFactor1, CLODLightManager::fFactor2, CLODLightManager::fStaticSunSize;
bool  CLODLightManager::bAdaptiveDrawDistanceEnabled;
int   CLODLightManager::nMinFPSValue, CLODLightManager::nMaxFPSValue;
float CLODLightManager::fNewFarClip, CLODLightManager::fMaxPossibleDrawDistance;
float CLODLightManager::fMaxDrawDistanceForNormalObjects, CLODLightManager::fTimedObjectsDrawDistance, CLODLightManager::fNeonsDrawDistance, CLODLightManager::fLODObjectsDrawDistance;
float CLODLightManager::fGenericObjectsDrawDistance, CLODLightManager::fAllNormalObjectsDrawDistance, CLODLightManager::fVegetationDrawDistance;
bool  CLODLightManager::bLoadAllBinaryIPLs, CLODLightManager::bPreloadLODs;
float CLODLightManager::fDrawDistance;
bool CLODLightManager::bRandomExplosionEffects, CLODLightManager::bReplaceSmokeTrailWithBulletTrail, CLODLightManager::bFestiveLights, CLODLightManager::bFestiveLightsAlways;

bool CLODLightManager::IsModelALamppost(unsigned short nModel)
{
	auto	it = pFileContent->lower_bound(PackKey(nModel, 0));
	return it != pFileContent->end() && it->first >> 16 == nModel;
}

void CLODLightManager::LoadDatFile()
{
	CIniReader iniReader("");
	auto DataFilePath = iniReader.GetIniPath();
	DataFilePath.replace_extension(".dat");

	if (FILE* hFile = CFileMgr::OpenFile(DataFilePath.string().c_str(), "r"))
	{
		unsigned short	nModel = 0xFFFF, nCurIndexForModel = 0;
		unsigned int	nModelIV = 0xFFFFFFFF;

		while (const char* pLine = CFileMgr::LoadLine(hFile))
		{
			if (pLine[0] && pLine[0] != '#')
			{
				if (pLine[0] == '%')
				{
					nCurIndexForModel = 0;
					if (strcmp(pLine, "%additional_coronas") != 0)
					{
						if (bIsIVEFLC)
							nModelIV = CLODLightManager::IV::GetHashKey((char *)(pLine + 1), 0);
						else
							nModel = GetModelInfoUInt16(pLine + 1);
					}
					else
					{
						nModel = 65534;
						nModelIV = 0xFFFFFFFF;
					}
				}
				else
				{
					float			fOffsetX, fOffsetY, fOffsetZ;
					unsigned int	nRed, nGreen, nBlue, nAlpha;
					float			fCustomSize = 1.0f;
					float			fDrawDistance = 0.0f;
					int				nNoDistance = 0;
					int				nDrawSearchlight = 0;
					int				nCoronaShowMode = 0;
					if (sscanf(pLine, "%3d %3d %3d %3d %f %f %f %f %f %2d %1d %1d", &nRed, &nGreen, &nBlue, &nAlpha, &fOffsetX, &fOffsetY, &fOffsetZ, &fCustomSize, &fDrawDistance, &nCoronaShowMode, &nNoDistance, &nDrawSearchlight) != 12)
						sscanf(pLine, "%3d %3d %3d %3d %f %f %f %f %2d %1d %1d", &nRed, &nGreen, &nBlue, &nAlpha, &fOffsetX, &fOffsetY, &fOffsetZ, &fCustomSize, &nCoronaShowMode, &nNoDistance, &nDrawSearchlight);
					if (bIsIVEFLC)
						pFileContentMMap->insert(std::make_pair(nModelIV, CLamppostInfo(CVector(fOffsetX, fOffsetY, fOffsetZ), CRGBA(static_cast<unsigned char>(nRed), static_cast<unsigned char>(nGreen), static_cast<unsigned char>(nBlue), static_cast<unsigned char>(nAlpha)), fCustomSize, nCoronaShowMode, nNoDistance, nDrawSearchlight, 0.0f, fDrawDistance)));
					else
						pFileContent->insert(std::make_pair(PackKey(nModel, nCurIndexForModel++), CLamppostInfo(CVector(fOffsetX, fOffsetY, fOffsetZ), CRGBA(static_cast<unsigned char>(nRed), static_cast<unsigned char>(nGreen), static_cast<unsigned char>(nBlue), static_cast<unsigned char>(nAlpha)), fCustomSize, nCoronaShowMode, nNoDistance, nDrawSearchlight, 0.0f, fDrawDistance)));
				}
			}
		}

		m_bCatchLamppostsNow = true;
		CFileMgr::CloseFile(hFile);
	}
	else
	{
		bRenderLodLights = 0;
		bRenderSearchlightEffects = 0;
	}
}

bool CLODLightManager::IsBlinkingNeeded(int BlinkType)
{
	signed int nOnDuration = 0;
	signed int nOffDuration = 0;

	switch (BlinkType)
	{
	case BlinkTypes::DEFAULT:
		return false;
	case BlinkTypes::RANDOM_FLASHING:
		nOnDuration = 500;
		nOffDuration = 500;
		break;
	case BlinkTypes::T_1S_ON_1S_OFF:
		nOnDuration = 1000;
		nOffDuration = 1000;
		break;
	case BlinkTypes::T_2S_ON_2S_OFF:
		nOnDuration = 2000;
		nOffDuration = 2000;
		break;
	case BlinkTypes::T_3S_ON_3S_OFF:
		nOnDuration = 3000;
		nOffDuration = 3000;
		break;
	case BlinkTypes::T_4S_ON_4S_OFF:
		nOnDuration = 4000;
		nOffDuration = 4000;
		break;
	case BlinkTypes::T_5S_ON_5S_OFF:
		nOnDuration = 5000;
		nOffDuration = 5000;
		break;
	case BlinkTypes::T_6S_ON_4S_OFF:
		nOnDuration = 6000;
		nOffDuration = 4000;
		break;
	default:
		return false;
	}

	return *CTimer::m_snTimeInMillisecondsPauseMode % (nOnDuration + nOffDuration) < nOnDuration;
}

