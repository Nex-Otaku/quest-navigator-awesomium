#include "utils.h"
#ifdef _WIN32
#include <shlwapi.h>
#endif

#include <Awesomium/STLHelpers.h>

using namespace Awesomium;

/*

����� ���������� string, � �� �������� ���������� ������ � UTF-8.

��� �������� � ������ Awesomium ��������������� � WebString �������� ToWebString.

��� ��������� �� WinApi:

1. ���� ������������ ��������� �� PTSTR ��� LPWSTR 
(� ����� ������������ ���� � ����, �.�. ������� ������)
�� ������ ��������� � wstring.
����� ������������ � string � UTF-8 �������� narrow.
������:
wstring wCmd = GetCommandLine();
string cmd = narrow(wCmd);

2. ���� ������������ � ����� �� TCHAR, 
�� ����� ������ ��������� � wstring ������� ������������� ��������� �� ������ ������.
����� ������������ � string � UTF-8 �������� narrow.
������:
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

// ���� � ����� ������
string getPlayerSystemPath()
{
	// �������� ������ ���� � ����������� EXE, ������� ��� �����
	TCHAR szBuf[MAX_PATH];
	PTSTR szPath = szBuf;
	GetModuleFileName(NULL, szPath, (DWORD)MAX_PATH);

	// ����������� ��� �����, ��������� ������ ���� � �����
	PathRemoveFileSpec(szPath);

	wstring wPath = szPath;
	string path = narrow(wPath);
	return path;
}

// �������� URL �� ������� ���� � �����
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

// URL � �����������
string getContentUrl()
{
	string playerPath = getPlayerSystemPath();
	string contentPath = playerPath + "\\testgame\\standalone_content\\gameBrowserTest.html";
	//string contentPath = playerPath + "\\standalone_content\\test.html";
	return getUrlFromFilePath(contentPath);
}

// �������� ������������ ������
void initOptions()
{
	// STUB
}

}