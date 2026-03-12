LLVM_CONFIG = llvm-config-18
BISON = bison

CXX = g++
CXXFLAGS = $(shell $(LLVM_CONFIG) --cxxflags)
LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags --libs core)

SRCS = frontend/ast.c frontend/y.tab.c frontend/lex.yy.c \
       frontend/semanticcheck.cpp \
       llvmirbuilder/irbuilder.cpp \
       optimizations/optimizer.cpp \
       assemblygenerator/assemblygen.cpp \
       main.cpp

TARGET = compiler

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

gen: frontend/parser.y frontend/grammar.lex
	cd frontend && PATH="/opt/homebrew/opt/m4/bin:$$PATH" $(BISON) -d -y parser.y
	cd frontend && flex grammar.lex

clean:
	rm -f $(TARGET)

.PHONY: all clean gen
