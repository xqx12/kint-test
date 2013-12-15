#define DEBUG_TYPE "xqx-callgraph"
#include <llvm/IRBuilder.h>
#include <llvm/Instructions.h>
#include <llvm/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Support/CFG.h"
#include "llvm/ADT/SCCIterator.h"

using namespace llvm;

namespace {

struct NamedParam {
	const char *Name;
	unsigned int Index;
};

struct xCallGraphPass : ModulePass {
	static char ID;
	xCallGraphPass() : ModulePass(ID) {}

	bool runOnModule(Module &);

private:
	typedef IRBuilder<> BuilderTy;
	BuilderTy *Builder;

    void print(raw_ostream &O, const Module* = 0) const { }

    // getAnalysisUsage - This pass requires the CallGraph.
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
      AU.addRequired<CallGraph>();
    }
//	void rewriteSize(Function *F);
//	void rewriteSizeAt(CallInst *I, NamedParam *NPs);
};

} // anonymous namespace




//bool xCallGraphPass::runOnModule(Module &M) {
	//unsigned sccNum = 0;
	//llvm::errs() << "SCC-CG in runOnModule\n";
	//CallGraphNode *rootNode = getAnalysis<CallGraph>().getRoot();
	//llvm::errs() << "SCC-CG in postOrder:";

	//for (scc_iterator<CallGraphNode*> SCCI = scc_begin(rootNode),
			//E = scc_end(rootNode); SCCI != E; ++SCCI) {
		//const std::vector<CallGraphNode*> &nextSCC = *SCCI;
		//errs() << "\nSCC #" << ++sccNum << " : " << nextSCC.size();
		
		//for (std::vector<CallGraphNode*>::const_iterator I = nextSCC.begin(),
				//E = nextSCC.end(); I != E; ++I){
			//errs() << ((*I)->getFunction() ? (*I)->getFunction()->getName()
					//: "external node") << ", ";
		//}
		//if (nextSCC.size() == 1 && SCCI.hasLoop())
			//errs() << " (Has self-loop).";
	//}
	//errs() << "\n";

	//return true;
//}

bool xCallGraphPass::runOnModule(Module &M) {
	CallGraph &CG = getAnalysis<CallGraph>();
//	CG.dump();
//	CG.viewGraph();
//	DominatorTreeBase<CallGraphNode> *DTB;
//	DTB = new DominatorTreeBase<CallGraphNode>(false);
//	DTB->recalculate(CG);

//	DTB->print(errs());

	CallGraphNode *cgNode = CG.getRoot();
	cgNode->dump();

	
	for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i) {
		
		//Function &F = *i;
		Function *F = i;
		if (F)
		{
			llvm::errs() << "CallFunction: " << F->getName() << "--------\n";
			errs() << "\tCallees:\n";
		}
		else 
		{
			llvm::errs() << "***NULL Function***\n";
			continue;
		}
		cgNode = CG.getOrInsertFunction(F);
//		cgNode->dump();
		for (CallGraphNode::const_iterator I = cgNode->begin(), E = cgNode->end();
				I != E; ++I){
			llvm::errs() << "\tCS<" << I->first << "> calls";
			if (Function *FI = I->second->getFunction())
				llvm::errs() << "\tfunctionName '" << FI->getName() <<"'\n";
			else
				llvm::errs() << "\texternal node\n";
			
		}

	}
	return true;
}

char xCallGraphPass::ID;

static RegisterPass<xCallGraphPass>
X("xqx-cg", "traverse the call graph test");
