#pragma once

#include <functional>
#include <map>
#include <sdk.hpp>
#include <Server/Components/Dialogs/dialogs.hpp>

#define MAGIC_DIALOG_ID 1337

namespace Core
{
class DialogManager : public PlayerDialogEventHandler
{
	typedef std::function<void(DialogResponse, int, StringView)> Callback;

public:
	DialogManager(IComponentList* components);
	~DialogManager();

	void onDialogResponse(IPlayer& player, int dialogId, DialogResponse response, int listItem, StringView inputText) override;

	void createDialog(IPlayer& player,
		DialogStyle style,
		StringView title,
		StringView body,
		StringView button1,
		StringView button2,
		DialogManager::Callback callback);
	void hideDialog(IPlayer& player);

private:
	// player id -> dialog callback
	std::map<unsigned int, DialogManager::Callback> dialogs;
	IDialogsComponent* dialogsComponent = nullptr;
};
}