#pragma once

#include <string>
#include <unordered_map>
#include <variant>

typedef std::variant<std::string, bool, int, double> SettingValue;

namespace Core
{
struct ISettingsStorage
{
	virtual ~ISettingsStorage() = default;
	virtual void set(std::string key, SettingValue value) = 0;
	virtual SettingValue& get(std::string key) = 0;
};

class HashMapStorage : public ISettingsStorage
{
	std::unordered_map<std::string, SettingValue> storage;

	virtual void set(std::string key, SettingValue value) override
	{
		storage[key] = value;
	}

	virtual SettingValue& get(std::string key) override { return storage[key]; }

	std::unordered_map<std::string, SettingValue>& getStorage()
	{
		return storage;
	}
};
} // namespace Core