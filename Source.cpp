#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>

#define DEFAULT_DPI 96
#define SCALEX(X) MulDiv(X, uDpiX, DEFAULT_DPI)
#define SCALEY(Y) MulDiv(Y, uDpiY, DEFAULT_DPI)
#define POINT2PIXEL(PT) MulDiv(PT, uDpiY, 72)

TCHAR szClassName[] = TEXT("Window");

BOOL GetScaling(HWND hWnd, UINT* pnX, UINT* pnY)
{
	BOOL bSetScaling = FALSE;
	const HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	if (hMonitor)
	{
		HMODULE hShcore = LoadLibrary(TEXT("SHCORE"));
		if (hShcore)
		{
			typedef HRESULT __stdcall GetDpiForMonitor(HMONITOR, int, UINT*, UINT*);
			GetDpiForMonitor* fnGetDpiForMonitor = reinterpret_cast<GetDpiForMonitor*>(GetProcAddress(hShcore, "GetDpiForMonitor"));
			if (fnGetDpiForMonitor)
			{
				UINT uDpiX, uDpiY;
				if (SUCCEEDED(fnGetDpiForMonitor(hMonitor, 0, &uDpiX, &uDpiY)) && uDpiX > 0 && uDpiY > 0)
				{
					*pnX = uDpiX;
					*pnY = uDpiY;
					bSetScaling = TRUE;
				}
			}
			FreeLibrary(hShcore);
		}
	}
	if (!bSetScaling)
	{
		HDC hdc = GetDC(NULL);
		if (hdc)
		{
			*pnX = GetDeviceCaps(hdc, LOGPIXELSX);
			*pnY = GetDeviceCaps(hdc, LOGPIXELSY);
			ReleaseDC(NULL, hdc);
			bSetScaling = TRUE;
		}
	}
	if (!bSetScaling)
	{
		*pnX = DEFAULT_DPI;
		*pnY = DEFAULT_DPI;
		bSetScaling = TRUE;
	}
	return bSetScaling;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hEdit;
	static HFONT hFont;
	static UINT uDpiX = DEFAULT_DPI, uDpiY = DEFAULT_DPI;
	switch (msg)
	{
	case WM_CREATE:
		hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), 0, WS_VISIBLE | WS_CHILD | ES_READONLY, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		{
			BOOL bDebuggerPresent;
			if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &bDebuggerPresent))
			{
				if (bDebuggerPresent)
				{
					SetWindowText(hEdit, TEXT("デバッグ中です。"));
				}
				else
				{
					SetWindowText(hEdit, TEXT("デバッグ中ではないです。"));
				}
			}
			else
			{
				SetWindowText(hEdit, TEXT("CheckRemoteDebuggerPresent関数が失敗しました。"));
			}
		}
		SendMessage(hWnd, WM_DPICHANGED, 0, 0);
		break;
	case WM_SIZE:
		MoveWindow(hEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		break;
	case WM_NCCREATE:
		{
			const HMODULE hModUser32 = GetModuleHandle(TEXT("user32.dll"));
			if (hModUser32)
			{
				typedef BOOL(WINAPI*fnTypeEnableNCScaling)(HWND);
				const fnTypeEnableNCScaling fnEnableNCScaling = (fnTypeEnableNCScaling)GetProcAddress(hModUser32, "EnableNonClientDpiScaling");
				if (fnEnableNCScaling)
				{
					fnEnableNCScaling(hWnd);
				}
			}
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	case WM_DPICHANGED:
		GetScaling(hWnd, &uDpiX, &uDpiY);
		DeleteObject(hFont);
		hFont = CreateFontW(-POINT2PIXEL(10), 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0, L"MS Shell Dlg");
		SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, 0);
		break;
	case WM_DESTROY:
		DeleteObject(hFont);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("プログラムがデバッグ中かどうか判別する"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
