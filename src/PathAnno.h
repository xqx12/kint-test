#pragma once

#include <llvm/Module.h>
#include <llvm/Type.h>
#include <llvm/Instructions.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/Metadata.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Debug.h>
#include <string>
#include <map>

#define MD_PATH_ENTRY	 "start"
#define MD_PATH_END		 "end"


//--------------data structure for entry and ends-----------
// < Name, <filename, lineno> >
typedef std::map<std::string, unsigned> EntryInfo;
typedef std::map<std::string, EntryInfo> Entry;
//typedef std::vector< Entry > Entrys;
typedef std::vector< std::pair<std::string, std::pair<std::string, unsigned> > > Entrys;

class PathAnnoPass : public llvm::FunctionPass {
protected:
	llvm::Module *M;

//	Entrys StartEntrys; 
public:
	static char ID;
	PathAnnoPass() : FunctionPass(ID) { }

	virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
		AU.setPreservesCFG();
	}
	virtual bool runOnFunction(llvm::Function &);
	virtual bool doInitialization(llvm::Module &);

	bool findLineInFunction(llvm::Function *F, llvm::BasicBlock **BB, std::string srcFile, unsigned srcLine);
	bool findLineInBB(llvm::BasicBlock *BB, std::string srcFile, unsigned srcLine);
	int AnnoStartEntrys(llvm::Function *F, Entrys *E, bool bEntry=true);
	int	AnnoEndEntrys(llvm::Function *F, Entrys *E );
};


