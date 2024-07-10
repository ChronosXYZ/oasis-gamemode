#include "PlayerExtension.hpp"
#include "../utils/Localization.hpp"
#include "TextDrawManager.hpp"

#include <fmt/printf.h>
#include <player.hpp>
#include <component.hpp>
#include <Server/Components/Timers/timers.hpp>
#include <Server/Components/Timers/Impl/timers_impl.hpp>
#include <Server/Components/Vehicles/vehicles.hpp>

#include <functional>
#include <memory>
#include <cmath>

namespace Core::Player
{
OasisPlayerExt::OasisPlayerExt(std::shared_ptr<PlayerModel> data,
	IPlayer& player, ITimersComponent* timerManager)
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
	_timerManager->create(new Impl::SimpleTimerHandler(std::bind(&IPlayer::kick,
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
		fmt::sprintf(
			"%s %s", _("#LIME#>>#WHITE#", _player), _(message, _player)));
}

void OasisPlayerExt::sendTranslatedMessage(const std::string& message)
{
	_player.sendClientMessage(Colour::White(), _(message, _player));
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

float OasisPlayerExt::getVehicleSpeed()
{
	auto vehicleData = queryExtension<IPlayerVehicleData>(_player);
	auto vehicle = vehicleData->getVehicle();
	if (vehicle)
	{
		auto velocity = vehicle->getVelocity();
		return std::sqrt(std::pow(velocity.x, 2) + std::pow(velocity.y, 2))
			* 100.0 * 1.6;
	}
	return 0.0;
}

bool OasisPlayerExt::isInMode(Modes::Mode mode)
{
	return this->_playerData->tempData->core->currentMode == mode;
}

const Modes::Mode& OasisPlayerExt::getMode()
{
	return this->_playerData->tempData->core->currentMode;
}

bool OasisPlayerExt::isInAnyMode()
{
	return !this->isInMode(Modes::Mode::None);
}
}