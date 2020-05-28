/*
 * CSE 523 / 524
 * SAMPATH KUMAR KILAPARTHI
 * 112079198
 * */



/*
 This pass is the core of this fault tolerance pipeline. It should act as a glue between  BlockSplitter and Modify IR. 
 This can be categorized as a meta info collection pass. Some of the key information pertaining to the placement of checkpointing calls
 is deduced here. This is organized into several parts.


 
 1. Collecting Alloca related meta info
 2. Collect Call instructions info.
 3. Runs Reaching Definition analysis.
 4. scrutinize every Basic Block for information like:
		1. load , store instuctions. (This includes normal load, store which are a part of llvm IR)
		2. Library specific load/Store instructions.  (i.e shmem_put and shmem_get)
		3. All the shmem library specific calls. 
		4. Tags if this block is part of a loop.
		5. Provides information to make an informed decision about the importance of this block.
		6. populated hnode info for every basic block.

This information plays a key role in identifying places to insert check pointing function call.
*/
#pragma once
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
//deb10-llvm10 update
//#include "llvm/IR/CallSite.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BlockFrequencyInfoImpl.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <vector>
#include <stack>
//#include "../LivenessAnalysis/LivenessAnalysis.cpp"
#include "LivenessInfo.h"
#include "shmemHeatInfo.h"
#include "HelperUtil.h"
#include "DFA.h"
#include <llvm/IR/DebugLoc.h>
#include <llvm/IR/DebugInfoMetadata.h>


//using namespace std;


using namespace llvm;
using namespace std;

//typedef std::pair<unsigned, unsigned> Edge;

/* helper to get the function. Currently not using it.
Input: Call instruction.
Output: name of the call function.
*/

	string ParseFunctionName(CallInst *call) {
		auto *fptr = call->getCalledFunction();
		if (!fptr) {
			return "received null as fptr";
		} else {
			return string(fptr->getName());
		}
	}

	namespace {

	//class shmemheat : public  BlockFrequencyInfoWrapperPass {
		class shmemheat : public  FunctionPass {
	public:


		/*  
		 *	TTR: There are duplications of information as a vector and a map for variable meta info & call instructions.
		 *	This can be avoided but I choose to use vector for iterating through alloca instrucions and call instructions.
		 */
		/*	Vector of all variable aAlloca specific metainformation */
		vector <VariableMetaInfo*> Variableinfos;
		/*	map of Alloca instruction and it's variable meta information*/
		DenseMap < Instruction *, VariableMetaInfo* > Inst2VarInfo_map;

		// call intruction and it's operands involved for alloca
		vector < CallMetaInfo *>  callinstvec;
		/* Mapping between a call instruction and it's corresponding CallMetainfo entry*/
		map < CallInst *, CallMetaInfo* > Callinst2AllocaMap;
		/* mapping between shmem function call and their ID's. This is done towards a generic way of inputting 
		library calls to this pass as  a user input*/
		//@deb, vector to hold allocainstructions
		vector < Instruction *>  allocaV;

		map <string, int> functionMap;
		/* mapping between Basic Block and it's corrresponding heat node*/
		map < bbt, heatNode *> heatmp;
		/* mapping between Basic Block ID and it's corrresponding head node. Just in case. May come handy*/
		map < int, heatNode *> heatIDmp;



		//std::map<unsigned, BasicBlock::iterator> IndexToInstr;
		//map< string, int> functionToCodeMap;
		static char ID;
		int id = 1;


		/* Must give inputs to the Live ness analysis.*/
		LivenessInfo bottom, initialState;
		
		
		/* Instance of liveness analysis which will be invoked in this pass. 
		This is a work around to ensure dependency on Liveness Analysis. The right 
		way is to expose LivenessAnalysis as an independent pass.*/
		LivenessAnalysis *rda;

		shmemheat() : FunctionPass(ID) {
			/* create an instance of reaching definition analysis*/
			rda = new LivenessAnalysis(bottom, initialState);

		}
		~shmemheat() {}

		/*
		 * This function peeks into the alloca instructions
		 *
		 */

		bool find_alloca_from_vec(AllocaInst *AI, int *index) {
			bool present = false;
			for (int i = 0; i < Variableinfos.size(); i++) {
				if (Variableinfos[i]->alloca == AI) {
					*index = i;
					present = true;
					break;
				}
			}
			return present;
		}

		/* TTR: We don't use and it's just a placeholder code.
		
		Identifies the type of Operand: Store, Load, POinter (GEpi), Direct alloca descendant. And print's it's information which is not handy here
		
		*/
		void peek_into_alloca_mappings(Value *v1) {
			errs() << "Print argument type: " << *(v1->getType()) << "\n";
			v1->dump();
			if (v1->getType()->isPointerTy()) {
				//errs() << "1. pointer type\n";
				if (isa<LoadInst>(v1)) {
					errs() << "Load type\n";

					Instruction *useinst;
					if ((useinst = cast<LoadInst>(v1))) {
						
						//errs() << "\nPrinting the alloca: \t";
						//errs() << vinfo->alloca->dump() << "\n";
						if (Inst2VarInfo_map.find(useinst) != Inst2VarInfo_map.end()) {
							VariableMetaInfo *vinfo = Inst2VarInfo_map[useinst];
							errs() << "Found alloca mapped instruction\n";
						} else {
							errs() << "Couldn't find it";
						}

					}
				} else if (isa<AllocaInst>(v1)) {
					errs() << "Alloca type\n";
					//AllocaInst *allocinst;
					int index = -1;
					if (AllocaInst *allocinst = cast<AllocaInst>(v1)) {
						for (auto vinfo : Variableinfos) {

							errs() << "\nCalculated flag: " << (vinfo->alloca == allocinst);

							if (vinfo->alloca == allocinst) {
								//errs() << "\nPrinting the alloca: \t";
								//errs() << vinfo->alloca->dump() << "\n";

								if (find_alloca_from_vec(allocinst, &index)) {
									errs() << "Found alloca direct instruction\n";
								} else {
									errs() << "Lookup failed\n";
								}
								break;
							}
						}
					}
				} else if (isa<StoreInst>(v1)) {

					errs() << "Found Store instruction\n";

				} else if (isa<GetElementPtrInst>(v1)) {
					errs() << "Found GEPI instruction\n";
					//AllocaInst *allocinst;
					if (GetElementPtrInst *GEPI = cast<GetElementPtrInst>(v1)) {

						errs() << GEPI->getNumOperands() << "\n";

						for (int i = 0; i < GEPI->getNumOperands(); i++) {
							errs() << "\t" << *(GEPI->getOperand(i)) << "\n";
						}
					}
				} else {
					errs() << "Went to else case\n";
				}
			} else {
				errs() << "Not a pointer type argument\n";
			}

		}

		/*
		 * This function prints the function arguments. 
		 *  Additionally, it gets corresponding alloca instruction for every operand of the call instruction.
		 *  Input: Operand typecasted into Instruction,  va (vector of alloca instructions linked to  out input Instruction)
		 *  Output: A vector of all Alloca instructions for the given instruction (operand).
		 */


		void get_allocainst_for_every_operand(Instruction *ci, vector <Instruction *> &va) {
			stack <Instruction *> s;
			//vector <Instruction *> v;
			Instruction *ii;
			//ii = cast<Intruction>(*ci);
			ii = ci;
			s.push(ii);
			bool flag = true;
			while (!s.empty()) {
				ii = s.top(); s.pop();

				/*  set of all variables used at our instruction ii. We go recursively upwards until we reach an alloca instruction. */
				for (Use &U : ii->operands()) {
					Value *v = U.get();
					errs() << "\n\t\t\t\t \"  starting on : " << *v << "\n";
					if (dyn_cast<Instruction>(v)) {
						//errs() << "\n\"  processing " << *dyn_cast<Instruction>(v) << "\n\t" << "\"" << " -> " << "\"" << *ii << "\"" << ";\n";
						//flag = true;
						ii = dyn_cast<Instruction>(v);
						s.push(ii);
						/* Checks if ii is an instance of Alloca instruction */
						if (AllocaInst *alloca = dyn_cast_or_null<AllocaInst>(ii)) {
							va.push_back(ii);
							errs() << "\n\"  processing " << *dyn_cast<Instruction>(v) << "\n\t" << "\"" << " -> " << "\"" << *ii << "\"" << ";\n";
						}
					}
				}
			}
		}
		/* This helper function deals with printing the call meta info for a given call instruction.
		This is more like a debug print output of CallMetaInfo.
		Input:  Call instruction and it's corresponsing Callmetainfo.
		Output: This function makes entries into CallMetaInfo with that of it's operands primary Alloca bases.
		*/
		void PrintFunctionArgsCallMetaInfo(CallInst *ci, CallMetaInfo *cmi) {
			// gets function name from the call instruction
			//deb-llvm10update
			//string cname = dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts())->getName().str();
			string cname = ci->getCalledFunction()->getName().str();
			// We check fucntions which contains get and put functions. We match the function string cname with selected patterns.
			Value *v1, *v2, *v3, *v4;
			LoadInst *li1, *li2;

			errs() << "Iterating over the operands on the call instruction\n";
			/* Iterate over every operand in the given call instruction.*/
			/*  Get all instructions that are used in the operand of our call instruciton. This may not directly include alloca instructions
			because those instructions could be devivatives of base alloca instructions. We call get_allocainst_for_every_operand () to get the
			base alloca instructions. 
			

			base alloca %1 = alloca
			%2 = add %1, 4
			call (%2)
			Here we get only %2, But we set out to get %1 as our base alloca.
			
			*/

			for (Use &U : ci->operands()) {
				vector <Instruction *> va;
				va.clear();
				Value *v = U.get();

				if (dyn_cast<Instruction>(v)) {
					//errs() << "\n\"" << *dyn_cast<Instruction>(v) << "\n\t" << "\"" << " -> " << "\"" << *ci << "\"" << ";\n";
					/* Go find alloca bases for every operand in the call instruction. we casted the value entry (Operand) into an instruction. 
					This gives the instruction for this operand.
					

					%4 = load %3
					call ( %4 )
					This gets us %4 = load %3 (instruciton) from %4.



					*/
					get_allocainst_for_every_operand(dyn_cast<Instruction>(v), va);
					/*  We got base alloca instructions for every operand*/
					for (int i = 0; i < va.size(); i++) {
						errs() <<  i << " 'th alloca map: " << *(va[i]) << "\n";
					}
					cmi->vva.push_back(va);
				}
			}


			errs() << "Alloca instructions ended\n";

			/* TTR: We don't need this . This is an experimental code towards obtaining more information of the call instruction operands.*/
			if (cname.find("put") != std::string::npos || cname.find("get") != std::string::npos) {

				errs() << "\n\nfunction args trace start\n";
				Function* fn = ci->getCalledFunction();
				//for (auto arg = fn->arg_begin(); arg != fn->arg_end(); ++arg) {
					//errs() << *(arg) << "\n";
				//}
				errs() << "function args trace end\n\n";

				v1 = ci->getArgOperand(0);
				v2 = ci->getArgOperand(1);
				v3 = ci->getArgOperand(2);
				v4 = ci->getArgOperand(3);
				/* Not sure if we need this. This just peeks into alloca bases of the Value (operand) of call instructions*/
				peek_into_alloca_mappings(v1);
				peek_into_alloca_mappings(v2);

				/*

				if (cname.find("put") != std::string::npos || cname.find("get") != std::string::npos) {

					for (auto i = 0; i < ci->getNumArgOperands(); i++)
					{
						ci->getArgOperand(i)->dump();
						if (ci->getArgOperand(i)->getType()->isPointerTy())
						{
							errs() << ci->getArgOperand(i)->stripPointerCasts()->getName().str() << "\n";
						}
						else
						{
							//errs() << ci->getArgOperand(i)->getName().str() << "\t";
							//errs() << ci->getOperand(i);
						}
					}
					//isIntegerTy()
					//case 1
					Type *a3, *a4;
					Value *v3 = ci->getArgOperand(3);
					a3 = ci->getArgOperand(2)->getType();
					a4 = ci->getArgOperand(3)->getType();
					a4->dump();
					if (a4->isIntegerTy())
					{
						// compare the values and see if it out of current PE
						if (ConstantInt* cint = dyn_cast<ConstantInt>(ci->getArgOperand(3))) {
							errs() << "const integer type\n";
							// foo indeed is a ConstantInt, we can use CI here
							errs() << "Const value: " << cint->getSExtValue() << "\n";
						}
						else {
							// foo was not actually a ConstantInt
							errs() << "Not a const\n";
						}
					}
					else
					{
						// Different types. It must me an integert according to the put and get definitions
						//errs() << "Different types\n";
					}
					//a3->dump();
					//errs() << a3->getSExtValue() << " get value\n";
					//ci->getArgOperand(2)->getSExtValue();
					//ci->getArgOperand(2)->dump();
					errs() << "\nPrinting the actual PE argument: ";
					ci->getArgOperand(3)->dump();
					errs() << "************************************************************************ \n\n";
				*/
				/*TTR: End*/
			}
		}
		/* gets the number of heat nodes for print helper*/
		int getNoOfNodes() {
			return id - 1;
		}
		/*  We ensure that these passes are run before the runonFunction() call*/

		void getAnalysisUsage(AnalysisUsage &AU) const {
			//AU.addRequired<BranchProbabilityInfoWrapperPass>();
			AU.addRequired<LoopInfoWrapperPass>();
			//AU.addRequired<LivenessAnalysisPass>();
			AU.setPreservesAll();


			/*
			passs1- live
			           ==>  pass2 <- pass1
					                      =>
			*/
		}

		void printResult() {
			// call instructions
			errs() << "\n\t Call instructions:\t" << functionMap["call"];
			errs() << '\n';
		}
		/* Helper function to print Basic Block related information at one go.*/
		void printheadnodeinfo() {

			int nodesize = getNoOfNodes();
			heatNode *tmp = NULL;
			for (auto &el: heatIDmp) {
				tmp = heatIDmp[el.first];
				errs() << "\nID: " << tmp->getID();
				errs() << "\nfreqcount: " << tmp->getfreqcount();
				errs() << "\nprofcount: " << tmp->getprofcount();
				errs() << "\nNo of call instructions " << tmp->getnoofcallins() << "\n";
				errs() << "\n Loop tagged info:" << tmp->attachedtoLoop << "\n";
				errs() << "\n Reacihng instruction information: " << "\n";
				errs() << "\n No of lib load instructions: " << tmp->nooflibloadins << "\n";
				errs() << "\n No of lib store instructions: " << tmp->nooflibstoreins << "\n";

				for (auto Instruction : tmp->reachingInstructions) {
					errs() << *Instruction << "\n";
					}
				errs() << "\n";
			}
		}
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
			else if (cname.find("shmem") != std::string::npos && cname.find("put") != std::string::npos ) {
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
			} else {
				value = DEFAULT;
			}
			return value;
		}

		/*
	 Processes the given BB and prints callMetainfo
	 Gathers all shmem related call instructions into cibookeep.

	 Input: Basic Block BB 
	 Output: Prints call metainfo and it's corresponding base alloca instructions.
	*/

		virtual bool ProcessBasicBlock(BasicBlock &BB) {


			for (BasicBlock::iterator bbs = BB.begin(), bbe = BB.end(); bbs != bbe; ++bbs) {
                
				// typecasting iterator into instruction pointer.
				Instruction* ii = &(*bbs);
				// Create a callSite object from the ii. 
				// This helps us to make use of callsite api to get name effectively.
				// TODO : Possible alternative implemtation:    CallInst *ci = dyn_cast<CallInst>(ii);
				//deb-llvm10update
				//****CallSite cs(ii);
				CallInst *cs = dyn_cast<CallInst>(ii);
				// Check if ii is a call instruction.
				//deb-llvm10update
				//****if (!cs.getInstruction()) continue;
				
				if (cs == NULL) continue;
				// Gets rid of any complex pointer castings to this instruction.
				
				//deb-llvm10update
				//****Value* called = cs->getCalledFunction();
				//****if (Function *fptr = dyn_cast<Function>(called)) {
				if (Function *fptr = cs->getCalledFunction()) {
					string cname = fptr->getName().str();

					// Add provision to look for different shmem functions
								/*
								shmem_init 1
								shmem_put  2
								shmem_get  3
								default    4
								*/
					switch (GetFunctionID(cname)) {
					case 1: {
						errs() << ii->getOperand(0)->getName();
						errs() << *ii;
						break;
					}
							/* Handles shmem_get and shmem_put instructions. Left the case 2 without a break intentionally*/
					case 2:
					case 3: {
						CallInst *ci = dyn_cast<CallInst>(ii);
						errs() << "\n\n\nFound  fxn call: " << *ii << "\n";
						errs() << "Function call: " << fptr->getName() << "\n";
						errs() << "\t\t\t No of arguments: " << fptr->arg_size() << "\n";
						errs() << "\t this gets arguments properly: " << ci->getNumArgOperands() << "\n";
						CallMetaInfo *cmi = new CallMetaInfo(ci);
						PrintFunctionArgsCallMetaInfo(ci, cmi);
						callinstvec.push_back(cmi);
						Callinst2AllocaMap[ci] = cmi;
						break;
					}
					case 4:
					case 5:
					case 6:{
					
						break;
					}
					default:
						errs() << "Default case invoked: " << cname << "\n";
						break;

					}
				}
			}
		}


		void DisplayAllocaforCallInstruction(CallInst *ci) {
			if (Callinst2AllocaMap.find(ci) != Callinst2AllocaMap.end()) {
				CallMetaInfo *cmi = Callinst2AllocaMap[ci];
					//cmi->vva.push_back(va);   sampath

				for (auto &va : cmi->vva) {
					for (auto al : va) {
					errs() << *al;
					}
				}
			}

		}
		void DisplayCallstatistics(Instruction *ins, uint64_t &count) {
			Instruction* ii = ins;
			CallInst *ci = cast<CallInst>(ii);
			//deb-llvm10update
			//string cname = dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts())->getName().str();
			string cname = ci->getCalledFunction()->getName().str();
			errs() << "\t\tPrinting function name: " << cname << " occurs " << count << " times.\n";
			// We check fucntions which contains get and put functions. We match the function string cname with selected patterns.

			int functionID = GetFunctionID(cname);
			if ( functionID >=2 && functionID <= 3 ) {

				for (auto i = 0; i < ci->getNumArgOperands(); i++) {

					//ci->getArgOperand(i)->dump();
					if (ci->getArgOperand(i)->getType()->isPointerTy()) {

						errs() << "\t\t" << ci->getArgOperand(i)->stripPointerCasts()->getName().str() << "\n";
					} else {
						//errs() << ci->getArgOperand(i)->getName().str() << "\t";
						//errs() << ci->getOperand(i);
					}
				}
				//isIntegerTy()
				//case 1
				Type *a3, *a4;
				Value *v3 = ci->getArgOperand(3);
				a3 = ci->getArgOperand(2)->getType();
				a4 = ci->getArgOperand(3)->getType();
				//a4->dump();
				if (a4->isIntegerTy()) {

					// compare the values and see if it out of current PE
					if (ConstantInt* cint = dyn_cast<ConstantInt>(ci->getArgOperand(3))) {
						errs() << "const integer type\n";
						// foo indeed is a ConstantInt, we can use CI here
						errs() << "Const value: " << cint->getSExtValue() << "\n";
					} else {
						// foo was not actually a ConstantInt
						errs() << "Not a const\n";
					}
				} else {
					// Different types. It must me an integert according to the put and get definitions
					//errs() << "Different types\n";
				}
				errs() << "\t\tPrinting the actual PE argument: ";
				ci->getArgOperand(3)->dump();
				errs() << "\t\t************************************************************************ \n\n";
			}


			DisplayAllocaforCallInstruction(ci);
		}
		/* checks if this name is of shmem library*/
		virtual bool isCallOfInterest(string &cname) {

			int value = GetFunctionID(cname);
			return DEFAULT != value;

		}

		virtual bool isShmemCall(string &cname) {
			return (cname.find("shmem") != std::string::npos);
		}
		/* Gets the count of store library instructions */
		int getlibstoreinstructions(vector <Instruction *> &callinst) {
			int libload = 0;
			for (auto ii : callinst) {
				//deb-llvm10update
				//****CallSite cs(ii);
				CallInst *cs = dyn_cast<CallInst>(ii);
				//****Value* called = cs->getCalledFunction();
				//****if (Function *fptr = dyn_cast<Function>(called)) {
				if (Function *fptr = cs->getCalledFunction()) {
					string cname = fptr->getName().str();
					if (cname.find("get") != std::string::npos) libload++;
				}
			}
			return libload;
		}

		int getlibloadinstructions(vector <Instruction *> &callinst) {
			int libload = 0;
			for (auto ii : callinst) {
				//deb-llvm10update
				//****CallSite cs(ii);
				CallInst *cs = dyn_cast<CallInst>(ii);
				//****Value* called = cs->getCalledFunction();
				//****if (Function *fptr = dyn_cast<Function>(called)) {
				if (Function *fptr = cs->getCalledFunction()) {
					string cname = fptr->getName().str();
					if (cname.find("put") != std::string::npos) libload++;
				}
			}

			return libload;
		}
		/*   This helper function gathers information required to populate heat node info for evrry basic block.
		Input: Basic Block to be analysed.
		Output: vec has shmem library calls of interest,
				call inst has all the shmem library call instructions, 
				loadstorecnt populates
					all the load, store instructions in this Basic Block.

		*/
		virtual bool isBlockOfInterest(BasicBlock &B, vector <Instruction *> &vec, vector <Instruction *> &callinst, pair<int, int> &loadstorecnt) {
			bool interest = true;
			int loadcnt = 0, storecnt = 0;
			for (auto &I : B) {
				Instruction* ii = &I;
				if (isa<LoadInst>(ii)) loadcnt++;
				else if (isa<StoreInst>(ii)) storecnt++;
				//deb-llvm10update
				//****CallSite cs(ii);
				CallInst *cs = dyn_cast<CallInst>(ii);
				
				//deb-llvm10update
				//****if (!cs.getInstruction()) continue;
				if (cs == NULL) continue;

				//****Value* called = cs->getCalledFunction();
				//****if (Function *fptr = dyn_cast<Function>(called)) {
				if (Function *fptr = cs->getCalledFunction()) {
					string cname = fptr->getName().str();
					if (isShmemCall(cname)) {
						callinst.push_back(ii);
					}
					if (isCallOfInterest(cname)) {
						CallInst *ci = cast<CallInst>(ii);
						//errs() << "Function call: " << fptr->getName() << "\n";
						interest = true;
						vec.push_back(ii);
					}
				}
			}
			loadstorecnt.first = loadcnt;
			loadstorecnt.second = storecnt;
			return interest;
		}

		bool Is_var_defed_and_used(VariableMetaInfo *varinfo) {


			int i = 1;
			/*for (auto *use : varinfo->alloca->users()) {
				Instruction *useinst;
				errs() << i++ << " \t" << *(dyn_cast<Instruction>(use)) << "\n";

				if (useinst = dyn_cast<GetElementPtrInst>(use)) {
					errs() << "*******************GEPI found\n";
				}
			}*/

			for (auto *use : varinfo->alloca->users()) {
				Instruction *useinst;
				errs() << i++ << " \t" << *(dyn_cast<Instruction>(use)) << "\n";

				if (useinst = dyn_cast<GetElementPtrInst>(use)) {
					errs() << "*******************GEPI found\n";
				}


				if ((useinst = dyn_cast<LoadInst>(use))) {
					//useinst->print(errs()); errs() << "\n";
					if (Inst2VarInfo_map.find(useinst) == Inst2VarInfo_map.end()) {
						Inst2VarInfo_map[useinst] = varinfo;
						//errs() << "\nLoad dump:\n";
						//useinst->dump();
					} else {
						errs() << "Replacing an existing entry\n";
					}
				} else if ((useinst = dyn_cast<StoreInst>(use))) {
					//useinst->print(errs()); errs() << "\n";
					if (useinst->getOperand(1) == varinfo->alloca) {
						Inst2VarInfo_map[useinst] = varinfo;
						varinfo->defblocks.insert(useinst->getParent());
					} else {
						return false;
					}
				} else {
					errs() << "\n|||||||||||Looping out|||||||||||||||||||";
					//useinst->print(errs()); errs() << "\n";
					return false;
				}
			}
			return true;
		}

		void printEveryInstruction(Function &Func) {

			for (auto block = Func.getBasicBlockList().begin(); block != Func.getBasicBlockList().end(); block++) {
				for (auto inst = block->begin(); inst != block->end(); inst++) {
					for (Use &U : inst->operands()) {
						Value *v = U.get();
						if (dyn_cast<Instruction>(v)) {
							errs() << "\n\"" << *dyn_cast<Instruction>(v) << "\n\t" << "\"" << " -> " << "\"" << *inst << "\"" << ";\n";
						}
					}
					errs() << "used\n";
				}
			}
		}


		// void processLLVMDbgDec(Function &Func){
            
  //           for (auto &Ins : Func.getEntryBlock()) {

            
  //               if (isa<CallInst>(Ins)) {

     
  //                   StringRef calledFN = cast<CallInst>(Ins).getCalledFunction()->getName();

  //                   if(calledFN == "llvm.dbg.declare" ){
                        
  //                       Instruction *II = &Ins;
  //                       CallInst *CI = dyn_cast<CallInst>(II);
		// 				AllocaInst *AI;    /* AllocaInst is the result */
                        
		// 				Metadata *Meta = cast<MetadataAsValue>(CI->getOperand(1))->getMetadata();
						
		// 				if (isa<ValueAsMetadata>(Meta)) {
		// 					Value *V = cast <ValueAsMetadata>(Meta)->getValue();
		// 					AI = cast<AllocaInst>(V);

		// 				}

		// 				DIVariable *V = cast<DIVariable>(cast<MetadataAsValue>(CI->getOperand(1))->getMetadata());
                        

  //                       if(V!=NULL){
  //                          errs()<<V->getName()<<"\n ";
  //                          errs()<<V->getLine()<<"\n ";
  //                          errs()<<V->getDirectory()<<"\n ";
  //                          errs()<<V->getFilename()<<"\n ";
  //                       }
  //                   }

  //               }

		// 	}

		// }

		void processAllocaInstructions(Function &Func) {

			for (auto &insref : Func.getEntryBlock()) {
				Instruction *I = &insref;

				if (isa<CallInst>(insref)) {

                 //Handle LLVM Debug Declare fnction information for fetching source level details of variables
				 //involved in alloca instructions.
				 if(cast<CallInst>(insref).getCalledFunction()!=NULL){
                    string calledFN = cast<CallInst>(insref).getCalledFunction()->getName().str();

                    if(calledFN == "llvm.dbg.declare" ){
                        
                        Instruction *II = &insref;
                        CallInst *CI = dyn_cast<CallInst>(II);
						AllocaInst *AI;    /* AllocaInst is encoded in the first metadata argument */
                        
						Metadata *Meta = cast<MetadataAsValue>(CI->getOperand(0))->getMetadata();
						
						if (isa<ValueAsMetadata>(Meta)) {
							Value *V = cast <ValueAsMetadata>(Meta)->getValue();
							AI = cast<AllocaInst>(V);
							//errs()<<"####result:"<<AI<<"\n";

						}

						DIVariable *V = cast<DIVariable>(cast<MetadataAsValue>(CI->getOperand(1))->getMetadata());
                        

                        if(V!=NULL){
                           errs()<<"\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n ";
                           errs()<<"Source var name:"<<V->getName()<<"\n ";
                           errs()<<"Row in Source file:"<<V->getLine()<<"\n ";
                           errs()<<"Path of Source file:"<<V->getDirectory()<<"\n ";
                           errs()<<"Source file name:"<<V->getFilename()<<"\n ";
                           
                        }
                     
                     for (auto i = allocaV.begin(); i != allocaV.end(); ++i){
                          
				          AllocaInst *alloca = dyn_cast_or_null<AllocaInst>(*i);
				          //errs()<<"Allocavector:"<<*i <<"alloca:"<<alloca<< "\n"; 
				          //if the alloca in llv.dbg.declare call, is found in allocaV vector
				          //Run and print the alloca identification in every call instruction
				          if(AI == alloca) 
				             PrintAllocaRelatedInstructions(alloca);

                        } 

                       } 
                    }

                }

				// We check if the given instruction can be casted to a Alloca instruction.
				if (AllocaInst *alloca = dyn_cast_or_null<AllocaInst>(&insref)) {
					//errs() << " \n Identified a alloca instruction : " << (I)->getNumOperands();
					//Store the alloca instuction in a vector.
					allocaV.push_back(&insref);

				}

				
			}
		}

		// Run the alloca identification in every call instruction
		void PrintAllocaRelatedInstructions(AllocaInst *alloca) {

				// 	/* This sets if the alloca instruction is of specific size or not.*/
				bool is_interesting = (alloca->getAllocatedType()->isSized());
				// 	//errs() << " \n issized (): " << is_interesting << "\nisstaticalloca: " << alloca->isStaticAlloca();
				// 	//errs() << " is array allocation: " << alloca->isArrayAllocation();
				// 	//errs() << "\n getallocasizeinbytes(): " << getAllocaSizeInBytes(alloca);
				bool isArray = alloca->isArrayAllocation() || alloca->getType()->getElementType()->isArrayTy();

				errs() << "Pointer type allocation: " << alloca->getAllocatedType()->isPointerTy()<<"\n";
			    errs() << "Array type allocation: " << alloca->getAllocatedType()->isArrayTy()<<"\n";
				// 	//if (isArray) errs() << " array[" << *(alloca->getArraySize()) << "]"  << *(alloca->getOperand(0)) <<"\n";

				VariableMetaInfo  *varinfo = new VariableMetaInfo(alloca);

				// 	/* tells if it is sized*/
				if (alloca->isStaticAlloca()) {
					varinfo->is_static_alloca = true;
				}
				// 	/* Tells if the alloca is a pointer allocation*/
				if (alloca->getAllocatedType()->isPointerTy()) {
					varinfo->isPointer = true;
				}
				// 	/* check if an allocation is array and retrieve it's size*/
				if (isArray || alloca->getAllocatedType()->isArrayTy()) {

				// 		The AllocaInst instruction allocates stack memory.The value that it
				// 			returns is always a pointer to memory.

				// 			You should run an experiment to double - check this, but I believe
				// 			AllocaInst::getType() returns the type of the value that is the result
				// 			of the alloca while AllocaInst::getAllocatedType() returns the type of
				// 			the value that is allocated.For example, if the alloca allocates a
				// 			struct { int; int }, then getAllocatedType() returns a struct type and
				// 			getType() return a "pointer to struct" type.

				// 			//errs() << "size : " << cast<ArrayType>(alloca->getAllocatedType())->getNumElements() << "\n";
				      errs() << "Allocated type" << *(alloca->getAllocatedType()) << " \n";

				// 		Value* arraysize = alloca->getArraySize();
				// 		/*Value* totalsize = ConstantInt::get(arraysize->getType(), CurrentDL->getTypeAllocSize(II->getAllocatedType()));
				// 		totalsize = Builder->CreateMul(totalsize, arraysize);
				// 		totalsize = Builder->CreateIntCast(totalsize, MySizeType, false);
				// 		TheState.SetSizeForPointerVariable(II, totalsize);*/
						const ConstantInt *CI = dyn_cast<ConstantInt>(alloca->getArraySize());
						varinfo->is_array_alloca = true;
						varinfo->arraysize = cast<ArrayType>(alloca->getAllocatedType())->getNumElements();
						//varinfo->arraysize = CI->getZExtValue();
						errs() << "\nAlloca instruction is an array inst of size : " << *(CI) << " sz  " << varinfo->arraysize;
					}

					if (Is_var_defed_and_used(varinfo)) {
						// variableinfos
						Variableinfos.push_back(varinfo);
					} else {
						delete varinfo;
					}



					for (auto op = alloca->op_begin(); op != alloca->op_end(); op++) {
						Value* v = op->get();
						StringRef name = v->getName();
						errs() << name << "\t alloca name:\n";
					}
					
					const llvm::DebugLoc &debugInfo = alloca->getDebugLoc();
					if (debugInfo)
					{
						//deb-llvm10 update
						//std::string filePath = debugInfo->getFilename();
						llvm::StringRef filePath = debugInfo->getFilename();
						int line = debugInfo->getLine();
						int column = debugInfo->getColumn();
						errs() << alloca << "::" << "File name = " << filePath << "Line no: " << line << ":" << column << "!\n";
					}
					else
					{
						errs() << "No Debug Info" << "!\n";
					}

			
		}

		// maintains mapping between call instruction and it's operands alloca mappings
		void ProcessAllBasicBlocks(Function &Func) {
			for (Function::iterator Its = Func.begin(), Ite = Func.end(); Its != Ite; ++Its) {
				errs() << "Calling PBB 0: ";
				ProcessBasicBlock(*Its);
			}
		}

		void LiveProcessAllBasicBlocks(Function &Func, vector<Instruction *> termIarray) {
			for (Function::iterator Its = Func.begin(), Ite = Func.end(); Its != Ite; ++Its) {
				Instruction *termI = Its->getTerminator();
				errs() << "termivator instruction: " << *termI << "\n";
				termIarray.push_back(termI);
			}
		}
		/* This assigns every instruction with a similar naming conventtion as in RDA and gets all teminal instructions.
		Input: Function  and Index to Instruction mapping.
		Output: Terminal instructions  and fills IndexToInstr
		*/
		vector<unsigned> assignIDsToInstrs(Function &F, vector<Instruction *> &IndexToInstr)
		{
			// Dummy instruction null has index 0;
			// Any real instruction's index > 0.
			//InstrToIndex[nullptr] = 0;
			//IndexToInstr[0] = nullptr;
			IndexToInstr.push_back(nullptr);

			unsigned counter = 1;
			vector<unsigned> termIDs;
			//for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
			for (Function::iterator bb = F.begin(), e = F.end(); bb != e; ++bb) {
				for (BasicBlock::iterator i = bb->begin(), e = bb->end(); i != e; ++i) {
						Instruction *instr = &(*i);
						//InstrToIndex[i] = counter;
						IndexToInstr.push_back(instr);
						if (instr->isTerminator()) {
							errs() << "terminator instruction: " << *instr << "\n";
							termIDs.push_back(counter);
						}
						counter++;
					}
				}
			return termIDs;
		}
		vector<unsigned> getIDsfortermarray(vector<Instruction *> &termIarray, vector<Instruction *> &IndexToInstrv) {
			vector <unsigned> ans;
			for (auto el1 : termIarray) {
				for (int i = 1; i < IndexToInstrv.size(); i++) {
					if (el1->isIdenticalTo(IndexToInstrv[i])) {
						ans.push_back(i);
						errs() << "found: " << *el1 << "\n";
						break;
					}
				}
			}
			return ans;
		}

		virtual bool runOnFunction(Function &Func) {

			errs().write_escaped(Func.getName());
			errs() << "\n\n************************************************************************ \n\n";
			errs() << "\nFunction Name: " << Func.getName();
			

			errs() << "\n\tFunction size " << Func.size();


			//printResult();
			//@Sampath
			/*
			 * Get hold of alloca instructions. Since, these intructions are Alloca we can use getEntryBlock() to
			 * iterate over the first few ones.
			 */
			//@deb
			//********Update-Spring2020**********************//
			/*
			 * Store alloca instuctions in a vector 
			 * Get hold of llvm.dbg.declare calls for printing source level information
			 * Find the corresponding alloca instruction from the first argument of llvm.dbg.declare calls
			 * Search this alloca instuction in the vector, if match is found, Run the alloca identification in every call instruction
			 */
			processAllocaInstructions(Func);
			// Run the alloca identification in every call instruction
			errs() << "\n\n************************************************************************ \n\n";
			errs() << "Run the alloca identification in every call instruction \n";

			

			ProcessAllBasicBlocks(Func);

			functionMap.clear();

			errs() << "\n\n************************************************************************ \n\n";
			errs() << "Running the Block Frequency Estimation Part \n";

			vector <Instruction *> insv, callinst;
			bool loop = false;

			/*

			TTR: This part is supposed to give us Branch probability information. Since, this requires this pass to 
			be a module pass. I have commented this part. This will come handy in the future.

			BranchProbabilityInfo &BPI = getAnalysis<BranchProbabilityInfoWrapperPass>().getBPI();
			LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
			BlockFrequencyInfo &locBFI = getBFI();
			locBFI.calculate(Func, BPI, LI);

			dbgs() << "&&&&&&&&&&&&&&&&&&&&&&\n";
			locBFI.print(llvm::dbgs());
			dbgs() << "%%%%%%%%%%%%%%%%%%%%%\n";
			BPI.print(llvm::dbgs());
			*/


			// for every bb in a loop mark them appropriately.
			LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
			/* From the meta info we gathered so far, we populate the head node information. we also have enought data to decide on whether a
			basic block is important to us or not. This info is cached for every Basic Block.*/
			// iterate through each block
			for (auto &B : Func) {
				/*uint64_t BBprofCount = locBFI.getBlockProfileCount(&B).hasValue() ? locBFI.getBlockProfileCount(&B).getValue() : 0;
				uint64_t BBfreqCount = locBFI.getBlockFreq(&B).getFrequency();
				*/
				uint64_t BBprofCount = 0;
				uint64_t BBfreqCount = 0;
				insv.clear();
				pair<int, int> loadstorecnt;
				loadstorecnt.first = 0;
				loadstorecnt.second = 0;

				if (isBlockOfInterest(B, insv, callinst, loadstorecnt)) {
					heatNode *hnode = new heatNode(id, &B);
					hnode->setfreqcount(BBfreqCount);
					hnode->setprofcount(BBprofCount);
					hnode->setnoofloadins(loadstorecnt.first);
					hnode->setnoofstoreins(loadstorecnt.second);
					errs() << "**********************************\nprof count: " << BBprofCount << "\t freq count: " << BBfreqCount;

					errs() << " This block  : \t" << B.getName() << " has\t " << B.size() << " Instructions.\n";
					errs() << " Found " << callinst.size() << " shmem related call instructions\n";
					errs() << " Found " << loadstorecnt.first << " load instructions\n";
					errs() << " Found " << loadstorecnt.second << " store instructions\n";
					errs() << " Display Call statistics: \n";
					hnode->setnoofcallins(callinst.size());
					/* Collecting lo store and lib load frequncy count of shmem lib calls*/
					int libload = getlibloadinstructions(callinst);
					int libstore = getlibstoreinstructions(callinst);
					
					hnode->nooflibloadins = libload;
					hnode->nooflibstoreins = libstore;

					for (auto ins : insv) {
						DisplayCallstatistics(ins, BBprofCount == 0 ? BBfreqCount : BBprofCount);
					}
					/*loop = LI.getLoopFor(&B);
					if (loop == false) {
						errs() << "Not an affine loop. Not of some interest\n";
					} else {
						// handle cases here
						errs() << "Affine loop found here\n";
						//errs() << loop->getCanonicalInductionVariable()->getName() << "\n";
					}*/
					//hnode->setatlcount(2);
					/* TTR: We don't need it now. Will be done later*/
					bool isLoop = LI.getLoopFor(&B);
					if (isLoop) {
						//hnode->setatlcount(1);
						//errs() << B.getName() << " block is inside loop\n";
						//errs() << B.getName() << " block is inside loop\n";
					}
					/* TTR: End*/


					heatmp[&B] = hnode;
					heatIDmp[id] = hnode;
					id++;
					callinst.clear();
					loadstorecnt.first = 0;
					loadstorecnt.second = 0;
				}
			}
			
	
			errs() << " I am erroring out\n";
			//getLoopFor
			for (auto &B : Func) {
				bool isLoop = LI.getLoopFor(&B);
				heatmp[&B]->setatlcount(LOOP_BODY);
				errs() << B.getName() << " block is inside loop\n";
			}
			/*
			for (Loop::iterator i = LI.begin(), e = LI.end(); i != e; ++i) {
		
				//std::vector<bbt*> loopBlocks = i->getBlocks();
				for (auto tmpbb : i->getBlocks()) {
					errs() << *tmpbb << " block is inside loop\n";
					// mark this heat node with that of loops entry
				}
			}*/
			/* Loops over all loops. Tag all Basic Blocks related to loop as loop body. And selectively tag loop headers and loop tails. */
			for (Loop *L : LI) {
				for (BasicBlock *BB : L->getBlocks())
				{
					dbgs() << "basicb name: " << BB->getName() << "\n";
					heatmp[BB]->setatlcount(LOOP_BODY);
				}

				// header
				bbt sheader = L->getHeader();
				if (sheader) {
					heatmp[sheader]->setatlcount(LOOP_HEADER);
				}

				
				// Get all the exir blocks and tag them as Loop tail
				SmallVector<BasicBlock*, 8> ExitingBlocks;
				L->getExitingBlocks(ExitingBlocks);
				for (BasicBlock *ExitingBlock : ExitingBlocks)heatmp[ExitingBlock]->setatlcount(LOOP_TAIL);

			}


			errs() << "\nLivenessAnalysis";
			/* This runs the libeness anlysis algorithm. populated all required information for us to consume.*/
			rda->runWorklistAlgorithm(&Func);
			/* This prints reaching definitions for every edge. But we need that info just for terminal instruciton of Basic Block. 
			This is because we are interested in block wise reaching definiiton analyis.*/
			rda->print();
			/* Expose edges from reaching definition analysis pass.
				These edges are defined as forward edges and backward edges.

				Edges are defined between two instructions which come one after other(forward or backward). When multiple blocks emerge from 
				terminal instruction. It will be having mulitple edges from that instruction.
			
			*/

			/* get edge information from the liveness analysis*/
			auto EdgeToInfo = rda->getEdgeToInfo();

			/* To make sense of edge information. we are assigning ID's to each and every instruction. */
			vector<Instruction *> IndexToInstrv;
			vector<unsigned> termIDs = assignIDsToInstrs(Func, IndexToInstrv);
			
			/* Mark all the terminal instructions as true for printing Reaching definition analysis for every block*/
			map<unsigned, bool> termIDsmap;
			for (auto el : termIDs) termIDsmap[el] = true;

			errs() << "\nLivenessAnalysis for every block ";

			/* For each terminal instruction we extract reaching definition analysis and insert into heat node for later use*/
			for (auto const &it : EdgeToInfo) {
				vector<Instruction *> tmp;
				/* The first entry in the edge correspons to actual instruction  edge id1->id2    id1 is what we require.*/
				if (termIDsmap.find(it.first.first) != termIDsmap.end()  /*termIDsmap.find(it.first.second) != termIDsmap.end()*/) {
					errs() << "Edge " << it.first.first
						<< "->"
						"Edge "
						<< it.first.second << ":";
					(it.second)->print();

					// Get the parent Basic  block
					bbt curBB = IndexToInstrv[it.first.first]->getParent();
					errs() << "reaching instructions from this block :" << *curBB << "\n";

					/* add  reaching definition info to each heat node for every basic block*/
					for (auto el : (it.second)->getliveness_defs()) {
						errs() << *IndexToInstrv[el] << "\n";
						tmp.push_back(IndexToInstrv[el]);
						heatmp[curBB]->reachingInstructions.push_back(IndexToInstrv[el]);
					}
					
					//heatmp[curBB]->reachingInstructions = tmp;
					errs() << "\n\n\n";
				}

			}
			//auto InstrToIndex = rda->getInstrToIndex();
			//auto IndexToInstr = rda->getIndexToInstr();


			errs() << "\nPrinting the instruction index mapping\n";
			/* This prints the instruction to ID mappping*/
			for (auto el : IndexToInstrv) {
				//Instruction *inst = &(*(el.second));
				if (el == nullptr) continue;
				errs() << *el << "\n";
			}
			vector<Instruction *> worklist;
			/*
			 * printEveryInstruction(Func);
			 */

			errs() << "\nprinting terminal instructions\n";


			/* TTR: This has been done before*/
			vector<Instruction *> termIarray;
			LiveProcessAllBasicBlocks(Func, termIarray);
			//auto termIDs = getIDsfortermarray(termIarray , IndexToInstrv);
			for (auto el : termIarray) errs() << "After live get term call: " << *el << "\n";
			/*for (int i = 0; i < termIDs.size(); i++) {
				errs() << "ID: " << termIDs[i] << " --> " << *termIarray[i] << "\n";
			}*/
			/*TTR: End*/


			errs() << '\n';
			/* print the heat node info finally*/
			printheadnodeinfo();

			return false;
		}
	};


	/*RegisterPass<shmemheat> X("shmemheat", "Prints shmem heat function  analysis");*/
}


char shmemheat::ID = 0;
//@Abdullah
//
/*
namespace llvm {
	void initializeshmemheatPass(PassRegistry &);
} */
void initializeshmemheatPass(PassRegistry &);
//char shmemheat::ID = 0;
INITIALIZE_PASS(shmemheat, "shmemheatpass",
	"Prints shmem heat function  analysis", false, false)


/*bool UsingArray = false;
//errs() << "Number of opeands: " << I->getNumOperands() << "\n";
for (unsigned num = 0; num < I->getNumOperands(); ++num)
	if (isa<ArrayType>(I->getOperand(num)->getType())) {
		errs() << "\nAlloca instruction is an array inst";
		UsingArray = true;
	}*/

	//I->print(errs());
	//errs() << "************\n";
	//errs() << "number of operands : " << insref.getNumOperands();
	//insref.dump();
	//errs() << "\n";



	/*virtual bool runOnModule(Module &M)
	{
		for (auto F = M.begin(), E = M.end(); F != E; ++F)
		{
			runOnFunction(*F);
		}
		return false;
	}*/


	//Value* val = arg;
	//errs() << val->getName().str() << " -> " << "\n";
	//errs() << ci->getArgOperand(i)->getName() << " \t";
	//errs() << "\t\t\t\t"<< ci->getArgOperand(i)->getValue() << "\n";


	//if (isa<CallInst>(ii)) {
	/*
	if (string(bbs->getOpcodeName()) == "call") {

			CallInst *ci = cast<CallInst>(ii);
			//errs() << "\t\t "<< cast<CallInst>(ii).getCalledFunction().getName()<< "\n";
			errs() << "\t\t "<< dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts()).getName()<< "\n";
			errs() << "\t\t"<<ParseFunctionName(ci) << "\n";
			PrintFunctionArgsCallMetaInfo(ci);
			errs() << bbs->getOpcodeName() << '\t';
			bbs->printAsOperand(errs(), false);
			errs() << '\n';
			functionMap[string(bbs->getOpcodeName())]++;
		}
		*/

		/*

		if (string(bbs->getOpcodeName()) == "call") {

			CallInst *ci = cast<CallInst>(ii);
			//errs() << "\t\t "<< cast<CallInst>(ii).getCalledFunction().getName()<< "\n";
			//errs() << "\t\t BokkaBokka" << dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts())->getName().str() << "\n";
			//errs() << "\t\t" << ParseFunctionName(ci) << "\n";
			//PrintFunctionArgsCallMetaInfo(ci);
			string cname = dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts())->getName().str();
			if (cname.find("sh_") != std::string::npos) {

				errs() << bbs->getOpcodeName() << '\t';
				bbs->printAsOperand(errs(), false);
				errs() << '\n';
				functionMap[string(bbs->getOpcodeName())]++;
			}
		}

		*/

		/*
			explain what  needs to be done
			Problmes: eeverywhere
			ex: which poart to use
				differentia; chckpoint system
					use later
					explain:
						example: wih examples
						Have them side by side and exaplaint
*/
