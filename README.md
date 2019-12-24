# shmemheat
Displays heat information of selected Openshmem functions

1. Place this folder in lib/Transforms of llvm code repository.
2. make -j number_of_cores build it
3. Generate IR using 
    clang -emit-llvm -S test5.c -o test5.ll
4. And run the pass using this command
  opt -load your_llvm_build_path/build/lib/LLVMshmemheatpass.so -shmemheat < test5.ll  > /dev/null 2> test5.txt
  The output will be redirected to test5.txt file.
 
 
Note: Refer https://www.cs.cornell.edu/~asampson/blog/llvm.html for more details.
