#include "Sprite.h"


uint32_t  CSprite::CalcAddr;
uint32_t  CSprite::FlushAddr;
uint32_t  CSprite::RenderOneAddr;
uint32_t  CSprite::RenderBufferedAddr;
bool (*CSprite::CalcScreenCoors)(const RwV3d& vecIn, RwV3d* vecOut, float* fWidth, float* fHeight, bool bCheckFarClip, bool bCheckNearClip);

bool CSprite::CalcScreenCoorsSA(RwV3d const &vecIn, RwV3d *vecOut, float *fWidth, float *fHeight, bool bCheckFarClip, bool bCheckNearClip)
{
	return ((bool(__cdecl *)(RwV3d const &, RwV3d *, float *, float *, bool, bool))CalcAddr)
		(vecIn, vecOut, fWidth, fHeight, bCheckFarClip, bCheckNearClip);
}

bool CSprite::CalcScreenCoorsIIIVC(RwV3d const &vecIn, RwV3d *vecOut, float *fWidth, float *fHeight, bool bCheckFarClip, bool bCheckNearClip)
{
	return ((bool(__cdecl *)(RwV3d const &, RwV3d *, float *, float *, bool))CalcAddr)
		(vecIn, vecOut, fWidth, fHeight, bCheckFarClip);
}

void CSprite::FlushSpriteBuffer()
{
	return ((void(__cdecl *)())FlushAddr)();
}

void CSprite::RenderOneXLUSprite_Rotate_Aspect(float x, float y, float z, float halfWidth, float halfHeight, unsigned char red, unsigned char green, unsigned char blue, short alpha, float rhw, float rotate, unsigned char aspect)
{
	return ((void(__cdecl *)(float x, float y, float z, float halfWidth, float halfHeight, unsigned char red, unsigned char green, unsigned char blue, short alpha, float rhw, float rotate, unsigned char aspect))RenderOneAddr)
		(x, y, z, halfWidth, halfHeight, red, green, blue, alpha, rhw, rotate, aspect);
}

void CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect(float x, float y, float z, float halfWidth, float halfHeight, unsigned char red, unsigned char green, unsigned char blue, short alpha, float rhw, float rotate, unsigned char aspect)
{
	return ((void(__cdecl *)(float x, float y, float z, float halfWidth, float halfHeight, unsigned char red, unsigned char green, unsigned char blue, short alpha, float rhw, float rotate, unsigned char aspect))RenderBufferedAddr)
		(x, y, z, halfWidth, halfHeight, red, green, blue, alpha, rhw, rotate, aspect);
}