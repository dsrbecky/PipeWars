//
// The following file is mostly based on DirectX sample
//

#include "StdAfx.h"
#include "Database.h"
using namespace std;

Grid grid;
TextWriter textWriter;
extern Database db;

//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// created, which will happen during application initialization and windowed/full screen 
// toggles. This is the best location to create D3DPOOL_MANAGED resources since these 
// resources need to be reloaded whenever the device is destroyed. Resources created  
// here should be released in the OnDestroyDevice callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	pD3DDevice = pd3dDevice;
    return S_OK;
}

//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// reset, which will happen after a lost device scenario. This is the best location to 
// create D3DPOOL_DEFAULT resources since these resources need to be reloaded whenever 
// the device is lost. Resources created here should be released in the OnLostDevice 
// callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnResetDevice( IDirect3DDevice9* pd3dDevice,
                                const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	return S_OK;
}

//--------------------------------------------------------------------------------------
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	if (fTime > 1 && db.entities.size() == 0) {
		db.loadTestMap();
	}
}

void SetupMatrices()
{
	// View matrix
    D3DXVECTOR3 vEyePt(50.0f, 50.0f, -50.0f);
    D3DXVECTOR3 vLookatPt(20.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
    pD3DDevice->SetTransform(D3DTS_VIEW, &matView);

    // Projection matrix
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 640.0f / 480.0f, 5.0f, 400.0f);
    pD3DDevice->SetTransform(D3DTS_PROJECTION, &matProj);
}

void SetupLight()
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
	
	pD3DDevice->SetLight(0, &light);
	pD3DDevice->LightEnable(0, TRUE);
}

//--------------------------------------------------------------------------------------
// This callback function will be called at the end of every frame to perform all the 
// rendering calls for the scene, and it will also be called if the window needs to be 
// repainted. After this function has returned, DXUT will call 
// IDirect3DDevice9::Present to display the contents of the next buffer in the swap chain
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
	HRESULT hr;

    // Clear the render target and the zbuffer 
    V( pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB( 0, 45, 50, 170 ), 1.0f, 0 ) );

    // Render the scene
    if( SUCCEEDED( pd3dDevice->BeginScene() ) )
    {
		pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
        pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
	    pD3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
	    pD3DDevice->SetRenderState(D3DRS_AMBIENT, 0xffffffff);

		SetupMatrices();
		SetupLight();

		grid.Render();
		db.Render();
		textWriter.Render();
        
        V( pd3dDevice->EndScene() );
    }
}

//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// entered a lost state and before IDirect3DDevice9::Reset is called. Resources created
// in the OnResetDevice callback should be released here, which generally includes all 
// D3DPOOL_DEFAULT resources. See the "Lost Devices" section of the documentation for 
// information about lost devices.
//--------------------------------------------------------------------------------------
void CALLBACK OnLostDevice( void* pUserContext )
{
	textWriter.ReleaseDeviceResources();
	grid.ReleaseDeviceResources();
	
	map<string, Mesh*>::iterator it = loadedMeshes.begin();
	while(it != loadedMeshes.end()) {
		it->second->ReleaseDeviceResources();
		it++;
	}
}

//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// been destroyed, which generally happens as a result of application termination or 
// windowed/full screen toggles. Resources created in the OnCreateDevice callback 
// should be released here, which generally includes all D3DPOOL_MANAGED resources. 
//--------------------------------------------------------------------------------------
void CALLBACK OnDestroyDevice( void* pUserContext )
{

}

//--------------------------------------------------------------------------------------
// Before handling window messages, DXUT passes incoming windows 
// messages to the application through this callback function. If the application sets 
// *pbNoFurtherProcessing to TRUE, then DXUT will not process this message.
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    switch( uMsg ) {
        case WM_LBUTTONDOWN:
            // Capture the mouse, so if the mouse button is 
            // released outside the window, we'll get the WM_LBUTTONUP message
            SetCapture( hWnd );
            return TRUE;
        case WM_LBUTTONUP:
            ReleaseCapture();
            break;
		case WM_CAPTURECHANGED:
			if( ( HWND )lParam != hWnd )  {
                ReleaseCapture();
            }
            break;
	}
    return 0;
}

//--------------------------------------------------------------------------------------
// As a convenience, DXUT inspects the incoming windows messages for
// keystroke messages and decodes the message parameters to pass relevant keyboard
// messages to the application.  The framework does not remove the underlying keystroke 
// messages, which are still passed to the application's MsgProc callback.
//--------------------------------------------------------------------------------------
void CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown ) {
        switch( nChar ) {
            case VK_F1:
                break;
        }
    }
}

void CALLBACK MouseProc( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta, int xPos, int yPos, void* pUserContext )
{
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
INT WINAPI wWinMain( HINSTANCE, HINSTANCE, LPWSTR, int )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // Set the callback functions. These functions allow DXUT to notify
    // the application about device changes, user input, and windows messages.  The 
    // callbacks are optional so you need only set callbacks for events you're interested 
    // in. However, if you don't handle the device reset/lost callbacks then the sample 
    // framework won't be able to reset your device since the application must first 
    // release all device resources before resetting.  Likewise, if you don't handle the 
    // device created/destroyed callbacks then DXUT won't be able to 
    // recreate your device resources.
    DXUTSetCallbackD3D9DeviceCreated( OnCreateDevice );
    DXUTSetCallbackD3D9DeviceReset( OnResetDevice );
	DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackD3D9FrameRender( OnFrameRender );
    DXUTSetCallbackD3D9DeviceLost( OnLostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnDestroyDevice );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( KeyboardProc );
	DXUTSetCallbackMouse( MouseProc, true );

    // Show the cursor and clip it when in full screen
    DXUTSetCursorSettings( true, true );

    // Initialize DXUT and create the desired Win32 window and Direct3D 
    // device for the application. Calling each of these functions is optional, but they
    // allow you to set several options which control the behavior of the framework.
    DXUTInit( true, true ); // Parse the command line and show msgboxes
    DXUTSetHotkeyHandling( true, true, false );  // handle the defaul hotkeys
    DXUTCreateWindow( L"PipeWars" );
    DXUTCreateDevice( true, 640, 480 );

    // Pass control to DXUT for handling the message pump and 
    // dispatching render calls. DXUT will call your FrameMove 
    // and FrameRender callback when there is idle time between handling window messages.
    DXUTMainLoop();

    // Perform any application-level cleanup here. Direct3D device resources are released within the
    // appropriate callback functions and therefore don't require any cleanup code here.

    return DXUTGetExitCode();
}