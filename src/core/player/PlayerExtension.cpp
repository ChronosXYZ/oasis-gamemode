#include "PlayerExtension.hpp"
#include "../utils/Localization.hpp"
#include "TextDrawManager.hpp"

#include <player.hpp>
#include <component.hpp>
#include <Server/Components/Timers/timers.hpp>
#include <Server/Components/Timers/Impl/timers_impl.hpp>

#include <functional>

namespace Core::Player
{
OasisPlayerExt::OasisPlayerExt(std::shared_ptr<PlayerModel> data, IPlayer& player, ITimersComponent* timerManager)
	: _player(player)
	, _playerData(data)
	, _timerManager(timerManager)
	, _textDrawManager(new TextDrawManager())
{
}

std::shared_ptr<PlayerModel> OasisPlayerExt::getPlayerData()
{
	return this->_playerData;
}

void OasisPlayerExt::delayedKick()
{
	_timerManager->create(new Impl::SimpleTimerHandler(
							  std::bind(
								  &IPlayer::kick,
								  std::reference_wrapper<IPlayer>(_player))),
		Milliseconds(DELAYED_KICK_INTERVAL_MS), false);
}

void OasisPlayerExt::setFacingAngle(float angle)
{
	auto rot = _player.getRotation().ToEuler();
	rot.z = angle;
	_player.setRotation(rot);
}

void OasisPlayerExt::freeExtension()
{
	_playerData.reset();
	_textDrawManager.reset();
}
void OasisPlayerExt::reset()
{
	_playerData.reset();
	_textDrawManager.reset();
}

void OasisPlayerExt::sendErrorMessage(const std::string& message)
{
	_player.sendClientMessage(Colour::White(),
		fmt::format("{} {}", _("#RED#[ERROR]#WHITE#", _player), message));
}

void OasisPlayerExt::sendInfoMessage(const std::string& message)
{
	_player.sendClientMessage(Colour::White(),
		fmt::format("{} {}", _("#BRIGHT_BLUE#[INFO]#WHITE#", _player), message));
}

const std::string OasisPlayerExt::getIP()
{
	PeerAddress::AddressString ipString;
	PeerNetworkData peerData = _player.getNetworkData();
	peerData.networkID.address.ToString(peerData.networkID.address, ipString);
	return ipString.data();
}

std::shared_ptr<TextDrawManager> OasisPlayerExt::getTextDrawManager()
{
	return _textDrawManager;
}
}