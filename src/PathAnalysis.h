/*
* Filename: PathAnalysis.h
* Last modified: 2014-01-29 08:40
* Author: Qixue Xiao <xiaoqixue_1@163.com>
* Description: 
*/

#include <llvm/DebugInfo.h>
#include <llvm/Module.h>
#include <llvm/Instructions.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/ConstantRange.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Analysis/CallGraph.h>
#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace llvm;

//--------------data structure for entry and ends-----------
// < Name, <filename, lineno> >
typedef std::map<std::string, unsigned> EntryInfo;
typedef std::map<std::string, EntryInfo> Entry;
typedef std::vector< Entry > Entrys;


typedef std::vector< std::pair<llvm::Module *, llvm::StringRef> > ModuleList;
typedef llvm::SmallPtrSet<llvm::Function *, 8> FuncSet;
typedef std::map<llvm::StringRef, llvm::Function *> FuncMap;
typedef std::map<std::string, FuncSet> FuncPtrMap;
typedef llvm::DenseMap<llvm::CallInst *, FuncSet> CalleeMap;
typedef std::set<llvm::StringRef> DescSet;
//typedef std::map<std::string, CRange> RangeMap;

//addbyxqx201401 for taint_analysis
typedef std::set<llvm::Value *> ValueSet;
typedef llvm::ArrayRef<ValueSet> ArraySet;



struct GlobalContext {
	// Map global function name to function defination
	FuncMap Funcs;

	// Map function pointers (IDs) to possible assignments
	FuncPtrMap FuncPtrs;
	
	// Map a callsite to all potential callees
	CalleeMap Callees;

	// Taints
	//TaintMap Taints;

	// Ranges
//	RangeMap IntRanges;
};

class IterativeModulePass {
protected:
	GlobalContext *Ctx;
	const char * ID;
public:
	IterativeModulePass(GlobalContext *Ctx_, const char *ID_)
		: Ctx(Ctx_), ID(ID_) { }
	
	// run on each module before iterative pass
	virtual bool doInitialization(llvm::Module *M)
		{ return true; }

	// run on each module after iterative pass
	virtual bool doFinalization(llvm::Module *M)
		{ return true; }

	// iterative pass
	virtual bool doModulePass(llvm::Module *M)
		{ return false; }

	virtual void run(ModuleList &modules);
};




