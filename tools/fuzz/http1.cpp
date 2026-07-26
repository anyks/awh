/**
 * @file: http1.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Структурный генератор нештатного трафика для парсера протокола HTTP/1.x
 *        с дифференциальной сверкой путей разбора
 *
 * @details Проверяется инвариант, на котором держится весь модуль: наблюдаемый
 *          результат разбора не зависит от того, как вход разбит на фрагменты.
 *          Инвариант нетривиален, потому что размер фрагмента переключает пути:
 *          крупноблочная обработка стартовой строки, целой строки заголовка,
 *          участков токена, значения и тела срабатывает только тогда, когда
 *          разбираемый участок присутствует во входном буфере целиком, а при
 *          подаче по одному октету не срабатывает ни разу. Эталоном служит
 *          посимвольная подача - самый простой и самый медленный путь
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <tuple>
#include <memory>
#include <utility>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <proto/http/parser/http1/http.hpp>

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Внутреннее окружение генератора
 *
 */
namespace {
	/**
	 * @brief Состояние генератора псевдослучайных чисел
	 *
	 * @details Зерно фиксировано намеренно: найденный дефект обязан повторяться
	 *          при том же числе итераций, иначе его невозможно ни воспроизвести,
	 *          ни проверить исправление
	 *
	 */
	static uint64_t gSeed = 0x9E3779B97F4A7C15ull;
	/**
	 * @brief Функция получения очередного псевдослучайного числа
	 *
	 * @note Название намеренно отличается от `random`: одноимённая функция есть в
	 *       стандартной библиотеке C, и обращение вида `::random()` разрешалось бы
	 *       в неё, оставляя генератор мёртвым кодом
	 *
	 * @return псевдослучайное число
	 *
	 */
	static uint64_t sequent() noexcept {
		// Выполняем перемешивание состояния генератора
		gSeed += 0x9E3779B97F4A7C15ull;
		// Получаем текущее значение состояния
		uint64_t result = gSeed;
		// Выполняем финальное перемешивание разрядов
		result = ((result ^ (result >> 30)) * 0xBF58476D1CE4E5B9ull);
		result = ((result ^ (result >> 27)) * 0x94D049BB133111EBull);
		// Выводим псевдослучайное число
		return (result ^ (result >> 31));
	}
	/**
	 * @brief Функция получения псевдослучайного числа в заданном диапазоне
	 *
	 * @param bound верхняя граница диапазона (не включается)
	 * @return      псевдослучайное число
	 *
	 */
	static size_t pick(const size_t bound) noexcept {
		// Выводим псевдослучайное число в заданном диапазоне
		return static_cast <size_t> (::sequent() % (bound > 0 ? bound : 1));
	}
	/**
	 * @brief Функция проверки срабатывания события с заданной вероятностью
	 *
	 * @param percent вероятность события в процентах
	 * @return        результат проверки
	 *
	 */
	static bool chance(const size_t percent) noexcept {
		// Выводим результат проверки срабатывания события
		return (::pick(100) < percent);
	}
	/**
	 * @brief Структура сборщика наблюдаемого результата разбора
	 *
	 * @details Сравнивается всё, что парсер отдаёт наружу: итог разбора, разобранная
	 *          стартовая строка, поток событий и собранные данные
	 *
	 * @note Два расхождения в сравнение намеренно не входят, и оба относятся только
	 *       к отвергнутым сообщениям. Первое - количество обработанных октетов:
	 *       крупноблочные пути возвращают начало участка, а посимвольный - позицию
	 *       недопустимого октета. Второе - содержимое недособранной стартовой
	 *       строки: посимвольный путь дописывает адрес запроса вплоть до самого
	 *       превышения лимита, а крупноблочный проверяет лимит до записи участка и
	 *       не дописывает ничего. Оба относятся к состоянию, которое по контракту
	 *       модуля после ошибки недействительно, поэтому стартовая строка сверяется
	 *       только у полностью разобранных сообщений
	 *
	 */
	typedef struct Outcome {
		// Итоговый статус разбора
		parser_t::status_t status;
		// Код ошибки разбора
		parser_http_t::error_t error;
		// Признак полноты разобранного сообщения
		bool complete;
		// Признак кодирования тела методом chunked
		bool chunked;
		// Размер тела сообщения
		int64_t bodySize;
		// Разобранная стартовая строка в текстовом виде
		std::string startLine;
		// Собранное тело сообщения
		std::string body;
		// Собранные заголовки сообщения
		std::vector <std::pair <std::string, std::string>> headers;
		// Собранные трейлеры сообщения
		std::vector <std::pair <std::string, std::string>> trailers;
		// Собранные фазовые события
		std::vector <std::pair <parser_t::phase_t, parser_t::part_t>> phases;
		// Собранные события границ чанков
		std::vector <std::tuple <parser_t::phase_t, uint64_t, std::string>> chunks;
		/**
		 * @brief Оператор сравнения результатов разбора
		 *
		 * @param other сравниваемый результат разбора
		 * @return      результат сравнения
		 *
		 */
		bool operator != (const struct Outcome & other) const noexcept {
			// Выводим результат сравнения результатов разбора
			return (
				(this->status != other.status) || (this->error != other.error) ||
				(this->complete != other.complete) || (this->chunked != other.chunked) ||
				(this->bodySize != other.bodySize) ||
				((this->status == parser_t::status_t::COMPLETE) && (this->startLine != other.startLine)) ||
				(this->body != other.body) || (this->headers != other.headers) ||
				(this->trailers != other.trailers) || (this->phases != other.phases) ||
				(this->chunks != other.chunks)
			);
		}
	} outcome_t;
	/**
	 * @brief Структура настроек разбора одного сообщения
	 *
	 * @details Лимиты для всех размеров фрагмента одного сообщения обязаны
	 *          совпадать: сравнивается зависимость результата от разбиения входа,
	 *          а не от настроек
	 *
	 */
	typedef struct Setup {
		// Направление разбираемого трафика
		direct_t direct;
		// Метод запроса, которому соответствует ожидаемый ответ
		method_t method;
		// Лимиты безопасности разбора
		parser_http_t::limits_t limits;
	} setup_t;
	/**
	 * @brief Функция формирования текстового представления стартовой строки
	 *
	 * @param parser объект парсера
	 * @param direct направление разбираемого трафика
	 * @return       текстовое представление стартовой строки
	 *
	 */
	static std::string startLine(const parser_http_t & parser, const direct_t direct) noexcept {
		// Получаем провайдер заголовков разобранного сообщения
		const provider_t * provider = parser.message().provider.get();
		// Если провайдер заголовков отсутствует
		if(provider == nullptr)
			// Выводим пустое представление стартовой строки
			return "";
		// Текстовый буфер представления стартовой строки
		char result[512];
		// Если разбирается запрос клиента
		if(direct == direct_t::REQUEST){
			// Получаем объект провайдера заголовков запроса клиента
			const request_t * request = static_cast <const request_t *> (provider);
			// Формируем представление стартовой строки запроса
			::snprintf(
				result, sizeof(result), "REQ m=%u n=%s u=%s v=%u",
				static_cast <uint32_t> (request->method), request->methodName.c_str(),
				request->uri.c_str(), static_cast <uint32_t> (request->version)
			);
		// Если разбирается ответ сервера
		} else {
			// Получаем объект провайдера заголовков ответа сервера
			const response_t * response = static_cast <const response_t *> (provider);
			// Формируем представление стартовой строки ответа
			::snprintf(
				result, sizeof(result), "RES c=%u m=%s v=%u",
				static_cast <uint32_t> (response->code), response->message.c_str(),
				static_cast <uint32_t> (response->version)
			);
		}
		// Выводим представление стартовой строки
		return result;
	}
	/**
	 * @brief Функция разбора сообщения с заданным размером фрагмента подачи
	 *
	 * @param fmk      объект фреймворка
	 * @param log      объект логирования
	 * @param setup    настройки разбора сообщения
	 * @param message  разбираемое сообщение
	 * @param fragment размер фрагмента подачи (0 - сообщение подаётся целиком)
	 * @return         наблюдаемый результат разбора
	 *
	 */
	static outcome_t parsing(const awh::fmk_t * fmk, const awh::log_t * log, const setup_t & setup, const std::string & message, const size_t fragment) noexcept {
		// Результат разбора сообщения
		outcome_t result;
		// Создаём объект парсера
		parser_http_t parser(setup.direct, fmk, log);
		// Применяем лимиты безопасности разбора
		parser.limits(setup.limits);
		// Если разбирается ответ сервера
		if(setup.direct == direct_t::RESPONSE)
			// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
			parser.method(setup.method);
		// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
		parser.on(parser_http_t::data_callback_t([&result](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
			// Собираем фрагмент тела сообщения
			result.body.append(static_cast <const char *> (buffer), size);
			// Продолжаем разбор
			return true;
		}));
		// Устанавливаем функцию обратного вызова обработки фазы разбора сообщения
		parser.on(parser_http_t::phase_callback_t([&result](const uint32_t, const parser_t::phase_t phase, const parser_t::part_t part) noexcept -> bool {
			// Собираем фазовое событие
			result.phases.emplace_back(phase, part);
			// Продолжаем разбор
			return true;
		}));
		// Устанавливаем функцию обратного вызова обработки границ чанков
		parser.on(parser_http_t::chunk_callback_t([&result](const parser_t::phase_t phase, const uint64_t size, const std::string_view extension) noexcept -> bool {
			// Собираем событие границы чанка
			result.chunks.emplace_back(phase, size, std::string(extension));
			// Продолжаем разбор
			return true;
		}));
		// Устанавливаем функцию обратного вызова обработки заголовков сообщения
		parser.on(parser_http_t::header_callback_t([&result](const uint32_t, const std::string_view name, const std::string_view value, const parser_t::part_t part) noexcept -> bool {
			// Если получен трейлер сообщения
			if(part == parser_t::part_t::TRAILER)
				// Собираем трейлер сообщения
				result.trailers.emplace_back(std::string(name), std::string(value));
			// Если получен заголовок сообщения
			else result.headers.emplace_back(std::string(name), std::string(value));
			// Продолжаем разбор
			return true;
		}));
		// Если сообщение подаётся целиком
		if(fragment == 0)
			// Выполняем разбор сообщения одним вызовом
			parser.parse(message.data(), message.size());
		// Если сообщение подаётся фрагментами
		else {
			/**
			 * Выполняем подачу сообщения фрагментами заданного размера
			 */
			for(size_t i = 0; i < message.size(); i += fragment){
				// Определяем размер очередного фрагмента подачи
				const size_t size = ((message.size() - i) < fragment ? (message.size() - i) : fragment);
				// Выполняем разбор очередного фрагмента сообщения
				parser.parse(message.data() + i, size);
			}
		}
		// Собираем итоговый статус разбора
		result.status = parser.status();
		// Собираем код ошибки разбора
		result.error = parser.error();
		// Собираем признак полноты разобранного сообщения
		result.complete = parser.message().flags.complete;
		// Собираем признак кодирования тела методом chunked
		result.chunked = parser.message().flags.chunked;
		// Собираем размер тела сообщения
		result.bodySize = parser.message().bodySize;
		// Собираем представление разобранной стартовой строки
		result.startLine = ::startLine(parser, setup.direct);
		// Выводим результат разбора сообщения
		return result;
	}
	/**
	 * @brief Функция формирования случайного участка символов токена
	 *
	 * @param length требуемая длина участка
	 * @return       участок символов токена
	 *
	 */
	static std::string token(const size_t length) noexcept {
		// Алфавит символов токена
		static const char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.!~*";
		// Результат работы функции
		std::string result;
		/**
		 * Формируем участок символов токена требуемой длины
		 */
		for(size_t i = 0; i < length; i++)
			// Дописываем очередной символ токена
			result.push_back(alphabet[::pick(sizeof(alphabet) - 1)]);
		// Выводим участок символов токена
		return result;
	}
	/**
	 * @brief Функция формирования случайного окончания строки
	 *
	 * @return окончание строки
	 *
	 */
	static std::string eol() noexcept {
		// Голое окончание строки формируется намеренно: его трактовка зависит от режима
		return (::chance(6) ? "\n" : "\r\n");
	}
	/**
	 * @brief Функция формирования стартовой строки запроса клиента
	 *
	 * @return стартовая строка запроса клиента
	 *
	 */
	static std::string request() noexcept {
		// Набор написаний метода запроса: распознаваемые, экзотические и недопустимые
		static const char * methods[] = {
			"GET", "GET", "GET", "PUT", "HEAD", "POST", "POST", "TRACE", "DELETE",
			"OPTIONS", "CONNECT", "PROPPATCH", "PURGE", "X", "get", "GE T", "GET\x01", ""
		};
		// Набор написаний версии протокола: допустимые и отвергаемые
		static const char * versions[] = {
			"HTTP/1.1", "HTTP/1.1", "HTTP/1.1", "HTTP/1.1", "HTTP/1.1", "HTTP/1.1",
			"HTTP/1.1", "HTTP/1.0", "HTTP/1.0", "HTTP/1.0",
			"HTTP/2.0", "HTTP/1.2", "HTTP/0.9", "HTTP1.1", "HTTP/1.", "http/1.1", "HTTP/1.1 "
		};
		// Результат работы функции
		std::string result;
		// Дописываем метод запроса
		result.append(methods[::pick(sizeof(methods) / sizeof(methods[0]))]);
		// Дописываем разделитель после метода запроса
		result.append(::chance(10) ? "  " : " ");
		// Дописываем адрес запрашиваемого ресурса
		result.append("/").append(::token(::pick(24)));
		// Если адрес дополняется параметрами запроса
		if(::chance(30))
			// Дописываем параметры запроса
			result.append("?").append(::token(::pick(16))).append("=").append(::token(::pick(16)));
		// Если в адрес вносится недопустимый символ
		if(::chance(2))
			// Вносим недопустимый символ в адрес
			result.push_back('\x01');
		// Дописываем разделитель перед версией протокола
		result.append(::chance(10) ? "  " : " ");
		// Дописываем версию протокола
		result.append(versions[::pick(sizeof(versions) / sizeof(versions[0]))]);
		// Дописываем окончание стартовой строки
		result.append(::eol());
		// Выводим стартовую строку запроса клиента
		return result;
	}
	/**
	 * @brief Функция формирования стартовой строки ответа сервера
	 *
	 * @return стартовая строка ответа сервера
	 *
	 */
	static std::string response() noexcept {
		// Набор кодов состояния ответа: обычные, бестелесные и недопустимые
		static const char * codes[] = {"200", "204", "304", "404", "500", "101", "100", "2000", "2A0", "20"};
		// Результат работы функции
		std::string result;
		// Дописываем версию протокола
		result.append(::chance(20) ? "HTTP/1.0" : "HTTP/1.1").append(" ");
		// Дописываем код состояния ответа
		result.append(codes[::pick(sizeof(codes) / sizeof(codes[0]))]);
		// Если код состояния дополняется пояснением
		if(::chance(85))
			// Дописываем пояснение к коду состояния
			result.append(" ").append(::token(::pick(12)));
		// Дописываем окончание стартовой строки
		result.append(::eol());
		// Выводим стартовую строку ответа сервера
		return result;
	}
	/**
	 * @brief Функция формирования блока заголовков сообщения
	 *
	 * @param chunked выводимый признак кодирования тела методом chunked
	 * @param length  выводимый размер тела фиксированного размера
	 * @return        блок заголовков сообщения
	 *
	 */
	static std::string headers(bool & chunked, size_t & length) noexcept {
		// Результат работы функции
		std::string result;
		// Сбрасываем признак кодирования тела методом chunked
		chunked = false;
		// Сбрасываем размер тела фиксированного размера
		length = 0;
		// Если сообщение содержит заголовок адреса сервера
		if(::chance(80))
			// Дописываем заголовок адреса сервера
			result.append("Host: ").append(::token(::pick(12) + 1)).append(".com").append(::eol());
		/**
		 * Дописываем произвольные заголовки сообщения
		 */
		for(size_t i = 0, count = ::pick(7); i < count; i++){
			// Формируем название заголовка
			std::string name = ::token(::pick(14) + 1);
			// Если в название заголовка вносится пробел перед двоеточием
			if(::chance(2))
				// Вносим пробел перед двоеточием
				name.append(" ");
			// Если в название заголовка вносится недопустимый символ
			else if(::chance(2))
				// Вносим недопустимый символ в название заголовка
				name.push_back('\x01');
			// Дописываем название заголовка
			result.append(name).append(":");
			// Дописываем ведущие пробельные символы значения
			result.append(::chance(20) ? (::chance(50) ? "\t" : "   ") : " ");
			// Дописываем значение заголовка
			result.append(::token(::pick(40)));
			// Если значение дополняется хвостовыми пробельными символами
			if(::chance(15))
				// Дописываем хвостовые пробельные символы значения
				result.append(" \t ");
			// Если в значение заголовка вносится недопустимый символ
			if(::chance(2))
				// Вносим недопустимый символ в значение заголовка
				result.push_back('\x01');
			// Дописываем окончание строки заголовка
			result.append(::eol());
			// Если заголовок продолжается устаревшим переносом строки
			if(::chance(2))
				// Дописываем устаревший перенос строки заголовка
				result.append(" ").append(::token(::pick(10))).append(::eol());
		}
		// Если сообщение содержит заголовок кодирования тела
		if(::chance(30)){
			// Набор написаний кодирования тела сообщения
			static const char * encodings[] = {"chunked", "chunked", "gzip, chunked", "chunked, gzip", "gzip", "identity"};
			// Выбираем написание кодирования тела сообщения
			const char * encoding = encodings[::pick(sizeof(encodings) / sizeof(encodings[0]))];
			// Дописываем заголовок кодирования тела сообщения
			result.append("Transfer-Encoding: ").append(encoding).append(::eol());
			// Устанавливаем признак кодирования тела методом chunked
			chunked = (::strcmp(encoding, "chunked") == 0) || (::strcmp(encoding, "gzip, chunked") == 0);
		}
		// Если сообщение содержит заголовок размера тела
		if(::chance(35)){
			// Выбираем размер тела фиксированного размера
			length = ::pick(64);
			// Если значение заголовка размера тела недопустимо
			if(::chance(10))
				// Дописываем недопустимое значение заголовка размера тела
				result.append("Content-Length: abc").append(::eol());
			// Если значение заголовка размера тела корректно
			else {
				// Дописываем заголовок размера тела сообщения
				result.append("Content-Length: ").append(std::to_string(length)).append(::eol());
				// Если заголовок размера тела дублируется
				if(::chance(8))
					// Дописываем повторный заголовок размера тела сообщения
					result.append("Content-Length: ").append(std::to_string(length + ::pick(2))).append(::eol());
			}
		}
		// Если сообщение содержит заголовок управления подключением
		if(::chance(25))
			// Дописываем заголовок управления подключением
			result.append("Connection: ").append(::chance(50) ? "keep-alive" : "close").append(::eol());
		// Если сообщение содержит заголовок ожидания продолжения
		if(::chance(10))
			// Дописываем заголовок ожидания продолжения
			result.append("Expect: 100-continue").append(::eol());
		// Если сообщение содержит заголовок смены протокола
		if(::chance(8))
			// Дописываем заголовок смены протокола
			result.append("Upgrade: websocket").append(::eol());
		// Дописываем окончание блока заголовков
		result.append(::eol());
		// Выводим блок заголовков сообщения
		return result;
	}
	/**
	 * @brief Функция формирования тела сообщения в кодировке chunked
	 *
	 * @return тело сообщения в кодировке chunked
	 *
	 */
	static std::string chunks() noexcept {
		// Результат работы функции
		std::string result;
		// Текстовый буфер строки размера чанка
		char line[32];
		/**
		 * Формируем чанки тела сообщения
		 */
		for(size_t i = 0, count = ::pick(4); i < count; i++){
			// Определяем размер очередного чанка
			const size_t size = (::pick(24) + 1);
			// Формируем строку размера чанка
			::snprintf(line, sizeof(line), "%zX", size);
			// Дописываем строку размера чанка
			result.append(line);
			// Если чанк дополняется расширениями
			if(::chance(25))
				// Дописываем расширения чанка
				result.append(";").append(::token(::pick(6) + 1)).append("=").append(::token(::pick(6) + 1));
			// Дописываем окончание строки размера чанка
			result.append(::eol());
			// Дописываем данные чанка
			result.append(::token(size));
			// Дописываем окончание данных чанка
			result.append(::chance(5) ? "" : ::eol());
		}
		// Дописываем строку размера последнего чанка
		result.append("0").append(::eol());
		/**
		 * Дописываем трейлеры сообщения
		 */
		for(size_t i = 0, count = ::pick(3); i < count; i++){
			// Набор названий трейлеров: обычные и запрещённые в блоке трейлеров
			static const char * names[] = {"X-Check", "X-Sum", "Content-Length", "Transfer-Encoding", "Host", "Connection"};
			// Дописываем трейлер сообщения
			result.append(names[::pick(sizeof(names) / sizeof(names[0]))]).append(": ").append(::token(::pick(8) + 1)).append(::eol());
		}
		// Дописываем окончание блока трейлеров
		result.append(::eol());
		// Выводим тело сообщения в кодировке chunked
		return result;
	}
	/**
	 * @brief Функция формирования разбираемого сообщения
	 *
	 * @param direct направление разбираемого трафика
	 * @return       разбираемое сообщение
	 *
	 */
	static std::string generate(const direct_t direct) noexcept {
		// Признак кодирования тела методом chunked
		bool chunked = false;
		// Размер тела фиксированного размера
		size_t length = 0;
		// Формируем стартовую строку сообщения
		std::string result = ((direct == direct_t::REQUEST) ? ::request() : ::response());
		// Дописываем блок заголовков сообщения
		result.append(::headers(chunked, length));
		// Если тело сообщения закодировано методом chunked
		if(chunked)
			// Дописываем тело сообщения в кодировке chunked
			result.append(::chunks());
		// Если тело сообщения имеет фиксированный размер
		else if(length > 0)
			// Дописываем тело сообщения фиксированного размера, изредка неполное
			result.append(::token(::chance(10) ? ::pick(length) : length));
		// Если сообщение портится точечной заменой октета
		if(::chance(6) && !result.empty())
			// Выполняем замену случайного октета сообщения
			result[::pick(result.size())] = static_cast <char> (::pick(256));
		// Если сообщение обрывается
		if(::chance(4) && !result.empty())
			// Выполняем обрыв сообщения в случайной позиции
			result.resize(::pick(result.size()));
		// Выводим разбираемое сообщение
		return result;
	}
	/**
	 * @brief Функция формирования настроек разбора сообщения
	 *
	 * @param direct  направление разбираемого трафика
	 * @param initial исходные лимиты безопасности разбора
	 * @return        настройки разбора сообщения
	 *
	 */
	static setup_t setup(const direct_t direct, const parser_http_t::limits_t & initial) noexcept {
		// Результат работы функции
		setup_t result{direct, method_t::NONE, initial};
		// Если применяется строгий набор лимитов безопасности
		if(::chance(12))
			// Применяем строгий набор лимитов безопасности
			result.limits = parser_http_t::limits_t::strict();
		// Если применяется строгая трактовка окончаний строк
		else if(::chance(20))
			// Применяем строгую трактовку окончаний строк
			result.limits.strictEOL = true;
		// Если применяется строгая трактовка лишних пробелов
		else if(::chance(20))
			// Применяем строгую трактовку лишних пробелов
			result.limits.strictSpaces = true;
		// Если лимит длины стартовой строки понижается
		if(::chance(6))
			// Понижаем лимит длины стартовой строки
			result.limits.maxRequestLine = (::pick(64) + 24);
		// Если лимит размера названия заголовка понижается
		if(::chance(5))
			// Понижаем лимит размера названия заголовка
			result.limits.maxHeaderName = (::pick(24) + 8);
		// Если разбирается ответ сервера
		if(direct == direct_t::RESPONSE){
			// Набор методов запроса, влияющих на кадрирование тела ответа
			static const method_t methods[] = {method_t::GET, method_t::HEAD, method_t::CONNECT, method_t::POST};
			// Выбираем метод запроса, которому соответствует ожидаемый ответ
			result.method = methods[::pick(sizeof(methods) / sizeof(methods[0]))];
		}
		// Выводим настройки разбора сообщения
		return result;
	}
	/**
	 * @brief Функция вывода сообщения в экранированном виде
	 *
	 * @param message выводимое сообщение
	 *
	 */
	static void dump(const std::string & message) noexcept {
		/**
		 * Перебираем октеты выводимого сообщения
		 */
		for(const char item : message){
			// Получаем беззнаковое значение октета
			const uint8_t octet = static_cast <uint8_t> (item);
			// Если октет является печатным символом
			if((octet >= 0x20) && (octet < 0x7F))
				// Выводим октет как есть
				::printf("%c", item);
			// Если октет печатным символом не является
			else ::printf("\\x%02X", octet);
		}
		// Выводим окончание строки
		::printf("\n");
	}
};

/**
 * @brief Функция входа в генератор
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из генератора
 *
 */
int32_t main(int32_t argc, char ** argv) noexcept {
	// Количество выполняемых итераций
	const size_t rounds = ((argc > 1) ? static_cast <size_t> (::strtoull(argv[1], nullptr, 10)) : 3000);
	// Создаём объект фреймворка
	awh::fmk_t fmk;
	// Создаём объект логирования
	awh::log_t log(&fmk);
	// Отключаем вывод сообщений парсера: генератор намеренно подаёт нештатный трафик
	log.level(awh::log_t::level_t::NONE);
	// Набор проверяемых размеров фрагмента подачи (0 - сообщение подаётся целиком)
	static const size_t fragments[] = {0, 2, 3, 5, 8, 17, 64, 250};
	// Количество разобранных сообщений
	size_t total = 0;
	// Количество полностью разобранных сообщений
	size_t completed = 0;
	// Количество отвергнутых сообщений
	size_t rejected = 0;
	// Количество выполненных сверок
	size_t checks = 0;
	// Исходные лимиты безопасности разбора
	const parser_http_t::limits_t initial;
	/**
	 * Выполняем требуемое количество итераций
	 */
	for(size_t round = 0; round < rounds; round++){
		/**
		 * Проверяем оба направления трафика
		 */
		for(const direct_t direct : {direct_t::REQUEST, direct_t::RESPONSE}){
			// Формируем настройки разбора сообщения
			const setup_t options = ::setup(direct, initial);
			// Формируем разбираемое сообщение
			const std::string message = ::generate(direct);
			/**
			 * Эталоном служит посимвольная подача: на ней не срабатывает ни один
			 * крупноблочный путь, поэтому она разбирает сообщение самым простым
			 * из имеющихся способов
			 */
			const outcome_t expected = ::parsing(&fmk, &log, options, message, 1);
			// Считаем разобранное сообщение
			total++;
			// Если сообщение разобрано полностью
			if(expected.status == parser_t::status_t::COMPLETE)
				// Считаем полностью разобранное сообщение
				completed++;
			// Если сообщение отвергнуто
			else if(expected.status == parser_t::status_t::ERROR)
				// Считаем отвергнутое сообщение
				rejected++;
			/**
			 * Выполняем разбор того же сообщения всеми проверяемыми размерами фрагмента
			 */
			for(const size_t fragment : fragments){
				// Выполняем разбор сообщения проверяемым размером фрагмента
				const outcome_t actual = ::parsing(&fmk, &log, options, message, fragment);
				// Считаем выполненную сверку
				checks++;
				// Если результат разбора совпал с эталонным
				if(!(actual != expected))
					// Переходим к следующему размеру фрагмента
					continue;
				// Выводим сообщение о расхождении результатов разбора
				::printf(
					"РАСХОЖДЕНИЕ: итерация %zu, направление %s, фрагмент %zu\n",
					round, ((direct == direct_t::REQUEST) ? "REQUEST" : "RESPONSE"), fragment
				);
				// Выводим эталонный результат разбора
				::printf(
					"  эталон:  статус=%u ошибка=%u полное=%d chunked=%d размер=%lld стартовая=%s\n"
					"           тело=%zu заголовков=%zu трейлеров=%zu фаз=%zu чанков=%zu\n",
					static_cast <uint32_t> (expected.status), static_cast <uint32_t> (expected.error),
					static_cast <int32_t> (expected.complete), static_cast <int32_t> (expected.chunked),
					static_cast <long long> (expected.bodySize), expected.startLine.c_str(),
					expected.body.size(), expected.headers.size(), expected.trailers.size(),
					expected.phases.size(), expected.chunks.size()
				);
				// Выводим полученный результат разбора
				::printf(
					"  получен: статус=%u ошибка=%u полное=%d chunked=%d размер=%lld стартовая=%s\n"
					"           тело=%zu заголовков=%zu трейлеров=%zu фаз=%zu чанков=%zu\n",
					static_cast <uint32_t> (actual.status), static_cast <uint32_t> (actual.error),
					static_cast <int32_t> (actual.complete), static_cast <int32_t> (actual.chunked),
					static_cast <long long> (actual.bodySize), actual.startLine.c_str(),
					actual.body.size(), actual.headers.size(), actual.trailers.size(),
					actual.phases.size(), actual.chunks.size()
				);
				// Выводим разбираемое сообщение
				::printf("  сообщение: ");
				// Выводим сообщение в экранированном виде
				::dump(message);
				// Выводим код выхода с ошибкой
				return 1;
			}
		}
	}
	// Выводим статистику выполненного прогона
	::printf(
		"http1 fuzz: %zu сообщений (%zu разобрано полностью, %zu отвергнуто), %zu сверок путей - расхождений нет\n",
		total, completed, rejected, checks
	);
	// Выводим успешный код выхода
	return 0;
}
