// Include the basic headers we need, the Windows API headers, and the DirectX 9 headers.
#include "main.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <windows.h>
#include <windowsx.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <time.h>
#include <math.h>
#include <dinput.h>

// Include the DirectX 9 libraries.
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")

// Define Constants
const int	kScreenWidth			= 1024;
const int	kScreenHeight			= 768;
const bool	kFullscreen				= false;
const bool	kVSync					= true;
const int	kMeasureFrameCount		= 60;
const int	kSafeZoneSize			= 8;
const int	kRandFactor				= 256;
const int	kRandModifier			= 1;
const int	kColors					= 6;
const float	kBallResetSpeed			= 8.0f;
const float kPI						= 3.1415926f;
const int	kKeyDelayReset			= 60;
const int	kNumParticles			= 5;
const byte	kTextMidScreen			= 0x05;
const int	kMainMenu				= 1;
const int	kGameMenu				= 2;
const int	kWinMenu				= 3;
const int	kLoseMenu				= 4;
const int	kShowPointTimeReset		= 64;

// Global Variables
LPDIRECT3D9				gDirect3D;
LPDIRECT3DDEVICE9		gDirect3DDevice;
ID3DXSprite				*gDirect3DXSprite;
ID3DXFont				*gDirectXFont, *gDirectXFont2;
LPDIRECT3DTEXTURE9		gPlayerLeft, gPlayerBottom, gCPURight, gCPUTop, gBall;
D3DXVECTOR3				gPlayerLeftPos = D3DXVECTOR3(16, (kScreenHeight >> 1) - 32, 0), gPlayerBottomPos = D3DXVECTOR3((kScreenWidth >> 1) - 32, kScreenHeight - 32, 0),
						gCPURightPos = D3DXVECTOR3(kScreenWidth - 32, (kScreenHeight >> 1) - 32, 0), gCPUTopPos = D3DXVECTOR3((kScreenWidth >> 1) - 32, 16, 0),
						gBallPos = D3DXVECTOR3((kScreenWidth >> 1) - 4, (kScreenHeight >> 1) - 4, 0);
bool					gPauseGame = false, gIsDebugMode = false, gTabPressed = false, gPlayerLastPoint = false;
int						gCurrentTileAnimation = 0, gCurrentFrame = 0, factor = 8, gPlayerPoints = 0, gCPUPoints = 0, gBallPredictHorizontal, gBallPredictVertical, gDifficulty = 1, gKeyDelay = 0, * gParticlesX, *gParticlesY, gMenu = kMainMenu, gScoreTo = 5, gLogoTime = 0, gShowPointTime = 0;
float					gTimePerFrame = 0, frameSum = 0, gFrameTimes[kMeasureFrameCount], gBallSpeed = kBallResetSpeed;
double					gBallDirection = 0.0f, gCPUSpeed;
clock_t					gFrameBeginTime, gFrameEndTime;
RECT					gTextRect = {0, 0, kScreenWidth, kScreenHeight};
LPDIRECTINPUT8			gDirectInput8;
LPDIRECTINPUTDEVICE8	gDirectInput8Keyboard;
BYTE					gKeystate[256];
long					gColorArray[kColors], gLogoColor = 0x00FFFFFF;

// Basic Entry point for Windows Programs.
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    HWND hWnd;
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "MyWindowClass";

	if (!kFullscreen)
	{
		wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	}

    RegisterClassEx(&wc);

    hWnd = CreateWindowEx(NULL,
                          "MyWindowClass",				// Window class name
                          "Pong",						// Window title
                          WS_EX_TOPMOST | WS_POPUP,		// Window style (WS_EX_TOPMOST | WS_POPUP, WS_OVERLAPPEDWINDOW)
                          0,							// Window X
                          0,							// Window Y
                          kScreenWidth,					// Window W
                          kScreenHeight,				// Window H
                          NULL,							// Parent (NULL)
                          NULL,							// Menu (NULL)
                          hInstance,					// Application handle
                          NULL);						// Multiple windows (NULL)

    ShowWindow(hWnd, nCmdShow);
	InitializeDirect3D(hWnd);
	InitializeDirectInput(hInstance, hWnd);

    MSG msg;
	ChangeColors(true);

	while(true)
	{
		gFrameBeginTime = clock();

		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// Translate the message and dispatch it to WindowProc()
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		DetectDirectInput();

		if (msg.message == WM_QUIT) { break; }
			
		if (!gPauseGame)
		{
			UpdateGameLogic();
			RenderFrame();
		}

		gFrameEndTime = clock();

		if (gCurrentFrame >= kMeasureFrameCount)
		{
			gCurrentFrame = 0;
		}
		else
		{
			++gCurrentFrame;
		}
		
		gTimePerFrame = (float)((gFrameEndTime - gFrameBeginTime) * 10.0);
		gFrameTimes[gCurrentFrame] = gTimePerFrame;
	}

	CleanDirect3D();
	CleanDirectInput();

    return msg.wParam;
}

// This function will handle messages.
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc (hWnd, message, wParam, lParam);
}

// This function initializes the DirectX interfaces.
void InitializeDirect3D(HWND hWnd)
{
    gDirect3D = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));

    d3dpp.Windowed = !kFullscreen;				// Are we in fullscreen?
    d3dpp.SwapEffect = D3DSWAPEFFECT_FLIP;		// The swap effect. D3DSWAPEFFECT_DISCARD
    d3dpp.hDeviceWindow = hWnd;					// Refers to our Window.
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;	// Color depth. 64-Bit Color: D3DFMT_A16B16G16R16, 32-Bit Color With Transparency: D3DFMT_A8R8G8B8, 32-Bit Color Short Alpha: D3DFMT_A2R10G10B10, 32-Bit Color No Alpha: D3DFMT_X8R8G8B8
	d3dpp.BackBufferWidth = kScreenWidth;		// Direct3D Width
	d3dpp.BackBufferHeight = kScreenHeight;		// Direct3D Height
	d3dpp.BackBufferCount = 3;					// Number of backbuffers
	//d3dpp.FullScreen_RefreshRateInHz = 60;
	d3dpp.PresentationInterval = (kVSync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE); // Immediate will disable VSync.

    gDirect3D->CreateDevice(D3DADAPTER_DEFAULT,						// Use the default adapter
							D3DDEVTYPE_HAL,							// Use hardware rasterization
							hWnd,									// Refers to our window
							D3DCREATE_HARDWARE_VERTEXPROCESSING,	// Harware Vertex Processing (Faster)
							&d3dpp,									// Our presentation parameters
							&gDirect3DDevice);						// And finally our device

	InitializeGraphics();
}

// This function wraps font initialization.
void CreateFont(LPCSTR name, LPD3DXFONT *reference, bool bold)
{
	D3DXCreateFont(gDirect3DDevice,				// The device the font is for.
				   24,							// Font-height.
				   0,							// Width. (Can be 0)
				   bold ? FW_BOLD : FW_NORMAL,	// The font weight.
				   1,							// Mip-map levels. (Should be 1, I think.)
				   false,						// Italic or not.
				   DEFAULT_CHARSET,				// Character Set. (DEFAULT_CHARSET)
				   OUT_DEFAULT_PRECIS,			// Output precision. (OUT_DEFAULT_PRECIS)
				   DEFAULT_QUALITY,				// Quality (DEFAULT_QUALITY)
				   DEFAULT_PITCH | FF_DONTCARE, // Pitch and Family (DEFAULT_PITCH | FF_DONTCARE)
				   name,						// The name of our font
				   reference);					// And where to hold it
}

// This function initializes the DirectX graphics.
void InitializeGraphics()
{
	D3DXCreateSprite(gDirect3DDevice, &gDirect3DXSprite);

	CreateFont("Courier New", &gDirectXFont, true);
	CreateFont("Arial", &gDirectXFont2, false);

	LoadContent();
}

// This will create DirectInput
void InitializeDirectInput(HINSTANCE hInstance, HWND hWnd)
{
    DirectInput8Create(hInstance,				// the handle to the application
                       DIRECTINPUT_VERSION,		// the compatible version
                       IID_IDirectInput8,		// the DirectInput interface version
                       (void**)&gDirectInput8,  // the pointer to the interface
                       NULL);					// COM stuff, so we'll set it to NULL

    gDirectInput8->CreateDevice(GUID_SysKeyboard,   // the default keyboard ID being used
                      &gDirectInput8Keyboard,		// the pointer to the device interface
                      NULL);						// COM stuff, so we'll set it to NULL

    gDirectInput8Keyboard->SetDataFormat(&c_dfDIKeyboard);
	gDirectInput8Keyboard->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
}

// This function loads a given texture.
int CreateTexture(LPCSTR filename,LPDIRECT3DTEXTURE9 *reference)
{
	return D3DXCreateTextureFromFileEx(
		gDirect3DDevice,		// Our DirectX Device
		filename,				// Our texture image!
		D3DX_DEFAULT_NONPOW2,	// width
		D3DX_DEFAULT_NONPOW2,	// height
		D3DX_FROM_FILE,			// MIP levels
		0,						// usage
		D3DFMT_DXT1,			// texture format
		D3DPOOL_MANAGED,		// mem pool
		D3DX_DEFAULT,			// filter
		D3DX_DEFAULT,			// MIP filter
		0xFF000000,				// transparent color key
		NULL,					// image info struct
		NULL,					// palette
		reference);				// the returned texture
}

// This function loads the content into the sprite textures.
void LoadContent()
{
	CreateTexture("paddle-up-down.png", &gPlayerLeft);
	CreateTexture("paddle-up-down.png", &gCPURight);
	CreateTexture("paddle-left-right.png", &gPlayerBottom);
	CreateTexture("paddle-left-right.png", &gCPUTop);
	CreateTexture("ball.png", &gBall);
}

// This function will render a frame. This does not have the code to UPDATE GAME LOGIC.
void RenderFrame()
{
    gDirect3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, /*D3DCOLOR_XRGB(0, 0, 0)*/gColorArray[5], 1.0f, 0);
    gDirect3DDevice->BeginScene();
	gDirect3DXSprite->Begin(D3DXSPRITE_ALPHABLEND);

	if (gMenu == kGameMenu)
	{
		gDirect3DXSprite->Draw(gPlayerLeft, NULL, NULL, &gPlayerLeftPos, gColorArray[0]);
		gDirect3DXSprite->Draw(gPlayerBottom, NULL, NULL, &gPlayerBottomPos, gColorArray[1]);
		gDirect3DXSprite->Draw(gCPURight, NULL, NULL, &gCPURightPos, gColorArray[2]);
		gDirect3DXSprite->Draw(gCPUTop, NULL, NULL, &gCPUTopPos, gColorArray[3]);
		gDirect3DXSprite->Draw(gBall, NULL, NULL, &gBallPos, gColorArray[4]);
	}

	gDirect3DXSprite->End();

	if (gIsDebugMode)
	{
		frameSum = 0;

		for (auto i = 0; i < kMeasureFrameCount; ++i) { frameSum += gFrameTimes[i]; }

		std::stringstream ss;
		ss << gTimePerFrame << ":" << gTimePerFrame / CLOCKS_PER_SEC << ", " << gFrameBeginTime << ":" << gFrameEndTime << "\r\n"
			<< 10 / ((frameSum / CLOCKS_PER_SEC) / kMeasureFrameCount) << " " << gCurrentFrame << " " << 10 / (gTimePerFrame / CLOCKS_PER_SEC) << "\r\n"
			<< gPlayerLeftPos.x << ", " << gPlayerLeftPos.y << "; " << gPlayerBottomPos.x << ", " << gPlayerBottomPos.y << "\r\n"
			<< gCPURightPos.x << ", " << gCPURightPos.y << "; " << gCPUTopPos.x << ", " << gCPUTopPos.y << "; " << gCPUSpeed << "\r\n"
			<< gPlayerPoints << " " << gCPUPoints << "\r\n"
			<< gBallPredictHorizontal << ", " << gBallPredictVertical << "\r\n"
			<< gBallPos.x << ", " << gBallPos.y << ": " << gBallDirection;
		auto s = ss.str();
		auto cs = s.c_str();

		gDirectXFont->DrawTextA(NULL,
								cs,
								strlen(cs),
								&gTextRect,
								DT_LEFT | DT_TOP,
								0xFFFFFFFF);
	}

	if (gMenu == kGameMenu || gMenu == kWinMenu || gMenu == kLoseMenu)
	{
		std::stringstream ss;
		ss << "P: " << gPlayerPoints << " C: " << gCPUPoints << " D: " << gDifficulty;
		auto s = ss.str();
		auto cs = s.c_str();

		gDirectXFont->DrawTextA(NULL,
								cs,
								strlen(cs),
								&gTextRect,
								DT_CENTER | DT_TOP,
								0xFFFFFFFF);
	}

	if (gShowPointTime > 0)
	{
		auto pointTransp = ((long)gShowPointTime << 2) << 24;

		if (gPlayerLastPoint)
		{
			gDirectXFont2->DrawTextA(NULL,
									"Player point!",
									13,
									&gTextRect,
									kTextMidScreen,
									0x00FF00 | pointTransp);
		}
		else
		{
			gDirectXFont2->DrawTextA(NULL,
									"CPU point!",
									10,
									&gTextRect,
									kTextMidScreen,
									0xFF0000 | pointTransp);
		}

		gShowPointTime--;
	}
	
	if (gMenu == kWinMenu)
	{
		gDirectXFont2->DrawTextA(NULL,
								"Player wins!",
								12,
								&gTextRect,
								kTextMidScreen,
								0xFF00FF00);
	}
	else if (gMenu == kLoseMenu)
	{
		gDirectXFont2->DrawTextA(NULL,
								"CPU wins!",
								9,
								&gTextRect,
								kTextMidScreen,
								0xFFFF0000);
	}
	else if (gMenu == kMainMenu)
	{
		std::stringstream mainMenuStringStream;
		mainMenuStringStream << "Press Enter to begin!\r\n\r\nSet Difficulty (Press TAB to change): " << gDifficulty << "\r\nRelative Difficulty: " << ((1024.0 / kScreenWidth) * (768.0 / kScreenHeight) * gDifficulty) << "\r\n\r\nWhat is the relative difficulty?\r\nThe relative difficulty is calculated based upon the screensize\r\nand selected difficulty level. It is a relative number corresponding\r\nto how hard the game will feel, compared to the base 1024x768\r\ndifficulty 1 version.";
		auto mainMenuString = mainMenuStringStream.str();
		auto cs = mainMenuString.c_str();

		gDirectXFont2->DrawTextA(NULL,
								 cs,
								 strlen(cs),
								 &gTextRect,
								 kTextMidScreen,
								 0xFFFFFFFF);
	}

    gDirect3DDevice->EndScene();
    gDirect3DDevice->Present(NULL, NULL, NULL, NULL);
}

// This will gather our Input Data.
void DetectDirectInput()
{
    gDirectInput8Keyboard->Acquire();
    gDirectInput8Keyboard->GetDeviceState(256, (LPVOID)gKeystate);
}

// This function cleans up the variables we had. The DirectX ones and the COM/C++ ones.
void CleanDirect3D()
{
	gPlayerLeft->Release();		// Close and Release our Player's Left
	gPlayerBottom->Release();	// Close and Release our Player's Bottom
	gCPURight->Release();		// Close and Release our CPU's Right
	gCPUTop->Release();			// Close and Release our CPU's Top
	gBall->Release();			// Close and Release our Ball

	gDirectXFont->Release();		// Close and Release our Font
	gDirect3DXSprite->Release();	// Close and Release our Sprite
    gDirect3DDevice->Release();		// Close and Release our Device
    gDirect3D->Release();			// Finally, Close and Release Direct3D
}

// This will close DirectInput
void CleanDirectInput()
{
    gDirectInput8Keyboard->Unacquire(); // make sure the keyboard is unacquired
    gDirectInput8->Release();			// close DirectInput before exiting
}


void UpdateGameLogic()
{
	SetCPUSpeed();

	if (gMenu == kGameMenu)
	{
		if (gKeystate[DIK_UP] & 0x80) { if (gPlayerLeftPos.y > 16) { gPlayerLeftPos.y -= factor; } }
		if (gKeystate[DIK_DOWN] & 0x80) { if (gPlayerLeftPos.y < kScreenHeight - 16 - 64) { gPlayerLeftPos.y += factor; } }
		if (gKeystate[DIK_LEFT] & 0x80) { if (gPlayerBottomPos.x > 16) { gPlayerBottomPos.x -= factor; } }
		if (gKeystate[DIK_RIGHT] & 0x80) { if (gPlayerBottomPos.x < kScreenWidth - 16 - 64) { gPlayerBottomPos.x += factor; } }
		if (gKeystate[DIK_RETURN] & 0x80)
		{
			if (gKeyDelay == 0)
			{
				gKeyDelay = kKeyDelayReset;
				gBallSpeed = kBallResetSpeed;
			}
		}
	}
	else if (gMenu == kWinMenu || gMenu == kLoseMenu)
	{
		if (gKeystate[DIK_RETURN] & 0x80)
		{
			gKeyDelay = kKeyDelayReset;
			gMenu = kGameMenu;
			gPlayerPoints = 0;
			gCPUPoints = 0;
		}
	}
	else if (gMenu == kMainMenu)
	{
		if (gKeystate[DIK_RETURN] & 0x80)
		{
			gKeyDelay = kKeyDelayReset;
			gMenu = kGameMenu;
			gPlayerPoints = 0;
			gCPUPoints = 0;
		}
	}

	if (gKeystate[DIK_ESCAPE] & 0x80)
	{
		PostQuitMessage(0);
	}
	if (gKeystate[DIK_F1] & 0x80)
	{
		if (gKeyDelay == 0)
		{
			gKeyDelay = kKeyDelayReset;
			gIsDebugMode = !gIsDebugMode;
		}
	}
	if (gKeystate[DIK_TAB] & 0x80)
	{
		if (gKeyDelay == 0)
		{
			gKeyDelay = kKeyDelayReset;
			if (gDifficulty < 4)
			{
				gDifficulty++;
			}
			else
			{
				gDifficulty = 1;
			}

			SetCPUSpeed();
		}
	}

	if (gKeyDelay > 0) { gKeyDelay--; }

	if (gMenu == kGameMenu)
	{
		gBallPos.x = (float)(gBallPos.x + (float)cos(gBallDirection) * gBallSpeed);
		gBallPos.y = (float)(gBallPos.y + (float)sin(gBallDirection) * gBallSpeed);

		for (auto i = 0; i < 4; i++)
		{
			if (i == 0)
			{
				if (gBallPos.y + 8 >= gPlayerLeftPos.y && gBallPos.y <= gPlayerLeftPos.y + 64 && gBallPos.x >= gPlayerLeftPos.x && gBallPos.x <= gPlayerLeftPos.x + 16)
				{
					gBallDirection = (float)((gBallPos.y - (gPlayerLeftPos.y + 32)) / 64) * 2.4 + (kPI * 2);
					ChangeColors(false);
				}
			}
			else if (i == 1)
			{
				if (gBallPos.y + 8 >= gPlayerBottomPos.y && gBallPos.y + 8 <= gPlayerBottomPos.y + 16 && gBallPos.x + 8 >= gPlayerBottomPos.x && gBallPos.x <= gPlayerBottomPos.x + 64)
				{
					gBallDirection = (float)((gBallPos.x - (gPlayerBottomPos.x + 32)) / 64) * 2.4 + (kPI * 1.5);
					ChangeColors(false);
				}
			}
			else if (i == 2)
			{
				if (gBallPos.y + 8 >= gCPURightPos.y && gBallPos.y <= gCPURightPos.y + 64 && gBallPos.x + 8 >= gCPURightPos.x && gBallPos.x + 8 <= gCPURightPos.x + 16)
				{
					gBallDirection = -(float)((gBallPos.y - (gCPURightPos.y + 32)) / 64) * 2.4 + kPI;
					ChangeColors(false);
				}
			}
			else if (i == 3)
			{
				if (gBallPos.y >= gCPUTopPos.y && gBallPos.y <= gCPUTopPos.y + 16 && gBallPos.x + 8 >= gCPUTopPos.x && gBallPos.x <= gCPUTopPos.x + 64)
				{
					gBallDirection = -(float)((gBallPos.x - (gCPUTopPos.x + 32)) / 64) * 2.4 + (kPI * 0.5);
					ChangeColors(false);
				}
			}
		}

		gBallPredictVertical = (int)((kScreenWidth - gBallPos.x) * sin(gBallDirection) + (gBallPos.y));
		gBallPredictHorizontal = (int)(gBallPos.y * cos(gBallDirection) + (gBallPos.x));

		if (gCPUTopPos.x + (32 - kSafeZoneSize) > gBallPredictHorizontal && gCPUTopPos.x > 16)
		{
			gCPUTopPos.x = (float)(gCPUTopPos.x - gCPUSpeed);
		}
		else if (gCPUTopPos.x + (32 + kSafeZoneSize) < gBallPredictHorizontal && gCPUTopPos.x < kScreenWidth - 16 - 64 - 1)
		{
			gCPUTopPos.x = (float)(gCPUTopPos.x + gCPUSpeed);
		}
	
		if (gCPURightPos.y + (32 - kSafeZoneSize) > gBallPredictVertical && gCPURightPos.y > 16)
		{
			gCPURightPos.y = (float)(gCPURightPos.y - gCPUSpeed);
		}
		else if (gCPURightPos.y + (32 + kSafeZoneSize) < gBallPredictVertical && gCPURightPos.y < kScreenHeight - 16 - 64 - 1)
		{
			gCPURightPos.y = (float)(gCPURightPos.y + gCPUSpeed);
		}

		if (gBallPos.x >= 0 && gBallPos.x < kScreenWidth && gBallPos.y < 16)
		{
			GetPoint(true); // Person Won
		}
		else if (gBallPos.x >= 0 && gBallPos.x < kScreenWidth && gBallPos.y > kScreenHeight - 16)
		{
			GetPoint(false); // CPU Won
		}
		else if (gBallPos.y >= 0 && gBallPos.y < kScreenHeight && gBallPos.x < 16)
		{
			GetPoint(false); // CPU Won
		}
		else if (gBallPos.y >= 0 && gBallPos.y < kScreenHeight && gBallPos.x > kScreenWidth - 16)
		{
			GetPoint(true); // Person Won
		}
	}
}

void GetPoint(bool playerPoint)
{
	if (playerPoint)
	{
		gBallDirection = 0.0f;
		gPlayerPoints++;
		gPlayerLastPoint = true;

		if (gPlayerPoints == gScoreTo)
		{
			gMenu = kWinMenu;
		}
	}
	else
	{
		gBallDirection = kPI;
		gCPUPoints++;
		gPlayerLastPoint = false;

		if (gCPUPoints == gScoreTo)
		{
			gMenu = kLoseMenu;
		}
	}

	gBallPos.x = (kScreenWidth >> 1) - 4;
	gBallPos.y = (kScreenHeight >> 1) - 4;
	gBallSpeed = 0.0f;

	if (gMenu == kGameMenu)
	{
		gShowPointTime = kShowPointTimeReset;
	}

	ChangeColors(true);
}

void ChangeColors(bool usePreset)
{
	for (auto i = 0; i < kColors; i++)
	{
		if (usePreset || gDifficulty == 1 || gDifficulty == 2)
		{
			if (i == 5)
			{
				gColorArray[i] = 0xFF000000;
			}
			else
			{
				gColorArray[i] = 0xFFFFFFFF;
			}
		}
		else
		{
			gColorArray[i] = 0xFF000000 | (rand() % kRandFactor * kRandModifier) | (rand() % kRandFactor * kRandModifier << 8) | (rand() % kRandFactor * kRandModifier << 16);
		}
	}
}

void SetCPUSpeed()
{
	gCPUSpeed = 3.0 + (0.75 * gDifficulty);
}
