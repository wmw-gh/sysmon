#include "message.h"
#include <windows.h>

void ram_get_total(LONGLONG* ramMb)
{
	LONGLONG totalRam = 0;
	if (GetPhysicallyInstalledSystemMemory((PULONGLONG)&totalRam))
	{
		debug_print("ram read ok, total ram in:\n");
		debug_print("Kb: %u\n", totalRam);
		debug_print("Mb: %u\n", totalRam/1024);
		debug_print("Gb: %f\n", (double)totalRam/(1024*1024));
	}
	else
	{
		debug_print("ram info not available\n");
	}
	
	*ramMb = (totalRam/1024);
}

void ram_get_load(int* ramLoad)
{
	MEMORYSTATUSEX memStat;
	memStat.dwLength = sizeof(memStat);
	
	if (GlobalMemoryStatusEx(&memStat))
	{
		debug_print("ram usage info read ok\n");
		debug_print("dwMemoryLoad: %u\n", memStat.dwMemoryLoad);
	}
	else
	{
		debug_print("ram usage info not available\n");
	}
	
	*ramLoad = memStat.dwMemoryLoad;
}
