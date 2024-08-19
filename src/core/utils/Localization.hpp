#pragma once

#include <tinygettext/dictionary_manager.hpp>
#include <player.hpp>

#include <string>
#include <unordered_map>
#include <memory>
#include <utility>

namespace Localization
{
inline const std::unordered_map<std::string, std::string> LANGUAGE_CHARSETS
	= { { "en", "ASCII" }, { "pt", "ASCII" }, { "ru", "CP1251" } };

inline const std::unordered_map<unsigned int, std::string> LANGUAGE_CODE_NAMES
	= { { 0, "en" }, { 1, "pt" }, { 2, "ru" } };

inline const auto LANGUAGES
	= std::to_array<std::string>({ "English", "Portuguese", "Russian" });

inline std::unique_ptr<tinygettext::DictionaryManager> gDictionaryManager
	= nullptr;

static inline tinygettext::Dictionary& getDictionary(
	const std::string& lang, const std::string& charset)
{
	return Localization::gDictionaryManager->get_dictionary(
		tinygettext::Language::from_name(lang), charset);
}
}

std::string _(const std::string& message, IPlayer& player);

/**
	Mark message for localization, but don't translate it right away.
*/
inline const std::string&& __(std::string&& message)
{
	return std::forward<std::string>(message);
}
