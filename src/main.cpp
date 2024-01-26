// Required for most of open.mp.
#include <functional>
#include <sdk.hpp>
#include <ctime>
#include <cstdlib>
#include <memory>

#include <Server/Components/Classes/classes.hpp>

#include "constants.hpp"
#include "core/CoreManager.hpp"
#include "core/utils/dotenv.h"
#include "core/utils/LocaleUtils.hpp"
#include "player.hpp"
#include "tinygettext/dictionary_manager.hpp"

class OasisGamemodeComponent final : public IComponent, public PlayerSpawnEventHandler, public PlayerConnectEventHandler, public PlayerTextEventHandler
{
private:
	ICore* _core = nullptr;
	IPlayerPool* playerPool = nullptr;

	shared_ptr<Core::CoreManager> coreManager;

public:
	PROVIDE_UID(OASIS_GM_UID);

	// When this component is destroyed we need to tell any linked components this it is gone.
	~OasisGamemodeComponent()
	{
		playerPool->getPlayerConnectDispatcher().removeEventHandler(this);
		playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);
		_core = nullptr;
		playerPool = nullptr;
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
		Locale::gDictionaryManager.reset(new tinygettext::DictionaryManager());
		Locale::gDictionaryManager->add_directory("locale/po");
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
		if (auto pData = this->coreManager->getPlayerData(player))
			player.setSkin(pData->lastSkinId);
		player.setPosition(consts::randomSpawnArray[rand() % consts::randomSpawnArray.size()]);
		player.sendClientMessage(Colour::White(), _("Hello, World!", player));
	}

	void onReady() override
	{
		// Fire events here at earliest.
		_core->setData(SettableCoreDataType::ModeText, "Oasis Freeroam");
		*_core->getConfig().getBool("game.use_entry_exit_markers") = false;

		// this->coreManager->addCommand("kill", [](reference_wrapper<IPlayer> player)
		// 	{
		// 		player.get().setHealth(0.0);
		// 		player.get().sendClientMessage(Colour::Yellow(), "You have killed yourself!");
		// 	});
	}

	void onFree(IComponent* component) override
	{
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
