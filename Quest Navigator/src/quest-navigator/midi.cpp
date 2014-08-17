#include "midi.h"
#include "utils.h"

namespace QuestNavigator {

	bool MidiService::muted = false;
	string MidiService::midiFile = "";

	void MidiService::play(string file, int volume)
	{
		// Предварительно закрываем предыдущий файл.
		// В один момент времени может проигрываться только один файл MIDI.
		close(true, "");

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
		if (err == MCIERR_INVALID_DEVICE_NAME) {
			// Устройство ещё не зарегистрировано, следовательно и файл не проигрывается.
			return false;
		}
		if (err != 0) {
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
			if (isPlaying(midiFile)) {
				MCIERROR err = mciSendString(L"stop MidiFile", NULL, 0, NULL);
				if (err != 0)
				{
					showError("Не удалось остановить MIDI-файл: " + midiFile);
					return;
				}
				err = mciSendString(L"close MidiFile", NULL, 0, NULL);
				if (err != 0)
				{
					showError("Не удалось закрыть MIDI-файл: " + midiFile);
					return;
				}
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
		if (!muted) {
			// Значение громкости для одного канала от 0 до 65535.
			WORD v = ((volume * 0xFFFF) / 100) & 0xFFFF;
			// Громкость для обоих каналов.
			result = (v << 16) | v;
		} else {
			result = 0;
		}
			
		return result;
	}

	void MidiService::setVolume(int volume)
	{
		// Неизвестно, как установить громкость для MIDI.
		// Поэтому все MIDI-файлы будут проигрываться на 100% громкости.
	}
}