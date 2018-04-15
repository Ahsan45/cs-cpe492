#include <iostream>
#include <queue>
#include <stdlib.h>
#include <unistd.h>
#include <cmath>
#include <fstream>
#include <sstream>
#include <time.h>
#include <vector>
#include <array>
using namespace std;

//If this value is true, all debug statements are printed.
bool DEBUG = false;

// Global Variables
unsigned long RCOUNT = 0;   // Increments when any virtual memory is referenced
unsigned long VCOUNT = 1;   // Increments when valid bit is set to 1
unsigned long PSCOUNT = 0;  // Increments when pages are swapped
unsigned long PCOUNT = 0;   // Increments when a virtual page is created
int MEMSIZE = 512;          // The maximum size of memory

// Page table for each program, a vector of arrays (size 3), where each vector index is a page.
// 1st array index is the page number (unique among all pages in all tables). We can use  the vector index to find the local page number for each program's page table
// 2nd array index is the valid bit, 0 if not in memory, 1 if in memory
// 3rd array index is the last recently accessed count in FIFO and LRU, it'll be the reference bit in the clock algorithm
// Memory vector represent pages in memory and will be used to easily swap out pages
class PageTable{
    private:
        int id;
        int hand = 0;
        std::vector<std::array<unsigned long, 3>> pages;
        vector<unsigned long> memory;
    public:
        PageTable(int id, int numPages) : id(id){
            hand = 0;
            pages.resize(numPages);
            for (int i = 0; i < numPages; i++){
                pages[i][0] = PCOUNT;
                pages[i][1] = 0;
                pages[i][2] = 0;
                PCOUNT++;
            }
        }

        ~PageTable(){}

        // Use to check the page tables at a certain point
        void print(){
            std::cout << id << std::endl;
            for (int i = 0; i < pages.size(); i++){
                if (DEBUG) std::cout << pages[i][0] << " " << pages[i][1] << " " << pages[i][2] << std::endl;
            }
            for (int i = 0; i < memory.size(); i++){
                if (DEBUG) cout << memory[i] << " ";
            }
            cout << endl;
        }

        // Load initial pages into memory
        void setup(int space){
            if (pages.size() < space){
                space = pages.size();
            }
            for (int i = 0; i < space; i++){
                pages[i][1] = 1;
                memory.push_back(pages[i][0]);
            }
        }

        // Convert unique id to local page
        int getLocalPage(int uniquePage){
            for (int i = 0; i < pages.size(); i++){
                if (pages[i][0] == uniquePage){
                    return i;
                }
            }
        }

        // Check if page is in main memory and update 3rd bit if using LRU
        bool checkMain(int localPage, string algo){
            if (algo == "LRU"){
                pages[localPage][2] = RCOUNT;
            }

            if (pages[localPage][1] == 1){
                return true;
            }
            else{
                return false;

            }
        }

        // First-in-first-out algo
        void FIFO(int localPage){
            /*if(DEBUG) cout << "localPage: " << localPage << endl;
            pages[getLocalPage(memory[hand])][1] = 0;
            pages[localPage][1] = 1;
            memory[hand] = pages[localPage][0];
            if (hand == memory.size() - 1){
                hand = 0;
            }
            else{
                ++hand;
            }*/
            
           pages[getLocalPage(memory[0])][1] = 0;
           pages[localPage][1] = 1;
           memory.erase(memory.begin());
           memory.push_back(pages[localPage][0]);
        }

        // Least recently used algo
        void LRU(int localPage){
            if(DEBUG) cout << "localPage: " << localPage << endl;
            int oldestPage = getLocalPage(memory[0]);
            int mainLoc = 0;
            for (int i = 1; i < memory.size(); i++){
                if(DEBUG) cout << "CHECK: " << i << endl;
                if (pages[getLocalPage(memory[i])][2] < pages[oldestPage][2]){
                    if (DEBUG) cout << "SWAP" << endl;
                    oldestPage = getLocalPage(memory[i]);
                    mainLoc = i;
                }
            }
            pages[oldestPage][1] = 0;
            pages[localPage][1] = 1;
            memory[mainLoc] = pages[localPage][0];
        }

        // Clock algo
        void clock(int localPage){
            if(DEBUG) cout << "localPage: " << localPage << endl;
            bool replaced = false;
            while (!replaced){
                if (pages[getLocalPage(memory[hand])][2] == 0){
                    pages[getLocalPage(memory[hand])][1] = 0;
                    pages[localPage][1] = 1;
                    pages[localPage][2] = 1;
                    memory[hand] = pages[localPage][0];
                    replaced = true;
                }
                else{
                    pages[getLocalPage(memory[hand])][2] = 0;
                }
                if (hand == memory.size() - 1){
                    hand = 0;
                }
                else{
                    hand++;
                }
            }
        }
        
        void pageSwap(int localPage, string algo, string pagingMethod, int originalPage){
            if (!checkMain(localPage, algo)){
                if (algo == "FIFO"){
                    FIFO(localPage);
                }
                else if (algo == "LRU"){
                    LRU(localPage);
                }
                else{
                    clock(localPage);
                }

                // If pre-paging is on, just call the function with an incremented local page and keep going if the following pages are already in memory
                if (pagingMethod == "+"){
                    pagingMethod = "-";
                    if (localPage == pages.size() - 1){
                        localPage = 0;
                    }
                    else {
                        ++localPage;
                    }
                    pageSwap(localPage, algo, pagingMethod, originalPage);
                    return;
                }
                return;
            }
            else {
                pagingMethod = "-";
                if (localPage == pages.size() - 1){
                    localPage = 0;
                }
                else if (localPage == originalPage){
                    return;
                }
                else {
                    ++localPage;
                }
                pageSwap(localPage, algo, pagingMethod, originalPage);
                return;
            }
        }
};

int main(int argc, char* argv[]){
    // Ensuring the correct amount of parameters
    if (argc != 6) {
        std::cout << "Usage ./assign2 plist ptrace P1 P2 P3\n"
                  << "P1: Size of pages/# of memory locations per page\n"
                  << "P2: Type of page replacement algo (FIFO, LRU, or Clock)\n"
                  << "P3: Turn on or off pre-paging ('+' for on, '-' for off)" << std::endl;
        return -1;
    }

    // Input values

    string plist = argv[1];      // Plist file
    string ptrace = argv[2];     // Ptrace file
    int sop = atoi(argv[3]);     // Size of pages
    string algo = argv[4];       // Algorithm: FIFO, LRU, or Clock
    string pre_paging = argv[5]; // Pre-paging: + for on, - for off

    // Setting up first ifstream as well as vector to hold the page tables
    std::ifstream i(plist + ".txt");
    std::string in;
    int program_id, total_pages;
    std::vector<PageTable> programs;

    // Opening plist and setting up the page tables
    if (i.is_open()){
        while (getline(i, in)){
            std::stringstream ss(in);
            int chk = 0;
            while (getline(ss, in, ' ')){
                if (chk == 0){
                    program_id = std::stoi(in);
                    chk++;
                }
                else{
                    if (DEBUG) std::cout << float(stoi(in))/float(sop) << std::endl;
                    total_pages = ceil(float(stoi(in))/float(sop));
                }
            }
            if (DEBUG) std::cout << program_id << " " << total_pages << std::endl;
            programs.push_back(PageTable(program_id, total_pages));
            
        }
    }
    i.close();

    // Default loading of memory
    // Dividing the total memory by size of pages to get how many pages can fit in memory. 
    // Divide that by number of programs to find how many pages each program is allocated.
    // As a check, sop = 2 -> page per program = 25, sop = 4 -> page per program = 12, sop = 8 -> page per program = 6, etc.
    
    int mem_space = (MEMSIZE/sop)/programs.size(); 
    if (DEBUG) cout << "Pages per program: " << mem_space << endl;

    for (int i = 0; i < programs.size(); i++){
        programs[i].setup(mem_space);
    }

    // Copy and paste this to print page table to check values at a certain point (It'll be really long if size of page is small)
    if (DEBUG) {
        for (int i = 0; i < programs.size(); i++){
            programs[i].print();
        }
    }
    
    // Begin reading ptrace and performing swaps as necessary
    int memory_ref;
    ifstream i2(ptrace + ".txt");
    if (i2.is_open()){
        while (getline(i2, in)){
            stringstream ss2(in);
            int chk = 0;
            while (getline(ss2, in, ' ')){
                if (chk == 0){
                    program_id = stoi(in);
                    chk++;
                }
                else{
                    if (DEBUG) cout << program_id << " " << stoi(in) << endl;
                    memory_ref = float(stoi(in))/float(sop);
                }
            }
            // Increment the reference count for each line
            ++RCOUNT;
            if (DEBUG) cout << program_id << " " << memory_ref << endl;
            //Swap if page isn't in main memory and increment page swap counter
            if (!programs[program_id].checkMain(memory_ref, algo)){
                programs[program_id].pageSwap(memory_ref, algo, pre_paging, memory_ref);
                ++PSCOUNT;
                if(DEBUG) programs[program_id].print();
                if(DEBUG) cout << "Page Swaps: " << PSCOUNT << endl;
                
            }
        }
    }
    if (DEBUG) cout << "RCOUNT: " << RCOUNT << endl;
    cout << "Total Page Faults: " << PSCOUNT << endl;

    

    return 0;
}
