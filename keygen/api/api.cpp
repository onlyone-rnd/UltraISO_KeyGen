
#include "api.hpp"

PVOID AllocateHeap(
    SIZE_T size)
{
    return RtlAllocateHeap(
        RtlProcessHeap(),
        HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS,
        size);
}

SIZE_T SizeHeap(
    PVOID memory)
{
    if (!memory)
        return NULL;
    return RtlSizeHeap(
        RtlProcessHeap(),
        NULL,
        memory);
}

BOOLEAN FreeHeap(
    PVOID memory)
{
    if (!memory)
        return FALSE;
    SIZE_T size = SizeHeap(memory);
    if (size != (SIZE_T)-1)
        RtlZeroMemory(memory, size);

    return RtlFreeHeap(
        RtlProcessHeap(),
        NULL,
        memory);
}

PVOID ReAllocateHeap(
    PVOID baseAddress,
    SIZE_T size)
{
    if (!RtlProcessHeap())
        return NULL;
    return RtlReAllocateHeap(
        RtlProcessHeap(),
        HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS,
        baseAddress,
        size);
}

USHORT GetThreadsNumber()
{
    SYSTEM_BASIC_INFORMATION BasicInfo;

    NTSTATUS status = NtQuerySystemInformation(
        SystemBasicInformation,
        &BasicInfo,
        sizeof(BasicInfo),
        nullptr);
    if (!NT_SUCCESS(status))
        return NULL;
    USHORT threads = (USHORT)BasicInfo.NumberOfProcessors;
    if (threads < 2)
        threads = 2;
    if (threads > 64)
        threads = 64;
    return threads;
}