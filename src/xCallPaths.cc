/*
 * =====================================================================================
 *
 *       Filename:  xCallPaths.cc
 *
 *    Description:  Get paths with opt pass. the same as PathGet.cc
 *
 *        Version:  1.0
 *        Created:  2014年02月06日 10时18分53秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Xiao Qixue (XQX), xiaoqixue_1@163.com
 *   Organization:  Tsinghua University
 *
 * =====================================================================================
 */


#include <llvm/Module.h>
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
#include <llvm/Support/Path.h>
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/CFG.h"

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <map>


// This will add -lxml2 dependancy when linking, and will break LLVM builds
// use CXXFLAGS="-I /usr/include/libxml2" and LDFLAGS "-l xml2"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>


// Uses Boost Graph Library
#include <boost/config.hpp>
#include <boost/utility.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

using namespace llvm;
using namespace boost;

#define Diag  std::cout //llvm::errs()

typedef std::vector< std::pair<std::string, std::pair<std::string, unsigned> > > Entrys;

// -------(BGL)---------------------------
//  typedef adjacency_list<setS, vecS, directedS, no_property, property<edge_weight_t, int> > Graph;
typedef adjacency_list<setS, vecS, bidirectionalS, no_property, property<edge_weight_t, int> > Graph;
typedef graph_traits<Graph>::vertex_descriptor Vertex;
typedef graph_traits<Graph>::edge_descriptor Edge;
typedef color_traits<default_color_type> Color;
typedef std::vector<default_color_type> ColorVec;

/*
 * =====================================================================================
 *        Class: my_dfs_visitor
 *  Description:  DFS Visitor class (used by the brute force DFS path builders)
 * =====================================================================================
 */
class CallPathsPass;
class my_dfs_visitor:public default_dfs_visitor {
	private:
		CallPathsPass *CP;
		Vertex target, root;
		std::vector<std::vector<Vertex> >*paths;
		std::vector<Vertex> tpath;
		std::vector<default_color_type> &colmap;
		long long ctr;
	public:
		my_dfs_visitor(CallPathsPass *_CP, Vertex _target, Vertex _root,
				std::vector<std::vector<Vertex> >*_paths,
				std::vector<default_color_type> &_colmap) :
			CP(_CP), target(_target), root(_root), paths(_paths), colmap(_colmap), ctr(0) { }
		bool duplicate(std::vector<Vertex> p)
		{
			for (std::vector<std::vector<Vertex> >::iterator it=paths->begin(); it != paths->end(); ++it) {
				std::vector<Vertex> cpath = *it;
				if (cpath.size() != p.size())
					continue;
				unsigned int i;
				for (i=0; i<cpath.size(); ++i) {
					if (p[i] != cpath[i])
						break;
				}
				if (i==cpath.size())
					return true;
			}
			return false;
		}
		void newPath(void)
		{
			if (tpath.size() > 0)
				if (tpath.front() == root)
					if (!duplicate(tpath))
						paths->push_back(tpath);

			std::cerr << "newPath " << paths->size() << " : " << tpath.size() << "\n";
		}
		void discover_vertex(Vertex u, const Graph & g)
		{
			//	  std::cerr << "discover " << CP->getName(u) << "\n";

			ctr++;

			if ((ctr%1000000)==0)
				std::cerr << "visited " << ctr << " vertices -- tpath size " 
					<< tpath.size() << "\n";

			tpath.push_back(u);
			if (u == target) {
				newPath();
			}
		}
		void finish_vertex(Vertex u, const Graph & g)
		{
			//	  std::cerr << "finish " << CP->getName(u) << " : " << colmap[u] <<"\n";

			tpath.pop_back();
			colmap[u] = Color::white();	  
		}
};


/*
 * =====================================================================================
 *        Class:  CallPathsPass
 *  Description:  get the call paths
 * =====================================================================================
 */
class CallPathsPass : public llvm::ModulePass {
	
	friend class my_dfs_visitor;

	protected:
		llvm::Module *M;

		//	Entrys StartEntrys; 
	public:
		static char ID;
		CallPathsPass() : ModulePass(ID) { }

		virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
			AU.setPreservesCFG();
			AU.addRequired<CallGraph>();
		}
		//virtual bool runOnFunction(llvm::Function &);
		virtual bool runOnModule(Module &_M);
		//virtual bool doInitialization(llvm::Module &);

		//bool findLineInFunction(llvm::Function *F, llvm::BasicBlock **BB, std::string srcFile, unsigned srcLine);
		//bool findLineInBB(llvm::BasicBlock *BB, std::string srcFile, unsigned srcLine);
		void getEntrys(std::string docname, Entrys *res, std::string entryName);
		void dumpEntrys(Entrys *E);

	public:
		// Set of filenames and line numbers pointing to areas of interest
		typedef std::map<std::string, std::vector<int> > defectList;
		defectList dl;
		// createCallPathsPass Input : All BB paths found
		std::vector<std::vector<BasicBlock*> > *bbpaths;
		// Parse defectFile and build the defectList (dl) map
		void getDefectList(std::string docname, defectList *res);
		void buildGraph(CallGraph *CG);
		void addBBEdges(BasicBlock *BB);
		std::string getName(Vertex v);
		std::string getBBName(Vertex v);
		void getEntryList(std::string docname, defectList *res, std::string entryName);
		void dumpEntryList(defectList *d);
		bool findLineInBB(BasicBlock *BB, std::string srcFile, unsigned srcLine);
		bool findLineInFunction(Function *F, BasicBlock **BB, std::string srcFile, unsigned srcLine);
		BasicBlock *getBB(std::string srcFile, int srcLine);
		BasicBlock *getBB(Vertex v);

	private:
		Graph funcG, bbG;
		std::map<Function*, Vertex> funcMap;   // Map functions to vertices  
		std::map<BasicBlock*, Vertex> bbMap;

		struct my_func_label_writer {
			CallPathsPass *CP;
			my_func_label_writer(CallPathsPass *_CP) : CP(_CP) { }
			template <class VertexOrEdge>
				void operator()(std::ostream& out, const VertexOrEdge& v) const {
					out << "[label=\"" << CP->getName(v) << "\"]";	
				}
		};
		struct my_bb_label_writer {
			CallPathsPass *CP;
			my_bb_label_writer(CallPathsPass *_CP) : CP(_CP) { }
			template <class VertexOrEdge>
				void operator()(std::ostream& out, const VertexOrEdge& v) const {
					out << "[label=\"" << CP->getBBName(v) << "\"]";	
				}
		};	
};

void CallPathsPass::getEntryList(std::string docname, defectList *res, std::string entryName)
{
	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile(docname.c_str());

	if (!doc) return;

	cur = xmlDocGetRootElement(doc);

	if (!cur) {
		xmlFreeDoc(doc);
		return;
	}

	// Iterate over defects
	while (cur) {
		if (!xmlStrcmp(cur->name, (const xmlChar *) "entry")) {
			xmlNodePtr en = cur->xmlChildrenNode;
			while (en) {

				if (xmlStrcmp(en->name, (const xmlChar *) entryName.c_str()))
				{
					en = en->next;
					continue;
				}

				xmlNodePtr d = en->xmlChildrenNode;
				std::string file;
				std::vector<int> lines;
				while (d) {
					if (!xmlStrcmp(d->name, (const xmlChar *) "file")) {
						file = (char*)xmlNodeListGetString(doc, d->xmlChildrenNode, 1);
						lines.clear();
					}
					else if (!xmlStrcmp(d->name, (const xmlChar *) "lines")) {
						xmlNodePtr e = d->xmlChildrenNode;
						while (e) {
							if (!xmlStrcmp(e->name, (const xmlChar *) "line")) {
								char *val = (char*)xmlNodeListGetString(doc, e->xmlChildrenNode, 1);
								lines.push_back(atoi(val));
							}
							e = e->next;
						}
					}
					d = d->next;
				}
				if (file != "" && lines.size() > 0)
					res->insert(std::make_pair(file,lines));

				en = en->next;
			}
			cur = cur->next;
		}
	}

	xmlFreeDoc(doc);
}

void CallPathsPass::dumpEntryList(defectList *d)
{
	for( defectList::iterator i = d->begin(), e = d->end(); i!=e; ++i)
	{
		Diag << "file:" << (i->first) << "\n";
		std::vector<int> lines = i->second;
		Diag << "lines:\n" ;
		for ( std::vector<int>::iterator li = lines.begin(), le = lines.end(); li!=le; ++li){
			Diag << "\t" << *li << "\n" ;
		}
	}
}
/*void CallPathsPass::getDefectList(std::string docname, defectList *res)*/
//{

//std::string file;
//std::vector<int> lines;

	////insert start 
	//file = "/home/xqx/projects/test/testcase/bench_test.c";
	//lines.push_back(30);
	//res->insert(std::make_pair(file,lines));

	////insert end
	//lines.clear();
	//lines.push_back(8);
	//res->insert(std::make_pair(file,lines));

/*}*/

void CallPathsPass::dumpEntrys(Entrys *E)
{
	for( Entrys::iterator i = E->begin(), e = E->end(); i!=e; ++i)
	{
		Diag << "name:" << (i->first) << "\n";
		std::string name = (i->first);	
		Diag << "\tfile:" << (i->second).first << ":" <<   (i->second).second <<"\n";
	}
}

void CallPathsPass::getEntrys(std::string docname, Entrys *res, std::string entryName)
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
			//EntryInfo eninfo;
			//Entry en;

			while (d) {
				if (!xmlStrcmp(d->name, (const xmlChar *) entryName.c_str())) {
					xmlNodePtr e = d->xmlChildrenNode;
					while( e ) {
						if(!xmlStrcmp(e->name, (const xmlChar *) "name")) {
							name = (char*)xmlNodeListGetString(doc, e->xmlChildrenNode, 1);
							Diag << "\tname: " << name.c_str() << "\n";
						}
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
					if (name != "" && file != "" && line != 0){
						Diag << "insert entry\n";	//res->insert(std::make_pair(file,lines));
						//res->push_back(en);				
						res->push_back(std::make_pair(name, make_pair(file,line)));
					}
				}
				d = d->next;
			}
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);
}

std::string CallPathsPass::getName(Vertex v)
{
	std::string res = "<null>";

	for (std::map<Function *, Vertex>::iterator it=funcMap.begin(); it != funcMap.end(); ++it) {
		if (v == it->second) {
			Function *F = it->first;
			if (F != NULL)
				res = F->getName();
		}
	} 
	return res;
}

std::string CallPathsPass::getBBName(Vertex v)
{
	//std::string res = "<null>";
	StringRef res;

	for (std::map<BasicBlock*, Vertex>::iterator it=bbMap.begin(); it != bbMap.end(); ++it) {
		if (v == it->second) {
			BasicBlock *BB = it->first;
			if (BB != NULL) {
				Function *F = BB->getParent();
				res = F->getName().str() + ":" + BB->getName().str();
			}
		}
	} 
	return res.str();
}

// This functions relies on the fact that a OutEdgeList of type setS doesn't allow
// non-unique edges (and returns false if such an edge is being inserted)
void CallPathsPass::addBBEdges(BasicBlock *BB)
{
	graph_traits<Graph>::edge_descriptor e; bool inserted;
	property_map<Graph, edge_weight_t>::type bbWeightmap = get(edge_weight, bbG);

	for (succ_iterator si = succ_begin(BB); si != succ_end(BB); ++si) {
		//	const char *bbName = si->getNameStr().c_str();
		// Dont get stuck in loops
		// if (!isBBPred(BB,*si))
		boost::tie(e, inserted) = add_edge(bbMap[BB], bbMap[*si], bbG);
		if (inserted)
			addBBEdges(*si);
		bbWeightmap[e] = 1;
	}
}
void CallPathsPass::buildGraph(CallGraph *CG)
{
	std::cerr << "Building Vertices... ";
	for (llvm::Module::iterator fit=M->begin(); fit != M->end(); ++fit) {
		Function *F = fit;
		funcMap[F] = add_vertex(funcG);
		for (Function::iterator bbIt = F->begin(), bb_ie = F->end(); bbIt != bb_ie; ++bbIt) {
			BasicBlock *BB = bbIt;
			bbMap[BB] = add_vertex(bbG);
		}
	}    
	std::cerr << "funcMap: " << num_vertices(funcG) << " - bbMap: " << num_vertices(bbG) << "\n";

	property_map<Graph, edge_weight_t>::type funcWeightmap = get(edge_weight, funcG);
	property_map<Graph, edge_weight_t>::type bbWeightmap = get(edge_weight, bbG);
	std::cerr << "Building Edges... ";

	for (Module::iterator fit = M->begin(); fit != M->end(); ++fit) {
		Function *F = fit;
		//	const char *fName = F->getNameStr().c_str();
		CallGraphNode *cgn = CG->getOrInsertFunction(F);
		if (cgn == NULL)
			continue;

		graph_traits<Graph>::edge_descriptor e; 
		bool inserted;

		// Create edges for Function-internal BBs
		if (!F->empty()) {
			BasicBlock *BB = &F->getEntryBlock();
			addBBEdges(BB);
		}

		// Create cross-functional edges
		for (CallGraphNode::iterator cit = cgn->begin(); cit != cgn->end(); ++cit) {
			CallGraphNode *tcgn = cit->second;
			Function *tF = tcgn->getFunction();
			if (tF == NULL)
				continue;
			//	  const char *tfName = tF->getNameStr().c_str();
			boost::tie(e, inserted) = add_edge(funcMap[F], funcMap[tF], funcG);
			funcWeightmap[e] = 1;

			if (tF->empty())
				continue;

			//CallSite myCs = cit->first;
			//CallSite *myCs = dyn_cast<CallSite>(cit->first);
			//Instruction *myI = myCs->getInstruction();
			Instruction *myI = dyn_cast<Instruction>(cit->first);
			BasicBlock *myBB = myI->getParent();

			Function::iterator cBBit = tF->getEntryBlock();
			BasicBlock *childBB = cBBit;
			if (childBB == NULL) 
				continue;
			boost::tie(e, inserted) = add_edge(bbMap[myBB], bbMap[childBB], bbG);
			bbWeightmap[e] = 1;
		}
	}  

	std::cerr << "funcMap: " << num_edges(funcG) << " - bbMap: " << num_edges(bbG) << "\n";

	std::ofstream funcs_file("funcs.dot");
	boost::write_graphviz(funcs_file, funcG, my_func_label_writer(this));    

	 std::ofstream bbs_file("bbs.dot");
	 boost::write_graphviz(bbs_file, bbG, my_bb_label_writer(this));    
}
bool CallPathsPass::runOnModule(Module &_M) {
	M = &_M;
	CallGraph *CG = &getAnalysis<CallGraph>();
	CallGraphNode *root = CG->getRoot();
	if (root == NULL) return false;
	Function *rootF = root->getFunction();
	if (rootF == NULL) return false;
	BasicBlock *rootBB = &rootF->getEntryBlock();
	if (rootBB == NULL) return false;

	// Parse and extract data from Defect XML
	//getDefectList("test", &dl);
	//if (dl.size()==0) return true;

	// Build the BGL Graph
	buildGraph(CG);

	std::cout << "\n============CallPathsPass=====\n\n";
	defectList enStart, enEnd;
	getEntryList("/tmp/entrys.xml", &enStart, "start");
	getEntryList("/tmp/entrys.xml", &enEnd, "end");
	getEntryList("/tmp/entrys.xml", &dl, "end");
	dumpEntryList(&enStart);
	dumpEntryList(&enEnd);
	dumpEntryList(&dl);
	// --------------------------------------------------------------
	// ---- Brute Force DFS on bbGraph 
#if 1
	Vertex s = bbMap[rootBB];
	for (defectList::iterator dit=dl.begin(); dit != dl.end(); ++dit) {
		std::string file = dit->first;
		std::vector<int> lines = dit->second;
		for (std::vector<int>::iterator lit=lines.begin(); lit != lines.end(); ++lit) {
			std::cerr << "Looking for '" << file << "' (" << *lit << ")\n";
			BasicBlock *BB = getBB(file, *lit);
			if (BB!= NULL) {
				Function *F = BB->getParent();
				std::cerr << "Finding Paths to " << F->getName().str() << " : " << BB->getName().str() << "\n";
				std::vector<default_color_type> colmap(num_vertices(bbG));
				std::vector<std::vector<Vertex> >paths;
				my_dfs_visitor vis(this, bbMap[BB], s, &paths, colmap);
				depth_first_search(bbG, vis, &colmap[0], s);

				std::cerr << "Finding " << paths.size() << " Paths end "  << "\n";
				// Now convert to bbPaths...

				for (unsigned int i = 0; i < paths.size(); ++i) {
					std::vector<BasicBlock*> tpath;
					for (std::vector<Vertex>::iterator it=paths[i].begin(); it != paths[i].end(); ++it) {
						//std::cerr << "set  Paths ******** "  << "\n";
						std::cout << getBBName(*it) << " -- ";
						tpath.push_back(getBB(*it));
					}
					//bbpaths->push_back(tpath);
					std::cout << "\n";
				}
			}
		}
	}
#endif

	 std::ofstream bbs_file("bbs-after.dot");
	 boost::write_graphviz(bbs_file, bbG, my_bb_label_writer(this));    

	return true;
}

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

bool CallPathsPass::findLineInBB(BasicBlock *BB, std::string srcFile, unsigned srcLine)
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

bool CallPathsPass::findLineInFunction(Function *F, BasicBlock **BB, std::string srcFile, unsigned srcLine)
{
	for (Function::iterator bbIt = F->begin(), bb_ie = F->end(); bbIt != bb_ie; ++bbIt) {
		*BB = bbIt;
		if (findLineInBB(*BB,srcFile,srcLine))
			return true;
	}
	return false;
}

BasicBlock *CallPathsPass::getBB(std::string srcFile, int srcLine)
{
	for (Module::iterator fit=M->begin(); fit != M->end(); ++fit) {
		Function *F = fit;
		for (Function::iterator bbit=F->begin(); bbit != F->end(); ++bbit) {
			BasicBlock *BB = bbit;
			if (findLineInBB(BB, srcFile, srcLine))
				return BB;
		}
	}
	return NULL;
}

BasicBlock *CallPathsPass::getBB(Vertex v)
{
	for (std::map<BasicBlock*, Vertex>::iterator it=bbMap.begin(); it != bbMap.end(); ++it) {
		if (v == it->second)
			return it->first;
	} 
	return NULL;
}

char CallPathsPass::ID = 0;
static RegisterPass<CallPathsPass>
X("xCallPaths", "get paths from CallGraph", false, true);//X(passArg, Name, CFGonly=false, isAnalysis=false)





