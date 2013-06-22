#include "utils.h"
#include "configuration.h"
#ifdef _WIN32
#include <shlwapi.h>
#endif

#include <Awesomium/STLHelpers.h>
#include <vector>
#include <algorithm>

using namespace Awesomium;

/*

Везде используем string, в нём хранятся юникодовые строки в UTF-8.

В исходном файле строки с русским текстом, заданные явно, тоже хранятся в UTF-8.
Visual Studio 2012 умеет определять кодировку исходного файла, с этим проблем быть не должно.

При передаче в движок Awesomium преобразовываем в WebString функцией ToWebString.

При получении из WinApi:

1. Если возвращается указатель на PTSTR или LPWSTR 
(в нашей конфигурации одно и тоже, т.к. включен Юникод)
то просто сохраняем в wstring.
Потом конвертируем в string в UTF-8 функцией narrow.
Пример:
wstring wCmd = GetCommandLine();
string cmd = narrow(wCmd);

2. Если возвращается в буфер из TCHAR, 
то также просто сохраняем в wstring простым присваиванием указателя на начало буфера.
Потом конвертируем в string в UTF-8 функцией narrow.
Пример:
	TCHAR szString[1024];
	PTSTR szUrl = szString;
	...
	UrlCreateFromPath(FileName, szUrl, &sUrl, NULL); 
	wstring wUrl = szUrl;
	string url = narrow(wUrl);

При получении строк из интерпретатора пользуемся функцией fromQsp.

*/

namespace QuestNavigator {

// UTF-16 wstring -> UTF-16 WebString
WebString WideToWebString(wstring str)
{
	WebString cmd((wchar16*)str.data());
	return cmd;
}

// UTF-16 wstring -> UTF-8 string
string narrow(wstring str)
{
	WebString webStr = WideToWebString(str);
	return ToString(webStr);
}
string fromQsp(QSP_CHAR* str)
{
	wstring wStr;
	if (str != NULL)
		wStr = str;
	return narrow(wStr);
}
string fromQsp(const QSP_CHAR* str)
{
	return fromQsp((QSP_CHAR*) str);
}

// UTF-8 string -> UTF-16 wstring
wstring widen(string str)
{
	WebString webStr = ToWebString(str);
	wstring wStr((wchar_t*)webStr.data());
	return wStr;
}

// Получаем URL из полного пути к файлу
string getUrlFromFilePath(string filePath)
{
	TCHAR szString[1024];
	PTSTR szUrl = szString;
	wstring wFilePath = widen(filePath);
	PTSTR FileName = (PTSTR)wFilePath.c_str();
	DWORD sUrl = 1024;
	HRESULT res = UrlCreateFromPath(FileName, szUrl, &sUrl, NULL);
	if (res != S_OK) {
		showError("Не удалось преобразовать в URL путь вида: [" + filePath + "]");
		return "";
	}
	wstring wUrl = szUrl;
	string url = narrow(wUrl);
	return url;
}

// URL к содержимому
string getContentUrl()
{
	// Если путь к содержимому не задан явно,
	// указываем путь по умолчанию для пака "assets.pak".
	// Файл "assets.pak" ищется в рабочей папке.
	string contentPath = Configuration::getString(ecpSkinFile);
	string contentUrl = (contentPath.length() == 0) ?
		"asset://webui/" + backSlashToSlash(DEFAULT_CONTENT_REL_PATH + "\\" + DEFAULT_SKIN_FILE) : 
		getUrlFromFilePath(contentPath);
	return contentUrl;
}

// Проверяем файл на существование и читаемость
bool fileExists(string path)
{
	DWORD dwAttrib = GetFileAttributes(widen(path).c_str());
	return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && 
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

// Проверяем папку на существование и читаемость
bool dirExists(string path)
{
	DWORD dwAttrib = GetFileAttributes(widen(path).c_str());
	return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && 
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

// Получаем путь к рабочей папке
string getCurrentDir()
{
	TCHAR buffer[MAX_PATH];
	PTSTR szDir = buffer;

	DWORD res = GetCurrentDirectory((DWORD)MAX_PATH, szDir);
	if (res == 0) {
		showError("Не могу прочесть текущую директорию");
		return "";
	}
	if (res > MAX_PATH) {
		showError("Путь к текущей директории не помещается в буфер");
		return "";
	}

	wstring wDir = szDir;
	string dir = narrow(wDir);
	return dir;
}

// Загрузка конфигурации плеера
void initOptions()
{
	// Разбираем параметры запуска
	int argCount = 0;
	LPWSTR* szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
	if (szArgList == NULL) {
		showError("Не могу прочесть аргументы командной строки");
		return;
	}
	vector<string> params;
	for (int i = 0; i < argCount; i++) {
		params.push_back(trim(narrow(szArgList[i])));
	}
	LocalFree(szArgList);
	bool contentPathSet = false;
	string contentPath = "";
	for (int i = 0; i < argCount; i++) {
		string param = params[i];
		// Если мы вызываем программу из командной строки,
		// то первым параметром идёт имя exe-файла, игнорируем его.
		if (endsWith(param, ".exe"))
			continue;
		bool isOption = startsWith(param, "-");
		if (!contentPathSet && !isOption) {
			// Нам передали путь к файлу в командной строке.
			// Просто сохраняем его, проверять будем позже,
			// когда убедимся что строка параметров разобрана правильно.
			contentPath = param;
			contentPathSet = true;
		} else if (isOption) {
			// Пока что ни одна опция не работает
			showError("Неизвестная опция: [" + param + "]");
			return;
		} else {
			showError("Неизвестный параметр: [" + param + "]\n" +
				"Возможно, путь к файлу содержит пробелы и вы забыли взять его в кавычки.");
			return;
		}
	}
	// Всё разобрано правильно
	if (contentPathSet) {
		// Нам передали путь к игре.
		// Всего три варианта:
		// 1. Это путь к архиву .qn;
		// 2. Это путь к файлу .qsp;
		// 3. Это путь к папке игры.
		// Проверяем путь на существование и читаемость.
		string contentDir = getCurrentDir();
		string skinFile = "";
		string gameFile = "";
		string configFile = "";

		bool bValidDirectory = dirExists(contentPath);
		bool bValidFile = fileExists(contentPath);
		if (bValidFile) {
			// Проверяем расширение файла
			bool bExtQn = endsWith(contentPath, ".qn");
			bool bExtQsp = endsWith(contentPath, ".qsp");
			if (!bExtQn && !bExtQsp) {
				showError("Неизвестный формат файла!\nПоддерживаемые форматы: qn, qsp");
				return;
			}
			if (bExtQn) {
				// STUB
				showError("Загрузка архива qn ещё не реализована.");
				return;
			} else {
				// STUB
				showError("Загрузка файла qsp ещё не реализована.");
				return;
			}
			// STUB
		} else if (bValidDirectory) {
			// Сохраняем путь к папке игры
			contentDir = contentPath;
			// Вычисляем пути к необходимым файлам
			skinFile = contentDir + "\\" + DEFAULT_SKIN_FILE;
			// STUB
			// Сделать проверку на количество QSP-файлов, 
			// если файлов более одного, то выводить ошибку.
			// Сделать поиск QSP-файлов.
			gameFile = contentDir + "\\" + "game.qsp";
			configFile = contentDir + "\\" + "config.xml";
		} else {
			DWORD error = GetLastError();
			if (error == ERROR_FILE_NOT_FOUND) {
				showError("Не найден файл: [" + contentPath + "]");
			} else if (error == ERROR_PATH_NOT_FOUND) {
				showError("Не найден путь: [" + contentPath + "]");
			} else {
				showError("Не удалось прочесть файл: [" + contentPath + "]");
			}
			return;
		}

		// STUB
		// Сделать проверку всех файлов на читаемость
		Configuration::setString(ecpContentDir, contentDir);
		Configuration::setString(ecpSkinFile, skinFile);
		Configuration::setString(ecpGameFile, gameFile);
		Configuration::setString(ecpConfigFile, configFile);

		//// В параметре конфигурации ContentPath пока что будет храниться
		//// полный путь к html-файлу проекта.
		//string fullPath = startsWith(contentPath.substr(1), ":\\") ?
		//	contentPath : getCurrentDir() + "\\" + contentPath;
		//Configuration::setContentPath(fullPath);
	}
}

// Показываем системный диалог MessageBox
void showMessage(string msg, string title)
{
	wstring wMsg = widen(msg);
	wstring wTitle = widen(title);
	MessageBox(0, wMsg.c_str(), wTitle.c_str(), MB_OK);
}
void showError(string msg)
{
	showMessage(msg, "Ошибка");
}

// Отсекаем пробелы в начале и конце строки
string trim(string text)
{
	size_t pos = text.find_first_not_of(' ');
	if (pos == string::npos)
		return "";
	string trimmed = text.substr(pos);
	pos = text.find_last_not_of(' ');
	if (pos == string::npos)
		return "";
	trimmed = text.substr(0, pos + 1);
	return trimmed;
}

// Начинается ли строка с последовательности символов
bool startsWith(string source, string prefix)
{
	return (source.length() > 0) && 
		(prefix.length() > 0) &&
		(source.find(prefix) == 0);
}

// Оканчивается ли строка последовательностью символов
bool endsWith(string source, string suffix)
{
	return (source.length() > 0) && 
		(suffix.length() > 0) &&
		(source.rfind(suffix) == source.length() - suffix.length());
}

// Преобразовываем обратные косые черты в прямые ("\" -> "/")
string backSlashToSlash(string text)
{
	return replaceAll(text, '\\', '/');
}

// Заменяем все вхождения символа в строке
string replaceAll(string source, char pattern, char replacement)
{
	replace(source.begin(), source.end(), pattern, replacement);
	return source;
}
// Заменяем все вхождения подстроки в строке
void replaceAll(string &s, const string &search, const string &replace)
{
    for (size_t pos = 0; ; pos += replace.length()) {
        // Locate the substring to replace
        pos = s.find(search, pos);
        if (pos == string::npos)
			break;
        // Replace by erasing and inserting
        s.erase(pos, search.length());
        s.insert(pos, replace);
    }
}

}