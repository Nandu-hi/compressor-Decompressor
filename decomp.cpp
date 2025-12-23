#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <stdexcept>
using namespace std;

class bitreader{
    public:
    unsigned char bits;
    int num;
    ifstream & infile;
    bitreader(ifstream& i):infile(i),bits(0),num(0){}
    char readbit(){
        if(num==0){
            char c;
            infile.get(c);
            bits=(unsigned) c;
            num=8;
        }
        num--;
        int bit=((bits >> num)& 1);
        if(bit==1) return '1';
        else return '0';
    }
};

int position(const string& s){
    int check=0;
    for (int i=0;i<s.length();i++) {
        if(s[i]=='(') check++;
        else if(s[i] == ')') check--;
        else if (s[i]==',' && check == 1) return i;
    }
    return -1;
}

void decoder(unordered_map<string,char> & mp,string s,string code){
    if(s.find('(')==string::npos){
        mp[code]=(char)stoi(s);
    }
    else{
        int comma=position(s);
        if(comma==-1) return;
        string s1=s.substr(1,comma-1);
        string s2=s.substr(comma+1);
        s2.pop_back();
        decoder(mp,s1,code+'0');
        decoder(mp,s2,code+'1');
    }
}

int main(int argc, char* argv[]){

    if (argc != 2) {
        throw runtime_error("Usage: ./decompress [folder_name]");
    }

    string foldername=argv[1];
    string treestring=foldername+"/tree.txt";
    string data=foldername+"/data.bin";

    if(foldername.back() == '/' || foldername.back() == '\\') {
        foldername.pop_back();
    }
    
    ifstream treefile(treestring);
    ifstream infile(data,ios::binary);
    string s;
    long long total;
    treefile >> total;
    getline(treefile,s);
    getline(treefile,s);

    string outName;
    size_t pos=foldername.find("_compressed");
    if(pos!=string::npos){
        outName=foldername.substr(0,pos)+"_restored"+s;
    }
    ofstream ofile(outName, ios::binary);
    getline(treefile,s);

    unordered_map<string,char> mp;
    if(s.find('(')==string::npos){
        mp["0"]=(char)stoi(s);
    }
    else decoder(mp,s,"");

    bitreader B(infile);
    string currbits="";
    long long decoded=0;
    while(decoded<total){
        currbits+=B.readbit();
        if(mp.find(currbits)!=mp.end()){
            ofile << mp[currbits];
            decoded++;
            currbits="";
        }
    }
    cout << "Decoded" << endl;
}