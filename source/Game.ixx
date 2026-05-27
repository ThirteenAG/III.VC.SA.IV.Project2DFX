module;

#include <stdafx.h>
#include "rw.h"

export module Game;

import Misc;
import Entity;

export namespace CGame
{
    GameRef<int> currArea;
}

export namespace CWorld
{
    float(__cdecl* FindGroundZFor3DCoordCRGO)(float x, float y, float z, bool* pCollisionResult, CEntity** pGroundObject) = nullptr;
    float(__cdecl* FindGroundZFor3DCoordCR)(float x, float y, float z, bool* pCollisionResult) = nullptr;

    float __cdecl FindGroundZFor3DCoord(float x, float y, float z, bool* pCollisionResult, CEntity** pGroundObject)
    {
        if (FindGroundZFor3DCoordCRGO)
            return FindGroundZFor3DCoordCRGO(x, y, z, pCollisionResult, pGroundObject);
        else
            return FindGroundZFor3DCoordCR(x, y, z, pCollisionResult);
    }
}

export RxObjSpace3dVertex* (__cdecl* RwIm3DTransform)(RxObjSpace3dVertex* pVerts, unsigned int numVerts, RwMatrix* ltm, unsigned int flags) = nullptr;
export int(__cdecl* RwIm3DRenderIndexedPrimitive)(int primType, short* indices, int numIndices) = nullptr;
export int(__cdecl* RwIm3DEnd)() = nullptr;
