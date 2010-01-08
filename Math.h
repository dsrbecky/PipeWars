#include "StdAfx.h"

static const double Epsilon = 0.001f;

inline D3DXVECTOR3 GetFrustumNormal(D3DXVECTOR3 near1, D3DXVECTOR3 near2, D3DXVECTOR3 far1)
{
	D3DXVECTOR3 p = near2 - near1;
	D3DXVECTOR3 q = far1 - near1;
	D3DXVECTOR3 n;
	D3DXVec3Cross(&n, &p, &q);
	return n;
}