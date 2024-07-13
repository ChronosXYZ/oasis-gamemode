#pragma once

#include <Server/Components/Dialogs/dialogs.hpp>

namespace Core
{
struct DialogResult
{
	DialogResult(DialogResponse response, int listItem, std::string inputText)
		: mResponse(response)
		, mListItem(listItem)
		, mInputText(inputText)
	{
	}

	const DialogResponse& response() { return mResponse; };
	const int& listItem() { return mListItem; };
	const std::string& inputText() { return mInputText; };

private:
	DialogResponse mResponse;
	int mListItem;
	std::string mInputText;
};
}