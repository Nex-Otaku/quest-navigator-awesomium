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
		string result = "";
		lockConfigData();
		result = _paramList[param].getString();
		unlockConfigData();
		return result;
	}
	void Configuration::setString(eConfigParam param, string value)
	{
		lockConfigData();
		_paramList[param].setString(value);
		unlockConfigData();
	}
	int Configuration::getInt(eConfigParam param)
	{
		int result = 0;
		lockConfigData();
		result = _paramList[param].getInt();
		unlockConfigData();
		return result;
	}
	void Configuration::setInt(eConfigParam param, int value)
	{
		lockConfigData();
		_paramList[param].setInt(value);
		unlockConfigData();
	}
	bool Configuration::getBool(eConfigParam param)
	{
		bool result = false;
		lockConfigData();
		result = _paramList[param].getBool();
		unlockConfigData();
		return result;
	}
	void Configuration::setBool(eConfigParam param, bool value)
	{
		lockConfigData();
		_paramList[param].setBool(value);
		unlockConfigData();
	}

	bool Configuration::init()
	{
		// Инициализируем структуру критической секции
		try {
			InitializeCriticalSection(&csConfigData);
		} catch (...) {
			showError("Не удалось проинициализировать структуру критической секции.");
			return false;
		}
		initedCritical = true;
		return true;
	}

	void Configuration::deinit()
	{
		// Высвобождаем структуру критической секции
		if (initedCritical) {
			DeleteCriticalSection(&csConfigData);
			initedCritical = false;
		}
	}

	// Синхронизация потоков
	bool Configuration::initedCritical = false;
	// Структура для критических секций
	CRITICAL_SECTION Configuration::csConfigData;

	// Входим в критическую секцию
	void Configuration::lockConfigData()
	{
		try {
			EnterCriticalSection(&csConfigData);
		} catch (...) {
			showError("Не удалось войти в критическую секцию.");
			exit(0);
		}
	}

	// Выходим из критической секции
	void Configuration::unlockConfigData()
	{
		LeaveCriticalSection(&csConfigData);
	}

}