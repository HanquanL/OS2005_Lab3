#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <unistd.h>
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
bool ifInst = false;
bool ifFrameTabe = false;
bool ifPageTable = false;
bool ifSummary = false;
bool ifCurrentPageTable = false;
bool ifAllPageTable = false;
void get_randomNumber();
int myRandom();
void readInputProcess(string inputFile,vector<Process*> *processes, vector<Instruction*> *instructions);
void printPageTable(vector<Process*> *processesVector);
void printFrameTable();
Instruction* get_next_instruction(vector<Instruction*> *instructions);
void printInstruction(Instruction *inst);
frame_t* getFrame(Pager* pager);
void initiaFrameTable(int default_frame_number, vector<frame_t*> *global_frame_table, vector<frame_t*> *freePool);
void printSummary(vector<Process*> *processVector, int instruction_idx);
void exitProcess(Process *current_proc, vector<frame_t*> *global_frame_table, vector<frame_t*> *freePool);
void unmapPage(frame_t *fte, bool pageExit);
void resetPte(page_t *pte);

int main(int argc, char *argv[]){
    string inputFile = argv[argc - 2];
    string rfile = argv[argc - 1];
    vector<Process*> processesVector;
    vector<Instruction*> instructions;
    Process *current_proc;
    int c;

    pager = new FIFO_Pager(&global_frame_table);  // FIFO_Pager is default pager
    randomNumbers.open(rfile);
    while(c = getopt(argc, argv, "f:o:a:") != -1){
        switch(c){
            case 'f':
                sscanf(optarg, "%d", &default_frame_number);
                break;
            case 'a':
                switch(optarg[0]){
                    case 'f':
                        pager = new FIFO_Pager(&global_frame_table);
                        break;
                    case 'a':
                        //pager = new Aging_Pager(&global_frame_table);
                        break;
                    case 'w':
                        //pager = new WorkingSet_Pager(&global_frame_table);
                        break;
                    case 'c':
                        //pager = new Clock_Pager(&global_frame_table);
                        break;
                    case 'r':
                        //pager = new Random_Pager(&global_frame_table);
                        break;
                    case 'e':
                        //pager = new NotRecentlyUsed_Pager(&global_frame_table);
                        break;
                    default:
                        fputs( "Invalid algorithm option\n",stderr);
                        return 1;
                }
                break;
            case 'o':
                for(int i = 0; i < 6; i++){
                    switch(optarg[i]){
                        case 'O':
                            ifInst = true;
                            break;
                        case 'F':
                            ifFrameTabe = true;
                            break;
                        case 'P':
                            ifPageTable = true;
                            break;
                        case 'S':
                            ifSummary = true;
                            break;
                        case 'x':
                            ifCurrentPageTable = true;
                            break;
                        case 'y':
                            ifAllPageTable = true;
                            break;
                    }
                }
                break;
        }
    }
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
                current_proc->state->context++;
                break;
            case 'e':
                current_proc->state->processExit++;
                exitProcess(current_proc, &global_frame_table, &freePool);
                break;
            default:
                current_proc->state->access++;
                pte = &(current_proc->page_table.at(inst->number));
                if(pte == nullptr){
                    cout << "SEGV" << endl;
                    current_proc->state->segv++;
                    break;
                }
                if(!pte -> PRESENT){
                    frame_t *frame_t = getFrame(pager);
                    if(frame_t->process != nullptr){
                        unmapPage(frame_t, false);
                    }
                    if(pte->file_mapped){
                        if(ifInst){
                            cout << " FIN" << endl;
                            current_proc->state->pagefin++;
                        }
                    }else if(pte->PAGEDOUT){
                        if(ifInst){
                            cout << " IN" << endl;
                            current_proc->state->pagein++;
                        }
                    }else{
                        if(ifInst){
                            cout << " ZERO" << endl;
                            current_proc->state->zeroOp++;
                        }
                    }
                
                    frame_t->process = current_proc;
                    frame_t->virtaul_page_number = inst->number;
                    frame_t->timeOfLastAccess = instruction_idx + 1;

                    resetPte(pte);
                    pte->PRESENT = 1;
                    pte->page_frame_number = frame_t->frame_number;
                    current_proc->page_table.at(inst->number);
                    if(ifInst){
                        cout << " MAP" << frame_t->frame_number << endl;
                    }
                    current_proc->state->map++;
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
    printSummary(&processesVector, instruction_idx);

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
            }else if(pte.PAGEDOUT){
                cout << " #";
            }else{
                cout << " *";
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

void printSummary(vector<Process*> *processVector, int instruction_idx){
    PageState *state;
    unsigned long long cost = 0LL;
    unsigned long long context_switchs = 0LL;
    unsigned long long processExit = 0LL;
    for(int i = 0; i < processVector->size(); i++){
        state = processVector->at(i)-> state;
        cost += ((state->map * 410) + (state->unmap * 440) + (state-> pagein * 3210) + (state->pageout * 2850) +
                (state->pagefin * 3350) + (state->pagefout * 2930) + (state->zeroOp * 160) + (state->segv *444) +
                (state->segprot * 414) + (state->access * 1) + (state->context * 140) + (state->processExit * 1430));

        context_switchs += state -> context;
        processExit += state -> processExit;
        printf("PROC[%d]: U=%llu M=%llu I=%llu O=%llu FI=%llu FO=%llu Z=%llu SV=%llu SP=%llu\n",
                processVector->at(i)->pid, state->unmap, state->map, state->pagein, state->pageout,
                state->pagefin, state->pagefout, state->zeroOp, state->segv, state->segprot);
    }
    printf("TOTAL: %llu %llu %llu %llu %llu\n", static_cast<unsigned long long>(instruction_idx), context_switchs, 
            processExit, cost, static_cast<unsigned long long>(sizeof(page_t)));
}

void exitProcess(Process *current_proc, vector<frame_t*> *global_frame_table, vector<frame_t*> *freePool){
    page_t *pte;
    cout << "exit current process " << current_proc->pid << endl;
    for(int i = 0; i < current_proc->page_table.size(); i++){
        pte = &(current_proc->page_table.at(i));
        pte->PAGEDOUT = 0;
        if(pte->PRESENT){
            int fteptr = pte->page_frame_number;
            frame_t *fte = global_frame_table->at(fteptr);
            unmapPage(fte, true);
            freePool->push_back(fte);
        }
    }
}

void unmapPage(frame_t *fte, bool pageExit){
    Process *process = fte->process;
    if(ifInst){
        printf("UNMAP %d:%d\n", process->pid, fte->virtaul_page_number);
    }
    process->state->unmap++;
    if(!pageExit && process->page_table.at(fte->virtaul_page_number).MODIFIED &
        !process->page_table.at(fte->virtaul_page_number).file_mapped){
            if(ifInst){
                printf(" OUT\n");
                process->state->pageout++;
            }
    }
    if(process->page_table.at(fte->virtaul_page_number).file_mapped &
        process->page_table.at(fte->virtaul_page_number).MODIFIED){
            if(ifInst){
                printf(" FOUT\n");
                process->state->pagefout++;
            }
    }
    if(!pageExit && process->page_table.at(fte->virtaul_page_number).MODIFIED &
        !process->page_table.at(fte->virtaul_page_number).file_mapped){
            process->page_table.at(fte->virtaul_page_number).PAGEDOUT = 1;
    }
    process->page_table.at(fte->virtaul_page_number).PRESENT = 0;
    fte->virtaul_page_number = -1;
    fte->process = nullptr;
}

void resetPte(page_t *pte){
    pte->PRESENT = 0;
    pte->MODIFIED = 0;
    pte->REFERENCED = 0;
    pte->page_frame_number = 0;
}