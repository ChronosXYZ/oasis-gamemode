#include "ModeBase.hpp"

#include "../core/utils/Localization.hpp"
#include "../core/utils/Common.hpp"
#include "Modes.hpp"
#include "deathmatch/DeathmatchController.hpp"

#include <fmt/printf.h>
#include <format>
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
	, duelWinRegistration(bus->register_handler<Core::Utils::Events::DuelWin>(
		  this, &ModeBase::onDuelWin))
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
	// spdlog::info("Player {} has left mode {}", player.getName().to_string(),
	// magic_enum::enum_name(this->mode));
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
		   "and is now on fire"),
		event.player.getName().to_string(), event.player.getID(),
		event.lastKillee.getName().to_string(), event.lastKillee.getID());
}

void ModeBase::onPlayerOnFireBeenKilled(
	Core::Utils::Events::PlayerOnFireBeenKilled event)
{
	this->sendMessageToAll(
		__("#LIME#>> #RED#PLAYER ON FIRE#LIGHT_GRAY#: %s(%d) "
		   "killed player on fire %s(%d)"),
		event.killer.getName().to_string(), event.killer.getID(),
		event.player.getName().to_string(), event.player.getID());
}

void ModeBase::onX1ArenaWin(Core::Utils::Events::X1ArenaWin event)
{
	this->sendMessageToAll(__("#LIME#>> #RED#X1#WHITE#: %s(%d) "
							  "has defeated %s(%d) with %s (%.1f HP, %.1f AP, "
							  "distance: %.1f, time: %s)"),
		event.winner.getName().to_string(), event.winner.getID(),
		event.loser.getName().to_string(), event.loser.getID(),
		Core::Utils::getWeaponName(event.weapon), event.healthLeft,
		event.armourLeft, event.distance,
		std::format("{:%OM:%OS}", event.fightDuration));
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
								  "has joined X1 arena (/x1)"),
			event.player.getName().to_string(), event.player.getID());
		break;
	}
	case Mode::Deathmatch:
	{
		if (this->mode == Mode::Deathmatch)
			break;
		this->sendMessageToAll(
			__("#LIME#>> #DEEP_SAFFRON#DM#LIGHT_GRAY#: %s(%d) "
			   "has joined DM mode (/dm %d)"),
			event.player.getName().to_string(), event.player.getID(),
			std::get<unsigned int>(event.joinData[Deathmatch::ROOM_INDEX]) + 1);
		break;
	}
	default:
	{
		break;
	}
	}
}

void ModeBase::onDuelWin(Core::Utils::Events::DuelWin event)
{
	for (auto player : this->players)
	{
		auto winnerExt = Core::Player::getPlayerExt(event.winner);
		auto loserExt = Core::Player::getPlayerExt(event.loser);
		this->sendModeMessage(*player,
			__("{%06x}%s(%d)#WHITE# "
			   "has defeated {%06x}%s(%d)#WHITE# (score: %d-%d, time: %s)"),
			winnerExt->getNormalizedColor(), event.winner.getName().to_string(),
			event.winner.getID(), loserExt->getNormalizedColor(),
			event.loser.getName().to_string(), event.loser.getID(),
			event.winnerScore, event.loserScore,
			std::format("{:%OM:%OS}", event.fightDuration));
	}
}

void ModeBase::onPlayerGiveDamage(IPlayer& player, IPlayer& to, float amount,
	unsigned int weapon, BodyPart part)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	playerExt->showNotification(
		fmt::sprintf("%s(%d)~n~~w~%.1f%%", to.getName().to_string(), to.getID(),
			(to.getArmour() + to.getHealth()) - amount), 3);
	player.playSound(17802, Vector3(0.0, 0.0, 0.0));
}

unsigned int ModeBase::playerCount() { return this->players.size(); }

const Mode& ModeBase::getModeType() { return this->mode; }
}