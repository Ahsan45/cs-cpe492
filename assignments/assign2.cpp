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
unsigned long long RCOUNT = 0;
unsigned long PCOUNT = 0;

// Page table for each program, a vector of arrays (size 3), where each vector index is a page.
// 1st array index is the page number (unique among all pages in all tables). We can use  the vector index to find the local page number for each program's page table
// 2nd array index is the valid bit, 0 if not in memory, 1 if in memory
// 3rd array index is the last recently accessed count in FIFO and LRU, it'll be the reference bit in the clock algorithm
class PageTable{
    private:
        int id;
        int memory;
        std::vector<std::array<unsigned long long, 3>> pages;
    public:
        PageTable(int id, int numPages) : id(id){
            pages.resize(numPages);
            for (int i = 0; i < numPages; i++){
                pages[i][0] = PCOUNT;
                pages[i][1] = 0;
                pages[i][2] = 0;
                PCOUNT++;
            }
        }

        // Use to check the page tables at a certain point
        void print(){
            std::cout << id << std::endl;
            for (int i = 0; i < pages.size(); i++){
                std::cout << pages[i][0] << " " << pages[i][1] << " " << pages[i][2] << std::endl;
            }
        }

        void setup(int space){
            if (pages.size() < space){
                space = pages.size();
            }
            for (int i = 0; i < space; i++){
                pages[i][1] = 1;
            }
        }

        bool checkMain(int localPage){
            if (pages[localPage][1] == 1)
                return true;
            return false;
        }
        
        // you can split this up into three different functions if you want
        void pageSwap(int localPage, string algo, string pagingMethod){
            if (!checkMain(localPage)){
                if (algo == "FIFO"){

                }
                else if (algo == "LRU"){

                }
                else{

                }

                if (pagingMethod == "+"){
                    pagingMethod = "-";
                    pageSwap(localPage + 1, algo, pagingMethod);
                    return;
                }
                return;
            }
            else {
                pagingMethod = "-";
                pageSwap(localPage + 1, algo, pagingMethod);
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
    int page_swaps = 0;

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
            if (!DEBUG) std::cout << program_id << " " << total_pages << std::endl;
            programs.push_back(PageTable(program_id, total_pages));
            
        }
    }
    i.close();

    // Default loading of memory
    // Dividing the total memory by size of pages to get how many pages can fit in memory. 
    // Divide that by number of programs to find how many pages each program is allocated.
    // As a check, sop = 2 -> page per program = 25, sop = 4 -> page per program = 12, sop = 8 -> page per program = 6, etc.
    for (int i = 0; i < programs.size(); i++){
    int mem_space = (512/sop)/programs.size(); 
        programs[i].setup(mem_space);
    }

    // Copy and paste this to print page table to check values at a certain point (It'll be really long if size of page is small)
    if (DEBUG) {
        for (int i = 0; i < programs.size(); i++){
            programs[i].print();
        }
    }
    
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
                    if (!DEBUG) cout << program_id << " " << stoi(in) << endl;
                    memory_ref = ceil(float(stoi(in))/float(sop));
                }
            }
            if (!DEBUG) cout << memory_ref << endl;
            if (!programs[program_id].checkMain(memory_ref)){
                programs[program_id].pageSwap(memory_ref, algo, pre_paging);
                page_swaps++;
                if(!DEBUG) cout << "Page Swaps: " << page_swaps << endl;
            }
        }
    }

    return 0;
}
