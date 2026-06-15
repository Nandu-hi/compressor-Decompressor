#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <stdexcept>
#include<omp.h>
using namespace std;

// Class to write individual bits into an output stream by packing them into bytes
class bitwriter{
    unsigned char buffer = 0; // Temporarily holds bits until a full byte (8 bits) is formed
    int bitCount = 0;         // Tracks how many bits are currently in the buffer
    ostream& ofile;           // The output stream to write bytes to

    public:
    
    bitwriter(ostream& o):ofile(o){}

    // Adds a single bit ('0' or '1') to the buffer
    void writebit(char bit){
        buffer=buffer << 1; // Shift existing bits to the left to make room
        if(bit=='1'){
            buffer=buffer | 1; // Set the least significant bit to 1 if the input is '1'
        }
        bitCount++;
        // When the buffer has 8 bits, write it to the file as a byte and reset
        if(bitCount==8){
            ofile.put(buffer);
            buffer=0;
            bitCount=0;
        }
    }

    // Writes any remaining bits in the buffer to the file, padded with trailing zeros
    void flush(){
        if(bitCount>0){
            buffer=buffer << (8-bitCount); // Shift remaining bits to the most significant positions
            ofile.put(buffer);
        }
    }
};

// Node structure for the Huffman Tree
class mynode{
    public:
    char c;         // The character (leaf nodes only)
    int f;          // Frequency of the character
    mynode * left;
    mynode* right;
    mynode(char a,int b):c(a),f(b),left(NULL),right(NULL){}
    
    // Operator overloads to allow comparison based on frequency (used for the Min-Heap)
    bool operator<(mynode & other) const {
        return f<other.f;
    }
    bool operator>(mynode & other) const {
        return f>other.f;
    }
    bool operator==(mynode & other){
        return f==other.f;
    }
};

// Utility function to swap two node pointers in the heap
void swap(mynode* & a,mynode* & b){
    mynode* temp=a;
    a=b;
    b=temp;
}

// Moves a node down the heap to maintain the Min-Heap property
void percolatedown(vector<mynode*> & a,int i){
    while(2*i<a.size()){
        int minindex=2*i; // Left child index
        // Check if right child exists and is smaller than left child
        if(2*i+1 < a.size() && *(a[2*i+1])<*(a[2*i])){
            minindex=2*i+1;
        }
        // If current node is greater than the smallest child, swap them
        if(*a[i]>*a[minindex]){
            swap(a[i],a[minindex]);
            i=minindex; // Continue percolating down
        }
        else break;
        
    }
}

// Moves a node up the heap to maintain the Min-Heap property
void percolateup(vector<mynode*> & a,int i){
    // While not at root and parent is greater than current node
    while(i>1 && *a[i/2]>*a[i]){
        swap(a[i/2],a[i]);
        i=i/2; // Continue percolating up
    }
}

// Removes and returns the node with the smallest frequency (the root of the Min-Heap)
mynode* deletemin(vector<mynode*> & a){
    mynode* temp=a[1]; // The minimum element is at index 1
    a[1]=a[a.size()-1]; // Move the last element to the root
    a.pop_back();       // Remove the last element
    percolatedown(a,1); // Restore the Min-Heap property
    return temp;
}

// Inserts a new node into the Min-Heap
void insert(vector<mynode*> & a, mynode* temp){
    a.push_back(temp);
    percolateup(a,a.size()-1); // Restore the Min-Heap property
}

// Serializes the Huffman Tree into a string using a Newick-like format
string newick(mynode* root){
    if(!root) return "";
    // If it's a leaf node, return the ASCII value of the character as a string
    if(!root->left && !root->right)return to_string((int)(unsigned char)root->c);
    // Otherwise, recursively serialize the left and right children, wrapped in parentheses
    return "(" + newick(root->left) + "," + newick(root->right) + ")";
}

// Recursively traverses the Huffman Tree to generate the binary code for each character
void populate(unordered_map<char,string> & mp,string s,mynode* root){
    if(!root) return;
    // If it's a leaf node, assign the accumulated binary string 's' to the character
    if(!root->left && !root->right){
        mp[root->c]=s;
    }
    else{
        // Traverse left (append '0') and right (append '1')
        populate(mp,s+"0",root->left);
        populate(mp,s+"1",root->right);
    }
}



int main(int argc,char * argv[]){
    // Ensure the user provides the filename as an argument
    if(argc!=2){
        throw runtime_error("usage ./comp [filename]");
    }

    string inpath = argv[1];
    ifstream infile(inpath);
    
    // Extract the original filename (stem) and extension to create the output folder name
    filesystem::path p(inpath);
    string stem=p.stem().string(); 
    string ex=p.extension().string();
    string folderName=stem+"_compressed";

    filesystem::create_directory(folderName);

    streampos start=infile.tellg();

    unordered_map<char,int> mp; // Map to store character frequencies
    vector<mynode *> v(1);      // 1-based index array for the Min-Heap
    string s;

    // Read the file line by line to calculate character frequencies
    while(getline(infile,s)){
        #pragma omp parallel for
        for(auto it:s){
            #pragma omp critical
            {
                mp[it]++;
            }
        }
        mp['\n']++; // Manually count newline characters since getline removes them
    }
    if(mp.empty()){
        throw runtime_error("file is empty");
    }
    // Print frequencies to console for debugging/information
    for(auto [u,w]:mp){
        cout << u << " " << w << endl;
    }
    // Create leaf nodes for every unique character
    for(auto [a,b]:mp){
        v.push_back(new mynode(a,b));
    }

    // Build the initial Min-Heap using the heapify process (O(n) time)
    for(int i=v.size()/2;i>0;i--){
        percolatedown(v,i);
    }

    // Build the Huffman Tree by repeatedly merging the two nodes with the lowest frequencies
    while(v.size()!=2){ // Stops when only the root node and the dummy 0-index element remain
        mynode* n1=deletemin(v); // Lowest frequency node
        mynode* n2=deletemin(v); // Second lowest frequency node
        // Create an internal parent node with frequency equal to the sum of its children
        mynode* temp= new mynode('\0',n1->f+n2->f);
        temp->left=n1;
        temp->right=n2;
        insert(v,temp);
    }

    mynode* root=v[1]; // The final remaining node is the root of the Huffman Tree
    string s1=newick(root); // Serialize the tree to save it
    unordered_map<char,string> codes; // Map to store the generated binary codes
    string s2="";
    // Edge case: if the file only has one unique character
    if(!root->left && !root->right){
        codes[root->c]="0";
    }
    else populate(codes,s2,root); // Generate codes for all characters

    // Write metadata to tree.txt (Total character count, file extension, and the serialized tree)
    string treepath=folderName+"/tree.txt";
    ofstream treeFile(treepath);
    long long charcount=0;
    #pragma omp parallel for
    for(auto [u,v]:mp){
        #pragma omp critical
        {
            charcount+=v;
        }
    }
    treeFile << charcount << endl;
    treeFile << ex << endl;
    treeFile << s1;
    treeFile.close();

    // Prepare to write the compressed binary data
    string datapath=folderName+"/data.bin";
    ofstream data(datapath, ios::binary);
    bitwriter bw(data);

    // Rewind the input file to read it again from the beginning
    infile.clear();
    infile.seekg(0, ios::beg);

    // Read the file again and convert characters to their binary codes
    while(getline(infile, s)){
        for(auto it:s){
            string code=codes[it];
            for(char c:code) bw.writebit(c); // Write bit by bit
        }
        // Manually write the newline code at the end of each line
        if(codes.find('\n')!=codes.end()){
            string newlineCode=codes['\n'];
            for (char c:newlineCode) bw.writebit(c);
        }
    }
    
    bw.flush(); // Ensure any remaining bits in the buffer are written out
    data.close();

    cout << "Compression complete. Folder created: " << folderName << endl;
    cout << "  |- tree.txt" << endl;
    cout << "  |- data.bin" << endl;

    return 0;
    
}