
.text
.global	system_call
.global	system_ret
system_call:
	pushq 	%rbp
	movq	%rsp,%rbp

	movq	%rax,-8 (%rbp)
	movq	%rcx,-16(%rbp)
	movq	%r11,-24(%rbp)

	movq 	%rbx,-32(%rbp)
	movq 	%r12,-48(%rbp)
	movq 	%r13,-56(%rbp)
	movq 	%r14,-64(%rbp)
	movq 	%r15,-72(%rbp)

	subq	$72,%rsp

	leaq	syscall_table(%rip),%rax
	shlq	$3,-8(%rbp)
	addq 	-8(%rbp),%rax
	movq 	%r10,%rcx
	callq 	*(%rax)

	movq	%rax,-8 (%rbp)
system_ret:
	movq	-8 (%rbp),%rax
	movq	-16(%rbp),%rcx
	movq	-24(%rbp),%r11

	movq 	-32(%rbp),%rbx
	movq 	-48(%rbp),%r12
	movq 	-56(%rbp),%r13
	movq 	-64(%rbp),%r14
	movq 	-72(%rbp),%r15

	leave

	sysretq

