#pragma once

#include <string>

namespace Core::TextDraws
{
struct ITextDrawWrapper
{
	virtual void show() = 0;
	virtual void hide() = 0;
	virtual void destroy() = 0;
	ITextDrawWrapper() {};
	virtual ~ITextDrawWrapper() {};
};
}