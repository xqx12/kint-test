#include <llvm/DebugInfo.h>
#include <llvm/Module.h>
#include <llvm/Pass.h>
#include <llvm/Constants.h>
#include <llvm/Instructions.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Analysis/CallGraph.h>

#include "Annotation.h"
#include "IntGlobal.h"

using namespace llvm;

#define TM (Ctx->Taints)

static inline StringRef asString(MDNode *MD) {
	if (MDString *S = dyn_cast_or_null<MDString>(MD->getOperand(0)))
		return S->getString();
	return "";
}

static inline MDString *toMDString(LLVMContext &VMCtx, DescSet *D) {
	std::string s;
	for (DescSet::iterator i = D->begin(), e = D->end(); i != e; ++i) {
		if (i != D->begin())
			s += ", ";
		s += (*i).str();
	}
	return MDString::get(VMCtx, s);
}

// Check both local taint and global sources
DescSet * TaintPass::getTaint(Value *V) {
	
	dumpValueMap();
	raw_ostream &OS = dbgs();
	OS << "getTaint: " ;
	V->dump();
	if (DescSet *DS = TM.get(V)){
		OS << "\tTM get V OK: \n" ;
		return DS;
	}

	OS << "stripPointerCasts: " ;
	V->stripPointerCasts()->dump();

	if (DescSet *DS = TM.get(V->stripPointerCasts())){
		OS << "\tTM get V->stripPointerCasts() OK: \n" ;
		return DS;
	}
	if ( V != V->stripPointerCasts()){
		V->dump();
		V->stripPointerCasts()->dump();
		assert(0&&"find stripPointerCasts diff");
	}
	
	// if value is not taint, check global taint.
	// For call, taint if any possible callee could return taint
	if (CallInst *CI = dyn_cast<CallInst>(V)) {
		if (!CI->isInlineAsm() && Ctx->Callees.count(CI)) {
			FuncSet &CEEs = Ctx->Callees[CI];
			for (FuncSet::iterator i = CEEs.begin(), e = CEEs.end();
				 i != e; ++i) {
				if (DescSet *DS = TM.get(getRetId(*i))){
					OS << "\tTM get getRetId OK:" << getRetId(*i) << " \n" ;
					TM.add(CI, *DS);
					TM.xadd(V, *( TM.xget(getRetId(*i))));
				}
			}
		}
	}
	// For arguments and loads
	if (DescSet *DS = TM.get(getValueId(V))){
		OS << "\tTM get getValueId OK:" << getValueId(V) << " \n" ;
		TM.add(V, *DS);
		TM.xadd(V, *(TM.xget(getValueId(V)))); 
	}
	return TM.get(V);
}

bool TaintPass::isTaintSource(const std::string &sID) {
	return TM.isSource(sID);
}

// find and mark taint source
bool TaintPass::checkTaintSource(Instruction *I)
{
	Module *M = I->getParent()->getParent()->getParent();
	bool changed = false;
	//addbyxqx201401
	raw_ostream &OS = dbgs();

	if (MDNode *MD = I->getMetadata(MD_TaintSrc)) {
		TM.add(I, asString(MD));
		TM.xadd(I,  NULL);
		DescSet &D = *TM.get(I);
		//dumpDesc(&D);
		changed |= TM.add(getValueId(I), D, true);
		TM.xadd(getValueId(I), NULL);
		//addbyxqx201401
		OS << "checkTaintSource: \n" << "getVualeID(I)=" << getValueId(I) << "\n";
		OS << "asSting(MD): " << asString(MD) << "\n";
		OS << "Inst: \n";
		I->dump();
		OS << "MD: \n";
		MD->dump();
		//D.dump();
		// mark all struct members as taint
		if (PointerType *PTy = dyn_cast<PointerType>(I->getType())) {
			if (StructType *STy = dyn_cast<StructType>(PTy->getElementType())) {
				for (unsigned i = 0; i < STy->getNumElements(); ++i){
					changed |= TM.add(getStructId(STy, M, i), D, true);
					OS << "getStructId= " << getStructId(STy,M,i) << "\n";
				}
			}
		}
	}
	return changed;
}

// Propagate taint within a function
bool TaintPass::runOnFunction(Function *F)
{
	bool changed = false;

	raw_ostream &OS = dbgs();
	OS << "TP::runOnFunc:" << F->getName() << "\n";
	OS << "----------Function Bady-----------------\n";
	F->dump();
	OS << "----------Function end-----------------\n\n\n";

	for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
		Instruction *I = &*i;
		
		// find and mark taint sources
		OS << "\t====instuction======\n";
		I->dump();
		//backtrace(I);
		OS << "\n" ;
		changed |= checkTaintSource(I);

		// for call instruction, propagate taint to arguments instead
		// of from arguments
		if (CallInst *CI = dyn_cast<CallInst>(I)) {
			//if (CI->isInlineAsm() || !Ctx->Callees.count(CI))
				//continue;
			if (CI->isInlineAsm() ){
				OS << "\tthis is InlineAsm call\n" ;
				continue;
			}
			OS << "\tthis callee count is:" << Ctx->Callees.count(CI)<<"\n" ;
			if ( !Ctx->Callees.count(CI)){
				OS << "\tthis callee count is:" << Ctx->Callees.count(CI)<<"\n" ;
				continue;
			}

			FuncSet &CEEs = Ctx->Callees[CI];
			for (FuncSet::iterator j = CEEs.begin(), je = CEEs.end();
				 j != je; ++j) {

				OS << "\tcallees:" << (*j)->getName() << "\n";
				// skip vaarg and builtin functions
				if ((*j)->isVarArg() 
					|| (*j)->getName().find('.') != StringRef::npos)
					continue;
				
				// mark corresponding args tainted on all possible callees
				for (unsigned a = 0; a < CI->getNumArgOperands(); ++a) {
					if (DescSet *DS = getTaint(CI->getArgOperand(a))){
						OS << "CI operands is tainted, num = " << a << "\n";
						CI->getArgOperand(a)->dump();
						OS << "getArgId(*j,a) = " << getArgId(*j,a) << "\n";
						changed |= TM.add(getArgId(*j, a), *DS);
						TM.xadd(getArgId(*j, a), CI->getArgOperand(a));
					}
				}
			}
			continue;
		}

		// check if any operand is taint
		DescSet D;
		ValueSet pV;
		for (unsigned j = 0; j < I->getNumOperands(); ++j)
			if (DescSet *DS = getTaint(I->getOperand(j))){
				OS << "I->getOperand(" << j << "):\n" ;
				I->getOperand(j)->dump();
				D.insert(DS->begin(), DS->end());
				//addbyxqx201401
				pV.insert(I->getOperand(j));
			}
		if (D.empty())
			continue;

		// propagate value and global taint
		TM.add(I, D);
		TM.xadd(I, pV);
		if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
			if (MDNode *ID = SI->getMetadata(MD_ID)){
				changed |= TM.add(asString(ID), D);
				TM.xadd(asString(ID), SI);
			}
		} else if (isa<ReturnInst>(I)) {
			changed |= TM.add(getRetId(F), D);
			TM.xadd(getRetId(F), I);
			OS << "returnInst:" << getRetId(F) << "\n";
		}

		OS << "\t====instuction======end\n\n";
	}
	OS << "\t++++++++TM status:+++++++++ \n" ;
	dumpTaints();
	OS << "\t++++++++TM status:+++++++++end \n\n" ;
	OS << "TP::runOnFunc:" << F->getName() << "--------------\n";
	return changed;
}

// write back
bool TaintPass::doFinalization(Module *M) {
	LLVMContext &VMCtx = M->getContext();
	raw_ostream &OS = dbgs();

	for (Module::iterator f = M->begin(), fe = M->end(); f != fe; ++f) {
		Function *F = &*f;
		
		OS << "taintpass: ----\n";
		OS << *F  << "\n";
		for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
			Instruction *I = &*i;
			if (DescSet *DS = getTaint(I)) {
				MDNode *MD = MDNode::get(VMCtx, toMDString(VMCtx, DS));
				I->setMetadata(MD_Taint, MD);
				//addbyxqx201401
				//ValueSet *VS = TM.xget(I);
				//if ( VS ) {
					////ArraySet ArrayValue(VS, VS->size());
					////std::vector<llvm::Value*> vec;
					//Value *RL[2]  ;
					//int count = 0;
					//for( ValueSet::iterator jj = VS->begin(),
							//jee = VS->end(); jj != jee; ++jj) {
						////vec.push_back(*jj);
						//RL[count++] = *jj ;
					//}
					//MDNode *MDTP = MDNode::get(VMCtx, RL);
					//I->setMetadata("TP" , MDTP);
				//}
			} else
			{
				I->setMetadata(MD_Taint, NULL);
			}
		}
		OS << "taintpass: after propagrate----\n";
		OS << *F  << "\n";
	}
	OS << "[[[[[[[[[[[dumptaint info]]]]]]]]]]]\n" ;
	dumpTSinkPath();
	return true;
}

bool TaintPass::doModulePass(Module *M) {
	bool changed = true, ret = false;
	int count = 0;
	raw_ostream &OS = dbgs();
	OS << "TaintPass::doModulePass----------------------\n";

	while (changed) {
		changed = false;
		for (Module::iterator i = M->begin(), e = M->end(); i != e; ++i){
			Function *F = &*i;
			OS  << "Function" << count++ << ":" << F->getName() << " \n";
			
			changed |= runOnFunction(&*i);
			if (changed)
				OS << "changed = true\n" ;
			else 
				OS << "changed = false\n" ;
			
		}
		ret |= changed;
	}

	OS << "TaintPass::doModulePass++++++++++++++++++++end-\n";
	return ret;
}

// debug
void TaintPass::dumpTaints() {
	raw_ostream &OS = dbgs();
	for (TaintMap::GlobalMap::iterator i = TM.GTS.begin(), 
		 e = TM.GTS.end(); i != e; ++i) {
		OS << (i->second.second ? "\t+S " : "\t+  ") << i->first << "\t";
		for (DescSet::iterator j = i->second.first.begin(),
			je = i->second.first.end(); j != je; ++j)
				OS << *j << " ";
		OS << "\n";
	}
}

void TaintPass::dumpValueMap() {
	raw_ostream &OS = dbgs();
	OS << "\t\t-------dumpValueMap\n"; 
	for (TaintMap::ValueMap::iterator i = TM.VTS.begin(), 
		 e = TM.VTS.end(); i != e; ++i) {
		OS << *(i->first) << ":#: " ;
		for (DescSet::iterator j = i->second.begin(),
			je = i->second.end(); j != je; ++j)
				OS << *j << " ";
		OS << "\n";
	}
	OS << "\t\t-------------------------\n"; 
	for (TaintMap::TaintParents::iterator ii = TM.VTP.begin(),
			ee = TM.VTP.end(); ii != ee; ++ii) {
		OS << *(ii->first) << ":$: \n" ;
		for( ValueSet::iterator jj = ii->second.begin(),
				jee = ii->second.end(); jj != jee; ++jj) {
			OS << "\t\t" << *(*jj) << "\n" ;
		}
		OS << "\n";
	}

	OS << "\t\t-------dumpValueMap-------end\n\n"; 
}

void TaintPass::dumpTSinkPath()
{
	raw_ostream &OS = dbgs();

	for (TaintMap::TaintParents::iterator i = TM.VTP.begin(),
			e = TM.VTP.end(); i != e; ++i) {
		Instruction *I = dyn_cast<Instruction>(i->first);
		if (!I)
			return;

		if ( I->getMetadata("sink")){
			int d = 0;
			OS << "sink:" << *I << ":::";
			getSrcbyInst(I) ;
			OS << "\n" ;
			dumpValueByPath(i->second, d);

		}
	}

}
void TaintPass::dumpValueByPath(ValueSet VS, int depth)
{
	if (VS.empty()) 
		return;

	raw_ostream &OS = dbgs();
	for (ValueSet::iterator i = VS.begin(),e = VS.end();
			i != e; i++) {
		for( int j=0; j<depth; j++){
			OS << "\t" ;
		}
		if (*i == NULL) {
			OS << "tanit src\n" ;
			break;
		}
		Instruction *I = dyn_cast<Instruction>(*i);
		if (!I) break;
		OS << *(*i) << ":::" ;
		getSrcbyInst(I) ;
		OS << "\n";
//		}
//		else{
//			OS << "taint src\n" ;
//			break;
//		}

		ValueSet VS_TMP;
		VS_TMP = TM.VTP[*i];
		dumpValueByPath(VS_TMP, ++depth);
	}


}

void TaintPass::dumpDesc( DescSet *D) {
	raw_ostream &OS = dbgs();
	std::string s;
	for (DescSet::iterator i = D->begin(), e = D->end(); i != e; ++i) {
		if (i != D->begin())
			s += ", ";
		s += (*i).str();
	}
	OS << "DumpDesc:\n" << s << "\n";
}

static void getPath(SmallVectorImpl<char> &Path, const MDNode *MD) {
	StringRef Filename = DIScope(MD).getFilename();
	if (sys::path::is_absolute(Filename))
		Path.append(Filename.begin(), Filename.end());
	else
		sys::path::append(Path, DIScope(MD).getDirectory(), Filename);
}

void TaintPass::backtrace(Instruction *I) {
	raw_ostream &OS = dbgs();
	const char *Prefix = " - ";
	MDNode *MD = I->getDebugLoc().getAsMDNode(I->getContext());
	OS << "--backtrace:\n";
	MD->dump();
	OS << "--backtrace----end\n";
	if (!MD)
		return;
	OS << "stack: \n";
	DILocation Loc(MD);
	for (;;) {
		SmallString<64> Path;
		getPath(Path, Loc.getScope());
		OS << Prefix << Path
		   << ':' << Loc.getLineNumber()
		   << ':' << Loc.getColumnNumber() << '\n';
		Loc = Loc.getOrigLocation();
		if (!Loc.Verify())
			break;
	}
}

void TaintPass::getSrcbyInst(Instruction *I) {
	raw_ostream &OS = dbgs();
	const char *Prefix = " - ";
	MDNode *MD = I->getDebugLoc().getAsMDNode(I->getContext());
	if (!MD)
		return;
	DILocation Loc(MD);

	SmallString<64> Path;
	getPath(Path, Loc.getScope());
	OS << Prefix << Path
		<< ':' << Loc.getLineNumber()
		<< ':' << Loc.getColumnNumber() << '\n';
}
#undef TM
