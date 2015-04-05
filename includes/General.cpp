#include "stdafx.h"
#include "General.h"
#include <math.h>


void CMatrix::SetRotateXOnly(float fAngle)
{
	matrix.right.x = 1.0f;
	matrix.right.y = 0.0f;
	matrix.right.z = 0.0f;

	matrix.up.x = 0.0f;
	matrix.up.y = cos(fAngle);
	matrix.up.z = sin(fAngle);

	matrix.at.x = 0.0f;
	matrix.at.y = -sin(fAngle);
	matrix.at.z = cos(fAngle);
}

void CMatrix::SetRotateYOnly(float fAngle)
{
	matrix.right.x = cos(fAngle);
	matrix.right.y = 0.0f;
	matrix.right.z = sin(fAngle);

	matrix.up.x = 0.0f;
	matrix.up.y = 1.0f;
	matrix.up.z = 0.0f;

	matrix.at.x = -sin(fAngle);
	matrix.at.y = 0.0f;
	matrix.at.z = cos(fAngle);
}

void CMatrix::SetRotateZOnly(float fAngle)
{
	matrix.at.x = 0.0f;
	matrix.at.y = 0.0f;
	matrix.at.z = 1.0f;

	matrix.up.x = -sin(fAngle);
	matrix.up.y = cos(fAngle);
	matrix.up.z = 0.0f;

	matrix.right.x = cos(fAngle);
	matrix.right.y = sin(fAngle);
	matrix.right.z = 0.0f;
}

void CMatrix::SetRotateOnly(float fAngleX, float fAngleY, float fAngleZ)
{
	matrix.right.x = cos(fAngleZ) * cos(fAngleY) - sin(fAngleZ) * sin(fAngleX) * sin(fAngleY);
	matrix.right.y = cos(fAngleZ) * sin(fAngleX) * sin(fAngleY) + sin(fAngleZ) * cos(fAngleY);
	matrix.right.z = -cos(fAngleX) * sin(fAngleY);

	matrix.up.x = -sin(fAngleZ) * cos(fAngleX);
	matrix.up.y = cos(fAngleZ) * cos(fAngleX);
	matrix.up.z = sin(fAngleX);

	matrix.at.x = sin(fAngleZ) * sin(fAngleX) * cos(fAngleY) + cos(fAngleZ) * sin(fAngleY);
	matrix.at.y = sin(fAngleZ) * sin(fAngleY) - cos(fAngleZ) * sin(fAngleX) * cos(fAngleY);
	matrix.at.z = cos(fAngleX) * cos(fAngleY);
}