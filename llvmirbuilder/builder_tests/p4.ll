; ModuleID = 'miniC'
source_filename = "miniC"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
entry:
  %a_0 = alloca i32, align 4
  %i_0 = alloca i32, align 4
  %n_0 = alloca i32, align 4
  %sum_0 = alloca i32, align 4
  %ret_val = alloca i32, align 4
  store i32 %0, ptr %n_0, align 4
  store i32 0, ptr %i_0, align 4
  store i32 0, ptr %sum_0, align 4
  %n_01 = load i32, ptr %n_0, align 4
  %cmp = icmp sle i32 %n_01, 0
  br i1 %cmp, label %if.then, label %if.else

return:                                           ; preds = %if.merge, %if.then
  %ret_load = load i32, ptr %ret_val, align 4
  ret i32 %ret_load

if.then:                                          ; preds = %entry
  store i32 0, ptr %ret_val, align 4
  br label %return

if.else:                                          ; preds = %entry
  br label %while.cond

while.cond:                                       ; preds = %while.body, %if.else
  %i_02 = load i32, ptr %i_0, align 4
  %n_03 = load i32, ptr %n_0, align 4
  %cmp4 = icmp slt i32 %i_02, %n_03
  br i1 %cmp4, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %read_val = call i32 @read()
  store i32 %read_val, ptr %a_0, align 4
  %sum_05 = load i32, ptr %sum_0, align 4
  %a_06 = load i32, ptr %a_0, align 4
  %add = add i32 %sum_05, %a_06
  store i32 %add, ptr %sum_0, align 4
  %i_07 = load i32, ptr %i_0, align 4
  %add8 = add i32 %i_07, 1
  store i32 %add8, ptr %i_0, align 4
  br label %while.cond

while.end:                                        ; preds = %while.cond
  br label %if.merge

if.merge:                                         ; preds = %while.end
  %sum_09 = load i32, ptr %sum_0, align 4
  store i32 %sum_09, ptr %ret_val, align 4
  br label %return
}
