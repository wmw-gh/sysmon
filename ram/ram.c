#include "message.h"
#include <windows.h>

void ram_get_total(LONGLONG* ramMb)
{
	LONGLONG totalRam = 0;
	if (GetPhysicallyInstalledSystemMemory((PULONGLONG)&totalRam))
	{
		log_string(L"ram read ok, total ram in:\n");
		log_string(L"Kb: %u\n", totalRam);
		log_string(L"Mb: %u\n", totalRam/1024);
		log_string(L"Gb: %f\n", (double)totalRam/(1024*1024));
	}
	else
	{
		log_string(L"ram info not available\n");
	}
	
	*ramMb = (totalRam/1024);
}

void ram_get_load(int* ramLoad)
{
	MEMORYSTATUSEX memStat;
	memStat.dwLength = sizeof(memStat);
	
	if (GlobalMemoryStatusEx(&memStat))
	{
		log_string(L"ram usage info read ok\n");
		log_string(L"dwMemoryLoad: %u\n", memStat.dwMemoryLoad);
	}
	else
	{
		log_string(L"ram usage info not available\n");
	}
	
	*ramLoad = memStat.dwMemoryLoad;
}
