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
	movl %ebx, -12(%ebp)
	jmp .while.cond
.return:
	movl -16(%ebp), %ebx
	movl %ebx, %eax
	popl %ebx
	leave
	ret
.while.cond:
	movl -12(%ebp), %ebx
	movl %ebx, %ecx
	cmpl $1, %ecx
	jg .while.body
	jmp .while.end
.while.body:
	movl -12(%ebp), %ebx
	subl $2, %ebx
	movl %ebx, -12(%ebp)
	jmp .while.cond
.while.end:
	movl -12(%ebp), %ebx
	movl %ebx, -16(%ebp)
	jmp .return
