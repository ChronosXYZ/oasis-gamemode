#pragma once

#include <Server/Components/Dialogs/dialogs.hpp>

#include <functional>
#include <map>

#define MAGIC_DIALOG_ID 1337

#define ACCENT_MAIN 0xFF0000FF // SERVER COLOR
#define ACCENT_SECONDARY 0xFFFFFFFF // SERVER COLOR
#define ACCENT_MAIN_E "{FF0000}" // SERVER COLOR Embedded
#define ACCENT_SECONDARY_E "{FFFFFF}" // SERVER COLOR Embedded

#define DIALOG_HEADER_TITLE "" ACCENT_MAIN_E "Oasis " ACCENT_SECONDARY_E " | %s"
#define DIALOG_TABLIST_TITLE_COLOR ACCENT_MAIN_E // Embedded
#define DIALOG_HEADER DIALOG_HEADER_TITLE // embedded
#define DIALOG_TABLIST DIALOG_TABLIST_TITLE_COLOR

namespace Core
{
class DialogManager : public PlayerDialogEventHandler
{
	typedef std::function<void(DialogResponse, int, StringView)> Callback;

public:
	DialogManager(IComponentList* components);
	~DialogManager();

	void onDialogResponse(IPlayer& player, int dialogId,
		DialogResponse response, int listItem, StringView inputText) override;

	void createDialog(IPlayer& player, DialogStyle style, StringView title,
		StringView body, StringView button1, StringView button2,
		DialogManager::Callback callback);
	void hideDialog(IPlayer& player);

private:
	// player id -> dialog callback
	std::map<unsigned int, DialogManager::Callback> dialogs;
	IDialogsComponent* dialogsComponent = nullptr;
};
}