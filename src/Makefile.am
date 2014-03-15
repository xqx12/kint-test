#AM_CXXFLAGS = `llvm-config --cxxflags` -Werror -Wall -O0 -g
AM_CXXFLAGS =  -Werror -Wall -O0 -g \
			   -I/home/xqx/data/xqx/projects/llvm-test/buildllvm31/include \
				-I`llvm-config --includedir` -D_DEBUG -D_GNU_SOURCE \
			   -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS \
			   -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer  \
			   -fvisibility-inlines-hidden -fno-exceptions \
			   -fPIC -Woverloaded-virtual -Wcast-qual
#GCC_ENABLE_CPP_RTTI = YES

noinst_LTLIBRARIES = libsat.la
lib_LTLIBRARIES    = libintck.la libcmpck.la
bin_PROGRAMS       = intglobal pathanalysis
EXTRA_DIST         = intck cmpck llvm/DataLayout.h llvm/DebugInfo.h llvm/IRBuilder.h

all-local: libintck.la libcmpck.la
	@cd $(top_builddir)/lib && $(LN_S) -f ../src/.libs/libintck.so
	@cd $(top_builddir)/lib && $(LN_S) -f ../src/.libs/libcmpck.so
	@cd $(top_builddir)/bin && $(LN_S) -f ../src/intglobal
	@cd $(top_builddir)/bin && $(LN_S) -f ../src/pathanalysis

libsat_la_CPPFLAGS = -I$(top_builddir)/lib
libsat_la_SOURCES  = ValueGen.cc PathGen.cc Diagnostic.cc SMTSolver.cc
libsat_la_SOURCES += ValueGen.h PathGen.h Diagnostic.h SMTSolver.h
libsat_la_SOURCES += SMTBoolector.cc
libsat_la_LIBADD   = -lboolector -llgl
#libsat_la_SOURCES += SMTSonolar.cc
#libsat_la_LIBADD   = -lsonolar
#libsat_la_SOURCES += SMTZ3.cc
#libsat_la_LIBADD   = -lz3 -lgomp
libsat_la_LDFLAGS  = -L$(top_builddir)/lib


libintck_la_CPPFLAGS = -I/usr/include/libxml2 \
			-I/home/xqx/data/xqx/projects/boost_1_55_0/installed/include \
				-L/home/xqx/data/xqx/projects/boost_1_55_0/installed/lib
libintck_la_SOURCES = IntRewrite.cc IntLibcalls.cc IntSat.cc \
	OverflowIdiom.cc OverflowSimplify.cc xCallGraph.cc xCFG.cc \
	LoadRewrite.cc xCallPaths.cc
libintck_la_LIBADD  = libsat.la
libintck_la_LDFLAGS = -module -lxml2

libcmpck_la_SOURCES = CmpTautology.cc CmpOverflow.cc CmpSat.cc
libcmpck_la_LIBADD  = libsat.la
libcmpck_la_LDFLAGS = -module

intglobal_LDFLAGS = `llvm-config --ldflags` -lLLVM-`llvm-config --version`
intglobal_SOURCES = IntGlobal.cc Annotation.cc CallGraph.cc Taint.cc Range.cc \
	IntGlobal.h Annotation.h CRange.h

pathanalysis_CPPFLAGS = -I/usr/include/libxml2 \
			-I/home/xqx/data/xqx/projects/boost_1_55_0/installed/include \
				-L/home/xqx/data/xqx/projects/boost_1_55_0/installed/lib
pathanalysis_LDFLAGS = `llvm-config --ldflags` -lLLVM-`llvm-config --version` -lxml2 
pathanalysis_SOURCES = PathAnalysis.cc PathAnno.cc PathGet.cc \
	PathAnalysis.h PathAnno.h PathGet.h
