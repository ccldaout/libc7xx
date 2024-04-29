	.global c7_getcontext
	.text
	.p2align 4
c7_getcontext:
	mov x0, 0
	ret

	.global c7_makecontext
	.text
	.p2align 4
c7_makecontext:
	ldp x3, x4, [x0]
	add x3, x3, x4
	sub x3, x3, #8
	lsr x3, x3, #4
	lsl x3, x3, #4
	stp x1, x3, [x0, #16 * 10]
	str x3,     [x0, #16 * 11]
	ret

	.global c7_swapcontext
	.text
	.p2align 4
c7_swapcontext:
	stp d8,  d9,  [x0, #16 * 1]
	stp d10, d11, [x0, #16 * 2]
	stp d12, d13, [x0, #16 * 3]
	stp d14, d15, [x0, #16 * 4]
	stp x19, x20, [x0, #16 * 5]
	stp x21, x22, [x0, #16 * 6]
	stp x23, x24, [x0, #16 * 7]
	stp x25, x26, [x0, #16 * 8]
	stp x27, x28, [x0, #16 * 9]
	mov x2,  sp
	stp x30, x2,  [x0, #16 * 10]
	str x29,      [x0, #16 * 11]

	ldp d8,  d9,  [x1, #16 * 1]
	ldp d10, d11, [x1, #16 * 2]
	ldp d12, d13, [x1, #16 * 3]
	ldp d14, d15, [x1, #16 * 4]
	ldp x19, x20, [x1, #16 * 5]
	ldp x21, x22, [x1, #16 * 6]
	ldp x23, x24, [x1, #16 * 7]
	ldp x25, x26, [x1, #16 * 8]
	ldp x27, x28, [x1, #16 * 9]
	ldp x30, x2,  [x1, #16 * 10]
	ldr x29,      [x1, #16 * 11]
	mov sp, x2
	mov x0, 0
	
	ret
