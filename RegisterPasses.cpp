
#include "llvm/InitializePasses.h"
#include "llvm/PassRegistry.h"


using namespace llvm;

void llvm::initializeShmemFaultTolerance(PassRegistry &Registry) {
	/*initializeBlockSplitterPass(Registry);
	initializeshmemheatPass(Registry);*/

}

void LLVMInitializeShmemFaultTolerance(LLVMPassRegistryRef R) {
	initializeShmemFaultTolerance(*unwrap(R));
}
