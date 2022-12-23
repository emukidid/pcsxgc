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
	mtctr	r3          # move func ptr to ctr
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
	blr                   # return

