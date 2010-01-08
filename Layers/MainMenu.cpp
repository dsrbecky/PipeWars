#include "StdAfx.h"
#include "Layer.h"
#include "../Database.h"

extern Player* localPlayer;
extern IDirect3DTexture9* loadTexture(IDirect3DDevice9* dev, string textureFilename);
extern void RenderBlackRectangle(IDirect3DDevice9* dev, int left, int top, int width, int height, float alfa = 0.75);

static string imagePath = "..\\data\\images\\";

const float fadeOutSpeed = 1.0;

class MainMenu: Layer
{
	static bool menuActive;

	int left;
	int top;
	static const int width = 250;
	static const int height = 275;

	float alfa;

public:

	struct Button
	{
		int left, top, width, height;
		string image, imagePressed;
		bool pressed;
		void (*onPressed)();
	};

	Button buttons[3];
	bool lastLeftButtonDown;

	MainMenu(): lastLeftButtonDown(false), left(0), top(0), alfa(1.0f)
	{
		Button start = {
			25, 25, 200, 50,
			"ServerButton1.jpg", "ServerButton2.jpg",
			false,
			OnStartPressed
		};
		Button join = {
			25, 85, 200, 50,
			"ClientButton1.jpg", "ClientButton2.jpg",
			false,
			OnJoinPressed
		};
		Button exit = {
			50, 200, 150, 50,
			"QuitButton1.jpg", "QuitButton2.jpg",
			false,
			OnExitPressed
		};
		buttons[0] = start;
		buttons[1] = join;
		buttons[2] = exit;
	}

	bool MouseProc(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown, int nMouseWheelDelta, int xPos, int yPos)
	{
		if (!menuActive) return false;

		Button* b = GetButton(xPos, yPos);
		// Mouse down - put button down
		if (b != NULL && !lastLeftButtonDown && bLeftButtonDown) {
			if (b != NULL)
				b->pressed = true;
		}
		// Mouse up - activate
		if (lastLeftButtonDown && !bLeftButtonDown) {
			if (b != NULL && b->pressed && b->onPressed != NULL)
				b->onPressed();
		}
		// Put other buttons up
		for(int i = 0; i < (sizeof(buttons) / sizeof(Button)); i++) {
			if (b == NULL || b != &buttons[i] || !bLeftButtonDown)
				buttons[i].pressed = false;
		}
		lastLeftButtonDown = bLeftButtonDown;
		return false;
	}

	Button* GetButton(int x, int y)
	{
		for(int i = 0; i < (sizeof(buttons) / sizeof(Button)); i++) {
			Button& b = buttons[i];
			if (left + b.left <= x && x <= left + b.left + b.width &&
				top + b.top <= y && y <= top + b.top + b.height) {
				return &b;
			}
		}
		return NULL;
	}

	void FrameMove(double fTime, float fElapsedTime)
	{
		if (!menuActive)
			alfa -= fadeOutSpeed * fElapsedTime;
		alfa = max(0, alfa);
	}

	void Render(IDirect3DDevice9* dev)
	{
		if (alfa == 0) return;

		int colorAlfa = ((int)(alfa * 255) << 24) + 0x00FFFFFF;

		left = (DXUTGetWindowWidth() - width) / 2;
		top = (DXUTGetWindowHeight() - height - 200) / 2 + 200;

		// Note - This will adjust alfa rendering state
		RenderBlackRectangle(dev, left, top, width, height, alfa * 0.75f);

		dev->SetRenderState(D3DRS_ZENABLE, false);
		dev->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		IDirect3DTexture9* header = loadTexture(dev, imagePath + "Header.jpg"); // 800x200

		ID3DXSprite* sprite;
		D3DXCreateSprite(dev, &sprite);
		sprite->Begin(0);

		// Header
		D3DXMATRIX scale;
		D3DXMatrixScaling(&scale, 800.0f / 1024, 200.0f / 256, 1.0f);		
		sprite->SetTransform(&scale);
		D3DXVECTOR3 pos(0, 0, 0);
		sprite->Draw(header, NULL, NULL, &pos, colorAlfa);

		// Buttons
		for(int i = 0; i < (sizeof(buttons) / sizeof(Button)); i++) {
			Button& b = buttons[i];
			D3DXMatrixScaling(&scale, (float)b.width / 256, (float)b.height / 64, 1.0f);
			sprite->SetTransform(&scale);
			pos = D3DXVECTOR3((float)(left + b.left), (float)(top + b.top), 0);
			pos.x /= scale._11;
			pos.y /= scale._22;
			IDirect3DTexture9* image = loadTexture(dev, imagePath + b.image);
			IDirect3DTexture9* imagePressed = loadTexture(dev, imagePath + b.imagePressed);
			sprite->Draw(b.pressed ? imagePressed : image, NULL, NULL, &pos, colorAlfa);
		}
		
		sprite->End();
		sprite->Release();
	}

	static void OnStartPressed()
	{
		localPlayer = new Player("David");
		localPlayer->position = D3DXVECTOR3(5, 4.3f, 14);
		localPlayer->score++;
		db.entities.push_front(localPlayer);
		menuActive = false;
	}

	static void OnJoinPressed()
	{
	}

	static void OnExitPressed()
	{
		DXUTShutdown(0);
	}
};

bool MainMenu::menuActive = true;

MainMenu mainMenu;