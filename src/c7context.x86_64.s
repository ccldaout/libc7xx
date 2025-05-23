	.global c7_getcontext
	.text
c7_getcontext:
	xor %rax, %rax
	ret
	.type c7_getcontext, @function
	.size c7_getcontext, .-c7_getcontext

	.global c7_makecontext
	.text
c7_makecontext:
	# (rdi:ctx*, rsi:func_addr, rdx:func_param)

	# determine stack base address by ctx->stack_area & ctx->stack_size
	movq (%rdi), %r8	# rdi:ctx r8:=ctx->stack_area
	movq 8(%rdi), %r9	#         r9:=ctx->stack_size
	addq %r9, %r8		# 1.
	subq $8, %r8		# 2.
	shrq $4, %r8		# 3.
	shlq $4, %r8		# 4. alignment stack bottom address
	# r8: aligned bottom address of stack area

	movq %rsi, (%r8)	# rsi:function address
	subq $8, %r8
	movq %r8, (%r8)		# as rbp
	#
	movq %r8,  16(%rdi)	# top of ctx:stack to ctx->rbp
	movq %r8,  24(%rdi)	# top of ctx:stack to ctx->rsp
	movq %rdx, 72(%rdi)	# rdi to ctx->rdi
	ret
	.type c7_makecontext, @function
	.size c7_makecontext, .-c7_makecontext

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
	movq %rdi, 72(%rdi)
	movq 16(%rsi), %rbp
	movq 24(%rsi), %rsp
	movq 32(%rsi), %rbx
	movq 40(%rsi), %r12
	movq 48(%rsi), %r13
	movq 56(%rsi), %r14
	movq 64(%rsi), %r15
	movq 72(%rsi), %rdi
	xor %rax, %rax
	pop %rbp
	ret
	.type c7_swapcontext, @function
	.size c7_swapcontext, .-c7_swapcontext
