#pragma once

#include "Server/Components/Timers/timers.hpp"
#include "Server/Components/Vehicles/vehicles.hpp"
#include <player.hpp>

namespace Core::Controllers
{
class SpeedometerController : PlayerChangeEventHandler
{
	IPlayerPool* _playerPool;
	IVehiclesComponent* _vehiclesComponent;
	ITimersComponent* _timersComponent;

public:
	SpeedometerController(IPlayerPool* playerPool, IVehiclesComponent* vehiclesComponent, ITimersComponent* timersComponent);
	~SpeedometerController();

	void onPlayerStateChange(IPlayer& player, PlayerState newState, PlayerState oldState) override;

	void updateSpeedometers();
};
}