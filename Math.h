#ifndef __MATH__
#define __MATH__

#include "StdAfx.h"

static const float NearClip = 2.0f;
static const float FarClip = 1000.0f;
static const double Epsilon = 0.001f;

// Rotates point cc-wise around Y
inline D3DXVECTOR3 RotateY(float angleDeg, D3DXVECTOR3 vec = D3DXVECTOR3(0, 0, 1))
{
	while(angleDeg < 0) angleDeg += 360;
	while(angleDeg >= 360) angleDeg -= 360;
	if (angleDeg ==   0) return vec;
	if (angleDeg ==  90) return D3DXVECTOR3(-vec.z, vec.y, vec.x);
	if (angleDeg == 180) return D3DXVECTOR3(-vec.x, vec.y, -vec.z);
	if (angleDeg == 270) return D3DXVECTOR3(vec.z, vec.y, -vec.x);
	
	float angleRad = angleDeg / 360 * 2 * D3DX_PI;
	float cosAlfa = cos(angleRad);
	float sinAlfa = sin(angleRad);
	float x = vec.x * cosAlfa + vec.z * -sinAlfa;   // cos -sin   vec.x
	float z = vec.x * sinAlfa + vec.z *  cosAlfa;   // sin  cos   vec.z
	return D3DXVECTOR3(x, vec.y, z);
}

// Returns number defining on which side of line the point P lies
inline float WhichSideOfLine(float px, float py, float x1, float y1, float x2, float y2)
{
	// Cross product with origin (x1, y1)
	return ((px - x1) * (y2 - y1)) - ((py - y1) * (x2 - x1));
}

// Returns true if Y-ray intersects triangle a-b-c
inline bool Intersect_YRay_Triangle(float x, float z, float ax, float az, float bx, float bz, float cx, float cz)
{
	float i = WhichSideOfLine(x, z, ax, az, bx, bz);
	float j = WhichSideOfLine(x, z, bx, bz, cx, cz);
	float k = WhichSideOfLine(x, z, cx, cz, ax, az);
	
	// If the point is on the same side of each of the lines then it is in the triangle.
	return (i >= 0 && j >= 0 && k >= 0) || (i <= 0 && j <= 0 && k <= 0);
}

inline void Intersect_YRay_Plane(float x, float z, float* outY, D3DXVECTOR3* a, D3DXVECTOR3* b, D3DXVECTOR3* c)
{
	D3DXVECTOR3 p = *b - *a;
	D3DXVECTOR3 q = *c - *a;
	D3DXVECTOR3 norm;
	D3DXVec3Cross(&norm, &p, &q);

	D3DXVECTOR3 origin = D3DXVECTOR3(x, 0, z);
	D3DXVECTOR3 originToPlane = *a - origin;
	float distOriginToPlane = D3DXVec3Dot(&originToPlane, &norm);
	float distOneUp = norm.y; // = (0, 1, 0) * norm
	*outY = distOriginToPlane / distOneUp;
}

inline D3DXVECTOR3 min3(D3DXVECTOR3 a, D3DXVECTOR3 b)
{
	return D3DXVECTOR3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}

inline D3DXVECTOR3 max3(D3DXVECTOR3 a, D3DXVECTOR3 b)
{
	return D3DXVECTOR3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}

inline D3DXVECTOR3 GetFrustumNormal(D3DXVECTOR3 near1, D3DXVECTOR3 near2, D3DXVECTOR3 far1)
{
	D3DXVECTOR3 p = near2 - near1;
	D3DXVECTOR3 q = far1 - near1;
	D3DXVECTOR3 n;
	D3DXVec3Cross(&n, &p, &q);
	return n;
}

#endif