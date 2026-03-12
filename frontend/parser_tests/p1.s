	.text
	.globl func
	.type func, @function
func:
	pushl %ebp
	movl %esp, %ebp
	subl $20, %esp
	pushl %ebx
.entry:
	movl $5, -8(%ebp)
	movl $2, -12(%ebp)
	movl 8(%ebp), %ebx
	movl $5, %ecx
	cmpl %ebx, %ecx
	jl .if.then
	jmp .if.else
.return:
	movl $1, %eax
	popl %ebx
	leave
	ret
.if.then:
	jmp .while.cond
.if.else:
	movl 8(%ebp), %ebx
	movl $2, %ecx
	cmpl %ebx, %ecx
	jl .if.then12
	jmp .if.else13
.while.cond:
	movl -12(%ebp), %ebx
	movl 8(%ebp), %ecx
	movl %ebx, %edx
	cmpl %ecx, %edx
	jl .while.body
	jmp .while.end
.while.body:
	movl -12(%ebp), %ebx
	addl $20, %ebx
	movl %ebx, -12(%ebp)
	jmp .while.cond
.while.end:
	movl -12(%ebp), %ebx
	movl $10, %ecx
	addl %ebx, %ecx
	movl %ecx, -8(%ebp)
	jmp .if.merge
.if.then12:
	movl $5, -12(%ebp)
	jmp .if.else13
.if.else13:
	jmp .if.merge
.if.merge:
	movl $1, -20(%ebp)
	jmp .return
