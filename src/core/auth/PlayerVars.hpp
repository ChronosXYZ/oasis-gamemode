#pragma once

#include <string>

namespace Core::Auth::PlayerVars
{
using namespace std::string_literals;

inline const auto LOGIN_ATTEMPTS_KEY = "AUTH_LOGIN_ATTEMPTS"s;
inline const auto IS_REQUEST_CLASS_ALREADY_CALLED = "AUTH_IS_REQUEST_CLASS_ALREADY_CALLED"s;
inline const auto PLAIN_TEXT_PASSWORD = "AUTH_PLAIN_TEXT_PASSWORD"s;
}