/**
 * @file http1.cpp
 * @date 2026-07-28
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
 * @brief Нагрузочная проверка парсера HTTP/1.x на больших объёмах данных
 *
 * @details Проверяется три свойства, которые на малых сообщениях не проявляются:
 *          доставка тела до байта, ограниченность потребления памяти и отсутствие
 *          роста памяти от сообщения к сообщению. Тело не хранится целиком ни на
 *          одной стороне: оно порождается и сверяется позиционной функцией, иначе
 *          измерялось бы потребление памяти самой проверкой
 *
 * @copyright Copyright © 2026
 *
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

#include <sys/resource.h>

#include <proto/http/parser/http1/http.hpp>

using namespace awh::http;

namespace {
	// Размер порции подачи данных приёмнику
	static constexpr size_t FEED = (64 * 1024);
	/**
	 * @brief Функция получения октета тела по его позиции
	 *
	 * @details Тело порождается позиционной функцией, а не хранится: проверка
	 *          многогигабайтного тела массивом измеряла бы память проверки
	 *
	 * @param position позиция октета в теле
	 * @return         октет тела
	 *
	 */
	static inline uint8_t octet(const uint64_t position) noexcept {
		// Выводим октет тела, зависящий от позиции нелинейно
		return static_cast <uint8_t> ((position * 31u) ^ (position >> 7));
	}
	/**
	 * @brief Функция получения пикового потребления памяти процессом
	 *
	 * @return пиковое потребление памяти в мебибайтах
	 *
	 */
	static double peakMemory() noexcept {
		// Сведения о потреблении ресурсов процессом
		struct rusage usage;
		// Получаем сведения о потреблении ресурсов
		if(::getrusage(RUSAGE_SELF, &usage) != 0)
			// Выводим отсутствие сведений
			return 0.0;
		/**
		 * На macOS ru_maxrss измеряется в байтах, на Linux - в килобайтах
		 */
		#ifdef __APPLE__
			return (static_cast <double> (usage.ru_maxrss) / (1024.0 * 1024.0));
		#else
			return (static_cast <double> (usage.ru_maxrss) / 1024.0);
		#endif
	}
	/**
	 * @brief Структура итогов одного прогона
	 *
	 */
	typedef struct Outcome {
		// Признак успешного прогона
		bool ok;
		// Количество доставленных октетов тела
		uint64_t bytes;
		// Количество расхождений содержимого тела
		uint64_t mismatches;
		// Пиковое потребление памяти в мебибайтах
		double memory;
		// Затраченное время в секундах
		double seconds;
		// Пояснение к неуспешному прогону
		std::string reason;
	} outcome_t;
}

/**
 * @brief Функция приёма тела фиксированного размера
 *
 * @param fmk    объект фреймворка
 * @param log    объект логирования
 * @param volume объём тела сообщения
 * @return       итоги прогона
 *
 */
static outcome_t receiveIdentity(const awh::fmk_t * fmk, const awh::log_t * log, const uint64_t volume) noexcept {
	// Итоги прогона
	outcome_t result{true, 0, 0, 0.0, 0.0, ""};
	// Создаём объект парсера-приёмника ответа
	parser_http_t parser(direct_t::RESPONSE, fmk, log);
	// Поднимаем предел размера тела до проверяемого объёма
	parser_http_t::limits_t limits;
	// Устанавливаем предел размера тела
	limits.maxBodySize = (volume + 1);
	// Применяем лимиты безопасности разбора
	parser.limits(limits);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	parser.method(method_t::GET);
	// Позиция сверки принятого тела
	uint64_t position = 0;
	// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
	parser.on(parser_http_t::data_callback_t([&result, &position](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Получаем указатель на принятые октеты тела
		const uint8_t * data = static_cast <const uint8_t *> (buffer);
		/**
		 * Сверяем принятые октеты тела с ожидаемыми
		 */
		for(size_t i = 0; i < size; i++){
			// Если принятый октет не совпал с ожидаемым
			if(data[i] != ::octet(position + i))
				// Считаем расхождение содержимого тела
				result.mismatches++;
		}
		// Сдвигаем позицию сверки принятого тела
		position += size;
		// Учитываем доставленные октеты тела
		result.bytes += size;
		// Продолжаем разбор
		return true;
	}));
	// Формируем блок заголовков ответа
	const std::string head = ("HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(volume) + "\r\n\r\n");
	// Запоминаем момент начала прогона
	const auto start = std::chrono::steady_clock::now();
	// Подаём приёмнику блок заголовков ответа
	parser.parse(head.data(), head.size());
	// Формируем буфер подачи тела
	std::vector <uint8_t> feed(FEED);
	// Позиция подачи тела
	uint64_t sent = 0;
	/**
	 * Подаём тело сообщения порциями
	 */
	while(sent < volume){
		// Определяем размер очередной порции подачи
		const size_t size = static_cast <size_t> (std::min <uint64_t> (FEED, (volume - sent)));
		/**
		 * Заполняем буфер подачи очередной порцией тела
		 */
		for(size_t i = 0; i < size; i++)
			// Формируем очередной октет порции тела
			feed[i] = ::octet(sent + i);
		// Подаём приёмнику очередную порцию тела
		parser.parse(feed.data(), size);
		// Сдвигаем позицию подачи тела
		sent += size;
	}
	// Запоминаем момент окончания прогона
	const auto finish = std::chrono::steady_clock::now();
	// Устанавливаем затраченное время
	result.seconds = std::chrono::duration <double> (finish - start).count();
	// Устанавливаем пиковое потребление памяти
	result.memory = ::peakMemory();
	// Если сообщение разобрано не полностью
	if(parser.status() != parser_t::status_t::COMPLETE){
		// Формируем пояснение к неуспешному прогону
		result.reason = ("сообщение не разобрано полностью: " + std::string(parser.errorName()));
		// Отмечаем неуспешный прогон
		result.ok = false;
	// Если доставлено не всё тело
	} else if(result.bytes != volume) {
		// Формируем пояснение к неуспешному прогону
		result.reason = ("доставлено " + std::to_string(result.bytes) + " из " + std::to_string(volume));
		// Отмечаем неуспешный прогон
		result.ok = false;
	// Если содержимое тела разошлось с ожидаемым
	} else if(result.mismatches > 0) {
		// Формируем пояснение к неуспешному прогону
		result.reason = ("расхождений содержимого: " + std::to_string(result.mismatches));
		// Отмечаем неуспешный прогон
		result.ok = false;
	}
	// Выводим итоги прогона
	return result;
}

/**
 * @brief Функция приёма тела в кодировке chunked
 *
 * @param fmk    объект фреймворка
 * @param log    объект логирования
 * @param volume объём тела сообщения
 * @param chunk  размер одного чанка тела
 * @return       итоги прогона
 *
 */
static outcome_t receiveChunked(const awh::fmk_t * fmk, const awh::log_t * log, const uint64_t volume, const size_t chunk) noexcept {
	// Итоги прогона
	outcome_t result{true, 0, 0, 0.0, 0.0, ""};
	// Создаём объект парсера-приёмника ответа
	parser_http_t parser(direct_t::RESPONSE, fmk, log);
	// Поднимаем пределы размера тела и размера чанка до проверяемого объёма
	parser_http_t::limits_t limits;
	// Устанавливаем предел размера тела
	limits.maxBodySize = (volume + 1);
	// Устанавливаем предел размера чанка
	limits.maxChunkSize = (chunk + 1);
	// Применяем лимиты безопасности разбора
	parser.limits(limits);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	parser.method(method_t::GET);
	// Позиция сверки принятого тела
	uint64_t position = 0;
	// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
	parser.on(parser_http_t::data_callback_t([&result, &position](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Получаем указатель на принятые октеты тела
		const uint8_t * data = static_cast <const uint8_t *> (buffer);
		/**
		 * Сверяем принятые октеты тела с ожидаемыми
		 */
		for(size_t i = 0; i < size; i++){
			// Если принятый октет не совпал с ожидаемым
			if(data[i] != ::octet(position + i))
				// Считаем расхождение содержимого тела
				result.mismatches++;
		}
		// Сдвигаем позицию сверки принятого тела
		position += size;
		// Учитываем доставленные октеты тела
		result.bytes += size;
		// Продолжаем разбор
		return true;
	}));
	// Формируем блок заголовков ответа
	const std::string head = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n";
	// Запоминаем момент начала прогона
	const auto start = std::chrono::steady_clock::now();
	// Подаём приёмнику блок заголовков ответа
	parser.parse(head.data(), head.size());
	// Формируем буфер подачи чанка
	std::vector <uint8_t> feed(chunk + 32);
	// Позиция подачи тела
	uint64_t sent = 0;
	// Текстовый буфер строки размера чанка
	char line[32];
	/**
	 * Подаём тело сообщения чанками
	 */
	while(sent < volume){
		// Определяем размер очередного чанка тела
		const size_t size = static_cast <size_t> (std::min <uint64_t> (chunk, (volume - sent)));
		// Формируем строку размера чанка
		const int32_t length = ::snprintf(line, sizeof(line), "%zX\r\n", size);
		// Подаём приёмнику строку размера чанка
		parser.parse(line, static_cast <size_t> (length));
		/**
		 * Заполняем буфер подачи очередным чанком тела
		 */
		for(size_t i = 0; i < size; i++)
			// Формируем очередной октет чанка тела
			feed[i] = ::octet(sent + i);
		// Подаём приёмнику данные чанка
		parser.parse(feed.data(), size);
		// Подаём приёмнику окончание данных чанка
		parser.parse("\r\n", 2);
		// Сдвигаем позицию подачи тела
		sent += size;
	}
	// Подаём приёмнику завершающий нулевой чанк
	parser.parse("0\r\n\r\n", 5);
	// Запоминаем момент окончания прогона
	const auto finish = std::chrono::steady_clock::now();
	// Устанавливаем затраченное время
	result.seconds = std::chrono::duration <double> (finish - start).count();
	// Устанавливаем пиковое потребление памяти
	result.memory = ::peakMemory();
	// Если сообщение разобрано не полностью
	if(parser.status() != parser_t::status_t::COMPLETE){
		// Формируем пояснение к неуспешному прогону
		result.reason = ("сообщение не разобрано полностью: " + std::string(parser.errorName()));
		// Отмечаем неуспешный прогон
		result.ok = false;
	// Если доставлено не всё тело
	} else if(result.bytes != volume) {
		// Формируем пояснение к неуспешному прогону
		result.reason = ("доставлено " + std::to_string(result.bytes) + " из " + std::to_string(volume));
		// Отмечаем неуспешный прогон
		result.ok = false;
	// Если содержимое тела разошлось с ожидаемым
	} else if(result.mismatches > 0) {
		// Формируем пояснение к неуспешному прогону
		result.reason = ("расхождений содержимого: " + std::to_string(result.mismatches));
		// Отмечаем неуспешный прогон
		result.ok = false;
	}
	// Выводим итоги прогона
	return result;
}

/**
 * @brief Функция сборки исходящего тела большого объёма
 *
 * @details Собранные байты не накапливаются, а разбираются встречным приёмником
 *          на лету: накопление многогигабайтного провода измеряло бы память
 *          проверки. Приёмник сверяет тело до октета и служит той же цели, что
 *          обратная разбираемость в генераторе - собранное обязано разбираться
 *          в отправленное
 *
 * @param fmk    объект фреймворка
 * @param log    объект логирования
 * @param volume объём тела сообщения
 * @param source признак подачи тела pull-источником вместо sendData
 * @param chunked признак кадрирования тела методом chunked
 * @return       итоги прогона
 *
 */
static outcome_t sendLarge(const awh::fmk_t * fmk, const awh::log_t * log, const uint64_t volume, const bool source, const bool chunked) noexcept {
	// Итоги прогона
	outcome_t result{true, 0, 0, 0.0, 0.0, ""};
	// Создаём объект парсера-отправителя ответа
	parser_http_t sender(direct_t::RESPONSE, fmk, log);
	// Создаём объект парсера-приёмника собираемого ответа
	parser_http_t receiver(direct_t::RESPONSE, fmk, log);
	// Поднимаем пределы приёмника до проверяемого объёма
	parser_http_t::limits_t limits;
	// Устанавливаем предел размера тела
	limits.maxBodySize = (volume + 1);
	// Устанавливаем предел размера чанка
	limits.maxChunkSize = (volume + 1);
	// Применяем лимиты безопасности разбора
	receiver.limits(limits);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	receiver.method(method_t::GET);
	// Позиция сверки принятого тела
	uint64_t position = 0;
	// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
	receiver.on(parser_http_t::data_callback_t([&result, &position](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Получаем указатель на принятые октеты тела
		const uint8_t * data = static_cast <const uint8_t *> (buffer);
		/**
		 * Сверяем принятые октеты тела с ожидаемыми
		 */
		for(size_t i = 0; i < size; i++){
			// Если принятый октет не совпал с ожидаемым
			if(data[i] != ::octet(position + i))
				// Считаем расхождение содержимого тела
				result.mismatches++;
		}
		// Сдвигаем позицию сверки принятого тела
		position += size;
		// Учитываем доставленные октеты тела
		result.bytes += size;
		// Продолжаем разбор
		return true;
	}));
	// Количество отданных сетевому слою октетов
	uint64_t written = 0;
	// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
	sender.on(parser_http_t::write_callback_t([&receiver, &written](const void * buffer, const size_t size) noexcept {
		// Учитываем отданные сетевому слою октеты
		written += size;
		// Подаём отданные байты встречному приёмнику
		receiver.parse(buffer, size);
	}));
	// Понижаем пороги выходного буфера: потребление памяти обязано ими ограничиваться
	sender.sendWaterMarks(256 * 1024, 64 * 1024);
	// Формируем контейнер заголовков ответа с провайдером
	headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
	// Если тело кадрируется методом chunked
	if(chunked)
		// Объявляем кодирование тела ответа
		response.emplace("Transfer-Encoding", "chunked");
	// Объявляем размер тела ответа
	else response.emplace("Content-Length", std::to_string(volume));
	// Запоминаем момент начала прогона
	const auto start = std::chrono::steady_clock::now();
	// Отправляем заголовки ответа (тело последует)
	sender.sendHeaders(response, false);
	// Позиция выдачи тела сообщения
	uint64_t offset = 0;
	// Размер одной порции выдачи тела
	static constexpr size_t PORTION = (64 * 1024);
	// Если тело подаётся pull-источником
	if(source){
		// Назначаем pull-источник данных тела сообщения
		sender.dataSource(parser_http_t::data_source_callback_t([&offset, volume](const uint32_t, uint8_t * buffer, const size_t cap, bool & eof) noexcept -> int64_t {
			// Определяем размер выдаваемой источником порции тела
			const size_t size = static_cast <size_t> (std::min <uint64_t> (cap, (volume - offset)));
			/**
			 * Заполняем выдаваемую порцию тела
			 */
			for(size_t i = 0; i < size; i++)
				// Формируем очередной октет порции тела
				buffer[i] = ::octet(offset + i);
			// Сдвигаем позицию выдачи тела
			offset += size;
			// Устанавливаем признак достижения конца тела
			eof = (offset >= volume);
			// Выводим размер выданной порции тела
			return static_cast <int64_t> (size);
		}));
		/**
		 * Прокачиваем источник до исчерпания тела
		 */
		while(sender.sourcePending()){
			// Если прокачка источника не возобновилась
			if(!sender.resumeSource())
				// Прекращаем прокачку источника
				break;
		}
	// Если тело выдаётся методом отправки
	} else {
		// Формируем буфер выдачи порции тела
		std::vector <uint8_t> portion(PORTION);
		/**
		 * Выдаём тело сообщения порциями
		 */
		while(offset < volume){
			// Определяем размер выдаваемой порции тела
			const size_t size = static_cast <size_t> (std::min <uint64_t> (PORTION, (volume - offset)));
			/**
			 * Заполняем выдаваемую порцию тела
			 */
			for(size_t i = 0; i < size; i++)
				// Формируем очередной октет порции тела
				portion[i] = ::octet(offset + i);
			// Выполняем выдачу очередной порции тела
			const size_t accepted = sender.sendData(portion.data(), size, ((offset + size) >= volume));
			// Если отправитель порцию не принял
			if(accepted == 0){
				// Формируем пояснение к неуспешному прогону
				result.reason = ("отправитель отверг порцию тела на позиции " + std::to_string(offset));
				// Отмечаем неуспешный прогон
				result.ok = false;
				// Прекращаем выдачу тела
				break;
			}
			// Сдвигаем позицию выдачи тела
			offset += accepted;
		}
	}
	// Запоминаем момент окончания прогона
	const auto finish = std::chrono::steady_clock::now();
	// Устанавливаем затраченное время
	result.seconds = std::chrono::duration <double> (finish - start).count();
	// Устанавливаем пиковое потребление памяти
	result.memory = ::peakMemory();
	// Если прогон уже признан неуспешным
	if(!result.ok)
		// Выводим итоги прогона
		return result;
	// Если встречный приёмник не разобрал сообщение целиком
	if(receiver.status() != parser_t::status_t::COMPLETE){
		// Формируем пояснение к неуспешному прогону
		result.reason = ("собранное сообщение не разобралось обратно: " + std::string(receiver.errorName()));
		// Отмечаем неуспешный прогон
		result.ok = false;
	// Если доставлено не всё тело
	} else if(result.bytes != volume) {
		// Формируем пояснение к неуспешному прогону
		result.reason = ("доставлено " + std::to_string(result.bytes) + " из " + std::to_string(volume));
		// Отмечаем неуспешный прогон
		result.ok = false;
	// Если содержимое тела разошлось с ожидаемым
	} else if(result.mismatches > 0) {
		// Формируем пояснение к неуспешному прогону
		result.reason = ("расхождений содержимого: " + std::to_string(result.mismatches));
		// Отмечаем неуспешный прогон
		result.ok = false;
	}
	// Выводим итоги прогона
	return result;
}

/**
 * @brief Функция приёма тела с обрывом соединения и работой после него
 *
 * @details Обрыв посреди тела обязан дать ошибку незавершённого сообщения, а не
 *          молча выдать усечённое тело за целое: иначе приложение приняло бы
 *          неполный файл за полученный. Доставленное до обрыва обязано совпасть с
 *          отправленным до октета, а объект после очистки - разобрать следующее
 *          сообщение как ни в чём не бывало
 *
 * @param fmk    объект фреймворка
 * @param log    объект логирования
 * @param volume объём тела сообщения
 * @param cycles количество циклов обрыва и восстановления
 * @return       итоги прогона
 *
 */
static outcome_t breakAndRestart(const awh::fmk_t * fmk, const awh::log_t * log, const uint64_t volume, const size_t cycles) noexcept {
	// Итоги прогона
	outcome_t result{true, 0, 0, 0.0, 0.0, ""};
	// Создаём объект парсера-приёмника ответа
	parser_http_t parser(direct_t::RESPONSE, fmk, log);
	// Поднимаем предел размера тела до проверяемого объёма
	parser_http_t::limits_t limits;
	// Устанавливаем предел размера тела
	limits.maxBodySize = (volume + 1);
	// Применяем лимиты безопасности разбора
	parser.limits(limits);
	// Позиция сверки принятого тела
	uint64_t position = 0;
	// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
	parser.on(parser_http_t::data_callback_t([&result, &position](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Получаем указатель на принятые октеты тела
		const uint8_t * data = static_cast <const uint8_t *> (buffer);
		/**
		 * Сверяем принятые октеты тела с ожидаемыми
		 */
		for(size_t i = 0; i < size; i++){
			// Если принятый октет не совпал с ожидаемым
			if(data[i] != ::octet(position + i))
				// Считаем расхождение содержимого тела
				result.mismatches++;
		}
		// Сдвигаем позицию сверки принятого тела
		position += size;
		// Учитываем доставленные октеты тела
		result.bytes += size;
		// Продолжаем разбор
		return true;
	}));
	// Формируем блок заголовков ответа
	const std::string head = ("HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(volume) + "\r\n\r\n");
	// Формируем буфер подачи тела
	std::vector <uint8_t> feed(FEED);
	// Потребление памяти после первого цикла
	double settled = 0.0;
	// Запоминаем момент начала прогона
	const auto start = std::chrono::steady_clock::now();
	/**
	 * Выполняем циклы обрыва соединения и работы после него
	 */
	for(size_t cycle = 0; cycle < cycles; cycle++){
		// Определяем позицию обрыва соединения: она гуляет от цикла к циклу
		const uint64_t cut = ((volume * (30 + (cycle % 60))) / 100);
		// Сбрасываем позицию сверки принятого тела
		position = 0;
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		parser.method(method_t::GET);
		// Подаём приёмнику блок заголовков ответа
		parser.parse(head.data(), head.size());
		// Позиция подачи тела
		uint64_t sent = 0;
		/**
		 * Подаём тело сообщения порциями до позиции обрыва
		 */
		while(sent < cut){
			// Определяем размер очередной порции подачи
			const size_t size = static_cast <size_t> (std::min <uint64_t> (FEED, (cut - sent)));
			/**
			 * Заполняем буфер подачи очередной порцией тела
			 */
			for(size_t i = 0; i < size; i++)
				// Формируем очередной октет порции тела
				feed[i] = ::octet(sent + i);
			// Подаём приёмнику очередную порцию тела
			parser.parse(feed.data(), size);
			// Сдвигаем позицию подачи тела
			sent += size;
		}
		// Сообщаем приёмнику о разрыве соединения
		parser.eof();
		// Если обрыв посреди тела не признан незавершённым сообщением
		if(parser.error() != parser_http_t::error_t::PREMATURE_EOF){
			// Формируем пояснение к неуспешному прогону
			result.reason = ("обрыв посреди тела не признан незавершённым сообщением: " + std::string(parser.errorName()));
			// Отмечаем неуспешный прогон
			result.ok = false;
			// Прекращаем прогон
			break;
		}
		// Если доставлено не ровно то, что успело прийти до обрыва
		if(position != cut){
			// Формируем пояснение к неуспешному прогону
			result.reason = ("до обрыва доставлено " + std::to_string(position) + " вместо " + std::to_string(cut));
			// Отмечаем неуспешный прогон
			result.ok = false;
			// Прекращаем прогон
			break;
		}
		/**
		 * Соединение оборвано - объект возвращается к состоянию нового
		 *
		 * Ровно так с ним поступит обвязка: разорванное соединение закрывается,
		 * а объект уходит обслуживать следующее
		 */
		parser.clear();
		// Восстанавливаем функцию обратного вызова обработки фрагмента тела сообщения
		parser.on(parser_http_t::data_callback_t([&result, &position](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
			// Получаем указатель на принятые октеты тела
			const uint8_t * data = static_cast <const uint8_t *> (buffer);
			/**
			 * Сверяем принятые октеты тела с ожидаемыми
			 */
			for(size_t i = 0; i < size; i++){
				// Если принятый октет не совпал с ожидаемым
				if(data[i] != ::octet(position + i))
					// Считаем расхождение содержимого тела
					result.mismatches++;
			}
			// Сдвигаем позицию сверки принятого тела
			position += size;
			// Учитываем доставленные октеты тела
			result.bytes += size;
			// Продолжаем разбор
			return true;
		}));
		// Восстанавливаем лимиты безопасности разбора
		parser.limits(limits);
		// Запоминаем потребление памяти после первого цикла
		if(cycle == 0)
			// Устанавливаем установившееся потребление памяти
			settled = ::peakMemory();
	}
	// Запоминаем момент окончания прогона
	const auto finish = std::chrono::steady_clock::now();
	// Устанавливаем затраченное время
	result.seconds = std::chrono::duration <double> (finish - start).count();
	// Устанавливаем пиковое потребление памяти
	result.memory = ::peakMemory();
	// Если прогон уже признан неуспешным
	if(!result.ok)
		// Выводим итоги прогона
		return result;
	// Если содержимое доставленного тела разошлось с ожидаемым
	if(result.mismatches > 0){
		// Формируем пояснение к неуспешному прогону
		result.reason = ("расхождений содержимого: " + std::to_string(result.mismatches));
		// Отмечаем неуспешный прогон
		result.ok = false;
		// Выводим итоги прогона
		return result;
	}
	/**
	 * Проверяем пригодность объекта после всех обрывов: он обязан разобрать
	 * следующее сообщение целиком
	 */
	{
		// Сбрасываем позицию сверки принятого тела
		position = 0;
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		parser.method(method_t::GET);
		// Подаём приёмнику блок заголовков ответа
		parser.parse(head.data(), head.size());
		// Позиция подачи тела
		uint64_t sent = 0;
		/**
		 * Подаём тело сообщения целиком
		 */
		while(sent < volume){
			// Определяем размер очередной порции подачи
			const size_t size = static_cast <size_t> (std::min <uint64_t> (FEED, (volume - sent)));
			/**
			 * Заполняем буфер подачи очередной порцией тела
			 */
			for(size_t i = 0; i < size; i++)
				// Формируем очередной октет порции тела
				feed[i] = ::octet(sent + i);
			// Подаём приёмнику очередную порцию тела
			parser.parse(feed.data(), size);
			// Сдвигаем позицию подачи тела
			sent += size;
		}
		// Если сообщение после обрывов разобрано не полностью
		if(parser.status() != parser_t::status_t::COMPLETE){
			// Формируем пояснение к неуспешному прогону
			result.reason = ("после обрывов сообщение не разобралось: " + std::string(parser.errorName()));
			// Отмечаем неуспешный прогон
			result.ok = false;
		// Если тело после обрывов доставлено не целиком
		} else if(position != volume) {
			// Формируем пояснение к неуспешному прогону
			result.reason = ("после обрывов доставлено " + std::to_string(position) + " из " + std::to_string(volume));
			// Отмечаем неуспешный прогон
			result.ok = false;
		// Если содержимое тела после обрывов разошлось с ожидаемым
		} else if(result.mismatches > 0) {
			// Формируем пояснение к неуспешному прогону
			result.reason = ("расхождений содержимого после обрывов: " + std::to_string(result.mismatches));
			// Отмечаем неуспешный прогон
			result.ok = false;
		}
	}
	// Если память выросла от цикла к циклу более чем на мегабайт
	if(result.ok && (settled > 0.0) && ((result.memory - settled) > 1.0)){
		// Формируем пояснение к неуспешному прогону
		result.reason = ("память выросла за циклы с " + std::to_string(settled) + " до " + std::to_string(result.memory) + " МБ");
		// Отмечаем неуспешный прогон
		result.ok = false;
	}
	// Выводим итоги прогона
	return result;
}

/**
 * @brief Функция докачки прерванного тела запросом диапазона
 *
 * @details Прерванная передача возобновляется не парсером, а приложением: оно
 *          запрашивает недостающий диапазон, и сервер отвечает [206 Partial
 *          Content]. Проверяется, что обе части, принятые разными сообщениями,
 *          складываются в исходное тело до октета
 *
 * @param fmk    объект фреймворка
 * @param log    объект логирования
 * @param volume объём тела сообщения
 * @param cycles количество циклов докачки
 * @return       итоги прогона
 *
 */
static outcome_t resumeByRange(const awh::fmk_t * fmk, const awh::log_t * log, const uint64_t volume, const size_t cycles) noexcept {
	// Итоги прогона
	outcome_t result{true, 0, 0, 0.0, 0.0, ""};
	// Позиция сверки принятого тела
	uint64_t position = 0;
	// Формируем буфер подачи тела
	std::vector <uint8_t> feed(FEED);
	// Лимиты безопасности разбора
	parser_http_t::limits_t limits;
	// Устанавливаем предел размера тела
	limits.maxBodySize = (volume + 1);
	/**
	 * @brief Функция подачи приёмнику части тела
	 *
	 * @param parser объект парсера-приёмника
	 * @param from   позиция начала части тела
	 * @param to     позиция конца части тела
	 *
	 */
	auto deliver = [&feed](parser_http_t & parser, const uint64_t from, const uint64_t to) noexcept -> void {
		// Позиция подачи части тела
		uint64_t sent = from;
		/**
		 * Подаём часть тела порциями
		 */
		while(sent < to){
			// Определяем размер очередной порции подачи
			const size_t size = static_cast <size_t> (std::min <uint64_t> (FEED, (to - sent)));
			/**
			 * Заполняем буфер подачи очередной порцией тела
			 */
			for(size_t i = 0; i < size; i++)
				// Формируем очередной октет порции тела
				feed[i] = ::octet(sent + i);
			// Подаём приёмнику очередную порцию тела
			parser.parse(feed.data(), size);
			// Сдвигаем позицию подачи части тела
			sent += size;
		}
	};
	// Запоминаем момент начала прогона
	const auto start = std::chrono::steady_clock::now();
	/**
	 * Выполняем циклы докачки прерванного тела
	 */
	for(size_t cycle = 0; cycle < cycles; cycle++){
		// Определяем позицию обрыва передачи
		const uint64_t cut = ((volume * (20 + (cycle % 70))) / 100);
		// Сбрасываем позицию сверки принятого тела
		position = 0;
		/**
		 * Принимаем первую часть тела до обрыва соединения
		 */
		{
			// Создаём объект парсера-приёмника ответа
			parser_http_t parser(direct_t::RESPONSE, fmk, log);
			// Применяем лимиты безопасности разбора
			parser.limits(limits);
			// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
			parser.method(method_t::GET);
			// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
			parser.on(parser_http_t::data_callback_t([&result, &position](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
				// Получаем указатель на принятые октеты тела
				const uint8_t * data = static_cast <const uint8_t *> (buffer);
				/**
				 * Сверяем принятые октеты тела с ожидаемыми
				 */
				for(size_t i = 0; i < size; i++){
					// Если принятый октет не совпал с ожидаемым
					if(data[i] != ::octet(position + i))
						// Считаем расхождение содержимого тела
						result.mismatches++;
				}
				// Сдвигаем позицию сверки принятого тела
				position += size;
				// Учитываем доставленные октеты тела
				result.bytes += size;
				// Продолжаем разбор
				return true;
			}));
			// Формируем блок заголовков ответа
			const std::string head = ("HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(volume) + "\r\n\r\n");
			// Подаём приёмнику блок заголовков ответа
			parser.parse(head.data(), head.size());
			// Подаём приёмнику первую часть тела
			deliver(parser, 0, cut);
			// Сообщаем приёмнику о разрыве соединения
			parser.eof();
			// Если обрыв посреди тела не признан незавершённым сообщением
			if(parser.error() != parser_http_t::error_t::PREMATURE_EOF){
				// Формируем пояснение к неуспешному прогону
				result.reason = ("обрыв не признан незавершённым сообщением: " + std::string(parser.errorName()));
				// Отмечаем неуспешный прогон
				result.ok = false;
				// Прекращаем прогон
				break;
			}
		}
		/**
		 * Принимаем остаток тела ответом на запрос диапазона
		 */
		{
			// Создаём объект парсера-приёмника ответа нового соединения
			parser_http_t parser(direct_t::RESPONSE, fmk, log);
			// Применяем лимиты безопасности разбора
			parser.limits(limits);
			// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
			parser.method(method_t::GET);
			// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
			parser.on(parser_http_t::data_callback_t([&result, &position](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
				// Получаем указатель на принятые октеты тела
				const uint8_t * data = static_cast <const uint8_t *> (buffer);
				/**
				 * Сверяем принятые октеты тела с ожидаемыми: сверка продолжается с той же
				 * позиции, на которой оборвалась первая часть - обе части обязаны сложиться
				 * в исходное тело
				 */
				for(size_t i = 0; i < size; i++){
					// Если принятый октет не совпал с ожидаемым
					if(data[i] != ::octet(position + i))
						// Считаем расхождение содержимого тела
						result.mismatches++;
				}
				// Сдвигаем позицию сверки принятого тела
				position += size;
				// Учитываем доставленные октеты тела
				result.bytes += size;
				// Продолжаем разбор
				return true;
			}));
			// Формируем блок заголовков ответа на запрос диапазона
			const std::string head = (
				"HTTP/1.1 206 Partial Content\r\nContent-Range: bytes " + std::to_string(cut) + "-" +
				std::to_string(volume - 1) + "/" + std::to_string(volume) + "\r\nContent-Length: " +
				std::to_string(volume - cut) + "\r\n\r\n"
			);
			// Подаём приёмнику блок заголовков ответа
			parser.parse(head.data(), head.size());
			// Подаём приёмнику остаток тела
			deliver(parser, cut, volume);
			// Если ответ на запрос диапазона разобран не полностью
			if(parser.status() != parser_t::status_t::COMPLETE){
				// Формируем пояснение к неуспешному прогону
				result.reason = ("докачка не разобралась: " + std::string(parser.errorName()));
				// Отмечаем неуспешный прогон
				result.ok = false;
				// Прекращаем прогон
				break;
			}
		}
		// Если сложенные части не дали исходного тела
		if(position != volume){
			// Формируем пояснение к неуспешному прогону
			result.reason = ("части сложились в " + std::to_string(position) + " вместо " + std::to_string(volume));
			// Отмечаем неуспешный прогон
			result.ok = false;
			// Прекращаем прогон
			break;
		}
	}
	// Запоминаем момент окончания прогона
	const auto finish = std::chrono::steady_clock::now();
	// Устанавливаем затраченное время
	result.seconds = std::chrono::duration <double> (finish - start).count();
	// Устанавливаем пиковое потребление памяти
	result.memory = ::peakMemory();
	// Если содержимое сложенного тела разошлось с ожидаемым
	if(result.ok && (result.mismatches > 0)){
		// Формируем пояснение к неуспешному прогону
		result.reason = ("расхождений содержимого: " + std::to_string(result.mismatches));
		// Отмечаем неуспешный прогон
		result.ok = false;
	}
	// Выводим итоги прогона
	return result;
}

/**
 * @brief Функция обрыва соединения посреди сборки исходящего сообщения
 *
 * @details Разорванное соединение уносит с собой недоотправленное сообщение.
 *          Бросить его подготовкой к следующему нельзя: resetSender отказывается
 *          и требует закрыть соединение - иначе следующий блок заголовков лёг бы
 *          в чужое тело. Проверяется и отказ, и штатный путь восстановления:
 *          объект возвращается к состоянию нового полной очисткой, после чего
 *          собирает следующее сообщение целиком
 *
 * @param fmk    объект фреймворка
 * @param log    объект логирования
 * @param volume объём тела сообщения
 * @param cycles количество циклов обрыва
 * @return       итоги прогона
 *
 */
static outcome_t senderBreak(const awh::fmk_t * fmk, const awh::log_t * log, const uint64_t volume, const size_t cycles) noexcept {
	// Итоги прогона
	outcome_t result{true, 0, 0, 0.0, 0.0, ""};
	// Создаём объект парсера-отправителя ответа
	parser_http_t sender(direct_t::RESPONSE, fmk, log);
	// Понижаем пороги выходного буфера
	sender.sendWaterMarks(256 * 1024, 64 * 1024);
	/**
	 * Признак накопления отданных байтов
	 *
	 * До разрыва байты только считаются: накопление многомегабайтного тела
	 * измеряло бы память проверки, а сверяется здесь сообщение после разрыва
	 */
	bool collect = false;
	// Собранные байты последнего сообщения
	std::string wire;
	// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
	sender.on(parser_http_t::write_callback_t([&collect, &wire](const void * buffer, const size_t size) noexcept {
		// Если байты накапливаются
		if(collect)
			// Собираем отданные сетевому слою байты
			wire.append(static_cast <const char *> (buffer), size);
	}));
	// Размер одной порции выдачи тела
	static constexpr size_t PORTION = (64 * 1024);
	// Формируем буфер выдачи порции тела
	std::vector <uint8_t> portion(PORTION);
	// Потребление памяти после первого цикла
	double settled = 0.0;
	// Запоминаем момент начала прогона
	const auto start = std::chrono::steady_clock::now();
	/**
	 * Выполняем циклы обрыва соединения посреди сборки
	 */
	for(size_t cycle = 0; cycle < cycles; cycle++){
		// Определяем позицию обрыва соединения
		const uint64_t cut = ((volume * (25 + (cycle % 50))) / 100);
		// До разрыва байты только считаются
		collect = false;
		// Очищаем собранные байты
		wire.clear();
		/**
		 * Собираем сообщение до позиции обрыва соединения
		 */
		{
			// Формируем контейнер заголовков ответа с провайдером
			headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
			// Объявляем размер тела ответа
			response.emplace("Content-Length", std::to_string(volume));
			// Отправляем заголовки ответа (тело последует)
			sender.sendHeaders(response, false);
			// Позиция выдачи тела сообщения
			uint64_t offset = 0;
			/**
			 * Выдаём тело сообщения порциями до позиции обрыва
			 */
			while(offset < cut){
				// Определяем размер выдаваемой порции тела
				const size_t size = static_cast <size_t> (std::min <uint64_t> (PORTION, (cut - offset)));
				// Выполняем выдачу очередной порции тела
				const size_t accepted = sender.sendData(portion.data(), size, false);
				// Если отправитель порцию не принял - прекращаем выдачу
				if(accepted == 0)
					// Прекращаем выдачу тела
					break;
				// Сдвигаем позицию выдачи тела
				offset += accepted;
			}
		}
		/**
		 * Пробуем подготовить отправитель к следующему сообщению
		 *
		 * Незавершённое сообщение бросить нельзя: подготовка обязана отказать,
		 * иначе следующий блок заголовков лёг бы строкой размера чанка либо
		 * продолжением тела в чужое сообщение
		 */
		sender.resetSender();
		// Начинаем накапливать отданные байты
		collect = true;
		// Очищаем собранные байты прежнего соединения
		wire.clear();
		/**
		 * Проверяем что подготовка отказала: собранный после неё блок заголовков
		 * на провод уйти не вправе
		 */
		{
			// Формируем контейнер заголовков ответа с провайдером
			headers_t probe(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (204)));
			// Пробуем отправить заголовки ответа поверх незавершённого сообщения
			sender.sendHeaders(probe, true);
			// Если блок заголовков всё же ушёл на провод
			if(!wire.empty()){
				// Формируем пояснение к неуспешному прогону
				result.reason = ("подготовка отправителя не отказала: на проводе " + std::to_string(wire.size()) + " октетов");
				// Отмечаем неуспешный прогон
				result.ok = false;
				// Прекращаем прогон
				break;
			}
		}
		/**
		 * Возвращаем объект к состоянию нового: разорванное соединение закрывается,
		 * а объект уходит обслуживать следующее
		 */
		sender.clear();
		// Восстанавливаем пороги выходного буфера
		sender.sendWaterMarks(256 * 1024, 64 * 1024);
		// Восстанавливаем функцию обратного вызова записи исходящих байтов в сеть
		sender.on(parser_http_t::write_callback_t([&collect, &wire](const void * buffer, const size_t size) noexcept {
			// Если байты накапливаются
			if(collect)
				// Собираем отданные сетевому слою байты
				wire.append(static_cast <const char *> (buffer), size);
		}));
		// Очищаем собранные байты прежнего соединения
		wire.clear();
		/**
		 * Собираем следующее сообщение целиком на новом соединении
		 */
		{
			// Объём тела следующего сообщения
			static constexpr size_t NEXT = (128 * 1024);
			// Формируем контейнер заголовков ответа с провайдером
			headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
			// Объявляем размер тела ответа
			response.emplace("Content-Length", std::to_string(NEXT));
			// Отправляем заголовки ответа (тело последует)
			sender.sendHeaders(response, false);
			// Позиция выдачи тела сообщения
			size_t offset = 0;
			/**
			 * Выдаём тело следующего сообщения порциями
			 */
			while(offset < NEXT){
				// Определяем размер выдаваемой порции тела
				const size_t size = std::min(PORTION, (NEXT - offset));
				/**
				 * Заполняем выдаваемую порцию тела
				 */
				for(size_t i = 0; i < size; i++)
					// Формируем очередной октет порции тела
					portion[i] = ::octet(offset + i);
				// Выполняем выдачу очередной порции тела
				const size_t accepted = sender.sendData(portion.data(), size, ((offset + size) >= NEXT));
				// Если отправитель порцию не принял
				if(accepted == 0){
					// Формируем пояснение к неуспешному прогону
					result.reason = "после разрыва отправитель не принял тело следующего сообщения";
					// Отмечаем неуспешный прогон
					result.ok = false;
					// Прекращаем выдачу тела
					break;
				}
				// Сдвигаем позицию выдачи тела
				offset += accepted;
			}
			// Если прогон признан неуспешным - прекращаем циклы
			if(!result.ok)
				// Прекращаем прогон
				break;
			// Позиция сверки принятого тела
			uint64_t position = 0;
			// Количество расхождений содержимого следующего сообщения
			uint64_t mismatches = 0;
			// Создаём объект парсера-приёмника собранного сообщения
			parser_http_t receiver(direct_t::RESPONSE, fmk, log);
			// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
			receiver.method(method_t::GET);
			// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
			receiver.on(parser_http_t::data_callback_t([&position, &mismatches](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
				// Получаем указатель на принятые октеты тела
				const uint8_t * data = static_cast <const uint8_t *> (buffer);
				/**
				 * Сверяем принятые октеты тела с ожидаемыми
				 */
				for(size_t i = 0; i < size; i++){
					// Если принятый октет не совпал с ожидаемым
					if(data[i] != ::octet(position + i))
						// Считаем расхождение содержимого тела
						mismatches++;
				}
				// Сдвигаем позицию сверки принятого тела
				position += size;
				// Продолжаем разбор
				return true;
			}));
			// Выполняем разбор собранного после разрыва сообщения
			const size_t consumed = receiver.parse(wire.data(), wire.size());
			// Если собранное после разрыва сообщение не разобралось целиком
			if((receiver.status() != parser_t::status_t::COMPLETE) || (consumed != wire.size())){
				// Формируем пояснение к неуспешному прогону
				result.reason = ("сообщение после разрыва не разобралось: " + std::string(receiver.errorName()) +
				 ", потреблено " + std::to_string(consumed) + " из " + std::to_string(wire.size()));
				// Отмечаем неуспешный прогон
				result.ok = false;
				// Прекращаем прогон
				break;
			}
			// Если тело сообщения после разрыва доставлено не целиком либо искажено
			if((position != NEXT) || (mismatches > 0)){
				// Формируем пояснение к неуспешному прогону
				result.reason = ("тело после разрыва: доставлено " + std::to_string(position) +
				 " из " + std::to_string(NEXT) + ", расхождений " + std::to_string(mismatches));
				// Отмечаем неуспешный прогон
				result.ok = false;
				// Прекращаем прогон
				break;
			}
			// Учитываем доставленные октеты тела
			result.bytes += (cut + NEXT);
		}
		// Запоминаем потребление памяти после первого цикла
		if(cycle == 0)
			// Устанавливаем установившееся потребление памяти
			settled = ::peakMemory();
	}
	// Запоминаем момент окончания прогона
	const auto finish = std::chrono::steady_clock::now();
	// Устанавливаем затраченное время
	result.seconds = std::chrono::duration <double> (finish - start).count();
	// Устанавливаем пиковое потребление памяти
	result.memory = ::peakMemory();
	// Если память выросла от цикла к циклу более чем на мегабайт
	if(result.ok && (settled > 0.0) && ((result.memory - settled) > 1.0)){
		// Формируем пояснение к неуспешному прогону
		result.reason = ("память выросла за циклы с " + std::to_string(settled) + " до " + std::to_string(result.memory) + " МБ");
		// Отмечаем неуспешный прогон
		result.ok = false;
	}
	// Выводим итоги прогона
	return result;
}

/**
 * @brief Функция входа в проверку
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из проверки
 *
 */
int32_t main(int32_t argc, char ** argv) noexcept {
	// Объём тела проверяемых сообщений в мебибайтах
	const uint64_t volume = ((argc > 1) ? ::strtoull(argv[1], nullptr, 10) : 1024) * 1024ull * 1024ull;
	// Количество циклов обрыва соединения
	const size_t cycles = ((argc > 2) ? static_cast <size_t> (::strtoull(argv[2], nullptr, 10)) : 32);
	// Создаём объект фреймворка
	awh::fmk_t fmk;
	// Создаём объект логирования
	awh::log_t log(&fmk);
	// Отключаем вывод сообщений парсера
	log.level(awh::log_t::level_t::NONE);
	// Потребление памяти до начала прогонов
	const double before = ::peakMemory();
	// Признак неуспешного прогона
	bool failed = false;
	/**
	 * @brief Функция вывода итогов одного прогона
	 *
	 * @param name    название прогона
	 * @param outcome итоги прогона
	 *
	 */
	auto report = [&failed, volume](const char * name, const outcome_t & outcome) noexcept -> void {
		// Выводим итоги прогона
		::printf(
			"%-28s %6.2f ГБ за %6.2f с (%7.0f МБ/с), пик памяти %6.1f МБ  %s%s\n",
			name, (static_cast <double> (volume) / (1024.0 * 1024.0 * 1024.0)), outcome.seconds,
			((outcome.seconds > 0.0) ? ((static_cast <double> (outcome.bytes) / (1024.0 * 1024.0)) / outcome.seconds) : 0.0),
			outcome.memory, (outcome.ok ? "ОК" : "СБОЙ: "), outcome.reason.c_str()
		);
		// Если прогон оказался неуспешным
		if(!outcome.ok)
			// Отмечаем неуспешный прогон
			failed = true;
	};
	// Выполняем приём тела фиксированного размера
	report("приём identity", ::receiveIdentity(&fmk, &log, volume));
	// Выполняем приём тела в кодировке chunked порциями по 16 КиБ
	report("приём chunked 16К", ::receiveChunked(&fmk, &log, volume, (16 * 1024)));
	// Выполняем приём тела в кодировке chunked порциями по 1 МиБ
	report("приём chunked 1М", ::receiveChunked(&fmk, &log, volume, (1024 * 1024)));
	// Выполняем сборку тела фиксированного размера методом отправки
	report("сборка identity push", ::sendLarge(&fmk, &log, volume, false, false));
	// Выполняем сборку тела в кодировке chunked методом отправки
	report("сборка chunked push", ::sendLarge(&fmk, &log, volume, false, true));
	// Выполняем сборку тела фиксированного размера pull-источником
	report("сборка identity pull", ::sendLarge(&fmk, &log, volume, true, false));
	// Выполняем сборку тела в кодировке chunked pull-источником
	report("сборка chunked pull", ::sendLarge(&fmk, &log, volume, true, true));
	// Выполняем циклы обрыва соединения посреди приёма тела
	report("обрыв приёма", ::breakAndRestart(&fmk, &log, (volume / cycles), cycles));
	// Выполняем циклы докачки прерванного тела запросом диапазона
	report("докачка 206", ::resumeByRange(&fmk, &log, (volume / cycles), cycles));
	// Выполняем циклы обрыва соединения посреди сборки
	report("обрыв сборки", ::senderBreak(&fmk, &log, (volume / cycles), cycles));
	// Выводим итоговое потребление памяти
	::printf(
		"\nпамять: до прогонов %.1f МБ, после %.1f МБ (прирост %.1f МБ на %.0f ГБ данных)\n",
		before, ::peakMemory(), (::peakMemory() - before),
		((static_cast <double> (volume) * 7.0) / (1024.0 * 1024.0 * 1024.0))
	);
	// Выводим код выхода
	return (failed ? 1 : 0);
}
