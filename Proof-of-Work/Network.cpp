// Network class is essentially the environment that the mining nodes run within. It is used to collectively simulate all peers that do not run a blockchain node - i.e. blockchain users but not miners.
// In real life the number of non-mining peers is large compared to the number of miners
#include <iostream>
#include <thread>
#include <random>
#include <chrono>
#include <string>
#include <iostream>

#include "Network.h"
#include "Transaction.h"
#include "MerkleTree.h"
#include "SHA256.h"

extern const double TRANSACTION_FREQUENCY;
extern const int TRANSACTIONS_TO_SHOW;

Network::Network() {
	// setup random number generator for transactions
	std::random_device rd;
	rng = std::mt19937_64(rd());
}

// populates the pool of unconfirmed transactions
void Network::generateTransactions() {
	std::uniform_int_distribution<unsigned int> dist(1, 100000);

	unsigned int i = 0;
	while(true){
		Transaction* t = new Transaction(i, dist(rng), dist(rng));
		p.lock();
		pool.push_back(t);
		p.unlock();
		i++;
		std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(TRANSACTION_FREQUENCY * 1000)));
	}
}

// returns distribution used by the RNG
std::uniform_int_distribution<unsigned long long int> Network::createDistribution(){

	p.lock();
	unsigned long long int max = pool.size();
	p.unlock();

	// handles race conditions where a transaction may be requested before any are generated
	if(max == 0){
		while(max == 0){
			std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(TRANSACTION_FREQUENCY * 1000)));
			p.lock();
			max = pool.size();
			p.unlock();
		}
	}

	// ternary used to handle race conditions meaning a transaction is requested before any are generated
	std::uniform_int_distribution<unsigned long long int> dist(0, max-1);
	return dist;
}

// called when a mining node is listening for transactions
Transaction& Network::getTransaction(int requester) {
	std::uniform_int_distribution<unsigned long long int> dist;
	unsigned long long int index;
	while(true){

		dist = createDistribution();
		index = dist(rng);
		p.lock();
		Transaction* t = pool[index];
		p.unlock();
		if(t != nullptr && !t->collected) {
			t->collected = true;
			return *t;
		}
	}
}

// called when a mining node stops mining a block containing a transaction (e.g. if an alternative block is received)
void Network::dropTransaction(int id, int dropper) {
	p.lock();
	Transaction *t = pool[id];
	if (t != nullptr) {
		t->collected = false; 
	}
	p.unlock();
}

// called when the transactions in a block have enough additional blocks mined on top of them to be treated as immutable 
void Network::confirmTransactions(std::vector<unsigned>& transactionIDS) {
	// confirm transactions
	for (unsigned transactionID : transactionIDS) {
		p.lock();
		Transaction* t = pool[transactionID];
		if(t != nullptr && t->confirm()){
			// add to the list of recently confirmed transactions
			if (recentConfirmations.size() == TRANSACTIONS_TO_SHOW) recentConfirmations.erase(recentConfirmations.begin());
			recentConfirmations.push_back(std::make_tuple(t->id, t->creationTime, t->confirmationTime));
			pool[transactionID] = nullptr;
			delete t;
		}
		p.unlock();
	}
		
}
