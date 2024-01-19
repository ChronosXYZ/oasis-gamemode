#include "AuthHandler.hpp"
#include "../CoreManager.hpp"
#include "network.hpp"
#include "player.hpp"
#include <exception>
#include <fmt/printf.h>
#include <spdlog/spdlog.h>
#include <string>

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

bool AuthHandler::loadPlayerData(IPlayer& player)
{
	auto db = this->coreManager.lock()->getDBConnection();
	pqxx::work txn(*db);
	pqxx::result res = txn.exec_params("SELECT id, \
					name, \
					users.password_hash, \
					\"language\", \
					email, \
					last_skin_id, \
					last_ip, \
					last_login_at, \
					bans.reason as \"ban_reason\", \
					bans.by as \"banned_by\", \
					bans.expires_at as \"ban_expires_at\", \
					admins.\"level\" as \"admin_level\", \
					admins.password_hash as \"admin_pass_hash\" \
					FROM users \
					LEFT JOIN bans \
					ON users.id = bans.user_id \
					LEFT JOIN admins \
					ON users.id = admins.user_id \
					WHERE name=$1",
		player.getName().to_string());

	if (res.size() == 0)
	{
		return false;
	}
	spdlog::info("Found player data for " + player.getName().to_string() + " in DB");

	auto row = res[0];
	std::shared_ptr<PlayerModel> playerData(new PlayerModel(row));

	this->coreManager.lock()->attachPlayerData(player, playerData);
	txn.commit();
	return true;
}

void AuthHandler::onPlayerConnect(IPlayer& player)
{
	if (!loadPlayerData(player))
	{
		this->showRegistrationDialog(player);
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
		"" DIALOG_HEADER " Registration",
		fmt::sprintf(_("{999999}Welcome to {FF0000}Oasis {FFFFFF}Freeroam, {ff9933}%s\n\n\n"
					   "{D3D3D3}> This name is not registered\n"
					   "{D3D3D3}> Please register with a valid password\n"
					   "{D3D3D3}> Acceptable passwords are at least 6 characters long\n\n\n"
					   "{D3D3D3}> If you have any trouble, please visit our discord server or contact any staff member:\n"
					   "{ff9933}oasisfreeroam.xyz",
						 player),
			player.getName().to_string()),
		_("Register", player),
		_("Quit", player),
		[&](DialogResponse resp, int listItem, StringView inputText)
		{
			switch (resp)
			{
			case DialogResponse_Left:
			{
				onRegistrationSubmit(player, inputText.to_string());
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
		"" DIALOG_HEADER " Login",
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
		auto playerData = this->coreManager.lock()->getPlayerData(player)->get();
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

void AuthHandler::onRegistrationSubmit(IPlayer& player, const std::string& password)
{
	if (password.length() <= 5 || password.length() > 48)
	{
		player.sendClientMessage(consts::RED_COLOR, _("Error: Password length must be between 6-48", player));
		this->showRegistrationDialog(player);
		return;
	}
	auto hashedPassword = Utils::argon2HashPassword(password);

	auto db = this->coreManager.lock()->getDBConnection();

	pqxx::work txn(*db);

	PeerAddress::AddressString ipString;
	PeerNetworkData peerData = player.getNetworkData();
	peerData.networkID.address.ToString(peerData.networkID.address, ipString);
	try
	{
		pqxx::result res = txn.exec_params("INSERT INTO users "
										   "(\"name\", password_hash, \"language\", email, last_skin_id, last_ip) "
										   "VALUES($1, $2, $3, $4, $5, $6)",
			player.getName().to_string(),
			hashedPassword,
			"en",
			"",
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
	loadPlayerData(player);
	player.spawn();
}