#pragma once

#include "../core/player/PlayerModel.hpp"
#include "../core/player/PlayerExtension.hpp"
#include "Modes.hpp"
#include "../core/utils/Events.hpp"

#include <eventbus/event_bus.hpp>
#include <memory>
#include <player.hpp>

#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace Modes
{
struct ModeBase
{
	ModeBase(Mode mode, std::shared_ptr<dp::event_bus> bus);
	virtual ~ModeBase();

	virtual void onModeSelect(IPlayer& player) = 0;
	virtual void onModeJoin(IPlayer& player, JoinData joinData);
	virtual void onModeLeave(IPlayer& player);
	virtual void onPlayerSave(
		std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn);
	virtual void onPlayerLoad(
		std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn);
	virtual void onPlayerOnFire(Core::Utils::Events::PlayerOnFireEvent event);
	virtual void onPlayerOnFireBeenKilled(
		Core::Utils::Events::PlayerOnFireBeenKilled event);
	virtual void onX1ArenaWin(Core::Utils::Events::X1ArenaWin event);
	virtual void onPlayerJoinedMode(Core::Utils::Events::PlayerJoinedMode);

	unsigned int playerCount();
	const Mode& getModeType();

	template <typename... T>
	inline void sendMessageToAll(const std::string& message, const T&... args)
	{
		for (auto player : this->players)
		{
			auto playerExt = Core::Player::getPlayerExt(*player);
			playerExt->sendTranslatedMessageFormatted(message, args...);
		}
	}

	template <typename EventType, typename ClassType, typename MemberFunction>
	inline dp::handler_registration hookEvent(
		dp::handler_registration& registration, ClassType* classInstance,
		MemberFunction&& memberFunction)
	{
		this->bus->remove_handler(registration);
		return this->bus->register_handler<EventType>(
			classInstance, memberFunction);
	}

protected:
	std::unordered_set<IPlayer*> players;
	typedef ModeBase super;
	Mode mode;
	std::shared_ptr<dp::event_bus> bus;

	dp::handler_registration playerOnFireEventRegistration;
	dp::handler_registration playerOnFireBeenKilledRegistration;
	dp::handler_registration x1ArenaWinRegistration;
	dp::handler_registration playerJoinedModeSubsciption;
};
}