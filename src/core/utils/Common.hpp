#pragma once

#include <numbers>

namespace Core::Utils
{
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
