#pragma once

#include "../ModeBase.hpp"
#include "../../core/CoreManager.hpp"
#include "../../core/commands/CommandManager.hpp"
#include "../../core/dialogs/DialogManager.hpp"
#include "../../core/utils/IDPool.hpp"
#include "Room.hpp"

#include <player.hpp>

#include <map>
#include <memory>
#include <string>

namespace Modes::Deathmatch
{
inline const unsigned int X1_VIRTUAL_WORLD_PREFIX = 200;
inline const std::string X1_ROOM_INDEX = "roomIndex";
inline const std::string X1_MODE_NAME = "X1";
class X1Controller : public ModeBase,
					 public PlayerSpawnEventHandler,
					 public PlayerDamageEventHandler
{
	X1Controller(std::weak_ptr<Core::CoreManager> coreManager,
		std::shared_ptr<Core::Commands::CommandManager> commandManager,
		std::shared_ptr<Core::DialogManager> dialogManager,
		IPlayerPool* playerPool, ITimersComponent* timersComponent,
		std::shared_ptr<dp::event_bus> bus);

	void initCommands();
	void initRooms();
	void createRoom(std::shared_ptr<Room> room);
	void setRandomSpawnPoint(IPlayer& player, std::shared_ptr<Room> room);
	void setupRoomForPlayer(IPlayer& player, std::shared_ptr<Room> room);

	void showArenaSelectionDialog(IPlayer& player);

	void onRoomJoin(IPlayer& player, unsigned int roomId);

	std::map<unsigned int, std::shared_ptr<Room>> rooms;
	std::unique_ptr<Core::Utils::IDPool> roomIdPool;

	std::weak_ptr<Core::CoreManager> coreManager;
	std::shared_ptr<Core::Commands::CommandManager> commandManager;
	std::shared_ptr<Core::DialogManager> dialogManager;
	IPlayerPool* playerPool;
	ITimersComponent* timersComponent;

public:
	virtual ~X1Controller();

	void onPlayerSpawn(IPlayer& player) override;
	void onPlayerDeath(IPlayer& player, IPlayer* killer, int reason) override;

	void onModeSelect(IPlayer& player) override;
	void onModeJoin(IPlayer& player, JoinData joinData) override;
	void onModeLeave(IPlayer& player) override;

	static X1Controller* create(std::weak_ptr<Core::CoreManager> coreManager,
		std::shared_ptr<Core::Commands::CommandManager> commandManager,
		std::shared_ptr<Core::DialogManager> dialogManager,
		IPlayerPool* playerPool, ITimersComponent* timersComponent,
		std::shared_ptr<dp::event_bus> bus);
};
}