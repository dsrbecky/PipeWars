#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"
#include <iomanip>

extern float hiQualityPipes;
extern float targetFPS;
extern int stat_objRendered;

extern Player* localPlayer;

class DebugStats: public InputLayer
{
	ID3DXFont* font;
public:

	void Render(IDirect3DDevice9* dev)
	{
		if (keyToggled_Alt['X']) return;

		if (font == NULL) {
			D3DXCreateFont( dev, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
							OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
							L"Arial", &font );
		}
		RECT rect = {8, 8, 20, 20};
		ostringstream msg;
		msg << fixed << std::setprecision(1);
		msg << "FPS = " << DXUTGetFPS() << " (target = " << targetFPS << ")" << "    ";
		msg << "Objects = " << stat_objRendered << " (hq = " << (int)hiQualityPipes << ")" << "    ";
		msg << "Pos = " << localPlayer->position.x << ","<< localPlayer->position.y << ","<< localPlayer->position.z << "    ";
		msg << "Press H for help or ESC to exit.";
		
		font->DrawTextA(NULL, msg.str().c_str(), -1, &rect, DT_NOCLIP, D3DCOLOR_XRGB(0xff, 0xff, 0xff));
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