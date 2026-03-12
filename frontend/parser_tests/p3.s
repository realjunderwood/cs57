	.text
	.globl func
	.type func, @function
func:
	pushl %ebp
	movl %esp, %ebp
	subl $28, %esp
	pushl %ebx
.entry:
	movl $5, -8(%ebp)
	movl $5, -20(%ebp)
	jmp .while.cond
.return:
	movl -28(%ebp), %ebx
	movl %ebx, %eax
	popl %ebx
	leave
	ret
.while.cond:
	movl -20(%ebp), %ebx
	movl 8(%ebp), %ecx
	movl %ebx, %edx
	cmpl %ecx, %edx
	jl .while.body
	jmp .while.end
.while.body:
	movl -20(%ebp), %ebx
	movl $10, %ecx
	addl %ebx, %ecx
	movl %ecx, -12(%ebp)
	movl 8(%ebp), %ecx
	imull %ecx, %ebx
	movl %ebx, -20(%ebp)
	jmp .while.cond
.while.end:
	jmp .while.cond6
.while.cond6:
	movl -20(%ebp), %ebx
	movl 8(%ebp), %ecx
	movl %ebx, %edx
	cmpl %ecx, %edx
	jl .while.body10
	jmp .while.end11
.while.body10:
	movl -20(%ebp), %ebx
	imull $10, %ebx
	movl %ebx, -20(%ebp)
	jmp .while.cond6
.while.end11:
	movl -20(%ebp), %ebx
	movl $5, %ecx
	imull %ebx, %ecx
	movl %ecx, -28(%ebp)
	jmp .return
