#include "DialogManager.hpp"
#include "DialogResult.hpp"
#include "IDialog.hpp"
#include <memory>

namespace Core
{

DialogManager::DialogManager(IComponentList* components)
{
	IDialogsComponent* dialogsComponent
		= components->queryComponent<IDialogsComponent>();
	this->dialogsComponent = dialogsComponent;
	this->dialogsComponent->getEventDispatcher().addEventHandler(this);
}

DialogManager::~DialogManager()
{
	this->dialogsComponent->getEventDispatcher().removeEventHandler(this);
}

void DialogManager::onDialogResponse(IPlayer& player, int dialogId,
	DialogResponse response, int listItem, StringView inputText)
{
	if (dialogId != MAGIC_DIALOG_ID)
		return;

	if (this->dialogs.count(player.getID()) == 0)
		return;

	this->dialogs[player.getID()](
		DialogResult(response, listItem, inputText.to_string()));
}

void DialogManager::hideDialog(IPlayer& player)
{
	IPlayerDialogData* dialogData = queryExtension<IPlayerDialogData>(player);
	dialogData->hide(player);
}

void DialogManager::showDialog(IPlayer& player,
	std::shared_ptr<InputDialog> dialog, DialogManager::Callback callback)
{
	this->showDialog(
		player, std::static_pointer_cast<IDialog>(dialog), callback);
}

void DialogManager::showDialog(IPlayer& player,
	std::shared_ptr<ListDialog> dialog, DialogManager::Callback callback)
{
	this->showDialog(
		player, std::static_pointer_cast<IDialog>(dialog), callback);
}

void DialogManager::showDialog(IPlayer& player,
	std::shared_ptr<MessageDialog> dialog, DialogManager::Callback callback)
{
	this->showDialog(
		player, std::static_pointer_cast<IDialog>(dialog), callback);
}

void DialogManager::showDialog(IPlayer& player,
	std::shared_ptr<TabListDialog> dialog, DialogManager::Callback callback)
{
	this->showDialog(
		player, std::static_pointer_cast<IDialog>(dialog), callback);
}

void DialogManager::showDialog(IPlayer& player,
	std::shared_ptr<TabListHeadersDialog> dialog,
	DialogManager::Callback callback)
{
	this->showDialog(
		player, std::static_pointer_cast<IDialog>(dialog), callback);
}

void DialogManager::showDialog(IPlayer& player, std::shared_ptr<IDialog> dialog,
	DialogManager::Callback callback)
{
	this->dialogs[player.getID()] = callback;

	IPlayerDialogData* dialogData = queryExtension<IPlayerDialogData>(player);
	dialogData->show(player, MAGIC_DIALOG_ID, dialog->style, dialog->title,
		dialog->content, dialog->button1, dialog->button2);
}

}