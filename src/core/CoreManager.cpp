#include "CoreManager.hpp"
#include "utils/Common.hpp"
#include <functional>
#include <memory>
#include <type_traits>

using namespace Core;

CoreManager::CoreManager(IComponentList* components, ICore* core, IPlayerPool* playerPool)
	: components(components)
	, _core(core)
	, _playerPool(playerPool)
	, _dialogManager(make_shared<DialogManager>(components))
{
	playerPool->getPlayerConnectDispatcher().addEventHandler(this);
	playerPool->getPlayerTextDispatcher().addEventHandler(this);

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
	_playerPool->getPlayerConnectDispatcher().removeEventHandler(this);
	_playerPool->getPlayerTextDispatcher().removeEventHandler(this);
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
	player.addExtension(new OasisPlayerExt(std::shared_ptr<PlayerModel>(new PlayerModel()), player), true);
}

void CoreManager::onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason)
{
}

shared_ptr<DialogManager> CoreManager::getDialogManager()
{
	return std::shared_ptr<DialogManager>(this->_dialogManager);
}

void CoreManager::initHandlers()
{
	_authHandler = make_unique<AuthHandler>(_playerPool, weak_from_this());

	this->addCommand("kill", [](reference_wrapper<IPlayer> player)
		{
			player.get().setHealth(0.0);
			player.get().sendClientMessage(Colour::Yellow(), "You have killed yourself!");
		});
}

shared_ptr<pqxx::connection> CoreManager::getDBConnection()
{
	return this->_dbConnection;
}

template <typename F>
void CoreManager::addCommand(string name, F handler)
{
	// static_assert(is_invocable_v<F>, "HER");
	this->_commandHandlers["/" + name] = std::unique_ptr<Utils::CommandCallback>(new Utils::CommandCallback(handler));
}

bool CoreManager::refreshPlayerData(IPlayer& player)
{
	auto db = this->getDBConnection();
	pqxx::work txn(*db);
	pqxx::result res = txn.exec_params("SELECT id, \
					name, \
					users.password_hash, \
					\"language\", \
					email, \
					last_skin_id, \
					last_ip, \
					last_login_at, \
					bans.reason as \"ban_reason\", \
					bans.by as \"banned_by\", \
					bans.expires_at as \"ban_expires_at\", \
					admins.\"level\" as \"admin_level\", \
					admins.password_hash as \"admin_pass_hash\" \
					FROM users \
					LEFT JOIN bans \
					ON users.id = bans.user_id \
					LEFT JOIN admins \
					ON users.id = admins.user_id \
					WHERE name=$1",
		player.getName().to_string());

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
		if (Utils::Strings::isNumber(part))
		{
			try
			{
				args.push_back(atoi(part.c_str()));
			}
			catch (...)
			{
				spdlog::debug("invalid number passed to the command");
			}
			continue;
		}
		else if (Utils::Strings::isDouble(part))
		{
			args.push_back(std::stod(part));
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
	catch (exception& e)
	{
		spdlog::debug("Failed to invoke command: {}", e.what());
		player.sendClientMessage(consts::RED_COLOR, "Invalid command parameters!");
	}
	return true;
}

void CoreManager::callCommandHandler(string cmdName, Utils::CallbackValuesType args)
{
	(*this->_commandHandlers[cmdName])(args);
}