//
/*

	Project Vape

	Fisheater & Nautical

	V. 01

	OFFSETS 

	Localplayer:	"client.dll"	+ 0x006CC00C
	Playerlist:		"client.dll"	+ 0x0062B104
	CrosshairID:    Localplayer		+ 0x2C40

	Playercount:    "client.dll"	+ 0x60ABE4

	Viewmatrix: "engine.dll" + 0x5D5C54
	Antiflick = Viewmatrix + 0x4C

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

struct Player {

	int health;
	int team;
	int mFlags;
	int isDormant;
	
	Vector origin;

	void ReadMem(HANDLE procHandle,DWORD addr) {

		ReadProcessMemory(procHandle, (DWORD*)(addr + 0x90), &health, sizeof(int), NULL); // read hp
		ReadProcessMemory(procHandle, (DWORD*)(addr + 0x9C), &team, sizeof(int), NULL); // read team
		ReadProcessMemory(procHandle, (DWORD*)(addr + 0x260), &origin, sizeof(Vector), NULL); // read origin
		ReadProcessMemory(procHandle, (DWORD*)(addr + 0x164), &isDormant, sizeof(bool), NULL); // read isDormant
	}
}myPlayer,plyList[64];

struct ViewMatrix {

	float A[4][4];
}worldToScreen;

// Global vars

WindProc game;
int playerCount = 0;
HPEN boxPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 100));
HBRUSH hpBar = CreateSolidBrush(RGB(255, 75, 75));

// Code

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

	int width = (int)(game.windowRect.right - game.windowRect.left);
	int height = (int)(game.windowRect.bottom - game.windowRect.top);

	float x = width / 2;
	float y = height / 2;

	x += 0.5 * to[0] * width + 0.5;
	y -= 0.5 * to[1] * height + 0.5;

	to[0] = x;
	to[1] = y;

	return true;
}

float Distance(Vector start, Vector end)
{
	float deltX = end.x - start.x;
	float deltY = end.y - start.y;
	float deltZ = end.z - start.z;

	return (sqrt(pow(deltX, 2) + pow(deltY, 2) + pow(deltZ, 2)));
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

		SetBkMode(memHDC, TRANSPARENT);
		SetTextAlign(memHDC, TA_CENTER);

		SelectObject(memHDC, boxPen);

		for (int i = 0; i < playerCount; i++) {

			if (plyList[i].isDormant) {

				continue;
			}

			if (plyList[i].team == 1002) {

				continue;
			}

			if (plyList[i].health <= 1) {

				continue;
			}

			float dist = Distance(plyList[i].origin, myPlayer.origin);

			if (dist < 1.0f) { // this is localplayer

				continue;
			}

			float * scrnPos = new float[2];

			if (WorldToScreenConvert(plyList[i].origin, scrnPos)) { // if they're on screen the do the works....

				// calc box size
				int width = 20000 / dist;
				int height = 50000 / dist;

				// draw box + hp bar
				Rectangle(memHDC, scrnPos[0] - width, scrnPos[1] - height, scrnPos[0] + width, scrnPos[1]);
				Rectangle(memHDC, scrnPos[0] - width - width / 3, scrnPos[1] - height, scrnPos[0] - width, scrnPos[1]);

				SelectObject(memHDC, hpBar);

				float frac = plyList[i].health / 100.0f;

				Rectangle(memHDC, scrnPos[0] - width - width / 3, scrnPos[1] - height * frac, scrnPos[0] - width, scrnPos[1]);
				SelectObject(memHDC, GetStockObject(HOLLOW_BRUSH));
				
				// draw dist info

				std::string distStr = std::string("Dist: ");
				distStr.append(std::to_string((int)dist));

				std::string teamStr = std::string("Team: ");
				teamStr.append(std::to_string((int)plyList[i].team));


				// Outlined text
				SetTextColor(memHDC, RGB(0, 0, 0));
				TextOutA(memHDC, scrnPos[0] + 1, scrnPos[1] - height - 19, distStr.c_str(), distStr.length());
				SetTextColor(memHDC, RGB(0, 255, 100));
				TextOutA(memHDC, scrnPos[0], scrnPos[1] - height - 20, distStr.c_str(), distStr.length());

				TextOutA(memHDC, scrnPos[0], scrnPos[1] - height - 40, teamStr.c_str(), teamStr.length());
			}

			delete[] scrnPos;
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
		InvalidateRect(hwnd, NULL, FALSE);
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

int main()
{

	printf("Startup..\n");

	while (!game.AttachToWindow("Garry's Mod")) {

		Sleep(1000);
	}

	while (!game.AttachToProcess()) {

		Sleep(1000);
	}

	game.CreateModuleSnapshot();

	HWND overlay = game.PrepOverlay("Project Vapor", OverlayCallback);

	MODULEENTRY32 client = game.GetModuleByName("client.dll");
	MODULEENTRY32 engine = game.GetModuleByName("engine.dll");

	DWORD localplyAddr;
	ReadProcessMemory(game.procHandle, (DWORD*)(client.modBaseAddr + 0x006CC00C), &localplyAddr, sizeof(DWORD), NULL);
	
	int antiflick = 1;
	
	while (true) {

		// Read antiflick for view matrix
		ReadProcessMemory(game.procHandle, (DWORD*)(engine.modBaseAddr + 0x5D5C54 + 0x4C), &antiflick, sizeof(int), NULL);

		if (antiflick == 1) {

			// Read view matrix
			ReadProcessMemory(game.procHandle, (DWORD*)(engine.modBaseAddr + 0x5D5C54), &worldToScreen, sizeof(ViewMatrix), NULL);
		}

		// Grab ply count
		ReadProcessMemory(game.procHandle, (DWORD*)(client.modBaseAddr + 0x60ABE4), &playerCount, sizeof(int), NULL);

		// Read localply
		myPlayer.ReadMem(game.procHandle, localplyAddr);

		// Player list loop
		for (int i = 0; i < playerCount; i++) {

			DWORD plyAddr;
			ReadProcessMemory(game.procHandle, (DWORD*)((client.modBaseAddr + 0x0062B104) + 0x10 * i), &plyAddr, sizeof(DWORD), NULL);

			plyList[i].ReadMem(game.procHandle, plyAddr);
		}

		// take care of window messages

		if (GetMessage(&game.overlayMsg, overlay, 0, 0) > 0) // return value <= 0 indicates a problem
		{
			// sends queued messages to our overlay window so it knows when to do shit, like paint or close, etc.
			TranslateMessage(&game.overlayMsg);
			DispatchMessage(&game.overlayMsg);
		}
	}
	
	
	system("pause");
	game.Detatch();
	return 0;
}

/* Viewmatrix finder
DWORD offset = 0x0;

for (; offset < 0x999999; offset += 0x4) {

ReadProcessMemory(game.procHandle, (DWORD*)(engine.modBaseAddr + offset), &worldToScreen, sizeof(ViewMatrix), NULL);

if (worldToScreen.A[1][1] > 1.332f && worldToScreen.A[1][1] < 1.334f) {


printf("0x%x\n", offset);
}
}
*/