#include "stdafx.h"
#include "..\includes\CLODLightManager.h"
#include "SearchlightsSA.h"

void CLODLightManager::SA::RenderLODLights()
{
	if (GetIsTimeInRange(20, 7) && *ActiveInterior == 0)
	{
		unsigned char	bAlpha;
		float	        fRadius;
		unsigned int	nTime = *CurrentTimeHours * 60 + *CurrentTimeMinutes;
		unsigned int    curMin = *CurrentTimeMinutes;
		CoronaFarClip = autoFarClip ? **fCurrentFarClip : CoronaFarClip;

		if (nTime >= 20 * 60)
			bAlpha = static_cast<unsigned char>((5.0f / 8.0f)*nTime - 750.0f); //http://goo.gl/O03RpE
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
					if (CCameraIsSphereVisible((int *)0xB6F028, (RwV3d*)&it->vecPos, 5.0f))
					{
						CVector*	pCamPos = &TheCamera.Cams[TheCamera.ActiveCam].Source;
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
								RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, bAlpha, (RwV3d*)&it->vecPos, (fRadius * it->fCustomSizeMult * CoronaRadiusMultiplier), CoronaFarClip, 1, 0, 0, 0, 0, 0.0f, false, 1.5f, false, bAlpha, false, true);
								RenderStaticShadowsForLODs ? CShadowsStoreStaticShadow(reinterpret_cast<unsigned int>(&*it), SSHADT_INTENSIVE, *(RwTexture **)0xC403F4, (CVector*)&it->vecPos, 8.0f, 0.0f, 0.0f, -8.0f, bAlpha, (it->colour.r / 3), (it->colour.g / 3), (it->colour.b / 3), 15.0f, 1.0f, CoronaFarClip, false, 0.0f) : nullptr;
							}
							else
							{
								if ((curMin > 0 && curMin < 10) || (curMin > 20 && curMin < 30) || (curMin > 40 && curMin < 50))
								{
									if ((int)it->vecPos.x % 2 || (int)it->vecPos.y % 2)
										RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, 255u, 0u, 0u, bAlpha, (RwV3d*)&it->vecPos, (fRadius * it->fCustomSizeMult * CoronaRadiusMultiplier), CoronaFarClip, 1, 0, 0, 0, 0, 0.0f, false, 1.5f, false, bAlpha, false, true);
									else
										RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, 0u, 255u, 0u, bAlpha, (RwV3d*)&it->vecPos, (fRadius * it->fCustomSizeMult * CoronaRadiusMultiplier), CoronaFarClip, 1, 0, 0, 0, 0, 0.0f, false, 1.5f, false, bAlpha, false, true);
								}
								else
								{
									if ((int)it->vecPos.x % 2 || (int)it->vecPos.y % 2)
										RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, 0u, 255u, 0u, bAlpha, (RwV3d*)&it->vecPos, (fRadius * it->fCustomSizeMult * CoronaRadiusMultiplier), CoronaFarClip, 1, 0, 0, 0, 0, 0.0f, false, 1.5f, false, bAlpha, false, true);
									else
										RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, 255u, 0u, 0u, bAlpha, (RwV3d*)&it->vecPos, (fRadius * it->fCustomSizeMult * CoronaRadiusMultiplier), CoronaFarClip, 1, 0, 0, 0, 0, 0.0f, false, 1.5f, false, bAlpha, false, true);
								}
							}
						}
					}
				}
			}
		}
	}
}


void CLODLightManager::SA::StoreCustomStaticShadows()
{
	if (GetIsTimeInRange(20, 7) && *ActiveInterior == 0)
	{
		unsigned char	bAlpha;
		unsigned int	nTime = (char)*CurrentTimeHours * 60 + (char)*CurrentTimeMinutes;

		if (nTime >= 20 * 60)
			bAlpha = static_cast<unsigned char>((5.0f / 6.0f)*nTime - 1000.0f);
		else if (nTime < 3 * 60)
			bAlpha = 200;
		else
			bAlpha = static_cast<unsigned char>((-5.0f / 6.0f)*nTime + 350.0f);

		for (auto it = m_pLampposts->cbegin(); it != m_pLampposts->cend(); it++)
		{
			if (CCameraIsSphereVisible((int *)0xB6F028, (RwV3d*)&it->vecPos, 5.0f))
			{
				CVector*	pCamPos = &TheCamera.Cams[TheCamera.ActiveCam].Source;
				float		fDistSqr = (pCamPos->x - it->vecPos.x)*(pCamPos->x - it->vecPos.x) + (pCamPos->y - it->vecPos.y)*(pCamPos->y - it->vecPos.y) + (pCamPos->z - it->vecPos.z)*(pCamPos->z - it->vecPos.z);

				if (fDistSqr > 280.0f*280.0f && fDistSqr < CoronaFarClip*CoronaFarClip)
				{
					//float	fRadius = min((fDistSqr < 290.0f*290.0f) ? (1.0f / 4.0f)*sqrt(fDistSqr) - 70.0f : (3.0f / 164.0f)*sqrt(fDistSqr) - (5.0f / 41.0f), 20.0f);
					CShadowsStoreStaticShadow(reinterpret_cast<unsigned int>(&*it), SSHADT_INTENSIVE, *(RwTexture **)0xC403F4, (CVector*)&it->vecPos, 8.0f, 0.0f, 0.0f, -8.0f, bAlpha, (it->colour.r / 2), (it->colour.g / 2), (it->colour.b / 2), 15.0f, 1.0f, CoronaFarClip, false, 0.0f);
				} else {
					CShadowsStoreStaticShadow(reinterpret_cast<unsigned int>(&*it), SSHADT_INTENSIVE, *(RwTexture **)0xC403F4, (CVector*)&it->vecPos, 8.0f, 0.0f, 0.0f, -8.0f, bAlpha, (it->colour.r / 3), (it->colour.g / 3), (it->colour.b / 3), 15.0f, 1.0f, CoronaFarClip, false, 0.0f);
				}
			}
		}
	}
	CShadowsUpdateStaticShadows();
}

Fps _fps;
void CLODLightManager::SA::DrawDistanceChanger()
{
	fNewFarClip = 500.0f;
	static DWORD* pPlayerPed = (DWORD*)0xB6F5F0;

	if (*pPlayerPed)
	{
		if (*(BYTE*)((*pPlayerPed) + 0x2F) != 8) //[byte] Location status
		{
			if (*ActiveInterior == 0)
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
		}
	}
}

void CLODLightManager::SA::RegisterLamppost(CEntity* pObj)
{
	unsigned short		nModelID = pObj->GetModelIndex();
	CMatrix				dummyMatrix;
	CSimpleTransform&	objTransform = pObj->GetTransform();

	if (objTransform.m_translate.x == 0.0f && objTransform.m_translate.y == 0.0f)
		return;
	
	dummyMatrix.SetTranslateOnly(objTransform.m_translate.x, objTransform.m_translate.y, objTransform.m_translate.z);
	dummyMatrix.SetRotateZOnly(objTransform.m_heading);

	auto	itEnd = pFileContent->upper_bound(PackKey(nModelID, 0xFFFF));
	for (auto it = pFileContent->lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
		m_pLampposts->push_back(CLamppostInfo(dummyMatrix * it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nNoDistance, it->second.nDrawSearchlight));
}

void CLODLightManager::SA::RegisterCustomCoronas()
{
	unsigned short		nModelID = 65534;

	gpCustomCoronaTexture = CPNGFileReadFromFile(szCustomCoronaTexturePath);
	if (gpCustomCoronaTexture)
	injector::WriteMemory<unsigned int>(0xC3E004, *(unsigned int*)gpCustomCoronaTexture);

	auto	itEnd = pFileContent->upper_bound(PackKey(nModelID, 0xFFFF));
	for (auto it = pFileContent->lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
		m_pLampposts->push_back(CLamppostInfo(it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nNoDistance, it->second.nDrawSearchlight));
}

void CLODLightManager::SA::LoadDatFile()
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
		m_bCatchLamppostsNow = true;

		CFileMgr::CloseFile(hFile);
	}
	else
	{
		//RenderSearchlightEffects = 0;
		bRenderLodLights = 0;
		injector::MakeNOP(0x53C131, 5);
	}
}

CEntity* CLODLightManager::SA::PossiblyAddThisEntity(CEntity* pEntity)
{
	if (m_bCatchLamppostsNow && IsModelALamppost(pEntity->GetModelIndex()))
		RegisterLamppost(pEntity);

	// Saves some hacking
	return pEntity;
}

static void __declspec(naked) LoadObjectInstHook()
{
	_asm
	{
	 push		eax
	 call		CLODLightManager::SA::PossiblyAddThisEntity
	 add		esp, 34h
	 retn
	}
}

DWORD jmpAddress;
static void __declspec(naked) asmInit()
{
	_asm
	{
	 call		CLODLightManager::SA::LoadDatFile
	 push 0x869B30
	 mov jmpAddress, 0x5B9253
	 jmp jmpAddress
	}
}

static void __declspec(naked) asmEnd()
{
	_asm
	{
	 cmp bRenderLodLights, 0
	 jz label1
	 call		CLODLightManager::SA::RegisterCustomCoronas
	 call		CLODLightManager::SA::EndRegistering
	 label1:
	 push 0
	 push 0x863A80
	 push 0x863A6C
	 mov jmpAddress, 0x53BCBC
	 jmp jmpAddress
	}
}

DWORD jmpAddress2;
static void __declspec(naked) LamppostsCoronaFarclpHook()
{
	_asm
	{
	 //mov     ecx, [esi + 14h]
	 mov jmpAddress2, 0x42BDF5
	 mov     ecx, [jmpAddress2]
	 mov     ecx, [ecx]
	 mov     ecx, [ecx]
	 push    edx
	 push    eax
	 mov jmpAddress2, 0x6FCFC9
	 jmp jmpAddress2
	}
}

float drawDist; DWORD modelID;
void __declspec(naked) IncreaseDrawDistanceForTimedObjects()
{
	if (TimedObjectsDrawDistance <= 10.0f)
		drawDist *= TimedObjectsDrawDistance;
	else
		drawDist = TimedObjectsDrawDistance;

	__asm ret
}

void IncreaseDrawDistanceForObjects()
{
	if (VegetationDrawDistance)
	{
		if (modelID >= 615 && modelID <= 792 && drawDist <= 300.0f)
		{
			if (VegetationDrawDistance <= 10.0f)
				drawDist *= VegetationDrawDistance;
			else
				drawDist = VegetationDrawDistance;

			if (drawDist > MaxDrawDistanceForNormalObjects)
				MaxDrawDistanceForNormalObjects = drawDist;
			__asm ret
		}
	}

	if (drawDist > 300.0f)
	{
		if (LODObjectsDrawDistance)
		{
			if (LODObjectsDrawDistance <= 10.0f)
				drawDist *= LODObjectsDrawDistance;
			else
				if (LODObjectsDrawDistance > drawDist)
				drawDist = LODObjectsDrawDistance;
		}
	}
	else
	{
		if (GenericObjectsDrawDistance)
		{
			if (modelID >= 615 && modelID <= 1572)
			{
				if (GenericObjectsDrawDistance <= 10.0f)
					drawDist *= GenericObjectsDrawDistance;
				else
					drawDist = GenericObjectsDrawDistance;
			} 
			else
			{
				if (AllNormalObjectsDrawDistance)
				{
					if (AllNormalObjectsDrawDistance <= 10.0f)
						drawDist *= AllNormalObjectsDrawDistance;
					else
						drawDist = AllNormalObjectsDrawDistance;
				}
			}
		}
		else
		{
			if (AllNormalObjectsDrawDistance)
			{
				if (AllNormalObjectsDrawDistance <= 10.0f)
					drawDist *= AllNormalObjectsDrawDistance;
				else
					drawDist = AllNormalObjectsDrawDistance;
			}
		}
		if (drawDist > MaxDrawDistanceForNormalObjects)
			MaxDrawDistanceForNormalObjects = drawDist;
	}
	__asm ret
}

void __declspec(naked) asm_IncreaseDrawDistanceForTimedObjects()
{
	__asm mov ecx, [esp + 0x0C]
	__asm lea edx, [esp + 0x28]
	__asm mov drawDist, ecx
	__asm call IncreaseDrawDistanceForTimedObjects
	__asm mov ecx, drawDist
	__asm mov hookJmpAddr, 0x5B3F4B
	__asm jmp hookJmpAddr
}

void __declspec(naked) asm_IncreaseDrawDistanceForObjects()
{
	__asm lea eax, [esp + 0x20]
	__asm add esp, 0x4
	__asm push eax
	__asm mov modelID, ecx
	__asm mov drawDist, edx
	__asm call IncreaseDrawDistanceForObjects
	__asm mov edx, drawDist
	__asm mov hookJmpAddr, 0x5B3DAD
	__asm jmp hookJmpAddr
}

void __cdecl CIplStoreLoadAll()
{
	static auto IplFilePoolLocate = (int (__cdecl *)(const char *name)) 0x404AC0;
	static auto CIplStoreRequestIplAndIgnore = (char *(__cdecl *)(int a1)) 0x405850;

	injector::address_manager::singleton().IsHoodlum() ?
		injector::WriteMemory<char>(0x015651C1 + 3, 0, true) :
		injector::WriteMemory<char>(0x405881 + 3, 0, true);

	for (auto it = IPLStreamNames.cbegin(); it != IPLStreamNames.cend(); it++)
	{
		CIplStoreRequestIplAndIgnore(IplFilePoolLocate(it->c_str()));
	}

	injector::address_manager::singleton().IsHoodlum() ?
		injector::WriteMemory<char>(0x015651C1 + 3, 1, true) :
		injector::WriteMemory<char>(0x405881 + 3, 1, true);
}

void LoadAllBinaryIPLs()
{	
	static auto CIplStoreLoad = (char *(__cdecl *)()) 0x5D54A0;
	CIplStoreLoad();
	CIplStoreLoadAll();
}

static std::vector<void*> lods; // CEntity*
static auto RequestModel = injector::cstd<void(int, int)>::call<0x4087E0>;
static auto LoadAllRequestedModels = injector::cstd<void(bool)>::call<0x40EA10>;
static void RequestLod(uint16_t id)
{
	return RequestModel(id, 2); // STREAMING_CANNOT_DELETE
}

void CLODLightManager::SA::ApplyMemoryPatches()
{
	if (bRenderLodLights)
	{
		injector::MakeJMP(0x5B924E, asmInit, true);
		injector::MakeJMP(0x53BCB0, asmEnd, true);

		injector::MakeJMP(0x538439, LoadObjectInstHook, true);
		injector::MakeCALL(0x53C131, RenderLODLights, true);
	}

	if (EnableDrawDistanceChanger)
	{
		injector::MakeCALL(0x53EBE4, DrawDistanceChanger, true);

		injector::WriteMemory(0x40C524, &fNewFarClip, true);
		injector::WriteMemory(0x553F79, &fNewFarClip, true);
		injector::WriteMemory(0x5556A7, &fNewFarClip, true);
		injector::WriteMemory(0x732515, &fNewFarClip, true);

		injector::WriteMemory(0x53D532, &fNewFarClip, true);
		injector::WriteMemory(0x53DC7B, &fNewFarClip, true);
		injector::WriteMemory(0x53DCB8, &fNewFarClip, true);
		injector::WriteMemory(0x53EA95, &fNewFarClip, true);
	}

	if (StaticSunSize)
	{
		injector::WriteMemory(0x6FC656, &StaticSunSize, true);
		injector::WriteMemory(0x6FC6E2, &StaticSunSize, true);
	}

	//Searchlight effect
	if (RenderSearchlightEffects)
	{
		injector::MakeCALL(0x53E184, RenderSearchLights, true);
	}

	/*if (SmoothEffect)
	{
		SmoothEffect = 1;
		injector::WriteMemory(0x6C60F2 + 1, 0xC4D994 + 0x24, true);
		injector::WriteMemory(0x6C6222 + 3, 0xC4D970 + 0x24, true);
	}*/

	//CPatch::RedirectCall(0x53E0BE, StoreCustomStaticShadows);

	if (bRenderLodLights)
	{
		injector::MakeNOP(0x6FAD78, 10, true); //disable ambientBrightness change
		injector::MakeNOP(0x6FAD84, 6, true);
		injector::MakeNOP(0x6FAE13, 10, true);
		injector::MakeNOP(0x6FAE1F, 6, true);
	}

	injector::WriteMemory(0x6FC051 + 0x2, 0x7080 * 0xA, true); // sun reflection
	injector::WriteMemory<float>(0x49DCF4, 550.0f, true); //Traffic lights corona draw distance
	injector::MakeJMP(0x6FCFC4, LamppostsCoronaFarclpHook, true);

	if (StaticShadowsDrawDistance)
	{
		injector::WriteMemory<float>(0x6FD3A6, StaticShadowsDrawDistance, true); // Lampposts shadows draw distance
		injector::WriteMemory<float>(0x6FD44F, StaticShadowsDrawDistance, true);
		injector::WriteMemory<float>(0x455EF3, StaticShadowsDrawDistance, true);
		injector::WriteMemory<float>(0x4561B3, StaticShadowsDrawDistance, true);
		injector::WriteMemory<float>(0x49DF7A, StaticShadowsDrawDistance, true);
		injector::WriteMemory<float>(0x53B5E2, StaticShadowsDrawDistance, true);
		injector::WriteMemory<float>(0x70C88B, StaticShadowsDrawDistance, true);
		injector::WriteMemory<float>(0x70C9F4, StaticShadowsDrawDistance, true);
	}

	if (StaticShadowsIntensity)
	{
		StaticShadowsIntensity *= 0.00390625f;
		injector::WriteMemory(0x6FD13C, &StaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
		injector::WriteMemory(0x6FD16E, &StaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
		injector::WriteMemory(0x6FD17C, &StaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
		injector::WriteMemory(0x6FD1CE, &StaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
		injector::WriteMemory(0x6FD2C0, &StaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
		injector::WriteMemory(0x6FD301, &StaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
		injector::WriteMemory(0x6FD30F, &StaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
		injector::WriteMemory(0x6FD3BC, &StaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
		injector::WriteMemory(0x6FD3DA, &StaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
		injector::WriteMemory(0x6FD3F8, &StaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
	}

	if (TimedObjectsDrawDistance)
	{
		injector::MakeJMP(0x5B3F43, asm_IncreaseDrawDistanceForTimedObjects, true);
		if ((TimedObjectsDrawDistance > 2.0f && TimedObjectsDrawDistance <= 10.0f) || (TimedObjectsDrawDistance > 300.0f))
		{
			injector::MakeInline<0x53C9B7>([](injector::reg_pack& regs)
			{
				injector::WriteMemory<unsigned char>(0x5D95B0, 0xC3u, true);
				injector::WriteMemory<unsigned char>(0x810CA0, 0xC3u, true);
				injector::WriteMemory<unsigned char>(0x5D9730, 0xC3u, true);
			});
			injector::WriteMemory(0x56420F, 0xB83074, true);
		}
	}

	if (LODObjectsDrawDistance || GenericObjectsDrawDistance || AllNormalObjectsDrawDistance || VegetationDrawDistance)
	{
		injector::MakeJMP(0x5B3DA5, asm_IncreaseDrawDistanceForObjects); //multiply dist

		if (GenericObjectsDrawDistance || AllNormalObjectsDrawDistance || VegetationDrawDistance)
		{
			injector::WriteMemory(0x554230 + 0x3FA + 0x2, &MaxDrawDistanceForNormalObjects, true); //fsub    ds:flt_858FD8
			injector::WriteMemory(0x554FE0 + 0x192 + 0x2, &MaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
			injector::WriteMemory(0x554FE0 + 0x1B8 + 0x2, &MaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
			injector::WriteMemory(0x554FE0 + 0x1DB + 0x2, &MaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
			injector::WriteMemory(0x554FE0 + 0x24E + 0x2, &MaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
			injector::WriteMemory(0x554FE0 + 0x258 + 0x2, &MaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
			injector::WriteMemory(0x554FE0 + 0x262 + 0x2, &MaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
			injector::WriteMemory(0x554FE0 + 0x314 + 0x2, &MaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
			injector::WriteMemory(0x554FE0 + 0x31E + 0x2, &MaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
			injector::WriteMemory(0x554FE0 + 0x328 + 0x2, &MaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
			injector::WriteMemory(0x554FE0 + 0x382 + 0x2, &MaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
			injector::WriteMemory(0x554FE0 + 0x39A + 0x2, &MaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
			injector::WriteMemory(0x554FE0 + 0x3A8 + 0x2, &MaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
			injector::WriteMemory(0x5B51E0 + 0x09A + 0x2, &MaxDrawDistanceForNormalObjects, true); // fcomp   ds:flt_858FD8
			injector::WriteMemory(0x554230 + 0x3B6 + 0x2, &MaxDrawDistanceForNormalObjects, true); // fcomp   ds:flt_858FD8
			injector::WriteMemory(0x554230 + 0x3D0 + 0x2, &MaxDrawDistanceForNormalObjects, true); // fld     ds:flt_858FD8
			injector::WriteMemory(0x554230 + 0x3FA + 0x2, &MaxDrawDistanceForNormalObjects, true); // fsub    ds:flt_858FD8
		}
	}

	if (bLoadAllBinaryIPLs)
	{
		injector::MakeCALL(0x5D19A4, LoadAllBinaryIPLs);
	}

	if (bPreloadLODs)
{
		using h53BCAB = injector::function_hooker<0x53BCAB, void()>;
		
		injector::MakeInline<0x5B5295, 0x5B5295 + 8>([](injector::reg_pack& regs)
		{
			regs.ecx = *(uint16_t*)(regs.eax + 0x22);  // mah let's make it unsigned
			regs.edx = *(uint16_t*)(regs.esi + 0x22);
			lods.push_back( (void*)regs.eax );
		});

		injector::make_static_hook<h53BCAB>([](h53BCAB::func_type AfterInit2)
		{
			// Put the id of the lods in another container
			std::vector<uint16_t> lods_id(lods.size());
			std::transform(lods.begin(), lods.end(), lods_id.begin(), [](void* entity)
			{
				return *(uint16_t*)((uintptr_t)(entity) + 0x22);
			});

			// Load all lod models
			std::for_each(lods_id.begin(), std::unique(lods_id.begin(), lods_id.end()), RequestLod);
			LoadAllRequestedModels(false);

			// Instantiate all lod entities RwObject
			if(false)
				std::for_each(lods.begin(), lods.end(), [](void* entity)
				{
					auto rwObject = *(void**)((uintptr_t)(entity) + 0x18);
					if(rwObject == nullptr)
						injector::thiscall<void(void*)>::vtbl<7>(entity);
				});

			return AfterInit2();
		});
	}
}

void CLODLightManager::SA::Init()
{
	CIniReader iniReader("");
	bRenderLodLights = iniReader.ReadInteger("LodLights", "RenderLodLights", 1);
	CoronaRadiusMultiplier = iniReader.ReadFloat("LodLights", "CoronaRadiusMultiplier", 1.0f);
	SlightlyIncreaseRadiusWithDistance = iniReader.ReadInteger("LodLights", "SlightlyIncreaseRadiusWithDistance", 1) != 0;
	if (strncmp(iniReader.ReadString("LodLights", "CoronaFarClip", "auto"), "auto", 4) != 0)
		CoronaFarClip = iniReader.ReadFloat("LodLights", "CoronaFarClip", 0.0f); 
	else
		autoFarClip = true;
	szCustomCoronaTexturePath = iniReader.ReadString("LodLights", "CustomCoronaTexturePath", ".\\corona.png");

	RenderStaticShadowsForLODs = iniReader.ReadInteger("StaticShadows", "RenderStaticShadowsForLODs", 0);
	IncreasePedsCarsShadowsDrawDistance = iniReader.ReadInteger("StaticShadows", "IncreaseCarsShadowsDrawDistance", 0);
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

	MaxDrawDistanceForNormalObjects = 300.0f; //iniReader.ReadFloat("IDETweaker", "MaxDrawDistanceForNormalObjects", 0.0f);
	TimedObjectsDrawDistance = iniReader.ReadFloat("IDETweaker", "TimedObjectsDrawDistance", 0.0f);
	LODObjectsDrawDistance = iniReader.ReadFloat("IDETweaker", "LODObjectsDrawDistance", 0.0f);
	GenericObjectsDrawDistance = iniReader.ReadFloat("IDETweaker", "GenericObjectsDrawDistance", 0.0f);
	AllNormalObjectsDrawDistance = iniReader.ReadFloat("IDETweaker", "AllNormalObjectsDrawDistance", 0.0f);
	VegetationDrawDistance = iniReader.ReadFloat("IDETweaker", "VegetationDrawDistance", 0.0f);
	bLoadAllBinaryIPLs = iniReader.ReadInteger("IDETweaker", "LoadAllBinaryIPLs", 0) == 1;
	bPreloadLODs = iniReader.ReadInteger("IDETweaker", "PreloadLODs", 0) == 1;

	ApplyMemoryPatches();
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		if (injector::address_manager::singleton().IsSA() && injector::address_manager::singleton().IsUS())
		{
			if (injector::address_manager::singleton().GetMajorVersion() == 1 && injector::address_manager::singleton().GetMinorVersion() == 0)
			{
				CLODLightManager::SA::Init();
			}
		}
	}
	return TRUE;
}