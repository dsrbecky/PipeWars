#ifndef __Layer__
#define __Layer__

class Layer
{
	ID3DXFont* font;

public:

	int textX;
	int textY;

	static const int maxKey = 0x100;
	bool keyDown[maxKey];
	bool keyToggled[maxKey];
	bool keyToggled_Alt[maxKey];

	Layer(): font(NULL), textX(0), textY(0)
	{
		ZeroMemory(&keyDown, sizeof(keyDown));
		ZeroMemory(&keyToggled, sizeof(keyToggled));
		ZeroMemory(&keyToggled_Alt, sizeof(keyToggled_Alt));
	}

	// Return true to indicate that the event was handled and  should be no longer processed
	virtual bool KeyboardProc(UINT nChar, bool bKeyDown, bool bAltDown)
	{
		if (nChar < maxKey) {
			if (!bAltDown) {
				keyDown[nChar] = bKeyDown;
			}
			if (bKeyDown) {
				if (bAltDown) {
					keyToggled_Alt[nChar] = !keyToggled_Alt[nChar];
				} else {
					keyToggled[nChar] = !keyToggled[nChar];
				}
			}
		}
		return false;
	}

	virtual bool MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, int nMouseWheelDelta, int xPos, int yPos)
	{
		return false;
	}

	virtual void FrameMove(double fTime, float fElapsedTime)
	{
	}

	virtual void PreRender(IDirect3DDevice9* dev)
	{
	}

	virtual void Render(IDirect3DDevice9* dev)
	{
	}
	
	virtual void ReleaseDeviceResources()
	{
		if (font != NULL) {
			font->Release();
			font = NULL;
		}
	}

	void RenderText(IDirect3DDevice9* dev, string text, int offset = 0, D3DCOLOR color = D3DCOLOR_XRGB(0xff, 0xff, 0xff))
	{
		if (font == NULL) {
			D3DXCreateFont( dev, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
							OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
							L"Arial", &font );
		}
		if (text.size() > 0) {
			RECT rect = {textX + offset, textY, textX + offset, textY};
			font->DrawTextA(NULL, text.c_str(), -1, &rect, DT_NOCLIP, color);
		}
	}
};

#endif