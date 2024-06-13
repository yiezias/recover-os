
.text
.global switch_to

switch_to:
	pushq 	%rbp
	movq 	%rsp,%rbp
	movq 	%rbx,-8 (%rbp) 
	movq 	%r12,-16(%rbp)
	movq 	%r13,-24(%rbp)
	movq 	%r14,-32(%rbp)
	movq 	%r15,-40(%rbp)
	movq 	%rdi,-48(%rbp)
	movq 	%rsi,-56(%rbp)

	/* 保存当前rbp到PCB中 */
	movq 	%rbp,(%rdi)

	movq 	%rsi,%rdi
	movq 	%rsi,%r12

	/* 将下一个任务rbp从其PCB中取出 */
	movq 	(%r12),%rbp

	movq 	-8 (%rbp),%rbx
	movq 	-16(%rbp),%r12
	movq 	-24(%rbp),%r13
	movq 	-32(%rbp),%r14
	movq 	-40(%rbp),%r15
	movq 	-48(%rbp),%rdi
	movq 	-56(%rbp),%rsi

	leave

	ret

