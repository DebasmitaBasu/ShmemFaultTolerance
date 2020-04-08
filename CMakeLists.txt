
add_llvm_library( LLVMShmemFaultTolerance
  shmemheat.cpp
  BlockSplitter.cpp
  ModifyIR.cpp
  RegisterPasses.cpp

  DEPENDS
  intrinsics_gen
  )

