	.file	"looped-sgemm.c"
	.text
	.p2align 4
	.globl	init_rand
	.type	init_rand, @function
init_rand:
.LFB6456:
	.cfi_startproc
	endbr64
	imull	%edx, %esi
	testl	%esi, %esi
	jle	.L7
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movslq	%esi, %rsi
	pushq	%rbx
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	movq	%rdi, %rbx
	subq	$8, %rsp
	.cfi_def_cfa_offset 32
	leaq	(%rdi,%rsi,4), %rbp
	.p2align 4
	.p2align 3
.L3:
	call	rand@PLT
	vxorps	%xmm1, %xmm1, %xmm1
	addq	$4, %rbx
	vcvtsi2ssl	%eax, %xmm1, %xmm0
	vmulss	.LC0(%rip), %xmm0, %xmm0
	vmovss	%xmm0, -4(%rbx)
	cmpq	%rbp, %rbx
	jne	.L3
	addq	$8, %rsp
	.cfi_def_cfa_offset 24
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	ret
	.p2align 4
	.p2align 3
.L7:
	.cfi_restore 3
	.cfi_restore 6
	ret
	.cfi_endproc
.LFE6456:
	.size	init_rand, .-init_rand
	.p2align 4
	.globl	init_const
	.type	init_const, @function
init_const:
.LFB6457:
	.cfi_startproc
	endbr64
	imull	%esi, %edx
	movq	%rdi, %rcx
	testl	%edx, %edx
	jle	.L30
	leal	-1(%rdx), %eax
	cmpl	$6, %eax
	jbe	.L18
	movl	%edx, %esi
	movq	%rdi, %rax
	vbroadcastss	%xmm0, %ymm1
	shrl	$3, %esi
	salq	$5, %rsi
	leaq	(%rsi,%rdi), %rdi
	andl	$32, %esi
	je	.L14
	leaq	32(%rcx), %rax
	vmovups	%ymm1, (%rcx)
	cmpq	%rdi, %rax
	je	.L28
	.p2align 4
	.p2align 3
.L14:
	vmovups	%ymm1, (%rax)
	vmovups	%ymm1, 32(%rax)
	addq	$64, %rax
	cmpq	%rdi, %rax
	jne	.L14
.L28:
	movl	%edx, %eax
	andl	$-8, %eax
	movl	%eax, %esi
	cmpl	%eax, %edx
	je	.L31
	vzeroupper
.L13:
	movl	%edx, %edi
	subl	%esi, %edi
	leal	-1(%rdi), %r8d
	cmpl	$2, %r8d
	jbe	.L16
	vshufps	$0, %xmm0, %xmm0, %xmm1
	vmovups	%xmm1, (%rcx,%rsi,4)
	movl	%edi, %esi
	andl	$-4, %esi
	addl	%esi, %eax
	andl	$3, %edi
	je	.L30
.L16:
	movslq	%eax, %rsi
	leal	1(%rax), %edi
	salq	$2, %rsi
	vmovss	%xmm0, (%rcx,%rsi)
	cmpl	%edi, %edx
	jle	.L30
	addl	$2, %eax
	vmovss	%xmm0, 4(%rcx,%rsi)
	cmpl	%edx, %eax
	jge	.L30
	vmovss	%xmm0, 8(%rcx,%rsi)
.L30:
	ret
	.p2align 4
	.p2align 3
.L31:
	vzeroupper
	ret
.L18:
	xorl	%esi, %esi
	xorl	%eax, %eax
	jmp	.L13
	.cfi_endproc
.LFE6457:
	.size	init_const, .-init_const
	.p2align 4
	.globl	timer
	.type	timer, @function
timer:
.LFB6458:
	.cfi_startproc
	endbr64
	subq	$40, %rsp
	.cfi_def_cfa_offset 48
	movl	$4, %edi
	movq	%rsp, %rsi
	movq	%fs:40, %rax
	movq	%rax, 24(%rsp)
	xorl	%eax, %eax
	call	clock_gettime@PLT
	imulq	$1000000000, (%rsp), %rax
	addq	8(%rsp), %rax
	movq	24(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L35
	addq	$40, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 8
	ret
.L35:
	.cfi_restore_state
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE6458:
	.size	timer, .-timer
	.p2align 4
	.globl	matmul_naive
	.type	matmul_naive, @function
matmul_naive:
.LFB6459:
	.cfi_startproc
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movl	%ecx, -12(%rsp)
	testl	%ecx, %ecx
	jle	.L48
	testl	%r8d, %r8d
	jle	.L48
	movq	%rdx, %rax
	movq	%rdi, %r10
	movslq	%r8d, %rdi
	movl	%r9d, %r14d
	movq	%rax, %r15
	movl	%r9d, %eax
	leaq	0(,%rdi,4), %rdx
	movq	%rsi, %r11
	shrl	%eax
	movl	%r9d, %r13d
	leaq	0(,%rdi,8), %rbp
	andl	$-2, %r14d
	salq	$3, %rax
	xorl	%esi, %esi
	movq	%rdx, -8(%rsp)
	movq	%rax, -24(%rsp)
	xorl	%eax, %eax
	.p2align 4
	.p2align 3
.L38:
	movslq	%esi, %rcx
	movq	%r11, %r12
	xorl	%r9d, %r9d
	movl	%eax, -16(%rsp)
	leaq	(%r10,%rcx,4), %rbx
	movq	-24(%rsp), %rcx
	movq	%rbx, -32(%rsp)
	addq	%rcx, %rbx
	.p2align 4
	.p2align 3
.L45:
	movl	%r9d, %ecx
	vxorps	%xmm1, %xmm1, %xmm1
	testl	%r13d, %r13d
	jle	.L44
	cmpl	$1, %r13d
	je	.L46
	movq	-32(%rsp), %rdx
	movq	%r12, %rax
	vxorps	%xmm1, %xmm1, %xmm1
	.p2align 4
	.p2align 3
.L40:
	vmovss	(%rax), %xmm0
	vmovq	(%rdx), %xmm2
	addq	$8, %rdx
	vinsertps	$0x10, (%rax,%rdi,4), %xmm0, %xmm0
	addq	%rbp, %rax
	vmulps	%xmm2, %xmm0, %xmm2
	vaddss	%xmm2, %xmm1, %xmm1
	vmovshdup	%xmm2, %xmm2
	vaddss	%xmm1, %xmm2, %xmm1
	cmpq	%rdx, %rbx
	jne	.L40
	movl	%r14d, %eax
	cmpl	%r14d, %r13d
	je	.L44
.L39:
	movl	%r8d, %edx
	imull	%eax, %edx
	addl	%esi, %eax
	cltq
	addl	%ecx, %edx
	movslq	%edx, %rdx
	vmovss	(%r11,%rdx,4), %xmm3
	vfmadd231ss	(%r10,%rax,4), %xmm3, %xmm1
.L44:
	vmovss	%xmm1, (%r15,%r9,4)
	incq	%r9
	addq	$4, %r12
	cmpq	%r9, %rdi
	jne	.L45
	movl	-16(%rsp), %eax
	movq	-8(%rsp), %rbx
	addl	%r13d, %esi
	incl	%eax
	addq	%rbx, %r15
	cmpl	%eax, -12(%rsp)
	jne	.L38
.L48:
	popq	%rbx
	.cfi_remember_state
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
.L46:
	.cfi_restore_state
	xorl	%eax, %eax
	vxorps	%xmm1, %xmm1, %xmm1
	jmp	.L39
	.cfi_endproc
.LFE6459:
	.size	matmul_naive, .-matmul_naive
	.p2align 4
	.globl	looped_sgemm
	.type	looped_sgemm, @function
looped_sgemm:
.LFB6460:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	vmovss	.LC2(%rip), %xmm0
	vxorps	%xmm1, %xmm1, %xmm1
	pushq	%r8
	.cfi_def_cfa_offset 24
	pushq	%rdx
	.cfi_def_cfa_offset 32
	movl	$111, %edx
	pushq	%r8
	.cfi_def_cfa_offset 40
	pushq	%rsi
	.cfi_def_cfa_offset 48
	movl	$111, %esi
	pushq	%r9
	.cfi_def_cfa_offset 56
	pushq	%rdi
	.cfi_def_cfa_offset 64
	movl	$101, %edi
	call	cblas_sgemm@PLT
	addq	$56, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE6460:
	.size	looped_sgemm, .-looped_sgemm
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC4:
	.string	"Memory allocation failed\n"
.LC5:
	.string	"Performing warmup runs..."
.LC6:
	.string	"Performing timing runs..."
.LC8:
	.string	"Run %d:\n"
.LC10:
	.string	"  Execution Time: %.3f ms\n"
.LC13:
	.string	"  Performance: %.2f GFLOPS\n"
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC15:
	.string	"\nAverage over %d runs: %.3f ms (%.2f GFLOPS)\n"
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB6461:
	.cfi_startproc
	endbr64
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movl	$1, %edi
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$72, %rsp
	.cfi_def_cfa_offset 128
	movq	%fs:40, %rax
	movq	%rax, 56(%rsp)
	xorl	%eax, %eax
	call	srand@PLT
	movl	$16777216, %esi
	movl	$64, %edi
	call	aligned_alloc@PLT
	movl	$67108864, %esi
	movl	$64, %edi
	movq	%rax, %rbx
	call	aligned_alloc@PLT
	movl	$16777216, %esi
	movl	$64, %edi
	movq	%rax, %rbp
	call	aligned_alloc@PLT
	testq	%rbx, %rbx
	movq	%rax, %r12
	sete	%al
	testq	%rbp, %rbp
	sete	%dl
	orb	%dl, %al
	jne	.L62
	testq	%r12, %r12
	je	.L62
	movq	%rbx, %r14
	leaq	16777216(%rbx), %r13
	.p2align 4
	.p2align 3
.L55:
	call	rand@PLT
	vxorps	%xmm4, %xmm4, %xmm4
	addq	$4, %r14
	vcvtsi2ssl	%eax, %xmm4, %xmm0
	vmulss	.LC0(%rip), %xmm0, %xmm0
	vmovss	%xmm0, -4(%r14)
	cmpq	%r14, %r13
	jne	.L55
	movq	%rbp, %r14
	leaq	67108864(%rbp), %r13
	.p2align 4
	.p2align 3
.L56:
	call	rand@PLT
	vxorps	%xmm5, %xmm5, %xmm5
	addq	$4, %r14
	vcvtsi2ssl	%eax, %xmm5, %xmm0
	vmulss	.LC0(%rip), %xmm0, %xmm0
	vmovss	%xmm0, -4(%r14)
	cmpq	%r13, %r14
	jne	.L56
	movl	$16777216, %edx
	xorl	%esi, %esi
	movq	%r12, %rdi
	movl	$10, %r13d
	call	memset@PLT
	leaq	.LC5(%rip), %rdi
	call	puts@PLT
	.p2align 4
	.p2align 3
.L57:
	vmovss	.LC2(%rip), %xmm0
	pushq	$4096
	.cfi_def_cfa_offset 136
	vxorps	%xmm1, %xmm1, %xmm1
	pushq	%r12
	.cfi_def_cfa_offset 144
	movl	$4096, %r9d
	pushq	$4096
	.cfi_def_cfa_offset 152
	movl	$4096, %r8d
	pushq	%rbp
	.cfi_def_cfa_offset 160
	movl	$1024, %ecx
	pushq	$4096
	.cfi_def_cfa_offset 168
	movl	$111, %edx
	pushq	%rbx
	.cfi_def_cfa_offset 176
	movl	$111, %esi
	movl	$101, %edi
	call	cblas_sgemm@PLT
	addq	$48, %rsp
	.cfi_def_cfa_offset 128
	decl	%r13d
	jne	.L57
	leaq	.LC6(%rip), %rdi
	xorl	%r13d, %r13d
	call	puts@PLT
	vxorpd	%xmm2, %xmm2, %xmm2
	leaq	32(%rsp), %rax
	movq	%rax, 24(%rsp)
	vmovsd	%xmm2, 16(%rsp)
	jmp	.L60
	.p2align 4
	.p2align 3
.L69:
	vxorpd	%xmm7, %xmm7, %xmm7
	vcvtsi2sdq	%rax, %xmm7, %xmm1
.L59:
	vmulsd	.LC7(%rip), %xmm1, %xmm3
	incl	%r13d
	leaq	.LC8(%rip), %rsi
	movl	$2, %edi
	movl	%r13d, %edx
	xorl	%eax, %eax
	vaddsd	16(%rsp), %xmm3, %xmm6
	vmovsd	%xmm3, 8(%rsp)
	vmovsd	%xmm6, 16(%rsp)
	call	__printf_chk@PLT
	vmovsd	.LC9(%rip), %xmm2
	leaq	.LC10(%rip), %rsi
	vmulsd	8(%rsp), %xmm2, %xmm0
	movl	$2, %edi
	movl	$1, %eax
	call	__printf_chk@PLT
	vmovsd	.LC11(%rip), %xmm6
	leaq	.LC13(%rip), %rsi
	movl	$2, %edi
	vdivsd	8(%rsp), %xmm6, %xmm0
	movl	$1, %eax
	vdivsd	.LC12(%rip), %xmm0, %xmm0
	call	__printf_chk@PLT
	cmpl	$5, %r13d
	je	.L68
.L60:
	movq	24(%rsp), %rsi
	movl	$4, %edi
	call	clock_gettime@PLT
	vmovss	.LC2(%rip), %xmm0
	movl	$111, %edx
	vxorps	%xmm1, %xmm1, %xmm1
	imulq	$1000000000, 32(%rsp), %r14
	movq	40(%rsp), %r15
	movl	$4096, %r9d
	pushq	$4096
	.cfi_def_cfa_offset 136
	movl	$4096, %r8d
	pushq	%r12
	.cfi_def_cfa_offset 144
	movl	$1024, %ecx
	pushq	$4096
	.cfi_def_cfa_offset 152
	movl	$111, %esi
	pushq	%rbp
	.cfi_def_cfa_offset 160
	movl	$101, %edi
	pushq	$4096
	.cfi_def_cfa_offset 168
	pushq	%rbx
	.cfi_def_cfa_offset 176
	call	cblas_sgemm@PLT
	movq	72(%rsp), %rsi
	addq	$48, %rsp
	.cfi_def_cfa_offset 128
	movl	$4, %edi
	call	clock_gettime@PLT
	imulq	$1000000000, 32(%rsp), %rdx
	movq	40(%rsp), %rax
	subq	%r14, %rax
	subq	%r15, %rdx
	addq	%rdx, %rax
	jns	.L69
	movq	%rax, %rdx
	andl	$1, %eax
	vxorpd	%xmm7, %xmm7, %xmm7
	shrq	%rdx
	orq	%rax, %rdx
	vcvtsi2sdq	%rdx, %xmm7, %xmm1
	vaddsd	%xmm1, %xmm1, %xmm1
	jmp	.L59
	.p2align 4
	.p2align 3
.L68:
	vmovsd	16(%rsp), %xmm2
	vmovsd	.LC11(%rip), %xmm4
	movl	$5, %edx
	leaq	.LC15(%rip), %rsi
	vdivsd	.LC14(%rip), %xmm2, %xmm2
	movl	$2, %edi
	movl	$2, %eax
	vmulsd	.LC9(%rip), %xmm2, %xmm0
	vdivsd	%xmm2, %xmm4, %xmm1
	vdivsd	.LC12(%rip), %xmm1, %xmm1
	call	__printf_chk@PLT
	movq	%rbx, %rdi
	call	free@PLT
	movq	%rbp, %rdi
	call	free@PLT
	movq	%r12, %rdi
	call	free@PLT
	movq	56(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L70
	addq	$72, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%rbp
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	ret
.L62:
	.cfi_restore_state
	movq	stderr(%rip), %rcx
	leaq	.LC4(%rip), %rdi
	movl	$25, %edx
	movl	$1, %esi
	call	fwrite@PLT
	movl	$1, %edi
	call	exit@PLT
.L70:
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE6461:
	.size	main, .-main
	.section	.rodata.cst4,"aM",@progbits,4
	.align 4
.LC0:
	.long	805306368
	.align 4
.LC2:
	.long	1065353216
	.section	.rodata.cst8,"aM",@progbits,8
	.align 8
.LC7:
	.long	-400107883
	.long	1041313291
	.align 8
.LC9:
	.long	0
	.long	1083129856
	.align 8
.LC11:
	.long	0
	.long	1109393408
	.align 8
.LC12:
	.long	0
	.long	1104006501
	.align 8
.LC14:
	.long	0
	.long	1075052544
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
