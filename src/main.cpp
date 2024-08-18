#include "core/CoreManager.hpp"
#include "core/utils/dotenv.h"
#include "core/utils/Localization.hpp"

#include <player.hpp>
#include <spdlog/spdlog.h>
#include <tinygettext/dictionary_manager.hpp>
#include <sdk.hpp>

#include <ctime>
#include <cstdlib>
#include <memory>

#define OASIS_GM_UID 0xF09FC558499EF9D7
#define OASIS_GAMEMODE_NAME "Oasis Freeroam"

class OasisGamemodeComponent final : public IComponent,
									 public PlayerTextEventHandler
{
private:
	ICore* _core = nullptr;
	IPlayerPool* playerPool = nullptr;
	bool _savedPlayerData = false;

	std::unique_ptr<Core::CoreManager> coreManager;

public:
	PROVIDE_UID(OASIS_GM_UID);

	// When this component is destroyed we need to tell any linked components
	// this it is gone.
	~OasisGamemodeComponent() { }

	// Implement the main component API.
	StringView componentName() const override { return "Oasis Gamemode"; }

	SemanticVersion componentVersion() const override
	{
		return SemanticVersion(0, 0, 1, 0);
	}

	void onLoad(ICore* c) override
	{
		// Cache core, player pool here
		_core = c;
		playerPool = &_core->getPlayers();
		_core->printLn("Oasis Gamemode has loaded");

		this->initTinygettext();
	}

	void initTinygettext()
	{
		Localization::gDictionaryManager.reset(
			new tinygettext::DictionaryManager());
		Localization::gDictionaryManager->add_directory("locale/po");
	}

	void onInit(IComponentList* components) override
	{
		dotenv::init();
		spdlog::set_level(spdlog::level::debug);

		auto db_connection_string = getenv("DB_CONNECTION_STRING");
		if (db_connection_string == NULL)
		{
			throw std::runtime_error(
				"DB_CONNECTION_STRING environment variable is not set!");
		}

		this->coreManager = Core::CoreManager::create(
			components, _core, playerPool, db_connection_string);

		srand(time(NULL));
		_core->printLn("Oasis Gamemode has initialized");
	}

	void onReady() override
	{
		// Fire events here at earliest.
		_core->setData(SettableCoreDataType::ModeText, OASIS_GAMEMODE_NAME);
		*_core->getConfig().getBool("game.use_entry_exit_markers") = false;
	}

	void onFree(IComponent* component) override
	{
		if (this->coreManager)
		{
			this->coreManager.reset();
		}
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
COMPONENT_ENTRY_POINT() { return new OasisGamemodeComponent(); }
