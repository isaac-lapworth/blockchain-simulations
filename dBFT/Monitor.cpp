#include <string>
#include <sstream>

#include "Monitor.h"
#include "Node.h"
#include "Network.h"

#include <curses.h>

extern const int BLOCK_SIZE;
extern const int BLOCK_TIME;
extern const double TRANSACTION_FREQUENCY;
extern const unsigned NUMBER_OF_NODES;
extern const int TRANSACTIONS_TO_SHOW;
extern const unsigned UNRESPONSIVE_NODES;
extern const unsigned MALICIOUS_NODES;

Monitor::Monitor(std::vector<std::vector<std::tuple<Semaphore, int, int, int>>>& semaphores): semaphores(semaphores) {
	flagStrings[Semaphore::PrepareRequest] = "PREPARE_REQUEST";
	flagStrings[Semaphore::PrepareResponse] = "PREPARE_RESPONSE";
	flagStrings[Semaphore::ChangeView] = "CHANGE_VIEW";
	flagStrings[Semaphore::BlockPublished] = "BLOCK_PUBLISHED";
}

void Monitor::display(std::vector<Node*> nodes, std::vector<std::tuple<unsigned, time_t, time_t>>* recentConfirmations) {

	// initialize the console for curses
	initscr();
	raw();

	// window for displaying consensus threads' data 
	int nodesHeight = NUMBER_OF_NODES + 3;
	int nodesWidth = 137;
	WINDOW* nodesWin = newwin(nodesHeight, nodesWidth, 0, 0);
	box(nodesWin, 0, 0);
	mvwprintw(nodesWin, 0, 0, "Consensus Node Local Data ");
	mvwprintw(nodesWin, 1, 1, "ID  Round  View  Role     Responsive  Honest   Activity             Working Hash");
	wrefresh(nodesWin);


	// window to show recently confirmed transactions
	int transactionsWidth = 41;
	WINDOW* transactionsWin = newwin(TRANSACTIONS_TO_SHOW + 3, transactionsWidth, nodesHeight, 0);
	box(transactionsWin, 0, 0);
	mvwprintw(transactionsWin, 0, 0, "Confirmed Transactions ");
	mvwprintw(transactionsWin, 1, 1, "   ID   Published       Confirmed");
	wrefresh(transactionsWin);

	// window to show nodes' message queues
	int messageHeight = nodesHeight - 1;
	int messageWidth = nodesWidth - transactionsWidth;
	WINDOW* messageWin = newwin(messageHeight, messageWidth, nodesHeight, transactionsWidth);
	box(messageWin, 0, 0);
	mvwprintw(messageWin, 0, 0, "Message Queues");
	wrefresh(messageWin);

	// stringstream converts data into C strings required by PDcurses
	std::stringstream ss;

	// window to show simulation settings
	int settingsHeight = nodesHeight + messageHeight;
	int settingsColTwo = nodesWidth / 2 + 2;
	WINDOW* settingsWin = newwin(TRANSACTIONS_TO_SHOW - messageHeight + 3, messageWidth, settingsHeight, transactionsWidth);
	box(settingsWin, 0, 0);
	mvwprintw(settingsWin, 0, 0, "Simulation Parameters ");
	mvwprintw(settingsWin, 1, 1, "Block Size: ");
	mvwprintw(settingsWin, 1, 13, std::to_string(BLOCK_SIZE).c_str());
	mvwprintw(settingsWin, 2, 1, "Block Frequency: ");
	mvwprintw(settingsWin, 2, 18, std::to_string(BLOCK_TIME).c_str());
	mvwprintw(settingsWin, 3, 1, "Transaction Frequency: ");
	mvwprintw(settingsWin, 3, 24, std::to_string(TRANSACTION_FREQUENCY).c_str());
	mvwprintw(settingsWin, 4, 1, "Consensus Nodes: ");
	mvwprintw(settingsWin, 4, 18, std::to_string(NUMBER_OF_NODES).c_str());
	mvwprintw(settingsWin, 5, 1, "Unresponsive Nodes: ");
	mvwprintw(settingsWin, 5, 21, std::to_string(UNRESPONSIVE_NODES).c_str());
	mvwprintw(settingsWin, 6, 1, "Malicious Nodes: ");
	mvwprintw(settingsWin, 6, 18, std::to_string(MALICIOUS_NODES).c_str());
	wrefresh(settingsWin);

	// continuously refresh data
	while (true) {

		try {

			// get most recently confirmed transactions
			Network::r.lock();
			int numberOfTransactions = static_cast<int>(recentConfirmations->size());
			Network::r.unlock();
			for (int i = 0; i < numberOfTransactions; i++) {
				Network::r.lock();
				std::tuple<unsigned, time_t, time_t> transactionData = (*recentConfirmations)[i];
				Network::r.unlock();
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
			for (unsigned i = 0; i < NUMBER_OF_NODES; i++) {
				Node::s.lock();
				auto flags = semaphores[i];
				Node::s.unlock();
				for (int j = 0; j < static_cast<int>(flags.size()); j++) {
					ss << flagStrings[std::get<0>(flags[j])] << "  ";
				}
				mvwprintw(messageWin, i + 1, 1, ss.str().substr(0, messageWidth - 3).c_str());
				ss.str("");
			}
			wrefresh(messageWin);

			// get latest node data 
			for (unsigned i = 0; i < NUMBER_OF_NODES; i++) {
				auto blockHeight = nodes[i]->blockHeight;
				std::string workingHash = "-";
				if (blockHeight > 0) workingHash = nodes[i]->blockchain.back().hash;
				ss << nodes[i]->id;
				ss << "     " << blockHeight;
				ss << "     " << nodes[i]->view;
				ss << "    " << (nodes[i]->speaker ? "SPEAKER " : "DELEGATE");
				ss << "    " << (nodes[i]->responsive ? "TRUE " : "FALSE");
				ss << "    " << (nodes[i]->honest ? "TRUE " : "FALSE");
				ss << "    " << nodes[i]->activity;
				ss << " " << workingHash;
				mvwprintw(nodesWin, i + 2, 1, ss.str().c_str());
				ss.str("");
			}
			wrefresh(nodesWin);

		} catch (...) {}
	}
}
