#include "Localization.hpp"
#include "Colors.hpp"
#include "../player/PlayerExtension.hpp"

#include <fmt/printf.h>
#include <spdlog/spdlog.h>
#include <regex>

std::string _(const std::string& message, IPlayer& player)
{
	if (!Localization::gDictionaryManager)
		return message;
	auto ext = queryExtension<Core::Player::OasisPlayerExt>(player);
	if (ext)
	{
		auto data = ext->getPlayerData();
		if (data)
		{
			auto& dict = Localization::getDictionary(data->language, Localization::LANGUAGE_CHARSETS.at(data->language));
			auto translation = dict.translate(message);
			std::regex colorRegex("\\#[^#]+\\#");

			for (std::smatch m; std::regex_search(translation, m, colorRegex);)
			{
				auto matchString = m.str();
				auto colorName = matchString.substr(1, matchString.length() - 2);
				if (!Utils::COLORS.contains(colorName))
				{
					spdlog::error(fmt::sprintf("Color %s doesn't exist in COLORS map. The broken translation is:\n%s", colorName, translation));
					continue;
				}
				auto colorExpansion = Utils::COLORS.at(colorName);
				translation.replace(m.position(), m.length(), fmt::sprintf("{%s}", colorExpansion));
			}
			return translation;
		}
	}
	return message;
}