#include <vector>
#include <mutex>
#include <random>
#include <string>

#include "Transaction.h"
#include "Block.h"

#ifndef NETWORK_H
#define NETWORK_H

class Network {

private:

	std::mt19937_64 rng;
	std::vector<Transaction*> pool;

	std::mutex p; // protects pool

	std::uniform_int_distribution<unsigned long long int> createDistribution();

public:

	Network();

	std::vector<std::tuple<unsigned, time_t, time_t>> recentConfirmations;

	void generateTransactions();
	Transaction& getTransaction(int requester);
	void dropTransaction(int id, int dropper);
	void confirmTransactions(std::vector<unsigned>& transactionIDS);
};

#endif
