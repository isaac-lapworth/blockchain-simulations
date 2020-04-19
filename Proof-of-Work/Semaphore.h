#ifndef SEMAPHORE_H
#define SEMAPHORE_H

// defines the types of messages nodes can pass between them 
enum class Semaphore {

	// Indicates to a node that another has found a block 
	BlockFound,

	// If a node calculates that the blockchain should be longer than it is (e.g. if another node publishes a solution or it detects a partition)
	// it requests the most recent block of other nodes
	RequestBlock,

	// Tell a node that their requested block has been sent
	BlockSent,

	// Tells a node the requested block is unavailable
	BlockUnavailable

};

#endif
