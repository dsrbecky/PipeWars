#ifndef __Layer__
#define __Layer__

class Layer
{
public:

	// Return TRUE to indicate that the event was handled and
	// should be no longer processed

	virtual bool KeyboardProc(UINT nChar, bool bKeyDown, bool bAltDown)
	{
		return false;
	}

	virtual bool MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, int nMouseWheelDelta, int xPos, int yPos)
	{
		return false;
	}

	virtual void FrameMove(double fTime, float fElapsedTime)
	{

	}

	virtual void PreRender()
	{

	}

	virtual void Render()
	{

	}
};

#endif