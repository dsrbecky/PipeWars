#include "StdAfx.h"
#include "Layer.h"

// Invokes array of other layers
// Rendering is done is reverse order (last layer is rendered first)
class LayerChain: public Layer
{
public:
	vector<Layer*> layers;

	void add(void* layer)
	{
		layers.push_back((Layer*)layer);
	}

	// Return TRUE to indicate that the event was handled and
	// should be no longer processed

	bool KeyboardProc(UINT nChar, bool bKeyDown, bool bAltDown)
	{
		for(int i = 0; i < (int)layers.size(); i++) {
			bool handled = layers[i]->KeyboardProc(nChar, bKeyDown, bAltDown);
			if (handled)
				break;
		}
		return true;
	}

	bool MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, int nMouseWheelDelta, int xPos, int yPos)
	{
		for(int i = 0; i < (int)layers.size(); i++) {
			bool handled = layers[i]->MouseProc(bLeftButtonDown, bRightButtonDown, bMiddleButtonDown, nMouseWheelDelta, xPos, yPos);
			if (handled)
				break;
		}
		return true;
	}

	void FrameMove(double fTime, float fElapsedTime)
	{
		for(int i = 0; i < (int)layers.size(); i++) {
			layers[i]->FrameMove(fTime, fElapsedTime);
		}
	}

	void PreRender()
	{
		for(int i = 0; i < (int)layers.size(); i++) {
			layers[i]->PreRender();
		}
	}

	void Render()
	{
		for(int i = layers.size() - 1; i >= 0; i--) {
			layers[i]->Render();
		}
	}

	void ReleaseDeviceResources()
	{
		for(int i = 0; i < (int)layers.size(); i++) {
			layers[i]->ReleaseDeviceResources();
		}
	}
};