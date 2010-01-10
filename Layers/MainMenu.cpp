#include "StdAfx.h"
#include "Layer.h"
#include "../Entities.h"
#include "../Resources.h"
#include "../Util.h"

extern Database db;
extern Player* localPlayer;
extern Resources resources;

const float fadeOutSpeed = 1.0;

class MainMenu: Layer
{
	static bool showHeader;
	static float headerAlfa;

	static bool showButtons;
	static float buttonsAlfa;

	// Input screen
	static bool showInput;
	static string inputQuery;
	static string input;
	static void (*inputOnEntered)();

	// Buttons area
	int left;
	int top;
	int width;
	int height;

	struct Button
	{
		int left, top, width, height;
		string image, imagePressed;
		bool pressed;
		void (*onPressed)();
	};

	Button buttons[3];
	bool lastLeftButtonDown;

public:

	MainMenu(): left(0), top(0), width(0), height(0), lastLeftButtonDown(false)
	{
		Button start = {
			20, 20, 200, 50,
			"ServerButton1.jpg", "ServerButton2.jpg",
			false,
			OnStartPressed
		};
		Button join = {
			240, 20, 200, 50,
			"ClientButton1.jpg", "ClientButton2.jpg",
			false,
			OnJoinPressed
		};
		Button exit = {
			630, 20, 150, 50,
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
		if (showButtons) {
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
		}
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

	bool KeyboardProc(UINT nChar, bool bKeyDown, bool bAltDown)
	{
		Layer::KeyboardProc(nChar, bKeyDown, bAltDown);

		// Handle input screen
		if (showInput && bKeyDown && !bAltDown && nChar != VK_SHIFT) {
			if (nChar == VK_RETURN) {
				if (inputOnEntered != NULL)
					inputOnEntered();
			} else if (nChar == VK_BACK) {
				if (input.size() > 0)
					input.erase(--(input.end()));
			} else {
				nChar = MapVirtualKeyA(nChar, MAPVK_VK_TO_CHAR);
				if ('A' <= nChar && nChar <= 'Z' && !DXUTIsKeyDown(VK_SHIFT))
					nChar += 'a' - 'A';
				input = input + (char)nChar;
				return true;
			}
		}
		return false;
	}

	void FrameMove(double fTime, float fElapsedTime)
	{
		if (!showHeader) {
			headerAlfa -= fadeOutSpeed * fElapsedTime;
			headerAlfa = max(0, headerAlfa);
		}
		if (!showButtons) {
			buttonsAlfa -= fadeOutSpeed * fElapsedTime;
			buttonsAlfa = max(0, buttonsAlfa);
		}
	}

	void Render(IDirect3DDevice9* dev)
	{
		if (headerAlfa == 0) return;

		resources.LoadFont(dev); // Preload

		int headerColor = ((int)(headerAlfa * 255 * 0.90f) << 24) + 0x00FFFFFF;

		left = 0;
		top = DXUTGetWindowHeight() - 90;
		width = DXUTGetWindowWidth();
		height = 90;
		buttons[2].left = width - 150 - 20;

		if (buttonsAlfa > 0) {
			// Note - This will adjust alfa rendering state
			RenderBlackRectangle(dev, left, top, width, height, keyToggled_Alt['X'] ? 1.0f : buttonsAlfa * 0.75f);
		}

		dev->SetRenderState(D3DRS_ZENABLE, false);
		dev->SetRenderState(D3DRS_ALPHABLENDENABLE, !keyToggled_Alt['X']);
		dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		IDirect3DTexture9* header = resources.LoadTexture(dev, "Header.jpg"); // 800x200

		ID3DXSprite* sprite;
		D3DXCreateSprite(dev, &sprite);
		sprite->Begin(0);

		// Header
		D3DXMATRIX scale;
		D3DXMatrixScaling(&scale, 800.0f / 1024, 200.0f / 256, 1.0f);		
		sprite->SetTransform(&scale);
		D3DXVECTOR3 pos(0, 0, 0);
		sprite->Draw(header, NULL, NULL, &pos, headerColor);

		// Buttons
		if (buttonsAlfa > 0) {
			int buttonsColor = ((int)(buttonsAlfa * 255) << 24) + 0x00FFFFFF;
			for(int i = 0; i < (sizeof(buttons) / sizeof(Button)); i++) {
				Button& b = buttons[i];
				D3DXMatrixScaling(&scale, (float)b.width / 256, (float)b.height / 64, 1.0f);
				sprite->SetTransform(&scale);
				pos = D3DXVECTOR3((float)(left + b.left), (float)(top + b.top), 0);
				pos.x /= scale._11;
				pos.y /= scale._22;
				IDirect3DTexture9* image = resources.LoadTexture(dev, b.image);
				IDirect3DTexture9* imagePressed = resources.LoadTexture(dev, b.imagePressed);
				sprite->Draw(b.pressed ? imagePressed : image, NULL, NULL, &pos, buttonsColor);
			}
		}
		
		sprite->End();
		sprite->Release();

		// Input screen
		if (showInput) {
			int w = 400;
			int h = 50;
			int x = (DXUTGetWindowWidth() - w) / 2;
			int y = (DXUTGetWindowHeight() - h - 200) / 3 + 200;
			RenderBlackRectangle(dev, x, y, w, h, keyToggled_Alt['X'] ? 1.0f : 0.75f);
			RenderBlackRectangle(dev, x + 5, y + 25, w - 10, h - 25 - 5, keyToggled_Alt['X'] ? 1.0f : 0.75f);
			textX = x + 10; textY = y + 8;
			RenderText(dev, inputQuery, 0, D3DCOLOR_XRGB(200, 200, 0));
			textY = y + 28;
			string cursor = ((int)(DXUTGetTime() * 2)) % 2 ? "" : "_";
			RenderText(dev, input + cursor, 0, D3DCOLOR_XRGB(255, 255, 255));
		}
	}

	static void OnStartPressed()
	{
		showButtons = false;
		showInput = true;
		inputQuery = "Enter name:";
		input = "Player";
		inputOnEntered = &OnStartNameEntered;
	}

	static void OnStartNameEntered()
	{
		showHeader = false;
		showInput = false;

		localPlayer = new Player(input);
		localPlayer->position = D3DXVECTOR3(5, 4.3f, 14);
		localPlayer->score++;
		db.add(localPlayer);
	}

	static void OnJoinPressed()
	{
		showButtons = false;
		showInput = true;
		inputQuery = "Enter name:";
		input = "Player";
		inputOnEntered = &OnJoinNameEntered;
	}

	static void OnJoinNameEntered()
	{
		inputQuery = "Enter IP address or host name of server:";
		input = "10.0.0.210";
		inputOnEntered = &OnJoinIPEntered;
	}

	static void OnJoinIPEntered()
	{
		showHeader = false;
		showInput = false;
	}

	static void OnExitPressed()
	{
		DXUTShutdown(0);
	}
};

bool MainMenu::showHeader = true;
float MainMenu::headerAlfa = 1.0f;

bool MainMenu::showButtons = true;
float MainMenu::buttonsAlfa = 1.0f;

bool MainMenu::showInput = false;
string MainMenu::inputQuery = "";
string MainMenu::input = "";
void (*MainMenu::inputOnEntered)() = NULL;

MainMenu mainMenu;