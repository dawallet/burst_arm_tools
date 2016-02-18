@ Implementation of Shabal: ARM, little-endian, ARM-only opcodes.
@
@ This implementation uses only ARM-state opcodes. It is meant to run
@ on pre-Thumb ARM systems.
@
@ WARNING: THIS IMPLEMENTATION DOES NOT SUPPORT INTERWORKING ON ARMv4T.
@ Interworking is about supporting mixed Thumb and ARM code. An ARM-only
@ function may support interworking by returning to the caller with a
@ "bx" opcode. The point of this implementation is to support pre-thumb
@ processors (e.g. the StrongARM) which do not have a "bx" opcode. In
@ order to make this code interwork-compliant on ARMv4T, replace each:
@    ldmfd   sp!, { xxxx, pc }
@ with this sequence:
@    ldmfd   sp!, { xxxx, lr }
@    bx      lr
@ where "xxxx" is the specification of some registers. This is needed in
@ the three public functions (shabal_init(), shabal() and shabal_close()).
@
@ On newer architectures (ARMv5 and beyond), the ldmfd opcode was amended
@ to support interworking transparently. The issue may arise only when
@ trying to use this code with callers using Thumb code on an ARMv4T
@ processor, such as the ARM7TDMI or ARM920T.
@
@ This code is compatible with APCS/ATPCS (old-style call conventions)
@ and AAPCS (new-style, "EABI" conventions). The AAPCS mandates 8-byte
@ stack alignment for calls to public functions, which is why shabal()
@ and shabal_close() appear to needlessly save the "r12" scratch
@ register: this keeps the stack aligned for the call to memcpy().
@
@ -----------------------------------------------------------------------
@ (c) 2010 SAPHIR project. This software is provided 'as-is', without
@ any epxress or implied warranty. In no event will the authors be held
@ liable for any damages arising from the use of this software.
@
@ Permission is granted to anyone to use this software for any purpose,
@ including commercial applications, and to alter it and redistribute it
@ freely, subject to no restriction.
@
@ Technical remarks and questions can be addressed to:
@ <thomas.pornin@cryptolog.com>
@ -----------------------------------------------------------------------

	.file   "shabal_tiny_armel_armonly.s"
	.text

@ shabal_inner(sc)
@    sc   pointer to the context structure
@
@ This function actually expects its argument in v1, not a1. Registers a1-a4
@ may be modified. Also, the input words must have already been byteswapped.
@
	.align  2
	.code   32
	.type   shabal_inner, %function
shabal_inner:
	stmfd   sp!, {v1 - v5, v7, v8, lr} @ Save registers.
	sub     sp, sp, #256           @ Allocate Bx[] buffer.

	@ Throughout the function, ip points to the current Bx[] word.
	mov     ip, sp

	mov     a4, #16
	add     v2, v1, #116           @ Set v2 to &B[0].
.L0m:
	ldr     a1, [v1], #4           @ Load next M word.
	ldr     a2, [v2], #4           @ Load next B word.
	add     a1, a1, a2             @ Add M word to B word.
	mov     a1, a1, ror #15        @ Rotate B word.
	str     a1, [ip], #4           @ Store new B word (in Bx[]).
	subs    a4, a4, #1             @ Decrement loop counter.
	bne     .L0m                   @ Loop 16 times.

	@ At that point, v1 points to offset 64 and v2 points to offset
	@ 180 (i.e. &C[0]).

	ldr     a2, [v1, #184]         @ Load Whigh.
	ldr     a1, [v1, #180]         @ Load Wlow.
	ldr     a4, [v1, #8]           @ Load A[1].
	ldr     a3, [v1, #4]!          @ Load A[0], update v1.
	eor     a2, a2, a4             @ Xor Whigh with A[1].
	eor     a1, a1, a3             @ Xor Wlow with A[0].
	str     a2, [v1, #4]           @ Store A[1].
	str     a1, [v1]               @ Store A[0].

	@ Conventions:
	@   v1   points to A[0], updated
	@   v2   points to context structure
	@   v3   points to C[0]
	@   v4   points to B[0]
	@   a1   contains previous A word (ap)
	@   a3   main counter, 56 downto 8.

	ldr     a1, [v1, #44]          @ Load ap.
	sub     v2, v1, #68
	add     v3, v1, #112
	add     v4, v1, #48
	mov     a3, #56

.L1m:
	and     a4, a3, #15            @ Compute index for C word.
	mov     a1, a1, ror #17        @ Rotate left ap by 15.
	ldr     a4, [v3, a4, lsl #2]   @ Load C word.
	add     a1, a1, a1, lsl #2     @ Multiply by 5.
	ldr     a2, [v1]               @ Load A word.
	eor     a1, a1, a4             @ Xor C word.
	ldr     r14, [ip, #-12]        @ Load Bx[u + 13].
	eor     a1, a1, a2             @ Xor A word.
	ldr     v7, [ip, #-28]         @ Load Bx[u + 9].
	add     a1, a1, a1, lsl #1     @ Multiply by 3.
	ldr     v8, [ip, #-40]         @ Load Bx[u + 6].
	eor     a1, a1, r14            @ Xor Bx[u + 13].
	rsb     v5, a3, #8             @ Compute index for M word (part 1).
	bic     v7, v7, v8             @ Compute (Bx[u + 9] & ~Bx[u + 6]).
	and     v5, v5, #15            @ Compute index for M word (part 2).
	eor     a1, a1, v7             @ Xor (Bx[u + 9] & ~Bx[u + 6]).
	ldr     v5, [v2, v5, lsl #2]   @ Load M word.
	ldr     a2, [ip, #-64]         @ Load Bx[u].
	eor     a1, a1, v5             @ Xor M word.
	mvn     a2, a2, ror #31        @ Compute ~rol(Bx[u], 1).
	str     a1, [v1], #4           @ Store new A word and update v1.
	eor     a2, a2, a1             @ Xor with new A word.
	cmp     v1, v4                 @ Test end of A[] buffer.
	str     a2, [ip], #4           @ Store new B word and update ip.
	sub     a3, a3, #1             @ Decrement main counter.
	bne     .L1m
	sub     v1, v1, #48            @ Adjust pointer for A words.
	cmp     a3, #8                 @ Test end of loop.
	bne     .L1m

	@ At that point, v1 is back to &A[0]. In the loop below, v1
	@ is updated through the whole A[] array.

.L2m:
	sub     a3, v1, v2             @ Compute counter.
	ldr     a1, [v1]               @ Load A word.
	sub     a4, a3, #8             @ Index for second C word (part 1).
	ldr     a2, [v1, #124]         @ Load third C word.
	and     a4, a4, #60            @ Index for second C word (part 2).
	sub     r14, a3, #24           @ Index for first C word (part 1).
	ldr     a4, [v3, a4]           @ Load second C word.
	and     r14, r14, #60          @ Index for first C word (part 2).
	ldr     r14, [v3, r14]         @ Load first C word.
	add     a1, a1, a2             @ Add third C word.
	add     a4, a4, r14            @ Add first to second C word.
	add     a1, a1, a4             @ Add first and second C words.
	str     a1, [v1], #4           @ Store new A word, update v1.
	cmp     v1, v4                 @ Test end of loop.
	bne     .L2m

	sub     ip, ip, #64
	mov     v5, v3
.L3m:
	ldr     a1, [v3]               @ Load C word.
	ldr     a2, [v2], #4           @ Load M word, update v2.
	ldr     a3, [ip], #4           @ Load B word (from Bx[]), update ip.
	sub     a1, a1, a2             @ Compute new B word.
	str     a1, [v4], #4           @ Store new B word, update v4.
	str     a3, [v3], #4           @ Store new C word, update v3.
	cmp     v4, v5
	bne     .L3m

	add     sp, sp, #256
	ldmfd   sp!, {v1 - v5, v7, v8, pc} @ Return.
	.size   shabal_inner, .-shabal_inner

@ shabal_close(sc, ub, n, dst)
@    sc    pointer to context structure
@    ub    extra bits
@    n     number of extra bits
@    dst   destination buffer
@
	.align  2
	.global shabal_close
	.code   32
	.type   shabal_close, %function
shabal_close:
	stmfd   sp!, {v1 - v4, r12, lr} @ Save registers.

	@ Compute extra byte into a2.
	mov     v1, #0x7F
	bic     a2, a2, v1, lsr a3
	mov     v1, #0x80
	orr     a2, a2, v1, lsr a3

	ldr     v2, [a1, #64]          @ Read sc->ptr into v2.
	mov     v1, a1                 @ Save context pointer.
	strb    a2, [a1, v2]!          @ Store extra byte, update a1.

	@ The loop below clears the rest of the buffer. It actually
	@ clears one extra byte (lsb of the ptr field), which is harmless.

	mov     a3, #0                 @ Clear a3.
	rsb     v2, v2, #64            @ Compute number of zeros, +1.
.L0c:
	subs    v2, v2, #1             @ Decrement loop counter.
	strb    a3, [a1, #1]!          @ Clear one more byte, update a1.
	bne     .L0c                   @ Loop until buffer end.

	mov     v4, a4                 @ Save destination buffer.
	mov     v3, #4                 @ Set loop counter.
.L1c:
	bl      shabal_inner           @ Call inner function.
	subs    v3, v3, #1             @ Decrement loop counter.
	bne     .L1c                   @ Loop 4 times.

	mov     a1, v4                 @ Set destination pointer.
	ldr     a3, [v1, #252]         @ Load output size (in bits).
	mov     a3, a3, lsr #3         @ Convert output size to bytes.
	add     a2, v1, #244           @ Compute source (part 1).
	sub     a2, a2, a3             @ Compute source (part 2).
	bl      memcpy(PLT)            @ Copy result.

	ldmfd   sp!, {v1 - v4, r12, pc} @ Restore registers and return.
	.size   shabal_close, .-shabal_close

@ shabal_init(sc, out_size)
@    sc         pointer to context structure
@    out_size   hash output size (in bits)
@
	.align  2
	.global shabal_init
	.code   32
	.type   shabal_init, %function
shabal_init:
	stmfd   sp!, {v1 - v2, lr}     @ Save registers.

	str     a2, [a1, #252]         @ Store output size in context.
	mov     a4, #16                @ Counter for first loop.
.L0i:
	str     a2, [a1], #4           @ Store next input word.
	subs    a4, a4, #1             @ Decrement loop counter.
	add     a2, a2, #1             @ Increment input word.
	bne     .L0i                   @ Loop 16 times.

	mov     v2, a2                 @ Save current input word.
	mov     a4, #45                @ Counter for second loop.
	mov     a2, #0
.L1i:
	str     a2, [a1], #4           @ Clear next word (post increment).
	subs    a4, a4, #1             @ Decrement loop counter.
	bne     .L1i                   @ Loop 45 times.

	sub     a2, a2, #1             @ Set a2 to -1.
	str     a2, [a1], #4           @ Store Wlow.
	str     a2, [a1], #4           @ Store Whigh.

	sub     v1, a1, #252           @ Compute pointer to structure.
	bl      shabal_inner           @ Call inner function.

	mov     a4, #16                @ Counter for third loop.
.L2i:
	str     v2, [v1], #4           @ Store next input word.
	subs    a4, a4, #1             @ Decrement loop counter.
	add     v2, v2, #1             @ Increment input word.
	bne     .L2i                   @ Loop 16 times.
	sub     v1, v1, #64            @ Recover pointer to structure.

	mov     a2, #0
	str     a2, [v1, #244]         @ Set Wlow to 0.
	str     a2, [v1, #248]         @ Set Whigh to 0.
	bl      shabal_inner           @ Call inner function.

	mov     a2, #1
	str     a2, [v1, #244]         @ Set Wlow to 1.

	ldmfd   sp!, {v1 - v2, pc}     @ Restore registers and return.
	.size   shabal_init, .-shabal_init

@ shabal(sc, data, len)
@    sc     pointer to context structure
@    data   pointer to input data
@    len    input data length (in bytes)
@
	.align  2
	.global shabal
	.code   32
	.type   shabal, %function
shabal:
	stmfd   sp!, {r0 - r7, r12, lr} @ Save some registers.
	ldmfd   sp!, {v1 - v4}         @ Reload v1..v3 with the arguments.

	@ Conventions:
	@   v1   pointer to context structure
	@   v2   pointer to data
	@   v3   remaining data length
	@   v4   cache of sc->ptr

	ldr     v4, [v1, #64]          @ Load sc->ptr into v4.
.L0s:
	cmp     v3, #0                 @ Check whether some data remains.
	bne     .L2s
.L1s:
	str     v4, [v1, #64]          @ Store sc->ptr.
	ldmfd	sp!, {v1 - v4, r12, pc} @ Restore registers and return.
.L2s:
	rsb     a3, v4, #64            @ Compute room in sc->buf.
	cmp     v3, a3                 @ See if enough remaining data.
	movlo   a3, v3                 @ Set a3 = min(64-ptr, len).
	sub     v3, v3, a3             @ Decrement len.
	mov     a2, v2                 @ Get source in a2.
	add     v2, v2, a3             @ Increment data pointer.
	add     a1, v1, v4             @ Compute destination in a1.
	add     v4, v4, a3             @ Update ptr.
	bl      memcpy(PLT)            @ Copy data into buffer.
	cmp     v4, #64                @ If not full buffer, then exit.
	bne     .L1s
	bl      shabal_inner           @ Call inner function.
	ldr     a1, [v1, #244]         @ Load Wlow.
	ldr     a2, [v1, #248]         @ Load Whigh.
	adds    a1, a1, #1             @ Increment Wlow.
	addeq   a2, a2, #1             @ Increment Whigh if Wlow == 0.
	str     a1, [v1, #244]         @ Store Wlow.
	str     a2, [v1, #248]         @ Store Whigh.
	mov     v4, #0                 @ Clear ptr.
	b       .L0s                   @ Loop.
	.size	shabal, .-shabal
