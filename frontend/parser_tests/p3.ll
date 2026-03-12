; ModuleID = 'miniC'
source_filename = "miniC"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
entry:
  %a_0 = alloca i32, align 4
  %a_1 = alloca i32, align 4
  %a_2 = alloca i32, align 4
  %b_0 = alloca i32, align 4
  %i_0 = alloca i32, align 4
  %ret_val = alloca i32, align 4
  store i32 %0, ptr %i_0, align 4
  store i32 5, ptr %a_0, align 4
  store i32 5, ptr %b_0, align 4
  br label %while.cond

return:                                           ; preds = %while.end11
  %ret_load = load i32, ptr %ret_val, align 4
  ret i32 %ret_load

while.cond:                                       ; preds = %while.body, %entry
  %b_01 = load i32, ptr %b_0, align 4
  %i_02 = load i32, ptr %i_0, align 4
  %cmp = icmp slt i32 %b_01, %i_02
  br i1 %cmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %b_03 = load i32, ptr %b_0, align 4
  %add = add i32 10, %b_03
  store i32 %add, ptr %a_1, align 4
  %i_05 = load i32, ptr %i_0, align 4
  %mul = mul i32 %b_03, %i_05
  store i32 %mul, ptr %b_0, align 4
  br label %while.cond

while.end:                                        ; preds = %while.cond
  br label %while.cond6

while.cond6:                                      ; preds = %while.body10, %while.end
  %b_07 = load i32, ptr %b_0, align 4
  %i_08 = load i32, ptr %i_0, align 4
  %cmp9 = icmp slt i32 %b_07, %i_08
  br i1 %cmp9, label %while.body10, label %while.end11

while.body10:                                     ; preds = %while.cond6
  %b_012 = load i32, ptr %b_0, align 4
  %mul13 = mul i32 %b_012, 10
  store i32 %mul13, ptr %b_0, align 4
  br label %while.cond6

while.end11:                                      ; preds = %while.cond6
  %b_015 = load i32, ptr %b_0, align 4
  %mul16 = mul i32 5, %b_015
  store i32 %mul16, ptr %ret_val, align 4
  br label %return
}
