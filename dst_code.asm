extern scanf
extern printf

section .data

scanf_str db "%lf", 0
printf_str db "%lf", 10, 0

section .text

global main

main:
mov rbp, rsp
sub rsp, 8  ; выравнивание на 16

lea rdi, scanf_str  ; in
sub rsp, 16
mov rsi, rsp
call scanf

lea rdi, scanf_str  ; in
sub rsp, 16
mov rsi, rsp
call scanf

lea rdi, scanf_str  ; in
sub rsp, 16
mov rsi, rsp
call scanf

vmovq xmm2, qword [rsp]  ; pop reg
add rsp, 16

vmovq xmm1, qword [rsp]  ; pop reg
add rsp, 16

vmovq xmm0, qword [rsp]  ; pop reg
add rsp, 16

sub rsp, 8
call L33  ; call
add rsp, 8

mov rax, __float64__(0.000000)  ; push imm
vmovq xmm5, rax
vmovq qword [rsp - 16], xmm5
sub rsp, 16
xor rax, rax

movq qword [rsp - 16], xmm3  ; push reg
sub rsp, 16

vmovq xmm4, qword [rsp]  ; je
vmovq xmm5, qword [rsp + 16]
add rsp, 32

vcmppd xmm5, xmm4, xmm5, 0x0
movmskpd r14d, xmm5
cmp r14d, 0x3
je L50

mov rax, __float64__(0.000000)  ; push imm
vmovq xmm5, rax
vmovq qword [rsp - 16], xmm5
sub rsp, 16
xor rax, rax

movq qword [rsp - 16], xmm3  ; push reg
sub rsp, 16

vmovq xmm4, qword [rsp]  ; ja
vmovq xmm5, qword [rsp + 16]
add rsp, 32

vcmppd xmm5, xmm4, xmm5, 0x1E
movmskpd r14d, xmm5
cmp r14d, 0x1
je L67

mov rax, __float64__(0.000000)  ; push imm
vmovq xmm5, rax
vmovq qword [rsp - 16], xmm5
sub rsp, 16
xor rax, rax

movq qword [rsp - 16], xmm3  ; push reg
sub rsp, 16

vmovq xmm4, qword [rsp]  ; jb
vmovq xmm5, qword [rsp + 16]
add rsp, 32

vcmppd xmm5, xmm4, xmm5, 0x11
movmskpd r14d, xmm5
cmp r14d, 0x1
je L100

mov rax, __float64__(888.000000)  ; push imm
vmovq xmm5, rax
vmovq qword [rsp - 16], xmm5
sub rsp, 16
xor rax, rax

lea rdi, printf_str  ; out
mov rax, 1
vmovq qword [rsp - 8], xmm0
sub rsp, 8
movq xmm0, qword [rsp + 8]
vmovq qword [rsp - 8], xmm1
sub rsp, 8
vmovq qword [rsp - 8], xmm2
sub rsp, 8
vmovq qword [rsp - 8], xmm3
sub rsp, 8
vmovq qword [rsp - 8], xmm4
sub rsp, 8

sub rsp, 8
call printf
add rsp, 8

vmovq xmm4, qword [rsp]
add rsp, 8
vmovq xmm3, qword [rsp]
add rsp, 8
vmovq xmm2, qword [rsp]
add rsp, 8
vmovq xmm1, qword [rsp]
add rsp, 8
vmovq xmm0, qword [rsp]
add rsp, 8
add rsp, 16

mov rsp, rbp  ; hlt
jmp END

L33:
movq qword [rsp - 16], xmm1  ; push reg
sub rsp, 16

movq qword [rsp - 16], xmm1  ; push reg
sub rsp, 16

vmovq xmm5, qword [rsp + 16]  ; arithmetic
mulsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

mov rax, __float64__(4.000000)  ; push imm
vmovq xmm5, rax
vmovq qword [rsp - 16], xmm5
sub rsp, 16
xor rax, rax

movq qword [rsp - 16], xmm0  ; push reg
sub rsp, 16

vmovq xmm5, qword [rsp + 16]  ; arithmetic
mulsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

movq qword [rsp - 16], xmm2  ; push reg
sub rsp, 16

vmovq xmm5, qword [rsp + 16]  ; arithmetic
mulsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

vmovq xmm5, qword [rsp + 16]  ; arithmetic
subsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

vmovq xmm3, qword [rsp]  ; pop reg
add rsp, 16

ret

L50:
movq qword [rsp - 16], xmm1  ; push reg
sub rsp, 16

mov rax, __float64__(-1.000000)  ; push imm
vmovq xmm5, rax
vmovq qword [rsp - 16], xmm5
sub rsp, 16
xor rax, rax

vmovq xmm5, qword [rsp + 16]  ; arithmetic
mulsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

movq qword [rsp - 16], xmm3  ; push reg
sub rsp, 16

vmovq xmm4, qword [rsp]  ; sqrt
vsqrtpd xmm4, xmm4
vmovq qword [rsp], xmm4

vmovq xmm5, qword [rsp + 16]  ; arithmetic
addsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

mov rax, __float64__(2.000000)  ; push imm
vmovq xmm5, rax
vmovq qword [rsp - 16], xmm5
sub rsp, 16
xor rax, rax

movq qword [rsp - 16], xmm0  ; push reg
sub rsp, 16

vmovq xmm5, qword [rsp + 16]  ; arithmetic
mulsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

vmovq xmm5, qword [rsp + 16]  ; arithmetic
divsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

lea rdi, printf_str  ; out
mov rax, 1
vmovq qword [rsp - 8], xmm0
sub rsp, 8
movq xmm0, qword [rsp + 8]
vmovq qword [rsp - 8], xmm1
sub rsp, 8
vmovq qword [rsp - 8], xmm2
sub rsp, 8
vmovq qword [rsp - 8], xmm3
sub rsp, 8
vmovq qword [rsp - 8], xmm4
sub rsp, 8

sub rsp, 8
call printf
add rsp, 8

vmovq xmm4, qword [rsp]
add rsp, 8
vmovq xmm3, qword [rsp]
add rsp, 8
vmovq xmm2, qword [rsp]
add rsp, 8
vmovq xmm1, qword [rsp]
add rsp, 8
vmovq xmm0, qword [rsp]
add rsp, 8
add rsp, 16

mov rsp, rbp  ; hlt
jmp END

L67:
movq qword [rsp - 16], xmm1  ; push reg
sub rsp, 16

mov rax, __float64__(-1.000000)  ; push imm
vmovq xmm5, rax
vmovq qword [rsp - 16], xmm5
sub rsp, 16
xor rax, rax

vmovq xmm5, qword [rsp + 16]  ; arithmetic
mulsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

movq qword [rsp - 16], xmm3  ; push reg
sub rsp, 16

vmovq xmm4, qword [rsp]  ; sqrt
vsqrtpd xmm4, xmm4
vmovq qword [rsp], xmm4

vmovq xmm5, qword [rsp + 16]  ; arithmetic
addsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

mov rax, __float64__(2.000000)  ; push imm
vmovq xmm5, rax
vmovq qword [rsp - 16], xmm5
sub rsp, 16
xor rax, rax

movq qword [rsp - 16], xmm0  ; push reg
sub rsp, 16

vmovq xmm5, qword [rsp + 16]  ; arithmetic
mulsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

vmovq xmm5, qword [rsp + 16]  ; arithmetic
divsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

lea rdi, printf_str  ; out
mov rax, 1
vmovq qword [rsp - 8], xmm0
sub rsp, 8
movq xmm0, qword [rsp + 8]
vmovq qword [rsp - 8], xmm1
sub rsp, 8
vmovq qword [rsp - 8], xmm2
sub rsp, 8
vmovq qword [rsp - 8], xmm3
sub rsp, 8
vmovq qword [rsp - 8], xmm4
sub rsp, 8

sub rsp, 8
call printf
add rsp, 8

vmovq xmm4, qword [rsp]
add rsp, 8
vmovq xmm3, qword [rsp]
add rsp, 8
vmovq xmm2, qword [rsp]
add rsp, 8
vmovq xmm1, qword [rsp]
add rsp, 8
vmovq xmm0, qword [rsp]
add rsp, 8
add rsp, 16

movq qword [rsp - 16], xmm1  ; push reg
sub rsp, 16

mov rax, __float64__(-1.000000)  ; push imm
vmovq xmm5, rax
vmovq qword [rsp - 16], xmm5
sub rsp, 16
xor rax, rax

vmovq xmm5, qword [rsp + 16]  ; arithmetic
mulsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

movq qword [rsp - 16], xmm3  ; push reg
sub rsp, 16

vmovq xmm4, qword [rsp]  ; sqrt
vsqrtpd xmm4, xmm4
vmovq qword [rsp], xmm4

vmovq xmm5, qword [rsp + 16]  ; arithmetic
subsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

mov rax, __float64__(2.000000)  ; push imm
vmovq xmm5, rax
vmovq qword [rsp - 16], xmm5
sub rsp, 16
xor rax, rax

movq qword [rsp - 16], xmm0  ; push reg
sub rsp, 16

vmovq xmm5, qword [rsp + 16]  ; arithmetic
mulsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

vmovq xmm5, qword [rsp + 16]  ; arithmetic
divsd xmm5, qword [rsp]
movq qword [rsp + 16], xmm5
add rsp, 16

lea rdi, printf_str  ; out
mov rax, 1
vmovq qword [rsp - 8], xmm0
sub rsp, 8
movq xmm0, qword [rsp + 8]
vmovq qword [rsp - 8], xmm1
sub rsp, 8
vmovq qword [rsp - 8], xmm2
sub rsp, 8
vmovq qword [rsp - 8], xmm3
sub rsp, 8
vmovq qword [rsp - 8], xmm4
sub rsp, 8

sub rsp, 8
call printf
add rsp, 8

vmovq xmm4, qword [rsp]
add rsp, 8
vmovq xmm3, qword [rsp]
add rsp, 8
vmovq xmm2, qword [rsp]
add rsp, 8
vmovq xmm1, qword [rsp]
add rsp, 8
vmovq xmm0, qword [rsp]
add rsp, 8
add rsp, 16

mov rsp, rbp  ; hlt
jmp END

L100:
mov rax, __float64__(7879.000000)  ; push imm
vmovq xmm5, rax
vmovq qword [rsp - 16], xmm5
sub rsp, 16
xor rax, rax

lea rdi, printf_str  ; out
mov rax, 1
vmovq qword [rsp - 8], xmm0
sub rsp, 8
movq xmm0, qword [rsp + 8]
vmovq qword [rsp - 8], xmm1
sub rsp, 8
vmovq qword [rsp - 8], xmm2
sub rsp, 8
vmovq qword [rsp - 8], xmm3
sub rsp, 8
vmovq qword [rsp - 8], xmm4
sub rsp, 8

sub rsp, 8
call printf
add rsp, 8

vmovq xmm4, qword [rsp]
add rsp, 8
vmovq xmm3, qword [rsp]
add rsp, 8
vmovq xmm2, qword [rsp]
add rsp, 8
vmovq xmm1, qword [rsp]
add rsp, 8
vmovq xmm0, qword [rsp]
add rsp, 8
add rsp, 16

mov rsp, rbp  ; hlt
jmp END


END:
