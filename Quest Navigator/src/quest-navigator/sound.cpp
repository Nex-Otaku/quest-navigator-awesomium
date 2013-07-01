#include "sound.h"
#include "utils.h"
#include "configuration.h"

namespace QuestNavigator {

	vector<SoundManager::ContainerMusic> SoundManager::vecMusic;
	AudioDevicePtr SoundManager::audioDevice = NULL;
	bool SoundManager::muted = false;

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
				if (setVolume)
				{
					it.volume = volume;
					float realVolume = getRealVolume(volume);
					it.sound->setVolume(realVolume);
				}
				foundPlaying = true;
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
			exit(0);
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
			// Убираем звук из вектора
			SoundManager::clearBySoundPtr(event->getOutputStream());
		}
	}

	void SoundManager::clearBySoundPtr(OutputStreamPtr sound)
	{
		// Останавливаем указанный трек и очищаем структуру данных
		lockMusicData();
		for (int i = 0; i < (int)vecMusic.size(); i++) {
			ContainerMusic& container = vecMusic[i];
			if (container.sound == sound) {
				container.sound->stop();
				container.sound = 0;
				vecMusic.erase(vecMusic.begin() + i);
				break;
			}
		}
		unlockMusicData();
	}

	bool SoundManager::init()
	{
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

		//Проверяем, проигрывается ли уже этот файл.
		//Если проигрывается, ничего не делаем.
		if (checkPlayingFileSetVolume(file, true, volume))
			return;

		string fullPath = getRightPath(Configuration::getString(ecpContentDir) + "\\" + file);
		// Проверяем существование и читаемость
		if (!fileExists(fullPath)) {
			showError("Не удалось открыть звуковой файл: " + file);
			return;
		}
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
		container.name = file;
		container.volume = volume;
		container.sound = sound;
		vecMusic.push_back(container);
		unlockMusicData();
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
				if (container.sound->isPlaying())
					container.sound->stop();
				container.sound = 0;
				if (!closeAll)
				{
					vecMusic.erase(vecMusic.begin() + i);
					break;
				}
			}
		}
		if (closeAll)
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
			float realVolume = getRealVolume(container.volume);
			container.sound->setVolume(realVolume);
		}
		unlockMusicData();
	}
}