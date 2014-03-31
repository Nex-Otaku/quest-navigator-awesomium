#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <string>
#include <Windows.h>
#include "../deps/tinyxml/tinyxml.h"

using namespace std;

namespace QuestNavigator {

	// В этом классе хранятся всяческие параметры плеера
	enum eConfigParam
	{
		// Основные параметры
		ecpContentDir = 0,
		ecpSkinFilePath,
		ecpGameFilePath,
		ecpGameHash,
		ecpCacheDir,
		ecpGameFileName,
		ecpConfigFilePath,
		ecpSaveDir,
		ecpAppDataDir,
		ecpSoundCacheEnabled,
		ecpSaveSlotMax,
		ecpWindowTitle,
		ecpIsFullscreen,
		ecpDefaultSkinName,
		ecpLimitSingleInstance,
		ecpRunningDefaultGame,

		// Настройки игры
		ecpGameWidth,
		ecpGameHeight,
		ecpGameMinWidth,
		ecpGameMinHeight,
		ecpGameMaxWidth,
		ecpGameMaxHeight,
		ecpGameTitle,
		ecpGameResizeable,
		ecpGameFullscreenAvailable,
		ecpGameStartFullscreen,
		ecpSkinName,
		ecpGameIsStandalone,

		ecpLast
	};

	enum eConfigValueType
	{
		ecvInt = 0,
		ecvString,
		ecvBool,
		ecvInvalid
	};

	class ConfigValue {
	private:
		eConfigValueType _type;
		string _str;
		int _num;
		bool _flag;

	public:
		ConfigValue();
		ConfigValue(string str);
		string getString();
		void setString(string str);
		ConfigValue(int num);
		int getInt();
		void setInt(int num);
		ConfigValue(bool flag);
		bool getBool();
		void setBool(bool flag);
		eConfigValueType getType();
	};

	class Configuration {
		// Параметр может быть разного типа
	private:
		// Хранилище параметров
		static ConfigValue _paramList[ecpLast];

		// Синхронизация потоков
		static bool initedCritical;
		// Структура для критических секций
		static CRITICAL_SECTION csConfigData;
		// Входим в критическую секцию
		static void lockConfigData();
		// Выходим из критической секции
		static void unlockConfigData();

	public:
		static string getString(eConfigParam param);
		static void setString(eConfigParam param, string value);
		static int getInt(eConfigParam param);
		static void setInt(eConfigParam param, int value);
		static bool getBool(eConfigParam param);
		static void setBool(eConfigParam param, bool value);
		static eConfigValueType getType(eConfigParam param);

		// Обработка настроек из XML-файла
		static bool loadXmlAttrib(TiXmlElement* pElem, string name, eConfigParam param);

		static bool init();
		static void deinit();
	};
}

#endif
