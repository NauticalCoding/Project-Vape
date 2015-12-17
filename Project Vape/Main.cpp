//
/*

	Project Vape

	Fisheater & Nautical

	V. 01

	OFFSETS 

	Localplayer:	"client.dll"	+ 0x006D0D7C
	Playerlist:		"client.dll"	+ 0x0062FF3C
	CrosshairID:    Localplayer		+ 0x2C40

	Viewmatrix: "engine.dll" + 0x5D5C54
	Antiflick = Viewmatrix + 0x4C

	Eyeangles:    "engine.dll" + 0x4B5484

	PlayerStruct offsets

	Health: 0x90
	Team: 0x9C
	Origin: 0x260
*/

#include "WindowProcess.h"
#include <string>

// Structures

struct Vector {

	float x;
	float y;
	float z;
};

struct Angle {

	float p;
	float y;
	float r;
};

struct Player {

	int health;
	int team;
	int mFlags;

	Vector origin;

	void ReadMem(HANDLE procHandle,DWORD addr) {

		ReadProcessMemory(procHandle, (DWORD*)(addr + 0x90), &health, sizeof(int), NULL); // read hp
		ReadProcessMemory(procHandle, (DWORD*)(addr + 0x9C), &team, sizeof(int), NULL); // read team
		ReadProcessMemory(procHandle, (DWORD*)(addr + 0x260), &origin, sizeof(Vector), NULL); // read origin
	}
}myPlayer,plyList[64];

struct ViewMatrix {

	float A[4][4];
}worldToScreen;


// Global vars

WindProc game;
int playerCount			= 64;
HPEN boxPen				= CreatePen(PS_SOLID, 2, RGB(0, 255, 100));
HBRUSH hpBar			= CreateSolidBrush(RGB(255, 75, 75));

int triggerShootDelay	= 25; // 25 ticks or milliseconds
int lastShot			= 0;

bool visualsOn			= false;
bool triggerbotOn		= false;
DWORD aimbotKey			= NULL;
bool aimbotOn			= false;

bool bindPressed		= false;
int bindPressedTick		= 0; // when was the bind pressed?


// Code - Other
float Distance(Vector start, Vector end)
{
	float deltX = end.x - start.x;
	float deltY = end.y - start.y;
	float deltZ = end.z - start.z;

	return (sqrt(pow(deltX, 2) + pow(deltY, 2) + pow(deltZ, 2)));
}

float Distance2D(Vector start, Vector end)
{
	float deltX = end.x - start.x;
	float deltY = end.y - start.y;

	return (sqrt(pow(deltX, 2) + pow(deltY, 2)));
}

Angle VecToAngle(Vector start, Vector end)
{

	Angle answer;

	end.z = end.z + 10.0f;

	float deltX = end.x - start.x;
	float deltY = end.y - start.y;
	float deltZ = end.z - start.z;

	float dist3D = Distance(start, end);
	float dist2D = Distance2D(start, end);

	answer.p = asinf(deltZ / dist3D) * (180.0f / 3.14f);
	answer.y = -asinf(deltY / dist2D) * (180.0f / 3.14f);
	answer.r = 0;

	if (deltX > 0) {

		answer.y = 180.0f - answer.y;
	}

	if (answer.y > 180.0f) {

		answer.y = -(360.0f - answer.y);
	}

	return answer;
}

// Code - Keybinds

void RunKeyBinds()
{

	if (aimbotKey == NULL) { // we need to choose an aimbot key..

		for (DWORD i = 0x0; i < 0x80; i += 0x1) { // loop throuh all keys

			if (GetAsyncKeyState(i) != 0) { // this is our key

				aimbotKey = i;
				printf("Aimbot binded to key: 0x%x\n", aimbotKey);
				break;
			}
		}

		return;
	}

	// aimbot
	if (GetAsyncKeyState(aimbotKey) != 0) {

		aimbotOn = true;
	}
	else {

		aimbotOn = false;
	}

	// reset 
	if (bindPressed && bindPressedTick < GetTickCount() - 500) {

		bindPressed = false;
	}

	// esp
	if (GetAsyncKeyState(VK_NUMPAD0) != 0 && !bindPressed) {

		bindPressed = true;
		bindPressedTick = GetTickCount();

		visualsOn = !visualsOn;
	}

	// triggerbot
	if (GetAsyncKeyState(VK_NUMPAD1) != 0 && !bindPressed) {

		bindPressed = true;
		bindPressedTick = GetTickCount();

		triggerbotOn = !triggerbotOn;
	}

}

// Code - Aim
void RunAim(DWORD engine)
{

	if (!aimbotOn) {

		return;
	}

	float angDiff = 0;
	Angle eyeAngles;
	Player targ;
	targ.health = -1337;

	// read eye angles
	ReadProcessMemory(game.procHandle, (DWORD*)(engine + 0x4B5484), &eyeAngles, sizeof(Angle), NULL);

	for (int i = 0; i < playerCount; i++) {

		if (plyList[i].health <= 1) { // they're dead

			continue;
		}

		if (Distance(myPlayer.origin, plyList[i].origin) < 2) { // this is localplayer

			continue;
		}

		if (plyList[i].team == 1002) { // this is a spectator

			continue;
		}

		Angle tempAng = VecToAngle(plyList[i].origin, myPlayer.origin);

		float tempAngDiff = abs(tempAng.y - eyeAngles.y) + abs(tempAng.p - eyeAngles.p);

		if (targ.health == -1337 || tempAngDiff < angDiff) {

			targ = plyList[i];
			angDiff = tempAngDiff;
		}
	}

	if (targ.health == -1337) { // no target found

		return;
	}

	Angle angToTarg = VecToAngle(targ.origin, myPlayer.origin);
	WriteProcessMemory(game.procHandle, (DWORD*)(engine + 0x4B5484), &angToTarg, sizeof(Angle), NULL);
}

// Code - Triggerbot
void TriggerBot()
{
	if (lastShot < GetTickCount() - triggerShootDelay) { // this fixes a bug where the triggerbot tries to shoot too fast and fails

		INPUT input;

		// left click press
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		SendInput(1, &input, sizeof(INPUT));

		ZeroMemory(&input, sizeof(INPUT)); // fill the memory address of 'input' with zeros

		// left click release
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
		SendInput(1, &input, sizeof(INPUT));

		lastShot = GetTickCount();
	}
}

bool CanTriggerBot(DWORD localplyAddr) {

	if (!triggerbotOn) { // if triggerbot isnt toggled..

		return false;
	}

	// read the CrosshairID
	int crosshairID;
	ReadProcessMemory(game.procHandle, (DWORD*)((localplyAddr + 0x2C40)), &crosshairID, sizeof(int), NULL);

	if (crosshairID == 0 || myPlayer.health <= 0 || crosshairID > playerCount) 
	{ 
		return false; // crosshairID: 0 = world, crosshairID > playercount = invalid target
	} 

	return true;
}

// Code - Drawing
bool WorldToScreenConvert(Vector from, float * to)
{

	float w = 0.0f;

	to[0] = worldToScreen.A[0][0] * from.x + worldToScreen.A[0][1] * from.y + worldToScreen.A[0][2] * from.z + worldToScreen.A[0][3];
	to[1] = worldToScreen.A[1][0] * from.x + worldToScreen.A[1][1] * from.y + worldToScreen.A[1][2] * from.z + worldToScreen.A[1][3];

	w = worldToScreen.A[3][0] * from.x + worldToScreen.A[3][1] * from.y + worldToScreen.A[3][2] * from.z + worldToScreen.A[3][3];

	if (w < 0.01f) {

		return false; // not in screen
	}

	float invW = 1.0f / w;

	to[0] *= invW;
	to[1] *= invW;

	float width = (float)(game.windowRect.right - game.windowRect.left);
	float height = (float)(game.windowRect.bottom - game.windowRect.top);

	float x = width / 2;
	float y = height / 2;

	x += 0.5 * to[0] * width + 0.5;
	y -= 0.5 * to[1] * height + 0.5;

	to[0] = x;
	to[1] = y;

	return true;
}

void DrawOutlinedText(HDC hdc, int x, int y, std::string text, COLORREF col)
{

	SetTextColor(hdc, RGB(0, 0, 0));
	TextOutA(hdc, x - 1, y - 1, text.c_str(), text.length());
	SetTextColor(hdc, col);
	TextOutA(hdc, x, y, text.c_str(), text.length());
}

LRESULT CALLBACK OverlayCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{

	case WM_PAINT:
	{

		PAINTSTRUCT paintStruc;
		HDC hdc = BeginPaint(hwnd, &paintStruc);
		HDC memHDC = CreateCompatibleDC(hdc);
		HBITMAP memBitMap = CreateCompatibleBitmap(hdc, game.windowWidth, game.windowHeight);
		SelectObject(memHDC, memBitMap);

		FillRect(memHDC, &game.bitMapRect, WHITE_BRUSH); // make memory dc see transparent

		if (visualsOn) {

			DrawOutlinedText(memHDC, 100, 15, std::string("Visuals: On"), RGB(0, 255, 0));
		}
		else {

			DrawOutlinedText(memHDC, 100, 15, std::string("Visuals: Off"), RGB(255, 0, 0));
		}

		if (triggerbotOn) {

			DrawOutlinedText(memHDC, 100, 35, std::string("Triggerbot: On"), RGB(0, 255, 0));
		}
		else {

			DrawOutlinedText(memHDC, 100, 35, std::string("Triggerbot: Off"), RGB(255, 0, 0));
		}

		if (aimbotOn) {

			DrawOutlinedText(memHDC, 100, 55, std::string("Aimbot: On"), RGB(0, 255, 0));
		}
		else {

			DrawOutlinedText(memHDC, 100, 55, std::string("Aimbot: Off"), RGB(255, 0, 0));
		}

		if (visualsOn) {
			SetBkMode(memHDC, TRANSPARENT);
			SetTextAlign(memHDC, TA_CENTER);

			SelectObject(memHDC, boxPen);

			for (int i = 0; i < playerCount; i++) {

				if (plyList[i].team == 1002) {

					continue;
				}

				if (plyList[i].health <= 1) {

					continue;
				}

				float dist = Distance(plyList[i].origin, myPlayer.origin);

				if (dist < 1.0f || dist > 5000.0f) { // this is localplayer

					continue;
				}

				float * scrnPos = new float[2];

				if (WorldToScreenConvert(plyList[i].origin, scrnPos)) { // if they're on screen the do the works....

					// calc box size
					int width = 20000 / dist;
					int height = 50000 / dist;

					// draw box + hp bar
					Rectangle(memHDC, scrnPos[0] - width, scrnPos[1] - height, scrnPos[0] + width, scrnPos[1]);
					Rectangle(memHDC, scrnPos[0] - width - width / 2, scrnPos[1] - height, scrnPos[0] - width, scrnPos[1]);

					SelectObject(memHDC, hpBar);

					float frac = plyList[i].health / 100.0f;
					if (frac > 1) {
						frac = 1.0f;
					}

					Rectangle(memHDC, scrnPos[0] - width - width / 2, scrnPos[1] - height * frac, scrnPos[0] - width, scrnPos[1]);
					SelectObject(memHDC, GetStockObject(HOLLOW_BRUSH));

					// draw dist info

					std::string distStr = std::string("Dist: ");
					distStr.append(std::to_string((int)dist));

					// Outlined text
					DrawOutlinedText(memHDC, scrnPos[0], scrnPos[1] - height - 20, distStr, RGB(0, 255, 100));

				}

				delete[] scrnPos;
			}
		}

		// Copy dat shit
		BitBlt(hdc, 0, 0, game.windowWidth, game.windowHeight, memHDC, 0, 0, SRCCOPY); // copy from our memory dc to our actual dc

		// Free memory
		DeleteObject(memBitMap);
		DeleteDC(memHDC);
		DeleteDC(hdc);
		EndPaint(hwnd, &paintStruc); // this method deletes the dc returned by BeginPaint
	}

	case WM_ERASEBKGND:
	{
		InvalidateRect(hwnd, NULL, false);
		return 1;
	}

	case WM_CLOSE:
	{

		DestroyWindow(hwnd);
		break;
	}

	case WM_DESTROY:
	{

		PostQuitMessage(0);
		break;
	}

	default:
	{

		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	}

	return 0;
}

// Code - Main
int main()
{

	printf("Loading Project Vapor by Nautical & Fisheater.");

	while (!game.AttachToWindow("Garry's Mod")) {
		
		Sleep(1000);
	}

	while (!game.AttachToProcess()) {

		Sleep(1000);
	}

	game.CreateModuleSnapshot();

	HWND overlay = game.PrepOverlay("Project Vapor by Nautical & Fisheater", OverlayCallback);

	MODULEENTRY32 client = game.GetModuleByName("client.dll");
	MODULEENTRY32 engine = game.GetModuleByName("engine.dll");

	DWORD localplyAddr;
	ReadProcessMemory(game.procHandle, (DWORD*)(client.modBaseAddr + 0x006D0D7C), &localplyAddr, sizeof(DWORD), NULL);
	
	int antiflick = 1;
	
	Sleep(1000);
	printf("\nPlease press a key to bind your aimbot. \n");

	while (true) {

		// Read antiflick for view matrix
		ReadProcessMemory(game.procHandle, (DWORD*)(engine.modBaseAddr + 0x5DFB54 + 0x4C), &antiflick, sizeof(int), NULL);

		if (antiflick == 1) {

			// Read view matrix
			ReadProcessMemory(game.procHandle, (DWORD*)(engine.modBaseAddr + 0x5DFB54), &worldToScreen, sizeof(ViewMatrix), NULL);
		}

		// Read localply
		myPlayer.ReadMem(game.procHandle, localplyAddr);

		// Player list loop
		for (int i = 0; i < playerCount; i++) {

			// clean the structure of old data
			plyList[i] = Player();

			DWORD plyAddr;
			ReadProcessMemory(game.procHandle, (DWORD*)((client.modBaseAddr + 0x0062FF3C) + 0x10 * i), &plyAddr, sizeof(DWORD), NULL);

			plyList[i].ReadMem(game.procHandle, plyAddr);
		}

		// take care of window messages

		if (GetMessage(&game.overlayMsg, overlay, 0, 0) > 0) // return value <= 0 indicates a problem
		{
			// sends queued messages to our overlay window so it knows when to do shit, like paint or close, etc.
			TranslateMessage(&game.overlayMsg);
			DispatchMessage(&game.overlayMsg);
		}

		// bind loop
		RunKeyBinds();

		// aimbot
		RunAim((DWORD)engine.modBaseAddr);

		// trigger bot
		if (CanTriggerBot(localplyAddr)) { // check if we should shoot..

			TriggerBot(); // simulate a click
		}
	}
	
	game.Detatch();
	system("pause");
	return 0;
}

/* Viewmatrix finder

DWORD offset = 0x0;

for (; offset < 0x999999; offset += 0x4) {

ReadProcessMemory(game.procHandle, (DWORD*)(engine.modBaseAddr + offset), &worldToScreen, sizeof(ViewMatrix), NULL);

if (worldToScreen.A[1][1] > 1.331f && worldToScreen.A[1][1] < 1.334f) {


printf("0x%x\n", offset);
}
}
*/