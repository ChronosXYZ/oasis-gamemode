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
struct ModeBase : public PlayerDamageEventHandler
{
	ModeBase(
		Mode mode, std::shared_ptr<dp::event_bus> bus, IPlayerPool* playerPool);
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
	virtual void onDuelWin(Core::Utils::Events::DuelWin event);

	virtual void onPlayerGiveDamage(IPlayer& player, IPlayer& to, float amount,
		unsigned int weapon, BodyPart part) override;

	unsigned int playerCount();
	const Mode& getModeType();

	template <typename... T>
	inline void sendMessageToAll(const std::string& message, T&&... args)
	{
		for (auto player : this->players)
		{
			auto playerExt = Core::Player::getPlayerExt(*player);
			playerExt->sendTranslatedMessage(message, std::forward<T>(args)...);
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

	template <typename... T>
	inline void sendModeMessage(
		IPlayer& player, const std::string& message, T&&... args)
	{
		auto mode = this->getModeType();
		player.sendClientMessage(Colour::White(),
			fmt::sprintf("%s {%s}%s{FFFFFF}: %s", _("#LIME#>>#WHITE#", player),
				Modes::getModeColor(mode), Modes::getModeShortName(mode),
				fmt::sprintf(_(message, player), std::forward<T>(args)...)));
	}

protected:
	std::unordered_set<IPlayer*> players;
	typedef ModeBase super;
	Mode mode;
	std::shared_ptr<dp::event_bus> bus;
	IPlayerPool* playerPool;

	dp::handler_registration playerOnFireEventRegistration;
	dp::handler_registration playerOnFireBeenKilledRegistration;
	dp::handler_registration x1ArenaWinRegistration;
	dp::handler_registration duelWinRegistration;
	dp::handler_registration playerJoinedModeSubsciption;
};
}