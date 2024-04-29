	.global c7_getcontext
	.text
c7_getcontext:
	movq %rbp, 16(%rdi)
	movq %rsp, 24(%rdi)
	movq %rbx, 32(%rdi)
	movq %r12, 40(%rdi)
	movq %r13, 48(%rdi)
	movq %r14, 56(%rdi)
	movq %r15, 64(%rdi)
	xor %rax, %rax
	ret

	.global c7_makecontext
	.text
c7_makecontext:

	# determine stack base address by ctx->uc_stack.ss_sp & ctx->uc_stack.ss_size
	movq (%rdi), %r8
	movq 8(%rdi), %r9
	addq %r9, %r8
	subq $8, %r8
	shrq $4, %r8
	shlq $4, %r8
	# *sp++ = func
	# *sp = sp
	movq %rsi, (%r8)
	subq $8, %r8
	movq %r8, (%r8)
	#
	movq %r8,  16(%rdi)
	movq %r8,  24(%rdi)
	movq %rbx, 32(%rdi)
	movq %r12, 40(%rdi)
	movq %r13, 48(%rdi)
	movq %r14, 56(%rdi)
	movq %r15, 64(%rdi)
	ret

	.global c7_swapcontext
	.text
c7_swapcontext:
	push %rbp
	movq %rbp, 16(%rdi)
	movq %rsp, 24(%rdi)
	movq %rbx, 32(%rdi)
	movq %r12, 40(%rdi)
	movq %r13, 48(%rdi)
	movq %r14, 56(%rdi)
	movq %r15, 64(%rdi)
	movq 16(%rsi), %rbp
	movq 24(%rsi), %rsp
	movq 32(%rsi), %rbx
	movq 40(%rsi), %r12
	movq 48(%rsi), %r13
	movq 56(%rsi), %r14
	movq 64(%rsi), %r15
	xor %rax, %rax
	pop %rbp
	ret
