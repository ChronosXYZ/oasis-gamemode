#pragma once

#include "DialogManager.hpp"
#include "auth/AuthController.hpp"
#include "commands/CommandManager.hpp"
#include "player/PlayerModel.hpp"
#include "../modes/Modes.hpp"
#include "utils/ServiceLocator.hpp"

#include <Server/Components/Classes/classes.hpp>
#include <future>
#include <player.hpp>
#include <eventbus/event_bus.hpp>

#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

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
					public PlayerTextEventHandler,
					public PlayerDamageEventHandler
{
public:
	IComponentList* const components;

	static std::shared_ptr<CoreManager> create(
		IComponentList* components, ICore* core, IPlayerPool* playerPool);
	~CoreManager();

	std::shared_ptr<PlayerModel> getPlayerData(IPlayer& player);
	std::shared_ptr<DialogManager> getDialogManager();
	std::shared_ptr<Commands::CommandManager> getCommandManager();
	std::shared_ptr<pqxx::connection> getDBConnection();

	bool refreshPlayerData(IPlayer& player);
	void selectMode(IPlayer& player, Modes::Mode mode);
	void joinMode(IPlayer& player, Modes::Mode mode,
		std::unordered_map<std::string, Core::PrimitiveType> joinData);
	void showModeSelectionDialog(IPlayer& player);

	void onPlayerConnect(IPlayer& player) override;
	void onPlayerDisconnect(
		IPlayer& player, PeerDisconnectReason reason) override;
	bool onPlayerRequestClass(IPlayer& player, unsigned int classId) override;
	bool onPlayerRequestSpawn(IPlayer& player) override;
	bool onPlayerText(IPlayer& player, StringView message) override;
	void onPlayerDeath(IPlayer& player, IPlayer* killer, int reason) override;

	void onPlayerLoggedIn(IPlayer& player);

	void onFree(IComponent* component);

private:
	CoreManager(
		IComponentList* components, ICore* core, IPlayerPool* playerPool);

	void initHandlers();
	void initSkinSelection();
	void savePlayer(std::shared_ptr<PlayerModel> data);
	void savePlayer(IPlayer& player);
	void saveAllPlayers();
	void removePlayerFromCurrentMode(IPlayer& player);
	template <typename... T>
	void sendNotificationToAllFormatted(
		const std::string& message, const T&... args);
	void runSaveThread(std::future<void> exitSignal);

	IPlayerPool* const _playerPool = nullptr;
	ICore* const _core = nullptr;
	IClassesComponent* const _classesComponent;

	std::shared_ptr<dp::event_bus> bus;

	std::shared_ptr<Commands::CommandManager> _commandManager;
	std::shared_ptr<DialogManager> _dialogManager;
	std::shared_ptr<pqxx::connection> _dbConnection;
	std::map<unsigned int, std::shared_ptr<PlayerModel>> _playerData;
	std::thread saveThread;
	std::promise<void> saveThreadExitSignal;

	// Controllers
	std::unique_ptr<Auth::AuthController> _authController;
	std::unique_ptr<ServiceLocator> _modes;
	std::unique_ptr<ServiceLocator> _playerControllers;
};
}