#include "AuthHandler.hpp"
#include "../CoreManager.hpp"
#include "Server/Components/Dialogs/dialogs.hpp"
#include "component.hpp"
#include "player.hpp"
#include "types.hpp"
#include <chrono>
#include <fmt/printf.h>

using namespace Core;

const std::string LOGIN_ATTEMPTS_KEY = "login_attempts";

AuthHandler::AuthHandler(IPlayerPool* playerPool, std::weak_ptr<CoreManager> coreManager)
	: coreManager(coreManager)
	, playerPool(playerPool)
{
	playerPool->getPlayerConnectDispatcher().addEventHandler(this);
}

AuthHandler::~AuthHandler()
{
	playerPool->getPlayerConnectDispatcher().removeEventHandler(this);
}

void AuthHandler::onPlayerConnect(IPlayer& player)
{
	if (!this->coreManager.lock()->refreshPlayerData(player))
	{
		this->showLanguageDialog(player);
		return;
	}
	else
	{
		auto data = this->coreManager.lock()->getPlayerData(player);
		data->setTempData(LOGIN_ATTEMPTS_KEY, 0);
		showLoginDialog(player, false);
		return;
	}
}

void AuthHandler::showRegistrationDialog(IPlayer& player)
{
	this->coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle_PASSWORD,
		"" DIALOG_HEADER "Registration",
		fmt::sprintf(_("{999999}Welcome to #RED#Oasis #WHITE#Freeroam, #DEEP_SAFFRON#%s\n\n\n"
					   "#LIGHT_GRAY#> This name is not registered\n"
					   "#LIGHT_GRAY#> Please register with a valid password\n"
					   "#LIGHT_GRAY#> Acceptable passwords are at least 6 characters long\n\n\n"
					   "#LIGHT_GRAY#> If you have any trouble, please visit our discord server or contact any staff member:\n"
					   "#DEEP_SAFFRON#oasisfreeroam.xyz",
						 player),
			player.getName().to_string()),
		_("Enter", player),
		_("Quit", player),
		[&](DialogResponse resp, int listItem, StringView inputText)
		{
			switch (resp)
			{
			case DialogResponse_Left:
			{
				onPasswordSubmit(player, inputText.to_string());
				break;
			}
			case DialogResponse_Right:
			{
				player.kick();
				break;
			}
			}
		});
}

void AuthHandler::showLoginDialog(IPlayer& player, bool wrongPass)
{
	int loginAttempts = 0;
	if (wrongPass)
	{
		auto data = this->coreManager.lock()->getPlayerData(player);
		loginAttempts = std::get<int>(data->getTempData(LOGIN_ATTEMPTS_KEY).value());
	}
	this->coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle_PASSWORD,
		"" DIALOG_HEADER "Login",
		wrongPass ? fmt::sprintf(_("#RED#Wrong Password #LIGHT_GRAY#entered for %s #RED#[%d/3]\n"
								   "#LIGHT_GRAY#Please re-write the correct password in the field below to login.\n\n"
								   "#LIGHT_GRAY#- If you have forgotten your password, request a password recovery at our discord server:\n"
								   "#DEEP_SAFFRON#oasisfreeroam.xyz",
									 player),
			player.getName().to_string(), loginAttempts) // TODO login attempts
				  : fmt::sprintf(_("#LIGHT_GRAY#Welcome back to #RED#Oasis #WHITE#Freeroam #DEEP_SAFFRON#%s\n\n"
								   "#LIGHT_GRAY#- Your name is registered in our database\n"
								   "#LIGHT_GRAY#- Login by entering your password\n\n"
								   "#LIGHT_GRAY#- If you have forgotten your password, request a password recovery at our discord server:\n#DEEP_SAFFRON#oasisfreeroam.xyz",
									 player),
					  player.getName().to_string()),
		_("Login", player),
		_("Quit", player),
		[&](DialogResponse resp, int listItem, StringView inputText)
		{
			switch (resp)
			{
			case DialogResponse_Left:
			{
				onLoginSubmit(player, inputText.to_string());
				break;
			}
			case DialogResponse_Right:
			{
				player.kick();
				break;
			}
			}
		});
}

void AuthHandler::onLoginSubmit(IPlayer& player, const std::string& password)
{
	auto playerData = this->coreManager.lock()->getPlayerData(player);
	auto playerExt = this->coreManager.lock()->getPlayerExt(player);
	if (password.length() > 5)
	{
		if (Utils::argon2VerifyEncodedHash(playerData->passwordHash, password))
		{
			player.sendClientMessage(Colour::White(), _("You have been logged in!", player));
			auto data = this->coreManager.lock()->getPlayerData(player);
			data->lastLoginAt = Utils::SQL::get_current_timestamp();
			player.spawn();
			playerData->deleteTempData(LOGIN_ATTEMPTS_KEY);
			return;
		}
	}

	auto loginAttempts = std::get<int>(playerData->getTempData(LOGIN_ATTEMPTS_KEY).value());
	playerData->setTempData(LOGIN_ATTEMPTS_KEY, ++loginAttempts);
	if (loginAttempts > 3)
	{
		player.sendClientMessage(consts::RED_COLOR, _("[Error] #WHITE#Too much login attempts!", player));
		playerExt->delayedKick();
		return;
	}
	this->showLoginDialog(player, true);
}

void AuthHandler::onPasswordSubmit(IPlayer& player, const std::string& password)
{
	if (password.length() <= 5 || password.length() > 48)
	{
		player.sendClientMessage(consts::RED_COLOR, _("[Error] #WHITE#Password length must be between 6-48", player));
		this->showRegistrationDialog(player);
		return;
	}
	auto hashedPassword = Utils::argon2HashPassword(password);
	auto pData = this->coreManager.lock()->getPlayerData(player);
	pData->passwordHash = hashedPassword;
	this->showEmailDialog(player);
}

void AuthHandler::onRegistrationSubmit(IPlayer& player)
{
	auto playerExt = this->coreManager.lock()->getPlayerExt(player);
	auto db = this->coreManager.lock()->getDBConnection();

	pqxx::work txn(*db);

	PeerAddress::AddressString ipString;
	PeerNetworkData peerData = player.getNetworkData();
	auto pData = this->coreManager.lock()->getPlayerData(player);
	peerData.networkID.address.ToString(peerData.networkID.address, ipString);
	try
	{
		pqxx::result res = txn.exec_params(SQLQueryManager::Get()->getQueryByName(Utils::SQL::Queries::CREATE_PLAYER).value(),
			player.getName().to_string(),
			pData->passwordHash,
			pData->language,
			pData->email,
			1,
			ipString.data());
		txn.commit();
	}
	catch (const std::exception& e)
	{
		txn.abort();
		spdlog::error(std::format("Error occurred when trying to create new user entry in DB. Error: {}", e.what()));
		player.sendClientMessage(consts::RED_COLOR, "Error occurred when trying to create user!");
		playerExt->delayedKick();
		return;
	}
	this->coreManager.lock()->refreshPlayerData(player);
	player.spawn();
}

void AuthHandler::showLanguageDialog(IPlayer& player)
{
	std::string languagesList;
	for (auto lang : consts::LANGUAGES)
	{
		languagesList.append(fmt::sprintf("%s\n", lang));
	}
	this->coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle_LIST,
		"" DIALOG_HEADER "| Select language",
		languagesList,
		"Select",
		"Quit",
		[&](DialogResponse resp, int listItem, StringView inputText)
		{
			switch (resp)
			{
			case DialogResponse_Left:
			{
				auto pData = this->coreManager.lock()->getPlayerData(player);
				pData->language = consts::LANGUAGE_CODE_NAME.at(listItem);
				showRegistrationDialog(player);
				break;
			}
			case DialogResponse_Right:
			{
				player.kick();
				break;
			}
			}
		});
}

void AuthHandler::showEmailDialog(IPlayer& player)
{
	this->coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle_PASSWORD,
		"" DIALOG_HEADER "| Email",
		fmt::sprintf(_("#LIGHT_GRAY#Please enter your #WHITE#Email address#LIGHT_GRAY#.\n\n"
					   "Use a correct email address as it can be used for password recovery\n"
					   "to restore your account incase you forget your password\n"
					   "You can skip entering email, but account features will be limited! Use #WHITE#/verify#LIGHT_GRAY# later",
						 player),
			player.getName().to_string()),
		_("Enter", player),
		_("Skip", player),
		[&](DialogResponse resp, int listItem, StringView inputText)
		{
			switch (resp)
			{
			case DialogResponse_Left:
			{
				onEmailSubmit(player, inputText.to_string());
				break;
			}
			case DialogResponse_Right:
			{
				onRegistrationSubmit(player);
				break;
			}
			}
		});
}

void AuthHandler::onEmailSubmit(IPlayer& player, const std::string& email)
{
	std::smatch m;
	if (!std::regex_match(email, m, consts::EMAIL_REGEX))
	{
		this->showEmailDialog(player);
		player.sendClientMessage(consts::RED_COLOR, _("[ERROR] #WHITE#Invalid email!", player));
		return;
	}

	auto data = this->coreManager.lock()->getPlayerData(player);
	data->email = email;
	onRegistrationSubmit(player);
}