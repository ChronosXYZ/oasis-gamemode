#pragma once

#include <fmt/core.h>
#include <player.hpp>
#include <stdexcept>
#include <variant>
#include <vector>

namespace Core::Commands
{
using CommandCallbackValue
	= std::variant<std::reference_wrapper<IPlayer>, std::string, int, double>;
using CommandCallbackValues = std::vector<CommandCallbackValue>;

// Helper concept to check if a type is one of the allowed types
template <typename T, typename... AllowedTypes>
concept AllowedType = (std::is_same_v<AllowedTypes, T> || ...);

template <typename... Allowed> struct CommandCallbackFunctionTraitHelper
{
	template <typename R, AllowedType<Allowed...>... Args>
	auto operator()(std::function<R(Args...)> t) -> void;
};

template <typename T, typename... Allowed>
concept CommandCallbackFunction
	= requires(T t, CommandCallbackFunctionTraitHelper<Allowed...> h) {
		  {
			  h(std::function(t))
		  } -> std::same_as<void>;
	  };

struct CommandCallbackWrapper
{
	template <typename Callable>
	CommandCallbackWrapper(Callable fn)
		: fn_(Adaptor<Callable>::to_call(std::move(fn)))
	{
	}

	void operator()(const CommandCallbackValues& vv) const { fn_(vv); }

private:
	using call = std::function<void(const CommandCallbackValues&)>;
	call fn_;

	////////////////////
	template <typename... Args> using Callable = std::function<void(Args...)>;

	template <typename... Args, std::size_t... Is>
	static call to_call_(Callable<Args...> fn, std::index_sequence<Is...>)
	{
		return [fn_ = std::move(fn)](const CommandCallbackValues& vv)
		{
			if (!(vv.size() == sizeof...(Is)))
			{
				throw std::invalid_argument(
					fmt::format("argument vector length doesn't match callback "
								"signature argument count: {}, expected {}",
						vv.size(), sizeof...(Is)));
			}
			try
			{
				fn_(std::get<Args>(vv[Is])...);
			}
			catch (const std::bad_variant_access)
			{
				throw std::invalid_argument(
					"invalid type of argument when tried to call the callback");
			}
		};
	}

	template <typename C, typename = void> struct Adaptor
	{
	};

	template <typename... Args> struct Adaptor<Callable<Args...>>
	{
		static auto to_call(Callable<Args...> fn)
		{
			constexpr auto N = sizeof...(Args);
			return to_call_(std::move(fn), std::make_index_sequence<N>());
		}
	};

	template <typename... Args>
	struct Adaptor<void (*)(Args...)> : public Adaptor<Callable<Args...>>
	{
	};

	template <typename C, typename... Args>
	struct Adaptor<void (C::*)(Args...)> : public Adaptor<Callable<Args...>>
	{
	};

	template <typename C, typename... Args>
	struct Adaptor<void (C::*)(Args...) const>
		: public Adaptor<void (C::*)(Args...)>
	{
	};

	template <typename C>
	struct Adaptor<C, std::void_t<decltype(&C::operator())>>
		: public Adaptor<decltype(&C::operator())>
	{
	};
};
}