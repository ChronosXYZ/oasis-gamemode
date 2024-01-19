#pragma once

#include "PlayerModel.hpp"
#include "player.hpp"
#include <component.hpp>
#include <memory>
#include <sdk.hpp>

namespace Core
{
class OasisPlayerDataExt : public IExtension
{
private:
	std::shared_ptr<PlayerModel> _playerData = nullptr;

public:
	PROVIDE_EXT_UID(0xBE727855C7D51E32)
	void setPlayerData(std::shared_ptr<PlayerModel> data)
	{
		_playerData = data;
	}

	std::shared_ptr<PlayerModel> getPlayerData()
	{
		return this->_playerData;
	}

	void freeExtension() override
	{
		_playerData.reset();
	}

	void reset() override
	{
		_playerData.reset();
	}
};
}