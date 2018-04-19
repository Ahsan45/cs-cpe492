#include <iostream>
#include <list>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <fstream>
#include <map>
#include <sstream>
#include <math.h>
// Global Variables
int BLOCKSIZE;          // Size of the block in the system
int BLOCKCOUNT;         //

bool DEBUG = true;      // If on, prints debug statements

class Ldisk{
    private:
        // Node that saves disk nodes
        class LdiskNode{
            public:
                bool free;          // Determines if the node is free or not.
                std::vector<int> block_set;

                // Generates vector based on first element and size
                std::vector<int> genVector(int first, int size){
                    std::vector<int> result;
                    result.resize(size);
                    for(int i = 0; i < size; i++){
                        // if (DEBUG) std::cout << "genVector Looping!" << std::endl;
                        result[i] = first + i;
                    }
                    return result;
                }

                LdiskNode(bool free, int first, int size) : free(free){
                    // if (DEBUG) std::cout << "Creating an LdiskNode" << std::endl;
                    // if (DEBUG) std::cout << "LdiskNode size: " << size << " first: " << first << std::endl;
                    block_set = genVector(first, size);
                    // if (DEBUG) std::cout << "Created an LdiskNode" << std::endl;
                }

                // Prints out LdiskNode in the format specified by PDF
                void print(){
                    if(free) std::cout << "Free: ";
                    else std::cout << "In use: ";
                    std::cout << block_set.front() << "-" << block_set.back() << std::endl;
                }

                // Helpful in combining two nodes
                void append(std::vector<int> append_set){
                    block_set.insert(block_set.end(), append_set.begin(), append_set.end());
                }

                // Cuts out size elements and returns it as a class
                LdiskNode split(int size){
                    int first_block = block_set[0];
                    block_set = genVector(block_set[size], block_set.size()-size);
                    return LdiskNode(this->free, first_block, size);
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
            // if (DEBUG) std::cout << "Initializing Ldisk!" << std::endl;
            nodes.push_front(LdiskNode(true, 0, block_count));
            // if (DEBUG) std::cout << "Initialized Ldisk!" << std::endl;
        }

        // Finds free node and returns its block_set, then sets the free node to occupied
        std::list<LdiskNode>::iterator findFree(){
            std::list<LdiskNode>::iterator it = nodes.begin();
            while(!(it->free) && it != nodes.end()){
                ++it;
            }
            return it;
        }
        // Splits the node at the iterator by the size of the first half, then returns iterator to the first half.
        std::list<LdiskNode>::iterator split(std::list<LdiskNode>::iterator it, int size){
            // if (DEBUG) std::cout << "split: Given size: " << size << std::endl;
            LdiskNode first_half = it->split(size);     // Gets the the first half
            // if (DEBUG) std::cout << "split: New split size: " << first_half.block_set.size() << std::endl;
            return nodes.insert(it, first_half);
        }

        // Recombine contiguous free/occupied nodes
        void recombine(){
            std::list<LdiskNode> new_nodes; // Represents new nodes list
            int mode=-1;                    // 1 is Free mode, 0 is Occupied mode, -1 is Unset

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

        // Fill free blocks then returns the vector of all the filled blocks
        std::vector<int> fillFree(int size){
            std::vector<int> blocks;
            std::list<LdiskNode>::iterator it;
            int node_size;
            if (DEBUG) std::cout << "fillFree block size: " << size << std::endl;
            while(size > 0){
                // if (DEBUG) std::cout << "fillFree Looping!" << std::endl;
                it = findFree();
                node_size = it->block_set.size();
                if (DEBUG) std::cout << "fillFree: LDISK free size - " << it->block_set.size() << std::endl;
                if(node_size == 0){
                    std::cout << "NOT ENOUGH SPACE FOR ALL FILES." << std::endl;
                    exit(0); 
                }
                if(size >= node_size){
                    if (DEBUG) std::cout << "Bigger than block size" << std::endl;
                    size -= node_size;
                }else{
                    if (DEBUG) std::cout << "Smaller than block size" << std::endl;
                    it = split(it, size);   // Splits then returns the first block of the split
                    size = 0;
                }
                it->free = false;
                std::copy(it->block_set.begin(), it->block_set.end(), std::back_inserter(blocks));  // Appends vector elements to blocks
            }
            // if (DEBUG) std::cout << "fillFree: Out of Loop!" << std::endl;
            recombine();
            return blocks;
        }

};

Ldisk LDISK;

class Lfile{
    private:
        std::list<int> addresses;
    public:
        void initLfile(int filesize) {
            int block_count = ceil( float(filesize) / float(BLOCKSIZE) );
            // if (DEBUG) std::cout << "Before fillFree" << std::endl;
            std::vector<int> blocks = LDISK.fillFree(block_count);
            // if (DEBUG) std::cout << "After fillFree" << std::endl;
            std::copy(blocks.begin(), blocks.end(), std::back_inserter(addresses));  // Appends vector elements to addresses
        }

        void print(){
            for(std::list<int>::iterator it=addresses.begin(); it != addresses.end(); ++it){
                if(it != addresses.begin()) std::cout << "->";
                std::cout << *it;
            }
            std::cout << std::endl;
        }
};

class FileTree{
    private:
        struct TreeNode{
            std::string name;
            bool is_dir;

            // Only for directories
            std::map<std::string, TreeNode> nodes; 

            // Only for fIles
            int size;
            std::string time;
            Lfile lfile;
        };
        TreeNode root;
    public:
        FileTree(){
            root.name = "./";
            root.is_dir = true;
        }

        // Gets the level of the tree. Used in breadth first traversal.
        int getLevel(){
            return getLevelHelper(root, 1);
        }
        int getLevelHelper(TreeNode& tree, int level){
            // if (DEBUG) std::cout << "getLevelHelper: Reached File!" << std::endl;
            // if (DEBUG) std::cout << "Tree name: " << tree.name << " Number of Children: " << tree.nodes.size() << std::endl;
            if(tree.nodes.size() > 0){
                int max = level+1;
                int new_level;
                for(std::map<std::string, TreeNode>::iterator it=tree.nodes.begin(); it != tree.nodes.end(); ++it){
                    new_level = getLevelHelper(it->second, level+1);
                    if(new_level > max) max = new_level;
                }
                return max;
            }else return level;
        }

        // Prints directory structure breadth first
        void printDir(){
            int levels = getLevel();
            for(int i = 0; i < levels; i++){
                std::cout << "[Dir Level: " << i << "]" << std::endl;
                printDirHelper(root, i);
                std::cout << std::endl;
            }
        }
        void printDirHelper(TreeNode& tree, int depth){
            if(depth != 0){
                for(std::map<std::string, TreeNode>::iterator it=tree.nodes.begin(); it != tree.nodes.end(); ++it){
                    if(it->second.is_dir) printDirHelper(it->second, depth-1);
                }
            }else{
                if(tree.is_dir) std::cout << tree.name << " ";
            }
        }

        // Prints Files breadth first
        void printFiles(){
            int levels = getLevel();
            // if (DEBUG) std::cout << "printFiles - tree level: " << levels << std::endl;
            for(int i = 0; i < levels; i++){
                // if (DEBUG) std::cout << "printFIles: Printing Files!" << std::endl;
                std::cout << "[File Level: " << i << "]" << std::endl;
                printFilesHelper(root, i);
            }
        }
        void printFilesHelper(TreeNode& tree, int depth){
            // if (DEBUG) std::cout << "printFilesHelper - Depth: " << depth << std::endl;
            // if (DEBUG) std::cout << "printFilesHelper - Name: " << tree.name << std::endl;
            // if (DEBUG) std::cout << "printFilesHelper - Children: " << tree.nodes.size() << std::endl;
            if(depth != 0){
                for(std::map<std::string, TreeNode>::iterator it=tree.nodes.begin(); it != tree.nodes.end(); ++it){
                    printFilesHelper(it->second, depth-1);
                }
            }else{
                if(!tree.is_dir){
                    std::cout << "File name: " << tree.name << std::endl;
                    std::cout << "File size: " << tree.size << std::endl;
                    std::cout << "File timestamp: " << tree.time << std::endl;
                    std::cout << "Addresses: ";
                    tree.lfile.print();
                }
            }
        }


        // Finds the directory to add new files/folders into
        TreeNode* getDirectory(std::string path){
            int loc;
            TreeNode* cur_node = &root;
            std::string name;
            path.erase(0,2);    //Since we're already at root
            while((loc = path.find("/")) != std::string::npos){
                // if (DEBUG) std::cout << "getDirectory: Looping!" << std::endl;
                name = path.substr(0, loc);
                path.erase(0, loc+1);           // +1 to delete the slash
                // if (DEBUG) std::cout << "getDirectory name: " << name << " leftover path: " << path << std::endl;

                // Add a directory if missing or get the directory
                if(cur_node->nodes.find(name) != cur_node->nodes.end()){
                    // if (DEBUG) std::cout << "Found Directory!" << std::endl;
                    cur_node = &(cur_node->nodes[name]);
                }else{
                    // if (DEBUG) std::cout << "Creating Directory!" << std::endl;
                    TreeNode new_dir;
                    new_dir.name = name;
                    new_dir.is_dir = true;
                    // if (DEBUG) std::cout << "getDirectory - Current Node before size: " << cur_node->nodes.size() << std::endl;
                    cur_node->nodes[name] = new_dir;
                    // if (DEBUG) std::cout << "getDirectory - Current Node after size: " << cur_node->nodes.size() << std::endl;
                    cur_node = &(cur_node->nodes[name]);
                    // if (DEBUG) std::cout << "Current Node name: " << cur_node->name << std::endl;
                }
            }
            return cur_node;
        }

        void updateRoot(std::string path){
            TreeNode* new_root = getDirectory(path);
            root = *new_root;
        }

        // Adds a directory
        void addDirectory(std::string full_path){
            int loc = full_path.rfind("/");
            std::string name = full_path.substr(loc+1);
            std::string path = full_path.substr(0, loc+1);
            TreeNode* node = getDirectory(path);           // Creates directory if missing.
            TreeNode new_dir;
            new_dir.name = name;
            new_dir.is_dir = true;
            node->nodes[name] = new_dir;
        }

        // Adds a file
        void addFile(std::string full_path, int size, std::string time){
            int loc = full_path.rfind("/");
            std::string name = full_path.substr(loc+1);
            // if (DEBUG) std::cout << "addFile: name - " << name << std::endl;
            std::string path = full_path.substr(0, loc+1);  // Remove root characters
            // if (DEBUG) std::cout << "Adding File" << std::endl;;
            TreeNode* dir = getDirectory(path);             // Creates directory if missing. 
            // if (DEBUG) std::cout << "Got Directory" << std::endl;;
            // if (DEBUG) std::cout << "Directory Name: " << dir->name << std::endl;
            TreeNode new_file;
            new_file.name = name;
            new_file.is_dir = false;
            new_file.size = size;
            new_file.time = time;
            new_file.lfile.initLfile(size);
            dir->nodes[name] = new_file;
            // if (DEBUG) std::cout << "addFile - Number of child nodes: " << dir->nodes.size() << std::endl;
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
        if(strcmp(argv[i], "-f") == 0){
            findex = i;
        }else if(strcmp(argv[i], "-d") == 0){
            dindex = i;
        }else if(strcmp(argv[i], "-s") == 0){
            sindex = i;
        }else if(strcmp(argv[i], "-b") == 0){
            bindex = i;
        }
    }

    // Save all of our argument variables
    std::string file_list = argv[findex+1];
    std::string dir_list = argv[dindex+1];
    int disk_size = atoi(argv[sindex+1]);   // For our current file_list, we need at least 37 MB
    BLOCKSIZE = atoi(argv[bindex+1]);
    int block_count = disk_size/BLOCKSIZE;

    if (DEBUG) {
        // std::cout << "file_list: "      << file_list    << std::endl;
        // std::cout << "dir_list: "       << dir_list     << std::endl;
        // std::cout << "disk_size: "      << disk_size    << std::endl;
        // std::cout << "BLOCKSIZE: "      << BLOCKSIZE    << std::endl;
        std::cout << "block_count: "    << block_count  << std::endl;
    }

    LDISK.initLdisk(block_count);
    // if (DEBUG) std::cout << "passed initLdisk" << std::endl;

    // Directory Tree
    FileTree tree;

    std::string item;
    std::string path;
    std::string time;
    int size;
    std::ifstream dirs(dir_list);
    if(dirs.is_open()){
        int column;
        while(getline(dirs, path)){
            // If filesize is 0 don't bother
            // if (DEBUG) std::cout << "Adding Directory: " << path << std::endl;
            tree.addDirectory(path);
            // if (DEBUG){
            //     char ch[2];
            //     fgets(ch, 2, stdin);
            // }
        }
    }
    dirs.close();
    if (DEBUG) tree.printDir();

    int allfilesize = 0;
    std::ifstream files(file_list);
    if(files.is_open()){
        int column;
        while(getline(files, item)){
            std::stringstream ss(item);
            column = -1;
            path = "";
            time = "";
            while(getline(ss, item, ' ')){
                if(item.length() != 0){
                    ++column;
                    // if (DEBUG) std::cout << "Column: " << column << std::endl;
                    // if (DEBUG) std::cout << "Item: " << item << std::endl;
                    // if (DEBUG) std::cout << item.length() << std::endl;
                    if (column == 6){
                        std::stringstream integer(item);
                        integer >> size;
                        // if (DEBUG) std::cout << "Column 6: " << size << std::endl;
                    }else if(column == 7 || column == 8 || column == 9){
                        if(column != 7) time += " ";
                        time += item;
                    }else if(column >= 10){
                        if(column != 10) path += " ";
                        path += item;
                    }
                }
            }
            if (DEBUG) allfilesize += size;
            // if (DEBUG) std::cout << "Time: " << time << std::endl;
            // if (DEBUG) std::cout << "Path: " << path << std::endl;
            // if (DEBUG) std::cout << "Size: " << size << std::endl;
            // If filesize is 0 don't bother
            if(size != 0) tree.addFile(path, size, time);
            // LDISK.diskFootprint();
            // if (DEBUG){
            //     char ch[2];
            //     fgets(ch, 2, stdin);
            // }
        }
    }
    if (DEBUG) std::cout << "File size in total: " << allfilesize << std::endl;
    files.close();
    if (DEBUG) tree.printFiles();
    return 0;
}
