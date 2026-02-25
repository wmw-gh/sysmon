#pragma once

extern void gpu_init(void);
extern void gpu_get_name(WCHAR* name);
extern void gpu_get_temperature(unsigned int* temp);
extern void gpu_get_load(unsigned int* load);
extern void gpu_get_lib_version(WCHAR* libName);
extern bool is_gpu_integrated(void);
