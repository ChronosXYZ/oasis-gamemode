#pragma once

#include "IDialog.hpp"

#include "player.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Core
{
typedef std::variant<std::string, bool, int, double> SettingValue;
typedef std::unordered_map<std::string, SettingValue> SettingsMap;

struct DialogManager;

class InputDialog : public IDialog
{
public:
	InputDialog(const std::string& title, const std::string& content,
		bool isPassword, const std::string& leftButton,
		const std::string& rightButton);
	bool isPassword;
};

class InputDialogBuilder
{
private:
	std::string title;
	std::string content;
	bool isPassword;
	std::string button1;
	std::string button2;

public:
	InputDialogBuilder() = default;

	InputDialogBuilder& setTitle(const std::string& title)
	{
		this->title = title;
		return *this;
	}

	InputDialogBuilder& setContent(const std::string& content)
	{
		this->content = content;
		return *this;
	}

	InputDialogBuilder& setIsPassword(bool isPassword)
	{
		this->isPassword = isPassword;
		return *this;
	}

	InputDialogBuilder& setLeftButton(const std::string& button1)
	{
		this->button1 = button1;
		return *this;
	}

	InputDialogBuilder& setRightButton(const std::string& button2)
	{
		this->button2 = button2;
		return *this;
	}

	std::shared_ptr<InputDialog> build()
	{
		return std::shared_ptr<InputDialog>(
			new InputDialog(title, content, isPassword, button1, button2));
	}
};

class ListDialog : public IDialog
{
public:
	ListDialog(const std::string& title, const std::vector<std::string> items,
		const std::string& leftButton, const std::string& rightButton);
	std::vector<std::string> items;
};

class ListDialogBuilder
{
private:
	std::string title;
	std::vector<std::string> items;
	std::string leftButton;
	std::string rightButton;

public:
	ListDialogBuilder() = default;

	ListDialogBuilder& setTitle(const std::string& title)
	{
		this->title = title;
		return *this;
	}

	ListDialogBuilder& setItems(const std::vector<std::string>& items)
	{
		this->items = items;
		return *this;
	}

	ListDialogBuilder& setLeftButton(const std::string& leftButton)
	{
		this->leftButton = leftButton;
		return *this;
	}

	ListDialogBuilder& setRightButton(const std::string& rightButton)
	{
		this->rightButton = rightButton;
		return *this;
	}

	std::shared_ptr<ListDialog> build()
	{
		return std::shared_ptr<ListDialog>(
			new ListDialog(title, items, leftButton, rightButton));
	}
};

class TabListHeadersDialog : public IDialog
{
public:
	TabListHeadersDialog(const std::string& title,
		const std::vector<std::string> columns,
		std::vector<std::vector<std::string>> items,
		const std::string& leftButton, const std::string& rightButton);
	std::vector<std::vector<std::string>> items;
};

class TabListHeadersDialogBuilder
{
private:
	std::string title;
	std::vector<std::string> columns;
	std::vector<std::vector<std::string>> items;
	std::string leftButton;
	std::string rightButton;

public:
	TabListHeadersDialogBuilder() = default;

	TabListHeadersDialogBuilder& setTitle(const std::string& title)
	{
		this->title = title;
		return *this;
	}

	TabListHeadersDialogBuilder& setColumns(
		const std::vector<std::string>& columns)
	{
		this->columns = columns;
		return *this;
	}

	TabListHeadersDialogBuilder& setItems(
		const std::vector<std::vector<std::string>>& items)
	{
		this->items = items;
		return *this;
	}

	TabListHeadersDialogBuilder& setLeftButton(const std::string& leftButton)
	{
		this->leftButton = leftButton;
		return *this;
	}

	TabListHeadersDialogBuilder& setRightButton(const std::string& rightButton)
	{
		this->rightButton = rightButton;
		return *this;
	}

	std::shared_ptr<TabListHeadersDialog> build()
	{
		return std::shared_ptr<TabListHeadersDialog>(new TabListHeadersDialog(
			title, columns, items, leftButton, rightButton));
	}
};

class TabListDialog : public IDialog
{
public:
	TabListDialog(const std::string& title,
		std::vector<std::vector<std::string>> items,
		const std::string& leftButton, const std::string& rightButton);
	std::vector<std::vector<std::string>> items;
};

class TabListDialogBuilder
{
private:
	std::string title;
	std::vector<std::vector<std::string>> items;
	std::string leftButton;
	std::string rightButton;

public:
	TabListDialogBuilder() = default;

	TabListDialogBuilder& setTitle(const std::string& title)
	{
		this->title = title;
		return *this;
	}

	TabListDialogBuilder& setItems(
		const std::vector<std::vector<std::string>>& items)
	{
		this->items = items;
		return *this;
	}

	TabListDialogBuilder& setLeftButton(const std::string& leftButton)
	{
		this->leftButton = leftButton;
		return *this;
	}

	TabListDialogBuilder& setRightButton(const std::string& rightButton)
	{
		this->rightButton = rightButton;
		return *this;
	}

	std::shared_ptr<TabListDialog> build()
	{
		return std::shared_ptr<TabListDialog>(
			new TabListDialog(title, items, leftButton, rightButton));
	}
};

class MessageDialog : public IDialog
{
public:
	MessageDialog(const std::string& title, const std::string& content,
		const std::string& button1, const std::string& button2);
};

class MessageDialogBuilder
{
private:
	std::string title;
	std::string content;
	std::string button1;
	std::string button2;

public:
	MessageDialogBuilder() = default;

	MessageDialogBuilder& setTitle(const std::string& title)
	{
		this->title = title;
		return *this;
	}

	MessageDialogBuilder& setContent(const std::string& content)
	{
		this->content = content;
		return *this;
	}

	MessageDialogBuilder& setLeftButton(const std::string& button1)
	{
		this->button1 = button1;
		return *this;
	}

	MessageDialogBuilder& setRightButton(const std::string& button2)
	{
		this->button2 = button2;
		return *this;
	}

	std::shared_ptr<MessageDialog> build()
	{
		return std::shared_ptr<MessageDialog>(
			new MessageDialog(title, content, button1, button2));
	}
};

enum class SettingType
{
	STRING,
	BOOLEAN,
	INTEGER,
	DOUBLE,
	ENUM,
	NONE
};

struct SettingItem
{
	std::string title;
	std::string description;
	std::string id;
	SettingType type;
	virtual ~SettingItem() {};
};

struct SettingStringItem : public SettingItem
{
	SettingStringItem(const std::string& id, const std::string& title,
		const std::string& description)
	{
		this->title = title;
		this->description = description;
		this->id = id;
		this->type = SettingType::STRING;
	}
};

struct SettingEnumItem : public SettingItem
{
	struct EnumChoice
	{
		std::string id;
		std::string text;
	};
	std::vector<EnumChoice> choices;
	bool translateChoices;

	SettingEnumItem(const std::string& id, const std::string& title,
		const std::vector<EnumChoice>& choices, bool translateChoices)
	{
		this->title = title;
		this->id = id;
		this->type = SettingType::ENUM;
		this->choices = choices;
		this->translateChoices = translateChoices;
	}
};

struct SettingBooleanItem : public SettingItem
{
	std::string trueText;
	std::string falseText;

	SettingBooleanItem(const std::string& id, const std::string& title,
		const std::string& description, const std::string& trueText,
		const std::string& falseText)
	{
		this->title = title;
		this->description = description;
		this->id = id;
		this->type = SettingType::BOOLEAN;
		this->trueText = trueText;
		this->falseText = falseText;
	}
};

struct SettingIntegerItem : public SettingItem
{
	int minValue;
	int maxValue;

	SettingIntegerItem(const std::string& id, const std::string& title,
		const std::string& description, int minValue, int maxValue)
	{
		this->title = title;
		this->description = description;
		this->id = id;
		this->type = SettingType::INTEGER;
		this->minValue = minValue;
		this->maxValue = maxValue;
	}
};

struct SettingDoubleItem : public SettingItem
{
	double minValue;
	double maxValue;

	SettingDoubleItem(const std::string& id, const std::string& title,
		const std::string& description, double minValue, double maxValue)
	{
		this->title = title;
		this->description = description;
		this->id = id;
		this->type = SettingType::DOUBLE;
		this->minValue = minValue;
		this->maxValue = maxValue;
	}
};

class SettingsDialog : public std::enable_shared_from_this<SettingsDialog>
{
	void showStringSettingDialog(std::shared_ptr<SettingStringItem> item);
	void showBooleanSettingDialog(std::shared_ptr<SettingBooleanItem> item);
	void showIntegerSettingDialog(std::shared_ptr<SettingIntegerItem> item);
	void showDoubleSettingDialog(std::shared_ptr<SettingDoubleItem> item);
	void showEnumSettingDialog(std::shared_ptr<SettingEnumItem> item);

	std::vector<std::shared_ptr<SettingItem>> items;
	std::unordered_map<std::string, SettingValue> values;
	TabListDialogBuilder innerDialogBuilder;
	std::shared_ptr<DialogManager> dialogManager;
	IPlayer& player;

	std::function<void(SettingsMap)> onSettingsDone;

public:
	SettingsDialog(IPlayer& player,
		std::shared_ptr<DialogManager> dialogManager,
		std::unordered_map<std::string, SettingValue> values,
		const std::string& title,
		std::vector<std::shared_ptr<SettingItem>> items,
		const std::string& leftButton, const std::string& rightButton,
		std::function<void(SettingsMap)> onSettingsDone = nullptr);
	void show();
};

class SettingsDialogBuilder
{
private:
	IPlayer& player;
	std::shared_ptr<DialogManager> dialogManager;
	std::string title;
	std::vector<std::shared_ptr<SettingItem>> items;
	std::unordered_map<std::string, SettingValue> values;
	std::string leftButton;
	std::string rightButton;
	std::function<void(SettingsMap)> onSettingsDone;

public:
	SettingsDialogBuilder(
		IPlayer& player, std::shared_ptr<DialogManager> dialogManager)
		: player(player)
		, dialogManager(dialogManager)
	{
	}

	SettingsDialogBuilder& setTitle(const std::string& title)
	{
		this->title = title;
		return *this;
	}

	SettingsDialogBuilder& setItems(
		const std::vector<std::shared_ptr<SettingItem>> items)
	{
		this->items = items;
		return *this;
	}

	SettingsDialogBuilder& setLeftButton(const std::string& leftButton)
	{
		this->leftButton = leftButton;
		return *this;
	}

	SettingsDialogBuilder& setRightButton(const std::string& rightButton)
	{
		this->rightButton = rightButton;
		return *this;
	}

	SettingsDialogBuilder& setInitialValues(
		std::unordered_map<std::string, SettingValue> initialValues)
	{
		this->values = initialValues;
		return *this;
	}

	SettingsDialogBuilder& setOnSettingsDone(
		std::function<void(SettingsMap)> onSettingsDone)
	{
		this->onSettingsDone = onSettingsDone;
		return *this;
	}

	std::shared_ptr<SettingsDialog> build()
	{
		if (this->values.empty())
		{
			for (auto& item : items)
			{
				switch (item->type)
				{
				case SettingType::BOOLEAN:
				{
					values[item->id] = false;
					break;
				}
				case SettingType::INTEGER:
				{
					auto intItem
						= std::dynamic_pointer_cast<SettingIntegerItem>(item);
					values[item->id] = intItem->minValue;
					break;
				}
				case SettingType::DOUBLE:
				{
					auto doubleItem
						= std::dynamic_pointer_cast<SettingDoubleItem>(item);
					values[item->id] = doubleItem->minValue;
					break;
				}
				case SettingType::ENUM:
				{
					auto enumItem
						= std::dynamic_pointer_cast<SettingEnumItem>(item);
					values[item->id] = enumItem->choices[0].id;
					break;
				}
				case SettingType::STRING:
				{
					values[item->id] = "";
					break;
				}
				default:
					break;
				}
			}
		}

		return std::shared_ptr<SettingsDialog>(
			new SettingsDialog(player, dialogManager, values, title, items,
				leftButton, rightButton, onSettingsDone));
	}
};

}