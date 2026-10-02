/**
 * @file memory.cpp
 * @date 2026-10-02
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Щуп памяти при снятии сжатого кадра ABC
 *
 * @details Вход готовится отдельным процессом. Сборка, мутации и оценка расхода
 *          памяти выполняются memory.py; рабочие исходники не подменяются.
 *          Числа ru_maxrss выдаются в единицах системы, перевод делает стенд
 *
 * @copyright Copyright © 2026
 *
 */
/**
 * Стандартные заголовочные файлы
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

/**
 * Системные заголовочные файлы
 */
#include <sys/resource.h>

/**
 * Если операционной системой является Linux
 */
#if defined(__linux__)
	#include <sys/wait.h>
	#include <unistd.h>
#endif

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/abc/chunk.hpp>
#include <codec/abc/encoding.hpp>
#include <sys/fmk.hpp>

/**
 * Используем стандартное пространство имён и пространство имён проекта
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Главная функция щупа
 *
 * @param argc количество параметров
 * @param argv параметры запуска
 * @return     код завершения
 *
 */
int32_t main(int32_t argc, char ** argv){
	// Проверяем наличие режима, имени файла, размера и метода сжатия
	if(argc != 5)
		// Сообщаем об ошибке параметров
		return 2;
	// Проверяем режим вызова
	if((string(argv[1]) != "generate") && (string(argv[1]) != "read"))
		// Сообщаем об ошибке режима
		return 2;
	/**
	 * Если операционной системой является Linux, начинаем учёт в новом процессе
	 *
	 * @details Пик RSS после exec может включать память запускающего Python.
	 *          Создаём потомка после загрузки щупа, до инициализации ядра AWH:
	 *          fork начинает новый учёт расхода, сохраняя только текущее содержимое.
	 *          Потомок остаётся в группе процесса, которую завершает таймаут стенда
	 */
	#if defined(__linux__)
		// Создаём процесс измерения
		const pid_t child = ::fork();
		// Если создать процесс не удалось
		if(child < 0)
			// Сообщаем об отказе запуска измерения
			return 12;
		/**
		 * Если выполняется родитель, ждём исхода измерения
		 */
		if(child > 0){
			// Код завершения процесса измерения
			int32_t status = 0;
			/**
			 * Повторяем ожидание только при прерывании сигналом
			 */
			while(::waitpid(child, &status, 0) < 0){
				// Если ожидание завершилось настоящей ошибкой
				if(errno != EINTR)
					// Сообщаем об отказе ожидания
					return 13;
			}
			// Передаём исход потомка; гибель по сигналу является отказом щупа
			return (WIFEXITED(status) ? WEXITSTATUS(status) : 14);
		}
	#endif
	// Инициализируем модуль ядра, как в наборе проверок
	fmk::initialize();
	// Объект сжатия данных
	compressor::block_t compressor;
	// Укладчик кадров
	abc::packer_t packer;
	// Устанавливаем модуль сжатия
	packer.compressor(&compressor);
	// Конец прочитанного размера
	char * end = nullptr;
	// Снимаем прежний код ошибки преобразования
	errno = 0;
	// Получаем запрошенный размер
	const uint64_t size = ::strtoull(argv[3], &end, 10);
	// Допускаем границу на один октет выше максимального входа в 128 МиБ
	if((errno != 0) || (end == argv[3]) || (* end != '\0') || (size > 134217729))
		// Сообщаем об ошибке размера
		return 2;
	// Снимаем прежний код ошибки преобразования
	errno = 0;
	// Получаем номер метода сжатия
	const uint64_t selected = ::strtoull(argv[4], &end, 10);
	// Проверяем номер метода сжатия
	if((errno != 0) || (end == argv[4]) || (* end != '\0') || (selected < 1) || (selected > 11))
		// Сообщаем об ошибке метода сжатия
		return 15;
	// Получаем настройки укладчика
	auto options = packer.settings();
	// Устанавливаем выбранный метод сжатия текста
	options.text = static_cast <compressor::method_t> (selected);
	// Передаём настройки укладчику
	packer.settings(options);
	// Буфер кадра
	vector <uint8_t> record;
	/**
	 * Если нужно подготовить вход отдельным процессом
	 */
	if(string(argv[1]) == "generate"){
		// Ограничиваем размер создаваемого входа 128 МиБ
		if((size == 0) || (size > 134217728))
			// Сообщаем об ошибке размера
			return 2;
		// Создаём хорошо сжимаемое содержимое
		const string payload(static_cast <size_t> (size), 'a');
		// Укладываем содержимое штатным укладчиком
		if(!packer.pack(payload.data(), payload.size(), abc::payload_t::TEXT, 1, 0, record))
			// Сообщаем об отказе укладки
			return 3;
		// Проверяем выбор ожидаемого метода сжатия
		if(record.at(0) != selected)
			// Сообщаем об отсутствии ожидаемого сжатия
			return 4;
		// Открываем файл кадра
		ofstream file(argv[2], ios::binary);
		// Записываем кадр для последующих независимых процессов
		file.write(reinterpret_cast <const char *> (record.data()), record.size());
		// Сообщаем параметры подготовленного кадра
		::printf("generated=%zu origin=%zu\n", record.size(), static_cast <size_t> (size));
		// Проверяем запись кадра
		return (file.good() ? 0 : 5);
	}
	// Открываем заранее подготовленный кадр
	ifstream file(argv[2], ios::binary);
	// Проверяем открытие файла
	if(!file)
		// Сообщаем об ошибке чтения
		return 6;
	// Читаем только сжатые октеты
	record.assign(istreambuf_iterator <char> (file), istreambuf_iterator <char> ());
	// Проверяем длину заголовка
	if(record.size() < abc::CHUNK_HEADER)
		// Сообщаем об ошибке кадра
		return 7;
	// Проверяем метод в заголовке фактического кадра
	if(record.at(0) != selected)
		// Сообщаем о несовпадении метода сжатия
		return 16;
	// Запоминаем настоящую длину исходного содержимого
	const size_t original = static_cast <size_t> (abc::gather(record.data() + 8, 4));
	// Подменяем объявленную длину без изменения сжатого содержимого
	abc::fixed(record.data() + 8, size, 4);
	// Пересчитываем сумму, чтобы дойти до проверки размера
	abc::fixed(record.data() + abc::CHUNK_DIGEST, abc::digest(record.data(), record.size()), 8);
	// Снятое содержимое
	vector <uint8_t> content;
	// Сведения о кадре
	abc::chunk_t chunk;
	// Смещение чтения
	size_t offset = 0;
	// Показатели процесса до и после снятия кадра
	struct rusage before, after;
	// Запоминаем исходный пик памяти
	if(::getrusage(RUSAGE_SELF, &before) != 0)
		// Сообщаем об ошибке измерения
		return 8;
	// Снимаем кадр настоящим кодом ABC
	const bool accepted = packer.unpack(record.data(), record.size(), offset, content, chunk);
	// Запоминаем итоговый пик памяти
	if(::getrusage(RUSAGE_SELF, &after) != 0)
		// Сообщаем об ошибке измерения
		return 9;
	// Проверяем содержимое положительного контрольного случая
	const bool valid = (accepted && (content.size() == original) && all_of(content.begin(), content.end(), [](const uint8_t value) noexcept -> bool {
		// Проверяем ожидаемый октет
		return (value == 'a');
	}));
	// Печатаем измерения в единицах системы и признаки причин отказа
	::printf("RESULT original=%zu declared=%zu packed=%zu accepted=%d error=%u offset=%zu output=%zu valid=%d invalid_chunk=%d compression_failed=%d rss_before=%ld rss_after=%ld rss_delta=%ld\n", original, static_cast <size_t> (size), record.size(), accepted, static_cast <unsigned int> (packer.error()), offset, content.size(), valid, (packer.error() == abc::error_t::INVALID_CHUNK), (packer.error() == abc::error_t::COMPRESSION_FAILED), before.ru_maxrss, after.ru_maxrss, after.ru_maxrss - before.ru_maxrss);
	// Проверяем исход, отдельно от измеренного расхода памяти
	return ((size == original) ? (valid ? 0 : 10) : ((!accepted && (offset == 0) && content.empty()) ? 0 : 11));
}
