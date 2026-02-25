#include <windows.h>
#include "pawnio_manager.h"
#include "message.h"
#include "PawnIOLib.h"
#include "embedded_data.h"

typedef HRESULT (*pawnio_open_fptr)(PHANDLE handle);
typedef HRESULT (*pawnio_load_fptr)(HANDLE handle, const UCHAR* blob, SIZE_T size);
typedef HRESULT (*pawnio_execute_fptr)(
  HANDLE handle,
  PCSTR name,
  const ULONG64* in,
  SIZE_T in_size,
  PULONG64 out,
  SIZE_T out_size,
  PSIZE_T return_size
);
typedef HRESULT (*pawnio_close_fptr)(HANDLE handle);

static HMODULE handle;
static HANDLE pawnio_handle;
static pawnio_open_fptr pawnio_open_local;
static pawnio_load_fptr pawnio_load_local;
static pawnio_execute_fptr pawnio_execute_local;
static pawnio_close_fptr pawnio_close_local;

void pawnio_manager_init(void)
{	
	/* Load .dll */
	debug_print("Loading PawnIOLib.dll\n");
	handle = LoadLibraryA("./pawnio_manager/PawnIOLib.dll");
	
	if (NULL == handle)
	{
		debug_print("PawnIOLib.dll not loaded correctly, exiting\n");
		debug_print("Error code: %u\n", GetLastError());
	}
	else
	{
		debug_print("PawnIOLib.dll loaded correctly\n");
		debug_print("handle: %u\n", handle);
	}
	

	/* Resolve function names */
	pawnio_open_local = (pawnio_open_fptr)GetProcAddress(handle, "pawnio_open");

	if (NULL == pawnio_open_local)
	{
		debug_print("cannot locate pawnio_open\n");
	}
	else
	{
		debug_print("pawnio_open located correctly\n");
		debug_print("address: %u\n", pawnio_open_local);
	}
	pawnio_load_local = (pawnio_load_fptr)GetProcAddress(handle, "pawnio_load");

	if (NULL == pawnio_load_local)
	{
		debug_print("cannot locate pawnio_load\n");
	}
	else
	{
		debug_print("pawnio_load located correctly\n");
		debug_print("address: %u\n", pawnio_load_local);
	}

	pawnio_execute_local = (pawnio_execute_fptr)GetProcAddress(handle, "pawnio_execute");

	if (NULL == pawnio_execute_local)
	{
		debug_print("cannot locate pawnio_execute\n");
	}
	else
	{
		debug_print("pawnio_execute located correctly\n");
		debug_print("address: %u\n", pawnio_execute_local);
	}

	pawnio_close_local = (pawnio_close_fptr)GetProcAddress(handle, "pawnio_close");

	if (NULL == pawnio_close_local)
	{
		debug_print("cannot locate pawnio_close\n");
	}
	else
	{
		debug_print("pawnio_close located correctly\n");
		debug_print("address: %u\n", pawnio_close_local);
	}

	/* Load correct blob */

	debug_print("start of binary: 0x%x\n", &embedded_data[0]);
	debug_print("loading binary blob of size %u\n", EMBEDDED_BLOB_SIZE);

	pawnio_open_local(&pawnio_handle);
	HRESULT result = pawnio_load_local(pawnio_handle, &embedded_data[0], EMBEDDED_BLOB_SIZE);

	if (result != S_OK)
	{
		debug_print("failed to load binary blob, error code: 0x%x\n", result);
	}
	else
	{
		debug_print("binary blob loaded correctly\n");
	}
}

void pawnio_manager_read_cpu_temp(float* temperature)
{
	#define F17H_TEMP_OFFSET_FLAG 0x80000
	debug_print("pawnio execute \"ioctl_read_smn\"\n");
	
	bool offset_flag = false;
	SIZE_T ret_size = 0;
	const ULONG64 F17H_M01H_THM_TCON_CUR_TMP = 0x00059800;
	#define OUT_SIZE 1
	ULONG64 out_buffer[OUT_SIZE] = { 0 };
	HRESULT result = pawnio_execute_local(pawnio_handle,               // handle from pawnio_open                         [OK]
										  "ioctl_read_smn",            // function name to execute                        [OK]
										  &F17H_M01H_THM_TCON_CUR_TMP, // const ULONG64* in input buffer                  [OK]
										  1,                           // SIZE_T in_size input buffer count               [OK]
										  &out_buffer[0],              // PULONG64 out output buffer                      [OK]
										  OUT_SIZE,				       // SIZE_T out_size output buffer count             [OK]
										  &ret_size);                  // PSIZE_T return_size Entries written in out_size [OK]
	
	debug_print("result: 0x%X\n", result);
	debug_print("ret_size: %u\n", ret_size);
	debug_print("out_buffer: %u\n", out_buffer[0]);
	
	unsigned int temp = (unsigned int)out_buffer[0];
	
	// check for temperature offset
	if (((unsigned int)temperature & F17H_TEMP_OFFSET_FLAG) != 0)
	{
		offset_flag = true;
	}
	
	temp = (temp >> 21) * 125;
	float t = temp * 0.001f;
	
	t += -49.0f;

	*temperature = t;
	
	debug_print("temperature: %f\n", t);
}
