#include "framework.h"

#include "resource.h"

#include "../GameCommon/CString.h"

#include "CServer.h"

constexpr auto MAX_LOADSTRING = 100;

HINSTANCE m_hInst;
HWND m_hWnd;

WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

enum Controls
{
	BTN_STARTSERVER = 200,
	BTN_STOPSERVER,
	BTN_ADDBOT,
	CBX_MAP_LIST,
	TEXT_EDIT,
	TEXT_MESSAGE
};

CServer* m_server;

HWND m_startServer;
HWND m_stopServer;
HWND m_port;
HWND m_message;
HWND m_addBot;
HWND m_mapList;

CString* c_port;

STARTUPINFO m_addBotStartupInfo;
PROCESS_INFORMATION m_addBotProcessInfo;

int m_botCount;

errno_t m_err = {};
FILE* m_fMapList = {};
char m_mapListName[32] = {};
wchar_t m_mapName[32] = {};

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

/*
*/
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	c_port = new CString(lpCmdLine);

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_GAMESERVER, szWindowClass, MAX_LOADSTRING);

	MyRegisterClass(hInstance);

	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAMESERVER));

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
			if (m_server->m_listenThreadRunning)
			{
				m_server->Frame();
			}
		}
	}

	delete m_server;
	delete c_port;

	return (int)msg.wParam;
}

/*
*/
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

/*
*/
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	m_hInst = hInstance;

	int x = (GetSystemMetrics(SM_CXSCREEN) / 2) - 265 / 2;
	int y = (GetSystemMetrics(SM_CYSCREEN) / 2) - 200 / 2;

	m_hWnd = CreateWindowW(szWindowClass, szTitle, WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
		x, y,
		315, 155, nullptr, nullptr, hInstance, nullptr);

	if (!m_hWnd)
	{
		return FALSE;
	}

	ShowWindow(m_hWnd, nCmdShow);
	UpdateWindow(m_hWnd);


	m_startServer = CreateWindow(WC_BUTTON, L"Start", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		3, 3, 75, 23,
		m_hWnd,
		(HMENU)Controls::BTN_STARTSERVER,
		m_hInst,
		NULL);

	m_stopServer = CreateWindow(WC_BUTTON, L"Stop", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		81, 3, 75, 23,
		m_hWnd,
		(HMENU)Controls::BTN_STOPSERVER,
		m_hInst,
		NULL);

	m_addBot = CreateWindow(WC_BUTTON, L"AddBot", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		159, 3, 75, 23,
		m_hWnd,
		(HMENU)Controls::BTN_ADDBOT,
		m_hInst,
		NULL);

	m_port = CreateWindow(WC_EDIT, L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
		237, 3, 57, 23,
		m_hWnd,
		(HMENU)Controls::TEXT_EDIT,
		m_hInst,
		NULL);

	m_message = CreateWindow(WC_EDIT, L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
		3, 29, 291, 23,
		m_hWnd,
		(HMENU)Controls::TEXT_MESSAGE,
		m_hInst,
		NULL);

	m_mapList = CreateWindow(WC_COMBOBOX, NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | BS_DEFSPLITBUTTON | CBS_DROPDOWN | CBS_HASSTRINGS | WS_VSCROLL,
		3, 55, 293, 23 * 5,
		m_hWnd,
		(HMENU)Controls::CBX_MAP_LIST,
		m_hInst,
		NULL);

	HFONT s_hFont = CreateFont(
		18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
		L"Segoe UI"
	);

	SendMessage(m_startServer, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
	SendMessage(m_stopServer, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
	SendMessage(m_message, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
	SendMessage(m_port, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
	SendMessage(m_addBot, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
	SendMessage(m_mapList, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));


	m_err = fopen_s(&m_fMapList, "C:/Users/junk_/source/repos/Game/mapList.txt", "rb");

	if (m_err == 0)
	{
		memset(m_mapListName, 0x00, 32);

		fscanf_s(m_fMapList, "%s", &m_mapListName, 32);

		while (!feof(m_fMapList))
		{
			SendMessageA(m_mapList, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)(m_mapListName));

			memset(m_mapListName, 0x00, 32);

			fscanf_s(m_fMapList, "%s", &m_mapListName, 32);
		}

		fclose(m_fMapList);
	}
	
	SendMessage(m_mapList, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

	m_server = new CServer();

	if (c_port->m_length > 0)
	{
		m_server->Start(c_port->m_text);
		
		SetWindowTextA(m_port, c_port->m_text);
	}
	else
	{
		m_server->Start("49152");
		
		SetWindowTextA(m_port, "49152");
	}

	CString* message = new CString("Running on port:");

	message->Append(m_server->m_listenSocket->m_port);

	SetWindowTextA(m_message, message->m_text);

	delete message;


	m_botCount = 0;

	return TRUE;
}

/*
*/
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
			DialogBox(m_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);

			return 0;
		}
		case IDM_EXIT:
		{
			DestroyWindow(hWnd);

			return 0;
		}
		case Controls::CBX_MAP_LIST:
		{
			if (HIWORD(wParam) == CBN_CLOSEUP)
			{
				int i = ComboBox_GetCurSel(m_mapList);
				
				ComboBox_GetLBText(m_mapList, i, m_mapName);

				CString* mapName = new CString(m_mapName);

				m_server->MapChange(mapName);

				delete mapName;
			}

			return 0;
		}
		case Controls::BTN_STARTSERVER:
		{
			if (m_server->m_listenThreadRunning)
			{
				m_server->Stop();
			}

			m_server->Start(c_port->m_text);

			m_server->Reset();

			m_server->m_currentMap = m_server->m_mapList->m_list;

			m_server->LoadEnvironment(m_server->m_currentMap->m_object);


			if (c_port)
			{
				delete c_port;
			}

			c_port = new CString(32);

			GetWindowTextA(m_port, c_port->m_text, 6);

			CString* formMessage = new CString("Running on port:");

			formMessage->Append(m_server->m_listenSocket->m_port);

			SetWindowTextA(m_message, formMessage->m_text);

			delete formMessage;


			return 0;
		}
		case Controls::BTN_STOPSERVER:
		{
			if (m_server->m_listenThreadRunning)
			{
				m_server->Stop();
			}

			SetWindowText(m_message, L"Idle");

			return 0;
		}
		case Controls::BTN_ADDBOT:
		{
			m_botCount++;

			return 0;
		}
		}
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		
		BeginPaint(hWnd, &ps);
		
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

/*
*/
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
		
			return (INT_PTR)TRUE;
		}
	
		break;
	}
	}

	return (INT_PTR)FALSE;
}