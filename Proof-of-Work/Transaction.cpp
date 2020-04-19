// A transaction represents a spending of currency. The difference between input and output is the transaction "fee", paid as incentive for miners to include the transaction in their next block.
// Real blockchains commonly use public-key cryptography (e.g. ECDSA) to allow nodes to verify the integrity of transactions, as well verifying correctness - i.e. that the sender has currency available to spend from previous transactions, is not double-spending etc. This is not in the simulation.

#include <string>
#include <array>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream>

#include "Transaction.h"

extern const unsigned AVAILABLE_CONTEXTS;
extern const int BLOCK_SIZE;

std::mutex Transaction::f;
static std::ofstream out("../../../Final\ Paper/template/data/example.csv");
std::ofstream& Transaction::csv = out;

// create a transaction from sender to recipient
Transaction::Transaction(unsigned int id, unsigned int input, unsigned int output):id(id), input(input), output(output) {
	creationTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	confirmations = 0;
	collected = false;
}

// converts an int to a hexadecimal string with leading zeroes
std::string Transaction::toHexString(unsigned int i) {
	std::stringstream stream;
	stream << std::setfill('0') << std::setw(8) << std::hex << i;
	return stream.str();
}

// converts the transaction to a hexadecimal string, ready for hashing
std::string Transaction::toString() {
	return toHexString(id) + toHexString(input) + toHexString(output);
}

bool Transaction::confirm() {
	confirmations++;	
	if(confirmations == AVAILABLE_CONTEXTS){

		// write out transaction data
		confirmationTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		f.lock();
		csv << creationTime << "," << confirmationTime << std::endl;
		f.unlock();
		return true;
	}
	return false;
}
