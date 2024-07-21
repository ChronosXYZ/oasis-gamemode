#pragma once

#include "../ModeBase.hpp"
#include "../../core/CoreManager.hpp"
#include "../../core/commands/CommandManager.hpp"
#include "../../core/dialogs/DialogManager.hpp"
#include "../../core/utils/IDPool.hpp"
#include "DuelOffer.hpp"
#include "Room.hpp"
#include "Room.tpp"

#include <functional>
#include <player.hpp>

#include <map>
#include <memory>
#include <string>

namespace Modes::Duel
{
inline const std::string DUEL_ROOM_INDEX = "roomIndex";
inline const std::string DUEL_MODE_NAME = "Duel";
class DuelController : public ModeBase, public PlayerSpawnEventHandler
{
	DuelController(std::weak_ptr<Core::CoreManager> coreManager,
		std::shared_ptr<Core::Commands::CommandManager> commandManager,
		std::shared_ptr<Core::DialogManager> dialogManager,
		IPlayerPool* playerPool, ITimersComponent* timersComponent,
		std::shared_ptr<dp::event_bus> bus);

	void initCommands();
	void setRandomSpawnPoint(IPlayer& player, std::shared_ptr<Room> room);
	void setupRoomForPlayer(IPlayer& player, std::shared_ptr<Room> room);
	void logStatsForPlayer(IPlayer& player, bool winner, int weapon);
	void createDuelOffer(IPlayer& player);

	void showDuelStatsDialog(IPlayer& player, unsigned int id);
	void showDuelCreationDialog(IPlayer& player);
	void showDuelMapSelectionDialog(IPlayer& player);
	void showDuelWeaponSelectionDialog(IPlayer& player);
	void showDuelRoundCountSettingDialog(IPlayer& player);
	void showDuelAcceptListDialog(IPlayer& player);
	void showDuelAcceptConfirmDialog(
		IPlayer& player, std::shared_ptr<DuelOffer> offer);

	// Commands
	void createDuel(IPlayer& player, int id);

	void onRoomJoin(IPlayer& player, unsigned int roomId);
	void onRoundEnd(IPlayer& winner, IPlayer& loser, int weaponId);
	void onDuelEnd(std::shared_ptr<Room> duelRoom);

	std::map<unsigned int, std::shared_ptr<Room>> rooms;
	std::unique_ptr<Core::Utils::IDPool> roomIdPool;

	std::weak_ptr<Core::CoreManager> coreManager;
	std::shared_ptr<Core::Commands::CommandManager> commandManager;
	std::shared_ptr<Core::DialogManager> dialogManager;
	IPlayerPool* playerPool;
	ITimersComponent* timersComponent;

public:
	virtual ~DuelController();

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

	static DuelController* create(std::weak_ptr<Core::CoreManager> coreManager,
		std::shared_ptr<Core::Commands::CommandManager> commandManager,
		std::shared_ptr<Core::DialogManager> dialogManager,
		IPlayerPool* playerPool, ITimersComponent* timersComponent,
		std::shared_ptr<dp::event_bus> bus);
};
}