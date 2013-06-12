#include "utils.h"
#ifdef _WIN32
#include <shlwapi.h>
#endif

#include <Awesomium/STLHelpers.h>

using namespace Awesomium;

/*

Везде используем string, в нём хранятся юникодовые строки в UTF-8.

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

// UTF-8 string -> UTF-16 wstring
wstring widen(string str)
{
	WebString webStr = ToWebString(str);
	wstring wStr((wchar_t*)webStr.data());
	return wStr;
}

// Путь к папке плеера
string getPlayerSystemPath()
{
	// Выясняем полный путь к запущенному EXE, включая имя файла
	TCHAR szBuf[MAX_PATH];
	PTSTR szPath = szBuf;
	GetModuleFileName(NULL, szPath, (DWORD)MAX_PATH);

	// Отбрасываем имя файла, оставляем только путь к папке
	PathRemoveFileSpec(szPath);

	wstring wPath = szPath;
	string path = narrow(wPath);
	return path;
}

// Получаем URL из полного пути к файлу
string getUrlFromFilePath(string filePath)
{
	TCHAR szString[1024];
	PTSTR szUrl = szString;
	wstring wFilePath = widen(filePath);
	PTSTR FileName = (PTSTR)wFilePath.c_str();
	DWORD sUrl = 1024;
	UrlCreateFromPath(FileName, szUrl, &sUrl, NULL); 
	wstring wUrl = szUrl;
	string url = narrow(wUrl);
	return url;
}

// URL к содержимому
string getContentUrl()
{
	string playerPath = getPlayerSystemPath();
	string contentPath = playerPath + "\\testgame\\standalone_content\\gameBrowserTest.html";
	//string contentPath = playerPath + "\\standalone_content\\test.html";
	return getUrlFromFilePath(contentPath);
}

// Загрузка конфигурации плеера
void initOptions()
{
	// STUB
}

}