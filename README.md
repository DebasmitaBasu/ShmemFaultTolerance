#Block Splitter

1.  clang -emit-llvm -S lastest_block_sample.c -o lastest_block_sample.ll
	emit-llvm emits IR code for a given code 

2.  opt -load /gpfs/home/ksampathkuma/llvm_build/build/lib/LLVMShmemFaultTolerance.so -BlockSplitterPass < lastest_block_sample.ll  > /dev/null 2> block_sample.txt
	Look for the LLVMShmemFaultTolerance.so shared object file in the build directory and invoke a pass


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

# adding passes to llvm namespace

Please refer to https://github.com/samolisov/llvm-experiments/tree/master/llvm-ir-custom-passes

Take it with caution. You may run into cyclic dependencies.
Follow the tutorial as it. But ensure that yor llvmbuild.txt is right

Note1:

[component_0]
type = Library
name = ShmemFaultTolerance   /*  Name of the folder*/
parent = Transforms
library_name = ShmemFaultTolerance  /* The name of library which you want it to compiled as*/
required_libraries = Analysis Core Support TransformUtils  /*  These must be right*/


Note2:
These passes are different from general passes which make use of opt. These passes must include

INITIALIZE_PASS(shmemheat, "shmemheatpass",
	"Prints shmem heat function  analysis", false, false)


Note3: 

Use names like ShmemFaultTolerancePass with caution. The LLVM might add an automated suffix(Pass). The result might look like ShmemFaultTolerancePassPass. Your pass won't be accesible as ShmemFaultTolerancePass anymore because at compile time make file looks for ShmemFaultTolerancePassPass.




## Build custom passes as a part of the LLVM build environment

Please refer to https://github.com/samolisov/llvm-experiments/tree/master/llvm-ir-custom-passes

There is no dynamic linking on Windows (this is OS weirdness) so we cannot use the plugins at all, unfortunately.

To build the pass, do the following:

 1. copy the `LLVMExperimentPasses` pass directory in the `lib/Transforms` one. Here and throughout, all paths are given at the
    root of the LLVM source directory.

 2. add the `add_subdirectory(LLVMExperimentPasses)` line into `lib/Transforms/CMakeLists.txt`

 3. for each implemented pass, add a function named `initialize${THE_NAME_OF_THE_PASS}Pass(PassRegistry &);` into the `include/llvm/InitializePasses.h`
    header file. Also add the funtion `void initializeLLVMExperimentPasses(PassRegistry &);` there. For example, for the `FunctionArgumentCount` pass
    add the following lines:

    ```cpp
    // my experiment passes
    void initializeLLVMExperimentPasses(PassRegistry &);
    void initializeFunctionArgumentCountPass(PassRegistry &);
    } // end namespace llvm

    ```

    (the functions **must be** defined inside the `llvm` namespace).

 4. add the `LLVMExperimentPasses` library to the `LLVM_LINK_COMPONENTS` list in the `tools/opt/CMakeLists.txt` file:

    ```cmake
    set(LLVM_LINK_COMPONENTS
        ${LLVM_TARGETS_TO_BUILD}
        AggressiveInstCombine
        ...
        ExperimentPasses
    )

    ```

    **Note:** The form of `ExperimentPasses`, not `LLVMExperimentPasses` is used here.

 5. register the passes into the `opt` tool by adding an invocation of the `initializeLLVMExperimentPasses` function to the `main` method
    of the tool (file `tools/opt/opt.cpp`):

    ```cpp
    // Initialize passes
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializeCore(Registry);
    ...
    initializeLLVMExperimentPasses(Registry);
    // For codegen passes, only passes that do IR to IR transformation are
    // supported.
    ```

 P.S. Points 2 - 5 can be automated using the following command (it is applicable only if you are using an LLVM Git repo):

 ```bash
 git apply <PATH_TO_YOUR_LLVM_EXPERIMENTS_COPY>/llvm-ir-custom-passes/patches/register-experiment-passes.patch
 ```

 The command must be executed from the **LLVM** Git repo, not **LLVM Experiments**.

 6. rebuild LLVM (`YOUR_LLVM_BUILDTREE` is the directory where you build LLVM) and install the new output files:

    ```bash
    cd YOUR_LLVM_BUILDTREE

    cmake -DCMAKE_CXX_COMPILER=YOUR_FAVOURITE_COMPILER -DCMAKE_C_COMPILER=YOUR_FAVOURITE_COMPILER -DCMAKE_LINKER=YOUR_FAVOURITE_LINKER .. -G"Ninja"

    cmake --build .

    cmake --build . --target install
    ```

  7. The passes are ready. For instance, the `FunctionArgumentCount` pass is registered as `fnargcnt` in the `opt` tool and can be invoked
     using the following command line:

     ```bash
     opt.exe -fnargcnt < sum.bc > sum-out.bc
     ```

