; ModuleID = 'miniC'
source_filename = "miniC"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
entry:
  %n_0 = alloca i32, align 4
  %res_0 = alloca i32, align 4
  %ret_val = alloca i32, align 4
  store i32 %0, ptr %n_0, align 4
  %n_01 = load i32, ptr %n_0, align 4
  store i32 %n_01, ptr %res_0, align 4
  br label %while.cond

return:                                           ; preds = %while.end
  %ret_load = load i32, ptr %ret_val, align 4
  ret i32 %ret_load

while.cond:                                       ; preds = %while.body, %entry
  %res_02 = load i32, ptr %res_0, align 4
  %cmp = icmp sgt i32 %res_02, 1
  br i1 %cmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %res_03 = load i32, ptr %res_0, align 4
  %sub = sub i32 %res_03, 2
  store i32 %sub, ptr %res_0, align 4
  br label %while.cond

while.end:                                        ; preds = %while.cond
  %res_04 = load i32, ptr %res_0, align 4
  store i32 %res_04, ptr %ret_val, align 4
  br label %return
}
