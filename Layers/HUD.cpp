#include "StdAfx.h"
#include "Layer.h"
#include "../Entities.h"
#include "../Resources.h"

extern Player* localPlayer;
extern Resources resources;

class HUD: Layer
{
	void Render(IDirect3DDevice9* dev)
	{
		if (localPlayer == NULL) return;

		dev->SetRenderState(D3DRS_ZENABLE, false);
		dev->SetRenderState(D3DRS_ALPHABLENDENABLE, !keyToggled_Alt['X']);
		dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		IDirect3DTexture9* hud = resources.LoadTexture(dev, "HUD2.png");
		IDirect3DTexture9* digits[] = {
			resources.LoadTexture(dev, "H0.png"),
			resources.LoadTexture(dev, "H1.png"),
			resources.LoadTexture(dev, "H2.png"),
			resources.LoadTexture(dev, "H3.png"),
			resources.LoadTexture(dev, "H4.png"),
			resources.LoadTexture(dev, "H5.png"),
			resources.LoadTexture(dev, "H6.png"),
			resources.LoadTexture(dev, "H7.png"),
			resources.LoadTexture(dev, "H8.png"),
			resources.LoadTexture(dev, "H9.png")
		};
		map<ItemType, IDirect3DTexture9*> weapons;
		weapons[Weapon_Revolver]     = resources.LoadTexture(dev, "RevolverMini2.png");
		weapons[Weapon_Shotgun]      = resources.LoadTexture(dev, "BoomstickMini2.png");
		weapons[Weapon_AK47]         = resources.LoadTexture(dev, "AK47Mini2.png");
		weapons[Weapon_Jackhammer]   = resources.LoadTexture(dev, "JackhammerMini2.png");
		weapons[Weapon_Nailgun]      = resources.LoadTexture(dev, "Nail GunMini2.png");
		IDirect3DTexture9* weaponDualRevolver = resources.LoadTexture(dev, "Dual RevolversMini2.png");

		ID3DXSprite* sprite; D3DXCreateSprite(dev, &sprite);
		sprite->Begin(0);

		float top = (float)DXUTGetWindowHeight() - 64; 
		D3DXVECTOR3 pos(0, top, 0);
		sprite->Draw(hud, NULL, NULL, &pos, 0xFFFFFFFF);

		RenderNumber(sprite, digits, 93,  top, localPlayer->health);
		RenderNumber(sprite, digits, 272, top, localPlayer->armour);
		RenderNumber(sprite, digits, 457, top, localPlayer->inventory[Ammo_Revolver + localPlayer->selectedWeapon]);

		IDirect3DTexture9* weaponImage = weapons[localPlayer->selectedWeapon];
		if (localPlayer->selectedWeapon == Weapon_Revolver && localPlayer->inventory[Weapon_Revolver] > 1) {
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