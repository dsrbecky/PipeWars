#include "StdAfx.h"
#include "Layer.h"

class DebugStats: public Layer
{
	ID3DXFont* font;
public:

	void Render(IDirect3DDevice9* dev)
	{
		if (font == NULL) {
			D3DXCreateFont( dev, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
							OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
							L"Arial", &font );
		}
		RECT rect = {8, 8, 20, 20};
		font->DrawTextW(NULL, DXUTGetFrameStats(true), -1, &rect, DT_NOCLIP, D3DCOLOR_XRGB(0xff, 0xff, 0xff));
	}

	void ReleaseDeviceResources()
	{
		if (font != NULL) {
			font->Release();
			font = NULL;
		}
	}
};

DebugStats debugStats;