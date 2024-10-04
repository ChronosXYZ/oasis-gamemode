#pragma once

#include "DialogResult.hpp"
#include "Dialogs.hpp"
#include "IDialog.hpp"
#include <Server/Components/Dialogs/dialogs.hpp>

#include <functional>
#include <map>
#include <memory>

#define MAGIC_DIALOG_ID 1337

#define ACCENT_MAIN 0xFF0000FF // SERVER COLOR
#define ACCENT_SECONDARY 0xFFFFFFFF // SERVER COLOR
#define ACCENT_MAIN_E "{FF0000}" // SERVER COLOR Embedded
#define ACCENT_SECONDARY_E "{FFFFFF}" // SERVER COLOR Embedded

#define DIALOG_HEADER_TITLE "" ACCENT_MAIN_E "Oasis" ACCENT_SECONDARY_E " | %s"
#define DIALOG_TABLIST_TITLE_COLOR ACCENT_MAIN_E // Embedded
#define DIALOG_HEADER DIALOG_HEADER_TITLE // embedded
#define DIALOG_TABLIST DIALOG_TABLIST_TITLE_COLOR

namespace Core
{
class DialogManager : public PlayerDialogEventHandler
{
	typedef std::function<void(DialogResult)> Callback;

public:
	DialogManager(IComponentList* components);
	~DialogManager();

	void onDialogResponse(IPlayer& player, int dialogId,
		DialogResponse response, int listItem, StringView inputText) override;

	void showDialog(IPlayer& player, std::shared_ptr<InputDialog> dialog,
		DialogManager::Callback callback);
	void showDialog(IPlayer& player, std::shared_ptr<ListDialog> dialog,
		DialogManager::Callback callback);
	void showDialog(IPlayer& player, std::shared_ptr<MessageDialog> dialog,
		DialogManager::Callback callback);
	void showDialog(IPlayer& player, std::shared_ptr<TabListDialog> dialog,
		DialogManager::Callback callback);
	void showDialog(IPlayer& player,
		std::shared_ptr<TabListHeadersDialog> dialog,
		DialogManager::Callback callback);

	void addSettingsDialog(
		unsigned int playerId, std::shared_ptr<SettingsDialog> dialog);
	void removeSettingsDialog(unsigned int playerId);

	void hideDialog(IPlayer& player);

private:
	// player id -> dialog callback
	std::map<unsigned int, DialogManager::Callback> dialogs;
	std::unordered_map<unsigned int, std::shared_ptr<SettingsDialog>>
		settingsDialogs;
	IDialogsComponent* dialogsComponent = nullptr;
	void showDialog(IPlayer& player, std::shared_ptr<IDialog> dialog,
		DialogManager::Callback callback);
};
}