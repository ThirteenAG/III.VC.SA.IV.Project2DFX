#pragma once
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include "General.h"
#include "Camera.h"
#include "..\includes\injector\injector.hpp"
#include "..\includes\injector\hooking.hpp"
#include "..\includes\injector\calling.hpp"
#include "..\includes\injector\assembly.hpp"
#include "IniReader.h"

int bRenderLodLights, AdaptiveDrawDistanceEnabled;
float CoronaRadiusMultiplier;
float CoronaFarClip;
float DrawDistance;
float MaxDrawDistanceForNormalObjects;
float TimedObjectsDrawDistance;
float LODObjectsDrawDistance;
float GenericObjectsDrawDistance;
float AllNormalObjectsDrawDistance;
float VegetationDrawDistance;
bool bLoadAllBinaryIPLs, bPreloadLODs;
int RenderStaticShadowsForLODs, IncreasePedsCarsShadowsDrawDistance;
float TrafficLightsShadowsIntensity, TrafficLightsShadowsDrawDistance;
float StaticShadowsIntensity, StaticShadowsDrawDistance;
char* szCustomCoronaTexturePath;
int nStaticShadowsIntensity;
DWORD nCoronasLimit;
uintptr_t hookJmpAddr, hookJmpAddr2;
int currentCamera;
int RenderSearchlightEffects, SmoothEffect;
float fOne = 1.0f;
float fSearchlightEffectVisibilityFactor;
float fSpotlightColorFactor = 255.0f;
int EnableDrawDistanceChanger;
float MinDrawDistanceOnTheGround, Factor1, Factor2, StaticSunSize;
float fNewFarClip, MaxPossibleDrawDistance;
bool autoFarClip, SlightlyIncreaseRadiusWithDistance;
int MinFPSValue, MaxFPSValue;

__int16 TempBufferRenderIndexList[4096];
RxObjSpace3dVertex TempVertexBuffer[500];
int TempBufferIndicesStored = 0;
unsigned int TempBufferVerticesStored = 0;

typedef struct { float X, Y, Z; } Vector3;
#define RwV3D RwV3d
float GetDistance(RwV3D *v1, RwV3D *v2);

struct WplInstance
{
	float PositionX;
	float PositionY;
	float PositionZ;
	float RotationX;
	float RotationY;
	float RotationZ;
	float RotationW;
	unsigned int ModelNameHash;
	unsigned int Unknown1;
	signed int LODIndex;
	unsigned int Unknown2;
	float Unknown3;
};

class CLamppostInfo
{
public:
	CVector			vecPos;
	CRGBA			colour;
	float			fCustomSizeMult;
	int 			nNoDistance;
	int             nDrawSearchlight;

public:
	CLamppostInfo(const CVector& pos, const CRGBA& col, float fCustomMult, int nNoDistance, int nDrawSearchlight)
		: vecPos(pos), colour(col), fCustomSizeMult(fCustomMult), nNoDistance(nNoDistance), nDrawSearchlight(nDrawSearchlight)
	{}
};

class CLODLightManager
{
private:
	static std::vector<const CLamppostInfo>*	          m_pLampposts;
	static std::map<unsigned int, const CLamppostInfo>*	  pFileContent;
	static bool							                  m_bCatchLamppostsNow;

	static bool							IsModelALamppost(unsigned short nModel);

	static inline bool					LampsRegisteringOn()
	{
		return m_bCatchLamppostsNow;
	}
	static inline void					EndRegistering()
	{
		m_bCatchLamppostsNow = false; m_pLampposts->shrink_to_fit();
	}
	static inline unsigned int	        PackKey(unsigned short nModel, unsigned short nIndex)
	{
		return nModel << 16 | nIndex;
	}

public:
	class III;
	class VC;
	class SA;
	class IV;
	class EFLC;
};

class CLODLightManager::III : public CLODLightManager
{
private:
	static char* CurrentTimeHours;
	static char* CurrentTimeMinutes;
	static float** fCurrentFarClip;
	static char* DataFilePath;
	static std::vector<const CEntityIII> VecEntities;

public:
	static void							Init();
	static void                         GenericIDEHook();
	static void                         IPLDataHook1();
	static void                         IPLDataHook2();
	static void                         LoadDatFile();

private:
	static void 						ApplyMemoryPatches();
	static void 						RegisterCustomCoronas();
	static void 						RegisterAllLampposts();
	static void                         RenderSearchLights();
	static void                         RenderHeliSearchLights();
	static void 						RegisterLamppost(CEntityIII* entity);
	static void                         DrawDistanceChanger();
	static void 						RenderLODLights();

	static int*                         (__cdecl *GetModelInfo)(const char*, int*);
	static char                         (__cdecl *GetIsTimeInRange)(char hourA, char hourB);
	static bool                         (__cdecl *const CShadowsStoreStaticShadow)(unsigned int id, unsigned char type, RwTexture *particle, CVector *pos, float x1, float y1, float x2, float y2, short alpha, unsigned char red, unsigned char green, unsigned char blue, float, float, float drawdist, bool lifetime, float updist);
	static float                        (__cdecl *FindGroundZFor3DCoord)(float x, float y, float z, BOOL *pCollisionResult, CEntity **pGroundObject);
	static RwV3D*                       (__cdecl *TransformPoint)(struct RwV3D *, struct RwMatrix *, struct RwV3D *);
	static bool                         (__thiscall *CCameraIsSphereVisible)(int *camera, RwV3D *origin, float radius);
	static void                         (__cdecl *RegisterCorona)(int coronaID, char r, char g, char b, char a, RwV3D *pos, float size, float farClip, char type, char flare, char reflection, char obstacles, char notUsed, float normalAngle);
	static void                         (__cdecl drawCustomSpotLight)(RwV3D StartPoint, RwV3D EndPoint, float TargetRadius, float baseRadius, float slColorFactor1, char slColorFactor2, float slAlpha = 1.0f);
	static void                         (__fastcall *CVectorNormalize)(RwV3D *in);
	static RwV3D*                       (__cdecl *CrossProduct)(RwV3D *out, RwV3D *a, RwV3D *b);
	static int                          (__cdecl *RwIm3DTransform)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags);
	static int                          (__cdecl *RwIm3DRenderIndexedPrimitive)(int primType, __int16 *indices, int numIndices);
	static int                          (__cdecl *RwIm3DEnd)();
	static int                          (__cdecl *RwRenderStateSet)(int a1, int a2);

	static inline RwV3D * GetCamPos()
	{
		return (RwV3D *)(0x006FACF8 + 0x770);
	}
	static inline unsigned short        GetModelInfoUInt16(const char* pName)
	{
		int nID; GetModelInfo(pName, &nID);	return static_cast<unsigned short>(nID);
	}
};

class CLODLightManager::VC : public CLODLightManager
{
private:
	static char* CurrentTimeHours;
	static char* CurrentTimeMinutes;
	static float** fCurrentFarClip;
	static char* DataFilePath;
	static std::vector<const CEntityVC> VecEntities;

public:
	static void							Init();
	static void                         GenericIDEHook();
	static void                         IPLDataHook1();
	static void                         IPLDataHook2();
	static void                         LoadDatFile();

private:
	static void 						ApplyMemoryPatches();
	static void 						FillIplStringsArray();
	static void 						RegisterCustomCoronas();
	static void 						RegisterAllLampposts();
	static void                         RenderSearchLights();
	static void                         RenderHeliSearchLights();
	static void 						RegisterLamppost(CEntityVC* entity);
	static void                         DrawDistanceChanger();
	static void 						RenderLODLights();

	static int*                         (__cdecl *GetModelInfo)(const char*, int*);
	static char                         (__cdecl *GetIsTimeInRange)(char hourA, char hourB);
public:
	static bool                         (__cdecl *const CShadowsStoreStaticShadow)(unsigned int id, unsigned char type, RwTexture *particle, CVector *pos, float x1, float y1, float x2, float y2, short alpha, unsigned char red, unsigned char green, unsigned char blue, float, float, float drawdist, bool lifetime, float updist);
private:
	static float                        (__cdecl *FindGroundZFor3DCoord)(float x, float y, float z, BOOL *pCollisionResult, CEntity **pGroundObject);
	static RwV3D*                       (__cdecl *TransformPoint)(RwV3d *a1, RwV3d *a2, int a3, RwMatrix *a4);
	static bool                         CCameraIsSphereVisible(RwV3D *origin, float radius);
	static void                         (__cdecl *RegisterCorona)(int id, char r, char g, char b, char alpha, RwV3D *pos, float radius, float farClp, char a9, char lensflare, char a11, char see_through_effect, char trace, float a14, char a15, float a16);
	static void                         (__cdecl drawCustomSpotLight)(RwV3D StartPoint, RwV3D EndPoint, float TargetRadius, float baseRadius, float slColorFactor1, char slColorFactor2, float slAlpha = 1.0f);
	static void                         (__fastcall *CVectorNormalize)(RwV3D *in);
	static RwV3D*                       (__cdecl *CrossProduct)(RwV3D *out, RwV3D *a, RwV3D *b);
	static int                          (__cdecl *RwIm3DTransform)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags);
	static int                          (__cdecl *RwIm3DRenderIndexedPrimitive)(int primType, __int16 *indices, int numIndices);
	static int                          (__cdecl *RwIm3DEnd)();
	static int                          (__cdecl *RwRenderStateSet)(int a1, int a2);

	static inline RwV3D * GetCamPos()
	{
		return (RwV3D *)(0x7E4688 + 0x7D8);
	}
	static inline unsigned short        GetModelInfoUInt16(const char* pName)
	{
		int nID; GetModelInfo(pName, &nID);	return static_cast<unsigned short>(nID);
	}
};

class CLODLightManager::SA : public CLODLightManager
{
private:
	static char* CurrentTimeHours;
	static char* CurrentTimeMinutes;
	static float** fCurrentFarClip;
	static char* DataFilePath;
	static CCamera& TheCamera;
	static int* ActiveInterior;

public:
	static void							Init();
	static void                         LoadDatFile();

private:
	static void							RegisterLamppost(CEntity* pObj);
	static void							RegisterCustomCoronas();
	static CEntity*						PossiblyAddThisEntity(CEntity* pEntity);
	static void							RenderShadows();
	static void							StoreCustomStaticShadows();
	static void                         RenderSearchLights();
	static bool                         IsModelALamppostNotTrafficLight(unsigned short nModelIndex);
	static void                         DrawDistanceChanger();
	static void 						ApplyMemoryPatches();
	static void							RenderLODLights();

	static char                         (__cdecl *GetIsTimeInRange)(char hourA, char hourB);
	static int*                         (__cdecl *GetModelInfo)(const char*, int*);
	static void                         (__cdecl *const CShadowsUpdateStaticShadows)();
	static bool                         (__cdecl *const CShadowsStoreStaticShadow)(unsigned int id, unsigned char type, RwTexture *particle, CVector *pos, float x1, float y1, float x2, float y2, short alpha, unsigned char red, unsigned char green, unsigned char blue, float, float, float drawdist, bool lifetime, float updist);
	static float                        (__cdecl *FindGroundZFor3DCoord)(float x, float y, float z, BOOL *pCollisionResult, CEntity **pGroundObject);
	static RwV3D*                       (__cdecl *TransformPoint)(RwV3D *outPoint, CMatrix *m, RwV3D *point);
	static bool                         (__thiscall *CCameraIsSphereVisible)(int *camera, RwV3d *origin, float radius);
	static void                         (__cdecl *RegisterCorona)(int id, int *vehicle, char r, char g, char b, char alpha, RwV3d *pos, float radius, float farClip, char type, char flareType, char enableReflection, char checkObstacles, int notUsed, float normalAngle, char longDistance, float nearClip, char someFadeFlag, float flashInertia, char onlyFromBelow, char flag);

	static C2dfx*                       (__fastcall *CLODLightManager::SA::Get2dfx)(CBaseModelInfo *model, int edx0, int number);
	static void                         (__cdecl *CLODLightManager::SA::drawSpotLight)(int coronaIndex, float StartX, float StartY, float StartZ, float EndX, float EndY, float EndZ, float TargetRadius, float intensity, char flag1, char drawShadow, RwV3D *pVec1, RwV3D *pVec2, RwV3D *pVec3, char unkTrue, float BaseRadius);
	static bool                         (__thiscall *CLODLightManager::SA::CObjectIsDamaged)(CObject *pclObject);
	static void                         (__cdecl drawCustomSpotLight)(RwV3D StartPoint, RwV3D EndPoint, float TargetRadius, float baseRadius, float slColorFactor1, char slColorFactor2, float slAlpha = 1.0f);
	static void                         (__fastcall *CVectorNormalize)(RwV3D *in);
	static RwV3D*                       (__cdecl *CrossProduct)(RwV3D *out, RwV3D *a, RwV3D *b);
	static int                          (__cdecl *RwIm3DTransform)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags);
	static int                          (__cdecl *RwIm3DRenderIndexedPrimitive)(int primType, __int16 *indices, int numIndices);
	static int                          (__cdecl *RwIm3DEnd)();
public:
	static inline void Shutdown()
	{
		delete m_pLampposts;
	}
	static inline unsigned short        GetModelInfoUInt16(const char* pName)
	{
		int nID; GetModelInfo(pName, &nID);	return static_cast<unsigned short>(nID);
	}
};

class CLODLightManager::IV : public CLODLightManager
{
private:
	static char* CurrentTimeHours;
	static char* CurrentTimeMinutes;
	static char* DataFilePath;

	public:
	static bool						IsModelALamppost(unsigned int nModel);

	static int                      (__cdecl *DrawCorona)(float x, float y, float z, float radius, unsigned int unk, float unk2, unsigned char r, unsigned char g, unsigned char b);
	static void                     (__stdcall *GetRootCam)(int *camera);
	static void                     (__stdcall *GetGameCam)(int *camera);
	static bool                     (__cdecl *CamIsSphereVisible)(int camera, float pX, float pY, float pZ, float radius);
	static void                     (__cdecl *GetCamPos)(int camera, float *pX, float *pY, float *pZ);

public:
	static void                     Init();

private:
	static void                     GetMemoryAddresses();
	static void                     IncreaseCoronaLimit();
	static void                     LoadDatFile();
	static void                     RenderLODLights();
	static void                     LoadObjectInstanceHook();

public:
	static inline unsigned int PackKey(unsigned int nModel, unsigned short nIndex)
	{
		return nModel | nIndex;
	}

	static inline unsigned int GetHashKey(char *str, unsigned int a2)
	{
		unsigned int v2;
		char *temp;
		char i;

		v2 = a2;
		temp = str;
		if (*str == '"')
			temp = str + 1;
		for (i = *temp; *temp; i = *temp)
		{
			if (*str == '"' && i == '"')
				break;
			++temp;
			if ((unsigned __int8)(i - 65) > 25)
			{
				if (i == '\\')
					i = '/';
			}
			else
				i += 32;
			v2 = (1025 * (v2 + i) >> 6) ^ 1025 * (v2 + i);
		}
		return 32769 * (9 * v2 ^ (9 * v2 >> 11));
	}

	static inline void RegisterLamppost(WplInstance* pObj)
	{
		DWORD       		nModelID = pObj->ModelNameHash;
		CMatrix				dummyMatrix;
		CSimpleTransform	objTransform;

		objTransform.m_translate.x = pObj->PositionX;
		objTransform.m_translate.y = pObj->PositionY;
		objTransform.m_translate.z = pObj->PositionZ;

		if (objTransform.m_translate.x == 0.0f && objTransform.m_translate.y == 0.0f)
			return;

		if (GetDistance((RwV3d*)&objTransform.m_translate, (RwV3d*)&CVector(-278.37f, -1377.48f, 90.98f)) <= 300.0f)
			return;

		dummyMatrix.SetTranslateOnly(objTransform.m_translate.x, objTransform.m_translate.y, objTransform.m_translate.z);
		dummyMatrix.SetRotateZOnly(objTransform.m_heading);

		auto	itEnd = pFileContent->upper_bound(PackKey(nModelID, 0xFFFF));
		for (auto it = pFileContent->lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
			m_pLampposts->push_back(CLamppostInfo(dummyMatrix * it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nNoDistance, it->second.nDrawSearchlight));
	}

	static inline void RegisterCustomCoronas()
	{
		DWORD		nModelID = 4294967294;

		auto	itEnd = pFileContent->upper_bound(PackKey(nModelID, 0xFFFF));
		for (auto it = pFileContent->lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
			m_pLampposts->push_back(CLamppostInfo(it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nNoDistance, it->second.nDrawSearchlight));
	}

	static inline WplInstance* PossiblyAddThisEntity(WplInstance* pInstance)
	{
		if (m_bCatchLamppostsNow && IsModelALamppost(pInstance->ModelNameHash))
			RegisterLamppost(pInstance);

		return pInstance;
	}
};

class CLODLightManager::EFLC : public CLODLightManager::IV
{
public:
	static char* CurrentTimeHours;
	static char* CurrentTimeMinutes;
	static char* DataFilePath;

	static int                      (__cdecl *DrawCorona)(float x, float y, float z, float radius, unsigned int unk, float unk2, unsigned char r, unsigned char g, unsigned char b);
	static void                     (__stdcall *GetRootCam)(int *camera);
	static void                     (__stdcall *GetGameCam)(int *camera);
	static bool                     (__cdecl *CamIsSphereVisible)(int camera, float pX, float pY, float pZ, float radius);
	static void                     (__cdecl *GetCamPos)(int camera, float *pX, float *pY, float *pZ);

	static void                     RenderLODLights();
};

std::map<unsigned int, const CLamppostInfo>*	 CLODLightManager::pFileContent;
std::vector<const CLamppostInfo>*	             CLODLightManager::m_pLampposts = nullptr;
std::vector<const CEntityVC>                     CLODLightManager::VC::VecEntities;
std::vector<const CEntityIII>                    CLODLightManager::III::VecEntities;
bool						                     CLODLightManager::m_bCatchLamppostsNow = false;

bool CLODLightManager::IsModelALamppost(unsigned short nModel)
{
	auto	it = pFileContent->lower_bound(PackKey(nModel, 0));
	return it != pFileContent->end() && it->first >> 16 == nModel;
}

bool CLODLightManager::IV::IsModelALamppost(unsigned int nModel)
{
	auto	it = pFileContent->lower_bound(PackKey(nModel, 0));
	return it != pFileContent->end() && it->first == nModel;
}

char* CLODLightManager::III::CurrentTimeHours = (char*)0x95CDA6;
char* CLODLightManager::III::CurrentTimeMinutes = (char*)0x95CDC8;
float** CLODLightManager::III::fCurrentFarClip = (float**)0x48E5DC;
char* CLODLightManager::III::DataFilePath;
char(__cdecl *CLODLightManager::III::GetIsTimeInRange)(char hourA, char hourB) = (char(__cdecl *)(char, char)) 0x473420;
int* (__cdecl *CLODLightManager::III::GetModelInfo)(const char*, int*) = (int*(__cdecl *)(const char*, int*)) 0x50B860;
bool(__cdecl *const CLODLightManager::III::CShadowsStoreStaticShadow)(unsigned int id, unsigned char type, RwTexture *particle, CVector *pos, float x1, float y1, float x2, float y2, short alpha, unsigned char red, unsigned char green, unsigned char blue, float, float, float drawdist, bool lifetime, float updist) = (decltype(CLODLightManager::III::CShadowsStoreStaticShadow))0x5130A0;
float(__cdecl *CLODLightManager::III::FindGroundZFor3DCoord)(float x, float y, float z, BOOL *pCollisionResult, CEntity **pGroundObject) = (float(__cdecl *)(float, float, float, BOOL *, CEntity **)) 0x4B3AE0;
RwV3D* (__cdecl *CLODLightManager::III::TransformPoint)(struct RwV3D *, struct RwMatrix *, struct RwV3D *) = (struct RwV3D *(__cdecl *)(struct RwV3D *, struct RwMatrix *, struct RwV3D *)) 0x4BA4D0;
bool(__thiscall *CLODLightManager::III::CCameraIsSphereVisible)(int *camera, RwV3D *origin, float radius) = (bool(__thiscall *)(int *, RwV3D *, float)) 0x0043D3B0;
void(__cdecl *CLODLightManager::III::RegisterCorona)(int coronaID, char r, char g, char b, char a, RwV3D *pos, float size, float farClip, char type, char flare, char reflection, char obstacles, char notUsed, float normalAngle) = (void(__cdecl *)(int, char, char, char, char, RwV3D *, float, float, char, char, char, char, char, float)) 0x4FA080;
void(__fastcall *CLODLightManager::III::CVectorNormalize)(RwV3D *in) = (void(__fastcall *)(RwV3D *in)) 0x4BA560;
RwV3D *(__cdecl *CLODLightManager::III::CrossProduct)(RwV3D *out, RwV3D *a, RwV3D *b) = (RwV3D *(__cdecl *)(RwV3D *out, RwV3D *a, RwV3D *b)) 0x4BA350;
int(__cdecl *CLODLightManager::III::RwIm3DTransform)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags) = (int(__cdecl *)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags)) 0x5B6720;
int(__cdecl *CLODLightManager::III::RwIm3DRenderIndexedPrimitive)(int primType, __int16 *indices, int numIndices) = (int(__cdecl *)(int primType, __int16 *indices, int numIndices)) 0x5B6820;
int(__cdecl *CLODLightManager::III::RwIm3DEnd)() = (int(__cdecl *)()) 0x5B67F0;
int(__cdecl *CLODLightManager::III::RwRenderStateSet)(int a1, int a2) = (int(__cdecl *)(int a1, int a2)) 0x5A43C0;


char* CLODLightManager::VC::CurrentTimeHours = (char*)0xA10B6B;
char* CLODLightManager::VC::CurrentTimeMinutes = (char*)0xA10B92;
float** CLODLightManager::VC::fCurrentFarClip = (float**)0x4A602D;
char* CLODLightManager::VC::DataFilePath;
char (__cdecl *CLODLightManager::VC::GetIsTimeInRange)(char hourA, char hourB) = (char(__cdecl *)(char, char)) 0x4870F0;
int* (__cdecl *CLODLightManager::VC::GetModelInfo)(const char*, int*) = (int*(__cdecl *)(const char*, int*)) 0x55F7D0;
bool(__cdecl *const CLODLightManager::VC::CShadowsStoreStaticShadow)(unsigned int id, unsigned char type, RwTexture *particle, CVector *pos, float x1, float y1, float x2, float y2, short alpha, unsigned char red, unsigned char green, unsigned char blue, float, float, float drawdist, bool lifetime, float updist) = (decltype(CLODLightManager::VC::CShadowsStoreStaticShadow))0x56E780;
float(__cdecl *CLODLightManager::VC::FindGroundZFor3DCoord)(float x, float y, float z, BOOL *pCollisionResult, CEntity **pGroundObject) = (float(__cdecl *)(float, float, float, BOOL *, CEntity **)) 0x4D53A0;
RwV3D* (__cdecl *CLODLightManager::VC::TransformPoint)(RwV3d *a1, RwV3d *a2, int a3, RwMatrix *a4) = (struct RwV3D *(__cdecl *)(RwV3d *a1, RwV3d *a2, int a3, RwMatrix *a4)) 0x647160;
bool CLODLightManager::VC::CCameraIsSphereVisible(RwV3D *origin, float radius)
{
	RwV3D SpherePos;
	SpherePos.x = origin->x;
	SpherePos.y = origin->y;
	SpherePos.z = origin->z;
	if (origin->z <= *(float *)0x688640)
		SpherePos.z = FindGroundZFor3DCoord(origin->x, origin->y, origin->z, false, false);
	TransformPoint(&SpherePos, &SpherePos, 1, (RwMatrix*)0x7E4EA8);

	if (SpherePos.y + radius >= *(float *)0x978534)
	{
		if (SpherePos.y - radius <= *(float *)0xA10678)
		{
			if (SpherePos.y * *(float *)0x7E4F44 + SpherePos.x * *(float *)0x7E4F40 <= radius)
			{
				if (SpherePos.y * *(float *)0x7E4F50 + SpherePos.x * *(float *)0x7E4F4C <= radius)
				{
					if (SpherePos.z * *(float *)0x7E4F60 + SpherePos.y * *(float *)0x7E4F5C <= radius)
					{
						if (SpherePos.z * *(float *)0x7E4F6C + SpherePos.y * *(float *)0x7E4F68 <= radius)
							return true;
					}
				}
			}
		}
	}
	return false;
}
void(__cdecl *CLODLightManager::VC::RegisterCorona)(int id, char r, char g, char b, char alpha, RwV3D *pos, float radius, float farClp, char a9, char lensflare, char a11, char see_through_effect, char trace, float a14, char a15, float a16) = (void(__cdecl *)(int id, char r, char g, char b, char alpha, RwV3D *pos, float radius, float farClp, char a9, char lensflare, char a11, char see_through_effect, char trace, float a14, char a15, float a16)) 0x5427A0;
void(__fastcall *CLODLightManager::VC::CVectorNormalize)(RwV3D *in) = (void(__fastcall *)(RwV3D *in)) 0x4DFEA0;
RwV3D *(__cdecl *CLODLightManager::VC::CrossProduct)(RwV3D *out, RwV3D *a, RwV3D *b) = (RwV3D *(__cdecl *)(RwV3D *out, RwV3D *a, RwV3D *b)) 0x4E00B0;
int(__cdecl *CLODLightManager::VC::RwIm3DTransform)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags) = (int(__cdecl *)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags)) 0x65AE90;
int(__cdecl *CLODLightManager::VC::RwIm3DRenderIndexedPrimitive)(int primType, __int16 *indices, int numIndices) = (int(__cdecl *)(int primType, __int16 *indices, int numIndices)) 0x65AF90;
int(__cdecl *CLODLightManager::VC::RwIm3DEnd)() = (int(__cdecl *)()) 0x65AF60;
int(__cdecl *CLODLightManager::VC::RwRenderStateSet)(int a1, int a2) = (int(__cdecl *)(int a1, int a2)) 0x649BA0;



char* CLODLightManager::SA::CurrentTimeHours = (char*)0xB70153;
char* CLODLightManager::SA::CurrentTimeMinutes = (char*)0xB70152;
float** CLODLightManager::SA::fCurrentFarClip = (float**)0x53EA95;
char* CLODLightManager::SA::DataFilePath;
CCamera&     CLODLightManager::SA::TheCamera = *(CCamera*)0xB6F028;
int* CLODLightManager::SA::ActiveInterior = (int*)0xB72914;
char(__cdecl *CLODLightManager::SA::GetIsTimeInRange)(char hourA, char hourB) = (char(__cdecl *)(char, char)) 0x52CEE0;
int* (__cdecl *CLODLightManager::SA::GetModelInfo)(const char*, int*) = (int*(__cdecl *)(const char*, int*)) 0x4C5940;
void(__cdecl *const CLODLightManager::SA::CShadowsUpdateStaticShadows)() = (decltype(CShadowsUpdateStaticShadows))0x707F40;
bool(__cdecl *const CLODLightManager::SA::CShadowsStoreStaticShadow)(unsigned int id, unsigned char type, RwTexture *particle, CVector *pos, float x1, float y1, float x2, float y2, short alpha, unsigned char red, unsigned char green, unsigned char blue, float, float, float drawdist, bool lifetime, float updist) = (decltype(CShadowsStoreStaticShadow))0x70BA00;
float(__cdecl *CLODLightManager::SA::FindGroundZFor3DCoord)(float x, float y, float z, BOOL *pCollisionResult, CEntity **pGroundObject) = (float(__cdecl *)(float, float, float, BOOL *, CEntity **)) 0x5696C0;
RwV3D *(__cdecl *CLODLightManager::SA::TransformPoint)(RwV3D *outPoint, CMatrix *m, RwV3D *point) = (RwV3D *(__cdecl *)(RwV3D *, CMatrix *, RwV3D *)) 0x59C890;
bool(__thiscall *CLODLightManager::SA::CCameraIsSphereVisible)(int *camera, RwV3d *origin, float radius) = (bool(__thiscall *)(int *, RwV3d *, float)) 0x00420D40;
void(__cdecl *CLODLightManager::SA::RegisterCorona)(int id, int *vehicle, char r, char g, char b, char alpha, RwV3d *pos, float radius, float farClip, char type, char flareType, char enableReflection, char checkObstacles, int notUsed, float normalAngle, char longDistance, float nearClip, char someFadeFlag, float flashInertia, char onlyFromBelow, char flag) = (void(__cdecl *)(int, int *, char, char, char, char, RwV3d *, float, float, char, char, char, char, int, float, char, float, char, float, char, char)) 0x6FC580;
void(__fastcall *CLODLightManager::SA::CVectorNormalize)(RwV3D *in) = (void(__fastcall *)(RwV3D *in)) 0x59C910;
RwV3D *(__cdecl *CLODLightManager::SA::CrossProduct)(RwV3D *out, RwV3D *a, RwV3D *b) = (RwV3D *(__cdecl *)(RwV3D *out, RwV3D *a, RwV3D *b)) 0x59C730;
int(__cdecl *CLODLightManager::SA::RwIm3DTransform)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags) = (int(__cdecl *)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags)) 0x7EF450;
int(__cdecl *CLODLightManager::SA::RwIm3DRenderIndexedPrimitive)(int primType, __int16 *indices, int numIndices) = (int(__cdecl *)(int primType, __int16 *indices, int numIndices)) 0x7EF550;
int(__cdecl *CLODLightManager::SA::RwIm3DEnd)() = (int(__cdecl *)()) 0x7EF520;
C2dfx *(__fastcall *CLODLightManager::SA::Get2dfx)(CBaseModelInfo *model, int edx0, int number) = (C2dfx *(__fastcall *)(CBaseModelInfo *, int, int)) 0x4C4C70;
void(__cdecl *CLODLightManager::SA::drawSpotLight)(int coronaIndex, float StartX, float StartY, float StartZ, float EndX, float EndY, float EndZ, float TargetRadius, float intensity, char flag1, char drawShadow, RwV3D *pVec1, RwV3D *pVec2, RwV3D *pVec3, char unkTrue, float BaseRadius) = (void(cdecl *)(int, float, float, float, float, float, float, float, float, char, char, RwV3D *, RwV3D *, RwV3D *, char, float)) 0x6C58E0;
bool(__thiscall *CLODLightManager::SA::CObjectIsDamaged)(CObject *pclObject) = (bool(__thiscall *)(CObject *))0x0046A2F0;
float GetDistance(RwV3D *v1, RwV3D *v2)
{
	RwV3D v3;
	v3.x = v2->x - v1->x;
	v3.y = v2->y - v1->y;
	v3.z = v2->z - v1->z;
	return sqrt(v3.x * v3.x + v3.y * v3.y + v3.z * v3.z);
}

RwV3D * GetCamPos()
{
	return (RwV3D *)(0xB6F338 + (*(BYTE *)0xB6F081 * 0x238));
}
int MTraffic1, MTraffic2, MTraffic3, MTraffic4, trafficlight1, VGSSTRIPTLIGHTS1, CJ_TRAFFIC_LIGHT3;
bool CLODLightManager::SA::IsModelALamppostNotTrafficLight(unsigned short nModelIndex)
{
	if (!MTraffic1)
	{
		MTraffic1 = GetModelInfoUInt16("MTraffic1");
		MTraffic2 = GetModelInfoUInt16("MTraffic2");
		MTraffic3 = GetModelInfoUInt16("MTraffic3");
		MTraffic4 = GetModelInfoUInt16("MTraffic4");
		trafficlight1 = GetModelInfoUInt16("trafficlight1");
		VGSSTRIPTLIGHTS1 = GetModelInfoUInt16("VGSSTRIPTLIGHTS1");
		CJ_TRAFFIC_LIGHT3 = GetModelInfoUInt16("CJ_TRAFFIC_LIGHT3");
	}

	if (nModelIndex != MTraffic1
	 && nModelIndex != MTraffic2
	 && nModelIndex != MTraffic3
	 && nModelIndex != MTraffic4
	 && nModelIndex != trafficlight1
	 && nModelIndex != VGSSTRIPTLIGHTS1
	 && nModelIndex != CJ_TRAFFIC_LIGHT3)
	 {
		return true;
	 }
	return false;
}

enum SShadType
{
	SSHADT_NONE,
	SSHADT_DEFAULT,		// SSHAD_RM_COLORONLY (shad_car, shad_ped, shad_heli, shad_bike, shad_rcbaron, bloodpool_64, wincrack_32, lamp_shad_64)
	SSHADT_INTENSIVE,	// SSHAD_RM_OVERLAP (shad_exp, headlight, headlight1, handman)
	SSHADT_NEGATIVE,	// SSHAD_RM_INVCOLOR
	SSHADT4,			// SSHAD_RM_COLORONLY
	SSHADT5,			// SSHAD_RM_COLORONLY
	SSHADT6,			// SSHAD_RM_COLORONLY
	SSHADT8 = 8			// SSHAD_RM_COLORONLY
};

enum SShadRenderMode
{
	SSHAD_RM_NONE,		// Shadow type: <0, 0, >8.
	SSHAD_RM_COLORONLY, // Shadow type: 1, 4, 5, 6, 8. The texture alpha is present if the alpha channel is set.
	SSHAD_RM_OVERLAP,	// Shadow type: 2. Any shadow alpha greater than 0 does not make any difference.
	SSHAD_RM_INVCOLOR	// Shadow type: 3. Same tip above goes here.
};

static RwTexture*			gpCustomCoronaTexture = nullptr;
RwImage*(__cdecl *const RtPNGImageRead)(const char* imageName) = (decltype(RtPNGImageRead))0x7CF9B0;
RwImage*(__cdecl *const RwImageFindRasterFormat)(RwImage* ipImage, int nRasterType, int* npWidth, int* npHeight, int* npDepth, int* npFormat) = (decltype(RwImageFindRasterFormat))0x8042C0;
RwRaster*(__cdecl *const RwRasterCreate)(int width, int height, int depth, int flags) = (decltype(RwRasterCreate))0x7FB230;
RwRaster*(__cdecl *const RwRasterSetFromImage)(RwRaster* raster, RwImage* image) = (decltype(RwRasterSetFromImage))0x804290;
RwTexture*(__cdecl *const RwTextureCreate)(RwRaster* raster) = (decltype(RwTextureCreate))0x7F37C0;
bool*(__cdecl *const RwImageDestroy)(RwImage* image) = (decltype(RwImageDestroy))0x802740;
RwTexture* CPNGFileReadFromFile(const char* pFileName)
{
	RwTexture*		pTexture = nullptr;

	if (RwImage* pImage = RtPNGImageRead(pFileName))
	{
		int		dwWidth, dwHeight, dwDepth, dwFlags;
		RwImageFindRasterFormat(pImage, 4/*rwRASTERTYPETEXTURE*/, &dwWidth, &dwHeight, &dwDepth, &dwFlags);
		if (RwRaster* pRaster = RwRasterCreate(dwWidth, dwHeight, dwDepth, dwFlags))
		{
			RwRasterSetFromImage(pRaster, pImage);

			pTexture = RwTextureCreate(pRaster);
		}
		RwImageDestroy(pImage);
	}
	return pTexture;
}

std::vector<std::string> IPLStreamNames = { "LAE_STREAM0", "LAE_STREAM1", "LAE_STREAM2", "LAE_STREAM3", "LAE_STREAM4", "LAE_STREAM5",
"LAE2_STREAM0", "LAE2_STREAM1", "LAE2_STREAM2", "LAE2_STREAM3", "LAE2_STREAM4", "LAE2_STREAM5", "LAE2_STREAM6", "LAHILLS_STREAM0",
"LAHILLS_STREAM1", "LAHILLS_STREAM2", "LAHILLS_STREAM3", "LAHILLS_STREAM4", "LAN_STREAM0", "LAN_STREAM1", "LAN_STREAM2", "LAN2_STREAM0",
"LAN2_STREAM1", "LAN2_STREAM2", "LAN2_STREAM3", "LAS_STREAM0", "LAS_STREAM1", "LAS_STREAM2", "LAS_STREAM3", "LAS_STREAM4", "LAS_STREAM5",
"LAS2_STREAM0", "LAS2_STREAM1", "LAS2_STREAM2", "LAS2_STREAM3", "LAS2_STREAM4", "LAW_STREAM0", "LAW_STREAM1", "LAW_STREAM2", "LAW_STREAM3", "LAW_STREAM4",
"LAW_STREAM5", "LAW2_STREAM0", "LAW2_STREAM1", "LAW2_STREAM2", "LAW2_STREAM3", "LAW2_STREAM4", "LAWN_STREAM0", "LAWN_STREAM1", "LAWN_STREAM2", "LAWN_STREAM3",
"COUNTN2_STREAM0", "COUNTN2_STREAM1", "COUNTN2_STREAM2", "COUNTN2_STREAM3", "COUNTN2_STREAM4", "COUNTN2_STREAM5", "COUNTN2_STREAM6", "COUNTN2_STREAM7", "COUNTN2_STREAM8",
"COUNTRYE_STREAM0", "COUNTRYE_STREAM1", "COUNTRYE_STREAM2", "COUNTRYE_STREAM3", "COUNTRYE_STREAM4", "COUNTRYE_STREAM5", "COUNTRYE_STREAM6", "COUNTRYE_STREAM7", "COUNTRYE_STREAM8",
"COUNTRYE_STREAM9", "COUNTRYN_STREAM0", "COUNTRYN_STREAM1", "COUNTRYN_STREAM2", "COUNTRYN_STREAM3", "COUNTRYS_STREAM0", "COUNTRYS_STREAM1", "COUNTRYS_STREAM2", "COUNTRYS_STREAM3", "COUNTRYS_STREAM4",
"COUNTRYW_STREAM0", "COUNTRYW_STREAM1", "COUNTRYW_STREAM2", "COUNTRYW_STREAM3", "COUNTRYW_STREAM4", "COUNTRYW_STREAM5", "COUNTRYW_STREAM6", "COUNTRYW_STREAM7", "COUNTRYW_STREAM8", "SFE_STREAM0",
"SFE_STREAM1", "SFE_STREAM2", "SFE_STREAM3", "SFN_STREAM0", "SFN_STREAM1", "SFN_STREAM2", "SFS_STREAM0", "SFS_STREAM1", "SFS_STREAM2", "SFS_STREAM3", "SFS_STREAM4", "SFS_STREAM5", "SFS_STREAM6",
"SFS_STREAM7", "SFS_STREAM8", "SFSE_STREAM0", "SFSE_STREAM1", "SFSE_STREAM2", "SFSE_STREAM3", "SFSE_STREAM4", "SFSE_STREAM5", "SFSE_STREAM6", "SFW_STREAM0", "SFW_STREAM1", "SFW_STREAM2", "SFW_STREAM3",
"SFW_STREAM4", "SFW_STREAM5", "VEGASE_STREAM0", "VEGASE_STREAM1", "VEGASE_STREAM2", "VEGASE_STREAM3", "VEGASE_STREAM4", "VEGASE_STREAM5", "VEGASE_STREAM6", "VEGASE_STREAM7", "VEGASE_STREAM8",
"VEGASN_STREAM0", "VEGASN_STREAM1", "VEGASN_STREAM2", "VEGASN_STREAM3", "VEGASN_STREAM4", "VEGASN_STREAM5", "VEGASN_STREAM6", "VEGASN_STREAM7", "VEGASN_STREAM8", "VEGASS_STREAM0", "VEGASS_STREAM1",
"VEGASS_STREAM2", "VEGASS_STREAM3", "VEGASS_STREAM4", "VEGASS_STREAM5", "VEGASW_STREAM0", "VEGASW_STREAM1", "VEGASW_STREAM2", "VEGASW_STREAM3", "VEGASW_STREAM4", "VEGASW_STREAM5", "VEGASW_STREAM6",
"VEGASW_STREAM7", "VEGASW_STREAM8", "VEGASW_STREAM9" };

char* CLODLightManager::IV::CurrentTimeHours;
char* CLODLightManager::IV::CurrentTimeMinutes;
char* CLODLightManager::EFLC::CurrentTimeHours;
char* CLODLightManager::EFLC::CurrentTimeMinutes;
char* CLODLightManager::IV::DataFilePath;
WplInstance*	pInstance;

int(__cdecl *CLODLightManager::IV::DrawCorona)(float x, float y, float z, float radius, unsigned int unk, float unk2, unsigned char r, unsigned char g, unsigned char b);
void(__stdcall *CLODLightManager::IV::GetRootCam)(int *camera);
void(__stdcall *CLODLightManager::IV::GetGameCam)(int *camera);
bool(__cdecl *CLODLightManager::IV::CamIsSphereVisible)(int camera, float pX, float pY, float pZ, float radius);
void(__cdecl *CLODLightManager::IV::GetCamPos)(int camera, float *pX, float *pY, float *pZ);

int(__cdecl *CLODLightManager::EFLC::DrawCorona)(float x, float y, float z, float radius, unsigned int unk, float unk2, unsigned char r, unsigned char g, unsigned char b);
void(__stdcall *CLODLightManager::EFLC::GetRootCam)(int *camera);
void(__stdcall *CLODLightManager::EFLC::GetGameCam)(int *camera);
bool(__cdecl *CLODLightManager::EFLC::CamIsSphereVisible)(int camera, float pX, float pY, float pZ, float radius);
void(__cdecl *CLODLightManager::EFLC::GetCamPos)(int camera, float *pX, float *pY, float *pZ);

void CLODLightManager::IV::LoadDatFile()
{
	CIniReader iniReader("");
	DataFilePath = iniReader.GetIniPath();
	char*			tempPointer;
	tempPointer = strrchr(DataFilePath, '.');
	*tempPointer = '\0';
	strcat(DataFilePath, ".dat");

	if (FILE* hFile = CFileMgr::OpenFile(DataFilePath, "r"))
	{
		DWORD nModel = 0xFFFFFFFF;  unsigned short nCurIndexForModel = 0;
		pFileContent = new std::map<unsigned int, const CLamppostInfo>;

		while (const char* pLine = CFileMgr::LoadLine(hFile))
		{
			if (pLine[0] && pLine[0] != '#')
			{
				if (pLine[0] == '%')
				{
					nCurIndexForModel = 0;
					if (strcmp(pLine, "%additional_coronas") != 0)
						nModel = GetHashKey((char *)(pLine + 1), 0);
					else
						nModel = 4294967294;
				}
				else
				{
					float			fOffsetX, fOffsetY, fOffsetZ;
					unsigned int	nRed, nGreen, nBlue;
					float			fCustomSize = 1.0f;
					int				nNoDistance = 0;
					int				nDrawSearchlight = 0;

					sscanf(pLine, "%f %f %f %d %d %d %f %d %d", &fOffsetX, &fOffsetY, &fOffsetZ, &nRed, &nGreen, &nBlue, &fCustomSize, &nNoDistance, &nDrawSearchlight);
					pFileContent->insert(std::make_pair(PackKey(nModel, nCurIndexForModel++), CLamppostInfo(CVector(fOffsetX, fOffsetY, fOffsetZ), CRGBA(static_cast<unsigned char>(nRed), static_cast<unsigned char>(nGreen), static_cast<unsigned char>(nBlue)), fCustomSize, nNoDistance, nDrawSearchlight)));
				}
			}
		}

		m_pLampposts = new std::vector<const CLamppostInfo>;
		m_bCatchLamppostsNow = true;

		CFileMgr::CloseFile(hFile);
	}
}