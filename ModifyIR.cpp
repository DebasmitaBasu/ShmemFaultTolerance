/*
 * CSE504 ASSIGNMENT 1
 * SAMPATH KUMAR KILAPARTHI
 * 112079198
 *
 * */
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
//deb-llvm10update
//#include "llvm/IR/CallSite.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <map>
#include "HelperUtil.h"

/*

This pass modifies the IR to add stub checkpointing calls. This comes as a third pass which works onthe information given out by shmemheat pass.


*/
using namespace llvm;
using namespace std;
//const int DEFAULT = 10;


static const std::string checkpointFuncName[] = { "checkpoint" };


namespace
{
	class ModifyIR : public FunctionPass
	{
	public:
		static char ID;
		map <string, int> opMap;
		ModifyIR() : FunctionPass(ID) {}
		~ModifyIR() {}
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


		virtual void PrintBasicBlock(BasicBlock &BB) {
			//errs() << " \t Process basic block \n";
			for (BasicBlock::iterator bbs = BB.begin(), bbe = BB.end(); bbs != bbe; ++bbs) {
				errs() << *bbs << "\n";
			}
			errs() << "\n";
		}
		virtual bool ProcessBasicBlock(BasicBlock &BB, vector< Instruction* > &cibookeep) {
			errs() << " \t Process basic block \n";
			for (BasicBlock::iterator bbs = BB.begin(), bbe = BB.end(); bbs != bbe; ++bbs) {

				Instruction* ii = &(*bbs);
				//deb-llvm10update
				//CallSite cs(ii);
				CallInst *cs = dyn_cast<CallInst>(ii);
				//****if (!cs.getInstruction()) continue;
				Value* called = cs->getCalledFunction();
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
		/*  This function add an instruction to insert checkpoint() call into the IR. This call is inserted 
		right after the instruction el given as an input*/

		void createFunction(Instruction *el, Function &F) {


			errs() << " \t Try modifying the IR";
			LLVMContext &Ctx = F.getContext();
			std::vector<Type*> paramTypes;
			std::vector<Value*> Args;

			// set the return type of the checkpoint () call
			Type *retType = Type::getInt8PtrTy(Ctx);

			//paramTypes.push_back(Type::getDoubleTy(Ctx);
			// fill in args here
			//Args.push_back(op->getOperand(0));
			//int (double)
			errs() << " \t Try modifying the IR";
			/* Create a function type with the checkpoint return and argument types as arguments */
			FunctionType *insertFuncType = FunctionType::get(retType, paramTypes, false);
			errs() << " \t Try modifying the IR";
			// Now chose the function type which one to use 
			int funcVal = 0;
			std::string newFuncName = checkpointFuncName[funcVal];
			
			/* Inserts the function call checkpoint() with newFuncname and insertFucntype return type*/
			//deb
			//Constant *newFunc = F.getParent()->getOrInsertFunction(newFuncName, insertFuncType);
			llvm::FunctionCallee newFunc = F.getParent()->getOrInsertFunction(newFuncName, insertFuncType);
			errs() << " \t Try modifying the IR";
			/* creates a call instruction with all prefilled entries*/
			CallInst *NewCI = CallInst::Create(newFunc, Args);

			//ReplaceInstWithInst(op, (NewCI));
			//(pr.second)->getInstList().insert(pr.first, (NewCI));
			errs() << " \t Try modifying the IR insert";

			//(pr.second)->getInstList().push_back(NewCI);
			/* This is where the magic happens. We inserted a new instruction into IR.*/
			NewCI->insertAfter(el);

			// printing the information to the consol
			/*errs() << "Information for this malloc call : " << mallocCallNo << "\n";
			errs() << "Allocation size = " << mallocSizeMapMap[I.second] << "\n";
			errs() << "Total Access(load/store) freq = " << I.first << "\n";
			errs() << "Read(load) freq = " << mallocLoadFreqMap[I.second] << "\n";
			errs() << "Write(load) freq = " << mallocStoreFreqMap[I.second] << "\n";*/
			errs() << "We used function  = " << newFuncName << "\n\n\n";

			//retFlag = true;
		}

		virtual bool runOnFunction(Function &Func)
		{
			errs().write_escaped(Func.getName());
			errs() << " \t Old function size " << Func.size();
			vector< Instruction* > cibookkeep;
			for (Function::iterator Its = Func.begin(), Ite = Func.end(); Its != Ite; ++Its) {
				ProcessBasicBlock(*Its, cibookkeep);
				errs() << " \t Processing ";
			}
			errs() << " \t Try modifying the IR";
			/*  For every instruction that ends with a sync routine we modify the IR to add a checkpoint() call*/
			for (auto el : cibookkeep) {
				/*IRBuilder Builder(el);
				Value *StoreAddr = Builder.CreatePtrToInt(si->getPointerOperand(), Builder.getInt32Ty());
				Value *Masked = Builder.CreateAnd(StoreAddr, 0xffff);
				Value *AlignedAddr = Builder.CreateIntToPtr(Masked, si->getPointerOperand()->getType());
				*/

				createFunction(el, Func);
			}

			errs() << " \t Printing the new IR after modification";
			for (Function::iterator Its = Func.begin(), Ite = Func.end(); Its != Ite; ++Its) {
				PrintBasicBlock(*Its);
			}
			errs() << '\n';

			return true;
		}
	};

}

//@Abdulllah
char ModifyIR::ID = 0;
void initializeModifyIRPass(PassRegistry &);
INITIALIZE_PASS(ModifyIR, "ModifyIRPass",
	"Changes the IR to add a custom function", false, false)
