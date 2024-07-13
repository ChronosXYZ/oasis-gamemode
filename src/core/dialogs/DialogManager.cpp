#include "DialogManager.hpp"

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

	this->dialogs[player.getID()](response, listItem, inputText);
}

void DialogManager::hideDialog(IPlayer& player)
{
	IPlayerDialogData* dialogData = queryExtension<IPlayerDialogData>(player);
	dialogData->hide(player);
}

void DialogManager::createDialog(IPlayer& player, DialogStyle style,
	StringView title, StringView body, StringView button1, StringView button2,
	DialogManager::Callback callback)
{
	this->dialogs[player.getID()] = callback;

	IPlayerDialogData* dialogData = queryExtension<IPlayerDialogData>(player);
	dialogData->show(
		player, MAGIC_DIALOG_ID, style, title, body, button1, button2);
}

}