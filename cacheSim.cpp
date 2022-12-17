/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <cmath>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;


class block {
public:
    bool validBit;
    bool dirtyBit;
    unsigned int tag;
    unsigned int way;
    block(bool validBit, bool dirtyBit, unsigned int tag, unsigned int way) : validBit(validBit), dirtyBit(dirtyBit), tag(tag), way(way) {};
};

class cache {
public:
    std::list<block>* L1;
    std::list<block>* L2;
    int L1Cyc;
    int L2Cyc;
    int MemCyc;
    int totalTime;
    int L1misses;
    int L1access;
    int L2misses;
    int L2access;

    int BSize;
    int L1Ways;
    int L2Ways;
    int L1BlocksNum;
    int L2BlocksNum;
    int L1SetsNum;
    int L2SetsNum;
    int L1tagSize;
    int L2tagSize;


    cache(int L1Cyc, int L2Cyc, int MemCyc, int BSize, int L1SetsNum, int L1Ways, int L2SetsNum, int L2Ways, int L1BlocksNum, int L2BlocksNum, int L1tagSize, int L2tagSize)
    : L1(nullptr), L2(nullptr), L1Cyc(L1Cyc), L2Cyc(L2Cyc), MemCyc(MemCyc), totalTime(0), L1misses(0), L1access(0), L2misses(0), L2access(0), BSize(BSize), L1Ways(L1Ways), L2Ways(L2Ways),
    L1BlocksNum(L1BlocksNum), L2BlocksNum(L2BlocksNum), L1SetsNum(L1SetsNum), L2SetsNum(L2SetsNum), L1tagSize(L1tagSize), L2tagSize(L2tagSize)
    {
        if(L1SetsNum > 0) L1 = new std::list<block>[L1SetsNum];

        for(int i = 0; i < L1SetsNum; i++) {
            for(int j = 0; j < L1Ways; j++) {
                L1[i].emplace_back(false, false, 0, j);
            }
        }

        if(L1SetsNum > 0) L2 = new std::list<block>[L2SetsNum];

        for(int i = 0; i < L2SetsNum; i++) {
            for(int j = 0; j < L2Ways; j++) {
                L2[i].emplace_back(false, false, 0, j);
            }
        }
    }



    unsigned int getTagFromAddress(long unsigned int address, bool isL1) {
        int tagSize = isL1 ? L1tagSize : L2tagSize;
        return (address >> (32-tagSize));
    }

    unsigned int getSetFromAddress(long unsigned int address, bool isL1) {
        int andByBits = log2(isL1 ? L1SetsNum : L2SetsNum);
        if(andByBits == 0) return 0; // if there is no Set Bit
        int set = address >> (BSize+2);
        int mask = andByBits - 1;
        return (set & mask);
    }

    void addBlock(unsigned int tag, unsigned int set, bool isL1) {

        std::list<block>* blockList = isL1 ? L1 : L2;

        /// if there is any empty space, find the lowest way that is empty
        for (std::list<block>::iterator it = blockList->begin(); it != blockList->end(); it++) {
            if(!it->validBit) {
                cout << "adding block to empty way_" << it->way << endl;
                it->tag = tag;
                it->validBit = true;
                useBlock(tag, set, isL1);
                return;
            }
        }

        /// if there is no empty space, remove the LRU (last block in list) and add it to the front
        std::list<block>::iterator it = --blockList->end();
        cout << "writing block over way_" << it->way << endl;

        if(!isL1) { // if in L2
            removeBlock(it->tag, set, ) // need to remove from L1 based on its L1tag
        }


        it->tag = tag;
        it->validBit = true;
        useBlock(tag, set, isL1);
    }

    bool thereIsEmptySpot(unsigned int tag, unsigned int set, bool isL1) {
        /// if there is any empty space, find the lowest way that is empty
        std::list<block> * blockList = isL1 ? L1 : L2;
        for (std::list<block>::iterator it = blockList->begin(); it != blockList->end(); it++) {
            if(it->tag == -1) {
                return true;
            }
        }
        return false;
    }


    void bringBlockFromMem(unsigned int tagL1, unsigned int setL1, unsigned int tagL2, unsigned int setL2) {
        if(thereIsEmptySpot(tagL2, setL2, false)) { // checking if there is empty space in L2
            // just add
        }
        else {
            // remove least recently used
            // check if it was in L1
            if(isBlockInCache(tagL1, setL1, true)) {
                // remove block from L1
                removeBlock(tagL1, setL1, true);
            }
        }
    }


    void read(long unsigned int address, bool isL1) {

    }

    void write(long unsigned int address, bool writeAlloc) {

        unsigned int tagL1 = getTagFromAddress(address, true);
        unsigned int setL1 = getSetFromAddress(address, true);
        unsigned int tagL2 = getTagFromAddress(address, false);
        unsigned int setL2 = getSetFromAddress(address, false);

        if(writeAlloc) {
            L1access++;
            totalTime += L1Cyc;
            if(isBlockInCacheL1(tagL1, setL1)) { // if hit in L1
                useBlock(tagL1, setL1, true);
            }
            else { // if miss on L1
                L1misses++;
                L2access++;
                totalTime += L2Cyc;
                if(isBlockInCacheL2(tagL2, setL2)) { // if hit in L2
                    useBlock(tagL2, setL2, false);
                }
                else {
                    L2misses++;
                    totalTime += MemCyc;
                    bringBlockFromMem(tagL1, setL1, tagL2, setL2);
                }
            }
        }
        else {

        }
    }

    void useBlock(unsigned int tag, unsigned int set, bool isL1) {

    }

    bool isBlockInCache(unsigned int tag, unsigned int set, bool isL1) {

    }

    void removeBlock(unsigned int tag, unsigned int set, bool isL1) {
        if(!isBlockInCache(tag, set, isL1)) return;

    }


};


int main(int argc, char **argv) {

    if (argc < 19) {
        cerr << "Not enough arguments" << endl;
        return 0;
    }

    // Get input arguments

    // File
    // Assuming it is the first argument
    char* fileString = argv[1];
    ifstream file(fileString); //input file stream
    string line;
    if (!file || !file.good()) {
        // File doesn't exist or some other error
        cerr << "File not found" << endl;
        return 0;
    }

    unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
            L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

    for (int i = 2; i < 19; i += 2) {
        string s(argv[i]);
        if (s == "--mem-cyc") {
            MemCyc = atoi(argv[i + 1]);
        } else if (s == "--bsize") {
            BSize = atoi(argv[i + 1]);
        } else if (s == "--l1-size") {
            L1Size = atoi(argv[i + 1]);
        } else if (s == "--l2-size") {
            L2Size = atoi(argv[i + 1]);
        } else if (s == "--l1-cyc") {
            L1Cyc = atoi(argv[i + 1]);
        } else if (s == "--l2-cyc") {
            L2Cyc = atoi(argv[i + 1]);
        } else if (s == "--l1-assoc") {
            L1Assoc = atoi(argv[i + 1]);
        } else if (s == "--l2-assoc") {
            L2Assoc = atoi(argv[i + 1]);
        } else if (s == "--wr-alloc") {
            WrAlloc = atoi(argv[i + 1]);
        } else {
            cerr << "Error in arguments" << endl;
            return 0;
        }
    }

    int L1Ways = pow(2, L1Assoc); // assoc = log2 (# of ways)
    int L2Ways = pow(2, L2Assoc); // assoc = log2 (# of ways)
    // sets = blocks / ways
    // blocks = cache_size / block_size
    int L1BlocksNum = pow(2, L1Size) / pow(2, BSize);
    int L2BlocksNum = pow(2, L2Size) / pow(2, BSize);
    int L1SetsNum = L1BlocksNum / L1Ways;
    int L2SetsNum = L2BlocksNum / L2Ways;

    int L1tagSize = 32 - (log2(L1SetsNum) + BSize + 2);
    int L2tagSize = 32 - (log2(L2SetsNum) + BSize + 2);

    cout << "block offset size: " << BSize+2 << ", L1 set size: " << log2(L1SetsNum) << ", L2 set size: " << log2(L2SetsNum) << ", L1 tag size:" << L1tagSize << ", L2 tag size:" << L2tagSize << endl;

    cout << "L1Ways: " << L1Ways << ", L2Ways: " << L2Ways << ", L2SetsNum: " << L1SetsNum << ", L2SetsNum: " << L2SetsNum << endl;



    while (getline(file, line)) {

        stringstream ss(line);
        string address;
        char operation = 0; // read (R) or write (W)
        if (!(ss >> operation >> address)) {
            // Operation appears in an Invalid format
            cout << "Command Format error" << endl;
            return 0;
        }

        // DEBUG - remove this line
        cout << "operation: " << operation;

        string cutAddress = address.substr(2); // Removing the "0x" part of the address

        // DEBUG - remove this line
        cout << ", address (hex)" << cutAddress;

        unsigned long int num = 0;
        num = strtoul(cutAddress.c_str(), NULL, 16);

        // DEBUG - remove this line
        cout << " (dec) " << num << endl;

    }

    double L1MissRate;
    double L2MissRate;
    double avgAccTime;

    printf("L1miss=%.03f ", L1MissRate);
    printf("L2miss=%.03f ", L2MissRate);
    printf("AccTimeAvg=%.03f\n", avgAccTime);

    return 0;
}
