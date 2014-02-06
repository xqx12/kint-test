/*
 * =====================================================================================
 *
 *       Filename:  PathGet.cc
 *
 *    Description:  Get Path in Call Graph, with BGL for get the shortest path. 
 *
 *        Version:  1.0
 *        Created:  2014年02月05日 09时06分38秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Xiao Qixue (XQX), xiaoqixue_1@163.com
 *   Organization:  Tsinghua University
 *
 * =====================================================================================
 */



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

#include <iostream>
#include <fstream>
#include <stdio.h>
#include "PathGet.h"

using namespace llvm;

// This will add -lxml2 dependancy when linking, and will break LLVM builds
// use CXXFLAGS="-I /usr/include/libxml2" and LDFLAGS "-l xml2"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>




bool PathGetPass::runOnModule(Module &_M) {
	//M = &_M;
	//CallGraph *CG = &getAnalysis<CallGraph>();
	//CallGraphNode *root = CG->getRoot();
	//root->dump();

	std::cout << "=====";
	return true;
}




char PathGetPass::ID = 0;
static RegisterPass<PathGetPass>
X("getpaths", "get paths from CallGraph", false, true);//X(passArg, Name, CFGonly=false, isAnalysis=false)





