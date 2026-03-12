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
  br label %while.cond

return:                                           ; preds = %while.end
  %ret_load = load i32, ptr %ret_val, align 4
  ret i32 %ret_load

while.cond:                                       ; preds = %while.body, %entry
  %i_01 = load i32, ptr %i_0, align 4
  %n_02 = load i32, ptr %n_0, align 4
  %cmp = icmp slt i32 %i_01, %n_02
  br i1 %cmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %read_val = call i32 @read()
  store i32 %read_val, ptr %a_0, align 4
  %sum_03 = load i32, ptr %sum_0, align 4
  %a_04 = load i32, ptr %a_0, align 4
  %add = add i32 %sum_03, %a_04
  store i32 %add, ptr %sum_0, align 4
  %i_05 = load i32, ptr %i_0, align 4
  %add6 = add i32 %i_05, 1
  store i32 %add6, ptr %i_0, align 4
  br label %while.cond

while.end:                                        ; preds = %while.cond
  %sum_07 = load i32, ptr %sum_0, align 4
  store i32 %sum_07, ptr %ret_val, align 4
  br label %return
}
