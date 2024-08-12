#pragma once

#include "../modes/Modes.hpp"
#include "../modes/duel/DuelOffer.hpp"

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace Core
{
struct PlayerTempData
{
	bool isLoggedIn = false;
	Modes::Mode currentMode = Modes::Mode::None;
	Modes::Mode lastMode = Modes::Mode::None;
	bool skinSelectionMode = false;
	bool isDying = false;

	unsigned int subsequentKills = 0;

	std::optional<std::shared_ptr<Modes::Duel::DuelOffer>> duelOfferSent;
	std::unordered_map<int, std::shared_ptr<Modes::Duel::DuelOffer>>
		duelOffersReceived;

	std::optional<std::shared_ptr<Modes::Duel::DuelOffer>>
		temporaryDuelSettings;
};
}