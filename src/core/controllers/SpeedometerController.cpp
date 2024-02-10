#include "SpeedometerController.hpp"
#include "../player/PlayerExtension.hpp"
#include "../textdraws/Speedometer.hpp"
#include "types.hpp"

#include <player.hpp>
#include <component.hpp>
#include <Server/Components/Timers/timers.hpp>
#include <Server/Components/Timers/Impl/timers_impl.hpp>

#include <memory>

namespace Core::Controllers
{

SpeedometerController::SpeedometerController(IPlayerPool* playerPool, IVehiclesComponent* vehiclesComponent, ITimersComponent* timersComponent)
	: _playerPool(playerPool)
	, _vehiclesComponent(vehiclesComponent)
	, _timersComponent(timersComponent)
{
	_playerPool->getPlayerChangeDispatcher().addEventHandler(this);
	_timersComponent->create(new Impl::SimpleTimerHandler(
								 std::bind(&SpeedometerController::updateSpeedometers, this)),
		Milliseconds(500), true);
}

SpeedometerController::~SpeedometerController()
{
	_playerPool->getPlayerChangeDispatcher().removeEventHandler(this);
}

void SpeedometerController::onPlayerStateChange(IPlayer& player, PlayerState newState, PlayerState oldState)
{
	auto playerExt = Player::getPlayerExt(player);
	if (newState == PlayerState_Driver) // player enters vehicle
	{
		std::shared_ptr<TextDraws::Speedometer> speedometerView(new TextDraws::Speedometer(player));
		playerExt->getTextDrawManager()->add(TextDraws::Speedometer::NAME, speedometerView);
		speedometerView->show();
	}
	else if (oldState == PlayerState_Driver) // player exits vehicle
	{
		playerExt->getTextDrawManager()->destroy(TextDraws::Speedometer::NAME);
	}
}

void SpeedometerController::updateSpeedometers()
{
	for (auto player : _playerPool->players())
	{
		auto playerExt = Player::getPlayerExt(*player);
		auto velocity = playerExt->getVehicleSpeed();
		if (auto speedometerViewOpt = playerExt->getTextDrawManager()->get(TextDraws::Speedometer::NAME))
		{
			auto speedometerView = std::dynamic_pointer_cast<TextDraws::Speedometer>(*speedometerViewOpt);
			speedometerView->update(playerExt->getVehicleSpeed());
		}
	}
}
}
