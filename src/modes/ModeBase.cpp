#include "ModeBase.hpp"
#include "player.hpp"
#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>

namespace Modes
{
void ModeBase::onModeJoin(IPlayer& player, std::unordered_map<std::string, Core::PrimitiveType> joinData)
{
	spdlog::info("Player {} has joined mode {}", player.getName().to_string(), magic_enum::enum_name(mode));
	this->players.emplace(&player);
}

void ModeBase::onModeLeave(IPlayer& player)
{
	this->players.erase(&player);
	spdlog::info("Player {} has left mode {}", player.getName().to_string(), magic_enum::enum_name(this->mode));
}

unsigned int ModeBase::playerCount()
{
	return this->players.size();
}

const Mode& ModeBase::getModeType()
{
	return this->mode;
}
}