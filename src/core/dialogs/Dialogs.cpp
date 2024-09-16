#include "Dialogs.hpp"

#include "../utils/Strings.hpp"
#include "../utils/Localization.hpp"

#include "DialogResult.hpp"
#include "player.hpp"
#include <Server/Components/Dialogs/dialogs.hpp>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "DialogManager.hpp"
#include "../player/PlayerExtension.hpp"

namespace Core
{
InputDialog::InputDialog(const std::string& title, const std::string& content,
	bool isPassword, const std::string& button1, const std::string& button2)
	: super(DialogStyle_INPUT, title, content, button1, button2)
	, isPassword(isPassword)
{
	this->title = title;
	this->content = content;
	this->isPassword = isPassword;
	if (isPassword)
		this->style = DialogStyle_PASSWORD;
	this->leftButton = button1;
	this->rightButton = button2;
}

ListDialog::ListDialog(const std::string& title,
	const std::vector<std::string> items, const std::string& button1,
	const std::string& button2)
	: super(DialogStyle_LIST, title, "", button1, button2)
	, items(items)
{
	std::string body;
	for (const auto& item : items)
	{
		body += Utils::Strings::trim_copy(item) + "\n";
	}
	this->content = body;
}

MessageDialog::MessageDialog(const std::string& title,
	const std::string& content, const std::string& button1,
	const std::string& button2)
	: super(DialogStyle_MSGBOX, title, content, button1, button2)
{
}

TabListHeadersDialog::TabListHeadersDialog(const std::string& title,
	const std::vector<std::string> columns,
	std::vector<std::vector<std::string>> items, const std::string& button1,
	const std::string& button2)
	: super(DialogStyle_TABLIST_HEADERS, title, "", button1, button2)
	, items(items)
{
	std::string body;
	for (const auto& column : columns)
	{
		body += Utils::Strings::trim_copy(column) + "\t";
	}
	body.erase(body.length() - 1);
	body += "\n";
	for (const auto& row : items)
	{
		for (const auto& value : row)
		{
			body += Utils::Strings::trim_copy(value) + "\t";
		}
		body.erase(body.length() - 1);
		body += "\n";
	}
	body.erase(body.length() - 1);

	this->content = body;
}

TabListDialog::TabListDialog(const std::string& title,
	std::vector<std::vector<std::string>> items, const std::string& button1,
	const std::string& button2)
	: super(DialogStyle_TABLIST, title, "", button1, button2)
	, items(items)
{
	std::string body;
	for (const auto& row : items)
	{
		for (const auto& value : row)
		{
			body += Utils::Strings::trim_copy(value) + "\t";
		}
		body.erase(body.length() - 1);
		body += "\n";
	}
	body.erase(body.length() - 1);

	this->content = body;
}

void SettingsDialog::showStringSettingDialog(SettingStringItem& item)
{
	auto dialog = std::shared_ptr<InputDialog>(
		new InputDialog(_(item.title, player), _(item.description, player),
			false, _("Save", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[this, &item](DialogResult result)
		{
			if (result.response())
				this->storage->set(item.id, result.inputText());
			this->show();
		});
}

void SettingsDialog::showBooleanSettingDialog(SettingBooleanItem& item)
{
	auto dialog = std::shared_ptr<MessageDialog>(
		new MessageDialog(_(item.title, player), _(item.description, player),
			_(item.trueText, player), _(item.falseText, player)));
	this->dialogManager->showDialog(player, dialog,
		[this, &item](DialogResult result)
		{
			this->storage->set(
				item.id, result.response() == DialogResponse_Left);
			this->show();
		});
}

void SettingsDialog::showIntegerSettingDialog(SettingIntegerItem& item)
{
	auto dialog = std::shared_ptr<InputDialog>(
		new InputDialog(_(item.title, player), _(item.description, player),
			false, _("Save", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[this, &item](DialogResult result)
		{
			if (!result.response() || result.inputText().empty())
			{
				this->show();
				return;
			}

			int value = std::stoi(result.inputText());
			if (value < item.minValue || value > item.maxValue)
			{
				auto playerExt = Player::getPlayerExt(player);
				playerExt->sendErrorMessage(
					__("Value should be between %d and %d"), item.minValue,
					item.maxValue);
				this->showIntegerSettingDialog(item);
				return;
			}
			this->storage->set(item.id, value);
			this->show();
		});
}

void SettingsDialog::showDoubleSettingDialog(SettingDoubleItem& item)
{
	auto dialog = std::shared_ptr<InputDialog>(
		new InputDialog(_(item.title, player), _(item.description, player),
			false, _("Save", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[this, &item](DialogResult result)
		{
			if (!result.response() || result.inputText().empty())
			{
				this->show();
				return;
			}

			double value = std::stod(result.inputText());
			if (value < item.minValue || value > item.maxValue)
			{
				auto playerExt = Player::getPlayerExt(player);
				playerExt->sendErrorMessage(
					__("Value should be between %.1f and %.1f"), item.minValue,
					item.maxValue);
				this->showDoubleSettingDialog(item);
				return;
			}
			this->storage->set(item.id, value);
			this->show();
		});
}

void SettingsDialog::showEnumSettingDialog(SettingEnumItem& item)
{
	std::vector<std::string> choices;

	for (auto& choice : item.choices)
	{
		choices.push_back(
			item.translateChoices ? _(choice.text, player) : choice.text);
	}

	auto dialog
		= std::shared_ptr<ListDialog>(new ListDialog(_(item.title, player),
			choices, _("Save", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[this, &item](DialogResult result)
		{
			if (result.response())
				this->storage->set(item.id, item.choices[result.listItem()].id);
			this->show();
		});
}

SettingsDialog::SettingsDialog(IPlayer& player,
	std::shared_ptr<DialogManager> dialogManager,
	std::shared_ptr<ISettingsStorage> storage, const std::string& title,
	std::vector<SettingItem>& items, const std::string& leftButton,
	const std::string& rightButton, bool assignDefaultValues,
	std::string doneItemText, std::function<void(bool)> onSettingsDone)
	: items(items)
	, dialogManager(dialogManager)
	, player(player)
	, storage(storage)
	, onSettingsDone(onSettingsDone)
{

	auto builder = TabListDialogBuilder()
					   .setTitle(title)
					   .setLeftButton(leftButton)
					   .setRightButton(rightButton);
	std::vector<std::vector<std::string>> rows;
	for (auto& item : items)
	{
		switch (item.type)
		{
		case SettingType::STRING:
		{
			auto stringItem = dynamic_cast<SettingStringItem&>(item);
			rows.push_back({ _(item.title, player), stringItem.defaultValue });
			if (assignDefaultValues)
				this->storage->set(stringItem.id, stringItem.defaultValue);
			break;
		}
		case SettingType::BOOLEAN:
		{
			auto boolItem = dynamic_cast<SettingBooleanItem&>(item);
			rows.push_back({ _(item.title, player),
				boolItem.defaultValue ? _(boolItem.trueText, player)
									  : _(boolItem.falseText, player) });
			if (assignDefaultValues)
				this->storage->set(boolItem.id, boolItem.defaultValue);
			break;
		}
		case SettingType::INTEGER:
		{
			auto intItem = dynamic_cast<SettingIntegerItem&>(item);
			rows.push_back({ _(item.title, player),
				std::to_string(intItem.defaultValue) });
			this->storage->set(intItem.id, intItem.defaultValue);
			break;
		}
		case SettingType::DOUBLE:
		{
			auto doubleItem = dynamic_cast<SettingDoubleItem&>(item);
			rows.push_back({ _(item.title, player),
				std::to_string(doubleItem.defaultValue) });
			if (assignDefaultValues)
				this->storage->set(doubleItem.id, doubleItem.defaultValue);
			break;
		}
		case SettingType::ENUM:
		{
			auto enumItem = dynamic_cast<SettingEnumItem&>(item);
			SettingEnumItem::EnumChoice& value = enumItem.choices[0];
			rows.push_back({ _(item.title, player),
				enumItem.translateChoices ? _(value.text, player)
										  : value.text });
			if (assignDefaultValues)
				this->storage->set(enumItem.id, value.id);
			break;
		}
		}
	}

	if (!doneItemText.empty())
		rows.push_back({ _(doneItemText, player) });
	rows.push_back({ _("Done", player) });

	builder.setItems(rows);
	this->innerDialog = builder.build();
}

void SettingsDialog::show()
{
	dialogManager->showDialog(player, this->innerDialog,
		[this](DialogResult result)
		{
			if (!result.response())
				return;
			auto& item = this->items[result.listItem()];
			switch (item.type)
			{
			case SettingType::STRING:
			{
				this->showStringSettingDialog(
					dynamic_cast<SettingStringItem&>(item));
				break;
			}
			case SettingType::BOOLEAN:
			{
				this->showBooleanSettingDialog(
					dynamic_cast<SettingBooleanItem&>(item));
				break;
			}
			case SettingType::INTEGER:
			{
				this->showIntegerSettingDialog(
					dynamic_cast<SettingIntegerItem&>(item));
				break;
			}
			case SettingType::DOUBLE:
			{
				this->showDoubleSettingDialog(
					dynamic_cast<SettingDoubleItem&>(item));
				break;
			}
			case SettingType::ENUM:
			{
				this->showEnumSettingDialog(
					dynamic_cast<SettingEnumItem&>(item));
				break;
			}
			}
		});
}

} // namespace Core