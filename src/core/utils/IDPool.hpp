#pragma once

#include <atomic>
#include <set>

namespace Core::Utils
{
class IDPool
{
	std::atomic<unsigned int> maxId = 0;
	std::set<unsigned int> freeIds;

public:
	unsigned int allocateId();
	void freeId(unsigned int id);
};
}