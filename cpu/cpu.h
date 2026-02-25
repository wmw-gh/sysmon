#pragma once

extern void cpu_init(void);
extern void cpu_read_name(WCHAR* name);
extern void cpu_read_load(float* load, float* totalLoad);
extern void cpu_get_core_count(unsigned int* coreCount);
