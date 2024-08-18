#pragma once

#include "../ModeBase.hpp"
#include "../../core/ModeManager.hpp"
#include "../../core/commands/CommandManager.hpp"
#include "../../core/dialogs/DialogManager.hpp"
#include "../../core/utils/IDPool.hpp"
#include "Room.hpp"
#include "Room.tpp"

#include <player.hpp>

#include <map>
#include <memory>
#include <string>

namespace Modes::X1
{
inline const std::string X1_ROOM_INDEX = "roomIndex";
inline const std::string X1_MODE_NAME = "X1";
class X1Controller : public ModeBase, public PlayerSpawnEventHandler
{
	void initCommands();
	void initRooms();
	void createRoom(std::shared_ptr<Room> room);
	void setRandomSpawnPoint(IPlayer& player, std::shared_ptr<Room> room);
	void setupRoomForPlayer(IPlayer& player, std::shared_ptr<Room> room);
	void logStatsForPlayer(IPlayer& player, bool winner, int weapon);

	void showArenaSelectionDialog(IPlayer& player);
	void showX1StatsDialog(IPlayer& player, unsigned int id);

	void onRoomJoin(IPlayer& player, unsigned int roomId);

	std::map<unsigned int, std::shared_ptr<Room>> rooms;
	std::unique_ptr<Core::Utils::IDPool> roomIdPool;

	std::weak_ptr<Core::ModeManager> modeManager;
	std::shared_ptr<Core::Commands::CommandManager> commandManager;
	std::shared_ptr<Core::DialogManager> dialogManager;
	std::shared_ptr<Core::Utils::IDPool> virtualWorldIdPool;
	IPlayerPool* playerPool;
	ITimersComponent* timersComponent;

public:
	X1Controller(std::weak_ptr<Core::ModeManager> modeManager,
		std::shared_ptr<Core::Utils::IDPool> virtualWorldIdPool,
		std::shared_ptr<Core::Commands::CommandManager> commandManager,
		std::shared_ptr<Core::DialogManager> dialogManager,
		IPlayerPool* playerPool, ITimersComponent* timersComponent,
		std::shared_ptr<dp::event_bus> bus);
	virtual ~X1Controller();

	void onPlayerSpawn(IPlayer& player) override;
	void onPlayerDeath(IPlayer& player, IPlayer* killer, int reason) override;

	void onModeSelect(IPlayer& player) override;
	void onModeJoin(IPlayer& player, JoinData joinData) override;
	void onModeLeave(IPlayer& player) override;
	void onPlayerOnFire(Core::Utils::Events::PlayerOnFireEvent event) override;
	void onPlayerOnFireBeenKilled(
		Core::Utils::Events::PlayerOnFireBeenKilled event) override;
	void onPlayerLoad(
		std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn) override;
	void onPlayerSave(
		std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn) override;
};
}