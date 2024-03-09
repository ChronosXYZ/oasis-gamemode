#pragma once

#include "CommandCallbackWrapper.hpp"
#include "CommandInfo.hpp"

#include <player.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace Core::Commands
{
class CommandManager : public PlayerTextEventHandler
{
	std::unordered_map<std::string, std::vector<std::unique_ptr<CommandCallbackWrapper>>> _commandHandlers;
	std::unordered_map<std::string, std::vector<std::shared_ptr<CommandInfo>>> _commandInfo;
	std::unordered_map<std::string, std::vector<std::shared_ptr<CommandInfo>>> _commandCategories;
	IPlayerPool* _playerPool;

	void callCommandHandler(const std::string& cmdName, CommandCallbackValues args);
	void sendCommandUsage(IPlayer& player, const std::string& name);

public:
	CommandManager(IPlayerPool* playerPool);
	~CommandManager();

	template <typename F>
		requires CommandCallbackFunction<F, std::reference_wrapper<IPlayer>, double, int, std::string>
	void addCommand(std::string name, F handler, CommandInfo info)
	{
		this->_commandHandlers[name].push_back(std::unique_ptr<CommandCallbackWrapper>(new CommandCallbackWrapper(handler)));
		auto infoPtr = std::make_shared<CommandInfo>(info);
		this->_commandInfo[name].push_back(infoPtr);
		this->_commandCategories.insert({ infoPtr->category, std::vector<std::shared_ptr<CommandInfo>>() });
		this->_commandCategories.at(infoPtr->category).push_back(infoPtr);
	};

	bool onPlayerCommandText(IPlayer& player, StringView commandText) override;
};
}