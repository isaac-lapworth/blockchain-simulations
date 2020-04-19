// Node class used to represent a miner in the blockchain system.
#include <vector>
#include <chrono>
#include <algorithm>
#include <map>
#include <cmath>
#include <iostream>
#include <ctime>

#include "Node.h"
#include "Block.h"
#include "Transaction.h"
#include "Network.h"
#include "Semaphore.h"
#include "SHA256.h"

extern const int BLOCK_SIZE;
extern const int BLOCK_TIME;
extern const int ADJUSTMENT_FREQUENCY;
extern const int CONFIRMATION_DEPTH;
extern const int SYNCHRONIZATION_FREQUENCY;
extern const int SYNCHRONIZATION_THRESHOLD;

std::mutex Node::s;
std::mutex Node::b;

Node::Node(unsigned int id, Network& network, std::vector<std::vector<std::tuple<Semaphore, int, int>>>& semaphores, std::map<int, Block>& sharedBlocks):
	id(id), network(network), semaphores(semaphores), sharedBlocks(sharedBlocks) {
}

// get transactions from the network to hash
void Node::getTransactions(std::vector<Transaction>& transactions){
	activity = "GETTING TRANSACTIONS  ";
	while(transactions.size() < BLOCK_SIZE){
		Transaction newTransaction = network.getTransaction(id);
		transactions.push_back(newTransaction);
	}
}

// no longer working on these transactions
void Node::dropTransactions(std::vector<Transaction>& transactions) {
	activity = "DROPPING TRANSACTIONS ";
	for (int i = static_cast<int>(transactions.size() - 1); i >= 0; i--) {
		network.dropTransaction(transactions[i].id, id);
	}
}

// add block to chain and confirm transactions now at the required depth
void Node::addBlock(Block b, int height) {
	activity = "ADDING BLOCK          ";
	
	if(height == blockchain.size()) blockchain.push_back(b);
	else blockchain[height] = b;

	// if the block is past confirmation depth, notify the network that the transactions 
	// can be treated as confirmed
	int blockHeight = static_cast<int>(blockchain.size());
	if (blockHeight > CONFIRMATION_DEPTH) {
		b = blockchain[blockHeight - CONFIRMATION_DEPTH];
		network.confirmTransactions(b.transactions.ids);
	} 

	if(height % ADJUSTMENT_FREQUENCY == 0) adjustDifficulty();
	if(height % SYNCHRONIZATION_FREQUENCY == 0) checkPartition(id + 1);
}

// publish a proof-of-work solution
// with semaphore, first int gives id of successful miner, second is not used
void Node::notifyNetwork(Block b) {
	activity = "PUBLISHING BLOCK      ";

	for (int i = static_cast<int>(semaphores.size()) - 1; i >= 0; i--) {
		if (i == id) continue;
		std::tuple<Semaphore, int, int> t = std::make_tuple(Semaphore::BlockFound, id, static_cast<int>(blockchain.size()-1));

		// locked and unlocked inside loop to allow temporary divergence of blockchains (called a fork)
		s.lock();
		semaphores[i].push_back(t);
		s.unlock();
	}
}

// request block of height from another node
void Node::requestBlock(int from, int height) {
	activity = "REQUESTING BLOCK      ";
	
	// first int is id of node, second is requested height
	s.lock();
	semaphores[from].push_back(std::make_tuple(Semaphore::RequestBlock, id, height));
	s.unlock();
}

// first int gives index of data array where block is, second is block height
void Node::sendBlock(int requester, int height) {
	activity = "SENDING BLOCK         ";

	// tell node block is unavailable - only hit after checkPartition is called
	if (height > blockchain.size()) {
		s.lock();
		semaphores[requester].push_back(std::make_tuple(Semaphore::BlockUnavailable, -1, -1));
		s.unlock();
		return;
	}

	int i = 0;
	b.lock();
	while (sharedBlocks.find(i) != sharedBlocks.end()) {
		i++;
	}

	sharedBlocks.insert(std::pair<int, Block>(i, blockchain[height]));
	b.unlock();

	// tell requester where to find block
	// if index is negative this indicates that transactions have not been sent since the requesting node has already confirmed and flushed them 
	s.lock();
	semaphores[requester].push_back(std::make_tuple(Semaphore::BlockSent, i, height));
	s.unlock();
}

// called when another miner sends a new block of the next block height
void Node::receiveBlock(int index, int height) {
	activity = "VALIDATING BLOCK      ";

	// retrieve block
	Block receivedBlock = sharedBlocks[index];
	
	// validate block hash
	std::string hash;
	b.lock();
	hash = sha256(blockchain[height - 1].hash + receivedBlock.transactions.getMerkleRoot() + std::to_string(receivedBlock.nonce)); 
	b.unlock();
	
	if (!Block::isValid(hash, receivedBlock.difficulty)) return;

	// add block to end of chain if it is still the right height (another block may have been received in the meantime)
	addBlock(receivedBlock, height);

	b.lock();
	sharedBlocks.erase(index);
	b.unlock();
}

// nodes with the same blockchain independently calculate the same network difficulty
void Node::adjustDifficulty() {
	activity = "CALCULATING DIFFICULTY";

	size_t blockHeight = blockchain.size();
	
	double averageTime = 0;
	for (int i = ADJUSTMENT_FREQUENCY - 1; i > 0; i--) {
		averageTime += blockchain.at(blockHeight - i).timestamp - blockchain.at(blockHeight - i - 1).timestamp;
	}
	averageTime /= (ADJUSTMENT_FREQUENCY-1)*1000;
	if (averageTime < BLOCK_TIME) {
		difficulty = blockchain.back().difficulty + 1;
	} else {
		difficulty = blockchain.back().difficulty - 1;
	}
}

// node calculates the total difficulty of the blockchain
void Node::checkPartition(unsigned neighbour) {
	activity = "CHECKING PARTITION    ";

	// condition met if all neighbours have been contacted
	if (neighbour == id) return;

	auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	auto blockchainAge = std::difftime(currentTime, blockchain.front().timestamp) / 1000;
	int expectedMinimumHeight = static_cast<int>(floor((100 - SYNCHRONIZATION_THRESHOLD)*0.01*(blockchainAge / BLOCK_TIME)));

	// this point is unlikely to be reached, but is crucial to PoW
	if (blockchain.size() < expectedMinimumHeight) {

		// pick random node to synchronise with at block height synch threshold under expected height
		// this kind of synchronisation will bring node into majority part of network, if one exists
		int numberOfNeighbours = static_cast<int>(semaphores.size() - 1);

		// return if there are no neighbours
		if (numberOfNeighbours < 1) return;

		synchronize(neighbour, expectedMinimumHeight);
	}
}

// request blocks until valid longer tail found
void Node::synchronize(int node, int height) {
	activity = "SYNCHRONIZING         ";

	// don't want/need to know about blocks at lower depth
	if (height < blockchain.size()) return;

	std::vector<int> blockIndices;
	int initialHeight = height;
	while(true){
		requestBlock(node, height);

		// track index of sent block message (may receive BlockFound messages to be processed after synchronization)
		int semaphoreIndex;
		while(true){
			for (semaphoreIndex = 0; semaphoreIndex < semaphores[id].size(); semaphoreIndex++) {
				switch (std::get<0>(semaphores[id][semaphoreIndex])) {
				case Semaphore::RequestBlock: {
					// get necessary data
					int requester = std::get<1>(semaphores[id][semaphoreIndex]);
					int height = std::get<2>(semaphores[id][semaphoreIndex]);

					// delete message
					s.lock();
					semaphores[id].erase(semaphores[id].begin() + semaphoreIndex);
					s.unlock();

					sendBlock(requester, height);
					break;
				}
				// requested block has been received
				case Semaphore::BlockSent:
					goto exit;
				// received if node sending is also partitioned when node guesses it is partitioned
				case Semaphore::BlockUnavailable:
					s.lock();
					semaphores[id].erase(semaphores[id].begin() + semaphoreIndex);
					s.unlock();
					checkPartition(node + 1);
					return;
				default:
					break;
				}
			}
		}
		exit:;

		// find where in the shared data array the block was shared
		int index = std::get<1>(semaphores[id][semaphoreIndex]);

		// delete message
		s.lock();
		semaphores[id].erase(semaphores[id].begin() + semaphoreIndex);
		s.unlock();

		blockIndices.emplace(blockIndices.begin(), index);
		Block b = sharedBlocks[index];

		// if its hash matches with an existing block in the chain we can copy over the blocks received
		if(height <= blockchain.size() && blockchain[height - 1].hash == b.previousHash) break;

		height -= 1;

		// if network is partitioned badly enough to be totally unsynchronized then wait to sync with another node
		if(height < 0) return;
	}

	// copy blockchain over
	for (int i = 0; i < blockIndices.size(); i++) {
		receiveBlock(blockIndices[i], i + height);
	}
}

// node spends vast majority of compute time here 
void Node::mine() {
	activity = "MINING                ";

	if (blockchain.size() == 0) {

		// start blockchain
		Block genesisBlock;
		blockchain.push_back(genesisBlock);
		std::vector<Transaction> dummyTransactions;

	} else {
		while (true) {
			
			// get transactions to include in the new block - nodes are not selective here, but could be an application-specific extension
			std::vector<Transaction> transactions;
			getTransactions(transactions);
			activity = "MINING                ";

			Block candidateBlock(blockchain.back().hash, transactions, difficulty);

			// generate hashes, iteratively increasing the candidate block's nonce value ("number only used once")
			while(semaphores[id].size() == 0) {
				if(candidateBlock.mine()) {
					
					// if a valid hash is found notify the network
					addBlock(candidateBlock, static_cast<int>(blockchain.size()));
					notifyNetwork(candidateBlock);
					goto cnt;
				}
			}
			dropTransactions(transactions);
			break;
			cnt:;
		}
		// switch on semaphore
		switch (std::get<0>(semaphores[id][0])) {
		case Semaphore::BlockFound: {
			// get necessary data 
			int node = std::get<1>(semaphores[id][0]);
			int height = std::get<2>(semaphores[id][0]);

			// delete message
			s.lock();
			semaphores[id].erase(semaphores[id].begin());
			s.unlock();

			synchronize(node, height);
			break;
		}
		case Semaphore::RequestBlock: {
			// get necessary data
			int requester = std::get<1>(semaphores[id][0]);
			int height = std::get<2>(semaphores[id][0]);

			// delete message
			s.lock();
			semaphores[id].erase(semaphores[id].begin());
			s.unlock();

			sendBlock(requester, height);
			break;
		}
		}
	}
}

void Node::run() {
	while(true){
		// mine blocks forever
		mine();
	}
}
