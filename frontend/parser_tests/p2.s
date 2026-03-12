	.text
	.globl func
	.type func, @function
func:
	pushl %ebp
	movl %esp, %ebp
	subl $24, %esp
	pushl %ebx
.entry:
	movl $10, -8(%ebp)
	movl $5, -12(%ebp)
	jmp .while.cond
.return:
	movl -24(%ebp), %ebx
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
	jmp .while.cond3
.while.end:
	movl -8(%ebp), %ebx
	movl -12(%ebp), %ecx
	addl %ecx, %ebx
	movl %ebx, -24(%ebp)
	jmp .return
.while.cond3:
	movl -12(%ebp), %ebx
	movl 8(%ebp), %ecx
	movl %ebx, %edx
	cmpl %ecx, %edx
	jl .while.body7
	jmp .while.end8
.while.body7:
	movl -12(%ebp), %ebx
	addl $20, %ebx
	movl %ebx, -12(%ebp)
	movl -12(%ebp), %ebx
	pushl %ecx
	pushl %edx
	pushl %ebx
	call print
	addl $4, %esp
	popl %edx
	popl %ecx
	jmp .while.cond3
.while.end8:
	movl -12(%ebp), %ebx
	addl $10, %ebx
	movl %ebx, -8(%ebp)
	jmp .while.cond
