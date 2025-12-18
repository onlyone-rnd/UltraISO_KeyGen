#pragma once

#include <windows.h>
#include <commctrl.h>
#include <Richedit.h>
#include <stdio.h>

const COLORREF rgbRed   = 0x000000FF;
const COLORREF rgbGreen = 0x0000FF00;
const COLORREF rgbBlue  = 0x00FF0000;
const COLORREF rgbBlack = 0x00000000;
const COLORREF rgbWhite = 0x00FFFFFF;
const COLORREF rgbCyan  = 0x00FFFF00;

extern HINSTANCE hInst;

typedef struct _PRINT_ITEM {
    WCHAR   Text[MAX_PATH * 2];
    ULONG    LineFeed;
    COLORREF Color;
} PRINT_ITEM;

extern HWND hWndDlg;
extern WCHAR TextBuf[MAX_PATH * 2];

void PrintText(
    _In_ PCWSTR Text,
    _In_ ULONG LineFeed,
    _In_ COLORREF Color);

void PrintMessage(
    _In_ PCWSTR Text,
    _In_ ULONG LineFeed,
    _In_ COLORREF Color);