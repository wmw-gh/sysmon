#define _CRT_SECURE_NO_WARNINGS // disable warnings for strncpy
#define COBJMACROS              // enable access to C++ object fields via functions in plain C

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <dxgi.h>
#include <pdh.h>
#include "re.h"
#include "nvapi.h"
#include "message.h"

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "ole32.lib")

#define delay_ms 250
#define NVD L"NVIDIA"
#define WIN L"Windows"
#define INTEGRATED L"AMD Radeon(TM) Graphics"
typedef enum
{
	WINDOWS,
	NVIDIA
} lib_t;

static NvPhysicalGpuHandle gpuHandles[NVAPI_MAX_PHYSICAL_GPUS];
static NvU32 gpuCount;
static WCHAR lib_name[7];
static WCHAR gpuName[128];
static lib_t lib_type;
static bool integrated;

static void detect_gpu(WCHAR* name);
static void nvidia_lib_init(void);

void gpu_init(void)
{
	detect_gpu(gpuName);

	// check if detected gpu is AMD or NVIDIA
	re_t pattern = re_compile(NVD);
	int match_length = 0;
    int match_idx = re_matchp(pattern, gpuName, &match_length);
	
    if (match_idx != -1)
    {
		wcscpy(lib_name, NVD);
		lib_type = NVIDIA;
		nvidia_lib_init();
    }
	else
	{
		wcscpy(lib_name, WIN);
		lib_type = WINDOWS;
		
		// check if GPU is integrated or not
		pattern  = re_compile(INTEGRATED);
		match_idx = re_matchp(pattern, gpuName, &match_length);
		
		if (match_idx != -1)
		{
			integrated = true;
		}
	}
}

void gpu_get_name(WCHAR* name)
{
	wcscpy(name, gpuName);
}

void gpu_get_temperature(unsigned int* temp)
{
	*temp = 0;
	
	if (WINDOWS == lib_type)
	{
		PDH_HQUERY query;
		PDH_HCOUNTER counter;
		PDH_FMT_COUNTERVALUE value;
		PDH_STATUS status;
		double tempC;
	
		status = PdhOpenQuery(NULL, 0, &query);
		if (status != ERROR_SUCCESS) 
		{
			log_string(L"PdhOpenQuery failed\n");
			return;
		}
		
		status = PdhAddEnglishCounter(query,
									  L"\\GPU Thermal Zone Information(*)\\Temperature",
									  0,
									  &counter);
		
		if (status != ERROR_SUCCESS) 
		{
			log_string(L"PdhAddEnglishCounter failed (0x%x)\n", status);
			PdhCloseQuery(query);
			return;
		}
		
		PdhCollectQueryData(query);
		Sleep(delay_ms);
		PdhCollectQueryData(query);
		
		status = PdhGetFormattedCounterValue(counter,
											 PDH_FMT_DOUBLE,
											 NULL,
											 &value);
		
		if (status == ERROR_SUCCESS) 
		{
			// Convert from Kelvin to Celsius
			tempC = (value.doubleValue / 10.0) - 273.15;
			log_string(L"GPU Temperature: %.1f °C\n", tempC);
		}
		else
		{
			log_string(L"PdhGetFormattedCounterValue failed\n");
		}
		
		*temp = (unsigned int)round(tempC);
	}
	else
	{
		NvU32 sensorIndex = 0;
		log_string(L"Thermal settings version %u\n", NV_GPU_THERMAL_SETTINGS_VER);
		log_string(L"Number of thermal settings %u\n", NVAPI_MAX_THERMAL_SENSORS_PER_GPU - 1);
		
		NV_GPU_THERMAL_SETTINGS thermalSettings;
		
		thermalSettings.version = NV_GPU_THERMAL_SETTINGS_VER;
		NvAPI_Status ret = NvAPI_GPU_GetThermalSettings(gpuHandles[0], sensorIndex, &thermalSettings);
		
		if (NVAPI_OK == ret)
		{
			log_string(L"Thermal settings of GPU read OK\n");
			log_string(L"Current temperature %u%cC\n", thermalSettings.sensor->currentTemp, 167U);
		}
		else
		{
			log_string(L"Unable to read thermal settings, error: %i\n", ret);
		}
		
		*temp = thermalSettings.sensor->currentTemp;
	}
}

void gpu_get_load(double* load)
{
	if (WINDOWS == lib_type)
	{
		PDH_HQUERY query;
		PDH_HCOUNTER counter;
		DWORD bufSize = 0;
		DWORD itemCount = 0;
		double totalLoad = 0.0;
		
		if (PdhOpenQuery(NULL, 0, &query) != ERROR_SUCCESS)
		{
			log_string(L"PdhOpenQuery failed");
		}
		
		if (PdhAddEnglishCounter(query, "\\GPU Engine(*engtype_3D)\\Utilization Percentage", 0, &counter) != ERROR_SUCCESS)
		{
			log_string(L"PdhAddEnglishCounter failed");
		}
		
		PdhCollectQueryData(query);
		Sleep(delay_ms);
		PdhCollectQueryData(query);
		
		PdhGetFormattedCounterArray(counter, PDH_FMT_DOUBLE, &bufSize, &itemCount, NULL);
		
		PDH_FMT_COUNTERVALUE_ITEM *items = (PDH_FMT_COUNTERVALUE_ITEM*)malloc(bufSize);
		
		if (!items)
		{
			log_string(L"Out of memory");
		}
		
		if (ERROR_SUCCESS != PdhGetFormattedCounterArray(counter, PDH_FMT_DOUBLE, &bufSize, &itemCount, items))
		{
			log_string(L"PdhGetFormattedCounterArray failed");
		}
		
		for (DWORD i = 0; i < itemCount; i++) 
		{
			if (items[i].FmtValue.CStatus == ERROR_SUCCESS)
			{
				totalLoad += items[i].FmtValue.doubleValue;
			}
		}
		
		log_string(L"iGPU 3D Load: %3.1f\n", totalLoad);
		*load = totalLoad;
		free(items);
		PdhCloseQuery(query);
	}
	else
	{
		log_string(L"Pstates settings version %u\n", NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER);
	
		NV_GPU_PERF_PSTATE_ID currentPstate;
		NvAPI_Status ret = NvAPI_GPU_GetCurrentPstate(gpuHandles[0], &currentPstate);
		
		if (NVAPI_OK == ret)
		{
			log_string(L"GPU Pstates read OK\n");
			log_string(L"GPU is in Pstate: %u\n", currentPstate);
		}
		else
		{
			log_string(L"Unable to read Pstates, error: %i\n", ret);
		}
		
		NV_GPU_DYNAMIC_PSTATES_INFO_EX dynamicPstates;
		dynamicPstates.version = NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER;
		ret = NvAPI_GPU_GetDynamicPstatesInfoEx(gpuHandles[0], &dynamicPstates);
		if (NVAPI_OK == ret)
		{
			log_string(L"GPU extended Pstates read OK\n");
			log_string(L"GPU load is: %u\n", dynamicPstates.utilization[0].percentage);
		}
		else
		{
			log_string(L"Unable to read extended Pstates, error: %i\n", ret);
		}
		
		*load = dynamicPstates.utilization[0].percentage;
	}
}

void gpu_get_lib_version(WCHAR* libName)
{
	wcscpy(libName, &lib_name);
}

static void detect_gpu(WCHAR* name)
{
	HRESULT result;
    IDXGIFactory *pFactory = NULL;
    IDXGIAdapter *pAdapter = NULL;
    UINT i = 0;

    // Initialize COM
    result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	
    if (FAILED(result))
    {
        log_string(L"CoInitializeEx failed: 0x%08X\n", result);
        return;
    }

    // Create DXGI Factory
    result = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&pFactory);
    if (FAILED(result))
    {
        log_string(L"CreateDXGIFactory failed: 0x%08X\n", result);
        CoUninitialize();
        return;
    }

    log_string(L"Enumerating GPU adapters:\n");

    // Enumerate adapters
    while (IDXGIFactory_EnumAdapters(pFactory, i, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
		
        ZeroMemory(&desc, sizeof(desc));

        result = IDXGIAdapter_GetDesc(pAdapter, &desc);
		
        if (SUCCEEDED(result))
        {
            //wprintf(L"Adapter %u:\n", i);
            //wprintf(L"Name: %s\n", desc.Description);
        }
		
		if (0 == i)
		{
			wcscpy(name, &(desc.Description));
		}

        IDXGIAdapter_Release(pAdapter);
        pAdapter = NULL;
        i++;
    }

    IDXGIFactory_Release(pFactory);
    CoUninitialize();
}

static void nvidia_lib_init(void)
{
	NvAPI_Status      status  = NVAPI_OK;
    NvAPI_ShortString szError = { 0 };

    status = NvAPI_Initialize();
    if (status != NVAPI_OK)
    {
        NvAPI_GetErrorMessage(status, szError);
        log_string(L"NvAPI_Initialize() failed. Error code is: %s", szError);
    }
	
	if (NVAPI_INVALID_ARGUMENT == NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount))
	{
		log_string(L"Unable to enumerate GPUs available on system\n");
	}
	else
	{
		log_string(L"Detected %u GPUs\n", gpuCount);
		log_string(L"Handle of first GPU: %u\n", (unsigned int)gpuHandles[0]);
	}
}

bool is_gpu_integrated(void)
{
	return integrated;
}
