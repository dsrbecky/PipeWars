#include "StdAfx.h"
#include "Database.h"

extern Player* localPlayer;

void Tristrip::Render(IDirect3DDevice9* dev)
{
	// Create buffer on demand
	if (buffer == NULL) {
		dev->CreateVertexBuffer(vb.size() * sizeof(float), 0, fvf, D3DPOOL_DEFAULT, &buffer, NULL);
		void* vbData;
		buffer->Lock(0, vb.size() * sizeof(float), &vbData, 0);
		copy(vb.begin(), vb.end(), (float*)vbData);
		buffer->Unlock();
	}

	if (texture == NULL && textureFilename.size() > 0) {
		if (FAILED(D3DXCreateTextureFromFileA(dev, (textureFilename).c_str(), &texture))) {
			MessageBoxA(NULL, ("Could not find texture " + textureFilename).c_str() , "COLLADA", MB_OK);
			exit(1);
		}
	}

	dev->SetStreamSource(0, buffer, 0, vbStride_bytes);
	dev->SetFVF(fvf);
	dev->SetTexture(0, texture);
	dev->SetMaterial(&material);
	int start = 0;
	for(int j = 0; j < (int)vertexCounts.size(); j++) {
		int vertexCount = vertexCounts[j];
		dev->DrawPrimitive(D3DPT_TRIANGLESTRIP, start, vertexCount - 2);
		start += vertexCount;
	}
}

void Mesh::Render(IDirect3DDevice9* dev, string hide1, string hide2)
{
	for(int i = 0; i < (int)tristrips.size(); i++) {
		Tristrip& ts = tristrips[i];
		if (hide1.size() > 0 && ts.getMaterialName().find(hide1) != -1)
			continue;
		if (hide2.size() > 0 && ts.getMaterialName().find(hide2) != -1)
			continue;
		ts.Render(dev);
	}
}

// Returns number defining on which side of line the point P lies
inline float whichSideOfLine(float px, float py, float x1, float y1, float x2, float y2)
{
	// Cross product with origin (x1, y1)
	return ((px - x1) * (y2 - y1)) - ((py - y1) * (x2 - x1));
}

// Returns true if Y-ray intersects triangle a-b-c
inline bool TriangleIntersectsYRay(float x, float z, float ax, float az, float bx, float bz, float cx, float cz)
{
	float i = whichSideOfLine(x, z, ax, az, bx, bz);
	float j = whichSideOfLine(x, z, bx, bz, cx, cz);
	float k = whichSideOfLine(x, z, cx, cz, ax, az);
	
	// If the point is on the same side of each of the lines then it is in the triangle.
	return (i >= 0 && j >= 0 && k >= 0) || (i <= 0 && j <= 0 && k <= 0);
}

void IntersectYRayWithPlane(float x, float z, float* outY, D3DXVECTOR3 tri[3])
{
	D3DXVECTOR3 a = tri[0];
	D3DXVECTOR3 p = tri[1] - a;
	D3DXVECTOR3 q = tri[2] - a;
	D3DXVECTOR3 norm;
	D3DXVec3Cross(&norm, &p, &q);
	float dist_tri_orig = D3DXVec3Dot(&a, &norm);
	float dist_up_orig = norm.y; // = (0, 1, 0) * norm
	*outY = dist_tri_orig / dist_up_orig;
}

bool Tristrip::IntersectsYRay(float x, float z, float* outY)
{
	int offset = 0;
	for(int i = 0; i < (int)vertexCounts.size(); i++) {
		int vertexCount = vertexCounts[i];
		// Test all triangles in set
		for(int j = 0; j < vertexCount - 2; j++) {
			// Test tringle at current offset 
			bool isIn = TriangleIntersectsYRay(x, z, 
				vb[offset + 0], vb[offset + 2], // A
				vb[offset + 3], vb[offset + 5], // B
				vb[offset + 6], vb[offset + 8]  // C
			);
			if (isIn) {
				if (outY != NULL) {
					IntersectYRayWithPlane(x, z, outY, (D3DXVECTOR3*)&vb[offset]);
				}
				return true;
			}
			offset += vbStride_floats; // Next
		}
		offset += 2 * vbStride_floats; // Next set
	}
	return false;
}

void Database::loadTestMap()
{
	add(3, 0, 0, 180, new MeshEntity("suzanne.dae", "Suzanne"));

	localPlayer = new Player("David");

	add(5, 4.3, 14, 0, localPlayer);
	add(12, 0.45, 28, 0, new Player("Ali"));
	add(25.4, -1.45, 32, 0, new Player("Shephan"));
	add(5.5, -3.5, 16, 0, new Player("Ed"));
	
	add(-10, 0.5, 8, 0, new Pipe("LeftTurn"));
	add(-10, 0.5, 0, 0, new Pipe("LongStraight"));
	add(-6, 0.5, -2, 180, new Pipe("UTurn"));
	add(-6, 0.5, 0, 0, new Pipe("LevelUp"));
	add(-10, 2.5, 8, 0, new Pipe("UTurn"));
	add(-10, 2.5, 6, 180, new Pipe("LevelUp"));
	add(-6, 4.5, -6, 270, new Pipe("LeftTurn"));
	add(-4, 4.5, -6, 90, new Pipe("LongStraight"));
	add(10, 2.5, -6, 270, new Pipe("LevelUp"));
	add(12, 2.5, -6, 90, new Pipe("LeftTurn"));
	add(16, 2.5, -12, 180, new Pipe("LeftTurn"));
	add(6, 2.5, -12, 90, new Pipe("STurn2"));
	add(4, 2.5, -12, 270, new Pipe("LeftTurn"));
	add(0, 0.5, 0, 180, new Pipe("LevelUp"));
	add(0, 0.5, 2, 0, new Pipe("LongStraight"));
	add(-2, 0.5, 12, 90, new Tank("Tank3x5"));
	
	add(6, 0.5, 12, 90, new Pipe("LongStraight"));
	add(16, 0.5, 8, 0, new Tank("Tank5x7"));
	add(18, -1.5, -2, 0, new Pipe("LevelUp"));
	add(22, -1.5, -8, 270, new Pipe("LeftTurn"));
	add(30, -3.5, -8, 270, new Pipe("LevelUp"));
	add(32, -3.5, -4, 90, new Pipe("UTurn"));
	add(30, -3.5, -4, 270, new Pipe("LeftTurn"));
	add(22, -3.5, 8, 90, new Tank("Tank5x7"));
	add(34, -3.5, 6, 90, new Pipe("STurn2"));

	add(30, -1.5, 12, 270, new Pipe("LevelUp"));
	add(38, -3.5, 12, 270, new Pipe("LevelUp"));
	add(40, -3.5, 12, 90, new Pipe("LeftTurn"));
	add(44, -3.5, 6, 180, new Pipe("LeftTurn"));
	
	add(10, 4.5, 30, 0, new Tank("Tank3x5"));
	add(2, 4.5, 28, 0, new Pipe("LeftTurn"));
	add(10, 4.5, 20, 0, new Pipe("LongStraight"));
	add(0, 4.5, 20, 0, new Pipe("STurn1"));
	add(10, 4.5, 18, 180, new Pipe("LeftTurn"));
	add(4, 4.5, 14, 270, new Pipe("LeftTurn"));
	
	add(20, 2.5, 32, 270, new Pipe("LevelUp"));
	add(22, 2.5, 32, 90, new Pipe("UTurn"));
	add(14, 0.5, 28, 90, new Pipe("LevelUp"));
	add(12, 0.5, 28, 270, new Pipe("UTurn"));
	add(20, -1.5, 32, 270, new Pipe("LevelUp"));
	add(22, -1.5, 32, 90, new Pipe("LongStraight"));
	add(30, -1.5, 32, 90, new Pipe("UTurn"));
	add(22, -3.5, 28, 90, new Pipe("LevelUp"));
	add(20, -3.5, 24, 270, new Pipe("UTurn"));
	
	add(26, -3.5, 12, 0, new Pipe("LongStraight"));
	add(22, -3.5, 24, 90, new Pipe("LeftTurn"));
	
	add(18, 0.5, 20, 0, new Pipe("LeftTurn"));
	add(28, 0.5, 28, 180, new Pipe("LeftTurn"));
	add(28, 0.5, 30, 0, new Pipe("LevelUp"));
	add(24, 2.5, 42, 90, new Pipe("LeftTurn"));
	add(22, 2.5, 42, 270, new Pipe("LevelUp"));
	add(10, 4.5, 38, 0, new Pipe("LeftTurn"));
	
	add(0, 0.5, 16, 0, new Pipe("LeftTurn"));
	add(12, -1.5, 20, 270, new Pipe("LevelUp"));
	add(14, -1.5, 20, 90, new Pipe("UTurn"));
	add(6, -3.5, 16, 90, new Pipe("LevelUp"));
	add(0, -3.5, 12, 0, new Pipe("LeftTurn"));
	add(4, -3.5, 6, 270, new Pipe("LeftTurn"));
	add(6, -3.5, 6, 90, new Pipe("LongStraight"));
	add(12, -3.5, 6, 90, new Pipe("LongStraight"));
	
	add(0, 0, 8, 0, new PowerUp(Weapon_Revolver));
	add(44, -3.4, 8, 0, new PowerUp(Weapon_Revolver));
	add(21.6, 0.1, 9.4, 0, new PowerUp(Weapon_Revolver));
	add(31.5, -1.1, 12, 0, new PowerUp(Weapon_Revolver));
	add(21, -3.2, 24, 0, new PowerUp(Weapon_Revolver));
	add(0, 0, 12, 0, new PowerUp(Weapon_AK47));
	add(20, -1.6, -6.8, 0, new PowerUp(Weapon_AK47));
	add(24, -3.4, 7, 0, new PowerUp(Weapon_Nailgun));
	add(14, 2.3, -7.2, 0, new PowerUp(Weapon_Nailgun));
	add(15, -1.75, 17.5, 0, new PowerUp(Weapon_Nailgun));
	add(25.5, -2.2, 28, 0, new PowerUp(Weapon_Shotgun));
	add(-8, 2.5, 9, 0, new PowerUp(Weapon_Shotgun));
	add(14.5, 0.6, 17.5, 0, new PowerUp(Weapon_Shotgun));
	
	add(new RespawnPoint(5, 4.3, 14));
	add(new RespawnPoint(12, 0.45, 28));
	add(new RespawnPoint(25.4, -1.45, 32));
	add(new RespawnPoint(5.5, -3.5, 16));
	add(new RespawnPoint(33.0, -3.0, -7.0));
	add(new RespawnPoint(-8, 0.4, -3));
	add(new RespawnPoint(0, -3, 12));
	
	add(10.0, 4.0, 22, 0, new PowerUp(HealthPack));
	add(6.0, 0.0, 20, 0, new PowerUp(HealthPack));
	add(16.0, 0.0, 12, 0, new PowerUp(HealthPack));
	add(44.0, -4.0, 6, 0, new PowerUp(ArmourPack));
	add(4.0, 4.0, 14.0, 0, new PowerUp(ArmourPack));
	add(0.0, 4.3, -6, 0, new PowerUp(Shiny));
	add(28.0, 1.0, 31, 0, new PowerUp(Shiny));
	add(0.0, 0.6, 7, 0, new PowerUp(Shiny));
	add(26.0, -4.0, 14, 0, new PowerUp(Shiny));
	add(28.0, -3.0, -8, 0, new PowerUp(Ammo_Revolver));
	add(18.0, 0.0, 15, 0, new PowerUp(Ammo_Revolver));
	add(26.0, -2.0, 32, 0, new PowerUp(Ammo_Nailgun));
	add(10.0, -4.0, 6, 0, new PowerUp(Ammo_Nailgun));
	add(23.0, 2.0, 29.0, 0, new PowerUp(Ammo_Shotgun));
	add(22.0, 2.0, 42.0, 0, new PowerUp(Ammo_Shotgun));
	add(-7.0, 0.0, -3.0, 0, new PowerUp(Ammo_Jackhammer));
	add(14.0, -4.0, 6.0, 0, new PowerUp(Ammo_Jackhammer));
	add(0.0, -4.0, 10, 0, new PowerUp(Ammo_AK47));
	add(28.0, -4.0, 2, 0, new PowerUp(Ammo_AK47));
	add(11.0, 4.0, 32, 0, new PowerUp(Monkey));
}