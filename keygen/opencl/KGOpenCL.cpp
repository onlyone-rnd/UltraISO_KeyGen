
#include "opencl.h"
#include "../gmp/gmp.h"
#include "../api/api.hpp"
#include "md5_512.hpp"
#include "../rnd/randomizer.h"
#include "../task/Task.h"
#include "../help/Helper.h"
#include "../whitelist.h"
#include "../res/resource.h"

#pragma warning(disable : 6054)

struct GPU_INFO
{
    UINT multiProcessorCount;
    UINT maxThreadsPerBlock;
    UINT maxThreadsPerMultiProcessor;
};

static cl_context       g_ctx = nullptr;
static cl_command_queue g_queue = nullptr;
static cl_program       g_prog = nullptr;
static cl_kernel        g_kernel = nullptr;
static cl_mem           g_msg_buf = nullptr;
static size_t           g_msg_len = 0;
static cl_device_id     g_device = nullptr;

static mpz_t d, n;

std::mutex m_mtx;
std::mutex r_mtx;
Task curr_task;

static BOOL m_finished;
static GPU_INFO m_gpu_prop;
static Randomizer m_rand;
static UINT num_threads_preblock;
static UINT num_blocks;
static PCHAR* m_dict;
static PCHAR  m_gpu_dict[16];
static PCHAR  m_cpu_dict[16];
static PSHORT m_sqnce[16];
static SHORT  m_globe_selected_pos[16];
static SHORT  m_globe_range[32];

static CHAR modulus_n[] = "A70F8F62AA6E97D1";
static CHAR d_str[] = "A7CAD9177AE95A9";

static CHAR g_msg[MAX_PATH];

volatile BOOL IsReleasedKG = FALSE;

cl_int check_cl(
    _In_ cl_int cl_status,
    _In_ PCWCH where)
{
    if (cl_status != CL_SUCCESS)
    {
        swprintf(TextBuf, L"OpenCL error %d at %s\0", cl_status, (PWCHAR)where);
        PrintMessage(TextBuf, 1, rgbRed);
    }
    return cl_status;
}

void PrintDeviceInfo(
    _In_ cl_device_id device)
{
    static CHAR DeviceNameA[MAX_PATH];
    static CHAR DeviceVendorA[MAX_PATH];
    static WCHAR BufferW[MAX_PATH];
    cl_int cl_status;
    cl_device_type DeviceType = 0;

    num_blocks = 0;
    num_threads_preblock = 0;

    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(DeviceNameA), DeviceNameA, nullptr);
    clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(DeviceVendorA), DeviceVendorA, nullptr);
    clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(DeviceType), &DeviceType, nullptr);
    cl_uint compute_units = 0;
    cl_status = check_cl(clGetDeviceInfo(
        device,
        CL_DEVICE_MAX_COMPUTE_UNITS,
        sizeof(compute_units),
        &compute_units,
        nullptr),
        L"clGetDeviceInfo(units)");
    if (cl_status == CL_SUCCESS)
        m_gpu_prop.multiProcessorCount = compute_units;
    size_t max_wg = 0;
    cl_status = check_cl(clGetDeviceInfo(
        device,
        CL_DEVICE_MAX_WORK_GROUP_SIZE,
        sizeof(max_wg),
        &max_wg,
        nullptr),
        L"clGetDeviceInfo(max_wg)");
    if (cl_status == CL_SUCCESS)
        m_gpu_prop.maxThreadsPerBlock = (UINT)max_wg;
    m_gpu_prop.maxThreadsPerMultiProcessor = m_gpu_prop.maxThreadsPerBlock;
    if (m_gpu_prop.multiProcessorCount == 0 || m_gpu_prop.maxThreadsPerBlock == 0)
        return;
    num_threads_preblock = m_gpu_prop.maxThreadsPerBlock;
    num_blocks = m_gpu_prop.multiProcessorCount;

    PCWCHAR szType = L"UNKNOWN";
    if (DeviceType & CL_DEVICE_TYPE_GPU)
        szType = L"GPU";
    else if (DeviceType & CL_DEVICE_TYPE_CPU)
        szType = L"CPU";
    else if (DeviceType & CL_DEVICE_TYPE_ACCELERATOR)
        szType = L"ACCELERATOR";
    PrintText(L"[ OpenCL Platform / Device ]\0", 1, rgbWhite);
    __stosw((PWORD)BufferW, 0, _countof(BufferW));
    a2w(DeviceVendorA, BufferW);
    swprintf(TextBuf, L"  Vendor : %s\0", BufferW);
    PrintText(TextBuf, 1, rgbWhite);
    __stosw((PWORD)BufferW, 0, _countof(BufferW));
    a2w(DeviceNameA, BufferW);
    swprintf(TextBuf, L"  Device : %s\0", BufferW);
    PrintText(TextBuf, 1, rgbWhite);
    swprintf(TextBuf, L"  Type    : %s\0", szType);
    PrintText(TextBuf, 2, rgbWhite);
    PrintText(L"[ Uses ]\0", 1, rgbWhite);
    swprintf(TextBuf, L"  Multiprocessor Count : %d\0", m_gpu_prop.multiProcessorCount);
    PrintText(TextBuf, 1, rgbWhite);
    swprintf(TextBuf, L"  Threads Count : %d\0", num_threads_preblock);
    PrintText(TextBuf, 1, rgbWhite);
    swprintf(TextBuf, L"  Blocks Count  : %d\0", num_blocks);
    PrintText(TextBuf, 2, rgbWhite);
}

ULONG InitOpenclThread(
    _In_ PVOID Param)
{
    if (g_ctx)
        return ERROR_SUCCESS;
    HRSRC hResInfo = NULL;
    HGLOBAL hResData = NULL;
    PCCH src_ptr = nullptr;
    SIZE_T src_len = 0;
    cl_int cl_status;
    cl_uint PlatformCount = 0;
    cl_platform_id platform = nullptr;
    cl_platform_id Platforms[8] = { 0 };
    PCCH build_opts = "-cl-mad-enable -cl-fast-relaxed-math";
    cl_status = check_cl(clGetPlatformIDs(0, nullptr, &PlatformCount), L"clGetPlatformIDs(count)");
    if (cl_status != CL_SUCCESS)
        goto error_exit;
    cl_status = check_cl(clGetPlatformIDs(PlatformCount, Platforms, nullptr), L"clGetPlatformIDs(list)");
    if (cl_status != CL_SUCCESS)
        goto error_exit;
    platform = Platforms[0];
    cl_status = check_cl(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &g_device, nullptr), L"clGetDeviceIDs(CPU fallback)");
    if (cl_status != CL_SUCCESS)
        goto error_exit;
    g_ctx = clCreateContext(nullptr, 1, &g_device, nullptr, nullptr, &cl_status);
    if (check_cl(cl_status, L"clCreateContext") != CL_SUCCESS)
        goto error_exit;
    g_queue = clCreateCommandQueue(g_ctx, g_device, 0, &cl_status);
    if (check_cl(cl_status, L"clCreateCommandQueue") != CL_SUCCESS)
        goto error_exit;

    hResInfo = FindResourceW(hInst, MAKEINTRESOURCE(OPEN_CL), RT_RCDATA);
    if (!hResInfo)
        goto error_exit;
    src_len = (SIZE_T)SizeofResource(hInst, hResInfo);
    if (!src_len)
        goto error_exit;
    hResData = LoadResource(hInst, hResInfo);
    if (!hResData)
        goto error_exit;
    src_ptr = (PCCH)LockResource(hResData);
    if (!src_ptr)
        goto error_exit;
    g_prog = clCreateProgramWithSource(g_ctx, 1, &src_ptr, &src_len, &cl_status);
    if (check_cl(cl_status, L"clCreateProgramWithSource") != CL_SUCCESS)
        goto error_exit;
    cl_status = check_cl(clBuildProgram(g_prog, 1, &g_device, build_opts, nullptr, nullptr), L"clBuildProgram");
    if (cl_status != CL_SUCCESS)
        goto error_exit;
    g_kernel = clCreateKernel(g_prog, "KGkernel", &cl_status);
    if (check_cl(cl_status, L"clCreateKernel") != CL_SUCCESS)
        goto error_exit;
    return ERROR_SUCCESS;
error_exit:
    {
        if (src_ptr)
        {
            FreeHeap((PVOID)src_ptr);
            src_ptr = nullptr;
        }
        if (g_kernel)
        {
            clReleaseKernel(g_kernel);
            g_kernel = nullptr;
        }
        if (g_prog)
        {
            clReleaseProgram(g_prog);
            g_prog = nullptr;
        }
        if (g_queue)
        {
            clReleaseCommandQueue(g_queue);
            g_queue = nullptr;
        }
        if (g_ctx)
        {
            clReleaseContext(g_ctx);
            g_ctx = nullptr;
        }
    }
    return ERROR_APP_INIT_FAILURE;
}

HANDLE InitOpenclOnce()
{
    return CreateThread(nullptr, NULL, InitOpenclThread, nullptr, 0, nullptr);
}

void PrintOpenclDeviceInfo()
{
    if (g_device)
        PrintDeviceInfo(g_device);
}

void ProduceSqnce()
{
    std::vector<short> s;
    m_rand.change_seed(0);
    s.assign({ 0, 1 });
    m_sqnce[0] = m_rand.unique_randomize(s, s.size());

    m_rand.change_seed(1);
    m_sqnce[1] = m_rand.unique_randomize(s, s.size());

    m_rand.change_seed(6);
    m_sqnce[6] = m_rand.unique_randomize(s, s.size());

    m_rand.change_seed(7);
    s.assign({ 0, 1, 2 });
    m_sqnce[7] = m_rand.unique_randomize(s, s.size());

    s.assign({ 0 });
    m_sqnce[8] = m_rand.unique_randomize(s, s.size());
    m_sqnce[9] = m_rand.unique_randomize(s, s.size());

    s.assign({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });
    for (int i = 2; i < 6; i++)
    {
        m_rand.change_seed(i);
        m_sqnce[i] = m_rand.unique_randomize(s, s.size());
    }

    for (int i = 10; i < 16; i++)
    {
        m_rand.change_seed(i);
        m_sqnce[i] = m_rand.unique_randomize(s, s.size());
    }
}

void ProduceGpuDict()
{
    std::vector<char> s;
    s = { '\x4', '\x5' };
    m_gpu_dict[0] = new char[2];
    for (int i = 0; i < 2; i++)
    {
        m_gpu_dict[0][i] = s[m_sqnce[0][i]];
    }

    s = { '\x5', '\x6' };
    m_gpu_dict[1] = new char[2];
    for (int i = 0; i < 2; i++)
    {
        m_gpu_dict[1][i] = s[m_sqnce[1][i]];
    }

    s = { '\x2', '\xD' };
    m_gpu_dict[6] = new char[2];
    for (int i = 0; i < 2; i++)
    {
        m_gpu_dict[6][i] = s[m_sqnce[6][i]];
    }

    s = { '\xA', '\xB', '\xC' };
    m_gpu_dict[7] = new char[2];
    for (int i = 0; i < 3; i++)
    {
        m_gpu_dict[7][i] = s[m_sqnce[7][i]];
    }

    m_gpu_dict[8] = new char[1];
    m_gpu_dict[9] = new char[1];
    m_gpu_dict[8][0] = 5;
    m_gpu_dict[9][0] = 3;

    s = { '\x0', '\x1', '\x2', '\x3', '\x4', '\x5', '\x6', '\x7',
        '\x8', '\x9', '\xA', '\xB', '\xC', '\xD', '\xE', '\xF' };
    for (int i = 2; i < 6; i++)
    {
        m_gpu_dict[i] = new char[16];
        for (int j = 0; j < 16; j++)
        {
            m_gpu_dict[i][j] = s[m_sqnce[i][j]];
        }
    }

    for (int i = 10; i < 16; i++)
    {
        m_gpu_dict[i] = new char[16];
        for (int j = 0; j < 16; j++)
        {
            m_gpu_dict[i][j] = s[m_sqnce[i][j]];
        }
    }
}

void ProduceCpuDict()
{
    std::string s;
    s = "45";
    m_cpu_dict[0] = new char[2];
    for (int i = 0; i < 2; i++)
    {
        m_cpu_dict[0][i] = s[m_sqnce[0][i]];
    }

    s = "56";
    m_cpu_dict[1] = new char[2];
    for (int i = 0; i < 2; i++)
    {
        m_cpu_dict[1][i] = s[m_sqnce[1][i]];
    }

    s = "2D";
    m_cpu_dict[6] = new char[2];
    for (int i = 0; i < 2; i++)
    {
        m_cpu_dict[6][i] = s[m_sqnce[6][i]];
    }

    s = "ABC";
    m_cpu_dict[7] = new char[2];
    for (int i = 0; i < 3; i++)
    {
        m_cpu_dict[7][i] = s[m_sqnce[7][i]];
    }

    m_cpu_dict[8] = new char[1];
    m_cpu_dict[9] = new char[1];
    m_cpu_dict[8][0] = '5';
    m_cpu_dict[9][0] = '3';

    s = "0123456789ABCDEF";
    for (int i = 2; i < 6; i++)
    {
        m_cpu_dict[i] = new char[16];
        for (int j = 0; j < 16; j++)
        {
            m_cpu_dict[i][j] = s[m_sqnce[i][j]];
        }
    }

    for (int i = 10; i < 16; i++)
    {
        m_cpu_dict[i] = new char[16];
        for (int j = 0; j < 16; j++)
        {
            m_cpu_dict[i][j] = s[m_sqnce[i][j]];
        }
    }
}

void PreInit()
{
    ProduceSqnce();
    ProduceCpuDict();
    ProduceGpuDict();
    m_dict = m_gpu_dict;
    memset(m_globe_selected_pos, 0, 16 * sizeof(short));
    m_globe_selected_pos[15] = -1;
    Task::init_globe_range();
    mpz_init_set_str(n, modulus_n, 16);
    mpz_init_set_str(d, d_str, 16);
}

BOOL InitKG(
    _In_ PWCHAR pUserName)
{
    PCHAR msg = (PCHAR)AllocateHeap(MAX_PATH);
    if (!msg)
        return FALSE;
    strcpy(msg, "UTRISO");
    w2a(pUserName, &msg[6]);
    g_msg_len = strlen(msg);
    strcpy(g_msg, msg);
    if (g_msg_buf)
    {
        clReleaseMemObject(g_msg_buf);
        g_msg_buf = nullptr;
    }
    cl_int cl_status;
    g_msg_buf = clCreateBuffer(
        g_ctx,
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        g_msg_len,
        msg,
        &cl_status);
    if (msg)
        FreeHeap(msg);
    check_cl(cl_status, L"clCreateBuffer(msg)");
    if (cl_status != CL_SUCCESS)
        return FALSE;
    return TRUE;
}

void GpuKG(
    _In_ PULONG64 data,
    _In_ int blocks,
    _In_ int threads)
{
    cl_int cl_status;
    size_t total = static_cast<size_t>(blocks) * static_cast<size_t>(threads);
    cl_mem buf_data = clCreateBuffer(g_ctx,
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        total * sizeof(cl_ulong),
        data,
        &cl_status);
    cl_status = check_cl(cl_status, L"clCreateBuffer(data)");
    if (cl_status == CL_SUCCESS)
    {
        cl_status =  clSetKernelArg(g_kernel, 0, sizeof(cl_mem), &buf_data);
        cl_status |= clSetKernelArg(g_kernel, 1, sizeof(cl_mem), &g_msg_buf);
        cl_status |= clSetKernelArg(g_kernel, 2, sizeof(int), &g_msg_len);
        cl_status = check_cl(cl_status, L"clSetKernelArg");
        if (cl_status == CL_SUCCESS)
        {
            size_t global_size = total;
            size_t local_size = static_cast<size_t>(threads);
            cl_status = clEnqueueNDRangeKernel(
                g_queue,
                g_kernel,
                1,
                nullptr,
                &global_size,
                &local_size,
                0,
                nullptr,
                nullptr);
            cl_status = check_cl(cl_status, L"clEnqueueNDRangeKernel");
            if (cl_status == CL_SUCCESS)
            {
                cl_status = check_cl(clFinish(g_queue), L"clFinish");
                if (cl_status == CL_SUCCESS)
                {
                    cl_status = clEnqueueReadBuffer(
                        g_queue,
                        buf_data,
                        CL_TRUE,
                        0,
                        total * sizeof(cl_ulong),
                        data,
                        0,
                        nullptr,
                        nullptr);
                    check_cl(cl_status, L"clEnqueueReadBuffer");
                }
            }
        }
        clReleaseMemObject(buf_data);
    }
}

void ReleaseOpenclOnce()
{
    if (g_msg_buf)
    {
        clReleaseMemObject(g_msg_buf);
        g_msg_buf = nullptr;
    }
    if (g_kernel)
    {
        clReleaseKernel(g_kernel);
        g_kernel = nullptr;
    }
    if (g_prog)
    {
        clReleaseProgram(g_prog);
        g_prog = nullptr;
    }
    if (g_queue)
    {
        clReleaseCommandQueue(g_queue);
        g_queue = nullptr;
    }
    if (g_ctx)
    {
        clReleaseContext(g_ctx);
        g_ctx = nullptr;
    }
    g_device = nullptr;
    IsReleasedKG = TRUE;
}

void ExitKG()
{
    if (n && d)
        mpz_clears(n, d, NULL);
    ReleaseOpenclOnce();
}

Task* RequestTask(
    _In_ size_t num)
{
    Task* range_result_p = new Task();
    m_mtx.lock();
    SHORT selected_pos[16];
    memcpy(selected_pos, m_globe_selected_pos, sizeof(selected_pos));
    SHORT curr_line = 15;
    UINT count = 0;
    while (curr_line >= 0)
    {
        if (++selected_pos[curr_line] > Task::m_globe_range[curr_line * 2 + 1])
        {
            selected_pos[curr_line] = Task::m_globe_range[curr_line * 2];
            --curr_line;
        }
        else
        {
            curr_line = 15;
            if (++count >= num)
            {
                break;
            }
        }
    }
    if (count >= num || curr_line < 0)
    {
        for (int i = 0; i < 16; i += 1)
        {
            range_result_p->start_range[i] = m_globe_selected_pos[i];
        }
        range_result_p->count = count;
        memcpy(m_globe_selected_pos, selected_pos, 16 * sizeof(short));
    }
    if (m_finished)
    {
        range_result_p->count = 0;
    }
    if (curr_line < 0)
    {
        m_finished = true;
    }
    m_mtx.unlock();
    return range_result_p;
}

PULONG64 GpuGenerateData(
    _In_ Task* _task)
{
    ULONG64 pat;

    PSHORT selected_pos = nullptr;
    SHORT curr_line = 15;
    selected_pos = _task->start_range;
    curr_task = *_task;
    PULONG64 data_p = new ULONG64[_task->count];
    ULONG count_i = 0;

    while (curr_line >= 0 && _task->count > 0)
    {
        if (++selected_pos[curr_line] > Task::m_globe_range[curr_line * 2 + 1])
        {
            selected_pos[curr_line] = Task::m_globe_range[curr_line * 2];
            --curr_line;
        }
        else
        {
            _task->count -= 1;
            curr_line = 15;
            pat = 0;
            pat |= (ULONG64)(m_dict[0][selected_pos[0]]) << 15 * 4;
            pat |= (ULONG64)(m_dict[1][selected_pos[1]]) << 14 * 4;
            pat |= (ULONG64)(m_dict[2][selected_pos[2]]) << 13 * 4;
            pat |= (ULONG64)(m_dict[3][selected_pos[3]]) << 12 * 4;
            pat |= (ULONG64)(m_dict[4][selected_pos[4]]) << 11 * 4;
            pat |= (ULONG64)(m_dict[5][selected_pos[5]]) << 10 * 4;
            pat |= (ULONG64)(m_dict[6][selected_pos[6]]) << 9 * 4;
            pat |= (ULONG64)(m_dict[7][selected_pos[7]]) << 8 * 4;
            pat |= (ULONG64)(m_dict[8][selected_pos[8]]) << 7 * 4;
            pat |= (ULONG64)(m_dict[9][selected_pos[9]]) << 6 * 4;
            pat |= (ULONG64)(m_dict[10][selected_pos[10]]) << 5 * 4;
            pat |= (ULONG64)(m_dict[11][selected_pos[11]]) << 4 * 4;
            pat |= (ULONG64)(m_dict[12][selected_pos[12]]) << 3 * 4;
            pat |= (ULONG64)(m_dict[13][selected_pos[13]]) << 2 * 4;
            pat |= (ULONG64)(m_dict[14][selected_pos[14]]) << 1 * 4;
            pat |= (ULONG64)(m_dict[15][selected_pos[15]]);
            data_p[count_i++] = pat;
        }
    }
    return data_p;
}

BOOL PrintRegCode(
    _In_ PCHAR regcode)
{
    std::lock_guard<std::mutex> lock(r_mtx);
    if (strlen(regcode) != 16)
        return FALSE;

    WCHAR LocalTextBuf[64];

    swprintf(LocalTextBuf, L"Registration Code: %c%c%c%c-%c%c%c%c-%c%c%c%c-%c%c%c%c\0",
        towupper(regcode[0]), towupper(regcode[1]), towupper(regcode[2]), towupper(regcode[3]),
        towupper(regcode[4]), towupper(regcode[5]), towupper(regcode[6]), towupper(regcode[7]),
        towupper(regcode[8]), towupper(regcode[9]), towupper(regcode[10]), towupper(regcode[11]),
        towupper(regcode[12]), towupper(regcode[13]), towupper(regcode[14]), towupper(regcode[15]));
    PrintMessage(LocalTextBuf, 1, rgbWhite);
    return TRUE;
}

const CHAR hex2str_map[] = "0123456789ABCDEF";

void GpuLocate(
    _In_ int step)
{
    SHORT selected_pos[16] = { 0 };
    memset(selected_pos, 0, 16 * sizeof(SHORT));
    SHORT curr_line = 15;
    memcpy(selected_pos, curr_task.start_range, sizeof(selected_pos));
    int local_step = -1;
    while (curr_line >= 0)
    {
        if (++selected_pos[curr_line] > Task::m_globe_range[curr_line * 2 + 1])
        {
            selected_pos[curr_line] = Task::m_globe_range[curr_line * 2];
            --curr_line;
        }
        else
        {
            curr_line = 15;
            if (++local_step >= step)
            {
                std::string pat;
                for (int i = 0; i < 16; i++)
                {
                    pat += hex2str_map[m_dict[i][selected_pos[i]]];
                }
                mpz_t m;
                mpz_init_set_str(m, pat.c_str(), 16);
                mpz_powm(m, m, d, n);
                PCHAR s = mpz_get_str(nullptr, 16, m);
                mpz_clear(m);

                BOOL res = PrintRegCode(s);
                free(s);
                if (res)
                {
                    _InterlockedCompareExchange16(&RegCodeFound, 1, 0);
                    _InterlockedCompareExchange16(&StopThreads, 1, 0);
                    break;
                }
            }
        }
    }
}

void GpuFindingMatch(
    _In_ PULONG64 md5,
    _In_ int len)
{
    for (int step = len - 1; step >= 0; step--)
    {
        int len_data = 68553;
        int i = 0;
        int index;
        do
        {
            index = (len_data + i) / 2;
            int result = memcmp(&whitelist[6 * index], (PCHAR)&md5[step], 6);
            if (result <= 0)
            {
                if (result >= 0)
                {
                    GpuLocate(step);
                    break;
                }
                i = index + 1;
            }
            else
            {
                len_data = index - 1;
            }
        } while (i <= len_data);
    }
}

void GpuDispatch(
    _In_ Task* _task)
{
    Helper::factor factor;
    UINT gpu_capability = num_blocks * num_threads_preblock;
    if (_task->count == gpu_capability)
    {
        factor = { static_cast<int>(num_blocks),
           static_cast<int>(num_threads_preblock) };
    }
    else
    {
        factor = Helper::factoring((int)_task->count, num_threads_preblock);
    }
    PULONG64 data = GpuGenerateData(_task);
    GpuKG(data, factor.a, factor.b);
    GpuFindingMatch(data, factor.a * factor.b);
    delete data;
}

void CALLBACK DriveGPU(
    _In_ PTP_CALLBACK_INSTANCE inst,
    _In_ PVOID param,
    _In_ PTP_WORK work)
{
    UINT gpu_capability = num_blocks * num_threads_preblock;
    Task* task = nullptr;
    while (task = RequestTask(gpu_capability), task->count > 0)
    {
        GpuDispatch(task);
        delete task;
        task = nullptr;
        if (_InterlockedCompareExchange16(&StopThreads, 1, 1))
            break;
        for (; _InterlockedCompareExchange16(&PauseThreads, 1, 1); )
            Sleep(100);
    }
    if (task)
        delete task;
}

void CALLBACK DriveCPU(
    _In_ PTP_CALLBACK_INSTANCE inst,
    _In_ PVOID param,
    _In_ PTP_WORK work)
{
    Task* task = (Task*)param;
    mpz_t m;
    CHAR pat[17];
    memset(pat, 0, sizeof(pat));
    PSHORT selected_pos = nullptr;
    SHORT curr_line = 15;
    selected_pos = task->start_range;
    UCHAR msg[64];
    memset(msg, 0, sizeof(msg));
    *reinterpret_cast<PUINT>(&msg[56]) = (UINT)(g_msg_len + 16) * 8;
    msg[g_msg_len + 16] = 0x80;
    memcpy(msg, g_msg, g_msg_len);
    ULONG64 md5;
    while (curr_line >= 0 && task->count > 0)
    {
        if (++selected_pos[curr_line] > Task::m_globe_range[curr_line * 2 + 1])
        {
            selected_pos[curr_line] = Task::m_globe_range[curr_line * 2];
            --curr_line;
        }
        else
        {
            task->count -= 1;
            curr_line = 15;
            pat[0] = m_cpu_dict[0][selected_pos[0]];
            pat[1] = m_cpu_dict[1][selected_pos[1]];
            pat[2] = m_cpu_dict[2][selected_pos[2]];
            pat[3] = m_cpu_dict[3][selected_pos[3]];
            pat[4] = m_cpu_dict[4][selected_pos[4]];
            pat[5] = m_cpu_dict[5][selected_pos[5]];
            pat[6] = m_cpu_dict[6][selected_pos[6]];
            pat[7] = m_cpu_dict[7][selected_pos[7]];
            pat[8] = m_cpu_dict[8][selected_pos[8]];
            pat[9] = m_cpu_dict[9][selected_pos[9]];
            pat[10] = m_cpu_dict[10][selected_pos[10]];
            pat[11] = m_cpu_dict[11][selected_pos[11]];
            pat[12] = m_cpu_dict[12][selected_pos[12]];
            pat[13] = m_cpu_dict[13][selected_pos[13]];
            pat[14] = m_cpu_dict[14][selected_pos[14]];
            pat[15] = m_cpu_dict[15][selected_pos[15]];
            mpz_init_set_str(m, pat, 16);
            mpz_powm(m, m, d, n);
            mpz_get_str((PCHAR)&msg[g_msg_len], 16, m);
            mpz_clear(m);
            if (strlen((PCHAR)&msg[g_msg_len]) == 16)
            {
                _strupr((PCHAR)&msg[g_msg_len]);
                msg[g_msg_len + 16] = 0x80;
                md5 = md5_512((PULONG)msg);
                int len_data = 68553;
                int i = 0;
                int index;
                do
                {
                    index = (len_data + i) / 2;
                    int result = memcmp(&whitelist[6 * index], (PUCHAR)&md5, 6);
                    if (result <= 0)
                    {
                        if (result >= 0)
                        {
                            PCHAR s = (PCHAR)&msg[g_msg_len];
                            BOOL res = PrintRegCode(s);
                            if (res)
                            {
                                _InterlockedCompareExchange16(&RegCodeFound, 1, 0);
                                _InterlockedCompareExchange16(&StopThreads, 1, 0);
                                break;
                            }
                        }
                        i = index + 1;
                    }
                    else
                    {
                        len_data = index - 1;
                    }
                } while (i <= len_data);
            }
        }
        if (_InterlockedCompareExchange16(&StopThreads, 1, 1))
            break;
        for (; _InterlockedCompareExchange16(&PauseThreads, 1, 1); )
            Sleep(100);
    }
}

#define CPU_CAPABILITY 1000000

ULONG DriveKG(
    _In_ PVOID Param)
{
    USHORT Threads = 0;
    if (_InterlockedCompareExchange16(&OnlyGPU, 1, 1))
        Threads = 1;
    else
    {
        Threads = GetThreadsNumber();
        Threads /= 2;
    }
    PTP_POOL pool = CreateThreadpool(nullptr);
    if (!pool)
    {
        PrintMessage(L"Error CreateThreadpool!\0", 1, rgbRed);
        return ERROR_SUCCESS;
    }
    SetThreadpoolThreadMaximum(pool, Threads);
    if (!SetThreadpoolThreadMinimum(pool, Threads))
    {
        PrintMessage(L"Error SetThreadpoolThreadMinimum!\0", 1, rgbRed);
        CloseThreadpool(pool);
        return ERROR_SUCCESS;
    }
    TP_CALLBACK_ENVIRON env;
    InitializeThreadpoolEnvironment(&env);
    SetThreadpoolCallbackPool(&env, pool);
    PTP_CLEANUP_GROUP cg = CreateThreadpoolCleanupGroup();
    if (!cg)
    {
        PrintMessage(L"Error CreateThreadpoolCleanupGroup!\0", 1, rgbRed);
        DestroyThreadpoolEnvironment(&env);
        CloseThreadpool(pool);
        return ERROR_SUCCESS;
    }
    SetThreadpoolCallbackCleanupGroup(&env, cg, nullptr);

    Task** tasks = nullptr;
    USHORT works_num = 0;
    USHORT tasks_num = 0;
    PTP_WORK* works = (PTP_WORK*)AllocateHeap(sizeof(PTP_WORK) * Threads);
    if (!works)
    {
        PrintMessage(L"Error Allocate works!\0", 1, rgbRed);
        goto Exit;
    }
    tasks = (Task**)AllocateHeap(sizeof(PTP_WORK) * Threads);
    if (!tasks)
    {
        PrintMessage(L"Error Allocate tasks!\0", 1, rgbRed);
        goto Exit;
    }
    tasks[0] = (Task*)new int(1);
    works[0] = CreateThreadpoolWork(DriveGPU, tasks[0], &env);
    if (!works[0])
    {
        PrintMessage(L"Error [DriveGPU] CreateThreadpoolWork!\0", 1, rgbRed);
        goto Exit;
    }
    SubmitThreadpoolWork(works[0]);
    if (_InterlockedCompareExchange16(&OnlyGPU, 1, 1) != 1)
    {
        for (;;)
        {
            works_num = 1;
            tasks_num = 1;
            for (USHORT i = 1; i < Threads; i++)
            {
                tasks[i] = RequestTask(CPU_CAPABILITY);
                if (!tasks[i])
                {
                    PrintMessage(L"Error RequestTask!\0", 1, rgbRed);
                    goto Exit;
                }
                tasks_num++;
                works[i] = CreateThreadpoolWork(DriveCPU, tasks[i], &env);
                if (!works[i])
                {
                    PrintMessage(L"Error [DriveCPU] CreateThreadpoolWork!\0", 1, rgbRed);
                    goto Exit;
                }
                works_num++;
                SubmitThreadpoolWork(works[i]);
            }
            for (USHORT i = 1; i < Threads; i++)
            {
                WaitForThreadpoolWorkCallbacks(works[i], FALSE);
                CloseThreadpoolWork(works[i]);
                works[i] = nullptr;
                delete(tasks[i]);
                tasks[i] = nullptr;
            }
            if (_InterlockedCompareExchange16(&StopThreads, 1, 1))
            {
                works_num = 0;
                tasks_num = 0;
                break;
            }
            for (; _InterlockedCompareExchange16(&PauseThreads, 1, 1); )
                Sleep(100);
        }
    }
    WaitForThreadpoolWorkCallbacks(works[0], FALSE);
    CloseThreadpoolWork(works[0]);
    works[0] = nullptr;
    delete(tasks[0]);
    tasks[0] = nullptr;

Exit:
    for (SIZE_T j = 0; j < works_num; j++)
    {
        if (works[j])
            CloseThreadpoolWork(works[j]);
    }
    for (SIZE_T j = 0; j < tasks_num; j++)
    {
        if (tasks[j])
            delete(tasks[j]);
    }
    if (works)
        FreeHeap(works);
    if (tasks)
        FreeHeap(tasks);
    CloseThreadpoolCleanupGroupMembers(cg, FALSE, nullptr);
    CloseThreadpoolCleanupGroup(cg);
    DestroyThreadpoolEnvironment(&env);
    CloseThreadpool(pool);
    return ERROR_SUCCESS;
}

HANDLE RunKG()
{
    return CreateThread(nullptr, NULL, DriveKG, nullptr, 0, nullptr);
}
