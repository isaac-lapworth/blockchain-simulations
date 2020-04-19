#include <string>
#include <vector>

#include "Transaction.h"

#ifndef MERKLETREE_H
#define MERKLETREE_H

class MerkleTree {

public:

	// stores the ids of input transactions
	std::vector<unsigned> ids;

	// stores the hashes of each node of the tree
	std::vector<std::string> hashes;

	MerkleTree(std::vector<Transaction> transactions);

	std::string getMerkleRoot();

};

#endif
