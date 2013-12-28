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
#include "llvm/DebugInfo.h"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

using namespace llvm;

namespace {

struct NamedParam {
	const char *Name;
	unsigned int Index;
};

struct xCallGraphPass : ModulePass {
	static char ID;
	xCallGraphPass() : ModulePass(ID) {}

	typedef std::map<Function *, Instruction *> CalledFunc;
//	typedef std::vector<CalledFunc> CalledFunctions;
	typedef std::vector< std::pair<Function *, Instruction *> > CalledFunctions;
	typedef std::map<const Function *, CalledFunctions> FunctionMapTy;
//	typedef std::map<const Function *, CallGraphNode * > FunctionMapTy;
	FunctionMapTy  calledFunctionMap;
	
	typedef std::pair<const llvm::BasicBlock *, const llvm::BasicBlock *> Edge;
	SmallVector<Edge, 32> BackEdges;
	typedef llvm::SmallVectorImpl<Edge> EdgeVec;
	//const EdgeVec &Backedges;

	std::map<std::string,Constant*> SourceFiles;
	Module* module;
	llvm::LLVMContext* context;
	Constant* constZero;

	bool runOnModule(Module &);
private:
	typedef IRBuilder<> BuilderTy;
	BuilderTy *Builder;

	DominatorTree *DT;

    void print(raw_ostream &O, const Module* = 0) const { }

    // getAnalysisUsage - This pass requires the CallGraph.
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
      AU.addRequired<CallGraph>();
      AU.addRequired<DominatorTree>();
    }
//	void rewriteSize(Function *F);
//	void rewriteSizeAt(CallInst *I, NamedParam *NPs);
	void printCalledFuncPath(Function *srcFunc, Function *dstFunc);
	void printCalledFuncAndCFGPath(Function *srcFunc, Function *dstFunc);
	Constant* getSourceFile(Instruction* I);
	bool isBackEdge(llvm::BasicBlock *From, llvm::BasicBlock *To);
};

} // anonymous namespace

static BasicBlock *findCommonDominator(BasicBlock *BB, DominatorTree *DT) {
	pred_iterator i = pred_begin(BB), e = pred_end(BB);
	BasicBlock *Dom = *i;
	for (++i; i != e; ++i)
		Dom = DT->findNearestCommonDominator(Dom, *i);
	return Dom;
}

bool xCallGraphPass::isBackEdge(llvm::BasicBlock *From, llvm::BasicBlock *To) {
	return std::find(BackEdges.begin(), BackEdges.end(), Edge(From, To))
		!= BackEdges.end();
}


//Constant* xCallGraphPass::getSourceFile(Instruction* I){

	//StringRef File;
	//if (MDNode *N = I->getMetadata("dbg")) {
		//DILocation Loc(N);
		//File = Loc.getFilename();
	//} else {
		//File = "Unknown Source File";
	//}

	//if (SourceFiles.count(File)) {
		//return SourceFiles.find(File)->second;
	//} else {

		////Create a global variable with the File string
		//Constant* stringConstant = llvm::ConstantArray::get(*context, File);
		//GlobalVariable* sourceFileStr = new GlobalVariable(*module, stringConstant->getType(), true,
														//llvm::GlobalValue::InternalLinkage,
														//stringConstant, "SourceFile");

		//constZero = ConstantInt::get(Type::getInt32Ty(*context), 0);

		////Get the int8ptr to our message
		//Constant* constArray = ConstantExpr::getInBoundsGetElementPtr(sourceFileStr, constZero);
		//Constant* sourceFilePtr = ConstantExpr::getBitCast(constArray, PointerType::getUnqual(Type::getInt8Ty(*context)));

		//SourceFiles[File] = sourceFilePtr;

		//return sourceFilePtr;
	//}
//}


// print the called function
void xCallGraphPass::printCalledFuncPath(Function *srcFunc, Function *dstFunc){
	CalledFunctions::iterator ii, ee;
	ii = calledFunctionMap[srcFunc].begin();
	ee = calledFunctionMap[dstFunc].end();
	for( ; ii != ee; ++ii){
		Function *FTmp = ii->first;
		Instruction *ITmp = ii->second;

		llvm::errs() << "\t" << FTmp->getName() << "-->" ;
		llvm::errs() << "\n" << *ITmp <<"\n";
		if( FTmp == dstFunc ){
			llvm::errs() << "end. \na path found\n\n";
			break;
		}
		else
		{
			printCalledFuncPath(FTmp, dstFunc);
		}
	}
	

}

void xCallGraphPass::printCalledFuncAndCFGPath(Function *srcFunc, Function *dstFunc){
	CalledFunctions::iterator ii, ee;
	ii = calledFunctionMap[srcFunc].begin();
	ee = calledFunctionMap[dstFunc].end();
	for( ; ii != ee; ++ii){
		Function *FTmp = ii->first;
		Instruction *ITmp = ii->second;

		llvm::errs() << "\t" << FTmp->getName() << "-->" ;
		llvm::errs() << "\n" << *ITmp <<"\n";

		BasicBlock *curBB = ITmp->getParent();
		pred_iterator i, e =  pred_end(curBB);
		
		DT = &getAnalysis<DominatorTree>(*FTmp);
		BackEdges.clear();
		if( FTmp ){
			FindFunctionBackedges(*FTmp, BackEdges);
			//Backedges = &BackEdges;
		}

		llvm::errs() << "\t\tCFG in :" << FTmp->getName() << "\n\t\t" << curBB->getName() << "->";
		for (i = pred_begin(curBB); i != e; )
		{

			BasicBlock *bb = *i;
			//bb->getName();
			llvm::errs() << "\t" << bb->getName() << "->";
			i = pred_begin(bb);
			if(i!=e && isBackEdge(*i,bb))
			{
				bb = findCommonDominator(bb, DT);
				llvm::errs() << "\tbk:" << bb->getName() << "->";
				i = pred_begin(bb);
				//break;
			}
		}
		llvm::errs() << "\n\t\tCFG end\n" ;
		if( FTmp == dstFunc ){
			llvm::errs() << "end. \na path found\n\n";
			break;
		}
		else
		{
			printCalledFuncAndCFGPath(FTmp, dstFunc);
		}
	}
	
}

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

	Function *startFunc;
	Function *endFunc;
	startFunc = M.getFunction("main");
	endFunc = M.getFunction("int_overflow");

//	llvm::errs() << "startFunc" << *startFunc << "endfunc:" << *endFunc << "\n" ;
	
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
		F = cgNode->getFunction();
		llvm::errs() << "Function-cgNode: " << F->getName() << "--------\n";
//		cgNode->dump();
		for (CallGraphNode::const_iterator I = cgNode->begin(), E = cgNode->end();
				I != E; ++I){
			llvm::errs() << "\tCS<" << I->first << "> calls";
			if(I->first){
				Instruction *TmpIns = dyn_cast<Instruction>(I->first);
				if(isa<Instruction>(TmpIns))
					llvm::errs() << "\t" << *TmpIns << "\n";
			}
			Function *FI = I->second->getFunction();
			if ( FI )
				llvm::errs() << "\tfunctionName '" << FI->getName() <<"'\n";
			else
				llvm::errs() << "\texternal node\n";
			
			//create the called graph;
			FunctionMapTy::iterator it = calledFunctionMap.find(FI);
			if(it==calledFunctionMap.end()) //not find
			{
				//CallGraphNode *newNode = new CallGraphNode(FI);
				//CallGraphNode *newNode = I->second;
				if(I->first){

				//	CallSite CS(dyn_cast<Instruction>(I->first));
				//	newNode->addCalledFunction(CS, newNode);
				//	calledFunctionMap[FI] = newNode;
					calledFunctionMap[FI].push_back(std::make_pair(F, dyn_cast<Instruction>(I->first)));
				}

				
			}
			else{
				//CallGraphNode *calledNode = calledFunctionMap[FI];
				if(I->first){
					//CallSite CS(dyn_cast<Instruction>(I->first));
					//calledNode->addCalledFunction(CS, calledNode);
					//calledFunctionMap[FI].push_back(std::make_pair(F, dyn_cast<Instruction>(I->first)));
					it->second.push_back(std::make_pair(F, dyn_cast<Instruction>(I->first)));
				}
			}
			
		}

	}

	llvm::errs() << "get Function Path: " << endFunc->getName() 
		<< " to " << startFunc->getName() << " \n";

	//CalledFunctions::iterator ii, ee;
	//ii = calledFunctionMap[endFunc].begin();
	//ee = calledFunctionMap[endFunc].end();
	//for( ; ii != ee; ++ii){
		//Function *FTmp = ii->first;
		//llvm::errs() << "called by:" << FTmp->getName() << "\n" ;
	//}
	printCalledFuncAndCFGPath(endFunc, startFunc);


	llvm::errs() << "on-end\n";
	return false;
}

char xCallGraphPass::ID;

static RegisterPass<xCallGraphPass>
X("xqx-cg", "traverse the call graph test");
