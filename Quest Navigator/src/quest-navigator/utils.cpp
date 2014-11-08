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

#define LOAD_XML_ATTRIB(name, configLine) if (valid) { valid = Configuration::loadXmlAttrib(root, name, configLine); }

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
		string contentPath = Configuration::getString(ecpSkinFilePath);
		return getUrlFromFilePath(contentPath);
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

	// Получаем путь к папке плеера
	string getPlayerDir()
	{
		TCHAR buffer[MAX_PATH];
		PTSTR szDir = buffer;

		DWORD res = GetModuleFileName(NULL, szDir, (DWORD)MAX_PATH);
		if (res == 0) {
			showError("Не могу прочесть путь к плееру");
			return "";
		}
		if (res == MAX_PATH) {
			showError("Путь к плееру не помещается в буфер");
			return "";
		}

		// Удаляем имя исполняемого файла, оставляем только путь к нему, без слэша в конце.
		res = PathRemoveFileSpec(szDir);
		if (res == 0) {
			showError("Ошибка при обработке пути к плееру");
			return "";
		}

		wstring wDir = szDir;
		string dir = narrow(wDir);
		return dir;
	}

	// Преобразовываем путь к файлу сохранения.
	string getRealSaveFile(string file)
	{
		// "D:\MySaveDirForCoolGame"
		string saveDir = Configuration::getString(ecpSaveDir);
		// "D:\CoolGame\saves\save1.sav"
		string originalSaveFilePath = getRightPath(file);
		// "D:\CoolGame\game.qsp"
		string gameFilePath = getRightPath(Configuration::getString(ecpGameFilePath));
		// "game.qsp"
		string gameFileName = Configuration::getString(ecpGameFileName);
		// "D:\CoolGame\"
		string gameDir = gameFilePath.substr(0, gameFilePath.length() - gameFileName.length());
		// "saves\save1.sav"
		string relativeSavePath = originalSaveFilePath.substr(gameDir.length());
		// "D:\MySaveDirForCoolGame\saves\save1.sav"
		string saveFile = getRightPath(saveDir + PATH_DELIMITER + relativeSavePath);
		return saveFile;
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

	// Преобразовываем относительный путь в абсолютный
	string relativePathToAbsolute(string relative)
	{
		// Если путь не относительный, сразу возвращаем.
		wstring wRelative = widen(relative);
		BOOL res = PathIsRelative(wRelative.c_str());
		if (res == FALSE) {
			return relative;
		}

		// Получаем рабочую директорию.
		TCHAR buffer[MAX_PATH];
		PTSTR szDir = buffer;
		DWORD curDirLen = GetCurrentDirectory((DWORD)MAX_PATH, szDir);
		if (curDirLen == 0) {
			showError("Не могу прочесть текущую директорию");
			return "";
		}
		if (curDirLen >= MAX_PATH) {
			showError("Путь к текущей директории не помещается в буфер");
			return "";
		}

		// Совмещаем путь к рабочей директории и относительный путь.
		TCHAR bufferAbsolute[MAX_PATH];
		PTSTR szAbsolute = bufferAbsolute;
		PTSTR szCombined = PathCombine(szAbsolute, szDir, wRelative.c_str());
		if (szCombined == NULL) {
			showError("Не удалось получить абсолютный путь");
			return "";
		}
		wstring wAbsolute = szCombined;
		string absolute = narrow(wAbsolute);

		return absolute;
	}

	// Приводим путь к каноничной форме.
	string canonicalizePath(string path)
	{
		wstring wPathSrc = widen(path);
		TCHAR buffer[MAX_PATH];
		PTSTR szCanonicalizedPath = buffer;
		BOOL res = PathCanonicalize(buffer, wPathSrc.c_str());
		if (res == FALSE) {
			showError("Не удалось привести путь к каноническому виду");
			return "";
		}
		wstring wCanonicalizedPath = szCanonicalizedPath;
		string result = narrow(wCanonicalizedPath);
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
		// В буфере будет завершающий нулевой байт.
		int totalBufferLength = dwFileSize + 1;
		// Выделяем блок памяти
		char* pFileChunk = NULL;
		// Может не хватить памяти (если файл слишком большой)
		try {
			pFileChunk = new char[totalBufferLength];
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
		// Записываем завершающий нулевой байт в буфер, для корректного чтения.
		pFileChunk[dwFileSize] = 0;
		// Возвращаем результат
		*bufferPtr = pFileChunk;
		*bufferLength = totalBufferLength;
		// Не забываем освободить память вызовом "delete" после использования!
		return true;
	}

	// Создаём папки
	bool buildDirectoryPath(string path)
	{
		if (dirExists(path))
			return true;
		wstring wPath = widen(path);
		int res = SHCreateDirectoryEx(NULL, wPath.c_str(), NULL);
		return res == ERROR_SUCCESS;
	}

	// Удаляем папку со всем содержимым.
	bool deleteDirectory(string path)
	{
		if (!dirExists(path))
			return true;
		wstring wPath = widen(path);

		// Making the directory name double null terminated.
		int nFolderPathLen = (int)wPath.length();
		WCHAR *pszFrom = new WCHAR[nFolderPathLen + 2];
		wcscpy(pszFrom, wPath.c_str());
		pszFrom[nFolderPathLen] = 0;
		pszFrom[++nFolderPathLen] = 0;
		SHFILEOPSTRUCT stSHFileOpStruct = { 0 };
		// Delete operation.
		stSHFileOpStruct.wFunc = FO_DELETE;
		// Folder name as double null terminated string.
		stSHFileOpStruct.pFrom = pszFrom;
		// Do not prompt the user.
		stSHFileOpStruct.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
		// Will read this to check for any operation is aborted.
		stSHFileOpStruct.fAnyOperationsAborted = FALSE;
		int nFileDeleteOprnRet = SHFileOperation(&stSHFileOpStruct);
		delete[] pszFrom;

		return (0 == nFileDeleteOprnRet) && (stSHFileOpStruct.fAnyOperationsAborted == FALSE);
	}


	// Загрузка конфигурации плеера
	bool initOptions(string contentPath)
	{
		// Устанавливаем параметры по умолчанию
		Configuration::setBool(ecpSoundCacheEnabled, false);
		Configuration::setInt(ecpSaveSlotMax, 5);
		Configuration::setString(ecpDefaultSkinName, DEFAULT_SKIN_NAME);
		Configuration::setBool(ecpLimitSingleInstance, false);
		Configuration::setString(ecpCacheDir, "");

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
		bool contentPathSet = contentPath != "";
		bool contentPathFound = false;
		for (int i = 0; i < argCount; i++) {
			string param = params[i];
			// Если мы вызываем программу из командной строки,
			// то первым параметром идёт имя exe-файла, игнорируем его.
			if (endsWith(param, ".exe"))
				continue;
			bool isOption = startsWith(param, "-");
			// Нам передали путь к файлу игры в командной строке.
			// Просто сохраняем его, проверять будем позже,
			// когда убедимся что строка параметров разобрана правильно.
			if (!contentPathFound && !isOption) {
				// Если нам заранее известен путь к файлу игры, 
				// то мы игнорируем путь, указанный в командной строке.
				// Типичная ситуация - выбор произвольной игры в "Полке игр".
				// При запуске плеера указана одна игра, а нам теперь нужна другая.
				if (!contentPathSet) {
					contentPath = param;
					contentPathSet = true;
				}
				contentPathFound = true;
			} else if (isOption) {
				// Разбираем опции
				if (param == OPTION_ENABLE_SOUND_CACHE) {
					// Кэшировать ли звуковые файлы.
					Configuration::setBool(ecpSoundCacheEnabled, true);
				} else if (startsWith(param, OPTION_DEFAULT_SKIN)) {
					// Какой из стандартных шаблонов выбирать по умолчанию для игры, 
					// при отсутствии у игры своего шаблона, 
					// и отсутствии указаний в конфиге игры.
					if (i + 1 == argCount) {
						showError("Не указано имя шаблона для опции " + OPTION_DEFAULT_SKIN);
					}
					Configuration::setString(ecpDefaultSkinName, params[i + 1]);
					i++;
				} else if (param == OPTION_RESTART) {
					// Убить "старые" окна плеера.
					// Используется при редактировании игры, 
					// чтобы запускать обновлённую версию 
					// без закрытия плеера с предыдущей версией вручную.
					Configuration::setBool(ecpLimitSingleInstance, true);
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
		// Всё разобрано правильно.

		// Определяем игру для запуска.
		// 1. Если игра указана параметром командной строки, запускаем её.
		// 2. Если параметр не указан, ищем игру в папке плеера.
		// 3. Если в папке плеера игра не найдена, игра запускается из assets\standalone_content

		string contentDir = "";
		string skinFilePath = "";
		string gameFileName = "";
		string gameFilePath = "";
		string configFilePath = "";
		string saveDir = "";
		string windowTitle = QN_APP_NAME + " " + QN_VERSION;
		bool runningDefaultGame = false;
		if (contentPathSet) {
			// Нам передали путь к игре.
			// Всего три варианта:
			// 1. Это путь к архиву .qn;
			// 2. Это путь к файлу .qsp;
			// 3. Это путь к папке игры.
			// Проверяем путь на существование и читаемость.

			bool bValidDirectory = dirExists(contentPath);
			bool bValidFile = !bValidDirectory && fileExists(contentPath);
			// Путь к файлу игры должен быть абсолютным.
			// Если он будет относительным,
			// библиотека не сможет правильно загрузить QSP-модули.
			if (bValidFile || bValidDirectory) {
				contentPath = relativePathToAbsolute(contentPath);
			}
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
					// Сохраняем путь к файлу игры
					gameFilePath = contentPath;
					gameFileName = contentPath;
					// Вычисляем путь к папке игры
					if (contentPath.length() > 0) {
						int pos = contentPath.find_last_of(PATH_DELIMITER);
						if  (pos != string::npos) {
							contentDir = contentPath.substr(0, pos);
							gameFileName = contentPath.substr(pos + 1);
						}
					}
					// Вычисляем пути к необходимым файлам
					skinFilePath = getRightPath(contentDir + PATH_DELIMITER + DEFAULT_SKIN_FILE);
					configFilePath = getRightPath(contentDir + PATH_DELIMITER + DEFAULT_CONFIG_FILE);
				}
				// STUB
			} else if (bValidDirectory) {
				// Сохраняем путь к папке игры.
				// Если в указанной папке есть вложенная папка "standalone_content",
				// то считаем её папкой игры. Иначе считаем папкой игры указанную папку.
				string deepDir = contentPath + PATH_DELIMITER + DEFAULT_CONTENT_REL_PATH;
				if (dirExists(deepDir)) {
					contentDir = deepDir;
				} else {
					contentDir = contentPath;
				}

				// Вычисляем пути к необходимым файлам
				skinFilePath = getRightPath(contentDir + PATH_DELIMITER + DEFAULT_SKIN_FILE);
				// Определяем файл игры (*.qsp) в указанной папке.
				if (!findGameFile(contentDir, gameFileName))
					return false;
				gameFilePath = getRightPath(contentDir + PATH_DELIMITER + gameFileName);
				configFilePath = getRightPath(contentDir + PATH_DELIMITER + DEFAULT_CONFIG_FILE);
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
		}
		
		if (contentDir == "") {
			// Запускаем игру по умолчанию
			runningDefaultGame = true;
			string assetsDir = getPlayerDir() + PATH_DELIMITER + ASSETS_DIR;
			configFilePath = getRightPath(assetsDir + PATH_DELIMITER 
				+ DEFAULT_CONTENT_REL_PATH + PATH_DELIMITER 
				+ DEFAULT_CONFIG_FILE);
			contentDir = assetsDir + PATH_DELIMITER + DEFAULT_CONTENT_REL_PATH;
			// Определяем файл игры (*.qsp) в указанной папке.
			if (!findGameFile(contentDir, gameFileName))
				return false;
			gameFilePath = contentDir + PATH_DELIMITER + gameFileName;
			skinFilePath = getRightPath(contentDir + PATH_DELIMITER + DEFAULT_SKIN_FILE);
		}

		// Приводим путь к файлу игры в каноничную форму.
		gameFilePath = canonicalizePath(gameFilePath);
		
		// Считаем уникальный хэш игры.
		string gameHash = md5(gameFilePath);

		// Папка для сохранений
		saveDir = "";
		// Путь к пользовательской папке "Мои документы"
		WCHAR wszPath[MAX_PATH];
		HRESULT hr = SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, wszPath);
		if (hr != S_OK) {
			showError("Не удалось получить путь к папке \"Мои документы\".");
			return false;
		}
		saveDir = getRightPath(narrow(wszPath) + PATH_DELIMITER + DEFAULT_SAVE_REL_PATH + PATH_DELIMITER + gameHash);

		// Папка с данными приложения.
		// В ней находится БД, а также кэшированные игры.
		hr = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, wszPath);
		if (hr != S_OK) {
			showError("Не удалось получить путь к папке \"Application Data\".");
			return false;
		}
		string appDataDir = narrow(wszPath) + PATH_DELIMITER + APP_DATA_DIR;

		// Приводим все пути к каноничной форме.
		contentDir = canonicalizePath(contentDir);
		skinFilePath = canonicalizePath(skinFilePath);
		configFilePath = canonicalizePath(configFilePath);
		saveDir = canonicalizePath(saveDir);
		appDataDir = canonicalizePath(appDataDir);

		// Проверяем все файлы на читаемость
		if (!fileExists(skinFilePath))
			skinFilePath = "";
		if (!fileExists(configFilePath))
			configFilePath = "";
		if (!fileExists(gameFilePath)) {
			showError("Не удалось загрузить игру из файла: " + gameFilePath);
			return false;
		}

		// Сохраняем конфигурацию
		Configuration::setString(ecpContentDir, contentDir);
		Configuration::setString(ecpSkinFilePath, skinFilePath);
		Configuration::setString(ecpGameFilePath, gameFilePath);
		Configuration::setString(ecpGameHash, gameHash);
		Configuration::setString(ecpGameFileName, gameFileName);
		Configuration::setString(ecpConfigFilePath, configFilePath);
		Configuration::setString(ecpSaveDir, saveDir);
		Configuration::setString(ecpAppDataDir, appDataDir);
		Configuration::setString(ecpWindowTitle, windowTitle);
		Configuration::setBool(ecpIsFullscreen, false);
		Configuration::setBool(ecpRunningDefaultGame, runningDefaultGame);

		// Загружаем настройки игры из файла config.xml
		bool gameConfigLoaded = loadGameConfig();
		if (!gameConfigLoaded)
			return false;

		return true;
	}

	// Загружаем настройки игры из файла config.xml
	bool loadGameConfig()
	{
		// Устанавливаем настройки по умолчанию
		Configuration::setInt(ecpGameWidth, 800);
		Configuration::setInt(ecpGameHeight, 600);
		Configuration::setInt(ecpGameMinWidth, 0);
		Configuration::setInt(ecpGameMinHeight, 0);
		Configuration::setInt(ecpGameMaxWidth, 0);
		Configuration::setInt(ecpGameMaxHeight, 0);
		Configuration::setString(ecpGameTitle, Configuration::getString(ecpGameFileName));
		Configuration::setBool(ecpGameResizeable, true);
		Configuration::setBool(ecpGameFullscreenAvailable, true);
		Configuration::setBool(ecpGameStartFullscreen, false);
		Configuration::setString(ecpSkinName, "");
		Configuration::setBool(ecpGameIsStandalone, false);

		string configFilePath = Configuration::getString(ecpConfigFilePath);
		// Если файл не найден, то всё в порядке,
		// просто оставляем дефолтные настройки.
		if (!fileExists(configFilePath))
			return true;

		TiXmlDocument doc;
		// Читаем файл в память
		void* buffer = NULL;
		int bufferLength = 0;
		if (!loadFileToBuffer(configFilePath, &buffer, &bufferLength)) {
			showError("Не удалось прочесть файл \"" + configFilePath + "\".");
			return false;
		}
		// Разбираем XML
		doc.Parse((const char*)buffer, 0, TIXML_ENCODING_UTF8);
		delete buffer;
		if (doc.Error())
		{
			showError("Не удалось загрузить XML-структуру из файла \"" + configFilePath + "\".");
			return false;
		}
		TiXmlElement* root = doc.FirstChildElement();
		if (root == NULL)
		{
			showError("Не найден корневой элемент конфигурационного файла \"" + configFilePath + "\".");
			return false;
		}

		bool valid = true;
		LOAD_XML_ATTRIB("width", ecpGameWidth);
		LOAD_XML_ATTRIB("height", ecpGameHeight);
		LOAD_XML_ATTRIB("minWidth", ecpGameMinWidth);
		LOAD_XML_ATTRIB("minHeight", ecpGameMinHeight);
		LOAD_XML_ATTRIB("maxWidth", ecpGameMaxWidth);
		LOAD_XML_ATTRIB("maxHeight", ecpGameMaxHeight);
		LOAD_XML_ATTRIB("title", ecpGameTitle);
		LOAD_XML_ATTRIB("resizeable", ecpGameResizeable);
		LOAD_XML_ATTRIB("fullscreenAvailable", ecpGameFullscreenAvailable);
		LOAD_XML_ATTRIB("startFullscreen", ecpGameStartFullscreen);
		LOAD_XML_ATTRIB("skinName", ecpSkinName);
		LOAD_XML_ATTRIB("standalone", ecpGameIsStandalone);
		if (!valid) {
			return false;
		}

		// Обрабатываем настройки игры
		if (Configuration::getBool(ecpGameFullscreenAvailable) && 
			Configuration::getBool(ecpGameStartFullscreen)) {
				Configuration::setBool(ecpIsFullscreen, true);
		}
		string gameTitle = Configuration::getString(ecpGameTitle);
		if (gameTitle != "") {
			Configuration::setString(ecpWindowTitle, gameTitle);
		}

		return true;
	}

	// Готовим игру к запуску.
	bool prepareGameFiles()
	{
		// Основной алгоритм обработки файлов:
		// 1. Если в корне архива или в корне указанной локальной папки 
		//    есть папка standalone_content, 
		//    то корневой папкой игры считается она,
		//    иначе - корневой папкой игры считается корень архива 
		//    или указанной локальной папки.
		// 2. Проверяем наличие и читаемость скина игры.
		// 3. Проверяем наличие папки "qsplib" уровнем выше корневой папки игры.
		// 4. Если есть файл скина и есть папка "qsplib", то запускается игра.
		// 5. Иначе, создаётся временная папка, в которую копируются:
		// 5.1 - файлы оформления игры - .js, .css, картинки;
		// 5.2 - файл скина (если есть в папке игры - то скин игры, 
		//                   иначе дефолтный либо один из стандартных, 
		//                   указанный в config.xml)
		// 5.3 - папка "qsplib".
		// 6. Запускается игра со скином из временной папки.

		// В режиме "standalone" ничего не копируем, всё уже должно быть на своих местах.
		bool bStandalone = Configuration::getBool(ecpGameIsStandalone);

		// Если плеер запущен не в режиме standalone,
		// то нам потребуется папка данных.
		string appDataDir = Configuration::getString(ecpAppDataDir);
		// Если папка ещё не существует, создаём.
		if (!bStandalone) {
			if (!buildDirectoryPath(appDataDir)) {
				showError("Не удалось создать папку данных для плеера: " + appDataDir);
				return false;
			}
		}

		bool bCopyQsplib = false;
		bool bCopySkin = false;
		// Если настройка "ContentDir" не задана, 
		// значит игру не нашли.
		// Тогда используем "игру" по умолчанию из папки "assets\standalone_content".
		string contentDir = Configuration::getString(ecpContentDir);
		string skinFilePath = Configuration::getString(ecpSkinFilePath);
		string selectedSkin = Configuration::getString(ecpDefaultSkinName);
		string gameFilePath = Configuration::getString(ecpGameFilePath);
		string gameHash = Configuration::getString(ecpGameHash);
		string assetsDir = getPlayerDir() + PATH_DELIMITER + ASSETS_DIR;
		// Проверяем наличие qsplib.
		string gameQsplibDir = contentDir + PATH_DELIMITER + ".." + PATH_DELIMITER + QSPLIB_DIR;
		bCopyQsplib = !dirExists(gameQsplibDir);

		// Проверяем наличие скина.
		bCopySkin = skinFilePath == "";
		if (bCopySkin) {
			// Выясняем, какой скин был указан для игры.
			string configSkin = Configuration::getString(ecpSkinName);
			if (configSkin != "") {
				// Выбираем указанный скин.
				selectedSkin = configSkin;
			}
		}

		// Требуется копирование.
		if (bCopyQsplib || bCopySkin) {
			// Если в игре включен режим "standalone",
			// то выводим ошибку и завершаем работу.
			if (bStandalone) {
				string error = "Ошибка: при включенном режиме \"standalone\" отсутствует " 
					+ (bCopyQsplib ? "папка \"" + QSPLIB_DIR + "\"." : "шаблон оформления.");
				showError(error);
				return false;
			}

			// Проверяем наличие папки "assets", 
			// внутри которой лежат необходимые папки шаблонов оформления, 
			// "qsplib" и файл игры по умолчанию.
			if (!dirExists(assetsDir)) {
				showError("Не найдена папка \"" + assetsDir + "\" системных файлов плеера.");
				return false;
			}

			// Создаём временную папку для игры.
			string gameFolder = getRightPath(appDataDir + PATH_DELIMITER + GAME_CACHE_DIR + PATH_DELIMITER + gameHash);
			gameFolder = canonicalizePath(gameFolder);
			if (!buildDirectoryPath(gameFolder)) {
				showError("Не удалось создать временную папку для игры: " + gameFolder);
				return false;
			}
			Configuration::setString(ecpCacheDir, gameFolder);
			string contentFolder = gameFolder + PATH_DELIMITER + DEFAULT_CONTENT_REL_PATH;
			if (!buildDirectoryPath(contentFolder)) {
				showError("Не удалось создать папку для содержимого игры: " + contentFolder);
				return false;
			}

			// Копируем qsplib.
			// true - из базовых файлов, false - из папки игры.
			string source = bCopyQsplib ? 
				assetsDir + PATH_DELIMITER + QSPLIB_DIR : gameQsplibDir;
			string dest = gameFolder + PATH_DELIMITER + QSPLIB_DIR;
			if (!copyFileTree(source, dest)) {
				showError("Не удалось скопировать папку библиотеки из \"" + source + "\" в \"" + dest + "\".");
				return false;
			}

			// Копируем скин.
			string destSkinFile = contentFolder + PATH_DELIMITER + DEFAULT_SKIN_FILE;
			if (bCopySkin) {
				// Копируем из базовых файлов

				string skinsDir = assetsDir + PATH_DELIMITER + SKINS_DIR;

				// Заполняем список существующих скинов.
				vector<string> skinsList;
				if (!getFoldersList(skinsDir, skinsList))
					return false;

				// Ищем указанный скин среди существующих.
				bool found = false;
				for (int i = 0; i < (int)skinsList.size(); i++)
				{
					if (skinsList[i] == selectedSkin) {
						found = true;
						break;
					}
				}

				// Если не найден, выходим.
				if (!found) {
					showError("Не найден шаблон оформления " + selectedSkin);
					return false;
				}

				// Копируем все файлы из папки шаблона
				string source = skinsDir + PATH_DELIMITER + selectedSkin;
				if (!copyFileTree(source, contentFolder)) {
					showError("Не удалось скопировать шаблон оформления из \"" + 
						source + "\" в \"" + contentFolder + "\".");
					return false;
				}
			} else {
				// Копируем из папки игры
				if (!copyFile(skinFilePath, destSkinFile)) {
					showError("Не удалось скопировать файл шаблона оформления из \"" + 
						skinFilePath + "\" в \"" + destSkinFile + "\".");
					return false;
				}
			}
			// Обновляем переменную окружения
			Configuration::setString(ecpSkinFilePath, destSkinFile);

			// Копируем все файлы, которые могут понадобиться браузеру.
			vector<string> masks;
			// Картинки - ".jpg", ".jpeg", ".gif", ".bmp", ".png".
			masks.push_back("*.jpg");
			masks.push_back("*.jpeg");
			masks.push_back("*.gif");
			masks.push_back("*.bmp");
			masks.push_back("*.png");
			masks.push_back("*.cur");
			masks.push_back("*.ani");
			// Стили и скрипты - ".css", ".js".
			// Если bCopySkin == false, значит, мы используем скин из папки игры.
			// Поэтому копируем из папки игры CSS и JS файлы.
			if (!bCopySkin) {
				masks.push_back("*.css");
				masks.push_back("*.js");
			}
			// Проходим по всем расширениям.
			for (int i = 0; i < (int)masks.size(); i++) {
				if (!copyFileTree(contentDir, contentFolder, masks[i])) {
					showError("Не удалось переместить файлы с маской \"" + masks[i] +
						"\" из \"" + contentDir + "\" в \"" + contentFolder + "\"");
					return false;
				}
			}
		}
		return true;
	}

	// Получаем идентификатор для мьютекса.
	string getInstanceMutexId()
	{
		string gameFileMd5 = Configuration::getString(ecpGameHash);
		string mutexId = QN_APP_GUID + gameFileMd5;
		return mutexId;
	}

	// Выводим текст в консоль.
	void writeConsole(HWND hWnd, string text)
	{
		// Мы передаём текст в другое приложение,
		// специально созданное для отображения лога.

		// Правильнее передавать текст в стандартный поток вывода,
		// но это не получилось. 
		// Может быть, в будущем будет исправлено.

		// Преобразовываем текст в виндовый формат.
		wstring wText = widen(text);
		// Заполняем структуру для передачи данных.
		COPYDATASTRUCT cds;
		cds.dwData = (ULONG_PTR)eidtLog;
		cds.cbData = ((DWORD)wText.length() + 1)*sizeof(wchar_t);
		cds.lpData = (PVOID)wText.c_str();
		HWND hwndSender = hWnd;
		HWND hwndReceiver = FindWindow(szLogWindowClass, NULL);
		if (hwndReceiver == NULL) {
			// Нет окна для логирования - просто выходим.
			return;
		}
		LRESULT res = SendMessage(hwndReceiver, WM_COPYDATA, (WPARAM)hwndSender, (LPARAM)&cds);
		if (res != TRUE) {
			showError("Ошибка при отправке сообщения окну логирования");
		}
	}

	// Возвращаем список файлов либо папок
	bool _getFilesOrFoldersList(string directory, string mask, vector<string>& list, bool bFileMode)
	{
		vector<string> result;

		WIN32_FIND_DATA ffd;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		DWORD dwError = 0;

		wstring wDir = widen(getRightPath(directory + PATH_DELIMITER + mask));
		if (wDir.size() + 1 > MAX_PATH) {
			showError("Слишком длинный путь");
			return false;
		}
		LPCWSTR szDir = wDir.c_str();

		string label = bFileMode ? "файла" : "папки";

		// Find the first file in the directory.

		hFind = FindFirstFile(szDir, &ffd);

		if (INVALID_HANDLE_VALUE == hFind) 
		{
			dwError = GetLastError();
			if (dwError == ERROR_FILE_NOT_FOUND) {
				list = result;
				return true;
			} else {
				showError("Не удалось осуществить поиск " + label + " в папке " + directory + " по заданной маске " + mask);
				return false;
			}
		} 

		// List all the files in the directory with some info about them.

		do
		{
			if (!bFileMode == ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)) {
				string fileName = narrow(ffd.cFileName);
				if ((fileName != ".") && (fileName != ".."))
					result.push_back(fileName);
			}
		}
		while (FindNextFile(hFind, &ffd) != 0);

		dwError = GetLastError();
		if (dwError != ERROR_NO_MORE_FILES) 
		{
			showError("Не удалось осуществить повторный поиск " + label + " в папке " + directory + " по заданной маске " + mask);
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

	// Возвращаем список файлов
	bool getFilesList(string directory, string mask, vector<string>& list)
	{
		return _getFilesOrFoldersList(directory, mask, list, true);
	}

	// Возвращаем список папок
	bool getFoldersList(string directory, vector<string>& list)
	{
		return _getFilesOrFoldersList(directory, "*.*", list, false);
	}

	// Копируем файл
	bool copyFile(string from, string to)
	{
		wstring wFrom = widen(from);
		wstring wTo = widen(to);
		// Копируем файл на новое место.
		BOOL res = CopyFileW(wFrom.c_str(), wTo.c_str(), FALSE);
		if (res == 0) {
			return false;
		}
		// Сбрасываем атрибут "только для чтения" с файла назначения.
		// Если этого не сделать, 
		// позже будут сбои при попытке перезаписать этот файл.
		DWORD dwAttrs = GetFileAttributes(wTo.c_str()); 
		if (dwAttrs == INVALID_FILE_ATTRIBUTES) {
			return false;
		}
		if ((dwAttrs & FILE_ATTRIBUTE_READONLY) != 0) { 
			res = SetFileAttributes(wTo.c_str(), dwAttrs ^ FILE_ATTRIBUTE_READONLY); 
			if (res == 0) {
				return false;
			}
		} 
		return true;
	}

	// Копируем дерево файлов
	bool copyFileTree(string from, string to)
	{
		return copyFileTree(from, to, "*.*");
	}
	bool copyFileTree(string from, string to, string mask)
	{
		if (from == to) {
			showError("Нельзя копировать папку в саму себя: \"" + to + "\"");
			return false;
		}
		// Если папка назначения не существует, создаём.
		if (!buildDirectoryPath(to)) {
			showError("Не удалось создать папку \"" + to + "\"");
			return false;
		}
		// Выясняем, какие файлы находятся в папке.
		vector<string> vecFiles;
		if (!getFilesList(from, mask, vecFiles))
			return false;
		// Копируем все найденные файлы.
		for (int i = 0; i < (int)vecFiles.size(); i++) {
			string source = from + PATH_DELIMITER + vecFiles[i];
			string dest = to + PATH_DELIMITER + vecFiles[i];
			if (!copyFile(source, dest)) {
				showError("Не удалось скопировать файл из \"" + source +
					"\" в \"" + dest + "\".");
				return false;
			}
		}
		// Получаем список вложенных папок.
		vector<string> vecFolders;
		if (!getFoldersList(from, vecFolders))
			return false;
		// Проходим по всем вложенным папкам.
		for (int i = 0; i < (int)vecFolders.size(); i++) {
			// Для каждой вложенной папки рекурсивно вызываем функцию копирования.
			string source = from + PATH_DELIMITER + vecFolders[i];
			string dest = to + PATH_DELIMITER + vecFolders[i];
			if (!copyFileTree(source, dest, mask))
				return false;
		}
		return true;
	}

	// Диалог для открытия файла игры с диска.
	string openGameFileDialog(HWND hwnd)
	{
		string filePath = "";

		OPENFILENAME ofn;       // common dialog box structure
		wchar_t wszFile[MAX_PATH];       // buffer for file name

		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hwnd;
		ofn.lpstrFile = wszFile;
		// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
		// use the contents of szFile to initialize itself.
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(wszFile);
		string extensions = "Игры QSP (*.QSP)\r*.QSP\rУстаревший формат (*.GAM)\r*.GAM\r";
		wstring wExtensions = widen(extensions);
		// Меняем \r на нулевой байт. 
		// Если в исходную строку сразу вписать нулевой байт,
		// то она обрежется при конвертации.
		std::replace(wExtensions.begin(), wExtensions.end(), '\r', '\0');
		ofn.lpstrFilter = wExtensions.c_str();
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		// Display the Open dialog box. 

		BOOL res = GetOpenFileName(&ofn);
		if (res == 0) {
			DWORD dwError = CommDlgExtendedError();
			if (dwError != 0) {
				showError("Ошибка при открытии файла");
			}
			return "";
		}

		// Возвращаем имя открытого файла.
		wstring wFilePath = ofn.lpstrFile;
		filePath = narrow(wFilePath);
		return filePath;
	}

	// Ищем первый файл "*.qsp" в папке.
	bool findGameFile(string dir, string &gameFileName) {
		// Ищем все QSP-файлы в корне указанной папки
		vector<string> gameFileList;
		if (!getFilesList(dir, "*.qsp", gameFileList))
			return false;
		int count = (int)gameFileList.size();
		if (count == 0) {
			showError("Не найден файл игры в папке " + dir);
			return false;
		}
		else if (count > 1) {
			showError("В корневой папке игры должен находиться только один файл с расширением \"*.qsp\". Папка: " + dir);
			return false;
		}
		gameFileName = gameFileList[0];
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
	// Заменяем "&" на "&amp;", не трогая "HTML entities".
	void replaceAmp(string &s)
	{
		// /&(?:[a-z\d]+|#\d+|#x[a-f\d]+);/i
		// 1. Находим "&".
		// 2. Проверяем следующий символ. 
		// Это должен быть строчный символ латинского алфавита, либо решётка.
		// 3. Проверяем остальные символы, пока не встретится ";".
		// 4. Если последовательность символов не удовлетворяет условиям, заменяем "&" на "&amp;".
		string az = "abcdefghijklmnopqrstuvwxyz";
		string digits = "0123456789";
		string validFirstEntityChar = az + "#";
		string validMainEntityChar = az + digits;
		size_t last_pos = 0;
		size_t pos = 0;
		size_t t = 0;
		size_t len = s.length();
		bool entityPossible = false;
		string replaced = "";
		while (pos < len) {
			last_pos = pos;
			pos = s.find('&', pos);
			if (pos != string::npos) {
				// Возможна замена.
				// Для HTML-entity, в запасе после "&" должно оставаться минимум два символа.
				entityPossible = pos < (len - 2);
				if (entityPossible) {
					// Первым символом после "&" 
					// должен быть строчный символ латинского алфавита, либо решётка.
					entityPossible = validFirstEntityChar.find(s[pos + 1]) != string::npos;
					if (entityPossible) {
						// После этого допускаются цифры и символы латинского алфавита 
						// в произвольном количестве.
						t = s.find_first_not_of(validMainEntityChar, pos + 2);
						entityPossible = t != string::npos;
						if (entityPossible) {
							// Следующим символом должен быть ";".
							entityPossible = s[t] == ';';
						}
					}
				}
				// Мы определили, 
				// находится ли в строке 
				// правильная последовательность HTML-entity.

				// Записываем часть от last_pos до вхождения "&".
				replaced += s.substr(last_pos, pos - last_pos);

				// Работаем с проанализированным куском.
				if (!entityPossible) {
					// Это не HTML-entity.
					// Выполняем замену.
					replaced += "&amp;";
					pos = pos + 1;
				} else {
					// Это HTML-entity.
					// Записываем кусок "как есть".
					replaced += s.substr(pos, t - pos);
					pos += t - pos;
				}
			} else {
				// Заменять нечего.
				// Записываем оставшийся кусок.
				replaced += s.substr(last_pos, len - last_pos);
				break;
			}
		}
		s = replaced;
	}
	// Заменяем переводы строк, но только вне HTML-тегов.
	void replaceNewlines(string &s)
	{
		// 1. Ищем открывающую скобку тега.
		// 2. Если не найдена, делаем замену по всей оставшейся строке. Выход из цикла.
		// 3. Если найдена, делаем замену по строке до скобки.
		// 4. Ищем закрывающую скобку.
		// 5. Если закрывающая скобка не найдена, выходим из цикла.
		// 6. Позиция устанавливается на следующий символ после закрывающей скобки.
		// 7. Переход на п. 1.
		string startTag = "<";
		string endTag = ">";
		size_t last_pos = 0;
		size_t pos = 0;
		size_t len = s.length();
		string replaced = "";
		string token = "";
		while (pos < len) {
			last_pos = pos;
			// Ищем открывающую скобку.
			pos = s.find(startTag, pos);
			bool bTag = pos != string::npos;
			// Определяем, до какого символа делать замену.
			token = bTag ? s.substr(last_pos, pos - last_pos) : s.substr(last_pos);
			// Выполняем замену.
			replaceAll(token, "\r\n", "<br />");
			replaceAll(token, "\r", "<br />");
			replaceAll(token, "\n", "<br />");
			// Записываем обработанную часть текста.
			replaced += token;
			// Если тег не найден, прерываем цикл.
			if (!bTag)
				break;
			// Ищем закрывающую скобку.
			last_pos = pos;
			pos = s.find(endTag, pos);
			if (pos != string::npos) {
				// Устанавливаем позицию на следующий за закрывающей скобкой символ.
				pos++;
				// Записываем тело тега.
				replaced += s.substr(last_pos, pos - last_pos);
			} else {
				// Заменять нечего.
				// Записываем оставшийся кусок.
				replaced += s.substr(last_pos);
				break;
			}
		}
		s = replaced;
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

	// Преобразовываем целое число в строку.
	string intToString(int value)
	{
		char buff[100];
		sprintf(buff, "%d", value);
		string sInteger = buff;
		return sInteger;
	}

	DWORD getWindowStyle() {
		return Configuration::getBool(ecpGameResizeable) ? WS_OVERLAPPEDWINDOW :
			(WS_OVERLAPPED     | 
			WS_CAPTION        | 
			WS_SYSMENU        | 
			WS_MINIMIZEBOX);
	}

	// Переключение полноэкранного режима.
	WINDOWPLACEMENT wpc;
	void toggleFullscreenByHwnd(HWND hWnd) {
		setFullscreenByHwnd(hWnd, !Configuration::getBool(ecpIsFullscreen));
	}
	// Установка полноэкранного режима.
	void setFullscreenByHwnd(HWND hWnd, bool fullscreen) {
		if (!fullscreen) {
			// Из всего экрана в оконное                                      
			// Устанавливаем стили оконного режима
			SetWindowLong(hWnd, GWL_STYLE, getWindowStyle());
			// Загружаем парметры предыдущего оконного режима
			SetWindowPlacement(hWnd, &wpc);
			// Показываем обычное окно
			ShowWindow(hWnd, SW_SHOWNORMAL);

			Configuration::setBool(ecpIsFullscreen, false);
		} else if (Configuration::getBool(ecpGameFullscreenAvailable)) {
			// Сохраняем параметры оконного режима
			GetWindowPlacement(hWnd, &wpc);
			// Устанавливаем новые стили
			SetWindowLong(hWnd, GWL_STYLE, WS_POPUP);
			SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);
			// Окно во весь экран
			ShowWindow(hWnd, SW_SHOWMAXIMIZED);

			Configuration::setBool(ecpIsFullscreen, true);
		}
	}
}