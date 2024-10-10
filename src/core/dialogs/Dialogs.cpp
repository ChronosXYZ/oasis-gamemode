#include "Dialogs.hpp"

#include "../utils/Strings.hpp"
#include "../utils/Localization.hpp"

#include "DialogResult.hpp"
#include "IDialog.hpp"
#include "player.hpp"
#include <Server/Components/Dialogs/dialogs.hpp>
#include <algorithm>
#include <bits/ranges_util.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <ranges>
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

void SettingsDialog::showStringSettingDialog(
	std::shared_ptr<SettingStringItem> item)
{
	auto dialog = std::shared_ptr<InputDialog>(
		new InputDialog(_(item->title, player), _(item->description, player),
			false, _("Save", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[this, item](DialogResult result)
		{
			if (result.response())
			{
				this->values[item->id] = result.inputText();
				if (this->onConfigurationChangedCallback)
					this->onConfigurationChangedCallback(
						{ item->id, result.inputText() });
			}
			this->show();
		});
}

void SettingsDialog::showBooleanSettingDialog(
	std::shared_ptr<SettingBooleanItem> item)
{
	auto dialog = std::shared_ptr<MessageDialog>(
		new MessageDialog(_(item->title, player), _(item->description, player),
			_(item->trueText, player), _(item->falseText, player)));
	this->dialogManager->showDialog(player, dialog,
		[this, item](DialogResult result)
		{
			auto value = result.response() == DialogResponse_Left;
			this->values[item->id] = value;
			if (this->onConfigurationChangedCallback)
				this->onConfigurationChangedCallback({ item->id, value });
			this->show();
		});
}

void SettingsDialog::showIntegerSettingDialog(
	std::shared_ptr<SettingIntegerItem> item)
{
	auto dialog = std::shared_ptr<InputDialog>(
		new InputDialog(_(item->title, player), _(item->description, player),
			false, _("Save", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[this, item](DialogResult result)
		{
			if (!result.response() || result.inputText().empty())
			{
				this->show();
				return;
			}

			int value = std::stoi(result.inputText());
			if (value < item->minValue || value > item->maxValue)
			{
				auto playerExt = Player::getPlayerExt(player);
				playerExt->sendErrorMessage(
					__("Value should be between %d and %d"), item->minValue,
					item->maxValue);
				this->showIntegerSettingDialog(item);
				return;
			}
			this->values[item->id] = value;
			if (this->onConfigurationChangedCallback)
				this->onConfigurationChangedCallback({ item->id, value });
			this->show();
		});
}

void SettingsDialog::showDoubleSettingDialog(
	std::shared_ptr<SettingDoubleItem> item)
{
	auto dialog = std::shared_ptr<InputDialog>(
		new InputDialog(_(item->title, player), _(item->description, player),
			false, _("Save", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[this, item](DialogResult result)
		{
			if (!result.response() || result.inputText().empty())
			{
				this->show();
				return;
			}

			double value = std::stod(result.inputText());
			if (value < item->minValue || value > item->maxValue)
			{
				auto playerExt = Player::getPlayerExt(player);
				playerExt->sendErrorMessage(
					__("Value should be between %.1f and %.1f"), item->minValue,
					item->maxValue);
				this->showDoubleSettingDialog(item);
				return;
			}
			this->values[item->id] = value;
			if (this->onConfigurationChangedCallback)
				this->onConfigurationChangedCallback({ item->id, value });
			this->show();
		});
}

void SettingsDialog::showEnumSettingDialog(
	std::shared_ptr<SettingEnumItem> item)
{
	std::vector<std::string> choices;

	for (auto& choice : item->choices)
	{
		choices.push_back(
			item->translateChoices ? _(choice.text, player) : choice.text);
	}

	auto dialog
		= std::shared_ptr<ListDialog>(new ListDialog(_(item->title, player),
			choices, _("Save", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[this, item](DialogResult result)
		{
			if (result.response())
			{
				auto value = item->choices[result.listItem()].id;
				this->values[item->id] = value;
				if (this->onConfigurationChangedCallback)
					this->onConfigurationChangedCallback({ item->id, value });
			}
			this->show();
		});
}

SettingsDialog::SettingsDialog(IPlayer& player,
	std::shared_ptr<DialogManager> dialogManager,
	std::unordered_map<std::string, SettingValue> values,
	const std::string& title, std::vector<std::shared_ptr<SettingItem>> items,
	const std::string& leftButton, const std::string& rightButton,
	std::function<void(SettingsMap)> onConfigurationDone,
	std::function<void(std::pair<std::string, SettingValue>)>
		onConfigurationChanged)
	: items(items)
	, dialogManager(dialogManager)
	, player(player)
	, values(values)
	, onConfigurationDoneCallback(onConfigurationDone)
	, onConfigurationChangedCallback(onConfigurationChanged)
{

	this->innerDialogBuilder = TabListDialogBuilder()
								   .setTitle(title)
								   .setLeftButton(_(leftButton, player))
								   .setRightButton(_(rightButton, player));
}

void SettingsDialog::show()
{
	auto settingsDialog = this->shared_from_this();
	this->dialogManager->addSettingsDialog(
		this->player.getID(), settingsDialog);

	std::vector<std::vector<std::string>> rows;
	for (auto item : items)
	{
		switch (item->type)
		{
		case SettingType::STRING:
		{
			rows.push_back({ _(item->title, player),
				std::get<std::string>(this->values.at(item->id)) });
			break;
		}
		case SettingType::BOOLEAN:
		{
			auto boolItem = std::dynamic_pointer_cast<SettingBooleanItem>(item);
			rows.push_back({ _(item->title, player),
				std::get<bool>(this->values.at(boolItem->id))
					? _(boolItem->trueText, player)
					: _(boolItem->falseText, player) });
			break;
		}
		case SettingType::INTEGER:
		{
			auto intItem = std::dynamic_pointer_cast<SettingIntegerItem>(item);
			rows.push_back({ _(item->title, player),
				std::to_string(std::get<int>(this->values.at(item->id))) });
			break;
		}
		case SettingType::DOUBLE:
		{
			auto doubleItem
				= std::dynamic_pointer_cast<SettingDoubleItem>(item);
			rows.push_back({ _(item->title, player),
				std::to_string(
					std::get<double>(this->values.at(doubleItem->id))) });
			break;
		}
		case SettingType::ENUM:
		{
			auto enumItem = std::dynamic_pointer_cast<SettingEnumItem>(item);
			auto it = std::find_if(enumItem->choices.begin(),
				enumItem->choices.end(),
				[this, item](const auto& choice)
				{
					return choice.id
						== std::get<std::string>(this->values.at(item->id));
				});

			rows.push_back({ _(item->title, player),
				enumItem->translateChoices ? _(it->text, player) : it->text });
			break;
		}
		case SettingType::NONE:
		{
			rows.push_back(
				{ _(item->title, player), _(item->description, player) });
			break;
		}
		}
	}

	this->innerDialogBuilder.setItems(rows);
	dialogManager->showDialog(player, this->innerDialogBuilder.build(),
		[this](DialogResult result)
		{
			if (!result.response())
			{
				auto it = std::find_if(this->items.begin(), this->items.end(),
					[](const auto item)
					{
						return item->type == SettingType::NONE;
					});
				if (it == this->items.end())
					if (this->onConfigurationDoneCallback)
						this->onConfigurationDoneCallback(this->values);
				this->dialogManager->removeSettingsDialog(this->player.getID());
				return;
			}
			auto item = this->items[result.listItem()];
			switch (item->type)
			{
			case SettingType::STRING:
			{
				this->showStringSettingDialog(
					std::dynamic_pointer_cast<SettingStringItem>(item));
				break;
			}
			case SettingType::BOOLEAN:
			{
				this->showBooleanSettingDialog(
					std::dynamic_pointer_cast<SettingBooleanItem>(item));
				break;
			}
			case SettingType::INTEGER:
			{
				this->showIntegerSettingDialog(
					std::dynamic_pointer_cast<SettingIntegerItem>(item));
				break;
			}
			case SettingType::DOUBLE:
			{
				this->showDoubleSettingDialog(
					std::dynamic_pointer_cast<SettingDoubleItem>(item));
				break;
			}
			case SettingType::ENUM:
			{
				this->showEnumSettingDialog(
					std::dynamic_pointer_cast<SettingEnumItem>(item));
				break;
			}
			case SettingType::NONE:
			{
				if (this->onConfigurationDoneCallback)
					this->onConfigurationDoneCallback(this->values);
				this->dialogManager->removeSettingsDialog(this->player.getID());
				break;
			}
			}
		});
}

} // namespace Core