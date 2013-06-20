#include "configuration.h"

namespace QuestNavigator {

	ConfigValue::ConfigValue()
	{
		_str = "";
	}
	ConfigValue::ConfigValue(string str)
	{
		_str = str;
	}
	string ConfigValue::getString()
	{
		return _str;
	}
	void ConfigValue::setString(string str)
	{
		_str = str;
	}

	ConfigValue Configuration::_paramList[ecpLast];

	string Configuration::getString(eConfigParam param)
	{
		return _paramList[param].getString();
	}
	void Configuration::setString(eConfigParam param, string value)
	{
		_paramList[param].setString(value);
	}

}