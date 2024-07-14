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
			std::vector<std::string> items;
			for (auto player : this->playersOnFire)
			{
				items.push_back(fmt::sprintf("%s (%d)\n",
					player->getName().to_string(), player->getID()));
			}
			if (items.empty())
			{
				auto dialog = std::shared_ptr<MessageDialog>(
					new MessageDialog(fmt::sprintf(DIALOG_HEADER_TITLE,
										  _("Players on fire", player)),
						_("No players on fire!", player), _("OK", player), ""));
				this->dialogManager->showDialog(player, dialog,
					[](DialogResult result)
					{
					});
			}
			else
			{
				auto dialog = std::shared_ptr<ListDialog>(
					new ListDialog(fmt::sprintf(DIALOG_HEADER_TITLE,
									   _("Players on fire", player)),
						items, _("OK", player), ""));
				this->dialogManager->showDialog(player, dialog,
					[](DialogResult result)
					{
					});
			}
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
	, commandManager(commandManager)
	, dialogManager(dialogManager)
{
	this->playerPool->getPlayerDamageDispatcher().addEventHandler(this);
	this->initCommands();
}

PlayerOnFireController::~PlayerOnFireController()
{
	this->playerPool->getPlayerDamageDispatcher().removeEventHandler(this);
}

void PlayerOnFireController::onPlayerDeath(
	IPlayer& player, IPlayer* killer, int reason)
{
	auto killeeData = Player::getPlayerData(player);
	auto killeeExt = Player::getPlayerExt(player);
	if (this->playersOnFire.contains(&player))
		this->bus->fire_event(
			Utils::Events::PlayerOnFireBeenKilled { .player = player,
				.killer = *killer,
				.mode = killeeExt->getMode() });
	killeeData->tempData->core->subsequentKills = 0;
	player.setWantedLevel(0);
	this->playersOnFire.erase(&player);

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
		this->playersOnFire.emplace(killer);
	}
}

void PlayerOnFireController::onPlayerDisconnect(
	IPlayer& player, PeerDisconnectReason reason)
{
	auto playerExt = Player::getPlayerExt(player);
	this->playersOnFire.erase(&player);
}

}
