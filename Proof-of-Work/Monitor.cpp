#include <string>
#include <sstream>

#include "Monitor.h"
#include "Node.h"
#include "Semaphore.h"
#include "Network.h"

#include <curses.h>

extern const int BLOCK_SIZE;
extern const int BLOCK_TIME;
extern const int ADJUSTMENT_FREQUENCY;
extern const double TRANSACTION_FREQUENCY;
extern const int CONFIRMATION_DEPTH;
extern const int SYNCHRONIZATION_FREQUENCY;
extern const unsigned AVAILABLE_CONTEXTS;
extern const int TRANSACTIONS_TO_SHOW;
extern const bool BINARY_HASH;

Monitor::Monitor(std::vector<std::vector<std::tuple<Semaphore, int, int>>>& semaphores): semaphores(semaphores) {
	flagStrings[Semaphore::BlockFound] = "BLOCK_FOUND";
	flagStrings[Semaphore::BlockSent] = "BLOCK_SENT";
	flagStrings[Semaphore::BlockUnavailable] = "BLOCK_UNAVAILABLE";
	flagStrings[Semaphore::RequestBlock] = "BLOCK_REQUEST";
}

void Monitor::display(std::vector<Node*> nodes, Network* network) {

	// initialize the console for curses
	initscr();
	raw();

	// window for displaying consensus threads' data 
	int nodesHeight = AVAILABLE_CONTEXTS + 3;
	int nodesWidth = 110;
	WINDOW* nodesWin = newwin(nodesHeight, nodesWidth, 0, 0);
	box(nodesWin, 0, 0);
	mvwprintw(nodesWin, 0, 0, "Consensus Node Local Data ");
	mvwprintw(nodesWin, 1, 1, "ID  Height  Activity   \t\tWorking Hash");
	wrefresh(nodesWin);

	// window to show nodes' message queues
	int messageHeight = nodesHeight - 1;
	WINDOW* messageWin = newwin(messageHeight, nodesWidth, nodesHeight, 0);
	box(messageWin, 0, 0);
	mvwprintw(messageWin, 0, 0, "Message Queues ");
	wrefresh(messageWin);

	// window to show recently confirmed transactions
	WINDOW* transactionsWin = newwin(TRANSACTIONS_TO_SHOW + 3, 41, 0, nodesWidth + 1);
	box(transactionsWin, 0, 0);
	mvwprintw(transactionsWin, 0, 0, "Confirmed Transactions ");
	mvwprintw(transactionsWin, 1, 1, "   ID   Published       Confirmed");
	wrefresh(transactionsWin);

	// string stream used to convert node data to C strings used by PDcurses
	std::stringstream ss;

	// window to show simulation settings
	int settingsHeight = nodesHeight + messageHeight;
	int settingsColTwo = nodesWidth / 2 + 2;
	WINDOW* settingsWin = newwin(6, nodesWidth, settingsHeight, 0);
	box(settingsWin, 0, 0);
	mvwprintw(settingsWin, 0, 0, "Simulation Parameters ");
	mvwprintw(settingsWin, 1, 1, "Block Size: ");
	mvwprintw(settingsWin, 1, 13, std::to_string(BLOCK_SIZE).c_str());
	mvwprintw(settingsWin, 2, 1, "Block Frequency: ");
	mvwprintw(settingsWin, 2, 18, std::to_string(BLOCK_TIME).c_str());
	mvwprintw(settingsWin, 3, 1, "Difficulty Calculation (blocks): ");
	mvwprintw(settingsWin, 3, 34, std::to_string(ADJUSTMENT_FREQUENCY).c_str());
	mvwprintw(settingsWin, 4, 1, "Transaction Frequency: ");
	mvwprintw(settingsWin, 4, 24, std::to_string(TRANSACTION_FREQUENCY).c_str());
	for (int i = 1; i < 5; i++) mvwprintw(settingsWin, i, settingsColTwo-2, "|");
	mvwprintw(settingsWin, 1, settingsColTwo, "Required Confirmations: ");
	mvwprintw(settingsWin, 1, settingsColTwo + 24, std::to_string(CONFIRMATION_DEPTH).c_str());
	mvwprintw(settingsWin, 2, settingsColTwo, "Partition Check (blocks): ");
	mvwprintw(settingsWin, 2, settingsColTwo + 26, std::to_string(SYNCHRONIZATION_FREQUENCY).c_str());
	mvwprintw(settingsWin, 3, settingsColTwo, "Consensus Nodes: ");
	mvwprintw(settingsWin, 3, settingsColTwo + 17, std::to_string(AVAILABLE_CONTEXTS).c_str());
	mvwprintw(settingsWin, 4, settingsColTwo, "Using Binary Hashes: ");
	mvwprintw(settingsWin, 4, settingsColTwo + 22, BINARY_HASH ? "True" : "False");
	wrefresh(settingsWin);

	// continuously update display
	while (true) {
		
		// get most recently confirmed transactions
		auto recentConfs = network->recentConfirmations;
		int numberOfTransactions = static_cast<int>(recentConfs.size());
		for (int i = 0; i < numberOfTransactions; i++) {
			std::tuple<unsigned, time_t, time_t> transactionData = recentConfs[i];
			ss << "   " << std::get<0>(transactionData);
			ss << "   " << std::get<1>(transactionData);
			ss << "   " << std::get<2>(transactionData);
			mvwprintw(transactionsWin, 1 + numberOfTransactions - i, 1, ss.str().c_str());
			ss.str("");
		}
		wrefresh(transactionsWin);

		// clear old messages
		werase(messageWin);
		box(messageWin, 0, 0);
		mvwprintw(messageWin, 0, 0, "Message Queues");
		// refresh message queue table
		try { // try-catch handles intermittent PDCurses bug
			for (unsigned i = 0; i < AVAILABLE_CONTEXTS; i++) {
				auto flags = semaphores[i];
				for (int j = 0; j < static_cast<int>(flags.size()); j++) {
					ss << flagStrings[std::get<0>(flags[j])] << "  "; 
					if (ss.str().size() + 15 > nodesWidth) break;
				}
				mvwprintw(messageWin, i + 1, 1, ss.str().substr(0, nodesWidth - 3).c_str());
				ss.str("");
			}
			wrefresh(messageWin);
		} catch (...) {
			// do nothing
		}

		// get latest node data 
		for (unsigned i = 0; i < AVAILABLE_CONTEXTS; i++) {
			
			auto blockHeight = nodes[i]->blockchain.size();
			std::string workingHash = "-";
			if (blockHeight > 0) workingHash = nodes[i]->blockchain.back().hash;
			ss << nodes[i]->id;
			ss << "     " << blockHeight;
			ss << "     " << nodes[i]->activity;
			ss << " \t" << workingHash;
			mvwprintw(nodesWin, i + 2, 1, ss.str().c_str());
			ss.str("");
		}
		wrefresh(nodesWin);
	}
}
