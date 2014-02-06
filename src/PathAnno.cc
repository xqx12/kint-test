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
#include "llvm/Analysis/CallGraph.h"

#include "PathAnno.h"
#include <iostream>
#include <stdio.h>

using namespace llvm;


static void getPath(SmallVectorImpl<char> &Path, const MDNode *MD) {
	if(MD==NULL)
		return;
	StringRef Filename = DIScope(MD).getFilename();
	if (sys::path::is_absolute(Filename))
		Path.append(Filename.begin(), Filename.end());
	else
		sys::path::append(Path, DIScope(MD).getDirectory(), Filename);
}

// get the Instruction in c code location
// addbyxqx201401
static std::string getInstPath(Instruction *I, unsigned &LineNo, unsigned &ColNo)
{
	MDNode *MD = I->getDebugLoc().getAsMDNode(I->getContext());
	if (!MD)
		return "";
	DILocation Loc(MD);
	SmallString<64> Path;
	getPath(Path, Loc.getScope());

	LineNo = Loc.getLineNumber();
	ColNo = Loc.getColumnNumber();
	std::string strPath = Path.str();
	return strPath;
	
}
#if 0
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
#endif

bool PathAnnoPass::findLineInBB(BasicBlock *BB, std::string srcFile, unsigned srcLine)
{
	for (BasicBlock::iterator it = BB->begin(), ie = BB->end(); it != ie; ++it) {
		if (Instruction *I = dyn_cast<Instruction>(&*it)) {
			unsigned bbLine, bbCol;
			std::string bbFile = getInstPath(I, bbLine, bbCol);
			//std::cerr << " :checking " << bbFile << " : " << bbLine << "\n";
			if ((bbFile == srcFile) && (bbLine == srcLine))
				return true;
		}
	}		
	return false;
}

bool PathAnnoPass::findLineInFunction(Function *F, BasicBlock **BB, std::string srcFile, unsigned srcLine)
{
	for (Function::iterator bbIt = F->begin(), bb_ie = F->end(); bbIt != bb_ie; ++bbIt) {
		*BB = bbIt;
		if (findLineInBB(*BB,srcFile,srcLine))
			return true;
	}
	return false;
}

int PathAnnoPass::AnnoStartEntrys(Function *F, Entrys *E, bool bEntry)
{
	int ret = 0;
	bool bFound;
	BasicBlock *bb = NULL;
	std::string Anno;
	raw_ostream &OS = dbgs();

	for( Entrys::iterator i = E->begin(), e = E->end(); i!=e; ++i)
	{
		std::cerr << "Finding in " <<  (i->second).first<< " : " << (i->second).second;
		OS << "in func:" << F->getName() <<"\n";
		bFound = findLineInFunction(F, &bb, (i->second).first, (i->second).second);  
		if(bFound) 
		{
			std::cerr << "Found in " <<  (i->second).first<< " : " << (i->second).second << "\n";
			//Annotation in the bb's first Instruction
			Instruction *I = &bb->front();
			if(I == NULL)
				continue;
			LLVMContext &VMCtx = I->getContext();
			StringRef Desc = (i->second).first + ":" ;
			MDNode *MD = MDNode::get(VMCtx, MDString::get(VMCtx, Desc));
			I->setMetadata(bEntry ? MD_PATH_ENTRY:MD_PATH_END,  MD);

			ret++;
		}
	}
	//if( findLineInFunction(F) )
	//{
			////do annotation

	//}
	//else{
	
	//}
	return ret;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  AnnoEndEntrys
 *  Description:  
 * =====================================================================================
 */
int	PathAnnoPass::AnnoEndEntrys(Function *F, Entrys *E )
{
	int ret = 0;

	return ret;
}		/* -----  end of function AnnoEndEntrys  ----- */




/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  runOnFunction
 *  Description:  checking all the BB in a Function.
 * =====================================================================================
 */
bool PathAnnoPass::runOnFunction(Function &F) {
	bool Changed = false;

	//CallGraph *CG = &getAnalysis<CallGraph>();
	//CallGraphNode *root = CG->getRoot();
	//if (root == NULL) return true;
	//Changed |= annotateArguments(F);

	//addbyxqx201312
	raw_ostream &OS = dbgs();
	OS << "Func:" << "\n";
	F.dump();
	
	SmallPtrSet<Instruction *, 4> EraseSet;
	for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
		Instruction *I = &*i;
		I->dump();

		std::string InstPath;
		unsigned LineNo,ColNo;
		char tmpBuf[256] = {0};
		InstPath = getInstPath(I, LineNo, ColNo);

		if(InstPath.empty()) continue;
		//TODO: how to convert int to string in llvm ??
		sprintf(tmpBuf,"\t%s::%d:%d\n",InstPath.c_str(),LineNo,ColNo); 
		OS << tmpBuf << "\n";
		//OS << "\t" << InstPath << "::" << LineNo << ":" << ColNo << "\n" ;
		//std::strstream sstr;
		//sstr << LineNo ;

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


