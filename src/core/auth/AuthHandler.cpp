#include "AuthHandler.hpp"
#include "../CoreManager.hpp"

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
	auto db = this->coreManager.lock()->getDBConnection();
	string stmtName("check_user_registered");
	db->prepare(stmtName,
		"SELECT id, \
					name, \
					users.password_hash, \
					\"language\", \
					email, \
					last_skin_id, \
					last_ip, \
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
					WHERE name=$1");
	pqxx::work txn(*db);
	pqxx::result res = txn.exec_prepared(stmtName, player.getName().to_string());

	if (res.size() == 0)
	{
		this->showRegistrationDialog(player);
		return;
	}

	spdlog::info("Found player data for " + player.getName().to_string() + " in DB");

	auto row = res[0];
	auto wPlayer = make_shared<Player>(std::unique_ptr<PlayerModel>(new PlayerModel(res[0])), player);

	this->coreManager.lock()->addRegisteredPlayer(wPlayer);
}

void AuthHandler::showRegistrationDialog(IPlayer& player)
{
	this->coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle_PASSWORD,
		"Registration",
		"Enter the password:",
		"OK",
		"Quit",
		[&](DialogResponse resp, int listItem, StringView inputText)
		{
			switch (resp)
			{
			case DialogResponse_Left:
			{
				player.sendClientMessage(Colour::White(), "You have been registered!");
				player.spawn();
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
