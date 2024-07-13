#pragma once

#include <Server/Components/Dialogs/dialogs.hpp>

namespace Core
{
struct IDialog
{
	IDialog(DialogStyle style)
		: style(style)
	{
	}
	virtual ~IDialog() { }

	DialogStyle style;
	std::string title;
	std::string content;
	std::string button1;
	std::string button2;

protected:
	typedef IDialog super;
};
}