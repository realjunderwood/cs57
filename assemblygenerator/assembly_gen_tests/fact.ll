; ModuleID = 'miniC'
source_filename = "miniC"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
entry:
  %i_0 = alloca i32, align 4
  %n_0 = alloca i32, align 4
  %prod_0 = alloca i32, align 4
  %ret_val = alloca i32, align 4
  store i32 %0, ptr %n_0, align 4
  store i32 1, ptr %i_0, align 4
  store i32 1, ptr %prod_0, align 4
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
  %i_03 = load i32, ptr %i_0, align 4
  %add = add i32 %i_03, 1
  store i32 %add, ptr %i_0, align 4
  %prod_04 = load i32, ptr %prod_0, align 4
  %i_05 = load i32, ptr %i_0, align 4
  %mul = mul i32 %prod_04, %i_05
  store i32 %mul, ptr %prod_0, align 4
  br label %while.cond

while.end:                                        ; preds = %while.cond
  %prod_06 = load i32, ptr %prod_0, align 4
  store i32 %prod_06, ptr %ret_val, align 4
  br label %return
}
