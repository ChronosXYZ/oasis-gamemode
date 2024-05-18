#include "CoreManager.hpp"
#include "SQLQueryManager.hpp"
#include "Server/Components/Vehicles/vehicles.hpp"
#include "commands/CommandInfo.hpp"
#include "commands/CommandManager.hpp"
#include "controllers/SpeedometerController.hpp"
#include "player/PlayerExtension.hpp"
#include "textdraws/ITextDrawWrapper.hpp"
#include "textdraws/ServerLogo.hpp"
#include "types.hpp"
#include "utils/Common.hpp"
#include "utils/QueryNames.hpp"
#include "utils/ServiceLocator.hpp"
#include "../modes/freeroam/FreeroamController.hpp"
#include "../modes/deathmatch/DeathmatchController.hpp"

#include <cstddef>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <fmt/printf.h>
#include <Server/Components/Timers/timers.hpp>
#include <stdexcept>

namespace Core
{
CoreManager::CoreManager(IComponentList* components, ICore* core, IPlayerPool* playerPool)
	: components(components)
	, _core(core)
	, _playerPool(playerPool)
	, _dialogManager(std::shared_ptr<DialogManager>(new DialogManager(components)))
	, _commandManager(std::shared_ptr<Commands::CommandManager>(new Commands::CommandManager(playerPool)))
	, _classesComponent(components->queryComponent<IClassesComponent>())
	, _playerControllers(std::make_unique<ServiceLocator>())
	, _modes(std::make_unique<ServiceLocator>())
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
		throw std::runtime_error("DB_CONNECTION_STRING environment variable is not set!");
	}
	this->_dbConnection = std::make_shared<pqxx::connection>(dbConnectionString);
}

std::shared_ptr<CoreManager> CoreManager::create(IComponentList* components, ICore* core, IPlayerPool* playerPool)
{
	std::shared_ptr<CoreManager> pManager(new CoreManager(components, core, playerPool));
	pManager->initHandlers();
	return pManager;
}

CoreManager::~CoreManager()
{
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
	auto playerExt = new Player::OasisPlayerExt(data,
		player,
		components->queryComponent<ITimersComponent>());
	this->_playerData[player.getID()] = data;
	player.addExtension(playerExt, true);

	auto txdManager = playerExt->getTextDrawManager();
	auto logo = std::shared_ptr<TextDraws::ServerLogo>(new TextDraws::ServerLogo(
		player,
		"OASIS",
		"freeroam",
		"oasisfreeroam.xyz"));
	txdManager->add(TextDraws::ServerLogo::NAME, logo);
	logo->show();

	_playerPool->sendDeathMessageToAll(NULL, player, 200);
}

void CoreManager::onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason)
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
	_authController = std::make_unique<Auth::AuthController>(_playerPool, weak_from_this());

	_modes->registerInstance(Modes::Freeroam::FreeroamController::create(
		weak_from_this(),
		_playerPool));
	_modes->registerInstance(Modes::Deathmatch::DeathmatchController::create(
		weak_from_this(),
		_playerPool,
		components->queryComponent<ITimersComponent>()));
	_playerControllers->registerInstance(new Controllers::SpeedometerController(
		_playerPool,
		components->queryComponent<IVehiclesComponent>(),
		components->queryComponent<ITimersComponent>()));
}

std::shared_ptr<pqxx::connection> CoreManager::getDBConnection()
{
	return this->_dbConnection;
}

bool CoreManager::refreshPlayerData(IPlayer& player)
{
	auto db = this->getDBConnection();
	pqxx::work txn(*db);
	pqxx::result res = txn.exec_params(SQLQueryManager::Get()->getQueryByName(Utils::SQL::Queries::LOAD_PLAYER).value(), player.getName().to_string());

	if (res.size() == 0)
	{
		return false;
	}
	spdlog::info("Found player data for " + player.getName().to_string() + " in DB");

	auto row = res[0];
	this->getPlayerData(player)->updateFromRow(row);
	txn.commit();
	return true;
}

void CoreManager::savePlayer(std::shared_ptr<PlayerModel> data)
{
	if (!data->tempData->core->isLoggedIn)
		return;

	auto db = this->getDBConnection();
	pqxx::work txn(*db);

	txn.exec_params(
		SQLQueryManager::Get()->getQueryByName(Utils::SQL::Queries::SAVE_PLAYER).value(),
		data->language,
		data->lastSkinId,
		data->lastIP,
		data->lastLoginAt,
		data->userId);

	txn.commit();
	spdlog::info("Player {} has been successfully saved", data->name);
}

void CoreManager::saveAllPlayers()
{
	for (const auto& [id, playerData] : this->_playerData)
	{
		this->savePlayer(playerData);
	}
	spdlog::info("Saved all player data!");
}

void CoreManager::savePlayer(IPlayer& player)
{
	savePlayer(this->_playerData[player.getID()]);
}

void CoreManager::onFree(IComponent* component)
{
	if (component->getUID() == DialogsComponent_UID)
	{
		this->_dialogManager.reset();
	}
}

void CoreManager::initSkinSelection()
{
	IClassesComponent* classesComponent = this->components->queryComponent<IClassesComponent>();
	for (int i = 0; i <= 311; i++)
	{
		if (i == 74) // skip invalid skin
			continue;
		classesComponent->create(i,
			TEAM_NONE,
			Vector3(0, 0, 0),
			0.0,
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

		Vector4 classSelectionPoint = CLASS_SELECTION_POINTS[random() % CLASS_SELECTION_POINTS.size()];
		player.setPosition(Vector3(classSelectionPoint));

		auto playerExt = Player::getPlayerExt(player);
		playerExt->setFacingAngle(classSelectionPoint.w);
		player.setCameraLookAt(Vector3(classSelectionPoint), PlayerCameraCutType_Cut);
		auto angleRad = Utils::deg2Rad(classSelectionPoint.w);
		player.setCameraPosition(Vector3(
			classSelectionPoint.x + 5.0 * std::sin(-angleRad),
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
		Player::getPlayerExt(player)->sendErrorMessage(_("You are not logged in yet!", player));
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

void CoreManager::showModeSelectionDialog(IPlayer& player)
{
	this->getDialogManager()->createDialog(player, DialogStyle::DialogStyle_TABLIST_HEADERS, _("Modes", player),
		fmt::sprintf(_("Mode\tCommand\tPlayers\n"
					   "Freeroam\t/fr\t%d\n"
					   "Deathmatch\t/dm\t%d\n"
					   "Protect the President\t/ptp\t%d\n"
					   "Derby\t/derby\t%d\n"
					   "Cops and Robbers\t/cnr\t%d",
						 player),
			_modes->resolve<Modes::Freeroam::FreeroamController>()->playerCount(),
			_modes->resolve<Modes::Deathmatch::DeathmatchController>()->playerCount(),
			0,
			0,
			0),
		_("Select", player),
		"",
		[&](DialogResponse resp, int listItem, StringView inputText)
		{
			selectMode(player, static_cast<Modes::Mode>(listItem));
		});
}

void CoreManager::selectMode(IPlayer& player, Modes::Mode mode)
{
	switch (mode)
	{
	case Modes::Mode::Freeroam:
	{
		this->_modes->resolve<Modes::Freeroam::FreeroamController>()->onModeSelect(player);
		break;
	}
	case Modes::Mode::Deathmatch:
	{
		this->_modes->resolve<Modes::Deathmatch::DeathmatchController>()->onModeSelect(player);
		break;
	}
	default:
	{
		Player::getPlayerExt(player)->sendErrorMessage(_("Mode is not implemented yet!", player));
		this->showModeSelectionDialog(player);
		break;
	}
	}
}

void CoreManager::joinMode(IPlayer& player, Modes::Mode mode, std::unordered_map<std::string, Core::PrimitiveType> joinData)
{
	this->removePlayerFromCurrentMode(player);
	auto pData = this->getPlayerData(player);

	pData->tempData->core->lastMode = pData->tempData->core->currentMode;
	pData->tempData->core->currentMode = mode;

	std::shared_ptr<Modes::ModeBase> modeBase;
	switch (mode)
	{
	case Modes::Mode::Freeroam:
	{
		modeBase = static_pointer_cast<Modes::ModeBase>(this->_modes->resolve<Modes::Freeroam::FreeroamController>());
		break;
	}
	case Modes::Mode::Deathmatch:
	{
		modeBase = static_pointer_cast<Modes::ModeBase>(this->_modes->resolve<Modes::Deathmatch::DeathmatchController>());
		break;
	}
	default:
	{
		Player::getPlayerExt(player)->sendErrorMessage(_("Mode is not implemented yet!", player));
		this->showModeSelectionDialog(player);
		break;
	}
	}
	if (modeBase.get() == nullptr)
		return;
	modeBase->onModeJoin(player, joinData);
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
		modeBase = static_pointer_cast<Modes::ModeBase>(this->_modes->resolve<Modes::Freeroam::FreeroamController>());
		break;
	}
	case Modes::Mode::Deathmatch:
	{
		modeBase = static_pointer_cast<Modes::ModeBase>(this->_modes->resolve<Modes::Deathmatch::DeathmatchController>());
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

template <typename... T>
void CoreManager::sendNotificationToAllFormatted(const std::string& message, const T&... args)
{
	for (const auto& player : this->_playerPool->players())
	{
		Player::getPlayerExt(*player)->sendTranslatedMessageFormatted(message, args...);
	}
}

std::shared_ptr<Commands::CommandManager> CoreManager::getCommandManager()
{
	return _commandManager;
}

bool CoreManager::onPlayerText(IPlayer& player, StringView message)
{
	player.setChatBubble(message, Colour::White(), 100.0, Milliseconds(CHAT_BUBBLE_EXPIRATION));
	return true;
}

void CoreManager::onPlayerDeath(IPlayer& player, IPlayer* killer, int reason)
{
	_playerPool->sendDeathMessageToAll(killer, player, reason);
}
}