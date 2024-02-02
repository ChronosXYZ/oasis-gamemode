#pragma once

#include <string>

namespace Core::Auth::PlayerVars
{
using namespace std::string_literals;

inline const auto LOGIN_ATTEMPTS_KEY = "login_attempts"s;
inline const auto IS_REQUEST_CLASS_ALREADY_CALLED = "isRequestClassAlreadyCalled"s;
inline const auto PLAIN_TEXT_PASSWORD = "plainTextPassword"s;
}