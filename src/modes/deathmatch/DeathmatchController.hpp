#pragma once

#include "../../modes/ModeBase.hpp"
#include "../../core/CoreManager.hpp"
#include "Room.hpp"
#include "Server/Components/Timers/timers.hpp"
#include "WeaponSet.hpp"
#include "textdraws/DeathmatchTimer.hpp"

#include <cstddef>
#include <player.hpp>
#include <eventbus/event_bus.hpp>
#include <pqxx/pqxx>

#include <memory>
#include <unordered_map>
#include <vector>

namespace Modes::Deathmatch
{
inline const unsigned int VIRTUAL_WORLD_PREFIX = 100;
inline const std::string MODE_NAME = "deathmatch";
inline const auto DEFAULT_WEAPON_SET = WeaponSet(WeaponSet::Value::Run);

class DeathmatchController : public Modes::ModeBase,
							 public PlayerDamageEventHandler,
							 public PlayerSpawnEventHandler,
							 public PlayerChangeEventHandler
{
	DeathmatchController(std::weak_ptr<Core::CoreManager> coreManager,
		std::shared_ptr<Core::Commands::CommandManager> commandManager,
		std::shared_ptr<Core::DialogManager> dialogManager,
		IPlayerPool* playerPool, ITimersComponent* timersComponent,
		std::shared_ptr<dp::event_bus> bus,
		std::shared_ptr<pqxx::connection> dbConnection);

	void initCommand();
	void initRooms();
	void showRoomSelectionDialog(IPlayer& player, bool modeSelection = true);
	void showRoundResultDialog(IPlayer& player, std::shared_ptr<Room> room);

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

	std::shared_ptr<TextDraws::DeathmatchTimer> createDeathmatchTimer(
		IPlayer& player);
	std::optional<std::shared_ptr<TextDraws::DeathmatchTimer>>
	getDeathmatchTimer(IPlayer& player);
	void updateDeathmatchTimer(
		IPlayer& player, std::size_t roomIndex, std::shared_ptr<Room> room);

	void onRoomJoin(IPlayer& player, std::size_t roomId);
	void onNewRound(std::shared_ptr<Room> room);
	void onRoundEnd(std::shared_ptr<Room> room);

	void setRandomSpawnPoint(IPlayer& player, std::shared_ptr<Room> room);
	void setupRoomForPlayer(IPlayer& player, std::shared_ptr<Room> room);
	void removePlayerFromRoom(IPlayer& player);
	void onTick();

	std::vector<std::shared_ptr<Room>> _rooms;
	std::unordered_map<std::string, ITimer*> _cbugFreezeTimers;

	std::weak_ptr<Core::CoreManager> coreManager;
	std::shared_ptr<Core::Commands::CommandManager> commandManager;
	std::shared_ptr<Core::DialogManager> dialogManager;
	IPlayerPool* _playerPool;
	ITimersComponent* _timersComponent;
	std::shared_ptr<pqxx::connection> dbConnection;

	ITimer* _ticker;

public:
	virtual ~DeathmatchController();
	void onModeJoin(IPlayer& player,
		std::unordered_map<std::string, Core::PrimitiveType> joinData) override;
	void onModeSelect(IPlayer& player) override;
	void onModeLeave(IPlayer& player) override;
	void onPlayerSave(IPlayer& player, pqxx::work& txn) override;
	void onPlayerLoad(IPlayer& player, pqxx::work& txn) override;

	void onPlayerSpawn(IPlayer& player) override;
	void onPlayerDeath(IPlayer& player, IPlayer* killer, int reason) override;
	void onPlayerKeyStateChange(
		IPlayer& player, uint32_t newKeys, uint32_t oldKeys) override;
	void onPlayerGiveDamage(IPlayer& player, IPlayer& to, float amount,
		unsigned int weapon, BodyPart part) override;

	void onPlayerOnFire(Core::Utils::Events::PlayerOnFireEvent event) override;

	static DeathmatchController* create(
		std::weak_ptr<Core::CoreManager> coreManager,
		std::shared_ptr<Core::Commands::CommandManager> commandManager,
		std::shared_ptr<Core::DialogManager> dialogManager,
		IPlayerPool* playerPool, ITimersComponent* timersComponent,
		std::shared_ptr<dp::event_bus> bus,
		std::shared_ptr<pqxx::connection> dbConnection);
};
}