/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <cmath>
#include <bitset>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;


class cache {
public:
    static float MemAccess; // rise by one on every memory access (when both L1 & L2 are miss)
    static int totalAccessCounter; // rise by one on every access
    static double runTime;
};

class L1 {
public:
    static float miss; // rise by one on every L1 miss
    static float access; // rise by one on every L1 access
    static std::list<int> leastRecentlyUsed; // LRU block is the first block in the linked list
};

class L2 {
public:
    static float miss; // rise by one on every L2 miss
    static float access; // rise by one on every L2 access
    static std::list<int> leastRecentlyUsed; // LRU block is the first block in the linked list
};

class block {
public:
    bool validBit;
    bool dirtyBit;
    unsigned int tag;
    unsigned int way;
    block(bool validBit, bool dirtyBit, unsigned int tag, unsigned int way) : validBit(validBit), dirtyBit(dirtyBit), tag(tag), way(way) {};
};

float cache::MemAccess = 0;
int cache::totalAccessCounter = 0;
double cache::runTime = 0;
float L1::miss = 0;
float L1::access = 0;
float L2::miss = 0;
float L2::access = 0;



unsigned int getTagFromAddress(long unsigned int address, int tagSize) {
    return (address >> (32-tagSize));
}

unsigned int getSetFromAddress(long unsigned int address, int shiftByBits, int andByBits) {
    if(andByBits == 0) return 0; // if there is no Set Bit
    int set = address >> shiftByBits;
    int mask = andByBits - 1;
    return (set & mask);
}

bool isBlockInCache(int tag, std::list<block> blockList) {
    for (auto block : blockList) {
        if(tag == block.tag) {
            return true;
        }
    }
    return false;
}

void usedThisBlock(int tag, std::list<block>* blockList) { // put the block that we read\wrote to at the begining
                                                             // of the LRU list
    for (std::list<block>::iterator it = blockList->begin(); it != blockList->end(); it++) {
        if(it->tag == tag) { // found the block we wanted
            cout << "adding block to empty way_" << it->way << endl;
            unsigned int way = it->way;
            bool dirtyFlag = it->dirtyBit;
            blockList->erase(it);
            blockList->emplace_front(true, dirtyFlag, tag, way);
        }
    }

}

bool addBlockLRU(int tag, std::list<block>* blockList) { // returns whether or not a dirtybit was DESTROYED

    /// if there is any empty space, find the lowest way that is empty
    for (std::list<block>::iterator it = blockList->begin(); it != blockList->end(); it++) {
        if(it->tag == -1) {
            cout << "adding block to empty way_" << it->way << endl;
            unsigned int way = it->way;
            blockList->erase(it);
            blockList->emplace_front(true, false, tag, way);
            return false;
        }
    }

    /// if there is no empty space, remove the LRU (last block in list) and add it to the front
    std::list<block>::iterator it = --blockList->end();
    unsigned int way = it->way;

    if(it->dirtyBit) {
        return true;
    }

    cout << "writing block over way_" << it->way << endl;
    blockList->erase(it);
    blockList->emplace_front(true, false, tag, way);
    return false;
}

void setDirtyBit(int tag, std::list<block>* blockList, bool flag) {

    /// if there is any empty space, find the lowest way that is empty
    for (std::list<block>::iterator it = blockList->begin(); it != blockList->end(); it++) {
        if(it->tag == tag) {
            cout << "set dirtybit at way_" << it->way << " to " << flag << endl;
            it->dirtyBit = flag;
            return;
        }
    }
}


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

    ///////////////////////////////////////////////////////
    /// INITIATING BLOCKS INCLUDING LEAST RECENTLY USED ///
    ///////////////////////////////////////////////////////

    std::list<block>* L1LRU = nullptr;
    if(L1SetsNum > 0) L1LRU = new std::list<block>[L1SetsNum];

    for(int i = 0; i < L1SetsNum; i++) {
        for(int j = 0; j < L1Ways; j++) {
            L1LRU[i].emplace_back(false, false, -1, j);
        }
    }

    std::list<block>* L2LRU = nullptr;
    if(L1SetsNum > 0) L2LRU = new std::list<block>[L2SetsNum];

    for(int i = 0; i < L2SetsNum; i++) {
        for(int j = 0; j < L2Ways; j++) {
            L2LRU[i].emplace_back(false, false, -1, j);
        }
    }


    ///////////////////////////////////////////////////////
    ////////// PROCESSING EACH LINE OF TRACE LOG //////////
    ///////////////////////////////////////////////////////

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
		cout << " (dec) " << num << endl ;

        // DEBUG - remove this line
        std::bitset<32> x(num);
        cout << " (bin) "<< x << endl ;

        // do it add our code here
        cache::totalAccessCounter++;

        /// check if the block is in L1
        unsigned int L1tag = getTagFromAddress(num, L1tagSize);
        unsigned int L1set = getSetFromAddress(num, BSize+2, log2(L1SetsNum));

        cout << "tag is " << L1tag << " and set is " << L1set << endl;
        bool hit_L1 = isBlockInCache(L1tag, L1LRU[L1set]);
        L1::access++;
        cache::runTime += L1Cyc;
        if(!hit_L1) {
            L1::miss++;
            cache::runTime += L2Cyc;
            unsigned int L2tag = getTagFromAddress(num, L2tagSize);
            unsigned int L2set = getSetFromAddress(num, BSize+2, log2(L2SetsNum));
            cout << "tag is " << L2tag << " and set is " << L2set << endl;

            L2::access++;
            bool hit_L2 = isBlockInCache(L2tag, L2LRU[L2set]);
            if(!hit_L2) {
                L2::miss++;
                cache::MemAccess++;
                cache::runTime += MemCyc;

                /// add block to L2
                if(operation == 'r' || WrAlloc == 1) {
                    bool dirtyBitChanged = addBlockLRU(L2tag, &L2LRU[L2set]);
                    if(dirtyBitChanged) cache::runTime += MemCyc;
                }
            }
            else { // if hit L2
                if(operation == 'w') {
                    setDirtyBit(L2tag, &L2LRU[L2set], true);
                }
                usedThisBlock(L2tag,&L2LRU[L2set]);

            }

            /// add block to L1
            if(operation == 'r' || WrAlloc == 1) {
                bool dirtyBitChanged = addBlockLRU(L1tag, &L1LRU[L1set]);
                if(dirtyBitChanged) cache::runTime += L2Cyc;
            }
        }
        else { // if hit L1
            if(operation == 'w') {
                setDirtyBit(L1tag, &L1LRU[L1set], true);
            }
            usedThisBlock(L1tag,&L1LRU[L1set]);
        }


        // DEBUG STUFF
        cout << "L1:" << endl;
        for(int i = 0; i < L1SetsNum; i++){
            for (std::list<block>::iterator it = L1LRU[i].begin(); it != L1LRU[i].end(); it++) {
                cout << it->tag << " " ;
            }
            cout << endl;
        }
        cout << endl;

        cout << "L2:" << endl;
        for(int i = 0; i < L2SetsNum; i++){
            for (std::list<block>::iterator it = L2LRU[i].begin(); it != L2LRU[i].end(); it++) {
                cout << it->tag << " " ;
            }
            cout << endl;
        }
        cout << endl;

    }

	double L1MissRate = L1::miss/L1::access;
	double L2MissRate = L2::miss/L2::access;
	//double avgAccTime = (L1::access * L1Cyc)  + (L2::access * L2Cyc) + (cache::MemAccess * MemCyc);
    double avgAccTime = cache::runTime / cache::totalAccessCounter;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
