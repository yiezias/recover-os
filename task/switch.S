
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

	/* 置换pml4，为什么在这里置换，
	 * 因为调用schedule的不一定是时钟中断，
	 * 此时栈可能存在于用户空间，
	 * 如果下一个任务没有映射该空间就会出错。 */
	movq 	+8(%rsi),%r13
	movq 	$0xffff800000000000,%r12
	subq 	%r12,%r13
	movq 	%r13,%cr3

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

