// Network class is used to collectively simulate all peers that do not run a blockchain node - i.e. NEO users
// In the real network the number of non-consensus nodes is large compared to those involved in consensus
#include <iostream>
#include <thread>
#include <random>
#include <chrono>
#include <iostream>

#include "Network.h"
#include "Transaction.h"

extern const double TRANSACTION_FREQUENCY;
extern const int TRANSACTIONS_TO_SHOW;

std::mutex Network::r;

// setup random number generator for transactions
Network::Network() {
	std::random_device rd;
	rng = std::mt19937_64(rd());
}

// populates the unconfirmed transaction pool 
void Network::generateTransactions() {
	std::uniform_int_distribution<unsigned int> dist(1, 100000);

	unsigned long id = 0;
	while (true) {
		Transaction* t = new Transaction(id, dist(rng), dist(rng));
		p.lock();
		pool.push_back(t);
		p.unlock();
		id++;
		std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(TRANSACTION_FREQUENCY * 1000)));
	}
}

// where consensus nodes spend wait time 
Transaction* Network::receiveTransaction(unsigned long* counter) {
	unsigned long c = *counter;
	while (c < pool.size()) {
		if (pool[c] != nullptr) {
			*counter = c+1;
			return pool[c];
		}
		c++;
	}
	return nullptr;
}

// finality guarantees of dBFT means we can confirm transactions after one node calls this function
// when collecting data on block times in presence of faults, add output to this function
void Network::confirmTransactions(std::vector<Transaction>& transactions) {
	for (Transaction transaction : transactions) {
		Transaction* t = pool[transaction.id];

		// return if another node has already notified the network
		if (t == nullptr) return;
		p.lock();
		t->confirm();

		// add to the list of recently confirmed transactions
		r.lock();
		if (recentConfirmations.size() == TRANSACTIONS_TO_SHOW) recentConfirmations.erase(recentConfirmations.begin());
		recentConfirmations.push_back(std::make_tuple(t->id, t->creationTime, t->confirmationTime));
		r.unlock();
		delete t;
		p.unlock();
	}
}
