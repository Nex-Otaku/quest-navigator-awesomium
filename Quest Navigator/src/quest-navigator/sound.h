#ifndef SOUND_H_
#define SOUND_H_

#include <string>
#include <vector>
#include "audiere.h"
#include <Windows.h>
#include "midi.h"

using namespace std;
using namespace audiere;

// Класс для работы со звуком
namespace QuestNavigator {

	class AudiereStopCallbackHolder : public RefImplementation<StopCallback> {
	public:
		// Колбэк Audiere, вызывается при остановке трека
		ADR_METHOD(void) streamStopped(StopEvent* event);
	};

	class SoundManager {
		struct ContainerMusic {
			string name; // Путь к файлу - такой, как задан в игре
			int volume;
			bool isMidi;
			OutputStreamPtr sound;
		};
	private:
		static vector<ContainerMusic> vecMusic;
		static AudioDevicePtr audioDevice;
		static bool muted;
		static bool cacheEnabled;

		static bool checkPlayingFileSetVolume(string file, bool setVolume, int volume);
		static float getRealVolume(int volume);

		// Синхронизация потоков
		static bool initedCritical;
		// Структура для критических секций
		static CRITICAL_SECTION csMusicData;
		// Входим в критическую секцию
		static void lockMusicData();
		// Выходим из критической секции
		static void unlockMusicData();
	public:
		// Вызывается при остановке трека
		static void clearBySoundPtr(OutputStreamPtr sound);

		static bool init();
		static void deinit();
		static void play(string file, int volume);
		static bool isPlaying(string file);
		static void close(bool closeAll, string file);
		static void mute(bool toBeMuted);
	};

}





#endif
