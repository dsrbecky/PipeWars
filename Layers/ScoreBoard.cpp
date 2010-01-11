#include "StdAfx.h"
#include "Layer.h"
#include "../Entities.h"
#include "../Resources.h"
#include "../Util.h"

extern Database db;
extern Player* localPlayer;
extern Resources resources;

class ScoreBoard: Layer
{
	static bool sortByScore(const Player* a, const Player* b)
	{
		return (*a).score > (*b).score || ((*a).score == (*b).score && string((*a).name) < string((*b).name));
	}

	virtual void Render(IDirect3DDevice9* dev)
	{
		if (!keyDown[VK_TAB]) return;

		IDirect3DTexture9* background = resources.LoadTexture(dev, "Scoreboard2.png");

	    dev->SetRenderState(D3DRS_ZENABLE, false);
		dev->SetRenderState(D3DRS_LIGHTING, false);
		dev->SetRenderState(D3DRS_ALPHABLENDENABLE, !keyToggled_Alt['X']);
		dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_INVSRCALPHA);
		dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA);
		dev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);

		ID3DXSprite* sprite; D3DXCreateSprite(dev, &sprite);
		sprite->Begin(0);

		int left = (DXUTGetWindowWidth() - 512) / 2;
		int top = (DXUTGetWindowHeight() - 512 - 64) / 2;

		D3DXVECTOR3 pos((float)left, (float)top, 0);
		sprite->Draw(background, NULL, NULL, &pos, 0xFFFFFFF);

		sprite->End();
		sprite->Release();

		vector<Player*> players;
		{ DbLoop_Players(db, it)
			players.push_back(player);
		}
		sort(players.begin(), players.end(), sortByScore);

		textX = left + 52 + 16;
		textY = top + 88 + 16;

		RenderScore(dev, "Name", "Score", "Kills", "Deaths", D3DCOLOR_XRGB(200, 200, 0));
		textY += 8;

		for(vector<Player*>::iterator it = players.begin(); it != players.end(); it++) {
			stringstream score;  score  << (*it)->score;
			stringstream kills;  kills  << (*it)->kills;
			stringstream deaths; deaths << (*it)->deaths;
			D3DCOLOR color = D3DCOLOR_XRGB(200, 200, 200);
			if ((*it) == localPlayer && localPlayer != NULL)
				color = D3DCOLOR_XRGB(255, 255, 255);
			RenderScore(dev, (*it)->name, score.str(), kills.str(), deaths.str(), color);
		}
	}

	void RenderScore(IDirect3DDevice9* dev, string name, string score, string kills, string deaths, D3DCOLOR color)
	{
		RenderText(dev, name, 0, color);
		RenderText(dev, score, 190, color);
		RenderText(dev, kills, 260, color);
		RenderText(dev, deaths, 330, color);
		textY += 16;
	}
};

ScoreBoard scoreBoard;