#include "ModeBase.hpp"

#include "../core/utils/Localization.hpp"
#include "../core/textdraws/Notification.hpp"
#include "Modes.hpp"

#include <fmt/printf.h>
#include <player.hpp>
#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>

namespace Modes
{
ModeBase::ModeBase(
	Mode mode, std::shared_ptr<dp::event_bus> bus, IPlayerPool* playerPool)
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
	, playerOnFireBeenKilledRegistration(
		  bus->register_handler<Core::Utils::Events::PlayerOnFireBeenKilled>(
			  this, &ModeBase::onPlayerOnFireBeenKilled))
	, playerPool(playerPool)
{
	playerPool->getPlayerDamageDispatcher().addEventHandler(this);
}

ModeBase::~ModeBase()
{
	this->bus->remove_handler(this->playerOnFireEventRegistration);
	this->bus->remove_handler(this->playerOnFireBeenKilledRegistration);
	this->bus->remove_handler(this->x1ArenaWinRegistration);
	playerPool->getPlayerDamageDispatcher().removeEventHandler(this);
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
	this->sendMessageToAll(
		__("#LIME#>> #RED#PLAYER ON FIRE#LIGHT_GRAY#: %s(%d) killed %s(%d) "
		   "and is now on fire!"),
		event.player.getName().to_string(), event.player.getID(),
		event.lastKillee.getName().to_string(), event.lastKillee.getID());
}

void ModeBase::onPlayerOnFireBeenKilled(
	Core::Utils::Events::PlayerOnFireBeenKilled event)
{
	this->sendMessageToAll(
		__("#LIME#>> #RED#PLAYER ON FIRE#LIGHT_GRAY#: %s(%d) "
		   "killed player on fire %s(%d)!"),
		event.killer.getName().to_string(), event.killer.getID(),
		event.player.getName().to_string(), event.player.getID());
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
		if (this->mode == Mode::X1)
			break;
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

void ModeBase::onPlayerGiveDamage(IPlayer& player, IPlayer& to, float amount,
	unsigned int weapon, BodyPart part)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	playerExt->showNotification(
		fmt::sprintf("%s~n~~w~%.1f%%", to.getName().to_string(),
			(to.getArmour() + to.getHealth()) - amount),
		Core::TextDraws::NotificationPosition::Top, 3);
	player.playSound(17802, Vector3(0.0, 0.0, 0.0));
}

unsigned int ModeBase::playerCount() { return this->players.size(); }

const Mode& ModeBase::getModeType() { return this->mode; }
}