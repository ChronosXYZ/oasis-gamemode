#include "AuthHandler.hpp"
#include "../CoreManager.hpp"
#include "Server/Components/Dialogs/dialogs.hpp"
#include "player.hpp"
#include <fmt/printf.h>

using namespace Core;

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
		showLoginDialog(player, false);
		return;
	}
}

void AuthHandler::showRegistrationDialog(IPlayer& player)
{
	this->coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle_PASSWORD,
		"" DIALOG_HEADER "Registration",
		fmt::sprintf(_("{999999}Welcome to {FF0000}Oasis {FFFFFF}Freeroam, {ff9933}%s\n\n\n"
					   "{D3D3D3}> This name is not registered\n"
					   "{D3D3D3}> Please register with a valid password\n"
					   "{D3D3D3}> Acceptable passwords are at least 6 characters long\n\n\n"
					   "{D3D3D3}> If you have any trouble, please visit our discord server or contact any staff member:\n"
					   "{ff9933}oasisfreeroam.xyz",
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
	this->coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle_PASSWORD,
		"" DIALOG_HEADER "Login",
		wrongPass ? fmt::sprintf(_("{FF0000}Wrong Password {D3D3D3}entered for {FFFFFF}%s {FF0000}[%d/3]\n"
								   "{D3D3D3}Please re-write the correct password in the field below to login.\n\n"
								   "{D3D3D3}- If you have forgotten your password, request a password recovery at our discord server:\n"
								   "{ff9933}oasisfreeroam.xyz",
									 player),
			player.getName().to_string(), 0) // TODO login attempts
				  : fmt::sprintf(_("{D3D3D3}Welcome back to {FF0000}Oasis {FFFFFF}Freeroam {ff9933}%s\n\n\n"
								   "- {D3D3D3}Your name is registered in our database\n"
								   "- {D3D3D3}Login by entering your password\n\n\n"
								   "- {D3D3D3}If you have forgotten your password, request a password recovery at our discord server:\n{ff9933}oasisfreeroam.xyz",
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
	if (password.length() > 5)
	{
		auto playerData = this->coreManager.lock()->getPlayerData(player);
		if (Utils::argon2VerifyEncodedHash(playerData->passwordHash, password))
		{
			player.sendClientMessage(Colour::White(), "You have been logged in!");
			player.spawn();
		}
		else
		{
			this->showLoginDialog(player, true);
		}
	}
	else
	{
		this->showLoginDialog(player, true);
		return;
	}
}

void AuthHandler::onPasswordSubmit(IPlayer& player, const std::string& password)
{
	if (password.length() <= 5 || password.length() > 48)
	{
		player.sendClientMessage(consts::RED_COLOR, _("Error: Password length must be between 6-48", player));
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
	auto db = this->coreManager.lock()->getDBConnection();

	pqxx::work txn(*db);

	PeerAddress::AddressString ipString;
	PeerNetworkData peerData = player.getNetworkData();
	auto pData = this->coreManager.lock()->getPlayerData(player);
	peerData.networkID.address.ToString(peerData.networkID.address, ipString);
	try
	{
		pqxx::result res = txn.exec_params("INSERT INTO users "
										   "(\"name\", password_hash, \"language\", email, last_skin_id, last_ip) "
										   "VALUES($1, $2, $3, $4, $5, $6)",
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
		player.kick();
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
		fmt::sprintf(_("{D3D3D3}Please enter your {FFFFFF}Email address{D3D3D3}.\n\n"
					   "Use a correct email address as it can be used for password recovery\n"
					   "to restore your account incase you forget your password\n"
					   "You can skip entering email, but account features will be limited! Use {D3D3D3}/verify{FFFFFF} later",
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
		player.sendClientMessage(consts::RED_COLOR, _("[ERROR]{FFFFFF} Invalid email!", player));
		return;
	}

	auto data = this->coreManager.lock()->getPlayerData(player);
	data->email = email;
	onRegistrationSubmit(player);
}