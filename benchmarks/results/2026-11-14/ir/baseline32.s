	.att_syntax
	.file	"baseline32.cpp"
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
	movsd	(%r10,%r8,8), %xmm1             # xmm1 = mem[0],zero
	movsd	(%r9,%r8,8), %xmm2              # xmm2 = mem[0],zero
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	movq	16(%rdx), %r9
	shlq	$3, %r9
	addq	8(%rdx), %r9
	addsd	%xmm1, %xmm2
	movsd	%xmm2, (%r9,%r8,8)
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
	jle	.LBB1_9
# %bb.1:
	movq	8(%rdi), %rcx
	movq	16(%rdi), %rdi
	leaq	(%rcx,%rdi,8), %rcx
	movq	8(%rsi), %rdi
	movq	16(%rsi), %rsi
	leaq	(%rdi,%rsi,8), %rsi
	movq	8(%rdx), %rdi
	movq	16(%rdx), %rdx
	leaq	(%rdi,%rdx,8), %rdx
	cmpq	$1, %rax
	jne	.LBB1_4
# %bb.2:
	xorl	%edi, %edi
	jmp	.LBB1_3
.LBB1_4:
	movq	%rcx, %r8
	subq	%rdx, %r8
	xorl	%edi, %edi
	cmpq	$-16, %r8
	ja	.LBB1_3
# %bb.5:
	movq	%rsi, %r8
	subq	%rdx, %r8
	cmpq	$-16, %r8
	ja	.LBB1_3
# %bb.6:
	movabsq	$9223372036854775806, %rdi      # imm = 0x7FFFFFFFFFFFFFFE
	andq	%rax, %rdi
	movapd	%xmm0, %xmm1
	unpcklpd	%xmm0, %xmm1                    # xmm1 = xmm1[0],xmm0[0]
	xorl	%r8d, %r8d
	.p2align	4
.LBB1_7:                                # =>This Inner Loop Header: Depth=1
	movupd	(%rcx,%r8,8), %xmm2
	movupd	(%rsi,%r8,8), %xmm3
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	mulpd	%xmm1, %xmm2
	addpd	%xmm3, %xmm2
	movupd	%xmm2, (%rdx,%r8,8)
	addq	$2, %r8
	cmpq	%r8, %rdi
	jne	.LBB1_7
	jmp	.LBB1_8
.LBB1_3:
	movsd	(%rsi,%rdi,8), %xmm1            # xmm1 = mem[0],zero
	movsd	(%rcx,%rdi,8), %xmm2            # xmm2 = mem[0],zero
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	mulsd	%xmm0, %xmm2
	addsd	%xmm1, %xmm2
	movsd	%xmm2, (%rdx,%rdi,8)
	incq	%rdi
.LBB1_8:
	cmpq	%rdi, %rax
	jne	.LBB1_3
.LBB1_9:
	retq
.Lfunc_end1:
	.size	unchecked, .Lfunc_end1-unchecked
	.cfi_endproc
                                        # -- End function
	.ident	"Ubuntu clang version 23.1.6 (++20261108033808+c47d7823685a-1~exp1~20261108153820.99)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
