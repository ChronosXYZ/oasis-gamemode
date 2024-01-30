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
#include <Server/Components/Classes/classes.hpp>

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

using namespace std;

namespace Core
{
const inline static std::string CLASS_SELECTION = "classSelection";
const inline static std::string CURRENT_MODE = "currentMode";

class CoreManager : public PlayerConnectEventHandler,
					public PlayerTextEventHandler,
					public std::enable_shared_from_this<CoreManager>,
					public ClassEventHandler,
					public PlayerSpawnEventHandler
{
public:
	IComponentList* const components;

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
	bool onPlayerRequestClass(IPlayer& player, unsigned int classId) override;
	bool onPlayerRequestSpawn(IPlayer& player) override;

	void onPlayerLoggedIn(IPlayer& player);

	void onFree(IComponent* component);

private:
	CoreManager(IComponentList* components, ICore* core, IPlayerPool* playerPool);

	void initHandlers();
	void initSkinSelection();
	void callCommandHandler(string cmdName, Utils::CallbackValuesType args);
	void savePlayer(IPlayer& player);
	void savePlayer(std::shared_ptr<PlayerModel> data);
	void saveAllPlayers();
	void showModeSelectionDialog(IPlayer& player);

	IPlayerPool* const _playerPool = nullptr;
	ICore* const _core = nullptr;
	IClassesComponent* const _classesComponent;

	shared_ptr<DialogManager> _dialogManager;
	shared_ptr<pqxx::connection> _dbConnection;
	std::map<unsigned int, shared_ptr<PlayerModel>> _playerData;
	std::map<Modes::Mode, unsigned int> _modePlayerCount {
		{ Modes::Mode::Freeroam, 0 },
		{ Modes::Mode::Deathmatch, 0 },
		{ Modes::Mode::Derby, 0 },
		{ Modes::Mode::PTP, 0 },
		{ Modes::Mode::CnR, 0 }
	};

	// Handlers
	unique_ptr<AuthHandler> _authHandler;
	unique_ptr<Modes::Freeroam::FreeroamHandler> _freeroam;

	map<string, unique_ptr<Utils::CommandCallback>> _commandHandlers;
};
}