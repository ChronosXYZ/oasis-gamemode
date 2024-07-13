#pragma once

#include "IDialog.hpp"
#include <vector>

namespace Core
{
class InputDialog : public IDialog
{
public:
	InputDialog(const std::string& title, const std::string& content,
		bool isPassword, const std::string& button1,
		const std::string& button2);
	bool isPassword;
};

class ListDialog : public IDialog
{
public:
	ListDialog(const std::string& title, const std::vector<std::string> items,
		const std::string& button1, const std::string& button2);
	std::vector<std::string> items;
};

class TabListHeadersDialog : public IDialog
{
public:
	TabListHeadersDialog(const std::string& title,
		const std::vector<std::string> columns,
		std::vector<std::vector<std::string>> items, const std::string& button1,
		const std::string& button2);
	std::vector<std::vector<std::string>> items;
};

class TabListDialog : public IDialog
{
public:
	TabListDialog(const std::string& title,
		std::vector<std::vector<std::string>> items, const std::string& button1,
		const std::string& button2);
	std::vector<std::vector<std::string>> items;
};

class MessageDialog : public IDialog
{
public:
	MessageDialog(const std::string& title, const std::string& content,
		const std::string& button1, const std::string& button2);
};
}