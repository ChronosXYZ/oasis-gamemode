#include "CommandManager.hpp"
#include "../utils/Strings.hpp"
#include "../player/PlayerExtension.hpp"
#include "CommandInfo.hpp"

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

	auto cmdText = commandText.to_string();
	auto cmdParts = Utils::Strings::split(cmdText, ' ');
	assert(!cmdParts.empty());

	// get command name and pop the first element from the vector
	auto commandName = cmdParts.at(0);
	cmdParts.erase(cmdParts.begin());
	commandName.erase(0, 1); // remove '/' from command name

	if (!this->_commandHandlers.contains(commandName))
	{
		return false;
	}
	CommandCallbackValues args;
	args.push_back(player);
	for (auto& part : cmdParts)
	{
		if (Utils::Strings::isNumber<int>(part))
		{
			args.push_back(stoi(part));
		}
		else if (Utils::Strings::isNumber<double>(part))
		{
			args.push_back(stod(part));
		}
		else
		{
			args.push_back(part);
		}
	}
	try
	{
		this->callCommandHandler(commandName, args);
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

void CommandManager::callCommandHandler(
	const std::string& cmdName, CommandCallbackValues args)
{
	bool notFound = true;
	for (const auto& handler : this->_commandHandlers[cmdName])
	{
		try
		{
			(*handler)(args);
		}
		catch (const std::invalid_argument& e)
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