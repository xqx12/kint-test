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

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "PathAnalysis.h"
#include "PathAnno.h"


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

void getEntrys(std::string docname, Entrys *res)
{
	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile(docname.c_str());

	if (!doc) return;
	Diag << "xmlfile:" << docname.c_str() << "\n";

	cur = xmlDocGetRootElement(doc);

	if (!cur) {
		xmlFreeDoc(doc);
		return;
	}

	// Iterate over defects
	while (cur) {
		if (!xmlStrcmp(cur->name, (const xmlChar *) "entry")) {
			Diag << "-----find entry-----\n";
			xmlNodePtr d = cur->xmlChildrenNode;
			std::string file; 
			std::string name ;
			unsigned line = 0;
			EntryInfo eninfo;
			Entry en;

			while (d) {
				if (!xmlStrcmp(d->name, (const xmlChar *) "name")) {
					name = (char*)xmlNodeListGetString(doc, d->xmlChildrenNode, 1);
					//file = (char*)xmlNodeListGetString(doc, d->xmlChildrenNode, 1);
					Diag << "\tname: " << name.c_str() << "\n";
					//lines.clear();
				}
				else if (!xmlStrcmp(d->name, (const xmlChar *) "info")) {
					xmlNodePtr e = d->xmlChildrenNode;
					while (e) {
						if (!xmlStrcmp(e->name, (const xmlChar *) "file")) {
							file = (char*)xmlNodeListGetString(doc, e->xmlChildrenNode, 1);
							Diag << "\tfilename: " << file.c_str() << "\n";
						}
						if (!xmlStrcmp(e->name, (const xmlChar *) "line")) {
							char *val = (char*)xmlNodeListGetString(doc, e->xmlChildrenNode, 1);
							//lines.push_back(atoi(val));
							line = atoi(val);
							Diag << "\tline: " << atoi(val) << "\n" ;
						}
						e = e->next;
					}
					eninfo.insert(std::make_pair(file, line));
				}
				en.insert(std::make_pair(name, eninfo));
				d = d->next;
			}
			if (name != "" && file != "" && line != 0){
				Diag << "insert entry\n";	//res->insert(std::make_pair(file,lines));
				//res->push_back(en);				
				res->push_back(std::make_pair(name, make_pair(file,line)));
			}
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);
}

void dumpEntrys(Entrys *E)
{
	for( Entrys::iterator i = E->begin(), e = E->end(); i!=e; ++i)
	{
		Diag << "name:" << (i->first) << "\n";
		std::string name = (i->first);	
		Diag << "\tfile:" << (i->second).first << ":" <<   (i->second).second <<"\n";
	}
}


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

	Entrys en;
	getEntrys("/tmp/entrys.xml", &en);
	//dumpEntrys(&en);

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

		PathAnnoPass PAnnoPass;
		for (Module::iterator j = M->begin(), je = M->end(); j != je; ++j)
			PAnnoPass.runOnFunction(*j);

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

