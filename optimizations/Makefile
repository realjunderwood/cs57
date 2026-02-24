LLVMCODE = optimizer
TEST = test

$(LLVMCODE): $(LLVMCODE).cpp
	g++ -g -I /usr/include/llvm-c-17/ -c $(LLVMCODE).cpp
	g++ $(LLVMCODE).o `llvm-config-17 --cxxflags --ldflags --libs core irreader` -I /usr/include/llvm-c-17/ -o $@

llvm_file: $(TEST).c
	clang -S -emit-llvm
	$(TEST).c -o $(TEST).ll

clean:
	rm -rf $(TEST).ll
	rm -rf $(LLVMCODE)
	rm -rf *.o
