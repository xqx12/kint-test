#define DEBUG_TYPE "int-sat"
#include "Diagnostic.h"
#include "PathGen.h"
#include "SMTSolver.h"
#include "ValueGen.h"
#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/Instructions.h>
#include <llvm/Function.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Pass.h>
#include <llvm/Assembly/Writer.h>
#include <llvm/ADT/OwningPtr.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

using namespace llvm;

static cl::opt<bool>
SMTModelOpt("smt-model", cl::desc("Output SMT model"));

namespace {

// Better to make this as a module pass rather than a function pass.
// Otherwise, put `M.getFunction("int.sat")' in doInitialization() and
// it will return NULL, since it's scheduled to run before -int-rewrite.
struct IntSat : ModulePass {
	static char ID;
	IntSat() : ModulePass(ID) {}

	virtual void getAnalysisUsage(AnalysisUsage &AU) const {
		AU.setPreservesAll();
	}

	virtual bool runOnModule(Module &);

private:
	Diagnostic Diag;
	Function *Trap;
	OwningPtr<DataLayout> TD;
	unsigned MD_bug;

	SmallVector<PathGen::Edge, 32> BackEdges;
	SmallPtrSet<Value *, 32> ReportedBugs;

	void runOnFunction(Function &);
	void check(CallInst *);
	void classify(Value *);
	SMTStatus query(Value *, Instruction *);
};

} // anonymous namespace

bool IntSat::runOnModule(Module &M) {
	Trap = M.getFunction("int.sat");
	if (!Trap)
		return false;
	//Diag.xqx_print("int.sat get");
	TD.reset(new DataLayout(&M));
	MD_bug = M.getContext().getMDKindID("bug");
	llvm::errs() << *Trap << "\n";
	for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i) {
		Function &F = *i;
		llvm::errs() << F << "\n";
		if (F.empty())
			continue;
		llvm::errs()  <<  "--------runOnFunction----------\n";
		runOnFunction(F);
	}
	return false;
}

void IntSat::runOnFunction(Function &F) {
	BackEdges.clear();
	FindFunctionBackedges(F, BackEdges);
	ReportedBugs.clear();
	for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
		CallInst *CI = dyn_cast<CallInst>(&*i);
		//llvm::errs()<< "funcName: " << i->getName() << "\n";
		if (CI && CI->getCalledFunction() == Trap){
			llvm::errs()  <<  "check CI:" << *CI << "\n";
			check(CI);
		}
	}
}

void IntSat::check(CallInst *I) {
	assert(I->getNumArgOperands() >= 1);
	Value *V = I->getArgOperand(0);
	llvm::errs()  <<  "Value:" << *V << "\n";
	assert(V->getType()->isIntegerTy(1));
	if (isa<ConstantInt>(V))
		return;
	if (ReportedBugs.count(V))
		return;

	const DebugLoc &DbgLoc = I->getDebugLoc();
	//llvm::errs()  <<  "DbgLoc:" << DbgLoc << "\n";
	//DbgLoc.dump();
	assert(V->getType()->isIntegerTy(1));
	if (DbgLoc.isUnknown())
		return;
	if (!I->getMetadata(MD_bug))
		return;
	llvm::errs()  <<  "Metadata:" << I->getMetadata(MD_bug) << "\n";

	int SMTRes;
	if (SMTFork() == 0)
		SMTRes = query(V, I);
	SMTJoin(&SMTRes);

	// Save to suppress furture warnings.
	if (SMTRes == SMT_SAT)
		ReportedBugs.insert(V);
}

SMTStatus IntSat::query(Value *V, Instruction *I) {
	llvm::errs()  <<  "query---\nValue:" << *V << "\n";
	llvm::errs()  <<  "Instruction:" << *I << "\n";

	SMTSolver SMT(SMTModelOpt);
	ValueGen VG(*TD, SMT);
	PathGen PG(VG, BackEdges);
	SMTExpr VExpr = VG.get(V);
	SMTExpr PExpr = PG.get(I->getParent());
	llvm::errs()  <<  "Value SMT ---\n" ;
	SMT.dump(VExpr) ;
	llvm::errs()  <<  "Path SMT ---\n" ;
	SMT.dump(PExpr) ;
//	SMTExpr Query = SMT.bvand(VG.get(V), PG.get(I->getParent()));
	SMTExpr Query = SMT.bvand(VExpr, PExpr);
	SMTModel Model = NULL;
	SMTStatus Res = SMT.query(Query, &Model);
	SMT.decref(Query);
	if (Res != SMT_SAT)
		return Res;
	// Output bug type.
	MDNode *MD = I->getMetadata(MD_bug);
	Diag.bug(cast<MDString>(MD->getOperand(0))->getString());
	// Output location.
	Diag.status(Res);
	Diag.classify(I);
	Diag.backtrace(I);
	Diag.xqx_backtrace(I);
	// Output model.
	if (SMTModelOpt && Model) {
		Diag << "model: |\n";
		raw_ostream &OS = Diag.os();
		for (ValueGen::iterator i = VG.begin(), e = VG.end(); i != e; ++i) {
			Value *KeyV = i->first;
			if (isa<Constant>(KeyV))
				continue;
			OS << "  ";
			WriteAsOperand(OS, KeyV, false, Trap->getParent());
			OS << ": ";
			APInt Val;
			SMT.eval(Model, i->second, Val);
			if (Val.getLimitedValue(0xa) == 0xa)
				OS << "0x";
			OS << Val.toString(16, false);
			OS << '\n';
		}
	}
	if (Model)
		SMT.release(Model);
	return Res;
}

char IntSat::ID;

static RegisterPass<IntSat>
X("int-sat", "Check int.sat for satisfiability", false, true);
