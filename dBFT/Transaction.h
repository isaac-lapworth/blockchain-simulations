#include <string>
#include <set>
#include <mutex>
#include <fstream>

#ifndef TRANSACTION_H
#define TRANSACTION_H

class Transaction {

private:
	std::string toHexString(unsigned int i);

public:

	unsigned int id;
	unsigned int input;
	unsigned int output;
	time_t creationTime;
	time_t confirmationTime;

	static std::ofstream& csv;
	static std::mutex f; // protects .csv output

	Transaction();
	Transaction(unsigned int id, unsigned int input, unsigned int output);

	std::string toString();
	void confirm();

};

#endif
