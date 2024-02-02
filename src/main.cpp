#include "constants.hpp"
#include "core/CoreManager.hpp"
#include "core/SQLQueryManager.hpp"
#include "core/utils/dotenv.h"
#include "core/utils/Localization.hpp"

#include <player.hpp>
#include <spdlog/spdlog.h>
#include <tinygettext/dictionary_manager.hpp>
#include <Server/Components/Classes/classes.hpp>
#include <sdk.hpp>

#include <functional>
#include <ctime>
#include <cstdlib>
#include <memory>

#define OASIS_GM_UID 0xF09FC558499EF9D7
#define OASIS_GAMEMODE_NAME "Oasis Freeroam"

class OasisGamemodeComponent final : public IComponent, public PlayerSpawnEventHandler, public PlayerConnectEventHandler, public PlayerTextEventHandler
{
private:
	ICore* _core = nullptr;
	IPlayerPool* playerPool = nullptr;
	bool _savedPlayerData = false;

	std::shared_ptr<Core::CoreManager> coreManager;

public:
	PROVIDE_UID(OASIS_GM_UID);

	// When this component is destroyed we need to tell any linked components this it is gone.
	~OasisGamemodeComponent()
	{
		playerPool->getPlayerConnectDispatcher().removeEventHandler(this);
		playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);
	}

	// Implement the main component API.
	StringView componentName() const override
	{
		return "Oasis Gamemode";
	}

	SemanticVersion componentVersion() const override
	{
		return SemanticVersion(0, 0, 1, 0);
	}

	void onLoad(ICore* c) override
	{
		// Cache core, player pool here
		_core = c;
		playerPool = &_core->getPlayers();
		_core->printLn("Oasis Gamemode loaded.");

		this->initTinygettext();
	}

	void initTinygettext()
	{
		Localization::gDictionaryManager.reset(new tinygettext::DictionaryManager());
		Localization::gDictionaryManager->add_directory("locale/po");
	}

	void onInit(IComponentList* components) override
	{
		dotenv::init();
		spdlog::set_level(spdlog::level::debug);

		// dispatch player callbacks
		playerPool->getPlayerConnectDispatcher().addEventHandler(this);
		playerPool->getPlayerSpawnDispatcher().addEventHandler(this);

		this->coreManager = Core::CoreManager::create(components, _core, playerPool);

		srand(time(NULL));
	}

	void onPlayerConnect(IPlayer& player) override
	{
	}

	void onPlayerSpawn(IPlayer& player) override
	{
	}

	void onReady() override
	{
		// Fire events here at earliest.
		_core->setData(SettableCoreDataType::ModeText, OASIS_GAMEMODE_NAME);
		*_core->getConfig().getBool("game.use_entry_exit_markers") = false;
	}

	void onFree(IComponent* component) override
	{
		this->coreManager->onFree(component);
	}

	void free() override
	{
		// Deletes the component.
		delete this;
	}

	void reset() override
	{
		// Resets data when the mode changes (GMX).
	}
};

// Automatically called when the compiled binary is loaded.
COMPONENT_ENTRY_POINT()
{
	return new OasisGamemodeComponent();
}
