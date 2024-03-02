#pragma once

#include "DialogManager.hpp"
#include "auth/AuthController.hpp"
#include "commands/CommandManager.hpp"
#include "player/PlayerModel.hpp"
#include "../modes/Modes.hpp"
#include "utils/ServiceLocator.hpp"

#include <Server/Components/Classes/classes.hpp>

#include <memory>
#include <player.hpp>

#define ACCENT_MAIN 0xFF0000FF // SERVER COLOR
#define ACCENT_SECONDARY 0xFFFFFFFF // SERVER COLOR
#define ACCENT_MAIN_E "{FF0000}" // SERVER COLOR Embedded
#define ACCENT_SECONDARY_E "{FFFFFF}" // SERVER COLOR Embedded

#define DIALOG_HEADER_TITLE "" ACCENT_MAIN_E "Oasis " ACCENT_SECONDARY_E " | %s"
#define DIALOG_TABLIST_TITLE_COLOR ACCENT_MAIN_E // Embedded
#define DIALOG_HEADER DIALOG_HEADER_TITLE // embedded
#define DIALOG_TABLIST DIALOG_TABLIST_TITLE_COLOR

namespace Core
{
using namespace std::string_literals;

inline const auto CLASS_SELECTION_POINTS = std::to_array({
	Vector4(1094.55, -2037.11, 82.75, 235.84) // LS beach
});

inline const auto GENERAL_COMMAND_CATEGORY = "general"s;

inline const auto CHAT_BUBBLE_EXPIRATION = 10000;

class CoreManager : public PlayerConnectEventHandler,
					public std::enable_shared_from_this<CoreManager>,
					public ClassEventHandler,
					public PlayerSpawnEventHandler,
					public PlayerTextEventHandler
{
public:
	IComponentList* const components;

	static std::shared_ptr<CoreManager> create(IComponentList* components, ICore* core, IPlayerPool* playerPool);
	~CoreManager();

	std::shared_ptr<PlayerModel> getPlayerData(IPlayer& player);
	std::shared_ptr<DialogManager> getDialogManager();
	std::shared_ptr<Commands::CommandManager> getCommandManager();
	std::shared_ptr<pqxx::connection> getDBConnection();

	bool refreshPlayerData(IPlayer& player);
	void selectMode(IPlayer& player, Modes::Mode mode);
	void showModeSelectionDialog(IPlayer& player);

	void onPlayerConnect(IPlayer& player) override;
	void onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason) override;
	bool onPlayerRequestClass(IPlayer& player, unsigned int classId) override;
	bool onPlayerRequestSpawn(IPlayer& player) override;
	bool onPlayerText(IPlayer& player, StringView message) override;

	void onPlayerLoggedIn(IPlayer& player);

	void onFree(IComponent* component);

private:
	CoreManager(IComponentList* components, ICore* core, IPlayerPool* playerPool);

	void initHandlers();
	void initSkinSelection();
	void savePlayer(IPlayer& player);
	void savePlayer(std::shared_ptr<PlayerModel> data);
	void saveAllPlayers();
	void removePlayerFromModes(IPlayer& player);

	IPlayerPool* const _playerPool = nullptr;
	ICore* const _core = nullptr;
	IClassesComponent* const _classesComponent;

	std::shared_ptr<Commands::CommandManager> _commandManager;
	std::shared_ptr<DialogManager> _dialogManager;
	std::shared_ptr<pqxx::connection> _dbConnection;
	std::map<unsigned int, std::shared_ptr<PlayerModel>> _playerData;
	std::map<Modes::Mode, std::set<unsigned int>> _modePlayerCount {
		{ Modes::Mode::Freeroam, {} },
		{ Modes::Mode::Deathmatch, {} },
		{ Modes::Mode::Derby, {} },
		{ Modes::Mode::PTP, {} },
		{ Modes::Mode::CnR, {} }
	};

	// Controllers
	std::unique_ptr<Auth::AuthController> _authController;
	std::unique_ptr<ServiceLocator> _modes;
	std::unique_ptr<ServiceLocator> _playerControllers;
};
}