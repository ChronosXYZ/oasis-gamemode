#include "Room.hpp"
#include "../../core/player/PlayerExtension.hpp"

namespace Modes::X1
{
template <typename... T>
void Room::sendMessageToAll(const std::string& message, const T&... args)
{
	for (auto player : this->players)
	{
		auto playerExt = Core::Player::getPlayerExt(*player);
		playerExt->sendTranslatedMessageFormatted(message, args...);
	}
}
}