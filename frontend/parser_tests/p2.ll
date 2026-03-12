; ModuleID = 'miniC'
source_filename = "miniC"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
entry:
  %a_0 = alloca i32, align 4
  %b_0 = alloca i32, align 4
  %c_0 = alloca i32, align 4
  %i_0 = alloca i32, align 4
  %ret_val = alloca i32, align 4
  store i32 %0, ptr %i_0, align 4
  store i32 10, ptr %a_0, align 4
  store i32 5, ptr %b_0, align 4
  br label %while.cond

return:                                           ; preds = %while.end
  %ret_load = load i32, ptr %ret_val, align 4
  ret i32 %ret_load

while.cond:                                       ; preds = %while.end8, %entry
  %a_01 = load i32, ptr %a_0, align 4
  %i_02 = load i32, ptr %i_0, align 4
  %cmp = icmp slt i32 %a_01, %i_02
  br i1 %cmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  br label %while.cond3

while.end:                                        ; preds = %while.cond
  %a_013 = load i32, ptr %a_0, align 4
  %b_014 = load i32, ptr %b_0, align 4
  %add15 = add i32 %a_013, %b_014
  store i32 %add15, ptr %ret_val, align 4
  br label %return

while.cond3:                                      ; preds = %while.body7, %while.body
  %b_04 = load i32, ptr %b_0, align 4
  %i_05 = load i32, ptr %i_0, align 4
  %cmp6 = icmp slt i32 %b_04, %i_05
  br i1 %cmp6, label %while.body7, label %while.end8

while.body7:                                      ; preds = %while.cond3
  %b_09 = load i32, ptr %b_0, align 4
  %add = add i32 %b_09, 20
  store i32 %add, ptr %b_0, align 4
  %b_010 = load i32, ptr %b_0, align 4
  call void @print(i32 %b_010)
  br label %while.cond3

while.end8:                                       ; preds = %while.cond3
  %b_011 = load i32, ptr %b_0, align 4
  %add12 = add i32 %b_011, 10
  store i32 %add12, ptr %a_0, align 4
  br label %while.cond
}
