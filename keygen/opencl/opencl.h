#pragma once

#include "../main.h"
#include <mutex>
#define CL_TARGET_OPENCL_VERSION 120
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include "cl/cl.h"

extern volatile SHORT StopThreads;
extern volatile SHORT PauseThreads;
extern volatile SHORT OnlyGPU;
extern volatile SHORT RegCodeFound;

extern volatile BOOL IsReleasedKG;

HANDLE InitOpenclOnce();
void ReleaseOpenclOnce();
void PrintOpenclDeviceInfo();
void PreInit();

BOOL InitKG(
    _In_ PWCHAR pUserName);
HANDLE RunKG();
void ExitKG();
