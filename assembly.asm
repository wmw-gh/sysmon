.code

readPBS_1 proc
	push r9
	push r8
	push rdx
	push rcx
	mov eax, 80000002h
	cpuid
	pop r10
	mov qword ptr [r10], rax
	pop r10
	mov qword ptr [r10], rbx
	pop r10
	mov qword ptr [r10], rcx
	pop r10
	mov qword ptr [r10], rdx
	ret
readPBS_1 endp

readPBS_2 proc
	push r9
	push r8
	push rdx
	push rcx
	mov eax, 80000003h
	cpuid
	pop r10
	mov qword ptr [r10], rax
	pop r10
	mov qword ptr [r10], rbx
	pop r10
	mov qword ptr [r10], rcx
	pop r10
	mov qword ptr [r10], rdx
	ret
readPBS_2 endp

readPBS_3 proc
	push r9
	push r8
	push rdx
	push rcx
	mov eax, 80000004h
	cpuid
	pop r10
	mov qword ptr [r10], rax
	pop r10
	mov qword ptr [r10], rbx
	pop r10
	mov qword ptr [r10], rcx
	pop r10
	mov qword ptr [r10], rdx
	ret
readPBS_3 endp
end

; notes:
; arguments for x64 assembly are located in: RCX, RDX, R8, and R9

; for converting to 64-bit, move value to eax as this will zero-extend value of edi to rax
; mov eax, edi