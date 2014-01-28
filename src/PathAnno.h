#pragma once

#include <llvm/Module.h>
#include <llvm/Type.h>
#include <llvm/Instructions.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/Metadata.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Debug.h>
#include <string>

#define MD_PATH_ENTRY	 "entry"
#define MD_PATH_END		 "end"

class PathAnnoPass : public llvm::FunctionPass {
protected:
	llvm::Module *M;
public:
	static char ID;
	PathAnnoPass() : FunctionPass(ID) { }

	virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
		AU.setPreservesCFG();
	}
	virtual bool runOnFunction(llvm::Function &);
	virtual bool doInitialization(llvm::Module &);
};


