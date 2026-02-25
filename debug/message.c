#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

static bool debug = false;

void debug_print(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	
	if (debug)
	{
		printf(msg);
	}
}

void debug_set(bool debugSet)
{
	debug = debugSet;
}