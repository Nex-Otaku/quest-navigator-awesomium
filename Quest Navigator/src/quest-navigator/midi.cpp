#include "midi.h"
#include "utils.h"

namespace QuestNavigator {

	bool MidiService::muted = false;
	string MidiService::midiFile = "";
	int MidiService::volume = 0;

	void MidiService::play(string file, int volume)
	{
		// Предварительно закрываем предыдущий файл.
		// В один момент времени может проигрываться только один файл.
		if (!midiFile.empty()) {
			close(false, midiFile);
		}

		string command = "open \"" + file + "\" type sequencer alias MidiFile";
		wstring wCommand = widen(command);
		LPCWSTR pCommand = wCommand.c_str();
		MCIERROR err = mciSendString(pCommand, NULL, 0, NULL);
		if (err != 0)
		{
			showError("Не удалось открыть MIDI-файл: " + file);
			return;
		}
		err = mciSendString(L"play MidiFile from 0", NULL, 0, NULL);
		if (err != 0)
		{
			showError("Не удалось запустить MIDI-файл: " + file);
			return;
		}
		midiFile = file;
		// STUB
		// Сделать установку громкости проигрываемого файла.
	}

	bool MidiService::isPlaying(string file)
	{
		if (midiFile != file) {
			return false;
		}
		WCHAR szReturnString[128];
		LPWSTR pszReturnString = szReturnString;
		MCIERROR err = mciSendString(L"status MidiFile mode", pszReturnString, sizeof(szReturnString), NULL);
		if (err != 0)
		{
			showError("Не удалось опросить MIDI-файл: " + file);
			return false;
		}
		string returnString = narrow(wstring(pszReturnString));
		return (returnString == "playing") || (returnString == "seeking");
	}

	void MidiService::close(bool closeAll, string file)
	{
		if (!closeAll && ((int)file.length() == 0))
			return;
		if (closeAll || (midiFile == file)) {
			MCIERROR err = mciSendString(L"stop MidiFile", NULL, 0, NULL);
			if (err != 0)
			{
				showError("Не удалось остановить MIDI-файл: " + file);
				return;
			}
			err = mciSendString(L"close MidiFile", NULL, 0, NULL);
			if (err != 0)
			{
				showError("Не удалось закрыть MIDI-файл: " + file);
				return;
			}
			midiFile = "";
		}
	}

	void MidiService::mute(bool toBeMuted)
	{
		// STUB
	}

	float MidiService::getRealVolume(int volume)
	{
		float result = 0;
		if (!muted)
			result = ((float)volume) / 100;
		else
			result = 0;
		return result;
	}
}