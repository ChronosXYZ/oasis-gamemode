#pragma once

#include "ModeManager.hpp"
#include "dialogs/DialogManager.hpp"
#include "auth/AuthController.hpp"
#include "commands/CommandManager.hpp"
#include "player/PlayerModel.hpp"
#include "utils/ConnectionPool.hpp"
#include "utils/IDPool.hpp"
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
					public ClassEventHandler,
					public PlayerSpawnEventHandler,
					public PlayerTextEventHandler,
					public PlayerDamageEventHandler
{
public:
	IComponentList* const components;

	static std::unique_ptr<CoreManager> create(IComponentList* components,
		ICore* core, IPlayerPool* playerPool,
		const std::string& db_connection_string);
	~CoreManager();

	void onPlayerConnect(IPlayer& player) override;
	void onPlayerDisconnect(
		IPlayer& player, PeerDisconnectReason reason) override;
	bool onPlayerRequestClass(IPlayer& player, unsigned int classId) override;
	bool onPlayerRequestSpawn(IPlayer& player) override;
	void onPlayerSpawn(IPlayer& player) override;
	bool onPlayerText(IPlayer& player, StringView message) override;
	void onPlayerDeath(IPlayer& player, IPlayer* killer, int reason) override;

private:
	CoreManager(IComponentList* components, ICore* core,
		IPlayerPool* playerPool, const std::string& connection_string);

	void initHandlers();
	void initSkinSelection();
	void savePlayer(std::shared_ptr<PlayerModel> data);
	void savePlayer(IPlayer& player);
	void saveAllPlayers();
	void runSaveThread(std::future<void> exitSignal);

	IPlayerPool* const playerPool = nullptr;
	ICore* const _core = nullptr;
	IClassesComponent* const _classesComponent;

	std::shared_ptr<dp::event_bus> bus;

	std::shared_ptr<Commands::CommandManager> _commandManager;
	std::shared_ptr<DialogManager> _dialogManager;
	cp::connection_pool connectionPool;
	std::thread saveThread;
	std::promise<void> saveThreadExitSignal;
	std::shared_ptr<Utils::IDPool> virtualWorldIdPool;
	std::shared_ptr<ModeManager> modeManager;

	// Controllers
	std::unique_ptr<Auth::AuthController> _authController;
	std::unique_ptr<ServiceLocator> _modes;
	std::unique_ptr<ServiceLocator> _playerControllers;
};
}