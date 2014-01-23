#pragma once

#include <llvm/DebugInfo.h>
#include <llvm/Module.h>
#include <llvm/Instructions.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/ConstantRange.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "CRange.h"

typedef std::vector< std::pair<llvm::Module *, llvm::StringRef> > ModuleList;
typedef llvm::SmallPtrSet<llvm::Function *, 8> FuncSet;
typedef std::map<llvm::StringRef, llvm::Function *> FuncMap;
typedef std::map<std::string, FuncSet> FuncPtrMap;
typedef llvm::DenseMap<llvm::CallInst *, FuncSet> CalleeMap;
typedef std::set<llvm::StringRef> DescSet;
typedef std::map<std::string, CRange> RangeMap;

//addbyxqx201401 for taint_analysis
typedef std::set<llvm::Value *> ValueSet;
typedef llvm::ArrayRef<ValueSet> ArraySet;


using namespace llvm;

class TaintMap {

public:
	typedef std::map<std::string, std::pair<DescSet, bool> > GlobalMap;
	typedef std::map<llvm::Value *, DescSet> ValueMap;
	//addbyxqx201401
	typedef std::map<llvm::Value *, ValueSet> TaintParents;
	typedef std::map<std::string, ValueSet> GlobalTaintParents;
	
	GlobalMap GTS;
	ValueMap VTS;
	//addbyxqx201401
	TaintParents VTP;
	GlobalTaintParents GTP;

	void add(llvm::Value *V, const DescSet &D) {
		raw_ostream &OS = dbgs();
		OS << "\t\t\t^^^^^^add(V,DescSet)^^^^^^^\n" ;
		V->dump();
		OS << "\t\t\t^^^^^^add(V,DescSet)^^^^^^^end\n" ;
		VTS[V].insert(D.begin(), D.end());
	}
	void add(llvm::Value *V, llvm::StringRef D) {
		raw_ostream &OS = dbgs();
		OS << "\t\t\t^^^^^^add(V,DescSet)^^^^^^^\n" ;
		V->dump();
		OS << "\t\t\t^^^^^^add(V,str)^^^^^^^end\n" ;
		VTS[V].insert(D);
	}
	DescSet* get(llvm::Value *V) {
		ValueMap::iterator it = VTS.find(V);
		if (it != VTS.end())
			return &it->second;
		return NULL;
	}

	DescSet* get(const std::string &ID) {
		if (ID.empty())
			return NULL;
		GlobalMap::iterator it = GTS.find(ID);
		if (it != GTS.end())
			return &it->second.first;
		return NULL;
	}
	bool add(const std::string &ID, const DescSet &D, bool isSource = false) {
		if (ID.empty())
			return false;
		raw_ostream &OS = dbgs();
		OS << "\t\t\t^^^^^^add(string,DescSet)^^^^^^^\n" ;
		OS << ID << "\n";
		OS << "\t\t\t^^^^^^add(string,DescSet)^^^^^^^end\n" ;
		std::pair<DescSet, bool> &entry = GTS[ID];
		bool isNew = entry.first.empty();
		entry.first.insert(D.begin(), D.end());
		entry.second |= isSource;
		return isNew;
	}
	bool isSource(const std::string &ID) {
		if (ID.empty())
			return false;
		GlobalMap::iterator it = GTS.find(ID);
		if (it == GTS.end())
			return false;
		return it->second.second;
	}
	//addbyxqx201401
	//
	void xadd(llvm::Value *V, const ValueSet &pV ) {
	//	VTS[V].insert(D.begin(), D.end());
		if( V == NULL) return ;
		VTP[V].insert(pV.begin(),pV.end());
	}
	void xadd(llvm::Value *V,  llvm::Value *pV) {
		//VTS[V].insert(D);
		if( V == NULL) return ;
		VTP[V].insert(pV);
	}
	ValueSet* xget(llvm::Value *V)
	{
		if (!V) 
			return NULL;
		TaintParents::iterator it = VTP.find(V);
		if (it == VTP.end())
			return NULL;
		return &it->second;
	}

	bool xadd(const std::string &ID, const ValueSet &pV) {
		if (ID.empty())
			return false;
		GTP[ID].insert(pV.begin(), pV.end());
		return true;
	}
	bool xadd(const std::string &ID, llvm::Value *pV) {
		if (ID.empty())
			return false;
		GTP[ID].insert(pV);
		return true;
	}
	ValueSet* xget(const std::string &ID)
	{
		if (ID.empty()) 
			return NULL;
		GlobalTaintParents::iterator it = GTP.find(ID);
		if (it == GTP.end())
			return NULL;
		return &it->second;
	}
};

struct GlobalContext {
	// Map global function name to function defination
	FuncMap Funcs;

	// Map function pointers (IDs) to possible assignments
	FuncPtrMap FuncPtrs;
	
	// Map a callsite to all potential callees
	CalleeMap Callees;

	// Taints
	TaintMap Taints;

	// Ranges
	RangeMap IntRanges;
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

class CallGraphPass : public IterativeModulePass {
private:
	bool runOnFunction(llvm::Function *);
	void processInitializers(llvm::Module *, llvm::Constant *, llvm::GlobalValue *);
	bool mergeFuncSet(FuncSet &S, const std::string &Id);
	bool mergeFuncSet(FuncSet &Dst, const FuncSet &Src);
	bool findFunctions(llvm::Value *, FuncSet &);
	bool findFunctions(llvm::Value *, FuncSet &, 
	                   llvm::SmallPtrSet<llvm::Value *, 4>);


public:
	CallGraphPass(GlobalContext *Ctx_)
		: IterativeModulePass(Ctx_, "CallGraph") { }
	virtual bool doInitialization(llvm::Module *);
	virtual bool doFinalization(llvm::Module *);
	virtual bool doModulePass(llvm::Module *);

	// debug
	void dumpFuncPtrs();
	void dumpCallees();
};

class TaintPass : public IterativeModulePass {
private:
	DescSet* getTaint(llvm::Value *);
	bool runOnFunction(llvm::Function *);
	bool checkTaintSource(llvm::Value *);
	bool markTaint(const std::string &Id, bool isSource);

	bool checkTaintSource(llvm::Instruction *I);
	bool checkTaintSource(llvm::Function *F);

	typedef llvm::DenseMap<llvm::Value *, DescSet> ValueTaintSet;
	ValueTaintSet VTS;

public:
	TaintPass(GlobalContext *Ctx_)
		: IterativeModulePass(Ctx_, "Taint") { }
	virtual bool doModulePass(llvm::Module *);
	virtual bool doFinalization(llvm::Module *);
	bool isTaintSource(const std::string &sID);

	// debug
	void dumpTaints();
	void dumpDesc( DescSet *D) ;
	void dumpValueMap(); 
	void backtrace(llvm::Instruction *I); 
	void dumpTSinkPath();
	void dumpValueByPath(ValueSet VS, int depth);
	void getSrcbyInst(Instruction *I); 
};


class RangePass : public IterativeModulePass {

private:
	const unsigned MaxIterations;	
	
	bool safeUnion(CRange &CR, const CRange &R);
	bool unionRange(llvm::StringRef, const CRange &, llvm::Value *);
	bool unionRange(llvm::BasicBlock *, llvm::Value *, const CRange &);
	CRange getRange(llvm::BasicBlock *, llvm::Value *);

	void collectInitializers(llvm::GlobalVariable *, llvm::Constant *);
	bool updateRangeFor(llvm::Function *);
	bool updateRangeFor(llvm::BasicBlock *);
	bool updateRangeFor(llvm::Instruction *);

	typedef std::map<llvm::Value *, CRange> ValueRangeMap;
	typedef std::map<llvm::BasicBlock *, ValueRangeMap> FuncValueRangeMaps;
	FuncValueRangeMaps FuncVRMs;

	typedef std::set<std::string> ChangeSet;
	ChangeSet Changes;
	
	typedef std::pair<const llvm::BasicBlock *, const llvm::BasicBlock *> Edge;
	typedef llvm::SmallVector<Edge, 16> EdgeList;
	EdgeList BackEdges;
	
	bool isBackEdge(const Edge &);
	
	CRange visitBinaryOp(llvm::BinaryOperator *);
	CRange visitCastInst(llvm::CastInst *);
	CRange visitSelectInst(llvm::SelectInst *);
	CRange visitPHINode(llvm::PHINode *);
	
	bool visitCallInst(llvm::CallInst *);
	bool visitReturnInst(llvm::ReturnInst *);
	bool visitStoreInst(llvm::StoreInst *);

	void visitBranchInst(llvm::BranchInst *, 
						 llvm::BasicBlock *, ValueRangeMap &);
	void visitTerminator(llvm::TerminatorInst *,
						 llvm::BasicBlock *, ValueRangeMap &);
	void visitSwitchInst(llvm::SwitchInst *, 
						 llvm::BasicBlock *, ValueRangeMap &);

public:
	RangePass(GlobalContext *Ctx_)
		: IterativeModulePass(Ctx_, "Range"), MaxIterations(5) { }
	
	virtual bool doInitialization(llvm::Module *);
	virtual bool doModulePass(llvm::Module *M);
	virtual bool doFinalization(llvm::Module *);

	// debug
	void dumpRange();
};


