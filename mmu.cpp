#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include "mmu.h"

using namespace std;

ifstream randomNumbers;
vector<int> randvals;
int randomRange,randomOffset = 0;
vector<frame_t> global_frame_table;
void get_randomNumber();
int myRandom();
void readInputProcess(string inputFile,vector<Process*> *processes, vector<instruction*> *instructions);


int main(int argc, char *argv[]){
    string inputFile = argv[argc - 2];
    string rfile = argv[argc - 1];
    vector<Process*> processesVector;
    vector<instruction*> instructions;
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

void readInputProcess(string inputFile,vector<Process*> *processesVector, vector<instruction*> *instructions){
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
            instruction *tempInstruction = new instruction(operation, page);
            instructions->push_back(tempInstruction);
        }
    }

    file.close();
}