#include <stdio.h>
#include <wchar.h>
#include <stdarg.h>

void log_string(const wchar_t *format, ...)
{
    if (format == NULL)
        return;

    FILE *file = _wfopen(L"log.txt", L"a, ccs=UTF-8");
    if (file == NULL)
        return;

    va_list args;
    va_start(args, format);

    vfwprintf(file, format, args);

    va_end(args);

    fclose(file);
}
