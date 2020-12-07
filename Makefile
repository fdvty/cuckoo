
cuckoo: main.cpp 
	g++ -mavx2 -O2 main.cpp -std=c++11 -o cuckoo 
	# -D _HANG_LINK

clean: 
	rm cuckoo
