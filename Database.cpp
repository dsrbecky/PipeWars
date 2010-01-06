#include "StdAfx.h"
#include "Database.h"

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

	dev->SetStreamSource(0, buffer, 0, vbStride);
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

void Mesh::Render(IDirect3DDevice9* dev)
{
	for(int i = 0; i < (int)tristrips.size(); i++) {
		tristrips[i].Render(dev);
	}
}

void Database::loadTestMap()
{
	add(3, 0, 0, 180, new MeshEntity("suzanne.dae", "Suzanne"));

	add(5, 4.3, 14, 0, new Player("David"));
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