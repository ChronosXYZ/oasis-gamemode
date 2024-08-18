#pragma once

#include "../modes/Modes.hpp"
#include "../modes/ModeBase.hpp"
#include "dialogs/DialogManager.hpp"
#include "player.hpp"
#include "player/PlayerModel.hpp"

#include <memory>
#include <unordered_map>

namespace Core
{
class ModeManager
{
	std::unordered_map<Modes::Mode, std::unique_ptr<Modes::ModeBase>> modes;

	std::shared_ptr<DialogManager> dialogManager;

public:
	ModeManager(std::shared_ptr<DialogManager> dialogManager);

	void selectMode(IPlayer& player, Modes::Mode mode);
	bool joinMode(
		IPlayer& player, Modes::Mode mode, Modes::JoinData joinData = {});
	void addMode(std::unique_ptr<Modes::ModeBase> mode);
	void savePlayer(std::shared_ptr<PlayerModel> data, pqxx::work& txn);
	void loadPlayerData(std::shared_ptr<PlayerModel> data, pqxx::work& txn);
	void showModeSelectionDialog(IPlayer& player);
	void removePlayerFromCurrentMode(IPlayer& player);
};
}