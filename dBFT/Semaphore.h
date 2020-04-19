#ifndef SEMAPHORE_H
#define SEMAPHORE_H

// defines the types of messages nodes can pass between them 
enum class Semaphore {

	// Indicates a block proposal has been sent  
	PrepareRequest,

	// Indicates a proposal has been approved
	PrepareResponse,

	// Indicates a proposal has been rejected 
	// or that the view time limit has been reached
	ChangeView,

	// Indicates that a full block has been published
	BlockPublished
};

#endif
