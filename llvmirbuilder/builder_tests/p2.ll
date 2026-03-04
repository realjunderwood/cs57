; ModuleID = 'miniC'
source_filename = "miniC"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
entry:
  %a_0 = alloca i32, align 4
  %a_1 = alloca i32, align 4
  %b_0 = alloca i32, align 4
  %p_0 = alloca i32, align 4
  %ret_val = alloca i32, align 4
  store i32 %0, ptr %p_0, align 4
  store i32 0, ptr %a_0, align 4
  %p_01 = load i32, ptr %p_0, align 4
  %cmp = icmp slt i32 %p_01, 0
  br i1 %cmp, label %if.then, label %if.else

return:                                           ; preds = %if.merge
  %ret_load = load i32, ptr %ret_val, align 4
  ret i32 %ret_load

if.then:                                          ; preds = %entry
  store i32 10, ptr %a_0, align 4
  br label %if.merge

if.else:                                          ; preds = %entry
  store i32 2, ptr %a_1, align 4
  store i32 0, ptr %b_0, align 4
  br label %while.cond

while.cond:                                       ; preds = %while.body, %if.else
  %b_02 = load i32, ptr %b_0, align 4
  %p_03 = load i32, ptr %p_0, align 4
  %cmp4 = icmp slt i32 %b_02, %p_03
  br i1 %cmp4, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %b_05 = load i32, ptr %b_0, align 4
  %add = add i32 %b_05, 2
  store i32 %add, ptr %b_0, align 4
  br label %while.cond

while.end:                                        ; preds = %while.cond
  br label %if.merge

if.merge:                                         ; preds = %while.end, %if.then
  %a_07 = load i32, ptr %a_0, align 4
  %b_08 = load i32, ptr %b_0, align 4
  %add9 = add i32 %a_07, %b_08
  store i32 %add9, ptr %ret_val, align 4
  br label %return
}
