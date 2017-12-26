#ifndef __GENERAL
#define __GENERAL

#define FUNC_CEntity__GetBoundCentre				0x534250
#define NOVMT __declspec(novtable)
#define SETVMT(a) *((DWORD_PTR*)this) = (DWORD_PTR)a

#include "rwsdk\rwcore.h"
#include "rwsdk\rpworld.h"

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

#define RwV3D RwV3d

class CFileMgr
{
public:
	static inline FILE*  OpenFile(const char* path, const char* mode)
	{
		return fopen(path, mode);
	};
	static inline  int  CloseFile(FILE* stream)
	{
		return fclose(stream);
	};
	static inline bool  ReadLine(FILE* stream, char* str, int num)
	{
		return fgets(str, num, stream) != nullptr;
	};
	static inline size_t Read(FILE* stream, void* buf, size_t len)
	{
		return fread(buf, 1, len, stream);
	};
	static inline size_t Write(FILE* stream, const char* ptr, size_t len)
	{
		return fwrite(ptr, 1, len, stream);
	};
	static inline bool  Seek(FILE* stream, long pos, int from)
	{
		return fseek(stream, pos, from) != 0;
	};
	static inline const char* LoadLine(FILE* hFile)
	{
		static char		cLineBuffer[512];

		if (!CFileMgr::ReadLine(hFile, cLineBuffer, sizeof(cLineBuffer)))
			return nullptr;

		for (int i = 0; cLineBuffer[i]; ++i)
		{
			if (cLineBuffer[i] == '\n')
				cLineBuffer[i] = '\0';
			else if (cLineBuffer[i] < ' ' || cLineBuffer[i] == ',')
				cLineBuffer[i] = ' ';
		}

		const char* p = cLineBuffer;
		while (*p <= ' ')
		{
			if (!*p++)
				break;
		}
		return p;
	};
};

class CTimer
{
public:
	static int&				m_snTimeInMilliseconds;
	static int&				m_snTimeInMillisecondsPauseMode;
	static float&			ms_fTimeStep;
	//static unsigned int&	m_FrameCounter;
};

class CSimpleTransform
{
public:
	CVector                         m_translate;
	float                           m_heading;

public:
	void							UpdateMatrix(CMatrix* pMatrix) const
	{
		pMatrix->SetTranslate(m_translate.x, m_translate.y, m_translate.z);
		pMatrix->SetRotateZOnly(m_heading);
	}

	void							UpdateRwMatrix(RwMatrix* pMatrix) const
	{
		pMatrix->right.x = cos(m_heading);
		pMatrix->right.y = sin(m_heading);
		pMatrix->right.z = 0.0f;

		pMatrix->at.x = 0.0f;
		pMatrix->at.y = 0.0f;
		pMatrix->at.z = 1.0f;

		pMatrix->up.x = -sin(m_heading);
		pMatrix->up.y = cos(m_heading);
		pMatrix->up.z = 0.0f;

		pMatrix->pos.x = m_translate.x;
		pMatrix->pos.y = m_translate.y;
		pMatrix->pos.z = m_translate.z;

		pMatrix->flags &= ~(0x00000003 | 0x00020000);
	}

	void							Invert(const CSimpleTransform& src)
	{
		m_translate.x = -(cos(src.m_heading) * src.m_translate.x - sin(src.m_heading) * src.m_translate.y);;
		m_translate.y = sin(src.m_heading) * src.m_translate.x - cos(m_heading) * src.m_translate.y;
		m_translate.z = -src.m_translate.z;
		m_heading = -src.m_heading;
	}
};

class CRGBA
{
public:
	BYTE r, g, b, a;

	inline CRGBA() {}

	inline CRGBA(const CRGBA& in)
		: r(in.r), g(in.g), b(in.b), a(in.a)
	{}

	inline CRGBA(const CRGBA& in, BYTE alpha)
		: r(in.r), g(in.g), b(in.b), a(alpha)
	{}

	inline CRGBA(BYTE red, BYTE green, BYTE blue, BYTE alpha = 255)
		: r(red), g(green), b(blue), a(alpha)
	{}

	template <typename T>
	friend CRGBA Blend(const CRGBA& One, T OneStrength, const CRGBA& Two, T TwoStrength)
		{	T	TotalStrength = OneStrength + TwoStrength;
			return CRGBA(	((One.r * OneStrength) + (Two.r * TwoStrength))/TotalStrength,
							((One.g * OneStrength) + (Two.g * TwoStrength))/TotalStrength,
							((One.b * OneStrength) + (Two.b * TwoStrength))/TotalStrength,
							((One.a * OneStrength) + (Two.a * TwoStrength))/TotalStrength); }

	template <typename T>
	friend CRGBA Blend(const CRGBA& One, T OneStrength, const CRGBA& Two, T TwoStrength, const CRGBA& Three, T ThreeStrength)
		{	T	TotalStrength = OneStrength + TwoStrength + ThreeStrength;
			return CRGBA(	((One.r * OneStrength) + (Two.r * TwoStrength) + (Three.r * ThreeStrength))/TotalStrength,
							((One.g * OneStrength) + (Two.g * TwoStrength) + (Three.g * ThreeStrength))/TotalStrength,
							((One.b * OneStrength) + (Two.b * TwoStrength) + (Three.b * ThreeStrength))/TotalStrength,
							((One.a * OneStrength) + (Two.a * TwoStrength) + (Three.a * ThreeStrength))/TotalStrength); }

	void	BaseColors__Constructor();
};

class CRect
{
public:
	float x1, y1;
	float x2, y2;

	inline CRect() {}
	inline CRect(float a, float b, float c, float d)
		: x1(a), y1(b), x2(c), y2(d)
	{}
};


class CPlaceable
{
private:
	CSimpleTransform				m_transform;
	CMatrix*						m_pCoords;

public:
	// Line up the VMTs
	virtual ~CPlaceable() {}

	inline CPlaceable() {}

	explicit inline CPlaceable(int dummy)
	{
		// Dummy ctor
		UNREFERENCED_PARAMETER(dummy);
	}

	inline CVector&					GetCoords()
		{ return m_pCoords ? reinterpret_cast<CVector&>(m_pCoords->matrix.pos) : m_transform.m_translate; }
	inline CMatrix*					GetMatrix()
		{ return m_pCoords; }
	inline CSimpleTransform&		GetTransform()
		{ return m_transform; }
	inline float					GetHeading()
		{ return m_pCoords ? atan2(-m_pCoords->GetUp()->x, m_pCoords->GetUp()->y) : m_transform.m_heading; }

	inline void						SetCoords(const CVector& pos)
	{	if ( m_pCoords ) { m_pCoords->matrix.pos.x = pos.x; m_pCoords->matrix.pos.y = pos.y; m_pCoords->matrix.pos.z = pos.z; }
		else m_transform.m_translate = pos; }
	inline void						SetHeading(float fHeading)
		{ if ( m_pCoords ) m_pCoords->SetRotateZOnly(fHeading); else m_transform.m_heading = fHeading; }

	void							AllocateMatrix();
};

// TODO: May not be the best place to put it?
class NOVMT CEntity	: public CPlaceable
{
public:
	virtual void	Add_CRect();
	virtual void	Add();
	virtual void	Remove();
	virtual void	SetIsStatic(bool);
	virtual void	SetModelIndex(int nModelIndex);
	virtual void	SetModelIndexNoCreate(int nModelIndex);
	virtual void	CreateRwObject();
	virtual void	DeleteRwObject();
	virtual CRect	GetBoundRect();
	virtual void	ProcessControl();
	virtual void	ProcessCollision();
	virtual void	ProcessShift();
	virtual void	TestCollision();
	virtual void	Teleport();
	virtual void	SpecialEntityPreCollisionStuff();
	virtual void	SpecialEntityCalcCollisionSteps();
	virtual void	PreRender();
	virtual void	Render();
	virtual void	SetupLighting();
	virtual void	RemoveLighting();
	virtual void	FlagToDestroyWhenNextProcessed();

//private:
	RwObject*		m_pRwObject;						// 0x18

	/********** BEGIN CFLAGS (0x1C) **************/
	unsigned long	bUsesCollision : 1;				// does entity use collision
	unsigned long	bCollisionProcessed : 1;			// has object been processed by a ProcessEntityCollision function
	unsigned long	bIsStatic : 1;					// is entity static
	unsigned long	bHasContacted : 1;				// has entity processed some contact forces
	unsigned long	bIsStuck : 1;						// is entity stuck
	unsigned long	bIsInSafePosition : 1;			// is entity in a collision free safe position
	unsigned long	bWasPostponed : 1;				// was entity control processing postponed
	unsigned long	bIsVisible : 1;					//is the entity visible

	unsigned long	bIsBIGBuilding : 1;				// Set if this entity is a big building
	unsigned long	bRenderDamaged : 1;				// use damaged LOD models for objects with applicable damage
	unsigned long	bStreamingDontDelete : 1;			// Dont let the streaming remove this
	unsigned long	bRemoveFromWorld : 1;				// remove this entity next time it should be processed
	unsigned long	bHasHitWall : 1;					// has collided with a building (changes subsequent collisions)
	unsigned long	bImBeingRendered : 1;				// don't delete me because I'm being rendered
	unsigned long	bDrawLast :1;						// draw object last
	unsigned long	bDistanceFade :1;					// Fade entity because it is far away

	unsigned long	bDontCastShadowsOn : 1;			// Dont cast shadows on this object
	unsigned long	bOffscreen : 1;					// offscreen flag. This can only be trusted when it is set to true
	unsigned long	bIsStaticWaitingForCollision : 1; // this is used by script created entities - they are static until the collision is loaded below them
	unsigned long	bDontStream : 1;					// tell the streaming not to stream me
	unsigned long	bUnderwater : 1;					// this object is underwater change drawing order
	unsigned long	bHasPreRenderEffects : 1;			// Object has a prerender effects attached to it
	unsigned long	bIsTempBuilding : 1;				// whether or not the building is temporary (i.e. can be created and deleted more than once)
	unsigned long	bDontUpdateHierarchy : 1;			// Don't update the aniamtion hierarchy this frame

	unsigned long	bHasRoadsignText : 1;				// entity is roadsign and has some 2deffect text stuff to be rendered
	unsigned long	bDisplayedSuperLowLOD : 1;
	unsigned long	bIsProcObject : 1;				// set object has been generate by procedural object generator
	unsigned long	bBackfaceCulled : 1;				// has backface culling on
	unsigned long	bLightObject : 1;					// light object with directional lights
	unsigned long	bUnimportantStream : 1;			// set that this object is unimportant, if streaming is having problems
	unsigned long	bTunnel : 1;						// Is this model part of a tunnel
	unsigned long	bTunnelTransition : 1;			// This model should be rendered from within and outside of the tunnel
	/********** END CFLAGS **************/

	WORD			RandomSeed;					// 0x20
	short			m_nModelIndex;				// 0x22
	void*			pReferences;				// 0x24
	void*			m_pLastRenderedLink;		// 0x28
	WORD			m_nScanCode;				// 0x2C
	BYTE			m_iplIndex;					// 0x2E
	BYTE			m_areaCode;					// 0x2F
	CEntity*		m_pLod;					// 0x30
	BYTE			numLodChildren;				// 0x34
	char			numLodChildrenRendered;		// 0x35

	//********* BEGIN CEntityInfo **********//
	BYTE			nType : 3;							// what type is the entity	// 0x36 (2 == Vehicle)
	BYTE			nStatus : 5;						// control status			// 0x36
	//********* END CEntityInfo ************//

	// VCS PC class extension
	bool			bIveBeenRenderedOnce : 1;		// for realtime shadows

public:
	explicit inline CEntity(int dummy)
		: CPlaceable(dummy)
	{
		// Dummy ctor
	}

	inline short&	ModelIndex()
						{ return m_nModelIndex; };
	inline short	GetModelIndex()
					{ return m_nModelIndex; }

	class CRealTimeShadow*		GetRealTimeShadow();
	void						SetRealTimeShadow(class CRealTimeShadow* pShadow);

	void			UpdateRW();
	void			RegisterReference(CEntity** pAddress);
	void			CleanUpOldReference(CEntity** pAddress);
};

class NOVMT CPhysical : public CEntity
{
private:
	float				pad1; // 56
	int					__pad2; // 60

	unsigned int		b0x01 : 1; // 64
	unsigned int		bApplyGravity : 1;
	unsigned int		bDisableFriction : 1;
	unsigned int		bCollidable : 1;
	unsigned int		b0x10 : 1;
	unsigned int		bDisableMovement : 1;
	unsigned int		b0x40 : 1;
	unsigned int		b0x80 : 1;

	unsigned int		bSubmergedInWater : 1; // 65
	unsigned int		bOnSolidSurface : 1;
	unsigned int		bBroken : 1;
	unsigned int		b0x800 : 1; // ref @ 0x6F5CF0
	unsigned int		b0x1000 : 1;//
	unsigned int		b0x2000 : 1;//
	unsigned int		b0x4000 : 1;//
	unsigned int		b0x8000 : 1;//

	unsigned int		b0x10000 : 1; // 66
	unsigned int		b0x20000 : 1; // ref @ CPhysical__processCollision
	unsigned int		bBulletProof : 1;
	unsigned int		bFireProof : 1;
	unsigned int		bCollisionProof : 1;
	unsigned int		bMeeleProof : 1;
	unsigned int		bInvulnerable : 1;
	unsigned int		bExplosionProof : 1;

	unsigned int		b0x1000000 : 1; // 67
	unsigned int		bAttachedToEntity : 1;
	unsigned int		b0x4000000 : 1;
	unsigned int		bTouchingWater : 1;
	unsigned int		bEnableCollision : 1;
	unsigned int		bDestroyed : 1;
	unsigned int		b0x40000000 : 1;
	unsigned int		b0x80000000 : 1;

	CVector				m_vecLinearVelocity; // 68
	CVector				m_vecAngularVelocity; // 80
	CVector				m_vecCollisionLinearVelocity; // 92
	CVector				m_vecCollisionAngularVelocity; // 104
	CVector				m_vForce;							// 0x74
	CVector				m_vTorque;							// 0x80
	float				fMass;								// 0x8C
	float				fTurnMass;							// 0x90
	float				m_fVelocityFrequency;					// 0x94
	float				m_fAirResistance;						// 0x98
	float				m_fElasticity;						// 0x9C
	float				m_fBuoyancyConstant;					// 0xA0

	CVector				vecCenterOfMass;					// 0xA4
	DWORD				dwUnk1;								// 0xB0
	void*				unkCPtrNodeDoubleLink;				// 0xB4
	BYTE				byUnk: 8;								// 0xB8
	BYTE				byCollisionRecords: 8;					// 0xB9
	BYTE				byUnk2: 8;								// 0xBA (Baracus)
	BYTE				byUnk3: 8;								// 0xBB

	float				pad4[6];								// 0xBC

	float				fDistanceTravelled;					// 0xD4
	float				fDamageImpulseMagnitude;				// 0xD8
	CEntity*			damageEntity;						// 0xDC
	BYTE				pad2[28];								// 0xE0
	CEntity*			pAttachedEntity;					// 0xFC
	CVector				m_vAttachedPosition;				// 0x100
	CVector				m_vAttachedRotation;				// 0x10C
	BYTE				pad3[20];								// 0x118
	float				fLighting;							// 0x12C col lighting? CPhysical::GetLightingFromCol
	float				fLighting_2;							// 0x130 added to col lighting in CPhysical::GetTotalLighting
	class CRealTimeShadow*	m_pShadow;							// 0x134

public:
	inline class CRealTimeShadow*	GetRealTimeShadow()
		{ return m_pShadow; }
	inline CVector&					GetLinearVelocity()
		{ return m_vecLinearVelocity; }

	inline void						SetRealTimeShadow(class CRealTimeShadow* pShadow)
		{ m_pShadow = pShadow; }

	// Temp
	CPhysical()
	: CEntity(0) {}
};

struct C2dfx
{
	RwV3D offset;
	int type;
	RwRGBA color;
	float coronaFarClip;
	float pointlightRange;
	float coronaSize;
	float shadowSize;
	union flagsInfo;
	char coronaFlashType;
	char coronaEnableReflection;
	char coronaFlareType;
	char shadowColorMultiplier;
	char shadowZDistance;
	char offsetX;
	char offsetY;
	char offsetZ;
	char __pad[2];
	RwTexture *coronaTex;
	RwTexture *shadowTex;
	int field_38;
	int field_3C;
};

struct CBaseModelInfo
{
	int _vmt, m_dwKey;
	short usageCount, m_wTxdIndex;
	char _fC, m_nbCount2dfx;
	short m_w2dfxIndex, m_wObjectInfoIndex, m_wFlags;
	int m_pColModel;
	float m_fDrawDistance;
	int m_pRwObject;
};

struct RxObjSpace3dVertex	// sizeof = 0x24
{
	RwV3D objVertex;
	RwV3D objNormal;
	DWORD color;
	float u;
	float v;
};

struct CObjectInfo
{
	float  m_fMass;
	float  m_fTurnMass;
	float  m_fAirResistance;
	float  m_fElasticity;
	float  m_fBuoyancyConstant;
	float  m_fUprootLimit;
	float  m_fColDamageMultiplier;
	BYTE   m_bColDamageEffect;
	BYTE   m_bSpecialColResponseCase;
	BYTE   m_bCameraAvoidObject;
	BYTE   m_bCausesExplosion;
	BYTE   m_bFxType;
	BYTE   field_21;
	BYTE   field_22;
	BYTE   field_23;
	RwV3D  m_vFxOffset;
	void  *m_pFxSystem;              // CFxSystem
	int    field_34;
	RwV3D  m_vBreakVelocity;
	float  m_fBreakVelocityRand;
	float  m_fSmashMultiplier;
	DWORD  m_dwSparksOnImpact;
};

struct CObject : public CPhysical
{
	int          field_138;
	char         m_bObjectType;
	char         field_13D;
	char         gap_13E[2];
	int          m_dwObjectFlags;
	char         m_bColDamageEffect;
	char         field_145;
	char         field_146;
	char         field_147;
	char         lastWeaponDamage;
	char         m_bColBrightness;
	char         m_bCarPartModelId;
	char         gap_14B[1];
	BYTE         m_bColorId[4];
	int          field_150;
	float        m_fHealth;
	float        field_158;
	int          m_fScale;
	CObjectInfo *m_pObjectInfo;
	int          field_164;
	WORD         field_168;
	WORD         m_wPaintjobTxd;
	RwTexture   *m_pPaintjobTex;
	int          field_170;
	int          field_174;
	float        field_178;
};


bool CalcScreenCoors(const CVector& vecIn, CVector* vecOut);
void LoadingScreenLoadingFile(const char* pText);

extern CRGBA*				BaseColors;
extern RwCamera*&			Camera;

static_assert(sizeof(CEntity) == 0x38, "Wrong size: CEntity");
static_assert(sizeof(CPhysical) == 0x138, "Wrong size: CPhysical");

struct CPool
{
	int objects;
	int flags, size, top;
	char initialized, _f11;
	short _pad;
};

struct CEntityVC	// sizeof = 0x64
{
	DWORD __vmt;
	CMatrix matrix;
	//DWORD rwObject; CMatrix sizeof is +4 for some reason
	BYTE flags;
	BYTE type;

	//unsigned char m_nFlags2;
	unsigned char b2_1 : 1;
	unsigned char m_bExplosionProof : 1;
	unsigned char m_bIsVisible : 1;
	unsigned char b2_8 : 1;
	unsigned char b2_10 : 1;
	unsigned char m_bFlashing : 1;
	unsigned char m_bIsBIGBuilding : 1;
	unsigned char m_bRenderDamaged : 1;

	BYTE field_53;
	BYTE field_54;
	BYTE field_55;
	BYTE field_56;
	BYTE field_57;
	WORD field_58;
	WORD field_5A;
	WORD m_nModelIndex;
	BYTE buildingIsland;
	BYTE interior;
	DWORD pReference;
};

class CEntityIII
{
public:
	DWORD __vmt;
	CMatrix matrix;
	//RwObject *m_pRwObject; CMatrix sizeof is +4 for some reason
	unsigned char m_nType : 3;
	unsigned char m_nState : 5;

	//unsigned char m_nFlags1;
	unsigned char m_bUsesCollision : 1;
	unsigned char m_bCollisionProcessed : 1;
	unsigned char m_bIsStatic : 1;
	unsigned char b1_8 : 1;
	unsigned char b1_10 : 1;
	unsigned char b1_20 : 1;
	unsigned char m_bIsInSafePosition : 1;
	unsigned char m_bHasContacted : 1;

	//unsigned char m_nFlags2;
	unsigned char b2_1 : 1;
	unsigned char m_bExplosionProof : 1;
	unsigned char m_bIsVisible : 1;
	unsigned char b2_8 : 1;
	unsigned char b2_10 : 1;
	unsigned char m_bFlashing : 1;
	unsigned char m_bIsBIGBuilding : 1;
	unsigned char m_bRenderDamaged : 1;

	//unsigned char m_nFlags3;
	unsigned char m_bBulletProof : 1;
	unsigned char m_bFireProof : 1;
	unsigned char m_bCollisionProof : 1;
	unsigned char m_bMeleeProof : 1;
	unsigned char m_bOnlyDamagedByPlayer : 1;
	unsigned char b3_20 : 1;
	unsigned char b3_40 : 1;
	unsigned char b3_80 : 1;

	//unsigned char m_nFlags4;
	unsigned char b4_1 : 1;
	unsigned char m_bHashtWall : 2;
	unsigned char m_bImBeingRendered : 1;
	unsigned char b4_8 : 1;
	unsigned char b4_10 : 1;
	unsigned char m_bDrawLast : 1;
	unsigned char b4_40 : 1;
	unsigned char b4_80 : 1;

	//unsigned char m_nFlags5;
	unsigned char b5_1 : 1;
	unsigned char b5_2 : 2;

	char _pad[2];
	unsigned short m_wScanCode;
	unsigned short m_wRandomSeed;
	short m_nModelIndex;
	short m_wLevel; // -1 - ignore level transitions
	void *m_pReferenceList; // CReference* ?
};

class Interval
{
private:
	unsigned int initial_;

public:
	inline Interval() : initial_(GetTickCount()) { }

	virtual ~Interval() { }

	inline unsigned int value() const
	{
		return GetTickCount() - initial_;
	}
};

class Fps
{
protected:
	unsigned int m_fps;
	unsigned int m_fpscount;
	Interval m_fpsinterval;

public:
	// Constructor
	Fps() : m_fps(0), m_fpscount(0) { }

	// Update
	void update()
	{
		// increase the counter by one
		m_fpscount++;

		// one second elapsed? (= 1000 milliseconds)
		if (m_fpsinterval.value() > 1000)
		{
			// save the current counter value to m_fps
			m_fps = m_fpscount;

			// reset the counter and the interval
			m_fpscount = 0;
			m_fpsinterval = Interval();
		}
	}

	// Get fps
	unsigned int get() const
	{
		return m_fps;
	}
};

template <class RandomIt>
void shuffleRefs(RandomIt first, RandomIt last) {
	typename std::iterator_traits<RandomIt>::difference_type i, n;
	n = last - first;
	for (i = n - 1; i > 0; --i) {
		using std::swap;
		swap(first[i].get(), first[std::rand() % (i + 1)].get());
	}
}

template <class MapType>
void shuffleMap(MapType &map) {
	std::vector<std::reference_wrapper<typename MapType::mapped_type>> v;
	for (auto &el : map) v.push_back(std::ref(el.second));
	shuffleRefs(v.begin(), v.end());
}

#endif