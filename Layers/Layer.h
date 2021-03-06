#ifndef __Layer__
#define __Layer__

class Layer
{
public:

	static const int maxKey = 0x100;
	bool keyDown[maxKey];
	bool keyToggled[maxKey];
	bool keyToggled_Alt[maxKey];

	Layer()
	{
		ZeroMemory(keyDown, sizeof(keyDown));
		ZeroMemory(keyToggled, sizeof(keyToggled));
		ZeroMemory(keyToggled_Alt, sizeof(keyToggled_Alt));
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

	virtual bool MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, int nMouseWheelDelta, int xPos, int yPos) { return false; }
	virtual void FrameMove(double fTime, float fElapsedTime) {}
	virtual void Render(IDirect3DDevice9* dev) {}
	virtual void ReleaseDeviceResources() {}
};

#endif