#include "configuration.h"
#include "utils.h"

namespace QuestNavigator {

	ConfigValue::ConfigValue()
	{
		_type = ecvInvalid;
		_str = "";
		_num = 0;
	}
	ConfigValue::ConfigValue(string str)
	{
		_type = ecvString;
		_str = str;
		_num = 0;
	}
	string ConfigValue::getString()
	{
		if (_type != ecvString) {
			showError("Неправильно указан тип параметра");
		}
		return _str;
	}
	void ConfigValue::setString(string str)
	{
		_type = ecvString;
		_str = str;
	}
	ConfigValue::ConfigValue(int num)
	{
		_type = ecvInt;
		_str = "";
		_num = num;
	}
	int ConfigValue::getInt()
	{
		if (_type != ecvInt) {
			showError("Неправильно указан тип параметра");
		}
		return _num;
	}
	void ConfigValue::setInt(int num)
	{
		_type = ecvInt;
		_num = num;
	}
	eConfigValueType ConfigValue::getType()
	{
		return _type;
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