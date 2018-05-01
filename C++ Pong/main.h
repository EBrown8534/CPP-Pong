#pragma once
#include <d3dx9.h>

void ChangeColors(bool);
void CleanDirect3D();
void CleanDirectInput();
void CreateFont(LPCSTR, LPD3DXFONT*, bool);
int CreateTexture(LPCSTR, LPDIRECT3DTEXTURE9*);
void DetectDirectInput();
void GetPoint(bool);
void InitializeDirect3D(HWND);
void InitializeDirectInput(HINSTANCE, HWND);
void InitializeGraphics();
void LoadContent();
void RenderFrame();
void SetCPUSpeed();
void UpdateGameLogic();
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
