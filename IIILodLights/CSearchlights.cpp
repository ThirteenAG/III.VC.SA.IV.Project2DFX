#include "CSearchlights.h"

extern float* CWeatherFoggyness;
extern CVector* GetCamPos();

struct CHeliIII
{
	char pad[0x34];
	RwV3D pos;
	char pad2[0x26C];
	RwV3D shadowPos;
	char pad3[0x4C];
	float unk;
	char pad4[0x30];
};

void CSearchlights::RenderHeliSearchLightsIII()
{
	static CHeliIII ***pPoliceHelis = (CHeliIII ***)0x54AE16;
	CHeliIII* HeliInst = nullptr;
	int nOffset = 0;
	short NumOfHelisRequired = *(short*)0x95CCAA;
	if (NumOfHelisRequired > 0)
	{
		nOffset = 0;
		do
		{
			HeliInst = *(nOffset + (*pPoliceHelis));
			if (HeliInst)
			{
				if (HeliInst->unk > 0.0f)
				{
					RwV3D EndPoint = HeliInst->shadowPos;

					if (EndPoint.x && EndPoint.y)
					{
						EndPoint.z = CLODLightManager::III::FindGroundZFor3DCoord(HeliInst->shadowPos.x, HeliInst->shadowPos.y, HeliInst->pos.z, 0, 0);
						drawCustomSpotLightIII(HeliInst->pos, EndPoint, 13.0f, 1.0f, 255.0f, 8, 0.8f); //white
					}
				}
			}
			nOffset++;
			--NumOfHelisRequired;
		} while (NumOfHelisRequired > 0);
	}
}

void CSearchlights::RenderSearchLightsIII()
{
	if ((CLODLightManager::III::bRenderOnlyDuringFoggyWeather && *CWeatherFoggyness) || !CLODLightManager::III::bRenderOnlyDuringFoggyWeather)
	{
		if (CLODLightManager::III::GetIsTimeInRange(20, 7))
		{
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATEZWRITEENABLE, (void*)0);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATEZTESTENABLE, (void*)1);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATESRCBLEND, (void*)2);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATEDESTBLEND, (void*)2);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATETEXTURERASTER, (void*)0);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATEFOGENABLE, (void*)0);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATESHADEMODE, (void*)2);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATEALPHATESTFUNCTION, (void*)7);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)0);
	
			for (auto it = CLODLightManager::III::m_pLampposts->cbegin(); it != CLODLightManager::III::m_pLampposts->cend(); ++it)
			{
				if (it->nDrawSearchlight)
				{
					CVector*	pCamPos = GetCamPos();
					float		fDistSqr = (pCamPos->x - it->vecPos.x)*(pCamPos->x - it->vecPos.x) + (pCamPos->y - it->vecPos.y)*(pCamPos->y - it->vecPos.y) + (pCamPos->z - it->vecPos.z)*(pCamPos->z - it->vecPos.z);

					if ((fDistSqr > 50.0f*50.0f) && (fDistSqr < 300.0f*300.0f))
					{
						float fVisibility = CLODLightManager::III::fSearchlightEffectVisibilityFactor * ((0.0233333f)*sqrt(fDistSqr) - 1.16667f);
						//if (*CWeatherFoggyness)
						//	fVisibility *= 0.5f;

						RwV3D EndPoint = *(RwV3D*)&it->vecPos;
						EndPoint.z = CLODLightManager::III::FindGroundZFor3DCoord(it->vecPos.x, it->vecPos.y, it->vecPos.z, 0, 0);

						if (!(it->colour.r == 255 && it->colour.g == 255 && it->colour.b == 255))
						{
							//yellow
							drawCustomSpotLightIII(*(RwV3D*)&it->vecPos, EndPoint, min((8.0f * (it->vecPos.z - EndPoint.z)), 90.0f), it->fCustomSizeMult / 6.0f, 5.0f, 8, fVisibility);
						}
						else //if (!(it->colour.r == 254 && it->colour.g == 117 && it->colour.b == 134))
						{
							//white
							drawCustomSpotLightIII(*(RwV3D*)&it->vecPos, EndPoint, min((8.0f * (it->vecPos.z - EndPoint.z)), 90.0f), it->fCustomSizeMult / 6.0f, 255.0f, 8, fVisibility);
						}
						//else
						//{
						//	//pink
						//	drawCustomSpotLightVC(*(RwV3D*)&it->vecPos, EndPoint, min((8.0f * (it->vecPos.z - EndPoint.z)), 90.0f), it->fCustomSizeMult / 6.0f, 200.0f, 18, fVisibility);
						//}
					}
				}
			}

			CLODLightManager::III::bRenderHeliSearchlights ? RenderHeliSearchLightsIII() : nullptr;
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATEZWRITEENABLE, (void*)1);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATEZTESTENABLE, (void*)1);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATESRCBLEND, (void*)5);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATEDESTBLEND, (void*)6);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATECULLMODE, (void*)2);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATEALPHATESTFUNCTION, (void*)5);
			CLODLightManager::III::RwRenderStateSetIII(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)2);
		}
	}
}

void __cdecl CSearchlights::drawCustomSpotLightIII(RwV3D StartPoint, RwV3D EndPoint, float TargetRadius, float baseRadius, float slColorFactor1, char slColorFactor2, float slAlpha)
{
	static short TempBufferRenderIndexList[4096];
	static RxObjSpace3dVertex TempVertexBuffer[500];
	static int TempBufferIndicesStored = 0;
	static unsigned int TempBufferVerticesStored = 0;

	RwMatrix *v12; // ebx@1
	RwV3D *v13; // eax@4
	signed int v19; // esi@8
	float v20; // fst7@9
	float v21; // ST7C_4@9
	float v22; // ST60_4@9
	float v23; // fst7@9
	float v24; // fst6@9
	float v25; // fst4@9
	float v26; // ST7C_4@9
	float v27; // ST68_4@9
	float v28; // ST6C_4@9
	float v29; // fst4@9
	float v30; // fst3@9
	float v31; // ST80_4@9
	float v32; // fst2@9
	float v33; // ST60_4@9
	float v34; // ST74_4@9
	float v35; // ST78_4@9
	float v36; // ST78_4@9
	float v37; // ST60_4@9
	float v38; // fst7@15
	float v39; // fst5@15
	float v40; // ST80_4@16
	float v41; // fst4@15
	float v42; // fst7@17
	unsigned int v43; // eax@17
	float v44; // fst7@17
	float v45; // fst7@17
	float v46; // fst7@17
	float v47; // fst7@19
	int v49; // ebp@21
	float v50; // fst7@22
	unsigned int v51; // edi@24
	signed int v52; // esi@25
	float v53; // fst7@26
	unsigned char *v54; // ebp@26
	float v55; // fst6@27
	int v56; // ebx@27
	unsigned __int64 v57; // qax@27
	float v59; // fst6@27
	int v60; // ebx@27
	unsigned __int64 v61; // qax@27
	float v63; // fst6@27
	int v64; // ebx@27
	unsigned __int64 v65; // qax@27
	float v66; // fst6@27
	float v67; // fst7@30
	float v68; // fst6@31
	signed int v88; // [sp+44h] [bp-424h]@8
	signed int v89; // [sp+44h] [bp-424h]@26
	void *v90; // [sp+44h] [bp-424h]@30
	float a4; // [sp+48h] [bp-420h]@9
	float EndPointa; // [sp+4Ch] [bp-41Ch]@9
	float v97; // [sp+50h] [bp-418h]@9
	float a6; // [sp+54h] [bp-414h]@9
	float a7; // [sp+58h] [bp-410h]@9
	float v104; // [sp+5Ch] [bp-40Ch]@9
	float v105; // [sp+64h] [bp-404h]@17
	RwV3D a; // [sp+68h] [bp-400h]@1
	float v107; // [sp+74h] [bp-3F4h]@8
	RwV3D v108; // [sp+78h] [bp-3F0h]@1
	float v109; // [sp+84h] [bp-3E4h]@9
	RwV3D v110; // [sp+88h] [bp-3E0h]@6
	RwV3D v111; // [sp+94h] [bp-3D4h]@9
	float v112; // [sp+A0h] [bp-3C8h]@9
	float v113; // [sp+A4h] [bp-3C4h]@9
	RwV3D v115; // [sp+ACh] [bp-3BCh]@16
	float v116; // [sp+B8h] [bp-3B0h]@9
	float v117; // [sp+BCh] [bp-3ACh]@9
	float v118; // [sp+C0h] [bp-3TargetRadiush]@9
	RwV3D out; // [sp+C4h] [bp-3A4h]@9
	float v120; // [sp+D0h] [bp-398h]@16
	float v121; // [sp+D4h] [bp-394h]@16
	float v122; // [sp+D8h] [bp-390h]@16
	float v123; // [sp+DCh] [bp-38Ch]@3
	float v124; // [sp+E0h] [bp-388h]@3
	float v125; // [sp+E4h] [bp-384h]@3
	float v126; // [sp+E8h] [bp-380h]@9
	float v127; // [sp+ECh] [bp-37Ch]@9
	float v128; // [sp+F0h] [bp-378h]@9
	RwV3D v132; // [sp+100h] [bp-368h]@9
	float v133; // [sp+10Ch] [bp-35Ch]@9
	float v134; // [sp+110h] [bp-358h]@9
	float v135; // [sp+118h] [bp-350h]@9
	float v136; // [sp+11Ch] [bp-34Ch]@9
	RwV3D b; // [sp+124h] [bp-344h]@9
	RwV3D v139; // [sp+14Ch] [bp-31Ch]@12
	RwV3D v140; // [sp+158h] [bp-310h]@14
	float v141; // [sp+164h] [bp-304h]@9
	float v142; // [sp+178h] [bp-2F0h]@16
	float v143; // [sp+17Ch] [bp-2ECh]@9
	float v144; // [sp+188h] [bp-2E0h]@9
	float v145; // [sp+19Ch] [bp-2CCh]@9
	float v146; // [sp+1TargetRadiush] [bp-2C0h]@9
	RwRGBAReal v148[30]; // [sp+1D8h] [bp-290h]@21
	RwRGBAReal v149[30]; // [sp+320h] [bp-148h]@21

	v108.x = EndPoint.x - StartPoint.x;
	a.x = v108.x;
	v108.y = EndPoint.y - StartPoint.y;
	a.y = v108.y;
	v108.z = EndPoint.z - StartPoint.z;
	a.z = v108.z;
	CLODLightManager::III::CVectorNormalize(&a);
	v12 = 0;
	EndPoint.x = a.x * 3.0f + EndPoint.x;
	EndPoint.y = a.y * 3.0f + EndPoint.y;
	EndPoint.z = a.z * 3.0f + EndPoint.z;
	v123 = a.x * 100.0f + StartPoint.x;
	v124 = a.y * 100.0f + StartPoint.y;
	v125 = a.z * 100.0f + StartPoint.z;
	v13 = (RwV3D*)GetCamPos();
	v110.x = v13->x - StartPoint.x;
	v110.y = v13->y - StartPoint.y;
	v110.z = v13->z - StartPoint.z;
	CLODLightManager::III::CVectorNormalize(&v110);
	v19 = 0;
	v107 = 0.0f;
	TempBufferIndicesStored = 0;
	TempBufferVerticesStored = 0;
	v88 = 0;
	do
	{
		b.x = 0;
		b.y = 0;
		b.z = 1.0f;
		CLODLightManager::III::CrossProduct(&out, &a, &b);
		CLODLightManager::III::CVectorNormalize(&out);
		CLODLightManager::III::CrossProduct(&v132, &out, &a);
		CLODLightManager::III::CVectorNormalize(&v132);
		v20 = (float)v88 * 0.15707964f;
		v21 = sin(v20);
		v22 = cos(v20);
		v23 = out.x * v21;
		v141 = v23;
		v24 = out.y * v21;
		v25 = out.z * v21;
		v26 = v25;
		v133 = v141 * baseRadius;
		v134 = v24 * baseRadius;
		v27 = v134 + StartPoint.y;
		v28 = v25 * baseRadius + StartPoint.z;
		v29 = v132.x * v22;
		v109 = v29;
		v30 = v132.y * v22;
		v31 = v30;
		v32 = v132.z * v22;
		v33 = v32;
		v145 = v32;
		v126 = v29 * baseRadius;
		v127 = v30 * baseRadius;
		v128 = v145 * baseRadius;
		v146 = v33;
		a4 = v126 + v133 + StartPoint.x;
		EndPointa = v127 + v27;
		v97 = v128 + v28;
		v144 = v23;
		v135 = v144 * TargetRadius;
		v136 = v24 * TargetRadius;
		v34 = v136 + v124;
		v35 = v26 * TargetRadius + v125;
		v116 = v109 * TargetRadius;
		v117 = v31 * TargetRadius;
		v118 = v33 * TargetRadius;
		v36 = v118 + v35;
		v37 = (v97 - EndPoint.z) / (v97 - v36);
		v143 = v135 + v123 + v116 - a4;
		v112 = v143 * v37;
		v113 = (v117 + v34 - EndPointa) * v37;
		v111.x = v112 + a4;
		v111.y = v113 + EndPointa;
		a6 = v111.x;
		a7 = v111.y;
		v111.z = (v36 - v97) * v37 + v97;
		v104 = v111.z;
		if (v19 && v19 != 10)
		{
			if (v19 == 20)
			{
				v139.x = v111.x;
				v139.y = v111.y;
				v139.z = v111.z;
			}
			else
			{
				if (v19 == 30)
				{
					v140.x = v111.x;
					v140.y = v111.y;
					v140.z = v111.z;
				}
			}
		}
		v38 = v111.x - a4;
		v39 = v111.y - EndPointa;
		v41 = v111.z - v97;
		v109 = v41;
		if (sqrt(v41 * v41 + v39 * v39 + v38 * v38) > 100.0f)
		{
			v115.x = v38;
			v40 = v39;
			v115.y = v40;
			v115.z = v109;
			CLODLightManager::III::CVectorNormalize(&v115);
			v142 = v115.z * 100.0f;
			v120 = v115.x * 100.0f + a4;
			a6 = v120;
			v121 = v115.y * 100.0f + EndPointa;
			a7 = v121;
			v122 = v142 + v97;
			v104 = v122;
		}
		v42 = slAlpha * 0.15000001f + 0.1f;
		v43 = TempBufferVerticesStored;
		TempVertexBuffer[TempBufferVerticesStored].objVertex.x = a4;
		v105 = v42;
		TempVertexBuffer[TempBufferVerticesStored].objVertex.y = EndPointa;
		v44 = a4 - StartPoint.x;
		TempVertexBuffer[TempBufferVerticesStored].objVertex.z = v97;
		v108.x = v44;
		TempVertexBuffer[TempBufferVerticesStored + 1].objVertex.x = a6;
		v45 = EndPointa - StartPoint.y;
		TempVertexBuffer[TempBufferVerticesStored + 1].objVertex.y = a7;
		TempVertexBuffer[TempBufferVerticesStored + 1].objVertex.z = v104;
		v108.y = v45;
		v108.z = v97 - StartPoint.z;
		CLODLightManager::III::CVectorNormalize(&v108);
		v46 = v108.x * v110.x + v108.z * v110.z + v108.y * v110.y;
		if (v46 < 0.0f)
			v46 = -v46;
		v47 = v46 * v46;
		if (v47 > v107)
			v107 = v47;
		v49 = TempBufferIndicesStored;
		*(&v149[0].red + TempBufferVerticesStored) = v105;
		*(&v149[0].green + TempBufferVerticesStored) = 0;
		*(&v148[0].red + TempBufferVerticesStored) = v47;
		*(&v148[0].green + TempBufferVerticesStored) = v47;
		if (v19 != 40)
		{
			v50 = baseRadius;
			TempBufferRenderIndexList[v49] = (short)TempBufferVerticesStored;
			TempBufferRenderIndexList[v49 + 1] = (short)(TempBufferVerticesStored + 3);
			TempBufferRenderIndexList[v49 + 2] = (short)(TempBufferVerticesStored + 1);
			v49 += 3;
			TempBufferIndicesStored = v49;
			if (v50 > 0.0f)
			{
				TempBufferRenderIndexList[v49] = (short)TempBufferVerticesStored;
				TempBufferRenderIndexList[v49 + 1] = (short)(TempBufferVerticesStored + 2);
				TempBufferRenderIndexList[v49 + 2] = (short)(TempBufferVerticesStored + 3);
				v49 += 3;
				TempBufferIndicesStored = v49;
			}
		}
		v51 = TempBufferVerticesStored + 2;
		++v19;
		TempBufferVerticesStored = v51;
		v88 = v19;
	} while (v19 <= 40);
	v52 = 0;
	if ((signed int)v51 >= 4)
	{
		v89 = 3;
		v53 = CLODLightManager::III::fSearchlightEffectVisibilityFactor / v107;
		v54 = (unsigned char *)&TempVertexBuffer[1 + CLODLightManager::III::nSmoothEffect].color;
		do
		{
			v55 = *(&v148[0].red + v52) * *(&v149[0].red + v52) * v53;
			v56 = (((unsigned __int64)(200.0f * v55) | ((unsigned __int64)(200.0f * v55) << slColorFactor2)) << slColorFactor2) + 0x80000000u;
			v57 = (unsigned __int64)(v55 * slColorFactor1);
			*((DWORD *)v54 - 9) = (DWORD)(v57 | v56);
			v59 = *(&v148[0].green + v52) * *(&v149[0].green + v52) * v53;
			v60 = (((unsigned __int64)(200.0f * v59) | ((unsigned __int64)(200.0f * v59) << slColorFactor2)) << slColorFactor2) + 0x80000000u;
			v61 = (unsigned __int64)(v59 * slColorFactor1);
			*(DWORD *)v54 = (DWORD)(v61 | v60);
			v63 = *(&v149[0].blue + v52) * *(&v148[0].blue + v52) * v53;
			v64 = (((unsigned __int64)(200.0f * v63) | ((unsigned __int64)(200.0f * v63) << slColorFactor2)) << slColorFactor2) + 0x80000000u;
			v65 = (unsigned __int64)(v63 * slColorFactor1);
			*((DWORD *)v54 + 9) = (DWORD)(v65 | v64);
			v66 = *(&v149[0].alpha + v52) * *(&v148[0].alpha + v52);
			*((DWORD *)v54 + 18) = (unsigned __int64)(v66 * v53 * 255.0f) | (((unsigned __int64)(200.0f * v66 * v53) | ((unsigned __int64)(200.0f * v66 * v53) << slColorFactor2)) << slColorFactor2) + 0x80000000u;
			v52 += 4;
			v54 = (unsigned char *)v54 + 144;
			v89 += 4;
		} while (v89 < (signed int)v51);
		v49 = TempBufferIndicesStored;
		v12 = 0;
	}
	if (v52 < (signed int)v51)
	{
		v67 = CLODLightManager::III::fSearchlightEffectVisibilityFactor / v107;
		v90 = (void *)(36 * v52 + (unsigned char *)&TempVertexBuffer[0 + CLODLightManager::III::nSmoothEffect].color);
		do
		{
			v68 = *(&v148[0].red + v52) * *(&v149[0].red + v52) * v67;
			*(unsigned __int64 *)v90 = ((unsigned __int64)(v68 * slColorFactor1) | (((unsigned __int64)(200.0 * v68) | ((unsigned __int64)(200.0 * v68) << slColorFactor2)) << slColorFactor2)) + 0x80000000u;
			++v52;
			v90 = (char *)v90 + 0x24;
		} while (v52 < (signed int)v51);
		v12 = 0;
	}

	if (v49 >(signed int)v12 && CLODLightManager::III::RwIm3DTransform(TempVertexBuffer, v51, v12, 0x18u))
	{
		CLODLightManager::III::RwIm3DRenderIndexedPrimitive(3, TempBufferRenderIndexList, TempBufferIndicesStored);
		CLODLightManager::III::RwIm3DEnd();
	}
}