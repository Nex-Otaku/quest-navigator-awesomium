#ifndef MIDI_H_
#define MIDI_H_

#include <string>
#include <vector>
#include <Windows.h>
#include <mmsystem.h>

using namespace std;

// Класс для работы с MIDI-файлами.

// Из-за убогого интерфейса MCI,
// простыми средствами Windows не получается
// сделать полноценную поддержку MIDI.

// Поэтому поддержка MIDI ограничена:
// проигрывается только один файл за раз;
// громкость выставить нельзя.

// В будущем нужно будет попробовать перейти на DirectMusic.

//http://www.codeproject.com/Articles/2651/Developing-MIDI-applications-with-DirectMusic

//http ://www.codeproject.com/Articles/1560/Playing-Midi-Files-with-DirectMusic

//http ://msdn.microsoft.com/en-us/library/windows/desktop/ms807157.aspx

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
