// Required for most of open.mp.
#include <sdk.hpp>

// Include the vehicle component information.
// #include <Server/Components/Vehicles/vehicles.hpp>

#include "constants.hpp"

class OasisGamemodeComponent final : public IComponent
{
private:
	// Hold a reference to the main server core.
	ICore* core_ = nullptr;

public:
	// Visit https://open.mp/uid to generate a new unique ID.
	PROVIDE_UID(OASIS_GM_UID);

	// When this component is destroyed we need to tell any linked components this it is gone.
	~OasisGamemodeComponent()
	{
		// Clean up what you did above.
		// if (vehicles_)
		// {
		// 	vehicles_->getPoolEventDispatcher().removeEventHandler(this);
		// }
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
		core_ = c;
		core_->printLn("Oasis Gamemode loaded.");
	}

	void onInit(IComponentList* components) override
	{
		// Cache components, add event handlers here.
		// vehicles_ = components->queryComponent<IVehiclesComponent>();
		// if (vehicles_)
		// {
		// 	vehicles_->getPoolEventDispatcher().addEventHandler(this);
		// }
	}

	void onReady() override
	{
		// Fire events here at earliest.
	}

	void onFree(IComponent* component) override
	{
		// Invalidate vehicles_ pointer so it can't be used past this point.
		// if (component == vehicles_)
		// {
		// 	vehicles_ = nullptr;
		// }
	}

	void free() override
	{
		// Deletes the component.
		delete this;
	}

	void reset() override
	{
		// Resets data when the mode changes.
	}
};

// Automatically called when the compiled binary is loaded.
COMPONENT_ENTRY_POINT()
{
	return new OasisGamemodeComponent();
}
