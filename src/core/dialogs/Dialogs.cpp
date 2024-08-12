#include "Dialogs.hpp"

#include "../utils/Strings.hpp"

#include <Server/Components/Dialogs/dialogs.hpp>

namespace Core
{
InputDialog::InputDialog(const std::string& title, const std::string& content,
	bool isPassword, const std::string& button1, const std::string& button2)
	: super(DialogStyle_INPUT)
{
	this->title = title;
	this->content = content;
	this->isPassword = isPassword;
	if (isPassword)
		this->style = DialogStyle_PASSWORD;
	this->button1 = button1;
	this->button2 = button2;
}

ListDialog::ListDialog(const std::string& title,
	const std::vector<std::string> items, const std::string& button1,
	const std::string& button2)
	: super(DialogStyle_LIST)
	, items(items)
{
	std::string body;
	for (const auto& item : items)
	{
		body += Utils::Strings::trim_copy(item) + "\n";
	}
	this->title = title;
	this->content = body;
	this->button1 = button1;
	this->button2 = button2;
}

MessageDialog::MessageDialog(const std::string& title,
	const std::string& content, const std::string& button1,
	const std::string& button2)
	: super(DialogStyle_MSGBOX)
{
	this->title = title;
	this->content = content;
	this->button1 = button1;
	this->button2 = button2;
}

TabListHeadersDialog::TabListHeadersDialog(const std::string& title,
	const std::vector<std::string> columns,
	std::vector<std::vector<std::string>> items, const std::string& button1,
	const std::string& button2)
	: super(DialogStyle_TABLIST_HEADERS)
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

	this->title = title;
	this->content = body;
	this->button1 = button1;
	this->button2 = button2;
}

TabListDialog::TabListDialog(const std::string& title,
	std::vector<std::vector<std::string>> items, const std::string& button1,
	const std::string& button2)
	: super(DialogStyle_TABLIST)
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

	this->title = title;
	this->content = body;
	this->button1 = button1;
	this->button2 = button2;
}
}
