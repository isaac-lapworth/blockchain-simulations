// Merkle trees are a "hash-tree" data structure used to store transactions in a block, with the Merkle root value used in mining.
#include <vector>
#include <string>

#include "MerkleTree.h"
#include "Transaction.h"

// the tree is stored as a vector, with the children of index n at 2n and 2n+1
// it is built from the leaves to the root by combining the roots of sub-trees by concatenating the hashes of their values
MerkleTree::MerkleTree(std::vector<Transaction> transactions) {

	// store the transaction ids 
	for (Transaction t : transactions) {
		ids.push_back(t.id);
	}

	// as binary trees, Merkle trees need an even number of leaves -> we duplicate the last element if this is the case
	if (transactions.size() % 2 == 1) transactions.push_back(transactions.back());

	// a library hash function is used here as it is faster than SHA-256
	std::hash<std::string> hash;

	// initially all transactions are hashed to form single-node trees - the leaves of the final tree
	for (Transaction t : transactions) {
		hashes.push_back(std::to_string(hash(t.toString())));
	}

	// in a BST of n leaves there are 2n-1 nodes
	auto numberOfNodes = 2 * hashes.size() - 1;
	int i = 0;
	while (hashes.size() < numberOfNodes) {
		hashes.push_back(std::to_string(hash(hashes[i])) + std::to_string(hash(hashes[i + 1])));
		i += 2;
	}
}

std::string MerkleTree::getMerkleRoot() {
	return hashes.back();
}
