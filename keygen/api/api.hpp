#pragma once

#include "../ntdll/ntdll.h"

PVOID AllocateHeap(
    SIZE_T size);

SIZE_T SizeHeap(
    PVOID memory);

BOOLEAN FreeHeap(
    PVOID memory);

PVOID ReAllocateHeap(
    PVOID baseAddress,
    SIZE_T size);

USHORT GetThreadsNumber();

FORCEINLINE void a2w(
    PSTR _In_ astr,
    PWSTR _Out_ wstr)
{
    for (; (*wstr = *astr); ++wstr, ++astr);
}

FORCEINLINE void w2a(
    PWSTR _In_ wstr,
    PSTR _Out_ astr)
{
    for (; (*astr = *wstr); ++astr, ++wstr);
}
