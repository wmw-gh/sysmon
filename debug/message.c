#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <stdarg.h>

#define LOG_FILE      L"log.txt"
#define LOG_FILE_OLD  L"log.old"
#define MAX_LOG_SIZE (1024 * 16)  // 16 kB

static void check_size(void)
{
	WIN32_FILE_ATTRIBUTE_DATA info;

    if (GetFileAttributesExW(LOG_FILE, GetFileExInfoStandard, &info))
    {
        LARGE_INTEGER size;
        size.HighPart = info.nFileSizeHigh;
        size.LowPart  = info.nFileSizeLow;

        if (size.QuadPart >= MAX_LOG_SIZE)
        {
            DeleteFileW(LOG_FILE_OLD);          // remove old backup
            MoveFileW(LOG_FILE, LOG_FILE_OLD);  // rename current to old
        }
    }
}

void log_string(const wchar_t *format, ...)
{
	check_size();
	
    if (format == NULL)
        return;

    FILE *file = _wfopen(LOG_FILE, L"a, ccs=UTF-8");
    if (file == NULL)
        return;

    va_list args;
    va_start(args, format);

    vfwprintf(file, format, args);

    va_end(args);

    fclose(file);
}
