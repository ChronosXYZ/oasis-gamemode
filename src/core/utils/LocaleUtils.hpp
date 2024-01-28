#pragma once

#include <fmt/core.h>
#include <fmt/printf.h>
#include <spdlog/spdlog.h>
#include <tinygettext/dictionary_manager.hpp>
#include <sdk.hpp>
#include <string>
#include <regex>
#include "../player/PlayerExtension.hpp"
#include "Colors.hpp"
#include "Strings.hpp"
#include "tinygettext/dictionary.hpp"
#include "tinygettext/language.hpp"
#include "../../constants.hpp"

namespace Locale
{
inline std::unique_ptr<tinygettext::DictionaryManager> gDictionaryManager = nullptr;

static inline tinygettext::Dictionary& getDictionary(const std::string& lang, const std::string& charset)
{
	return Locale::gDictionaryManager->get_dictionary(tinygettext::Language::from_name(lang), charset);
}
}

static inline std::string _(const std::string& message, IPlayer& player)
{
	if (!Locale::gDictionaryManager)
		return message;
	auto ext = queryExtension<Core::OasisPlayerExt>(player);
	if (ext)
	{
		auto data = ext->getPlayerData();
		if (data)
		{
			auto& dict = Locale::getDictionary(data->language, consts::LANGUAGE_CHARSETS.at(data->language));
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