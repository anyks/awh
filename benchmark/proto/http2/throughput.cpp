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
 * @brief Сценарии измерения пропускной способности протокола HTTP/2 — разбор потока
 *        запросов, приём тела с учётом окон управления потоком, полный обмен через
 *        пару парсеров и стоимость мультиплексирования множества потоков
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <string>
#include <vector>
#include <cstdio>

/**
 * Подключаем заголовочный файл бенчмарков протокола HTTP/2
 */
#include "http2.hpp"

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
	 * @brief Количество запросов сценария разбора потока запросов
	 *
	 */
	static constexpr size_t REQUEST_ROUNDS = 100000;
	/**
	 * @brief Количество обменов сценариев полного цикла запрос-ответ
	 *
	 */
	static constexpr size_t ROUNDTRIP_ROUNDS = 50000;
	/**
	 * @brief Объём тела сценария приёма данных в октетах
	 *
	 */
	static constexpr size_t BODY_SIZE = (64 * 1024 * 1024);
	/**
	 * @brief Размер кадра данных сценария приёма данных
	 *
	 * @details Совпадает со значением SETTINGS_MAX_FRAME_SIZE по умолчанию: именно
	 *          такими кадрами нагружает соединение любой согласованный пир
	 *
	 */
	static constexpr size_t FRAME_SIZE = 16384;
	/**
	 * @brief Количество одновременных потоков сценария мультиплексирования
	 *
	 */
	static constexpr size_t STREAM_COUNT = 64;
	/**
	 * @brief Размер порции подачи входящего потока в октетах
	 *
	 * @details Поток подаётся порциями размером с типичное чтение из сокета, а не
	 *          целиком: подача одним куском переложила бы на парсер буферизацию всего
	 *          объёма и завысила бы показатель - в реальной работе такого не бывает
	 *
	 */
	static constexpr size_t CHUNK_SIZE = (64 * 1024);
	/**
	 * @brief Порог скорости разбора потока запросов в запросах в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке с четырёхкратным запасом:
	 *          они ловят регрессии на порядок, а не колебания окружения. Показательный
	 *          пример такой регрессии - удаление разобранного префикса из начала
	 *          входного буфера вместо продвижения курсора: стоимость становится
	 *          квадратичной по объёму подачи
	 *
	 */
	static constexpr double REQUEST_THRESHOLD = 41000.0;
	/**
	 * @brief Порог скорости приёма тела в мегабайтах в секунду
	 *
	 * @details Ловит регрессии учёта окон управления потоком: если приёмное окно
	 *          перестанет пополняться крупными шагами, передача выродится в обмен
	 *          кадр-обновление и показатель упадёт на порядок
	 *
	 */
	static constexpr double BODY_THRESHOLD = 8000.0;
	/**
	 * @brief Порог скорости полного обмена в обменах в секунду
	 *
	 */
	static constexpr double ROUNDTRIP_THRESHOLD = 12000.0;
	/**
	 * @brief Порог скорости полного обмена по множеству потоков
	 *
	 * @details Отдельный порог для мультиплексирования: планировщик отправки
	 *          упорядочивает потоки по срочности на каждом проходе, поэтому
	 *          стоимость одного обмена здесь заведомо выше
	 *
	 */
	static constexpr double MULTIPLEX_THRESHOLD = 12000.0;
	/**
	 * @brief Порог количества выделений памяти на разобранный запрос
	 *
	 * @details Разбор запроса создаёт объект потока и провайдер заголовков, поэтому
	 *          выделения неизбежны. Ограничение сверху ловит появление выделения
	 *          на каждый заголовок либо на каждый кадр
	 *
	 */
	static constexpr double REQUEST_ALLOCATIONS_THRESHOLD = 2.5;
	/**
	 * @brief Порог количества выделений памяти на кадр данных
	 *
	 * @details Приём тела в установившемся режиме выделений требовать не должен:
	 *          входной буфер переиспользуется, а тело отдаётся приложению
	 *          представлением без копирования
	 *
	 */
	static constexpr double BODY_ALLOCATIONS_THRESHOLD = 0.5;

	/**
	 * @brief Структура итогов прогона обмена
	 *
	 */
	typedef struct Transfer {
		// Количество выполненных операций
		size_t operations;
		// Количество принятых октетов
		size_t received;
		// Количество переданных кадров
		size_t frames;
		// Затраченное время в секундах
		double seconds;
		// Количество выполненных выделений памяти
		size_t allocations;
		// Суммарный объём выделенной памяти в октетах
		size_t bytes;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Transfer() noexcept :
		 operations(0), received(0), frames(0), seconds(0.0), allocations(0), bytes(0) {}
	} transfer_t;

	/**
	 * @brief Функция прогона разбора потока запросов
	 *
	 * @param counting признак учёта выделений памяти
	 * @param output   итоги прогона
	 * @return         результат прогона
	 *
	 */
	static bool requests(const bool counting, transfer_t & output) noexcept {
		// Создаём объект кодера заголовков клиента
		h2::hpack::encoder_t encoder;
		// Буфер входящего потока байт
		string input(awh::benchmark::http2::preface());
		// Резервируем память под поток запросов
		input.reserve(REQUEST_ROUNDS * 128);
		/**
		 * Готовим поток запросов заранее: кодирование заголовков клиентом
		 * к измеряемой работе сервера отношения не имеет
		 */
		for(size_t i = 0; i < REQUEST_ROUNDS; i++){
			// Буфер закодированного блока заголовков
			string block;
			// Кодируем блок заголовков запроса
			encoder.encode(awh::benchmark::http2::request(i), block, true);
			// Дописываем кадр заголовков с завершением потока
			input += awh::benchmark::http2::frame(0x01, 0x05, static_cast <uint32_t> (1 + (i * 2)), block);
			// Считаем сформированный кадр
			output.frames++;
		}
		// Создаём объект парсера сервера
		parser_http2_t server(direct_t::REQUEST, awh::benchmark::http2::fmk(), awh::benchmark::http2::log());
		// Количество разобранных запросов
		size_t parsed = 0;
		// Устанавливаем функцию обратного вызова записи исходящих байт
		server.on(parser_http2_t::write_callback_t([](const void *, const size_t) noexcept {}));
		// Минимальный ответ сервера: заголовки без тела
		vector <h2::hpack::field_t> answer;
		// Дописываем псевдо-заголовок статуса ответа
		answer.emplace_back(":status", "200");
		// Дописываем заголовок длины содержимого
		answer.emplace_back("content-length", "0");
		/**
		 * Устанавливаем функцию обратного вызова провайдера заголовков. Сервер обязан
		 * ответить: без ответа поток остаётся полуоткрытым, такие потоки копятся
		 * и упираются в лимит одновременных потоков - ровно как у настоящего сервера
		 */
		server.on(parser_http2_t::provider_callback_t([&](const uint32_t sid, const provider_t * provider, const bool) noexcept -> bool {
			// Если получен провайдер запроса
			if(provider != nullptr){
				// Считаем разобранный запрос
				parsed++;
				// Отправляем минимальный ответ с завершением потока
				server.sendHeaders(sid, answer, true);
			}
			// Продолжаем разбор
			return true;
		}));
		// Отправляем preface сервера
		server.sendPreface();
		// Если требуется учёт выделений памяти
		if(counting)
			// Включаем учёт выделений памяти
			awh::benchmark::counting(true);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Подаём поток запросов порциями размером с чтение из сокета
		 */
		for(size_t offset = 0; offset < input.size(); offset += CHUNK_SIZE)
			// Подаём очередную порцию потока на разбор
			server.parse(input.data() + offset, ::min(CHUNK_SIZE, (input.size() - offset)));
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		// Если выполнялся учёт выделений памяти
		if(counting){
			// Отключаем учёт выделений памяти
			awh::benchmark::counting(false);
			// Получаем статистику выделений памяти
			awh::benchmark::allocations(output.allocations, output.bytes);
		}
		// Устанавливаем количество разобранных запросов
		output.operations = parsed;
		// Устанавливаем объём разобранного потока
		output.received = input.size();
		// Устанавливаем затраченное время
		output.seconds = std::chrono::duration <double> (finish - start).count();
		// Выводим результат прогона
		return (parsed == REQUEST_ROUNDS);
	}
	/**
	 * @brief Функция прогона приёма тела сообщения
	 *
	 * @param counting признак учёта выделений памяти
	 * @param output   итоги прогона
	 * @return         результат прогона
	 *
	 */
	static bool body(const bool counting, transfer_t & output) noexcept {
		// Создаём объект кодера заголовков клиента
		h2::hpack::encoder_t encoder;
		// Формируем заголовки запроса с телом
		vector <h2::hpack::field_t> fields;
		// Дописываем псевдо-заголовок метода запроса
		fields.emplace_back(":method", "POST");
		// Дописываем псевдо-заголовок схемы запроса
		fields.emplace_back(":scheme", "https");
		// Дописываем псевдо-заголовок пути запроса
		fields.emplace_back(":path", "/upload");
		// Дописываем псевдо-заголовок авторитета запроса
		fields.emplace_back(":authority", "www.example.com");
		// Буфер закодированного блока заголовков
		string block;
		// Кодируем блок заголовков запроса
		encoder.encode(fields, block, true);
		// Буфер входящего потока байт
		string input(awh::benchmark::http2::preface());
		// Резервируем память под поток целиком
		input.reserve(BODY_SIZE + (BODY_SIZE / FRAME_SIZE) * 16 + 256);
		// Дописываем кадр заголовков запроса
		input += awh::benchmark::http2::frame(0x01, 0x04, 1, block);
		// Полезная нагрузка одного кадра данных
		const string payload(FRAME_SIZE, 'x');
		// Количество кадров данных потока
		const size_t total = (BODY_SIZE / FRAME_SIZE);
		/**
		 * Дописываем кадры данных: последний кадр завершает поток
		 */
		for(size_t i = 0; i < total; i++){
			// Дописываем кадр данных
			input += awh::benchmark::http2::frame(0x00, ((i + 1 == total) ? 0x01 : 0x00), 1, payload);
			// Считаем сформированный кадр
			output.frames++;
		}
		// Создаём объект парсера сервера
		parser_http2_t server(direct_t::REQUEST, awh::benchmark::http2::fmk(), awh::benchmark::http2::log());
		// Объём принятого тела
		size_t accepted = 0;
		// Устанавливаем функцию обратного вызова записи исходящих байт
		server.on(parser_http2_t::write_callback_t([](const void *, const size_t) noexcept {}));
		// Устанавливаем функцию обратного вызова тела сообщения
		server.on(parser_http2_t::data_callback_t([&](const uint32_t, const void *, const size_t size, const bool) noexcept -> bool {
			// Суммируем объём принятого тела
			accepted += size;
			// Продолжаем разбор
			return true;
		}));
		// Отправляем preface сервера
		server.sendPreface();
		// Если требуется учёт выделений памяти
		if(counting)
			// Включаем учёт выделений памяти
			awh::benchmark::counting(true);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Подаём поток порциями размером с чтение из сокета
		 */
		for(size_t offset = 0; offset < input.size(); offset += CHUNK_SIZE)
			// Подаём очередную порцию потока на разбор
			server.parse(input.data() + offset, ::min(CHUNK_SIZE, (input.size() - offset)));
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		// Если выполнялся учёт выделений памяти
		if(counting){
			// Отключаем учёт выделений памяти
			awh::benchmark::counting(false);
			// Получаем статистику выделений памяти
			awh::benchmark::allocations(output.allocations, output.bytes);
		}
		// Устанавливаем количество принятых октетов
		output.received = accepted;
		// Устанавливаем количество выполненных операций
		output.operations = total;
		// Устанавливаем затраченное время
		output.seconds = std::chrono::duration <double> (finish - start).count();
		// Выводим результат прогона
		return (accepted == BODY_SIZE);
	}
	// Описание ошибки уровня соединения последнего прогона
	static string gFailure;

	/**
	 * @brief Функция получения описания ошибки последнего прогона
	 *
	 * @return описание ошибки уровня соединения
	 *
	 */
	static const char * failureReason() noexcept {
		// Выводим описание ошибки уровня соединения
		return gFailure.c_str();
	}
	/**
	 * @brief Функция прогона полного обмена через пару парсеров
	 *
	 * @param streams количество одновременных потоков
	 * @param output  итоги прогона
	 * @return        результат прогона
	 *
	 */
	static bool roundtrip(const size_t streams, transfer_t & output) noexcept {
		// Создаём объект парсера клиента
		parser_http2_t client(direct_t::RESPONSE, awh::benchmark::http2::fmk(), awh::benchmark::http2::log());
		// Создаём объект парсера сервера
		parser_http2_t server(direct_t::REQUEST, awh::benchmark::http2::fmk(), awh::benchmark::http2::log());
		// Заголовки ответа сервера, подготовленные заранее
		const vector <h2::hpack::field_t> answer = awh::benchmark::http2::response(0);
		// Количество принятых сервером запросов
		size_t handled = 0;
		// Количество принятых клиентом ответов
		size_t completed = 0;
		// Соединяем парсеры каналами записи: исходящие байты одного разбирает другой
		client.on(parser_http2_t::write_callback_t([&](const void * buffer, const size_t size) noexcept {
			// Подаём исходящие байты клиента на разбор серверу
			server.parse(buffer, size);
		}));
		// Устанавливаем функцию обратного вызова записи исходящих байт сервера
		server.on(parser_http2_t::write_callback_t([&](const void * buffer, const size_t size) noexcept {
			// Подаём исходящие байты сервера на разбор клиенту
			client.parse(buffer, size);
		}));
		// Устанавливаем функцию обратного вызова провайдера заголовков сервера
		server.on(parser_http2_t::provider_callback_t([&](const uint32_t sid, const provider_t * provider, const bool) noexcept -> bool {
			// Если получен провайдер запроса
			if(provider != nullptr){
				// Считаем принятый запрос
				handled++;
				// Отправляем заголовки ответа
				server.sendHeaders(sid, answer, false);
				// Отправляем тело ответа с завершением потока
				server.sendData(sid, awh::benchmark::http2::payload().data(), awh::benchmark::http2::payload().size(), true);
			}
			// Продолжаем разбор
			return true;
		}));
		// Устанавливаем функцию обратного вызова провайдера заголовков клиента
		client.on(parser_http2_t::provider_callback_t([&](const uint32_t, const provider_t * provider, const bool) noexcept -> bool {
			// Если получен провайдер ответа - считаем завершённый обмен
			if(provider != nullptr)
				// Считаем завершённый обмен
				completed++;
			// Продолжаем разбор
			return true;
		}));
		// Очищаем описание ошибки уровня соединения
		gFailure.clear();
		// Устанавливаем функцию обратного вызова ошибки уровня соединения клиента
		client.on(parser_http2_t::error_callback_t([&](const parser_http2_t::error_t, const std::string_view message) noexcept {
			// Запоминаем описание первой ошибки
			if(gFailure.empty())
				// Запоминаем описание ошибки клиента
				gFailure = ("клиент: " + std::string(message));
		}));
		// Устанавливаем функцию обратного вызова ошибки уровня соединения сервера
		server.on(parser_http2_t::error_callback_t([&](const parser_http2_t::error_t, const std::string_view message) noexcept {
			// Запоминаем описание первой ошибки
			if(gFailure.empty())
				// Запоминаем описание ошибки сервера
				gFailure = ("сервер: " + std::string(message));
		}));
		// Отправляем preface клиента
		client.sendPreface();
		// Отправляем preface сервера
		server.sendPreface();
		/**
		 * Готовим наборы заголовков заранее: сборка набора - работа приложения,
		 * и в замер стоимости обмена попадать не должна
		 */
		vector <vector <h2::hpack::field_t>> requests;
		// Резервируем память под наборы заголовков запросов
		requests.reserve(64);
		/**
		 * Выполняем формирование всех наборов заголовков запросов
		 */
		for(size_t i = 0; i < 64; i++)
			// Дописываем очередной набор заголовков запроса
			requests.push_back(awh::benchmark::http2::request(i));
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем обмены пачками по числу одновременных потоков: пачка отражает
		 * мультиплексирование, при котором планировщик обслуживает потоки вперемешку
		 */
		for(size_t i = 0; i < ROUNDTRIP_ROUNDS; i += streams){
			// Вычисляем размер очередной пачки обменов
			const size_t portion = ::min(streams, (ROUNDTRIP_ROUNDS - i));
			/**
			 * Открываем потоки пачки
			 */
			for(size_t j = 0; j < portion; j++){
				// Выделяем идентификатор нового потока клиента
				const uint32_t sid = client.nextStreamId();
				// Если пространство идентификаторов исчерпано
				if(sid == 0)
					// Прекращаем прогон
					return false;
				// Отправляем заголовки запроса с завершением потока
				client.sendHeaders(sid, requests[(i + j) % requests.size()], true);
				// Считаем переданный кадр
				output.frames++;
			}
		}
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		// Устанавливаем количество выполненных обменов
		output.operations = completed;
		// Устанавливаем затраченное время
		output.seconds = std::chrono::duration <double> (finish - start).count();
		// Выводим результат прогона
		return (completed == ROUNDTRIP_ROUNDS);
	}
	/**
	 * @brief Функция формирования результата измерения разбора потока запросов
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t requestThroughput() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона
		transfer_t transfer;
		// Выполняем прогон разбора потока запросов
		if(!::requests(false, transfer)){
			// Буфер сведений о неудачном прогоне
			char reason[128];
			// Формируем сведения о неудачном прогоне
			::snprintf(reason, sizeof(reason), "прогон не выполнен: разобрано %zu запросов из %zu", transfer.operations, REQUEST_ROUNDS);
			// Устанавливаем сведения о неудачном прогоне
			result.details = reason;
			// Выводим результат измерения
			return result;
		}
		// Вычисляем скорость разбора потока запросов
		result.value = ((transfer.seconds > 0.0) ? (static_cast <double> (transfer.operations) / transfer.seconds) : 0.0);
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details), "запросов: %zu, объём потока: %.1f МБ (%.1f октетов на запрос), %.1f МБ/с",
			transfer.operations, (static_cast <double> (transfer.received) / (1024.0 * 1024.0)),
			(static_cast <double> (transfer.received) / static_cast <double> (transfer.operations)),
			((transfer.seconds > 0.0) ? ((static_cast <double> (transfer.received) / (1024.0 * 1024.0)) / transfer.seconds) : 0.0)
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция формирования результата измерения приёма тела
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t bodyThroughput() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона
		transfer_t transfer;
		// Выполняем прогон приёма тела
		if(!::body(false, transfer)){
			// Устанавливаем сведения о неудачном прогоне
			result.details = "прогон не выполнен: тело принято не полностью";
			// Выводим результат измерения
			return result;
		}
		// Объём принятого тела в мегабайтах
		const double megabytes = (static_cast <double> (transfer.received) / (1024.0 * 1024.0));
		// Вычисляем скорость приёма тела
		result.value = ((transfer.seconds > 0.0) ? (megabytes / transfer.seconds) : 0.0);
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details), "кадров: %zu, принято: %.1f МБ, кадров/с: %.0f",
			transfer.operations, megabytes,
			((transfer.seconds > 0.0) ? (static_cast <double> (transfer.operations) / transfer.seconds) : 0.0)
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция формирования результата измерения полного обмена
	 *
	 * @param streams количество одновременных потоков
	 * @return        результат измерения
	 *
	 */
	static awh::benchmark::result_t roundtripThroughput(const size_t streams) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона
		transfer_t transfer;
		// Выполняем прогон полного обмена
		if(!::roundtrip(streams, transfer)){
			// Буфер сведений о неудачном прогоне
			char reason[128];
			// Формируем сведения о неудачном прогоне
			::snprintf(reason, sizeof(reason), "прогон не выполнен: завершено %zu обменов из %zu [%s]", transfer.operations, ROUNDTRIP_ROUNDS, ::failureReason());
			// Устанавливаем сведения о неудачном прогоне
			result.details = reason;
			// Выводим результат измерения
			return result;
		}
		// Вычисляем скорость полного обмена
		result.value = ((transfer.seconds > 0.0) ? (static_cast <double> (transfer.operations) / transfer.seconds) : 0.0);
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details), "обменов: %zu, одновременных потоков: %zu, кадров: %zu",
			transfer.operations, streams, transfer.frames
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция измерения количества выделений памяти на разобранный запрос
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t requestAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона
		transfer_t transfer;
		// Выполняем прогон разбора потока запросов с учётом выделений памяти
		if(!::requests(true, transfer)){
			// Устанавливаем сведения о неудачном прогоне
			result.details = "прогон не выполнен: разобраны не все запросы";
			// Устанавливаем заведомо превышающее порог значение
			result.value = 1000.0;
			// Выводим результат измерения
			return result;
		}
		// Вычисляем количество выделений памяти на разобранный запрос
		result.value = (static_cast <double> (transfer.allocations) / static_cast <double> (transfer.operations));
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details), "запросов: %zu, выделений: %zu, выделено: %.1f МБ (%.1f октетов на запрос)",
			transfer.operations, transfer.allocations,
			(static_cast <double> (transfer.bytes) / (1024.0 * 1024.0)),
			(static_cast <double> (transfer.bytes) / static_cast <double> (transfer.operations))
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция измерения количества выделений памяти на кадр данных
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t bodyAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона
		transfer_t transfer;
		// Выполняем прогон приёма тела с учётом выделений памяти
		if(!::body(true, transfer)){
			// Устанавливаем сведения о неудачном прогоне
			result.details = "прогон не выполнен: тело принято не полностью";
			// Устанавливаем заведомо превышающее порог значение
			result.value = 1000.0;
			// Выводим результат измерения
			return result;
		}
		// Вычисляем количество выделений памяти на кадр данных
		result.value = (static_cast <double> (transfer.allocations) / static_cast <double> (transfer.operations));
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details), "кадров: %zu, выделений: %zu, выделено: %.1f МБ",
			transfer.operations, transfer.allocations,
			(static_cast <double> (transfer.bytes) / (1024.0 * 1024.0))
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий разбора потока запросов
	static const bool gRequests = awh::benchmark::add(
		"http2/parse/request-stream", "запросов/с", REQUEST_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::requestThroughput
	);
	// Регистрируем сценарий приёма тела сообщения
	static const bool gBody = awh::benchmark::add(
		"http2/parse/data-body", "МБ/с", BODY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bodyThroughput
	);
	// Регистрируем сценарий полного обмена по одному потоку
	static const bool gRoundtrip = awh::benchmark::add(
		"http2/session/round-trip", "обменов/с", ROUNDTRIP_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::roundtripThroughput(1); }
	);
	// Регистрируем сценарий полного обмена по множеству потоков
	static const bool gMultiplex = awh::benchmark::add(
		"http2/session/multiplexed", "обменов/с", MULTIPLEX_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::roundtripThroughput(STREAM_COUNT); }
	);
	// Регистрируем сценарий количества выделений памяти на разобранный запрос
	static const bool gRequestAllocations = awh::benchmark::add(
		"http2/allocations/per-request", "выделений", REQUEST_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::requestAllocations
	);
	// Регистрируем сценарий количества выделений памяти на кадр данных
	static const bool gBodyAllocations = awh::benchmark::add(
		"http2/allocations/per-data-frame", "выделений", BODY_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::bodyAllocations
	);
};
