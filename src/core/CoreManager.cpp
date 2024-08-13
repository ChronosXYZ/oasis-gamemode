#include "CoreManager.hpp"
#include "SQLQueryManager.hpp"
#include "Server/Components/Vehicles/vehicles.hpp"
#include "commands/CommandManager.hpp"
#include "controllers/PlayerOnFireController.hpp"
#include "controllers/SpeedometerController.hpp"
#include "dialogs/DialogResult.hpp"
#include "dialogs/Dialogs.hpp"
#include "eventbus/event_bus.hpp"
#include "player.hpp"
#include "player/PlayerExtension.hpp"
#include "textdraws/ITextDrawWrapper.hpp"
#include "textdraws/ServerLogo.hpp"
#include "textdraws/Notification.hpp"
#include "types.hpp"
#include "utils/Colors.hpp"
#include "utils/Common.hpp"
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

namespace Core
{
CoreManager::CoreManager(
	IComponentList* components, ICore* core, IPlayerPool* playerPool)
	: components(components)
	, _core(core)
	, _playerPool(playerPool)
	, _dialogManager(
		  std::shared_ptr<DialogManager>(new DialogManager(components)))
	, _commandManager(std::shared_ptr<Commands::CommandManager>(
		  new Commands::CommandManager(playerPool)))
	, _classesComponent(components->queryComponent<IClassesComponent>())
	, _playerControllers(std::make_unique<ServiceLocator>())
	, _modes(std::make_unique<ServiceLocator>())
	, bus(std::make_shared<dp::event_bus>())
{
	this->initSkinSelection();

	_playerPool->getPlayerConnectDispatcher().addEventHandler(this);
	_playerPool->getPlayerSpawnDispatcher().addEventHandler(this);
	_playerPool->getPlayerTextDispatcher().addEventHandler(this);
	_playerPool->getPlayerDamageDispatcher().addEventHandler(this);

	_classesComponent->getEventDispatcher().addEventHandler(this);

	auto dbConnectionString = getenv("DB_CONNECTION_STRING");
	if (dbConnectionString == NULL)
	{
		throw std::runtime_error(
			"DB_CONNECTION_STRING environment variable is not set!");
	}
	this->_dbConnection
		= std::make_shared<pqxx::connection>(dbConnectionString);
	this->saveThread = std::thread(&CoreManager::runSaveThread, this,
		std::move(this->saveThreadExitSignal.get_future()));
	this->saveThread.detach();
}

std::shared_ptr<CoreManager> CoreManager::create(
	IComponentList* components, ICore* core, IPlayerPool* playerPool)
{
	std::shared_ptr<CoreManager> pManager(
		new CoreManager(components, core, playerPool));
	pManager->initHandlers();
	return pManager;
}

CoreManager::~CoreManager()
{
	this->saveThreadExitSignal.set_value();
	saveAllPlayers();
	_playerPool->getPlayerConnectDispatcher().removeEventHandler(this);
	_playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);
	_playerPool->getPlayerTextDispatcher().removeEventHandler(this);
	_playerPool->getPlayerDamageDispatcher().removeEventHandler(this);

	_classesComponent->getEventDispatcher().removeEventHandler(this);
}

std::shared_ptr<PlayerModel> CoreManager::getPlayerData(IPlayer& player)
{
	auto ext = queryExtension<Player::OasisPlayerExt>(player);
	if (ext)
	{
		if (auto data = ext->getPlayerData())
		{
			return data;
		}
	}
	return {};
}

void CoreManager::onPlayerConnect(IPlayer& player)
{
	auto data = std::shared_ptr<PlayerModel>(new PlayerModel());
	auto playerExt = new Player::OasisPlayerExt(
		data, player, components->queryComponent<ITimersComponent>());
	this->_playerData[player.getID()] = data;
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

	_playerPool->sendDeathMessageToAll(NULL, player, 200);
}

void CoreManager::onPlayerDisconnect(
	IPlayer& player, PeerDisconnectReason reason)
{
	this->savePlayer(player);
	this->_playerData.erase(player.getID());
	this->removePlayerFromCurrentMode(player);
	_playerPool->sendDeathMessageToAll(NULL, player, 201);
}

std::shared_ptr<DialogManager> CoreManager::getDialogManager()
{
	return this->_dialogManager;
}

void CoreManager::initHandlers()
{
	_authController = std::make_unique<Auth::AuthController>(
		_playerPool, _dialogManager, weak_from_this());

	_modes->registerInstance(Modes::Freeroam::FreeroamController::create(
		weak_from_this(), _playerPool, this->_dialogManager, bus));
	_modes->registerInstance(Modes::Deathmatch::DeathmatchController::create(
		weak_from_this(), _commandManager, _dialogManager, _playerPool,
		components->queryComponent<ITimersComponent>(), this->bus,
		_dbConnection));
	_modes->registerInstance(Modes::X1::X1Controller::create(weak_from_this(),
		_commandManager, _dialogManager, _playerPool,
		components->queryComponent<ITimersComponent>(), this->bus));
	_modes->registerInstance(Modes::Duel::DuelController::create(
		weak_from_this(), _commandManager, _dialogManager, _playerPool,
		components->queryComponent<ITimersComponent>(), this->bus));
	_playerControllers->registerInstance(new Controllers::SpeedometerController(
		_playerPool, components->queryComponent<IVehiclesComponent>(),
		components->queryComponent<ITimersComponent>()));
	_playerControllers->registerInstance(
		new Controllers::PlayerOnFireController(this->_playerPool, this->bus,
			this->_commandManager, this->_dialogManager));
}

std::shared_ptr<pqxx::connection> CoreManager::getDBConnection()
{
	return this->_dbConnection;
}

bool CoreManager::refreshPlayerData(IPlayer& player)
{
	auto db = this->getDBConnection();
	pqxx::work txn(*db);
	pqxx::result res
		= txn.exec_params(SQLQueryManager::Get()
							  ->getQueryByName(Utils::SQL::Queries::LOAD_PLAYER)
							  .value(),
			player.getName().to_string());

	if (res.size() == 0)
	{
		return false;
	}
	spdlog::info(
		"Found player data for " + player.getName().to_string() + " in DB");

	auto row = res[0];
	auto data = Player::getPlayerData(player);
	data->updateFromRow(row);

	this->_modes->resolve<Modes::Deathmatch::DeathmatchController>()
		->onPlayerLoad(data, txn);
	this->_modes->resolve<Modes::X1::X1Controller>()->onPlayerLoad(data, txn);
	txn.commit();
	return true;
}

void CoreManager::saveAllPlayers()
{
	for (const auto [id, data] : this->_playerData)
	{
		this->savePlayer(data);
	}
	spdlog::info("Saved all player data!");
}

void CoreManager::savePlayer(std::shared_ptr<PlayerModel> data)
{
	if (!data->tempData->core->isLoggedIn)
		return;

	auto db = this->getDBConnection();
	pqxx::work txn(*db);

	// save general player info
	txn.exec_params(SQLQueryManager::Get()
						->getQueryByName(Utils::SQL::Queries::SAVE_PLAYER)
						.value(),
		data->language, data->lastSkinId, data->lastIP, data->lastLoginAt,
		data->userId);

	// save DM info
	this->_modes->resolve<Modes::Deathmatch::DeathmatchController>()
		->onPlayerSave(data, txn);

	// save X1 info
	this->_modes->resolve<Modes::X1::X1Controller>()->onPlayerSave(data, txn);

	// save duel info
	this->_modes->resolve<Modes::Duel::DuelController>()->onPlayerSave(
		data, txn);

	txn.commit();
	spdlog::info("Player {} has been successfully saved", data->name);
}

void CoreManager::savePlayer(IPlayer& player)
{
	this->savePlayer(this->_playerData[player.getID()]);
}

void CoreManager::onFree(IComponent* component)
{
	if (component->getUID() == DialogsComponent_UID)
	{
		// this->_authController.reset();
		// this->_modes->clear();
		// this->_playerControllers->clear();
		this->_dialogManager.reset();
	}
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
	auto pData = this->getPlayerData(player);
	if (!pData->tempData->core->isLoggedIn)
		return true;
	if (!pData->tempData->core->skinSelectionMode)
	{
		// first player request class call
		pData->tempData->core->skinSelectionMode = true;

		Vector4 classSelectionPoint
			= CLASS_SELECTION_POINTS[random() % CLASS_SELECTION_POINTS.size()];
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
		player.setSkin(pData->lastSkinId);
		this->removePlayerFromCurrentMode(player);
	}

	return true;
}

void CoreManager::onPlayerLoggedIn(IPlayer& player)
{
	auto pData = this->getPlayerData(player);
	pData->tempData->auth.reset();
	pData->tempData->core->isLoggedIn = true;

	// hack to get class selection buttons appear again
	player.setSpectating(false);
}

bool CoreManager::onPlayerRequestSpawn(IPlayer& player)
{
	auto pData = this->getPlayerData(player);
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

	showModeSelectionDialog(player);

	return false;
}

void CoreManager::onPlayerSpawn(IPlayer& player)
{
	auto playerData = Player::getPlayerData(player);
	playerData->tempData->core->isDying = false;
}

void CoreManager::showModeSelectionDialog(IPlayer& player)
{
	auto dialog = std::shared_ptr<TabListHeadersDialog>(
		new TabListHeadersDialog(_("Modes", player),
			{ _("Mode", player), _("Command", player), _("Players", player) },
			{ { _("Freeroam", player), "/fr",
				  std::to_string(
					  _modes->resolve<Modes::Freeroam::FreeroamController>()
						  ->playerCount()) },
				{ _("Deathmatch", player), "/dm",
					std::to_string(
						_modes
							->resolve<Modes::Deathmatch::DeathmatchController>()
							->playerCount()) },
				{ _("Protect the President", player), "/ptp",
					std::to_string(0) },
				{ _("Derby", player), "/derby", std::to_string(0) },
				{ _("Cops and Robbers", player), "/cnr", std::to_string(0) } },
			_("Select", player), ""));
	this->getDialogManager()->showDialog(player, dialog,
		[&](DialogResult result)
		{
			selectMode(player, static_cast<Modes::Mode>(result.listItem()));
		});
}

unsigned int CoreManager::allocateVirtualWorldId()
{
	return this->virtualWorldIdPool.allocateId();
}

void CoreManager::freeVirtualWorldId(unsigned int id)
{
	this->virtualWorldIdPool.freeId(id);
}

void CoreManager::selectMode(IPlayer& player, Modes::Mode mode)
{
	switch (mode)
	{
	case Modes::Mode::Freeroam:
	{
		this->_modes->resolve<Modes::Freeroam::FreeroamController>()
			->onModeSelect(player);
		break;
	}
	case Modes::Mode::Deathmatch:
	{
		this->_modes->resolve<Modes::Deathmatch::DeathmatchController>()
			->onModeSelect(player);
		break;
	}
	case Modes::Mode::X1:
	{
		this->_modes->resolve<Modes::X1::X1Controller>()->onModeSelect(player);
		break;
	}
	default:
	{
		Player::getPlayerExt(player)->sendErrorMessage(
			__("Mode is not implemented yet"));
		this->showModeSelectionDialog(player);
		break;
	}
	}
}

bool CoreManager::joinMode(IPlayer& player, Modes::Mode mode,
	std::unordered_map<std::string, Core::PrimitiveType> joinData)
{
	auto pData = Player::getPlayerData(player);
	auto playerExt = Player::getPlayerExt(player);
	if (pData->tempData->core->isDying)
	{
		playerExt->sendErrorMessage(__("You cannot join a mode while dying"));
		return false;
	}
	this->removePlayerFromCurrentMode(player);

	pData->tempData->core->lastMode = pData->tempData->core->currentMode;
	pData->tempData->core->currentMode = mode;

	std::shared_ptr<Modes::ModeBase> modeBase;
	switch (mode)
	{
	case Modes::Mode::Freeroam:
	{
		modeBase = static_pointer_cast<Modes::ModeBase>(
			this->_modes->resolve<Modes::Freeroam::FreeroamController>());
		break;
	}
	case Modes::Mode::Deathmatch:
	{
		modeBase = static_pointer_cast<Modes::ModeBase>(
			this->_modes->resolve<Modes::Deathmatch::DeathmatchController>());
		break;
	}
	case Modes::Mode::X1:
	{
		modeBase = static_pointer_cast<Modes::ModeBase>(
			this->_modes->resolve<Modes::X1::X1Controller>());
		break;
	}
	case Modes::Mode::Duel:
	{
		modeBase = static_pointer_cast<Modes::ModeBase>(
			this->_modes->resolve<Modes::Duel::DuelController>());
		break;
	}
	default:
	{
		Player::getPlayerExt(player)->sendErrorMessage(
			__("Mode is not implemented yet!"));
		this->showModeSelectionDialog(player);
		break;
	}
	}
	if (modeBase.get() == nullptr)
		return false;
	modeBase->onModeJoin(player, joinData);
	return true;
}

void CoreManager::removePlayerFromCurrentMode(IPlayer& player)
{
	auto mode = Player::getPlayerExt(player)->getMode();
	if (mode == Modes::Mode::None)
		return;
	std::shared_ptr<Modes::ModeBase> modeBase;
	switch (mode)
	{
	case Modes::Mode::Freeroam:
	{
		modeBase = static_pointer_cast<Modes::ModeBase>(
			this->_modes->resolve<Modes::Freeroam::FreeroamController>());
		break;
	}
	case Modes::Mode::Deathmatch:
	{
		modeBase = static_pointer_cast<Modes::ModeBase>(
			this->_modes->resolve<Modes::Deathmatch::DeathmatchController>());
		break;
	}
	case Modes::Mode::X1:
	{
		modeBase = static_pointer_cast<Modes::ModeBase>(
			this->_modes->resolve<Modes::X1::X1Controller>());
		break;
	}
	case Modes::Mode::Duel:
	{
		modeBase = static_pointer_cast<Modes::ModeBase>(
			this->_modes->resolve<Modes::Duel::DuelController>());
		break;
	}
	default:
	{
		break;
	}
	}
	if (modeBase.get() == nullptr)
		return;
	modeBase->onModeLeave(player);
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

std::shared_ptr<Commands::CommandManager> CoreManager::getCommandManager()
{
	return _commandManager;
}

bool CoreManager::onPlayerText(IPlayer& player, StringView message)
{
	auto playerExt = Player::getPlayerExt(player);
	if (!playerExt->isAuthorized())
		return false;
	player.setChatBubble(
		message, Colour::White(), 100.0, Milliseconds(CHAT_BUBBLE_EXPIRATION));

	for (auto sPlayer : _playerPool->players())
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
	_playerPool->sendDeathMessageToAll(killer, player, reason);

	auto playerData = Player::getPlayerData(player);
	playerData->tempData->core->isDying = true;

	if (killer)
	{
		auto killerExt = Player::getPlayerExt(*killer);
		player.sendGameText(
			fmt::sprintf(_("~w~You got killed by~n~~r~%s(%d)", player),
				killer->getName().to_string(), killer->getID()),
			Seconds(4), 3);

		killerExt->showNotification(
			fmt::sprintf(_("~w~You killed~n~~r~%s(%d)", player),
				player.getName().to_string(), player.getID()),
			TextDraws::NotificationPosition::Bottom, 6);
	}
}
}