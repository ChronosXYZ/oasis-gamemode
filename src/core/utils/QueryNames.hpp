#pragma once

#include <string>

using namespace std;

#define constant inline static const

namespace Utils::SQL::Queries
{
constant string LOAD_PLAYER = "load_player";
constant string CREATE_PLAYER = "create_player";
constant string SAVE_PLAYER = "save_player";
}