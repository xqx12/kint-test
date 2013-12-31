#define DEBUG_TYPE "xqx-cfg"
#include <llvm/IRBuilder.h>
#include <llvm/Instructions.h>
#include <llvm/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/Analysis/Dominators.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/GraphWriter.h"
#include <llvm/ADT/OwningPtr.h>

#include "PathGen.h"
#include "SMTSolver.h"
#include "ValueGen.h"

using namespace llvm;


namespace {

struct NamedParam {
	const char *Name;
	unsigned int Index;
};

struct xCFGPass : FunctionPass {
	static char ID;
	//xCFGPass() : ModulePass(ID) {}
	xCFGPass() : FunctionPass(ID) {
//      initializexCFGPass(*PassRegistry::getPassRegistry());
	}

	bool runOnModule(Module &);
	bool runOnFunction(Function &F); 

private:
	typedef IRBuilder<> BuilderTy;
	BuilderTy *Builder;
	OwningPtr<DataLayout> TD;

    void print(raw_ostream &O, const Module* = 0) const { }

    // getAnalysisUsage - This pass requires the CallGraph.
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
     // AU.addRequired<CallGraph>();
    }
//	void rewriteSize(Function *F);
//	void rewriteSizeAt(CallInst *I, NamedParam *NPs);
};

} // anonymous namespace




//bool xCFGPass::runOnModule(Module &M) {
	//for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i) {
		
		//Function &F = *i;
		//Function *FF = i;
		//if (&F)
		//{
			//llvm::errs() << "CallFunction: " << F.getName() << "--------\n";
		//}
		//else 
		//{
			//llvm::errs() << "***NULL Function***\n";
			//continue;
		//}

		//std::string Filename = "cfg." + F.getName().str() + ".dot";
		//errs() << "Writing '" << Filename << "'...";
		//llvm::errs() << F << "\n";

		//std::string ErrorInfo;
		//raw_fd_ostream File(Filename.c_str(), ErrorInfo);

		//if (ErrorInfo.empty())
			//WriteGraph(File, (const Function*)&F);
		//else
			//errs() << "  error opening file for writing!";
		//errs() << "\n";
	//}
	//return false;
//}

//bool xCFGPass::runOnFunction(Function &F) {
	//std::string Filename = "cfg." + F.getName().str() + ".dot";
	//errs() << "Writing '" << Filename << "'...";

	//std::string ErrorInfo;
	//raw_fd_ostream File(Filename.c_str(), ErrorInfo);

	//if (ErrorInfo.empty())
		//WriteGraph(File, (const Function*)&F);
	//else
		//errs() << "  error opening file for writing!";
	//errs() << "\n";
	//return false;
//}


bool xCFGPass::runOnFunction(Function &F) {
	
	llvm::errs() << "Func: " << F.getName() << "\n";
	Instruction *IIntOf = NULL;
	for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
		Instruction *I = &*i;
//		llvm::errs() << I->getOpcodeName() << "\n";
		//CallInst *ICall = dyn_cast<CallInst>(&*I);
		if( I->getOpcode() == Instruction::Call ){
			CallInst *ICall = dyn_cast<CallInst>(&*I);
			StringRef FName = ICall->getCalledFunction()->getName();
			if( FName == "int_overflow" ){
				llvm::errs() << "callee: " << FName << "\n";
				IIntOf = I;
				break;
			}
		}
		
	}
	
	if( IIntOf == NULL ) return false;

	BasicBlock *curBB = IIntOf->getParent();
	llvm::errs() << "curbb:" << curBB->getName() << "\n\n";
	pred_iterator i, e =  pred_end(curBB);
//	BasicBlock *endBB = *e;
	llvm::errs() << "curbb:" << curBB->getName() << "\n\n";
//	llvm::errs() << "endbb:" << endBB->getName() << "\n\n";

	
	
	for (i = pred_begin(curBB); i != e; )
	{

		BasicBlock *bb = *i;
		//bb->getName();
		llvm::errs() << "\n\n" <<  bb->getName() << "\n";
		int count = 0;
		for(pred_iterator itr=pred_begin(bb), ite=pred_end(bb); itr!=ite; itr++)
		{
			BasicBlock *bbtmp = *itr;
			llvm::errs() << "pre_BBs" << ++count   << ":" << bbtmp->getName() << "\n";
		}
		i = pred_begin(bb);

	}


	return true;
}


char xCFGPass::ID;

static RegisterPass<xCFGPass>
X("xqx-cfg", "traverse the control flow in a function ");
//INITIALIZE_PASS(xCFGPass, "xqx-cfg","view cfgs", false, true)
