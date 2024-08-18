#include "ModeManager.hpp"
#include <stdexcept>

namespace Core
{

void ModeManager::removePlayerFromCurrentMode(IPlayer& player)
{
	auto mode = Player::getPlayerExt(player)->getMode();
	if (mode == Modes::Mode::None)
		return;
	std::shared_ptr<Modes::ModeBase> modeBase;

	try
	{
		this->modes.at(mode)->onModeLeave(player);
	}
	catch (const std::out_of_range& e)
	{
		return;
	}
}

ModeManager::ModeManager(std::shared_ptr<DialogManager> dialogManager)
	: dialogManager(dialogManager)
{
}

void ModeManager::selectMode(IPlayer& player, Modes::Mode mode)
{
	try
	{
		this->modes.at(mode)->onModeSelect(player);
	}
	catch (const std::out_of_range& e)
	{
		Player::getPlayerExt(player)->sendErrorMessage(
			__("Mode is not implemented yet!"));
		return;
	}
}

bool ModeManager::joinMode(
	IPlayer& player, Modes::Mode mode, Modes::JoinData joinData)
{
	auto playerData = Player::getPlayerData(player);
	auto playerExt = Player::getPlayerExt(player);
	if (playerData->tempData->core->isDying)
	{
		playerExt->sendErrorMessage(__("You cannot join a mode while dying"));
		return false;
	}
	this->removePlayerFromCurrentMode(player);

	playerData->tempData->core->lastMode
		= playerData->tempData->core->currentMode;
	playerData->tempData->core->currentMode = mode;

	try
	{
		this->modes.at(mode)->onModeJoin(player, joinData);
	}
	catch (const std::out_of_range& e)
	{
		Player::getPlayerExt(player)->sendErrorMessage(
			__("Mode is not implemented yet!"));
		return false;
	}

	return true;
}

void ModeManager::addMode(std::unique_ptr<Modes::ModeBase> mode)
{
	this->modes[mode->getModeType()] = std::move(mode);
}

void ModeManager::savePlayer(std::shared_ptr<PlayerModel> data, pqxx::work& txn)
{
	for (const auto& [_, mode] : this->modes)
	{
		mode->onPlayerSave(data, txn);
	}
}

void ModeManager::loadPlayerData(
	std::shared_ptr<PlayerModel> data, pqxx::work& txn)
{
	for (const auto& [_, mode] : this->modes)
	{
		mode->onPlayerLoad(data, txn);
	}
}

void ModeManager::showModeSelectionDialog(IPlayer& player)
{
	auto dialog = std::shared_ptr<TabListHeadersDialog>(
		new TabListHeadersDialog(_("Modes", player),
			{ _("Mode", player), _("Command", player), _("Players", player) },
			{ { _("Freeroam", player), "/fr",
				  std::to_string(modes[Modes::Mode::Freeroam]->playerCount()) },
				{ _("Deathmatch", player), "/dm",
					std::to_string(
						modes[Modes::Mode::Deathmatch]->playerCount()) },
				{ _("Protect the President", player), "/ptp",
					std::to_string(0) },
				{ _("Derby", player), "/derby", std::to_string(0) },
				{ _("Cops and Robbers", player), "/cnr", std::to_string(0) } },
			_("Select", player), ""));
	this->dialogManager->showDialog(player, dialog,
		[&](DialogResult result)
		{
			this->selectMode(
				player, static_cast<Modes::Mode>(result.listItem()));
		});
}
}