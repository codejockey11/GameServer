#include "framework.h"

#include "resource.h"

#include "../GameCommon/CCommandLine.h"
#include "../GameCommon/CErrorLog.h"
#include "../GameCommon/CLocal.h"
#include "../GameCommon/CString.h"
#include "../GameCommon/CWindow.h"

#include "CServer.h"

enum Controls
{
	BTN_START_SERVER = 200,
	BTN_STOP_SERVER,
	BTN_ADD_BOT,
	BTN_STOP_BOT,
	CBX_MAP_LIST,
	CBX_BOT_LIST,
	TEXT_EDIT,
	TEXT_MESSAGE
};

char m_mapListName[32] = {};

CCommandLine* m_commandLine;
CErrorLog* m_errorLog;
CLocal* m_local;
CServer* m_server;
CWindow* m_window;

errno_t m_err = {};

FILE* m_fMapList = {};

HWND m_hBotList;
HWND m_hMapList;
HWND m_hMessage;
HWND m_hPort;

int32_t m_botCount = 0;

WCHAR m_botName[32] = {};
WCHAR m_mapName[32] = {};

LRESULT CALLBACK    WndProc(HWND, uint32_t, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, uint32_t, WPARAM, LPARAM);

/*
*/
int32_t APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int32_t nCmdShow)
{
	m_commandLine = new CCommandLine();

	m_commandLine->ServerConstructor(lpCmdLine);

	m_local = new CLocal();

	CString* logFile = new CString(m_local->m_installPath->m_text);

	logFile->Append("GameServerLog.txt");

	m_errorLog = new CErrorLog(logFile->m_text);

	SAFE_DELETE(logFile);

	m_window = new CWindow(hInstance, WndProc, "GameServerClass", IDC_GAMESERVER, IDI_GAMESERVER, IDI_SMALL, "Game Server", 331, 155, 0, 0);

	m_window->AddButton(L"Start", 3, 3, 75, 23, (HMENU)Controls::BTN_START_SERVER);
	m_window->AddButton(L"Stop", 81, 3, 75, 23, (HMENU)Controls::BTN_STOP_SERVER);
	m_window->AddButton(L"AddBot", 159, 3, 75, 23, (HMENU)Controls::BTN_ADD_BOT);
	m_window->AddButton(L"StopBot", 237, 3, 75, 23, (HMENU)Controls::BTN_STOP_BOT);

	m_hPort = m_window->AddTextEdit(L"49152", 3, 29, 57, 23, (HMENU)Controls::TEXT_EDIT);
	m_hMessage = m_window->AddTextEdit(L"Stopped", 63, 29, 249, 23, (HMENU)Controls::TEXT_MESSAGE);

	m_hMapList = m_window->AddComboBox(3, 55, 154, 23 * 5, (HMENU)Controls::CBX_MAP_LIST);
	m_hBotList = m_window->AddComboBox(159, 55, 154, 23 * 5, (HMENU)Controls::CBX_BOT_LIST);

	CString* mapList = new CString(m_local->m_installPath->m_text);

	mapList->Append("mapList.txt");

	m_err = fopen_s(&m_fMapList, mapList->m_text, "rb");

	if (m_err == 0)
	{
		memset(m_mapListName, 0x00, 32);

		fscanf_s(m_fMapList, "%s", &m_mapListName, 32);

		while (!feof(m_fMapList))
		{
			SendMessageA(m_hMapList, (uint32_t)CB_ADDSTRING, (WPARAM)0, (LPARAM)(m_mapListName));

			memset(m_mapListName, 0x00, 32);

			fscanf_s(m_fMapList, "%s", &m_mapListName, 32);
		}

		fclose(m_fMapList);
	}

	SAFE_DELETE(mapList);

	SendMessageA(m_hMapList, (uint32_t)CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

	m_server = new CServer(m_local, m_errorLog);

	if (strlen(m_commandLine->m_port) > 0)
	{
		m_window->SetTextForControl(m_hPort, m_commandLine->m_port);
	}

	GetWindowTextA(m_hPort, m_commandLine->m_port, 6);

	m_server->Start(m_commandLine->m_port);

	CString* messageString = new CString("Running on port:");

	messageString->Append(m_server->m_listenSocket->m_port);

	m_window->SetTextForControl(m_hMessage, messageString->m_text);

	SAFE_DELETE(messageString);

	
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

	SAFE_DELETE(m_server);
	SAFE_DELETE(m_local);
	SAFE_DELETE(m_commandLine);

	SAFE_DELETE(m_window);

	return (int32_t)msg.wParam;
}

/*
*/
LRESULT CALLBACK WndProc(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int32_t wmId = LOWORD(wParam);

		switch (wmId)
		{
		case IDM_ABOUT:
		{
			DialogBox(m_window->m_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);

			return 0;
		}
		case IDM_EXIT:
		{
			DestroyWindow(hWnd);

			return 0;
		}
		case IDM_CLEARBOTS:
		{
			ComboBox_ResetContent(m_hBotList);

			return 0;
		}
		case Controls::CBX_MAP_LIST:
		{
			if (HIWORD(wParam) == CBN_CLOSEUP)
			{
				int32_t i = ComboBox_GetCurSel(m_hMapList);
				
				ComboBox_GetLBText(m_hMapList, i, m_mapName);

				CString* mapName = new CString(m_mapName);

				CListNode* node = m_server->m_mapList->Search(mapName->m_text);

				m_server->m_currentMap = (CString*)node->m_object;

				m_server->MapChange();

				SAFE_DELETE(mapName);
			}

			return 0;
		}
		case Controls::BTN_START_SERVER:
		{
			if (m_server->m_listenThreadRunning)
			{
				return 0;
			}

			GetWindowTextA(m_hPort, m_commandLine->m_port, 6);

			m_server->Start(m_commandLine->m_port);

			m_server->Reset();

			m_server->LoadEnvironment();

			CString* formMessage = new CString("Running on port:");

			formMessage->Append(m_server->m_listenSocket->m_port);

			m_window->SetTextForControl(m_hMessage, formMessage->m_text);

			SAFE_DELETE(formMessage);

			return 0;
		}
		case Controls::BTN_STOP_SERVER:
		{
			if (!m_server->m_listenThreadRunning)
			{
				return 0;
			}

			m_server->Stop();

			m_window->SetTextForControl(m_hMessage, "Stopped");

			ComboBox_ResetContent(m_hBotList);

			m_botCount = 0;

			return 0;
		}
		case Controls::BTN_ADD_BOT:
		{
			char buffer[32] = {};

			sprintf_s(buffer, 32, "Bot%02i", m_botCount);

			CString* botName = new CString(buffer);

			if (m_server->StartBot(botName))
			{
				ComboBox_AddString(m_hBotList, botName->GetWide());

				m_botCount++;

				SendMessageA(m_hBotList, (uint32_t)CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
			}

			SAFE_DELETE(botName);

			return 0;
		}
		case Controls::BTN_STOP_BOT:
		{
			int32_t i = ComboBox_GetCurSel(m_hBotList);

			if (i == -1)
			{
				return 0;
			}

			ComboBox_GetLBText(m_hBotList, i, m_botName);

			CString* botName = new CString(m_botName);

			m_server->StopBot(botName);

			SAFE_DELETE(botName);

			ComboBox_DeleteString(m_hBotList, i);

			SendMessageA(m_hBotList, (uint32_t)CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

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
INT_PTR CALLBACK About(HWND hDlg, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	lParam;

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