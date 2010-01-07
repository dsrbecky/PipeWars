#include "StdAfx.h"
#include "Database.h"
#include "Layers/LayerChain.h"

Database db;

Player* localPlayer = NULL;

// Layers have interface that allows them to both handle user input and participate in
// the rendering.  Usually, the layer will set some internal state based on the input
// and later render the coresponding output (or adjust the rendering state).
// The layers are listed in the order in which they receive user input.  If a layer
// does not wish to handle the user input it is passed to the next layer in the chain.
// Also note that the layers are completly decoupled.

LayerChain layers;

class RenderState; extern RenderState renderState;
class Camera; extern Camera camera;
class DebugStats; extern DebugStats debugStats;
class DebugGrid; extern DebugGrid debugGrid;
class PlayerControl; extern PlayerControl playerControl;
class MeshRenderer; extern MeshRenderer meshRenderer;

void InitLayers()
{
	layers.add(&renderState);
	layers.add(&camera);
	layers.add(&debugStats);
	// layers.add(&debugGrid);
	layers.add(&playerControl);
	layers.add(&meshRenderer);
}

void CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
	layers.KeyboardProc(nChar, bKeyDown, bAltDown);
}

void CALLBACK MouseProc( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta, int xPos, int yPos, void* pUserContext )
{
	layers.MouseProc(bLeftButtonDown, bRightButtonDown, bMiddleButtonDown, nMouseWheelDelta, xPos, yPos);
}

void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	layers.FrameMove(fTime, fElapsedTime);
}

void CALLBACK OnFrameRender(IDirect3DDevice9* dev, double fTime, float fElapsedTime, void* pUserContext)
{
    dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB( 0, 45, 50, 170 ), 1.0f, 0);

	layers.PreRender(dev);

    if(SUCCEEDED(dev->BeginScene())) {

		layers.Render(dev);

		dev->EndScene();
    }
}

void CALLBACK OnLostDevice(void* pUserContext)
{
	layers.ReleaseDeviceResources();
}

void RenderSplashScreen(IDirect3DDevice9* dev)
{
	string textureFilename = "..\\data\\images\\Splash.jpg";
	IDirect3DTexture9* texture;
	if (FAILED(D3DXCreateTextureFromFileA(dev, textureFilename.c_str(), &texture))) {
		MessageBoxA(NULL, ("Could not find texture " + textureFilename).c_str() , "COLLADA", MB_OK);
		exit(1);
	}

    dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB( 0, 0, 0, 0 ), 1.0f, 0);

    if(SUCCEEDED(dev->BeginScene())) {
		ID3DXSprite* sprite;
		D3DXCreateSprite(dev, &sprite);
		sprite->Begin(0);
		D3DXMATRIX scale;
		D3DXMatrixScaling(&scale, 800.0f / 1024, 600.0f / 1024, 1.0f);
		sprite->SetTransform(&scale);
		sprite->Draw(texture, NULL, NULL, NULL, 0xFFFFFFFF);
		sprite->End();
		sprite->Release();
		dev->EndScene();
    }

	dev->Present(NULL, NULL, NULL, NULL);

	texture->Release();
}

extern void releaseLoadedMeshes();

INT WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	InitLayers();

	DXUTSetCallbackKeyboard( KeyboardProc );
	DXUTSetCallbackMouse( MouseProc, true );
	DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackD3D9FrameRender( OnFrameRender );
    DXUTSetCallbackD3D9DeviceLost( OnLostDevice );
    
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen

    DXUTInit( true, true ); // Parse the command line and show msgboxes
    DXUTSetHotkeyHandling( true, true, false );  // handle the defaul hotkeys
    DXUTCreateWindow( L"PipeWars" );
    DXUTCreateDevice( true, 800, 600 );

	RenderSplashScreen(DXUTGetD3D9Device());

	db.loadTestMap();

    DXUTMainLoop();

	layers.ReleaseDeviceResources();
	db.clear();
	releaseLoadedMeshes();

    return DXUTGetExitCode();
}