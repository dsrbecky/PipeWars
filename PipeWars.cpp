#include "StdAfx.h"
using namespace std;

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
LPDIRECT3D9         pD3D = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9   pD3DDevice = NULL; // Our rendering device

//-----------------------------------------------------------------------------
// Name: InitD3D()
// Desc: Initializes Direct3D
//-----------------------------------------------------------------------------
HRESULT InitD3D( HWND hWnd )
{
    // Create the D3D object.
    if( NULL == ( pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
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
    if( FAILED( pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &d3dpp, &pD3DDevice ) ) )
    {
        return E_FAIL;
    }

    // Turn on the zbuffer
    pD3DDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

    // Turn on ambient lighting 
    pD3DDevice->SetRenderState( D3DRS_AMBIENT, 0xffffffff );

    return S_OK;
}

// Define one or more tristrips that share same material
class Tristrip
{
public:
	DWORD fvf;
	int vbStride; // in bytes
	IDirect3DVertexBuffer9* vb;
	vector<int> vertexCounts; // framgmentation of tristrips into groups
	D3DMATERIAL9 material;
	IDirect3DTexture9* texure;

	void Render()
	{
		pD3DDevice->SetStreamSource(0, vb, 0, vbStride);
		pD3DDevice->SetFVF(fvf);
		pD3DDevice->SetTexture(0, texure);
		pD3DDevice->SetMaterial(&material);
		int start = 0;
		for(int j = 0; j < (int)vertexCounts.size(); j++) {
			int vertexCount = vertexCounts[j];
			pD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, start, vertexCount - 2);
			start += vertexCount;
		}
	}
};

class Mesh
{
public:
	vector<Tristrip> tristrips;

	void Render()
	{
		for(int i = 0; i < (int)tristrips.size(); i++) {
			tristrips[i].Render();
		}
	}
};

DAE dae;

map<string, domCOLLADA*> loadedFiles;

domCOLLADA* loadCollada(string filename)
{
	// Cached
	if (loadedFiles.count(filename) > 0) {
		return loadedFiles[filename];
	} else {
		domCOLLADA* doc = dae.open(filename);
		if (doc == NULL) {
			MessageBoxA(NULL, ("Could not open " + filename).c_str(), "COLLADA", MB_OK);
			exit(1);
		}
		loadedFiles[filename] = doc;
		return doc;
	}
}

map<string, Mesh*> loadedMeshes;

Mesh* loadMesh(string filename, string geometryName)
{
	string filenameAndGeometryName = filename + "\\" + geometryName;
	// Cached
	if (loadedMeshes.count(filenameAndGeometryName) > 0) {
		return loadedMeshes[filenameAndGeometryName];
	}

	domCOLLADA* doc = loadCollada(filename);

	// Get the mesh in COLLADA file
	domGeometry_Array geoms = doc->getLibrary_geometries_array().get(0)->getGeometry_array();
	domMeshRef meshRef;
	for(u_int i = 0; i < geoms.getCount(); i++) {
		if (geoms.get(i)->getName() == geometryName) {
			meshRef = geoms.get(i)->getMesh();
			break;
		}
	}
	if (meshRef == NULL) {
		MessageBoxA(NULL, ("Could not mesh " + filenameAndGeometryName).c_str(), "COLLADA", MB_OK);
		exit(1);
	}

	Mesh* mesh = new Mesh();
	loadedMeshes[filenameAndGeometryName] = mesh;  // Cache
	
	// Load the <tristrips/> elements
	// (other types are ignored for now)

	for(u_int i = 0; i < meshRef->getTristrips_array().getCount(); i++) {
		domTristripsRef tristripsRef = meshRef->getTristrips_array().get(i);

		Tristrip ts;
		ts.fvf = 0;

		// Resolve all data sources

		int posOffset = -1; domListOfFloats* posSrc;
		int norOffset = -1; domListOfFloats* norSrc;
		int colOffset = -1; domListOfFloats* colSrc;
		int texOffset = -1; domListOfFloats* texSrc;

		for(u_int j = 0; j < tristripsRef->getInput_array().getCount(); j++) {
			domInputLocalOffsetRef input = tristripsRef->getInput_array().get(j);
			daeElementRef source = input->getSource().getElement();
			// Defined per vertex - forward
			if (source->typeID() == domVertices::ID()) {
				source = daeSafeCast<domVertices>(source)->getInput_array().get(0)->getSource().getElement();
			}
			int offset = (int)input->getOffset();
			domListOfFloats* src = &(daeSafeCast<domSource>(source)->getFloat_array()->getValue());
			if (input->getSemantic() == string("VERTEX")) {
				posOffset = offset;
				posSrc = src;
				ts.fvf |= D3DFVF_XYZ;
			} else if (input->getSemantic() == string("NORMAL")) {
				norOffset = offset;
				norSrc = src;
				ts.fvf |= D3DFVF_NORMAL;
			} else if (input->getSemantic() == string("COLOR")) {
				colOffset = offset;
				colSrc = src;
				ts.fvf |= D3DFVF_DIFFUSE;
			} else if (input->getSemantic() == string("TEXCOORD")) {
				texOffset = offset;
				texSrc = src;
				ts.fvf |= D3DFVF_TEX2;
			}
		}

		// Load the <P/> elementes

		int pStride = 0;
		pStride = max(pStride, posOffset + 1);
		pStride = max(pStride, norOffset + 1);
		pStride = max(pStride, colOffset + 1);
		pStride = max(pStride, texOffset + 1);

		vector<float> vb; // Vertex buffer data

		for(u_int j = 0; j < tristripsRef->getP_array().getCount(); j++) {
			domListOfUInts p = tristripsRef->getP_array().get(j)->getValue();

			ts.vertexCounts.push_back(p.getCount() / pStride);

			for(u_int k = 0; k < p.getCount(); k += pStride) {
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
					int r = (int)((colSrc->get(4 * index + 0)) * 255);
					int g = (int)((colSrc->get(4 * index + 1)) * 255);
					int b = (int)((colSrc->get(4 * index + 2)) * 255);
					int a = (int)((colSrc->get(4 * index + 3)) * 255);
					u_int argb = (a << 24) + (r << 16) + (g << 8) + (b);
					vb.push_back(*((float*)&argb));
				}

				if (texOffset != -1) {
					int index = (int)p.get(k + texOffset);
					vb.push_back((float)texSrc->get(2 * index + 0));
					vb.push_back(1 - (float)texSrc->get(2 * index + 1));
				}

				// Note vertex buffer stride (bytes)
				if (j == 0 && k == 0) {
					ts.vbStride = vb.size() * sizeof(float);
				}
			}
		}

		// Copy the buffer to graphic card memory

		pD3DDevice->CreateVertexBuffer(vb.size() * sizeof(float), 0, ts.fvf, D3DPOOL_DEFAULT, &ts.vb, NULL);
		void* vbData;
		ts.vb->Lock(0, vb.size() * sizeof(float), &vbData, 0);
		copy(vb.begin(), vb.end(), (float*)vbData);
		ts.vb->Unlock();

		// Load the material

		// Default
		D3DCOLORVALUE white; white.a = white.r = white.g = white.b = 1;
		D3DCOLORVALUE black; black.a = 1; black.r = black.g = black.b = 0;
		ts.material.Ambient = ts.material.Diffuse = ts.material.Specular = white;
		ts.material.Emissive = black;
		ts.material.Power = 12.5;

		if (FAILED(D3DXCreateTextureFromFileA(pD3DDevice, "..\\data\\meshes\\suzanne.png", &ts.texure))) {
			MessageBox( NULL, L"Could not find texture map", L"COLLADA", MB_OK );
			exit(1);
		}

		// Done with this <tristrips/>

		mesh->tristrips.push_back(ts);
	}

	return mesh;
}

Mesh* suzzane;

void InitGeometry()
{
	suzzane = loadMesh("..\\data\\meshes\\suzanne.dae", "Suzanne-Geometry");
}

//-----------------------------------------------------------------------------
// Name: Cleanup()
// Desc: Releases all previously initialized objects
//-----------------------------------------------------------------------------
void Cleanup()
{
    if( pD3DDevice != NULL )
        pD3DDevice->Release();

    if( pD3D != NULL )
        pD3D->Release();
}



//-----------------------------------------------------------------------------
// Name: SetupMatrices()
// Desc: Sets up the world, view, and projection transform matrices.
//-----------------------------------------------------------------------------
void SetupMatrices()
{
    // Set up world matrix
    D3DXMATRIXA16 matWorld;
    D3DXMatrixRotationY( &matWorld, timeGetTime() / 1000.0f );
    pD3DDevice->SetTransform( D3DTS_WORLD, &matWorld );

    // Set up our view matrix. A view matrix can be defined given an eye point,
    // a point to lookat, and a direction for which way is up. Here, we set the
    // eye five units back along the z-axis and up three units, look at the 
    // origin, and define "up" to be in the y-direction.
    D3DXVECTOR3 vEyePt( 0.0f, 3.0f,-5.0f );
    D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
    pD3DDevice->SetTransform( D3DTS_VIEW, &matView );

    // For the projection matrix, we set up a perspective transform (which
    // transforms geometry from 3D view space to 2D viewport space, with
    // a perspective divide making objects smaller in the distance). To build
    // a perpsective transform, we need the field of view (1/4 pi is common),
    // the aspect ratio, and the near and far clipping planes (which define at
    // what distances geometry should be no longer be rendered).
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f );
    pD3DDevice->SetTransform( D3DTS_PROJECTION, &matProj );
}




//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Draws the scene
//-----------------------------------------------------------------------------
void Render()
{
    // Clear the backbuffer and the zbuffer
    pD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                         D3DCOLOR_XRGB( 0, 0, 255 ), 1.0f, 0 );

    // Begin the scene
    if( SUCCEEDED( pD3DDevice->BeginScene() ) )
    {
        // Setup the world, view, and projection matrices
        SetupMatrices();

		suzzane->Render();

        // End the scene
        pD3DDevice->EndScene();
    }

    // Present the backbuffer contents to the display
    pD3DDevice->Present( NULL, NULL, NULL, NULL );
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
        InitGeometry();
        
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

    UnregisterClass( L"D3D Tutorial", wc.hInstance );
    return 0;
}