#include "CoreManager.hpp"
#include "ModeManager.hpp"
#include "SQLQueryManager.hpp"
#include "Server/Components/Vehicles/vehicles.hpp"
#include "commands/CommandManager.hpp"
#include "controllers/PlayerOnFireController.hpp"
#include "controllers/SpeedometerController.hpp"
#include "eventbus/event_bus.hpp"
#include "player.hpp"
#include "player/PlayerExtension.hpp"
#include "textdraws/ITextDrawWrapper.hpp"
#include "textdraws/ServerLogo.hpp"
#include "textdraws/Notification.hpp"
#include "types.hpp"
#include "utils/Colors.hpp"
#include "utils/Common.hpp"
#include "utils/ConnectionPool.hpp"
#include "utils/IDPool.hpp"
#include "utils/QueryNames.hpp"
#include "utils/ServiceLocator.hpp"
#include "../modes/freeroam/FreeroamController.hpp"
#include "../modes/deathmatch/DeathmatchController.hpp"
#include "../modes/x1/X1Controller.hpp"
#include "../modes/duel/DuelController.hpp"

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <future>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <fmt/printf.h>
#include <Server/Components/Timers/timers.hpp>
#include <stdexcept>
#include <string>
#include <thread>
#include <random>

namespace Core
{
CoreManager::CoreManager(IComponentList* components, ICore* core,
	IPlayerPool* playerPool, const std::string& connection_string)
	: components(components)
	, _core(core)
	, playerPool(playerPool)
	, _dialogManager(
		  std::shared_ptr<DialogManager>(new DialogManager(components)))
	, _commandManager(std::shared_ptr<Commands::CommandManager>(
		  new Commands::CommandManager(playerPool)))
	, _classesComponent(components->queryComponent<IClassesComponent>())
	, _playerControllers(std::make_unique<ServiceLocator>())
	, bus(std::make_shared<dp::event_bus>())
	, connectionPool(connection_string, 8)
	, virtualWorldIdPool(std::make_shared<Utils::IDPool>())
{
	this->initSkinSelection();

	playerPool->getPlayerConnectDispatcher().addEventHandler(this);
	playerPool->getPlayerSpawnDispatcher().addEventHandler(this);
	playerPool->getPlayerTextDispatcher().addEventHandler(this);
	playerPool->getPlayerDamageDispatcher().addEventHandler(this);

	_classesComponent->getEventDispatcher().addEventHandler(this);

	this->saveThread = std::thread(&CoreManager::runSaveThread, this,
		std::move(this->saveThreadExitSignal.get_future()));
	this->saveThread.detach();

	this->modeManager = std::make_shared<ModeManager>(this->_dialogManager);
}

std::unique_ptr<CoreManager> CoreManager::create(IComponentList* components,
	ICore* core, IPlayerPool* playerPool,
	const std::string& db_connection_string)
{
	std::unique_ptr<CoreManager> pManager(
		new CoreManager(components, core, playerPool, db_connection_string));
	pManager->initHandlers();
	return pManager;
}

CoreManager::~CoreManager()
{
	this->saveThreadExitSignal.set_value();
	saveAllPlayers();
	playerPool->getPlayerConnectDispatcher().removeEventHandler(this);
	playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);
	playerPool->getPlayerTextDispatcher().removeEventHandler(this);
	playerPool->getPlayerDamageDispatcher().removeEventHandler(this);

	_classesComponent->getEventDispatcher().removeEventHandler(this);
}

void CoreManager::onPlayerConnect(IPlayer& player)
{
	auto data = std::shared_ptr<PlayerModel>(new PlayerModel());
	auto playerExt = new Player::OasisPlayerExt(
		data, player, components->queryComponent<ITimersComponent>());
	this->playerData[player.getID()] = data;
	player.addExtension(playerExt, true);

	player.setColour(Colour::FromRGBA(
		Utils::NICKNAME_COLORS[rand() % Utils::NICKNAME_COLORS.size()]));

	auto txdManager = playerExt->getTextDrawManager();
	auto logo
		= std::shared_ptr<TextDraws::ServerLogo>(new TextDraws::ServerLogo(
			player, "OASIS", "freeroam", "oasisfreeroam.xyz"));
	auto notificationTxd
		= std::shared_ptr<TextDraws::Notification>(new TextDraws::Notification(
			player, this->components->queryComponent<ITimersComponent>()));
	txdManager->add(TextDraws::ServerLogo::NAME, logo);
	txdManager->add(TextDraws::Notification::NAME, notificationTxd);
	logo->show();

	playerPool->sendDeathMessageToAll(NULL, player, 200);
}

void CoreManager::onPlayerDisconnect(
	IPlayer& player, PeerDisconnectReason reason)
{
	this->savePlayer(player);
	this->modeManager->removePlayerFromCurrentMode(player);
	playerPool->sendDeathMessageToAll(NULL, player, 201);
	this->playerData.erase(player.getID());
}

void CoreManager::initHandlers()
{
	_authController = std::make_unique<Auth::AuthController>(this->components,
		this->playerPool, this->connectionPool, this->modeManager,
		this->_dialogManager);

	modeManager->addMode(
		std::make_unique<Modes::Freeroam::FreeroamController>(this->components,
			this->playerPool, this->virtualWorldIdPool, this->modeManager,
			this->_dialogManager, this->_commandManager, this->bus));
	modeManager->addMode(
		std::make_unique<Modes::Deathmatch::DeathmatchController>(
			this->modeManager, this->_commandManager, _dialogManager,
			playerPool, components->queryComponent<ITimersComponent>(),
			this->bus, connectionPool, this->virtualWorldIdPool));
	modeManager->addMode(std::make_unique<Modes::X1::X1Controller>(
		this->modeManager, this->virtualWorldIdPool, _commandManager,
		_dialogManager, playerPool,
		components->queryComponent<ITimersComponent>(), this->bus));
	modeManager->addMode(std::make_unique<Modes::Duel::DuelController>(
		modeManager, _commandManager, _dialogManager, playerPool,
		components->queryComponent<ITimersComponent>(), this->bus,
		this->virtualWorldIdPool));

	_playerControllers->registerInstance(new Controllers::SpeedometerController(
		playerPool, components->queryComponent<IVehiclesComponent>(),
		components->queryComponent<ITimersComponent>()));
	_playerControllers->registerInstance(
		new Controllers::PlayerOnFireController(this->playerPool, this->bus,
			this->_commandManager, this->_dialogManager));
}

void CoreManager::saveAllPlayers()
{
	for (const auto [id, data] : this->playerData)
	{
		this->savePlayer(data);
	}
	spdlog::info("Saved all player data!");
}

void CoreManager::savePlayer(std::shared_ptr<PlayerModel> data)
{
	if (!data->tempData->core->isLoggedIn)
		return;

	auto basic_tx = cp::tx(this->connectionPool);
	auto& txn = basic_tx.get();

	// save general player info
	txn.exec_params(SQLQueryManager::Get()
						->getQueryByName(Utils::SQL::Queries::SAVE_PLAYER)
						.value(),
		data->language, data->lastSkinId, data->lastIP, data->lastLoginAt, data->userId);

	// save player settings
	txn.exec_params(SQLQueryManager::Get()
						->getQueryByName(Utils::SQL::Queries::SAVE_PLAYER_SETTINGS)
						.value(),
		data->settings->pmsEnabled, (int)data->settings->notificationPos, data->userId);

	this->modeManager->savePlayer(data, txn);
	basic_tx.commit();

	spdlog::info("Player {} has been successfully saved", data->name);
}

void CoreManager::savePlayer(IPlayer& player)
{
	this->savePlayer(this->playerData[player.getID()]);
}

void CoreManager::initSkinSelection()
{
	IClassesComponent* classesComponent
		= this->components->queryComponent<IClassesComponent>();
	for (int i = 0; i <= 311; i++)
	{
		if (i == 74) // skip invalid skin
			continue;
		classesComponent->create(i, TEAM_NONE, Vector3(0, 0, 0), 0.0,
			WeaponSlots { WeaponSlotData { 0, 0 } });
	}
}

bool CoreManager::onPlayerRequestClass(IPlayer& player, unsigned int classId)
{
	auto playerData = Player::getPlayerData(player);
	if (!playerData->tempData->core->isLoggedIn)
		return true;
	if (!playerData->tempData->core->skinSelectionMode)
	{
		// first player request class call
		playerData->tempData->core->skinSelectionMode = true;

		Vector4 classSelectionPoint
			= CLASS_SELECTION_POINTS[rand() % CLASS_SELECTION_POINTS.size()];
		player.setPosition(Vector3(classSelectionPoint));

		auto playerExt = Player::getPlayerExt(player);
		playerExt->setFacingAngle(classSelectionPoint.w);
		player.setCameraLookAt(
			Vector3(classSelectionPoint), PlayerCameraCutType_Cut);
		auto angleRad = Utils::deg2Rad(classSelectionPoint.w);
		player.setCameraPosition(
			Vector3(classSelectionPoint.x + 5.0 * std::sin(-angleRad),
				classSelectionPoint.y + 5.0 * std::cos(-angleRad),
				classSelectionPoint.z));
		player.setSkin(playerData->lastSkinId);
		this->modeManager->removePlayerFromCurrentMode(player);
	}

	return true;
}

bool CoreManager::onPlayerRequestSpawn(IPlayer& player)
{
	auto pData = Player::getPlayerData(player);
	if (!pData->tempData->core->isLoggedIn)
	{
		Player::getPlayerExt(player)->sendErrorMessage(
			__("You are not logged in yet!"));
		return false;
	}
	if (pData->tempData->core->skinSelectionMode)
	{
		pData->tempData->core->skinSelectionMode = false;
	}
	pData->lastSkinId = player.getSkin();

	this->modeManager->showModeSelectionDialog(player);

	return false;
}

void CoreManager::onPlayerSpawn(IPlayer& player)
{
	auto playerData = Player::getPlayerData(player);
	playerData->tempData->core->isDying = false;
}

void CoreManager::runSaveThread(std::future<void> exitSignal)
{
	while (exitSignal.wait_for(std::chrono::minutes(3))
		== std::future_status::timeout)
	{
		spdlog::info("Saving player data...");
		this->saveAllPlayers();
	}
}

bool CoreManager::onPlayerText(IPlayer& player, StringView message)
{
	auto playerExt = Player::getPlayerExt(player);
	if (!playerExt->isAuthorized())
		return false;
	player.setChatBubble(
		message, Colour::White(), 100.0, Milliseconds(CHAT_BUBBLE_EXPIRATION));

	for (auto sPlayer : playerPool->players())
	{
		if (!playerExt->isInAnyMode())
			sPlayer->sendClientMessage(Colour::White(),
				fmt::sprintf("{%06x}%s(%d){FFFFFF}: %s",
					player.getColour().RGBA() >> 8,
					player.getName().to_string(), player.getID(),
					message.to_string()));
		else
			sPlayer->sendClientMessage(Colour::White(),
				fmt::sprintf("{%s}%s: {%06x}%s(%d){FFFFFF}: %s",
					Modes::getModeColor(playerExt->getMode()),
					Modes::getModeShortName(playerExt->getMode()),
					player.getColour().RGBA() >> 8,
					player.getName().to_string(), player.getID(),
					message.to_string()));
	}

	return false;
}

void CoreManager::onPlayerDeath(IPlayer& player, IPlayer* killer, int reason)
{
	playerPool->sendDeathMessageToAll(killer, player, reason);

	auto playerData = Player::getPlayerData(player);
	playerData->tempData->core->isDying = true;

	if (killer)
	{
		auto killerExt = Player::getPlayerExt(*killer);
		auto playerExt = Player::getPlayerExt(player);
		playerExt->showNotification(
			fmt::sprintf(_("~w~You got killed by~n~~r~%s(%d)", player),
				killer->getName().to_string(), killer->getID()), 4);

		killerExt->showNotification(
			fmt::sprintf(_("~w~You killed~n~~r~%s(%d)", player),
				player.getName().to_string(), player.getID()), 6);
	}
}
}