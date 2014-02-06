/*
 * =====================================================================================
 *
 *       Filename:  PathGet.h
 *
 *    Description:  PathGet  
 *
 *        Version:  1.0
 *        Created:  2014年02月05日 09时54分57秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Xiao Qixue (XQX), xiaoqixue_1@163.com
 *   Organization:  Tsinghua University
 *
 * =====================================================================================
 */

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


// Uses Boost Graph Library
#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

using namespace llvm;



class PathGetPass : public llvm::ModulePass {
protected:
	llvm::Module *M;

//	Entrys StartEntrys; 
public:
	static char ID;
	PathGetPass() : ModulePass(ID) { }

	virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
		AU.setPreservesCFG();
	}
	//virtual bool runOnFunction(llvm::Function &);
	virtual bool runOnModule(Module &_M);
	//virtual bool doInitialization(llvm::Module &);

	//bool findLineInFunction(llvm::Function *F, llvm::BasicBlock **BB, std::string srcFile, unsigned srcLine);
	//bool findLineInBB(llvm::BasicBlock *BB, std::string srcFile, unsigned srcLine);
};


