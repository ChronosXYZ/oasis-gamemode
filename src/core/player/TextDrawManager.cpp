#include "TextDrawManager.hpp"

#include <memory>

namespace Core::Player
{
void TextDrawManager::add(const std::string& name, std::shared_ptr<TextDraws::ITextDrawWrapper> wrapper)
{
	this->_textdrawWrappers[name] = wrapper;
}

std::optional<std::shared_ptr<TextDraws::ITextDrawWrapper>> TextDrawManager::get(const std::string& name)
{
	if (_textdrawWrappers.contains(name))
	{
		return _textdrawWrappers[name];
	}
	return {};
}

void TextDrawManager::destroy(const std::string& name)
{
	if (_textdrawWrappers.contains(name))
	{
		_textdrawWrappers[name]->destroy();
		this->_textdrawWrappers.erase(name);
	}
}
}