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

template<uintptr_t addr>
void CExplosionAddModifiedExplosion()
{
	using printstr_hook = injector::function_hooker<addr, void(DWORD*, DWORD*, int, CVector const&, unsigned int)>;
	injector::make_static_hook<printstr_hook>([](printstr_hook::func_type AddExplosion, DWORD* CEntity, DWORD* CEntity2, int eExplosionType, CVector const& a1, unsigned int a2)
	{
		std::random_shuffle(ExplosionTypes.begin(), ExplosionTypes.end());
		for (auto it = ExplosionTypes.begin(); it != ExplosionTypes.end(); ++it)
		{
			AddExplosion(CEntity, CEntity2, *it, a1, a2);
		}
		return;
	});
}

template<uintptr_t addr>
void CBulletTracesAddTrace()
{
	using printstr_hook = injector::function_hooker<addr, void(CVector *, CVector *)>;
	injector::make_static_hook<printstr_hook>([](printstr_hook::func_type AddTrace, CVector* start, CVector* end)
	{
		injector::MakeNOP(0x563B56, 5);  //    sub_50D140
		injector::MakeNOP(0x563BBB, 5);  //    sub_510790
		injector::MakeNOP(0x563BDD, 5);  //    sub_4AF970
		//injector::MakeNOP(0x563C43, 5);  //    sub_50D140
		injector::MakeNOP(0x563CB6, 5);  //    sub_4CCE20
		injector::MakeNOP(0x563CC1, 5);  //    sub_4E67F0
		injector::MakeNOP(0x563CD4, 5);  //    sub_403620
		injector::MakeNOP(0x563D0C, 5);  //    sub_4EA420
		injector::MakeNOP(0x563D23, 5);  //    sub_474CC0
		injector::MakeNOP(0x563D40, 5);  //    sub_5A41D0
		injector::MakeNOP(0x563D68, 5);  //    sub_5A41D0
		injector::MakeNOP(0x563D90, 5);  //    sub_5A41D0
		injector::MakeNOP(0x563DD4, 5);  //    sub_50D140
		injector::MakeNOP(0x563E0D, 5);  //    sub_551950
		injector::MakeNOP(0x563E37, 5);  //    sub_57C5F0
		injector::MakeNOP(0x563E7E, 5);  //    sub_50D140
		injector::MakeNOP(0x563EA0, 5);  //    sub_57C840
		injector::MakeNOP(0x563EBF, 5);  //    sub_57C840
		injector::MakeNOP(0x563ECA, 5);  //    sub_4E5A10
		injector::MakeNOP(0x563F0B, 5);  //    sub_5552C0
		injector::MakeNOP(0x563F73, 5);  //    sub_50D140
		injector::MakeNOP(0x563F85, 5);  //    sub_57C5F0
		injector::cstd<void(CVector *startPoint, CVector *endPoint, int intensity)>::call<0x563B00>(start, end, 0);
		injector::MakeCALL(0x563B56, 0x50D140, true);
		injector::MakeCALL(0x563BBB, 0x510790, true);
		injector::MakeCALL(0x563BDD, 0x4AF970, true);
		injector::MakeCALL(0x563C43, 0x50D140, true);
		injector::MakeCALL(0x563CB6, 0x4CCE20, true);
		injector::MakeCALL(0x563CC1, 0x4E67F0, true);
		injector::MakeCALL(0x563CD4, 0x403620, true);
		injector::MakeCALL(0x563D0C, 0x4EA420, true);
		injector::MakeCALL(0x563D23, 0x474CC0, true);
		injector::MakeCALL(0x563D40, 0x5A41D0, true);
		injector::MakeCALL(0x563D68, 0x5A41D0, true);
		injector::MakeCALL(0x563D90, 0x5A41D0, true);
		injector::MakeCALL(0x563DD4, 0x50D140, true);
		injector::MakeCALL(0x563E0D, 0x551950, true);
		injector::MakeCALL(0x563E37, 0x57C5F0, true);
		injector::MakeCALL(0x563E7E, 0x50D140, true);
		injector::MakeCALL(0x563EA0, 0x57C840, true);
		injector::MakeCALL(0x563EBF, 0x57C840, true);
		injector::MakeCALL(0x563ECA, 0x4E5A10, true);
		injector::MakeCALL(0x563F0B, 0x5552C0, true);
		injector::MakeCALL(0x563F73, 0x50D140, true);
		injector::MakeCALL(0x563F85, 0x57C5F0, true);
		return;
	});
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
		injector::WriteMemory<float>(0x5F726C, *(float*)0x5F726C * (DrawDistance / 1.8f), true);
		injector::MakeInline<0x486AF2>([](injector::reg_pack&)
		{
			injector::thiscall<void()>::call<0x488CC0>();
			injector::WriteMemory<float>(0x5F726C, *(float*)0x5F726C * (DrawDistance / 1.8f), true);
		});
		injector::MakeInline<0x486B3A>([](injector::reg_pack& regs)
		{
			*(uintptr_t*)regs.esp = 0x486D16;
			injector::WriteMemory<float>(0x5F726C, *(float*)0x5F726C * (DrawDistance / 1.8f), true);
		});
		injector::MakeInline<0x48B314>([](injector::reg_pack& regs)
		{
			*(uintptr_t*)regs.esp = 0x48B42C;
			injector::WriteMemory<float>(0x5F726C, *(float*)0x5F726C * (DrawDistance / 1.8f), true);
		});
		injector::WriteMemory<float>(0x487629 + 6, 1.2f * (DrawDistance / 1.8f), true);
	}

	if (MaxDrawDistanceForNormalObjects)
	{
		//injector::WriteMemory<float>(0x5F72A4, MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4A8AB1, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4A8AC6, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4A8AD9, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4A8B0E, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4A8B21, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4A8B34, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4A8B82, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4A8B97, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4A8BAA, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4A8BDF, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4A8BF2, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4A8C05, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4A8DA6, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4AA391, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4AA3A6, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4AA3B9, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4AA3EE, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4AA401, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4AA414, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4AA462, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4AA477, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4AA48A, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4AA4BF, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4AA4D2, &MaxDrawDistanceForNormalObjects, true);
		injector::WriteMemory(0x4AA4E5, &MaxDrawDistanceForNormalObjects, true);
	}

	if (EnableDrawDistanceChanger)
	{
		injector::MakeJMP(0x48E072, DrawDistanceChanger, true);

		injector::WriteMemory(0x48E5DA + 0x2, &fNewFarClip, true);
		//injector::WriteMemory(0x48E5E6 + 0x2, &fNewFarClip, true);
	}

	if (bRandomExplosionEffects)
	{
		for (int i = 0; i < 10; ++i)
		{
			if (i != 1)
			ExplosionTypes.push_back(i);
		}

		CExplosionAddModifiedExplosion<(0x4309EB)>(); // = 0x5591C0 + 0x0  -> call    AddExplosion__10CExplosionFP7CEntityP7CEntity14eExplosionTypeRC7CVectorUi; CExplosion::AddExplosion((CEntity *,CEntity *,eExplosionType,CVector const &,uint))
		CExplosionAddModifiedExplosion<(0x442E65)>(); // = 0x5591C0 + 0x0  -> call    AddExplosion__10CExplosionFP7CEntityP7CEntity14eExplosionTypeRC7CVectorUi; CExplosion::AddExplosion((CEntity *,CEntity *,eExplosionType,CVector const &,uint))
		CExplosionAddModifiedExplosion<(0x53BF2A)>(); // = 0x5591C0 + 0x0  -> call    AddExplosion__10CExplosionFP7CEntityP7CEntity14eExplosionTypeRC7CVectorUi; CExplosion::AddExplosion((CEntity *,CEntity *,eExplosionType,CVector const &,uint))
		CExplosionAddModifiedExplosion<(0x53DA3C)>(); // = 0x5591C0 + 0x0  -> call    AddExplosion__10CExplosionFP7CEntityP7CEntity14eExplosionTypeRC7CVectorUi; CExplosion::AddExplosion((CEntity *,CEntity *,eExplosionType,CVector const &,uint))
		CExplosionAddModifiedExplosion<(0x541DAB)>(); // = 0x5591C0 + 0x0  -> call    AddExplosion__10CExplosionFP7CEntityP7CEntity14eExplosionTypeRC7CVectorUi; CExplosion::AddExplosion((CEntity *,CEntity *,eExplosionType,CVector const &,uint))
		CExplosionAddModifiedExplosion<(0x549773)>(); // = 0x5591C0 + 0x0  -> call    AddExplosion__10CExplosionFP7CEntityP7CEntity14eExplosionTypeRC7CVectorUi; CExplosion::AddExplosion((CEntity *,CEntity *,eExplosionType,CVector const &,uint))
		CExplosionAddModifiedExplosion<(0x549F90)>(); // = 0x5591C0 + 0x0  -> call    AddExplosion__10CExplosionFP7CEntityP7CEntity14eExplosionTypeRC7CVectorUi; CExplosion::AddExplosion((CEntity *,CEntity *,eExplosionType,CVector const &,uint))
		CExplosionAddModifiedExplosion<(0x54A349)>(); // = 0x5591C0 + 0x0  -> call    AddExplosion__10CExplosionFP7CEntityP7CEntity14eExplosionTypeRC7CVectorUi; CExplosion::AddExplosion((CEntity *,CEntity *,eExplosionType,CVector const &,uint))
		CExplosionAddModifiedExplosion<(0x54C265)>(); // = 0x5591C0 + 0x0  -> call    AddExplosion__10CExplosionFP7CEntityP7CEntity14eExplosionTypeRC7CVectorUi; CExplosion::AddExplosion((CEntity *,CEntity *,eExplosionType,CVector const &,uint))
		CExplosionAddModifiedExplosion<(0x54C6C0)>(); // = 0x5591C0 + 0x0  -> call    AddExplosion__10CExplosionFP7CEntityP7CEntity14eExplosionTypeRC7CVectorUi; CExplosion::AddExplosion((CEntity *,CEntity *,eExplosionType,CVector const &,uint))
		CExplosionAddModifiedExplosion<(0x54C7AD)>(); // = 0x5591C0 + 0x0  -> call    AddExplosion__10CExplosionFP7CEntityP7CEntity14eExplosionTypeRC7CVectorUi; CExplosion::AddExplosion((CEntity *,CEntity *,eExplosionType,CVector const &,uint))
		CExplosionAddModifiedExplosion<(0x54CC04)>(); // = 0x5591C0 + 0x0  -> call    AddExplosion__10CExplosionFP7CEntityP7CEntity14eExplosionTypeRC7CVectorUi; CExplosion::AddExplosion((CEntity *,CEntity *,eExplosionType,CVector const &,uint))
		//CExplosionAddModifiedExplosion<(0x55B743)>(); // = 0x5591C0 + 0x0  -> call    AddExplosion__10CExplosionFP7CEntityP7CEntity14eExplosionTypeRC7CVectorUi; CExplosion::AddExplosion((CEntity *,CEntity *,eExplosionType,CVector const &,uint)) molotov/grenade expl
		CExplosionAddModifiedExplosion<(0x55B7A9)>(); // = 0x5591C0 + 0x0  -> call    AddExplosion__10CExplosionFP7CEntityP7CEntity14eExplosionTypeRC7CVectorUi; CExplosion::AddExplosion((CEntity *,CEntity *,eExplosionType,CVector const &,uint))
		CExplosionAddModifiedExplosion<(0x564ADE)>(); // = 0x5591C0 + 0x0  -> call    AddExplosion__10CExplosionFP7CEntityP7CEntity14eExplosionTypeRC7CVectorUi; CExplosion::AddExplosion((CEntity *,CEntity *,eExplosionType,CVector const &,uint))
	}

	if (bReplaceSmokeTrailWithBulletTrail)
	{
		CBulletTracesAddTrace<(0x55F9C7)>(); // = 0x518E90 + 0x0  -> call    sub_518E90
		CBulletTracesAddTrace<(0x560599)>(); // = 0x518E90 + 0x0  -> call    sub_518E90
		CBulletTracesAddTrace<(0x560F21)>(); // = 0x518E90 + 0x0  -> call    sub_518E90
		CBulletTracesAddTrace<(0x56186B)>(); // = 0x518E90 + 0x0  -> call    sub_518E90
		CBulletTracesAddTrace<(0x562B07)>(); // = 0x518E90 + 0x0  -> call    sub_518E90
		CBulletTracesAddTrace<(0x562E4F)>(); // = 0x518E90 + 0x0  -> call    sub_518E90
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
		RenderSearchlightEffects = 0;
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
		if (bPreloadLODs)
		{
			if (modelID == 404 || modelID == 405 || modelID == 416 || modelID == 402 || modelID == 403)
			{
				if (IDEDrawDistance == 3000)
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

	MaxDrawDistanceForNormalObjects = iniReader.ReadFloat("DistanceLimits", "MaxDrawDistanceForNormalObjects", 0.0f);
	DrawDistance = iniReader.ReadFloat("DistanceLimits", "DrawDistance", 0.0f);
	bPreloadLODs = iniReader.ReadInteger("DistanceLimits", "PreloadLODs", 0) == 1;

	bRandomExplosionEffects = iniReader.ReadInteger("Misc", "RandomExplosionEffects", 0) == 1;
	bReplaceSmokeTrailWithBulletTrail = iniReader.ReadInteger("Misc", "ReplaceSmokeTrailWithBulletTrail", 0) == 1;

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

				CIniReader iniReader("");
				if (bPreloadLODs = iniReader.ReadInteger("DistanceLimits", "PreloadLODs", 0) == 1)
				{
					/*injector::WriteMemory(0x591E0B, &nLevelPortland, true);
					injector::WriteMemory(0x591E16, &nLevelPortland, true);
					injector::WriteMemory(0x591E28, &nLevelPortland, true);
					injector::WriteMemory(0x591E3B, &nLevelPortland, true);*/

					injector::WriteMemory<unsigned char>(0x40A6E6, 0xBD, true); //mov ebp
					injector::WriteMemory(0x40A6E6 + 0x1, nLevelPortland, true);

					//opcode
					injector::MakeNOP(0x5887FD, 5, true);
					injector::MakeNOP(0x588809, 5, true);
					injector::MakeNOP(0x58881A, 5, true);

					injector::WriteMemory(0x4A8F79 + 0x1, &nLevelPortland, true);

					injector::WriteMemory(0x4B61BC, nLevelPortland, true);

					injector::MakeInline<0x40B7DA, 0x40B8F4>([](injector::reg_pack& regs)
					{
						CPopulationDealWithZoneChange(0x8F6250, *(char*)0x941514, 0);
						LoadCollisionFile1(*(char*)0x941514);
						*(DWORD*)0x8F6250 = *(char*)0x941514;
						sub_595BD0();
					});

					injector::MakeNOP(0x4764AF, 5, true); //CFileLoader::LoadMapZones((char const *))
				}
			}
		}
	}
	return TRUE;
}