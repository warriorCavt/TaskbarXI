// TaskbarXI for Windows 11.
// By Chris Andriessen 2022
// https://github.com/ChrisAnd1998/TaskbarXI

#pragma comment (lib, "dwmapi.lib")

#define WIN32_LEAN_AND_MEAN
#define NOT_BUILD_WINDOWS_DEPRECATE

#include <iostream>
#include <thread>
#include <dwmapi.h>
#include <string> 

#include <shellapi.h>
#include "resource.h"
#define APPWM_ICONNOTIFY (WM_APP + 1)
#define WM_NOTIFY_TB 3141
NOTIFYICONDATA nid = {};

std::string cur_dir;
std::string cur_cmd;

int working = -1;

int eventtrigger;

BOOL CALLBACK EnumCallbackTaskbars(HWND hWND, LPARAM lParam);
BOOL CALLBACK EnumCallbackMaximized(HWND hWND, LPARAM lParam);
BOOL CALLBACK EnumCallbackInstances(HWND hWND, LPARAM lParam);

std::thread thread_List[10];
HWND animating_List[10];
HWND maximized_List[10];
HWND taskbar_List[10];
int thread_Count;
int animating_Count;
int taskbar_Count;
int maximized_Count;

int mtaskbar_Revert;
int staskbar_Revert;

int maxCountChanged;
int oldMaxCount;

int trayleft;

int taskbariscenter = 1;

int isstore;

bool boxopen;

void initTray(HWND parent);
VOID SetTaskbar();

int square;
int corner_Radius;
int ignoremax;
int notray;
int hidetraywnd;
int stop;
int restart;
int createstartup;
int removestartup;
int sticky;
int smoothresize;
int blur;
int expandspeed;
int shrinkspeed;

const HINSTANCE hModule = LoadLibrary(TEXT("user32.dll"));

struct ACCENTPOLICY
{
	int nAccentState;
	int nFlags;
	int nColor;
	int nAnimationId;
};
struct WINCOMPATTRDATA
{
	int nAttribute;
	PVOID pData;
	ULONG ulDataSize;
};

void callSetTaskbar() {
	std::thread{ SetTaskbar }.detach();
}

int fixedTaskbarWidth = -1;

int __cdecl main(int argc, char** argv) {
	return WinMain(0, 0, NULL, 0);
}

typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hModule, "SetWindowCompositionAttribute");

void SetWindowBlur()
{
	if (hModule)
	{
		if (SetWindowCompositionAttribute)
		{
			ACCENTPOLICY policy = { 3, 0, 0, 0 };
			WINCOMPATTRDATA data = { 19, &policy, sizeof(ACCENTPOLICY) };
			for (;;) {
				for (HWND tb : taskbar_List) {
					if (tb != 0) {
						SetWindowCompositionAttribute(tb, &data);
					}
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(14));
			}
		}
	}
}

VOID CALLBACK WinEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
	if (eventtrigger == 0) {
		if (working == 0) {
			eventtrigger = 1;
			int length = 20;
			wchar_t* title = new wchar_t[length];
			GetClassName(hwnd, title, length);
			if (wcscmp(title, L"MSTaskListWClass") == 0) {
				callSetTaskbar();
			}
			if (wcscmp(title, L"MSTaskSwWClass") == 0) {
				callSetTaskbar();
			}
			if (wcscmp(title, L"ToolbarWindow32") == 0) {
				callSetTaskbar();
			}
			title = NULL;
			free(title);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			eventtrigger = 0;
		}
	}
}

void exiting() {
	std::wcout << "Exiting TaskbarXI..." << std::endl;
	Shell_NotifyIcon(NIM_DELETE, &nid);
	for (HWND tb : taskbar_List) {
		if (tb != 0) {
			RECT rect_tb;
			GetWindowRect(tb, &rect_tb);

			INT curDPI = GetDpiForWindow(tb) * 1.041666666666667;

			HRGN region_Empty = CreateRectRgn(abs(rect_tb.left - rect_tb.left) * curDPI / 100, 0, abs(rect_tb.right - rect_tb.left) * curDPI / 100, rect_tb.bottom * curDPI / 100);
			SetWindowRgn(tb, region_Empty, TRUE);

			HWND Shell_TrayWnd = FindWindow(L"Shell_TrayWnd", 0);
			HWND TrayNotifyWnd = FindWindowEx(Shell_TrayWnd, 0, L"TrayNotifyWnd", NULL);

			HWND SysPager = FindWindowEx(TrayNotifyWnd, 0, L"SysPager", NULL);
			HWND ToolbarWindow32 = FindWindowEx(TrayNotifyWnd, 0, L"ToolbarWindow32", NULL);
			HWND Button = FindWindowEx(TrayNotifyWnd, 0, L"Button", NULL);

			HWND RebarWindow32 = FindWindowEx(tb, 0, L"RebarWindow32", NULL);
			HWND WorkerW = FindWindowEx(tb, 0, L"WorkerW", NULL);
			HWND MSTaskSwWClass = FindWindowEx(tb, 0, L"MSTaskSwWClass", NULL);
			HWND MSTaskListWClass = FindWindowEx(tb, 0, L"MSTaskListWClass", NULL);

			if (RebarWindow32 != 0) {
				SendMessage(RebarWindow32, WM_SETREDRAW, TRUE, NULL);
			}

			if (WorkerW != 0) {
				SendMessage(WorkerW, WM_SETREDRAW, TRUE, NULL);
			}

			if (MSTaskSwWClass != 0) {
				SendMessage(MSTaskSwWClass, WM_SETREDRAW, TRUE, NULL);
			}

			if (MSTaskListWClass != 0) {
				SendMessage(MSTaskListWClass, WM_SETREDRAW, TRUE, NULL);
			}

			SendMessage(GetParent(tb), WM_SETREDRAW, TRUE, NULL);
			SendMessage(GetParent(GetParent(tb)), WM_SETREDRAW, TRUE, NULL);

			ShowWindow(ToolbarWindow32, SW_SHOW);
			ShowWindow(SysPager, SW_SHOW);
			ShowWindow(Button, SW_SHOW);

			SendMessage(tb, WM_THEMECHANGED, TRUE, NULL);
			SendMessage(tb, WM_SETTINGCHANGE, TRUE, NULL);
		}
	}

	exit(0);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (boxopen == true) {
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	switch (uMsg)
	{
	case APPWM_ICONNOTIFY:
	{
		switch (lParam)
		{
		case WM_LBUTTONUP:
			callSetTaskbar();
			if (isstore == 1) {
				std::string storedir = "\\";
				WinExec(("powershell.exe start shell:AppsFolder" + storedir + "40210ChrisAndriessen.Taskbar11_y1dazs5f5wq00!TaskbarXIGUI").c_str(), SW_HIDE);
			}
			else {
				std::wstring chars = L"";
				chars += (wchar_t)34;
				std::string quote(chars.begin(), chars.end());
				std::string guidir = cur_dir;
				guidir.replace(guidir.find("TaskbarXI.exe"), sizeof("TaskbarXI.exe") - 1, "TaskbarXIMFCGUI.exe");
				WinExec((quote + guidir + quote).c_str(), SW_HIDE);
			}
			break;
		case WM_RBUTTONUP:
			boxopen = true;
			callSetTaskbar();
			if (MessageBox(NULL, L"Do you want to EXIT TaskbarXI?", L"TaskbarXI", MB_YESNO) == IDYES)
			{
				exiting();
			}
			else {
				boxopen = false;
			}
			break;
		}
	}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void create_startup() {
	if (isstore == 0) {
		std::wstring chars = L"";
		chars += (wchar_t)34;
		std::string quote(chars.begin(), chars.end());
		std::string path = quote;
		path.append(cur_dir);
		path.append(quote);
		path.append(cur_cmd);
		HKEY hkey = NULL;
		LONG createStatus = RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey);
		LONG status = RegSetValueExA(hkey, "TaskbarXI", 0, REG_SZ, (LPBYTE)path.c_str(), (path.size() + 1) * sizeof(wchar_t));
		RegCloseKey(hkey);
	}
	if (isstore == 1) {
		std::string storepath = "Explorer.exe taskbarxi:";
		std::string path = storepath.append(cur_cmd);
		WinExec(("powershell.exe Set-Itemproperty -path 'HKCU:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run' -Name 'TaskbarXI' -value '" + path + "'").c_str(), SW_HIDE);
	}
}

void remove_startup() {
	if (isstore == 0) {
		HKEY hkey = NULL;
		LONG createStatus = RegOpenKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey);
		LONG status = RegDeleteValue(hkey, L"TaskbarXI");
		RegCloseKey(hkey);
		exiting();
	}
	if (isstore == 1) {
		std::string tt = "";
		WinExec(("powershell.exe Remove-ItemProperty -path 'HKCU:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run' -Name 'TaskbarXI'" + tt).c_str(), SW_HIDE);
	}
}

HRESULT UpdateWindows11RoundCorners(HWND hWnd)
{
	typedef HRESULT(WINAPI* PFNSETWINDOWATTRIBUTE)(HWND hWnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
	enum DWMWINDOWATTRIBUTE
	{
		DWMWA_WINDOW_CORNER_PREFERENCE = 33
	};
	enum DWM_WINDOW_CORNER_PREFERENCE
	{
		DWMWCP_DEFAULT = 0,
		DWMWCP_DONOTROUND = 1,
		DWMWCP_ROUND = 2,
		DWMWCP_ROUNDSMALL = 3
	};
	HMODULE hDwmApi = ::LoadLibrary(TEXT("dwmapi.dll"));
	if (hDwmApi)
	{
		auto* pfnSetWindowAttribute = reinterpret_cast<PFNSETWINDOWATTRIBUTE>(
			::GetProcAddress(hDwmApi, "DwmSetWindowAttribute"));
		if (pfnSetWindowAttribute)
		{
			auto preference = static_cast<DWM_WINDOW_CORNER_PREFERENCE>(2);
			return pfnSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE,
				&preference, sizeof(DWM_WINDOW_CORNER_PREFERENCE));
		}
		::FreeLibrary(hDwmApi);
	}
	return E_FAIL;
}

std::string animating;

void SetWindowRegionAnimated(HWND hWND, HRGN region) {
	try {
		INT curDPI = GetDpiForWindow(hWND) * 1.041666666666667;
		HRGN currenttbreg = CreateRectRgn(0, 0, 0, 0);
		RECT currenttbrect;
		RECT newtbrect;

		GetWindowRgn(hWND, currenttbreg);
		GetRgnBox(currenttbreg, &currenttbrect);
		GetRgnBox(region, &newtbrect);

		int makebigger;

		if (currenttbrect.right == 0) {
			SetWindowRgn(hWND, region, TRUE);
			GetWindowRgn(hWND, currenttbreg);
			GetRgnBox(currenttbreg, &currenttbrect);
			GetRgnBox(region, &newtbrect);
		}

		if (abs((currenttbrect.right * curDPI / 100) - (newtbrect.right)) >= 100) {
			SetWindowRgn(hWND, region, TRUE);
			return;
		}

		if (currenttbrect.right * curDPI / 100 <= newtbrect.right) {
			makebigger = 1;
		}
		else {
			makebigger = 0;
		}

		int left = abs(currenttbrect.left * curDPI / 100);
		int top = 0;
		int right = abs(currenttbrect.right * curDPI / 100);
		int bottom = newtbrect.bottom;

		if (newtbrect.right == 0) {
			SetWindowRgn(hWND, region, TRUE);
			return;
		}

		for (;;) {
			int currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

			if (taskbariscenter == 1) {
				if (left == newtbrect.left) {
					SetWindowRgn(hWND, region, TRUE);
					break;
				}
			}

			if (right == newtbrect.right) {
				SetWindowRgn(hWND, region, TRUE);
				break;
			}

			if (makebigger == 1) {
				if (taskbariscenter == 1) {
					if (left <= newtbrect.left) {
						SetWindowRgn(hWND, region, TRUE);
						break;
					}
				}

				if (right >= newtbrect.right) {
					SetWindowRgn(hWND, region, TRUE);
					break;
				}

				if (taskbariscenter == 1) {
					left = abs(left - 1);
				}

				right = abs(right + 1);
			}
			else {
				if (taskbariscenter == 1) {
					if (left >= newtbrect.left) {
						SetWindowRgn(hWND, region, TRUE);
						break;
					}
				}

				if (right <= newtbrect.right) {
					SetWindowRgn(hWND, region, TRUE);
					break;
				}

				if (taskbariscenter == 1) {
					left = abs(left + 1);
				}

				right = abs(right - 1);
			}

			if (makebigger == 1) {
				for (;;) {
					int elapsed = (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) - currentTime;
					int speed = expandspeed;
					if (taskbariscenter == 0) {
						speed = expandspeed / 12;
					}
					if (elapsed >= speed / curDPI) {
						break;
					}
					elapsed = NULL;
					speed = NULL;
				}
			}
			else {
				for (;;) {
					int elapsed = (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) - currentTime;
					int speed = shrinkspeed;
					if (taskbariscenter == 0) {
						speed = shrinkspeed / 12;
					}
					if (elapsed >= speed / curDPI) {
						break;
					}
					elapsed = NULL;
					speed = NULL;
				}
			}

			if (square == 0) {
				HRGN framereg = CreateRoundRectRgn(left, top, right, bottom, corner_Radius, corner_Radius);
				SetWindowRgn(hWND, framereg, TRUE);
				framereg = NULL;
			}
			else {
				HRGN framereg = CreateRectRgn(left, top, right, bottom);
				SetWindowRgn(hWND, framereg, TRUE);
				framereg = NULL;
			}

			currentTime = NULL;
		}

		SetWindowRgn(hWND, region, TRUE);

		SendMessage(hWND, WM_WINDOWPOSCHANGED, TRUE, 0);
		SendMessage(hWND, WM_PARENTNOTIFY, 0x00000201, 0x0039065A);

		SetWindowRgn(hWND, region, TRUE);

		curDPI = NULL;
		currenttbreg = NULL;

		left = NULL;
		top = NULL;
		right = NULL;
		bottom = NULL;

		return;
	}

	catch (...) { SetWindowRgn(hWND, region, TRUE); }
}

void taskbarLoop() {
	MSG msg;
	for (;;) {
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		callSetTaskbar();
	}
}

int WINAPI WinMain(_In_opt_ HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	FreeConsole();

	LPWSTR* szArgList;
	int argCount;

	szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);

	for (int i = 0; i < argCount; i++)
	{
		if (wcscmp(szArgList[i], L"-stop") == 0) {
			stop = 1;
		}
		if (wcscmp(szArgList[i], L"-restart") == 0) {
			restart = 1;
		}
		if (wcscmp(szArgList[i], L"-square") == 0) {
			square = 1;
			cur_cmd.append(" -square");
		}
		if (wcscmp(szArgList[i], L"-radius") == 0) {
			corner_Radius = _wtoi(szArgList[++i]);
			std::string xcr = std::to_string(corner_Radius);
			cur_cmd.append(" -radius " + xcr);
		}
		if (wcscmp(szArgList[i], L"-ignoremax") == 0) {
			ignoremax = 1;
			cur_cmd.append(" -ignoremax");
		}
		if (wcscmp(szArgList[i], L"-notray") == 0) {
			notray = 1;
			cur_cmd.append(" -notray");
		}
		if (wcscmp(szArgList[i], L"-hidetraywnd") == 0) {
			hidetraywnd = 1;
			cur_cmd.append(" -hidetraywnd");
		}
		if (wcscmp(szArgList[i], L"-sticky") == 0) {
			sticky = 1;
			cur_cmd.append(" -sticky");
		}
		if (wcscmp(szArgList[i], L"-smoothresize") == 0) {
			smoothresize = 1;
			cur_cmd.append(" -smoothresize");
		}
		if (wcscmp(szArgList[i], L"-expandspeed") == 0) {
			expandspeed = _wtoi(szArgList[++i]);
			std::string xcr = std::to_string(expandspeed);
			cur_cmd.append(" -expandspeed " + xcr);
		}
		if (wcscmp(szArgList[i], L"-shrinkspeed") == 0) {
			shrinkspeed = _wtoi(szArgList[++i]);
			std::string xcr = std::to_string(shrinkspeed);
			cur_cmd.append(" -shrinkspeed " + xcr);
		}

		if (wcscmp(szArgList[i], L"-blur") == 0) {
			blur = 1;
			cur_cmd.append(" -blur");
		}
		if (wcscmp(szArgList[i], L"-createstartup") == 0) {
			createstartup = 1;
		}
		if (wcscmp(szArgList[i], L"-removestartup") == 0) {
			removestartup = 1;
		}
		if (wcscmp(szArgList[i], L"-console") == 0) {
			AllocConsole();
			FILE* fpstdin = stdin, * fpstdout = stdout, * fpstderr = stderr;
			freopen_s(&fpstdout, "CONOUT$", "w", stdout);
			freopen_s(&fpstderr, "CONOUT$", "w", stderr);
			cur_cmd.append(" -console");
		}
		if (wcscmp(szArgList[i], L"-help") == 0) {
			AllocConsole();
			FILE* fpstdin = stdin, * fpstdout = stdout, * fpstderr = stderr;
			freopen_s(&fpstdout, "CONOUT$", "w", stdout);
			freopen_s(&fpstderr, "CONOUT$", "w", stderr);

			std::wcout << "                                                  " << std::endl;
			std::wcout << "    bTTTTTTTTTTTTTTTTTTTTTTTTTTTbb     bTTTTTb    " << std::endl;
			std::wcout << "    bTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTb   bTTTTTb    " << std::endl;
			std::wcout << "    bbTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTb  bTTTTTb    " << std::endl;
			std::wcout << "             bbbbbbbb         bTTTTTb  bTTTTTb    " << std::endl;
			std::wcout << "              bbbbbbb     bbbbTTTTTTb  bTTTTTb    " << std::endl;
			std::wcout << "               bTTTTTT   bTTTTTTTTTb   bTTTTTb    " << std::endl;
			std::wcout << "           bbbb bTTTTTT  bTTTTTTTTTb   bTTTTTb    " << std::endl;
			std::wcout << "         bbTTTTb bTTTTTT  bbbbbTTTTTb  bTTTTTb    " << std::endl;
			std::wcout << "        bTTTTTTb  bTTTTTT      bTTTTbb bTTTTTb    " << std::endl;
			std::wcout << "      bTTTTTTTb    bTTTTTTbbbbbTTTTTT  bTTTTTb    " << std::endl;
			std::wcout << "    bTTTTTTTb       bTTTTTTTTTTTTTTTb  bTTTTTb    " << std::endl;
			std::wcout << "     TTTTTb          bTTTTTTTTTTTTb    bTTTTTb    " << std::endl;
			std::wcout << "      bbb             bbbbbbbbbbb      bbbbbb     " << std::endl;
			std::wcout << "                                                  " << std::endl;

			std::wcout << "An application written in C++ to modify the Windows 11 Taskbar. By Chris Andriessen." << std::endl;
			std::wcout << "https://github.com/ChrisAnd1998/TaskbarXI" << std::endl;
			std::wcout << "" << std::endl;
			std::wcout << "PARAMETERS:" << std::endl;
			std::wcout << "-help				(Shows this window)" << std::endl;
			std::wcout << "-stop				(Stops TaskbarXI and reverts the taskbar to default)" << std::endl;
			std::wcout << "-restart				(Does not refresh the taskbar region when starting)" << std::endl;
			std::wcout << "-square				(Uses square corners instead of rounded corners)" << std::endl;
			std::wcout << "-radius <radius>		(Define the corner radius you want to be used)" << std::endl;
			std::wcout << "-ignoremax			(Does not revert the taskbar on maximized window)" << std::endl;
			std::wcout << "-notray				(Disables system tray icon)" << std::endl;
			std::wcout << "-hidetraywnd			(Hides the system tray area)" << std::endl;
			std::wcout << "-createstartup			(Creates a startup entry including the current parameters)" << std::endl;
			std::wcout << "-removestartup			(Removes startup entry and exits TaskbarXI)" << std::endl;
			std::wcout << "-console			(Displays a console window)" << std::endl;
			std::wcout << "-sticky				(Sticks the system tray to the taskbar (removes the tray icons to keep it stable))" << std::endl;
			std::wcout << "-smoothresize				(Resizes the taskbar smoothly)" << std::endl;
			std::wcout << "-expandspeed <speed>		(Define the speed you want to be used for the expand animation (default: 90))" << std::endl;
			std::wcout << "-shrinkspeed <speed>		(Define the speed you want to be used for the shrink animation (default: 700))" << std::endl;
			std::wcout << "-blur				(Makes the taskbar blurred)" << std::endl;
			std::wcout << "" << std::endl;
			std::wcout << "EXAMPLE: TaskbarXI.exe -ignoremax -expandspeed 100 -square ";

			FreeConsole();

		}
	}

	LocalFree(szArgList);
	

	

	if (corner_Radius == 0) {
		corner_Radius = 15;
	}

	if (expandspeed == 0) {
		expandspeed = 90;
	}

	if (shrinkspeed == 0) {
		shrinkspeed = 700;
	}

	EnumWindows(EnumCallbackInstances, NULL);

	working = -1;

	SetWinEventHook(EVENT_SYSTEM_MOVESIZESTART, EVENT_SYSTEM_MOVESIZEEND, NULL, WinEventProcCallback, 0, 0, WINEVENT_SKIPOWNPROCESS);
	SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_DESTROY, NULL, WinEventProcCallback, 0, 0, WINEVENT_SKIPOWNPROCESS);
	SetWinEventHook(EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND, NULL, WinEventProcCallback, 0, 0, WINEVENT_SKIPOWNPROCESS);
	SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, WinEventProcCallback, 0, 0, WINEVENT_SKIPOWNPROCESS);

	SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

	std::wcout << "Initializing..." << std::endl;

	HWND Explorer = NULL;

	do
	{
		std::wcout << "Looking for Explorer..." << std::endl;
		Explorer = FindWindow(L"Shell_TrayWnd", 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	} while (Explorer == 0);

	std::wcout << "Explorer found!" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	MSG msg;

	WNDCLASSEX wnd = { 0 };

	wnd.hInstance = hInstance;
	wnd.lpszClassName = L"TaskbarXI";
	wnd.lpfnWndProc = WndProc;
	wnd.style = CS_HREDRAW | CS_VREDRAW;
	wnd.cbSize = sizeof(WNDCLASSEX);

	wnd.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
	wnd.hbrBackground = (HBRUSH)BLACK_BRUSH;
	RegisterClassEx(&wnd);

	HWND tray_hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, L"TaskbarXI", L"TrayWindow", WS_OVERLAPPEDWINDOW, 0, 0, 400, 400, NULL, NULL, hInstance, NULL);

	nid.cbSize = sizeof(nid);
	nid.hWnd = tray_hwnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	nid.uCallbackMessage = APPWM_ICONNOTIFY;
	nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	wcscpy_s(nid.szTip, L"TaskbarXI");

	Shell_NotifyIcon(NIM_DELETE, &nid);

	if (notray == 0) {
		if (sticky == 0) {
			Shell_NotifyIcon(NIM_ADD, &nid);
		}
	}

	Explorer = NULL;

	std::wcout << "Looking for taskbars..." << std::endl;

	taskbar_Count = 0;
	ZeroMemory(&taskbar_List, sizeof(taskbar_List));

	EnumWindows(EnumCallbackTaskbars, NULL);

	for (HWND tb : taskbar_List) {
		if (restart == 0) {
			HRGN region_Empty = CreateRectRgn(0, 0, 0, 0);
			SetWindowRgn(tb, region_Empty, FALSE);
			SendMessage(tb, WM_THEMECHANGED, TRUE, NULL);
		}
	}

	animating_Count = 0;
	ZeroMemory(&animating_List, sizeof(animating_List));

	if (stop == 1) {
		exiting();
	}

	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");

	cur_dir = std::string(buffer);

	if (cur_dir.find("40210ChrisAndriessen") != std::string::npos) {
		isstore = 1;
	}

	if (createstartup == 1) {
		create_startup();
	}

	if (removestartup == 1) {
		remove_startup();
		exiting();
	}

	if (blur == 1) {
		std::thread{ SetWindowBlur }.detach();
	}

	std::wcout << "Initialize complete!" << std::endl;
	std::wcout << "Application is running!" << std::endl;

	std::atexit(exiting);

	std::thread{ taskbarLoop }.detach();

	for (;;) {
		PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void SetTaskbar() {
	try {
		std::wcout.clear();

		working = 1;

		HWND curreg_Check_handle = FindWindow(0, L"Shell_TrayWnd");
		HRGN curreg_Check_region = CreateRectRgn(0, 0, 0, 0);
		GetWindowRgn(curreg_Check_handle, curreg_Check_region);

		RECT rect_Shell_TrayWnd;
		GetWindowRect(curreg_Check_handle, &rect_Shell_TrayWnd);

		int currentWidth = rect_Shell_TrayWnd.right - rect_Shell_TrayWnd.left;

		if (fixedTaskbarWidth == -1) {
			fixedTaskbarWidth = currentWidth;
		}

		int left = rect_Shell_TrayWnd.left;
		int top = rect_Shell_TrayWnd.top;
		int right = left + fixedTaskbarWidth;
		int bottom = rect_Shell_TrayWnd.bottom;

		HRGN fixedRegion = CreateRectRgn(left, top, right, bottom);
		SetWindowRgn(curreg_Check_handle, fixedRegion, TRUE);

		DeleteObject(curreg_Check_region);
		working = 0;
	} catch (...) {}
}

BOOL CALLBACK EnumCallbackTaskbars(HWND hWND, LPARAM lParam) {
	HWND isataskbar = FindWindowEx(hWND, 0, L"Start", NULL);
	HWND isprimarytaskbar = FindWindowEx(hWND, 0, L"RebarWindow32", NULL);

	if (isataskbar != 0 && isprimarytaskbar != 0) {
		std::wcout << "Main taskbar found! @ hWid : " << hWND << std::endl;
		taskbar_List[taskbar_Count] = hWND;
		taskbar_Count += 1;
	}

	if (isataskbar != 0 && isprimarytaskbar == 0) {
		std::wcout << "A Secondary taskbar found! @ hWid : " << hWND << std::endl;
		taskbar_List[taskbar_Count] = hWND;
		taskbar_Count += 1;
	}

	hWND = NULL;

	return true;
}

BOOL CALLBACK EnumCallbackMaximized(HWND hWND, LPARAM lParam) {
	WINDOWPLACEMENT wp;
	GetWindowPlacement(hWND, &wp);
	INT wl = GetWindowLong(hWND, GWL_STYLE);
	INT Cloaked;
	INT cl = DwmGetWindowAttribute(hWND, DWMWINDOWATTRIBUTE::DWMWA_CLOAKED, &Cloaked, sizeof(Cloaked));

	if (ignoremax == 0) {
		if (wp.showCmd == 3) {
			if (Cloaked == 0) {
				if (wl and WS_VISIBLE == WS_VISIBLE) {
					if ((wl | WS_MAXIMIZE) == wl) {
						if ((wl | WS_VISIBLE) == wl) {
							maximized_List[maximized_Count] = hWND;
							maximized_Count += 1;
						}
					}
				}
			}
		}
	}

	oldMaxCount = maximized_Count;

	wp.length = NULL;
	wl = NULL;
	Cloaked = NULL;
	cl = NULL;

	return true;
}

BOOL CALLBACK EnumCallbackInstances(HWND hWND, LPARAM lParam) {
	int length = 256;
	wchar_t* title = new wchar_t[length];

	GetClassName(hWND, title, length);

	if (wcscmp(title, L"TaskbarXI") == 0) {
		DWORD PID;
		GetWindowThreadProcessId(hWND, &PID);

		DWORD MYPID;
		MYPID = GetCurrentProcessId();

		if (MYPID != PID) {
			std::wcout << "Another TaskbarXI instance has been detected! Terminating other instance..." << std::endl;
			HANDLE HTARGET = OpenProcess(PROCESS_ALL_ACCESS, false, PID);

			NOTIFYICONDATA inid = {};
			inid.cbSize = sizeof(inid);
			inid.hWnd = hWND;
			inid.uID = 1;

			Shell_NotifyIcon(NIM_DELETE, &inid);

			TerminateProcess(HTARGET, 0);
		}
	}

	hWND = NULL;
	title = NULL;
	length = NULL;

	return true;
}
