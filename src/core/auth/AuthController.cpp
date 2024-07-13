#include "AuthController.hpp"
#include "../CoreManager.hpp"
#include "../utils/Localization.hpp"
#include "../SQLQueryManager.hpp"
#include "../utils/QueryNames.hpp"
#include "../utils/Argon2idHash.hpp"
#include "../player/PlayerExtension.hpp"

#include <fmt/printf.h>
#include <Server/Components/Timers/timers.hpp>
#include <Server/Components/Timers/Impl/timers_impl.hpp>
#include <functional>
#include <memory>
#include <spdlog/spdlog.h>

namespace Core::Auth
{

AuthController::AuthController(IPlayerPool* playerPool,
	std::shared_ptr<DialogManager> dialogManager,
	std::weak_ptr<CoreManager> coreManager)
	: _coreManager(coreManager)
	, playerPool(playerPool)
	, classesComponent(
		  coreManager.lock()->components->queryComponent<IClassesComponent>())
	, timersComponent(
		  coreManager.lock()->components->queryComponent<ITimersComponent>())
	, dialogManager(dialogManager)
{
	playerPool->getPlayerConnectDispatcher().addEventHandler(this);
}

AuthController::~AuthController()
{
	playerPool->getPlayerConnectDispatcher().removeEventHandler(this);
}

void AuthController::onPlayerConnect(IPlayer& player)
{
	player.setSpectating(true);

	if (!this->_coreManager.lock()->refreshPlayerData(player))
	{
		this->showLanguageDialog(player);
	}
	else
	{
		auto data = Player::getPlayerData(player);
		data->tempData->auth->loginAttempts = 0;
		showLoginDialog(player, false);
	}
	timersComponent->create(new Impl::SimpleTimerHandler(std::bind(
								&AuthController::interpolatePlayerCamera, this,
								std::reference_wrapper<IPlayer>(player))),
		Milliseconds(100), false);
}

void AuthController::showRegistrationDialog(IPlayer& player)
{
	auto dialog = std::shared_ptr<InputDialog>(new InputDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Registration", player)),
		fmt::sprintf(_("{999999}Welcome to #RED#Oasis #WHITE#Freeroam, "
					   "#DEEP_SAFFRON#%s\n\n\n"
					   "#LIGHT_GRAY#> This name is not registered\n"
					   "#LIGHT_GRAY#> Please register with a valid password\n"
					   "#LIGHT_GRAY#> Acceptable passwords are at least 6 "
					   "characters long\n\n\n"
					   "#LIGHT_GRAY#> If you have any trouble, please visit "
					   "our discord server or contact any staff member:\n"
					   "#DEEP_SAFFRON#oasisfreeroam.xyz",
			player)),
		true, _("Enter", player), _("Quit", player)));
	this->dialogManager->showDialog(player, dialog,
		[&](DialogResult result)
		{
			switch (result.response())
			{
			case DialogResponse_Left:
			{
				onPasswordSubmit(player, result.inputText());
				break;
			}
			case DialogResponse_Right:
			{
				Player::getPlayerExt(player)->delayedKick();
				break;
			}
			}
		});
}

void AuthController::showLoginDialog(IPlayer& player, bool wrongPass)
{
	int loginAttempts = 0;
	if (wrongPass)
	{
		auto data = Player::getPlayerData(player);
		loginAttempts = data->tempData->auth->loginAttempts;
	}

	auto dialog = std::shared_ptr<InputDialog>(new InputDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Login", player)),
		wrongPass
			? fmt::sprintf(
				_("#RED#Wrong Password #LIGHT_GRAY#entered for %s #RED#[%d/3]\n"
				  "#LIGHT_GRAY#Please re-write the correct password in the "
				  "field below to login.\n\n"
				  "#LIGHT_GRAY#- If you have forgotten your password, request "
				  "a password recovery at our discord server:\n"
				  "#DEEP_SAFFRON#oasisfreeroam.xyz",
					player),
				player.getName().to_string(), loginAttempts)
			: fmt::sprintf(
				_("#LIGHT_GRAY#Welcome back to #RED#Oasis #WHITE#Freeroam "
				  "#DEEP_SAFFRON#%s\n\n"
				  "#LIGHT_GRAY#- Your name is registered in our database\n"
				  "#LIGHT_GRAY#- Login by entering your password\n\n"
				  "#LIGHT_GRAY#- If you have forgotten your password, request "
				  "a password recovery at our discord "
				  "server:\n#DEEP_SAFFRON#oasisfreeroam.xyz",
					player),
				player.getName().to_string()),
		true, _("Login", player), _("Quit", player)));
	this->dialogManager->showDialog(player, dialog,
		[&](DialogResult result)
		{
			switch (result.response())
			{
			case DialogResponse_Left:
			{
				onLoginSubmit(player, result.inputText());
				break;
			}
			case DialogResponse_Right:
			{
				Player::getPlayerExt(player)->delayedKick();
				break;
			}
			}
		});
}

void AuthController::onLoginSubmit(IPlayer& player, const std::string& password)
{
	spdlog::info(password);
	auto playerData = Player::getPlayerData(player);
	auto playerExt = Player::getPlayerExt(player);
	if (password.length() > 5)
	{
		if (Utils::argon2VerifyEncodedHash(playerData->passwordHash, password))
		{
			playerExt->sendInfoMessage(__("You have been logged in!"));
			playerData->lastLoginAt = Utils::SQL::get_current_timestamp();
			playerData->lastIP = playerExt->getIP();
			this->_coreManager.lock()->onPlayerLoggedIn(player);
			return;
		}
	}
	int loginAttempts = playerData->tempData->auth->loginAttempts;
	playerData->tempData->auth->loginAttempts = ++loginAttempts;
	if (loginAttempts > 3)
	{
		playerExt->sendErrorMessage(_("Too much login attempts!", player));
		playerExt->delayedKick();
		return;
	}
	this->showLoginDialog(player, true);
}

void AuthController::onPasswordSubmit(
	IPlayer& player, const std::string& password)
{
	if (password.length() <= 5 || password.length() > 48)
	{
		Player::getPlayerExt(player)->sendErrorMessage(
			_("Password length must be between 6-48", player));
		this->showRegistrationDialog(player);
		return;
	}
	auto hashedPassword = Utils::argon2HashPassword(password);
	auto pData = this->_coreManager.lock()->getPlayerData(player);
	pData->passwordHash = hashedPassword;
	this->showEmailDialog(player);
	pData->tempData->auth->plainTextPassword = password;
}

void AuthController::onRegistrationSubmit(IPlayer& player)
{
	auto playerExt = Player::getPlayerExt(player);
	auto db = this->_coreManager.lock()->getDBConnection();

	pqxx::work txn(*db);

	auto pData = this->_coreManager.lock()->getPlayerData(player);
	try
	{
		pqxx::result res = txn.exec_params(
			SQLQueryManager::Get()
				->getQueryByName(Utils::SQL::Queries::CREATE_PLAYER)
				.value(),
			player.getName().to_string(), pData->passwordHash, pData->language,
			pData->email, 1, playerExt->getIP());
		txn.commit();
	}
	catch (const std::exception& e)
	{
		txn.abort();
		spdlog::error(std::format("Error occurred when trying to create new "
								  "user entry in DB. Error: {}",
			e.what()));
		playerExt->sendErrorMessage(
			_("Something went wrong when trying to create user!", player));
		playerExt->delayedKick();
		return;
	}
	this->_coreManager.lock()->refreshPlayerData(player);
	this->showRegistrationInfoDialog(player);
}

void AuthController::showLanguageDialog(IPlayer& player)
{
	std::vector<std::string> items(
		std::begin(Localization::LANGUAGES), std::end(Localization::LANGUAGES));
	auto dialog = std::shared_ptr<ListDialog>(
		new ListDialog(fmt::sprintf(DIALOG_HEADER_TITLE, "Select language"),
			items, "Select", "Quit"));
	this->dialogManager->showDialog(player, dialog,
		[&](DialogResult result)
		{
			switch (result.response())
			{
			case DialogResponse_Left:
			{
				auto pData = this->_coreManager.lock()->getPlayerData(player);
				pData->language
					= Localization::LANGUAGE_CODE_NAMES.at(result.listItem());
				showRegistrationDialog(player);
				break;
			}
			case DialogResponse_Right:
			{
				Player::getPlayerExt(player)->delayedKick();
				break;
			}
			}
		});
}

void AuthController::showEmailDialog(IPlayer& player)
{
	auto dialog = std::shared_ptr<InputDialog>(new InputDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Enter your email", player)),
		fmt::sprintf(
			_("#LIGHT_GRAY#Please enter your #WHITE#Email "
			  "address#LIGHT_GRAY#.\n\n"
			  "Use a correct email address as it can be used for password "
			  "recovery\n"
			  "to restore your account incase you forget your password\n"
			  "You can skip entering email, but account features will be "
			  "limited! Use #WHITE#/verify#LIGHT_GRAY# later",
				player),
			player.getName().to_string()),
		false, _("Enter", player), _("Skip", player)));
	this->dialogManager->showDialog(player, dialog,
		[&](DialogResult result)
		{
			switch (result.response())
			{
			case DialogResponse_Left:
			{
				onEmailSubmit(player, result.inputText());
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

void AuthController::onEmailSubmit(IPlayer& player, const std::string& email)
{
	std::smatch m;
	if (!std::regex_match(email, m, EMAIL_REGEX))
	{
		this->showEmailDialog(player);
		Player::getPlayerExt(player)->sendErrorMessage(
			_("Invalid email!", player));
		return;
	}

	auto data = this->_coreManager.lock()->getPlayerData(player);
	data->email = email;
	onRegistrationSubmit(player);
}

void AuthController::showRegistrationInfoDialog(IPlayer& player)
{
	auto pData = Player::getPlayerData(player);
	auto dialog = std::shared_ptr<MessageDialog>(new MessageDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Registration info", player)),
		fmt::sprintf(_("#DRY_HIGHLIGHTER_GREEN#Your account has been "
					   "successfully registered and logged in.\n\n"
					   "#MILLION_GREY#Username: #WHITE#%s\n"
					   "#MILLION_GREY#Account ID: #WHITE#%d\n"
					   "#MILLION_GREY#Password: #WHITE#%s\n"
					   "#MILLION_GREY#Registration date: #WHITE#%s\n\n"
					   "#MILLION_GREY#Use '#RED#F8#MILLION_GREY#' to take "
					   "screenshot and save the password.",
						 player),
			pData->name, pData->userId,
			pData->tempData->auth->plainTextPassword,
			std::format("{:%Y-%m-%d}", pData->registrationDate)),
		_("OK", player), ""));
	this->_coreManager.lock()->getDialogManager()->showDialog(player, dialog,
		[&](DialogResult result)
		{
			Player::getPlayerExt(player)->sendInfoMessage(
				_("You have successfully registered!", player));
			this->_coreManager.lock()->onPlayerLoggedIn(player);
		});
}

void AuthController::interpolatePlayerCamera(IPlayer& player)
{
	player.interpolateCameraPosition(Vector3(1093.0, -2036.0, 90.0),
		Vector3(21.1088, -1806.9847, 79.4125), 15000, PlayerCameraCutType_Move);
	player.interpolateCameraLookAt(Vector3(384.0, -1557.0, 20.0),
		Vector3(455.1533, -1868.1967, 31.9840), 15000,
		PlayerCameraCutType_Move);
}
}