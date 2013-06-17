#ifdef _WIN32
#include <Windows.h>
#endif

#include "quest-navigator/listener.h"

#ifdef _WIN32
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, wchar_t*, 
					  int nCmdShow) {
#else
int main() {
#endif

	QnApplicationListener listener;
	listener.Run();

	return 0;
}
