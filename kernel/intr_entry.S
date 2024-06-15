
// 定义 intr_entry_table

#define SAVE_ALL                \
	pushq	%rbp            ;\
	movq	%rsp,%rbp       ;\
	movq	%rdi,-8(%rbp)  ;\
	movq	%rsi,-16(%rbp)  ;\
	movw	%ds ,-18(%rbp)   ;\
	movw	%es ,-20(%rbp)   ;\
	movw	%gs ,-22(%rbp)   ;\
	movw	%fs ,-24(%rbp)   ;\
	movq	%rax,-32(%rbp)  ;\
	movq	%rbx,-40(%rbp)  ;\
	movq	%rcx,-48(%rbp)  ;\
	movq	%rdx,-56(%rbp)  ;\
	movq	%r8 ,-64(%rbp)  ;\
	movq	%r9 ,-72(%rbp)  ;\
	movq	%r10,-80(%rbp)  ;\
	movq	%r11,-88(%rbp)  ;\
	movq	%r12,-96(%rbp)  ;\
	movq	%r13,-104(%rbp) ;\
	movq	%r14,-112(%rbp) ;\
	movq	%r15,-120(%rbp) ;\
	subq	$120,%rsp 	;



#define MK_INTR_ETY(NR, EC)			\
	.text					;\
	.global intr##NR##_entry		;\
	intr##NR##_entry:			;\
	EC					;\
	SAVE_ALL				;\
	movb	$0x20,%al			;\
	out	%al,$0xa0			;\
	out	%al,$0x20			;\
	movq	$##NR##,%rdi			;\
	movq	%rbp,%rsi			;\
	addq	$48,%rsi			;\
	leaq	intr_handle_table(%rip),%rax	;\
	addq 	$##NR##*8,%rax;\
	callq	*(%rax)				;\
	leaq	intr_exit(%rip),%rax		;\
	jmp	*%rax				;\
	.data					;\
	.quad	intr##NR##_entry


.text
.global intr_exit
intr_exit:
    movq	-8(%rbp) ,%rdi
    movq	-16(%rbp) ,%rsi
    movw	-18(%rbp)  ,%ds 
    movw	-20(%rbp)  ,%es 
    movw	-22(%rbp)  ,%gs 
    movw	-24(%rbp)  ,%fs 
    movq	-32(%rbp) ,%rax
    movq	-40(%rbp) ,%rbx
    movq	-48(%rbp) ,%rcx
    movq	-56(%rbp) ,%rdx
    movq	-64(%rbp) ,%r8 
    movq	-72(%rbp) ,%r9 
    movq	-80(%rbp) ,%r10
    movq	-88(%rbp) ,%r11
    movq	-96(%rbp) ,%r12
    movq	-104(%rbp),%r13
    movq	-112(%rbp),%r14
    movq	-120(%rbp),%r15
    leave
    addq	$8,%rsp
    iretq


.data
.global intr_entry_table
intr_entry_table:


MK_INTR_ETY(0x00,pushq	$0)
MK_INTR_ETY(0x01,pushq	$0)
MK_INTR_ETY(0x02,pushq	$0)
MK_INTR_ETY(0x03,pushq	$0)
MK_INTR_ETY(0x04,pushq	$0)
MK_INTR_ETY(0x05,pushq	$0)
MK_INTR_ETY(0x06,pushq	$0)
MK_INTR_ETY(0x07,pushq	$0)
MK_INTR_ETY(0x08,nop	 )
MK_INTR_ETY(0x09,pushq	$0)
MK_INTR_ETY(0x0a,nop 	 )
MK_INTR_ETY(0x0b,nop 	 )
MK_INTR_ETY(0x0c,pushq	$0)
MK_INTR_ETY(0x0d,nop 	 )
MK_INTR_ETY(0x0e,nop 	 )
MK_INTR_ETY(0x0f,pushq	$0)
MK_INTR_ETY(0x10,pushq	$0)
MK_INTR_ETY(0x11,nop 	 )
MK_INTR_ETY(0x12,pushq	$0)
MK_INTR_ETY(0x13,pushq	$0)
MK_INTR_ETY(0x14,pushq	$0)
MK_INTR_ETY(0x15,pushq	$0)
MK_INTR_ETY(0x16,pushq	$0)
MK_INTR_ETY(0x17,pushq	$0)
MK_INTR_ETY(0x18,nop 	 )
MK_INTR_ETY(0x19,pushq	$0)
MK_INTR_ETY(0x1a,nop 	 )
MK_INTR_ETY(0x1b,nop 	 )
MK_INTR_ETY(0x1c,pushq	$0)
MK_INTR_ETY(0x1d,nop 	 )
MK_INTR_ETY(0x1e,nop 	 )
MK_INTR_ETY(0x1f,pushq	$0)
MK_INTR_ETY(0x20,pushq	$0)
MK_INTR_ETY(0x21,pushq	$0)
MK_INTR_ETY(0x22,pushq	$0)
MK_INTR_ETY(0x23,pushq	$0)
MK_INTR_ETY(0x24,pushq	$0)
MK_INTR_ETY(0x25,pushq	$0)
MK_INTR_ETY(0x26,pushq	$0)
MK_INTR_ETY(0x27,pushq	$0)
MK_INTR_ETY(0x28,pushq	$0)
MK_INTR_ETY(0x29,pushq	$0)
MK_INTR_ETY(0x2a,pushq	$0)
MK_INTR_ETY(0x2b,pushq	$0)
MK_INTR_ETY(0x2c,pushq	$0)
MK_INTR_ETY(0x2d,pushq	$0)
MK_INTR_ETY(0x2e,pushq	$0)
MK_INTR_ETY(0x2f,pushq	$0)


