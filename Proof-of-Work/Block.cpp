#include <chrono>
#include <vector>
#include <string>
#include <iostream>

#include "Block.h"
#include "MerkleTree.h"
#include "Transaction.h"
#include "SHA256.h"

extern const int INITIAL_DIFFICULTY;
extern const bool BINARY_HASH;

bool Block::isValid(std::string hash, int difficulty) {
	// check leading zeros of the hash
	int i = 0;
	while (i < difficulty) {
		if (BINARY_HASH) {
			if (hash[i] > '7') return false;
		}
		else if (hash[i] != '0') return false;
		i++;
	}
	return true;
}

// produces a genesis block with a dummy coinbase transaction (if tracking all nodes' balance, this transaction would credit this node with all initial currency)
Block::Block():transactions({Transaction(0, 0, 0)}){
	nonce = 0;
	difficulty = INITIAL_DIFFICULTY;
	previousHash = "0000000000000000000000000000000000000000000000000000000000000000";

	// calculate the hash of the first block
	while (!mine());
}

Block::Block(std::string previousHash, std::vector<Transaction> transactions, int difficulty) :previousHash(previousHash), transactions(transactions), difficulty(difficulty) {
	nonce = 0;
}

// this is where proof-of-work happens
bool Block::mine() {
	nonce++;

	// concatenate and hash the block data
	hash = sha256(previousHash + transactions.getMerkleRoot() + std::to_string(nonce));

	// check leading zeros of the hash
	if (!isValid(hash, difficulty)) return false;

	// record when the block is solved
	timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	return true;
}
