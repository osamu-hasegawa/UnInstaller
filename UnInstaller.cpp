// UnInstaller.cpp: アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "UnInstaller.h"
#include <cstdio>
#include <commctrl.h>
#include <shlobj.h>
#include <windows.h>

#define MAX_LOADSTRING 100
#define USER_BUFF_SIZE	1024

// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名

												// このコード モジュールに含まれる関数の宣言を転送します:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void				FileDirectoryRemove();

//Basler pylon 5のアプリケーション名(GetWindowTextより取得)
char appName[] =
{
	'p', '\0', 'y', '\0', 'l', '\0', 'o', '\0', 'n', '\0', 0x20, '\0',
	'C', '\0', 'a', '\0', 'm', '\0', 'e', '\0', 'r', '\0', 'a', '\0', 0x20, '\0',
	'S', '\0', 'o', '\0', 'f', '\0', 't', '\0', 'w', '\0', 'a', '\0', 'r', '\0', 'e', '\0', 0x20, '\0',
	'S', '\0', 'u', '\0', 'i', '\0', 't', '\0', 'e', '\0', 0x20, '\0',
	'5', '\0', '.', '\0', '0', '\0', '.', '\0', '0', '\0', '.', '\0', '6', '\0', '1', '\0', '5', '\0', '0', '\0', '\0', '\0',
	'4', '\0', 'e', '\0', '6', '\0', 'f', '\0', '-', '\0', 'a', '\0', '5', '\0', '7', '\0', 'f', '\0', '-', '\0',
	'7', '\0', '1', '\0', '8', '\0', 'd', '\0', '8', '\0', 'b', '\0', '0', '\0', '1', '\0', '4', '\0', 'e', '\0', '3', '\0', '3', '\0', ']'
};

#define	APP_TITLE	L"Camera Device UnInstaller"
#define	BLINK_STR	L"\r\n\r\n\r\nCamera Device Driverを削除しています。\r\n\r\nPCを操作しないで下さい。"
#define	FINISH_STR	L"\r\n\r\n\r\nCamera Device Driverを削除しました。\r\n\r\n終了して下さい。"

#define pb_step 10
#define pb_min 0
#define pb_max 100
#define WM_BASLER_MONITOR_TIME_OUT	WM_USER+1
#define WM_BASLER_CLOSE				WM_USER+2

HWND uScopehWnd;
HWND ParentWnd;
static HANDLE th_text;
static HANDLE th_progress;
static HANDLE th_uscope;
static HANDLE th_basler;
HWND staWnd;
HFONT hFont;
RECT rcClient;  // Client area of parent window.
int cyVScroll;  // Height of scroll bar arrow.
HWND hwndPB;    // Handle of progress bar.
int gCmdShow;
HANDLE hThreadEventText;
HANDLE hThreadEventProg;
HANDLE hThreadEventUscope;
HANDLE hThreadEventBasler;

UINT	lpThreadIdText;
UINT	lpThreadIdProg;
UINT	lpThreadIdUscope;
UINT	lpThreadIdBasler;
int nBufferLength;
char lpBuffer[USER_BUFF_SIZE];
int curPath;
char filePath[USER_BUFF_SIZE];
DWORD dwWidth;
DWORD dwHeight;


typedef enum TimerID {
	WAIT_TIME = 100,
} timer;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: ここにコードを挿入してください。
	HANDLE hFind;
	WIN32_FIND_DATA win32fd;
	hFind = ::FindFirstFile((LPWSTR)L"C:\\Program Files\\Basler\\*", &win32fd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, L"すでにアンインストール済です。\r\nインストールして下さい", L"Camera Device UnInstaller",
		MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
		::FindClose(hFind);
		return FALSE;
	}
	::FindClose(hFind);

	// グローバル文字列を初期化しています。
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_UNINSTALLER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	gCmdShow = nCmdShow;
	// アプリケーションの初期化を実行します:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_UNINSTALLER));

	MSG msg;

	// メイン メッセージ ループ:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}


//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDC_UNINSTALLER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_UNINSTALLER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}


DWORD WINAPI BaslerThread(void* vpArguments)
{
	//	スレッドの起動引数に、初期化完了イベントをのせる
	HANDLE hThreadInitEvent = (HANDLE)vpArguments;
	// メッセージ
	MSG oMsg;
	// メッセージキューの作成
	::PeekMessage(&oMsg, NULL, 0, 0, PM_NOREMOVE);
	// スレッド初期化完了(メッセージキュー生成完了)
	::SetEvent(hThreadInitEvent);


	//Basler Install exe 起動
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si = { sizeof(STARTUPINFO) };

	nBufferLength = USER_BUFF_SIZE;
	memset(lpBuffer, 0, sizeof(lpBuffer));
	curPath = GetCurrentDirectory((DWORD)nBufferLength, (LPTSTR)lpBuffer);
	memset(filePath, 0, sizeof(filePath));
	memcpy(filePath, lpBuffer, curPath * 2);
	memcpy((filePath + curPath * 2), L"\\uScopeCameraDriver\\Basler pylon 5.0.0.6150.exe", strlen("\\uScopeCameraDriver\\Basler pylon 5.0.0.6150.exe") * 2);

	CreateProcess((LPCWSTR)filePath
		, NULL, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);

	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);

	//メインの処理に通知
	::PostMessage((HWND)ParentWnd, WM_BASLER_CLOSE, 0, 0);

	//WM_QUITの受信を待つ
	while (1)
	{
		if (::PeekMessage(&oMsg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (0 == ::GetMessage(&oMsg, NULL, 0, 0))
			{
				break;//WM_QUITのみ
			}
		}
	}

	ExitThread(0);

	//スレッドの終了を待つ
	WaitForSingleObject(th_basler, INFINITE);
	//スレッドのハンドルを閉じる
	CloseHandle(th_basler);

	return FALSE;
}


DWORD WINAPI TextBlinkThread(void* vpArguments)
{
	//	スレッドの起動引数に、初期化完了イベントをのせる
	HANDLE hThreadInitEvent = (HANDLE)vpArguments;
	// メッセージ
	MSG oMsg;
	// メッセージキューの作成
	::PeekMessage(&oMsg, NULL, 0, 0, PM_NOREMOVE);
	// スレッド初期化完了(メッセージキュー生成完了)
	::SetEvent(hThreadInitEvent);


	//文字点滅開始
	int blinkCount = 0;
	while (1)
	{
		if (::PeekMessage(&oMsg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (0 == ::GetMessage(&oMsg, NULL, 0, 0))
			{//WM_QUITのみ
				SetWindowText(staWnd, FINISH_STR);
				ShowWindow(staWnd, gCmdShow);
				UpdateWindow(staWnd);
				break;
			}
		}

		Sleep(500);
		if (blinkCount % 2)
		{
			SetWindowText(staWnd, L"");
		}
		else
		{
			SetWindowText(staWnd, BLINK_STR);
		}
		UpdateWindow(staWnd);
		ShowWindow(staWnd, gCmdShow);
		blinkCount++;
	}
	ExitThread(0);

	//スレッドの終了を待つ
	WaitForSingleObject(th_text, INFINITE);
	//スレッドのハンドルを閉じる
	CloseHandle(th_text);

	return FALSE;
}


DWORD WINAPI ProgressThread(void* vpArguments)
{
	//	スレッドの起動引数に、初期化完了イベントをのせる
	HANDLE hThreadInitEvent = (HANDLE)vpArguments;
	// メッセージ
	MSG oMsg;
	// メッセージキューの作成
	::PeekMessage(&oMsg, NULL, 0, 0, PM_NOREMOVE);
	// スレッド初期化完了(メッセージキュー生成完了)
	::SetEvent(hThreadInitEvent);

	UINT iProg = 0;
	UINT uPos = 0;
	while (1)
	{
		if (::PeekMessage(&oMsg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (0 == ::GetMessage(&oMsg, NULL, 0, 0))
			{
				uPos = SendMessage(hwndPB, PBM_SETPOS, pb_max, 0);
				UpdateWindow(hwndPB);
				ShowWindow(hwndPB, gCmdShow);
				break;//WM_QUITのみ
			}
		}

		Sleep(12000);
		if (iProg < pb_max)
		{
			UINT uPos = SendMessage(hwndPB, PBM_GETPOS, 0, 0);
			SendMessage(hwndPB, PBM_STEPIT, 0, 0);
			UpdateWindow(hwndPB);
			ShowWindow(hwndPB, gCmdShow);
			iProg += pb_step;
		}
		else
		{
			break;
		}
	}
	ExitThread(0);

	//スレッドの終了を待つ
	WaitForSingleObject(th_progress, INFINITE);
	//スレッドのハンドルを閉じる
	CloseHandle(th_progress);

	return FALSE;
}

DWORD WINAPI uscopeThread(void* vpArguments)
{
#if 1
	//画面が表示されるまで待つ
	while (1)
	{
		uScopehWnd = FindWindow(0, (LPCWSTR)appName);
		if (uScopehWnd == NULL)
		{
			Sleep(1000);
			continue;
		}
		else
		{
			Sleep(5000);
			break;
		}
	}
#else
	uScopehWnd = FindWindow(0, (LPCWSTR)appName);
#endif

	//これよりBasler pylon5のインストール画面を制御する
	//Welcome Back画面で起動
	//Modify the current installationにフォーカスを当てる
	PostMessage((HWND)uScopehWnd, WM_KEYDOWN, VK_TAB, 0);
	PostMessage((HWND)uScopehWnd, WM_KEYUP, VK_TAB, 0);
	Sleep(300);

	//Repair current installationへ移行
	PostMessage((HWND)uScopehWnd, WM_KEYDOWN, VK_TAB, 0);
	PostMessage((HWND)uScopehWnd, WM_KEYUP, VK_TAB, 0);
	Sleep(300);

	//Uninstall the pylon version completelyへ移行
	PostMessage((HWND)uScopehWnd, WM_KEYDOWN, VK_TAB, 0);
	PostMessage((HWND)uScopehWnd, WM_KEYUP, VK_TAB, 0);
	Sleep(300);

	//Uninstall the pylon version completelyを押下する
	PostMessage((HWND)uScopehWnd, WM_KEYDOWN, VK_SPACE, 0);
	PostMessage((HWND)uScopehWnd, WM_KEYUP, VK_SPACE, 0);
	Sleep(300);

	//NEXTへ移行
	PostMessage((HWND)uScopehWnd, WM_KEYDOWN, VK_TAB, 0);
	PostMessage((HWND)uScopehWnd, WM_KEYUP, VK_TAB, 0);
	Sleep(300);

	//NEXTを押下する
	PostMessage((HWND)uScopehWnd, WM_KEYDOWN, VK_SPACE, 0);
	PostMessage((HWND)uScopehWnd, WM_KEYUP, VK_SPACE, 0);
	Sleep(500);

	//-----次のWarning画面へ移行-----
	//Yesへ移行
	PostMessage((HWND)uScopehWnd, WM_KEYDOWN, VK_TAB, 0);
	PostMessage((HWND)uScopehWnd, WM_KEYUP, VK_TAB, 0);
	Sleep(300);

	//Yesを押下する
	PostMessage((HWND)uScopehWnd, WM_KEYDOWN, VK_SPACE, 0);
	PostMessage((HWND)uScopehWnd, WM_KEYUP, VK_SPACE, 0);

	//Baslerのアンインストールが終了するまで待つ。
	Sleep(1000 * 60 * 2);

	//メインの処理に通知
	PostMessage((HWND)ParentWnd, WM_BASLER_MONITOR_TIME_OUT, 0, 0);


	//	スレッドの起動引数に、初期化完了イベントをのせています
	HANDLE hThreadInitEvent = (HANDLE)vpArguments;
	// メッセージ
	MSG oMsg;
	// メッセージキューの作成
	::PeekMessage(&oMsg, NULL, 0, 0, PM_NOREMOVE);
	// スレッド初期化完了(メッセージキュー生成完了)
	::SetEvent(hThreadInitEvent);

	//WM_QUITの受信を待つ
	while (1)
	{
		if (::PeekMessage(&oMsg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (0 == ::GetMessage(&oMsg, NULL, 0, 0))
			{
				break;//WM_QUITのみ
			}
		}
	}
	ExitThread(0);

	//スレッドの終了を待つ
	WaitForSingleObject(th_uscope, INFINITE);
	//スレッドのハンドルを閉じる
	CloseHandle(th_uscope);

	return FALSE;
}


void FileDirectoryRemove()
{
	//フォルダ内の全ファイルを削除する
	//My Documentフォルダのパスを取得する
	int    path_size = 0;
	char lpszPath[MAX_PATH] = "";
	LPITEMIDLIST pidl;
	::SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl);
	::SHGetPathFromIDList(pidl, (LPWSTR)lpszPath);
	::CoTaskMemFree(pidl);

	char srcPathStr[USER_BUFF_SIZE];
	memset(srcPathStr, 0, sizeof(srcPathStr));

	//戻り値はNULL終端まで含めた文字数
	path_size = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)lpszPath, -1, NULL, 0, NULL, NULL);
	memcpy(srcPathStr, lpszPath, (path_size - 1) * 2);
	memcpy((srcPathStr + (path_size - 1) * 2), L"\\uScopeCameraDriver\\*", strlen("\\uScopeCameraDriver\\*") * 2);

	HANDLE hFind;
	WIN32_FIND_DATA win32fd;
	hFind = ::FindFirstFile((LPWSTR)srcPathStr, &win32fd);

	if (hFind == INVALID_HANDLE_VALUE) {
		::FindClose(hFind);
		return;
	}

	char delFilePath[USER_BUFF_SIZE];
	memset(delFilePath, 0, sizeof(delFilePath));
	memcpy(delFilePath, lpszPath, (path_size - 1) * 2);
	memcpy((delFilePath + (path_size - 1) * 2), L"\\uScopeCameraDriver\\", strlen("\\uScopeCameraDriver\\") * 2);
	int delFilePathLen = ((path_size - 1) * 2) + (strlen("\\uScopeCameraDriver\\") * 2);

//ファイルをひとつずつ削除する
	char workdelFilePath[USER_BUFF_SIZE];
	char   *cstring = 0;
	int out_size = 0;
	int resultrem = 0;

	do
	{
		if (FILE_ATTRIBUTE_DIRECTORY == win32fd.dwFileAttributes)
		{
			::FindNextFile(hFind, &win32fd);
			continue;
		}

		memset(workdelFilePath, 0, sizeof(workdelFilePath));
		memcpy(workdelFilePath, delFilePath, delFilePathLen);

		// UTF-16文字列からShift-JISに変換したときの文字列長を求める。
		out_size = WideCharToMultiByte(CP_ACP, 0, win32fd.cFileName, -1, NULL, 0, NULL, NULL);
		
		memcpy((workdelFilePath + delFilePathLen), win32fd.cFileName, (out_size - 1) * 2);

		//読み取り属性なら解除する→削除できないため
		DWORD fileAtt = GetFileAttributes((LPCWSTR)workdelFilePath);
		if (fileAtt & FILE_ATTRIBUTE_READONLY)
		{
			SetFileAttributes((LPCWSTR)workdelFilePath, FILE_ATTRIBUTE_ARCHIVE);
		}

		out_size = WideCharToMultiByte(CP_ACP, 0, (LPCWCH)workdelFilePath, -1, NULL, 0, NULL, NULL);

		// Shift-JIS文字列の領域を確保する。
		cstring = new char[(out_size + 1) * sizeof(char)];
		memset(cstring, 0, (out_size + 1));
		// UTF-16文字列からShift-JISに変換する。
		WideCharToMultiByte(CP_ACP, 0, (LPCWCH)workdelFilePath, -1, cstring, out_size, NULL, NULL);

		resultrem = remove(cstring);
		delete [] cstring;

	} while (::FindNextFile(hFind, &win32fd));

	::FindClose(hFind);

//フォルダを削除する
	char remdir[USER_BUFF_SIZE];
	memset(remdir, 0, sizeof(remdir));
	memcpy(remdir, lpszPath, (path_size - 1) * 2);
	memcpy((remdir + (path_size - 1) * 2), L"\\uScopeCameraDriver", strlen("\\uScopeCameraDriver") * 2);
	BOOL removeResult = ::RemoveDirectory((LPCWSTR)remdir);

}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します。
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // グローバル変数にインスタンス処理を格納します。

	dwWidth = GetSystemMetrics(SM_CXSCREEN); // 画面の横幅を取得 
	dwHeight = GetSystemMetrics(SM_CYSCREEN); // 画面の高さを取得 

	HWND hWnd = CreateWindowEx(WS_EX_TOPMOST, szWindowClass, APP_TITLE, WS_OVERLAPPED,
		CW_USEDEFAULT, CW_USEDEFAULT, dwWidth, dwHeight, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}
	
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}


//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND  - アプリケーション メニューの処理
//  WM_PAINT    - メイン ウィンドウの描画
//  WM_DESTROY  - 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
	case WM_CREATE:
		ParentWnd = hWnd;

		//スタティックコントロール作成
		staWnd = CreateWindow(
			L"static", BLINK_STR, SS_CENTER | WS_CHILD | WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT, dwWidth, (dwHeight-(dwHeight/2)), ParentWnd, NULL, hInst, NULL);

		//フォント作成
		hFont = CreateFont(48, 0, 0, 0,
			FW_NORMAL, TRUE, FALSE, 0,
			SHIFTJIS_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH, L"メイリオ");

		//フォント変更のメッセージを送信する
		SendMessage(staWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));


		//プログレスバーの作成
		GetClientRect(ParentWnd, &rcClient);
		cyVScroll = GetSystemMetrics(SM_CYVSCROLL);
		hwndPB = CreateWindowEx(0, PROGRESS_CLASS, (LPTSTR)NULL,
			WS_CHILD | WS_VISIBLE, (dwWidth/4), (dwHeight-(dwHeight/3)),
			(dwWidth/2), cyVScroll,
			ParentWnd, (HMENU)0, hInst, NULL);


		//プログレスバー初期設定
		SendMessage(hwndPB, PBM_SETRANGE, (WPARAM)0, MAKELPARAM(pb_min, pb_max));
		SendMessage(hwndPB, PBM_SETSTEP, (WPARAM)pb_step, 0);

		//		文字点滅スレッドの生成
		hThreadEventText = ::CreateEvent(
			NULL      // SECURITY_ATTRIBUTES構造体
			, TRUE      // リセットのタイプ( TRUE: 自動 / FALSE: 手動 )
			, FALSE     // 初期状態( TRUE: シグナル状態 / FALSE: 非シグナル状態 )
			, NULL      // イベントオブジェクトの名前
		);
		th_text = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)TextBlinkThread, (void*)hThreadEventText, 0, (LPDWORD)&lpThreadIdText);

		//		プログレスバーの生成
		hThreadEventProg = ::CreateEvent(
			NULL      // SECURITY_ATTRIBUTES構造体
			, TRUE      // リセットのタイプ( TRUE: 自動 / FALSE: 手動 )
			, FALSE     // 初期状態( TRUE: シグナル状態 / FALSE: 非シグナル状態 )
			, NULL      // イベントオブジェクトの名前
		);
		th_progress = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ProgressThread, (void*)hThreadEventProg, 0, (LPDWORD)&lpThreadIdProg);

		//		Baslerアプリ起動スレッドの生成
		hThreadEventBasler = ::CreateEvent(
			NULL      // SECURITY_ATTRIBUTES構造体
			, TRUE      // リセットのタイプ( TRUE: 自動 / FALSE: 手動 )
			, FALSE     // 初期状態( TRUE: シグナル状態 / FALSE: 非シグナル状態 )
			, NULL      // イベントオブジェクトの名前
		);
		th_basler = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)BaslerThread, (void*)hThreadEventBasler, 0, (LPDWORD)&lpThreadIdBasler);

#if 0
		//Baslerが起動するまで待つ。
		SetTimer(hWnd, WAIT_TIME, 20000, NULL);
#else
		//		Baslerアプリへコマンド送信スレッドの生成
		hThreadEventUscope = ::CreateEvent(
			NULL      // SECURITY_ATTRIBUTES構造体
			, TRUE      // リセットのタイプ( TRUE: 自動 / FALSE: 手動 )
			, FALSE     // 初期状態( TRUE: シグナル状態 / FALSE: 非シグナル状態 )
			, NULL      // イベントオブジェクトの名前
		);
		th_uscope = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)uscopeThread, (void*)hThreadEventUscope, 0, (LPDWORD)&lpThreadIdUscope);
#endif
		break;

	case WM_BASLER_MONITOR_TIME_OUT:
		//Baslerの終了
		uScopehWnd = FindWindow(0, (LPCWSTR)appName);
		::PostMessage((HWND)uScopehWnd, WM_CLOSE, 0, 0);

		break;

	case WM_BASLER_CLOSE:

		//インストール関連の全ファイル、フォルダ削除
		Sleep(2000);//ファイルが削除できるようになるまでのWAIT
		FileDirectoryRemove();

		// スレッド終了メッセージの送信
		::PostThreadMessage(lpThreadIdText, WM_QUIT, 0, 0);
		// スレッド終了メッセージの送信
		::PostThreadMessage(lpThreadIdProg, WM_QUIT, 0, 0);
		// スレッド終了メッセージの送信
		::PostThreadMessage(lpThreadIdUscope, WM_QUIT, 0, 0);
		// スレッド終了メッセージの送信
		::PostThreadMessage(lpThreadIdBasler, WM_QUIT, 0, 0);

		break;

	case WM_TIMER:
		if (wParam == WAIT_TIME)
		{
			KillTimer(hWnd, WAIT_TIME);

			//		Baslerアプリへコマンド送信スレッドの生成
			hThreadEventUscope = ::CreateEvent(
				NULL      // SECURITY_ATTRIBUTES構造体
				, TRUE      // リセットのタイプ( TRUE: 自動 / FALSE: 手動 )
				, FALSE     // 初期状態( TRUE: シグナル状態 / FALSE: 非シグナル状態 )
				, NULL      // イベントオブジェクトの名前
			);
			th_uscope = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)uscopeThread, (void*)hThreadEventUscope, 0, (LPDWORD)&lpThreadIdUscope);
		}
		break;
	case WM_CLOSE:
		break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 選択されたメニューの解析:

		switch (wmId)
		{
		case IDM_ABOUT:
			//                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: HDC を使用する描画コードをここに追加してください...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
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
