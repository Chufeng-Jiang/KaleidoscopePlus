	.att_syntax
	.file	"baseline128.cpp"
	.text
	.globl	checked                         # -- Begin function checked
	.prefalign	4, .Lfunc_end0, nop
	.type	checked,@function
checked:                                # @checked
	.cfi_startproc
# %bb.0:
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
	pushq	%rax
	.cfi_def_cfa_offset 64
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	.cfi_offset %rbp, -16
	movsd	%xmm0, (%rsp)                   # 8-byte Spill
	movq	24(%rdi), %r12
	testq	%r12, %r12
	jle	.LBB0_5
# %bb.1:
	movq	%rdx, %rbx
	movq	%rsi, %r14
	movq	%rdi, %r15
	movq	24(%rsi), %r13
	xorl	%ebp, %ebp
	testq	%r13, %r13
	cmovleq	%rbp, %r13
	.p2align	4
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	cmpq	%rbp, %r13
	je	.LBB0_6
# %bb.3:                                #   in Loop: Header=BB0_2 Depth=1
	cmpq	%rbp, 24(%rbx)
	jle	.LBB0_6
# %bb.4:                                #   in Loop: Header=BB0_2 Depth=1
	movq	16(%r15), %rax
	shlq	$3, %rax
	addq	8(%r15), %rax
	movsd	(%rax,%rbp,8), %xmm0            # xmm0 = mem[0],zero
	movq	16(%r14), %rax
	shlq	$3, %rax
	addq	8(%r14), %rax
	movsd	(%rax,%rbp,8), %xmm1            # xmm1 = mem[0],zero
	movsd	(%rsp), %xmm2                   # 8-byte Reload
                                        # xmm2 = mem[0],zero
	callq	_ZL7elementddd
	movq	16(%rbx), %rax
	shlq	$3, %rax
	addq	8(%rbx), %rax
	movsd	%xmm0, (%rax,%rbp,8)
	incq	%rbp
	cmpq	%rbp, %r12
	jne	.LBB0_2
.LBB0_5:
	addq	$8, %rsp
	.cfi_def_cfa_offset 56
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
	retq
.LBB0_6:
	.cfi_def_cfa_offset 64
	callq	abort@PLT
.Lfunc_end0:
	.size	checked, .Lfunc_end0-checked
	.cfi_endproc
                                        # -- End function
	.prefalign	4, .Lfunc_end1, nop     # -- Begin function _ZL7elementddd
	.type	_ZL7elementddd,@function
_ZL7elementddd:                         # @_ZL7elementddd
	.cfi_startproc
# %bb.0:
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	mulsd	%xmm2, %xmm0
	addsd	%xmm1, %xmm0
	retq
.Lfunc_end1:
	.size	_ZL7elementddd, .Lfunc_end1-_ZL7elementddd
	.cfi_endproc
                                        # -- End function
	.globl	unchecked                       # -- Begin function unchecked
	.prefalign	4, .Lfunc_end2, nop
	.type	unchecked,@function
unchecked:                              # @unchecked
	.cfi_startproc
# %bb.0:
	pushq	%r15
	.cfi_def_cfa_offset 16
	pushq	%r14
	.cfi_def_cfa_offset 24
	pushq	%r13
	.cfi_def_cfa_offset 32
	pushq	%r12
	.cfi_def_cfa_offset 40
	pushq	%rbx
	.cfi_def_cfa_offset 48
	subq	$16, %rsp
	.cfi_def_cfa_offset 64
	.cfi_offset %rbx, -48
	.cfi_offset %r12, -40
	.cfi_offset %r13, -32
	.cfi_offset %r14, -24
	.cfi_offset %r15, -16
	movq	24(%rdi), %rbx
	testq	%rbx, %rbx
	jle	.LBB2_3
# %bb.1:
	movq	16(%rdi), %r14
	shlq	$3, %r14
	addq	8(%rdi), %r14
	movq	16(%rsi), %r15
	shlq	$3, %r15
	addq	8(%rsi), %r15
	movq	16(%rdx), %r12
	shlq	$3, %r12
	addq	8(%rdx), %r12
	xorl	%r13d, %r13d
	movsd	%xmm0, 8(%rsp)                  # 8-byte Spill
	.p2align	4
.LBB2_2:                                # =>This Inner Loop Header: Depth=1
	movsd	(%r14,%r13,8), %xmm0            # xmm0 = mem[0],zero
	movsd	(%r15,%r13,8), %xmm1            # xmm1 = mem[0],zero
	movsd	8(%rsp), %xmm2                  # 8-byte Reload
                                        # xmm2 = mem[0],zero
	callq	_ZL7elementddd
	movsd	%xmm0, (%r12,%r13,8)
	incq	%r13
	cmpq	%r13, %rbx
	jne	.LBB2_2
.LBB2_3:
	addq	$16, %rsp
	.cfi_def_cfa_offset 48
	popq	%rbx
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r13
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end2:
	.size	unchecked, .Lfunc_end2-unchecked
	.cfi_endproc
                                        # -- End function
	.ident	"Ubuntu clang version 23.1.6 (++20261108033808+c47d7823685a-1~exp1~20261108153820.99)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
