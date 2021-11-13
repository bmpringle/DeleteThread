test:
	clang++ -O3 -std=c++20 test.cpp -o tst
test-dbg:
	clang++ -g -O0 -std=c++20 test.cpp -o tst