#include "CommandManager.hpp"
#include "../utils/Strings.hpp"
#include "../player/PlayerExtension.hpp"
#include "CommandInfo.hpp"

#include <exception>
#include <functional>
#include <spdlog/spdlog.h>
#include <player.hpp>
#include <stdexcept>
#include <variant>

namespace Core::Commands
{

CommandManager::CommandManager(IPlayerPool* playerPool)
	: _playerPool(playerPool)
{
	_playerPool->getPlayerTextDispatcher().addEventHandler(this);
}

CommandManager::~CommandManager()
{
	_playerPool->getPlayerTextDispatcher().removeEventHandler(this);
}

bool CommandManager::onPlayerCommandText(
	IPlayer& player, StringView commandText)
{
	auto playerExt = Player::getPlayerExt(player);
	if (!playerExt->isAuthorized())
		return true;

	auto preparedCommandText = commandText.to_string();
	auto cmdParts = Utils::Strings::split(preparedCommandText, ' ');
	assert(!cmdParts.empty());

	// get command name and pop the first element from the vector
	auto commandName = cmdParts.at(0);
	preparedCommandText.erase(0, commandName.size());
	Utils::Strings::trim(preparedCommandText);
	commandName.erase(0, 1); // remove '/' from command name

	if (!this->_commandHandlers.contains(commandName))
	{
		return false;
	}

	try
	{
		this->callCommandHandler(player, commandName, preparedCommandText);
	}
	catch (const std::invalid_argument& e)
	{
		this->sendCommandUsage(player, commandName);
	}
	catch (const std::exception& e)
	{
		spdlog::debug("Failed to invoke command: {}", e.what());
		Player::getPlayerExt(player)->sendErrorMessage(
			__("Failed to invoke command!"));
	}
	return true;
}

void CommandManager::callCommandHandler(IPlayer& player,
	const std::string& cmdName, std::string preparedCommandArgs)
{
	bool notFound = true;
	for (const auto& handler : this->_commandHandlers[cmdName])
	{
		try
		{
			auto result = handler(player, preparedCommandArgs);
			if (!result)
				continue;
		}
		catch (const std::exception& e)
		{
			spdlog::debug(e.what());
			continue;
		}
		notFound = false;
		break;
	}
	if (notFound)
	{
		throw std::invalid_argument("command called with invalid arguments");
	}
}

void CommandManager::sendCommandUsage(IPlayer& player, const std::string& name)
{
	for (auto info : this->_commandInfo[name])
	{
		std::string usageText = "/" + name;
		for (const auto& arg : info->args)
		{
			usageText += fmt::format(" [{}]", _(arg, player));
		}
		player.sendClientMessage(Colour::White(),
			fmt::format(
				"{} {}", _("#GOLD_FUSION#[USAGE]#WHITE#", player), usageText));
	}
}
}