#pragma once

#include "../ModeBase.hpp"
#include "../../core/ModeManager.hpp"
#include "../../core/commands/CommandManager.hpp"
#include "../../core/dialogs/DialogManager.hpp"
#include "../../core/utils/IDPool.hpp"
#include "DuelOffer.hpp"
#include "Room.hpp"
#include "Room.tpp"

#include <array>
#include <functional>
#include <player.hpp>

#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace Modes::Duel
{
inline const std::string DUEL_ROOM_ID = "roomId";
inline const std::string DUEL_MODE_NAME = "Duel";

inline const std::array<std::string, 21> ROUND_START_TEXT
	= { __("Kill him!"), __("Blast his ass!"), __("Humiliate him!"),
		  __("Kickass!"), __("GOOO!"), __("Drop him!"), __("Slaughter!"),
		  __("BAMBOOZLE HIM!"), __("REKT HIM!"), __("SHREKT HIM!"),
		  __("Murder!"), __("Kill him!"), __("Get him!"), __("Waste his ass!"),
		  __("Smash him!"), __("Smoke him!"), __("RAGE!"), __("NO MERCY!"),
		  __("ITS WAR!"), __("PUMP HIM!"), __("Show him who's boss!") };

class DuelController : public ModeBase,
					   public PlayerSpawnEventHandler,
					   public PlayerConnectEventHandler
{
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
	void showDuelResults(std::shared_ptr<Room> room);

	// Commands
	void createDuel(IPlayer& player, int id);
	unsigned int createDuelRoom(std::shared_ptr<DuelOffer> offer);
	void deleteDuel(unsigned int id, IPlayer* initiator = nullptr);

	void onRoomJoin(IPlayer& player, unsigned int roomId);
	void onRoundEnd(IPlayer* winner, IPlayer* loser, int weaponId);
	void onDuelEnd(std::shared_ptr<Room> duelRoom);

	void deleteDuelOfferFromPlayer(IPlayer& player, bool deleteRoom);

	std::map<unsigned int, std::shared_ptr<Room>> rooms;
	std::unique_ptr<Core::Utils::IDPool> roomIdPool;

	std::weak_ptr<Core::ModeManager> modeManager;
	std::shared_ptr<Core::Commands::CommandManager> commandManager;
	std::shared_ptr<Core::DialogManager> dialogManager;
	std::shared_ptr<Core::Utils::IDPool> virtualWorldIdPool;
	IPlayerPool* playerPool;
	ITimersComponent* timersComponent;

public:
	DuelController(std::weak_ptr<Core::ModeManager> modeManager,
		std::shared_ptr<Core::Commands::CommandManager> commandManager,
		std::shared_ptr<Core::DialogManager> dialogManager,
		IPlayerPool* playerPool, ITimersComponent* timersComponent,
		std::shared_ptr<dp::event_bus> bus,
		std::shared_ptr<Core::Utils::IDPool> virtualWorldIdPool);
	virtual ~DuelController();

	void onPlayerSpawn(IPlayer& player) override;
	void onPlayerDeath(IPlayer& player, IPlayer* killer, int reason) override;
	void onPlayerGiveDamage(IPlayer& player, IPlayer& to, float amount,
		unsigned int weapon, BodyPart part) override;
	void onPlayerDisconnect(
		IPlayer& player, PeerDisconnectReason reason) override;

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