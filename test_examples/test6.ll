; ModuleID = 'test6.c'
source_filename = "test6.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca [9 x i64], align 16
  %7 = alloca i64*, align 8
  store i32 0, i32* %1, align 4
  %8 = call i32 (...) @shmem_init()
  %9 = call i32 (...) @shmem_my_pe()
  store i32 %9, i32* %4, align 4
  %10 = call i32 (...) @shmem_n_pes()
  store i32 %10, i32* %5, align 4
  store i32 0, i32* %2, align 4
  br label %11

; <label>:11:                                     ; preds = %20, %0
  %12 = load i32, i32* %2, align 4
  %13 = icmp slt i32 %12, 9
  br i1 %13, label %14, label %23

; <label>:14:                                     ; preds = %11
  %15 = load i32, i32* %4, align 4
  %16 = sext i32 %15 to i64
  %17 = load i32, i32* %2, align 4
  %18 = sext i32 %17 to i64
  %19 = getelementptr inbounds [9 x i64], [9 x i64]* %6, i64 0, i64 %18
  store i64 %16, i64* %19, align 8
  br label %20

; <label>:20:                                     ; preds = %14
  %21 = load i32, i32* %2, align 4
  %22 = add nsw i32 %21, 1
  store i32 %22, i32* %2, align 4
  br label %11

; <label>:23:                                     ; preds = %11
  %24 = call i32 (i64, ...) bitcast (i32 (...)* @shmem_malloc to i32 (i64, ...)*)(i64 72)
  %25 = sext i32 %24 to i64
  %26 = inttoptr i64 %25 to i64*
  store i64* %26, i64** %7, align 8
  %27 = load i32, i32* %4, align 4
  %28 = add nsw i32 %27, 1
  %29 = load i32, i32* %5, align 4
  %30 = srem i32 %28, %29
  store i32 %30, i32* %3, align 4
  %31 = load i64*, i64** %7, align 8
  %32 = getelementptr inbounds [9 x i64], [9 x i64]* %6, i32 0, i32 0
  %33 = load i32, i32* %3, align 4
  %34 = call i32 (i64*, i64*, i32, i32, ...) bitcast (i32 (...)* @shmem_long_put to i32 (i64*, i64*, i32, i32, ...)*)(i64* %31, i64* %32, i32 9, i32 %33)
  %35 = load i64*, i64** %7, align 8
  %36 = getelementptr inbounds [9 x i64], [9 x i64]* %6, i32 0, i32 0
  %37 = getelementptr inbounds i64, i64* %36, i64 3
  %38 = load i32, i32* %3, align 4
  %39 = call i32 (i64*, i64*, i32, i32, ...) bitcast (i32 (...)* @shmem_long_put to i32 (i64*, i64*, i32, i32, ...)*)(i64* %35, i64* %37, i32 5, i32 %38)
  %40 = call i32 (...) @shmem_barrier_all()
  %41 = load i64*, i64** %7, align 8
  %42 = call i32 (i64*, ...) bitcast (i32 (...)* @shmem_free to i32 (i64*, ...)*)(i64* %41)
  %43 = call i32 (...) @shmem_finalize()
  ret i32 0
}

declare i32 @shmem_init(...) #1

declare i32 @shmem_my_pe(...) #1

declare i32 @shmem_n_pes(...) #1

declare i32 @shmem_malloc(...) #1

declare i32 @shmem_long_put(...) #1

declare i32 @shmem_barrier_all(...) #1

declare i32 @shmem_free(...) #1

declare i32 @shmem_finalize(...) #1

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 4.0.1 (https://github.com/llvm-mirror/clang.git 3c8961bedc65c9a15cbe67a2ef385a0938f7cfef) (https://github.com/llvm-mirror/llvm.git c8fccc53ed66d505898f8850bcc690c977a7c9a7)"}
