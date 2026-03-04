; ModuleID = 'miniC'
source_filename = "miniC"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
entry:
  %a_0 = alloca i32, align 4
  %i_0 = alloca i32, align 4
  %max_0 = alloca i32, align 4
  %n_0 = alloca i32, align 4
  %ret_val = alloca i32, align 4
  store i32 %0, ptr %n_0, align 4
  store i32 0, ptr %i_0, align 4
  store i32 0, ptr %max_0, align 4
  br label %while.cond

return:                                           ; preds = %while.end
  %ret_load = load i32, ptr %ret_val, align 4
  ret i32 %ret_load

while.cond:                                       ; preds = %if.else, %entry
  %i_01 = load i32, ptr %i_0, align 4
  %n_02 = load i32, ptr %n_0, align 4
  %cmp = icmp slt i32 %i_01, %n_02
  br i1 %cmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %read_val = call i32 @read()
  store i32 %read_val, ptr %a_0, align 4
  %a_03 = load i32, ptr %a_0, align 4
  %max_04 = load i32, ptr %max_0, align 4
  %cmp5 = icmp sgt i32 %a_03, %max_04
  br i1 %cmp5, label %if.then, label %if.else

while.end:                                        ; preds = %while.cond
  %max_08 = load i32, ptr %max_0, align 4
  store i32 %max_08, ptr %ret_val, align 4
  br label %return

if.then:                                          ; preds = %while.body
  %a_06 = load i32, ptr %a_0, align 4
  store i32 %a_06, ptr %max_0, align 4
  br label %if.else

if.else:                                          ; preds = %if.then, %while.body
  %i_07 = load i32, ptr %i_0, align 4
  %add = add i32 %i_07, 1
  store i32 %add, ptr %i_0, align 4
  br label %while.cond
}
