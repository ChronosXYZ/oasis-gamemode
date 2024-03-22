#pragma once

#include "../../modes/IMode.hpp"
#include "../../core/CoreManager.hpp"
#include "Room.hpp"
#include "Server/Components/Timers/timers.hpp"

#include <cstddef>
#include <player.hpp>

#include <memory>
#include <unordered_map>
#include <vector>

namespace Modes::Deathmatch
{
inline const unsigned int VIRTUAL_WORLD_PREFIX = 100;
inline const std::string MODE_NAME = "deathmatch";

class DeathmatchController : public Modes::IMode,
							 public PlayerDamageEventHandler,
							 public PlayerSpawnEventHandler,
							 public PlayerChangeEventHandler
{
	DeathmatchController(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool, ITimersComponent* timersComponent);

	void initCommand();
	void initRooms();
	void showRoomSelectionDialog(IPlayer& player, bool modeSelection = true);
	void onRoomJoin(IPlayer& player, std::size_t roomId);
	void setupSpawn(IPlayer& player, std::shared_ptr<Room> room);
	void setupRoomForPlayer(IPlayer& player, std::shared_ptr<Room> room);
	void removePlayerFromRoom(IPlayer& player);
	void onTick();

	std::vector<std::shared_ptr<Room>> _rooms;
	std::unordered_map<std::string, ITimer*> _freezeTimers;

	std::weak_ptr<Core::CoreManager> _coreManager;
	IPlayerPool* _playerPool;
	ITimersComponent* _timersComponent;

	ITimer* _ticker;

public:
	~DeathmatchController();
	void onModeJoin(IPlayer& player, std::unordered_map<std::string, Core::PrimitiveType> joinData) override;
	void onModeSelect(IPlayer& player) override;
	void onModeLeave(IPlayer& player) override;

	void onPlayerSpawn(IPlayer& player) override;
	void onPlayerDeath(IPlayer& player, IPlayer* killer, int reason) override;
	void onPlayerKeyStateChange(IPlayer& player, uint32_t newKeys, uint32_t oldKeys) override;

	static DeathmatchController* create(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool, ITimersComponent* timersComponent);
};
}