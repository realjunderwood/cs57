; ModuleID = 'miniC'
source_filename = "miniC"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
entry:
  %a_0 = alloca i32, align 4
  %b_0 = alloca i32, align 4
  %p_0 = alloca i32, align 4
  %ret_val = alloca i32, align 4
  store i32 %0, ptr %p_0, align 4
  store i32 10, ptr %a_0, align 4
  %p_02 = load i32, ptr %p_0, align 4
  %add = add i32 10, %p_02
  store i32 %add, ptr %b_0, align 4
  %b_03 = load i32, ptr %b_0, align 4
  store i32 %b_03, ptr %ret_val, align 4
  br label %return

return:                                           ; preds = %entry
  %ret_load = load i32, ptr %ret_val, align 4
  ret i32 %ret_load
}
