#define _CRT_SECURE_NO_WARNINGS // disable warnings for strncpy

#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <string.h>
#include <re.h>
#include "message.h"

#define BUFF_SIZE 4096
#define LINE_SIZE 96
#define PING_TIMEOUT 1000

static char ipconf_cmd[] = "ipconfig /all";
static char buffer_ipconf[BUFF_SIZE];
static char buffer_ping[BUFF_SIZE];
static DWORD bytes_read_ipconf;
static DWORD bytes_read_ping;
static ULONG rtt;
static void execute_cmd(char* cmd, char* buffer, DWORD* bytes_read);
static void find_line(char* buffer, const char* word, char* line);

unsigned __stdcall ping_thread(void) 
{
    HANDLE hIcmp;
    char sendData[] = "ping";
    DWORD replySize;
    ICMP_ECHO_REPLY *reply;
    IPAddr ipAddr;

    ipAddr = inet_addr("8.8.8.8");
    if (ipAddr == INADDR_NONE) 
	{
        printf("Invalid IP address\n");
        return 1;
    }

    hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) 
	{
        printf("IcmpCreateFile failed\n");
        return 1;
    }

    replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData);
    reply = (ICMP_ECHO_REPLY *)malloc(replySize);

    if (!reply) 
	{
        printf("Memory allocation failed\n");
        IcmpCloseHandle(hIcmp);
        return 1;
    }

    while (1)
	{
        DWORD result = IcmpSendEcho(hIcmp,
									ipAddr,
									sendData,
									sizeof(sendData),
									NULL,
									reply,
									replySize,
									PING_TIMEOUT
        );

        if (result != 0) 
		{
            rtt = reply->RoundTripTime;
        } 
		else 
		{
            rtt = PING_TIMEOUT;
        }

        Sleep(1000);
    }

    free(reply);
    IcmpCloseHandle(hIcmp);
    return 0;
}

void net_init(void)
{
	// read ip configuration
	execute_cmd(&ipconf_cmd[0], &buffer_ipconf[0], &bytes_read_ipconf);
	
	// start thread with ping
	HANDLE thread;
    unsigned threadID;

    thread = (HANDLE)_beginthreadex(
        NULL,             // security attributes
        0,                // stack size (0 = default)
        ping_thread,      // thread function
        NULL,             // argument
        0,                // creation flags
        &threadID         // thread ID
    );
}

extern void	net_read_ip(WCHAR* ip)
{
	const char word[] = "IPv4 Address";
	
	char* line = malloc(sizeof(char)*LINE_SIZE);
	memset(line, ' ', LINE_SIZE);
	line[LINE_SIZE] = '\0';
	
	find_line(&buffer_ipconf[0], &word[0], &line[0]);
	
	re_t pattern = re_compile("\\d+\\.\\d+\\.\\d+\\.\\d+");
    int match_length = 0;
    int match_idx = re_matchp(pattern, line, &match_length);
    if (match_idx != -1)
    {
      debug_print("match at idx %i, %i chars long.\n", match_idx, match_length);
    }

	char* ip_addr = malloc((sizeof(char)*match_length) + 1);
	(void)memset(ip_addr, ' ', match_length + 1);
	ip_addr[match_length] = '\0';
	strncpy(ip_addr, line + match_idx, match_length);

	debug_print("ip address: %s\n", ip_addr);
	
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, ip_addr, -1, NULL, 0);
	WCHAR* wide = malloc(sizeof(WCHAR) * size_needed);
	MultiByteToWideChar(CP_UTF8, 0, ip_addr, -1, wide, size_needed);
	
	wcsncpy(ip, wide, match_length);
	
	free(wide);
	free(ip_addr);
	free(line);
}

extern void	net_read_gateway(WCHAR* gateway)
{
	const char word[] = "Default Gateway";

	char* line = malloc(sizeof(char)*LINE_SIZE);
	memset(line, ' ', LINE_SIZE);
	line[LINE_SIZE] = '\0';
	
	find_line(&buffer_ipconf[0], &word[0], &line[0]);
	
	re_t pattern = re_compile("\\d+\\.\\d+\\.\\d+\\.\\d+");
    int match_length = 0;
    int match_idx = re_matchp(pattern, line, &match_length);
    if (match_idx != -1)
    {
      debug_print("match at idx %i, %i chars long.\n", match_idx, match_length);
    }

	char* ip_addr = malloc((sizeof(char)*match_length) + 1);
	(void)memset(ip_addr, ' ', match_length + 1);
	ip_addr[match_length] = '\0';
	strncpy(ip_addr, line + match_idx, match_length);

	debug_print("ip address: %s\n", ip_addr);
	
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, ip_addr, -1, NULL, 0);
	WCHAR* wide = malloc(sizeof(WCHAR) * size_needed);
	MultiByteToWideChar(CP_UTF8, 0, ip_addr, -1, wide, size_needed);
	
	wcsncpy(gateway, wide, size_needed);
	
	free(wide);
	free(ip_addr);
	free(line);
}

extern void	net_read_dhcp(WCHAR* dhcp)
{
	const char word[] = "DHCP Server";

	char* line = malloc(sizeof(char)*LINE_SIZE);
	memset(line, ' ', LINE_SIZE);
	line[LINE_SIZE] = '\0';
	
	find_line(&buffer_ipconf[0], &word[0], &line[0]);
		
	re_t pattern = re_compile("\\d+\\.\\d+\\.\\d+\\.\\d+");
    int match_length = 0;
    int match_idx = re_matchp(pattern, line, &match_length);
    if (match_idx != -1)
    {
		debug_print("match at idx %i, %i chars long.\n", match_idx, match_length);
    }
	else
	{
		debug_print("no match found\n");
	}

	char* ip_addr = malloc((sizeof(char)*match_length) + 1);
	(void)memset(ip_addr, ' ', match_length + 1);
	ip_addr[match_length] = '\0';
	strncpy(ip_addr, line + match_idx, match_length);

	debug_print("ip address: %s\n", ip_addr);
	
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, ip_addr, -1, NULL, 0);
	WCHAR* wide = malloc(sizeof(WCHAR) * size_needed);
	MultiByteToWideChar(CP_UTF8, 0, ip_addr, -1, wide, size_needed);
	
	wcsncpy(dhcp, wide, match_length);
	
	free(wide);
	free(ip_addr);
	free(line);
}

void net_read_ping(ULONG* ping_ms)
{
	*ping_ms = rtt;
}

// executes cmd, returns buffer with output and number of bytes read
// buffer is fixed to BUFF_SIZE 4096
static void execute_cmd(char* cmd, char* buffer, DWORD* bytes_read)
{
	// create std and err pipes
	SECURITY_ATTRIBUTES sa = {0};
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	
	HANDLE handleOutRead, handleOutWrite;
	HANDLE handleInRead, handleInWrite;
	
	if (CreatePipe(&handleOutRead, &handleOutWrite, &sa, 0))
	{
		debug_print("created std pipe\n");
	}
	else
	{
		debug_print("failed to create std pipe\n");
	}
	
	SetHandleInformation(handleOutRead, HANDLE_FLAG_INHERIT, 0);
	
	if (CreatePipe(&handleInRead, &handleInWrite, &sa, 0))
	{
		debug_print("created err pipe\n");
	}
	else
	{
		debug_print("failed to create err pipe\n");
	}

	SetHandleInformation(handleInRead, HANDLE_FLAG_INHERIT, 0);

	// execute command
	STARTUPINFO info = {0};
	info.cb = sizeof(info);
    info.hStdError = handleOutWrite;
    info.hStdOutput = handleOutWrite;
    info.hStdInput = handleInRead;
	info.dwFlags = STARTF_USESTDHANDLES;
   
    PROCESS_INFORMATION processInfo = {0};
	
	BOOL proc_result = CreateProcess(NULL,          // module name (if null use command line)
									 cmd,           // command to execute
									 NULL,          // process security attributes
									 NULL,          // primary thread security attributes 
									 TRUE,          // handles inherited flag
									 0,             // creation flags 
									 NULL,          // use parent's environment 
									 NULL,     	    // use parent's current directory
									 &info,         // STARTUPINFO pointer
									 &processInfo); // receives PROCESS_INFORMATION 
	if (proc_result)
	{
		/* process created, read from pipes*/
		WaitForSingleObject(processInfo.hProcess, INFINITE);
		
		/* reading command output */
		BOOL result = ReadFile(handleOutRead, // pipe
				               buffer,        // output buffer
				               BUFF_SIZE,     // number of bytes allocated
				               bytes_read,    // number of bytes read
				               NULL);
		
		if (result)
		{
			debug_print("data read correctly from pipe\n");
			debug_print("read %u bytes\n", *bytes_read);
			debug_print("data: \n%s", buffer);
		}
		else
		{
			debug_print("failed to read from pipe, 0x%X\n", GetLastError());
		}
		
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	else
	{
		debug_print("failed to start process, error code %u\n", GetLastError());
	}

	/* handle cleanup */
	CloseHandle(handleOutRead);
	CloseHandle(handleOutWrite);
	CloseHandle(handleInRead);
	CloseHandle(handleInWrite);
}

// finds line containing word in buffer
// end of line is recognized as '\n'
// line should be buffer with lenght of 96 (Windows max line length is 8191, prolly too much in this case)
// function assumes line argument is empty and properly terminated
static void find_line(char* buffer, const char* word, char* line)
{
	int word_start = 0;
	int word_end = 0;
	
	char *start = strstr(buffer, word);
	if (NULL == start)
	{
		debug_print("did not find string %s\n", word);
	}
	else
	{
		word_start = (int)(start - buffer);
		debug_print("beginning of string %u\n", buffer);
		debug_print("symbol at %u\n", start);
		debug_print("found word %s at position %u\n", word, word_start);
	}
	
	char *end = strchr(buffer+word_start, '\n');
	if (NULL == end)
	{
		debug_print("did not find newline\n");
	}
	else
	{
		word_end = (int)(end - buffer);
		debug_print("beginning of string %u\n", buffer);
		debug_print("symbol at %u\n", end);
		debug_print("found newline at position %u\n", word_end);
	}
	
	int length = word_end - word_start;
	debug_print("length of string to be copied %u\n", length);
	strncpy(line, buffer+word_start, length);
	debug_print("full string with IP address: %s\n", line);
}
