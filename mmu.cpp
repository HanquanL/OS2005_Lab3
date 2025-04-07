#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "mmu.h"

using namespace std;

ifstream randomNumbers;
vector<int> randvals;
int randomRange,randomOffset = 0;
void get_randomNumber();
int myRandom();
void readInputFile(string inputFile);

int main(int argc, char *argv[]){
    string inputFile = argv[argc - 2];
    string rfile = argv[argc - 1];
    randomNumbers.open(rfile);
    get_randomNumber();
    cout << myRandom() << endl; //for test purposes
    cout << myRandom() << endl; //for test purposes
    cout << myRandom() << endl; //for test purposes


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

void readInputFile(string inputFile){
    fstream file;
    file.open(inputFile);
    
}