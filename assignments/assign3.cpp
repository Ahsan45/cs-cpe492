#include <iostream>
#include <list>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <fstream>
#include <map>
#include <chrono>
#include <ctime>
#include <sstream>
#include <math.h>
// Global Variables
int BLOCKSIZE;          // Size of the block in the system

bool DEBUG = false;      // If on, prints debug statements

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
                        result[i] = first + i;
                    }
                    return result;
                }

                // Initializes LdiskNode by generating a vector from first to size-1
                LdiskNode(bool free, int first, int size) : free(free){
                    block_set = genVector(first, size);
                }

                // Prints out LdiskNode in the format specified by PDF
                void print(){
                    if(free) std::cout << "Free: ";
                    else std::cout << "In use: ";
                    std::cout << block_set.front() << "-" << block_set.back() << std::endl;
                }

                // Appends the given vector to this blockset. Helpful in combining two nodes.
                void append(std::vector<int> append_set){
                    block_set.insert(block_set.end(), append_set.begin(), append_set.end());
                }

                // Cuts out size elements from the front and returns it
                LdiskNode split(int size){
                    int first_block = block_set[0];                                 // Saves it since block_set will be updated
                    block_set = genVector(block_set[size], block_set.size()-size);  // Updates current block set to be equal to the second half
                    return LdiskNode(this->free, first_block, size);                // Returns the second haff
                }

                // Cuts out size elements from the back and returns it
                LdiskNode backSplit(int size){
                    int second_block = block_set[block_set.size()-size];            // Saves the first block of the second half
                    block_set = genVector(block_set[0], block_set.size()-size);     // Updates current block set to be equal to the first half
                    return LdiskNode(this->free, second_block, size);               // Returns the first half
                }

                // Checks if a given block is included in this Node
                bool blockIncluded(int block){
                    if(block >= block_set[0] && block <= block_set[block_set.size()-1]){
                        return true;
                    }else return false;
                }
        };
        std::list<LdiskNode> nodes;                         // Saves a linked list of all the nodes
        typedef std::list<LdiskNode>::iterator LDNiter;     // Shortenes the iterator

    public:
        // Initiates Ldisk. We do this after BLOCKSIZE is set, since the very first node's size will be BLOCKSIZE.
        void initLdisk(int block_count){
            nodes.push_front(LdiskNode(true, 0, block_count));
        }

        // Gets the sum of occupied blocks
        int sumOccupied(){
            int size = 0;
            for(LDNiter it=nodes.begin(); it != nodes.end(); ++it){
                if(!it->free){
                    size += it->block_set.size();
                }
            }
            return size;
        }

        // Gets the sum of free blocks
        int sumFree(){
            int size = 0;
            for(LDNiter it=nodes.begin(); it != nodes.end(); ++it){
                if(it->free){
                    size += it->block_set.size();
                }
            }
            return size;
        }

        // Prints out disk footprint without disk fragmentation. Fragmentation is calculated by the tree.
        void diskFootprint(){
            for(LDNiter it=nodes.begin(); it != nodes.end(); ++it){
                it->print();
            }
        }

        // Finds a node that includes the given block
        LDNiter findNode(int block){
            LDNiter it;
            for(it = nodes.begin(); (!it->blockIncluded(block)) && it != nodes.end(); ++it);
            return it;
        }

        // Gets the first free node and returns an iterator pointing to that node.
        LDNiter findFree(){
            LDNiter it;
            for(it = nodes.begin(); !(it->free) && it != nodes.end(); ++it);
            return it;
        }

        // Splits the node by cutting out size elements from the front. It returns iterator to that first half.
        LDNiter split(LDNiter it, int size){
            if(size > 0 && size < it->block_set.size()) {
                LdiskNode first_half = it->split(size); // Gets the the first half
                return nodes.insert(it, first_half);
            }else return it;
        }

        // Splits the node by cutting out size elements from the back. It returns iterator to that second half.
        LDNiter backSplit(LDNiter it, int size){
            if(size > 0 && size < it->block_set.size()) {
                LdiskNode second_half = it->backSplit(size);   // Gets the the second half
                return nodes.insert(++it, second_half);
            }else return it;
        }

        // Splits from a specified block and size elements from that block, inclusive.
        LDNiter rangeSplit(int first, int size){
            LDNiter node = findNode(first);
            node = backSplit(node, node->block_set[node->block_set.size()-1] - first + 1);
            node = split(node, size);
            return node;
        }

        // Recombine contiguous free/occupied nodes
        void recombine(){
            std::list<LdiskNode> new_nodes; // Represents new nodes list
            int mode=-1;                    // 1 is Free mode, 0 is Occupied mode, -1 is Unset

            for(LDNiter it=nodes.begin(); it != nodes.end(); ++it){
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

        // Allocate free blocks then returns the vector of all the filled blocks
        std::vector<int> allocate(int size){
            std::vector<int> blocks;
            LDNiter it;
            int node_size;
            if(size > sumFree()){
                std::cout << "NOT ENOUGH SPACE TO GROW OR ADD FILE." << std::endl;
                return blocks;
            }else{
                while(size > 0){
                    it = findFree();
                    node_size = it->block_set.size();
                    if(size >= node_size){
                        size -= node_size;
                    }else{
                        it = split(it, size);   // Splits then returns the first block of the split
                        size = 0;
                    }
                    it->free = false;
                    std::copy(it->block_set.begin(), it->block_set.end(), std::back_inserter(blocks));  // Appends vector elements to blocks
                }
                recombine();
                return blocks;
            }
        }

        // Frees memory starting at block i up to size number of blocks
        void free(std::vector<int> free_blocks){
            LDNiter node;
            // Get a range of contiguous blocks and send it to split
            std::vector<int>::iterator it = free_blocks.begin();
            int first = *it;
            int prev = *it;
            int cur;
            while(it != free_blocks.end()){
                cur = *(++it);
                // If not contiguous, free the contiguous blocks
                if(cur-prev != 1){
                    node = rangeSplit(first, prev-first+1);
                    node->free = true;
                    first = *it;
                }
                prev = *it;
            }
            recombine();
        }
};

Ldisk LDISK;

class Lfile{
    public:
        std::list<int> addresses;
        void initLfile(int filesize) {
            if(filesize > 0){
                int block_count = ceil( float(filesize) / float(BLOCKSIZE) );
                std::vector<int> blocks = LDISK.allocate(block_count);
                std::copy(blocks.begin(), blocks.end(), std::back_inserter(addresses)); // Appends vector elements to addresses list
            }
        }

        void print(){
            for(std::list<int>::iterator it=addresses.begin(); it != addresses.end(); ++it){
                if(it != addresses.begin()) std::cout << "->";
                std::cout << *it;
            }
            std::cout << std::endl;
        }

        void updateNumBlocks(int new_filesize){
            int block_count = ceil( float(new_filesize) / float(BLOCKSIZE) );
            int old_size = addresses.size();
            if(block_count > old_size){
                std::vector<int> blocks = LDISK.allocate(block_count - old_size);       // Get new blocks
                std::copy(blocks.begin(), blocks.end(), std::back_inserter(addresses)); // Appends vector elements to addresses list
            }else if(block_count < old_size){
                std::list<int>::iterator it = addresses.begin();
                for(int i = 0; i < block_count; ++it, ++i);
                std::vector<int> free_blocks(it, addresses.end());
                LDISK.free(free_blocks);
                addresses.erase(it, addresses.end());
            }
        }
};

class FileTree{
    private:
        struct TreeNode{
            std::string name;
            std::string path;
            bool is_dir;
            TreeNode *parent;

            // Only for directories
            std::map<std::string, TreeNode> nodes;

            // Only for fIles
            int size;
            std::string time;
            Lfile lfile;
        };
        TreeNode root;
        TreeNode* cur_dir;
        typedef std::map<std::string, TreeNode>::iterator treeMapIter;
    public:
        FileTree(){
            root.name = "/";
            root.is_dir = true;
            root.parent = &root;
            cur_dir = &root;
        }

        TreeNode* getCurDir(){
            return cur_dir;
        }

        // Gets the level of the tree. Used in breadth first traversal.
        int getLevel(){
            return getLevelHelper(root, 1);
        }
        int getLevelHelper(TreeNode& tree, int level){
            if(tree.nodes.size() > 0){
                int max = level+1;
                int new_level;
                for(treeMapIter it=tree.nodes.begin(); it != tree.nodes.end(); ++it){
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
                for(treeMapIter it=tree.nodes.begin(); it != tree.nodes.end(); ++it){
                    if(it->second.is_dir) printDirHelper(it->second, depth-1);
                }
            }else{
                if(tree.is_dir) std::cout << tree.path << tree.name << std::endl;
            }
        }

        // Prints Files breadth first
        void printFiles(){
            int levels = getLevel();
            for(int i = 0; i < levels; i++){
                std::cout << "[File Level: " << i << "]" << std::endl;
                printFilesHelper(root, i);
            }
        }
        void printFilesHelper(TreeNode& tree, int depth){
            if(depth != 0){
                for(treeMapIter it=tree.nodes.begin(); it != tree.nodes.end(); ++it){
                    printFilesHelper(it->second, depth-1);
                }
            }else{
                if(!tree.is_dir){
                    std::cout << "File name: " << tree.name << std::endl;
                    std::cout << "File path: " << tree.path << std::endl;
                    std::cout << "File size: " << tree.size << std::endl;
                    std::cout << "File timestamp: " << tree.time << std::endl;
                    std::cout << "Addresses: ";
                    tree.lfile.print();
                }
            }
        }

        // Gets total size
        int getTotalSize(){
            return getSizeHelper(root);
        }
        int getSizeHelper(TreeNode& tree){
            int size = 0;
            if(!tree.is_dir) size += tree.size;
            else{
                for(treeMapIter it=tree.nodes.begin(); it != tree.nodes.end(); ++it) size += getSizeHelper(it->second);
            }
            return size;
        }

        std::string getCurTime(){
            std::string time = "";
            std::string token;

            // Get current system time
            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
            time_t t_now = std::chrono::system_clock::to_time_t ( now );
            std::string str_now = ctime(&t_now);
            std::stringstream ss_time(str_now);

            // Get the correct date format
            int column = -1;
            while(getline(ss_time, token, ' ')){
                ++column;
                if (column == 1 || column == 2 || column == 3){
                    if(column != 1) time += " ";
                    if(column == 3) time += token.substr(0,5);
                    else time += token;
                }
            }
            return time;
        }

        // Changes ' ' to '\ '
        std::string escapeSpace(std::string original){
            int loc;
            if((loc = original.find("\\ ")) == std::string::npos){
                while((loc = original.find(" ")) != std::string::npos){
                    original.replace(loc, 1, "\\$%$");    // I replaced it with a weird sequence not a space since space loops forever
                }
                while((loc = original.find("$%$")) != std::string::npos){
                    original.replace(loc, 3," ");
                }  
            }
            return original;
        }

        // Finds the directory to add new files/folders into.
        TreeNode* getDirectory(std::string path){
            int loc;
            TreeNode *current;
            std::string name, path_traversal = path;
            if(path[0] != '/'){
                current = cur_dir;
            }else{
                current = &root;
                path_traversal.erase(0,1);    // Get rid of first '/'
            }
            while((loc = path_traversal.find("/")) != std::string::npos){
                name = path_traversal.substr(0, loc);
                path_traversal.erase(0, loc+1);           // +1 to delete the slash
                // Add a directory if missing or get the directory
                if(current->nodes.find(name) != current->nodes.end()){
                    current = &(current->nodes[name]);
                }else{
                    std::cout << "Directory not found: " << path << std::endl;
                    current = NULL;
                    break;
                }
            }   
            if(current != NULL && !(current->is_dir)) {
                std::cout << "Not a directory: " << path << std::endl;
                current = NULL;   
            }
            return current;
        }

        // Makes the path valid (remove double slashes and add a slash at the end)
        std::string makePathValid(std::string path){
            int loc;
            // Remove multiple slashes
            while((loc = path.find("//")) != std::string::npos) path.erase(loc, 1);
            if(path[path.size()-1] != '/') path += "/";
            path = escapeSpace(path);
            return path;
        }

        // Changes directory
        void chdir(std::string path){
            path = makePathValid(path);
            TreeNode* dir = getDirectory(path);
            // Only update on valid directories
            if(dir != NULL){
                cur_dir = dir;
            }
        }

        void ls(){
            for(treeMapIter it=cur_dir->nodes.begin(); it != cur_dir->nodes.end(); ++it){
                if(it != cur_dir->nodes.begin()) std::cout << " ";
                std::cout << it->second.name;
            }
            std::cout << std::endl;
        }

        // Moves up a directory
        void moveUp(){
            cur_dir = cur_dir->parent;
        }

        // Adds a directory
        void addDirectory(std::string full_path){
            full_path = makePathValid(full_path);
            full_path = full_path.substr(0, full_path.size()-1);    // Remove last slash
            int loc = full_path.rfind("/");
            std::string name = full_path.substr(loc+1);
            if(name != ""){               
                std::string path = full_path.substr(0, loc+1);
                TreeNode* node = getDirectory(path);
                // Set up new directory
                TreeNode new_dir;
                new_dir.name = name;
                new_dir.path = path;
                new_dir.is_dir = true;
                new_dir.parent = node;
                // Add new directory
                node->nodes[name] = new_dir;
            }
        }

        // Adds a file at the given path which has the given size and timestamp
        void addFile(std::string full_path, int size, std::string time){
            full_path = makePathValid(full_path);
            full_path = full_path.substr(0, full_path.size()-1);    // Remove last slash
            int loc = full_path.rfind("/");
            std::string name = full_path.substr(loc+1);
            std::string path = full_path.substr(0, loc+1);          // Remove root characters
            TreeNode* dir = getDirectory(path);
            if(dir->nodes.find(name) == dir->nodes.end()){
                TreeNode new_file;
                new_file.name = name;
                new_file.path = path;
                new_file.is_dir = false;
                new_file.parent = dir;
                new_file.size = size;
                new_file.time = time;
                new_file.lfile.initLfile(size);
                dir->nodes[name] = new_file;  
            }else std::cout << "File already exists." << std::endl;
        }

        // Appends the given number of bytes (amt) from the given file
        void append(std::string full_path, int amt){
            full_path = makePathValid(full_path);
            full_path = full_path.substr(0, full_path.size()-1);    // Remove last slash
            int loc = full_path.rfind("/");
            std::string name = full_path.substr(loc+1);             // After last slash
            std::string path = full_path.substr(0, loc+1);          // Remove root characters
            TreeNode* dir = getDirectory(path);
            if(dir != NULL){
                treeMapIter it = dir->nodes.find(name);
                if(it == dir->nodes.end()) std::cout << "File does not exist." << std::endl;
                else {
                    if(!it->second.is_dir) {
                        it->second.size += amt;
                        it->second.lfile.updateNumBlocks(it->second.size);
                        it->second.time = getCurTime();
                    }
                    else std::cout << "This is a directory!" << std::endl;
                }
            }
        }

        // Removes the given number of bytes (amt) from the given file
        void remove(std::string full_path, int amt){
            full_path = makePathValid(full_path);
            full_path = full_path.substr(0, full_path.size()-1);    // Remove last slash
            int loc = full_path.rfind("/");
            std::string name = full_path.substr(loc+1);             // After last slash
            std::string path = full_path.substr(0, loc+1);          // Remove root characters
            TreeNode* dir = getDirectory(path);
            if(dir != NULL){
                treeMapIter it = dir->nodes.find(name);
                if(it == dir->nodes.end()) std::cout << "File does not exist." << std::endl;
                else{
                    if(!it->second.is_dir){
                        it->second.size -= amt;
                        if(it->second.size < 0){
                            it->second.size = 0;
                        }
                        it->second.lfile.updateNumBlocks(it->second.size);
                        it->second.time = getCurTime();
                    }else  std::cout << "This is a directory!" << std::endl;
                }
            }
        }

        void del(std::string full_path){
            full_path = makePathValid(full_path);
            full_path = full_path.substr(0, full_path.size()-1);    // Remove last slash
            int loc = full_path.rfind("/");
            std::string name = full_path.substr(loc+1);
            std::string path = full_path.substr(0, loc+1);          // Remove root characters
            TreeNode* dir = getDirectory(path);
            if(dir != NULL){
                treeMapIter it = dir->nodes.find(name);
                if(it == dir->nodes.end()) std::cout << "Directory or File does not exist." << std::endl;
                else {
                    if(!it->second.is_dir) {
                        remove(full_path, it->second.size);
                        dir->nodes.erase(name);
                    }else{
                        if(it->second.nodes.size() > 0) std::cout << "Directory is not empty." << std::endl;
                        else {
                            dir->nodes.erase(name);
                            dir->time = getCurTime();
                        }
                    }
                }
            }
        }

        void fragmentation(){
            std::cout << "fragmentation: " << LDISK.sumOccupied()*BLOCKSIZE-getTotalSize() << " bytes" << std::endl;
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
    int disk_size = atoi(argv[sindex+1]);   // For our current file_list, we need at least 128 MB or 134217728
    BLOCKSIZE = atoi(argv[bindex+1]);
    int block_count = disk_size/BLOCKSIZE;

    LDISK.initLdisk(block_count);

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
            // If filesize is 0 don't bother. substr.(1) gets rid of period
            tree.addDirectory(path.substr(1));
        }
    }
    dirs.close();

    std::ifstream files(file_list);
    if(files.is_open()){
        std::cout << "Loading Files! Hold on, should take a minute." << std::endl;
        int column;
        while(getline(files, item)){
            std::stringstream ss(item);
            column = -1;
            path = "";
            time = "";
            while(getline(ss, item, ' ')){
                if(item.length() != 0){
                    ++column;
                    if (column == 6){
                        std::stringstream integer(item);
                        integer >> size;
                    }else if(column == 7 || column == 8 || column == 9){
                        if(column != 7) time += " ";
                        time += item;
                    }else if(column >= 10){
                        if(column != 10) path += " ";
                        path += item;
                    }
                }
            }
            // If filesize is 0 don't bother. substr(1) gets rid of the period.
            if(size != 0) tree.addFile(path.substr(1), size, time);
        }
        std::cout << "Finished Loading Files!" << std::endl;
    }
    files.close();

    std::string input;
    std::string token;
    std::string args[2];
    while(1){
        std::cout << "$ ";
        std::getline(std::cin, input);
        if(input == "cd.."){
            tree.moveUp();
        }else if(input.substr(0, 3) == "cd "){
            std::stringstream ss(input.substr(3));
            std::getline(ss, token, '\n');
            tree.chdir(token);
        }else if(input == "ls"){
            tree.ls();
        }else if(input.substr(0, 6) == "mkdir "){
            std::stringstream ss(input.substr(6));
            std::getline(ss, token, '\n');
            std::string new_dir = tree.getCurDir()->path+tree.getCurDir()->name+"/"+token;
            tree.addDirectory(new_dir);
        }else if(input.substr(0, 7) == "create "){
            std::stringstream ss(input.substr(7));
            getline(ss, token, '\n');
            std::string name = token;
            std::string new_dir = tree.getCurDir()->path+tree.getCurDir()->name+"/"+token;  // New directory to be added
            tree.addFile(new_dir, 0, tree.getCurTime());
        }else if(input.substr(0, 7) == "append "){
            std::stringstream ss(input.substr(7));
            std::getline(ss, token, ' ');
            std::string path = token;
            std::getline(ss, token, ' ');
            int amount = atoi(token.c_str());
            tree.append(path, amount);
        }else if(input.substr(0, 7) == "remove "){
            std::stringstream ss(input.substr(7));
            std::getline(ss, token, ' ');
            std::string path = token;
            std::getline(ss, token, ' ');
            int amount = atoi(token.c_str());
            tree.remove(path, amount);
        }else if(input.substr(0, 7) == "delete "){
            std::stringstream ss(input.substr(7));
            std::getline(ss, token, '\n');
            tree.del(token);
        }else if(input == "exit"){
            exit(0);
        }else if(input == "dir"){
            tree.printDir();
        }else if(input == "pfiles"){
            tree.printFiles();
        }else if(input == "prdisk"){
            LDISK.diskFootprint();
            tree.fragmentation();
        }else{
            std::cout << "Command not recognized." << std::endl;
        }
        std::cin.clear();
    }

    return 0;
}
