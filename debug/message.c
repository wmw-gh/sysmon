#include <stdio.h>
#include <wchar.h>
#include <stdarg.h>
#include <locale.h>

void log_string(const wchar_t *format, ...)
{
    if (format == NULL)
        return;

    setlocale(LC_ALL, "");

    FILE *file = _wfopen(L"log.txt", L"a, ccs=UTF-8");
    if (file == NULL)
        return;

    va_list args;
    va_start(args, format);

    vfwprintf(file, format, args);
    fwprintf(file, L"\n");

    va_end(args);

    fclose(file);
}
