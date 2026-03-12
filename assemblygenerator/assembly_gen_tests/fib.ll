; ModuleID = 'miniC'
source_filename = "miniC"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
entry:
  %a1_0 = alloca i32, align 4
  %a2_0 = alloca i32, align 4
  %c_0 = alloca i32, align 4
  %n_0 = alloca i32, align 4
  %t_0 = alloca i32, align 4
  %ret_val = alloca i32, align 4
  store i32 %0, ptr %n_0, align 4
  store i32 1, ptr %a1_0, align 4
  store i32 1, ptr %a2_0, align 4
  store i32 0, ptr %c_0, align 4
  br label %while.cond

return:                                           ; preds = %while.end
  ret i32 1

while.cond:                                       ; preds = %while.body, %entry
  %c_01 = load i32, ptr %c_0, align 4
  %n_02 = load i32, ptr %n_0, align 4
  %cmp = icmp slt i32 %c_01, %n_02
  br i1 %cmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %a1_03 = load i32, ptr %a1_0, align 4
  call void @print(i32 %a1_03)
  %c_04 = load i32, ptr %c_0, align 4
  %add = add i32 %c_04, 1
  store i32 %add, ptr %c_0, align 4
  %a2_05 = load i32, ptr %a2_0, align 4
  store i32 %a2_05, ptr %t_0, align 4
  %add8 = add i32 %a1_03, %a2_05
  store i32 %add8, ptr %a2_0, align 4
  %t_09 = load i32, ptr %t_0, align 4
  store i32 %t_09, ptr %a1_0, align 4
  br label %while.cond

while.end:                                        ; preds = %while.cond
  store i32 1, ptr %ret_val, align 4
  br label %return
}
