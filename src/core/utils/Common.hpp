#pragma once

#include <bits/utility.h>
#include <functional>
#include <numbers>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <player.hpp>

namespace Utils
{

using CallbackValueType = std::variant<std::reference_wrapper<IPlayer>, std::string, int, double>;
using CallbackValuesType = std::vector<CallbackValueType>;

// Helper concept to check if a type is one of the allowed types
template <typename T, typename... AllowedTypes>
concept allowed_type
	= (std::is_same_v<AllowedTypes, T> || ...);

template <typename... Allowed>
struct callback_function_check_helper
{
	template <typename R, allowed_type<Allowed...>... Args>
	auto operator()(std::function<R(Args...)> t) -> void;
};

template <typename T, typename... Allowed>
concept callback_function = requires(T t, callback_function_check_helper<Allowed...> h) {
	{
		h(std::function(t))
	} -> std::same_as<void>;
};

struct CommandCallback
{
	template <typename Callable>
	CommandCallback(Callable fn)
		: fn_(Adaptor<Callable>::to_call(std::move(fn)))
	{
	}

	void operator()(const CallbackValuesType& vv) const { fn_(vv); }

private:
	using call = std::function<void(const CallbackValuesType&)>;
	call fn_;

	////////////////////
	template <typename... Args>
	using Callable = std::function<void(Args...)>;

	template <typename... Args, std::size_t... Is>
	static call to_call_(Callable<Args...> fn, std::index_sequence<Is...>)
	{
		return [fn_ = std::move(fn)](const CallbackValuesType& vv)
		{
			if (!(vv.size() == sizeof...(Is)))
			{
				throw std::invalid_argument("argument vector length doesn't match callback signature argument count");
			}
			/* if signature matches */ fn_(std::get<Args>(vv[Is])...);
		};
	}

	template <typename C, typename = void>
	struct Adaptor
	{
	};

	template <typename... Args>
	struct Adaptor<Callable<Args...>>
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
	struct Adaptor<void (C::*)(Args...) const> : public Adaptor<void (C::*)(Args...)>
	{
	};

	template <typename C>
	struct Adaptor<C, std::void_t<decltype(&C::operator())>> : public Adaptor<decltype(&C::operator())>
	{
	};
};

const inline double deg2Rad(const double& degrees)
{
	return degrees * std::numbers::pi / 180;
}

template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept
{
	return static_cast<typename std::underlying_type<E>::type>(e);
}
}
