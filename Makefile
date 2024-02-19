test:
	g++ -std=c++2b tests/tests.cpp *.cpp -I. -o test
	./test
	rm test