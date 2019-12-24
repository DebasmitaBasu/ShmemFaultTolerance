/*
 * 
 * SAMPATH KUMAR KILAPARTHI
 * 112079198
 *
 * */


#pragma once
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <map>
#include "HelperUtil.h"

using namespace llvm;
using namespace std;

namespace
{
class BlockSplitter : public FunctionPass
{
public:
	static char ID;
	BlockSplitter() : FunctionPass(ID){}
	~BlockSplitter() {}
	int GetFunctionID(string &cname) {
		/*
		shmem_init 1
		shmem_put  2
		shmem_get  3
		default    4
		*/
		int value = DEFAULT;
		if (cname.find("shmem_init") != std::string::npos) {
			value = 1;
		}
		else if (cname.find("shmem") != std::string::npos && cname.find("put") != std::string::npos) {
			value = 2;
		}
		else if (cname.find("shmem") != std::string::npos && cname.find("get") != std::string::npos) {
			value = 3;
		}
		else if (cname.find("shmem") != std::string::npos && cname.find("init") != std::string::npos) {
			value = 4;
		}
		else if (cname.find("shmem") != std::string::npos && cname.find("barrier_all") != std::string::npos) {
			value = 5;
		}
		else if (cname.find("shmem") != std::string::npos && cname.find("finalize") != std::string::npos) {
			value = 6;
		}
		else {
			value = DEFAULT;
		}
		return value;
	}

	void SplitBlockHelper(Instruction* ii) {
		errs() << " \t Split helper\n";
		BasicBlock *head = ii->getParent();
		BasicBlock *tail = SplitBlock(head, ii);
		return;

	}
	virtual bool PrintBasicBlock(BasicBlock &BB) {
		errs() << " \t Process basic block :" << BB.getName() << "\n";
		for (BasicBlock::iterator bbs = BB.begin(), bbe = BB.end(); bbs != bbe; ++bbs) {
			errs() << *bbs << "\n";
		}
		errs() << "\n";
	}
	virtual bool ProcessBasicBlock(BasicBlock &BB, vector<Instruction*> &cibookeep) {
		errs() << " \t Process basic block \n";
		for (BasicBlock::iterator bbs = BB.begin(), bbe = BB.end(); bbs != bbe; ++bbs) {

			Instruction* ii = &(*bbs);
			CallSite cs(ii);
			if (!cs.getInstruction()) continue;
			Value* called = cs.getCalledValue()->stripPointerCasts();
			if (Function *fptr = dyn_cast<Function>(called)) {
				string cname = fptr->getName().str();
				switch (GetFunctionID(cname)) {
				case 1:
				case 2:
				case 3: 
				case 4:
				case 5: 
				case 6:
					cibookeep.push_back(ii);
					break;
				default:
					errs() << "Default case invoked: " << cname << "\n";
					break;

				}
			}
		}
	}


	virtual bool runOnFunction(Function &Func)
	{
		errs().write_escaped(Func.getName());
		errs() << " \t Old function size " << Func.size() ;
		vector<Instruction*> cibookkeep;
		for (Function::iterator Its = Func.begin(), Ite = Func.end(); Its != Ite; ++Its) {
			ProcessBasicBlock(*Its, cibookkeep);
			errs() << " \t Processing ";
		}

		for (auto el : cibookkeep) {
			SplitBlockHelper(el);
		}
		errs() << " \t New function size " << Func.size();
		errs() << " \t Printing the new IR after modification";
		for (Function::iterator Its = Func.begin(), Ite = Func.end(); Its != Ite; ++Its) {
			PrintBasicBlock(*Its);
		}
		errs() << '\n' ;
		return true;
	}	
};


/*
RegisterPass<BlockSplitter> X("BlockSplitter" , "Splits Blocks which have shmem functions");*/
}
char BlockSplitter::ID = 0;
INITIALIZE_PASS(BlockSplitter, "BlockSplitterPass",
	"Splits the blocks appropriately", false, false)



