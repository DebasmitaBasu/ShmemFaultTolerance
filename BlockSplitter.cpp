/*
 * 
 * SAMPATH KUMAR KILAPARTHI
 *  This pass splits the llvm basic blocks into custom defined shmem basic blocks. The criteria is customly defined by the user. 
 *  This implementation uses all the shmem related call instructions to decide on a criteria to split them accordingly
 *	    
 *
 * 
 
Input Basic Block:


  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca double, align 8
  store i32 0, i32* %1, align 4
  %5 = call i32 (...) @shmem_init()		// split this here
  store double 0.000000e+00, double* %4, align 8
  %6 = call i32 (...) @shmem_barrier_all()
  store i32 0, i32* %2, align 4
  br label %7



  Output Basic Block:


%1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca double, align 8
  store i32 0, i32* %1, align 4
  br label %.split						// Here we splitted the previous Basic block into two shmem basic blocks.
										// Our criteria is based on shmem_init() in this case. The splits usually involve 
										// branch instructions with a label .split which is being added by SplitBlock() llvm function.
										// Refer to llvm documentation for SplitBlock() for more detailed information.

	 Process basic block :.split
  %5 = call i32 (...) @shmem_init()
  store double 0.000000e+00, double* %4, align 8
  br label %.split.split

	

*/


#pragma once
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/InitializePasses.h" /*deb*/
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

	/* Input: String name extracted from Call Site Instruction 
				e.g shmem_init extracted from IR Call instruction @shmem_init()

	   Output:
				A user defined map between the shmem functions and it's correspinding ID. 
				This information can be passed in as an alternate input to make it more 
				generic based so as to work with other libraries.
				e.g
				shmem_init 1
				shmem_put  2
				shmem_get  3
				default    4
	 Used by:  ProcessBasicBlock()
		
	*/
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
	/*
	Helper function to split a basic block in a specific instruction.

	Input:	Pointer to the Instruction (e.g shmem_init) where the block needs to be splitted
	Output:
			Given an old basic block :
			Old = head + tail(tail has the instruction ii)

			This generates two basic blocks:
			Generated Basic Block :
			Basic Block1 = [ Head, Instruction ii) + Branch instruction
			Basic Block2 = [ Instruction, Tail ]


Old basic Block:

	head	---->	  %1 = alloca i32, align 4
					  %2 = alloca i32, align 4
					  %3 = alloca i32, align 4
					  %4 = alloca double, align 8
					  store i32 0, i32* %1, align 4
					  %5 = call i32 (...) @shmem_init()		// split this here
					  store double 0.000000e+00, double* %4, align 8
					  %6 = call i32 (...) @shmem_barrier_all()
					  store i32 0, i32* %2, align 4
					  br label %7


Newly created Basic Blocks:

	head	---->	  %1 = alloca i32, align 4
					  %2 = alloca i32, align 4
					  %3 = alloca i32, align 4
					  %4 = alloca double, align 8
					  store i32 0, i32* %1, align 4
					  br label %.split	    ( Newly created instruction )
   
   This will be named with a prefix .split

   tail    ----->	  %5 = call i32 (...) @shmem_init()
					  store double 0.000000e+00, double* %4, align 8
					  br label %.split.split
		
	*/
	void SplitBlockHelper(Instruction* ii) {
		errs() << " \t Split helper\n";
		// get the pointer on the BB that needs to be splitted
		BasicBlock *head = ii->getParent();
		// Splits the old basic block in to two basic blocks. 
		BasicBlock *tail = SplitBlock(head, ii);
		return;

	}

	/*
	Helper  function to print every instruction of a given Basic Block
	*/
	virtual bool PrintBasicBlock(BasicBlock &BB) {
		errs() << " \t Process basic block :" << BB.getName() << "\n";
		for (BasicBlock::iterator bbs = BB.begin(), bbe = BB.end(); bbs != bbe; ++bbs) {
			errs() << *bbs << "\n";
		}
		errs() << "\n";
	}


	/*
	 Processes the given BB and populates call instuctions into cibookeep. 
	 Gathers all shmem related call instructions into cibookeep.

	 Input: Basic Block BB , Vector to hold Instructions
	 Output: Output is populated call instructions in a vector cibookeep
	*/


	virtual bool ProcessBasicBlock(BasicBlock &BB, vector<Instruction*> &cibookeep) {
		errs() << " \t Process basic block \n";
		for (BasicBlock::iterator bbs = BB.begin(), bbe = BB.end(); bbs != bbe; ++bbs) {


			// typecasting iterator into instruction pointer.
			Instruction* ii = &(*bbs);
			// Create a callSite object from the ii. 
			// This helps us to make use of callsite api to get name effectively.
			// TODO : Possible alternative implemtation:    CallInst *ci = dyn_cast<CallInst>(ii);
			CallSite cs(ii);
			// Check if ii is a call instruction.
			if (!cs.getInstruction()) continue;
			// Gets rid of any complex pointer castings to this instruction.
			Value* called = cs.getCalledValue()->stripPointerCasts();
			// Gets the function pointer to the Called function in the call instruction.
			if (Function *fptr = dyn_cast<Function>(called)) {
				// get name of the function from the fptr
				string cname = fptr->getName().str();

				// Pushes shmem specific call instructions into the cibookeep.
				// TODO Can do better
				switch (GetFunctionID(cname)) {
				case 1:
				case 2:
				case 3: 
				case 4:
				case 5: 
				case 6:
					/* Intentional case statements without breaks. This encompasses all the cases from 1-6*/
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
		// Process each basic block  and look for shmem related call instructions.
		for (Function::iterator Its = Func.begin(), Ite = Func.end(); Its != Ite; ++Its) {
			ProcessBasicBlock(*Its, cibookkeep);
			errs() << " \t Processing ";
		}
		// Splits basic blocks for every entry into cibookeep.
		// Splitblock manipulates the Func (i.e it adds more entry into Func ). 
        // And hence the book keeping of call instructions.

		for (auto el : cibookkeep) {
			SplitBlockHelper(el);
		}
		// This function size differs from that of the old Fuction size.
		errs() << " \t New function size " << Func.size();
		errs() << " \t Printing the new IR after modification";
		for (Function::iterator Its = Func.begin(), Ite = Func.end(); Its != Ite; ++Its) {
			PrintBasicBlock(*Its);
		}
		errs() << '\n' ;
		// Returning true since we modified the IR.
		return true;
	}	
};


/*
RegisterPass<BlockSplitter> X("BlockSplitter" , "Splits Blocks which have shmem functions");*/
}

// This needs to be done to add this pass to llvm namespace.
char BlockSplitter::ID = 0;
INITIALIZE_PASS(BlockSplitter, "BlockSplitterPass",
	"Splits the blocks appropriately", false, false)



	



