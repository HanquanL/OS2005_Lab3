linker: mmu.cpp
		g++ -o mmu mmu.cpp -std=c++11 -lpthread
clean:
		rm -f mmu *~