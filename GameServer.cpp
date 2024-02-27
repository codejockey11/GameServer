// GameServer.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "GameServer.h"
#include "windowsx.h"

constexpr auto MAX_LOADSTRING = 100;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

enum Controls
{
	BTN_STARTSERVER = 200,
	BTN_STOPSERVER,
	BTN_ADDBOT,
	TEXT_EDIT,
	TEXT_MESSAGE
};

CServer* m_server;

HWND m_startServer;
HWND m_stopServer;
HWND m_port;
HWND m_message;
HWND m_addBot;

CString* c_port;

static HFONT s_hFont = NULL;

STARTUPINFO m_addBotStartupInfo;
PROCESS_INFORMATION m_addBotProcessInfo;

int m_botCount;


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	// TODO: Place code here.
	c_port = new CString(lpCmdLine);


	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_GAMESERVER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}


	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAMESERVER));

	MSG msg = {};



	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);

			DispatchMessage(&msg);
		}
		else
		{
			if (m_server != nullptr)
			{
				m_server->m_frametime->Frame();

				m_server->ReceiveClients();
			}
		}
	}


	delete m_server;
	delete c_port;

	return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex = {};

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAMESERVER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_GAMESERVER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	int x = (GetSystemMetrics(SM_CXSCREEN) / 2) - 265 / 2;
	int y = (GetSystemMetrics(SM_CYSCREEN) / 2) - 150 / 2;

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
		x, y,
		265, 150, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	const TCHAR* fontName = _T("MS Shell Dlg");
	const long nFontSize = 10;

	HDC hdc = GetDC(hWnd);

	LOGFONT logFont = { 0 };
	logFont.lfHeight = -MulDiv(nFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	logFont.lfWeight = FW_NORMAL;
	_tcscpy_s(logFont.lfFaceName, fontName);

	s_hFont = CreateFontIndirect(&logFont);

	ReleaseDC(hWnd, hdc);


	RECT bounds;
	GetClientRect(hWnd, &bounds);


	m_startServer = CreateWindow(WC_BUTTON, L"Restart", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		/* position */ 6, 3,
		/* size     */ 64, 22,
		hWnd,
		(HMENU)Controls::BTN_STARTSERVER,
		hInst,
		NULL);

	m_stopServer = CreateWindow(WC_BUTTON, L"Stop", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		/* position */ 64 + 6 + 3, 3,
		/* size     */ 64, 22,
		hWnd,
		(HMENU)Controls::BTN_STOPSERVER,
		hInst,
		NULL);

	m_port = CreateWindow(WC_EDIT, L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
		64 + 6 + 3 + 64 + 3 + 6, 3,
		95, 22,   // set size in WM_SIZE message 
		hWnd,         // parent window 
		(HMENU)Controls::TEXT_EDIT,   // edit control ID 
		hInst,
		NULL);

	m_message = CreateWindow(WC_EDIT, L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
		6, 31, 235, 22,   // set size in WM_SIZE message 
		hWnd,         // parent window 
		(HMENU)Controls::TEXT_MESSAGE,   // edit control ID 
		hInst,
		NULL);

	m_addBot = CreateWindow(WC_BUTTON, L"AddBot", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		/* position */ bounds.right - 64 - 6, bounds.bottom - 22 - 6,
		/* size     */ 64, 22,
		hWnd,
		(HMENU)Controls::BTN_ADDBOT,
		hInst,
		NULL);

	SendMessage(m_startServer, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
	SendMessage(m_stopServer, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
	SendMessage(m_message, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
	SendMessage(m_port, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
	SendMessage(m_addBot, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));

	m_server = new CServer();

	if (c_port->GetLength() > 0)
	{
		m_server->StartServer(c_port->GetText());
		SetWindowText(m_port, c_port->GetWText());
	}
	else
	{
		m_server->StartServer("26105");
		SetWindowText(m_port, L"26105");
	}

	WCHAR port[32] = {};

	GetWindowText(m_port, port, 6);

	WCHAR buffer[32] = {};

	swprintf_s(buffer, 32, L"Running on port:%s", port);

	SetWindowText(m_message, buffer);


	m_botCount = 0;

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);

		switch (wmId)
		{
		case IDM_ABOUT:
		{
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);

			return 0;
		}
		case IDM_EXIT:
		{
			DestroyWindow(hWnd);

			return 0;
		}
		case Controls::BTN_STARTSERVER:
		{
			if (m_server->m_listenThreadRunning)
			{
				m_server->Shutdown();
			}

			if (c_port)
			{
				delete c_port;
			}

			WCHAR port[32] = {};

			GetWindowText(m_port, port, 6);

			c_port = new CString(port);

			m_server->StartServer(c_port->GetText());

			WCHAR buffer[32] = {};

			swprintf_s(buffer, 32, L"Running on port:%s", port);

			SetWindowText(m_message, buffer);

			return 0;
		}
		case Controls::BTN_STOPSERVER:
		{
			m_server->Shutdown();

			SetWindowText(m_message, L"Stopped");

			return 0;
		}
		case Controls::BTN_ADDBOT:
		{
			m_addBotStartupInfo = {};
			m_addBotStartupInfo.cb = sizeof(STARTUPINFO);

			m_addBotProcessInfo = {};

			WCHAR accountInfo[64] = {};

			wsprintf(accountInfo, L"GameBot.exe Bot%02i 127.0.0.1 26105", m_botCount);

#ifdef _DEBUG
			bool r = CreateProcess(L"C:/Users/junk_/source/repos/GameBot/x64/Debug/GameBot.exe", // No module name (use command line)
				accountInfo,           // Command line
				NULL,           // Process handle not inheritable
				NULL,           // Thread handle not inheritable
				FALSE,          // Set handle inheritance to FALSE
				0,              // No creation flags
				NULL,           // Use parent's environment block
				NULL,           // Use parent's starting directory 
				&m_addBotStartupInfo,            // Pointer to STARTUPINFO structure
				&m_addBotProcessInfo);           // Pointer to PROCESS_INFORMATION structure
#else
			bool r = CreateProcess(L"GameBot.exe", // No module name (use command line)
				accountInfo,           // Command line
				NULL,           // Process handle not inheritable
				NULL,           // Thread handle not inheritable
				FALSE,          // Set handle inheritance to FALSE
				0,              // No creation flags
				NULL,           // Use parent's environment block
				NULL,           // Use parent's starting directory 
				&m_addBotStartupInfo,            // Pointer to STARTUPINFO structure
				&m_addBotProcessInfo);           // Pointer to PROCESS_INFORMATION structure
#endif

			m_botCount++;

			break;

			return 0;
		}
		}
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code that uses hdc here...
		EndPaint(hWnd, &ps);

		return 0;
	}
	case WM_CLOSE:
	case WM_DESTROY:
	{
		PostQuitMessage(0);

		return 0;
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}