; ModuleID = 'miniC'
source_filename = "miniC"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
entry:
  %n_0 = alloca i32, align 4
  %sq_0 = alloca i32, align 4
  %ret_val = alloca i32, align 4
  store i32 %0, ptr %n_0, align 4
  %n_01 = load i32, ptr %n_0, align 4
  %mul = mul i32 %n_01, %n_01
  store i32 %mul, ptr %sq_0, align 4
  %sq_03 = load i32, ptr %sq_0, align 4
  store i32 %sq_03, ptr %ret_val, align 4
  br label %return

return:                                           ; preds = %entry
  %ret_load = load i32, ptr %ret_val, align 4
  ret i32 %ret_load
}
