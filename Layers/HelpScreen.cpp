#include "StdAfx.h"
#include "Layer.h"
#include "../Util.h"

class HelpScreen: Layer
{
	int lineHeight;
	int tabWidth;

	IDirect3DDevice9* dev;

public:

	HelpScreen(): lineHeight(16), tabWidth(120) {}
	
	void Render(IDirect3DDevice9* device)
	{
		if (true ^ keyToggled['H'] ^ keyToggled_Alt['H']) return;

		dev = device;

		RenderBlackRectangle(dev, 50, 50, 700, 450, keyToggled_Alt['X'] ? 1.0f : 0.75f);

		textX = 82; textY = 66; tabWidth = 100;

		RenderHeader("Game control:");
		RenderKey("A,S,D,W,E", "Movement");
		RenderKey("Left mouse", "Shoot");
		RenderKey("1,2,3,4,5", "Select weapon");
		RenderKey("Middle mouse", "Rotate camera");
		RenderKey("Scroll wheel", "Zoom");
		RenderKey("Tab", "Scoreboard");
		RenderKey("H", "Help");
		RenderKey("M", "Music");
		RenderKey("ESC", "Exit");
		RenderKey("Alt+Enter", "Toggle full screen");
		RenderKey("", "");
		RenderKey("", "");
		RenderKey("", "");

		RenderHeader("Credits:");
		RenderKey("Programming", "David Srbecky");
		RenderKey("3D modeling", "Alison Shaw");
		RenderKey("Music", "D. Schneidemesser");
		RenderKey("", "");


		textX = 400; textY = 66; tabWidth = 60;

		RenderHeader("Rendering:");
		RenderKey("Alt+W", "Wireframe");
		RenderKey("Alt+Z", "Z-Buffer");
		RenderKey("Alt+X", "Alpha blending");
		RenderKey("Alt+B", "Bounding boxes");
		RenderKey("Alt+N", "Other player's names");
		RenderKey("Alt+G", "Grid");
		RenderKey("Alt+C", "Frustum culling");
		RenderKey("Alt+V", "Debug frustum");
		RenderKey("Alt+F", "Frame stats");
		RenderKey("Alt+P", "Show path");
		RenderKey("Alt+L", "Show only this level");
		RenderKey("Alt+E", "Enable rendering");
		RenderKey("", "");

		RenderHeader("Lighting:");
		RenderKey("Alt+A", "Ambient");
		RenderKey("Alt+S", "Specular");
		RenderKey("Alt+D", "Diffuse");
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