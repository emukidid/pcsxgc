#define	r0	0
#define	r1	1
#define	sp	1
#define	r2	2
#define	toc	2
#define	r3	3
#define	r4	4
#define	r5	5
#define	r6	6
#define	r7	7
#define	r8	8
#define	r9	9
#define	r10	10
#define	r11	11
#define	r12	12
#define	r13	13
#define	r14	14
#define	r15	15
#define	r16	16
#define	r17	17
#define	r18	18
#define	r19	19
#define	r20	20
#define	r21	21
#define	r22	22
#define	r23	23
#define	r24	24
#define	r25	25
#define	r26	26
#define	r27	27
#define	r28	28
#define	r29	29
#define	r30	30
#define	r31	31

# avoid touching r13 and r2 to conform to eabi spec

# void recRun(register void (*func)(), register u32 hw1, register u32 hw2)
.text
.align  4
.globl  recRun
recRun:
	# prologue code
	mflr	r0            # move from LR to r0
	stmw	r14, -72(r1)  # store non-volatiles (-72 == -(32-14)*4)
	stw		r0, 4(r1)     # store old LR
	stwu	r1, -80(r1)   # increment and store sp (-80 == -((32-14)*4+8))
	
	# execute code
	mtctr	r3            # move func ptr to ctr
	mr	r31, r4         # save hw1 to r31
	mr	r30, r5         # save hw2 to r30
	bctrl               # branch to ctr (*func)

# void returnPC()
.text
.align  4
.globl  returnPC
returnPC:
	# end code
	lwz		r0, 84(r1)    # re-load LR (84 == (32-14)*4+8+4)
	addi	r1, r1, 80    # increment SP (80 == (32-14)*4+8)
	mtlr	r0            # set LR
	lmw		r14, -72(r1)  # reload non-volatiles (-72 == -((32-14)*4))
	blr                 # return

#// Memory functions that only works with a linear memory
#
#        .text
#        .align  4
#        .globl  dynMemRead8
#dynMemRead8:
#// assumes that memory pointer is in r30
#	addis    r2,r3,-0x1f80
#	srwi.     r4,r2,16
#	bne+     .norm8
#	cmplwi   r2,0x1000
#	blt-     .norm8
#	b        psxHwRead8
#.norm8:
#	clrlwi   r5,r3,3
#	lbzx     r3,r5,r30
#	blr
#
#        .text
#        .align  4
#        .globl  dynMemRead16
#dynMemRead16:
#// assumes that memory pointer is in r30
#	addis    r2,r3,-0x1f80
#	srwi.     r4,r2,16
#	bne+     .norm16
#	cmplwi   r2,0x1000
#	blt-     .norm16
#	b        psxHwRead16
#.norm16:
#	clrlwi   r5,r3,3
#	lhbrx    r3,r5,r30
#	blr
#
#        .text
#        .align  4
#        .globl  dynMemRead32
#dynMemRead32:
#// assumes that memory pointer is in r30
#	addis    r2,r3,-0x1f80
#	srwi.     r4,r2,16
#	bne+     .norm32
#	cmplwi   r2,0x1000
#	blt-     .norm32
#	b        psxHwRead32
#.norm32:
#	clrlwi   r5,r3,3
#	lwbrx    r3,r5,r30
#	blr
#
#/*
#	N P Z
#	0 0 0 X
#-	0 0 1 X
#	1 0 0 X
#	1 0 1 X
#
#P | (!N & Z)
#P | !(N | !Z)
#*/
#
#        .text
#        .align  4
#        .globl  dynMemWrite32
#dynMemWrite32:
#// assumes that memory pointer is in r30
#	addis    r2,r3,-0x1f80
#	srwi.    r5,r2,16
#	bne+     .normw32
#	cmplwi   r2,0x1000
#	blt      .normw32
#	b        psxHwWrite32
#.normw32:
#	mtcrf    0xFF, r3
#	clrlwi   r5,r3,3
#	crandc   0, 2, 0
#	cror     2, 1, 0
#	bne+     .okw32
#	// write test
#	li			r2,0x0130
#	addis    r2,r2,0xfffe
#	cmplw    r3,r2
#	bnelr
#.okw32:
#	stwbrx   r4,r5,r30
#	blr

