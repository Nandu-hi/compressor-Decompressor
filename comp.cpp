#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <stdexcept>
using namespace std;


class bitwriter{
    unsigned char buffer = 0;
    int bitCount = 0;
    ostream& ofile;

    public:
    
    bitwriter(ostream& o):ofile(o){}

    void writebit(char bit){
        buffer=buffer << 1;
        if(bit=='1'){
            buffer=buffer | 1;
        }
        bitCount++;
        if(bitCount==8){
            ofile.put(buffer);
            buffer=0;
            bitCount=0;
        }
    }

    void flush(){
        if(bitCount>0){
            buffer=buffer << (8-bitCount);
            ofile.put(buffer);
        }
    }
};


class mynode{
    public:
    char c;
    int f;
    mynode * left;
    mynode* right;
    mynode(char a,int b):c(a),f(b),left(NULL),right(NULL){}
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

void swap(mynode* & a,mynode* & b){
    mynode* temp=a;
    a=b;
    b=temp;
}

void percolatedown(vector<mynode*> & a,int i){
    while(2*i<a.size()){
        int minindex=2*i;
        if(2*i+1 < a.size() && *(a[2*i+1])<*(a[2*i])){
            minindex=2*i+1;
        }
        if(*a[i]>*a[minindex]){
            swap(a[i],a[minindex]);
            i=minindex;
        }
        else break;
        
    }
}

void percolateup(vector<mynode*> & a,int i){
    while(i>1 && *a[i/2]>*a[i]){
        swap(a[i/2],a[i]);
        i=i/2;
    }
}

mynode* deletemin(vector<mynode*> & a){
    mynode* temp=a[1];
    a[1]=a[a.size()-1];
    a.pop_back();
    percolatedown(a,1);
    return temp;
}

void insert(vector<mynode*> & a, mynode* temp){
    a.push_back(temp);
    percolateup(a,a.size()-1);
}

string newick(mynode* root){
    if(!root) return "";
    if(!root->left && !root->right)return to_string((int)(unsigned char)root->c);
    return "(" + newick(root->left) + "," + newick(root->right) + ")";
}

void populate(unordered_map<char,string> & mp,string s,mynode* root){
    if(!root) return;
    if(!root->left && !root->right){
        mp[root->c]=s;
    }
    else{
        populate(mp,s+"0",root->left);
        populate(mp,s+"1",root->right);
    }
}



int main(int argc,char * argv[]){
    if(argc!=2){
        throw runtime_error("usage ./comp [filename]");
    }

    string inpath = argv[1];
    ifstream infile(inpath);
    
    filesystem::path p(inpath);
    string stem=p.stem().string(); 
    string ex=p.extension().string();
    string folderName=stem+"_compressed";

    filesystem::create_directory(folderName);

    streampos start=infile.tellg();

    unordered_map<char,int> mp;
    vector<mynode *> v(1);
    string s;

    while(getline(infile,s)){
        for(auto it:s){
            mp[it]++;
        }
        mp['\n']++;
    }
    if(mp.empty()){
        throw runtime_error("file is empty");
    }
    for(auto [u,w]:mp){
        cout << u << " " << w << endl;
    }
    for(auto [a,b]:mp){
        v.push_back(new mynode(a,b));
    }

    for(int i=v.size()/2;i>0;i--){
        percolatedown(v,i);
    }

    while(v.size()!=2){
        mynode* n1=deletemin(v);
        mynode* n2=deletemin(v);
        mynode* temp= new mynode('\0',n1->f+n2->f);
        temp->left=n1;
        temp->right=n2;
        insert(v,temp);
    }

    mynode* root=v[1];
    string s1=newick(root);
    unordered_map<char,string> codes;
    string s2="";
    if(!root->left && !root->right){
        codes[root->c]="0";
    }
    else populate(codes,s2,root);

    string treepath=folderName+"/tree.txt";
    ofstream treeFile(treepath);
    long long charcount=0;
    for(auto [u,v]:mp){
        charcount+=v;
    }
    treeFile << charcount << endl;
    treeFile << ex << endl;
    treeFile << s1;
    treeFile.close();

    string datapath=folderName+"/data.bin";
    ofstream data(datapath, ios::binary);
    bitwriter bw(data);

    infile.clear();
    infile.seekg(0, ios::beg);

    while(getline(infile, s)){
        for(auto it:s){
            string code=codes[it];
            for(char c:code) bw.writebit(c);
        }
        if(codes.find('\n')!=codes.end()){
            string newlineCode=codes['\n'];
            for (char c:newlineCode) bw.writebit(c);
        }
    }
    
    bw.flush();
    data.close();

    cout << "Compression complete. Folder created: " << folderName << endl;
    cout << "  |- tree.txt" << endl;
    cout << "  |- data.bin" << endl;

    return 0;
    
}