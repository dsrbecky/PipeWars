#include "StdAfx.h"
#include "Database.h"
using namespace std;

LPDIRECT3DDEVICE9 pD3DDevice = NULL;
Database db;

void Tristrip::Render()
{
	// Create buffer on demand
	if (buffer == NULL) {
		pD3DDevice->CreateVertexBuffer(vb.size() * sizeof(float), 0, fvf, D3DPOOL_DEFAULT, &buffer, NULL);
		void* vbData;
		buffer->Lock(0, vb.size() * sizeof(float), &vbData, 0);
		copy(vb.begin(), vb.end(), (float*)vbData);
		buffer->Unlock();
	}

	pD3DDevice->SetStreamSource(0, buffer, 0, vbStride);
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

void Mesh::Render()
{
	for(int i = 0; i < (int)tristrips.size(); i++) {
		tristrips[i].Render();
	}
}

void TextWriter::Render()
{
	if (font == NULL) {
		D3DXCreateFont( pD3DDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
						OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
						L"Arial", &font );
	}
	RECT rect = {8, 8, 20, 20};
	font->DrawTextW(NULL, DXUTGetFrameStats(true), -1, &rect, DT_NOCLIP, D3DCOLOR_XRGB(0xff, 0xff, 0xff));
}

void Database::Render()
{
	for(int i = 0; i < (int)db.entities.size(); i++) {
		MeshEntity* entity = dynamic_cast<MeshEntity*>(entities[i]);
		if (entity == NULL)
			continue; // Other type

		D3DXMATRIXA16 matWorld;
		D3DXMatrixIdentity(&matWorld);

		D3DXMATRIXA16 matScale;
		D3DXMatrixScaling(&matScale, entity->scale, entity->scale, entity->scale);
		D3DXMatrixMultiply(&matWorld, &matWorld, &matScale);

		D3DXMATRIXA16 matRot;
		D3DXMatrixRotationY(&matRot, entity->rotY / 360 * 2 * D3DX_PI);
		D3DXMatrixMultiply(&matWorld, &matWorld, &matRot);

		D3DXMATRIXA16 matTrans;
		D3DXMatrixTranslation(&matTrans, entity->position.x, entity->position.y, entity->position.z);
		D3DXMatrixMultiply(&matWorld, &matWorld, &matTrans);

		pD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);

		entity->Render();
	}
}

void Database::loadTestMap()
{
	db.add(5, 4.3, 14, 0, new Player("David"));
	db.add(12, 0.45, 28, 0, new Player("Ali"));
	db.add(25.4, -1.45, 32, 0, new Player("Shephan"));
	db.add(5.5, -3.5, 16, 0, new Player("Ed"));
	
	db.add(-10, 0.5, 8, 0, new Pipe("LeftTurn"));
	db.add(-10, 0.5, 0, 0, new Pipe("LongStraight"));
	db.add(-6, 0.5, -2, 180, new Pipe("UTurn"));
	db.add(-6, 0.5, 0, 0, new Pipe("LevelUp"));
	db.add(-10, 2.5, 8, 0, new Pipe("UTurn"));
	db.add(-10, 2.5, 6, 180, new Pipe("LevelUp"));
	db.add(-6, 4.5, -6, 270, new Pipe("LeftTurn"));
	db.add(-4, 4.5, -6, 90, new Pipe("LongStraight"));
	db.add(10, 2.5, -6, 270, new Pipe("LevelUp"));
	db.add(12, 2.5, -6, 90, new Pipe("LeftTurn"));
	db.add(16, 2.5, -12, 180, new Pipe("LeftTurn"));
	db.add(6, 2.5, -12, 90, new Pipe("STurn2"));
	db.add(4, 2.5, -12, 270, new Pipe("LeftTurn"));
	db.add(0, 0.5, 0, 180, new Pipe("LevelUp"));
	db.add(0, 0.5, 2, 0, new Pipe("LongStraight"));
	db.add(-2, 0.5, 12, 90, new Tank("Tank3x5"));
	
	db.add(6, 0.5, 12, 90, new Pipe("LongStraight"));
	db.add(16, 0.5, 8, 0, new Tank("Tank5x7"));
	db.add(18, -1.5, -2, 0, new Pipe("LevelUp"));
	db.add(22, -1.5, -8, 270, new Pipe("LeftTurn"));
	db.add(30, -3.5, -8, 270, new Pipe("LevelUp"));
	db.add(32, -3.5, -4, 90, new Pipe("UTurn"));
	db.add(30, -3.5, -4, 270, new Pipe("LeftTurn"));
	db.add(22, -3.5, 8, 90, new Tank("Tank5x7"));
	db.add(34, -3.5, 6, 90, new Pipe("STurn2"));

	db.add(30, -1.5, 12, 270, new Pipe("LevelUp"));
	db.add(38, -3.5, 12, 270, new Pipe("LevelUp"));
	db.add(40, -3.5, 12, 90, new Pipe("LeftTurn"));
	db.add(44, -3.5, 6, 180, new Pipe("LeftTurn"));
	
	db.add(10, 4.5, 30, 0, new Tank("Tank3x5"));
	db.add(2, 4.5, 28, 0, new Pipe("LeftTurn"));
	db.add(10, 4.5, 20, 0, new Pipe("LongStraight"));
	db.add(0, 4.5, 20, 0, new Pipe("STurn1"));
	db.add(10, 4.5, 18, 180, new Pipe("LeftTurn"));
	db.add(4, 4.5, 14, 270, new Pipe("LeftTurn"));
	
	db.add(20, 2.5, 32, 270, new Pipe("LevelUp"));
	db.add(22, 2.5, 32, 90, new Pipe("UTurn"));
	db.add(14, 0.5, 28, 90, new Pipe("LevelUp"));
	db.add(12, 0.5, 28, 270, new Pipe("UTurn"));
	db.add(20, -1.5, 32, 270, new Pipe("LevelUp"));
	db.add(22, -1.5, 32, 90, new Pipe("LongStraight"));
	db.add(30, -1.5, 32, 90, new Pipe("UTurn"));
	db.add(22, -3.5, 28, 90, new Pipe("LevelUp"));
	db.add(20, -3.5, 24, 270, new Pipe("UTurn"));
	
	db.add(26, -3.5, 12, 0, new Pipe("LongStraight"));
	db.add(22, -3.5, 24, 90, new Pipe("LeftTurn"));
	
	db.add(18, 0.5, 20, 0, new Pipe("LeftTurn"));
	db.add(28, 0.5, 28, 180, new Pipe("LeftTurn"));
	db.add(28, 0.5, 30, 0, new Pipe("LevelUp"));
	db.add(24, 2.5, 42, 90, new Pipe("LeftTurn"));
	db.add(22, 2.5, 42, 270, new Pipe("LevelUp"));
	db.add(10, 4.5, 38, 0, new Pipe("LeftTurn"));
	
	db.add(0, 0.5, 16, 0, new Pipe("LeftTurn"));
	db.add(12, -1.5, 20, 270, new Pipe("LevelUp"));
	db.add(14, -1.5, 20, 90, new Pipe("UTurn"));
	db.add(6, -3.5, 16, 90, new Pipe("LevelUp"));
	db.add(0, -3.5, 12, 0, new Pipe("LeftTurn"));
	db.add(4, -3.5, 6, 270, new Pipe("LeftTurn"));
	db.add(6, -3.5, 6, 90, new Pipe("LongStraight"));
	db.add(12, -3.5, 6, 90, new Pipe("LongStraight"));
	
	db.add(0, 0, 8, 0, new PowerUp(Weapon_Revolver));
	db.add(44, -3.4, 8, 0, new PowerUp(Weapon_Revolver));
	db.add(21.6, 0.1, 9.4, 0, new PowerUp(Weapon_Revolver));
	db.add(31.5, -1.1, 12, 0, new PowerUp(Weapon_Revolver));
	db.add(21, -3.2, 24, 0, new PowerUp(Weapon_Revolver));
	db.add(0, 0, 12, 0, new PowerUp(Weapon_AK47));
	db.add(20, -1.6, -6.8, 0, new PowerUp(Weapon_AK47));
	db.add(24, -3.4, 7, 0, new PowerUp(Weapon_Nailgun));
	db.add(14, 2.3, -7.2, 0, new PowerUp(Weapon_Nailgun));
	db.add(15, -1.75, 17.5, 0, new PowerUp(Weapon_Nailgun));
	db.add(25.5, -2.2, 28, 0, new PowerUp(Weapon_Shotgun));
	db.add(-8, 2.5, 9, 0, new PowerUp(Weapon_Shotgun));
	db.add(14.5, 0.6, 17.5, 0, new PowerUp(Weapon_Shotgun));
	
	db.add(new RespawnPoint(5, 4.3, 14));
	db.add(new RespawnPoint(12, 0.45, 28));
	db.add(new RespawnPoint(25.4, -1.45, 32));
	db.add(new RespawnPoint(5.5, -3.5, 16));
	db.add(new RespawnPoint(33.0, -3.0, -7.0));
	db.add(new RespawnPoint(-8, 0.4, -3));
	db.add(new RespawnPoint(0, -3, 12));
	
	db.add(10.0, 4.0, 22, 0, new PowerUp(HealthPack));
	db.add(6.0, 0.0, 20, 0, new PowerUp(HealthPack));
	db.add(16.0, 0.0, 12, 0, new PowerUp(HealthPack));
	db.add(44.0, -4.0, 6, 0, new PowerUp(ArmourPack));
	db.add(4.0, 4.0, 14.0, 0, new PowerUp(ArmourPack));
	db.add(0.0, 4.3, -6, 0, new PowerUp(Shiny));
	db.add(28.0, 1.0, 31, 0, new PowerUp(Shiny));
	db.add(0.0, 0.6, 7, 0, new PowerUp(Shiny));
	db.add(26.0, -4.0, 14, 0, new PowerUp(Shiny));
	db.add(28.0, -3.0, -8, 0, new PowerUp(Ammo_Revolver));
	db.add(18.0, 0.0, 15, 0, new PowerUp(Ammo_Revolver));
	db.add(26.0, -2.0, 32, 0, new PowerUp(Ammo_Nailgun));
	db.add(10.0, -4.0, 6, 0, new PowerUp(Ammo_Nailgun));
	db.add(23.0, 2.0, 29.0, 0, new PowerUp(Ammo_Shotgun));
	db.add(22.0, 2.0, 42.0, 0, new PowerUp(Ammo_Shotgun));
	db.add(-7.0, 0.0, -3.0, 0, new PowerUp(Ammo_Jackhammer));
	db.add(14.0, -4.0, 6.0, 0, new PowerUp(Ammo_Jackhammer));
	db.add(0.0, -4.0, 10, 0, new PowerUp(Ammo_AK47));
	db.add(28.0, -4.0, 2, 0, new PowerUp(Ammo_AK47));
	db.add(11.0, 4.0, 32, 0, new PowerUp(Monkey));
}