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
// wait time between consensus rounds (large compared to time for consensus)
extern const int BLOCK_TIME = 4;
// frequency (in seconds) at which transactions are generated
extern const double TRANSACTION_FREQUENCY = 0.2;
// number of bookkeepers
extern const unsigned NUMBER_OF_NODES = std::thread::hardware_concurrency() - 2;
// number of recent transactions to display
extern const int TRANSACTIONS_TO_SHOW = 20;
// number of nodes which are inactive
extern const unsigned UNRESPONSIVE_NODES = 0;
// number of nodes which are malicious
extern const unsigned MALICIOUS_NODES = 1;
// choose speaker randomly
// being based on PBFT, the nodes are state machine replicas - but only one may be the speaker and propose a block
// this requires a centralised selection process so that they are known at the start of the round 
// in other protocols, the 'random' order of nodes who have their blocks added to the chain stems from the fact that any node may publish a candidate solution
// this could be extended to decide the speaker based on the hash of the previous block in a distributed fashion
extern const bool RANDOM_SPEAKER = false;

int main() {

	// shared by threads, allows all to retrieve and confirm transactions
	Network network;

	// allows nodes to pass messages
	std::vector<std::vector<std::tuple<Semaphore, int, int, int>>> semaphores(NUMBER_OF_NODES);

	// the proposal allows the speaker to share the transactions and hash of the proposal for verification
	Block fullBlock;
	std::pair<std::vector<Transaction>, std::string> proposal;

	// start consensus threads
	std::vector<std::thread> threads;
	std::vector<Node*> nodes;
	for (unsigned i = 0; i < NUMBER_OF_NODES;) {
		// numbers of malicious and unresponsive nodes set here i.e. to show the 1/3 Byzantine fault tolerance limit
		Node* n = new Node(i++, network, semaphores, &fullBlock, &proposal, (i >= UNRESPONSIVE_NODES), (i < NUMBER_OF_NODES - MALICIOUS_NODES));
		nodes.push_back(n);
		threads.push_back(std::thread(&Node::run, n));
	}

	// start thread which prints simulation info to the console
	Monitor* m = new Monitor(semaphores);
	std::thread display(&Monitor::display, m, nodes, &(network.recentConfirmations));

	// the main thread simulates the network
	network.generateTransactions();
	return 0;
}
