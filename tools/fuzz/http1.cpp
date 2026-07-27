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
	 * @note Стартовая строка сверяется в том числе у отвергнутых сообщений: её
	 *       недособранное содержимое обязано совпадать у обоих путей, иначе
	 *       разбиение входа влияло бы на то, что видит потребитель после отказа.
	 *       В сравнение не входит только количество обработанных октетов - на
	 *       участках имени и значения заголовка крупноблочный путь возвращает
	 *       начало участка, а посимвольный позицию недопустимого октета, и после
	 *       ошибки разбора это число смысла не имеет
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
				(this->bodySize != other.bodySize) || (this->startLine != other.startLine) ||
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
		/**
		 * Изредка дописываем пустые строки перед стартовой строкой запроса: сервер
		 * обязан их игнорировать (RFC 9112 §2.2), но в строгом режиме окончаний строк
		 * они отвергаются, а их поток обязан упираться в предел
		 */
		if(::chance(8)){
			// Дописываем от одной до нескольких пустых строк перед стартовой строкой
			for(size_t i = 0, count = (::pick(12) + 1); i < count; i++)
				// Дописываем очередную пустую строку
				result.append(::eol());
		}
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
		/**
		 * Набор кодов состояния ответа
		 *
		 * Коды вне диапазона 100..599 присутствуют намеренно: RFC 9110 §15 объявляет
		 * их недопустимыми, но предписывает получателю обрабатывать такой ответ так,
		 * как если бы код принадлежал классу 5xx, а не отвергать сообщение
		 */
		static const char * codes[] = {
			"200", "204", "304", "404", "500", "101", "100",
			"000", "099", "600", "999",
			"2000", "2A0", "20"
		};
		// Результат работы функции
		std::string result;
		// Дописываем версию протокола
		result.append(::chance(20) ? "HTTP/1.0" : "HTTP/1.1").append(" ");
		// Дописываем код состояния ответа
		result.append(codes[::pick(sizeof(codes) / sizeof(codes[0]))]);
		/**
		 * Если код состояния дополняется пояснением
		 *
		 * Длина пояснения изредка берётся заведомо большой: reason-phrase учитывается
		 * в общем бюджете стартовой строки, и без длинных пояснений понижённый лимит
		 * этой длины никогда не достигался бы
		 */
		if(::chance(85))
			// Дописываем пояснение к коду состояния
			result.append(" ").append(::token(::chance(20) ? (::pick(90) + 12) : ::pick(12)));
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
			/**
			 * Дописываем ведущие пробельные символы значения
			 *
			 * Изредка их поток удлиняется: отброшенные при разборе октеты входят в
			 * бюджет блока заголовков наравне с сохранёнными, и без длинных прогонов
			 * этот учёт не достигается
			 */
			if(::chance(8))
				// Дописываем длинный поток ведущих пробельных символов значения
				result.append(std::string((::pick(300) + 1), (::chance(50) ? '\t' : ' ')));
			// Дописываем обычные ведущие пробельные символы значения
			else result.append(::chance(20) ? (::chance(50) ? "\t" : "   ") : " ");
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
			/**
			 * Набор написаний кодирования тела сообщения
			 *
			 * Написания с пустыми элементами списка присутствуют намеренно: RFC 9110
			 * §5.6.1.2 обязывает получателя их игнорировать, и завершающая запятая
			 * не отменяет того, что последним кодированием объявлен chunked
			 */
			static const char * encodings[] = {
				"chunked", "chunked", "gzip, chunked", "chunked, gzip", "gzip", "identity",
				"chunked,", " , chunked , ", "chunked, , gzip", ",,"
			};
			// Выбираем написание кодирования тела сообщения
			const char * encoding = encodings[::pick(sizeof(encodings) / sizeof(encodings[0]))];
			// Дописываем заголовок кодирования тела сообщения
			result.append("Transfer-Encoding: ").append(encoding).append(::eol());
			// Устанавливаем признак кодирования тела методом chunked
			chunked = (
				(::strcmp(encoding, "chunked") == 0) || (::strcmp(encoding, "gzip, chunked") == 0) ||
				(::strcmp(encoding, "chunked,") == 0) || (::strcmp(encoding, " , chunked , ") == 0)
			);
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
			/**
			 * Изредка дописываем к токену параметр: элементом списка Connection обязан
			 * быть голый токен, и параметр обязан отсекаться от его имени
			 */
			result.append("Connection: ").append(::chance(50) ? "keep-alive" : "close")
			 .append(::chance(15) ? ";foo=bar" : "").append(::eol());
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
			/**
			 * Изредка дописываем BWS после размера чанка
			 *
			 * По RFC 9112 §7.1 расширения имеют вид *( BWS ";" BWS ... ), поэтому пробел
			 * перед точкой с запятой допустим, а без неё - нет: разбор обязан развести
			 * эти два случая, иначе размер чанка читался бы до пробела
			 */
			if(::chance(10))
				// Дописываем BWS после размера чанка
				result.append(::chance(50) ? " " : "\t");
			// Если чанк дополняется расширениями
			if(::chance(25)){
				// Дописываем расширения чанка
				result.append(";").append(::token(::pick(6) + 1)).append("=").append(::token(::pick(6) + 1));
				/**
				 * Изредка вносим в расширения октет с края правила допустимости: DEL и
				 * управляющий символ обязаны отвергаться, пробел и obs-text - приниматься
				 */
				if(::chance(20)){
					// Набор октетов с края правила допустимости расширений чанка
					static const char octets[] = {'\x7F', '\x01', ' ', '\x80', '\xFF'};
					// Вносим октет с края правила допустимости в расширения чанка
					result.push_back(octets[::pick(sizeof(octets) / sizeof(octets[0]))]);
				}
			}
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
			// Набор названий трейлеров: пригодные и по представителю каждой непригодной категории RFC 9110 §6.5.1
			static const char * names[] = {
				"X-Check", "X-Sum", "Digest",
				"Content-Length", "Transfer-Encoding", "Host", "Connection", "Trailer",
				"Cache-Control", "If-Match", "Range", "Authorization", "Set-Cookie",
				"Date", "Location", "Vary", "Content-Type", "Content-Encoding"
			};
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
		/**
		 * Если суммарный бюджет блока заголовков понижается
		 *
		 * Без понижения бюджет не достигается: отброшенные при разборе ведущие OWS
		 * входят в него наравне с сохранёнными октетами, и оба пути разбора обязаны
		 * считать их одинаково - иначе строка с потоком пробелов разбиралась бы
		 * по-разному в зависимости от нарезки входа
		 */
		if(::chance(6))
			// Понижаем суммарный бюджет блока заголовков
			result.limits.maxHeadersTotal = (::pick(400) + 64);
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
 * @brief Окружение проверки отправляющей стороны
 *
 * @details Приёмная сторона проверяется дифференциально - разбиением одного и того
 *          же входа. У отправляющей стороны входа нет, поэтому свойства другие:
 *          собранное сообщение обязано разбираться обратно в то же самое, а способ
 *          выдачи байтов наружу и способ подачи тела не должны влиять на провод
 *
 */
namespace {
	/**
	 * @brief Структура описания собираемого исходящего сообщения
	 *
	 * @details Описание формируется один раз и собирается по нему несколько раз
	 *          разными способами: расхождение провода между способами и есть
	 *          проверяемый дефект
	 *
	 */
	typedef struct Outgoing {
		// Направление собираемого трафика
		direct_t direct;
		// Метод собираемого запроса
		method_t method;
		// Адрес запрашиваемого ресурса
		std::string target;
		// Код состояния собираемого ответа
		uint16_t code;
		// Заголовки собираемого сообщения
		std::vector <std::pair <std::string, std::string>> headers;
		// Тело собираемого сообщения
		std::string body;
		// Трейлеры собираемого сообщения
		std::vector <std::pair <std::string, std::string>> trailers;
		// Признак кадрирования тела методом chunked
		bool chunked;
		// Признак сборки сообщения версии HTTP/1.0
		bool legacy;
		// Признак неисправимого объявления кодирования (chunked не последний)
		bool unfixable;
		// Размер одной порции выдачи тела
		size_t portion;
		// Верхний порог выходного буфера
		size_t high;
		// Нижний порог выходного буфера
		size_t low;
	} outgoing_t;
	/**
	 * @brief Функция проверки отказа отправителя кадрировать тело сообщения
	 *
	 * @details Тело запроса HTTP/1.0 без Content-Length кадрировать нечем: получатель
	 *          обязан считать такой запрос запросом без тела (RFC 9112 §6.3). Отправитель
	 *          тело не принимает, и на проводе остаётся один блок заголовков
	 *
	 * @param outgoing описание собираемого исходящего сообщения
	 * @return         результат проверки
	 *
	 */
	static bool messageDropped(const struct Outgoing & outgoing) noexcept {
		// Выводим признак отказа отправителя собрать сообщение целиком
		return outgoing.unfixable;
	}
	/**
	 * @brief Функция проверки отказа отправителя кадрировать тело сообщения
	 *
	 * @param outgoing описание собираемого исходящего сообщения
	 * @return         результат проверки
	 *
	 */
	static bool bodyRefused(const struct Outgoing & outgoing) noexcept {
		// Выводим признак отказа отправителя кадрировать тело сообщения
		return (outgoing.legacy && outgoing.chunked && (outgoing.direct == direct_t::REQUEST));
	}
	/**
	 * @brief Функция формирования описания собираемого исходящего сообщения
	 *
	 * @return описание собираемого исходящего сообщения
	 *
	 */
	static outgoing_t compose() noexcept {
		// Набор методов собираемого запроса
		static const method_t methods[] = {method_t::GET, method_t::POST, method_t::PUT, method_t::DEL, method_t::PATCH};
		// Результат работы функции
		outgoing_t result;
		// Выбираем направление собираемого трафика
		result.direct = (::chance(50) ? direct_t::REQUEST : direct_t::RESPONSE);
		// Выбираем метод собираемого запроса
		result.method = methods[::pick(sizeof(methods) / sizeof(methods[0]))];
		// Формируем адрес запрашиваемого ресурса
		result.target = ("/" + ::token(::pick(20)));
		// Выбираем код состояния собираемого ответа
		result.code = static_cast <uint16_t> (::chance(70) ? 200 : (::chance(50) ? 404 : 500));
		// Выбираем кадрирование тела сообщения
		result.chunked = ::chance(50);
		/**
		 * Выбираем версию собираемого сообщения. В HTTP/1.0 кодирования chunked
		 * не существует: объявление отправитель снимает с провода, тело ответа
		 * кадрируется закрытием соединения, а телу запроса требуется Content-Length -
		 * без него кадрировать его нечем, и отправитель тело не принимает вовсе,
		 * ни методом отправки, ни через pull-источник
		 */
		result.legacy = ::chance(20);
		// Выбираем неисправимость объявления кодирования: собирать такое сообщение отправитель откажется
		result.unfixable = (result.chunked && !result.legacy && ::chance(10));
		// Формируем тело собираемого сообщения
		result.body = ::token(::pick(4000));
		// Определяем размер одной порции выдачи тела
		result.portion = (::pick(700) + 1);
		// Определяем верхний порог выходного буфера
		result.high = (::pick(3000) + 64);
		// Определяем нижний порог выходного буфера
		result.low = (::pick(result.high / 2) + 1);
		/**
		 * Формируем заголовки собираемого сообщения
		 *
		 * Названия делаются заведомо различными: контейнер заголовков по умолчанию
		 * работает в режиме замены одноимённых полей, и повторяющееся название
		 * проверяло бы политику контейнера, а не кадрирование отправителя
		 */
		for(size_t i = 0, count = ::pick(6); i < count; i++)
			// Дописываем очередной заголовок собираемого сообщения
			result.headers.emplace_back(("X-" + ::token(::pick(10) + 1) + "-" + std::to_string(i)), ::token(::pick(30)));
		// Если тело сообщения кадрируется методом chunked (в HTTP/1.0 его нет, а с ним нет и трейлеров)
		if(result.chunked && !result.legacy && !result.body.empty()){
			/**
			 * Формируем трейлеры собираемого сообщения: запрещённые в блоке трейлеров
			 * поля отправитель отбрасывает сам, и здесь они не формируются - проверка
			 * их отбрасывания лежит на модульных тестах
			 */
			for(size_t i = 0, count = ::pick(3); i < count; i++)
				// Дописываем очередной трейлер собираемого сообщения
				result.trailers.emplace_back(("X-Trailer-" + ::token(::pick(6) + 1) + "-" + std::to_string(i)), ::token(::pick(12) + 1));
		}
		// Выводим описание собираемого исходящего сообщения
		return result;
	}
	/**
	 * @brief Функция сборки исходящего сообщения
	 *
	 * @param fmk      объект фреймворка
	 * @param log      объект логирования
	 * @param outgoing описание собираемого исходящего сообщения
	 * @param pull     признак выдачи байтов pull-моделью вместо функции обратного вызова записи
	 * @param source   признак подачи тела pull-источником вместо sendData
	 * @return         собранные байты исходящего сообщения
	 *
	 */
	static std::string emit(const awh::fmk_t * fmk, const awh::log_t * log, const outgoing_t & outgoing, const bool pull, const bool source) noexcept {
		// Собранные байты исходящего сообщения
		std::string wire;
		// Создаём объект парсера-отправителя
		parser_http_t sender(outgoing.direct, fmk, log);
		// Устанавливаем пороги выходного буфера
		sender.sendWaterMarks(outgoing.high, outgoing.low);
		// Если байты выдаются функцией обратного вызова записи
		if(!pull){
			// Устанавливаем функцию обратного вызова записи исходящих байтов
			sender.on(parser_http_t::write_callback_t([&wire](const void * buffer, const size_t size) noexcept {
				// Собираем отданные сетевому слою байты
				wire.append(static_cast <const char *> (buffer), size);
			}));
		}
		/**
		 * @brief Функция выборки накопленных исходящих байтов pull-моделью
		 *
		 */
		auto drain = [&sender, &wire, pull]() noexcept -> void {
			// Если байты выдаются функцией обратного вызова записи
			if(!pull)
				// Выборка не требуется - буфер опустошается сам
				return;
			/**
			 * Выбираем накопленные исходящие байты до опустошения буфера
			 */
			while(true){
				// Получаем ещё не отправленные исходящие байты
				const std::string_view chunk = sender.pending();
				// Если исходящих байтов не осталось
				if(chunk.empty())
					// Прекращаем выборку
					break;
				// Собираем выбранные исходящие байты
				wire.append(chunk.data(), chunk.size());
				// Освобождаем выбранные байты из исходящего буфера
				sender.consumePending(chunk.size());
			}
		};
		// Формируем контейнер заголовков собираемого сообщения
		const version_t version = (outgoing.legacy ? version_t::HTTP1_0 : version_t::HTTP1_1);
		// Формируем контейнер заголовков собираемого сообщения
		headers_t block((outgoing.direct == direct_t::REQUEST)
		 ? headers_t(std::make_unique <request_t> (version, outgoing.method, outgoing.target))
		 : headers_t(std::make_unique <response_t> (version, outgoing.code)));
		/**
		 * Дописываем заголовки собираемого сообщения
		 */
		for(const auto & header : outgoing.headers)
			// Дописываем очередной заголовок собираемого сообщения
			block.emplace(header.first, header.second);
		/**
		 * Если тело сообщения кадрируется методом chunked
		 *
		 * Изредка объявляется кодирование, где chunked не последний: собрать такое
		 * сообщение отправитель обязан отказаться - дописывание применило бы chunked
		 * к телу дважды, а получатель отверг бы кадр
		 */
		if(outgoing.chunked)
			// Дописываем заголовок кодирования тела сообщения
			block.emplace("Transfer-Encoding", (outgoing.unfixable ? "chunked, gzip" : "chunked"));
		// Если тело сообщения кадрируется фиксированным размером
		else block.emplace("Content-Length", std::to_string(outgoing.body.size()));
		// Отправляем заголовки собираемого сообщения
		sender.sendHeaders(block, (outgoing.body.empty() && outgoing.trailers.empty()));
		// Выбираем накопленные исходящие байты
		drain();
		// Если тело сообщения подаётся pull-источником
		if(source && !outgoing.body.empty()){
			// Позиция чтения тела сообщения источником
			size_t position = 0;
			/**
			 * Изредка источник объявляет конец тела, не выдав анонсированного объёма
			 *
			 * Тело фиксированного размера завершается строго по исчерпании Content-Length,
			 * поэтому досрочный конец тела у источника обязан оставить сообщение
			 * незавершённым, а остаток - дойти до провода методом выдачи тела. Случай
			 * применим только к кадрированию фиксированного размера: у chunked конец
			 * тела источника законно завершает сообщение нулевым чанком
			 */
			const size_t announced = ((!outgoing.chunked && !outgoing.legacy && ::chance(12))
			 ? (outgoing.body.size() - 1) : outgoing.body.size());
			// Устанавливаем pull-источник данных тела сообщения
			sender.dataSource(parser_http_t::data_source_callback_t([&sender, &outgoing, &position, announced](const uint32_t, uint8_t * buffer, const size_t cap, bool & eof) noexcept -> int64_t {
				/**
				 * Пробуем вклиниться в тело из самого источника
				 *
				 * Источник и прямая выдача - взаимоисключающие способы подачи одного и
				 * того же тела, а блок трейлеров завершил бы его нулевым чанком поверх
				 * недочитанного остатка. Момент выбран наихудший: выходной буфер сейчас
				 * между резервированием участка под текущую порцию и его фиксацией, и
				 * принятая запись разорвала бы кадрирование прямо внутри заголовка чанка.
				 * Обе попытки обязаны быть отвергнуты без следа на проводе - иначе
				 * сообщение не разберётся обратно в отправленное
				 */
				if(::chance(15))
					// Пробуем выдать порцию тела поверх активного источника
					sender.sendData("INTRUDER", 8, false);
				// Если выполняется попытка завершить тело блоком трейлеров
				if(::chance(15)){
					// Формируем блок трейлеров сообщения
					headers_t intruder;
					// Дописываем трейлер сообщения
					intruder.emplace("X-Intruder", "yes");
					// Пробуем завершить тело блоком трейлеров поверх активного источника
					sender.sendHeaders(intruder, false);
				}
				// Определяем размер выдаваемой источником порции тела
				const size_t size = std::min(std::min(cap, outgoing.portion), (announced - position));
				// Копируем очередную порцию тела сообщения
				::memcpy(buffer, (outgoing.body.data() + position), size);
				// Смещаем позицию чтения тела сообщения
				position += size;
				// Устанавливаем признак достижения конца тела сообщения
				eof = (position >= announced);
				// Выводим размер выданной порции тела
				return static_cast <int64_t> (size);
			}));
			/**
			 * Прокачиваем pull-источник до исчерпания тела: при заполнении выходного
			 * буфера прокачка останавливается и возобновляется выборкой
			 */
			while(sender.sourcePending()){
				// Выбираем накопленные исходящие байты
				drain();
				// Если прокачка источника не возобновилась
				if(!sender.resumeSource())
					// Прекращаем прокачку источника
					break;
			}
			/**
			 * Дошлём остаток тела, не выданный источником: сообщение обязано остаться
			 * незавершённым, иначе на проводе окажется усечённое тело, а получатель
			 * дочитывал бы недостающие байты до таймаута
			 */
			while(position < outgoing.body.size()){
				// Определяем размер выдаваемой порции остатка тела
				const size_t size = std::min(outgoing.portion, (outgoing.body.size() - position));
				// Выполняем выдачу очередной порции остатка тела
				const size_t accepted = sender.sendData((outgoing.body.data() + position), size, true);
				// Если отправитель порцию не принял - выдача остатка невозможна
				if(accepted == 0)
					// Прекращаем выдачу остатка тела
					break;
				// Смещаем позицию выдачи тела сообщения
				position += accepted;
				// Выбираем накопленные исходящие байты
				drain();
			}
			// Выбираем накопленные исходящие байты
			drain();
		// Если тело сообщения подаётся напрямую
		} else if(!outgoing.body.empty()) {
			// Позиция выдачи тела сообщения
			size_t position = 0;
			/**
			 * Выдаём тело сообщения порциями до полной передачи
			 */
			while(position < outgoing.body.size()){
				// Определяем размер выдаваемой порции тела
				const size_t size = std::min(outgoing.portion, (outgoing.body.size() - position));
				// Определяем признак завершения сообщения текущей порцией
				const bool last = (((position + size) >= outgoing.body.size()) && outgoing.trailers.empty());
				// Выполняем выдачу очередной порции тела сообщения
				const size_t accepted = sender.sendData((outgoing.body.data() + position), size, last);
				// Смещаем позицию выдачи тела сообщения
				position += accepted;
				// Выбираем накопленные исходящие байты
				drain();
				// Если выходной буфер отверг порцию целиком
				if((accepted == 0) && (sender.pending().empty()))
					// Прекращаем выдачу тела - продвижение невозможно
					break;
			}
		}
		// Если сообщение завершается блоком трейлеров
		if(!outgoing.trailers.empty()){
			// Формируем контейнер трейлеров сообщения
			headers_t trailers;
			/**
			 * Дописываем трейлеры собираемого сообщения
			 */
			for(const auto & trailer : outgoing.trailers)
				// Дописываем очередной трейлер собираемого сообщения
				trailers.emplace(trailer.first, trailer.second);
			// Отправляем трейлеры с завершением сообщения
			sender.sendHeaders(trailers, true);
		}
		// Выбираем накопленные исходящие байты
		drain();
		// Выводим собранные байты исходящего сообщения
		return wire;
	}
	/**
	 * @brief Функция проверки обратной разбираемости собранного сообщения
	 *
	 * @param fmk      объект фреймворка
	 * @param log      объект логирования
	 * @param outgoing описание собранного исходящего сообщения
	 * @param wire     собранные байты исходящего сообщения
	 * @param reason   выводимая причина расхождения
	 * @return         результат проверки
	 *
	 */
	static bool roundtrip(const awh::fmk_t * fmk, const awh::log_t * log, const outgoing_t & outgoing, const std::string & wire, std::string & reason) noexcept {
		// Создаём объект парсера-приёмника собранного сообщения
		parser_http_t receiver(outgoing.direct, fmk, log);
		// Если разбирается ответ сервера
		if(outgoing.direct == direct_t::RESPONSE)
			// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
			receiver.method(method_t::GET);
		// Принятое тело сообщения
		std::string body;
		// Принятые заголовки сообщения
		std::vector <std::pair <std::string, std::string>> headers;
		// Принятые трейлеры сообщения
		std::vector <std::pair <std::string, std::string>> trailers;
		// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
		receiver.on(parser_http_t::data_callback_t([&body](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
			// Собираем фрагмент принятого тела сообщения
			body.append(static_cast <const char *> (buffer), size);
			// Продолжаем разбор
			return true;
		}));
		// Устанавливаем функцию обратного вызова обработки заголовков сообщения
		receiver.on(parser_http_t::header_callback_t([&headers, &trailers](const uint32_t, const std::string_view name, const std::string_view value, const parser_t::part_t part) noexcept -> bool {
			// Если принят трейлер сообщения
			if(part == parser_t::part_t::TRAILER)
				// Собираем принятый трейлер сообщения
				trailers.emplace_back(std::string(name), std::string(value));
			// Если принят заголовок сообщения
			else headers.emplace_back(std::string(name), std::string(value));
			// Продолжаем разбор
			return true;
		}));
		// Выполняем разбор собранного сообщения
		const size_t consumed = receiver.parse(wire.data(), wire.size());
		/**
		 * Собранное сообщение обязано потребляться получателем целиком. Непотреблённый
		 * хвост означает, что отправитель выдал на провод байты, кадрированием сообщения
		 * не описанные: получатель прочитает их как начало следующего сообщения и
		 * рассинхронизирует кадрирование соединения. Сверка тела такой хвост не ловит -
		 * получатель его просто не отдаёт наружу
		 */
		if(consumed != wire.size()){
			// Формируем причину расхождения
			reason = ("на проводе остался непотреблённый хвост: потреблено " +
			 std::to_string(consumed) + " из " + std::to_string(wire.size()));
			// Выводим отрицательный результат
			return false;
		}
		/**
		 * Ответ HTTP/1.0 без Content-Length кадрируется закрытием соединения: объявление
		 * кодирования chunked отправитель с провода снял, и конец такого сообщения
		 * обозначается не разметкой, а концом потока - о нём приёмнику надо сообщить
		 */
		if(outgoing.legacy && outgoing.chunked && (outgoing.direct == direct_t::RESPONSE))
			// Сообщаем приёмнику о закрытии соединения
			receiver.eof();
		// Если сообщение разобрано не полностью
		if(receiver.status() != parser_t::status_t::COMPLETE){
			// Формируем причину расхождения
			reason = ("сообщение не разобралось обратно: " + std::string(receiver.errorName()));
			// Выводим отрицательный результат
			return false;
		}
		/**
		 * Определяем ожидаемое тело: если отправитель кадрировать тело отказался,
		 * до получателя оно дойти не может и обязано быть пустым - иначе
		 * некадрированные байты ушли бы на провод в обход отказа
		 */
		const std::string & expected = (::bodyRefused(outgoing) ? std::string() : outgoing.body);
		// Если принятое тело сообщения не совпало с ожидаемым
		if(body != expected){
			// Формируем причину расхождения
			reason = ("тело разошлось: ожидалось " + std::to_string(expected.size()) +
			 ", принято " + std::to_string(body.size()));
			// Выводим отрицательный результат
			return false;
		}
		// Если количество принятых трейлеров не совпало с отправленным
		if(trailers.size() != outgoing.trailers.size()){
			// Формируем причину расхождения
			reason = ("трейлеры разошлись: отправлено " + std::to_string(outgoing.trailers.size()) +
			 ", принято " + std::to_string(trailers.size()));
			// Выводим отрицательный результат
			return false;
		}
		/**
		 * @brief Функция регистронезависимого сравнения названий заголовков
		 *
		 * @note Названия заголовков регистронезависимы по RFC 9110 §5.1, и отправитель
		 *       приводит их к каноническому написанию. Сверять названия побайтово
		 *       означало бы проверять не сохранность заголовка, а способ его записи
		 *
		 * @param first  первое сравниваемое название
		 * @param second второе сравниваемое название
		 * @return       результат сравнения
		 *
		 */
		auto equals = [](const std::string & first, const std::string & second) noexcept -> bool {
			// Если размеры названий не совпадают
			if(first.size() != second.size())
				// Выводим отрицательный результат сравнения
				return false;
			/**
			 * Перебираем октеты сравниваемых названий
			 */
			for(size_t i = 0; i < first.size(); i++){
				// Приводим октет первого названия к нижнему регистру
				const char a = ((first[i] >= 'A') && (first[i] <= 'Z') ? static_cast <char> (first[i] | 0x20) : first[i]);
				// Приводим октет второго названия к нижнему регистру
				const char b = ((second[i] >= 'A') && (second[i] <= 'Z') ? static_cast <char> (second[i] | 0x20) : second[i]);
				// Если октеты названий не совпали
				if(a != b)
					// Выводим отрицательный результат сравнения
					return false;
			}
			// Выводим положительный результат сравнения
			return true;
		};
		/**
		 * Перебираем отправленные заголовки сообщения
		 *
		 * Сверка выполняется на вхождение, а не на равенство: отправитель добавляет
		 * к блоку заголовки кадрирования тела, и их наличие сверяется не здесь
		 */
		for(const auto & sent : outgoing.headers){
			// Признак присутствия отправленного заголовка среди принятых
			bool found = false;
			/**
			 * Перебираем принятые заголовки сообщения
			 */
			for(const auto & received : headers){
				// Если отправленный заголовок найден среди принятых
				if(equals(received.first, sent.first) && (received.second == sent.second)){
					// Отмечаем присутствие отправленного заголовка
					found = true;
					// Прекращаем перебор принятых заголовков
					break;
				}
			}
			// Если отправленный заголовок среди принятых отсутствует
			if(!found){
				// Формируем причину расхождения
				reason = ("заголовок потерян: отправлено \"" + sent.first + ": " + sent.second + "\", принято:");
				/**
				 * Перебираем принятые заголовки сообщения для вывода в причину
				 */
				for(const auto & received : headers)
					// Дописываем принятый заголовок в причину расхождения
					reason.append(" \"").append(received.first).append(": ").append(received.second).append("\"");
				// Выводим отрицательный результат
				return false;
			}
		}
		// Получаем провайдер заголовков принятого сообщения
		const provider_t * provider = receiver.message().provider.get();
		// Если разбирается запрос клиента
		if((outgoing.direct == direct_t::REQUEST) && (provider != nullptr)){
			// Получаем объект провайдера заголовков запроса клиента
			const request_t * request = static_cast <const request_t *> (provider);
			// Если стартовая строка запроса разошлась с отправленной
			if((request->method != outgoing.method) || (request->uri != outgoing.target)){
				// Формируем причину расхождения
				reason = ("стартовая строка запроса разошлась: uri=" + request->uri);
				// Выводим отрицательный результат
				return false;
			}
		// Если разбирается ответ сервера
		} else if(provider != nullptr) {
			// Получаем объект провайдера заголовков ответа сервера
			const response_t * response = static_cast <const response_t *> (provider);
			// Если код состояния ответа разошёлся с отправленным
			if(response->code != outgoing.code){
				// Формируем причину расхождения
				reason = ("код состояния ответа разошёлся: " + std::to_string(response->code));
				// Выводим отрицательный результат
				return false;
			}
		}
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция проверки реентрантности функций обратного вызова
	 *
	 * @details Моделируется враждебное поведение приложения: обработчики изредка
	 *          сбрасывают парсер прямо из своего вызова. Метод `clear` при этом
	 *          удаляет установленные функции обратного вызова, то есть уничтожает
	 *          ту самую функцию, тело которой выполняется. Этот класс реентрантности
	 *          давал дефекты работы с памятью не раз, поэтому проверяется постоянно
	 *
	 * @param fmk объект фреймворка
	 * @param log объект логирования
	 * @return    результат проверки пригодности парсера после сброса
	 *
	 */
	static bool reentrancy(const awh::fmk_t * fmk, const awh::log_t * log) noexcept {
		// Выбираем направление разбираемого трафика
		const direct_t direct = (::chance(50) ? direct_t::REQUEST : direct_t::RESPONSE);
		// Создаём объект парсера
		parser_http_t parser(direct, fmk, log);
		// Формируем разбираемое сообщение
		const std::string message = ::generate(direct);
		// Признак удаления функций обратного вызова из обработчика
		bool cleared = false;
		/**
		 * @brief Функция подписки обработчиков, сбрасывающих парсер из своего вызова
		 *
		 */
		auto attach = [&parser, &cleared]() noexcept -> void {
			// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
			parser.on(parser_http_t::data_callback_t([&parser](const uint32_t, const void *, const size_t, const bool) noexcept -> bool {
				// Если обработчик требует сбросить парсер
				if(::chance(3))
					// Выполняем сброс парсера прямо из обработчика
					parser.reset();
				// Продолжаем разбор
				return true;
			}));
			// Устанавливаем функцию обратного вызова обработки заголовков сообщения
			parser.on(parser_http_t::header_callback_t([&parser, &cleared](const uint32_t, const std::string_view, const std::string_view, const parser_t::part_t) noexcept -> bool {
				// Если обработчик требует полностью очистить парсер
				if(::chance(1)){
					// Отмечаем удаление функций обратного вызова
					cleared = true;
					// Выполняем полную очистку парсера прямо из обработчика
					parser.clear();
				// Если обработчик требует сбросить парсер
				} else if(::chance(3))
					// Выполняем сброс парсера прямо из обработчика
					parser.reset();
				// Продолжаем разбор
				return true;
			}));
			// Устанавливаем функцию обратного вызова обработки фазы разбора сообщения
			parser.on(parser_http_t::phase_callback_t([](const uint32_t, const parser_t::phase_t, const parser_t::part_t) noexcept -> bool {
				// Если обработчик требует прервать разбор
				if(::chance(2))
					// Прерываем разбор сообщения
					return false;
				// Продолжаем разбор
				return true;
			}));
			// Устанавливаем функцию обратного вызова обработки границ чанков
			parser.on(parser_http_t::chunk_callback_t([&parser](const parser_t::phase_t, const uint64_t, const std::string_view) noexcept -> bool {
				// Если обработчик требует сбросить парсер
				if(::chance(2))
					// Выполняем сброс парсера прямо из обработчика
					parser.reset();
				// Продолжаем разбор
				return true;
			}));
		};
		// Подписываем обработчики, сбрасывающие парсер из своего вызова
		attach();
		// Определяем размер фрагмента подачи сообщения
		const size_t fragment = (::pick(64) + 1);
		/**
		 * Выполняем подачу сообщения фрагментами
		 */
		for(size_t i = 0; i < message.size(); i += fragment){
			// Определяем размер очередного фрагмента подачи
			const size_t size = ((message.size() - i) < fragment ? (message.size() - i) : fragment);
			// Выполняем разбор очередного фрагмента сообщения
			parser.parse(message.data() + i, size);
		}
		// Подавляем предупреждение о неиспользуемом признаке очистки
		(void) cleared;
		/**
		 * Проверяем пригодность парсера после реентрантных сбросов: он обязан
		 * разобрать следующее корректное сообщение как ни в чём не бывало
		 *
		 * Враждебные обработчики на время проверки заменяются безобидными: иначе
		 * они сбрасывали бы парсер и посреди проверочного сообщения, и проверка
		 * измеряла бы поведение обработчиков, а не пригодность парсера
		 */
		parser.on(parser_http_t::data_callback_t([](const uint32_t, const void *, const size_t, const bool) noexcept -> bool { return true; }));
		parser.on(parser_http_t::header_callback_t([](const uint32_t, const std::string_view, const std::string_view, const parser_t::part_t) noexcept -> bool { return true; }));
		parser.on(parser_http_t::phase_callback_t([](const uint32_t, const parser_t::phase_t, const parser_t::part_t) noexcept -> bool { return true; }));
		parser.on(parser_http_t::chunk_callback_t([](const parser_t::phase_t, const uint64_t, const std::string_view) noexcept -> bool { return true; }));
		// Выполняем сброс парсера перед проверочным сообщением
		parser.reset();
		// Если разбирается ответ сервера
		if(direct == direct_t::RESPONSE)
			// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
			parser.method(method_t::GET);
		// Формируем заведомо корректное проверочное сообщение
		const std::string probe = ((direct == direct_t::REQUEST)
		 ? std::string("GET / HTTP/1.1\r\nHost: anyks.com\r\n\r\n")
		 : std::string("HTTP/1.1 204 No Content\r\n\r\n"));
		// Выполняем разбор проверочного сообщения
		parser.parse(probe.data(), probe.size());
		// Пригодным считается только полный разбор проверочного сообщения
		return (parser.status() == parser_t::status_t::COMPLETE);
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
	// Количество собранных исходящих сообщений
	size_t emitted = 0;
	// Количество выполненных сеансов проверки реентрантности
	size_t reentrant = 0;
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
		/**
		 * Проверяем отправляющую сторону: собранное сообщение обязано разбираться
		 * обратно в то же самое, а способ выдачи байтов наружу и способ подачи тела
		 * не должны влиять на то, что оказывается на проводе
		 */
		{
			// Формируем описание собираемого исходящего сообщения
			const outgoing_t outgoing = ::compose();
			// Собираем сообщение с выдачей байтов функцией обратного вызова записи
			const std::string pushed = ::emit(&fmk, &log, outgoing, false, false);
			// Собираем то же сообщение с выдачей байтов pull-моделью
			const std::string pulled = ::emit(&fmk, &log, outgoing, true, false);
			/**
			 * Собираем то же сообщение с подачей тела pull-источником. Сообщения с
			 * трейлерами так не собираются: источник завершает тело сам по достижении
			 * его конца, и блок трейлеров дописывать уже некуда - это ограничение
			 * интерфейса, зафиксированное в документации метода dataSource
			 */
			const std::string sourced = (outgoing.trailers.empty()
			 ? ::emit(&fmk, &log, outgoing, false, true) : pushed);
			// Считаем собранное исходящее сообщение
			emitted++;
			// Если способ выдачи байтов повлиял на провод
			if(pushed != pulled){
				// Выводим сообщение о расхождении способов выдачи байтов
				::printf(
					"РАСХОЖДЕНИЕ ОТПРАВКИ: итерация %zu, push %zu октетов, pull %zu октетов\n",
					round, pushed.size(), pulled.size()
				);
				// Выводим код выхода с ошибкой
				return 1;
			}
			// Причина расхождения обратной разбираемости
			std::string reason;
			/**
			 * Сообщение, собранное каждым из способов, обязано разбираться обратно в
			 * то же самое. Побайтового совпадения провода со способом подачи тела не
			 * требуется: pull-источник выдаёт тело своими порциями, и разбивка на
			 * чанки у него другая, а границы чанков семантики сообщения не несут
			 */
			for(const auto & wire : {pushed, sourced}){
				/**
				 * Если отправитель отказался собирать сообщение - на проводе обязана
				 * остаться пустота, и обратная разбираемость к ней неприменима
				 */
				if(::messageDropped(outgoing)){
					// Если отказ не пуст на проводе
					if(!wire.empty()){
						// Выводим сообщение о нарушенном отказе сборки
						::printf("НАРУШЕН ОТКАЗ СБОРКИ: итерация %zu, на проводе %zu октетов\n", round, wire.size());
						// Выводим собранное сообщение в экранированном виде
						::dump(wire.substr(0, 512));
						// Выводим код выхода с ошибкой
						return 1;
					}
					// Переходим к следующему способу выдачи байтов
					continue;
				}
				// Если собранное сообщение не разбирается обратно в то же самое
				if(!::roundtrip(&fmk, &log, outgoing, wire, reason)){
					// Выводим сообщение о расхождении обратной разбираемости
					::printf(
						"РАСХОЖДЕНИЕ КРУГА: итерация %zu, способ %s, %s\n", round,
						((wire.size() == pushed.size()) ? "sendData" : "источник"), reason.c_str()
					);
					// Выводим собранное сообщение
					::printf("  провод: ");
					// Выводим собранное сообщение в экранированном виде
					::dump(wire.substr(0, 512));
					// Выводим код выхода с ошибкой
					return 1;
				}
			}
		}
		/**
		 * Проверяем реентрантность: обработчики сбрасывают и очищают парсер прямо
		 * из своего вызова, после чего парсер обязан оставаться пригодным
		 */
		if(::chance(50)){
			// Считаем выполненный сеанс проверки реентрантности
			reentrant++;
			// Если парсер после реентрантных сбросов оказался непригоден
			if(!::reentrancy(&fmk, &log)){
				// Выводим сообщение о непригодности парсера
				::printf("РЕЕНТРАНТНОСТЬ: итерация %zu, парсер не разобрал проверочное сообщение\n", round);
				// Выводим код выхода с ошибкой
				return 1;
			}
		}
	}
	// Выводим статистику выполненного прогона
	::printf(
		"http1 fuzz: %zu сообщений (%zu разобрано полностью, %zu отвергнуто), %zu сверок путей,\n"
		"            %zu собранных исходящих сообщений, %zu сеансов реентрантности - расхождений нет\n",
		total, completed, rejected, checks, emitted, reentrant
	);
	// Выводим успешный код выхода
	return 0;
}
