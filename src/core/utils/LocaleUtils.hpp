#pragma once

#include <tinygettext/dictionary_manager.hpp>
#include <sdk.hpp>
#include <string>
#include "../player/PlayerExtension.hpp"
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
			return dict.translate(message);
		}
	}
	return message;
}