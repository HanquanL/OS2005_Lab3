#ifndef mmu_h
#define mmu_h
#include <iostream>
#include <vector>

using namespace std;

#define MAX_PTE 64
#define MAX_FRAMES 128

class vma{
    public:
        int start_vpage;
        int end_vpage;
        int write_protected;
        int file_mapped;
        vma(int start_vpage, int end_vpage, int write_protected, int file_mapped){
            this->start_vpage = start_vpage;
            this->end_vpage = end_vpage;
            this->write_protected = write_protected;
            this->file_mapped = file_mapped;
        }
};

struct page_t{
    unsigned int PRESENT: 1; 
    unsigned int REFERENCED: 1;
    unsigned int MODIFIED: 1;
    unsigned int WRITE_PROTECTED: 1;
    unsigned int PAGEDOUT: 1;
    unsigned int page_frame_number: 7;
    // ensure objects has at most 32 bits
    unsigned int file_mapped: 1;
    unsigned int spare_bits: 19;
};

class PageState{
    public:
        unsigned long long segv;
        unsigned long long segprot;
        unsigned long long pageout;
        unsigned long long pagein;
        unsigned long long unmap;
        unsigned long long map;
        unsigned long long pagefin;
        unsigned long long pagefout;
        unsigned long long zeroOp;
        unsigned long long access;
        unsigned long long context;
        unsigned long long processExit;
        PageState(){
            segv = 0;
            segprot = 0;
            pageout = 0;
            pagein = 0;
            unmap = 0;
            map = 0;
            pagefin = 0;
            pagefout = 0;
            zeroOp = 0;
            access = 0;
            context = 0;
            processExit = 0;
        };
};

class Process{
    public:
        int pid;
        vector<vma> vmas;
        vector<page_t> page_table;
        PageState *state;
    Process(int pid) : page_table(MAX_PTE){
        this->pid = pid;
        this->state = new PageState();
    };
};

class Instruction{
    public:
        char operation;
        int number;
    Instruction(char operation, int number){
            this->operation = operation;
            this->number = number;
    };
};

struct frame_t{
    Process *process;
    int virtaul_page_number;
    int frame_number;
    // Age coutner for aging algorithm
    unsigned int age_counter: 32;
    // Timestamp for working set algorithm
    unsigned long long timeOfLastAccess;
};

class Pager{
    public:
        virtual frame_t* select_victim_frame() = 0;
};

class FIFO_Pager : public Pager{
    private:
        int hand;
        vector<frame_t*> *frames;
    public:
        frame_t* select_victim_frame() override{
            int framePointer = hand;
            hand++;
            if(hand >= frames->size()){
                hand = 0;
            }
            return frames->at(framePointer);
        };
        FIFO_Pager(vector<frame_t*> *frames){
            this->frames = frames;
            this->hand = 0;
        };

};





#endif