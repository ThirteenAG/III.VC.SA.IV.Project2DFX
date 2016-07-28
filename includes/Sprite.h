#ifndef __SPRITE
#define __SPRITE
#include "..\includes\stdafx.h"

class CSprite
{
public:
	static uint32_t CalcAddr;
	static uint32_t FlushAddr;
	static uint32_t RenderOneAddr;
	static uint32_t RenderBufferedAddr;

	static bool		(*CalcScreenCoors)(const RwV3d& vecIn, RwV3d* vecOut, float* fWidth, float* fHeight, bool bCheckFarClip, bool bCheckNearClip);
	static bool		CalcScreenCoorsSA(const RwV3d& vecIn, RwV3d* vecOut, float* fWidth, float* fHeight, bool bCheckFarClip, bool bCheckNearClip);
	static bool		CalcScreenCoorsIIIVC(const RwV3d& vecIn, RwV3d* vecOut, float* fWidth, float* fHeight, bool bCheckFarClip, bool bCheckNearClip);
	static void		FlushSpriteBuffer();

	static void		RenderOneXLUSprite_Rotate_Aspect(float x, float y, float z, float halfWidth, float halfHeight, 
						unsigned char red, unsigned char green, unsigned char blue, short alpha, float rhw, 
						float rotate, unsigned char aspect);

	static void		RenderBufferedOneXLUSprite_Rotate_Aspect(float x, float y, float z, float halfWidth, float halfHeight, 
						unsigned char red, unsigned char green, unsigned char blue, short alpha, float rhw, 
						float rotate, unsigned char aspect);
};
#endif