#include "stdafx.h"
#include "..\includes\CLODLightManager.h"
#include "SearchlightsVC.h"

void CLODLightManager::VC::RenderLODLights()
{
	if (GetIsTimeInRange(19, 7))
	{
		unsigned char	bAlpha;
		float	        fRadius;
		unsigned int	nTime = *CurrentTimeHours * 60 + *CurrentTimeMinutes;
		unsigned int    curMin = *CurrentTimeMinutes;
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
				if ((it->vecPos.z >= -15.0f) && (it->vecPos.z <= 530.0f))
				{
					if (CCameraIsSphereVisible((RwV3d*)&it->vecPos, 5.0f))
					{
						RwV3D*	pCamPos = GetCamPos();
						float		fDistSqr = (pCamPos->x - it->vecPos.x)*(pCamPos->x - it->vecPos.x) + (pCamPos->y - it->vecPos.y)*(pCamPos->y - it->vecPos.y) + (pCamPos->z - it->vecPos.z)*(pCamPos->z - it->vecPos.z);

						if ((fDistSqr > 250.0f*250.0f && fDistSqr < CoronaFarClip*CoronaFarClip) || it->nNoDistance)
						{
							if (!it->nNoDistance)
								fRadius = (fDistSqr < 300.0f*300.0f) ? (0.07f)*sqrt(fDistSqr) - 17.5f : 3.5f; // http://goo.gl/vhAZSx
							else
								fRadius = 3.5f;

							if (SlightlyIncreaseRadiusWithDistance)
								fRadius *= min((0.0025f)*sqrt(fDistSqr) + 0.25f, 4.0f); // http://goo.gl/3kDpnC

							if (it->fCustomSizeMult != 0.45f)
							{
								RegisterCorona(reinterpret_cast<unsigned int>(&*it), it->colour.r, it->colour.g, it->colour.b, bAlpha, (RwV3d*)&it->vecPos, (fRadius * it->fCustomSizeMult * CoronaRadiusMultiplier), CoronaFarClip, 1, 0, 4, 0, 0, 0.0f, false, 1.5f);
								RenderStaticShadowsForLODs ? CShadowsStoreStaticShadow(reinterpret_cast<unsigned int>(&*it), 2, *(RwTexture **)0x978DB4, (CVector*)&it->vecPos, 8.0f, 0.0f, 0.0f, -8.0f, bAlpha, it->colour.r, it->colour.g, it->colour.b, 15.0f, 1.0f, CoronaFarClip, false, 0.0f) : nullptr;
							}
							else
							{
								if ((curMin > 0 && curMin < 10) || (curMin > 20 && curMin < 30) || (curMin > 40 && curMin < 50))
								{
									if ((int)it->vecPos.x % 2 || (int)it->vecPos.y % 2)
										RegisterCorona(reinterpret_cast<unsigned int>(&*it), 255u, 0u, 0u, bAlpha, (RwV3d*)&it->vecPos, (fRadius * it->fCustomSizeMult * CoronaRadiusMultiplier), CoronaFarClip, 1, 0, 4, 0, 0, 0.0f, false, 1.5f);
									else
										RegisterCorona(reinterpret_cast<unsigned int>(&*it), 0u, 255u, 0u, bAlpha, (RwV3d*)&it->vecPos, (fRadius * it->fCustomSizeMult * CoronaRadiusMultiplier), CoronaFarClip, 1, 0, 4, 0, 0, 0.0f, false, 1.5f);
								}
								else
								{
									if ((int)it->vecPos.x % 2 || (int)it->vecPos.y % 2)
										RegisterCorona(reinterpret_cast<unsigned int>(&*it), 0u, 255u, 0u, bAlpha, (RwV3d*)&it->vecPos, (fRadius * it->fCustomSizeMult * CoronaRadiusMultiplier), CoronaFarClip, 1, 0, 4, 0, 0, 0.0f, false, 1.5f);
									else
										RegisterCorona(reinterpret_cast<unsigned int>(&*it), 255u, 0u, 0u, bAlpha, (RwV3d*)&it->vecPos, (fRadius * it->fCustomSizeMult * CoronaRadiusMultiplier), CoronaFarClip, 1, 0, 4, 0, 0, 0.0f, false, 1.5f);
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
void CLODLightManager::VC::DrawDistanceChanger()
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

void CLODLightManager::VC::RegisterLamppost(CEntityVC* entity)
{
	auto	itEnd = pFileContent->upper_bound(PackKey(entity->m_nModelIndex, 0xFFFF));
	for (auto it = pFileContent->lower_bound(PackKey(entity->m_nModelIndex, 0)); it != itEnd; it++)
		m_pLampposts->push_back(CLamppostInfo(entity->matrix * it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nNoDistance, it->second.nDrawSearchlight));
}

void CLODLightManager::VC::RegisterAllLampposts()
{
	for (auto i : VecEntities) {
		if (IsModelALamppost(i.m_nModelIndex))
		{
			RegisterLamppost(&i);
		}
	}
}

void CLODLightManager::VC::RegisterCustomCoronas()
{
	unsigned short		nModelID = 65534;

	auto	itEnd = pFileContent->upper_bound(PackKey(nModelID, 0xFFFF));
	for (auto it = pFileContent->lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
		m_pLampposts->push_back(CLamppostInfo(it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nNoDistance, it->second.nDrawSearchlight));
}


DWORD SirenParticlesPosRed, SirenParticlesPosBlue;
DWORD jmpAddr;
static void __declspec(naked) StoreSirenParticlesRed()
{
	__asm mov SirenParticlesPosRed, eax
	__asm mov dl, [esp + 00000090h]
	__asm mov jmpAddr, 0x58C6D5
	__asm jmp jmpAddr
}

static void __declspec(naked) RenderSirenParticlesRed()
{
	CLODLightManager::VC::CShadowsStoreStaticShadow(rand(), 2, *(RwTexture **)0x978DB4, (CVector*)SirenParticlesPosRed, 8.0f, 0.0f, 0.0f, -8.0f, 128, 25, 0, 0, 15.0f, 1.0f, 100.0f, false, 8.0f);
	__asm mov jmpAddr, 0x58C796
	__asm jmp jmpAddr
}

static void __declspec(naked) StoreSirenParticlesBlue()
{
	__asm mov SirenParticlesPosBlue, eax
	__asm mov     cl, [esp + 80h]
	__asm mov jmpAddr, 0x58C71F
	__asm jmp jmpAddr
}

static void __declspec(naked) RenderSirenParticlesBlue()
{
	__asm add     esp, 40h
	CLODLightManager::VC::CShadowsStoreStaticShadow(rand(), 2, *(RwTexture **)0x978DB4, (CVector*)SirenParticlesPosBlue, 8.0f, 0.0f, 0.0f, -8.0f, 128, 0, 0, 25, 15.0f, 1.0f, 100.0f, false, 8.0f);
	__asm mov jmpAddr, 0x58C796
	__asm jmp jmpAddr
}

void CLODLightManager::VC::ApplyMemoryPatches()
{
	if (bRenderLodLights)
	{
		injector::MakeCALL(0x4A6547, RenderLODLights, true);

		injector::MakeNOP(0x544186, 6, true); //disable ambientBrightness change
		injector::MakeNOP(0x544533, 6, true);
	}

	if (RenderSearchlightEffects)
	{
		injector::MakeJMP(0x4A6560, RenderSearchLights, true);
	}

	if (SmoothEffect)
	{
		SmoothEffect = 1;
	}

	injector::WriteMemory<char>(0x542E66, 127, true); // sun reflection

	injector::WriteMemory<float>(0x68A860, 300.0f, true); // Traffic lights coronas draw distance

	injector::MakeJMP(0x58C6CE, StoreSirenParticlesRed, true);
	injector::MakeJMP(0x58C70C, RenderSirenParticlesRed, true);
	injector::MakeJMP(0x58C718, StoreSirenParticlesBlue, true);
	injector::MakeJMP(0x58C769, RenderSirenParticlesBlue, true);

	if (TrafficLightsShadowsDrawDistance)
	{
		injector::WriteMemory<float>(0x68A848, TrafficLightsShadowsDrawDistance, true);
	}

	if (StaticShadowsDrawDistance)
	{
		injector::WriteMemory<float>(0x6882A4, StaticShadowsDrawDistance, true);
		injector::WriteMemory(0x541590 + 0x7A4 + 2, &StaticShadowsDrawDistance, true);
		injector::WriteMemory(0x541590 + 0x8D5 + 2, &StaticShadowsDrawDistance, true);
	}

	if (StaticShadowsIntensity)
	{
		injector::WriteMemory<float>(0x69587C, StaticShadowsIntensity, true);
		injector::WriteMemory<int>(0x465914, 255, true);
	}

	if (TrafficLightsShadowsIntensity)
	{
		injector::WriteMemory(0x463F90 + 0x7E9 + 2, &TrafficLightsShadowsIntensity, true);
		injector::WriteMemory(0x463F90 + 0x82E + 2, &TrafficLightsShadowsIntensity, true);
		injector::WriteMemory(0x463F90 + 0x873 + 2, &TrafficLightsShadowsIntensity, true);
		injector::WriteMemory(0x463F90 + 0xF4F + 2, &TrafficLightsShadowsIntensity, true);
		injector::WriteMemory(0x463F90 + 0xF94 + 2, &TrafficLightsShadowsIntensity, true);
		injector::WriteMemory(0x463F90 + 0xFD7 + 2, &TrafficLightsShadowsIntensity, true);
		injector::WriteMemory(0x463F90 + 0x18CE + 2, &TrafficLightsShadowsIntensity, true);
		injector::WriteMemory(0x463F90 + 0x1913 + 2, &TrafficLightsShadowsIntensity, true);
		injector::WriteMemory(0x463F90 + 0x1956 + 2, &TrafficLightsShadowsIntensity, true);
	}

	//Car Shadows
	if (IncreasePedsCarsShadowsDrawDistance)
	{
		injector::MakeJMP(0x56DA3F, 0x56DBF3, true); //ped shadows draw distance

		//Car Shadows
		injector::WriteMemory<unsigned char>(0x0056DEC1, 0x85u, true); //headlight twitching fix
		injector::WriteMemory<unsigned char>(0x0056DD36, 0x75u, true); //headlight on far distance 
		injector::WriteMemory<unsigned char>(0x0056E004, 0x75u, true); //shadow on far distance 
		injector::WriteMemory<unsigned char>(0x0058E2B7, 0x55u, true); //rgb
		injector::WriteMemory<unsigned char>(0x0058E2B9, 0x55u, true);
		injector::WriteMemory<unsigned char>(0x0058E2BB, 0x55u, true);
	}

	if (DrawDistance)
	{
		injector::WriteMemory<float>(0x690220, DrawDistance, true);
	}

	if (MaxDrawDistanceForNormalObjects)
	{
		injector::WriteMemory<float>(0x69022C, MaxDrawDistanceForNormalObjects, true);
	}

	if (EnableDrawDistanceChanger)
	{
		injector::MakeJMP(0x4A65CD, DrawDistanceChanger, true);

		injector::WriteMemory(0x4A602B + 0x2, &fNewFarClip, true);
		injector::WriteMemory(0x4A6037 + 0x2, &fNewFarClip, true);
	}
}

void CLODLightManager::VC::LoadDatFile()
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
					int				nDrawSearchlight = 0;
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
		RenderSearchlightEffects = 0;
		bRenderLodLights = 0;
	}
}

DWORD jmpAddress = 0x48D4C4;
char* buffer = (char *)0x7D7C38;
char* tempptr;
char* tempptr2;
unsigned int IDEmodelID, IDEDrawDistance;
char sIDEDrawDistance[5], Flags2[20];
void __declspec(naked) CLODLightManager::VC::GenericIDEHook()
{
	__asm MOV BYTE PTR SS : [EBP + 7D7C38h], 20h

	tempptr = strchr(buffer, ',');
	tempptr2 = strchr(buffer, '.');
	if (!tempptr && !tempptr2)
	{
		sscanf(buffer, "%d %*s %*s %*d %d %*s %*s %*s", &IDEmodelID, &IDEDrawDistance);

		if ((IDEmodelID >= 300 && IDEmodelID <= 632))
		{
			if (IDEDrawDistance >= 10 && IDEDrawDistance < 300)
			{
				sprintf(sIDEDrawDistance, "%d", IDEDrawDistance);
				tempptr = strstr(buffer + 10, sIDEDrawDistance);

				if (IDEDrawDistance >= 100)
					strncpy(Flags2, tempptr + 4, 15);
				else
					strncpy(Flags2, tempptr + 3, 15);

				strncpy(tempptr, "300  ", 5);
				strncpy(tempptr + 5, Flags2, 15);
			}
		}
		if (bPreloadLODs)
		{
			if (IDEmodelID == 2600 || IDEmodelID == 2544 || IDEmodelID == 2634 || IDEmodelID == 2545)
			{
				if (IDEDrawDistance == 3000 || IDEDrawDistance == 1000)
				{
					sprintf(sIDEDrawDistance, "%d", IDEDrawDistance);
					tempptr = strstr(buffer + 10, sIDEDrawDistance);

					strncpy(Flags2, tempptr + 5, 15);

					strncpy(tempptr, "0    ", 5);
					strncpy(tempptr + 6, Flags2, 15);
					//MessageBox(0, buffer, "0", 0);
				}
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
				strncpy(tempptr2 + 11, "3", 1);
			}
		}
	}
	__asm jmp jmpAddress
}

DWORD ipljmpAddress1 = 0x48AD9C;
DWORD ipljmpAddress2 = 0x48AF57;
DWORD _EAX;
CEntityVC* EntityVC;

void __declspec(naked) CLODLightManager::VC::IPLDataHook1()
{
	_asm
	{
		mov _EAX, eax
		mov eax, 0x4DFAE0
		call eax
		mov eax, _EAX
		push    ebx
			mov EntityVC, ebx
	}

	VecEntities.push_back(*EntityVC);
	__asm jmp ipljmpAddress1
}

void __declspec(naked) CLODLightManager::VC::IPLDataHook2()
{
	_asm
	{
		push    esi
			mov EntityVC, esi
	}

	VecEntities.push_back(*EntityVC);
	_asm
	{
		mov _EAX, eax
		mov eax, 0x4DB3F0
		call eax
		mov eax, _EAX
		jmp ipljmpAddress2
	}
}

void CLODLightManager::VC::Init()
{
	CIniReader iniReader("");
	bRenderLodLights = iniReader.ReadInteger("LodLights", "RenderLodLights", 1);
	CoronaRadiusMultiplier = iniReader.ReadFloat("LodLights", "CoronaRadiusMultiplier", 1.0f);
	SlightlyIncreaseRadiusWithDistance = iniReader.ReadInteger("LodLights", "SlightlyIncreaseRadiusWithDistance", 1) != 0;
	if (strncmp(iniReader.ReadString("LodLights", "CoronaFarClip", "auto"), "auto", 4) != 0)
		CoronaFarClip = iniReader.ReadFloat("LodLights", "CoronaFarClip", 0.0f);
	else
		autoFarClip = true;

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

	MaxDrawDistanceForNormalObjects = iniReader.ReadFloat("DistanceLimits", "MaxDrawDistanceForNormalObjects", 0.0);
	DrawDistance = iniReader.ReadFloat("DistanceLimits", "DrawDistance", 0.0);
	bPreloadLODs = iniReader.ReadInteger("DistanceLimits", "PreloadLODs", 0) == 1;

	LoadDatFile();
	if (bRenderLodLights)
	{
		RegisterAllLampposts();
		RegisterCustomCoronas();
	}
	ApplyMemoryPatches();

	//delete pFileContent;
	//VecEntities.clear();
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		if (injector::address_manager::singleton().IsVC())
		{
			if (injector::address_manager::singleton().GetMajorVersion() == 1 && injector::address_manager::singleton().GetMinorVersion() == 0)
			{
				injector::MakeJMP(0x48D4BD, CLODLightManager::VC::GenericIDEHook);
				injector::MakeJMP(0x48AD96, CLODLightManager::VC::IPLDataHook1);
				injector::MakeJMP(0x48AF51, CLODLightManager::VC::IPLDataHook2);
				injector::MakeCALL(0x4A4D10, CLODLightManager::VC::Init);

				CIniReader iniReader("");
				if (bPreloadLODs = iniReader.ReadInteger("DistanceLimits", "PreloadLODs", 0) == 1)
				{
					injector::WriteMemory(0x4DE4A7, nLevelPortland, true);

					injector::WriteMemory(0x4C8C31 + 0x1, &nLevelPortland, true);

					injector::MakeInline<0x40EEB8, 0x40EEB8 + 5>([](injector::reg_pack& regs)
					{	});

					injector::WriteMemory(0x691538, 0x4DDDDD, true); //CFileLoader::LoadMapZones((char const *))
				}
			}
		}
	}
	return TRUE;
}