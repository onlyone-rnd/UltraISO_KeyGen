
#include "main.h"
#include <mutex>
#include "opencl/opencl.h"
#include "ntdll/ntdll.h"
#include "api/api.hpp"
#include "res/version.h"
#include "res/resource.h"

HINSTANCE hInst = NULL;
HWND hWndDlg = NULL;
HWND hRich = NULL;
HWND hTop = NULL;
HWND hGpu = NULL;
HWND hUserName = NULL;
HWND hStartBtn = NULL;
HWND hPauseBtn = NULL;
HWND hStopBtn = NULL;

HANDLE hInitThread = NULL;
HANDLE hKGThread = NULL;

ULONG ExitCode = 0;

WNDPROC OldWndProc = NULL;

ULONG NameSize = 0;
WCHAR UserName[MAX_PATH];
WCHAR TextBuf[MAX_PATH * 2];

LONG StartClearAll = 0;
LONG StartClearMode = 0;

volatile SHORT StopThreads = 0;
volatile SHORT PauseThreads = 0;
volatile SHORT OnlyGPU = 0;
volatile SHORT RegCodeFound = 0;

std::mutex pm_mtx;

#define INIT_TIMER 0x888
#define WAIT_TIMER 0x999
#define WM_APP_PRINTTEXT (WM_APP + 1)
#define WM_APP_INITOPENCL (WM_APP + 2)

void PrintText(
    _In_ PCWSTR Text,
    _In_ ULONG LineFeed,
    _In_ COLORREF Color)
{
    ULONG len = GetWindowTextLengthW(hRich);
    SendMessageW(hRich, EM_SETSEL, len, len);

    CHARFORMAT rich;
    memset(&rich, 0, sizeof(rich));
    rich.cbSize = sizeof(CHARFORMAT);
    rich.dwMask = CFM_COLOR;
    rich.crTextColor = Color;

    SendMessageW(hRich, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&rich);
    SendMessageW(hRich, EM_REPLACESEL, TRUE, (LPARAM)Text);
    for (ULONG i = 0; i < LineFeed; i++)
    {
        SendMessageW(hRich, EM_REPLACESEL, TRUE, (LPARAM)L"\r\n");
    }
    SendMessageW(hRich, EM_SCROLLCARET, 0, 0);
    SendMessageW(hRich, WM_VSCROLL, SB_BOTTOM, 0);
}

void PrintMessage(
    _In_ PCWSTR Text,
    _In_ ULONG LineFeed,
    _In_ COLORREF Color)
{
    std::lock_guard<std::mutex> lock(pm_mtx);
    PRINT_ITEM* item = (PRINT_ITEM*)AllocateHeap(sizeof(PRINT_ITEM));
    if (item)
    {
        wcscpy(item->Text, Text);
        item->Color = Color;
        item->LineFeed = LineFeed;
        PostMessageW(hWndDlg, WM_APP_PRINTTEXT, 0, (LPARAM)item);
    }
}

void ClearRichEdit()
{
    CHARRANGE cr;
    cr.cpMin = StartClearAll - 3;
    cr.cpMax = -1;

    SendMessageW(hRich, EM_EXSETSEL, 0, (LPARAM)&cr);
    SendMessageW(hRich, EM_REPLACESEL, FALSE, (LPARAM)L"");
}

void ClearMode()
{
    CHARRANGE cr;
    cr.cpMin = StartClearMode - 14;
    cr.cpMax = -1;

    SendMessageW(hRich, EM_EXSETSEL, 0, (LPARAM)&cr);
    SendMessageW(hRich, EM_REPLACESEL, FALSE, (LPARAM)L"");
}

#define VK_CTRL_C 0x3
#define VK_CTRL_V 0x16

LRESULT CALLBACK UserNameProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CHAR:
        {
            WCHAR c = wParam;
            if ((c < '0' || c > '9') &&
                (c < 'A' || c > 'Z') &&
                (c < 'a' || c > 'z') &&
                c != VK_BACK &&
                c != VK_SPACE &&
                c != VK_CTRL_C &&
                c != VK_CTRL_V)
            {
                return NULL;
            }
        }
    }
    return CallWindowProcW(
        OldWndProc,
        hWnd,
        uMsg,
        wParam,
        lParam);
}

void InitOpencl(
    HWND hWnd)
{
    ClearRichEdit();
    hInitThread = InitOpenclOnce();
    if (hInitThread)
        SetTimer(hWnd, INIT_TIMER, 100, nullptr);
    else
        PrintText(L"Error Create InitOpenclOnce Thread!", 1, rgbRed);
}

LRESULT CALLBACK DialogProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    USHORT id = 0;
    USHORT code = 0;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIconW(hInst, MAKEINTRESOURCE(MAIN_ICON)));
            SendMessageW(hWnd, WM_SETTEXT, 0, (LPARAM)PRODUCT_NAME);
            hWndDlg = hWnd;
            hTop = GetDlgItem(hWnd, TOP_CHK);
            hGpu = GetDlgItem(hWnd, GPU_CHK);
            hRich = GetDlgItem(hWnd, LOG_RED);
            hUserName = GetDlgItem(hWnd, NAME_EDT);
            hStartBtn = GetDlgItem(hWnd, START_BTN);
            hPauseBtn = GetDlgItem(hWnd, PAUSE_BTN);
            hStopBtn = GetDlgItem(hWnd, STOP_BTN);

            SendMessageW(hTop, BM_CLICK, 0, 0);
            SendMessageW(hGpu, BM_SETCHECK, 1, 0);
            if (SendMessageW(hGpu, BM_GETCHECK, 0, 0) == BST_CHECKED)
                _InterlockedCompareExchange16(&OnlyGPU, 1, 0);

            OldWndProc = (WNDPROC)SetWindowLongPtrW(hUserName, GWLP_WNDPROC, (LONG_PTR)UserNameProc);
            SendMessageW(hRich, EM_SETBKGNDCOLOR, FALSE, rgbBlack);
            SendMessageW(hRich, EM_SETSEL, SendMessageW(hRich, EM_LINEINDEX, 0, 0) - 1, -1);

            PrintText(PRODUCT_NAME, 1, rgbCyan);
            PrintText(LEGAL_COPYRIGHT, 2, rgbCyan);

            StartClearAll = GetWindowTextLengthW(hRich);

            ULONG cbUserName = _countof(UserName);
            GetUserNameW(UserName, &cbUserName);
            SendMessageW(hUserName, WM_SETTEXT, 0, (LPARAM)UserName);
            EnableWindow(hUserName, FALSE);
            EnableWindow(hStartBtn, FALSE);
            EnableWindow(hPauseBtn, FALSE);
            EnableWindow(hStopBtn, FALSE);

            InitOpencl(hWnd);
        }
        break;
        case WM_APP_PRINTTEXT:
        {
            PRINT_ITEM* item = (PRINT_ITEM*)lParam;
            if (item)
            {
                PrintText(item->Text, item->LineFeed, item->Color);
                FreeHeap(item);
            }
        }
        break;
        case WM_APP_INITOPENCL:
        {
            InitOpencl(hWnd);
        }
        break;
        case WM_TIMER:
        {
            switch (wParam)
            {
                case INIT_TIMER:
                {
                    if (WaitForSingleObject(hInitThread, 1) != WAIT_TIMEOUT)
                    {
                        GetExitCodeThread(hInitThread, &ExitCode);
                        CloseHandle(hInitThread);
                        hInitThread = NULL;
                        KillTimer(hWnd, INIT_TIMER);
                        if (ExitCode == ERROR_SUCCESS)
                        {
                            PrintOpenclDeviceInfo();
                            PreInit();
                            PrintText(L"Status Device: Ready!", 1, rgbGreen);
                            StartClearMode = GetWindowTextLengthW(hRich);
                            if (_InterlockedCompareExchange16(&OnlyGPU, 1, 1))
                                PrintText(L"[ GPU only ] mode is enabled!", 1, rgbGreen);
                            else
                                PrintText(L"[ GPU only ] mode is disabled!", 1, rgbGreen);
                            EnableWindow(hUserName, TRUE);
                            EnableWindow(hStartBtn, TRUE);
                            if (_InterlockedCompareExchange16(&RegCodeFound, 1, 1))
                            {
                                if (IsReleasedKG)
                                {
                                    IsReleasedKG = FALSE;
                                    goto StartGeneration;
                                }
                            }
                        }
                    }
                }
                break;
                case WAIT_TIMER:
                {
                    if (_InterlockedCompareExchange16(&RegCodeFound, 1, 1))
                    {
                        KillTimer(hWnd, WAIT_TIMER);
                        WaitForSingleObject(hKGThread, INFINITE);
                        _InterlockedCompareExchange16(&StopThreads, 0, 1);
                        CloseHandle(hKGThread);
                        hKGThread = NULL;
                        PrintText(L"\r\nGeneration stopped!", 1, rgbGreen);
                        ExitKG();
                        EnableWindow(hUserName, TRUE);
                        EnableWindow(hStartBtn, TRUE);
                        EnableWindow(hGpu, TRUE);
                        EnableWindow(hPauseBtn, FALSE);
                        EnableWindow(hStopBtn, FALSE);
                    }
                }
                break;
            }
        }
        break;
        case WM_CLOSE:
        {
            if (!hKGThread)
            {
                ExitKG();
                EndDialog(hWnd, NULL);
            }
        }
        break;
        case WM_COMMAND:
        {
            id = LOWORD(wParam);
            code = HIWORD(wParam);

            switch (id)
            {
                case START_BTN:
                {
                    if (_InterlockedCompareExchange16(&RegCodeFound, 1, 1) == 1)
                    {
                        if (IsReleasedKG)
                            InitOpencl(hWnd);                    
                    }
                    else
                    {
                        StartGeneration:
                        __stosw((PWORD)UserName, 0, _countof(UserName));
                        NameSize = SendMessageW(hUserName, WM_GETTEXT, _countof(UserName), (LPARAM)UserName);
                        if (NameSize)
                        {
                            PrintText(L"Generation Started!", 1, rgbGreen);
                            swprintf(TextBuf, L"\r\nRegistration Name: %s\0", UserName);
                            PrintText(TextBuf, 1, rgbWhite);
                            if (InitKG(UserName))
                            {
                                hKGThread = NULL;
                                hKGThread = RunKG();
                                if (hKGThread)
                                {
                                    EnableWindow(hUserName, FALSE);
                                    EnableWindow(hStartBtn, FALSE);
                                    EnableWindow(hGpu, FALSE);
                                    EnableWindow(hPauseBtn, TRUE);
                                    EnableWindow(hStopBtn, TRUE);
                                    _InterlockedCompareExchange16(&RegCodeFound, 0, 1);
                                    SetTimer(hWnd, WAIT_TIMER, 100, nullptr);
                                }
                                else
                                    PrintText(L"Error Create RunKG Thread!", 1, rgbRed);
                            }
                        }
                    }
                }
                break;
                case PAUSE_BTN:
                {
                    if (_InterlockedCompareExchange16(&PauseThreads, 1, 1))
                    {
                        _InterlockedCompareExchange16(&PauseThreads, 0, 1);
                        EnableWindow(hStopBtn, TRUE);
                        PrintText(L"Generation Continued!", 1, rgbGreen);
                    }
                    else
                    {
                        _InterlockedCompareExchange16(&PauseThreads, 1, 0);
                        EnableWindow(hStopBtn, FALSE);
                        PrintText(L"Generation Paused!", 1, rgbGreen);
                    }
                }
                break;
                case STOP_BTN:
                {
                    _InterlockedCompareExchange16(&StopThreads, 1, 0);
                    WaitForSingleObject(hKGThread, INFINITE);
                    _InterlockedCompareExchange16(&StopThreads, 0, 1);
                    CloseHandle(hKGThread);
                    hKGThread = NULL;
                    EnableWindow(hUserName, TRUE);
                    EnableWindow(hStartBtn, TRUE);
                    EnableWindow(hGpu, TRUE);
                    EnableWindow(hPauseBtn, FALSE);
                    EnableWindow(hStopBtn, FALSE);
                    ExitKG();
                    PostMessageW(hWndDlg, WM_APP_INITOPENCL, 0, 0);
                }
                break;
                case TOP_CHK:
                {
                    if (code == BN_CLICKED)
                    {
                        if (SendMessageW(hTop, BM_GETCHECK, 0, 0) == BST_CHECKED)
                            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
                        else
                            SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
                    }
                }
                break;
                case GPU_CHK:
                {
                    if (code == BN_CLICKED)
                    {
                        if (SendMessageW(hGpu, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            ClearMode();
                            _InterlockedCompareExchange16(&OnlyGPU, 1, 0);
                            PrintText(L"[ GPU only ] mode is enabled!", 1, rgbGreen);
                        }
                        else
                        {
                            ClearMode();
                            _InterlockedCompareExchange16(&OnlyGPU, 0, 1);
                            PrintText(L"[ GPU only ] mode is disabled!", 1, rgbGreen);
                        }
                    }
                }
                break;
            }
        }
        break;

    }
    return NULL;
}

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    hInst = hInstance;
    LoadLibraryW(L"Riched20.dll");
    InitCommonControls();
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    DialogBoxParam(hInst, MAKEINTRESOURCE(MAIN_DLG), NULL, DialogProc, NULL);
    DWORD err = GetLastError();
    ExitProcess(NULL);
    return 0;
}