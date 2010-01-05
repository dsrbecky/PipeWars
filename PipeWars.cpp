#include "StdAfx.h"
#include "Database.h"
#include "ColladaImport.h"
using namespace std;

HRESULT InitD3D( HWND hWnd )
{
    if( NULL == ( pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
        return E_FAIL;

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof( d3dpp ) );
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    if( FAILED( pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &d3dpp, &pD3DDevice ) ) ) {
        return E_FAIL;
    }

    pD3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE );
    pD3DDevice->SetRenderState(D3DRS_AMBIENT, 0xffffffff);

    return S_OK;
}

Mesh* suzzane;

void InitGeometry()
{
	suzzane = loadMesh("..\\data\\meshes\\suzanne.dae", "Suzanne-Geometry");
}

void Cleanup()
{
    if(pD3DDevice != NULL)
        pD3DDevice->Release();

    if(pD3D != NULL)
        pD3D->Release();
}

void SetupMatrices()
{
    // World matrix
    D3DXMATRIXA16 matWorld;
    D3DXMatrixRotationY(&matWorld, timeGetTime() / 1000.0f);
    pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);

	// View matrix
    D3DXVECTOR3 vEyePt(0.0f, 3.0f,-5.0f);
    D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
    pD3DDevice->SetTransform(D3DTS_VIEW, &matView);

    // Projection matrix
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 600.0f / 400.0f, 1.0f, 100.0f);
    pD3DDevice->SetTransform(D3DTS_PROJECTION, &matProj);
}

void Render()
{
    pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

    if(SUCCEEDED(pD3DDevice->BeginScene())) {
        SetupMatrices();

		suzzane->Render();

        pD3DDevice->EndScene();
    }

    pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch(msg) {
        case WM_DESTROY:
            Cleanup();
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
INT WINAPI wWinMain( HINSTANCE hInst, HINSTANCE, LPWSTR, INT )
{
    // Register the window class
    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
        GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
        L"PipeWars", NULL
    };
    RegisterClassEx(&wc);

    // Create the application's window
    HWND hWnd = CreateWindow( L"PipeWars", L"PipeWars",
                              WS_OVERLAPPEDWINDOW, 100, 100, 600, 400,
                              NULL, NULL, wc.hInstance, NULL );

    // Initialize Direct3D
    if( SUCCEEDED( InitD3D( hWnd ) ) )
    {
        // Create the scene geometry
        InitGeometry();
        
        // Show the window
        ShowWindow( hWnd, SW_SHOWDEFAULT );
        UpdateWindow( hWnd );

        // Enter the message loop
        MSG msg;
        ZeroMemory(&msg, sizeof(msg));
        while(msg.message != WM_QUIT) {
            if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
			} else {
                Render();
			}
        }
    }

    UnregisterClass(L"PipeWars", wc.hInstance);
    return 0;
}