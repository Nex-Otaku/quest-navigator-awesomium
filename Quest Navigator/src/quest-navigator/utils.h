#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <Awesomium/WebString.h>

using namespace std;
using namespace Awesomium;

namespace QuestNavigator {

	// Константы

	static const string DEFAULT_CONTENT_REL_PATH = "standalone_content\\gameAwesomium.html";

	// Утилиты для преобразования строк

	// UTF-16 wstring -> UTF-16 WebString
	WebString WideToWebString(wstring str);
	// UTF-16 wstring -> UTF-8 string
	string narrow(wstring str);
	// UTF-8 string -> UTF-16 wstring
	wstring widen(string str);

	// Утилиты для обработки строк

	// Отсекаем пробелы в начале и конце строки
	string trim(string text);
	// Начинается ли строка с последовательности символов
	bool startsWith(string source, string prefix);
	// Оканчивается ли строка последовательностью символов
	bool endsWith(string source, string suffix);
	// Преобразовываем обратные косые черты в прямые ("\" -> "/")
	string backSlashToSlash(string text);
	// Заменяем все вхождения подстроки в строке
	string replaceAll(string source, char pattern, char replacement);

	// Утилиты для работы с файловой системой

	// Получаем URL из полного пути к файлу
	string getUriFromFilePath(string filePath);
	// URL к содержимому
	string getContentUrl();
	// Проверяем файл на существование и читаемость
	bool fileExists(string path);
	// Получаем путь к рабочей папке
	string getCurrentDir();

	// Прочее

	// Устанавливаем конфигурацию плеера при старте
	void initOptions();

	// Показываем системный диалог MessageBox
	void showMessage(string msg, string title);
	void showError(string msg);

}





#endif
