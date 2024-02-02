
#include "AuthHandler.hpp"
#include "PlayerVars.hpp"
#include "../CoreManager.hpp"
#include "../PlayerVars.hpp"
#include "../utils/Localization.hpp"
#include "../SQLQueryManager.hpp"
#include "../utils/QueryNames.hpp"
#include "../utils/Argon2idHash.hpp"
#include "../player/PlayerExtension.hpp"
#include "../../constants.hpp"

#include <fmt/printf.h>
#include <spdlog/spdlog.h>

namespace Core::Auth
{

AuthHandler::AuthHandler(IPlayerPool* playerPool, std::weak_ptr<CoreManager> coreManager)
	: _coreManager(coreManager)
	, _playerPool(playerPool)
	, _classesComponent(coreManager.lock()->components->queryComponent<IClassesComponent>())
{
	playerPool->getPlayerConnectDispatcher().addEventHandler(this);
	_classesComponent->getEventDispatcher().addEventHandler(this);
}

AuthHandler::~AuthHandler()
{
	_playerPool->getPlayerConnectDispatcher().removeEventHandler(this);
	_classesComponent->getEventDispatcher().removeEventHandler(this);
}

void AuthHandler::onPlayerConnect(IPlayer& player)
{
}

void AuthHandler::showRegistrationDialog(IPlayer& player)
{
	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle_PASSWORD,
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Registration", player)),
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
		auto data = this->_coreManager.lock()->getPlayerData(player);
		loginAttempts = std::get<int>(data->getTempData(PlayerVars::LOGIN_ATTEMPTS_KEY).value());
	}
	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle_PASSWORD,
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Login", player)),
		wrongPass ? fmt::sprintf(_("#RED#Wrong Password #LIGHT_GRAY#entered for %s #RED#[%d/3]\n"
								   "#LIGHT_GRAY#Please re-write the correct password in the field below to login.\n\n"
								   "#LIGHT_GRAY#- If you have forgotten your password, request a password recovery at our discord server:\n"
								   "#DEEP_SAFFRON#oasisfreeroam.xyz",
									 player),
			player.getName().to_string(), loginAttempts)
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
	auto playerData = this->_coreManager.lock()->getPlayerData(player);
	auto playerExt = Player::getPlayerExt(player);
	if (password.length() > 5)
	{
		if (Utils::argon2VerifyEncodedHash(playerData->passwordHash, password))
		{
			playerExt->sendInfoMessage(_("You have been logged in!", player));
			playerData->lastLoginAt = Utils::SQL::get_current_timestamp();
			playerData->lastIP = playerExt->getIP();
			playerData->deleteTempData(PlayerVars::LOGIN_ATTEMPTS_KEY);
			this->_coreManager.lock()->onPlayerLoggedIn(player);
			playerData->deleteTempData(PlayerVars::IS_REQUEST_CLASS_ALREADY_CALLED);
			return;
		}
	}

	auto loginAttempts = std::get<int>(playerData->getTempData(PlayerVars::LOGIN_ATTEMPTS_KEY).value());
	playerData->setTempData(PlayerVars::LOGIN_ATTEMPTS_KEY, ++loginAttempts);
	if (loginAttempts > 3)
	{
		playerExt->sendErrorMessage(_("Too much login attempts!", player));
		playerExt->delayedKick();
		return;
	}
	this->showLoginDialog(player, true);
}

void AuthHandler::onPasswordSubmit(IPlayer& player, const std::string& password)
{
	if (password.length() <= 5 || password.length() > 48)
	{
		Player::getPlayerExt(player)->sendErrorMessage(_("Password length must be between 6-48", player));
		this->showRegistrationDialog(player);
		return;
	}
	auto hashedPassword = Utils::argon2HashPassword(password);
	auto pData = this->_coreManager.lock()->getPlayerData(player);
	pData->passwordHash = hashedPassword;
	this->showEmailDialog(player);
	pData->setTempData(PlayerVars::PLAIN_TEXT_PASSWORD, password);
}

void AuthHandler::onRegistrationSubmit(IPlayer& player)
{
	auto playerExt = Player::getPlayerExt(player);
	auto db = this->_coreManager.lock()->getDBConnection();

	pqxx::work txn(*db);

	auto pData = this->_coreManager.lock()->getPlayerData(player);
	try
	{
		pqxx::result res = txn.exec_params(SQLQueryManager::Get()->getQueryByName(Utils::SQL::Queries::CREATE_PLAYER).value(),
			player.getName().to_string(),
			pData->passwordHash,
			pData->language,
			pData->email,
			1,
			playerExt->getIP());
		txn.commit();
	}
	catch (const std::exception& e)
	{
		txn.abort();
		spdlog::error(std::format("Error occurred when trying to create new user entry in DB. Error: {}", e.what()));
		playerExt->sendInfoMessage(_("Something went wrong when trying to create user!", player));
		playerExt->delayedKick();
		return;
	}
	this->_coreManager.lock()->refreshPlayerData(player);
	pData->deleteTempData(PlayerVars::IS_REQUEST_CLASS_ALREADY_CALLED);
	this->showRegistrationInfoDialog(player);
}

void AuthHandler::showLanguageDialog(IPlayer& player)
{
	std::string languagesList;
	for (auto lang : Localization::LANGUAGES)
	{
		languagesList.append(fmt::sprintf("%s\n", lang));
	}
	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle_LIST,
		fmt::sprintf(DIALOG_HEADER_TITLE, "Select language"),
		languagesList,
		"Select",
		"Quit",
		[&](DialogResponse resp, int listItem, StringView inputText)
		{
			switch (resp)
			{
			case DialogResponse_Left:
			{
				auto pData = this->_coreManager.lock()->getPlayerData(player);
				pData->language = Localization::LANGUAGE_CODE_NAMES.at(listItem);
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
	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle_INPUT,
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Enter your email", player)),
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
	if (!std::regex_match(email, m, EMAIL_REGEX))
	{
		this->showEmailDialog(player);
		Player::getPlayerExt(player)->sendErrorMessage(_("Invalid email!", player));
		return;
	}

	auto data = this->_coreManager.lock()->getPlayerData(player);
	data->email = email;
	onRegistrationSubmit(player);
}

bool AuthHandler::onPlayerRequestClass(IPlayer& player, unsigned int classId)
{
	auto pData = this->_coreManager.lock()->getPlayerData(player);
	if (pData->getTempData(Core::PlayerVars::IS_LOGGED_IN) || pData->getTempData(Core::PlayerVars::CURRENT_MODE))
		return true;

	if (!pData->getTempData(PlayerVars::IS_REQUEST_CLASS_ALREADY_CALLED))
	{
		if (!this->_coreManager.lock()->refreshPlayerData(player))
		{
			this->showLanguageDialog(player);
		}
		else
		{
			auto data = this->_coreManager.lock()->getPlayerData(player);
			data->setTempData(PlayerVars::LOGIN_ATTEMPTS_KEY, 0);
			showLoginDialog(player, false);
		}
		pData->setTempData(PlayerVars::IS_REQUEST_CLASS_ALREADY_CALLED, true);
	}

	player.setSpectating(true);
	player.interpolateCameraPosition(Vector3(1093.0, -2036.0, 90.0),
		Vector3(21.1088, -1806.9847, 79.4125),
		15000,
		PlayerCameraCutType_Move);
	player.interpolateCameraLookAt(Vector3(384.0, -1557.0, 20.0),
		Vector3(455.1533, -1868.1967, 31.9840),
		15000,
		PlayerCameraCutType_Move);
	return true;
}

void AuthHandler::showRegistrationInfoDialog(IPlayer& player)
{
	auto pData = this->_coreManager.lock()->getPlayerData(player);
	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle::DialogStyle_MSGBOX,
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Registration info", player)),
		fmt::sprintf(_("#DRY_HIGHLIGHTER_GREEN#Your account has been successfully registered and logged in.\n\n"
					   "#MILLION_GREY#Username: #WHITE#%s\n"
					   "#MILLION_GREY#Account ID: #WHITE#%d\n"
					   "#MILLION_GREY#Password: #WHITE#%s\n"
					   "#MILLION_GREY#Registration date: #WHITE#%s\n\n"
					   "#MILLION_GREY#Use '#RED#F8#MILLION_GREY#' to take screenshot and save the password.",
						 player),
			pData->name,
			pData->userId,
			std::get<std::string>(*pData->getTempData(PlayerVars::PLAIN_TEXT_PASSWORD)),
			std::format("{:%Y-%m-%d}", pData->registrationDate)),
		_("OK", player), "",
		[&](DialogResponse resp, int listItem, StringView inputText)
		{
			Player::getPlayerExt(player)->sendInfoMessage(_("You have successfully registered!", player));
			this->_coreManager.lock()->onPlayerLoggedIn(player);
		});
}
}