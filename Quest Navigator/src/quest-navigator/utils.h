#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <vector>
#include <Awesomium/WebString.h>
#include "../deps/qsp/qsp.h"
#include "../deps/qsp/bindings/default/qsp_default.h"
#include "../deps/tinyxml/tinyxml.h"

using namespace std;
using namespace Awesomium;

namespace QuestNavigator {

	// Константы

	static const string DEFAULT_CONTENT_REL_PATH = "standalone_content";
	static const string ASSETS_DIR = "assets";
	static const string SKINS_DIR = "skins";
	static const string QSPLIB_DIR = "qsplib";
	static const string APP_DATA_DIR = "Quest Navigator Data";
	static const string GAME_CACHE_DIR = "Cached Games";
	static const string DEFAULT_SKIN_NAME = "default";
	static const string DEFAULT_SKIN_FILE = "gameAwesomium.html";
	static const string DEFAULT_CONFIG_FILE = "config.xml";
	static const string DEFAULT_SAVE_REL_PATH = "Сохранения игр Quest Navigator";
	static const string QN_WINDOWS_UPDATE_FEED = "http://feeds.feedburner.com/text-games/quest-navigator-windows";
	static const string QN_COMPANY_NAME = "QSP";
	static const string QN_APP_NAME = "Quest Navigator";
	static const string QN_VERSION = "0.0.29-test";
	static const string QN_APP_GUID = "95b90169-f70d-4269-b982-d9c3038b8348";
	static const wchar_t szWindowClass[] = L"QnViewWinClass";
	static const wchar_t szLogWindowClass[] = L"QnLogWinClass";

	static const string OPTION_ENABLE_SOUND_CACHE = "-enable-sound-cache";
	static const string OPTION_DEFAULT_SKIN = "-default-skin";
	static const string OPTION_RESTART = "-restart";
#ifdef _WIN32
	static const string PATH_DELIMITER = "\\";
#else
	static const string PATH_DELIMITER = "/";
#endif

	// Тип данных для передачи между экземплярами плеера, а также вспомогательными окнами.
	enum eIpcDataType {
		eidtRestart = 1,
		eidtLog
	};

	// Коды ошибок.
	enum eErrorCodes {
		eecUnableEnterCs1 = 1,			// Не удалось войти в критическую секцию (1).
		eecFailToCreateEvent,			// Не получилось создать событие.
		eecFailToCreateTimer,			// Не получилось создать таймер.
		eecFailToSetEvent,				// Не удалось запустить событие синхронизации потоков.
		eecFailToCloseHandle,			// Не удалось высвободить описатель объекта ядра.
		eecUnableEnterCs2,				// Не удалось войти в критическую секцию (2).
		eecLibThreadAlreadyStarted,		// Не удалось запустить поток библиотеки: он уже запущен.
		eecFailToInitCs,				// Не удалось проинициализировать структуру критической секции.
		eecFailToBeginLibThread,		// Не получилось создать поток интерпретатора.
		eecUnableEnterCs3,				// Не удалось войти в критическую секцию (3).
		eecFailToCalcBorders,			// Не удалось рассчитать толщину рамок окна.
		eecFailToCreateMainWindow		// Не удалось создать основное окно плеера.
	};

	// Утилиты для преобразования строк

	// UTF-16 wstring -> UTF-16 WebString
	WebString WideToWebString(wstring str);
	// UTF-16 wstring -> UTF-8 string
	string narrow(wstring str);
	string fromQsp(QSP_CHAR* str);
	string fromQsp(const QSP_CHAR* str);
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
	string backslashToSlash(string text);
	// Преобразовываем прямые косые черты в обратные ("/" -> "\")
	string slashToBackslash(string text);
	// Заменяем все вхождения подстроки в строке
	string replaceAll(string source, char pattern, char replacement);
	void replaceAll(string &s, const string &search, const string &replace);
	// Заменяем "&" на "&amp;", не трогая "HTML entities".
	void replaceAmp(string &s);
	// Заменяем переводы строк, но только вне HTML-тегов.
	void replaceNewlines(string &s);
	// Переводим все символы в верхний регистр
	string toUpper(string str);
	// Переводим все символы в нижний регистр
	string toLower(string str);
	// Преобразование URL ссылки вида <a href="EXEC:...">...</a> в код QSP
	string unescapeHtml(string text);
	// Обратное преобразование URL в "сырой" вид
	string decodeUrl(string url);
	// Преобразовываем целое число в строку.
	string intToString(int value);

	// Утилиты для работы с файловой системой

	// Получаем URL из полного пути к файлу
	string getUriFromFilePath(string filePath);
	// URL к содержимому
	string getContentUrl();
	// Проверяем файл на существование и читаемость
	bool fileExists(string path);
	// Проверяем папку на существование и читаемость
	bool dirExists(string path);
	// Получаем путь к папке плеера
	string getPlayerDir();
	// Преобразовываем путь к файлу сохранения
	string getRealSaveFile(string file);
	// Меняем слэши в пути к файлу в зависимости от системы
	string getRightPath(string path);
	// Преобразовываем относительный путь в абсолютный
	string relativePathToAbsolute(string relative);
	// Приводим путь к каноничной форме.
	string canonicalizePath(string path);
	// Загружаем файл в память
	bool loadFileToBuffer(string path, void** bufferPtr, int* bufferLength);
	// Создаём папки
	bool buildDirectoryPath(string path);
	// Удаляем папку и всё её содержимое.
	bool deleteDirectory(string path);
	// Возвращаем список файлов
	bool getFilesList(string directory, string mask, vector<string>& list);
	// Возвращаем список папок
	bool getFoldersList(string directory, vector<string>& list);
	// Копируем файл
	bool copyFile(string from, string to);
	// Копируем дерево файлов
	bool copyFileTree(string from, string to);
	bool copyFileTree(string from, string to, string mask);
	// Диалог для открытия файла игры с диска.
	string openGameFileDialog(HWND hWnd);
	// Ищем первый файл "*.qsp" в папке.
	bool findGameFile(string dir, string &gameFileName);

	// Прочее

	// Устанавливаем конфигурацию плеера при старте
	bool initOptions(string contentPath);
	// Загружаем настройки игры из файла config.xml
	bool loadGameConfig();
	// Готовим игру к запуску
	bool prepareGameFiles();
	// Получаем идентификатор для мьютекса.
	string getInstanceMutexId();
	// Выводим текст в консоль.
	void writeConsole(HWND hWnd, string text);

	// Проверяем наличие апдейта при старте
	void checkUpdate();
	// Завершаем работу апдейтера по выходу из приложения
	void finishUpdate();

	// Показываем системный диалог MessageBox
	void showMessage(string msg, string title);
	void showError(string msg);

	// Стиль для окна согласно настройкам игры
	DWORD getWindowStyle();
	// Переключение полноэкранного режима
	void toggleFullscreenByHwnd(HWND hWnd);
	// Установка полноэкранного режима.
	void setFullscreenByHwnd(HWND hWnd, bool fullscreen);
}





#endif
