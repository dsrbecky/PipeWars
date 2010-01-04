#include "StdAfx.h"
using namespace std;

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
LPDIRECT3D9         g_pD3D = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9   g_pd3dDevice = NULL; // Our rendering device

LPD3DXMESH          g_pMesh = NULL; // Our mesh object in sysmem
D3DMATERIAL9*       g_pMeshMaterials = NULL; // Materials for our mesh
LPDIRECT3DTEXTURE9* g_pMeshTextures = NULL; // Textures for our mesh
DWORD               g_dwNumMaterials = 0L;   // Number of mesh materials




//-----------------------------------------------------------------------------
// Name: InitD3D()
// Desc: Initializes Direct3D
//-----------------------------------------------------------------------------
HRESULT InitD3D( HWND hWnd )
{
    // Create the D3D object.
    if( NULL == ( g_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
        return E_FAIL;

    // Set up the structure used to create the D3DDevice. Since we are now
    // using more complex geometry, we will create a device with a zbuffer.
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof( d3dpp ) );
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    // Create the D3DDevice
    if( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &d3dpp, &g_pd3dDevice ) ) )
    {
        return E_FAIL;
    }

    // Turn on the zbuffer
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

    // Turn on ambient lighting 
    g_pd3dDevice->SetRenderState( D3DRS_AMBIENT, 0xffffffff );

    return S_OK;
}

class Tristrip
{
public:
	int vbOffset;
	int vbCount;
	string material;
	DWORD fvf;
};

class Mesh
{
public:
	vector<Tristrip> tristrips;
	IDirect3DVertexBuffer9* vb;
};

Mesh suzzane;

//-----------------------------------------------------------------------------
// Name: InitGeometry()
// Desc: Load the mesh and build the material and texture arrays
//-----------------------------------------------------------------------------
HRESULT InitGeometry()
{
	DAE dae;
	domCOLLADA* doc = dae.open("..\\data\\meshes\\suzanne.dae");
	if (doc == NULL) {
		MessageBox(NULL, L"Could open suzanne.dae", L"COLLADA", MB_OK);
        return E_FAIL;
	}
	
	domGeometry_Array geoms = doc->getLibrary_geometries_array().get(0)->getGeometry_array();
	domMeshRef mesh;
	for(u_int i = 0; i < geoms.getCount(); i++) {
		if (geoms.get(i)->getName() == std::string("Suzanne-Geometry")) {
			mesh = geoms.get(i)->getMesh();
			break;
		}
	}

	Mesh outMesh;
	vector<float> vb;
	DWORD fvf = 0;
	
	for(u_int i = 0; i < mesh->getTristrips_array().getCount(); i++) {
		domTristripsRef tristrips = mesh->getTristrips_array().get(i);

		int posOffset = -1; domListOfFloats* posSrc;
		int norOffset = -1; domListOfFloats* norSrc;
		int colOffset = -1; domListOfFloats* colSrc;
		int texOffset = -1; domListOfFloats* texSrc;

		for(u_int j = 0; j < tristrips->getInput_array().getCount(); j++) {
			domInputLocalOffsetRef input = tristrips->getInput_array().get(j);
			daeElementRef source = input->getSource().getElement();
			if (source->typeID() == domVertices::ID()) {
				source = daeSafeCast<domVertices>(source)->getInput_array().get(0)->getSource().getElement();
			}
			int offset = (int)input->getOffset();
			domListOfFloats* src = &(daeSafeCast<domSource>(source)->getFloat_array()->getValue());
			if (input->getSemantic() == string("VERTEX")) {
				posOffset = offset;
				posSrc = src;
				fvf = fvf | D3DFVF_XYZ;
			}
			if (input->getSemantic() == string("NORMAL")) {
				norOffset = offset;
				norSrc = src;
				fvf = fvf | D3DFVF_NORMAL;
			}
			if (input->getSemantic() == string("COLOR")) {
				colOffset = offset;
				colSrc = src;
				fvf = fvf | D3DFVF_DIFFUSE;
			}
			if (input->getSemantic() == string("TEXCOORD")) {
				texOffset = offset;
				texSrc = src;
				fvf = fvf | D3DFVF_TEX2;
			}
		}

		int stride = 0;
		stride = max(stride, posOffset + 1);
		stride = max(stride, norOffset + 1);
		stride = max(stride, colOffset + 1);
		stride = max(stride, texOffset + 1);

		for(u_int j = 0; j < tristrips->getP_array().getCount(); j++) {
			domListOfUInts p = tristrips->getP_array().get(j)->getValue();

			Tristrip outTs;
			outTs.vbOffset = vb.size();
			outTs.vbCount = (p.getCount() / stride) - 2;
			outTs.material = tristrips->getMaterial();
			outTs.fvf = fvf;
			outMesh.tristrips.push_back(outTs);

			for(u_int k = 0; k < p.getCount(); k += stride) {
				if (posOffset != -1) {
					int index = (int)p.get(k + posOffset);
					vb.push_back((float)posSrc->get(3 * index + 0));
					vb.push_back((float)posSrc->get(3 * index + 1));
					vb.push_back((float)posSrc->get(3 * index + 2));
				}

				if (norOffset != -1) {
					int index = (int)p.get(k + norOffset);
					vb.push_back((float)norSrc->get(3 * index + 0));
					vb.push_back((float)norSrc->get(3 * index + 1));
					vb.push_back((float)norSrc->get(3 * index + 2));
				}

				if (colOffset != -1) {
					int index = (int)p.get(k + colOffset);
					vb.push_back((float)colSrc->get(4 * index + 3)); // A
					vb.push_back((float)colSrc->get(4 * index + 0)); // R
					vb.push_back((float)colSrc->get(4 * index + 1)); // G
					vb.push_back((float)colSrc->get(4 * index + 2)); // B
				}

				if (texOffset != -1) {
					int index = (int)p.get(k + texOffset);
					vb.push_back((float)texSrc->get(2 * index + 0));
					vb.push_back((float)texSrc->get(2 * index + 1));
				}
			}
		}
	}

	g_pd3dDevice->CreateVertexBuffer(vb.size() * sizeof(float), 0, fvf, D3DPOOL_DEFAULT, &outMesh.vb, NULL);
	void* vbData;
	outMesh.vb->Lock(0, vb.size() * sizeof(float), &vbData, 0);
	copy(vb.begin(), vb.end(), (float*)vbData);
	outMesh.vb->Unlock();

	suzzane = outMesh;

    LPD3DXBUFFER pD3DXMtrlBuffer;
	
    // Load the mesh from the specified file
    if( FAILED( D3DXLoadMeshFromX( L"Tiger.x", D3DXMESH_SYSTEMMEM,
                                   g_pd3dDevice, NULL,
                                   &pD3DXMtrlBuffer, NULL, &g_dwNumMaterials,
                                    &g_pMesh ) ) )
    {
        // If model is not in current folder, try parent folder
        if( FAILED( D3DXLoadMeshFromX( L"..\\Tiger.x", D3DXMESH_SYSTEMMEM,
                                       g_pd3dDevice, NULL,
                                       &pD3DXMtrlBuffer, NULL, &g_dwNumMaterials,
                                       &g_pMesh ) ) )
        {
            MessageBox( NULL, L"Could not find tiger.x", L"Meshes.exe", MB_OK );
            return E_FAIL;
        }
    }

    // We need to extract the material properties and texture names from the 
    // pD3DXMtrlBuffer
    D3DXMATERIAL* d3dxMaterials = ( D3DXMATERIAL* )pD3DXMtrlBuffer->GetBufferPointer();
    g_pMeshMaterials = new D3DMATERIAL9[g_dwNumMaterials];
    if( g_pMeshMaterials == NULL )
        return E_OUTOFMEMORY;
    g_pMeshTextures = new LPDIRECT3DTEXTURE9[g_dwNumMaterials];
    if( g_pMeshTextures == NULL )
        return E_OUTOFMEMORY;

    for( DWORD i = 0; i < g_dwNumMaterials; i++ )
    {
        // Copy the material
        g_pMeshMaterials[i] = d3dxMaterials[i].MatD3D;

        // Set the ambient color for the material (D3DX does not do this)
        g_pMeshMaterials[i].Ambient = g_pMeshMaterials[i].Diffuse;

        g_pMeshTextures[i] = NULL;
        if( d3dxMaterials[i].pTextureFilename != NULL &&
            lstrlenA( d3dxMaterials[i].pTextureFilename ) > 0 )
        {
            // Create the texture
            if( FAILED( D3DXCreateTextureFromFileA( g_pd3dDevice,
                                                    d3dxMaterials[i].pTextureFilename,
                                                    &g_pMeshTextures[i] ) ) )
            {
                // If texture is not in current folder, try parent folder
                const CHAR* strPrefix = "..\\";
                CHAR strTexture[MAX_PATH];
                strcpy_s( strTexture, MAX_PATH, strPrefix );
                strcat_s( strTexture, MAX_PATH, d3dxMaterials[i].pTextureFilename );
                // If texture is not in current folder, try parent folder
                if( FAILED( D3DXCreateTextureFromFileA( g_pd3dDevice,
                                                        strTexture,
                                                        &g_pMeshTextures[i] ) ) )
                {
                    MessageBox( NULL, L"Could not find texture map", L"Meshes.exe", MB_OK );
                }
            }
        }
    }

    // Done with the material buffer
    pD3DXMtrlBuffer->Release();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Cleanup()
// Desc: Releases all previously initialized objects
//-----------------------------------------------------------------------------
VOID Cleanup()
{
    if( g_pMeshMaterials != NULL )
        delete[] g_pMeshMaterials;

    if( g_pMeshTextures )
    {
        for( DWORD i = 0; i < g_dwNumMaterials; i++ )
        {
            if( g_pMeshTextures[i] )
                g_pMeshTextures[i]->Release();
        }
        delete[] g_pMeshTextures;
    }
    if( g_pMesh != NULL )
        g_pMesh->Release();

    if( g_pd3dDevice != NULL )
        g_pd3dDevice->Release();

    if( g_pD3D != NULL )
        g_pD3D->Release();
}



//-----------------------------------------------------------------------------
// Name: SetupMatrices()
// Desc: Sets up the world, view, and projection transform matrices.
//-----------------------------------------------------------------------------
VOID SetupMatrices()
{
    // Set up world matrix
    D3DXMATRIXA16 matWorld;
    D3DXMatrixRotationY( &matWorld, timeGetTime() / 1000.0f );
    g_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

    // Set up our view matrix. A view matrix can be defined given an eye point,
    // a point to lookat, and a direction for which way is up. Here, we set the
    // eye five units back along the z-axis and up three units, look at the 
    // origin, and define "up" to be in the y-direction.
    D3DXVECTOR3 vEyePt( 0.0f, 3.0f,-5.0f );
    D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
    g_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

    // For the projection matrix, we set up a perspective transform (which
    // transforms geometry from 3D view space to 2D viewport space, with
    // a perspective divide making objects smaller in the distance). To build
    // a perpsective transform, we need the field of view (1/4 pi is common),
    // the aspect ratio, and the near and far clipping planes (which define at
    // what distances geometry should be no longer be rendered).
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f );
    g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
}




//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Draws the scene
//-----------------------------------------------------------------------------
VOID Render()
{
    // Clear the backbuffer and the zbuffer
    g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                         D3DCOLOR_XRGB( 0, 0, 255 ), 1.0f, 0 );

    // Begin the scene
    if( SUCCEEDED( g_pd3dDevice->BeginScene() ) )
    {
        // Setup the world, view, and projection matrices
        SetupMatrices();

        // Meshes are divided into subsets, one for each material. Render them in
        // a loop
        for( DWORD i = 0; i < g_dwNumMaterials; i++ )
        {
            // Set the material and texture for this subset
            g_pd3dDevice->SetMaterial( &g_pMeshMaterials[i] );
            g_pd3dDevice->SetTexture( 0, g_pMeshTextures[i] );

            // Draw the mesh subset
            // g_pMesh->DrawSubset( i );
        }

		g_pd3dDevice->SetStreamSource(0, suzzane.vb, 0, 12 * sizeof(float));
		for(int i = 0; i < suzzane.tristrips.size(); i++) {
			Tristrip ts = suzzane.tristrips[i];
			g_pd3dDevice->SetFVF(ts.fvf);
			g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, ts.vbOffset / 12, ts.vbCount);
		}

        // End the scene
        g_pd3dDevice->EndScene();
    }

    // Present the backbuffer contents to the display
    g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: The window's message handler
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_DESTROY:
            Cleanup();
            PostQuitMessage( 0 );
            return 0;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}




//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
INT WINAPI wWinMain( HINSTANCE hInst, HINSTANCE, LPWSTR, INT )
{
    // Register the window class
    WNDCLASSEX wc =
    {
        sizeof( WNDCLASSEX ), CS_CLASSDC, MsgProc, 0L, 0L,
        GetModuleHandle( NULL ), NULL, NULL, NULL, NULL,
        L"D3D Tutorial", NULL
    };
    RegisterClassEx( &wc );

    // Create the application's window
    HWND hWnd = CreateWindow( L"D3D Tutorial", L"D3D Tutorial 06: Meshes",
                              WS_OVERLAPPEDWINDOW, 100, 100, 300, 300,
                              NULL, NULL, wc.hInstance, NULL );

    // Initialize Direct3D
    if( SUCCEEDED( InitD3D( hWnd ) ) )
    {
        // Create the scene geometry
        if( SUCCEEDED( InitGeometry() ) )
        {
            // Show the window
            ShowWindow( hWnd, SW_SHOWDEFAULT );
            UpdateWindow( hWnd );

            // Enter the message loop
            MSG msg;
            ZeroMemory( &msg, sizeof( msg ) );
            while( msg.message != WM_QUIT )
            {
                if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
                {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                }
                else
                    Render();
            }
        }
    }

    UnregisterClass( L"D3D Tutorial", wc.hInstance );
    return 0;
}



