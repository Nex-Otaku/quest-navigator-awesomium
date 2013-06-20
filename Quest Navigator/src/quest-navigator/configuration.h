#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <string>

using namespace std;

namespace QuestNavigator {

	// В этом классе хранятся всяческие параметры плеера
	enum eConfigParam
	{
		// Строки
		ecpContentDir = 0,
		ecpSkinFile,
		ecpGameFile,
		ecpConfigFile,

		ecpLast
	};

	class ConfigValue {
	private:
		string _str;
	public:
		ConfigValue();
		ConfigValue(string str);
		string getString();
		void setString(string str);
	};

	class Configuration {
		// Параметр может быть разного типа
	private:
		// Хранилище параметров
		static ConfigValue _paramList[ecpLast];

	public:
		static string getString(eConfigParam param);
		static void setString(eConfigParam param, string value);

	};
}

#endif
