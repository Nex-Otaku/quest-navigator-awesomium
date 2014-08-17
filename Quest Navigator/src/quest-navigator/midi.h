#ifndef MIDI_H_
#define MIDI_H_

#include <string>
#include <vector>
#include <Windows.h>
#include <mmsystem.h>

using namespace std;

// Класс для работы со звуком
namespace QuestNavigator {

	class MidiService {
	private:
		static bool muted;
		static DWORD getRealVolume(int volume);
		static string midiFile;
	public:
		static void play(string file, int volume);
		static bool isPlaying(string file);
		static void close(bool closeAll, string file);
		static void mute(bool toBeMuted);
		static void setVolume(int volume);
	};

}





#endif
