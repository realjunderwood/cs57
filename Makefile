LLVM_CONFIG = /opt/homebrew/opt/llvm/bin/llvm-config
BISON = /opt/homebrew/Cellar/bison/3.8.2/bin/bison

CXX = g++
CXXFLAGS = $(shell $(LLVM_CONFIG) --cxxflags) -I /opt/homebrew/opt/llvm/include
LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags --libs core)

SRCS = frontend/ast.c frontend/y.tab.c frontend/lex.yy.c \
       frontend/semanticcheck.cpp \
       llvmirbuilder/irbuilder.cpp \
       optimizations/optimizer.cpp \
       main.cpp

TARGET = compiler

all: $(TARGET)

$(TARGET): $(SRCS)
	PATH="/opt/homebrew/opt/m4/bin:$$PATH" $(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

gen: frontend/parser.y frontend/grammar.lex
	cd frontend && PATH="/opt/homebrew/opt/m4/bin:$$PATH" $(BISON) -d -y parser.y
	cd frontend && flex grammar.lex

clean:
	rm -f $(TARGET)

.PHONY: all clean gen
