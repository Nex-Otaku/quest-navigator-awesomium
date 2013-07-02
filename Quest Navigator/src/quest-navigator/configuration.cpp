#include "configuration.h"
#include "utils.h"

namespace QuestNavigator {

	ConfigValue::ConfigValue()
	{
		_type = ecvInvalid;
		_str = "";
		_num = 0;
		_flag = false;
	}
	ConfigValue::ConfigValue(string str)
	{
		_type = ecvString;
		_str = str;
		_num = 0;
		_flag = false;
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
		_flag = false;
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
	ConfigValue::ConfigValue(bool flag)
	{
		_type = ecvBool;
		_str = "";
		_num = 0;
		_flag = flag;
	}
	bool ConfigValue::getBool()
	{
		if (_type != ecvBool) {
			showError("Неправильно указан тип параметра");
		}
		return _flag;
	}
	void ConfigValue::setBool(bool flag)
	{
		_type = ecvBool;
		_flag = flag;
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
	bool Configuration::getBool(eConfigParam param)
	{
		return _paramList[param].getBool();
	}
	void Configuration::setBool(eConfigParam param, bool value)
	{
		_paramList[param].setBool(value);
	}


}