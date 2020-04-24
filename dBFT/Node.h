#include <vector>
#include <mutex>
#include <random>

#include "Block.h"
#include "Transaction.h"
#include "Network.h"
#include "Semaphore.h"

#ifndef NODE_H
#define NODE_H

class Node {

private:

	Network& network;

	std::mt19937_64 rng;
	unsigned long transactionCounter; // records from which point to continue listening for transactions each round 

	// local memory
	time_t viewStart;
	std::map<unsigned int, Transaction> bookkeeperMemory;

	// shared memory
	std::vector<std::vector<std::tuple<Semaphore, int, int, int>>>& semaphores;
	Block* fullBlock;
	std::pair<std::vector<Transaction>, std::string>* proposal;

	// send a message to all other threads message queues
	void broadcast(std::tuple<Semaphore, int, int, int> message);
	// executes a round of consensus
	void round();
	// while waiting, returns true if message received is for the current block height and view
	// otherwise deletes the old message
	bool filterMessage();
	// returns true if round has timed out
	bool timedOut();
	// node monitors network transactions for time BLOCK_TIME each round
	void wait(bool speaker);
	// the speaker creates and broadcasts a block proposal
	void proposeBlock();
	// the delegates validate the proposal
	void validateProposal();
	// all nodes count responses to see if a majority exists 
	bool listenForResponses();
	// if there are a majority of prepare responses publish a full block
	void publishFullBlock();
	// and add the new block to the blockchain
	void addBlock();
	// updates the random speaker, when enabled
	static int getRandomSpeaker(int height, int view);
	
public:

	// local memory
	const unsigned int id;
	bool responsive;
	bool honest;
	std::string activity;
	std::vector<Block> blockchain;
	int blockHeight = 0;
	int view;
	bool speaker;

	// variables to store data when using random speaker
	static int highestRound;
	static int highestView;
	static int randomSpeaker;

	static std::mutex s; // to protect the semaphore array
	static std::mutex b; // to protect the shared block 
	static std::mutex r; // protects RNG used for random speaker mode

	Node(unsigned int id, Network& network, std::vector<std::vector<std::tuple<Semaphore, int, int, int>>>& semaphores, Block* fullBlock, std::pair<std::vector<Transaction>, std::string>* proposal, bool responsive, bool honest);

	void run();
};

#endif
