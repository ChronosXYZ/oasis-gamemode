#include "ModeBase.hpp"

#include "../core/player/PlayerExtension.hpp"
#include "../core/utils/Localization.hpp"

#include <player.hpp>
#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>

namespace Modes
{
ModeBase::ModeBase(Mode mode, std::shared_ptr<dp::event_bus> bus)
	: mode(mode)
	, bus(bus)
	, playerOnFireEventRegistration(bus->register_handler<Core::Utils::Events::PlayerOnFireEvent>(this, &ModeBase::onPlayerOnFire))
{
}

ModeBase::~ModeBase()
{
	this->bus->remove_handler(this->playerOnFireEventRegistration);
}

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

void ModeBase::onPlayerOnFire(Core::Utils::Events::PlayerOnFireEvent event)
{
	if (event.mode != this->mode)
		return;
	for (auto player : players)
	{
		auto playerExt = Core::Player::getPlayerExt(*player);
		playerExt->sendTranslatedMessageFormatted(
			__("#LIME#>> #RED#PLAYER ON FIRE#LIGHT_GRAY#: %s(%d) killed %s(%d) and is now on fire!"),
			event.player.getName().to_string(),
			event.player.getID(),
			event.lastKillee.getName().to_string(),
			event.lastKillee.getID());
	}
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