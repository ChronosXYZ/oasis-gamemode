#include "Dialogs.hpp"

#include "../utils/Strings.hpp"
#include "../utils/Localization.hpp"

#include "DialogResult.hpp"
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

void SettingsDialog::showStringSettingDialog(SettingStringItem& item)
{
	auto dialog = std::shared_ptr<InputDialog>(
		new InputDialog(_(item.title, player), _(item.description, player),
			false, _("Save", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[this, &item](DialogResult result)
		{
			if (result.response())
				this->values.emplace(item.id, result.inputText());
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
			this->values.emplace(
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
			this->values.emplace(item.id, value);
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
			this->values.emplace(item.id, value);
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
				this->values.emplace(
					item.id, item.choices[result.listItem()].id);
			this->show();
		});
}

SettingsDialog::SettingsDialog(IPlayer& player,
	std::shared_ptr<DialogManager> dialogManager,
	std::unordered_map<std::string, SettingValue> values,
	const std::string& title, std::vector<SettingItem>& items,
	const std::string& leftButton, const std::string& rightButton,
	std::function<void(SettingsMap)> onSettingsDone)
	: items(items)
	, dialogManager(dialogManager)
	, player(player)
	, values(values)
	, onSettingsDone(onSettingsDone)
{

	auto builder = TabListDialogBuilder()
					   .setTitle(title)
					   .setLeftButton(_(leftButton, player))
					   .setRightButton(_(rightButton, player));
	std::vector<std::vector<std::string>> rows;
	for (auto& item : items)
	{
		switch (item.type)
		{
		case SettingType::STRING:
		{
			rows.push_back({ _(item.title, player),
				std::get<std::string>(this->values.at(item.id)) });
			break;
		}
		case SettingType::BOOLEAN:
		{
			auto boolItem = dynamic_cast<SettingBooleanItem&>(item);
			rows.push_back({ _(item.title, player),
				std::get<bool>(this->values.at(boolItem.id))
					? _(boolItem.trueText, player)
					: _(boolItem.falseText, player) });
			break;
		}
		case SettingType::INTEGER:
		{
			auto intItem = dynamic_cast<SettingIntegerItem&>(item);
			rows.push_back({ _(item.title, player),
				std::to_string(std::get<int>(this->values.at(item.id))) });
			break;
		}
		case SettingType::DOUBLE:
		{
			auto doubleItem = dynamic_cast<SettingDoubleItem&>(item);
			rows.push_back({ _(item.title, player),
				std::to_string(
					std::get<double>(this->values.at(doubleItem.id))) });
			break;
		}
		case SettingType::ENUM:
		{
			auto enumItem = dynamic_cast<SettingEnumItem&>(item);
			SettingEnumItem::EnumChoice& value = enumItem.choices[0];
			rows.push_back({ _(item.title, player),
				enumItem.translateChoices ? _(value.text, player)
										  : value.text });
			break;
		}
		case SettingType::NONE:
		{
			rows.push_back(
				{ _(item.title, player), _(item.description, player) });
			break;
		}
		}
	}

	builder.setItems(rows);
	this->innerDialog = builder.build();
}

void SettingsDialog::show()
{
	dialogManager->showDialog(player, this->innerDialog,
		[this](DialogResult result)
		{
			if (!result.response())
			{
				auto it = std::find_if(this->items.begin(), this->items.end(),
					[](const auto& item)
					{
						return item.type == SettingType::NONE;
					});
				if (it == this->items.end())
					if (this->onSettingsDone)
						this->onSettingsDone(this->values);
				return;
			}
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
			case SettingType::NONE:
			{
				if (this->onSettingsDone)
					this->onSettingsDone(this->values);
				break;
			}
			}
		});
}

} // namespace Core