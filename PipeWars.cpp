#include "StdAfx.h"
#include "Entities.h"
#include "Resources.h"
#include "Layers/LayerChain.h"
#include "Util.h"
#include "Network/Network.h"

Player* localPlayer = NULL;
Resources resources;
Database db;
Network clientNetwork(db);
Database serverDb;
Network serverNetwork(serverDb);

// Layers have interface that allows them to both handle user input and participate in
// the rendering.  Usually, the layer will set some internal state based on the input
// and later render the coresponding output (or adjust the rendering state).
// The layers are listed in the order in which they receive user input.  If a layer
// does not wish to handle the user input it is passed to the next layer in the chain.
// Also note that the layers are completly decoupled.

LayerChain layers;

class MainMenu; extern MainMenu mainMenu;
class ScoreBoard; extern ScoreBoard scoreBoard;
class HelpScreen; extern HelpScreen helpScreen;
class HUD; extern HUD hud;
class GameLogic; extern GameLogic gameLogic;
class Renderer; extern Renderer renderer;

void InitLayers()
{
	layers.add(&mainMenu);
	layers.add(&scoreBoard);
	layers.add(&helpScreen);
	layers.add(&hud);
	layers.add(&gameLogic);
	layers.add(&renderer);
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
	fElapsedTime = min(0.1f, fElapsedTime);

	layers.FrameMove(fTime, fElapsedTime);
}

void CALLBACK OnFrameRender(IDirect3DDevice9* dev, double fTime, float fElapsedTime, void* pUserContext)
{
    dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB( 0, 25, 25, 35 ), 1.0f, 0);
    if(SUCCEEDED(dev->BeginScene())) {
		layers.Render(dev);
		dev->EndScene();
    }
}

void CALLBACK OnLostDevice(void* pUserContext)
{
	layers.ReleaseDeviceResources();
	resources.ReleaseDeviceResources();
}

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

	resources.LoadTestMap(&db);
	resources.LoadTestMap(&serverDb);
	resources.LoadMusic();

    DXUTMainLoop();

	layers.ReleaseDeviceResources();
	resources.Release();

    return DXUTGetExitCode();
}