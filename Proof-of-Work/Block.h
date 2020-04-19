#include <string>
#include <vector>

#include "Transaction.h"
#include "Merkletree.h"

#ifndef BLOCK_H
#define BLOCK_H

class Block {

public:

	time_t timestamp;
	unsigned long long int nonce;
	std::string previousHash;
	MerkleTree transactions;
	std::string hash;
	int difficulty;

	Block();
	Block(std::string previousBlockHash, std::vector<Transaction> transactions, int difficulty);

	static bool isValid(std::string hash, int difficulty);
	bool mine();

};

#endif
