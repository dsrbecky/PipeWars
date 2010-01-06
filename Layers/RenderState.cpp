#include "StdAfx.h"
#include "Layer.h"

class RenderState: public InputLayer
{
public:
	void SetupLight(IDirect3DDevice9* dev)
	{
		D3DLIGHT9 light;
		ZeroMemory(&light, sizeof(light));
		
		D3DCOLORVALUE white = {1, 1, 1, 1};
		light.Type = D3DLIGHT_DIRECTIONAL;
		light.Diffuse  = white;
		light.Ambient  = white;
		light.Specular = white;

		light.Direction.x = 250.0f;
		light.Direction.y = 5000.0f;
		light.Direction.z = -250.0f;
		    
		// Don't attenuate.
		light.Attenuation0 = 1.0f; 
		light.Range        = 10000.0f;
		
		dev->SetLight(0, &light);
		dev->LightEnable(0, TRUE);
	}

	void PreRender(IDirect3DDevice9* dev)
	{
		dev->SetRenderState(D3DRS_FILLMODE, keyToggled['W'] ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
        dev->SetRenderState(D3DRS_LIGHTING, !keyToggled['L']);
	    dev->SetRenderState(D3DRS_ZENABLE, !keyToggled['Z']);
	    dev->SetRenderState(D3DRS_AMBIENT, 0x40404040);

		SetupLight(dev);
	}
};

RenderState renderState;