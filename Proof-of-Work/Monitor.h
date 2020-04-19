#include <vector>

#include "Node.h"
#include "Network.h"

#ifndef MONITOR_H
#define MONITOR_H

class Monitor
{
public:
	Monitor(std::vector<std::vector<std::tuple<Semaphore, int, int>>>& semaphores);
	void display(std::vector<Node*> nodes, 
		Network* network);
private:
	std::vector<std::vector<std::tuple<Semaphore, int, int>>>& semaphores;
	std::map<Semaphore, std::string> flagStrings;
};

#endif
