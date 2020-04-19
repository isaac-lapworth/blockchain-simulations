#include <chrono>
#include <vector>
#include <string>

#include "Block.h"
#include "Transaction.h"
#include "SHA256.h"

// produces a genesis block with a dummy coinbase transaction (if tracking all nodes' balance, this transaction would credit this node with all initial currency)
Block::Block() :transactions({ Transaction(0, 0, 0) }) {
	previousHash = "0000000000000000000000000000000000000000000000000000000000000000";
	hash = sha256(previousHash + MerkleTree(transactions).getMerkleRoot());
	// record when the block is created
	timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

Block::Block(std::string previousHash, std::vector<Transaction> transactions) :previousHash(previousHash), transactions(transactions) {
	hash = sha256(previousHash + MerkleTree(transactions).getMerkleRoot());
	timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
