#pragma once

#include "CommandCallbackWrapper.hpp"

#include <player.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace Core::Commands
{
class CommandManager : public PlayerTextEventHandler
{
	std::unordered_map<std::string, std::unique_ptr<CommandCallbackWrapper>> _commandHandlers;
	IPlayerPool* _playerPool;

	void callCommandHandler(const std::string& cmdName, CommandCallbackValues args);

public:
	CommandManager(IPlayerPool* playerPool);
	~CommandManager();

	template <typename F>
		requires CommandCallbackFunction<F, std::reference_wrapper<IPlayer>, double, int, std::string>
	void addCommand(std::string name, F handler)
	{
		this->_commandHandlers["/" + name] = std::unique_ptr<CommandCallbackWrapper>(new CommandCallbackWrapper(handler));
	};

	bool onPlayerCommandText(IPlayer& player, StringView commandText) override;
};
}