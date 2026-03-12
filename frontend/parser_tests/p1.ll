; ModuleID = 'miniC'
source_filename = "miniC"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
entry:
  %a_0 = alloca i32, align 4
  %b_0 = alloca i32, align 4
  %i_0 = alloca i32, align 4
  %ret_val = alloca i32, align 4
  store i32 %0, ptr %i_0, align 4
  store i32 5, ptr %a_0, align 4
  store i32 2, ptr %b_0, align 4
  %i_02 = load i32, ptr %i_0, align 4
  %cmp = icmp slt i32 5, %i_02
  br i1 %cmp, label %if.then, label %if.else

return:                                           ; preds = %if.merge
  ret i32 1

if.then:                                          ; preds = %entry
  br label %while.cond

if.else:                                          ; preds = %entry
  %i_010 = load i32, ptr %i_0, align 4
  %cmp11 = icmp slt i32 2, %i_010
  br i1 %cmp11, label %if.then12, label %if.else13

while.cond:                                       ; preds = %while.body, %if.then
  %b_03 = load i32, ptr %b_0, align 4
  %i_04 = load i32, ptr %i_0, align 4
  %cmp5 = icmp slt i32 %b_03, %i_04
  br i1 %cmp5, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %b_06 = load i32, ptr %b_0, align 4
  %add = add i32 %b_06, 20
  store i32 %add, ptr %b_0, align 4
  br label %while.cond

while.end:                                        ; preds = %while.cond
  %b_07 = load i32, ptr %b_0, align 4
  %add8 = add i32 10, %b_07
  store i32 %add8, ptr %a_0, align 4
  br label %if.merge

if.then12:                                        ; preds = %if.else
  store i32 5, ptr %b_0, align 4
  br label %if.else13

if.else13:                                        ; preds = %if.then12, %if.else
  br label %if.merge

if.merge:                                         ; preds = %if.else13, %while.end
  store i32 1, ptr %ret_val, align 4
  br label %return
}
