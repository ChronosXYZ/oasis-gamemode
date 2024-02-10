#pragma once

#include "../textdraws/ITextDrawWrapper.hpp"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace Core::Player
{
class TextDrawManager
{
	std::unordered_map<std::string, std::shared_ptr<TextDraws::ITextDrawWrapper>> _textdrawWrappers;

public:
	void add(const std::string& name, std::shared_ptr<TextDraws::ITextDrawWrapper> wrapper);
	std::optional<std::shared_ptr<TextDraws::ITextDrawWrapper>> get(const std::string& name);
	void destroy(const std::string& name);
};
}