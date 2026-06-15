#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <stdexcept>
using namespace std;

// Class to read a binary file bit by bit
class bitreader{
    public:
    unsigned char bits; // Stores the current byte being processed
    int num;            // Tracks how many bits are left unread in the current byte
    ifstream & infile;  // The input stream to read bytes from
    
    bitreader(ifstream& i):infile(i),bits(0),num(0){}
    
    // Reads and returns a single bit ('0' or '1') from the file
    char readbit(){
        // If all bits in the current byte are used up, read the next byte
        if(num==0){
            char c;
            infile.get(c);
            bits=(unsigned) c;
            num=8; // Reset the bit counter
        }
        num--;
        // Extract the specific bit at the current position using bitwise operations
        int bit=((bits >> num)& 1);
        if(bit==1) return '1';
        else return '0';
    }
};

// Helper function to find the top-level comma separating left and right subtrees in the Newick string
int position(const string& s){
    int check=0; // Tracks parenthesis nesting level
    for (int i=0;i<s.length();i++) {
        if(s[i]=='(') check++;
        else if(s[i] == ')') check--;
        // Return the index of the comma that splits the main two subtrees
        else if (s[i]==',' && check == 1) return i; 
    }
    return -1;
}

// Recursively rebuilds the binary code dictionary by parsing the serialized Newick tree string
void decoder(unordered_map<string,char> & mp,string s,string code){
    // Base case: no parentheses mean it's a leaf node. Convert the string to a character.
    if(s.find('(')==string::npos){
        mp[code]=(char)stoi(s); // stoi converts the ASCII number string back to an integer/char
    }
    else{
        // Recursive case: find the middle comma, split into left and right, and traverse deeper
        int comma=position(s);
        if(comma==-1) return;
        string s1=s.substr(1,comma-1); // Left subtree string
        string s2=s.substr(comma+1);   // Right subtree string
        s2.pop_back(); // Remove the closing parenthesis at the end of the right subtree
        
        // Traverse left (append '0') and right (append '1')
        decoder(mp,s1,code+'0');
        decoder(mp,s2,code+'1');
    }
}

int main(int argc, char* argv[]){

    // Ensure the user provides the compressed folder name as an argument
    if (argc != 2) {
        throw runtime_error("Usage: ./decompress [folder_name]");
    }

    string foldername=argv[1];
    string treestring=foldername+"/tree.txt";
    string data=foldername+"/data.bin";

    // Remove trailing slash if the user added one to the folder name
    if(foldername.back() == '/' || foldername.back() == '\\') {
        foldername.pop_back();
    }
    
    ifstream treefile(treestring);
    ifstream infile(data,ios::binary);
    string s;
    long long total;
    
    // Read the metadata from tree.txt
    treefile >> total;    // Total number of characters to decode
    getline(treefile,s);  // Consume the newline character after the number
    getline(treefile,s);  // Read the original file extension

    // Determine the name of the output restored file
    string outName;
    size_t pos=foldername.find("_compressed");
    if(pos!=string::npos){
        // Replace "_compressed" with "_restored" and append the original extension
        outName=foldername.substr(0,pos)+"_restored"+s; 
    }
    ofstream ofile(outName, ios::binary);
    
    // Read the serialized Newick tree string
    getline(treefile,s);

    // Rebuild the code-to-character mapping dictionary
    unordered_map<string,char> mp;
    // Edge case: handle a tree with only one distinct character
    if(s.find('(')==string::npos){
        mp["0"]=(char)stoi(s);
    }
    else decoder(mp,s,"");

    bitreader B(infile);
    string currbits="";
    long long decoded=0;
    
    // Read bits and decode characters until the total original character count is reached
    // (This prevents interpreting padded trailing zeros as actual data)
    while(decoded<total){
        currbits+=B.readbit(); // Read one bit at a time
        // If the accumulated bits match a valid Huffman code in our map
        if(mp.find(currbits)!=mp.end()){
            ofile << mp[currbits]; // Write the corresponding character to the output file
            decoded++;             // Increment decoded character count
            currbits="";           // Reset bit accumulator for the next character
        }
    }
    cout << "Decoded" << endl;
}