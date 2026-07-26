/**
 * @file: throughput.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения пропускной способности протокола HTTP/1.x —
 *        разбор входящих сообщений на разных нагрузках, сборка исходящих сообщений
 *        и учёт выделений памяти в установившемся потоковом режиме
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <string>
#include <cstdio>
#include <cstdint>

/**
 * Подключаем заголовочный файл бенчмарков протокола HTTP/1.x
 */
#include "http1.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Внутренние параметры и сценарии бенчмарков пропускной способности
 *
 */
namespace {
	/**
	 * @brief Размер тела сообщения сценариев с телом
	 *
	 */
	static constexpr size_t BODY_SIZE = (8 * 1024 * 1024);
	/**
	 * @brief Размер одного чанка сценариев с кодировкой chunked
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
	 * @brief Количество сообщений сценариев сборки исходящих сообщений
	 *
	 */
	static constexpr size_t SEND_ROUNDS = 20000;
	/**
	 * @brief Порог скорости разбора запроса без заголовков в сообщениях в секунду
	 *
	 * @details Пороги пропускной способности зависят от машины и режима сборки:
	 *          библиотека собирается без флагов оптимизации и медленнее
	 *          оптимизированной в несколько раз. Поэтому пороги откалиброваны
	 *          по неоптимизированной сборке с четырёхкратным запасом - они ловят
	 *          регрессии на порядок, а не колебания окружения
	 *
	 */
	static constexpr double TINY_THRESHOLD = 900000.0;
	/**
	 * @brief Порог скорости разбора запроса браузера в мегабайтах в секунду
	 *
	 */
	static constexpr double TYPICAL_THRESHOLD = 75.0;
	/**
	 * @brief Порог скорости разбора при побайтовой подаче в мегабайтах в секунду
	 *
	 * @details Сценарий проверяет стоимость потокового контракта в худшем случае:
	 *          каждый байт подаётся отдельным вызовом, и постоянные накладные
	 *          расходы на вызов разбора не амортизируются вовсе
	 *
	 */
	static constexpr double FRAGMENT_THRESHOLD = 11.0;
	/**
	 * @brief Порог скорости разбора тела фиксированного размера в мегабайтах в секунду
	 *
	 */
	static constexpr double IDENTITY_THRESHOLD = 400.0;
	/**
	 * @brief Порог скорости разбора тела в кодировке chunked в мегабайтах в секунду
	 *
	 */
	static constexpr double CHUNKED_THRESHOLD = 400.0;
	/**
	 * @brief Порог скорости сборки исходящего сообщения в мегабайтах в секунду
	 *
	 */
	static constexpr double SEND_THRESHOLD = 5000.0;
	/**
	 * @brief Порог количества выделений памяти на одно разобранное сообщение
	 *
	 * @details Ограничение сверху: парсер копит имена и значения заголовков в
	 *          переиспользуемые накопители, поэтому в установившемся режиме
	 *          keep-alive выделений быть не должно вовсе. В отличие от пропускной
	 *          способности показатель от машины и режима сборки не зависит,
	 *          поэтому порог задан вплотную к измеренному значению
	 *
	 */
	static constexpr double PARSE_ALLOCATIONS_THRESHOLD = 0.5;
	/**
	 * @brief Порог количества выделений памяти на одно отправленное сообщение
	 *
	 * @details Ограничение сверху на сборку исходящего сообщения. Измеренное
	 *          значение - два выделения на сообщение, и оба приходятся на
	 *          формирование контейнера заголовков и его сериализацию, а не на
	 *          буферы отправителя: они выделенную память переиспользуют.
	 *          Показательный пример регрессии, которую ловит порог: передача
	 *          выходного буфера сетевому слою через временный объект вместо
	 *          обмена смартбуферами освобождает выделенную память на каждой
	 *          передаче, и следующая запись начинает рост буфера заново -
	 *          на сообщение с восьмью порциями тела это даёт свыше десяти
	 *          выделений вместо двух
	 *
	 */
	static constexpr double SEND_ALLOCATIONS_THRESHOLD = 4.0;

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
	 * @brief Функция прогона разбора эталонного сообщения
	 *
	 * @param message  эталонное сообщение
	 * @param rounds   количество разбираемых сообщений
	 * @param fragment размер фрагмента подачи (0 - сообщение подаётся целиком)
	 * @param consume  признак чтения тела сообщения потребителем
	 * @param output   итоги прогона
	 * @return         результат прогона (false - сообщение разобрано с ошибкой)
	 *
	 */
	static bool parsing(const string & message, const size_t rounds, const size_t fragment, const bool consume, outcome_t & output) noexcept {
		// Создаём объект парсера запросов клиента
		parser_http_t parser(direct_t::REQUEST, awh::benchmark::http1::fmk(), awh::benchmark::http1::log());
		// Контрольная сумма принятых данных
		static volatile size_t checksum = 0;
		// Если потребитель читает тело сообщения
		if(consume){
			// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
			parser.on(parser_http_t::data_callback_t([](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
				// Сумма октетов принятого фрагмента тела
				size_t sum = 0;
				// Получаем байтовый указатель на фрагмент тела
				const uint8_t * data = static_cast <const uint8_t *> (buffer);
				/**
				 * Читаем фрагмент тела целиком: без чтения замерялась бы не пропускная
				 * способность, а скорость перешагивания через данные - парсер отдаёт
				 * тело без копирования и сам его не трогает
				 */
				for(size_t i = 0; i < size; i++)
					// Суммируем очередной октет фрагмента тела
					sum += data[i];
				// Накапливаем контрольную сумму принятых данных
				checksum = (checksum + sum);
				// Продолжаем разбор
				return true;
			}));
		}
		// Устанавливаем функцию обратного вызова обработки заголовков сообщения
		parser.on(parser_http_t::header_callback_t([](const uint32_t, const string_view name, const string_view value, const parser_t::part_t) noexcept -> bool {
			// Накапливаем контрольную сумму разобранных заголовков
			checksum = (checksum + name.size() + value.size());
			// Продолжаем разбор
			return true;
		}));
		/**
		 * Выполняем прогрев: первые сообщения выводят накопители парсера на рабочий
		 * объём, и их выделения памяти к установившемуся режиму отношения не имеют
		 */
		for(size_t i = 0; i < 4; i++){
			// Выполняем сброс парсера для разбора следующего сообщения
			parser.reset();
			// Выполняем разбор эталонного сообщения
			parser.parse(message.data(), message.size());
			// Если сообщение разобрано с ошибкой
			if(parser.status() != parser_t::status_t::COMPLETE)
				// Выводим отрицательный результат
				return false;
		}
		// Включаем учёт выделений памяти
		awh::benchmark::counting(true);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем разбор эталонного сообщения требуемое количество раз
		 */
		for(size_t i = 0; i < rounds; i++){
			// Выполняем сброс парсера для разбора следующего сообщения
			parser.reset();
			// Если сообщение подаётся целиком
			if(fragment == 0)
				// Выполняем разбор эталонного сообщения одним вызовом
				parser.parse(message.data(), message.size());
			// Если сообщение подаётся фрагментами
			else {
				/**
				 * Выполняем подачу сообщения фрагментами заданного размера
				 */
				for(size_t offset = 0; offset < message.size(); offset += fragment)
					// Выполняем разбор очередного фрагмента сообщения
					parser.parse(message.data() + offset, ::min(fragment, (message.size() - offset)));
			}
		}
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		// Отключаем учёт выделений памяти
		awh::benchmark::counting(false);
		// Получаем статистику выделений памяти
		awh::benchmark::allocations(output.allocations, output.allocated);
		// Если последнее сообщение разобрано с ошибкой
		if(parser.status() != parser_t::status_t::COMPLETE)
			// Выводим отрицательный результат
			return false;
		// Устанавливаем количество обработанных сообщений
		output.messages = rounds;
		// Устанавливаем количество обработанных октетов
		output.bytes = (rounds * message.size());
		// Устанавливаем затраченное время
		output.seconds = std::chrono::duration <double> (finish - start).count();
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция прогона сборки исходящего сообщения с телом в кодировке chunked
	 *
	 * @param rounds количество собираемых сообщений
	 * @param output итоги прогона
	 * @return       результат прогона (false - сообщение собрано некорректно)
	 *
	 */
	static bool sending(const size_t rounds, outcome_t & output) noexcept {
		// Создаём объект парсера-отправителя ответа
		parser_http_t sender(direct_t::RESPONSE, awh::benchmark::http1::fmk(), awh::benchmark::http1::log());
		// Количество отданных сетевому слою октетов
		size_t written = 0;
		// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
		sender.on(parser_http_t::write_callback_t([&written](const void *, const size_t size) noexcept {
			// Учитываем отданные сетевому слою октеты
			written += size;
		}));
		// Формируем порцию тела исходящего сообщения
		const string portion(CHUNK_SIZE, 'z');
		// Количество порций тела на одно сообщение
		static constexpr size_t PORTIONS = 8;
		/**
		 * Выполняем прогрев: первые сообщения выводят выходной буфер отправителя
		 * на рабочий объём, и их выделения памяти к установившемуся режиму отношения не имеют
		 */
		for(size_t i = 0; i < 4; i++){
			// Формируем контейнер заголовков ответа с провайдером
			headers_t response(make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
			// Отправляем заголовки ответа (тело последует)
			sender.sendHeaders(response, false);
			/**
			 * Отправляем тело ответа порциями
			 */
			for(size_t j = 0; j < PORTIONS; j++)
				// Отправляем очередную порцию тела ответа
				sender.sendData(portion.data(), portion.size(), (j == (PORTIONS - 1)));
		}
		// Сбрасываем количество отданных сетевому слою октетов
		written = 0;
		// Включаем учёт выделений памяти
		awh::benchmark::counting(true);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем сборку исходящего сообщения требуемое количество раз
		 */
		for(size_t i = 0; i < rounds; i++){
			// Формируем контейнер заголовков ответа с провайдером
			headers_t response(make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
			// Отправляем заголовки ответа (тело последует)
			sender.sendHeaders(response, false);
			/**
			 * Отправляем тело ответа порциями
			 */
			for(size_t j = 0; j < PORTIONS; j++)
				// Отправляем очередную порцию тела ответа
				sender.sendData(portion.data(), portion.size(), (j == (PORTIONS - 1)));
		}
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		// Отключаем учёт выделений памяти
		awh::benchmark::counting(false);
		// Получаем статистику выделений памяти
		awh::benchmark::allocations(output.allocations, output.allocated);
		// Если тело исходящих сообщений отдано сетевому слою не полностью
		if(written < (rounds * PORTIONS * CHUNK_SIZE))
			// Выводим отрицательный результат
			return false;
		// Устанавливаем количество обработанных сообщений
		output.messages = rounds;
		// Устанавливаем количество обработанных октетов
		output.bytes = written;
		// Устанавливаем затраченное время
		output.seconds = std::chrono::duration <double> (finish - start).count();
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция формирования сведений о прогоне
	 *
	 * @param outcome итоги прогона
	 * @return        сведения о прогоне
	 *
	 */
	static string details(const outcome_t & outcome) noexcept {
		// Буфер сведений о прогоне
		char result[256];
		// Формируем сведения о прогоне
		::snprintf(
			result, sizeof(result),
			"сообщений: %zu, выделений: %zu (%.2f на сообщение), выделено: %.1f МБ",
			outcome.messages, outcome.allocations,
			(outcome.messages > 0 ? (static_cast <double> (outcome.allocations) / static_cast <double> (outcome.messages)) : 0.0),
			(static_cast <double> (outcome.allocated) / (1024.0 * 1024.0))
		);
		// Выводим сведения о прогоне
		return result;
	}
	/**
	 * @brief Функция формирования результата измерения скорости разбора в сообщениях в секунду
	 *
	 * @param message эталонное сообщение
	 * @param rounds  количество разбираемых сообщений
	 * @return        результат измерения
	 *
	 */
	static awh::benchmark::result_t messagesPerSecond(const string & message, const size_t rounds) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона разбора
		outcome_t outcome;
		// Выполняем прогон разбора эталонного сообщения
		if(!::parsing(message, rounds, 0, false, outcome)){
			// Устанавливаем сведения о неудачном прогоне
			result.details = "прогон не выполнен: сообщение разобрано с ошибкой";
			// Выводим результат измерения
			return result;
		}
		// Вычисляем скорость разбора в сообщениях в секунду
		result.value = ((outcome.seconds > 0.0) ? (static_cast <double> (outcome.messages) / outcome.seconds) : 0.0);
		// Устанавливаем сведения о прогоне
		result.details = ::details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция формирования результата измерения скорости разбора в мегабайтах в секунду
	 *
	 * @param message  эталонное сообщение
	 * @param rounds   количество разбираемых сообщений
	 * @param fragment размер фрагмента подачи (0 - сообщение подаётся целиком)
	 * @param consume  признак чтения тела сообщения потребителем
	 * @return         результат измерения
	 *
	 */
	static awh::benchmark::result_t throughput(const string & message, const size_t rounds, const size_t fragment, const bool consume) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона разбора
		outcome_t outcome;
		// Выполняем прогон разбора эталонного сообщения
		if(!::parsing(message, rounds, fragment, consume, outcome)){
			// Устанавливаем сведения о неудачном прогоне
			result.details = "прогон не выполнен: сообщение разобрано с ошибкой";
			// Выводим результат измерения
			return result;
		}
		// Объём обработанных данных в мегабайтах
		const double megabytes = (static_cast <double> (outcome.bytes) / (1024.0 * 1024.0));
		// Вычисляем пропускную способность в мегабайтах в секунду
		result.value = ((outcome.seconds > 0.0) ? (megabytes / outcome.seconds) : 0.0);
		// Устанавливаем сведения о прогоне
		result.details = ::details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция формирования результата измерения количества выделений памяти на разобранное сообщение
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t parseAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона разбора
		outcome_t outcome;
		// Выполняем прогон разбора эталонного запроса браузера
		if(!::parsing(awh::benchmark::http1::typical(), TYPICAL_ROUNDS, 0, false, outcome)){
			// Устанавливаем сведения о неудачном прогоне
			result.details = "прогон не выполнен: сообщение разобрано с ошибкой";
			// Устанавливаем заведомо превышающее порог значение
			result.value = 1000.0;
			// Выводим результат измерения
			return result;
		}
		// Вычисляем количество выделений памяти на одно сообщение
		result.value = ((outcome.messages > 0) ? (static_cast <double> (outcome.allocations) / static_cast <double> (outcome.messages)) : 0.0);
		// Устанавливаем сведения о прогоне
		result.details = ::details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция формирования результата измерения скорости сборки исходящего сообщения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t sendThroughput() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона сборки
		outcome_t outcome;
		// Выполняем прогон сборки исходящих сообщений
		if(!::sending(SEND_ROUNDS, outcome)){
			// Устанавливаем сведения о неудачном прогоне
			result.details = "прогон не выполнен: тело отдано сетевому слою не полностью";
			// Выводим результат измерения
			return result;
		}
		// Объём обработанных данных в мегабайтах
		const double megabytes = (static_cast <double> (outcome.bytes) / (1024.0 * 1024.0));
		// Вычисляем пропускную способность в мегабайтах в секунду
		result.value = ((outcome.seconds > 0.0) ? (megabytes / outcome.seconds) : 0.0);
		// Устанавливаем сведения о прогоне
		result.details = ::details(outcome);
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция формирования результата измерения количества выделений памяти на отправленное сообщение
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t sendAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона сборки
		outcome_t outcome;
		// Выполняем прогон сборки исходящих сообщений
		if(!::sending(SEND_ROUNDS, outcome)){
			// Устанавливаем сведения о неудачном прогоне
			result.details = "прогон не выполнен: тело отдано сетевому слою не полностью";
			// Устанавливаем заведомо превышающее порог значение
			result.value = 1000.0;
			// Выводим результат измерения
			return result;
		}
		// Вычисляем количество выделений памяти на одно сообщение
		result.value = ((outcome.messages > 0) ? (static_cast <double> (outcome.allocations) / static_cast <double> (outcome.messages)) : 0.0);
		// Устанавливаем сведения о прогоне
		result.details = ::details(outcome);
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий разбора запроса без заголовков
	static const bool gTiny = awh::benchmark::add(
		"http1/parse/tiny-request", "сообщений/с", TINY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::messagesPerSecond(awh::benchmark::http1::tiny(), TINY_ROUNDS); }
	);
	// Регистрируем сценарий разбора запроса браузера
	static const bool gTypical = awh::benchmark::add(
		"http1/parse/typical-request", "МБ/с", TYPICAL_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::throughput(awh::benchmark::http1::typical(), TYPICAL_ROUNDS, 0, false); }
	);
	// Регистрируем сценарий разбора запроса браузера при побайтовой подаче
	static const bool gFragmented = awh::benchmark::add(
		"http1/parse/fragmented-request", "МБ/с", FRAGMENT_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::throughput(awh::benchmark::http1::typical(), FRAGMENT_ROUNDS, 1, false); }
	);
	// Регистрируем сценарий разбора тела фиксированного размера
	static const bool gIdentity = awh::benchmark::add(
		"http1/parse/identity-body", "МБ/с", IDENTITY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::throughput(awh::benchmark::http1::identity(BODY_SIZE), BODY_ROUNDS, 0, true); }
	);
	// Регистрируем сценарий разбора тела в кодировке chunked
	static const bool gChunked = awh::benchmark::add(
		"http1/parse/chunked-body", "МБ/с", CHUNKED_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::throughput(awh::benchmark::http1::chunked(BODY_SIZE, CHUNK_SIZE), BODY_ROUNDS, 0, true); }
	);
	// Регистрируем сценарий количества выделений памяти на разобранное сообщение
	static const bool gParseAllocations = awh::benchmark::add(
		"http1/allocations/per-parsed-message", "выделений", PARSE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::parseAllocations
	);
	// Регистрируем сценарий сборки исходящего сообщения
	static const bool gSend = awh::benchmark::add(
		"http1/send/chunked-stream", "МБ/с", SEND_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::sendThroughput
	);
	// Регистрируем сценарий количества выделений памяти на отправленное сообщение
	static const bool gSendAllocations = awh::benchmark::add(
		"http1/allocations/per-sent-message", "выделений", SEND_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::sendAllocations
	);
};
