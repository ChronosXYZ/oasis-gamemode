#include "PlayerOnFireController.hpp"

#include "../player/PlayerExtension.hpp"
#include "../utils/Events.hpp"

#include <fmt/printf.h>
#include <memory>
#include <string>
#include <vector>

namespace Core::Controllers
{
void PlayerOnFireController::initCommands()
{
	this->commandManager->addCommand(
		"onfire",
		[this](std::reference_wrapper<IPlayer> player)
		{
			std::string body = _("Players on fire:\n", player);
			bool exists = false;
			for (auto& playersPerMode : this->playersOnFire)
			{
				for (auto& player : playersPerMode.second)
				{
					body += fmt::sprintf("- %s (%d)\n",
						player->getName().to_string(), player->getID());
					exists = true;
				}
			}
			if (!exists)
			{
				body += _("None", player);
			}
			this->dialogManager->createDialog(player, DialogStyle_MSGBOX,
				fmt::sprintf(DIALOG_HEADER_TITLE, _("Players on fire", player)),
				body, "OK", "",
				[](DialogResponse response, int listItem, StringView input)
				{
				});
		},
		Commands::CommandInfo {
			.args = {},
			.description = __("Show all players on fire"),
			.category = "general",
		});
}

PlayerOnFireController::PlayerOnFireController(IPlayerPool* playerPool,
	std::shared_ptr<dp::event_bus> bus,
	std::shared_ptr<Commands::CommandManager> commandManager,
	std::shared_ptr<DialogManager> dialogManager)
	: bus(bus)
	, playerPool(playerPool)
	, roundEndRegistration(bus->register_handler<Utils::Events::RoundEndEvent>(
		  this, &PlayerOnFireController::onRoundEnd))
	, commandManager(commandManager)
	, dialogManager(dialogManager)
{
	this->playerPool->getPlayerDamageDispatcher().addEventHandler(this);
	this->initCommands();
}

PlayerOnFireController::~PlayerOnFireController()
{
	this->playerPool->getPlayerDamageDispatcher().removeEventHandler(this);
	this->bus->remove_handler(this->roundEndRegistration);
}

void PlayerOnFireController::onPlayerDeath(
	IPlayer& player, IPlayer* killer, int reason)
{
	auto killeeData = Player::getPlayerData(player);
	auto killeeExt = Player::getPlayerExt(player);
	killeeData->tempData->core->subsequentKills = 0;
	player.setWantedLevel(0);
	this->playersOnFire[killeeExt->getMode()].erase(&player);

	if (killer == nullptr)
		return;
	auto killerData = Player::getPlayerData(*killer);
	auto killerExt = Player::getPlayerExt(*killer);
	auto kills = ++killerData->tempData->core->subsequentKills;
	killer->setWantedLevel(kills);
	if (kills == 6)
	{
		this->bus->fire_event(
			Utils::Events::PlayerOnFireEvent { .player = *killer,
				.lastKillee = player,
				.mode = killerExt->getMode() });
	}
	this->playersOnFire[killerExt->getMode()].emplace(killer);
}

void PlayerOnFireController::onPlayerDisconnect(
	IPlayer& player, PeerDisconnectReason reason)
{
	auto playerExt = Player::getPlayerExt(player);
	this->playersOnFire[playerExt->getMode()].erase(&player);
}

void PlayerOnFireController::onModeLeave(IPlayer& player, Modes::Mode mode)
{
	auto playerData = Player::getPlayerData(player);
	playerData->tempData->core->subsequentKills = 0;
	player.setWantedLevel(0);
	this->playersOnFire[mode].erase(&player);
}

void PlayerOnFireController::onRoundEnd(Utils::Events::RoundEndEvent event)
{
	for (auto player : event.players)
	{
		auto playerData = Player::getPlayerData(*player);
		playerData->tempData->core->subsequentKills = 0;
		player->setWantedLevel(0);
		this->playersOnFire[event.mode].erase(player);
	}
}
}
