#include "IDPool.hpp"
#include <cstddef>

namespace Core::Utils
{
unsigned int IDPool::allocateId()
{
	if (!this->freeIds.empty())
	{
		auto firstUnusedId = this->freeIds.begin();
		auto id = *firstUnusedId;
		this->freeIds.erase(firstUnusedId);
		return id;
	}
	return this->maxId++;
}

void IDPool::freeId(unsigned int id)
{
	if (id < this->maxId)
		this->freeIds.insert(id);
}
}