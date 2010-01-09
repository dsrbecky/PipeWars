#include "StdAfx.h"
#include "Layer.h"

extern void RenderBlackRectangle(IDirect3DDevice9* dev, int left, int top, int width, int height, float alfa);

class HelpScreen: Layer
{
	static const int lineHeight = 16;
	static const int tabWidth = 100;

	IDirect3DDevice9* dev;
	
	void Render(IDirect3DDevice9* device)
	{
		if (true ^ keyToggled['H'] ^ keyToggled_Alt['H']) return;

		dev = device;

		RenderBlackRectangle(dev, 50, 50, 700, 450, keyToggled_Alt['X'] ? 1.0f : 0.75f);

		textX = 82; textY = 66;

		RenderHeader("Game control:");
		RenderKey("A,S,D,W", "Movement");
		RenderKey("Left mouse", "Shoot");
		RenderKey("1,2,3,4,5", "Select weapon");
		RenderKey("Middle mouse", "Rotate camera");
		RenderKey("Scroll wheel", "Zoom");
		RenderKey("Tab", "Scoreboard");
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
		RenderKey("Alt+X", "Alpha blending");
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