#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <Awesomium/WebString.h>

using namespace std;
using namespace Awesomium;

namespace QuestNavigator {

	// Утилиты для преобразования строк

	// UTF-16 wstring -> UTF-16 WebString
	WebString WideToWebString(wstring str);
	// UTF-16 wstring -> UTF-8 string
	string narrow(wstring str);
	// UTF-8 string -> UTF-16 wstring
	wstring widen(string str);

	// Утилиты для работы с файловой системой

	// Путь к папке плеера
	string getPlayerSystemPath();
	// Получаем URL из полного пути к файлу
	string getUriFromFilePath(string filePath);
	// URL к содержимому
	string getContentUrl();

	// Прочее

	// Устанавливаем конфигурацию плеера при старте
	void initOptions();
}





#endif
