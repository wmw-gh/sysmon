#include <windows.h>
#include <winternl.h>
#include <stdbool.h>
#include "cpu.h"
#include "assembly.h"

#define MAX_SAMPLES 10

typedef __kernel_entry NTSTATUS (*query_sys_info_fptr)(SYSTEM_INFORMATION_CLASS SystemInformationClass, 
													   PVOID SystemInformation,
													   ULONG SystemInformationLength,
													   PULONG ReturnLength);
static query_sys_info_fptr query_sys_info_local;

static int g_coreCount;
static long long* g_coreData;
static float* g_coreLoad;
static int sample;

static void read_load(void)
{
	int size = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * 64;
	int* kernelHandle = malloc(size);
	memset(kernelHandle, 0, size);
	unsigned long retSize;
	
	NTSTATUS status = query_sys_info_local(SystemProcessorPerformanceInformation,
										   (void*)kernelHandle,
										   (unsigned int)size, 
										   &retSize);
	
	SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* perfInformationPtr = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION*)kernelHandle;
	int perfItemsCount = (int)(retSize / sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));
	
    for (int i = 0; i < perfItemsCount; i++)
    {
		g_coreData[i]   = perfInformationPtr[i].IdleTime.QuadPart;
		g_coreData[i+1] = perfInformationPtr[i].KernelTime.QuadPart + perfInformationPtr[i].UserTime.QuadPart;
    }
	free(kernelHandle);
}

unsigned __stdcall cpu_load_thread(void) 
{
	while (1)
	{
		long long* l_coreData = malloc(sizeof(long long) * g_coreCount * 2);
		
		for (int i = 0; i < g_coreCount; i++)
		{
			l_coreData[i]   = g_coreData[i];   // idle
			l_coreData[i+1] = g_coreData[i+1]; // total
		}
		
		read_load();
		
		for (int i = 0; i < g_coreCount; i++)
		{
			long long idle_delta = (g_coreData[i] - l_coreData[i]);
			long long busy_delta = (g_coreData[i+1] - l_coreData[i+1]);
			double idle;
			
			if (idle_delta > busy_delta)
			{
				idle = 1.0;
			}
			else
			{
				// avoid division by 0
				if (busy_delta == 0)
				{
					idle = 0.0;
				}
				else
				{
					idle  = idle_delta / (double)busy_delta;
				}
			}
			
			// normalize idle ratio to between 0.0 and 1.0
			idle = idle < 0.0 ? 0.0 : idle;
			idle = idle > 1.0 ? 1.0 : idle;
			
			float load_local;
			
			if (1.0 == idle)
			{
				load_local = 0.0;
			}
			else
			{
				load_local = (float)(100.0 * (1.0 - idle));
			}
	
			g_coreLoad[i * sample] = load_local;
		}

		sample++;
		if (sample == MAX_SAMPLES)
		{
			sample = 0;
		}
		
		free(l_coreData);
		Sleep(100);
	}
}

static void read_core_count(void)
{
	// read core count
	int size = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * 64;
	int* kernelHandle = malloc(size);
	unsigned long retSize;
	
	NTSTATUS status = query_sys_info_local(SystemProcessorPerformanceInformation,
										   (void*)kernelHandle,
										   (unsigned int)size, 
										   &retSize);
	
	SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* perfInformationPtr = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION*)kernelHandle;
	g_coreCount = (unsigned int)(retSize / sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));
	free(kernelHandle);	
}

void cpu_init(void)
{
	HMODULE handle = LoadLibraryA("ntdll.dll");
	
	if (NULL == handle)
	{
		// handle error: GetLastError()
	}

	query_sys_info_local = (query_sys_info_fptr)GetProcAddress(handle, "NtQuerySystemInformation");

	if (NULL == query_sys_info_local)
	{
		// handle error
	}
	
	read_core_count();

	// allocate data
	g_coreData = malloc(sizeof(long long) * g_coreCount * 2);
	g_coreLoad = malloc(sizeof(float) * g_coreCount * MAX_SAMPLES);
	
	memset(g_coreData, 0, g_coreCount * 2);
	memset(g_coreLoad, 0, g_coreCount * MAX_SAMPLES);
	
	// start thread gathering cpu load data
	HANDLE thread;
    unsigned threadID;

    thread = (HANDLE)_beginthreadex(
        NULL,             // security attributes
        0,                // stack size (0 = default)
        cpu_load_thread,  // thread function
        NULL,             // argument
        0,                // creation flags
        &threadID         // thread ID
    );
}

void cpu_get_core_count(unsigned int* coreCount)
{
	*coreCount = g_coreCount;
}

static void reg_to_string(long *reg, char *str, unsigned int start)
{
	for (int i = 0; i < 4; i++)
	{
		str[start + i] = (char)(*reg>>(i*8));
	}
}

void cpu_read_load(float* load, float* totalLoad)
{
	float sum;
	float total;
	
	for (int i = 0; i < g_coreCount; i++)
	{
		sum = 0.0;
		for (int j = 0; j < MAX_SAMPLES; j++)
		{
			sum += g_coreLoad[i * j];
		}
		load[i] = sum / MAX_SAMPLES;
		total += load[i];
	}

	*totalLoad = total / g_coreCount;
}

// read Processor Brand String
void cpu_read_name(WCHAR* cpuName)
{
	char localName[48];
	long long eax = 0;
	long long ebx = 0;
	long long ecx = 0;
	long long edx = 0;

	readPBS_1(&eax, &ebx, &ecx, &edx);
	
	reg_to_string(&eax, &localName, 0);
	reg_to_string(&ebx, &localName, 4);
	reg_to_string(&ecx, &localName, 8);
	reg_to_string(&edx, &localName, 12);
	
	readPBS_2(&eax, &ebx, &ecx, &edx);
	
	reg_to_string(&eax, &localName, 16);
	reg_to_string(&ebx, &localName, 20);
	reg_to_string(&ecx, &localName, 24);
	reg_to_string(&edx, &localName, 28);
	
	readPBS_3(&eax, &ebx, &ecx, &edx);
	
	reg_to_string(&eax, &localName, 32);
	reg_to_string(&ebx, &localName, 36);
	reg_to_string(&ecx, &localName, 40);
	reg_to_string(&edx, &localName, 44);
	
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, localName, -1, NULL, 0);
	MultiByteToWideChar(CP_UTF8, 0, localName, -1, cpuName, size_needed);
}
