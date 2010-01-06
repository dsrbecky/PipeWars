#ifndef __Layer__
#define __Layer__

class IDirect3DDevice9;

class Layer
{
public:
	// Return TRUE to indicate that the event was handled and  should be no longer processed
	virtual bool KeyboardProc(UINT nChar, bool bKeyDown, bool bAltDown) { return false; }
	virtual bool MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, int nMouseWheelDelta, int xPos, int yPos) { return false; }
	virtual void FrameMove(double fTime, float fElapsedTime) {}
	virtual void PreRender(IDirect3DDevice9* dev) {}
	virtual void Render(IDirect3DDevice9* dev) {}
	virtual void ReleaseDeviceResources() {}
};

#endif