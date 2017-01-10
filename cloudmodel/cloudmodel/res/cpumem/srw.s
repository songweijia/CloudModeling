	.file	"seq_read_write.c"
	.comm	buf,8,8
	.section	.text.unlikely,"ax",@progbits
.LCOLDB0:
	.text
.LHOTB0:
	.p2align 4,,15
	.globl	init
	.type	init, @function
init:
.LFB49:
	.cfi_startproc
	sarl	$3, %esi
	testl	%esi, %esi
	jle	.L3
	leal	-1(%rsi), %eax
	movslq	%esi, %rsi
	leaq	(%rdi,%rsi,8), %rdi
	movl	$165, %esi
	cltq
	leaq	8(,%rax,8), %rdx
	subq	%rdx, %rdi
	jmp	memset
	.p2align 4,,10
	.p2align 3
.L3:
	rep ret
	.cfi_endproc
.LFE49:
	.size	init, .-init
	.section	.text.unlikely
.LCOLDE0:
	.text
.LHOTE0:
	.p2align 4
	.globl	doRead
	.type	doRead, @function
doRead:
.LFB50:
	.cfi_startproc
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
	subq	$104, %rsp
	.cfi_def_cfa_offset 160
	movq	%rdi, 24(%rsp)
	movl	%esi, 20(%rsp)
	movl	%edx, 16(%rsp)
	movq	%fs:40, %rax
	movq	%rax, 88(%rsp)
	xorl	%eax, %eax
	movq	24(%rsp), %rbp
	movl	20(%rsp), %eax
	movl	%eax, 12(%rsp)
	leaq	48(%rsp), %rax
	movq	%rax, %rsi
	movl	$0, %edi
	call	clock_gettime
	jmp	.L6
.L9:
	movl	$0, %ebx
	jmp	.L7
.L8:
	movslq	%ebx, %rax
	salq	$3, %rax
	addq	%rbp, %rax
	movq	(%rax), %r8
	movq	8(%rax), %r9
	movq	16(%rax), %r10
	movq	24(%rax), %r11
	movq	32(%rax), %r12
	movq	40(%rax), %r13
	movq	48(%rax), %r14
	movq	56(%rax), %r15
	movq	64(%rax), %r8
	movq	72(%rax), %r9
	movq	80(%rax), %r10
	movq	88(%rax), %r11
	movq	96(%rax), %r12
	movq	104(%rax), %r13
	movq	112(%rax), %r14
	movq	120(%rax), %r15
	movq	128(%rax), %r8
	movq	136(%rax), %r9
	movq	144(%rax), %r10
	movq	152(%rax), %r11
	movq	160(%rax), %r12
	movq	168(%rax), %r13
	movq	176(%rax), %r14
	movq	184(%rax), %r15
	movq	192(%rax), %r8
	movq	200(%rax), %r9
	movq	208(%rax), %r10
	movq	216(%rax), %r11
	movq	224(%rax), %r12
	movq	232(%rax), %r13
	movq	240(%rax), %r14
	movq	248(%rax), %r15

	addl	$32, %ebx
.L7:
	cmpl	12(%rsp), %ebx
	jl	.L8
.L6:
	movl	16(%rsp), %eax
	leal	-1(%rax), %edx
	movl	%edx, 16(%rsp)
	testl	%eax, %eax
	jg	.L9
	leaq	64(%rsp), %rax
	movq	%rax, %rsi
	movl	$0, %edi
	call	clock_gettime
	movq	%r8, %rdx
	movq	%r9, %rax
	orq	%rdx, %rax
	movq	%r10, %rdx
	orq	%rdx, %rax
	movq	%r11, %rdx
	orq	%rdx, %rax
	movq	%r12, %rdx
	orq	%rdx, %rax
	movq	%r13, %rdx
	orq	%rdx, %rax
	movq	%r14, %rdx
	orq	%rdx, %rax
	movq	%r15, %rdx
	orq	%rdx, %rax
	movq	%rax, 40(%rsp)
	movq	64(%rsp), %rdx
	movq	48(%rsp), %rax
	subq	%rax, %rdx
	movq	%rdx, %rax
	imulq	$1000000000, %rax, %rax
	movq	72(%rsp), %rcx
	movq	56(%rsp), %rdx
	subq	%rdx, %rcx
	movq	%rcx, %rdx
	addq	%rdx, %rax
	movq	88(%rsp), %rsi
	xorq	%fs:40, %rsi
	je	.L11
	call	__stack_chk_fail
.L11:
	addq	$104, %rsp
	.cfi_def_cfa_offset 56
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
	.cfi_endproc
.LFE50:
	.size	doRead, .-doRead
	.p2align 4
	.globl	doWrit
	.type	doWrit, @function
doWrit:
.LFB51:
	.cfi_startproc
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
	subq	$88, %rsp
	.cfi_def_cfa_offset 144
	movq	%rdi, 24(%rsp)
	movl	%esi, 20(%rsp)
	movl	%edx, 16(%rsp)
	movq	%fs:40, %rax
	movq	%rax, 72(%rsp)
	xorl	%eax, %eax
	movq	24(%rsp), %rbp
	movl	20(%rsp), %eax
	movl	%eax, 12(%rsp)
	movabsq	$6510615555426900480, %r8
	movabsq	$6510615555426900481, %r9
	movabsq	$6510615555426900482, %r10
	movabsq	$6510615555426900483, %r11
	movabsq	$6510615555426900484, %r12
	movabsq	$6510615555426900485, %r13
	movabsq	$6510615555426900486, %r14
	movabsq	$6510615555426900487, %r15
	leaq	32(%rsp), %rax
	movq	%rax, %rsi
	movl	$0, %edi
	call	clock_gettime
	jmp	.L13
.L16:
	movl	$0, %ebx
	jmp	.L14
.L15:
	movslq	%ebx, %rax
	salq	$3, %rax
	addq	%rbp, %rax
	movq	%r8, (%rax)
	movq	%r9, 8(%rax)
	movq	%r10, 16(%rax)
	movq	%r11, 24(%rax)
	movq	%r12, 32(%rax)
	movq	%r13, 40(%rax)
	movq	%r14, 48(%rax)
	movq	%r15, 56(%rax)
	movq	%r8, 64(%rax)
	movq	%r9, 72(%rax)
	movq	%r10, 80(%rax)
	movq	%r11, 88(%rax)
	movq	%r12, 96(%rax)
	movq	%r13, 104(%rax)
	movq	%r14, 112(%rax)
	movq	%r15, 120(%rax)
	movq	%r8, 128(%rax)
	movq	%r9, 136(%rax)
	movq	%r10, 144(%rax)
	movq	%r11, 152(%rax)
	movq	%r12, 160(%rax)
	movq	%r13, 168(%rax)
	movq	%r14, 176(%rax)
	movq	%r15, 184(%rax)
	movq	%r8, 192(%rax)
	movq	%r9, 200(%rax)
	movq	%r10, 208(%rax)
	movq	%r11, 216(%rax)
	movq	%r12, 224(%rax)
	movq	%r13, 232(%rax)
	movq	%r14, 240(%rax)
	movq	%r15, 248(%rax)

	addl	$32, %ebx
.L14:
	cmpl	12(%rsp), %ebx
	jl	.L15
.L13:
	movl	16(%rsp), %eax
	leal	-1(%rax), %edx
	movl	%edx, 16(%rsp)
	testl	%eax, %eax
	jg	.L16
	leaq	48(%rsp), %rax
	movq	%rax, %rsi
	movl	$0, %edi
	call	clock_gettime
	movq	48(%rsp), %rdx
	movq	32(%rsp), %rax
	subq	%rax, %rdx
	movq	%rdx, %rax
	imulq	$1000000000, %rax, %rax
	movq	56(%rsp), %rcx
	movq	40(%rsp), %rdx
	subq	%rdx, %rcx
	movq	%rcx, %rdx
	addq	%rdx, %rax
	movq	72(%rsp), %rsi
	xorq	%fs:40, %rsi
	je	.L18
	call	__stack_chk_fail
.L18:
	addq	$88, %rsp
	.cfi_def_cfa_offset 56
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
	.cfi_endproc
.LFE51:
	.size	doWrit, .-doWrit
	.section	.rodata
	.align 8
.LC1:
	.string	"usage:%s <buffersize(KB)> <num>\n"
.LC2:
	.string	"memalign() failed...exit.\n"
	.align 8
.LC3:
	.string	"buffer_size: %u KB\tread: %.3f GB/s\twrite: %.3f GB/s\n"
	.section	.text.unlikely
.LCOLDB4:
	.section	.text.startup,"ax",@progbits
.LHOTB4:
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB52:
	.cfi_startproc
	cmpl	$2, %edi
	jle	.L29
	pushq	%r14
	.cfi_def_cfa_offset 16
	.cfi_offset 14, -16
	pushq	%r13
	.cfi_def_cfa_offset 24
	.cfi_offset 13, -24
	movl	$10, %edx
	pushq	%r12
	.cfi_def_cfa_offset 32
	.cfi_offset 12, -32
	pushq	%rbp
	.cfi_def_cfa_offset 40
	.cfi_offset 6, -40
	pushq	%rbx
	.cfi_def_cfa_offset 48
	.cfi_offset 3, -48
	movq	8(%rsi), %rdi
	movq	%rsi, %rbx
	xorl	%esi, %esi
	call	strtol
	movq	16(%rbx), %rdi
	movl	%eax, %ebp
	xorl	%esi, %esi
	sall	$10, %ebp
	movl	$10, %edx
	call	strtol
	movslq	%ebp, %rsi
	movl	$1024, %edi
	movq	%rax, %rbx
	call	memalign
	testq	%rax, %rax
	movq	%rax, %r14
	je	.L30
	movl	%ebp, %r13d
	sarl	$3, %r13d
	testl	%r13d, %r13d
	jle	.L27
	leal	-1(%r13), %eax
	movl	$165, %esi
	cltq
	leaq	8(,%rax,8), %rdx
	movslq	%r13d, %rax
	leaq	(%r14,%rax,8), %rdi
	subq	%rdx, %rdi
	call	memset
.L27:
	movl	%ebx, %edx
	movl	%r13d, %esi
	movq	%r14, %rdi
	call	doRead
	movl	%ebx, %edx
	movl	%r13d, %esi
	movq	%r14, %rdi
	movq	%rax, %r12
	call	doWrit
	pxor	%xmm0, %xmm0
	testq	%rax, %rax
	pxor	%xmm1, %xmm1
	cvtsi2sd	%ebp, %xmm0
	cvtsi2sd	%ebx, %xmm1
	mulsd	%xmm1, %xmm0
	js	.L23
	pxor	%xmm1, %xmm1
	cvtsi2sdq	%rax, %xmm1
.L24:
	movapd	%xmm0, %xmm3
	testq	%r12, %r12
	divsd	%xmm1, %xmm3
	movapd	%xmm3, %xmm1
	js	.L25
	pxor	%xmm2, %xmm2
	cvtsi2sdq	%r12, %xmm2
.L26:
	divsd	%xmm2, %xmm0
	movl	%ebp, %edx
	movl	$.LC3, %esi
	popq	%rbx
	.cfi_restore 3
	.cfi_def_cfa_offset 40
	popq	%rbp
	.cfi_restore 6
	.cfi_def_cfa_offset 32
	popq	%r12
	.cfi_restore 12
	.cfi_def_cfa_offset 24
	popq	%r13
	.cfi_restore 13
	.cfi_def_cfa_offset 16
	popq	%r14
	.cfi_restore 14
	.cfi_def_cfa_offset 8
	sarl	$10, %edx
	movl	$1, %edi
	movl	$2, %eax
	jmp	__printf_chk
.L29:
	movq	(%rsi), %rdx
	movl	$1, %edi
	movl	$.LC1, %esi
	xorl	%eax, %eax
	jmp	__printf_chk
.L23:
	.cfi_def_cfa_offset 48
	.cfi_offset 3, -48
	.cfi_offset 6, -40
	.cfi_offset 12, -32
	.cfi_offset 13, -24
	.cfi_offset 14, -16
	movq	%rax, %rdx
	pxor	%xmm1, %xmm1
	shrq	%rdx
	andl	$1, %eax
	orq	%rax, %rdx
	cvtsi2sdq	%rdx, %xmm1
	addsd	%xmm1, %xmm1
	jmp	.L24
.L25:
	movq	%r12, %rax
	pxor	%xmm2, %xmm2
	shrq	%rax
	andl	$1, %r12d
	orq	%r12, %rax
	cvtsi2sdq	%rax, %xmm2
	addsd	%xmm2, %xmm2
	jmp	.L26
.L30:
	popq	%rbx
	.cfi_restore 3
	.cfi_def_cfa_offset 40
	popq	%rbp
	.cfi_restore 6
	.cfi_def_cfa_offset 32
	popq	%r12
	.cfi_restore 12
	.cfi_def_cfa_offset 24
	popq	%r13
	.cfi_restore 13
	.cfi_def_cfa_offset 16
	popq	%r14
	.cfi_restore 14
	.cfi_def_cfa_offset 8
	movq	stderr(%rip), %rcx
	movl	$26, %edx
	movl	$1, %esi
	movl	$.LC2, %edi
	jmp	fwrite
	.cfi_endproc
.LFE52:
	.size	main, .-main
	.section	.text.unlikely
.LCOLDE4:
	.section	.text.startup
.LHOTE4:
	.ident	"GCC: (Ubuntu 5.4.0-6ubuntu1~16.04.2) 5.4.0 20160609"
	.section	.note.GNU-stack,"",@progbits
