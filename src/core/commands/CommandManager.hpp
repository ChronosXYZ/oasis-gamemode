#pragma once

#include "CommandInfo.hpp"

#include <functional>
#include <player.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace Core::Commands
{

// Define a concept that checks if a callable matches the signature int(int,
// double)
template <typename F>
concept MatchesSignature
	= std::is_same_v<
		  bool (F::*)(std::reference_wrapper<IPlayer>, std::string) const, F>
	|| std::is_same_v<bool (*)(std::reference_wrapper<IPlayer>, std::string),
		F>;
// Concept for accepting std::function with two arguments: IPlayer& and
// std::string
template <typename F>
concept MatchesStdFunctionSignature = requires(F f) {
	{
		f(std::declval<std::reference_wrapper<IPlayer>>(),
			std::declval<std::string>())
	} -> std::same_as<bool>;
};
// concept MatchesSignature
// 	= std::is_same_v<
// 		  bool (F::*)(std::reference_wrapper<IPlayer>, std::string) const, F>
// 	|| std::is_same_v<bool (*)(std::reference_wrapper<IPlayer>, std::string),
// 		F>;

// Specialization for free functions
template <typename R, typename... Args>
concept MatchesFreeFunctionSignature = requires(R (*f)(Args...)) {
	{
		f
	} -> std::same_as<R (*)(Args...)>;
};

using HandlerSignature = bool(std::reference_wrapper<IPlayer>, std::string);

struct common_tag
{
};

class CommandManager : public PlayerTextEventHandler
{
	std::unordered_map<std::string,
		std::vector<std::function<HandlerSignature>>>
		_commandHandlers;
	std::unordered_map<std::string, std::vector<std::shared_ptr<CommandInfo>>>
		_commandInfo;
	std::unordered_map<std::string, std::vector<std::shared_ptr<CommandInfo>>>
		_commandCategories;
	IPlayerPool* _playerPool;

	void callCommandHandler(IPlayer& player, const std::string& cmdName,
		std::string preparedCommandArgs);
	void sendCommandUsage(IPlayer& player, const std::string& name);

public:
	CommandManager(IPlayerPool* playerPool);
	~CommandManager();

	template <MatchesSignature F>
	void addCommand(std::string name, F handler, CommandInfo info)
	{
		this->addCommand(name, handler, info, common_tag {});
	};

	template <MatchesStdFunctionSignature F>
	void addCommand(std::string name, F handler, CommandInfo info)
	{
		this->addCommand(name, handler, info, common_tag {});
	};

	template <MatchesFreeFunctionSignature<HandlerSignature> F>
	void addCommand(std::string name, F handler, CommandInfo info)
	{
		this->addCommand(name, handler, info, common_tag {});
	};

	template <typename F>
	void addCommand(
		std::string name, F handler, CommandInfo info, common_tag tag)
	{
		this->_commandHandlers[name].push_back(
			(std::function<HandlerSignature>(handler)));
		auto infoPtr = std::make_shared<CommandInfo>(info);
		this->_commandInfo[name].push_back(infoPtr);
		this->_commandCategories.insert(
			{ infoPtr->category, std::vector<std::shared_ptr<CommandInfo>>() });
		this->_commandCategories.at(infoPtr->category).push_back(infoPtr);
	};

	bool onPlayerCommandText(IPlayer& player, StringView commandText) override;
};
}