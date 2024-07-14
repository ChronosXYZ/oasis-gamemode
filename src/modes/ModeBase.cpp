#include "ModeBase.hpp"

#include "../core/player/PlayerExtension.hpp"
#include "../core/utils/Localization.hpp"
#include "Modes.hpp"

#include <player.hpp>
#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>

namespace Modes
{
ModeBase::ModeBase(Mode mode, std::shared_ptr<dp::event_bus> bus)
	: mode(mode)
	, bus(bus)
	, playerOnFireEventRegistration(
		  bus->register_handler<Core::Utils::Events::PlayerOnFireEvent>(
			  this, &ModeBase::onPlayerOnFire))
	, x1ArenaWinRegistration(
		  bus->register_handler<Core::Utils::Events::X1ArenaWin>(
			  this, &ModeBase::onX1ArenaWin))
	, playerJoinedModeSubsciption(
		  bus->register_handler<Core::Utils::Events::PlayerJoinedMode>(
			  this, &ModeBase::onPlayerJoinedMode))
{
}

ModeBase::~ModeBase()
{
	this->bus->remove_handler(this->playerOnFireEventRegistration);
	this->bus->remove_handler(this->x1ArenaWinRegistration);
}

void ModeBase::onModeJoin(IPlayer& player, JoinData joinData)
{
	spdlog::info("Player {} has joined mode {}", player.getName().to_string(),
		magic_enum::enum_name(mode));
	this->players.emplace(&player);
	this->bus->fire_event(Core::Utils::Events::PlayerJoinedMode {
		.player = player, .mode = this->mode, .joinData = joinData });
}

void ModeBase::onModeLeave(IPlayer& player)
{
	this->players.erase(&player);
	spdlog::info("Player {} has left mode {}", player.getName().to_string(),
		magic_enum::enum_name(this->mode));
}

void ModeBase::onPlayerSave(
	std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn)
{
}

void ModeBase::onPlayerLoad(
	std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn)
{
}

void ModeBase::onPlayerOnFire(Core::Utils::Events::PlayerOnFireEvent event)
{
	if (event.mode != this->mode)
		return;
	for (auto player : players)
	{
		auto playerExt = Core::Player::getPlayerExt(*player);
		playerExt->sendTranslatedMessageFormatted(
			__("#LIME#>> #RED#PLAYER ON FIRE#LIGHT_GRAY#: %s(%d) killed %s(%d) "
			   "and is now on fire!"),
			event.player.getName().to_string(), event.player.getID(),
			event.lastKillee.getName().to_string(), event.lastKillee.getID());
	}
}

void ModeBase::onX1ArenaWin(Core::Utils::Events::X1ArenaWin event)
{
	this->sendMessageToAll(
		__("#LIME#>> #RED#X1#WHITE#: %s(%d) "
		   "has defeated %s(%d) with %.1f health and %.1f armour left!"),
		event.winner.getName().to_string(), event.winner.getID(),
		event.loser.getName().to_string(), event.loser.getID(),
		event.healthLeft, event.armourLeft);
}

void ModeBase::onPlayerJoinedMode(Core::Utils::Events::PlayerJoinedMode event)
{
	switch (event.mode)
	{
	case Mode::X1:
	{
		this->sendMessageToAll(__("#LIME#>> #RED#X1#LIGHT_GRAY#: %s(%d) "
								  "has joined X1 arena (/x1)!"),
			event.player.getName().to_string(), event.player.getID());
		break;
	}
	default:
	{
		break;
	}
	}
}

unsigned int ModeBase::playerCount() { return this->players.size(); }

const Mode& ModeBase::getModeType() { return this->mode; }
}