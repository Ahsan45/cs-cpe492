#include <iostream>
#include <queue>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <time.h>
#include <vector>
#include <array>
using namespace std;

//If this value is true, all debug statements are printed.
bool DEBUG = false;


class PageTable{
    private:
        int id;
        std::vector<std::array<unsigned long long, 3>> pages;
    public:
        PageTable(int id, int numPages) : id(id){
            pages.resize(numPages);
            for (int i = 0; i < numPages; i++){
                pages[i][0] = i;
                pages[i][1] = 0;
                pages[i][2] = 0;
            }
        }

        void print(){
            std::cout << id << std::endl;
            for (int i = 0; i < pages.size(); i++){
                std::cout << pages[i][0] << " " << pages[i][1] << " " << pages[i][2] << std::endl;
            }
        }


};

int main(int argc, char* argv[]){
    if (argc != 6) {
        std::cout << "Usage ./assign2 plist ptrace P1 P2 P3\n"
                  << "P1: Size of pages/# of memory locations per page\n"
                  << "P2: Type of page replacement algo (FIFO, LRU, or Clock)\n"
                  << "P3: Turn on or off pre-paging ('+' for on, '-' for off)" << std::endl;
        return -1;
    }

    string plist = argv[1];
    string ptrace = argv[2];
    int sop = atoi(argv[3]);
    string algo = argv[4];
    string pre_paging = argv[5];
    
    std::ifstream i(plist + ".txt");
    std::string in;
    int program_id, total_pages;
    std::vector<PageTable> programs;
    int count = 0;

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
                    total_pages = (float(stoi(in))/float(sop) + 0.5);
                }
            }
            if (!DEBUG) std::cout << program_id << " " << total_pages << std::endl;
            programs.push_back(PageTable(program_id, total_pages));
            
        }
    }

    /*
    for (int i = 0; i < programs.size(); i++){
        programs[i].print();
    }
    */

    return 0;
}