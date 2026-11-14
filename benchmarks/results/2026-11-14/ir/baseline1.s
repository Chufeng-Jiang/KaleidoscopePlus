	.att_syntax
	.file	"baseline1.cpp"
	.text
	.globl	checked                         # -- Begin function checked
	.prefalign	4, .Lfunc_end0, nop
	.type	checked,@function
checked:                                # @checked
	.cfi_startproc
# %bb.0:
	pushq	%rax
	.cfi_def_cfa_offset 16
	movq	24(%rdi), %rax
	testq	%rax, %rax
	jle	.LBB0_5
# %bb.1:
	movq	24(%rsi), %rcx
	xorl	%r8d, %r8d
	testq	%rcx, %rcx
	cmovleq	%r8, %rcx
	.p2align	4
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	cmpq	%r8, %rcx
	je	.LBB0_6
# %bb.3:                                #   in Loop: Header=BB0_2 Depth=1
	cmpq	%r8, 24(%rdx)
	jle	.LBB0_6
# %bb.4:                                #   in Loop: Header=BB0_2 Depth=1
	movq	16(%rdi), %r9
	shlq	$3, %r9
	addq	8(%rdi), %r9
	movq	16(%rsi), %r10
	shlq	$3, %r10
	addq	8(%rsi), %r10
	movsd	(%r9,%r8,8), %xmm1              # xmm1 = mem[0],zero
	mulsd	%xmm0, %xmm1
	addsd	(%r10,%r8,8), %xmm1
	movq	16(%rdx), %r9
	shlq	$3, %r9
	addq	8(%rdx), %r9
	movsd	%xmm1, (%r9,%r8,8)
	incq	%r8
	cmpq	%r8, %rax
	jne	.LBB0_2
.LBB0_5:
	popq	%rax
	.cfi_def_cfa_offset 8
	retq
.LBB0_6:
	.cfi_def_cfa_offset 16
	callq	abort@PLT
.Lfunc_end0:
	.size	checked, .Lfunc_end0-checked
	.cfi_endproc
                                        # -- End function
	.globl	unchecked                       # -- Begin function unchecked
	.prefalign	4, .Lfunc_end1, nop
	.type	unchecked,@function
unchecked:                              # @unchecked
	.cfi_startproc
# %bb.0:
	movq	24(%rdi), %rax
	testq	%rax, %rax
	jle	.LBB1_14
# %bb.1:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	pushq	%r15
	.cfi_def_cfa_offset 24
	pushq	%r14
	.cfi_def_cfa_offset 32
	pushq	%r13
	.cfi_def_cfa_offset 40
	pushq	%r12
	.cfi_def_cfa_offset 48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	.cfi_offset %rbp, -16
	movq	8(%rdi), %r8
	movq	16(%rdi), %rdi
	leaq	(%r8,%rdi,8), %rbx
	movq	8(%rsi), %r9
	movq	16(%rsi), %rsi
	leaq	(%r9,%rsi,8), %r11
	movq	8(%rdx), %r10
	movq	16(%rdx), %rdx
	leaq	(%r10,%rdx,8), %r14
	cmpq	$16, %rax
	jae	.LBB1_3
# %bb.2:
	xorl	%ecx, %ecx
.LBB1_8:
	leaq	1(%rcx), %r15
	testb	$1, %al
	je	.LBB1_10
# %bb.9:
	movsd	(%rbx,%rcx,8), %xmm1            # xmm1 = mem[0],zero
	mulsd	%xmm0, %xmm1
	addsd	(%r11,%rcx,8), %xmm1
	movsd	%xmm1, (%r14,%rcx,8)
	movq	%r15, %rcx
.LBB1_10:
	cmpq	%r15, %rax
	je	.LBB1_13
# %bb.11:
	leaq	(%r10,%rdx,8), %rdx
	addq	$8, %rdx
	leaq	(%r9,%rsi,8), %rsi
	addq	$8, %rsi
	leaq	(%r8,%rdi,8), %rdi
	addq	$8, %rdi
	.p2align	4
.LBB1_12:                               # =>This Inner Loop Header: Depth=1
	movsd	-8(%rdi,%rcx,8), %xmm1          # xmm1 = mem[0],zero
	mulsd	%xmm0, %xmm1
	addsd	-8(%rsi,%rcx,8), %xmm1
	movsd	%xmm1, -8(%rdx,%rcx,8)
	movsd	(%rdi,%rcx,8), %xmm1            # xmm1 = mem[0],zero
	mulsd	%xmm0, %xmm1
	addsd	(%rsi,%rcx,8), %xmm1
	movsd	%xmm1, (%rdx,%rcx,8)
	addq	$2, %rcx
	cmpq	%rcx, %rax
	jne	.LBB1_12
	jmp	.LBB1_13
.LBB1_3:
	movq	%rbx, %r15
	subq	%r14, %r15
	xorl	%ecx, %ecx
	cmpq	$-32, %r15
	ja	.LBB1_8
# %bb.4:
	movq	%r11, %r15
	subq	%r14, %r15
	cmpq	$-32, %r15
	ja	.LBB1_8
# %bb.5:
	leaq	(,%rdx,8), %r15
	leaq	(,%rdi,8), %r13
	leaq	(,%rsi,8), %r12
	movabsq	$9223372036854775804, %rcx      # imm = 0x7FFFFFFFFFFFFFFC
	andq	%rax, %rcx
	movapd	%xmm0, %xmm1
	unpcklpd	%xmm0, %xmm1                    # xmm1 = xmm1[0],xmm0[0]
	addq	%r10, %r15
	addq	$16, %r15
	addq	%r9, %r12
	addq	$16, %r12
	addq	%r8, %r13
	addq	$16, %r13
	xorl	%ebp, %ebp
	.p2align	4
.LBB1_6:                                # =>This Inner Loop Header: Depth=1
	movupd	-16(%r13,%rbp,8), %xmm2
	movupd	(%r13,%rbp,8), %xmm3
	movupd	-16(%r12,%rbp,8), %xmm4
	movupd	(%r12,%rbp,8), %xmm5
	mulpd	%xmm1, %xmm2
	addpd	%xmm4, %xmm2
	mulpd	%xmm1, %xmm3
	addpd	%xmm5, %xmm3
	movupd	%xmm2, -16(%r15,%rbp,8)
	movupd	%xmm3, (%r15,%rbp,8)
	addq	$4, %rbp
	cmpq	%rbp, %rcx
	jne	.LBB1_6
# %bb.7:
	cmpq	%rcx, %rax
	jne	.LBB1_8
.LBB1_13:
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%r12
	.cfi_def_cfa_offset 40
	popq	%r13
	.cfi_def_cfa_offset 32
	popq	%r14
	.cfi_def_cfa_offset 24
	popq	%r15
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	.cfi_restore %rbx
	.cfi_restore %r12
	.cfi_restore %r13
	.cfi_restore %r14
	.cfi_restore %r15
	.cfi_restore %rbp
.LBB1_14:
	retq
.Lfunc_end1:
	.size	unchecked, .Lfunc_end1-unchecked
	.cfi_endproc
                                        # -- End function
	.ident	"Ubuntu clang version 23.1.6 (++20261108033808+c47d7823685a-1~exp1~20261108153820.99)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
