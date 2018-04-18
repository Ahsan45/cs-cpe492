#include <iostream>
#include <list>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <fstream>

// Global Variables
int BLOCKSIZE;  // Size of the block in the system

class Ldisk{
    private:
        // Node that saves disk nodes
        class LdiskNode{
            public:
                bool free;          // Determines if the node is free or not.
                std::vector<int> block_set;

                LdiskNode(bool free, int first, int size) : free(free){
                    block_set.resize(size);
                    for(int i = 0; i < size; i++){
                        block_set[i] = first + i;
                    }
                }

                void print(){
                    if(free) std::cout << "Free: ";
                    else std::cout << "In use: ";
                    std::cout << block_set.front() << "-" << block_set.back() << std::endl;
                }

                void append(std::vector<int> append_set){
                    block_set.insert(block_set.end(), append_set.begin(), append_set.end());
                }
        };

        std::list<LdiskNode> nodes;

    public:
        // Prints out disk footprint
        void diskFootprint(){
            for(std::list<LdiskNode>::iterator it=nodes.begin(); it != nodes.end(); ++it){
                it->print();
            }
            // Todo: get disk fragmentation
        }

        // Initiates Ldisk. We do this after BLOCKSIZE is set.
        void initLdisk(int block_count){
            nodes.push_front(LdiskNode(true, 0, block_count));
        }

        // FInds free node and returns its block_set
        std::vector<int> fillFree(){
            std::list<LdiskNode>::iterator it = nodes.begin();
            while(!(it->free)){
                ++it;
            }
            it->free = false;
            return it->block_set;
        }

        // Recombine contiguous free/occupied nodes
        void recombine(){
            std::list<LdiskNode> new_nodes;
            int mode=-1;   // 1 is Free mode, 0 is Occupied mode, -1 is Unset

            for(std::list<LdiskNode>::iterator it=nodes.begin(); it != nodes.end(); ++it){
                // What to do if the previous one was free
                bool free = it->free;
                if(mode == 1){
                    if(free){
                        new_nodes.back().append(it->block_set);
                    }else{
                        mode = 0;
                        new_nodes.push_back(*it);
                    }
                // What to do if the previous one was occupied
                }else if(mode == 0){
                    if( !(free) ){
                        new_nodes.back().append(it->block_set);
                    }else{
                        mode = 1;
                        new_nodes.push_back(*it);
                    }
                // What to do if this is the first iteration
                }else{
                    if(free) mode = 1;
                    else mode = 0;
                    new_nodes.push_front(*it);
                }
            }
            nodes = new_nodes;
        }

};

Ldisk LDISK;

class Lfile{
    private:
        std::list<int> addresses;
    public:
        Lfile(std::string filename, int filesize) {
            int block_count = filesize / BLOCKSIZE;
            while(block_count > 0){
                std::vector<int> block_set = LDISK.fillFree();
                block_count -= block_set.size();
                std::copy(block_set.begin(), block_set.end(), std::back_inserter(addresses));   // Add the given vectors to the back of Addresses
            }
            LDISK.recombine();
        }
};

class FileTree{
    private:
        struct TreeNode{
            std::string name;
            bool directory;
            std::vector<TreeNode> nodes;

            // For fIles only
            int size;
            std::string time;
        };
        TreeNode root;
    public:
        FileTree(){
            root.name = "./";
            root.directory = true;
        }
        int getHeight(){
            getHeightHelper(root, 0);
        }
        int getHeightHelper(TreeNode tree, int height){
            if(tree.nodes.size() > 0){
                int max = height;
                for(std::vector<TreeNode>::iterator it=tree.nodes.begin(); it != tree.nodes.end(); ++it){
                    if(it->directory){
                        int dir_height = getHeightHelper(*it, height+1);
                        if(dir_height > max){
                            max = dir_height;
                        }
                    }
                }
                return max;
            }else return height+1; 
        }

        //Not done yet
        void print(){
            printHelper(root);
        }
        void printHelper(TreeNode tree){
            for(std::vector<TreeNode>::iterator it=tree.nodes.begin(); it != tree.nodes.end(); ++it){
                if(it->directory){
                    std::cout << it->name << " ";
                }
                std::cout << std::endl;
            }

            for(std::vector<TreeNode>::iterator it=tree.nodes.begin(); it != tree.nodes.end(); ++it){
                if(it->directory){
                    std::cout << it->name << " ";
                }
            }
        }
};

int main(int argc, char* argv[]){
    if (argc != 9) {
        std::cout   << "Usage ./assign2 "
                    << "-f [input file storing information on files] "
                    << "-d [input file storing information on directories] "
                    << "-s [disk size] "
                    << "-b [block size]"
                    << std::endl;
        return -1;
    }

    // Find the indexies of our flags
    int findex = 0; // Index of flag -f
    int dindex = 0; // Index of flag -d
    int sindex = 0; // Index of flag -s
    int bindex = 0; // Index of flag -b
    for(int i = 1; i < argc; i++){
        // I check if index is 0 first for efficiency
        if(findex != 0 && strcmp(argv[i], "-f") == 0){
            findex = i;
        }else if(dindex != 0 && strcmp(argv[i], "-d") == 0){
            dindex = i;
        }else if(sindex != 0 && strcmp(argv[i], "-s") == 0){
            sindex = i;
        }else if(bindex != 0 && strcmp(argv[i], "-b") == 0){
            bindex = i;
        }
    }

    // Save all of our argument variables
    std::string file_list = argv[findex+1];
    std::string dir_list = argv[dindex+1];
    int disk_size = atoi(argv[sindex+1]);
    BLOCKSIZE = atoi(argv[bindex+1]);
    int block_count = disk_size/BLOCKSIZE;

    std::ifstream files(file_list);
    return 0;
}