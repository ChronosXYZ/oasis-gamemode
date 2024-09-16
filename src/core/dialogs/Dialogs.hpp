#pragma once

#include "IDialog.hpp"
#include "../settings/SettingsStorage.hpp"

#include "player.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Core
{
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
	ENUM
};

struct SettingItem
{
	std::string title;
	std::string description;
	std::string id;
	SettingType type;
	virtual ~SettingItem() = default;
};

struct SettingStringItem : public SettingItem
{
	std::string defaultValue;
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
};

struct SettingBooleanItem : public SettingItem
{
	bool defaultValue;
	std::string trueText;
	std::string falseText;
};

struct SettingIntegerItem : public SettingItem
{
	int defaultValue;
	int minValue;
	int maxValue;
};

struct SettingDoubleItem : public SettingItem
{
	double defaultValue;
	double minValue;
	double maxValue;
};

class SettingsDialog
{
	void showStringSettingDialog(SettingStringItem& item);
	void showBooleanSettingDialog(SettingBooleanItem& item);
	void showIntegerSettingDialog(SettingIntegerItem& item);
	void showDoubleSettingDialog(SettingDoubleItem& item);
	void showEnumSettingDialog(SettingEnumItem& item);

	std::vector<SettingItem> items;
	std::shared_ptr<TabListDialog> innerDialog;
	std::shared_ptr<ISettingsStorage> storage;
	std::shared_ptr<DialogManager> dialogManager;
	IPlayer& player;

	std::function<void(bool)> onSettingsDone;

public:
	SettingsDialog(IPlayer& player,
		std::shared_ptr<DialogManager> dialogManager,
		std::shared_ptr<ISettingsStorage> storage, const std::string& title,
		std::vector<SettingItem>& items, const std::string& leftButton,
		const std::string& rightButton, bool assignDefaultValues = true,
		std::string doneItemText = "",
		std::function<void(bool)> onSettingsDone = nullptr);
	void show();
};

class SettingsDialogBuilder
{
private:
	IPlayer& player;
	std::shared_ptr<DialogManager> dialogManager;
	std::shared_ptr<ISettingsStorage> storage;
	std::string title;
	std::vector<SettingItem> items;
	std::string leftButton;
	std::string rightButton;
	bool assignDefaultValues = true;
	std::string doneItemText;
	std::function<void(bool)> onSettingsDone;

public:
	SettingsDialogBuilder(IPlayer& player,
		std::shared_ptr<DialogManager> dialogManager,
		std::shared_ptr<ISettingsStorage> storage)
		: player(player)
		, dialogManager(dialogManager)
		, storage(storage)
	{
	}

	SettingsDialogBuilder& setTitle(const std::string& title)
	{
		this->title = title;
		return *this;
	}

	SettingsDialogBuilder& setItems(const std::vector<SettingItem>& items)
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

	SettingsDialogBuilder& setAssignDefaultValues(bool assignDefaultValues)
	{
		this->assignDefaultValues = assignDefaultValues;
		return *this;
	}

	SettingsDialogBuilder& setDoneItemText(const std::string& doneItemText)
	{
		this->doneItemText = doneItemText;
		return *this;
	}

	SettingsDialogBuilder& setOnSettingsDone(
		std::function<void(bool)> onSettingsDone)
	{
		this->onSettingsDone = onSettingsDone;
		return *this;
	}

	std::shared_ptr<SettingsDialog> build()
	{
		return std::make_shared<SettingsDialog>(player, dialogManager, storage,
			title, items, leftButton, rightButton, assignDefaultValues,
			doneItemText, onSettingsDone);
	}
};

}