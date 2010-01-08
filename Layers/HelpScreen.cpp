#include "StdAfx.h"
#include "Layer.h"

class HelpScreen: Layer
{
	static const int lineHeight = 16;
	static const int tabWidth = 100;

	struct CUSTOMVERTEX {
		static const int FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE;
		FLOAT x, y, z, rhw;
		DWORD color;
	};

	IDirect3DDevice9* dev;
	
	void Render(IDirect3DDevice9* device)
	{
		if (true ^ keyToggled['H'] ^ keyToggled_Alt['H']) return;

		dev = device;

		float left = 50;
		float top = 50;
		float width = 700;
		float height = 450;
		float rhw = 1.0f;
		DWORD color = 0xB0000000;
		CUSTOMVERTEX corners[] = {
			{left, top, 0, rhw, color},
			{left + width, top, 0, rhw, color},
			{left, top + height, 0, rhw, color},
			{left + width, top + height, 0, rhw, color},
		};

	    dev->SetRenderState(D3DRS_ZENABLE, false);
		dev->SetRenderState(D3DRS_LIGHTING, false);
		dev->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		dev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
		dev->SetFVF(CUSTOMVERTEX::FVF);
		dev->SetTexture(0, NULL);
		dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, corners, sizeof(CUSTOMVERTEX));

		textX = 82; textY = 66;

		RenderHeader("Game control:");
		RenderKey("A,S,D,W", "Movement");
		RenderKey("Left mouse", "Shoot");
		RenderKey("1,2,3,4,5", "Select weapon");
		RenderKey("Middle mouse", "Rotate camera");
		RenderKey("Scroll wheel", "Zoom");
		RenderKey("H", "Help");
		RenderKey("ESC", "Exit");
		RenderKey("Alt+Enter", "Toggle full screen");
		RenderKey("", "");

		RenderHeader("Lighting:");
		RenderKey("Alt+A", "Ambient");
		RenderKey("Alt+S", "Specular");
		RenderKey("Alt+D", "Diffuse");
		RenderKey("", "");

		textX = 400; textY = 66;

		RenderHeader("Rendering:");
		RenderKey("Alt+W", "Wireframe");
		RenderKey("Alt+Z", "Z-Buffer");
		RenderKey("Alt+B", "Bounding boxes");
		RenderKey("Alt+G", "Grid");
		RenderKey("Alt+C", "Frustum culling");
		RenderKey("Alt+V", "View frustum");
		RenderKey("Alt+F", "Frame stats");
		RenderKey("", "");
	}

	void RenderHeader(string text)
	{
		RenderText(dev, text, -16, D3DCOLOR_XRGB(200, 200, 0));
		textY += 3 * lineHeight / 2;
	}

	void RenderKey(string key, string function)
	{
		RenderText(dev, key);
		RenderText(dev, function, tabWidth);
		textY += lineHeight;
	}
};

HelpScreen helpScreen;