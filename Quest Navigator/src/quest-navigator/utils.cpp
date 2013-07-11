#include "utils.h"
#include "configuration.h"
#ifdef _WIN32
#include <shlwapi.h>
#include <Shlobj.h>
#include <winsparkle.h>
#endif

#include <Awesomium/STLHelpers.h>
#include <vector>
#include <algorithm>
#include "../deps/md5/md5.h"

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
		wstring wStr = L"";
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
		wstring wStr = wstring((wchar_t*)webStr.data());
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
			"asset://webui/" + backslashToSlash(DEFAULT_CONTENT_REL_PATH + "\\" + DEFAULT_SKIN_FILE) : 
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

	// Меняем слэши в пути к файлу в зависимости от системы
	string getRightPath(string path)
	{
		// Вызывая эту функцию для обработки пути,
		// мы будем уверены что слэши всегда направлены в нужную сторону.
		string result = path;
#ifdef WIN32
		result = slashToBackslash(path);
#endif
		return result;
	}

	// Загружаем файл в память
	bool loadFileToBuffer(string path, void** bufferPtr, int* bufferLength)
	{
		// Открываем файл для чтения
		wstring wPath = widen(path);
		HANDLE hFile = CreateFile(wPath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			return false;
		// Узнаём размер файла
		DWORD dwFileSize = GetFileSize(hFile, NULL);
		if (dwFileSize == INVALID_FILE_SIZE) {
			CloseHandle(hFile);
			return false;
		}
		// Выделяем блок памяти
		char* pFileChunk = NULL;
		// Может не хватить памяти (если файл слишком большой)
		try {
			pFileChunk = new char[dwFileSize];
		} catch (...) {
			CloseHandle(hFile);
			return false;
		}
		// Читаем файл в память
		DWORD dwBytesRead = 0;
		BOOL res = ReadFile(hFile, pFileChunk, dwFileSize, &dwBytesRead, NULL);
		if ((res == FALSE) || (dwBytesRead != dwFileSize)) {
			CloseHandle(hFile);
			delete pFileChunk;
			return false;
		}
		// Закрываем файл
		res = CloseHandle(hFile);
		if (res == 0) {
			delete pFileChunk;
			return false;
		}
		// Возвращаем результат
		*bufferPtr = pFileChunk;
		*bufferLength = (int)dwFileSize;
		// Не забываем освободить память вызовом "delete" после использования!
		return true;
	}

	// Создаём папки
	bool buildDirectoryPath(string path)
	{
		wstring wPath = widen(path);
		int res = SHCreateDirectoryEx(NULL, wPath.c_str(), NULL);
		return res == ERROR_SUCCESS;
	}


	// Загрузка конфигурации плеера
	bool initOptions()
	{
		// Устанавливаем параметры по умолчанию
		Configuration::setBool(ecpSoundCacheEnabled, false);
		Configuration::setInt(ecpSaveSlotMax, 5);

		// Разбираем параметры запуска
		int argCount = 0;
		LPWSTR* szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
		if (szArgList == NULL) {
			showError("Не могу прочесть аргументы командной строки");
			return false;
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
				// Разбираем опции
				if (param == "-enable-sound-cache") {
					Configuration::setBool(ecpSoundCacheEnabled, true);
				} else {
					showError("Неизвестная опция: [" + param + "]");
					return false;
				}
			} else {
				showError("Неизвестный параметр: [" + param + "]\n" +
					"Возможно, путь к файлу содержит пробелы и вы забыли взять его в кавычки.");
				return false;
			}
		}
		// Всё разобрано правильно
		string contentDir = "";
		string skinFile = "";
		string gameFile = "";
		string configFile = "";
		string saveDir = "";
		string windowTitle = QN_APP_NAME + " " + QN_VERSION;
		if (contentPathSet) {
			// Нам передали путь к игре.
			// Всего три варианта:
			// 1. Это путь к архиву .qn;
			// 2. Это путь к файлу .qsp;
			// 3. Это путь к папке игры.
			// Проверяем путь на существование и читаемость.
			contentDir = getCurrentDir();

			bool bValidDirectory = dirExists(contentPath);
			bool bValidFile = fileExists(contentPath);
			if (bValidFile) {
				// Проверяем расширение файла
				bool bExtQn = endsWith(contentPath, ".qn");
				bool bExtQsp = endsWith(contentPath, ".qsp");
				if (!bExtQn && !bExtQsp) {
					showError("Неизвестный формат файла!\nПоддерживаемые форматы: qn, qsp");
					return false;
				}
				if (bExtQn) {
					// STUB
					showError("Загрузка архива qn ещё не реализована.");
					return false;
				} else {
					// STUB
					showError("Загрузка файла qsp ещё не реализована.");
					return false;
				}
				// STUB
			} else if (bValidDirectory) {
				// Сохраняем путь к папке игры
				contentDir = contentPath;
				// Вычисляем пути к необходимым файлам
				skinFile = getRightPath(contentDir + "\\" + DEFAULT_SKIN_FILE);
				// Ищем все QSP-файлы в корне указанной папки
				vector<string> gameFileList;
				if (!getFilesList(contentDir, "*.qsp", gameFileList))
					return false;
				int count = (int)gameFileList.size();
				if (count == 0) {
					showError("Не найден файл игры в папке " + contentDir);
					return false;
				} else if (count > 1) {
					showError("В корневой папке игры должен находиться только один файл с расширением .qsp");
					return false;
				}
				gameFile = getRightPath(contentDir + "\\" + gameFileList[0]);
				configFile = getRightPath(contentDir + "\\" + "config.xml");
			} else {
				DWORD error = GetLastError();
				if (error == ERROR_FILE_NOT_FOUND) {
					showError("Не найден файл: [" + contentPath + "]");
				} else if (error == ERROR_PATH_NOT_FOUND) {
					showError("Не найден путь: [" + contentPath + "]");
				} else {
					showError("Не удалось прочесть файл: [" + contentPath + "]");
				}
				return false;
			}

			// STUB
			// Сделать проверку всех файлов на читаемость

			saveDir = "";
			// Путь к пользовательской папке "Мои документы"
			WCHAR wszPath[MAX_PATH];
			HRESULT hr = SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, wszPath);
			if (hr != S_OK) {
				showError("Не удалось получить путь к папке \"Мои документы\".");
				return false;
			}
			saveDir = getRightPath(narrow(wszPath) + "\\" + DEFAULT_SAVE_REL_PATH + "\\" + md5(contentDir));
		}
		// Сохраняем конфигурацию
		Configuration::setString(ecpContentDir, contentDir);
		Configuration::setString(ecpSkinFile, skinFile);
		Configuration::setString(ecpGameFile, gameFile);
		Configuration::setString(ecpConfigFile, configFile);
		Configuration::setString(ecpSaveDir, saveDir);
		Configuration::setString(ecpWindowTitle, windowTitle);
		return true;
	}

	// Возвращаем список файлов
	bool getFilesList(string directory, string mask, vector<string>& list)
	{
		vector<string> result;

		WIN32_FIND_DATA ffd;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		DWORD dwError = 0;

		wstring wDir = widen(getRightPath(directory + "\\" + mask));
		if (wDir.size() + 1 > MAX_PATH) {
			showError("Слишком длинный путь");
			return false;
		}
		LPCWSTR szDir = wDir.c_str();

		// Find the first file in the directory.

		hFind = FindFirstFile(szDir, &ffd);

		if (INVALID_HANDLE_VALUE == hFind) 
		{
			dwError = GetLastError();
			if (dwError == ERROR_FILE_NOT_FOUND) {
				list = result;
				return true;
			} else {
				showError("Не удалось осуществить поиск файла в папке " + directory + " по заданной маске " + mask);
				return false;
			}
		} 

		// List all the files in the directory with some info about them.

		do
		{
			if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				result.push_back(narrow(ffd.cFileName));
			}
		}
		while (FindNextFile(hFind, &ffd) != 0);

		dwError = GetLastError();
		if (dwError != ERROR_NO_MORE_FILES) 
		{
			showError("Не удалось осуществить повторный поиск файла в папке " + directory + " по заданной маске " + mask);
			return false;
		}

		BOOL res = FindClose(hFind);
		if (res == 0) {
			showError("Не удалось высвободить дескриптор поиска");
			return false;
		}

		list = result;
		return true;
	}

	// Проверяем наличие апдейта при старте
	void checkUpdate()
	{
		// Initialize WinSparkle as soon as the app itself is initialized, right
		// before entering the event loop:
		win_sparkle_set_appcast_url(QN_WINDOWS_UPDATE_FEED.c_str());
		win_sparkle_set_app_details(widen(QN_COMPANY_NAME).c_str(), 
			widen(QN_APP_NAME).c_str(),
			widen(QN_VERSION).c_str());

		win_sparkle_init();
	}
	// Завершаем работу апдейтера по выходу из приложения
	void finishUpdate()
	{
		win_sparkle_cleanup();
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
	string backslashToSlash(string text)
	{
		return replaceAll(text, '\\', '/');
	}

	// Преобразовываем прямые косые черты в обратные ("/" -> "\")
	string slashToBackslash(string text)
	{
		return replaceAll(text, '/', '\\');
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
	// Переводим все символы в верхний регистр
	string toUpper(string str)
	{
		string strUpper = str;
		transform(str.begin(), str.end(), strUpper.begin(), ::toupper);
		return strUpper;
	}
	// Переводим все символы в нижний регистр
	string toLower(string str)
	{
		string strLower = str;
		transform(str.begin(), str.end(), strLower.begin(), ::tolower);
		return strLower;
	}

	// Преобразование URL ссылки вида <a href="EXEC:...">...</a> в код QSP
	string unescapeHtml(string text)
	{
		if (text == "")
			return "";
		replaceAll(text, "&quot;", "\"");
		replaceAll(text, "&#39;", "'");
		replaceAll(text, "&lt;", "<");
		replaceAll(text, "&gt;", ">");
		replaceAll(text, "&amp;", "&");
		return text;
	}

	// Обратное преобразование URL в "сырой" вид
	string decodeUrl(string url)
	{
		string decoded = "";
		char ch;
		int i, ii;
		for (i = 0; i < (int)url.length(); i++) {
			if (int(url[i]) == 37) {
				sscanf(url.substr(i + 1, 2).c_str(), "%x", &ii);
				ch = static_cast<char>(ii);
				decoded += ch;
				i = i + 2;
			} else {
				decoded += url[i];
			}
		}
		return decoded;
	}

}