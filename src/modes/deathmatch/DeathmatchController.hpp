#pragma once

#include "../ModeBase.hpp"
#include "../../core/ModeManager.hpp"
#include "../../core/commands/CommandManager.hpp"
#include "../../core/dialogs/DialogManager.hpp"
#include "../../core/utils/IDPool.hpp"
#include "../../core/utils/ConnectionPool.hpp"
#include "Room.hpp"
#include "Server/Components/Timers/timers.hpp"
#include "WeaponSet.hpp"
#include "textdraws/DeathmatchTimer.hpp"

#include <cstddef>
#include <map>
#include <player.hpp>
#include <eventbus/event_bus.hpp>
#include <pqxx/pqxx>

#include <memory>
#include <unordered_map>
#include <vector>

namespace Modes::Deathmatch
{
inline const std::string MODE_NAME = "deathmatch";
inline const auto DEFAULT_WEAPON_SET = WeaponSet(WeaponSet::Value::Run);
inline const std::string ROOM_INDEX = "roomIndex";

class DeathmatchController : public Modes::ModeBase,
							 public PlayerSpawnEventHandler,
							 public PlayerChangeEventHandler
{
	void initCommand();
	void initRooms();
	void showRoomSelectionDialog(IPlayer& player, bool modeSelection = true);
	void showRoundResultDialog(IPlayer& player, std::shared_ptr<Room> room);
	void showDeathmatchStatsDialog(IPlayer& player, unsigned int id);

	void showRoomCreationDialog(IPlayer& player);
	void showRoomMapSelectionDialog(IPlayer& player);
	void showRoomWeaponSetSelectionDialog(IPlayer& player);
	void showRoomPrivacyModeSelectionDialog(IPlayer& player);
	void showRoomSetCbugEnabledDialog(IPlayer& player);
	void showRoomSetRoundTimeDialog(IPlayer& player);
	void showRoomSetHealthDialog(IPlayer& player);
	void showRoomSetArmorDialog(IPlayer& player);
	void showRoomSetRefillHealthDialog(IPlayer& player);
	void showRoomSetRandomMapDialog(IPlayer& player);
	void createRoom(IPlayer& player);
	void deleteRoom(unsigned int roomId);

	std::shared_ptr<TextDraws::DeathmatchTimer> createDeathmatchTimer(
		IPlayer& player);
	std::optional<std::shared_ptr<TextDraws::DeathmatchTimer>>
	getDeathmatchTimer(IPlayer& player);
	void updateDeathmatchTimer(
		IPlayer& player, unsigned int roomIndex, std::shared_ptr<Room> room);

	void onRoomJoin(IPlayer& player, unsigned int roomId);
	void onRoomLeave(IPlayer& player, unsigned int roomId);
	void onNewRound(std::shared_ptr<Room> room);
	void onRoundEnd(std::shared_ptr<Room> room);

	void setRandomSpawnPoint(IPlayer& player, std::shared_ptr<Room> room);
	void setupRoomForPlayer(IPlayer& player, std::shared_ptr<Room> room);
	void removePlayerFromRoom(IPlayer& player);
	void onTick();

	std::map<unsigned int, std::shared_ptr<Room>> rooms;
	std::unique_ptr<Core::Utils::IDPool> roomIdPool;

	std::weak_ptr<Core::ModeManager> modeManager;
	std::shared_ptr<Core::Commands::CommandManager> commandManager;
	std::shared_ptr<Core::DialogManager> dialogManager;
	std::shared_ptr<Core::Utils::IDPool> virtualWorldIdPool;
	IPlayerPool* _playerPool;
	ITimersComponent* _timersComponent;
	cp::connection_pool& dbPool;

	ITimer* _ticker;

public:
	DeathmatchController(std::weak_ptr<Core::ModeManager> modeManager,
		std::shared_ptr<Core::Commands::CommandManager> commandManager,
		std::shared_ptr<Core::DialogManager> dialogManager,
		IPlayerPool* playerPool, ITimersComponent* timersComponent,
		std::shared_ptr<dp::event_bus> bus, cp::connection_pool& dbPool,
		std::shared_ptr<Core::Utils::IDPool> virtualWorldIdPool);
	virtual ~DeathmatchController();

	void onModeJoin(IPlayer& player,
		std::unordered_map<std::string, Core::PrimitiveType> joinData) override;
	void onModeSelect(IPlayer& player) override;
	void onModeLeave(IPlayer& player) override;
	void onPlayerSave(
		std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn) override;
	void onPlayerLoad(
		std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn) override;

	void onPlayerSpawn(IPlayer& player) override;
	void onPlayerDeath(IPlayer& player, IPlayer* killer, int reason) override;
	void onPlayerKeyStateChange(
		IPlayer& player, uint32_t newKeys, uint32_t oldKeys) override;
	virtual void onPlayerGiveDamage(IPlayer& player, IPlayer& to, float amount,
		unsigned int weapon, BodyPart part) override;

	void onPlayerOnFire(Core::Utils::Events::PlayerOnFireEvent event) override;
	void onPlayerOnFireBeenKilled(
		Core::Utils::Events::PlayerOnFireBeenKilled event) override;
};
}