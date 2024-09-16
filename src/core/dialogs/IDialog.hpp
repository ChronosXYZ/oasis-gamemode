#pragma once

#include <Server/Components/Dialogs/dialogs.hpp>
#include <string>

namespace Core
{
struct IDialog
{
	IDialog(DialogStyle style, const std::string& title,
		const std::string& content, const std::string& leftButton,
		const std::string& rightButton)
		: style(style)
		, title(title)
		, content(content)
		, leftButton(leftButton)
		, rightButton(rightButton)
	{
	}
	virtual ~IDialog() { }

	DialogStyle style;
	std::string title;
	std::string content;
	std::string leftButton;
	std::string rightButton;

protected:
	typedef IDialog super;
};
}