#include <vector>

#include "Node.h"

#ifndef MONITOR_H
#define MONITOR_H

class Monitor
{
public:
	Monitor(std::vector<std::vector<std::tuple<Semaphore, int, int, int>>>& semaphores);
	void display(std::vector<Node*> nodes, std::vector<std::tuple<unsigned, time_t, time_t>>* recentConfirmations);
private:
	std::vector<std::vector<std::tuple<Semaphore, int, int, int>>>& semaphores;
	std::map<Semaphore, std::string> flagStrings;
};

#endif
