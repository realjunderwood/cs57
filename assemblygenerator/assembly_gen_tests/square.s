	.text
	.globl func
	.type func, @function
func:
	pushl %ebp
	movl %esp, %ebp
	subl $16, %esp
	pushl %ebx
.entry:
	movl 8(%ebp), %ebx
	imull %ebx, %ebx
	movl %ebx, -12(%ebp)
	movl -12(%ebp), %ebx
	movl %ebx, -16(%ebp)
	jmp .return
.return:
	movl -16(%ebp), %ebx
	movl %ebx, %eax
	popl %ebx
	leave
	ret
