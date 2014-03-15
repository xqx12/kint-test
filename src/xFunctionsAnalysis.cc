#define DEBUG_TYPE "xqx-cfg"
#include <llvm/IRBuilder.h>
#include <llvm/Instructions.h>
#include <llvm/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/Support/GraphWriter.h"
#include <llvm/ADT/OwningPtr.h>


using namespace llvm;


namespace {

struct NamedParam {
	const char *Name;
	unsigned int Index;
};

struct xFunctionAnalysisPass : FunctionPass {
	static char ID;
	//xFunctionAnalysisPass() : ModulePass(ID) {}
	xFunctionAnalysisPass() : FunctionPass(ID) {}

	bool runOnModule(Module &);
	bool runOnFunction(Function &F); 

private:
	typedef IRBuilder<> BuilderTy;
	BuilderTy *Builder;

    void print(raw_ostream &O, const Module* = 0) const { }

    // getAnalysisUsage - This pass requires the CallGraph.
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
    }
};

} // anonymous namespace


bool xFunctionAnalysisPass::runOnFunction(Function &F) {
	
	llvm::errs() << "Func: " << F.getName() << "\n";

	for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
		Instruction *I = &*i;
//		llvm::errs() << I->getOpcodeName() << "\n";
		//CallInst *ICall = dyn_cast<CallInst>(&*I);
		if( I->getOpcode() == Instruction::Call ){
			CallInst *ICall = dyn_cast<CallInst>(&*I);
			StringRef FName = ICall->getCalledFunction()->getName();
			if( FName == "int_overflow" ){
				llvm::errs() << "callee: " << FName << "\n";
				break;
			}
		}
		
	}
	
	return true;
}


char xFunctionAnalysisPass::ID;

static RegisterPass<xFunctionAnalysisPass>
X("xqx-function-analysis", "Analysis functions in a Module ");
//INITIALIZE_PASS(xFunctionAnalysisPass, "xqx-cfg","view cfgs", false, true)
