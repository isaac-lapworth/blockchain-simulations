#include <vector>
#include <thread>
#include <array>
#include <random>
#include <chrono>
#include <map>
#include <iostream>

#include "Node.h"
#include "Network.h"
#include "Semaphore.h"
#include "Monitor.h"

/* Constants declared as global variables to simplify data collection */
// the maximum number of transactions that can be included in a new block
extern const int BLOCK_SIZE = 5;
// targeted average time that a block is generated
extern const int BLOCK_TIME = 10;
// number of non-zeros required to begin with
extern const int INITIAL_DIFFICULTY = 2;
// number of blocks after which the difficulty is adjusted
extern const int ADJUSTMENT_FREQUENCY = 20;
// rate at which transactions are generated, one every TF seconds
extern const double TRANSACTION_FREQUENCY = 0.1;
// since proof-of-work is probabilistic, transactions are never 100% confirmed in the history
// this heuristic is the number of blocks deep a transaction needs to be before it is treated as confirmed and data is output
extern const int CONFIRMATION_DEPTH = 5;
// % difference in expected blockchain length that will cause a node to request a copy of another's due to suspicion of Byzantine behaviour
extern const int SYNCHRONIZATION_THRESHOLD = 30;
// frequency (in blocks) at which a node compares its blockchain to the expected length, detecting a network partition
extern const int SYNCHRONIZATION_FREQUENCY = 20;
// number of miners (number of cores minus two since display + network simulation both require a thread)
extern const unsigned AVAILABLE_CONTEXTS = std::thread::hardware_concurrency() - 2;
// number of recent transactions to display
extern const int TRANSACTIONS_TO_SHOW = 20;
// if true, mining is 2x harder per character instead of 16x
extern const bool BINARY_HASH = false;

int main() {

	// shared by threads, allows all to retrieve and confirm transactions 
	Network network;

	// allows nodes to pass messages
	std::vector<std::vector<std::tuple<Semaphore, int, int>>> semaphores(AVAILABLE_CONTEXTS);
	// allows nodes to pass data (blocks and transactions)
	std::map<int, Block> sharedBlocks;

	// start mining threads
	std::vector<Node*> nodes;
	std::vector<std::thread> threads;
	for(unsigned i = 0; i < AVAILABLE_CONTEXTS;){
		Node* n = new Node(i++, network, semaphores, sharedBlocks);
		nodes.push_back(n);
		threads.push_back(std::thread(&Node::run, n));
	}

	// start thread which prints simulation info to the console
	Monitor* m = new Monitor(semaphores);
	std::thread display(&Monitor::display, m, nodes, &network);

	network.generateTransactions();
	return 0;
}
