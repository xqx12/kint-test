kint-test
---------

sort out the code of kint in this repo.

kint git clone from xiw github ,the url : origin git://g.csail.mit.edu/kint

I git push to mygithub for develop,

git remote add origin https://github.com/xqx12/kint-test.git

git push -u origin master  && git push -u origin devbyxqx


###kint analysis

####CallPaths analysis

There are two ways to get the call graph for Callpath analysis. one is get the callgraph data by llvm pass, the other is to build the callgraph by ourself. 

The first is of a merit that it will simple and precise. but it must be in a llvm pass, so it may be not get the external data easy, such as the sink of the path .  

The second could do more work by ourself, but it will be more flexible.

The path analysis in Symbolic execution project is not complete, it only add the basicblock in the node of Callgrap, and not consider the cfg node of a path.



*** Paths analysis ***

1. firstly, Getting path entry and end from user. And then annotating them in llvm IR. 
	I will code it with a individual program as intglobal.

2. secondly, search the path from entry to end or end to entry. it need to use llvm pass, so now I code it in a llvm plugin as intck. 


