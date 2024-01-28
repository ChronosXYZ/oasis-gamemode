#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <map>
#include <exception>
#include <stdexcept>
#include <variant>
#include <string>

#include <component.hpp>
#include <player.hpp>
#include <spdlog/spdlog.h>
#include <pqxx/pqxx>
#include <sdk.hpp>

#include "player/PlayerExtension.hpp"
#include "player/PlayerExtension.hpp"
#include "player/PlayerModel.hpp"
#include "DialogManager.hpp"
#include "SQLQueryManager.hpp"
#include "auth/AuthHandler.hpp"
#include "utils/Common.hpp"
#include "utils/Strings.hpp"
#include "utils/QueryNames.hpp"

using namespace std;

namespace Core
{
class CoreManager : public PlayerConnectEventHandler, public PlayerTextEventHandler, public std::enable_shared_from_this<CoreManager>
{
public:
	const IComponentList* components;

	static shared_ptr<CoreManager> create(IComponentList* components, ICore* core, IPlayerPool* playerPool);
	~CoreManager();

	shared_ptr<PlayerModel> getPlayerData(IPlayer& player);
	OasisPlayerExt* getPlayerExt(IPlayer& player);
	shared_ptr<DialogManager> getDialogManager();
	shared_ptr<pqxx::connection> getDBConnection();

	template <typename F>
		requires Utils::callback_function<F, reference_wrapper<IPlayer>, double, int, std::string>
	void addCommand(std::string name, F handler);
	bool refreshPlayerData(IPlayer& player);

	void onPlayerConnect(IPlayer& player) override;
	void onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason) override;
	bool onPlayerCommandText(IPlayer& player, StringView commandText) override;

	void onFree(IComponent* component);

private:
	CoreManager(IComponentList* components, ICore* core, IPlayerPool* playerPool);

	void initHandlers();
	void callCommandHandler(string cmdName, Utils::CallbackValuesType args);
	void savePlayer(IPlayer& player);
	void savePlayer(std::shared_ptr<PlayerModel> data);
	void saveAllPlayers();

	IPlayerPool* const _playerPool = nullptr;
	ICore* const _core = nullptr;

	shared_ptr<DialogManager> _dialogManager;
	shared_ptr<pqxx::connection> _dbConnection;
	std::map<unsigned int, shared_ptr<PlayerModel>> _playerData;

	// Handlers
	unique_ptr<AuthHandler> _authHandler;

	map<string, unique_ptr<Utils::CommandCallback>> _commandHandlers;
};
}