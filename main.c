#define _CRT_SECURE_NO_WARNINGS // disable warnings for itoa
#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <windows.h>
#include <math.h>
#include "gpu.h"
#include "ram.h"
#include "net.h"
#include "cpu.h"
#include "pawnio_manager.h"
#include <dxgi.h>

#define FIXED_WIDTH  48

typedef struct
{
	int CoreCount;
	WCHAR Vendor[48];
	float Temp;
	float Load[12];
	float TotalLoad;
} CpuInfo_t;

typedef struct
{
	WCHAR Name[128];
	unsigned int Temp;
	double Load;
	WCHAR LibName[8];
	bool Integrated;
} GpuInfo_t;

typedef struct
{
	WCHAR Ip[48];
	WCHAR Gateway[48];
	WCHAR Dhcp[48];
	ULONG ping_ms;
} NetInfo_t;

typedef struct
{
	LONGLONG TotalMb;
	int Load;
} RamInfo_t;

typedef struct
{
	unsigned int Refresh_ms;
	CpuInfo_t Cpu;
	GpuInfo_t Gpu;
	NetInfo_t Network;
	RamInfo_t Ram;
} SystemInfo_t;

typedef struct
{
    HANDLE hConsole;
    int width;
    int height;
    CHAR_INFO* buffer;
} Console_t;

static SystemInfo_t SysInfo;
static time_t TimeOld;
static WCHAR line_buffer[FIXED_WIDTH];
static Console_t Console = {0};

static void console_get_size(int* w, int* h)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(Console.hConsole, &info);

    *w = info.srWindow.Right  - info.srWindow.Left + 1;
    *h = info.srWindow.Bottom - info.srWindow.Top  + 1;
}

static void resize_buffer(int new_w, int new_h)
{
    if (new_w <= 0 || new_h <= 0)
        return;

    if (new_w == Console.width && new_h == Console.height)
        return;

    free(Console.buffer);

    Console.width  = new_w;
    Console.height = new_h;

    Console.buffer = (CHAR_INFO*)malloc(sizeof(CHAR_INFO) * new_w * new_h);
    if (!Console.buffer)
    {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
}

static void console_init(void)
{
    Console.hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    int w, h;
    console_get_size(&w, &h);
    resize_buffer(w, h);
	
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(Console.hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(Console.hConsole, &cursorInfo);
}

static void clear_buffer(WORD attributes)
{
    int size = Console.width * Console.height;

    for (int i = 0; i < size; ++i)
    {
        Console.buffer[i].Char.UnicodeChar = L' ';
        Console.buffer[i].Attributes = attributes;
    }
}

static void draw_text(int x, int y, const wchar_t* text, WORD attr)
{
    if (y < 0 || y >= Console.height)
        return;

    for (int i = 0; text[i] != 0; ++i)
    {
        int px = x + i;
        if (px < 0 || px >= Console.width)
            continue;

        int index = y * Console.width + px;
        Console.buffer[index].Char.UnicodeChar = text[i];
        Console.buffer[index].Attributes = attr;
    }
}

static void present(void)
{
    SMALL_RECT rect = {0, 0, Console.width - 1, Console.height - 1};

    COORD bufferSize = { (SHORT)Console.width, (SHORT)Console.height };
    COORD bufferCoord = {0, 0};

    WriteConsoleOutputW(
        Console.hConsole,
        Console.buffer,
        bufferSize,
        bufferCoord,
        &rect
    );
}

static void update_buffer(void)
{
	int line = 0;
	int w, h;
	
	console_get_size(&w, &h);
	resize_buffer(w, h);		
	clear_buffer(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	
	swprintf(line_buffer, FIXED_WIDTH, L"SysMon v1.0");
	draw_text(0, line, line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"console width: %u console height: %u", Console.width, Console.height);
	draw_text(0, line, line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"CPU: %s", SysInfo.Cpu.Vendor);
	draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"temperature: %2.2f \u00B0C", SysInfo.Cpu.Temp);
	draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"load total: %3.1f %%", SysInfo.Cpu.TotalLoad);
	draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	
	for (int i = 0; i < SysInfo.Cpu.CoreCount; i++)
	{
		swprintf(line_buffer, FIXED_WIDTH, L"core %u load: %3.1f %%", i+1, SysInfo.Cpu.Load[i]);
		draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		line++;
	}
	
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"GPU: %s", SysInfo.Gpu.Name);
	draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"library: %s", SysInfo.Gpu.LibName);
	draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"integrated: %s", SysInfo.Gpu.Integrated ? L"true" : L"false");
	draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"temperature: %u \u00B0C", SysInfo.Gpu.Temp);
	draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"load: %1.0f %%", SysInfo.Gpu.Load);
	draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"RAM total: %llu Mb", SysInfo.Ram.TotalMb);
	draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"used: %u %%", SysInfo.Ram.Load);
	draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"Network:");
	draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"local IP address: %s", SysInfo.Network.Ip);
	draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"default gateway: %s", SysInfo.Network.Gateway);
	draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"DHCP server: %s", SysInfo.Network.Dhcp);
	draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
	
	swprintf(line_buffer, FIXED_WIDTH, L"ping status: %lu ms", SysInfo.Network.ping_ms);
	draw_text(0, line, &line_buffer, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	line++;
}
int main(void)
{	
	// module initialization
	pawnio_manager_init();
	gpu_init();	
	net_init();
	cpu_init();
	
	// CPU
	cpu_read_name(&(SysInfo.Cpu.Vendor[0]));
	cpu_get_core_count(&(SysInfo.Cpu.CoreCount));
	cpu_read_load(&(SysInfo.Cpu.Load[0]), &(SysInfo.Cpu.TotalLoad));
	
	// GPU
	gpu_get_lib_version(&SysInfo.Gpu.LibName);
	gpu_get_name(&SysInfo.Gpu.Name);
	SysInfo.Gpu.Integrated = is_gpu_integrated();

	// RAM 
	ram_get_total(&(SysInfo.Ram.TotalMb));
	
	// network 
	net_read_ip(&(SysInfo.Network.Ip[0]));
	net_read_gateway(&(SysInfo.Network.Gateway[0]));
	net_read_dhcp(&(SysInfo.Network.Dhcp[0]));
	
	console_init();
	
	for (;;)
	{
		net_read_ping(&(SysInfo.Network.ping_ms));
		cpu_read_load(&(SysInfo.Cpu.Load[0]), &(SysInfo.Cpu.TotalLoad));
	
		time_t time_new = time(NULL);
		if (difftime(time_new, TimeOld) >= 1.0)
		{
			pawnio_manager_read_cpu_temp(&(SysInfo.Cpu.Temp));
			ram_get_load(&(SysInfo.Ram.Load));
		
			if (SysInfo.Gpu.Integrated)
			{
				SysInfo.Gpu.Temp = (unsigned int)round(SysInfo.Cpu.Temp);
			}
			else
			{
				gpu_get_temperature(&(SysInfo.Gpu.Temp));
			}
		
			gpu_get_load(&(SysInfo.Gpu.Load));
			
			update_buffer();
			present();
		}
		
		TimeOld = time_new;
		Sleep(100);
	}
	
	return 0;
}
