#include "midi.h"
#include "utils.h"

namespace QuestNavigator {

	bool MidiService::muted = false;
	string MidiService::midiFile = "";

	void MidiService::play(string file, int volume)
	{
		// Предварительно закрываем предыдущий файл.
		// В один момент времени может проигрываться только один файл.
		if (!midiFile.empty()) {
			close(false, midiFile);
		}

		// Открываем файл.
		string command = "open \"" + file + "\" type sequencer alias MidiFile";
		wstring wCommand = widen(command);
		LPCWSTR pCommand = wCommand.c_str();
		MCIERROR err = mciSendString(pCommand, NULL, 0, NULL);
		if (err != 0)
		{
			showError("Не удалось открыть MIDI-файл: " + file);
			return;
		}
		// Запускаем проигрывание.
		err = mciSendString(L"play MidiFile from 0", NULL, 0, NULL);
		if (err != 0)
		{
			showError("Не удалось запустить MIDI-файл: " + file);
			return;
		}
		// Устанавливаем громкость проигрываемого файла.
		setVolume(volume);
		// Сохраняем имя файла.
		midiFile = file;
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
		muted = toBeMuted;
	}

	DWORD MidiService::getRealVolume(int volume)
	{
		DWORD result = 0;
		if (!muted)
			result = (volume * 0xFFFF) / 100;
		else
			result = 0;
		return result;
	}

	void MidiService::setVolume(int volume)
	{
		// Увы, этот способ установки громкости не работает.

		// Громкость задаётся целым числом от 0 до 1000.
		//string command = "setaudio MidiFile volume to " + intToString(getRealVolume(volume));
		//wstring wCommand = widen(command);
		//LPCWSTR pCommand = wCommand.c_str();
		//MCIERROR err = mciSendString(pCommand, NULL, 0, NULL);
		//if (err != 0)
		//{
		//	showError("Не удалось установить громкость для воспроизведения MIDI-файла");
		//	return;
		//}
	}
}