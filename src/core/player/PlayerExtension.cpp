#include "PlayerExtension.hpp"
#include "../utils/LocaleUtils.hpp"

namespace Core
{
OasisPlayerExt::OasisPlayerExt(std::shared_ptr<PlayerModel> data, IPlayer& player)
	: _player(player)
	, _playerData(data)
{
}

std::shared_ptr<PlayerModel> OasisPlayerExt::getPlayerData()
{
	return this->_playerData;
}

void OasisPlayerExt::delayedKick()
{
	std::thread([&]()
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(DELAYED_KICK_INTERVAL_MS));
			_player.kick();
		})
		.detach();
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
}
void OasisPlayerExt::reset()
{
	_playerData.reset();
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
}