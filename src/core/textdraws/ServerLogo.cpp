#include "ServerLogo.hpp"

#include <component.hpp>
#include <Server/Components/TextDraws/textdraws.hpp>
#include <spdlog/spdlog.h>
#include <types.hpp>
#include <player.hpp>

namespace Core::TextDraws
{

ServerLogo::ServerLogo(IPlayer& player, const std::string& name, const std::string& bottomLabel, const std::string& website)
	: _playerTextDrawData(queryExtension<IPlayerTextDrawData>(player))
{
	_topLabel = _playerTextDrawData->create(Vector2(576.5000, 1.5000), name);
	_topLabel->setStyle(TextDrawStyle_3);
	_topLabel->setLetterSize(Vector2(0.4299, 1.7999));
	_topLabel->setAlignment(TextDrawAlignmentTypes::TextDrawAlignment_Center);
	_topLabel->setColour(Colour::FromRGBA(-18));
	_topLabel->setShadow(1);
	_topLabel->setOutline(0);
	_topLabel->setBackgroundColour(Colour::FromRGBA(170));
	_topLabel->setProportional(true);
	_topLabel->setTextSize(Vector2(45.5000, 0.0000));

	_bottomLabel = _playerTextDrawData->create(Vector2(577.5000, 17.0000), bottomLabel);
	_bottomLabel->setStyle(TextDrawStyle_3);
	_bottomLabel->setLetterSize(Vector2(0.2999, 1.3999));
	_bottomLabel->setAlignment(TextDrawAlignmentTypes::TextDrawAlignment_Center);
	_bottomLabel->setColour(Colour::FromRGBA(-18));
	_bottomLabel->setShadow(0);
	_bottomLabel->setOutline(1);
	_bottomLabel->setBackgroundColour(Colour::FromRGBA(170));
	_bottomLabel->setProportional(true);
	_bottomLabel->setTextSize(Vector2(87.0, 0.0));

	_box = _playerTextDrawData->create(Vector2(577.5, 32.0), "_");
	_box->setStyle(TextDrawStyle_2);
	_box->setLetterSize(Vector2(0.1199, 0.2999));
	_box->setAlignment(TextDrawAlignment_Center);
	_box->setColour(Colour::FromRGBA(255));
	_box->setShadow(0);
	_box->setOutline(0);
	_box->setBackgroundColour(Colour::FromRGBA(255));
	_box->setProportional(true);
	_box->useBox(true);
	_box->setBoxColour(Colour::FromRGBA(-16777114));
	_box->setTextSize(Vector2(87.0, 50.0));

	_website = _playerTextDrawData->create(Vector2(577.5, 29.0), website);
	_website->setStyle(TextDrawStyle_2);
	_website->setLetterSize(Vector2(0.1199, 0.7999));
	_website->setAlignment(TextDrawAlignment_Center);
	_website->setColour(Colour::FromRGBA(255));
	_website->setShadow(0);
	_website->setOutline(0);
	_website->setBackgroundColour(Colour::FromRGBA(255));
	_website->setProportional(true);
	_website->setTextSize(Vector2(87.0, 0.0));
}

void ServerLogo::destroy()
{
	_playerTextDrawData->release(_topLabel->getID());
	_playerTextDrawData->release(_bottomLabel->getID());
	_playerTextDrawData->release(_box->getID());
	_playerTextDrawData->release(_website->getID());
}

void ServerLogo::show()
{
	_topLabel->show();
	_bottomLabel->show();
	_box->show();
	_website->show();
}

void ServerLogo::hide()
{
	_topLabel->hide();
	_bottomLabel->hide();
	_box->hide();
	_website->hide();
}

const std::string& ServerLogo::name()
{
	return NAME;
}

}