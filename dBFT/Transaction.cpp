// Real blockchains commonly use public-key cryptography to allow nodes to verify the integrity of transactions, as well verifying correctness 
// i.e. that the sender has currency available to spend from previous transactions, is not double-spending etc. This is not in the simulation, as malicious nodes have not been programmed and transactions are assumed to be correct.
// input and output fields are dummy data, as explained in the solution section of the project paper

#include <string>
#include <array>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream>

#include "Transaction.h"

/*
extern const unsigned NUMBER_OF_NODES;
extern const unsigned UNRESPONSIVE_NODES; // for data collection
extern const unsigned MALICIOUS_NODES; // for data collection
*/

std::mutex Transaction::f;
static std::ofstream out("../../../Final\ Paper/template/data/example.csv");
std::ofstream& Transaction::csv = out;

// default constructor (required for use of Transaction in pairs by Node class)
Transaction::Transaction() {}

// create a transaction from sender to recipient
Transaction::Transaction(unsigned int id, unsigned int input, unsigned int output) :id(id), input(input), output(output) {
	creationTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
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

void Transaction::confirm() {
	// write out transaction data
	confirmationTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	f.lock();
	csv << creationTime << "," << confirmationTime << std::endl;
	f.unlock();
}
