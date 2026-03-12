	.text
	.globl func
	.type func, @function
func:
	pushl %ebp
	movl %esp, %ebp
	subl $20, %esp
	pushl %ebx
.entry:
	movl $1, -8(%ebp)
	movl $1, -16(%ebp)
	jmp .while.cond
.return:
	movl -20(%ebp), %ebx
	movl %ebx, %eax
	popl %ebx
	leave
	ret
.while.cond:
	movl -8(%ebp), %ebx
	movl 8(%ebp), %ecx
	movl %ebx, %edx
	cmpl %ecx, %edx
	jl .while.body
	jmp .while.end
.while.body:
	movl -8(%ebp), %ebx
	addl $1, %ebx
	movl %ebx, -8(%ebp)
	movl -16(%ebp), %ebx
	movl -8(%ebp), %ecx
	imull %ecx, %ebx
	movl %ebx, -16(%ebp)
	jmp .while.cond
.while.end:
	movl -16(%ebp), %ebx
	movl %ebx, -20(%ebp)
	jmp .return
