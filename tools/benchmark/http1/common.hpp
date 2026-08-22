/**
 * @file common.hpp
 * @date 2026-07-26
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Общее окружение эталонных стендов сравнения парсера протокола HTTP/1.x —
 *        эталонные сообщения, параметры нагрузки, учёт выделений памяти,
 *        разбор параметров запуска и вывод результатов
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_HTTP1__
#define __AWH_BENCHMARK_RIVAL_HTTP1__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <string>
#include <chrono>

/**
 * Системные заголовочные файлы
 */
#include <sys/resource.h>

/**
 * @brief Пространство имён эталонных стендов сравнения парсера протокола HTTP/1.x
 *
 * @details Эталонные сообщения и параметры нагрузки обязаны совпадать со
 *          сценариями `benchmark/proto/http1` библиотеки AWH: сравниваются
 *          реализации разбора, а не разные объёмы работы, поэтому любое
 *          расхождение здесь обесценивает отчёт целиком
 *
 */
namespace rival {
	/**
	 * @brief Размер тела сообщения сценариев с телом в октетах
	 *
	 */
	static constexpr size_t BODY_SIZE = (8 * 1024 * 1024);
	/**
	 * @brief Размер одного чанка сценариев с кодировкой chunked в октетах
	 *
	 */
	static constexpr size_t CHUNK_SIZE = (16 * 1024);
	/**
	 * @brief Количество сообщений сценария разбора запроса без заголовков
	 *
	 */
	static constexpr size_t TINY_ROUNDS = 300000;
	/**
	 * @brief Количество сообщений сценария разбора запроса браузера
	 *
	 */
	static constexpr size_t TYPICAL_ROUNDS = 100000;
	/**
	 * @brief Количество сообщений сценария побайтовой подачи
	 *
	 */
	static constexpr size_t FRAGMENT_ROUNDS = 5000;
	/**
	 * @brief Количество сообщений сценариев с телом
	 *
	 */
	static constexpr size_t BODY_ROUNDS = 50;
	/**
	 * @brief Количество сообщений прогрева перед замером
	 *
	 * @details Первые сообщения выводят внутренние накопители разбираемой
	 *          реализации на рабочий объём, и их выделения памяти к
	 *          установившемуся режиму отношения не имеют
	 *
	 */
	static constexpr size_t WARMUP_ROUNDS = 4;
	/**
	 * @brief Структура итогов прогона сценария
	 *
	 */
	typedef struct Outcome {
		// Количество обработанных сообщений
		size_t messages;
		// Количество обработанных октетов
		size_t bytes;
		// Затраченное время в секундах
		double seconds;
		// Количество выполненных выделений памяти
		size_t allocations;
		// Суммарный объём выделенной памяти в октетах
		size_t allocated;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Outcome() noexcept :
		 messages(0), bytes(0), seconds(0.0), allocations(0), allocated(0) {}
	} outcome_t;
	/**
	 * @brief Внутреннее состояние учёта выделений памяти
	 *
	 */
	namespace counter {
		// Количество выполненных выделений памяти
		static size_t count = 0;
		// Суммарный объём выделенной памяти в октетах
		static size_t bytes = 0;
		// Флаг активности учёта выделений памяти
		static bool enabled = false;
	};
	/**
	 * @brief Контрольная сумма обработанных данных
	 *
	 * @details Потребитель обязан прочитать каждый октет тела: без чтения
	 *          замерялась бы не пропускная способность, а скорость
	 *          перешагивания через данные. Накопитель объявлен изменчивым,
	 *          чтобы оптимизатор не удалил чтение целиком
	 *
	 */
	static volatile size_t checksum = 0;
	/**
	 * @brief Функция потребления фрагмента тела сообщения
	 *
	 * @note Повторяет работу потребителя сценариев AWH октет в октет:
	 *       сумма фрагмента накапливается в регистре и переносится в
	 *       изменчивый накопитель один раз на фрагмент
	 *
	 * @param buffer буфер фрагмента тела сообщения
	 * @param size   размер фрагмента тела сообщения
	 *
	 */
	static inline void consume(const void * buffer, const size_t size) noexcept {
		// Сумма октетов принятого фрагмента тела
		size_t sum = 0;
		// Получаем байтовый указатель на фрагмент тела
		const uint8_t * data = static_cast <const uint8_t *> (buffer);
		/**
		 * Читаем фрагмент тела целиком
		 */
		for(size_t i = 0; i < size; i++)
			// Суммируем очередной октет фрагмента тела
			sum += data[i];
		// Накапливаем контрольную сумму принятых данных
		checksum = (checksum + sum);
	}
	/**
	 * @brief Функция учёта разобранного заголовка сообщения
	 *
	 * @param name  размер названия заголовка
	 * @param value размер значения заголовка
	 *
	 */
	static inline void account(const size_t name, const size_t value) noexcept {
		// Накапливаем контрольную сумму разобранных заголовков
		checksum = (checksum + name + value);
	}
	/**
	 * @brief Функция управления учётом выделений памяти
	 *
	 * @param mode режим учёта выделений памяти
	 *
	 */
	static inline void counting(const bool mode) noexcept {
		// Если учёт выделений памяти включается
		if(mode){
			// Сбрасываем количество выполненных выделений памяти
			counter::count = 0;
			// Сбрасываем объём выделенной памяти
			counter::bytes = 0;
		}
		// Устанавливаем режим учёта выделений памяти
		counter::enabled = mode;
	}
	/**
	 * @brief Функция получения эталонного запроса без заголовков
	 *
	 * @return эталонный запрос
	 *
	 */
	static inline const std::string & tiny() noexcept {
		// Эталонный запрос без заголовков
		static const std::string result = "GET / HTTP/1.1\r\n\r\n";
		// Выводим эталонный запрос
		return result;
	}
	/**
	 * @brief Функция получения эталонного запроса браузера
	 *
	 * @return эталонный запрос
	 *
	 */
	static inline const std::string & typical() noexcept {
		// Эталонный запрос браузера с типовым набором заголовков
		static const std::string result =
			"GET /path/to/some/resource?query=value&other=thing HTTP/1.1\r\n"
			"Host: www.anyks.com\r\n"
			"User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36\r\n"
			"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n"
			"Accept-Language: ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7\r\n"
			"Accept-Encoding: gzip, deflate, br\r\n"
			"Cookie: session=8f3a9c2b1e7d4f05; theme=dark; lang=ru; tracking=abcdef0123456789abcdef0123456789\r\n"
			"Connection: keep-alive\r\n"
			"Upgrade-Insecure-Requests: 1\r\n"
			"\r\n";
		// Выводим эталонный запрос
		return result;
	}
	/**
	 * @brief Функция получения эталонного запроса с телом фиксированного размера
	 *
	 * @param size размер тела сообщения
	 * @return     эталонный запрос
	 *
	 */
	static inline const std::string & identity(const size_t size) noexcept {
		// Эталонный запрос с телом фиксированного размера
		static std::string result = "";
		// Если эталонный запрос ещё не построен
		if(result.empty()){
			// Формируем блок заголовков запроса
			result.append("POST /upload HTTP/1.1\r\nHost: www.anyks.com\r\nContent-Length: ").append(std::to_string(size)).append("\r\n\r\n");
			// Дописываем тело запроса
			result.append(size, 'x');
		}
		// Выводим эталонный запрос
		return result;
	}
	/**
	 * @brief Функция получения эталонного запроса с телом в кодировке chunked
	 *
	 * @param size    размер тела сообщения
	 * @param portion размер одного чанка
	 * @return        эталонный запрос
	 *
	 */
	static inline const std::string & chunked(const size_t size, const size_t portion) noexcept {
		// Эталонный запрос с телом в кодировке chunked
		static std::string result = "";
		// Если эталонный запрос ещё не построен
		if(result.empty()){
			// Формируем блок заголовков запроса
			result.append("POST /upload HTTP/1.1\r\nHost: www.anyks.com\r\nTransfer-Encoding: chunked\r\n\r\n");
			// Формируем данные одного чанка
			const std::string block(portion, 'y');
			// Текстовый буфер строки размера чанка
			char line[32];
			// Формируем строку размера чанка
			const int length = ::snprintf(line, sizeof(line), "%zX\r\n", portion);
			/**
			 * Дописываем чанки тела запроса до достижения требуемого размера
			 */
			for(size_t i = 0; i < (size / portion); i++)
				// Дописываем очередной чанк тела запроса
				result.append(line, static_cast <size_t> (length)).append(block).append("\r\n", 2);
			// Дописываем последний чанк и пустой блок трейлеров
			result.append("0\r\n\r\n");
		}
		// Выводим эталонный запрос
		return result;
	}
	/**
	 * @brief Функция прогона разбора эталонного сообщения
	 *
	 * @note Функция объявлена шаблонной намеренно: разбираемая реализация
	 *       подставляется по значению типа и вызовы её методов инлайнятся.
	 *       Виртуальный интерфейс добавил бы косвенный вызов на каждый
	 *       фрагмент подачи, а сценарий побайтовой подачи измеряет как раз
	 *       стоимость одного вызова - искажение составило бы десятки процентов
	 *
	 * @tparam Subject разбираемая реализация: `reset()`, `feed(данные, размер)`, `complete()`
	 *
	 * @param subject  разбираемая реализация
	 * @param message  эталонное сообщение
	 * @param rounds   количество разбираемых сообщений
	 * @param fragment размер фрагмента подачи (0 - сообщение подаётся целиком)
	 * @param output   итоги прогона
	 * @return         результат прогона (false - сообщение разобрано с ошибкой)
	 *
	 */
	template <typename Subject>
	static bool parsing(Subject & subject, const std::string & message, const size_t rounds, const size_t fragment, outcome_t & output) noexcept {
		/**
		 * Выполняем прогрев разбираемой реализации
		 */
		for(size_t i = 0; i < WARMUP_ROUNDS; i++){
			// Выполняем сброс состояния для разбора следующего сообщения
			subject.reset();
			// Если сообщение разобрано с ошибкой
			if(!subject.feed(message.data(), message.size()))
				// Выводим отрицательный результат
				return false;
			// Если сообщение разобрано не полностью
			if(!subject.complete())
				// Выводим отрицательный результат
				return false;
		}
		// Включаем учёт выделений памяти
		counting(true);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем разбор эталонного сообщения требуемое количество раз
		 */
		for(size_t i = 0; i < rounds; i++){
			// Выполняем сброс состояния для разбора следующего сообщения
			subject.reset();
			// Если сообщение подаётся целиком
			if(fragment == 0)
				// Выполняем разбор эталонного сообщения одним вызовом
				subject.feed(message.data(), message.size());
			// Если сообщение подаётся фрагментами
			else {
				/**
				 * Выполняем подачу сообщения фрагментами заданного размера
				 */
				for(size_t offset = 0; offset < message.size(); offset += fragment){
					// Вычисляем размер очередного фрагмента подачи
					const size_t length = ((message.size() - offset) < fragment ? (message.size() - offset) : fragment);
					// Выполняем разбор очередного фрагмента сообщения
					subject.feed(message.data() + offset, length);
				}
			}
		}
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		// Отключаем учёт выделений памяти
		counting(false);
		// Если последнее сообщение разобрано не полностью
		if(!subject.complete())
			// Выводим отрицательный результат
			return false;
		// Устанавливаем количество обработанных сообщений
		output.messages = rounds;
		// Устанавливаем количество обработанных октетов
		output.bytes = (rounds * message.size());
		// Устанавливаем затраченное время
		output.seconds = std::chrono::duration <double> (finish - start).count();
		// Устанавливаем количество выполненных выделений памяти
		output.allocations = counter::count;
		// Устанавливаем объём выделенной памяти
		output.allocated = counter::bytes;
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция извлечения количества сообщений в секунду
	 *
	 * @param outcome итоги прогона сценария
	 * @return        количество сообщений в секунду
	 *
	 */
	static inline double perSecond(const outcome_t & outcome) noexcept {
		// Если время прогона не измерено
		if(outcome.seconds <= 0.0)
			// Выводим нулевое количество сообщений в секунду
			return 0.0;
		// Выводим количество сообщений в секунду
		return (static_cast <double> (outcome.messages) / outcome.seconds);
	}
	/**
	 * @brief Функция извлечения пропускной способности в мебибайтах в секунду
	 *
	 * @param outcome итоги прогона сценария
	 * @return        пропускная способность
	 *
	 */
	static inline double megabytes(const outcome_t & outcome) noexcept {
		// Если время прогона не измерено
		if(outcome.seconds <= 0.0)
			// Выводим нулевую пропускную способность
			return 0.0;
		// Выводим пропускную способность в мебибайтах в секунду
		return ((static_cast <double> (outcome.bytes) / 1048576.0) / outcome.seconds);
	}
	/**
	 * @brief Функция извлечения количества выделений памяти на одно сообщение
	 *
	 * @param outcome итоги прогона сценария
	 * @return        количество выделений памяти на одно сообщение
	 *
	 */
	static inline double allocated(const outcome_t & outcome) noexcept {
		// Если сообщения не обрабатывались
		if(outcome.messages == 0)
			// Выводим нулевое количество выделений памяти
			return 0.0;
		// Выводим количество выделений памяти на одно сообщение
		return (static_cast <double> (outcome.allocations) / static_cast <double> (outcome.messages));
	}
	/**
	 * @brief Функция вывода результата прогона сценария
	 *
	 * @note Формат вывода повторяет набор бенчмарков AWH, поэтому результаты
	 *       стендов и библиотеки сводятся в одну таблицу без пересчёта
	 *
	 * @param name    название сценария
	 * @param units   единица измерения характеристики
	 * @param value   измеренное значение характеристики
	 * @param outcome итоги прогона сценария
	 *
	 */
	static inline void report(const char * name, const char * units, const double value, const outcome_t & outcome) noexcept {
		// Выводим измеренное значение характеристики
		::printf("%-32s %14.2f   (%s)\n", name, value, units);
		// Выводим сведения о прогоне сценария
		::printf(
			"%34sсообщений: %zu, выделений: %zu (%.2f на сообщение), выделено: %.1f МБ\n",
			"", outcome.messages, outcome.allocations, allocated(outcome),
			(static_cast <double> (outcome.allocated) / 1048576.0)
		);
	}
	/**
	 * @brief Функция вывода сообщения о неудачном прогоне сценария
	 *
	 * @param name   название сценария
	 * @param reason причина отказа от прогона
	 *
	 */
	static inline void skip(const char * name, const char * reason) noexcept {
		// Выводим сообщение о неудачном прогоне сценария
		::printf("%-32s %14s   (%s)\n", name, "—", reason);
	}
	/**
	 * @brief Функция проверки соответствия сценария фильтру запуска
	 *
	 * @param name   название сценария
	 * @param filter фильтр названий сценариев
	 * @return       результат проверки соответствия
	 *
	 */
	static inline bool selected(const char * name, const char * filter) noexcept {
		// Если фильтр не задан, выполняются все сценарии
		return ((filter == nullptr) || (::strstr(name, filter) != nullptr));
	}
	/**
	 * @brief Функция вывода контрольной суммы обработанных данных
	 *
	 * @details Контрольная сумма складывается из размеров разобранных заголовков
	 *          и суммы октетов тела: при одинаковой нагрузке и одинаковом объёме
	 *          работы потребителя она обязана совпасть у всех сравниваемых
	 *          реализаций. Расхождение означает, что какая-то из них разбирает
	 *          не то же самое - например, отдаёт значение заголовка вместе с
	 *          окружающими пробелами или пропускает часть тела
	 *
	 * @param argc длина массива параметров
	 * @param argv массив параметров
	 *
	 */
	static inline void digest(const int32_t argc, char ** argv) noexcept {
		/**
		 * Перебираем параметры запуска стенда
		 */
		for(int32_t i = 1; i < argc; i++){
			// Если запрошен вывод контрольной суммы обработанных данных
			if(::strcmp(argv[i], "--checksum") == 0){
				// Выводим контрольную сумму обработанных данных
				::printf("контрольная сумма: %zu\n", static_cast <size_t> (checksum));
				// Выходим из функции
				return;
			}
		}
	}
	/**
	 * @brief Функция получения фильтра названий сценариев из параметров запуска
	 *
	 * @param argc длина массива параметров
	 * @param argv массив параметров
	 * @return     фильтр названий сценариев
	 *
	 */
	static inline const char * filter(const int32_t argc, char ** argv) noexcept {
		/**
		 * Перебираем параметры запуска стенда
		 */
		for(int32_t i = 1; i < argc; i++){
			// Если задан фильтр названий выполняемых сценариев
			if(::strncmp(argv[i], "--filter=", 9) == 0)
				// Выводим фильтр названий сценариев
				return (argv[i] + 9);
		}
		// Выводим отсутствие фильтра названий сценариев
		return nullptr;
	}
	/**
	 * @brief Перечисление измеряемых характеристик сценария
	 *
	 */
	enum class metric_t : uint8_t {
		MESSAGES    = 0x01, // Количество сообщений в секунду
		THROUGHPUT  = 0x02, // Пропускная способность в мебибайтах в секунду
		ALLOCATIONS = 0x03  // Количество выделений памяти на одно сообщение
	};
	/**
	 * @brief Функция выполнения сценария сравнения
	 *
	 * @note Прогон вынесен в общее окружение намеренно: порядок прогрева,
	 *       границы замера и способ подачи обязаны совпадать у всех
	 *       сравниваемых реализаций, а повторение логики в каждом стенде
	 *       рано или поздно даёт расхождение
	 *
	 * @tparam Subject разбираемая реализация
	 *
	 * @param name     название сценария
	 * @param metric   измеряемая характеристика сценария
	 * @param subject  разбираемая реализация
	 * @param message  эталонное сообщение
	 * @param rounds   количество разбираемых сообщений
	 * @param fragment размер фрагмента подачи (0 - сообщение подаётся целиком)
	 * @param mask     фильтр названий сценариев
	 *
	 */
	template <typename Subject>
	static void execute(const char * name, const metric_t metric, Subject & subject, const std::string & message, const size_t rounds, const size_t fragment, const char * mask) noexcept {
		// Если название сценария не соответствует фильтру
		if(!selected(name, mask))
			// Выходим без выполнения сценария
			return;
		// Итоги прогона сценария
		outcome_t outcome;
		// Если сообщение разобрано с ошибкой
		if(!parsing(subject, message, rounds, fragment, outcome)){
			// Выводим сообщение о неудачном прогоне сценария
			skip(name, "прогон не выполнен: сообщение разобрано с ошибкой");
			// Выходим из сценария
			return;
		}
		/**
		 * Определяем измеряемую характеристику сценария
		 */
		switch(static_cast <uint8_t> (metric)){
			// Если измеряется количество сообщений в секунду
			case static_cast <uint8_t> (metric_t::MESSAGES):
				// Выводим результат прогона сценария
				report(name, "сообщений/с", perSecond(outcome), outcome);
			break;
			// Если измеряется пропускная способность
			case static_cast <uint8_t> (metric_t::THROUGHPUT):
				// Выводим результат прогона сценария
				report(name, "МБ/с", megabytes(outcome), outcome);
			break;
			// Если измеряется количество выделений памяти
			case static_cast <uint8_t> (metric_t::ALLOCATIONS):
				// Выводим результат прогона сценария
				report(name, "выделений", allocated(outcome), outcome);
			break;
		}
	}
};

/**
 * @brief Оператор выделения памяти с учётом статистики
 *
 * @note Оператор подменяется на уровне программы, поэтому заголовочный файл
 *       подключается ровно одной единицей трансляции каждого стенда. Учёт
 *       ведётся только по операторам языка: сравниваемые реализации на языке C
 *       динамическую память не выделяют вовсе - хранение разобранного сообщения
 *       возложено на вызывающую сторону, и это свойство отчёт как раз и фиксирует
 *
 * @param size размер выделяемой памяти
 * @return     указатель на выделенную память
 *
 */
void * operator new (size_t size){
	// Если учёт выделений памяти активен
	if(rival::counter::enabled){
		// Считаем выполненное выделение памяти
		rival::counter::count++;
		// Суммируем объём выделенной памяти
		rival::counter::bytes += size;
	}
	// Выполняем выделение памяти
	void * result = ::malloc(size > 0 ? size : 1);
	// Если память не выделена
	if(result == nullptr)
		// Завершаем приложение - обработка нехватки памяти в стенде не предусмотрена
		::abort();
	// Выводим указатель на выделенную память
	return result;
}
/**
 * @brief Оператор выделения памяти под массив с учётом статистики
 *
 * @param size размер выделяемой памяти
 * @return     указатель на выделенную память
 *
 */
void * operator new [] (size_t size){
	// Выполняем выделение памяти
	return operator new (size);
}
/**
 * @brief Оператор освобождения памяти
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete (void * ptr) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
/**
 * @brief Оператор освобождения памяти массива
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete [] (void * ptr) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
/**
 * @brief Оператор освобождения памяти с указанием размера
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete (void * ptr, size_t) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
/**
 * @brief Оператор освобождения памяти массива с указанием размера
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete [] (void * ptr, size_t) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}

#endif // __AWH_BENCHMARK_RIVAL_HTTP1__
