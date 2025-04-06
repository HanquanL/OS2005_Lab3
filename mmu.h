#ifndef mmu_h
#define mmu_h
#include <iostream>
#include <vector>

using namespace std;

#define MAX_PTE 64
#define MAX_FRAMES 128

class Process{
    public:
        vector<vma> vmas;
        page_table page_table[MAX_PTE];

};

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

struct page_table{
    unsigned int PRESENT: 1; 
    unsigned int REFERENCED: 1;
    unsigned int MODIFIED: 1;
    unsigned int WRITE_PROTECTED: 1;
    unsigned int PAGEDOUT: 1;
    unsigned int page_frame_number: 7;
    unsigned int free_to_use: 20;
};




#endif