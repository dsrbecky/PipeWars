#include "StdAfx.h"
#include "Resources.h"

extern Resources resources;

static int textX = 0;
static int textY = 0;

struct CUSTOMVERTEX {
	static const int FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE;
	FLOAT x, y, z, rhw;
	DWORD color;
};

inline void RenderBlackRectangle(IDirect3DDevice9* dev, int left, int top, int width, int height, float alfa)
{
	DWORD color = (int)(alfa * 255) << 24;
	CUSTOMVERTEX corners[] = {
		{(float)left, (float)top, 0, 1.0f, color},
		{(float)left + (float)width, (float)top, 0, 1.0f, color},
		{(float)left, (float)top + (float)height, 0, 1.0f, color},
		{(float)left + (float)width, (float)top + (float)height, 0, 1.0f, color},
	};

    dev->SetRenderState(D3DRS_ZENABLE, false);
	dev->SetRenderState(D3DRS_ALPHABLENDENABLE, alfa != 1.0f);
	dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	dev->SetFVF(CUSTOMVERTEX::FVF);
	dev->SetTexture(0, NULL);

	dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, corners, sizeof(CUSTOMVERTEX));

	dev->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
}

static void RenderSplashScreen(IDirect3DDevice9* dev)
{
	IDirect3DTexture9* texture = resources.LoadTexture(dev, "Splash.jpg");

    dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB( 0, 0, 0, 0 ), 1.0f, 0);

    if(SUCCEEDED(dev->BeginScene())) {
		ID3DXSprite* sprite;
		D3DXCreateSprite(dev, &sprite);
		sprite->Begin(0);
		D3DXMATRIX scale;
		D3DXMatrixScaling(&scale, 800.0f / 1024, 600.0f / 1024, 1.0f);
		sprite->SetTransform(&scale);
		sprite->Draw(texture, NULL, NULL, NULL, 0xFFFFFFFF);
		sprite->End();
		sprite->Release();
		dev->EndScene();
    }

	dev->Present(NULL, NULL, NULL, NULL);
}

static void RenderText(IDirect3DDevice9* dev, string text, int offset = 0, D3DCOLOR color = D3DCOLOR_XRGB(0xff, 0xff, 0xff))
{
	if (text.size() > 0) {
		RECT rect = {textX + offset, textY, textX + offset, textY};
		resources.LoadFont(dev)->DrawTextA(NULL, text.c_str(), -1, &rect, DT_NOCLIP, color);
	}
}