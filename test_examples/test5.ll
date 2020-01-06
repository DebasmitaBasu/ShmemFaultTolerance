; ModuleID = 'test5.c'
source_filename = "test5.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [10 x i8] c"%d : %ld\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i64*, align 8
  %3 = alloca i64, align 8
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  store i32 0, i32* %7, align 4
  %8 = call i32 (...) @shmem_init()
  %9 = call i32 (...) @shmem_my_pe()
  store i32 %9, i32* %5, align 4
  %10 = call i32 (...) @shmem_n_pes()
  store i32 %10, i32* %6, align 4
  %11 = call i32 (i64, ...) bitcast (i32 (...)* @shmem_malloc to i32 (i64, ...)*)(i64 8)
  %12 = sext i32 %11 to i64
  %13 = inttoptr i64 %12 to i64*
  store i64* %13, i64** %2, align 8
  %14 = load i32, i32* %5, align 4
  %15 = sext i32 %14 to i64
  %16 = load i64*, i64** %2, align 8
  store i64 %15, i64* %16, align 8
  %17 = load i32, i32* %5, align 4
  %18 = add nsw i32 %17, 1
  %19 = load i32, i32* %6, align 4
  %20 = srem i32 %18, %19
  store i32 %20, i32* %4, align 4
  %21 = call i32 (...) @shmem_barrier_all()
  %22 = load i64*, i64** %2, align 8
  %23 = load i32, i32* %4, align 4
  %24 = call i32 (i64*, i64*, i32, i32, ...) bitcast (i32 (...)* @shmem_long_get to i32 (i64*, i64*, i32, i32, ...)*)(i64* %3, i64* %22, i32 1, i32 %23)
  %25 = load i64*, i64** %2, align 8
  %26 = load i32, i32* %4, align 4
  %27 = call i32 (i64*, i64*, i32, i32, ...) bitcast (i32 (...)* @shmem_long_get to i32 (i64*, i64*, i32, i32, ...)*)(i64* %3, i64* %25, i32 1, i32 %26)
  %28 = load i32, i32* %5, align 4
  %29 = load i64, i64* %3, align 8
  %30 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([10 x i8], [10 x i8]* @.str, i32 0, i32 0), i32 %28, i64 %29)
  %31 = call i32 (...) @shmem_finalize()
  ret i32 0
}

declare i32 @shmem_init(...) #1

declare i32 @shmem_my_pe(...) #1

declare i32 @shmem_n_pes(...) #1

declare i32 @shmem_malloc(...) #1

declare i32 @shmem_barrier_all(...) #1

declare i32 @shmem_long_get(...) #1

declare i32 @printf(i8*, ...) #1

declare i32 @shmem_finalize(...) #1

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 4.0.1 (https://github.com/llvm-mirror/clang.git 3c8961bedc65c9a15cbe67a2ef385a0938f7cfef) (https://github.com/llvm-mirror/llvm.git c8fccc53ed66d505898f8850bcc690c977a7c9a7)"}
