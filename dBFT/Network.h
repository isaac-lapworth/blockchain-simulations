#include <vector>
#include <mutex>
#include <random>
#include <string>
#include <map>

#include "Transaction.h"
#include "Block.h"

#ifndef NETWORK_H
#define NETWORK_H

class Network {

private:

	std::mt19937_64 rng;
	std::mutex p; // protects pool
	std::vector<Transaction*> pool;


public:

	Network();
	static std::mutex r; // protects recent confirmations
	std::vector<std::tuple<unsigned, time_t, time_t>> recentConfirmations;
	// network thread fills pool with transactions
	void generateTransactions(); 
	// nodes call this to iterate over map and copy transactions into local memory
	Transaction* receiveTransaction(unsigned long* counter); 
	// called by nodes when blocks are agreed to tell the network the transactions are confirmed (output timestamps)
	void confirmTransactions(std::vector<Transaction>& transactions);
};

#endif
