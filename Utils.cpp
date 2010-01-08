#include "StdAfx.h"

struct CUSTOMVERTEX {
	static const int FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE;
	FLOAT x, y, z, rhw;
	DWORD color;
};

void RenderBlackRectangle(IDirect3DDevice9* dev, int left, int top, int width, int height, float alfa = 0.75)
{
	DWORD color = (int)(alfa * 255) << 24;
	CUSTOMVERTEX corners[] = {
		{(float)left, (float)top, 0, 1.0f, color},
		{(float)left + (float)width, (float)top, 0, 1.0f, color},
		{(float)left, (float)top + (float)height, 0, 1.0f, color},
		{(float)left + (float)width, (float)top + (float)height, 0, 1.0f, color},
	};

    dev->SetRenderState(D3DRS_ZENABLE, false);
	dev->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	dev->SetFVF(CUSTOMVERTEX::FVF);
	dev->SetTexture(0, NULL);

	dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, corners, sizeof(CUSTOMVERTEX));

	dev->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
}