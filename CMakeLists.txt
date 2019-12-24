
add_llvm_loadable_module( LLVMShmemFaultTolerance
  shmemheat.cpp
  BlockSplitter.cpp
  ModifyIR.cpp
  RegisterPasses.cpp

  DEPENDS
  intrinsics_gen
  )

