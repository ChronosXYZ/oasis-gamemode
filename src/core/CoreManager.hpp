#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <map>
#include <exception>
#include <stdexcept>
#include <variant>
#include <string>
#include <set>

#include <component.hpp>
#include <player.hpp>
#include <spdlog/spdlog.h>
#include <pqxx/pqxx>
#include <sdk.hpp>
#include <Server/Components/Classes/classes.hpp>

#include "PlayerVars.hpp"
#include "player/PlayerExtension.hpp"
#include "player/PlayerExtension.hpp"
#include "player/PlayerModel.hpp"
#include "DialogManager.hpp"
#include "SQLQueryManager.hpp"
#include "auth/AuthHandler.hpp"
#include "utils/Common.hpp"
#include "utils/Strings.hpp"
#include "utils/QueryNames.hpp"

#include "../modes/freeroam/Freeroam.hpp"
#include "../modes/Constants.hpp"

namespace Core
{
class CoreManager : public PlayerConnectEventHandler,
					public PlayerTextEventHandler,
					public std::enable_shared_from_this<CoreManager>,
					public ClassEventHandler,
					public PlayerSpawnEventHandler
{
public:
	IComponentList* const components;

	static std::shared_ptr<CoreManager> create(IComponentList* components, ICore* core, IPlayerPool* playerPool);
	~CoreManager();

	std::shared_ptr<PlayerModel> getPlayerData(IPlayer& player);
	std::shared_ptr<DialogManager> getDialogManager();
	std::shared_ptr<pqxx::connection> getDBConnection();

	template <typename F>
		requires Utils::callback_function<F, std::reference_wrapper<IPlayer>, double, int, std::string>
	void addCommand(std::string name, F handler)
	{
		this->_commandHandlers["/" + name] = std::unique_ptr<Utils::CommandCallback>(new Utils::CommandCallback(handler));
	};
	bool refreshPlayerData(IPlayer& player);
	void selectMode(IPlayer& player, Modes::Mode mode);

	void onPlayerConnect(IPlayer& player) override;
	void onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason) override;
	bool onPlayerCommandText(IPlayer& player, StringView commandText) override;
	bool onPlayerRequestClass(IPlayer& player, unsigned int classId) override;
	bool onPlayerRequestSpawn(IPlayer& player) override;

	void onPlayerLoggedIn(IPlayer& player);

	void onFree(IComponent* component);

private:
	CoreManager(IComponentList* components, ICore* core, IPlayerPool* playerPool);

	void initHandlers();
	void initSkinSelection();
	void callCommandHandler(const std::string& cmdName, Utils::CallbackValuesType args);
	void savePlayer(IPlayer& player);
	void savePlayer(std::shared_ptr<PlayerModel> data);
	void saveAllPlayers();
	void showModeSelectionDialog(IPlayer& player);
	void removePlayerFromModes(IPlayer& player);

	IPlayerPool* const _playerPool = nullptr;
	ICore* const _core = nullptr;
	IClassesComponent* const _classesComponent;

	std::shared_ptr<DialogManager> _dialogManager;
	std::shared_ptr<pqxx::connection> _dbConnection;
	std::map<unsigned int, std::shared_ptr<PlayerModel>> _playerData;
	std::map<Modes::Mode, std::set<unsigned int>> _modePlayerCount {
		{ Modes::Mode::Freeroam, {} },
		{ Modes::Mode::Deathmatch, {} },
		{ Modes::Mode::Derby, {} },
		{ Modes::Mode::PTP, {} },
		{ Modes::Mode::CnR, {} }
	};

	// Handlers
	std::unique_ptr<Auth::AuthHandler> _authHandler;
	std::unique_ptr<Modes::Freeroam::FreeroamHandler> _freeroam;

	std::unordered_map<std::string, std::unique_ptr<Utils::CommandCallback>> _commandHandlers;
};
}