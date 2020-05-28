
#include "llvm/InitializePasses.h"
#include "llvm/PassRegistry.h"


using namespace llvm;

void llvm::initializeLLVMShmemFaultTolerance(PassRegistry &Registry) {
	initializeBlockSplitterPass(Registry);
	initializeModifyIRPass(Registry);
	initializeshmemheatPass(Registry);

}

/*

void LLVMInitializeShmemFaultTolerance(LLVMPassRegistryRef R) {
	initializeShmemFaultTolerance(*unwrap(R));
}

*/
