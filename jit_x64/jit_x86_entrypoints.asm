; MSFT calling conventions
; Args: RCX, RDX, R8, R9
; RAX, RCX, RDX, R8-11 volatile
; RBX, RBP, RDI, RSI, R12-15 non-volatile

; Register assignments (first 5)
; RCX, RDX, RDI, RSI, R8
; Cpu state = RBX
; Cycle count = RAX
; Temps = R9-12
; Unused = RBP, R13, R14, R15

OFFSET_REGPTR = 0
OFFSET_REGCOUNT = 12
OFFSET_CYCLE = 16
OFFSET_CYCLEEND = 24
OFFSET_CS_BASE = 32
OFFSET_IP_MASK = 36
OFFSET_IP = 40
OFFSET_MODE = 44
OFFSET_DATA_SEGS_PTR = 48
OFFSET_Z = 56
OFFSET_N = 60
OFFSET_I = 64
OFFSET_C = 68
OFFSET_OTHER_FLAGS = 72

extern x64UnjittedShim : proc

.code
x64ReturnToC proc
pop rax
pop rdx
pop rbx
pop r12
pop rsi
pop rdi
ret
x64ReturnToC endp

loadreg macro r
mov rcx, [r]
mov rdx, [r+8]
mov rdi, [r+16]
mov rsi, [r+24]
mov r8, [r+32]
endm

savereg macro r
mov [r], rcx
mov [r+8], rdx
mov [r+16], rdi
mov [r+24], rsi
mov [r+32], r8
endm

x64EnterJitCode proc
; save NV regs that might get clobbered
push rdi
push rsi
push r12
push rbx

push rcx ; save our context pointer
push rdx ; save our cpustate struct

mov rbx, rdx
mov r12, r8

mov byte ptr [rax+12], 0

; load registers
mov rax, [rdx+OFFSET_REGPTR]
loadreg rax

; Jump into the actual code
jmp r12
x64EnterJitCode endp

x64Unjitted proc
; Some instruction has not been jitted!
mov rax, [rbx+OFFSET_REGPTR]
savereg rax
call x64UnjittedShim
jmp x64ReturnToC
x64Unjitted endp

x64NormalJitExit proc
x64NormalJitExit endp


x64InstructionPrologue proc
cmp byte ptr [rbx + OFFSET_I], 2
ja got_interrupt
cmp [rbx + OFFSET_CYCLEEND], rax
ja hit_target
ret

got_interrupt:
hit_target:
x64InstructionPrologue endp

END
