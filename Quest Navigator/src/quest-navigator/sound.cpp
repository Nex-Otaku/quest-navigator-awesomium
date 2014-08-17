#include "sound.h"
#include "utils.h"
#include "configuration.h"

namespace QuestNavigator {

	vector<SoundManager::ContainerMusic> SoundManager::vecMusic;
	AudioDevicePtr SoundManager::audioDevice = NULL;
	bool SoundManager::muted = false;
	bool SoundManager::cacheEnabled = false;

	bool SoundManager::checkPlayingFileSetVolume(string file, bool setVolume, int volume)
	{
		if (file.length() == 0)
			return false;
		bool foundPlaying = false;
		lockMusicData();
		for (int i = 0; i < (int)vecMusic.size(); i++)
		{
			ContainerMusic& it = vecMusic[i];
			if (it.name == file)
			{
				if (it.isMidi) {
					foundPlaying = MidiService::isPlaying(it.name);
					if (setVolume && foundPlaying)
					{
						it.volume = volume;
						MidiService::setVolume(volume);
					}
				} else {
					foundPlaying = (setVolume && cacheEnabled) || !cacheEnabled || it.sound->isPlaying();
					if (setVolume)
					{
						it.volume = volume;
						float realVolume = getRealVolume(volume);
						it.sound->setVolume(realVolume);
						// Если файл уже не проигрывается, но остался в кэше,
						// запускаем проигрывание.
						// Трек по завершению был отмотан на начало,
						// поэтому просто вызываем play().
						if (cacheEnabled && !it.sound->isPlaying()) {
							it.sound->play();
						}
					}
				}
				break;
			}
		}
		unlockMusicData();
		return foundPlaying;
	}

	float SoundManager::getRealVolume(int volume)
	{
		float result = 0;
		if (!muted)
			result = ((float) volume) / 100;
		else 
			result = 0;
		return result;
	}

	// Синхронизация потоков
	bool SoundManager::initedCritical = false;
	// Структура для критических секций
	CRITICAL_SECTION SoundManager::csMusicData;

	// Входим в критическую секцию
	void SoundManager::lockMusicData()
	{
		try {
			EnterCriticalSection(&csMusicData);
		} catch (...) {
			showError("Не удалось войти в критическую секцию.");
			exit(eecUnableEnterCs3);
		}
	}

	// Выходим из критической секции
	void SoundManager::unlockMusicData()
	{
		LeaveCriticalSection(&csMusicData);
	}

	// Колбэк Audiere, вызывается при остановке трека
	void AudiereStopCallbackHolder::streamStopped(StopEvent* event)
	{
		if (event->getReason() == StopEvent::STREAM_ENDED) {
			// Обрабатываем завершение трека
			SoundManager::clearBySoundPtr(event->getOutputStream());
		}
	}

	void SoundManager::clearBySoundPtr(OutputStreamPtr sound)
	{
		// Останавливаем указанный трек
		lockMusicData();
		for (int i = 0; i < (int)vecMusic.size(); i++) {
			ContainerMusic& container = vecMusic[i];
			if (container.sound == sound) {
				if (cacheEnabled) {
					// Устанавливаем на начало
					container.sound->reset();
				} else {
					// Выгружаем из памяти
					container.sound->stop();
					container.sound = 0;
					vecMusic.erase(vecMusic.begin() + i);
				}
				break;
			}
		}
		unlockMusicData();
	}

	bool SoundManager::init()
	{
		// Загружаем настройки
		cacheEnabled = Configuration::getBool(ecpSoundCacheEnabled);
		// Инициализируем структуру критической секции
		try {
			InitializeCriticalSection(&csMusicData);
		} catch (...) {
			showError("Не удалось проинициализировать структуру критической секции.");
			return false;
		}
		initedCritical = true;
		// Запускаем звуковую библиотеку
		audioDevice = OpenDevice();
		if (!audioDevice) {
			showError("Не удалось запустить звуковую подсистему");
			deinit();
			return false;
		}
		// Устанавливаем колбэк
		StopCallbackPtr cbHolder = new AudiereStopCallbackHolder();
		audioDevice->registerCallback(cbHolder.get());
		return true;
	}

	void SoundManager::deinit()
	{
		// Высвобождаем структуру критической секции
		if (initedCritical) {
			DeleteCriticalSection(&csMusicData);
			initedCritical = false;
		}
	}

	void SoundManager::play(string file, int volume)
	{
		if (file.length() == 0) {
			showError("Не задано имя звукового файла");
			return;
		}

		// Проверяем, проигрывается ли уже этот файл.
		// Если проигрывается, ничего не делаем.
		// Если файл закэширован, функция перезапустит его и вернёт true,
		// опять же ничего делать не требуется.
		if (checkPlayingFileSetVolume(file, true, volume))
			return;

		string fullPath = getRightPath(file);
		// Проверяем существование и читаемость
		if (!fileExists(fullPath)) {
			showError("Не удалось открыть звуковой файл: " + file);
			return;
		}

		if (endsWith(file, ".mid")) {
			lockMusicData();
			// Очищаем список от других файлов MIDI.
			// Допускается прогигрывать не более одного MIDI-файла одновременно.
			// Мы найдём лишь один файл, поэтому достаточно прогнать цикл по вектору всего один раз.
			for (int i = 0; i < (int)vecMusic.size(); i++)
			{
				ContainerMusic& container = vecMusic[i];
				if (container.isMidi) {
					vecMusic.erase(vecMusic.begin() + i);
					break;
				}
			}

			// Добавляем файл в список.
			MidiService::play(file, volume);
			ContainerMusic container;
			container.isMidi = true;
			container.name = file;
			container.volume = volume;
			vecMusic.push_back(container);
			unlockMusicData();
		} else {
			// Читаем файл в память
			void* buffer = NULL;
			int bufferLength = 0;
			if (!loadFileToBuffer(fullPath, &buffer, &bufferLength)) {
				showError("Не удалось прочесть звуковой файл: " + file);
				return;
			}

			// Создаём объект файла на основе блока памяти
			FilePtr fp = CreateMemoryFile((const void*)buffer, bufferLength);
			// Содержимое буфера скопировано, он нам больше не нужен, высвобождаем память.
			delete buffer;
			if (!fp) {
				showError("Не удалось разместить в памяти звуковой файл: " + file);
				return;
			}
			OutputStreamPtr sound = OpenSound(audioDevice, fp);
			if (!sound)
			{
				showError("Неизвестный формат звукового файла: " + file);
				return;
			}

			// Добавляем файл в список
			lockMusicData();
			float realVolume = getRealVolume(volume);
			sound->setVolume(realVolume);
			sound->play();
			ContainerMusic container;
			container.isMidi = false;
			container.name = file;
			container.volume = volume;
			container.sound = sound;
			vecMusic.push_back(container);
			unlockMusicData();
		}
	}

	bool SoundManager::isPlaying(string file)
	{
		return checkPlayingFileSetVolume(file, false, 0);
	}

	void SoundManager::close(bool closeAll, string file)
	{
		if (!closeAll && ((int)file.length() == 0))
			return;
		lockMusicData();
		for (int i = 0; i < (int)vecMusic.size(); i++)
		{
			ContainerMusic& container = vecMusic[i];
			if (closeAll || (container.name == file))
			{
				if (container.isMidi) {
					MidiService::close(closeAll, file);
					if (!closeAll)
					{
						vecMusic.erase(vecMusic.begin() + i);
						break;
					}
				} else {
					if (container.sound->isPlaying())
						container.sound->stop();
					if (cacheEnabled) {
						// Устанавливаем трек на начало
						container.sound->reset();
					}
					else {
						// Высвобождаем память
						container.sound = 0;
					}
					if (!closeAll)
					{
						if (!cacheEnabled)
							vecMusic.erase(vecMusic.begin() + i);
						break;
					}
				}
			}
		}
		if (!cacheEnabled && closeAll)
			vecMusic.clear();
		unlockMusicData();
	}

	void SoundManager::mute(bool toBeMuted)
	{
		lockMusicData();
		muted = toBeMuted;
		for (int i = 0; i < (int)vecMusic.size(); i++)
		{
			ContainerMusic& container = vecMusic[i];
			if (container.isMidi) {
				MidiService::mute(toBeMuted);
				MidiService::setVolume(container.volume);
			} else {
				float realVolume = getRealVolume(container.volume);
				container.sound->setVolume(realVolume);
			}
		}
		unlockMusicData();
	}
}