	.text
	.globl func
	.type func, @function
func:
	pushl %ebp
	movl %esp, %ebp
	subl $24, %esp
	pushl %ebx
.entry:
	movl $0, -12(%ebp)
	movl $0, -20(%ebp)
	jmp .while.cond
.return:
	movl -24(%ebp), %ebx
	movl %ebx, %eax
	popl %ebx
	leave
	ret
.while.cond:
	movl -12(%ebp), %ebx
	movl 8(%ebp), %ecx
	movl %ebx, %edx
	cmpl %ecx, %edx
	jl .while.body
	jmp .while.end
.while.body:
	pushl %ecx
	pushl %edx
	call read
	popl %edx
	popl %ecx
	movl %eax, %ebx
	movl %ebx, -8(%ebp)
	movl -20(%ebp), %ebx
	movl -8(%ebp), %ecx
	addl %ecx, %ebx
	movl %ebx, -20(%ebp)
	movl -12(%ebp), %ebx
	addl $1, %ebx
	movl %ebx, -12(%ebp)
	jmp .while.cond
.while.end:
	movl -20(%ebp), %ebx
	movl %ebx, -24(%ebp)
	jmp .return
