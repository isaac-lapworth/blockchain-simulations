#include <string>
#include <vector>

#include "Transaction.h"
#include "Merkletree.h"

#ifndef BLOCK_H
#define BLOCK_H

class Block {

public:

	time_t timestamp;
	std::string previousHash;
	MerkleTree transactions;
	std::string hash;

	Block();
	Block(std::string previousBlockHash, std::vector<Transaction> transactions);

};

#endif
