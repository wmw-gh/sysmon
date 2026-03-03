#include <windows.h>
#include <stdbool.h>
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
	log_string(L"Loading PawnIOLib.dll\n");
	handle = LoadLibraryA("./pawnio_manager/PawnIOLib.dll");
	
	if (NULL == handle)
	{
		log_string(L"PawnIOLib.dll not loaded correctly, exiting\n");
		log_string(L"Error code: %u\n", GetLastError());
	}
	else
	{
		log_string(L"PawnIOLib.dll loaded correctly\n");
		log_string(L"handle: %u\n", handle);
	}
	

	/* Resolve function names */
	pawnio_open_local = (pawnio_open_fptr)GetProcAddress(handle, "pawnio_open");

	if (NULL == pawnio_open_local)
	{
		log_string(L"cannot locate pawnio_open\n");
	}
	else
	{
		log_string(L"pawnio_open located correctly\n");
		log_string(L"address: %u\n", pawnio_open_local);
	}
	pawnio_load_local = (pawnio_load_fptr)GetProcAddress(handle, "pawnio_load");

	if (NULL == pawnio_load_local)
	{
		log_string(L"cannot locate pawnio_load\n");
	}
	else
	{
		log_string(L"pawnio_load located correctly\n");
		log_string(L"address: %u\n", pawnio_load_local);
	}

	pawnio_execute_local = (pawnio_execute_fptr)GetProcAddress(handle, "pawnio_execute");

	if (NULL == pawnio_execute_local)
	{
		log_string(L"cannot locate pawnio_execute\n");
	}
	else
	{
		log_string(L"pawnio_execute located correctly\n");
		log_string(L"address: %u\n", pawnio_execute_local);
	}

	pawnio_close_local = (pawnio_close_fptr)GetProcAddress(handle, "pawnio_close");

	if (NULL == pawnio_close_local)
	{
		log_string(L"cannot locate pawnio_close\n");
	}
	else
	{
		log_string(L"pawnio_close located correctly\n");
		log_string(L"address: %u\n", pawnio_close_local);
	}

	/* Load correct blob */
	log_string(L"start of binary: 0x%x\n", &blobs_AMDFamily17_bin[0]);
	log_string(L"loading binary blob of size %u\n", blobs_AMDFamily17_bin_len);

	pawnio_open_local(&pawnio_handle);
	HRESULT result = pawnio_load_local(pawnio_handle, &blobs_AMDFamily17_bin[0], blobs_AMDFamily17_bin_len);

	if (result != S_OK)
	{
		log_string(L"failed to load binary blob, error code: 0x%x\n", result);
	}
	else
	{
		log_string(L"binary blob loaded correctly\n");
	}
}

void pawnio_manager_read_cpu_temp(float* temperature)
{
	#define F17H_TEMP_OFFSET_FLAG 0x80000
	log_string(L"pawnio execute \"ioctl_read_smn\"\n");
	
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
	
	log_string(L"result: 0x%X\n", result);
	log_string(L"ret_size: %u\n", ret_size);
	log_string(L"out_buffer: %u\n", out_buffer[0]);
	
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
	
	log_string(L"temperature: %f\n", t);
}
