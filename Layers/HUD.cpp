#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"
#include <map>

extern Player* localPlayer;
extern IDirect3DTexture9* loadTexture(IDirect3DDevice9* dev, string textureFilename);

static string imagePath = "..\\data\\images\\";

class HUD: Layer
{
	void Render(IDirect3DDevice9* dev)
	{
		if (localPlayer == NULL) return;

		IDirect3DTexture9* hud = loadTexture(dev, imagePath + "HUD2.png");
		IDirect3DTexture9* digits[] = {
			loadTexture(dev, imagePath + "H0.png"),
			loadTexture(dev, imagePath + "H1.png"),
			loadTexture(dev, imagePath + "H2.png"),
			loadTexture(dev, imagePath + "H3.png"),
			loadTexture(dev, imagePath + "H4.png"),
			loadTexture(dev, imagePath + "H5.png"),
			loadTexture(dev, imagePath + "H6.png"),
			loadTexture(dev, imagePath + "H7.png"),
			loadTexture(dev, imagePath + "H8.png"),
			loadTexture(dev, imagePath + "H9.png")
		};
		map<ItemType, IDirect3DTexture9*> weapons;
		weapons[Weapon_Revolver]     = loadTexture(dev, imagePath + "RevolverMini2.png");
		weapons[Weapon_Shotgun]      = loadTexture(dev, imagePath + "BoomstickMini2.png");
		weapons[Weapon_AK47]         = loadTexture(dev, imagePath + "AK47Mini2.png");
		weapons[Weapon_Jackhammer]   = loadTexture(dev, imagePath + "JackhammerMini2.png");
		weapons[Weapon_Nailgun]      = loadTexture(dev, imagePath + "Nail GunMini2.png");
		IDirect3DTexture9* weaponDualRevolver = loadTexture(dev, imagePath + "Dual RevolversMini2.png");

		ID3DXSprite* sprite; D3DXCreateSprite(dev, &sprite);
		sprite->Begin(0);

		float top = (float)DXUTGetWindowHeight() - 64; 
		D3DXVECTOR3 pos(0, top, 0);
		sprite->Draw(hud, NULL, NULL, &pos, 0xFFFFFFFF);

		RenderNumber(sprite, digits, 93,  top, localPlayer->health);
		RenderNumber(sprite, digits, 272, top, localPlayer->armour);
		RenderNumber(sprite, digits, 457, top, localPlayer->inventory[Ammo_Revolver + localPlayer->selectedWeapon]);

		IDirect3DTexture9* weaponImage = weapons[localPlayer->selectedWeapon];
		if (localPlayer->selectedWeapon == Weapon_Revolver && localPlayer->inventory[Weapon_Revolver > 1]) {
			weaponImage = weaponDualRevolver;
		}
		pos.x = 580; sprite->Draw(weaponImage, NULL, NULL, &pos, 0xFFFFFFFF);
		
		sprite->End();
		sprite->Release();
	}

	void RenderNumber(ID3DXSprite* sprite, IDirect3DTexture9* digits[], float x, float y, int num)
	{
		if (num > 999) num = 999;

		if (num >= 100) {
			D3DXVECTOR3 pos(x, y, 0);
			sprite->Draw(digits[num / 100], NULL, NULL, &pos, 0xFFFFFFFF);
		}
		if (num >= 10) {
			D3DXVECTOR3 pos(x + 32, y, 0);
			sprite->Draw(digits[(num / 10) % 10], NULL, NULL, &pos, 0xFFFFFFFF);
		}
		if (num >= 0) {
			D3DXVECTOR3 pos(x + 64, y, 0);
			sprite->Draw(digits[num % 10], NULL, NULL, &pos, 0xFFFFFFFF);
		}
	}
};

HUD hud;