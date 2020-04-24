#include <vector>
#include <mutex>
#include <map>

#include "Block.h"
#include "Transaction.h"
#include "Network.h"
#include "Semaphore.h"

#ifndef NODE_H
#define NODE_H

extern const int INITIAL_DIFFICULTY;

class Node {

private:

	Network& network;
	std::vector<std::vector<std::tuple<Semaphore, int, int>>>& semaphores;
	std::map<int, Block>& sharedBlocks;
	int difficulty = INITIAL_DIFFICULTY; // the number of leading zeros required on the hash of a block to be able to add it to the chain (initially)
	
	static std::mutex s; // to protect the semaphore data array
	static std::mutex b; // to protect shared block array

	void getTransactions(std::vector<Transaction>& transactions);
	void dropTransactions(std::vector<Transaction>& transactions);
	void addBlock(Block b, int height);
	void notifyNetwork(Block b);
	void requestBlock(int from, int height);
	void sendBlock(int requester, int height);
	void receiveBlock(int index, int height);
	void checkPartition(unsigned neighbour);
	void adjustDifficulty();
	void synchronize(int node, int height);
	void mine();

public:

	const unsigned int id;
	std::vector<Block> blockchain;
	std::string activity; // information on the node's operation for display

	Node(unsigned int id, Network& network, std::vector<std::vector<std::tuple<Semaphore, int, int>>>& semaphores, std::map<int, Block>& sharedBlocks);

	void run();
};

#endif
