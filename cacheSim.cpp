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

void addBlockLRU(int tag, std::list<block> blockList) {

    for (std::list<block>::reverse_iterator it = blockList.rbegin(); it != blockList.rend(); it++){
        cout << "way_" << it->way << endl;
        if(it->tag == -1) {
            unsigned int way = it->way;
            blockList.erase(it.base());
            blockList.emplace_front(0, 0, -1, way);
        }
    }
    for (std::list<block>::reverse_iterator it = blockList.rbegin(); it != blockList.rend(); it++) {
        cout << "way_" << it->way << endl;
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

    int L1tagSize = 32 - (log2(L1SetsNum) + BSize*8 + 2);
    int L2tagSize = 32 - (log2(L2SetsNum) + BSize*8 + 2);

    cout << "L1Ways: " << L1Ways << ", L2Ways: " << L2Ways << ", L2SetsNum: " << L1SetsNum << ", L2SetsNum: " << L2SetsNum << endl;

    ///////////////////////////////////////////////////////
    /// INITIATING BLOCKS INCLUDING LEAST RECENTLY USED ///
    ///////////////////////////////////////////////////////

    auto L1LRU = new std::list<block>[L1SetsNum];
    for(int i = 0; i < L1SetsNum; i++) {
        for(int j = 0; j < L1Ways; j++) {
            L1LRU[i].emplace_back(0, 0, -1, j);
        }
    }
    auto L2LRU = new std::list<block>[L2SetsNum];
    for(int i = 0; i < L2SetsNum; i++) {
        for(int j = 0; j < L2Ways; j++) {
            L2LRU[i].emplace_back(0, 0, -1, j);
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

        /// check if the block is in L1
        unsigned int L1tag = getTagFromAddress(num, L1tagSize);
        unsigned int L1set = getSetFromAddress(num, BSize*8+2, log2(L1SetsNum));

        cout << "tag is " << L1tag << " and set is " << L1set << endl;
        bool hit_L1 = isBlockInCache(L1tag, L1LRU[L1set]);
        L1::access++;
        if(!hit_L1) {
            L1::miss++;
            unsigned int L2tag = getTagFromAddress(num, L2tagSize);
            unsigned int L2set = getSetFromAddress(num, BSize*8+2, log2(L2SetsNum));
            cout << "tag is " << L2tag << " and set is " << L2set << endl;

            L2::access++;
            bool hit_L2 = isBlockInCache(L2tag, L2LRU[L2set]);
            if(!hit_L2) {
                L2::miss++;
                cache::MemAccess++;
                /// add block to L2
                //L2LRU[L2set].
            }
            addBlockLRU(0, L1LRU[L1set]);
            /// add block to L1
        }

	}

	double L1MissRate = L1::miss/L1::access;
	double L2MissRate = L2::miss/L2::access;
	double avgAccTime = (L1::access * L1Cyc)  + (L2::access * L2Cyc) + (cache::MemAccess * MemCyc);
    avgAccTime = avgAccTime / cache::totalAccessCounter;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
