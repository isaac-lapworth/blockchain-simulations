// Node class used to represent a bookkeeper in the blockchain system.
#include <vector>
#include <chrono>
#include <algorithm>
#include <map>
#include <cmath>
#include <iostream>
#include <ctime>
#include <random>
#include <cstdlib>

#include "Node.h"
#include "Block.h"
#include "Transaction.h"
#include "Network.h"
#include "Semaphore.h"

extern const int BLOCK_SIZE;
extern const int BLOCK_TIME;
extern const unsigned NUMBER_OF_NODES;
extern const bool RANDOM_SPEAKER;

std::mutex Node::s;
std::mutex Node::b;
std::mutex Node::r;

int Node::highestRound = -1;
int Node::highestView = -1;

int Node::getRandomSpeaker(int height, int view) {
	unsigned speaker;
	r.lock();
	srand(height + view);
	speaker = rand() % NUMBER_OF_NODES;
	r.unlock();
	return speaker;
}

Node::Node(unsigned int id, Network& network, std::vector<std::vector<std::tuple<Semaphore, int, int, int>>>& semaphores, Block* fullBlock, std::pair<std::vector<Transaction>, std::string>* proposal, bool responsive, bool honest) :
	id(id), network(network), semaphores(semaphores), fullBlock(fullBlock), proposal(proposal), responsive(responsive), honest(honest) {
	
	// initialize random number generator
	std::random_device rd;
	rng = std::mt19937_64(rd());

	// add a genesis block 
	Block genesisBlock;
	blockchain.push_back(genesisBlock);
}

void Node::broadcast(std::tuple<Semaphore, int, int, int> message) {
	activity = "BROADCASTING MESSAGE";
	for (unsigned i = 0; i < NUMBER_OF_NODES; i++) {
		// std::vector may change position in memory when adding/erasing elements so must be read atomically
		s.lock();
		semaphores[i].push_back(message);
		s.unlock();
	}
}

bool Node::filterMessage(){

	s.lock();
	int h = std::get<1>(semaphores[id][0]);
	int v = std::get<2>(semaphores[id][0]);
	s.unlock();

	// if the message is from a node still working at an old block height
	if (h < blockHeight ||
		// or the same block height but an old view
		(h == blockHeight && v < view)) {
		// delete their message
		s.lock();
		semaphores[id].erase(semaphores[id].begin());
		s.unlock();
		return false;
	}
	return true;
}

// node checks if the current round has timed out
bool Node::timedOut() {
	time_t t = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - viewStart;
	return t > pow(2, view + 1) * BLOCK_TIME * 1000;
}

void Node::wait(bool speaker) {
	activity = "MONITORING NETWORK  ";

	// if the node is the speaker, listen for transactions until the waiting period is over
	if (speaker) {
		time_t until = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() + BLOCK_TIME * 1000;
		while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() < until) {
			Transaction* t = network.receiveTransaction(&transactionCounter);
			if (t != nullptr) {
				Transaction copy(*t);
				bookkeeperMemory[copy.id] = copy;
			}
		}
	} 
	// otherwise receive transactions until the speaker prepares a proposal
	else {
		while (true) {
			s.lock();
			auto size = semaphores[id].size();
			s.unlock();

			if(size != 0 && filterMessage()) break;
			if (timedOut()) break;
			Transaction* t = network.receiveTransaction(&transactionCounter);
			if (t != nullptr) {
				Transaction copy(*t);
				bookkeeperMemory[copy.id] = copy;
			}
		}
	}
}

void Node::proposeBlock() {
	activity = "PUBLISHING PROPOSAL ";

	// pick some random transactions from memory
	// initialise vars
	std::uniform_int_distribution<unsigned int> dist(0, (int) bookkeeperMemory.size()-1);
	std::vector<Transaction> transactions;
	std::map<unsigned, bool> collected;

	// collect transactions
	for (int i = 0; i < BLOCK_SIZE; i++) {
		std::map<unsigned int, Transaction>::iterator iter = bookkeeperMemory.begin();
		unsigned id = dist(rng);

		// avoid duplicates 
		std::advance(iter, dist(rng));
		if (collected[iter->second.id]) continue;
		collected[iter->second.id] = true;
		transactions.push_back(iter->second);
	}

	// publish a block proposal
	std::string hash = (honest ? Block(blockchain.back().hash, transactions).hash : "");
	*proposal = std::pair<std::vector<Transaction>, std::string>(transactions, hash);

	// notify the delegates
	broadcast(std::tuple<Semaphore, int, int, int>(Semaphore::PrepareRequest, blockHeight, view, id));
}

void Node::validateProposal() {
	activity = "VALIDATING PROPOSAL ";

	s.lock();
	auto size = semaphores[id].size();
	auto flag = std::get<0>(semaphores[id][0]);
	s.unlock();

	// check that the block is valid (hash of transactions and own previous hash equals hash sent)
	if (size > 0 && flag == Semaphore::PrepareRequest && 
		Block(blockchain.back().hash, std::get<0>(*proposal)).hash == std::get<1>(*proposal)) {
		broadcast(std::tuple<Semaphore, int, int, int>((honest ? Semaphore::PrepareResponse : Semaphore::ChangeView), blockHeight, view, id));
	}
	// else request a view change 
	else broadcast(std::tuple<Semaphore, int, int, int>((honest ? Semaphore::ChangeView : Semaphore::PrepareResponse), blockHeight, view, id));
}

void Node::addBlock() {
	activity = "ADDING BLOCK        ";
	blockHeight++;
	blockchain.push_back(*fullBlock);

	// notify the rest of the network which transactions are now final
	std::vector<Transaction> confirmedTransactions = proposal->first;
	if(speaker) network.confirmTransactions(confirmedTransactions);

	// delete the transactions from local memory
	for (Transaction t : confirmedTransactions) {
		bookkeeperMemory.erase(t.id);
	}
}

void Node::publishFullBlock() {
	activity = "PUBLISHING BLOCK    ";
	b.lock();
	// publish a block if this node is the first to reach detect consensus
	if (fullBlock->hash != proposal->second) {
		*fullBlock = Block(blockchain.back().hash, proposal->first);
		broadcast(std::tuple<Semaphore, int, int, int>(Semaphore::BlockPublished, blockHeight, view, id));
	}
	b.unlock();
	addBlock();
}

bool Node::listenForResponses() {
	activity = "RECEIVING RESPONSES ";
	double supermajority = 2.0 / 3.0;
	unsigned approvals = 0;
	unsigned rejections = 0;
	bool response = false;

	// arrays to check nodes are not sending multiple responses
	// approving nodes can however later request a view change 
	// progression still only occurs if a 2/3 majority is reached, preserving safety properties
	int* receivedApprovals = new int[NUMBER_OF_NODES];
	int* receivedRejections = new int[NUMBER_OF_NODES];

	// initialize arrays to zero
	for (unsigned i = 0; i < NUMBER_OF_NODES; i++) {
		receivedApprovals[i] = 0;
		receivedRejections[i] = 0;
	}

	while (approvals + rejections < NUMBER_OF_NODES) {
		s.lock();
		auto size = semaphores[id].size();
		s.unlock();

		if (size > 0) {

			// retrieve message data 
			s.lock();
			int senderHeight = std::get<1>(semaphores[id][0]);
			int senderView = std::get<2>(semaphores[id][0]);
			unsigned senderID = std::get<3>(semaphores[id][0]);
			s.unlock();

			if (senderHeight == blockHeight && senderView == view) {
				s.lock();
				auto flag = std::get<0>(semaphores[id][0]);
				s.unlock();

				switch (flag) {

				// the speaker backs their own proposal
				case Semaphore::PrepareRequest:
					if (receivedApprovals[senderID] == 0) {
						receivedApprovals[senderID] = 1;
						approvals++;
					}
					break;

				// response of a delegate that approves of the proposal
				case Semaphore::PrepareResponse:
					if (receivedApprovals[senderID] == 0) {
						receivedApprovals[senderID] = 1;
						approvals++;
					}
					break;

				// if a node rejects the proposal they request a view change
				// (with the next view having a different speaker)
				case Semaphore::ChangeView:
					if (receivedRejections[senderID] == 0) {
						receivedRejections[senderID] = 1;
						rejections++;
					}
					break;

				// if a block has been published consensus has been reached 
				case Semaphore::BlockPublished:
					addBlock();
					response = true;

					// remove read message from queue if about to exit
					s.lock();
					semaphores[id].erase(semaphores[id].begin());
					s.unlock();
					goto exit;
				}
			}

			// remove read message from queue
			// since the wait of time t occurs each view, messages of different views should not be interleaved
			s.lock();
			semaphores[id].erase(semaphores[id].begin());
			s.unlock();

			// check for a majority in either direction
			// note: use of goto here is the neatest solution to exit the nested loop and is used pragmatically, rather than avoiding it on principle
			if (approvals > supermajority * NUMBER_OF_NODES) {
				publishFullBlock();
				response = true;
				goto exit;
			}

			if (rejections > supermajority * NUMBER_OF_NODES) {
				response = false;
				goto exit;
			}
		}

		// node will request a view change if the round of consensus times out 
		else if(timedOut()) {
			broadcast(std::tuple<Semaphore, int, int, int>(Semaphore::ChangeView, blockHeight, view, id));
			response = false;
			goto exit;
		}
	}
	exit:
	delete receivedApprovals;
	delete receivedRejections;
	return response;
}

void Node::round() {
	activity = "INITIALISING ROUND  ";
	// reset the view index
	view = 0;
	
	bool consensusReached = false;
	while (true) {		
		
		// record the view start time 
		viewStart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		// determine the speaker node 
		if (RANDOM_SPEAKER) speaker = getRandomSpeaker(blockHeight, view) == id;
		else speaker = (blockHeight - view) % NUMBER_OF_NODES == id;

		// wait - should be long compared to time for consensus
		wait(speaker);

		// commence consensus
		if (speaker) proposeBlock();
		else if(!timedOut()) validateProposal();
	
		// await a majority
		bool consensus = listenForResponses();
		if (consensus) break;
		else view++;
	}
}

void Node::run() {
	activity = "NONE                ";
	while (responsive) {
		round();
	}
}
