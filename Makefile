src = $(wildcard *.c) $(wildcard */*.c)
inc = $(dir $(src))
dep = $(addprefix /I,$(inc))

abs = $(notdir $(src))
obj = $(abs:.c=.obj) $(abs */*.c)

CFLAGS = /W4 /c /nologo 
DFLAGS = /Zi /Od /MDd /FS

sysmon : $(obj) assembly
	cl $(dep) $(obj) assembly.obj /link gpu/nvapi64.lib iphlpapi.lib ws2_32.lib /out:sysmon.exe
	
assembly :
	ML64 assembly.asm /c

main.obj: main.c
	cl $(CFLAGS) $(dep) main.c
	
AMDFamily0F.obj: blobs/AMDFamily0F.c
	cl $(CFLAGS) $(dep) blobs/AMDFamily0F.c

AMDFamily10.obj: blobs/AMDFamily10.c
	cl $(CFLAGS) $(dep) blobs/AMDFamily10.c

AMDFamily17.obj: blobs/AMDFamily17.c
	cl $(CFLAGS) $(dep) blobs/AMDFamily17.c
	
cpu.obj: cpu/cpu.c
	cl $(CFLAGS) $(dep) cpu/cpu.c
	
message.obj: debug/message.c
	cl $(CFLAGS) $(dep) debug/message.c

gpu.obj: gpu/gpu.c
	cl $(CFLAGS) $(dep) gpu/gpu.c

net.obj: net/net.c
	cl $(CFLAGS) $(dep) net/net.c
	
pawnio_manager.obj: pawnio_manager/pawnio_manager.c
	cl $(CFLAGS) $(dep) pawnio_manager/pawnio_manager.c

ram.obj: ram/ram.c
	cl $(CFLAGS) $(dep) ram/ram.c

re.obj : re/re.c
	cl $(CFLAGS) $(dep) re/re.c

.PHONY: clean binary
clean:
	del sysmon.exe $(obj) assembly.obj

binary:
	xxd -i blobs/AMDFamily0F.bin > blobs/AMDFamily0F.c
	xxd -i blobs/AMDFamily10.bin > blobs/AMDFamily10.c
	xxd -i blobs/AMDFamily17.bin > blobs/AMDFamily17.c
	
# unused assembly options
# /subsystem:windows /defaultlib:kernel32.lib /defaultlib:user32.lib