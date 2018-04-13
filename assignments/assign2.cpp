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
#include <map>
using namespace std;

int MEMSIZE = 512;  // The maximum size of memory
int PROGSIZE;       // Size of each program in pages
int SOP;            // Size of pages

// If this value is true, all debug statements are printed.
bool DEBUG = false;

// Global Variables
unsigned long RCOUNT = 0;  // Increments when any virtual memory is referenced
unsigned long VCOUNT = 1; // Increments when valid bit is set to 1
unsigned long PSCOUNT = 0; // Increments when pages are swapped
unsigned long PCOUNT = 0;        // Increments when a virtual page is created

class PhysicalMemory{
    private:
        std::vector<unsigned long> pageFrames;
        std::map<unsigned long, unsigned long> hashtable;
    public:
        void initPhysicalMemory(){
            int size = float(MEMSIZE)/float(SOP);
            cout << size << endl;
            pageFrames.resize(size, 0);
        }

        // Prints out all of memory
        void print(){
            for (int i = 0; i < pageFrames.size(); i++){
                std::cout << i << " " << pageFrames[i] << std::endl;
            }
        }

        // Swaps in the given Virtual Memory address into a physical memory address.
        void swapIn(unsigned long virtualPage, int processid, int pageNumber){
            unsigned long pageFrame = processid*PROGSIZE+pageNumber;
            pageFrames[pageFrame] = virtualPage;
            hashtable[virtualPage] = pageFrame;
        }

        unsigned long findPhysical(unsigned long virtualPage){
            return hashtable[virtualPage];
        }
};

PhysicalMemory MAINMEM;

// Page table for each program, a vector of arrays (size 3), where each vector index is a page.
// 1st array index is the page number (unique among all pages in all tables). We can use  the vector index to find the local page number for each program's page table
// 2nd array index is the valid bit, 0 if not in memory, 1 if in memory
// 3rd array index is the last recently accessed count in FIFO and LRU, it'll be the reference bit in the clock algorithm
class PageTable{
    private:
        int id;                 // Process ID
        unsigned long hand;     // Clock Hand
        std::vector<std::array<unsigned long, 3>> pages;
    public:
        PageTable(int id, int numPages) : id(id){
            pages.resize(numPages);
            hand = 0;
            for (int i = 0; i < numPages; i++){
                pages[i][0] = PCOUNT++;
                pages[i][1] = 0;
                pages[i][2] = 0;
            }
        }

        // Use to check the page tables at a certain point
        void print(){
            // std::cout << "Process ID: " << id << std::endl;
            for (int i = 0; i < pages.size(); i++){
                std::cout << pages[i][0] << " " << pages[i][1] << " " << pages[i][2] << std::endl;
            }
            if (DEBUG){
                char str[2];
                fgets(str, 2, stdin);
            }
        }

        int getSize(){
            return pages.size();
        }

        int getID(){
            return id;
        }

        void setup(int space, string algo){
            if (pages.size() < space) space = pages.size();
            for (int i = 0; i < space; i++) {
                pages[i][1] = 1;
                MAINMEM.swapIn(pages[i][0], id, i);
                if(algo == "FIFO") pages[i][2] = VCOUNT++;
                else if(algo == "LRU") pages[i][2] = RCOUNT++;
                else if(algo == "Clock") pages[i][2] = 1;
            }
        }

        bool checkMain(int localPage, string algo){
            if(algo == "LRU") pages[localPage][2] = RCOUNT++;
            if(pages[localPage][1]==1) return true;
            else return false;
        }

        void FIFO(int localPage){
            if(DEBUG) cout << "localPage: " << localPage << endl;
            int oldestPage = -1;
            // Find the oldest loaded page
            for (int i = 0; i < pages.size(); i++){
                // Set the inital oldestPage at the first valid bit it finds.
                if(oldestPage < 0){
                    if(pages[i][1] == 1) oldestPage = i;
                }else{
                    if(pages[i][2] < pages[oldestPage][2] && pages[i][1] == 1) oldestPage = i;
                }
            }
            // Unload the old page and load the new
            pages[oldestPage][1] = 0;
            pages[localPage][1] = 1;
            pages[localPage][2] = VCOUNT++;
        }

        void LRU(int localPage){
            if(DEBUG) cout << "localPage: " << localPage << endl;
            int oldestPage = 0;
            // Find the oldest loaded page
            for (int i = 1; i < pages.size(); i++){
                // Set the inital oldestPage at the first valid bit it finds.
                if(pages[i][2] < pages[oldestPage][2]) oldestPage = i;
            }
            // Unload the old page and load the new
            pages[oldestPage][1] = 0;
            pages[localPage][1] = 1;
        }

        void clock(int localPage){
            bool replaced = false;
            while(!replaced){
                if(pages[hand][1] == 1){
                    if(pages[hand][2] == 1){
                        pages[hand][2] == 0;
                        hand = (hand+1) % PROGSIZE;
                    }else{
                        pages[hand][1] = 0;
                        pages[localPage][2] = 1;
                        pages[localPage][1] = 1;
                    }
                }
            }
        }
        
        // you can split this up into three different functions if you want
        void pageSwap(int localPage, string algo, string pagingMethod){
            if (!checkMain(localPage, algo)){
                if (algo == "FIFO") FIFO(localPage);
                else if (algo == "LRU") LRU(localPage);
                else if (algo == "Clock") clock(localPage);

                if (pagingMethod == "+"){
                    pageSwap(localPage + 1, algo, "-");
                }
            } else pageSwap(localPage + 1, algo, pagingMethod);
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
    SOP = atoi(argv[3]);     // Size of pages
    string algo = argv[4];       // Algorithm: FIFO, LRU, or Clock
    string pre_paging = argv[5]; // Pre-paging: + for on, - for off

    // Setting up first ifstream as well as vector to hold the page tables
    std::ifstream i(plist + ".txt");
    std::string in;
    int program_id, total_pages;
    std::vector<PageTable> programs;
    MAINMEM.initPhysicalMemory();

    // Opening plist and setting up the page tables
    if (i.is_open()){
        int chk;
        while (getline(i, in)){
            std::stringstream ss(in);
            chk = 0;
            while (getline(ss, in, ' ')){
                if (chk == 0){
                    program_id = std::stoi(in);
                    ++chk;
                }
                else{
                    if (DEBUG) std::cout << float(stoi(in))/float(SOP) << std::endl;
                    total_pages = ceil(float(stoi(in))/float(SOP));
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
    // As a check, SOP = 2 -> page per program = 25, SOP = 4 -> page per program = 12, SOP = 8 -> page per program = 6, etc.
    PROGSIZE = (MEMSIZE/SOP)/programs.size();
    for (int i = 0; i < programs.size(); i++){
        programs[i].setup(PROGSIZE, algo);
    }
    
    int memory_ref, line=0;
    ifstream i2(ptrace + ".txt");
    if (i2.is_open()){ 
        int chk;
        while (getline(i2, in)){
            ++line;
            std::stringstream ss(in);
            chk = 0;
            while (getline(ss, in, ' ')){
                if (chk == 0){
                    program_id = stoi(in);
                    ++chk;
                }
                else{
                    if (DEBUG) cout << program_id << " " << stoi(in) << endl;
                    memory_ref = float(stoi(in))/float(SOP);
                }
            }
            if (!programs[program_id].checkMain(memory_ref, algo)){
                // If the given memory doesn't have space to pre-page don't.
                if(memory_ref + 1 >= programs[program_id].getSize()) programs[program_id].pageSwap(memory_ref, algo, "-");
                else programs[program_id].pageSwap(memory_ref, algo, pre_paging);
                ++PSCOUNT;
                if(DEBUG) programs[program_id].print();
                if(DEBUG) cout << "Page Swaps: " << PCOUNT << endl;
            }
        }
    }

    // Copy and paste this to print page table to check values at a certain point (It'll be really long if size of page is small)
    if (!DEBUG) {
        for (int i = 0; i < programs.size(); i++){
            programs[i].print();
        }
    }

    return 0;
}
