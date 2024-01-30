#include "CoreManager.hpp"
#include "Server/Components/Classes/classes.hpp"
#include <memory>

namespace Core
{
CoreManager::CoreManager(IComponentList* components, ICore* core, IPlayerPool* playerPool)
	: components(components)
	, _core(core)
	, _playerPool(playerPool)
	, _dialogManager(make_shared<DialogManager>(components))
	, _classesComponent(components->queryComponent<IClassesComponent>())
{
	this->initSkinSelection();

	_playerPool->getPlayerConnectDispatcher().addEventHandler(this);
	_playerPool->getPlayerTextDispatcher().addEventHandler(this);
	_playerPool->getPlayerSpawnDispatcher().addEventHandler(this);

	_classesComponent->getEventDispatcher().addEventHandler(this);

	this->_dbConnection = make_shared<pqxx::connection>(getenv("DB_CONNECTION_STRING"));
}

shared_ptr<CoreManager> CoreManager::create(IComponentList* components, ICore* core, IPlayerPool* playerPool)
{
	shared_ptr<CoreManager> pManager(new CoreManager(components, core, playerPool));
	pManager->initHandlers();
	return pManager;
}

CoreManager::~CoreManager()
{
	saveAllPlayers();
	_playerPool->getPlayerConnectDispatcher().removeEventHandler(this);
	_playerPool->getPlayerTextDispatcher().removeEventHandler(this);
	_playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);

	_classesComponent->getEventDispatcher().removeEventHandler(this);
}

shared_ptr<PlayerModel> CoreManager::getPlayerData(IPlayer& player)
{
	auto ext = queryExtension<OasisPlayerExt>(player);
	if (ext)
	{
		if (auto data = ext->getPlayerData())
		{
			return data;
		}
	}
	return {};
}

OasisPlayerExt* CoreManager::getPlayerExt(IPlayer& player)
{
	return queryExtension<OasisPlayerExt>(player);
}

void CoreManager::onPlayerConnect(IPlayer& player)
{
	auto data = std::shared_ptr<PlayerModel>(new PlayerModel());
	this->_playerData[player.getID()] = data;
	player.addExtension(new OasisPlayerExt(data, player), true);
}

void CoreManager::onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason)
{
	this->savePlayer(player);
	this->_playerData.erase(player.getID());
}

shared_ptr<DialogManager> CoreManager::getDialogManager()
{
	return this->_dialogManager;
}

void CoreManager::initHandlers()
{
	_authHandler = make_unique<AuthHandler>(_playerPool, weak_from_this());

	_freeroam = make_unique<Modes::Freeroam::FreeroamHandler>(weak_from_this(), _playerPool);

	// FIXME move this definition outta here
	this->addCommand("kill", [](reference_wrapper<IPlayer> player)
		{
			player.get().setHealth(0.0);
			player.get().sendClientMessage(Colour::Yellow(), "You have killed yourself!");
		});
	this->addCommand("skin", [&](reference_wrapper<IPlayer> player, int skinId)
		{
			if (skinId < 0 || skinId > 311)
			{
				player.get().sendClientMessage(consts::RED_COLOR, _("[ERROR] #WHITE#Invalid skin ID!", player));
				return;
			}
			player.get().setSkin(skinId);
			auto data = this->getPlayerData(player.get());
			data->lastSkinId = skinId;
			player.get().sendClientMessage(Colour::Yellow(), fmt::sprintf("You have changed your skin to ID: %d!", skinId));
		});
}

shared_ptr<pqxx::connection> CoreManager::getDBConnection()
{
	return this->_dbConnection;
}

template <typename F>
	requires Utils::callback_function<F, reference_wrapper<IPlayer>, double, int, string>
void CoreManager::addCommand(string name, F handler)
{
	this->_commandHandlers["/" + name] = std::unique_ptr<Utils::CommandCallback>(new Utils::CommandCallback(handler));
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

bool CoreManager::onPlayerCommandText(IPlayer& player, StringView commandText)
{
	auto cmdText = commandText.to_string();
	auto cmdParts = Utils::Strings::split(cmdText, ' ');
	assert(!cmdParts.empty());

	// get command name and pop the first element from the vector
	auto commandName = cmdParts.at(0);
	cmdParts.erase(cmdParts.begin());

	if (!this->_commandHandlers.contains(commandName))
	{
		return false;
	}
	Utils::CallbackValuesType args;
	args.push_back(player);
	for (auto& part : cmdParts)
	{
		if (Utils::Strings::isNumber<int>(part))
		{
			args.push_back(stoi(part));
		}
		else if (Utils::Strings::isNumber<double>(part))
		{
			args.push_back(stod(part));
		}
		else
		{
			args.push_back(part);
		}
	}
	try
	{
		this->callCommandHandler(commandName, args);
	}
	catch (const std::bad_variant_access&)
	{
		player.sendClientMessage(consts::RED_COLOR, _("[ERROR] #WHITE#Invalid command parameters!", player));
	}
	catch (const exception& e)
	{
		spdlog::debug("Failed to invoke command: {}", e.what());
		player.sendClientMessage(consts::RED_COLOR, _("[Error] #WHITE#Failed to invoke command!", player));
	}
	return true;
}

void CoreManager::callCommandHandler(string cmdName, Utils::CallbackValuesType args)
{
	(*this->_commandHandlers[cmdName])(args);
}

void CoreManager::savePlayer(shared_ptr<PlayerModel> data)
{
	auto db = this->getDBConnection();
	pqxx::work txn(*db);

	txn.exec_params(
		SQLQueryManager::Get()->getQueryByName(Utils::SQL::Queries::SAVE_PLAYER).value(),
		data->language,
		data->lastSkinId,
		data->lastIP,
		data->lastLoginAt,
		data->account_id);

	txn.commit();
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
	if (!pData->getTempData(CLASS_SELECTION))
	{
		// first player request class call
		pData->setTempData(CLASS_SELECTION, true);
		player.setSkin(pData->lastSkinId);
		return true;
	}
	return true;
}

void CoreManager::onPlayerLoggedIn(IPlayer& player)
{
	Vector4 classSelectionPoint = consts::CLASS_SELECTION_POINTS[random() % consts::CLASS_SELECTION_POINTS.size()];
	player.setPosition(Vector3(classSelectionPoint));

	auto playerExt = this->getPlayerExt(player);
	playerExt->setFacingAngle(classSelectionPoint.w);
	player.setCameraLookAt(Vector3(classSelectionPoint), PlayerCameraCutType_Cut);
	auto angleRad = Utils::deg2Rad(classSelectionPoint.w);
	player.setCameraPosition(Vector3(
		classSelectionPoint.x + 5.0 * std::sin(-angleRad),
		classSelectionPoint.y + 5.0 * std::cos(-angleRad),
		classSelectionPoint.z));

	auto pData = this->getPlayerData(player);
}

bool CoreManager::onPlayerRequestSpawn(IPlayer& player)
{
	auto pData = this->getPlayerData(player);
	if (pData->getTempData(Core::CLASS_SELECTION))
	{
		pData->deleteTempData(Core::CLASS_SELECTION);
	}
	pData->lastSkinId = player.getSkin();

	showModeSelectionDialog(player);

	return false;
}

void CoreManager::showModeSelectionDialog(IPlayer& player)
{
	this->getDialogManager()->createDialog(player, DialogStyle::DialogStyle_LIST, _("Modes", player),
		_("Freeroam\nDeathmatch\nProtect the President\nDerby\nCops and Robbers", player),
		_("Select", player),
		"",
		[&](DialogResponse resp, int listItem, StringView inputText)
		{
			auto pData = this->getPlayerData(player);
			switch (listItem)
			{
			case 0:
			{
				pData->setTempData(CURRENT_MODE, Modes::Freeroam::MODE_NAME);
				player.setVirtualWorld(0);
				player.spawn();
				break;
			}
			default:
			{
				player.sendClientMessage(consts::RED_COLOR, _("[Error] #WHITE#Mode is not implemented yet!", player));
				this->showModeSelectionDialog(player);
				break;
			}
			}
		});
}
}