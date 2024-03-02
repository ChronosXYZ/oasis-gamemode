#pragma once

#include "../../modes/IMode.hpp"
#include "../../core/CoreManager.hpp"
#include "Room.hpp"

#include <cstddef>
#include <player.hpp>

#include <memory>
#include <vector>

namespace Modes::Deathmatch
{
inline const unsigned int VIRTUAL_WORLD_PREFIX = 100;
class DeathmatchController : public Modes::IMode, public PlayerDamageEventHandler, public PlayerSpawnEventHandler
{
	DeathmatchController(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool);

	void initCommand();
	void initRooms();
	void showRoomSelectionDialog(IPlayer& player);
	void onRoomJoin(IPlayer& player, std::shared_ptr<Room> room, std::size_t roomId);
	void setupSpawn(IPlayer& player, std::shared_ptr<Room> room);
	void setupRoomForPlayer(IPlayer& player, std::shared_ptr<Room> room);
	void removePlayerFromRoom(IPlayer& player);

	std::vector<std::shared_ptr<Room>> _rooms;

	std::weak_ptr<Core::CoreManager> _coreManager;
	IPlayerPool* _playerPool;

public:
	~DeathmatchController();
	void onModeJoin(IPlayer& player) override;
	void onModeLeave(IPlayer& player) override;

	void onPlayerSpawn(IPlayer& player) override;
	void onPlayerDeath(IPlayer& player, IPlayer* killer, int reason) override;

	static DeathmatchController* create(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool);
};
}