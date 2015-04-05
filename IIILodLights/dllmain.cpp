#include "stdafx.h"
#include "..\includes\CLODLightManager.h"
#include "SearchlightsIII.h"

void CLODLightManager::III::RenderLODLights()
{
	if (GetIsTimeInRange(19, 7))
	{
		unsigned char	bAlpha;
		float	        fRadius;
		unsigned int	nTime = (char)*CurrentTimeHours * 60 + (char)*CurrentTimeMinutes;
		unsigned int    curMin = (char)*CurrentTimeMinutes;
		CoronaFarClip = autoFarClip ? **fCurrentFarClip : CoronaFarClip;

		if (nTime >= 19 * 60)
			bAlpha = static_cast<unsigned char>((1.0f / 2.0f)*nTime - 570.0f); // http://goo.gl/3rI2tc
		else if (nTime < 3 * 60)
			bAlpha = 150;
		else
			bAlpha = static_cast<unsigned char>((-5.0f / 8.0f)*nTime + 262.5f); // http://goo.gl/M8Dev9

		for (auto it = m_pLampposts->cbegin(); it != m_pLampposts->cend(); it++)
		{
			if (GetDistance((RwV3d*)&it->vecPos, GetCamPos()) <= CoronaFarClip)
			{
				if ((it->vecPos.z >= -15.0f) && (it->vecPos.z <= 1030.0f))
				{
					if (CCameraIsSphereVisible((int *)0x6FACF8, (RwV3d*)&it->vecPos, 5.0f))
					{
						RwV3D*	pCamPos = GetCamPos();
						float		fDistSqr = (pCamPos->x - it->vecPos.x)*(pCamPos->x - it->vecPos.x) + (pCamPos->y - it->vecPos.y)*(pCamPos->y - it->vecPos.y) + (pCamPos->z - it->vecPos.z)*(pCamPos->z - it->vecPos.z);

						if ((fDistSqr > 250.0f*250.0f && fDistSqr < CoronaFarClip*CoronaFarClip) || it->nNoDistance)
						{
							if (!it->nNoDistance)
								fRadius = (fDistSqr < 300.0f*300.0f) ? (0.07f)*sqrt(fDistSqr) - 17.5f : 3.5f;
							else
								fRadius = 3.5f;

							if (SlightlyIncreaseRadiusWithDistance)
								fRadius *= min((0.0025f)*sqrt(fDistSqr) + 0.25f, 4.0f); // http://goo.gl/3kDpnC

							if (it->fCustomSizeMult != 0.45f)
							{
								RegisterCorona(reinterpret_cast<unsigned int>(&*it), it->colour.r, it->colour.g, it->colour.b, bAlpha, (RwV3d*)&it->vecPos, (fRadius * it->fCustomSizeMult * CoronaRadiusMultiplier), CoronaFarClip, 1, 0, 4, 0, 0, 0.0f);
								RenderStaticShadowsForLODs ? CShadowsStoreStaticShadow(reinterpret_cast<unsigned int>(&*it), 2, *(RwTexture **)0x8F2A00, (CVector*)&it->vecPos, 8.0f, 0.0f, 0.0f, -8.0f, bAlpha, it->colour.r, it->colour.g, it->colour.b, 15.0f, 1.0f, CoronaFarClip, false, 0.0f) : nullptr;
							}
							else
							{
								if ((curMin > 0 && curMin < 10) || (curMin > 20 && curMin < 30) || (curMin > 40 && curMin < 50))
								{
									if ((int)it->vecPos.x % 2 || (int)it->vecPos.y % 2)
										RegisterCorona(reinterpret_cast<unsigned int>(&*it), 255u, 0u, 0u, bAlpha, (RwV3d*)&it->vecPos, (fRadius * it->fCustomSizeMult * CoronaRadiusMultiplier), CoronaFarClip, 1, 0, 4, 0, 0, 0.0f);
									else
										RegisterCorona(reinterpret_cast<unsigned int>(&*it), 0u, 255u, 0u, bAlpha, (RwV3d*)&it->vecPos, (fRadius * it->fCustomSizeMult * CoronaRadiusMultiplier), CoronaFarClip, 1, 0, 4, 0, 0, 0.0f);
								}
								else
								{
									if ((int)it->vecPos.x % 2 || (int)it->vecPos.y % 2)
										RegisterCorona(reinterpret_cast<unsigned int>(&*it), 0u, 255u, 0u, bAlpha, (RwV3d*)&it->vecPos, (fRadius * it->fCustomSizeMult * CoronaRadiusMultiplier), CoronaFarClip, 1, 0, 4, 0, 0, 0.0f);
									else
										RegisterCorona(reinterpret_cast<unsigned int>(&*it), 255u, 0u, 0u, bAlpha, (RwV3d*)&it->vecPos, (fRadius * it->fCustomSizeMult * CoronaRadiusMultiplier), CoronaFarClip, 1, 0, 4, 0, 0, 0.0f);
								}
							}
						}
					}
				}
			}
		}
	}
}

Fps _fps;
void CLODLightManager::III::DrawDistanceChanger()
{
	if (AdaptiveDrawDistanceEnabled)
	{
		_fps.update();
		int FPScount = _fps.get();
		if (FPScount < MinFPSValue)
		{
			MinDrawDistanceOnTheGround -= 2.0f;
		}
		else if (FPScount >= MaxFPSValue)
		{
			MinDrawDistanceOnTheGround += 2.0f;
		}
		if (MinDrawDistanceOnTheGround < 800.0f)
			MinDrawDistanceOnTheGround = 800.0f;
		else
		if (MinDrawDistanceOnTheGround > MaxPossibleDrawDistance)
			MinDrawDistanceOnTheGround = MaxPossibleDrawDistance;
	}
	fNewFarClip = (Factor1 / Factor2) * (GetCamPos()->z) + MinDrawDistanceOnTheGround;
}

void CLODLightManager::III::RegisterLamppost(CEntityIII* entity)
{
	auto	itEnd = pFileContent->upper_bound(PackKey(entity->m_nModelIndex, 0xFFFF));
	for (auto it = pFileContent->lower_bound(PackKey(entity->m_nModelIndex, 0)); it != itEnd; it++)
		m_pLampposts->push_back(CLamppostInfo(entity->matrix * it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nNoDistance, it->second.nDrawSearchlight));
}

void CLODLightManager::III::RegisterAllLampposts()
{
	for (auto i : VecEntities) 
	{
		if (IsModelALamppost(i.m_nModelIndex))
		{
			RegisterLamppost(&i);
		}
	}
}

void CLODLightManager::III::RegisterCustomCoronas()
{
	unsigned short		nModelID = 65534;

	auto	itEnd = pFileContent->upper_bound(PackKey(nModelID, 0xFFFF));
	for (auto it = pFileContent->lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
		m_pLampposts->push_back(CLamppostInfo(it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nNoDistance, it->second.nDrawSearchlight));
}

void CLODLightManager::III::ApplyMemoryPatches()
{
	if (bRenderLodLights)
	{
		injector::MakeCALL(0x48D440, RenderLODLights, true);

		injector::MakeNOP(0x4F8E82, 6, true); //disable ambientBrightness change
		injector::MakeNOP(0x4F8F16, 6, true);
	}

	if (RenderSearchlightEffects)
	{
		injector::MakeJMP(0x48E085, RenderSearchLights, true);
	}

	if (SmoothEffect)
	{
		SmoothEffect = 1;
	}

	if (TrafficLightsShadowsDrawDistance)
	{
		injector::WriteMemory(0x455E3F + 0x2, &TrafficLightsShadowsDrawDistance, true);
		injector::WriteMemory(0x455F2D + 0x2, &TrafficLightsShadowsDrawDistance, true);
	}

	if (StaticShadowsDrawDistance)
	{
		injector::WriteMemory<float>(0x5F00E0, StaticShadowsDrawDistance, true);
		injector::WriteMemory<float>(0x5EDF3C, StaticShadowsDrawDistance, true);
		injector::WriteMemory<float>(0x5FB214, StaticShadowsDrawDistance, true);
	}

	if (StaticShadowsIntensity)
	{
		injector::WriteMemory<float>(0x5FB304, StaticShadowsIntensity, true);
		injector::WriteMemory<int>(0x4FACE6, 255, true);
	}

	if (TrafficLightsShadowsIntensity)
	{
		injector::WriteMemory<float>(0x5F00EC, TrafficLightsShadowsIntensity, true);
	}

	//Car Shadows
	if (IncreasePedsCarsShadowsDrawDistance)
	{
		injector::WriteMemory<unsigned char>(0x00513AC2, 0x75u, true); //headlight on far distance 
		injector::WriteMemory<unsigned char>(0x0051388F, 0x75u, true); //shadow on far distance 
		injector::WriteMemory<unsigned char>(0x005394C6, 0x55u, true); //rgb
		injector::WriteMemory<unsigned char>(0x005394C8, 0x55u, true);
		injector::WriteMemory<unsigned char>(0x005394CA, 0x55u, true);
		injector::WriteMemory<unsigned int>(0x537983, 0x008F2A00, true);
		injector::MakeNOP(0x518DCA, 5, true);
		injector::MakeJMP(0x513CFF, 0x513D92); //ped shadows draw distance
	}

	if (DrawDistance)
	{
		injector::WriteMemory<float>(0x5F726C, DrawDistance, true);
	}

	if (MaxDrawDistanceForNormalObjects)
	{
		injector::WriteMemory<float>(0x5F72A4, MaxDrawDistanceForNormalObjects, true);
	}

	if (EnableDrawDistanceChanger)
	{
		injector::MakeJMP(0x48E072, DrawDistanceChanger, true);

		injector::WriteMemory(0x48E5DA + 0x2, &fNewFarClip, true);
		injector::WriteMemory(0x48E5E6 + 0x2, &fNewFarClip, true);
	}
}

void CLODLightManager::III::LoadDatFile()
{
	CIniReader iniReader("");
	DataFilePath = iniReader.GetIniPath();
	char*			tempPointer;
	tempPointer = strrchr(DataFilePath, '.');
	*tempPointer = '\0';
	strcat(DataFilePath, ".dat");

	if (FILE* hFile = CFileMgr::OpenFile(DataFilePath, "r"))
	{
		unsigned short	nModel = 0xFFFF, nCurIndexForModel = 0;
		pFileContent = new std::map<unsigned int, const CLamppostInfo>;

		while (const char* pLine = CFileMgr::LoadLine(hFile))
		{
			if (pLine[0] && pLine[0] != '#')
			{
				if (pLine[0] == '%')
				{
					nCurIndexForModel = 0;
					if (strcmp(pLine, "%additional_coronas") != 0)
						nModel = GetModelInfoUInt16(pLine + 1);
					else
						nModel = 65534;
				}
				else
				{
					float			fOffsetX, fOffsetY, fOffsetZ;
					unsigned int	nRed, nGreen, nBlue;
					float			fCustomSize = 1.0f;
					int				nNoDistance = 0;
					int             nDrawSearchlight = 0;
					sscanf(pLine, "%d %d %d %f %f %f %f %d %d", &nRed, &nGreen, &nBlue, &fOffsetX, &fOffsetY, &fOffsetZ, &fCustomSize, &nNoDistance, &nDrawSearchlight);
					pFileContent->insert(std::make_pair(PackKey(nModel, nCurIndexForModel++), CLamppostInfo(CVector(fOffsetX, fOffsetY, fOffsetZ), CRGBA(static_cast<unsigned char>(nRed), static_cast<unsigned char>(nGreen), static_cast<unsigned char>(nBlue)), fCustomSize, nNoDistance, nDrawSearchlight)));
				}
			}
		}

		m_pLampposts = new std::vector<const CLamppostInfo>;

		CFileMgr::CloseFile(hFile);
	}
	else
	{
		bRenderLodLights = 0;
	}

	if (RenderSearchlightEffects)
	{
		mi_lamppost1 = GetModelInfoUInt16("lamppost1");
		mi_lamppost2 = GetModelInfoUInt16("lamppost2");
		mi_lamppost3 = GetModelInfoUInt16("lamppost3");
		mi_sub_floodlite = GetModelInfoUInt16("sub_floodlite");
		mi_Streetlamp1 = GetModelInfoUInt16("Streetlamp1");
		mi_mlamppost = GetModelInfoUInt16("mlamppost");
		mi_doublestreetlght1 = GetModelInfoUInt16("doublestreetlght1");
		mi_Streetlamp2 = GetModelInfoUInt16("Streetlamp2");
		mi_bollardlight = GetModelInfoUInt16("bollardlight");
		mi_lampost_coast = GetModelInfoUInt16("lampost_coast");
		mi_doc_floodlite = GetModelInfoUInt16("doc_floodlite");
	}
}

DWORD jmpAddress = 0x47620F;
char* buffer = (char *)0x6ED4E0;
char* tempptr;
char* tempptr2;
unsigned int modelID, IDEDrawDistance;
char sIDEDrawDistance[5], Flags2[20];
void __declspec(naked) CLODLightManager::III::GenericIDEHook()
{
	__asm MOV BYTE PTR DS : [EBX + 6ED4E0h], 20h

	tempptr = strchr(buffer, ',');
	tempptr2 = strchr(buffer, '.');
	if (!tempptr && !tempptr2)
	{
		sscanf(buffer, "%d %*s %*s %*d %d %*s %*s %*s", &modelID, &IDEDrawDistance);
		if ((modelID >= 1100 && modelID <= 1438) || modelID == 887)
		{
			if (IDEDrawDistance >= 10 && IDEDrawDistance < 300)
			{
				sprintf(sIDEDrawDistance, "%d", IDEDrawDistance);
				tempptr = strstr(buffer + 10, sIDEDrawDistance);
				//MessageBox(0, buffer, "0", 0);

				if (IDEDrawDistance >= 100)
					strncpy(Flags2, tempptr + 4, 15);
				else
					strncpy(Flags2, tempptr + 3, 15);

				strncpy(tempptr, "300  ", 5);
				strncpy(tempptr + 5, Flags2, 15);
				//MessageBox(0, buffer, "0", 0);
			}
		}
	}
	else
	{
		tempptr2 = strstr(buffer, "shad_exp");
		if (!tempptr && tempptr2)
		{
			sscanf(tempptr2 + 11, "%d", &IDEDrawDistance);

			if (IDEDrawDistance >= 100 && IDEDrawDistance < 300)
			{
				//MessageBox(0, tempptr2 + 11, "0", 0);
				strncpy(tempptr2 + 11, "3", 1);
				//MessageBox(0, buffer, "0", 0);
			}
		}
	}
	__asm jmp jmpAddress

}

DWORD ipljmpAddress1 = 0x4787FE;
DWORD ipljmpAddress2 = 0x4789A1;
DWORD _EAX;
CEntityIII* EntityIII;
void __declspec(naked) CLODLightManager::III::IPLDataHook1()
{
	_asm
	{
		mov _EAX, eax
		mov eax, 0x4B8DB0
		call eax
		mov eax, _EAX
		push    ebp
			mov EntityIII, ebp
	}

	VecEntities.push_back(*EntityIII);
	__asm jmp ipljmpAddress1
}

void __declspec(naked) CLODLightManager::III::IPLDataHook2()
{
	_asm
	{
		push    ebp
			mov EntityIII, ebp
	}

	VecEntities.push_back(*EntityIII);
	_asm
	{
		mov _EAX, eax
		mov eax, 0x4AE930
		call eax
		mov eax, _EAX
		jmp ipljmpAddress2
	}
}

void CLODLightManager::III::Init()
{
	CIniReader iniReader("");
	bRenderLodLights = iniReader.ReadInteger("LodLights", "RenderLodLights", 1);
	CoronaRadiusMultiplier = iniReader.ReadFloat("LodLights", "CoronaRadiusMultiplier", 0.0f);
	SlightlyIncreaseRadiusWithDistance = iniReader.ReadInteger("LodLights", "SlightlyIncreaseRadiusWithDistance", 1) != 0;
	if (strncmp(iniReader.ReadString("LodLights", "CoronaFarClip", "auto"), "auto", 4) != 0)
		CoronaFarClip = iniReader.ReadFloat("LodLights", "CoronaFarClip", 0.0f);
	else
		autoFarClip = true;

	DrawDistance = iniReader.ReadFloat("DistanceLimits", "DrawDistance", 0.0f);
	MaxDrawDistanceForNormalObjects = iniReader.ReadFloat("DistanceLimits", "MaxDrawDistanceForNormalObjects", 0.0f);

	RenderStaticShadowsForLODs = iniReader.ReadInteger("StaticShadows", "RenderStaticShadowsForLODs", 0);
	IncreasePedsCarsShadowsDrawDistance = iniReader.ReadInteger("StaticShadows", "IncreasePedsCarsShadowsDrawDistance", 0);
	StaticShadowsIntensity = iniReader.ReadFloat("StaticShadows", "StaticShadowsIntensity", 0.0f);
	StaticShadowsDrawDistance = iniReader.ReadFloat("StaticShadows", "StaticShadowsDrawDistance", 0.0f);
	TrafficLightsShadowsIntensity = iniReader.ReadFloat("StaticShadows", "TrafficLightsShadowsIntensity", 0.0f);
	TrafficLightsShadowsDrawDistance = iniReader.ReadFloat("StaticShadows", "TrafficLightsShadowsDrawDistance", 0.0f);

	RenderSearchlightEffects = iniReader.ReadInteger("SearchLights", "RenderSearchlightEffects", 1);
	fSearchlightEffectVisibilityFactor = iniReader.ReadFloat("SearchLights", "SearchlightEffectVisibilityFactor", 0.4f);
	SmoothEffect = iniReader.ReadInteger("SearchLights", "SmoothEffect", 1);

	EnableDrawDistanceChanger = iniReader.ReadInteger("DrawDistanceChanger", "Enable", 0);
	MinDrawDistanceOnTheGround = iniReader.ReadFloat("DrawDistanceChanger", "MinDrawDistanceOnTheGround", 800.0f);
	Factor1 = iniReader.ReadFloat("DrawDistanceChanger", "Factor1", 2.0f);
	Factor2 = iniReader.ReadFloat("DrawDistanceChanger", "Factor2", 1.0f);
	StaticSunSize = iniReader.ReadFloat("DrawDistanceChanger", "StaticSunSize", 20.0f);

	AdaptiveDrawDistanceEnabled = iniReader.ReadInteger("AdaptiveDrawDistance", "Enable", 0);
	MinFPSValue = iniReader.ReadInteger("AdaptiveDrawDistance", "MinFPSValue", 0);
	MaxFPSValue = iniReader.ReadInteger("AdaptiveDrawDistance", "MaxFPSValue", 0);
	MaxPossibleDrawDistance = iniReader.ReadFloat("AdaptiveDrawDistance", "MaxPossibleDrawDistance", 0.0f);

	LoadDatFile();
	if (bRenderLodLights)
	{
		RegisterAllLampposts();
		RegisterCustomCoronas();
	}
	ApplyMemoryPatches();
	
	//delete pFileContent;
	VecEntities.clear();
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		if (injector::address_manager::singleton().IsIII())
		{
			if (injector::address_manager::singleton().GetMajorVersion() == 1 && injector::address_manager::singleton().GetMinorVersion() == 0)
			{
				injector::MakeJMP(0x476208, CLODLightManager::III::GenericIDEHook);
				injector::MakeJMP(0x4787F8, CLODLightManager::III::IPLDataHook1);
				injector::MakeJMP(0x47899B, CLODLightManager::III::IPLDataHook2);
				injector::MakeCALL(0x48C09F, CLODLightManager::III::Init);
			}
		}
	}
	return TRUE;
}