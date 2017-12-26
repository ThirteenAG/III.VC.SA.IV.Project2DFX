#ifndef __CLODLightManager
#define __CLODLightManager

enum BlinkTypes
{
	DEFAULT,
	RANDOM_FLASHING,
	T_1S_ON_1S_OFF,
	T_2S_ON_2S_OFF,
	T_3S_ON_3S_OFF,
	T_4S_ON_4S_OFF,
	T_5S_ON_5S_OFF,
	T_6S_ON_4S_OFF
};

class CLamppostInfo
{
public:
	CVector			vecPos;
	CRGBA			colour;
	float			fCustomSizeMult;
	int 			nNoDistance;
	int             nDrawSearchlight;
	float			fHeading;
	int				nCoronaShowMode;

	CLamppostInfo(const CVector& pos, const CRGBA& col, float fCustomMult, int CoronaShowMode, int nNoDistance, int nDrawSearchlight, float heading)
		: vecPos(pos), colour(col), fCustomSizeMult(fCustomMult), nCoronaShowMode(CoronaShowMode), nNoDistance(nNoDistance), nDrawSearchlight(nDrawSearchlight), fHeading(heading)
	{}
};

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

class CLODLightManager
{
public:
	static std::vector<CLamppostInfo>*					m_pLampposts;
	static std::map<unsigned int, CLamppostInfo>*		pFileContent;
	static bool							                m_bCatchLamppostsNow;

public:
	static bool bRenderLodLights;
	static float fCoronaRadiusMultiplier;
	static bool bSlightlyIncreaseRadiusWithDistance;
	static float fCoronaFarClip;
	static bool autoFarClip;
	static char* szCustomCoronaTexturePath;
	static bool bRenderStaticShadowsForLODs;
	static bool bIncreasePedsCarsShadowsDrawDistance;
	static float fStaticShadowsIntensity, fStaticShadowsDrawDistance;
	static float fTrafficLightsShadowsIntensity, fTrafficLightsShadowsDrawDistance;
	static bool bRenderSearchlightEffects;
	static bool bRenderOnlyDuringFoggyWeather;
	static bool bRenderHeliSearchlights;
	static int nSmoothEffect;
	static float fSearchlightEffectVisibilityFactor;
	static bool bEnableDrawDistanceChanger;
	static float fMinDrawDistanceOnTheGround, fFactor1, fFactor2, fStaticSunSize;
	static bool bAdaptiveDrawDistanceEnabled;
	static int nMinFPSValue, nMaxFPSValue;
	static float fNewFarClip, fMaxPossibleDrawDistance;
	static float fMaxDrawDistanceForNormalObjects, fTimedObjectsDrawDistance, fNeonsDrawDistance, fLODObjectsDrawDistance;
	static float fGenericObjectsDrawDistance, fAllNormalObjectsDrawDistance, fVegetationDrawDistance;
	static bool bLoadAllBinaryIPLs, bPreloadLODs;
	static float fDrawDistance;
	static bool bRandomExplosionEffects, bReplaceSmokeTrailWithBulletTrail, bFestiveLights, bFestiveLightsAlways;

	static int*							ActiveInterior;
	static char*						CurrentTimeHours;
	static char*						CurrentTimeMinutes;
	static float**						fCurrentFarClip;
	static RwTexture*					gpCustomCoronaTexture;

	static inline unsigned short        GetModelInfoUInt16(const char* pName)
	{
		static int* (__cdecl *GetModelInfoSA)(const char*, int*) = (int*(__cdecl *)(const char*, int*)) 0x4C5940;
		static int* (__cdecl *GetModelInfoVC)(const char*, int*) = (int*(__cdecl *)(const char*, int*)) 0x55F7D0;
		static int* (__cdecl *GetModelInfoIII)(const char*, int*) = (int*(__cdecl *)(const char*, int*)) 0x50B860;
		int nID = 0;
		auto gvm = injector::address_manager::singleton();
		if (gvm.IsSA())
			GetModelInfoSA(pName, &nID);
		else
			if (gvm.IsVC())
				GetModelInfoVC(pName, &nID);
			else
				GetModelInfoIII(pName, &nID);

		return static_cast<unsigned short>(nID);
	}

	static inline unsigned int	        PackKey(unsigned short nModel, unsigned short nIndex)
	{
		return nModel << 16 | nIndex;
	}

	static inline float GetDistance(RwV3D *v1, RwV3D *v2)
	{
		RwV3D v3;
		v3.x = v2->x - v1->x;
		v3.y = v2->y - v1->y;
		v3.z = v2->z - v1->z;
		return sqrt(v3.x * v3.x + v3.y * v3.y + v3.z * v3.z);
	}

	static bool							IsModelALamppost(unsigned short nModel);
	static void                         LoadDatFile();
	static bool							IsBlinkingNeeded(int BlinkType);

public:
	class III;
	class VC;
	class SA;
	class IV;
	class EFLC;
};

class CLODLightManager::SA : public CLODLightManager
{
public:
	static void							Init();
	static void 						ApplyMemoryPatches();
	static void 						RegisterCustomCoronas();
	static CEntity*						PossiblyAddThisEntity(CEntity* pEntity);
	static void							RegisterLamppost(CEntity* pObj);
	static void							RegisterLODLights();
	static void                         DrawDistanceChanger();

	static char                         (__cdecl *GetIsTimeInRange)(char hourA, char hourB);
	static int*                         (__cdecl *GetModelInfo)(const char*, int*);
	static void                         (__cdecl *const CShadowsUpdateStaticShadows)();
	static bool                         (__cdecl *const CShadowsStoreStaticShadow)(unsigned int id, unsigned char type, RwTexture *particle, CVector *pos, float x1, float y1, float x2, float y2, short alpha, unsigned char red, unsigned char green, unsigned char blue, float, float, float drawdist, bool lifetime, float updist);
	static float                        (__cdecl *FindGroundZFor3DCoord)(float x, float y, float z, BOOL *pCollisionResult, CEntity **pGroundObject);
	static RwV3D*                       (__cdecl *TransformPoint)(RwV3D *outPoint, CMatrix *m, RwV3D *point);
	static bool                         (__thiscall *CCameraIsSphereVisible)(int *camera, RwV3d *origin, float radius);
	static void                         (__cdecl *RegisterCorona)(int id, int *vehicle, char r, char g, char b, char alpha, RwV3d *pos, float radius, float farClip, char type, char flareType, char enableReflection, char checkObstacles, int notUsed, float normalAngle, char longDistance, float nearClip, char someFadeFlag, float flashInertia, char onlyFromBelow, char flag);

	static C2dfx*                       (__fastcall *Get2dfx)(CBaseModelInfo *model, int edx0, int number);
	static void                         (__cdecl *drawSpotLight)(int coronaIndex, float StartX, float StartY, float StartZ, float EndX, float EndY, float EndZ, float TargetRadius, float intensity, char flag1, char drawShadow, RwV3D *pVec1, RwV3D *pVec2, RwV3D *pVec3, char unkTrue, float BaseRadius);
	static bool                         (__thiscall *CObjectIsDamaged)(CObject *pclObject);
	static void                         (__fastcall *CVectorNormalize)(RwV3D *in);
	static RwV3D*                       (__cdecl *CrossProduct)(RwV3D *out, RwV3D *a, RwV3D *b);
	static int                          (__cdecl *RwIm3DTransform)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags);
	static int                          (__cdecl *RwIm3DRenderIndexedPrimitive)(int primType, short *indices, int numIndices);
	static int                          (__cdecl *RwIm3DEnd)();
};

class CLODLightManager::VC : public CLODLightManager
{
public:
	static void							Init();
	static void 						ApplyMemoryPatches();
	static void 						RegisterCustomCoronas();
	static void							RegisterLamppost(CEntityVC* entity);
	static void							RegisterLODLights();
	static void                         DrawDistanceChanger();

	static int*                         (__cdecl *GetModelInfo)(const char*, int*);
	static char                         (__cdecl *GetIsTimeInRange)(char hourA, char hourB);
	static bool                         (__cdecl *const CShadowsStoreStaticShadow)(unsigned int id, unsigned char type, RwTexture *particle, CVector *pos, float x1, float y1, float x2, float y2, short alpha, unsigned char red, unsigned char green, unsigned char blue, float, float, float drawdist, bool lifetime, float updist);
	static float                        (__cdecl *FindGroundZFor3DCoord)(float x, float y, float z, BOOL *pCollisionResult, CEntity **pGroundObject);
	static RwV3D*                       (__cdecl *TransformPoint)(RwV3d *a1, RwV3d *a2, int a3, RwMatrix *a4);
	static bool                         CCameraIsSphereVisible(RwV3D *origin, float radius);
	static void                         (__cdecl *RegisterCorona)(int id, char r, char g, char b, char alpha, RwV3D *pos, float radius, float farClp, char a9, char lensflare, char a11, char see_through_effect, char trace, float a14, char a15, float a16);
	static void                         (__fastcall *CVectorNormalize)(RwV3D *in);
	static RwV3D*                       (__cdecl *CrossProduct)(RwV3D *out, RwV3D *a, RwV3D *b);
	static int                          (__cdecl *RwIm3DTransform)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags);
	static int                          (__cdecl *RwIm3DRenderIndexedPrimitive)(int primType, short *indices, int numIndices);
	static int                          (__cdecl *RwIm3DEnd)();
	static int                          (__cdecl *RwRenderStateSetVC)(RwRenderState nState, void *pParam);
};

class CLODLightManager::III : public CLODLightManager
{
public:
	static void							Init();
	static void 						ApplyMemoryPatches();
	static void 						RegisterCustomCoronas();
	static void							RegisterLamppost(CEntityIII* entity);
	static void							RegisterLODLights();
	static void                         DrawDistanceChanger();

	static int*                         (__cdecl *GetModelInfo)(const char*, int*);
	static char                         (__cdecl *GetIsTimeInRange)(char hourA, char hourB);
	static bool                         (__cdecl *const CShadowsStoreStaticShadow)(unsigned int id, unsigned char type, RwTexture *particle, CVector *pos, float x1, float y1, float x2, float y2, short alpha, unsigned char red, unsigned char green, unsigned char blue, float, float, float drawdist, bool lifetime, float updist);
	static float                        (__cdecl *FindGroundZFor3DCoord)(float x, float y, float z, BOOL *pCollisionResult, CEntity **pGroundObject);
	static RwV3D*                       (__cdecl *TransformPoint)(RwV3D *, RwMatrix *, RwV3D *);
	static bool                         (__thiscall *CCameraIsSphereVisible)(int *camera, RwV3D *origin, float radius);
	static void                         (__cdecl *RegisterCorona)(int coronaID, char r, char g, char b, char a, RwV3D *pos, float size, float farClip, char type, char flare, char reflection, char obstacles, char notUsed, float normalAngle);
	static void                         (__fastcall *CVectorNormalize)(RwV3D *in);
	static RwV3D*                       (__cdecl *CrossProduct)(RwV3D *out, RwV3D *a, RwV3D *b);
	static int                          (__cdecl *RwIm3DTransform)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags);
	static int                          (__cdecl *RwIm3DRenderIndexedPrimitive)(int primType, short *indices, int numIndices);
	static int                          (__cdecl *RwIm3DEnd)();
	static int                          (__cdecl *RwRenderStateSetIII)(RwRenderState nState, void *pParam);
};

class CLODLightManager::IV : public CLODLightManager
{
public:
	static void							Init();
	static void							GetMemoryAddresses();
	static void							IncreaseCoronaLimit();
	static void							RegisterCustomCoronas();
	static WplInstance*					PossiblyAddThisEntity(WplInstance * pInstance);
	static void							RegisterLamppost(WplInstance * pObj);
	static void							RegisterLODLights();


	static int							(__cdecl *DrawCorona)(float x, float y, float z, float radius, unsigned int unk, float unk2, unsigned char r, unsigned char g, unsigned char b);
	static int							(__cdecl *DrawCorona2)(int id, char r, char g, char b, float a5, CVector* pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14);
	static int							(__cdecl *DrawCorona3)(int id, char r, char g, char b, float a5, CVector* pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14);
	static void							(__stdcall *GetRootCam)(int *camera);
	static void							(__stdcall *GetGameCam)(int *camera);
	static bool							(__cdecl *CamIsSphereVisible)(int camera, float pX, float pY, float pZ, float radius);
	static void							(__cdecl *GetCamPos)(int camera, float *pX, float *pY, float *pZ);

	static unsigned int GetHashKey(char *str, unsigned int a2)
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
};

class CLODLightManager::EFLC : public CLODLightManager::IV
{

};

#endif