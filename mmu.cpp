#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include "mmu.h"

using namespace std;

#define MAX_PTE 64
#define MAX_FRAMES 128

ifstream randomNumbers;
vector<int> randvals;
int randomRange,randomOffset = 0;
vector<frame_t*> global_frame_table;
vector<frame_t*> freePool;
int instruction_idx = 0;
bool EOI = false;
Pager* pager;
int default_frame_number = 16;
void get_randomNumber();
int myRandom();
void readInputProcess(string inputFile,vector<Process*> *processes, vector<Instruction*> *instructions);
void printPageTable(vector<Process*> *processesVector);
void printFrameTable();
Instruction* get_next_instruction(vector<Instruction*> *instructions);
void printInstruction(Instruction *inst);
frame_t* getFrame(Pager* pager);
void initiaFrameTable(int default_frame_number, vector<frame_t*> *global_frame_table, vector<frame_t*> *freePool);


int main(int argc, char *argv[]){
    string inputFile = argv[argc - 2];
    string rfile = argv[argc - 1];
    vector<Process*> processesVector;
    vector<Instruction*> instructions;
    Process *current_proc;
    randomNumbers.open(rfile);
    get_randomNumber();
    // cout << myRandom() << endl; //for test purposes
    // cout << myRandom() << endl; //for test purposes
    readInputProcess(inputFile, &processesVector, &instructions);
    // for(auto process : processesVector){
    //     cout << "Process ID: " << process->pid << endl;
    //     for(auto vma : process->vmas){
    //         cout << "VMA: " << vma.start_vpage << " " << vma.end_vpage << " " 
    //         << vma.write_protected << " " << vma.file_mapped << endl;
    //     }
    // }
    // for(auto inst : instructions){
    //     cout << "Instruction: " << inst->operation << " " << inst->page_number << endl;
    // }
    initiaFrameTable(default_frame_number, &global_frame_table, &freePool);
    pager = new FIFO_Pager(&global_frame_table);
    while(!EOI){
        Instruction *inst = get_next_instruction(&instructions);
        if(inst == nullptr){
            break;
        }
        printInstruction(inst);
        page_t *pte;
        switch(inst->operation){
            case 'c':
                current_proc = processesVector.at(inst->number);
                break;
            case 'e':
                break;
            default:
                
                pte = &(current_proc->page_table.at(inst->number));
                if(pte == nullptr){
                    cout << "SEGV" << endl;
                    break;
                }
                if(pte -> file_mapped){
                    cout << " FINALIZE " << endl;
                };
                if(!pte -> PRESENT){
                    frame_t *old_frame_t = getFrame(pager);
                    old_frame_t->process = current_proc;
                    old_frame_t->virtaul_page_number = inst->number;
                    old_frame_t->timeOfLastAccess = instruction_idx + 1;

                    pte->PRESENT = 1;
                    pte->page_frame_number = old_frame_t->frame_number;
                    
                }
                pte->REFERENCED = 1;
                if(inst -> operation == 'w'){
                    if(pte -> WRITE_PROTECTED){
                        cout << " SEGPROT" << endl;
                    }else{
                        pte->MODIFIED = 1;
                    }
                }
                break;
        }
        instruction_idx++;
       
    }
    printPageTable( &processesVector );
    printFrameTable();

    return 0;
}

void get_randomNumber(){
    string line;
    int randomNumber;
    getline(randomNumbers, line);
    istringstream randRange(line);
    randRange >> randomRange;
    while(getline(randomNumbers, line)){
        istringstream iss(line);
        iss >> randomNumber;
        randvals.push_back(randomNumber);
    }
}

int myRandom(){
    int number = randvals[randomOffset];
    randomOffset = (randomOffset + 1) % randomRange;
    return number;
}

void readInputProcess(string inputFile,vector<Process*> *processesVector, vector<Instruction*> *instructions){
    ifstream file(inputFile);
    int numOfProcesses = 0;
    if(!file.is_open()){
        cout << "Error opening file" << inputFile << endl;
        return;
    }

    string line;
    // helper lambda to get next non-comment & non-empty line
    auto getNextLine = [&file, &line]() -> bool {
        while (getline(file, line)) {
            // ignore empty lines and lines that start with "#"
            if(line.empty() || line[0] == '#'){
                continue;
            }
            return true; // found a valid line
        }
        return false; // no more lines to read
    };

    // Read the number of processes
    if(getNextLine()){
        istringstream iss(line);
        iss >> numOfProcesses;
    }

    // Read the process information
    for(int p = 0; p < numOfProcesses; p++){
        // read number of VMAs for this process
        int numOfVMAs = 0;
        if(getNextLine()){
            istringstream iss(line);
            iss >> numOfVMAs;
        }

        Process *process = new Process(p);

        // Read the VMAs for this process
        for(int v = 0; v < numOfVMAs; v++){
            if(getNextLine()){
                int start_vpage, end_vpage, write_protected, file_mapped;
                istringstream iss(line);
                iss >> start_vpage >> end_vpage >> write_protected >> file_mapped;
                process->vmas.push_back(vma(start_vpage, end_vpage, write_protected, file_mapped));
            }
        }

        processesVector->push_back(process);
    }

    // Read the instructions
    while(getline(file, line)){
        if(!line.empty() && line[0] != '#'){
            istringstream iss(line);
            char operation;
            int page;
            iss >> operation >> page;
            Instruction *tempInstruction = new Instruction(operation, page);
            instructions->push_back(tempInstruction);
        }
    }

    file.close();
}

void printPageTable(vector<Process*> *processesVector){
    for(int i = 0; i < processesVector -> size(); i++){
        Process *current_proc = processesVector -> at(i);
        page_t pte;
        printf("PT[%d]:",current_proc -> pid);
        for(int j = 0; j < current_proc->page_table.size(); j++){
            pte = current_proc -> page_table.at(j);
            if(pte.PRESENT){
                printf( " %d:", j);
                if(pte.REFERENCED){
                    cout << "R";
                }else{
                    cout << "-";
                }

                if(pte.MODIFIED){
                    cout << "M";
                }else{
                    cout << "-";
                }

                if(pte.PAGEDOUT){
                    cout << "S";
                }else{
                    cout << "-";
                }
            }
        }
        cout << " " << endl;
    }
}

void printFrameTable(){
    frame_t *fte;
    printf("FT: ");
    for(int i = 0; i < global_frame_table.size(); i++){
        fte = global_frame_table[i];
        if(fte->virtaul_page_number == -1){
            cout << " *";
        }else{
            printf(" %d:%d", fte->process->pid, fte->virtaul_page_number);
        }
    }
    cout << " " << endl;
}

Instruction* get_next_instruction(vector<Instruction*> *instructions){
    if(instruction_idx < instructions->size()){
        Instruction *tempInstruction = instructions->at(instruction_idx);
        return tempInstruction;
    }else{
        EOI = true;
        return nullptr;
    }
}

void printInstruction(Instruction *inst){
    printf("%d: ==> %c %d\n", instruction_idx, inst->operation, inst->number);
}

frame_t* getFrame(Pager* pager){
    if(freePool.size() > 0){
        frame_t *frame = freePool.front();
        freePool.erase(freePool.begin());
        return frame;
    }else{
        return pager->select_victim_frame();
    }
}

void initiaFrameTable(int default_frame_number, vector<frame_t*> *global_frame_table, vector<frame_t*> *freePool){
    for(int i = 0; i < default_frame_number; i++){
        frame_t *frame = new frame_t();
        frame->virtaul_page_number = -1;
        frame->frame_number = i;
        global_frame_table->push_back(frame);
        freePool->push_back(frame);
    }
}