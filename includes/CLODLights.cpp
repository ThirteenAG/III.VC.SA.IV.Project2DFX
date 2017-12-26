#include "CLODLights.h"
#include "Sprite.h"

std::map<unsigned int, CLODLightsLinkedListNode*>	CLODLights::UsedMap;
CLODLightsLinkedListNode							CLODLights::FreeList, CLODLights::UsedList;
std::vector<CLODLightsLinkedListNode>				CLODLights::aLinkedList;
std::vector<CRegisteredCorona>					    CLODLights::aCoronas;
std::map<unsigned int, CRGBA>	                    CLODLights::FestiveLights;

float* CWeatherFoggyness;
RwTexture** g_TexCoronastar;
void* RwEngineInstance;
extern int numCoronas;
extern void(*_RwRenderStateSet)(RwRenderState nState, void *pParam);
extern CVector* GetCamPos();
void(*CLODLights::RegisterCorona)(unsigned int nID, CEntity *pAttachTo, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, unsigned char coronaType, unsigned char flareType, bool enableReflection, bool checkObstacles, int unused, float normalAngle, bool longDistance, float nearClip, unsigned char bFadeIntensity, float FadeSpeed, bool bOnlyFromBelow, bool reflectionDelay) = CLODLights::RegisterNormalCorona;

void CRegisteredCorona::Update()
{
	if (!RegisteredThisFrame)
	{
		Intensity = 0;
	}
	if (!Intensity && !JustCreated)
	{
		Identifier = 0;
	}
	JustCreated = 0;
	RegisteredThisFrame = 0;
}

void CLODLights::RegisterCoronaMain(unsigned int nID, CEntity* pAttachTo, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, RwTexture* pTex, unsigned char flareType, unsigned char reflectionType, unsigned char LOSCheck, unsigned char unused, float normalAngle, bool bNeonFade, float PullTowardsCam, bool bFadeIntensity, float FadeSpeed, bool bOnlyFromBelow, bool bWhiteCore)
{
	UNREFERENCED_PARAMETER(unused);

	CVector		vecPosToCheck = Position;
	CVector*	pCamPos = GetCamPos();
	if (Range * Range >= (pCamPos->x - vecPosToCheck.x)*(pCamPos->x - vecPosToCheck.x) + (pCamPos->y - vecPosToCheck.y)*(pCamPos->y - vecPosToCheck.y))
	{
		if (bNeonFade)
		{
			float		fDistFromCam = CVector(*pCamPos - vecPosToCheck).Magnitude();

			if (fDistFromCam < 35.0f)
				return;
			if (fDistFromCam < 50.0f)
				A *= static_cast<unsigned char>((fDistFromCam - 35.0f) * (2.0f / 3.0f));
		}

		// Is corona already present?
		CRegisteredCorona*		pSuitableSlot;
		auto it = UsedMap.find(nID);

		if (it != UsedMap.end())
		{
			pSuitableSlot = it->second->GetFrom();

			if (pSuitableSlot->Intensity == 0 && A == 0)
			{
				// Mark as free
				it->second->GetFrom()->Identifier = 0;
				it->second->Add(&FreeList);
				UsedMap.erase(nID);
				return;
			}
		}
		else
		{
			if (!A)
				return;

			// Adding a new element
			auto	pNewEntry = FreeList.First();
			if (!pNewEntry)
			{
				//LogToFile("ERROR: Not enough space for coronas!");
				return;
			}

			pSuitableSlot = pNewEntry->GetFrom();

			// Add to used list and push this index to the map
			pNewEntry->Add(&UsedList);
			UsedMap[nID] = pNewEntry;

			pSuitableSlot->FadedIntensity = A;
			pSuitableSlot->OffScreen = true;
			pSuitableSlot->JustCreated = true;
			pSuitableSlot->Identifier = nID;
		}

		pSuitableSlot->Red = R;
		pSuitableSlot->Green = G;
		pSuitableSlot->Blue = B;
		pSuitableSlot->Intensity = A;
		pSuitableSlot->Coordinates = Position;
		pSuitableSlot->Size = Size;
		pSuitableSlot->NormalAngle = normalAngle;
		pSuitableSlot->Range = Range;
		pSuitableSlot->pTex = pTex;
		pSuitableSlot->FlareType = flareType;
		pSuitableSlot->ReflectionType = reflectionType;
		pSuitableSlot->LOSCheck = LOSCheck;
		pSuitableSlot->RegisteredThisFrame = true;
		pSuitableSlot->PullTowardsCam = PullTowardsCam;
		pSuitableSlot->FadeSpeed = FadeSpeed;

		pSuitableSlot->NeonFade = bNeonFade;
		pSuitableSlot->OnlyFromBelow = bOnlyFromBelow;
		pSuitableSlot->WhiteCore = bWhiteCore;

		pSuitableSlot->bIsAttachedToEntity = false;
		pSuitableSlot->pEntityAttachedTo = nullptr;
	}
}

void CLODLights::RegisterNormalCorona(unsigned int nID, CEntity *pAttachTo, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, unsigned char coronaType, unsigned char flareType, bool enableReflection, bool checkObstacles, int unused, float normalAngle, bool longDistance, float nearClip, unsigned char bFadeIntensity, float FadeSpeed, bool bOnlyFromBelow, bool reflectionDelay)
{
	CLODLights::RegisterCoronaMain(nID, pAttachTo, R, G, B, A, Position, Size, Range, *(g_TexCoronastar + coronaType), flareType, enableReflection, checkObstacles, unused, normalAngle, longDistance, nearClip, bFadeIntensity, FadeSpeed, bOnlyFromBelow, reflectionDelay);
}

void CLODLights::RegisterFestiveCorona(unsigned int nID, CEntity *pAttachTo, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, unsigned char coronaType, unsigned char flareType, bool enableReflection, bool checkObstacles, int unused, float normalAngle, bool longDistance, float nearClip, unsigned char bFadeIntensity, float FadeSpeed, bool bOnlyFromBelow, bool reflectionDelay)
{
	auto it = FestiveLights.find(nID);
	if (it != FestiveLights.end())
	{
		CLODLights::RegisterCoronaMain(nID, pAttachTo, it->second.r, it->second.g, it->second.b, A, Position, Size, Range, *(g_TexCoronastar + coronaType), flareType, enableReflection, checkObstacles, unused, normalAngle, longDistance, nearClip, bFadeIntensity, FadeSpeed, bOnlyFromBelow, reflectionDelay);
	}
	else
	{
		FestiveLights[nID] = CRGBA(random(0, 255), random(0, 255), random(0, 255), 0);

		CLODLights::RegisterCoronaMain(nID, pAttachTo, R, G, B, A, Position, Size, Range, *(g_TexCoronastar + coronaType), flareType, enableReflection, checkObstacles, unused, normalAngle, longDistance, nearClip, bFadeIntensity, FadeSpeed, bOnlyFromBelow, reflectionDelay);
	}
}

void CLODLights::Update()
{
	auto pNode = UsedList.First();
	if (pNode)
	{
		while (pNode != &UsedList)
		{
			unsigned int	nIndex = pNode->GetFrom()->Identifier;
			auto			pNext = pNode->GetNextNode();

			pNode->GetFrom()->Update();

			// Did it become invalid?
			if (!pNode->GetFrom()->Identifier)
			{
				// Remove from used list
				pNode->Add(&FreeList);
				UsedMap.erase(nIndex);
			}

			pNode = pNext;
		}
	}
}

void CLODLights::Init()
{
	if (aCoronas.size() != numCoronas)
	{
		aLinkedList.resize(numCoronas);
		aCoronas.resize(numCoronas);

		// Initialise the lists
		FreeList.Init();
		UsedList.Init();

		for (size_t i = 0; i < aLinkedList.size(); i++)
		{
			aLinkedList[i].Add(&FreeList);
			aLinkedList[i].SetEntry(&aCoronas[i]);
		}
	}
}

void CLODLights::RenderBuffered()
{
	int		nWidth = RwRasterGetWidth(RwCameraGetRaster(Camera));
	int		nHeight = RwRasterGetHeight(RwCameraGetRaster(Camera));

	RwRaster*	pLastRaster = nullptr;
	bool		bLastZWriteRenderState = true;

	_RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);
	_RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
	_RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	_RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	_RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);

	for (size_t i = 0; i < aCoronas.size(); i++)
	{
		if (aCoronas[i].Identifier)
		{
			if (aCoronas[i].Intensity > 0)
			{
				RwV3d	vecCoronaCoords, vecTransformedCoords;
				float	fComputedWidth, fComputedHeight;

				vecCoronaCoords.x = aCoronas[i].Coordinates.x;
				vecCoronaCoords.y = aCoronas[i].Coordinates.y;
				vecCoronaCoords.z = aCoronas[i].Coordinates.z;

				if (CSprite::CalcScreenCoors(vecCoronaCoords, &vecTransformedCoords, &fComputedWidth, &fComputedHeight, true, true))
				{
					aCoronas[i].OffScreen = !(vecTransformedCoords.x >= 0.0 && vecTransformedCoords.x <= nWidth &&
						vecTransformedCoords.y >= 0.0 && vecTransformedCoords.y <= nHeight);

					if (aCoronas[i].Intensity > 0 && vecTransformedCoords.z <= aCoronas[i].Range)
					{
						float	fInvFarClip = 1.0f / vecTransformedCoords.z;
						float	fHalfRange = aCoronas[i].Range * 0.5f;

						short	nFadeIntensity = (short)(aCoronas[i].Intensity * (vecTransformedCoords.z > fHalfRange ? 1.0f - (vecTransformedCoords.z - fHalfRange) / fHalfRange : 1.0f));

						if (bLastZWriteRenderState != aCoronas[i].LOSCheck == false)
						{
							bLastZWriteRenderState = aCoronas[i].LOSCheck == false;
							CSprite::FlushSpriteBuffer();

							_RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)bLastZWriteRenderState);
						}

						if (aCoronas[i].pTex)
						{
							float	fColourFogMult = Min(40.0f, vecTransformedCoords.z) * *CWeatherFoggyness * 0.025f + 1.0f;

							if (aCoronas[i].Identifier == 1)	// Sun core
								vecTransformedCoords.z = RwCameraGetFarClipPlane(Camera) * 0.95f;

							// This R* tweak broke the sun
							//_RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
							if (pLastRaster != RwTextureGetRaster(aCoronas[i].pTex))
							{
								pLastRaster = RwTextureGetRaster(aCoronas[i].pTex);
								CSprite::FlushSpriteBuffer();

								_RwRenderStateSet(rwRENDERSTATETEXTURERASTER, pLastRaster);
							}

							RwV3d		vecCoronaCoordsAfterPull = vecCoronaCoords;
							CVector		vecTempVector(vecCoronaCoordsAfterPull);
							vecTempVector -= *GetCamPos();
							vecTempVector.Normalise();

							vecCoronaCoordsAfterPull.x -= (vecTempVector.x * aCoronas[i].PullTowardsCam);
							vecCoronaCoordsAfterPull.y -= (vecTempVector.y * aCoronas[i].PullTowardsCam);
							vecCoronaCoordsAfterPull.z -= (vecTempVector.z * aCoronas[i].PullTowardsCam);

							if (CSprite::CalcScreenCoors(vecCoronaCoordsAfterPull, &vecTransformedCoords, &fComputedWidth, &fComputedHeight, true, true))
							{
								CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect(vecTransformedCoords.x, vecTransformedCoords.y, vecTransformedCoords.z, aCoronas[i].Size * fComputedWidth, aCoronas[i].Size * fComputedHeight * fColourFogMult, aCoronas[i].Red / fColourFogMult, aCoronas[i].Green / fColourFogMult, aCoronas[i].Blue / fColourFogMult, nFadeIntensity, fInvFarClip * 20.0f, 0.0, 0xFF);
							}
						}
					}
				}
				else
					aCoronas[i].OffScreen = true;
			}
		}
	}

	CSprite::FlushSpriteBuffer();
}

void CLODLights::Inject()
{
	auto& gvm = injector::address_manager::singleton();
	if (gvm.IsSA())
	{
		CWeatherFoggyness = (float*)0xC81300;
		g_TexCoronastar = (RwTexture**)0xC3E000;

		CSprite::CalcScreenCoors = &CSprite::CalcScreenCoorsSA;
		CSprite::CalcAddr = 0x70CE30;
		CSprite::FlushAddr = 0x70CF20;
		CSprite::RenderOneAddr = 0x70D490;
		CSprite::RenderBufferedAddr = 0x70E780;

		injector::MakeCALL(0x5BA36D, CLODLights::Init, true);
	}
	else
	{
		if (gvm.IsVC())
		{
			CWeatherFoggyness = (float*)0x94DDC0;
			g_TexCoronastar = (RwTexture**)0x695538;

			CSprite::CalcScreenCoors = &CSprite::CalcScreenCoorsIIIVC;
			CSprite::CalcAddr = 0x5778B0;
			CSprite::FlushAddr = 0x577790;
			CSprite::RenderOneAddr = 0x576690;
			CSprite::RenderBufferedAddr = 0x576B30;

			struct Init
			{
				void operator()(injector::reg_pack&)
				{
					((void(__cdecl *)())0x542830)(); //CCoronas::Init();
					CLODLights::Init();
				}
			}; injector::MakeInline<Init>(0x4A4E55);
		}
		else
		{
			if (gvm.IsIII())
			{
				CWeatherFoggyness = (float*)0x885AF4;
				g_TexCoronastar = (RwTexture**)0x5FAF44;

				CSprite::CalcScreenCoors = &CSprite::CalcScreenCoorsIIIVC;
				CSprite::CalcAddr = 0x51C3A0;
				CSprite::FlushAddr = 0x51C520;
				CSprite::RenderOneAddr = 0x51D110;
				CSprite::RenderBufferedAddr = 0x51CCD0;

				struct Init
				{
					void operator()(injector::reg_pack&)
					{
						((void(__cdecl *)())0x4F9F90)(); //CCoronas::Init();
						CLODLights::Init();
					}
				}; injector::MakeInline<Init>(0x48C22A);
			}
		}
	}
}