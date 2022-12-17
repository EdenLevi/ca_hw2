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
    unsigned int address;
    unsigned int tag;
    unsigned int way;
    block(bool validBit, bool dirtyBit, unsigned int address, unsigned int tag, unsigned int way) : validBit(validBit), dirtyBit(dirtyBit), address(address), tag(tag), way(way) {};
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
                L1[i].emplace_back(false, false, 0, 0, j);
            }
        }

        if(L2SetsNum > 0) L2 = new std::list<block>[L2SetsNum];

        for(int i = 0; i < L2SetsNum; i++) {
            for(int j = 0; j < L2Ways; j++) {
                L2[i].emplace_back(false, false, 0, 0, j);
            }
        }
    }



    unsigned int getTagFromAddress(long unsigned int address, bool isL1) {
        int tagSize = isL1 ? L1tagSize : L2tagSize;
        /*(int tag = (address >> BSize) >> (pow(2, BSize) - );

        int offset_size_bits = BSize; // BlockSizeBits
        int Size, Assoc, BlockSizeBits;
        if(isL1) {
            Size =
        }
        int set_field_size_bits = Size - Assoc - BlockSizeBits;

        int tag = (address >> offset_size_bits) >> set_field_size_bits;*/
        cout << tagSize << endl;
        return (address >> (32-tagSize));
    }

    unsigned int getSetFromAddress(long unsigned int address, bool isL1) {

        cout << "I am trying to get set from address: " << address << endl;
        int set = address >> BSize;
        int setBits = 32 - ((isL1 ? L1tagSize : L2tagSize) + BSize);
        int mask = pow(2, setBits) - 1;
        set = set && mask;
        cout << "The set I got is: " << set << endl;
        return set;


        /*int andByBits = log2(isL1 ? L1SetsNum : L2SetsNum);
        if(andByBits == 0) return 0; // if there is no Set Bit
        int set = address >> (BSize);
        int mask = andByBits - 1;
        return (set & mask);*/



        //int bits = isL1 ? log2(L1SetsNum) : log2(L2SetsNum);
        //return ((address >> BSize) % (1 << bits));

    }

    void addBlock(unsigned int address, unsigned int tag, unsigned int set, bool isL1) {

        std::list<block>* blockList = isL1 ? L1 : L2;

        /// if there is any empty space, find the lowest way that is empty
        for (std::list<block>::iterator it = blockList[set].begin(); it != blockList[set].end(); it++) {
            if(!it->validBit) {
                cout << "adding block to empty way_" << it->way << " in " << (isL1 ? "L1" : "L2") << endl;
                it->address = address;
                it->tag = tag;
                it->validBit = true;
                useBlock(tag, set, isL1);
                return;
            }
        }

        /// if there is no empty space, remove the LRU (last block in list) and add it to the front
        std::list<block>::iterator it = --(blockList[set].end());
        cout << "writing block over way_" << it->way << endl;

        if(!isL1) { // if we are removing from L2, make sure block is removed from L1 (if exists there)
            removeBlock(getTagFromAddress(it->address, true), getSetFromAddress(it->address, true), true);
        }

        it->address = address;
        it->tag = tag;
        it->validBit = true;
        useBlock(tag, set, isL1);
    }



    void read(long unsigned int address) {

        unsigned int tagL1 = getTagFromAddress(address, true);
        unsigned int setL1 = getSetFromAddress(address, true);
        unsigned int tagL2 = getTagFromAddress(address, false);
        unsigned int setL2 = getSetFromAddress(address, false);

        cout << "tagL1 " << tagL1 << endl;
        cout << "setL1 " << setL1 << endl;

        L1access++;
        totalTime += L1Cyc;
        if(isBlockInCache(tagL1, setL1, true)) { // if hit in L1
            cout << "hit L1" << endl;
            useBlock(tagL1, setL1, true);
        }
        else { // if miss on L1
            cout << "miss L1" << endl;
            L1misses++;
            L2access++;
            totalTime += L2Cyc;

            cout << "tagL2 " << tagL2 << endl;
            cout << "setL2 " << setL2 << endl;

            if(isBlockInCache(tagL2, setL2, false)) { // if hit in L2

                cout << "hit L2" << endl;

                useBlock(tagL2, setL2, false);
                addBlock(address, tagL1, setL1, true); // bring to L1 also
                /// maybe need another L1 cycle? for bringing from L2 to L1
            }
            else { // if miss on L2
                cout << "miss L2" << endl;
                L2misses++;
                totalTime += MemCyc;
                addBlock(address, tagL2, setL2, false); // bring block from mem to L2
                addBlock(address, tagL1, setL1, true); // bring block from mem to L1
            }
        }

    }

    void write(long unsigned int address, bool writeAlloc) {

        unsigned int tagL1 = getTagFromAddress(address, true);
        unsigned int setL1 = getSetFromAddress(address, true);
        unsigned int tagL2 = getTagFromAddress(address, false);
        unsigned int setL2 = getSetFromAddress(address, false);

        cout << "tagL1 " << tagL1 << endl;
        cout << "setL1 " << setL1 << endl;

        L1access++;
        totalTime += L1Cyc;
        if(isBlockInCache(tagL1, setL1, true)) { // if hit in L1
            useBlock(tagL1, setL1, true);
            cout << "hit L1" << endl;
        }
        else { // if miss on L1

            cout << "miss L1" << endl;

            L1misses++;
            L2access++;
            totalTime += L2Cyc;
            if(isBlockInCache(tagL2, setL2, false)) { // if hit in L2
                cout << "hit L2" << endl;

                useBlock(tagL2, setL2, false);
                if(writeAlloc) addBlock(address, tagL1, setL1, true); // bring to L1 also
            }
            else { // if miss on L2
                cout << "miss L2" << endl;

                L2misses++;
                totalTime += MemCyc;
                if(writeAlloc) addBlock(address, tagL2, setL2, false); // bring block from mem to L2
                if(writeAlloc) addBlock(address, tagL1, setL1, true); // bring block from mem to L1
            }
        }
    }

    void useBlock(unsigned int tag, unsigned int set, bool isL1) {

        std::list<block>* blockList = isL1 ? L1 : L2;

        for (std::list<block>::iterator it = blockList[set].begin(); it != blockList[set].end(); it++) {
            if(it->validBit && it->tag == tag) { // found the block we wanted
                unsigned int way = it->way;
                unsigned long int address = it->address;
                blockList[set].erase(it);
                blockList[set].emplace_front(true, false, address, tag, way);
                return;
            }
        }
    }

    bool isBlockInCache(unsigned int tag, unsigned int set, bool isL1) {
        std::list<block>* blockList = isL1 ? L1 : L2;
        for (std::list<block>::iterator it = blockList[set].begin(); it != blockList[set].end(); it++) {
            if(it->validBit && tag == it->tag) {
                return true;
            }
        }
        return false;
    }

    void removeBlock(unsigned int tag, unsigned int set, bool isL1) {
        if(!isBlockInCache(tag, set, isL1)) return;
        std::list<block>* blockList = isL1 ? L1 : L2;

        for (std::list<block>::iterator it = blockList[set].begin(); it != blockList[set].end(); it++) {
            if(it->validBit && it->tag == tag) { // found the block we wanted
                it->tag = 0;
                it->address = 0;
                it->validBit = false;
                return;
            }
        }

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

    cout << "we are here" << endl;

    int L1Ways = pow(2, L1Assoc); // assoc = log2 (# of ways)
    int L2Ways = pow(2, L2Assoc); // assoc = log2 (# of ways)
    // sets = blocks / ways
    // blocks = cache_size / block_size
    cout << "got " << L1Ways << " L1Ways" << endl;
    cout << "got " << L2Ways << " L2Ways" << endl;

    int L1BlocksNum = pow(2, L1Size) / pow(2, BSize);
    int L2BlocksNum = pow(2, L2Size) / pow(2, BSize);
    cout << "got " << L1BlocksNum << " L1BlocksNum" << endl;
    cout << "got " << L2BlocksNum << " L2BlocksNum" << endl;

    int L1SetsNum = L1BlocksNum / L1Ways;
    int L2SetsNum = L2BlocksNum / L2Ways;
    cout << "got " << L1SetsNum << " L1SetsNum" << endl;
    cout << "got " << L2SetsNum << " L2SetsNum" << endl;

   // int L1SetsNum = L1Size - L1Assoc - BSize;
   // int L2SetsNum = L2Size - L2Assoc - BSize;

    int L1tagSize = 32 - ((log2(L1SetsNum) + BSize)) ;
    int L2tagSize = 32 - ((log2(L2SetsNum) + BSize)) ;
    cout << "got " << L1tagSize << " L1tagSize" << endl;
    cout << "got " << L2tagSize << " L2tagSize" << endl;
    //int L1tagSize = 32 - BSize - L1SetsNum;
    //int L2tagSize = 32 - BSize - L2SetsNum;

    //int numOfBlocksL1_inBits = L1Size - BSize;
    //int numOfSetsL1_inBits = numOfBlocksL1_inBits - L1Assoc;
    //int numOfBlocksL2_inBits = L2Size - BSize;
    //int numOfSetsL2_inBits = numOfBlocksL2_inBits - L1Assoc;

   // L1tagSize = 32 - numOfSetsL1_inBits - BSize;
   // L2tagSize = 32 - numOfSetsL2_inBits - BSize;


    cout << "block offset size: " << BSize << ", L1 set size: " << log2(L1SetsNum) << ", L2 set size: " << log2(L2SetsNum) << ", L1 tag size:" << L1tagSize << ", L2 tag size:" << L2tagSize << endl;

    cout << "L1Ways: " << L1Ways << ", L2Ways: " << L2Ways << ", L2SetsNum: " << L1SetsNum << ", L2SetsNum: " << L2SetsNum << endl;

    cache Cache(L1Cyc, L2Cyc, MemCyc, BSize, L1SetsNum, L1Ways, L2SetsNum, L2Ways, L1BlocksNum, L2BlocksNum, L1tagSize, L2tagSize);

    int cmdCounter = 0;
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

        if(operation == 'r') Cache.read(num);
        else if(operation == 'w') Cache.write(num, WrAlloc);
        cmdCounter++;


        // DEBUG STUFF
        cout << "L1:" << endl;
        for(int i = 0; i < L1SetsNum; i++){
            for (std::list<block>::iterator it = Cache.L1[i].begin(); it != Cache.L1[i].end(); it++) {
                if(it->validBit){
                    cout << it->tag << " " ;
                }
                else{
                    cout << "INVALID" << " " ;
                }
            }
            cout << endl;
        }
        cout << endl;

        cout << "L2:" << endl;
        for(int i = 0; i < L2SetsNum; i++){
            for (std::list<block>::iterator it = Cache.L2[i].begin(); it != Cache.L2[i].end(); it++) {
                if(it->validBit){
                    cout << it->tag << " " ;
                }
                else{
                    cout << "INVALID" << " " ;
                }
            }
            cout << endl;
        }
        cout << endl;

    }

    double L1MissRate = (double)Cache.L1misses/Cache.L1access;
    double L2MissRate = (double)Cache.L2misses/Cache.L2access;
    double avgAccTime = (double)Cache.totalTime/cmdCounter;

    printf("L1miss=%.03f ", L1MissRate);
    printf("L2miss=%.03f ", L2MissRate);
    printf("AccTimeAvg=%.03f\n", avgAccTime);

    return 0;
}
