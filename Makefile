
cuckoo: main.cpp 
	g++ -mavx2 -O2 main.cpp -std=c++11 -o cuckoo 

clean: 
	rm cuckoo
