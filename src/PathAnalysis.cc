/*
* Filename: PathAnalysis.cc
* Last modified: 2014-01-28 20:00
* Author: Qixue Xiao <xiaoqixue_1@163.com>
* Description: 
*/

#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Module.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/IRReader.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/Path.h"

#include "PathAnalysis.h"
//#include "IntGlobal.h"
//#include "Annotation.h"

using namespace llvm;

static cl::list<std::string>
InputFilenames(cl::Positional, cl::OneOrMore,
               cl::desc("<input bitcode files>"));

static cl::opt<bool>
Verbose("v", cl::desc("Print information about actions taken"));

static cl::opt<bool>
NoWriteback("p", cl::desc("Do not writeback annotated bytecode"));

ModuleList Modules;
GlobalContext GlobalCtx;

#define Diag if (Verbose) llvm::errs()


#if 0
void IterativeModulePass::run(ModuleList &modules) {

	ModuleList::iterator i, e;
	Diag << "[" << ID << "] Initializing " << modules.size() << " modules ";
	for (i = modules.begin(), e = modules.end(); i != e; ++i) {
		doInitialization(i->first);
		Diag << ".";
	}
	Diag << "\n";

	unsigned iter = 0, changed = 1;
	while (changed) {
		++iter;
		changed = 0;
		for (i = modules.begin(), e = modules.end(); i != e; ++i) {
			Diag << "[" << ID << " / " << iter << "] ";
			Diag << "'" << i->first->getModuleIdentifier() << "'";

			bool ret = doModulePass(i->first);
			if (ret) {
				++changed;
				Diag << " [CHANGED]\n";
			} else
				Diag << "\n";
		}
		Diag << "[" << ID << "] Updated in " << changed << " modules.\n";
	}

	Diag << "\n[" << ID << "] Postprocessing ...\n";
	for (i = modules.begin(), e = modules.end(); i != e; ++i) {
		if (doFinalization(i->first) && !NoWriteback) {
			Diag << "[" << ID << "] Writeback " << i->second << "\n";
			doWriteback(i->first, i->second);
		}
	}
			
	Diag << "[" << ID << "] Done!\n";
}
#endif

int main(int argc, char **argv)
{
	llvm::errs() << "path Analysis main \n" ;
	// Print a stack trace if we signal out.
	sys::PrintStackTraceOnErrorSignal();
	PrettyStackTraceProgram X(argc, argv);

//	llvm_shutdown_obj Y;  // Call llvm_shutdown() on exit.
	cl::ParseCommandLineOptions(argc, argv, "pathanalysis\n");
	SMDiagnostic Err;	
	
	// Loading modules
	Diag << "Total " << InputFilenames.size() << " file(s)\n";
//	llvm::errs() << "Total " << InputFilenames.size() << " file(s)\n";

	for (unsigned i = 0; i < InputFilenames.size(); ++i) {
		// use separate LLVMContext to avoid type renaming
		LLVMContext *LLVMCtx = new LLVMContext();
		Module *M = ParseIRFile(InputFilenames[i], Err, *LLVMCtx);

		if (M == NULL) {
			errs() << argv[0] << ": error loading file '" 
				<< InputFilenames[i] << "'\n";
			continue;
		}

		Diag << "Loading '" << InputFilenames[i] << "'\n";

		// annotate
		//static AnnotationPass AnnoPass;
		//AnnoPass.doInitialization(*M);
		//for (Module::iterator j = M->begin(), je = M->end(); j != je; ++j)
			//AnnoPass.runOnFunction(*j);
		//if (!NoWriteback)
			//doWriteback(M, InputFilenames[i].c_str());

		Modules.push_back(std::make_pair(M, InputFilenames[i]));
	}

	return 0;
}

