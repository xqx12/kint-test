#include <llvm/Module.h>
#include <llvm/Instructions.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/Metadata.h>
#include <llvm/Constants.h>
#include <llvm/Pass.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/Debug.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/DebugInfo.h>

#include "PathAnno.h"

using namespace llvm;



static bool annotateSink(CallInst *CI) {
	#define P std::make_pair
	static std::pair<const char *, int> Allocs[] = {
		P("dma_alloc_from_coherent", 1),
		P("__kmalloc", 0),
		P("kmalloc", 0),
		P("__kmalloc_node", 0),
		P("kmalloc_node", 0),
		P("kzalloc", 0),
		P("kcalloc", 0),
		P("kcalloc", 1),
		P("kmemdup", 1),
		P("memdup_user", 1),
		P("pci_alloc_consistent", 1),
		P("__vmalloc", 0),
		P("vmalloc", 0),
		P("vmalloc_user", 0),
		P("vmalloc_node", 0),
		P("vzalloc", 0),
		P("vzalloc_node", 0),
	};
	#undef P

	LLVMContext &VMCtx = CI->getContext();
	StringRef Name = CI->getCalledFunction()->getName();
	
	//addbyxqx201401
	raw_ostream &OS = dbgs();
	OS << "annotateSink: \n";
	CI->dump();
	OS << "GetCalledFunc:" << Name << "\n";

	for (unsigned i = 0; i < sizeof(Allocs) / sizeof(Allocs[0]); ++i) {
		if (Name == Allocs[i].first) {
			Value *V = CI->getArgOperand(Allocs[i].second);
			if (Instruction *I = dyn_cast_or_null<Instruction>(V)) {
				MDNode *MD = MDNode::get(VMCtx, MDString::get(VMCtx, Name));
				I->setMetadata(MD_PATH_END, MD);
				OS << "SetMetadata:\n" ;
				MD->dump();
				return true;
			}
		}
	}
	return false;
}


bool PathAnnoPass::runOnFunction(Function &F) {
	bool Changed = false;

	//Changed |= annotateArguments(F);

	//addbyxqx201312
	raw_ostream &OS = dbgs();
	OS << "Func:" << "\n";
	F.dump();
	
	SmallPtrSet<Instruction *, 4> EraseSet;
	for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
		Instruction *I = &*i;
		I->dump();
		if (CallInst *CI = dyn_cast<CallInst>(I)) {
			Changed |= annotateSink(CI);
		}

		//if (isa<LoadInst>(I) || isa<StoreInst>(I)) {
			//Changed |= annotateLoadStore(I);
		//} else if (CallInst *CI = dyn_cast<CallInst>(I)) {
			//if (!CI->getCalledFunction())
				//continue;
			//Changed |= annotateTaintSource(CI, EraseSet);
			//Changed |= annotateSink(CI);
		//}
	}
	for (SmallPtrSet<Instruction *, 4>::iterator i = EraseSet.begin(),
			e = EraseSet.end(); i != e; ++i) {
		(*i)->eraseFromParent();
	}
	return Changed;
}

bool PathAnnoPass::doInitialization(Module &M)
{
	this->M = &M;
	return true;
}


char PathAnnoPass::ID;

static RegisterPass<PathAnnoPass>
X("path-anno", "add path entry and end");


