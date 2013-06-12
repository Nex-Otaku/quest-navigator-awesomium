#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <Awesomium/WebString.h>

using namespace std;
using namespace Awesomium;

namespace QuestNavigator {

	// ������� ��� �������������� �����

	// UTF-16 wstring -> UTF-16 WebString
	WebString WideToWebString(wstring str);
	// UTF-16 wstring -> UTF-8 string
	string narrow(wstring str);
	// UTF-8 string -> UTF-16 wstring
	wstring widen(string str);

	// ������� ��� ������ � �������� ��������

	// ���� � ����� ������
	string getPlayerSystemPath();
	// �������� URL �� ������� ���� � �����
	string getUriFromFilePath(string filePath);
	// URL � �����������
	string getContentUrl();

	// ������

	// ������������� ������������ ������ ��� ������
	void initOptions();
}





#endif
