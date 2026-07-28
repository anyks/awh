/**
 * @file: http.cpp
 * @date: 2026-07-19
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация парсера сессии HTTP/2 — управление состояниями потоков, окнами flow control, обменом SETTINGS,
 *        приоритетами, частотными лимитами и сборка исходящих фреймов соединения
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <algorithm>

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/parser/http2/http.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние вспомогательные функции (внутренняя компоновка)
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Пространство имён названий заголовков, участвующих в сравнениях
	 *
	 * @details Названия вынесены константами намеренно. Сравнение вида
	 *          `name.compare("upgrade")` строит представление из указателя,
	 *          а значит вычисляет длину литерала вызовом strlen во время
	 *          выполнения - на каждый заголовок каждого блока. У константы
	 *          длина известна на этапе компиляции, и сравнение сводится
	 *          к проверке длины и сравнению памяти
	 *
	 */
	namespace header {
		// Псевдо-заголовки запроса и ответа (RFC 9113 §8.3)
		static constexpr string_view METHOD = ":method";
		static constexpr string_view SCHEME = ":scheme";
		static constexpr string_view PATH = ":path";
		static constexpr string_view AUTHORITY = ":authority";
		static constexpr string_view PROTOCOL = ":protocol";
		static constexpr string_view STATUS = ":status";
		// Заголовки, запрещённые в HTTP/2 (RFC 9113 §8.2.2)
		static constexpr string_view UPGRADE = "upgrade";
		static constexpr string_view KEEP_ALIVE = "keep-alive";
		static constexpr string_view CONNECTION = "connection";
		static constexpr string_view PROXY_CONNECTION = "proxy-connection";
		static constexpr string_view TRANSFER_ENCODING = "transfer-encoding";
		// Заголовки, запрещённые в трейлерах
		static constexpr string_view TE = "te";
		static constexpr string_view HOST = "host";
		static constexpr string_view RANGE = "range";
		static constexpr string_view EXPECT = "expect";
		static constexpr string_view TRAILER = "trailer";
		static constexpr string_view CONTENT_TYPE = "content-type";
		static constexpr string_view CACHE_CONTROL = "cache-control";
		static constexpr string_view CONTENT_RANGE = "content-range";
		static constexpr string_view MAX_FORWARDS = "max-forwards";
		static constexpr string_view AUTHORIZATION = "authorization";
		static constexpr string_view CONTENT_LENGTH = "content-length";
		static constexpr string_view CONTENT_ENCODING = "content-encoding";
		static constexpr string_view PROXY_AUTHORIZATION = "proxy-authorization";
		// Заголовок приоритета запроса (RFC 9218 §5)
		static constexpr string_view PRIORITY = "priority";
	};
	/**
	 * @brief Пространство имён значений заголовков, участвующих в сравнениях
	 *
	 */
	namespace value {
		// Методы запроса, меняющие обработку тела
		static constexpr string_view CONNECT = "CONNECT";
		static constexpr string_view HEAD = "HEAD";
		// Метод, которому допустима звёздочка вместо пути (RFC 9110 §7.1)
		static constexpr string_view OPTIONS = "OPTIONS";
		// Коды состояния, при которых тело ответа отсутствует
		static constexpr string_view SWITCHING = "101";
		static constexpr string_view NO_CONTENT = "204";
		static constexpr string_view NOT_MODIFIED = "304";
		// Элементы структурированного поля приоритета (RFC 9218 §4)
		static constexpr string_view INCREMENTAL = "i";
		static constexpr string_view INCREMENTAL_ON = "i=?1";
		static constexpr string_view INCREMENTAL_OFF = "i=?0";
	};

	/**
	 * @brief Функция приведения названия заголовка к нижнему регистру (RFC 9113 §8.2.1)
	 *
	 * @details Если название уже в нижнем регистре - возвращается исходная строка без копии
	 *          (горячий путь), иначе выполняется копия в переиспользуемый буфер.
	 *
	 * @param name   название заголовка
	 * @param buffer переиспользуемый буфер для приведённого названия
	 * @return       название заголовка в нижнем регистре
	 *
	 */
	string_view lowerName(string_view name, string & buffer) noexcept {
		/**
		 * Выполняем поиск первой заглавной латинской буквы
		 */
		for(size_t i = 0; i < name.size(); ++i){
			// Если найдена заглавная латинская буква
			if((name[i] >= 'A') && (name[i] <= 'Z')){
				// Копируем название заголовка в буфер
				buffer.assign(name);
				/**
				 * Приводим оставшиеся символы к нижнему регистру
				 */
				for(size_t j = i; j < buffer.size(); ++j){
					// Если символ является заглавной латинской буквой
					if((buffer[j] >= 'A') && (buffer[j] <= 'Z'))
						// Приводим символ к нижнему регистру
						buffer[j] = static_cast <char> (buffer[j] + 32);
				}
				// Выводим приведённое название заголовка
				return buffer;
			}
		}
		// Название уже в нижнем регистре - выводим без копии
		return name;
	}
	/**
	 * @brief Функция проверки принадлежности символа к набору token (RFC 9110 §5.6.2)
	 *
	 * @note Заглавные латинские буквы исключены намеренно: в HTTP/2 имена
	 *       заголовков передаются только в нижнем регистре (RFC 9113 §8.2.1)
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 *
	 */
	bool isTokenChar(const uint8_t letter) noexcept {
		// Цифры и латинские буквы нижнего регистра допустимы
		if(((letter >= '0') && (letter <= '9')) || ((letter >= 'a') && (letter <= 'z')))
			// Символ допустим
			return true;
		/**
		 * Определяем принадлежность символа к прочим разрешённым символам token
		 */
		switch(letter){
			// Прочие разрешённые символы token (RFC 9110 §5.6.2)
			case '!': case '#': case '$': case '%': case '&': case '\'':
			case '*': case '+': case '-': case '.': case '^': case '_':
			case '`': case '|': case '~':
				// Символ допустим
				return true;
		}
		// Символ недопустим
		return false;
	}
	/**
	 * @brief Функция проверки допустимости имени заголовка (RFC 9113 §8.2.1)
	 *
	 * @details Имя обязано быть непустым, целиком в нижнем регистре и состоять
	 *          только из символов token; двоеточие допустимо исключительно
	 *          первым символом (псевдо-заголовок).
	 *
	 * @param name имя заголовка
	 * @return     результат проверки
	 *
	 */
	/**
	 * @brief Функция проверки принадлежности схемы запроса протоколу HTTP
	 *
	 * @details Форму цели запроса задают только схемы [http] и [https]: для прочих
	 *          её задаёт не HTTP, и проверять путь по правилам HTTP нельзя.
	 *          Схема в URI регистронезависима (RFC 3986 §3.1)
	 *
	 * @param scheme значение псевдо-заголовка схемы
	 * @return       результат проверки
	 *
	 */
	bool isHttpScheme(string_view scheme) noexcept {
		// Если длина схемы не совпадает ни с одной из проверяемых
		if((scheme.size() != 4) && (scheme.size() != 5))
			// Схема протоколу HTTP не принадлежит
			return false;
		// Собираемая схема в нижнем регистре
		char letters[5] = {0};
		/**
		 * Выполняем приведение схемы к нижнему регистру
		 */
		for(size_t i = 0; i < scheme.size(); i++)
			// Приводим очередной символ схемы к нижнему регистру
			letters[i] = static_cast <char> (((scheme[i] >= 'A') && (scheme[i] <= 'Z')) ? (scheme[i] + 32) : scheme[i]);
		// Выводим результат сравнения схемы с проверяемыми
		return ((string_view(letters, scheme.size()) == "http") || (string_view(letters, scheme.size()) == "https"));
	}
	bool isValidHeaderName(string_view name) noexcept {
		// Пустое имя заголовка недопустимо
		if(name.empty())
			// Имя заголовка некорректно
			return false;
		/**
		 * Выполняем перебор всех символов имени заголовка
		 */
		for(size_t i = 0; i < name.size(); ++i){
			// Получаем очередной символ имени заголовка
			const uint8_t letter = static_cast <uint8_t> (name[i]);
			// Если получено двоеточие
			if(letter == ':'){
				// Двоеточие допустимо только первым символом псевдо-заголовка
				if(i > 0)
					// Имя заголовка некорректно
					return false;
				// Переходим к следующему символу
				continue;
			}
			// Если найден символ вне набора token (включая верхний регистр и пробелы)
			if(!isTokenChar(letter))
				// Имя заголовка некорректно
				return false;
		}
		// Имя заголовка корректно
		return true;
	}
	/**
	 * @brief Функция проверки допустимости значения заголовка (RFC 9113 §8.2.1)
	 *
	 * @details Значение не может содержать NUL/CR/LF (иначе при трансляции в HTTP/1.1
	 *          получаем расщепление сообщения) и не может начинаться либо
	 *          заканчиваться пробельным символом.
	 *
	 * @param value значение заголовка
	 * @return      результат проверки
	 *
	 */
	bool isValidHeaderValue(string_view value) noexcept {
		/**
		 * Выполняем перебор всех символов значения заголовка
		 */
		for(const char letter : value){
			// Если найден символ, разрывающий поле заголовка
			if((letter == '\0') || (letter == '\n') || (letter == '\r'))
				// Значение заголовка некорректно
				return false;
		}
		// Если значение заголовка не пустое
		if(!value.empty()){
			// Начальный или конечный пробельный символ недопустим
			if((value.front() == ' ') || (value.front() == '\t') ||
			   (value.back() == ' ') || (value.back() == '\t'))
				// Значение заголовка некорректно
				return false;
		}
		// Значение заголовка корректно
		return true;
	}
	/**
	 * @brief Функция проверки принадлежности заголовка к запрещённым в HTTP/2 connection-specific заголовкам (RFC 9113 §8.2.2)
	 *
	 * @note Имя заголовка к этому моменту уже провалидировано и находится
	 *       в нижнем регистре, поэтому сравнение выполняется строгое
	 *
	 * @param name имя заголовка
	 * @return     результат проверки
	 *
	 */
	bool isConnectionSpecific(string_view name) noexcept {
		// Выполняем сравнение со списком запрещённых заголовков
		return (
			(name == header::UPGRADE) ||
			(name == header::KEEP_ALIVE) ||
			(name == header::CONNECTION) ||
			(name == header::PROXY_CONNECTION) ||
			(name == header::TRANSFER_ENCODING)
		);
	}
	/**
	 * @brief Функция проверки допустимости заголовка в секции трейлеров (RFC 9110 §6.5.1)
	 *
	 * @details В трейлерах запрещены поля управления сообщением, маршрутизации,
	 *          аутентификации и описания содержимого: получатель может их не увидеть,
	 *          так как секция приходит после тела.
	 *
	 * @param name имя заголовка
	 * @return     результат проверки
	 *
	 */
	bool isForbiddenTrailer(string_view name) noexcept {
		// Выполняем сравнение со списком запрещённых в трейлерах заголовков
		return (
			(name == header::TE) ||
			(name == header::HOST) ||
			(name == header::RANGE) ||
			(name == header::EXPECT) ||
			(name == header::TRAILER) ||
			(name == header::CONTENT_TYPE) ||
			(name == header::CACHE_CONTROL) ||
			(name == header::CONTENT_RANGE) ||
			(name == header::MAX_FORWARDS) ||
			(name == header::AUTHORIZATION) ||
			(name == header::CONTENT_LENGTH) ||
			(name == header::CONTENT_ENCODING) ||
			(name == header::PROXY_AUTHORIZATION)
		);
	}
	/**
	 * @brief Функция валидации HTTP-семантики блока заголовков (RFC 9113 §8.1-8.3)
	 *
	 * @param fields          декодированные заголовки блока
	 * @param isRequest       блок принадлежит запросу клиента (true) или ответу сервера (false)
	 * @param isTrailers      блок является трейлерами
	 * @param connectProtocol расширенный CONNECT разрешён нашим SETTINGS (RFC 8441)
	 * @param fmk             объект фреймворка
	 * @return                код ошибки протокола (NO_ERROR - блок корректен, PROTOCOL_ERROR - malformed)
	 *
	 */
	http::h2::error_t validateHeaders(const vector <http::h2::hpack::field_view_t> & fields, const bool isRequest, const bool isTrailers, const bool connectProtocol, const fmk_t * fmk) noexcept {
		// Значение псевдо-заголовка [:method]
		string_view method{""};
		// Значение псевдо-заголовка [:authority]
		string_view authority{""};
		// Значение псевдо-заголовка [:scheme]
		string_view scheme{""};
		// Значение псевдо-заголовка [:path]
		string_view path{""};
		// Значение заголовка [host]
		string_view host{""};
		// Флаг наличия обычного (не псевдо) заголовка
		bool seenRegular = false;
		// Флаг наличия заголовка [host]
		bool hasHost = false;
		// Флаг наличия заголовка [content-length]
		bool hasLength = false;
		// Значение заголовка [content-length]
		string_view length{""};
		// Флаг наличия псевдо-заголовка [:protocol] расширенного CONNECT (RFC 8441)
		bool hasProtocol = false;
		// Флаги наличия обязательных псевдо-заголовков
		bool hasPath      = false,
		     hasMethod    = false,
		     hasScheme    = false,
			 hasStatus    = false,
			 hasAuthority = false;
		/**
		 * Выполняем перебор всех заголовков блока
		 */
		for(const auto & field : fields){
			// Получаем имя заголовка
			const string_view name = field.name;
			/**
			 * Имя обязано быть непустым, в нижнем регистре и состоять из символов token,
			 * значение - не содержать CR/LF/NUL и обрамляющих пробелов (RFC 9113 §8.2.1)
			 */
			if(!::isValidHeaderName(name) || !::isValidHeaderValue(field.value))
				// Блок заголовков некорректен
				return http::h2::error_t::PROTOCOL_ERROR;
			// Если заголовок является псевдо-заголовком
			if(name.front() == ':'){
				// Псевдо-заголовок после обычного заголовка недопустим (RFC 9113 §8.3)
				if(seenRegular)
					// Блок заголовков некорректен
					return http::h2::error_t::PROTOCOL_ERROR;
				// Псевдо-заголовки запрещены в трейлерах (RFC 9113 §8.1)
				if(isTrailers)
					// Блок заголовков некорректен
					return http::h2::error_t::PROTOCOL_ERROR;
				// Если блок принадлежит запросу клиента
				if(isRequest){
					// Если получен псевдо-заголовок [:method]
					if(name == header::METHOD){
						// Повторный псевдо-заголовок недопустим
						if(hasMethod)
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
						// Помечаем что псевдо-заголовок получен
						hasMethod = true;
						// Пустой метод запроса недопустим - это токен (RFC 9110 §9.1)
						if(field.value.empty())
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
						// Запоминаем значение метода запроса
						method = field.value;
					// Если получен псевдо-заголовок [:scheme]
					} else if(name == header::SCHEME) {
						// Повторный псевдо-заголовок недопустим
						if(hasScheme)
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
						// Помечаем что псевдо-заголовок получен
						hasScheme = true;
						// Запоминаем значение схемы запроса
						scheme = field.value;
						// Пустая схема запроса недопустима (RFC 9113 §8.3.1)
						if(field.value.empty())
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
					// Если получен псевдо-заголовок [:path]
					} else if(name == header::PATH) {
						// Повторный псевдо-заголовок недопустим
						if(hasPath)
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
						// Помечаем что псевдо-заголовок получен
						hasPath = true;
						// Запоминаем значение пути запроса
						path = field.value;
						// Пустое значение [:path] недопустимо (RFC 9113 §8.3.1)
						if(field.value.empty())
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
					// Если получен псевдо-заголовок [:authority]
					} else if(name == header::AUTHORITY) {
						// Повторный псевдо-заголовок недопустим
						if(hasAuthority)
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
						// Помечаем что псевдо-заголовок получен
						hasAuthority = true;
						// Запоминаем значение авторитета запроса
						authority = field.value;
					// Если получен псевдо-заголовок [:protocol] расширенного CONNECT (RFC 8441 §4)
					} else if(name == header::PROTOCOL) {
						/**
						 * Расширенный CONNECT допустим только если мы сами его разрешили
						 * параметром SETTINGS_ENABLE_CONNECT_PROTOCOL (RFC 8441 §3)
						 */
						if(!connectProtocol)
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
						// Повторный псевдо-заголовок недопустим
						if(hasProtocol)
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
						// Пустое значение протокола недопустимо
						if(field.value.empty())
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
						// Помечаем что псевдо-заголовок получен
						hasProtocol = true;
					// Неизвестный/неуместный псевдо-заголовок недопустим
					} else return http::h2::error_t::PROTOCOL_ERROR;
				// Если блок принадлежит ответу сервера
				} else {
					// Если получен псевдо-заголовок [:status]
					if(name == header::STATUS){
						// Повторный псевдо-заголовок недопустим
						if(hasStatus)
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
						// Помечаем что псевдо-заголовок получен
						hasStatus = true;
						// Статус-код обязан состоять ровно из трёх цифр (RFC 9113 §8.3.2)
						if(field.value.size() != 3)
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
						// Статус 101 в HTTP/2 не определён - апгрейд протокола выполняется иначе (RFC 9113 §8.1)
						if(field.value == value::SWITCHING)
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
						/**
						 * Выполняем перебор всех символов статус-кода
						 */
						for(const char letter : field.value){
							// Если найден символ не являющийся цифрой - блок некорректен
							if((letter < '0') || (letter > '9'))
								// Блок заголовков некорректен
								return http::h2::error_t::PROTOCOL_ERROR;
						}
					// Неизвестный псевдо-заголовок недопустим в ответе
					} else return http::h2::error_t::PROTOCOL_ERROR;
				}
			// Если заголовок является обычным заголовком
			} else {
				// Помечаем что обычный заголовок встречен
				seenRegular = true;
				// В секции трейлеров допустимы не все заголовки (RFC 9110 §6.5.1)
				if(isTrailers && ::isForbiddenTrailer(name))
					// Блок заголовков некорректен
					return http::h2::error_t::PROTOCOL_ERROR;
				// Connection-specific заголовки запрещены в HTTP/2 (RFC 9113 §8.2.2)
				if(::isConnectionSpecific(name))
					// Блок заголовков некорректен
					return http::h2::error_t::PROTOCOL_ERROR;
				// Заголовок [te] допускает только значение [trailers] (RFC 9113 §8.2.2)
				if((name == header::TE) && !fmk->compare("trailers", field.value))
					// Блок заголовков некорректен
					return http::h2::error_t::PROTOCOL_ERROR;
				// Если получен заголовок [host] - запоминаем его для сверки с [:authority]
				if(name == header::HOST){
					// Помечаем что заголовок получен
					hasHost = true;
					// Запоминаем значение авторитета запроса
					host = field.value;
				// Если получен заголовок [content-length]
				} else if(name == header::CONTENT_LENGTH) {
					// Пустое значение длины тела недопустимо (RFC 9110 §8.6)
					if(field.value.empty())
						// Блок заголовков некорректен
						return http::h2::error_t::PROTOCOL_ERROR;
					/**
					 * Выполняем перебор всех символов длины тела
					 */
					for(const char letter : field.value){
						// Если найден символ не являющийся цифрой
						if((letter < '0') || (letter > '9'))
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
					}
					// Повторный content-length допустим только с тем же значением (RFC 9110 §8.6)
					if(hasLength && (length.compare(field.value) != 0))
						// Блок заголовков некорректен
						return http::h2::error_t::PROTOCOL_ERROR;
					// Помечаем что заголовок получен
					hasLength = true;
					// Запоминаем объявленную длину тела
					length = field.value;
				}
			}
		}
		// В трейлерах обязательных псевдо-заголовков нет
		if(isTrailers)
			// Блок заголовков корректен
			return http::h2::error_t::NO_ERROR;
		/**
		 * Заголовок [host] и псевдо-заголовок [:authority] обязаны указывать на один
		 * и тот же ресурс, иначе запрос считается малформированным (RFC 9113 §8.3.1):
		 * расхождение - классический вектор десинхронизации на прокси
		 */
		if(hasHost && hasAuthority && !fmk->compare(host, authority))
			// Блок заголовков некорректен
			return http::h2::error_t::PROTOCOL_ERROR;
		// Если блок принадлежит запросу клиента
		if(isRequest){
			/**
			 * Метод запроса - регистрозависимый токен (RFC 9110 §9.1), поэтому
			 * сравнение выполняется строгое: [connect] методом CONNECT не является
			 */
			if(method == value::CONNECT){
				/**
				 * Метод CONNECT требует наличия [:authority] (RFC 9113 §8.5), причём
				 * непустого: значение задаёт хост и порт назначения туннеля, поэтому
				 * пустая строка адресата не описывает и делает запрос малформированным
				 */
				if(!hasAuthority || authority.empty())
					// Блок заголовков некорректен
					return http::h2::error_t::PROTOCOL_ERROR;
				/**
				 * Расширенный CONNECT (RFC 8441 §4), напротив, ОБЯЗАН нести [:scheme] и [:path]:
				 * туннель поднимается к конкретному ресурсу, а не к хосту целиком
				 */
				if(hasProtocol){
					// Отсутствие [:scheme] либо [:path] делает запрос некорректным
					if(!hasScheme || !hasPath)
						// Блок заголовков некорректен
						return http::h2::error_t::PROTOCOL_ERROR;
				// Классический CONNECT запрещает [:scheme]/[:path]
				} else if(hasScheme || hasPath)
					// Блок заголовков некорректен
					return http::h2::error_t::PROTOCOL_ERROR;
			// Псевдо-заголовок [:protocol] допустим только с методом CONNECT (RFC 8441 §4)
			} else if(hasProtocol)
				// Блок заголовков некорректен
				return http::h2::error_t::PROTOCOL_ERROR;
			// Для остальных методов обязательны [:method]/[:scheme]/[:path]
			else if(!hasMethod || !hasScheme || !hasPath)
				// Блок заголовков некорректен
				return http::h2::error_t::PROTOCOL_ERROR;
			/**
			 * Схемы [http] и [https] задают форму цели запроса (RFC 9113 §8.3.1):
			 * путь либо начинается с косой черты, либо равен звёздочке, и звёздочка
			 * допустима только методу OPTIONS - она адресует сервер целиком, а не
			 * ресурс. Прочие схемы проверке не подлежат: их форму задаёт не HTTP
			 */
			if(::isHttpScheme(scheme) && hasPath){
				// Если путь не начинается с косой черты
				if(path.empty() || (path.front() != '/')){
					// Звёздочка допустима только методу OPTIONS
					if((path != "*") || (method != value::OPTIONS))
						// Блок заголовков некорректен
						return http::h2::error_t::PROTOCOL_ERROR;
				}
			}
			/**
			 * У схем с обязательным адресатом (http/https) запрос обязан нести
			 * :authority либо Host, и присутствующее поле не может быть пустым
			 * (RFC 9113 §8.3.1). Классический CONNECT сюда не заходит: у него
			 * нет схемы, а непустой :authority уже проверен выше
			 */
			if(::isHttpScheme(scheme)){
				// Если адресат не задан ни псевдо-заголовком, ни полем Host
				if(!hasAuthority && !hasHost)
					// Блок заголовков некорректен
					return http::h2::error_t::PROTOCOL_ERROR;
				// Если псевдо-заголовок адресата присутствует пустым
				if(hasAuthority && authority.empty())
					// Блок заголовков некорректен
					return http::h2::error_t::PROTOCOL_ERROR;
				// Если поле Host присутствует пустым
				if(hasHost && host.empty())
					// Блок заголовков некорректен
					return http::h2::error_t::PROTOCOL_ERROR;
			}
			/**
			 * Устаревший подкомпонент userinfo в адресате запрещён для схем [http]
			 * и [https] (RFC 9113 §8.3.1): он переносил бы в запрос учётные данные,
			 * которым место в заголовке авторизации. Отделяет его символ [@]
			 */
			if(::isHttpScheme(scheme) && (authority.find('@') != string_view::npos))
				// Блок заголовков некорректен
				return http::h2::error_t::PROTOCOL_ERROR;
		// Ответ сервера обязан содержать [:status]
		} else if(!hasStatus)
			// Блок заголовков некорректен
			return http::h2::error_t::PROTOCOL_ERROR;
		// Блок заголовков корректен
		return http::h2::error_t::NO_ERROR;
	}
};

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP2::Limits::Limits() noexcept :
 parser_t::limits_t(),
 rstLimitRate(RST_LIMIT_RATE),
 rstLimitBurst(RST_LIMIT_BURST),
 ctrlLimitRate(CTRL_LIMIT_RATE),
 ctrlLimitBurst(CTRL_LIMIT_BURST),
 prioLimitRate(PRIO_LIMIT_RATE),
 prioLimitBurst(PRIO_LIMIT_BURST),
 maxHeaderBlockSize(MAX_HEADER_BLOCK_SIZE),
 maxContinuationFrames(MAX_CONTINUATION_FRAMES) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP2::Settings::Settings() noexcept :
 windowSize(h2::proto::DEFAULT_WINDOW_SIZE),
 enablePush(h2::proto::DEFAULT_ENABLE_PUSH),
 maxFrameSize(h2::proto::DEFAULT_MAX_FRAME_SIZE),
 headerTableSize(h2::proto::DEFAULT_HEADER_TABLE_SIZE),
 maxHeaderListSize(h2::proto::MAX_HEADER_LIST_SIZE),
 maxConcurrentStreams(h2::proto::MAX_COUNT_STREAMS),
 enableConnectProtocol(0), noRfc7540Priorities(1) {}

/**
 * @brief Метод списания токенов
 *
 * @param value число списываемых токенов
 * @return      результат списания (false - токенов не хватает, превышение лимита)
 *
 */
bool awh::http::Parser_HTTP2::Ratelim::drain(const uint64_t value) noexcept {
	// Если токенов не хватает - фиксируем превышение лимита
	if(this->value < value)
		// Токенов не хватает
		return false;
	// Списываем токены
	this->value -= value;
	// Списание выполнено успешно
	return true;
}
/**
 * @brief Метод пополнения токенов по текущему времени
 *
 * @param stamp текущее время (секунды)
 *
 */
void awh::http::Parser_HTTP2::Ratelim::update(const uint64_t stamp) noexcept {
	// Если время не продвинулось вперёд - пополнять нечего
	if(stamp <= this->stamp)
		// Выходим из метода
		return;
	// Вычисляем количество прошедших секунд
	const uint64_t seconds = (stamp - this->stamp);
	// Запоминаем момент обновления
	this->stamp = stamp;
	// Если пополнение отключено - количество токенов не меняется
	if(this->rate == 0)
		// Выходим из метода
		return;
	/**
	 * Скачок часов вперёд переполнил бы произведение, а обёртка урезала бы запас
	 * вместо его пополнения. Считать точное значение не требуется: как только
	 * пополнение перекрывает стартовый запас, результат от него уже не зависит
	 */
	if(seconds > (this->burst / this->rate))
		// Восстанавливаем полный стартовый запас токенов
		this->value = this->burst;
	// Иначе пополняем токены пропорционально прошедшему времени
	else {
		// Пополняем токены пропорционально прошедшему времени
		this->value += (this->rate * seconds);
		// Ограничиваем количество токенов стартовым запасом
		if(this->value > this->burst)
			// Устанавливаем предельное количество токенов
			this->value = this->burst;
	}
}
/**
 * @brief Метод инициализации лимита
 *
 * @param burst стартовый запас токенов
 * @param rate  пополнение токенов в секунду
 *
 */
void awh::http::Parser_HTTP2::Ratelim::init(const uint64_t burst, const uint64_t rate) noexcept {
	// Сбрасываем последний момент обновления
	this->stamp = 0;
	// Устанавливаем скорость пополнения токенов
	this->rate = rate;
	// Устанавливаем стартовый запас токенов и предел пополнения
	this->value = this->burst = burst;
}
/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP2::Ratelim::Ratelim() noexcept :
 rate(0), burst(0), value(0), stamp(0) {}

/**
 * @brief Метод получения логического объёма ещё не отправленных данных тела
 *
 * @return объём не отправленных данных (без учтённого префикса)
 *
 */
size_t awh::http::Parser_HTTP2::Stream::pending() const noexcept {
	// Выводим объём буфера отправки без уже отправленного префикса
	return (this->sendBuffer.size() - this->sendOffset);
}
/**
 * @brief Метод снятия учтённого префикса буфера отправки
 *
 * @details Очистка при полном расходе, иначе амортизированная компактификация
 *
 */
void awh::http::Parser_HTTP2::Stream::compactSendBuffer() noexcept {
	// Если буфер отправки израсходован полностью
	if(this->sendOffset == this->sendBuffer.size()){
		// Сбрасываем отправленный префикс
		this->sendOffset = 0;
		// Очищаем буфер отправки
		this->sendBuffer.clear();
	// Если отправленный префикс не меньше остатка - компактифицируем буфер
	} else if(this->sendOffset >= (this->sendBuffer.size() - this->sendOffset)) {
		// Удаляем отправленный префикс из буфера
		this->sendBuffer.erase(0, this->sendOffset);
		// Сбрасываем отправленный префикс
		this->sendOffset = 0;
	}
}
/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP2::Stream::Stream() noexcept :
 id(0), sourceEof(false), headersDone(false),
 endStreamSent(false), endStreamPending(false),
 writableNotified(false), queued(false), recvBody(0), contentLength(-1), bodyless(false), bodylessSend(false),
 urgency(h2::proto::DEFAULT_URGENCY), incremental(false), prioritized(false),
 localWindow(h2::proto::DEFAULT_WINDOW_SIZE),
 remoteWindow(h2::proto::DEFAULT_WINDOW_SIZE),
 sendOffset(0), sendBuffer{""},
 headersSent(false), trailersPending(false),
 state(h2::stream_state_t::IDLE),
 source(nullptr), headers(nullptr) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP2::Ratelims::Ratelims() noexcept : now(0) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP2::Window::Window() noexcept :
 local(h2::proto::DEFAULT_WINDOW_SIZE),
 remote(h2::proto::DEFAULT_WINDOW_SIZE),
 localMax(h2::proto::DEFAULT_WINDOW_SIZE),
 localInit(h2::proto::DEFAULT_WINDOW_SIZE) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP2::Buffer::Buffer() noexcept :
 input{""}, output{""}, inputPos(0), outputPos(0) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP2::Header_Block::Header_Block() noexcept :
 stream(0), frames(0), promised(0), buffer{""} {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP2::Flags::Flags() noexcept :
 inPump(false), inParse(false),
 goawaySent(false), goawayGraceful(false), hbcRefused(false),
 hbcEndStream(false), settingsAcked(false),
 settingsReceived(false), prioritiesLocked(false), goawayReceived(false),
 prefaceReceived(false) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP2::Transfer::Transfer() noexcept :
 lastStreamId(0),
 localOpened(0),
 nextStreamId(1),
 peerStreamCount(0),
 localStreamCount(0),
 settingsAckPending(0),
 resetStreams(RESET_STREAMS_CACHE),
 resetCursor(0),
 sendLowWater(SEND_LOW_WATER),
 sendHighWater(SEND_HIGH_WATER),
 outputHighWater(OUTPUT_HIGH_WATER) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP2::Callbacks::Callbacks() noexcept :
 data(nullptr), push(nullptr),
 write(nullptr), begin(nullptr),
 close(nullptr), error(nullptr),
 phase(nullptr), header(nullptr),
 goaway(nullptr), writable(nullptr),
 settings(nullptr), provider(nullptr) {}

/**
 * @brief Метод передачи исходящих байтов сетевому слою через функцию обратного вызова записи
 *
 * @details Если функция записи не установлена - байты остаются во внутреннем
 *          буфере до выборки через pending()/consumePending().
 *
 */
void awh::http::Parser_HTTP2::flush() noexcept {
	// Если функция обратного вызова записи не установлена - работаем в pull-модели
	if(this->_callbacks.write == nullptr)
		// Выходим из метода
		return;
	/**
	 * Отдаём исходящие байты, пока они есть: функция обратного вызова могла
	 * реентрантно породить новые исходящие данные (например, через sendData)
	 */
	while(this->outputPending() > 0){
		// Локальный буфер исходящих байтов
		string buffer{""};
		// Забираем буфер исходящих байтов себе (O(1), без копирования)
		buffer.swap(this->_buffer.output);
		// Запоминаем уже отданный префикс буфера
		const size_t offset = this->_buffer.outputPos;
		// Сбрасываем отданный префикс нового (пустого) буфера
		this->_buffer.outputPos = 0;
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Отдаём исходящие байты сетевому слою
			this->_callbacks.write(buffer.data() + offset, buffer.size() - offset);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
			// Прерываем передачу исходящих байтов
			return;
		}
	}
}
/**
 * @brief Метод получения логического объёма ещё не отправленных исходящих байтов
 *
 * @return объём не отправленных исходящих байтов
 *
 */
size_t awh::http::Parser_HTTP2::outputPending() const noexcept {
	// Выводим объём буфера исходящих байтов без уже отданного префикса
	return (this->_buffer.output.size() - this->_buffer.outputPos);
}
/**
 * @brief Метод очистки входного буфера вместе с разобранным префиксом
 *
 */
void awh::http::Parser_HTTP2::clearInput() noexcept {
	// Очищаем буфер неразобранного хвоста входящих данных
	this->_buffer.input.clear();
	// Сбрасываем разобранный префикс входного буфера
	this->_buffer.inputPos = 0;
}
/**
 * @brief Метод получения объёма ещё не разобранных входящих байтов
 *
 * @return объём не разобранных входящих байтов
 *
 */
size_t awh::http::Parser_HTTP2::inputPending() const noexcept {
	// Выводим объём входного буфера без уже разобранного префикса
	return (this->_buffer.input.size() - this->_buffer.inputPos);
}
/**
 * @brief Метод разбора накопленного входного буфера (preface + поток фреймов)
 *
 * @return результат разбора (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::Parser_HTTP2::parseInput() noexcept {
	// Позиция разбора во входном буфере (с учётом уже разобранного префикса)
	size_t pos = this->_buffer.inputPos;
	// Если connection preface ещё не получен (мы - сервер)
	if(!this->_flags.prefaceReceived){
		// Если preface целиком ещё не пришёл - ждём больше данных
		if(this->inputPending() < h2::proto::PREFACE.size())
			// Продолжаем ожидание данных
			return h2::status_t::OK;
		// Если полученные байты не совпадают с magic-строкой preface
		if(string_view(this->_buffer.input.data() + pos, h2::proto::PREFACE.size()) != h2::proto::PREFACE){
			// Очищаем входной буфер
			this->clearInput();
			// Фиксируем ошибку уровня соединения
			return this->fail(error_t::PROTOCOL_ERROR, "invalid connection preface");
		}
		// Пропускаем разобранный preface
		pos += h2::proto::PREFACE.size();
		// Помечаем что preface получен
		this->_flags.prefaceReceived = true;
	}
	/**
	 * Разбор потока фреймов. Указатель и размер буфера перечитываем на каждой
	 * итерации: функция обратного вызова могла реентрантно вызвать parse() и
	 * дописать данные во входной буфер, спровоцировав перевыделение памяти.
	 */
	for(;;){
		// Получаем указатель на входной буфер
		const uint8_t * buffer = reinterpret_cast <const uint8_t *> (this->_buffer.input.data());
		// Получаем общий размер входного буфера
		const size_t total = this->_buffer.input.size();
		/**
		 * Входной буфер мог быть очищен реентрантным вызовом reset() либо clear()
		 * из пользовательской функции: продолжать разбор по устаревшему смещению
		 * нельзя - оно указывает за пределы буфера, а разность размеров переполняется
		 * и обесценивает проверку доступного объёма
		 */
		if(pos > total){
			// Считаем разобранным весь оставшийся буфер
			pos = total;
			// Прерываем разбор
			break;
		}
		// Если заголовок фрейма целиком ещё не пришёл - прерываем разбор
		if((total - pos) < h2::proto::FRAME_HEADER_SIZE)
			// Прерываем разбор до прихода новых данных
			break;
		// Заголовок текущего фрейма
		h2::frame::header_t header{};
		// Выполняем разбор заголовка фрейма
		h2::frame::parser::header(buffer + pos, total - pos, header);
		// Если размер фрейма превышает согласованный лимит (RFC 9113 §4.2)
		if(header.length > this->_local.maxFrameSize){
			// Очищаем входной буфер
			this->clearInput();
			// Фиксируем ошибку уровня соединения
			return this->fail(error_t::FRAME_SIZE_ERROR, "frame exceeds SETTINGS_MAX_FRAME_SIZE");
		}
		// Если фрейм целиком ещё не пришёл - ждём больше данных
		if((total - pos) < (h2::proto::FRAME_HEADER_SIZE + header.length))
			// Прерываем разбор до прихода новых данных
			break;
		// Получаем указатель на полезную нагрузку фрейма
		const uint8_t * payload = (buffer + pos + h2::proto::FRAME_HEADER_SIZE);
		/**
		 * Пока идёт сборка блока заголовков, допустим только CONTINUATION того же потока
		 * (RFC 9113 §6.10) - иначе PROTOCOL_ERROR (защита от перемешивания блоков)
		 */
		if((this->_hbc.stream != 0) && !((header.type == h2::frame_t::CONTINUATION) && (header.streamId == this->_hbc.stream))){
			// Очищаем входной буфер
			this->clearInput();
			// Фиксируем ошибку уровня соединения
			return this->fail(error_t::PROTOCOL_ERROR, "expected CONTINUATION");
		}
		// Выполняем обработку полного фрейма
		const h2::status_t status = this->dispatch(header, payload);
		// Если обработка фрейма завершилась ошибкой уровня соединения
		if(status == h2::status_t::ERROR){
			// Очищаем входной буфер
			this->clearInput();
			// Прерываем разбор с ошибкой
			return h2::status_t::ERROR;
		}
		// Пропускаем разобранный фрейм
		pos += (h2::proto::FRAME_HEADER_SIZE + header.length);
	}
	// Отмечаем разобранный префикс без сдвига всего буфера
	this->_buffer.inputPos = pos;
	// Если входной буфер разобран полностью
	if(this->_buffer.inputPos >= this->_buffer.input.size())
		// Очищаем входной буфер вместе с разобранным префиксом
		this->clearInput();
	// Если разобранный префикс не меньше неразобранного остатка - компактифицируем буфер
	else if(this->_buffer.inputPos >= this->inputPending()) {
		// Удаляем разобранный префикс из буфера
		this->_buffer.input.erase(0, this->_buffer.inputPos);
		// Сбрасываем разобранный префикс
		this->_buffer.inputPos = 0;
	}
	// Разбор выполнен успешно
	return h2::status_t::OK;
}
/**
 * @brief Метод декодирования накопленного блока заголовков и вызова функций обратного вызова
 *
 * @return результат обработки (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::Parser_HTTP2::deliverHeaders() noexcept {
	// Запоминаем идентификатор потока собираемого блока
	const uint32_t streamId = this->_hbc.stream;
	// Запоминаем идентификатор обещанного потока
	const uint32_t promised = this->_hbc.promised;
	// Запоминаем флаг отклонённого потока
	const bool refused = this->_flags.hbcRefused;
	// Запоминаем флаг END_STREAM собираемого блока
	const bool endStream = this->_flags.hbcEndStream;
	// Код ошибки протокола
	error_t err = error_t::NO_ERROR;
	// Получаем ссылку на переиспользуемый список декодированных заголовков блока
	vector <h2::hpack::field_view_t> & fields = this->_fields;
	/**
	 * Лимит суммарного размера распакованного списка заголовков (защита от decompression
	 * bomb). В обоих источниках 0 означает "без лимита", поэтому берётся не минимум,
	 * а строжайший из заданных: иначе maxHeadersTotal == 0 обнулял бы и лимит SETTINGS
	 */
	uint64_t listLimit = this->_limits.maxHeadersTotal;
	// Если лимит SETTINGS_MAX_HEADER_LIST_SIZE задан
	if(this->_local.maxHeaderListSize != 0){
		// Применяем его, если общий лимит не задан либо менее строг
		if((listLimit == 0) || (this->_local.maxHeaderListSize < listLimit))
			// Применяем лимит из наших параметров SETTINGS
			listLimit = this->_local.maxHeaderListSize;
	}
	// Выполняем декодирование накопленного блока заголовков
	const h2::status_t status = this->_decoder.decode(this->_hbc.buffer, fields, listLimit, err);
	// Сбрасываем идентификатор потока собираемого блока (сборка завершена)
	this->_hbc.stream = 0;
	// Сбрасываем счётчик фреймов блока
	this->_hbc.frames = 0;
	// Сбрасываем идентификатор обещанного потока
	this->_hbc.promised = 0;
	// Очищаем накопитель блока заголовков
	this->_hbc.buffer.clear();
	// Сбрасываем флаг отклонённого потока
	this->_flags.hbcRefused = false;
	// Сбрасываем флаг END_STREAM собираемого блока
	this->_flags.hbcEndStream = false;
	// Ошибка HPACK - это всегда ошибка соединения (таблица рассинхронизирована)
	if(status != h2::status_t::OK)
		// Фиксируем ошибку уровня соединения
		return this->fail(err, "HPACK decode failed");
	/**
	 * Список заголовков превысил лимит. Блок при этом разобран целиком, динамическая
	 * таблица синхронна, поэтому рвать соединение незачем - отвергаем только этот поток
	 * (RFC 9113 §10.5.1): иначе одно сообщение с раздутыми заголовками уносило бы
	 * с собой все остальные потоки соединения
	 */
	if(this->_decoder.overflowed()){
		// Если блок принадлежит PUSH_PROMISE - отвергаем обещанный поток
		const uint32_t target = ((promised != 0) ? promised : streamId);
		// Если поток отклонён ранее - сбрасывать нечего
		if(!refused){
			// Отклоняем поток с кодом чрезмерного поведения
			this->rejectStream(target, error_t::ENHANCE_YOUR_CALM);
			// Закрываем поток с вызовом функции обратного вызова закрытия
			this->closeStream(target, error_t::ENHANCE_YOUR_CALM);
		}
		// Обработка блока завершена (соединение живёт)
		return h2::status_t::OK;
	}
	// Поток отклонён по лимиту: блок декодирован (HPACK синхронен), но событий нет
	if(refused)
		// Обработка блока завершена
		return h2::status_t::OK;
	// Блок принадлежит PUSH_PROMISE - это обещанный запрос для отдельного потока
	if(promised != 0)
		// Выполняем доставку декодированного блока обещанного запроса
		return this->deliverPushPromise(streamId, promised, fields);
	// Поток уже создан и провалидирован в обработчике HEADERS
	stream_t * stream = this->findStream(streamId);
	// Если поток исчез - внутренняя ошибка
	if(stream == nullptr)
		// Фиксируем ошибку уровня соединения
		return this->fail(error_t::INTERNAL_ERROR, "stream vanished");
	// Повторный HEADERS на потоке - это трейлеры
	const bool isTrailers = stream->headersDone;
	// Определяем принадлежность блока: запрос клиента (мы - сервер) или ответ сервера
	const bool isRequest = (this->_direct == direct_t::REQUEST);
	// Выполняем валидацию HTTP-семантики блока заголовков (RFC 9113 §8)
	const error_t vErr = ::validateHeaders(fields, isRequest, isTrailers, (this->_local.enableConnectProtocol != 0), this->_fmk);
	// Если блок заголовков малформирован
	if(vErr != error_t::NO_ERROR){
		// Малформированный запрос/ответ - потоковая ошибка (RFC 9113 §8.1.1), соединение живёт
		this->rejectStream(streamId, vErr);
		// Закрываем поток с вызовом функции обратного вызова закрытия
		this->closeStream(streamId, vErr);
		// Обработка блока завершена
		return h2::status_t::OK;
	}
	// Если декодированные заголовки превышают лимиты безопасности
	if(!this->checkHeaderLimits(fields)){
		// Превышение лимитов - потоковая ошибка, соединение живёт
		this->rejectStream(streamId, error_t::ENHANCE_YOUR_CALM);
		// Закрываем поток с вызовом функции обратного вызова закрытия
		this->closeStream(streamId, error_t::ENHANCE_YOUR_CALM);
		// Обработка блока завершена
		return h2::status_t::OK;
	}
	/**
	 * Признак информационного (1xx) ответа сервера: он промежуточный, за ним обязан
	 * прийти финальный блок заголовков, поэтому поток не считается получившим заголовки
	 * и фазы приёма сообщения по нему не начинаются (RFC 9113 §8.1)
	 */
	bool informational = false;
	// Если разбирается запрос клиента и это не блок трейлеров
	if(isRequest && !isTrailers){
		/**
		 * Выполняем поиск псевдо-заголовка метода (его формат уже провалидирован)
		 */
		for(const h2::hpack::field_view_t & field : fields){
			// Если псевдо-заголовок метода запроса найден
			if(field.name == header::METHOD){
				// Ответ на запрос методом HEAD содержимого не несёт (RFC 9110 §9.3.2)
				if(field.value == value::HEAD)
					// Помечаем что отправляемое по потоку сообщение тела нести не может
					stream->bodylessSend = true;
				// Прекращаем поиск
				break;
			}
		}
	// Если разбирается ответ сервера и это не блок трейлеров
	} else if(!isRequest && !isTrailers) {
		/**
		 * Выполняем поиск псевдо-заголовка статуса (его формат уже провалидирован)
		 */
		for(const h2::hpack::field_view_t & field : fields){
			// Если псевдо-заголовок статуса найден
			if(field.name == header::STATUS){
				// Информационным считается ответ с кодом 1xx
				informational = (field.value.front() == '1');
				/**
				 * Ответы 204 и 304 тела не несут, но content-length в них допустим
				 * и описывает тело, которого не будет (RFC 9110 §8.6, §15.4.5)
				 */
				if((field.value == value::NO_CONTENT) || (field.value == value::NOT_MODIFIED))
					// Помечаем что сообщение не может нести тело
					stream->bodyless = true;
				// Прекращаем поиск
				break;
			}
		}
	}
	// Если информационный ответ пытается завершить поток - это малформированный ответ
	if(informational && endStream){
		// Малформированный ответ - потоковая ошибка, соединение живёт
		this->rejectStream(streamId, error_t::PROTOCOL_ERROR);
		// Закрываем поток с вызовом функции обратного вызова закрытия
		this->closeStream(streamId, error_t::PROTOCOL_ERROR);
		// Обработка блока завершена
		return h2::status_t::OK;
	}
	// Если это первый блок заголовков потока - собираем провайдер из псевдо-заголовков
	if(!isTrailers){
		/**
		 * Запоминаем объявленную длину тела: её расхождение с суммой длин DATA делает
		 * сообщение малформированным (RFC 9113 §8.1.1). Формат значения уже
		 * провалидирован, повторы согласованы между собой. Промежуточный ответ 1xx
		 * тела не описывает, поэтому его content-length не учитывается и не должен
		 * пережить приход финального блока заголовков
		 */
		stream->contentLength = -1;
		/**
		 * Выполняем поиск объявленной длины тела в финальном блоке заголовков
		 */
		for(const h2::hpack::field_view_t & field : fields){
			// Промежуточный ответ тела не описывает - его длину не учитываем
			if(informational)
				// Прекращаем поиск
				break;
			// Если получен заголовок объявленной длины тела
			if(field.name == header::CONTENT_LENGTH){
				// Объявленная длина тела
				uint64_t declared = 0;
				/**
				 * Выполняем перебор всех цифр объявленной длины тела
				 */
				for(const char letter : field.value){
					// Если накопление переполняет допустимый диапазон - лимит и так будет превышен
					if(declared > ((UINT64_MAX - static_cast <uint64_t> (letter - '0')) / 10)){
						// Ограничиваем длину тела максимально возможной
						declared = UINT64_MAX;
						// Прекращаем накопление
						break;
					}
					// Накапливаем объявленную длину тела
					declared = ((declared * 10) + static_cast <uint64_t> (letter - '0'));
				}
				// Запоминаем объявленную длину тела потока
				stream->contentLength = static_cast <int64_t> (::min(declared, static_cast <uint64_t> (INT64_MAX)));
				// Прекращаем поиск
				break;
			}
		}
		/**
		 * Применяем расширенный приоритет, объявленный заголовком (RFC 9218 §5).
		 * Кадр PRIORITY_UPDATE перекрывает любой другой сигнал приоритета (§7),
		 * причём независимо от порядка прихода: кадр вправе опередить секцию
		 * заголовков, и решать эту гонку RFC предписывает в его пользу
		 */
		if(!stream->prioritized){
			/**
			 * Выполняем поиск заголовка расширенного приоритета
			 */
			for(const h2::hpack::field_view_t & field : fields){
				// Если получен заголовок расширенного приоритета
				if(field.name == header::PRIORITY){
					// Применяем расширенный приоритет к потоку
					this->applyPriority(* stream, field.value);
					// Прекращаем поиск
					break;
				}
			}
		}
		// Выполняем построение провайдера заголовков потока
		stream->headers = this->buildProvider(fields, isRequest);
		// Если блок является финальным - уведомляем о начале приёма сообщения потока
		if(!informational && !this->firePhase(streamId, phase_t::BEGIN, part_t::NONE))
			// Обработка блока завершена (поток сброшен, соединение живёт)
			return h2::status_t::OK;
	// Если это блок трейлеров - тело потока принято, начинаются трейлеры
	} else {
		// Уведомляем о завершении приёма тела потока (трейлеры приходят только после тела)
		if(!this->firePhase(streamId, phase_t::END, part_t::BODY))
			// Обработка блока завершена (поток сброшен, соединение живёт)
			return h2::status_t::OK;
		// Уведомляем о начале приёма трейлеров потока
		if(!this->firePhase(streamId, phase_t::BEGIN, part_t::TRAILER))
			// Обработка блока завершена (поток сброшен, соединение живёт)
			return h2::status_t::OK;
	}
	// Если функция обратного вызова установлена
	if(this->_callbacks.header != nullptr){
		// Запоминаем поколение состояния соединения перед доставкой заголовков
		const uint64_t epoch = this->_epoch;
		// Определяем часть сообщения, к которой относятся заголовки
		const part_t part = (isTrailers ? part_t::TRAILER : part_t::HEADERS);
		/**
		 * Выполняем доставку всех декодированных заголовков блока
		 */
		for(const h2::hpack::field_view_t & field : fields){
			// Если функция обратного вызова потребовала сбросить поток
			if(!this->fireHeader(streamId, field.name, field.value, part)){
				// Если поток ещё существует (функция обратного вызова могла его закрыть)
				if(this->findStream(streamId) != nullptr){
					// Сбрасываем поток с кодом CANCEL
					this->rejectStream(streamId, error_t::CANCEL);
					// Закрываем поток с вызовом функции обратного вызова закрытия
					this->closeStream(streamId, error_t::CANCEL);
				}
				// Обработка блока завершена (соединение живёт)
				return h2::status_t::OK;
			}
			/**
			 * Функция обратного вызова могла реентрантно сбросить парсер: список
			 * заголовков и арена декодера при этом уничтожены, продолжать перебор нельзя
			 */
			if(epoch != this->_epoch)
				// Обработка блока завершена
				return h2::status_t::OK;
		}
	}
	/**
	 * Функция обратного вызова могла реентрантно закрыть поток (sendRstStream) и
	 * удалить его из карты - перечитываем указатель, иначе запись по нему = use-after-free
	 */
	stream = this->findStream(streamId);
	// Если поток удалён - обработка блока завершена
	if(stream == nullptr)
		// Обработка блока завершена
		return h2::status_t::OK;
	/**
	 * Помечаем что блок заголовков потока получен (повторный HEADERS = трейлеры).
	 * Информационный ответ таким блоком не является - следующий HEADERS будет финальным
	 */
	if(!informational)
		// Помечаем что блок заголовков потока получен
		stream->headersDone = true;
	// Если функция обратного вызова потребовала сбросить поток
	if(!this->fireProvider(streamId, (isTrailers ? nullptr : stream->headers.get()), endStream)){
		// Если поток ещё существует (функция обратного вызова могла его закрыть)
		if(this->findStream(streamId) != nullptr){
			// Сбрасываем поток с кодом CANCEL
			this->rejectStream(streamId, error_t::CANCEL);
			// Закрываем поток с вызовом функции обратного вызова закрытия
			this->closeStream(streamId, error_t::CANCEL);
		}
		// Обработка блока завершена (соединение живёт)
		return h2::status_t::OK;
	}
	/**
	 * Признак безтелесности снимается до выхода наружу: обработчик фазы вправе
	 * закрыть поток либо сбросить весь парсер, и объект потока к следующей строке
	 * уже не существует. Значение при этом не устареет - оно выведено из блока
	 * заголовков, который уже разобран
	 */
	const bool bodyless = stream->bodyless;
	// Если это первый финальный блок заголовков потока
	if(!isTrailers && !informational){
		// Уведомляем о завершении приёма блока заголовков потока
		if(!this->firePhase(streamId, phase_t::END, part_t::HEADERS))
			// Обработка блока завершена (поток сброшен, соединение живёт)
			return h2::status_t::OK;
		/**
		 * Если END_STREAM не получен - за заголовками последует тело. Исключение -
		 * безтелесные сообщения (ответ на HEAD, статусы 204 и 304): там тела не будет
		 * независимо от кадрирования, и фаза приёма тела попросту солгала бы
		 */
		if(!endStream && !bodyless){
			// Уведомляем о начале приёма тела потока
			if(!this->firePhase(streamId, phase_t::BEGIN, part_t::BODY))
				// Обработка блока завершена (поток сброшен, соединение живёт)
				return h2::status_t::OK;
		}
	// Если это блок трейлеров
	} else if(isTrailers) {
		// Уведомляем о завершении приёма трейлеров потока
		if(!this->firePhase(streamId, phase_t::END, part_t::TRAILER))
			// Обработка блока завершена (поток сброшен, соединение живёт)
			return h2::status_t::OK;
	}
	// Если получен END_STREAM - сообщение потока полностью принято
	if(endStream){
		// Если объём принятого тела не совпал с объявленным - поток сброшен
		if(!this->checkBodyLength(streamId))
			// Обработка блока завершена (соединение живёт)
			return h2::status_t::OK;
		// Уведомляем о завершении приёма всего сообщения потока
		if(!this->firePhase(streamId, phase_t::END, part_t::NONE))
			// Обработка блока завершена (поток сброшен, соединение живёт)
			return h2::status_t::OK;
	}
	/**
	 * Переход по END_STREAM может закрыть и удалить поток - выполняем последним
	 * и тоже после перечитывания (функция обратного вызова могла удалить поток)
	 */
	if(endStream){
		// Перечитываем указатель на поток
		stream_t * stream = this->findStream(streamId);
		// Если поток ещё существует
		if(stream != nullptr)
			// Применяем полученный END_STREAM (ссылка на поток может стать недействительной)
			this->applyRemoteEndStream(* stream);
	}
	// Обработка блока завершена
	return h2::status_t::OK;
}
/**
 * @brief Метод аварийного завершения соединения (ошибка, GOAWAY, запись в лог)
 *
 * @param code    код ошибки протокола
 * @param message текстовое описание ошибки
 * @return        статус ошибки (для проброса из обработчиков)
 *
 */
awh::http::h2::status_t awh::http::Parser_HTTP2::fail(const error_t code, const char * message) noexcept {
	/**
	 * Сбрасываем незавершённую сборку блока заголовков: иначе после ошибки любой
	 * следующий не-CONTINUATION фрейм залипал бы в PROTOCOL_ERROR без восстановления
	 */
	this->_hbc.stream = 0;
	// Сбрасываем счётчик фреймов блока
	this->_hbc.frames = 0;
	// Сбрасываем идентификатор обещанного потока
	this->_hbc.promised = 0;
	// Очищаем накопитель блока заголовков
	this->_hbc.buffer.clear();
	// Сбрасываем флаг отклонённого потока
	this->_flags.hbcRefused = false;
	// Сбрасываем флаг END_STREAM собираемого блока
	this->_flags.hbcEndStream = false;
	// Фиксируем код ошибки уровня соединения
	this->_error = code;
	// Устанавливаем итоговый статус разбора
	this->_status = status_t::ERROR;
	// Записываем сообщение об ошибке разбора в лог
	this->_log->print(
		"HTTP/2 %s parsing failed: %s [%s]",
		log_t::flag_t::WARNING,
		(this->_direct == direct_t::REQUEST ? "request" : "response"),
		message, h2::errorName(code)
	);
	// Запоминаем поколение состояния соединения перед уведомлением
	const uint64_t epoch = this->_epoch;
	// Если функция обратного вызова установлена
	if(this->_callbacks.error != nullptr){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Уведомляем об ошибке уровня соединения
			this->_callbacks.error(code, message);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (code), message), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	/**
	 * Функция обратного вызова могла реентрантно сбросить парсер, переиспользуя его
	 * под новое соединение: GOAWAY относится к прежнему, а в очереди нового он занял бы
	 * место перед connection preface и был бы отвергнут пиром
	 */
	if(epoch != this->_epoch)
		// Выводим статус ошибки для проброса из обработчиков
		return h2::status_t::ERROR;
	// Ставим GOAWAY в очередь отправки (соединение необходимо закрыть)
	this->sendGoaway(code);
	// Выводим статус ошибки для проброса из обработчиков
	return h2::status_t::ERROR;
}
/**
 * @brief Метод проверки корректности нового потока, открываемого пиром (чётность + монотонность id)
 *
 * @param id  идентификатор потока
 * @param err код ошибки протокола
 * @return    результат проверки (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::Parser_HTTP2::validateNewStream(const uint32_t id, error_t & err) noexcept {
	/**
	 * Поток инициирует пир: для нас-сервера это клиент (нечётные идентификаторы),
	 * для нас-клиента это сервер (чётные идентификаторы, server push) - RFC 9113 §5.1.1
	 */
	const bool peerOdd = (this->_direct == direct_t::REQUEST);
	// Если чётность идентификатора не соответствует инициатору
	if(((id & 1u) != 0) != peerOdd){
		// Фиксируем код ошибки протокола
		err = error_t::PROTOCOL_ERROR;
		// Проверка не пройдена
		return h2::status_t::ERROR;
	}
	// Идентификатор обязан строго возрастать (повтор/уменьшение - ошибка)
	if(id <= this->_transfer.lastStreamId){
		// Фиксируем код ошибки протокола
		err = error_t::PROTOCOL_ERROR;
		// Проверка не пройдена
		return h2::status_t::ERROR;
	}
	// Проверка пройдена успешно
	return h2::status_t::OK;
}
/**
 * @brief Метод обработки одного полного фрейма
 *
 * @param header  заголовок фрейма
 * @param payload полезная нагрузка фрейма (ровно h.length байт)
 * @return        результат обработки (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::Parser_HTTP2::dispatch(const h2::frame::header_t & header, const uint8_t * payload) noexcept {
	// Код ошибки протокола
	error_t err = error_t::NO_ERROR;
	/**
	 * Первым фреймом соединения пир обязан прислать SETTINGS - это часть
	 * connection preface для обеих сторон (RFC 9113 §3.4)
	 */
	if(!this->_flags.settingsReceived && (header.type != h2::frame_t::SETTINGS))
		// Фиксируем ошибку уровня соединения
		return this->fail(error_t::PROTOCOL_ERROR, "expected SETTINGS as first frame");
	/**
	 * Диспетчеризация по типу фрейма
	 */
	switch(header.type){
		// Фрейм параметров соединения (RFC 9113 §6.5)
		case h2::frame_t::SETTINGS: {
			// Список разобранных параметров SETTINGS
			vector <h2::frame::setting_entry_t> items;
			// Если разбор полезной нагрузки завершился ошибкой
			if(h2::frame::parser::settings(header, payload, items, err) != h2::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(err, "bad SETTINGS");
			/**
			 * Если получен ACK на наш SETTINGS. Обычный SETTINGS пира флаг подтверждения
			 * не трогает: наш SETTINGS остаётся подтверждённым независимо от него
			 */
			if((header.flags & h2::flag::ACK) != 0){
				/**
				 * Подтверждение на SETTINGS, которого мы не отправляли, - ошибка
				 * соединения (RFC 9113 §6.5.3). Без счётчика неподтверждённых отправок
				 * пир слал бы пустые ACK бесплатно: они не несут параметров и потому
				 * не попадают ни под какой другой учёт
				 */
				if(this->_transfer.settingsAckPending == 0)
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::PROTOCOL_ERROR, "unexpected SETTINGS ACK");
				// Пополняем лимит частоты управляющих фреймов по текущему времени
				this->_ratelims.ctrl.update(this->_ratelims.now);
				// Если лимит частоты управляющих фреймов превышен (защита от flood подтверждениями)
				if(!this->_ratelims.ctrl.drain(1))
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::ENHANCE_YOUR_CALM, "SETTINGS flood");
				// Списываем одно ожидаемое подтверждение
				--this->_transfer.settingsAckPending;
				/**
				 * Подтверждённым наш SETTINGS считается только когда неподтверждённых
				 * отправок не осталось: обвязка вправе отправить SETTINGS дважды подряд,
				 * и первый же ACK иначе объявил бы подтверждённой вторую отправку -
				 * таймер SETTINGS_TIMEOUT перестал бы её сторожить
				 */
				this->_flags.settingsAcked = (this->_transfer.settingsAckPending == 0);
				/**
				 * Подтверждение не является объявлением параметров и требование
				 * connection preface не закрывает (RFC 9113 §3.4): пир по-прежнему
				 * обязан прислать свой SETTINGS до любого содержательного кадра
				 */
				return h2::status_t::OK;
			}
			// Помечаем что SETTINGS пира получен (connection preface выполнен)
			this->_flags.settingsReceived = true;
			// Пополняем лимит частоты управляющих фреймов по текущему времени
			this->_ratelims.ctrl.update(this->_ratelims.now);
			// Если лимит частоты управляющих фреймов превышен (защита от flood)
			if(!this->_ratelims.ctrl.drain(1))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::ENHANCE_YOUR_CALM, "SETTINGS flood");
			/**
			 * Выполняем применение всех полученных параметров SETTINGS пира
			 */
			for(const auto & item : items){
				/**
				 * Диспетчеризация по идентификатору параметра
				 */
				switch(item.id){
					// Размер динамической таблицы HPACK
					case h2::setting_t::HEADER_TABLE_SIZE: {
						/**
						 * RFC 7541 §4.2: пир сообщил, какую таблицу готов держать его декодер -
						 * ограничиваем свой кодер и ставим Dynamic Table Size Update
						 */
						this->_remote.headerTableSize = item.value;
						// Ограничиваем размер таблицы нашего кодера
						this->_encoder.setMaxTableSize(item.value);
					} break;
					// Разрешение server push
					case h2::setting_t::ENABLE_PUSH: {
						// Значение параметра обязано быть 0 или 1
						if(item.value > 1)
							// Фиксируем ошибку уровня соединения
							return this->fail(error_t::PROTOCOL_ERROR, "invalid ENABLE_PUSH");
						/**
						 * Параметром распоряжается только клиент: сервер push не принимает,
						 * поэтому анонсировать своё согласие ему нечем и значение, отличное
						 * от нуля, обязано рвать соединение (RFC 9113 §6.5.2)
						 */
						if((this->_direct == direct_t::RESPONSE) && (item.value != 0))
							// Фиксируем ошибку уровня соединения
							return this->fail(error_t::PROTOCOL_ERROR, "server sent ENABLE_PUSH");
						// Применяем полученное значение параметра
						this->_remote.enablePush = item.value;
					} break;
					// Лимит одновременных потоков
					case h2::setting_t::MAX_CONCURRENT_STREAMS:
						// Применяем полученное значение параметра
						this->_remote.maxConcurrentStreams = item.value;
					break;
					// Начальное окно потока
					case h2::setting_t::INITIAL_WINDOW_SIZE: {
						// Значение окна не может превышать максимально допустимое (RFC 9113 §6.9.2)
						if(item.value > static_cast <uint32_t> (h2::proto::MAX_WINDOW_SIZE))
							// Фиксируем ошибку уровня соединения
							return this->fail(error_t::FLOW_CONTROL_ERROR, "INITIAL_WINDOW_SIZE too large");
						/**
						 * RFC 9113 §6.9.2: изменение начального окна сдвигает окна отправки
						 * всех открытых потоков на дельту (окно может стать отрицательным)
						 */
						const int32_t newInit = static_cast <int32_t> (item.value);
						// Вычисляем дельту изменения начального окна
						const int64_t delta = (static_cast <int64_t> (newInit) - this->_remote.windowSize);
						// Применяем полученное значение параметра
						this->_remote.windowSize = newInit;
						/**
						 * Выполняем сдвиг окон отправки всех открытых потоков
						 */
						for(auto & item : this->_transfer.streams){
							// Вычисляем новое окно отправки потока
							const int64_t window = (static_cast <int64_t> (item.second.remoteWindow) + delta);
							// Если новое окно превышает максимально допустимое
							if(window > h2::proto::MAX_WINDOW_SIZE)
								// Фиксируем ошибку уровня соединения
								return this->fail(error_t::FLOW_CONTROL_ERROR, "stream window overflow on SETTINGS");
							// Применяем новое окно отправки потока
							item.second.remoteWindow = static_cast <int32_t> (window);
						}
					} break;
					// Максимальный размер фрейма
					case h2::setting_t::MAX_FRAME_SIZE: {
						// Значение параметра обязано укладываться в допустимый диапазон
						if((item.value < h2::proto::MIN_MAX_FRAME_SIZE) || (item.value > h2::proto::MAX_MAX_FRAME_SIZE))
							// Фиксируем ошибку уровня соединения
							return this->fail(error_t::PROTOCOL_ERROR, "invalid MAX_FRAME_SIZE");
						// Применяем полученное значение параметра
						this->_remote.maxFrameSize = item.value;
					} break;
					// Лимит размера списка заголовков
					case h2::setting_t::MAX_HEADER_LIST_SIZE:
						// Применяем полученное значение параметра
						this->_remote.maxHeaderListSize = item.value;
					break;
					// Разрешение расширенного метода CONNECT (RFC 8441 §3)
					case h2::setting_t::ENABLE_CONNECT_PROTOCOL: {
						// Значение параметра обязано быть 0 или 1
						if(item.value > 1)
							// Фиксируем ошибку уровня соединения
							return this->fail(error_t::PROTOCOL_ERROR, "invalid ENABLE_CONNECT_PROTOCOL");
						/**
						 * Отзыв уже данного разрешения запрещён (RFC 8441 §3): клиент мог
						 * начать формировать расширенный CONNECT, полагаясь на анонс
						 */
						if((this->_remote.enableConnectProtocol == 1) && (item.value == 0))
							// Фиксируем ошибку уровня соединения
							return this->fail(error_t::PROTOCOL_ERROR, "ENABLE_CONNECT_PROTOCOL revoked");
						// Применяем полученное значение параметра
						this->_remote.enableConnectProtocol = item.value;
					} break;
					// Отказ от приоритетов RFC 7540 (RFC 9218 §2.1)
					case h2::setting_t::NO_RFC7540_PRIORITIES: {
						// Значение параметра обязано быть 0 или 1
						if(item.value > 1)
							// Фиксируем ошибку уровня соединения
							return this->fail(error_t::PROTOCOL_ERROR, "invalid NO_RFC7540_PRIORITIES");
						/**
						 * Значение параметра фиксируется на всё соединение и не может меняться
						 * после того, как было объявлено (RFC 9218 §2.1). Проверка привязана
						 * к самому параметру, а не к порядку кадров: пир вправе прислать
						 * SETTINGS ACK раньше собственного SETTINGS
						 */
						if(this->_flags.prioritiesLocked && (this->_remote.noRfc7540Priorities != item.value))
							// Фиксируем ошибку уровня соединения
							return this->fail(error_t::PROTOCOL_ERROR, "NO_RFC7540_PRIORITIES changed");
						// Применяем полученное значение параметра
						this->_remote.noRfc7540Priorities = item.value;
						// Фиксируем объявленное пиром значение параметра
						this->_flags.prioritiesLocked = true;
					} break;
				}
			}
			// Подтверждаем получение SETTINGS пира (ACK)
			h2::frame::serialize::settings(this->_buffer.output, nullptr, 0, true);
			// Уведомляем о применённом SETTINGS пира
			this->fireSettings();
			// Изменение окон могло разблокировать отправку - прокачиваем отправку
			this->pump();
			// Обработка фрейма завершена
			return h2::status_t::OK;
		}
		// Фрейм проверки живости соединения (RFC 9113 §6.7)
		case h2::frame_t::PING: {
			// Opaque-данные фрейма PING
			uint8_t opaque[8];
			// Если разбор полезной нагрузки завершился ошибкой
			if(h2::frame::parser::ping(header, payload, opaque, err) != h2::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(err, "bad PING");
			/**
			 * Лимит частоты списывается и на запрос, и на подтверждение. Запрос
			 * требует от нас ответного кадра - это прямое усиление. Подтверждение
			 * ответа не требует и потому не попадает ни под какой другой учёт,
			 * но разбор каждого кадра стоит нам работы, а пиру - ничего
			 */
			// Пополняем лимит частоты управляющих фреймов по текущему времени
			this->_ratelims.ctrl.update(this->_ratelims.now);
			// Если лимит частоты управляющих фреймов превышен
			if(!this->_ratelims.ctrl.drain(1))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::ENHANCE_YOUR_CALM, "PING flood");
			// Если получен PING без флага ACK - требуется ответ
			if((header.flags & h2::flag::ACK) == 0)
				// Отвечаем фреймом PING с флагом ACK
				h2::frame::serialize::ping(this->_buffer.output, opaque, true);
			// Обработка фрейма завершена
			return h2::status_t::OK;
		}
		// Фрейм обновления окна flow control (RFC 9113 §6.9)
		case h2::frame_t::WINDOW_UPDATE: {
			// Инкремент окна flow control
			uint32_t increment = 0;
			// Если разбор полезной нагрузки завершился ошибкой
			if(h2::frame::parser::windowUpdate(header, payload, increment, err) != h2::status_t::OK){
				/**
				 * Нулевой инкремент на потоке - потоковая ошибка, соединение живёт
				 * (RFC 9113 §6.9). Некорректная длина фрейма и нулевой инкремент
				 * на самом соединении остаются ошибками уровня соединения
				 */
				if((header.streamId != 0) && (err == error_t::PROTOCOL_ERROR) && !this->idleStream(header.streamId)){
					// Сбрасываем поток с кодом нарушения протокола
					this->rejectStream(header.streamId, err);
					// Закрываем поток с вызовом функции обратного вызова закрытия
					this->closeStream(header.streamId, err);
					// Обработка фрейма завершена (соединение живёт)
					return h2::status_t::OK;
				}
				// Фиксируем ошибку уровня соединения
				return this->fail(err, "bad WINDOW_UPDATE");
			}
			// Если обновляется окно всего соединения
			if(header.streamId == 0){
				// Если новое окно превышает максимально допустимое
				if((static_cast <int64_t> (this->_window.remote) + increment) > h2::proto::MAX_WINDOW_SIZE)
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::FLOW_CONTROL_ERROR, "connection window overflow");
				// Применяем инкремент окна соединения
				this->_window.remote += static_cast <int32_t> (increment);
			// Если обновляется окно конкретного потока
			} else {
				// Выполняем поиск потока
				stream_t * stream = this->findStream(header.streamId);
				// WINDOW_UPDATE на ещё не открытом (idle) потоке - ошибка соединения (RFC 9113 §5.1)
				if((stream == nullptr) && this->idleStream(header.streamId))
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::PROTOCOL_ERROR, "WINDOW_UPDATE on idle stream");
				// Если поток найден
				if(stream != nullptr){
					/**
					 * Переполнение окна отправки потока обрывает только этот поток: само
					 * соединение исправно, и RFC 9113 §6.9.1 требует здесь именно
					 * RST_STREAM, оставляя GOAWAY для переполнения окна соединения
					 */
					if((static_cast <int64_t> (stream->remoteWindow) + increment) > h2::proto::MAX_WINDOW_SIZE){
						// Сбрасываем поток с кодом переполнения окна
						this->rejectStream(header.streamId, error_t::FLOW_CONTROL_ERROR);
						// Закрываем поток с вызовом функции обратного вызова закрытия
						this->closeStream(header.streamId, error_t::FLOW_CONTROL_ERROR);
						// Обработка фрейма завершена (соединение живёт)
						return h2::status_t::OK;
					}
					// Применяем инкремент окна потока
					stream->remoteWindow += static_cast <int32_t> (increment);
				}
			}
			// Окно открылось - досылаем отложенные данные
			this->pump();
			// Обработка фрейма завершена
			return h2::status_t::OK;
		}
		// Фрейм завершения соединения (RFC 9113 §6.8)
		case h2::frame_t::GOAWAY: {
			// Разобранная полезная нагрузка GOAWAY
			h2::frame::goaway_t goaway;
			// Если разбор полезной нагрузки завершился ошибкой
			if(h2::frame::parser::goaway(header, payload, goaway, err) != h2::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(err, "bad GOAWAY");
			// Помечаем что GOAWAY получен
			this->_flags.goawayReceived = true;
			// Уведомляем о полученном GOAWAY
			this->fireGoaway(goaway.lastStreamId, goaway.code, goaway.debugData);
			// Очищаем снимок идентификаторов закрываемых потоков
			this->_transfer.closeIds.clear();
			/**
			 * Собираем наши потоки с идентификатором выше объявленного: пир их не обработал
			 * и уже не обработает (RFC 9113 §6.8), поэтому их можно безопасно повторить
			 * на новом соединении. Снимок нужен, так как закрытие меняет карту потоков
			 */
			for(const auto & item : this->_transfer.streams){
				// Если поток инициирован нами и пиром не обработан
				if(!this->peerInitiated(item.first) && (item.first > goaway.lastStreamId))
					// Добавляем идентификатор потока в снимок
					this->_transfer.closeIds.push_back(item.first);
			}
			// Запоминаем поколение состояния соединения перед закрытием потоков
			const uint64_t epoch = this->_epoch;
			/**
			 * Выполняем закрытие всех необработанных пиром потоков. Перебор идёт по индексу,
			 * а не итератором: пользовательская функция обратного вызова закрытия работает
			 * с живым объектом парсера, и любое перевыделение снимка оставило бы висячие
			 * итераторы прямо посреди перебора
			 */
			for(size_t i = 0; i < this->_transfer.closeIds.size(); i++){
				// Получаем идентификатор закрываемого потока
				const uint32_t sid = this->_transfer.closeIds[i];
				// Закрываем поток с кодом отклонения (запрос можно повторить)
				this->closeStream(sid, error_t::REFUSED_STREAM);
				// Если функция обратного вызова закрытия сбросила парсер - снимок недействителен
				if(epoch != this->_epoch)
					// Прекращаем закрытие потоков
					break;
			}
			// Обработка фрейма завершена
			return h2::status_t::OK;
		}
		// Фрейм аварийного закрытия потока (RFC 9113 §6.4)
		case h2::frame_t::RST_STREAM: {
			// Код ошибки, с которым сброшен поток
			error_t code = error_t::NO_ERROR;
			// Если разбор полезной нагрузки завершился ошибкой
			if(h2::frame::parser::rstStream(header, payload, code, err) != h2::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(err, "bad RST_STREAM");
			// RST_STREAM на ещё не открытом (idle) потоке - ошибка соединения (RFC 9113 §5.1)
			if((this->findStream(header.streamId) == nullptr) && this->idleStream(header.streamId))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::PROTOCOL_ERROR, "RST_STREAM on idle stream");
			// Пополняем лимит частоты входящих RST_STREAM по текущему времени
			this->_ratelims.rst.update(this->_ratelims.now);
			// Если лимит частоты входящих RST_STREAM превышен (защита от Rapid Reset, CVE-2023-44487)
			if(!this->_ratelims.rst.drain(1))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::ENHANCE_YOUR_CALM, "RST_STREAM flood (Rapid Reset)");
			// Запоминаем поток, оборванный сбросом пира: кадры на нём ещё могут быть в полёте
			this->markReset(header.streamId, false);
			// Закрываем поток с полученным кодом ошибки
			this->closeStream(header.streamId, code);
			// Обработка фрейма завершена
			return h2::status_t::OK;
		}
		// Фрейм блока заголовков (RFC 9113 §6.2)
		case h2::frame_t::HEADERS: {
			// Разобранная полезная нагрузка HEADERS
			h2::frame::headers_t headers;
			// Если разбор полезной нагрузки завершился ошибкой
			if(h2::frame::parser::headers(header, payload, headers, err) != h2::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(err, "bad HEADERS");
			/**
			 * Поток не может зависеть от самого себя (RFC 9113 §5.3.1). Поля приоритета
			 * из HEADERS в остальном игнорируются как устаревшие, но замкнутая на себя
			 * зависимость обязана быть отвергнута потоковой ошибкой. Отвергается именно
			 * поток, а не кадр: блок заголовков обязан быть декодирован в любом случае,
			 * иначе динамическая таблица HPACK разъедется с кодером пира и первая же
			 * ссылка на её запись оборвёт живое соединение
			 */
			const bool selfDependent = (headers.hasPriority && (headers.streamDep == header.streamId));
			// Флаг отклонённого потока (блок декодируем только для синхронизации HPACK)
			bool refused = false;
			// Запоминаем поколение состояния соединения перед пользовательскими вызовами
			const uint64_t epoch = this->_epoch;
			// Выполняем поиск потока
			stream_t * stream = this->findStream(header.streamId);
			// Если поток ещё не существует - пир открывает новый поток
			if(stream == nullptr){
				/**
				 * Блок заголовков на потоке, недавно оборванном сбросом, - потоковая ошибка,
				 * а не ошибка соединения (RFC 9113 §5.1): его отправили до того, как сброс
				 * дошёл до отправителя. Инициатор потока здесь роли не играет - клиент так же
				 * вправе получить запоздалый ответ на поток, который сбросил сам. Блок
				 * при этом обязан быть декодирован, иначе
				 * динамическая таблица HPACK разъедется с кодером пира. Период, в течение
				 * которого кадры игнорируются, ограничен размером кольца: за его пределами
				 * поток снова считается неизвестным, как и допускает тот же параграф
				 */
				if(this->wasReset(header.streamId)){
					/**
					 * На сброс пира отвечаем потоковой ошибкой, на свой - молчим: пир уже
					 * знает, что поток закрыт, и повторный кадр сброса ему ни о чём не сообщит
					 */
					if(!this->resetLocally(header.streamId))
						// Отклоняем поток с кодом закрытого потока
						this->rejectStream(header.streamId, error_t::STREAM_CLOSED);
					// Помечаем что поток отклонён (блок декодируется только ради синхронизации HPACK)
					refused = true;
				}
				/**
				 * Поток, завершённый END_STREAM с обеих сторон, пир закрыл сам и знает
				 * об этом, поэтому блок заголовков на нём - ошибка соединения (§5.1).
				 * Для потока пира речь только о последнем использованном идентификаторе:
				 * меньшие пир не вправе использовать вовсе - это переиспользование (§5.1.1).
				 * Для нашего собственного потока признак другой: идентификатор не выше
				 * наибольшего нами открытого означает поток, который мы уже использовали
				 * и закрыли, а больший - поток в состоянии idle, и его отвергает проверка
				 * чётности ниже. Без этого различения запоздалый ответ на наш завершённый
				 * поток уходил бы в проверку чётности и рвал соединение с PROTOCOL_ERROR
				 * вместо STREAM_CLOSED, которого требует тот же параграф
				 */
				else if(this->peerInitiated(header.streamId) ?
				        (header.streamId == this->_transfer.lastStreamId) :
				        (header.streamId <= this->_transfer.localOpened))
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::STREAM_CLOSED, "HEADERS on closed stream");
				// Проверяем чётность и монотонность идентификатора нового потока
				else if(this->validateNewStream(header.streamId, err) != h2::status_t::OK)
					// Фиксируем ошибку уровня соединения
					return this->fail(err, "invalid new stream id");
				// Если пир открывает новый поток
				else {
					// Запоминаем наибольший принятый идентификатор потока
					this->_transfer.lastStreamId = header.streamId;
					/**
					 * Отклоняем новый поток, если: он замкнут зависимостью на себя
					 * (RFC 9113 §5.3.1), исчерпан лимит одновременных потоков (§5.1.2)
					 * либо мы уже отправили GOAWAY (§6.8) - новые потоки пира после этого
					 * не обслуживаются. Это потоковая ошибка, соединение остаётся живым;
					 * блок заголовков всё равно декодируем
					 */
					if(selfDependent || this->_flags.goawaySent || (this->_transfer.peerStreamCount >= this->_local.maxConcurrentStreams)){
						// Отклоняем поток: замкнутая зависимость - нарушение протокола, исчерпание лимита - отказ в потоке
						this->rejectStream(header.streamId, (selfDependent ? error_t::PROTOCOL_ERROR : error_t::REFUSED_STREAM));
						// Помечаем что поток отклонён
						refused = true;
					// Если поток может быть открыт
					} else {
						// Учитываем поток в лимите одновременных потоков пира
						++this->_transfer.peerStreamCount;
						// Получаем объект нового потока
						stream_t & stream = this->stream(header.streamId);
						// Переводим поток в состояние OPEN
						stream.state = h2::stream_state_t::OPEN;
						/**
						 * Применяем приоритет, объявленный кадром PRIORITY_UPDATE до открытия
						 * потока (RFC 9218 §7.1). Делается это до разбора блока заголовков:
						 * заголовок [priority] пришёл позже кадра и обязан его перекрыть -
						 * так же поступает эталонная реализация
						 */
						this->applyPendingPriority(stream);
						// Если функция обратного вызова потребовала отклонить поток
						if(!this->fireBegin(header.streamId)){
							/**
							 * Функция обратного вызова могла реентрантно сбросить парсер: сброс потока
							 * относится к прежнему соединению, а в очереди нового этот кадр занял бы
							 * место перед connection preface
							 */
							if(epoch != this->_epoch)
								// Обработка фрейма завершена
								return h2::status_t::OK;
							// Сбрасываем поток с кодом CANCEL
							this->rejectStream(header.streamId, error_t::CANCEL);
							// Закрываем поток с вызовом функции обратного вызова закрытия
							this->closeStream(header.streamId, error_t::CANCEL);
							// Помечаем что поток отклонён
							refused = true;
						}
					}
				}
			// Если поток уже существует - HEADERS на существующем потоке
			} else {
				/**
				 * Диспетчеризация по состоянию потока
				 */
				switch(stream->state){
					// Поток зарезервирован пиром через PUSH_PROMISE
					case h2::stream_state_t::RESERVED_REMOTE:
						// Ответ на server push: reserved(remote) -> half-closed(local)
						stream->state = h2::stream_state_t::HALF_CLOSED_LOCAL;
					break;
					// Поток открыт либо наша половина закрыта
					case h2::stream_state_t::OPEN:
					case h2::stream_state_t::HALF_CLOSED_LOCAL: {
						/**
						 * Повторный HEADERS - это трейлеры, они обязаны нести END_STREAM
						 * (RFC 9113 §8.1). Их отсутствие делает сообщение малформированным,
						 * а это потоковая ошибка (§8.1.1): соединение живёт, но блок
						 * заголовков всё равно декодируется для синхронизации HPACK
						 */
						if(stream->headersDone && !headers.endStream){
							// Сбрасываем поток с кодом нарушения протокола
							this->rejectStream(header.streamId, error_t::PROTOCOL_ERROR);
							// Закрываем поток с вызовом функции обратного вызова закрытия
							this->closeStream(header.streamId, error_t::PROTOCOL_ERROR);
							// Помечаем что поток отклонён (события по нему не порождаем)
							refused = true;
						}
					} break;
					// Половина пира закрыта
					case h2::stream_state_t::HALF_CLOSED_REMOTE: {
						/**
						 * HEADERS на закрытой половине потока - потоковая ошибка (RFC 9113 §5.1),
						 * соединение живёт. Блок заголовков всё равно принимаем и декодируем,
						 * иначе динамическая таблица HPACK рассинхронизируется
						 */
						this->rejectStream(header.streamId, error_t::STREAM_CLOSED);
						// Закрываем поток с вызовом функции обратного вызова закрытия
						this->closeStream(header.streamId, error_t::STREAM_CLOSED);
						// Помечаем что поток отклонён (события по нему не порождаем)
						refused = true;
					} break;
					// Поток закрыт с обеих сторон
					case h2::stream_state_t::CLOSED:
						/**
						 * Кадр после принятого END_STREAM - ошибка соединения (RFC 9113 §5.1),
						 * в отличие от закрытой половины потока выше. Ветка защитная и через
						 * карту потоков недостижима: состояние CLOSED присваивается только
						 * рядом с closeStream(), а тот стирает поток из карты до возврата.
						 * Рабочий путь для завершённого потока - ветка stream == nullptr выше
						 */
						return this->fail(error_t::STREAM_CLOSED, "HEADERS on closed stream");
					// Остальные состояния для HEADERS недопустимы
					default:
						// Фиксируем ошибку уровня соединения
						return this->fail(error_t::PROTOCOL_ERROR, "HEADERS in invalid stream state");
				}
				/**
				 * Замкнутая на себя зависимость на существующем потоке (RFC 9113 §5.3.1).
				 * Проверяется после состояния потока: поток, уже отклонённый по состоянию,
				 * сброшен и закрыт, и второй кадр сброса на нём пиру не нужен
				 */
				if(selfDependent && !refused){
					// Сбрасываем поток с кодом нарушения протокола
					this->rejectStream(header.streamId, error_t::PROTOCOL_ERROR);
					// Закрываем поток с вызовом функции обратного вызова закрытия
					this->closeStream(header.streamId, error_t::PROTOCOL_ERROR);
					// Помечаем что поток отклонён (события по нему не порождаем)
					refused = true;
				}
			}
			/**
			 * Пользовательская функция обратного вызова могла реентрантно сбросить парсер:
			 * фрагмент блока ссылается во входной буфер прежнего соединения, а его сборка
			 * и декодирование наполнили бы динамическую таблицу HPACK нового соединения
			 * записями, которых пир не присылал
			 */
			if(epoch != this->_epoch)
				// Обработка фрейма завершена
				return h2::status_t::OK;
			// Учитываем первый фрейм блока
			this->_hbc.frames = 1;
			// Начинаем сборку блока заголовков потока
			this->_hbc.stream = header.streamId;
			// Помещаем первый фрагмент блока в накопитель
			this->_hbc.buffer.assign(headers.block.data(), headers.block.size());
			// Запоминаем флаг отклонённого потока
			this->_flags.hbcRefused = refused;
			// Запоминаем флаг END_STREAM собираемого блока
			this->_flags.hbcEndStream = headers.endStream;
			// Защита от CONTINUATION flood (2024): лимит размера блока заголовков
			if(this->_hbc.buffer.size() > this->_limits.maxHeaderBlockSize)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::ENHANCE_YOUR_CALM, "header block too large");
			// Если блок заголовков завершён - декодируем и доставляем его
			if(headers.endHeaders)
				// Выполняем декодирование и доставку блока заголовков
				return this->deliverHeaders();
			// Иначе ждём фреймы CONTINUATION
			return h2::status_t::OK;
		}
		// Фрейм продолжения блока заголовков (RFC 9113 §6.10)
		case h2::frame_t::CONTINUATION: {
			// Фрагмент блока заголовков
			string_view block{};
			// Флаг завершения блока заголовков
			bool endHeaders = false;
			// CONTINUATION вне сборки блока заголовков недопустим
			if(this->_hbc.stream == 0)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::PROTOCOL_ERROR, "unexpected CONTINUATION");
			// Если разбор полезной нагрузки завершился ошибкой
			if(h2::frame::parser::continuation(header, payload, block, endHeaders, err) != h2::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(err, "bad CONTINUATION");
			// Защита от CONTINUATION flood (2024): лимит числа фреймов в блоке заголовков
			if(++this->_hbc.frames > this->_limits.maxContinuationFrames)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::ENHANCE_YOUR_CALM, "too many CONTINUATION frames");
			// Дописываем фрагмент блока в накопитель
			this->_hbc.buffer.append(block.data(), block.size());
			// Защита от CONTINUATION flood (2024): лимит размера блока заголовков
			if(this->_hbc.buffer.size() > this->_limits.maxHeaderBlockSize)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::ENHANCE_YOUR_CALM, "header block too large");
			// Если блок заголовков завершён - декодируем и доставляем его
			if(endHeaders)
				// Выполняем декодирование и доставку блока заголовков
				return this->deliverHeaders();
			// Иначе ждём следующие фреймы CONTINUATION
			return h2::status_t::OK;
		}
		// Фрейм данных тела (RFC 9113 §6.1)
		case h2::frame_t::DATA: {
			// Разобранная полезная нагрузка DATA
			h2::frame::data_t data{};
			// Если разбор полезной нагрузки завершился ошибкой
			if(h2::frame::parser::data(header, payload, data, err) != h2::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(err, "bad DATA");
			/**
			 * Flow control приёма учитывает ПОЛНУЮ длину полезной нагрузки, включая padding
			 * (RFC 9113 §6.9.1), и делается до разбора состояния потока: принятые байты
			 * списываются с окна соединения независимо от судьбы самого потока
			 */
			if(static_cast <int32_t> (header.length) > this->_window.local)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::FLOW_CONTROL_ERROR, "connection receive window exhausted");
			// Выполняем поиск потока
			stream_t * stream = this->findStream(header.streamId);
			// Если поток не найден
			if(stream == nullptr){
				// DATA на ещё не открытом (idle) потоке - ошибка соединения (RFC 9113 §5.1)
				if(this->idleStream(header.streamId))
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::PROTOCOL_ERROR, "DATA on idle stream");
				/**
				 * Поток уже закрыт и удалён: пир мог отправить данные до того, как получил
				 * наш RST_STREAM. Это потоковая ошибка (RFC 9113 §6.1), соединение живёт,
				 * но принятые байты обязаны быть возвращены в окно приёма соединения
				 */
				this->replenishReceiveWindow(nullptr, header.length);
				/**
				 * Поток, оборванный нашим сбросом, пир уже закрыл: кадры на нём обязаны быть
				 * просто проигнорированы (RFC 9113 §5.1). Поток, закрытый завершением обмена
				 * либо сброшенный самим пиром, отвергается потоковой ошибкой (§6.1)
				 */
				if(!this->resetLocally(header.streamId))
					// Сбрасываем поток с кодом STREAM_CLOSED
					this->rejectStream(header.streamId, error_t::STREAM_CLOSED);
				// Обработка фрейма завершена (соединение живёт)
				return h2::status_t::OK;
			}
			/**
			 * Зарезервированный под push поток тела не несёт: до блока заголовков он
			 * принимает только RST_STREAM, PRIORITY и WINDOW_UPDATE, поэтому DATA на нём
			 * означает рассинхронизацию и является ошибкой соединения (RFC 9113 §5.1)
			 */
			if((stream->state == h2::stream_state_t::RESERVED_LOCAL) || (stream->state == h2::stream_state_t::RESERVED_REMOTE))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::PROTOCOL_ERROR, "DATA on reserved stream");
			// Данные принимаем только в состояниях OPEN и HALF_CLOSED_LOCAL (RFC 9113 §5.1)
			if((stream->state != h2::stream_state_t::OPEN) && (stream->state != h2::stream_state_t::HALF_CLOSED_LOCAL)){
				// Пополняем окно приёма соединения (поток закрывается)
				this->replenishReceiveWindow(nullptr, header.length);
				// Сбрасываем поток с кодом STREAM_CLOSED (потоковая ошибка, RFC 9113 §6.1)
				this->rejectStream(header.streamId, error_t::STREAM_CLOSED);
				// Закрываем поток с вызовом функции обратного вызова закрытия
				this->closeStream(header.streamId, error_t::STREAM_CLOSED);
				// Обработка фрейма завершена (соединение живёт)
				return h2::status_t::OK;
			}
			/**
			 * Тело не может прийти раньше финального блока заголовков сообщения
			 * (RFC 9113 §8.1). Поток мог быть открыт нами и ещё не получить ответ,
			 * либо нести только промежуточный ответ 1xx - в обоих случаях DATA
			 * малформирован. Это потоковая ошибка, соединение живёт
			 */
			if(!stream->headersDone){
				// Пополняем окно приёма соединения (поток закрывается)
				this->replenishReceiveWindow(nullptr, header.length);
				// Сбрасываем поток как малформированный
				this->rejectStream(header.streamId, error_t::PROTOCOL_ERROR);
				// Закрываем поток с вызовом функции обратного вызова закрытия
				this->closeStream(header.streamId, error_t::PROTOCOL_ERROR);
				// Обработка фрейма завершена (соединение живёт)
				return h2::status_t::OK;
			}
			/**
			 * Сообщение, которое тела нести не может (ответ на HEAD, статусы 204 и 304),
			 * с непустым телом малформировано (RFC 9110 §9.3.2, §15.3.5, §15.4.5).
			 * Пустой DATA допустим - содержимого он не добавляет
			 */
			if(stream->bodyless && !data.data.empty()){
				// Пополняем окно приёма соединения (поток закрывается)
				this->replenishReceiveWindow(nullptr, header.length);
				// Сбрасываем поток как малформированный
				this->rejectStream(header.streamId, error_t::PROTOCOL_ERROR);
				// Закрываем поток с вызовом функции обратного вызова закрытия
				this->closeStream(header.streamId, error_t::PROTOCOL_ERROR);
				// Обработка фрейма завершена (соединение живёт)
				return h2::status_t::OK;
			}
			/**
			 * Защита от flood DATA-фреймами без полезной нагрузки и без END_STREAM:
			 * учитываем фактические данные (после снятия padding), иначе padding-only
			 * кадры обходили бы лимит, прокачивая окно туда-обратно (бесполезная CPU-нагрузка)
			 */
			if(data.data.empty() && !data.endStream){
				// Пополняем лимит частоты управляющих фреймов по текущему времени
				this->_ratelims.ctrl.update(this->_ratelims.now);
				// Если лимит частоты управляющих фреймов превышен
				if(!this->_ratelims.ctrl.drain(1))
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::ENHANCE_YOUR_CALM, "empty DATA flood");
			}
			// Если окно приёма потока исчерпано
			if(static_cast <int32_t> (header.length) > stream->localWindow)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::FLOW_CONTROL_ERROR, "stream receive window exhausted");
			// Учитываем принятые данные в суммарном размере тела потока
			stream->recvBody += data.data.size();
			// Если принято больше, чем объявлено заголовком content-length (RFC 9113 §8.1.1)
			if(!stream->bodyless && (stream->contentLength >= 0) && (stream->recvBody > static_cast <uint64_t> (stream->contentLength))){
				// Пополняем окно приёма соединения (поток закрывается)
				this->replenishReceiveWindow(nullptr, header.length);
				// Сбрасываем поток как малформированный (потоковая ошибка)
				this->rejectStream(header.streamId, error_t::PROTOCOL_ERROR);
				// Закрываем поток с вызовом функции обратного вызова закрытия
				this->closeStream(header.streamId, error_t::PROTOCOL_ERROR);
				// Обработка фрейма завершена (соединение живёт)
				return h2::status_t::OK;
			}
			// Если суммарный размер тела потока превысил лимит безопасности
			if(stream->recvBody > this->_limits.maxBodySize){
				// Пополняем окно приёма соединения (поток закрывается)
				this->replenishReceiveWindow(nullptr, header.length);
				// Сбрасываем поток с кодом ENHANCE_YOUR_CALM
				this->rejectStream(header.streamId, error_t::ENHANCE_YOUR_CALM);
				// Закрываем поток с вызовом функции обратного вызова закрытия
				this->closeStream(header.streamId, error_t::ENHANCE_YOUR_CALM);
				// Обработка фрейма завершена (соединение живёт)
				return h2::status_t::OK;
			}
			// Запоминаем флаг END_STREAM (поток может быть удалён функцией обратного вызова)
			const bool endStream = data.endStream;
			// Запоминаем поколение состояния соединения перед доставкой тела
			const uint64_t epoch = this->_epoch;
			// Если функция обратного вызова потребовала сбросить поток
			if(!this->fireData(header.streamId, data.data.data(), data.data.size(), endStream)){
				/**
				 * Функция обратного вызова могла реентрантно сбросить парсер: принятые байты
				 * учтены в окне прежнего соединения, а WINDOW_UPDATE попал бы в очередь нового
				 * раньше его connection preface
				 */
				if(epoch != this->_epoch)
					// Обработка фрейма завершена
					return h2::status_t::OK;
				// Пополняем окно приёма соединения (поток закрывается)
				this->replenishReceiveWindow(nullptr, header.length);
				// Если поток ещё существует (функция обратного вызова могла его закрыть)
				if(this->findStream(header.streamId) != nullptr){
					// Сбрасываем поток с кодом CANCEL
					this->rejectStream(header.streamId, error_t::CANCEL);
					// Закрываем поток с вызовом функции обратного вызова закрытия
					this->closeStream(header.streamId, error_t::CANCEL);
				}
				// Обработка фрейма завершена (соединение живёт)
				return h2::status_t::OK;
			}
			// Если парсер сброшен реентрантно - окно прежнего соединения больше не ведётся
			if(epoch != this->_epoch)
				// Обработка фрейма завершена
				return h2::status_t::OK;
			// Перечитываем указатель на поток (функция обратного вызова могла его удалить)
			stream = this->findStream(header.streamId);
			// Пополняем окно приёма и при просадке шлём WINDOW_UPDATE (потоку - только если он остаётся открыт)
			this->replenishReceiveWindow(((endStream || (stream == nullptr)) ? nullptr : stream), header.length);
			// Если получен END_STREAM и поток ещё существует
			if(endStream && (stream != nullptr)){
				// Если объём принятого тела не совпал с объявленным - поток сброшен
				if(!this->checkBodyLength(header.streamId))
					// Обработка фрейма завершена (соединение живёт)
					return h2::status_t::OK;
				// Уведомляем о завершении приёма тела потока
				if(!this->firePhase(header.streamId, phase_t::END, part_t::BODY))
					// Обработка фрейма завершена (поток сброшен, соединение живёт)
					return h2::status_t::OK;
				// Уведомляем о завершении приёма всего сообщения потока
				if(!this->firePhase(header.streamId, phase_t::END, part_t::NONE))
					// Обработка фрейма завершена (поток сброшен, соединение живёт)
					return h2::status_t::OK;
				// Перечитываем указатель на поток (функция обратного вызова могла его удалить)
				stream = this->findStream(header.streamId);
				// Если поток ещё существует
				if(stream != nullptr)
					// Применяем полученный END_STREAM (ссылка на поток может стать недействительной)
					this->applyRemoteEndStream(* stream);
			}
			// Обработка фрейма завершена
			return h2::status_t::OK;
		}
		// Фрейм приоритета (RFC 9113 §6.3)
		case h2::frame_t::PRIORITY: {
			/**
			 * Кадры приоритета состояния не меняют и потоков не открывают, поэтому их
			 * поток ограничивается отдельным лимитом: он заметно щедрее лимита управляющих
			 * фреймов, так как переустановка приоритетов на каждый загружаемый ресурс -
			 * штатное поведение клиента
			 */
			this->_ratelims.prio.update(this->_ratelims.now);
			// Если лимит частоты кадров приоритета превышен
			if(!this->_ratelims.prio.drain(1))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::ENHANCE_YOUR_CALM, "PRIORITY flood");
			// Разобранная полезная нагрузка PRIORITY
			h2::frame::priority_t priority;
			// Если разбор полезной нагрузки завершился ошибкой
			if(h2::frame::parser::priority(header, payload, priority, err) != h2::status_t::OK){
				/**
				 * Некорректная длина PRIORITY - потоковая ошибка (RFC 9113 §6.3): сбрасываем
				 * поток, если он существует, иначе просто игнорируем deprecated-фрейм.
				 * Нулевой идентификатор потока остаётся ошибкой соединения
				 */
				if(err == error_t::FRAME_SIZE_ERROR){
					/**
					 * Сброс отправляется независимо от того, существует ли поток: кадр
					 * приоритета допустим в любом состоянии, включая закрытое, поэтому
					 * молчание в ответ на некорректную длину нарушает RFC 9113 §6.3
					 */
					this->rejectStream(header.streamId, err);
					// Закрываем поток, если он существует
					this->closeStream(header.streamId, err);
					// Обработка фрейма завершена (соединение живёт)
					return h2::status_t::OK;
				}
				// Фиксируем ошибку уровня соединения
				return this->fail(err, "bad PRIORITY");
			}
			/**
			 * Поток не может зависеть от самого себя (RFC 9113 §5.3.1): это потоковая
			 * ошибка. Сами приоритеты RFC 7540 объявлены устаревшими и игнорируются,
			 * но проверка обязательна - иначе граница зависимостей у пира замкнётся
			 */
			if(priority.streamDep == header.streamId){
				// Сбрасываем поток с кодом нарушения протокола
				this->rejectStream(header.streamId, error_t::PROTOCOL_ERROR);
				// Закрываем поток, если он существует
				this->closeStream(header.streamId, error_t::PROTOCOL_ERROR);
				// Обработка фрейма завершена (соединение живёт)
				return h2::status_t::OK;
			}
			// Приоритеты RFC 7540 deprecated - игнорируем
			return h2::status_t::OK;
		}
		// Фрейм обновления расширенного приоритета потока (RFC 9218 §7.1)
		case h2::frame_t::PRIORITY_UPDATE: {
			/**
			 * Приоритет объявляет только клиент: сервер отправлять кадр не вправе,
			 * и получивший его клиент обязан оборвать соединение (RFC 9218 §7.1).
			 * Проверка стоит до разбора нагрузки - отвергается сам факт кадра,
			 * а не его содержимое
			 */
			if(this->_direct == direct_t::RESPONSE)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::PROTOCOL_ERROR, "client received PRIORITY_UPDATE");
			// Пополняем лимит частоты кадров приоритета по текущему времени
			this->_ratelims.prio.update(this->_ratelims.now);
			// Если лимит частоты кадров приоритета превышен
			if(!this->_ratelims.prio.drain(1))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::ENHANCE_YOUR_CALM, "PRIORITY_UPDATE flood");
			// Идентификатор приоритизируемого потока
			uint32_t sid = 0;
			// Значение поля приоритета
			string_view value{};
			// Если разбор полезной нагрузки завершился ошибкой
			if(h2::frame::parser::priorityUpdate(header, payload, sid, value, err) != h2::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(err, "bad PRIORITY_UPDATE");
			/**
			 * Нулевой идентификатор не принадлежит ни одному потоку и приоритизирован
			 * быть не может (RFC 9218 §7.1). Проверка обязана быть явной: по чётности
			 * ноль неотличим от потока, инициированного сервером
			 */
			if(sid == 0)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::PROTOCOL_ERROR, "PRIORITY_UPDATE for stream 0");
			// Выполняем поиск потока
			stream_t * stream = this->findStream(sid);
			// Если поток уже открыт
			if(stream != nullptr){
				// Применяем расширенный приоритет к потоку
				this->applyPriority(* stream, value);
				// Помечаем что приоритет потока задан кадром
				stream->prioritized = true;
			/**
			 * Кадр приоритизирует и поток запроса, и поток push (RFC 9218 §7.1),
			 * поэтому инициатор потока сам по себе решает мало - решает состояние.
			 * Наш собственный поток, идентификатор которого ещё не выдан, для сервера
			 * означает push в состоянии idle, а его §7.1 запрещает прямо. Выданный
			 * и уже закрытый поток сигнал просто отбрасывает: приоритизировать нечего,
			 * и тот же параграф это разрешает
			 */
			} else if(!this->peerInitiated(sid)){
				// Если поток нами ещё не выдан
				if(this->idleStream(sid))
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::PROTOCOL_ERROR, "PRIORITY_UPDATE for idle push stream");
			}
			/**
			 * Кадр допустим и для потока в состоянии idle: сигнал вправе опередить
			 * HEADERS и обязан примениться при открытии потока (RFC 9218 §7.1).
			 * Объект потока под него не создаём - до прихода HEADERS это позволило бы
			 * пиру заполнить карту потоков даром; запись кладётся в кольцо, ёмкость
			 * которого и ограничивает цену такого сигнала. Идентификатор не выше
			 * наибольшего принятого относится к потоку, который уже открывался:
			 * приоритет закрытого потока смысла не имеет и не запоминается
			 */
			else if(sid > this->_transfer.lastStreamId){
				// Если запомнить приоритет не удалось - лимит одновременных потоков исчерпан
				if(!this->deferPriority(sid, value))
					// Фиксируем ошибку уровня соединения (RFC 9218 §7.1)
					return this->fail(error_t::PROTOCOL_ERROR, "PRIORITY_UPDATE max concurrent streams exceeded");
			}
			// Обработка фрейма завершена
			return h2::status_t::OK;
		}
		// Фрейм анонса server push (RFC 9113 §6.6)
		case h2::frame_t::PUSH_PROMISE: {
			// PUSH_PROMISE отправляет только сервер; принимает только клиент (RFC 9113 §8.4)
			if(this->_direct == direct_t::REQUEST)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::PROTOCOL_ERROR, "server received PUSH_PROMISE");
			// Мы запретили push своим SETTINGS_ENABLE_PUSH=0 - пир не вправе пушить
			if(this->_local.enablePush == 0)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::PROTOCOL_ERROR, "PUSH_PROMISE while push disabled");
			// Разобранная полезная нагрузка PUSH_PROMISE
			h2::frame::push_promise_t promise{};
			// Если разбор полезной нагрузки завершился ошибкой
			if(h2::frame::parser::pushPromise(header, payload, promise, err) != h2::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(err, "bad PUSH_PROMISE");
			// Ассоциированный поток (на котором пришёл промис) должен существовать и быть живым
			stream_t * assoc = this->findStream(header.streamId);
			/**
			 * Промис допустим только на потоке, который мы открыли и который ещё не
			 * закрыт нами (RFC 9113 §6.6): в нашей системе координат это состояния
			 * open и half-closed(local). Промис на чужом либо на push-потоке
			 * означает нарушение протокола
			 */
			if((assoc == nullptr) ||
			   ((assoc->state != h2::stream_state_t::OPEN) && (assoc->state != h2::stream_state_t::HALF_CLOSED_LOCAL)))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::PROTOCOL_ERROR, "PUSH_PROMISE on invalid stream");
			// Идентификатор обещанного потока: чётный (инициирует сервер) и строго возрастающий
			if(this->validateNewStream(promise.promisedStreamId, err) != h2::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(err, "invalid promised stream id");
			// Запоминаем наибольший принятый идентификатор потока
			this->_transfer.lastStreamId = promise.promisedStreamId;
			/**
			 * Отклоняем обещанный поток, если исчерпан лимит одновременных потоков
			 * (RFC 9113 §5.1.2) либо мы уже отправили GOAWAY. Те же правила, что для
			 * HEADERS: RST_STREAM(REFUSED_STREAM) на обещанном идентификаторе, но блок
			 * всё равно декодируем для синхронизации HPACK. Поток не создаём
			 */
			const bool refusePush = (this->_flags.goawaySent || (this->_transfer.peerStreamCount >= this->_local.maxConcurrentStreams));
			// Если обещанный поток отклоняется
			if(refusePush)
				// Отклоняем обещанный поток с кодом REFUSED_STREAM
				this->rejectStream(promise.promisedStreamId, error_t::REFUSED_STREAM);
			// Если обещанный поток принимается
			else {
				/**
				 * Резервируем обещанный поток: reserved(remote). Он инициирован пиром
				 * (сервером) и учитывается в лимите одновременных потоков (RFC 9113 §5.1.2)
				 */
				stream_t & stream = this->stream(promise.promisedStreamId);
				// Переводим обещанный поток в состояние RESERVED_REMOTE
				stream.state = h2::stream_state_t::RESERVED_REMOTE;
				// Учитываем поток в лимите одновременных потоков пира
				++this->_transfer.peerStreamCount;
			}
			/**
			 * Начинаем сборку блока заголовков обещанного запроса; CONTINUATION придут
			 * на ассоциированном потоке (h.streamId), а заголовки относятся к обещанному
			 */
			this->_hbc.stream = header.streamId;
			// Запоминаем идентификатор обещанного потока (0 - если поток отклонён)
			this->_hbc.promised = (refusePush ? 0 : promise.promisedStreamId);
			// Учитываем первый фрейм блока
			this->_hbc.frames = 1;
			// PUSH_PROMISE не несёт END_STREAM
			this->_flags.hbcEndStream = false;
			// Запоминаем флаг отклонённого потока
			this->_flags.hbcRefused = refusePush;
			// Помещаем первый фрагмент блока в накопитель
			this->_hbc.buffer.assign(promise.block.data(), promise.block.size());
			// Защита от CONTINUATION flood (2024): лимит размера блока заголовков
			if(this->_hbc.buffer.size() > this->_limits.maxHeaderBlockSize)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::ENHANCE_YOUR_CALM, "header block too large");
			// Если блок заголовков завершён - декодируем и доставляем его
			if(promise.endHeaders)
				// Выполняем декодирование и доставку блока заголовков
				return this->deliverHeaders();
			// Иначе ждём фреймы CONTINUATION
			return h2::status_t::OK;
		}
	}
	// Неизвестный тип фрейма - по RFC 9113 §4.1 игнорируется
	return h2::status_t::OK;
}
/**
 * @brief Метод доставки декодированного блока обещанного запроса (PUSH_PROMISE, сторона клиента)
 *
 * @param sid         идентификатор ассоциированного потока клиента
 * @param promisedSid идентификатор обещанного потока
 * @param fields      декодированные заголовки обещанного запроса
 * @return            результат обработки (OK/ERROR)
 *
 */
awh::http::h2::status_t awh::http::Parser_HTTP2::deliverPushPromise(const uint32_t sid, const uint32_t promisedSid, const vector <h2::hpack::field_view_t> & fields) noexcept {
	// Выполняем поиск обещанного потока
	stream_t * stream = this->findStream(promisedSid);
	// Если обещанный поток исчез - внутренняя ошибка
	if(stream == nullptr)
		// Фиксируем ошибку уровня соединения
		return this->fail(error_t::INTERNAL_ERROR, "promised stream vanished");
	// Обещанный блок - это всегда запрос (псевдо-заголовки запроса), без трейлеров
	const error_t vErr = ::validateHeaders(fields, true, false, (this->_local.enableConnectProtocol != 0), this->_fmk);
	// Если блок заголовков малформирован
	if(vErr != error_t::NO_ERROR){
		// Малформированный обещанный запрос - потоковая ошибка, соединение живёт
		this->rejectStream(promisedSid, vErr);
		// Закрываем обещанный поток с вызовом функции обратного вызова закрытия
		this->closeStream(promisedSid, vErr);
		// Обработка блока завершена
		return h2::status_t::OK;
	}
	// Если декодированные заголовки превышают лимиты безопасности
	if(!this->checkHeaderLimits(fields)){
		// Превышение лимитов - потоковая ошибка, соединение живёт
		this->rejectStream(promisedSid, error_t::ENHANCE_YOUR_CALM);
		// Закрываем обещанный поток с вызовом функции обратного вызова закрытия
		this->closeStream(promisedSid, error_t::ENHANCE_YOUR_CALM);
		// Обработка блока завершена
		return h2::status_t::OK;
	}
	// Запоминаем поколение состояния соединения перед пользовательскими вызовами
	const uint64_t epoch = this->_epoch;
	// Если функция обратного вызова потребовала отклонить push
	if(!this->firePush(sid, promisedSid)){
		// Если обещанный поток ещё существует (функция обратного вызова могла его закрыть)
		if(this->findStream(promisedSid) != nullptr){
			// Отклоняем push с кодом CANCEL
			this->rejectStream(promisedSid, error_t::CANCEL);
			// Закрываем обещанный поток с вызовом функции обратного вызова закрытия
			this->closeStream(promisedSid, error_t::CANCEL);
		}
		// Обработка блока завершена (соединение живёт)
		return h2::status_t::OK;
	}
	/**
	 * Функция обратного вызова могла реентрантно сбросить парсер: список заголовков
	 * и арена декодера при этом уничтожены, обращаться к ним нельзя
	 */
	if(epoch != this->_epoch)
		// Обработка блока завершена
		return h2::status_t::OK;
	// Перечитываем указатель на обещанный поток (функция обратного вызова могла его удалить)
	stream = this->findStream(promisedSid);
	// Если обещанный поток удалён - обработка блока завершена
	if(stream == nullptr)
		// Обработка блока завершена
		return h2::status_t::OK;
	// Собираем провайдер обещанного запроса из псевдо-заголовков
	stream->headers = this->buildProvider(fields, true);
	// Если функция обратного вызова установлена
	if(this->_callbacks.header != nullptr){
		/**
		 * Выполняем доставку всех декодированных заголовков обещанного запроса
		 */
		for(const h2::hpack::field_view_t & field : fields){
			// Если функция обратного вызова потребовала сбросить поток
			if(!this->fireHeader(promisedSid, field.name, field.value, part_t::HEADERS)){
				// Если обещанный поток ещё существует (функция обратного вызова могла его закрыть)
				if(this->findStream(promisedSid) != nullptr){
					// Сбрасываем поток с кодом CANCEL
					this->rejectStream(promisedSid, error_t::CANCEL);
					// Закрываем обещанный поток с вызовом функции обратного вызова закрытия
					this->closeStream(promisedSid, error_t::CANCEL);
				}
				// Обработка блока завершена (соединение живёт)
				return h2::status_t::OK;
			}
			// Если парсер сброшен реентрантно - перебор списка заголовков продолжать нельзя
			if(epoch != this->_epoch)
				// Обработка блока завершена
				return h2::status_t::OK;
		}
	}
	// Перечитываем указатель на обещанный поток (функция обратного вызова могла его удалить)
	stream = this->findStream(promisedSid);
	// Если обещанный поток удалён - обработка блока завершена
	if(stream == nullptr)
		// Обработка блока завершена
		return h2::status_t::OK;
	/**
	 * Внимание: headersDone НЕ выставляем - обещанный запрос пришёл фреймом PUSH_PROMISE
	 * на ассоциированном потоке, а не HEADERS на этом. Реальный ответный HEADERS сервера
	 * будет первым на push-потоке (иначе он ошибочно считался бы трейлерами).
	 * Обещанный запрос завершён на END_HEADERS (тела у него нет); endStream = false -
	 * клиент ещё ждёт ответных HEADERS сервера на этом потоке (reserved -> half-closed(local))
	 */
	if(!this->fireProvider(promisedSid, stream->headers.get(), false)){
		// Если обещанный поток ещё существует (функция обратного вызова могла его закрыть)
		if(this->findStream(promisedSid) != nullptr){
			// Сбрасываем поток с кодом CANCEL
			this->rejectStream(promisedSid, error_t::CANCEL);
			// Закрываем обещанный поток с вызовом функции обратного вызова закрытия
			this->closeStream(promisedSid, error_t::CANCEL);
		}
	}
	// Обработка блока завершена
	return h2::status_t::OK;
}
/**
 * @brief Метод получения существующего либо создания нового потока
 *
 * @param id идентификатор потока
 * @return   объект потока
 *
 */
awh::http::Parser_HTTP2::stream_t & awh::http::Parser_HTTP2::stream(const uint32_t id) noexcept {
	// Получаем существующий либо создаём новый объект потока
	stream_t & result = this->_transfer.streams[id];
	// Если поток создан только что (идентификатор ещё не установлен)
	if(result.id == 0){
		// Устанавливаем идентификатор потока
		result.id = id;
		// Устанавливаем окно приёма потока из анонсированного пиру значения
		result.localWindow = this->_window.localInit;
		// Устанавливаем окно отправки потока из параметров SETTINGS пира
		result.remoteWindow = this->_remote.windowSize;
	}
	// Выводим объект потока
	return result;
}
/**
 * @brief Метод поиска потока без создания
 *
 * @param id идентификатор потока
 * @return   объект потока либо nullptr
 *
 */
awh::http::Parser_HTTP2::stream_t * awh::http::Parser_HTTP2::findStream(const uint32_t id) noexcept {
	// Выполняем поиск потока в карте активных потоков
	const auto i = this->_transfer.streams.find(id);
	// Выводим найденный объект потока либо nullptr
	return ((i == this->_transfer.streams.end()) ? nullptr : &i->second);
}
/**
 * @brief Метод применения отправленного нами END_STREAM (переход состояния, возможно закрытие потока)
 *
 * @param stream объект потока (ссылка может стать недействительной после вызова)
 *
 */
void awh::http::Parser_HTTP2::applyLocalEndStream(stream_t & stream) noexcept {
	// RFC 9113 §5.1: отправка END_STREAM закрывает локальную половину потока
	if(stream.state == h2::stream_state_t::OPEN)
		// Переводим поток в состояние HALF_CLOSED_LOCAL
		stream.state = h2::stream_state_t::HALF_CLOSED_LOCAL;
	// Если половина пира уже была закрыта - поток завершён
	else if(stream.state == h2::stream_state_t::HALF_CLOSED_REMOTE) {
		// Запоминаем идентификатор потока
		const uint32_t id = stream.id;
		// Переводим поток в состояние CLOSED
		stream.state = h2::stream_state_t::CLOSED;
		// Закрываем поток штатно (ссылка на поток после этого недействительна)
		this->closeStream(id, error_t::NO_ERROR);
	}
}
/**
 * @brief Метод применения полученного END_STREAM (переход состояния, возможно закрытие потока)
 *
 * @param stream объект потока (ссылка может стать недействительной после вызова)
 *
 */
void awh::http::Parser_HTTP2::applyRemoteEndStream(stream_t & stream) noexcept {
	// RFC 9113 §5.1: получение END_STREAM закрывает удалённую половину потока
	if(stream.state == h2::stream_state_t::OPEN)
		// Переводим поток в состояние HALF_CLOSED_REMOTE
		stream.state = h2::stream_state_t::HALF_CLOSED_REMOTE;
	// Если наша половина уже была закрыта - поток завершён
	else if(stream.state == h2::stream_state_t::HALF_CLOSED_LOCAL) {
		// Запоминаем идентификатор потока
		const uint32_t id = stream.id;
		// Переводим поток в состояние CLOSED
		stream.state = h2::stream_state_t::CLOSED;
		// Закрываем поток штатно (ссылка на поток после этого недействительна)
		this->closeStream(id, error_t::NO_ERROR);
	}
}
/**
 * @brief Метод проверки того, что поток инициирован пиром (а не нами)
 *
 * @param id идентификатор потока
 * @return   результат проверки
 *
 */
bool awh::http::Parser_HTTP2::peerInitiated(const uint32_t id) const noexcept {
	// Пир инициирует нечётные потоки, если мы - сервер (разбираем запросы)
	const bool peerOdd = (this->_direct == direct_t::REQUEST);
	// Выполняем сравнение чётности идентификатора потока
	return (((id & 1u) != 0) == peerOdd);
}
/**
 * @brief Метод проверки того, что поток ещё ни разу не использовался (состояние idle)
 *
 * @param id идентификатор потока
 * @return   результат проверки
 *
 */
bool awh::http::Parser_HTTP2::idleStream(const uint32_t id) const noexcept {
	// Если поток инициирован пиром - он не использован, пока превышает наибольший принятый
	if(this->peerInitiated(id))
		// Выводим признак неиспользованного потока
		return (id > this->_transfer.lastStreamId);
	/**
	 * Наш собственный поток использован, если его идентификатор уже выдан: без этой
	 * ветки любой запоздалый фрейм на нашем закрытом потоке (RST_STREAM/WINDOW_UPDATE
	 * после завершённого обмена - штатное поведение пиров) рвал бы соединение
	 */
	return (id >= this->_transfer.nextStreamId);
}
/**
 * @brief Метод запоминания потока, оборванного сбросом
 *
 * @param id идентификатор потока
 *
 */
void awh::http::Parser_HTTP2::markReset(const uint32_t id, const bool local) noexcept {
	// Нулевой идентификатор не принадлежит потоку и служит признаком пустой ячейки
	if(id == 0)
		// Выходим из метода
		return;
	/**
	 * Выполняем поиск потока в кольце: повторная запись того же идентификатора
	 * вытеснила бы чужой, кадры которого ещё могут быть в полёте
	 */
	for(auto & item : this->_transfer.resetStreams){
		// Если идентификатор не совпадает - переходим к следующей ячейке
		if(item.id != id)
			// Переходим к следующей ячейке кольца
			continue;
		/**
		 * Наш сброс сильнее принятого: после него кадры пира обязаны игнорироваться,
		 * а сбросы двух сторон вправе разойтись в сети и встретиться на одном потоке
		 */
		if(local)
			// Запоминаем что сброс отправлен нами
			item.local = true;
		// Выходим из метода
		return;
	}
	// Записываем идентификатор потока в текущую ячейку кольца
	this->_transfer.resetStreams[this->_transfer.resetCursor].id = id;
	// Записываем источник сброса в текущую ячейку кольца
	this->_transfer.resetStreams[this->_transfer.resetCursor].local = local;
	// Продвигаем позицию записи по кольцу
	this->_transfer.resetCursor = ((this->_transfer.resetCursor + 1) % this->_transfer.resetStreams.size());
}
/**
 * @brief Метод проверки того, что поток был недавно оборван сбросом
 *
 * @param id идентификатор потока
 * @return   результат проверки
 *
 */
bool awh::http::Parser_HTTP2::wasReset(const uint32_t id) const noexcept {
	// Нулевой идентификатор не принадлежит потоку и служит признаком пустой ячейки
	if(id == 0)
		// Выводим отрицательный результат
		return false;
	/**
	 * Перебор, а не поиск по множеству: кольцо короткое и лежит одним куском памяти,
	 * а проверка нужна только на редком пути - кадр на потоке, которого нет в карте
	 */
	for(const auto & item : this->_transfer.resetStreams){
		// Если идентификатор найден в кольце
		if(item.id == id)
			// Выводим положительный результат
			return true;
	}
	// Выводим отрицательный результат
	return false;
}
/**
 * @brief Метод проверки того, что поток был оборван нашим сбросом
 *
 * @param id идентификатор потока
 * @return   результат проверки
 *
 */
bool awh::http::Parser_HTTP2::resetLocally(const uint32_t id) const noexcept {
	// Нулевой идентификатор не принадлежит потоку и служит признаком пустой ячейки
	if(id == 0)
		// Выводим отрицательный результат
		return false;
	/**
	 * Выполняем перебор кольца оборванных сбросом потоков
	 */
	for(const auto & item : this->_transfer.resetStreams){
		// Если идентификатор найден в кольце
		if(item.id == id)
			// Выводим признак того, что сброс отправлен нами
			return item.local;
	}
	// Выводим отрицательный результат
	return false;
}
/**
 * @brief Метод отправки RST_STREAM с учётом оборванного потока
 *
 * @param id   идентификатор потока
 * @param code код ошибки, с которым обрывается поток
 *
 */
void awh::http::Parser_HTTP2::rejectStream(const uint32_t id, const error_t code) noexcept {
	// Запоминаем поток, оборванный нашим сбросом: кадры на нём ещё могут быть в полёте
	this->markReset(id, true);
	// Отправляем фрейм сброса потока
	h2::frame::serialize::rstStream(this->_buffer.output, id, code);
}
/**
 * @brief Метод удаления потока из карты с корректным учётом счётчика встречных потоков
 *
 * @param id идентификатор потока
 *
 */
void awh::http::Parser_HTTP2::eraseStream(const uint32_t id) noexcept {
	// Выполняем поиск потока в карте активных потоков
	const auto i = this->_transfer.streams.find(id);
	// Если поток не найден - удалять нечего
	if(i == this->_transfer.streams.end())
		// Выходим из метода
		return;
	// Если поток инициирован пиром - освобождаем слот в его лимите одновременных потоков
	if(this->peerInitiated(id)){
		// Если счётчик активных потоков пира не пуст
		if(this->_transfer.peerStreamCount > 0)
			// Уменьшаем счётчик активных потоков пира
			--this->_transfer.peerStreamCount;
	// Иначе освобождаем слот в лимите одновременных потоков, разрешённом нам пиром
	} else if(this->_transfer.localStreamCount > 0)
		// Уменьшаем счётчик активных потоков, открытых нами
		--this->_transfer.localStreamCount;
	// Удаляем поток из карты активных потоков
	this->_transfer.streams.erase(i);
}
/**
 * @brief Метод закрытия потока с вызовом функции обратного вызова закрытия
 *
 * @param id   идентификатор потока
 * @param code код ошибки закрытия
 *
 */
void awh::http::Parser_HTTP2::closeStream(const uint32_t id, const error_t code) noexcept {
	/**
	 * Если поток отсутствует в карте - закрывать нечего: метод идемпотентен, чтобы
	 * повторный сброс уже закрытого потока не порождал лишнего события закрытия
	 */
	if(this->_transfer.streams.find(id) == this->_transfer.streams.end())
		// Выходим из метода
		return;
	/**
	 * Удаляем поток из карты ДО уведомления: пользовательская функция обратного вызова
	 * вправе закрыть связанный поток, а тот - снова наш (парные потоки туннеля), и пока
	 * запись остаётся в карте, проверка идемпотентности пропускает повторное закрытие -
	 * уведомления начинают вызывать друг друга до исчерпания стека
	 */
	this->eraseStream(id);
	// Если функция обратного вызова установлена
	if(this->_callbacks.close != nullptr){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Уведомляем о закрытии потока
			this->_callbacks.close(id, code);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint32_t> (code)), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод вызова функции обратного вызова обработки фазы приёма сообщения потока
 *
 * @param id    идентификатор потока
 * @param phase фаза приёма сообщения потока
 * @param part  часть сообщения (заголовки, трейлеры, тело), NONE - сообщение целиком
 * @return      результат обработки (false - поток сброшен)
 *
 */
bool awh::http::Parser_HTTP2::firePhase(const uint32_t id, const phase_t phase, const part_t part) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callbacks.phase != nullptr){
		// Результат обработки пользовательской функцией
		bool result = false;
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Уведомляем о фазе приёма сообщения потока
			result = this->_callbacks.phase(id, phase, part);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, static_cast <uint16_t> (phase), static_cast <uint16_t> (part)), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
		// Если функция обратного вызова потребовала сбросить поток
		if(!result){
			// Если поток ещё существует (функция обратного вызова могла его закрыть)
			if(this->findStream(id) != nullptr){
				// Сбрасываем поток с кодом CANCEL
				this->rejectStream(id, error_t::CANCEL);
				// Закрываем поток с вызовом функции обратного вызова закрытия
				this->closeStream(id, error_t::CANCEL);
			}
			// Поток сброшен
			return false;
		}
	}
	// Продолжаем обработку
	return true;
}
/**
 * @brief Метод вызова функции обратного вызова обработки применённого SETTINGS пира
 *
 */
void awh::http::Parser_HTTP2::fireSettings() noexcept {
	// Если функция обратного вызова не установлена - вызывать нечего
	if(this->_callbacks.settings == nullptr)
		// Выходим из метода
		return;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Уведомляем о применённом SETTINGS пира
		this->_callbacks.settings();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод вызова функции обратного вызова о готовности потока принимать данные
 *
 * @param id идентификатор потока
 *
 */
void awh::http::Parser_HTTP2::fireWritable(const uint32_t id) noexcept {
	// Если функция обратного вызова не установлена - вызывать нечего
	if(this->_callbacks.writable == nullptr)
		// Выходим из метода
		return;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Уведомляем о готовности потока принимать данные
		this->_callbacks.writable(id);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод вызова функции обратного вызова обработки открытия нового потока
 *
 * @param id идентификатор потока
 * @return   результат обработки (false - поток требуется сбросить)
 *
 */
bool awh::http::Parser_HTTP2::fireBegin(const uint32_t id) noexcept {
	// Если функция обратного вызова не установлена - поток принимается
	if(this->_callbacks.begin == nullptr)
		// Продолжаем обработку
		return true;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Уведомляем об открытии нового потока
		return this->_callbacks.begin(id);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Ошибка пользовательской функции - поток сбрасывается
	return false;
}
/**
 * @brief Метод вызова функции обратного вызова обработки анонса server push
 *
 * @param sid         идентификатор ассоциированного потока клиента
 * @param promisedSid идентификатор обещанного потока
 * @return            результат обработки (false - push требуется отклонить)
 *
 */
bool awh::http::Parser_HTTP2::firePush(const uint32_t sid, const uint32_t promisedSid) noexcept {
	// Если функция обратного вызова не установлена - push принимается
	if(this->_callbacks.push == nullptr)
		// Продолжаем обработку
		return true;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Уведомляем об анонсе server push
		return this->_callbacks.push(sid, promisedSid);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid, promisedSid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Ошибка пользовательской функции - push отклоняется
	return false;
}
/**
 * @brief Метод вызова функции обратного вызова обработки полученного GOAWAY
 *
 * @param sid   наибольший идентификатор обработанного пиром потока
 * @param code  код ошибки завершения соединения
 * @param debug отладочные данные пира
 *
 */
void awh::http::Parser_HTTP2::fireGoaway(const uint32_t sid, const error_t code, const string_view debug) noexcept {
	// Если функция обратного вызова не установлена - вызывать нечего
	if(this->_callbacks.goaway == nullptr)
		// Выходим из метода
		return;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Уведомляем о полученном GOAWAY
		this->_callbacks.goaway(sid, code, debug);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid, static_cast <uint32_t> (code)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод вызова функции обратного вызова обработки провайдера заголовков потока
 *
 * @param id        идентификатор потока
 * @param provider  провайдер заголовков потока (nullptr для трейлеров)
 * @param endStream флаг завершения потока
 * @return          результат обработки (false - поток требуется сбросить)
 *
 */
bool awh::http::Parser_HTTP2::fireProvider(const uint32_t id, const provider_t * provider, const bool endStream) noexcept {
	// Если функция обратного вызова не установлена - продолжаем обработку
	if(this->_callbacks.provider == nullptr)
		// Продолжаем обработку
		return true;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Уведомляем о провайдере заголовков потока
		return this->_callbacks.provider(id, provider, endStream);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, endStream), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Ошибка пользовательской функции - поток сбрасывается
	return false;
}
/**
 * @brief Метод вызова функции обратного вызова обработки фрагмента тела потока
 *
 * @param id        идентификатор потока
 * @param buffer    буфер данных тела
 * @param size      размер данных тела
 * @param endStream флаг завершения потока
 * @return          результат обработки (false - поток требуется сбросить)
 *
 */
bool awh::http::Parser_HTTP2::fireData(const uint32_t id, const void * buffer, const size_t size, const bool endStream) noexcept {
	// Если функция обратного вызова не установлена - продолжаем обработку
	if(this->_callbacks.data == nullptr)
		// Продолжаем обработку
		return true;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Уведомляем о фрагменте тела потока
		return this->_callbacks.data(id, buffer, size, endStream);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, size, endStream), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Ошибка пользовательской функции - поток сбрасывается
	return false;
}
/**
 * @brief Метод вызова функции обратного вызова обработки заголовка или трейлера потока
 *
 * @param id    идентификатор потока
 * @param name  название заголовка
 * @param value значение заголовка
 * @param part  часть сообщения (HEADERS или TRAILER)
 * @return      результат обработки (false - поток требуется сбросить)
 *
 */
bool awh::http::Parser_HTTP2::fireHeader(const uint32_t id, const string_view name, const string_view value, const part_t part) noexcept {
	// Если функция обратного вызова не установлена - продолжаем обработку
	if(this->_callbacks.header == nullptr)
		// Продолжаем обработку
		return true;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Уведомляем о заголовке потока
		return this->_callbacks.header(id, name, value, part);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, string(name), string(value)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Ошибка пользовательской функции - поток сбрасывается
	return false;
}
/**
 * @brief Метод постановки потока в очередь готовых к отправке
 *
 * @param stream объект потока
 *
 */
void awh::http::Parser_HTTP2::markReady(stream_t & stream) noexcept {
	// Если поток уже стоит в очереди готовых - добавлять нечего
	if(stream.queued)
		// Выходим из метода
		return;
	// Помечаем что поток стоит в очереди готовых
	stream.queued = true;
	// Ставим поток в очередь готовых к отправке
	this->_transfer.readyIds.push_back(stream.id);
}
/**
 * @brief Метод прокачки отправки по всем потокам с учётом окон и порога выходного буфера
 *
 * @details Round-robin: за каждый проход отправляется не более одного DATA-фрейма
 *          с потока, пока хоть один поток делает прогресс - исключает голодание
 *          потоков (head-of-line blocking).
 *
 */
void awh::http::Parser_HTTP2::pump() noexcept {
	// Защита от реентерабельности (writable -> sendData -> pump)
	if(this->_flags.inPump)
		// Выходим из метода
		return;
	/**
	 * Флаг прогресса отправки
	 * Помечаем что прокачка отправки уже выполняется
	 */
	bool progress = this->_flags.inPump = true;
	/**
	 * Выполняем проходы прокачки, пока хоть один поток делает прогресс
	 */
	while(progress){
		// Сбрасываем флаг прогресса отправки
		progress = false;
		// Очищаем снимок идентификаторов потоков
		this->_transfer.pumpIds.clear();
		// Число записей, сохраняемых в очереди готовых
		size_t keep = 0;
		/**
		 * Собираем снимок идентификаторов из очереди готовых, попутно уплотняя её
		 * на месте. Наружу этот перебор не выходит, поэтому очередь под ним вырасти
		 * не может; сам снимок нужен отдельным буфером - упорядочивание переставляет
		 * ячейки, и порядка очереди после него не остаётся
		 */
		for(size_t i = 0; i < this->_transfer.readyIds.size(); i++){
			// Получаем идентификатор очередного потока очереди
			const uint32_t id = this->_transfer.readyIds[i];
			// Выполняем поиск потока
			stream_t * stream = this->findStream(id);
			// Если поток удалён - запись очереди снимается вместе с ним
			if(stream == nullptr)
				// Переходим к следующей записи очереди
				continue;
			/**
			 * Поток без данных в буфере, с исчерпанным источником, без отложенного
			 * завершения и без отложенных трейлеров прогресса дать не может: он ждёт
			 * пира, а не окна отправки. Такую запись снимаем с очереди - она своё
			 * отработала, а назначенный заново источник поставит поток обратно
			 */
			if((stream->pending() == 0) && this->sourceDone(* stream) && !stream->endStreamPending && !stream->trailersPending){
				// Снимаем признак нахождения потока в очереди готовых
				stream->queued = false;
				// Переходим к следующей записи очереди
				continue;
			}
			// Сохраняем запись в уплотняемой очереди готовых
			this->_transfer.readyIds[keep++] = id;
			/**
			 * Данные тела отправляются только из состояний open и half-closed(remote)
			 * (RFC 9113 §5.1). Из очереди такой поток не снимается: содержимое при нём
			 * остаётся, а состояние ещё вправе смениться - зарезервированный под push
			 * поток отправит своё тело сразу после собственных заголовков
			 */
			if((stream->state != h2::stream_state_t::OPEN) && (stream->state != h2::stream_state_t::HALF_CLOSED_REMOTE))
				// Переходим к следующей записи очереди
				continue;
			// Формируем ячейку снимка планировщика
			transfer_t::slot_t slot;
			// Запоминаем идентификатор потока
			slot.id = id;
			// Запоминаем срочность потока
			slot.urgency = stream->urgency;
			// Запоминаем признак инкрементальной доставки потока
			slot.incremental = stream->incremental;
			// Добавляем ячейку в снимок
			this->_transfer.pumpIds.push_back(slot);
		}
		// Усекаем очередь готовых до числа сохранённых записей
		this->_transfer.readyIds.resize(keep);
		/**
		 * Упорядочиваем снимок по расширенному приоритету (RFC 9218 §10): сначала
		 * более срочные потоки, внутри одной срочности неинкрементальные обслуживаются
		 * последовательно в порядке идентификаторов, инкрементальные - поочерёдно
		 */
		::sort(this->_transfer.pumpIds.begin(), this->_transfer.pumpIds.end(), [](const transfer_t::slot_t & left, const transfer_t::slot_t & right) noexcept -> bool {
			// Если срочность потоков различается - вперёд идёт более срочный
			if(left.urgency != right.urgency)
				// Выводим результат сравнения срочности
				return (left.urgency < right.urgency);
			// Внутри одной срочности неинкрементальные потоки обслуживаются первыми
			if(left.incremental != right.incremental)
				// Выводим результат сравнения признака инкрементальности
				return !left.incremental;
			// При прочих равных порядок задаёт идентификатор потока
			return (left.id < right.id);
		});
		// Запоминаем поколение состояния соединения перед прокачкой потоков
		const uint64_t epoch = this->_epoch;
		// Срочность группы последовательно обслуживаемых потоков
		uint16_t served = 0xFFFF;
		/**
		 * Выполняем прокачку отправки по всем потокам снимка
		 */
		for(const transfer_t::slot_t & slot : this->_transfer.pumpIds){
			// Получаем идентификатор очередного потока снимка
			const uint32_t id = slot.id;
			// Выполняем поиск потока (поток мог быть удалён на предыдущей итерации)
			stream_t * stream = this->findStream(id);
			// Если поток удалён - переходим к следующему
			if(stream == nullptr)
				// Переходим к следующему потоку снимка
				continue;
			/**
			 * Неинкрементальный поток обслуживается последовательно: пока в его группе
			 * срочности уже кто-то отправляет данные, остальные ждут (RFC 9218 §10)
			 */
			if(!stream->incremental && (static_cast <uint16_t> (stream->urgency) == served))
				// Переходим к следующему потоку снимка
				continue;
			// Запоминаем срочность потока (прокачка может удалить объект потока)
			const uint16_t urgency = static_cast <uint16_t> (stream->urgency);
			// Запоминаем признак инкрементальной доставки потока
			const bool incremental = stream->incremental;
			// Если прокачка потока сделала прогресс
			if(this->pumpStream(* stream)){
				// Помечаем что прогресс отправки есть
				progress = true;
				// Запоминаем срочность занятой последовательной группы
				if(!incremental)
					// Устанавливаем срочность обслуживаемой группы
					served = urgency;
			}
			/**
			 * Функция обратного вызова готовности могла реентрантно сбросить парсер:
			 * снимок идентификаторов при этом уничтожен, перебор продолжать нельзя
			 */
			if(epoch != this->_epoch){
				// Помечаем что прокачка отправки завершена
				this->_flags.inPump = false;
				// Прекращаем прокачку отправки
				return;
			}
		}
	}
	// Помечаем что прокачка отправки завершена
	this->_flags.inPump = false;
}
/**
 * @brief Метод отправки не более одного DATA-фрейма потока
 *
 * @param stream объект потока (ссылка может стать недействительной после вызова)
 * @return       признак прогресса отправки
 *
 */
bool awh::http::Parser_HTTP2::pumpStream(stream_t & stream) noexcept {
	// Запоминаем идентификатор потока
	const uint32_t id = stream.id;
	/**
	 * Данные тела мы вправе слать только из состояний open и half-closed(remote)
	 * (RFC 9113 §5.1). Зарезервированный под push поток до отправки своих заголовков
	 * в это множество не входит - иначе DATA ушёл бы раньше блока заголовков.
	 * Проверка стоит до обращения к источнику данных: тянуть тело в поток,
	 * который отправлять его не может, бессмысленно
	 */
	if((stream.state != h2::stream_state_t::OPEN) && (stream.state != h2::stream_state_t::HALF_CLOSED_REMOTE))
		// Прогресса отправки нет
		return false;
	// Дозагружаем буфер отправки из pull-источника данных (источник может сбросить и удалить поток)
	this->refillFromSource(stream);
	// Перечитываем указатель на поток (источник данных мог удалить поток)
	stream_t * sp = this->findStream(id);
	// Если поток удалён - считаем это прогрессом (карта потоков изменилась)
	if(sp == nullptr)
		// Прогресс отправки есть
		return true;
	// Получаем актуальную ссылку на поток
	stream_t & st = * sp;
	// Получаем логический объём ещё не отправленных данных тела
	const size_t remaining = st.pending();
	// Backpressure TCP-стадии: не раздуваем выходной буфер (по логическому объёму)
	if(this->outputPending() >= this->_transfer.outputHighWater)
		// Прогресса отправки нет
		return false;
	// Вычисляем доступное окно отправки (минимум окон соединения и потока)
	const int32_t window = ::min(this->_window.remote, st.remoteWindow);
	// Вычисляем свободное место в выходном буфере
	const size_t cap = (this->_transfer.outputHighWater - this->outputPending());
	// Вычисляем размер отправляемого фрагмента данных
	const size_t size = ::min({
		remaining,
		static_cast <size_t> (window > 0 ? window : 0),
		static_cast <size_t> (this->_remote.maxFrameSize),
		cap
	});
	// Если отправить ничего нельзя
	if(size == 0){
		/**
		 * Окно закрыто (данные остаются в буфере отправки) либо слать нечего.
		 * Завершение потока пустым DATA с END_STREAM не списывает окно (длина 0)
		 */
		if((remaining == 0) && this->sourceDone(st) && !st.endStreamSent){
			// Если на завершение тела отложена секция трейлеров - выпускаем её
			if(this->flushTrailers(st))
				// Прогресс отправки есть
				return true;
		}
		// Если тело отправлено полностью и требуется завершить поток
		if((remaining == 0) && st.endStreamPending && this->sourceDone(st) && !st.endStreamSent){
			// Отправляем пустой DATA-фрейм с флагом END_STREAM
			h2::frame::serialize::data(this->_buffer.output, st.id, string_view{}, true);
			// Помечаем что END_STREAM отправлен
			st.endStreamSent = true;
			// Применяем отправленный END_STREAM (ссылка на поток может стать недействительной)
			this->applyLocalEndStream(st);
			// Прогресс отправки есть
			return true;
		}
		// Прогресса отправки нет
		return false;
	}
	/**
	 * Определяем является ли фрагмент последним (END_STREAM). Отложенная секция
	 * трейлеров завершает поток вместо него, поэтому флаг на данные не ставится
	 */
	const bool last = (st.endStreamPending && (size == remaining) && this->sourceDone(st) && !st.trailersPending);
	// Отправляем DATA-фрейм с фрагментом данных тела
	h2::frame::serialize::data(this->_buffer.output, st.id, string_view(st.sendBuffer.data() + st.sendOffset, size), last);
	// Отмечаем отправленный префикс без сдвига всего буфера
	st.sendOffset += size;
	// Выполняем амортизированную очистку/компактификацию буфера отправки
	st.compactSendBuffer();
	// Списываем отправленные байты из окна соединения
	this->_window.remote -= static_cast <int32_t> (size);
	// Списываем отправленные байты из окна потока
	st.remoteWindow -= static_cast <int32_t> (size);
	// Сигнализируем о готовности потока принимать данные (если буфер просел)
	this->maybeNotifyWritable(st);
	/**
	 * Функция обратного вызова готовности могла реентрантно закрыть поток
	 * (sendRstStream) и удалить его из карты - перечитываем указатель,
	 * иначе дальнейшая запись по нему = use-after-free
	 */
	sp = this->findStream(id);
	// Если поток удалён - отправка всё равно состоялась
	if(sp == nullptr)
		// Прогресс отправки есть
		return true;
	// Если отправлен последний фрагмент
	if(last){
		// Помечаем что END_STREAM отправлен
		sp->endStreamSent = true;
		// Применяем отправленный END_STREAM (ссылка на поток может стать недействительной)
		this->applyLocalEndStream(* sp);
	}
	// Прогресс отправки есть
	return true;
}
/**
 * @brief Метод дозагрузки буфера отправки из pull-источника данных (если он задан)
 *
 * @param stream объект потока
 *
 */
void awh::http::Parser_HTTP2::refillFromSource(stream_t & stream) noexcept {
	// Если источник данных не задан либо его тело уже закончилось - дозагружать нечего
	if((stream.source == nullptr) || stream.sourceEof)
		// Выходим из метода
		return;
	/**
	 * Ответ на запрос методом HEAD содержимого не несёт (RFC 9110 §9.3.2), поэтому
	 * источник данных не опрашивается вовсе: иначе HEAD по большому ресурсу заставил
	 * бы приложение вычитать его целиком - ради того, чтобы всё вычитанное отбросить
	 */
	if(stream.bodylessSend){
		// Помечаем что конец тела источника достигнут
		stream.sourceEof = true;
		// Помечаем что на последнем фрагменте нужно выставить END_STREAM
		stream.endStreamPending = true;
		// Выходим из метода
		return;
	}
	// Запоминаем идентификатор потока
	const uint32_t id = stream.id;
	// Указатель на объект потока (источник данных вправе закрыть поток)
	stream_t * sp = &stream;
	/**
	 * Держим буфер наполненным до high-water, запрашивая источник данных порциями
	 */
	while((sp->pending() < this->_transfer.sendHighWater) && !sp->sourceEof){
		// Вычисляем ёмкость запрашиваемой порции (не больше одного DATA-фрейма пира)
		const size_t cap = ::min(static_cast <size_t> (this->_remote.maxFrameSize), this->_transfer.sendHighWater - sp->pending());
		// Запоминаем текущий размер буфера отправки
		const size_t offset = sp->sendBuffer.size();
		// Резервируем место под порцию данных прямо в буфере отправки (без промежуточной копии)
		sp->sendBuffer.resize(offset + cap);
		// Флаг достижения конца тела
		bool eof = false;
		// Результат запроса данных у источника
		int64_t bytes = -1;
		/**
		 * Забираем источник данных на время вызова: приложение вправе сбросить поток
		 * прямо из источника, а уничтожение вызываемого объекта под собственным вызовом
		 * недопустимо
		 */
		data_source_callback_t source = ::move(sp->source);
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Запрашиваем порцию данных у источника (источник пишет напрямую в буфер отправки)
			bytes = source(id, reinterpret_cast <uint8_t *> (&sp->sendBuffer[offset]), cap, eof);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
		// Перечитываем указатель на поток (источник данных мог его закрыть либо сбросить парсер)
		sp = this->findStream(id);
		// Если поток удалён - буфер отправки уничтожен вместе с ним, дозагружать некуда
		if(sp == nullptr)
			// Выходим из метода
			return;
		// Возвращаем источник данных потоку, если из самого источника не назначен новый
		if(sp->source == nullptr)
			// Возвращаем источник данных обратно потоку
			sp->source = ::move(source);
		// Обрезаем буфер отправки до фактически записанного источником размера
		sp->sendBuffer.resize(offset + static_cast <size_t> (((bytes > 0) && (bytes <= static_cast <int64_t> (cap))) ? bytes : 0));
		// Если источник сообщил об ошибке данных либо нарушил контракт (записал больше ёмкости)
		if((bytes < 0) || (bytes > static_cast <int64_t> (cap))){
			// Сбрасываем поток с кодом INTERNAL_ERROR
			this->rejectStream(id, error_t::INTERNAL_ERROR);
			// Закрываем поток с вызовом функции обратного вызова закрытия (ссылка на поток недействительна)
			this->closeStream(id, error_t::INTERNAL_ERROR);
			// Выходим из метода
			return;
		}
		// Если достигнут конец тела источника
		if(eof){
			// Помечаем что конец тела источника достигнут
			sp->sourceEof = true;
			// Помечаем что на последнем фрагменте нужно выставить END_STREAM
			sp->endStreamPending = true;
		}
		// Если источник временно без данных - прерываем дозагрузку
		if((bytes == 0) && !eof)
			// Прерываем дозагрузку до следующей прокачки
			break;
	}
}
/**
 * @brief Метод сигнализации о готовности потока принимать данные (один раз на провал буфера)
 *
 * @param stream объект потока
 *
 */
void awh::http::Parser_HTTP2::maybeNotifyWritable(stream_t & stream) noexcept {
	// Сигнал отдаём только для push-модели (sendData), не для pull-источника данных
	if((stream.source != nullptr) || (this->_callbacks.writable == nullptr))
		// Выходим из метода
		return;
	// Если сигнал ещё не подан и буфер отправки опустился ниже low-water
	if(!stream.writableNotified && (stream.pending() <= this->_transfer.sendLowWater)){
		// Помечаем что сигнал для текущего провала буфера подан
		stream.writableNotified = true;
		// Уведомляем о готовности потока принимать данные
		this->fireWritable(stream.id);
	}
}
/**
 * @brief Метод проверки того, что все данные потока для отправки уже получены
 *
 * @param stream объект потока
 * @return       результат проверки (нет источника данных или достигнут его eof)
 *
 */
bool awh::http::Parser_HTTP2::sourceDone(const stream_t & stream) const noexcept {
	// Источник данных не задан либо конец его тела достигнут
	return ((stream.source == nullptr) || stream.sourceEof);
}
/**
 * @brief Метод пополнения окна приёма (соединения/потока) с отправкой WINDOW_UPDATE при просадке
 *
 * @param stream   объект потока (nullptr - только окно соединения)
 * @param consumed число принятых байт
 *
 */
void awh::http::Parser_HTTP2::replenishReceiveWindow(stream_t * stream, const uint32_t consumed) noexcept {
	// Списываем принятые байты из окна приёма соединения
	this->_window.local -= static_cast <int32_t> (consumed);
	// Если окно приёма соединения просело ниже половины целевого размера
	if(this->_window.local < (this->_window.localMax / 2)){
		// Вычисляем инкремент для восстановления окна до целевого размера
		const uint32_t delta = static_cast <uint32_t> (this->_window.localMax - this->_window.local);
		// Отправляем WINDOW_UPDATE для окна соединения
		h2::frame::serialize::windowUpdate(this->_buffer.output, 0, delta);
		// Восстанавливаем окно приёма соединения
		this->_window.local += static_cast <int32_t> (delta);
	}
	// Если задан объект потока - пополняем и его окно приёма
	if(stream != nullptr){
		// Списываем принятые байты из окна приёма потока
		stream->localWindow -= static_cast <int32_t> (consumed);
		// Если окно приёма потока просело ниже половины начального размера
		if(stream->localWindow < (this->_window.localInit / 2)){
			// Вычисляем инкремент для восстановления окна до начального размера
			const uint32_t delta = static_cast <uint32_t> (this->_window.localInit - stream->localWindow);
			// Отправляем WINDOW_UPDATE для окна потока
			h2::frame::serialize::windowUpdate(this->_buffer.output, stream->id, delta);
			// Восстанавливаем окно приёма потока
			stream->localWindow += static_cast <int32_t> (delta);
		}
	}
}
/**
 * @brief Метод применения расширенного приоритета к потоку (RFC 9218 §4)
 *
 * @param stream объект потока
 * @param value  значение поля приоритета
 *
 */
void awh::http::Parser_HTTP2::applyPriority(stream_t & stream, string_view value) noexcept {
	// Разбираем сигнал приоритета прямо в признаки потока
	this->parsePriority(value, stream.urgency, stream.incremental);
}
/**
 * @brief Метод разбора значения поля расширенного приоритета (RFC 9218 §4)
 *
 * @param value       значение поля приоритета
 * @param urgency     срочность потока (выходной параметр)
 * @param incremental признак инкрементальной доставки (выходной параметр)
 *
 */
void awh::http::Parser_HTTP2::parsePriority(const string_view value, uint8_t & urgency, bool & incremental) const noexcept {
	/**
	 * Сигнал приоритета задаёт его целиком: параметр, в сигнале отсутствующий,
	 * принимает значение по умолчанию, а не сохраняет прежнее (RFC 9218 §4).
	 * Без сброса [u=1] после прежнего [i] оставлял бы поток инкрементальным
	 */
	urgency = h2::proto::DEFAULT_URGENCY;
	// Снимаем признак инкрементальной доставки потока
	incremental = false;
	// Текущая позиция разбора значения поля приоритета
	size_t pos = 0;
	/**
	 * Разбираем структурированный словарь вида "u=2, i" (RFC 8941): нас интересуют
	 * только ключи [u] и [i], остальные по требованию RFC 9218 §4.3 игнорируются,
	 * как и любые синтаксически некорректные элементы
	 */
	while(pos < value.size()){
		// Определяем границу текущего элемента словаря
		const size_t end = ::min(value.find(',', pos), value.size());
		// Формируем текущий элемент словаря
		string_view item = value.substr(pos, end - pos);
		// Сдвигаем позицию за разделитель элементов
		pos = (end + 1);
		// Снимаем начальные пробельные символы элемента
		while(!item.empty() && ((item.front() == ' ') || (item.front() == '\t')))
			// Сдвигаем начало элемента
			item.remove_prefix(1);
		// Снимаем конечные пробельные символы элемента
		while(!item.empty() && ((item.back() == ' ') || (item.back() == '\t')))
			// Сдвигаем конец элемента
			item.remove_suffix(1);
		/**
		 * Отсекаем параметры члена словаря (RFC 8941 §3.1.2): они уточняют значение,
		 * но самого значения не отменяют, а неизвестные обязаны игнорироваться.
		 * Без этого [u=1;q=0.5] отбрасывался целиком, и срочность оставалась
		 * значением по умолчанию. Обрезка по первой точке с запятой безопасна:
		 * значения ключей [u] и [i] - целое и логическое, точки с запятой в них нет
		 */
		const size_t params = item.find(';');
		// Если параметры члена словаря заданы
		if(params != string_view::npos)
			// Отсекаем параметры члена словаря
			item = item.substr(0, params);
		// Снимаем пробельные символы, оставшиеся перед разделителем параметров
		while(!item.empty() && ((item.back() == ' ') || (item.back() == '\t')))
			// Сдвигаем конец элемента
			item.remove_suffix(1);
		// Пустой элемент игнорируем
		if(item.empty())
			// Переходим к следующему элементу словаря
			continue;
		// Если получен ключ срочности потока
		if((item.size() == 3) && (item.compare(0, 2, "u=") == 0)){
			// Извлекаем значение срочности потока
			const char letter = item[2];
			// Значение срочности обязано быть цифрой в допустимом диапазоне
			if((letter >= '0') && (letter <= ('0' + static_cast <char> (h2::proto::MAX_URGENCY))))
				// Применяем срочность потока
				urgency = static_cast <uint8_t> (letter - '0');
		// Если получен ключ инкрементальной доставки без значения (эквивалент ?1)
		} else if(item == value::INCREMENTAL)
			// Помечаем поток инкрементальным
			incremental = true;
		// Если получен ключ инкрементальной доставки со значением
		else if(item == value::INCREMENTAL_ON)
			// Помечаем поток инкрементальным
			incremental = true;
		// Если инкрементальная доставка явно отключена
		else if(item == value::INCREMENTAL_OFF)
			// Снимаем признак инкрементальной доставки
			incremental = false;
	}
}
/**
 * @brief Метод запоминания приоритета ещё не открытого потока (RFC 9218 §7.1)
 *
 * @param id    идентификатор приоритизируемого потока
 * @param value значение поля приоритета
 * @return      результат запоминания (false - исчерпан лимит одновременных потоков)
 *
 */
bool awh::http::Parser_HTTP2::deferPriority(const uint32_t id, const string_view value) noexcept {
	// Формируем запись отложенного приоритета
	transfer_t::pending_t pending;
	// Запоминаем идентификатор приоритизируемого потока
	pending.id = id;
	// Разбираем сигнал приоритета в поля записи
	this->parsePriority(value, pending.urgency, pending.incremental);
	/**
	 * Выполняем поиск прежней записи по этому же потоку
	 */
	for(transfer_t::pending_t & item : this->_transfer.pendingPriorities){
		// Если запись по этому потоку уже есть
		if(item.id == id){
			// Новый сигнал заменяет прежний целиком (RFC 9218 §4)
			item = pending;
			// Приоритет запомнен (нового потока сигнал не добавил)
			return true;
		}
	}
	/**
	 * Сумма приоритизированных потоков в состоянии idle и активных потоков пира
	 * не вправе превысить объявленный нами SETTINGS_MAX_CONCURRENT_STREAMS
	 * (RFC 9218 §7.1): иначе пир занимал бы сигналами приоритета слоты сверх
	 * того лимита, которым мы ограничили его одновременные потоки
	 */
	if((this->_transfer.pendingPriorities.size() + this->_transfer.peerStreamCount) >= this->_local.maxConcurrentStreams)
		// Приоритет не запомнен - лимит одновременных потоков исчерпан
		return false;
	/**
	 * Кольцо ограничивает цену сигнала и сверху, независимо от лимита потоков:
	 * при высоком SETTINGS_MAX_CONCURRENT_STREAMS оно вытесняет самую старую
	 * запись, а не растёт вслед за объявленным лимитом
	 */
	if(this->_transfer.pendingPriorities.size() >= PENDING_PRIORITIES_CACHE)
		// Снимаем самую старую запись кольца
		this->_transfer.pendingPriorities.erase(this->_transfer.pendingPriorities.begin());
	// Запоминаем приоритет до открытия потока
	this->_transfer.pendingPriorities.push_back(pending);
	// Приоритет запомнен
	return true;
}
/**
 * @brief Метод применения приоритета, отложенного до открытия потока
 *
 * @param stream объект открываемого потока
 *
 */
void awh::http::Parser_HTTP2::applyPendingPriority(stream_t & stream) noexcept {
	// Если отложенных приоритетов нет - применять нечего
	if(this->_transfer.pendingPriorities.empty())
		// Выходим из метода
		return;
	// Число записей, сохраняемых в кольце
	size_t count = 0;
	/**
	 * Уплотняем кольцо на месте: записи потоков с идентификатором не выше
	 * открываемого сохранению не подлежат. Идентификаторы пира строго возрастают
	 * (RFC 9113 §5.1.1), поэтому такие потоки открыты уже не будут, и без снятия
	 * их сигналы вытесняли бы из кольца актуальные
	 */
	for(const transfer_t::pending_t & item : this->_transfer.pendingPriorities){
		// Если запись относится к открываемому потоку
		if(item.id == stream.id){
			// Применяем срочность потока
			stream.urgency = item.urgency;
			// Применяем признак инкрементальной доставки потока
			stream.incremental = item.incremental;
			// Помечаем что приоритет потока задан кадром
			stream.prioritized = true;
		// Если запись относится к потоку, который ещё может быть открыт
		} else if(item.id > stream.id)
			// Сохраняем запись в кольце
			this->_transfer.pendingPriorities[count++] = item;
	}
	// Усекаем кольцо до числа сохранённых записей
	this->_transfer.pendingPriorities.resize(count);
}
/**
 * @brief Метод предупреждения о полностью снятом лимите списка заголовков
 *
 */
void awh::http::Parser_HTTP2::checkHeaderListLimits() const noexcept {
	/**
	 * Оба лимита распакованного списка сняты. Блок заголовков ограничен размером
	 * на проводе, но каждая индексная ссылка длиной в байт разворачивается в арене
	 * декодера в полную пару название/значение из динамической таблицы: блок в 64 КиБ
	 * ссылок даёт сотни мегабайт, и остановить это в такой конфигурации нечем
	 */
	if((this->_limits.maxHeadersTotal == 0) && (this->_local.maxHeaderListSize == 0))
		// Записываем сообщение о снятом лимите в лог
		this->_log->print(
			"HTTP/2 decoded header list is unlimited: both maxHeadersTotal and SETTINGS_MAX_HEADER_LIST_SIZE are 0",
			log_t::flag_t::WARNING
		);
}
/**
 * @brief Метод сверки отправляемого блока заголовков с лимитом пира
 *
 * @param sid идентификатор потока
 *
 */
void awh::http::Parser_HTTP2::checkPeerHeaderList(const uint32_t sid) const noexcept {
	// Если пир лимит списка заголовков не анонсировал - сверять не с чем
	if(this->_remote.maxHeaderListSize == 0)
		// Выходим из метода
		return;
	/**
	 * Лимит носит рекомендательный характер (RFC 9113 §6.5.2), поэтому отправку
	 * не блокируем: пир вправе как принять блок, так и отвергнуть его. Но молча
	 * отправленный блок сверх лимита выглядит как беспричинный сброс потока
	 * на стороне пира, поэтому причину фиксируем в логе
	 */
	if(this->_encoder.listSize() > this->_remote.maxHeaderListSize)
		// Записываем сообщение о превышении лимита пира в лог
		this->_log->print(
			"HTTP/2 header list of stream %u is %llu bytes and exceeds peer SETTINGS_MAX_HEADER_LIST_SIZE (%u)",
			log_t::flag_t::WARNING, sid, this->_encoder.listSize(), this->_remote.maxHeaderListSize
		);
}
/**
 * @brief Метод проверки соответствия принятого тела объявленному content-length
 *
 * @details RFC 9113 §8.1.1: расхождение суммы длин DATA с content-length делает
 *          сообщение малформированным. При расхождении поток сбрасывается
 *
 * @param sid идентификатор потока
 * @return    результат проверки (false - поток сброшен)
 *
 */
bool awh::http::Parser_HTTP2::checkBodyLength(const uint32_t sid) noexcept {
	// Выполняем поиск потока
	stream_t * stream = this->findStream(sid);
	// Если поток отсутствует либо длина тела не объявлена - проверять нечего
	if((stream == nullptr) || (stream->contentLength < 0))
		// Проверка пройдена
		return true;
	/**
	 * Ответ на HEAD и ответы 204/304 объявляют длину тела, которого не будет:
	 * сверять сумму длин DATA с ней нельзя (RFC 9110 §8.6, §9.3.2)
	 */
	if(stream->bodyless)
		// Проверка пройдена
		return true;
	// Если объём принятого тела совпадает с объявленным
	if(stream->recvBody == static_cast <uint64_t> (stream->contentLength))
		// Проверка пройдена
		return true;
	/**
	 * Сумма длин DATA не совпала с объявленной content-length: сообщение малформировано
	 * (RFC 9113 §8.1.1). Это потоковая ошибка - соединение остаётся живым
	 */
	this->rejectStream(sid, error_t::PROTOCOL_ERROR);
	// Закрываем поток с вызовом функции обратного вызова закрытия
	this->closeStream(sid, error_t::PROTOCOL_ERROR);
	// Проверка не пройдена
	return false;
}
/**
 * @brief Метод проверки декодированных заголовков на лимиты безопасности
 *
 * @param fields декодированные заголовки блока
 * @return       результат проверки (false - лимиты превышены)
 *
 */
bool awh::http::Parser_HTTP2::checkHeaderLimits(const vector <h2::hpack::field_view_t> & fields) const noexcept {
	// Если число заголовков в блоке превышает лимит
	if(fields.size() > this->_limits.maxHeaderCount)
		// Лимиты превышены
		return false;
	/**
	 * Выполняем перебор всех заголовков блока
	 */
	for(const h2::hpack::field_view_t & field : fields){
		// Если длина имени заголовка превышает лимит
		if(field.name.size() > this->_limits.maxHeaderName)
			// Лимиты превышены
			return false;
		// Если длина значения заголовка превышает лимит
		if(field.value.size() > this->_limits.maxHeaderValue)
			// Лимиты превышены
			return false;
	}
	// Лимиты не превышены
	return true;
}
/**
 * @brief Метод проверки допустимости отправки блока заголовков в поток
 *
 * @param sid идентификатор потока
 * @return    результат проверки
 *
 */
bool awh::http::Parser_HTTP2::canSendHeaders(const uint32_t sid) noexcept {
	// Нулевой идентификатор потока не принадлежит ни одному потоку (RFC 9113 §5.1.1)
	if(sid == 0){
		// Записываем сообщение об ошибке в лог
		this->_log->print("HTTP/2 headers are not allowed for stream 0", log_t::flag_t::WARNING);
		// Отправка недопустима
		return false;
	}
	// Выполняем поиск потока
	const stream_t * stream = this->findStream(sid);
	// Если поток существует - блок заголовков допустим не в каждом его состоянии
	if(stream != nullptr){
		/**
		 * После отправленного END_STREAM наша половина закрыта, и слать в неё нечего
		 * (RFC 9113 §5.1). Признак отложенного END_STREAM здесь не проверяется: он
		 * означает ровно то, что тело дочитано до конца, а это и есть момент отправки
		 * секции трейлеров - завершение потока переезжает с последнего кадра тела на них
		 */
		if(stream->endStreamSent){
			// Записываем сообщение об ошибке в лог
			this->_log->print("HTTP/2 stream %u is already half-closed (local)", log_t::flag_t::WARNING, sid);
			// Отправка недопустима
			return false;
		}
		/**
		 * Блок заголовков допустим из состояний: open и half-closed(remote) (ответ,
		 * продолжение, трейлеры) и reserved(local) (ответ на собственный push).
		 * Поток, зарезервированный пиром, отвечает не наша сторона (RFC 9113 §5.1)
		 */
		if((stream->state != h2::stream_state_t::OPEN) && (stream->state != h2::stream_state_t::HALF_CLOSED_REMOTE) &&
		   (stream->state != h2::stream_state_t::RESERVED_LOCAL) && (stream->state != h2::stream_state_t::IDLE)){
			// Записываем сообщение об ошибке в лог
			this->_log->print("HTTP/2 stream %u does not accept headers in its current state", log_t::flag_t::WARNING, sid);
			// Отправка недопустима
			return false;
		}
		// Отправка допустима
		return true;
	}
	// Поток, инициируемый пиром, мы открыть не можем (RFC 9113 §5.1.1)
	if(this->peerInitiated(sid)){
		// Записываем сообщение об ошибке в лог
		this->_log->print("HTTP/2 stream %u is initiated by peer and cannot be opened locally", log_t::flag_t::WARNING, sid);
		// Отправка недопустима
		return false;
	}
	// Наш идентификатор потока обязан быть выделен методом nextStreamId()
	if(sid >= this->_transfer.nextStreamId){
		// Записываем сообщение об ошибке в лог
		this->_log->print("HTTP/2 stream %u is not allocated by nextStreamId", log_t::flag_t::WARNING, sid);
		// Отправка недопустима
		return false;
	}
	// Поток уже открывался и был закрыт - повторно открыть его нельзя (RFC 9113 §5.1)
	if(sid <= this->_transfer.localOpened){
		// Записываем сообщение об ошибке в лог
		this->_log->print("HTTP/2 stream %u is already closed", log_t::flag_t::WARNING, sid);
		// Отправка недопустима
		return false;
	}
	// Если соединение помечено на завершение - новые потоки на нём не открываются (RFC 9113 §6.8)
	if(this->_flags.goawayReceived || this->_flags.goawaySent){
		// Записываем сообщение об ошибке в лог
		this->_log->print("HTTP/2 stream %u cannot be opened after GOAWAY", log_t::flag_t::WARNING, sid);
		// Отправка недопустима
		return false;
	}
	// Если исчерпан лимит одновременных потоков, разрешённый пиром (RFC 9113 §5.1.2)
	if(this->_transfer.localStreamCount >= this->_remote.maxConcurrentStreams){
		// Записываем сообщение об ошибке в лог
		this->_log->print("HTTP/2 peer concurrent streams limit (%u) reached", log_t::flag_t::WARNING, this->_remote.maxConcurrentStreams);
		// Отправка недопустима
		return false;
	}
	// Отправка допустима
	return true;
}
/**
 * @brief Метод откладывания секции трейлеров до конца отправки тела потока
 *
 * @param sid       идентификатор потока
 * @param fields    заголовки секции трейлеров
 * @param endStream флаг завершения потока
 * @return          результат откладывания (true - отправка отложена)
 *
 */
bool awh::http::Parser_HTTP2::deferTrailers(const uint32_t sid, const vector <h2::hpack::field_t> & fields, const bool endStream) noexcept {
	// Выполняем поиск потока
	stream_t * stream = this->findStream(sid);
	// Если поток отсутствует либо его заголовки ещё не отправлены - откладывать нечего
	if((stream == nullptr) || !stream->headersSent)
		// Откладывание не требуется
		return false;
	// Если тело потока отправлено полностью - трейлеры уходят сразу за ним
	if((stream->pending() == 0) && this->sourceDone(* stream))
		// Откладывание не требуется
		return false;
	// Повторная секция трейлеров недопустима
	if(stream->trailersPending){
		// Записываем сообщение об ошибке в лог
		this->_log->print("HTTP/2 trailers for stream %u are already pending", log_t::flag_t::WARNING, sid);
		// Отправка отложена (повторную секцию отбрасываем)
		return true;
	}
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Запоминаем заголовки секции трейлеров
		stream->trailers = fields;
		// Помечаем что секция трейлеров отложена
		stream->trailersPending = true;
		// Ставим поток в очередь готовых к отправке
		this->markReady(* stream);
		// Если трейлеры не завершают поток - это нарушение (RFC 9113 §8.1)
		if(!endStream)
			// Записываем сообщение об ошибке в лог
			this->_log->print("HTTP/2 trailers for stream %u must carry END_STREAM", log_t::flag_t::WARNING, sid);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid, fields.size(), endStream), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Отправка отложена
	return true;
}
/**
 * @brief Метод отправки отложенной секции трейлеров потока
 *
 * @param stream объект потока (ссылка может стать недействительной после вызова)
 * @return       признак отправки секции трейлеров
 *
 */
bool awh::http::Parser_HTTP2::flushTrailers(stream_t & stream) noexcept {
	// Если отложенной секции трейлеров нет - отправлять нечего
	if(!stream.trailersPending)
		// Секция трейлеров не отправлена
		return false;
	// Запоминаем идентификатор потока
	const uint32_t sid = stream.id;
	// Сбрасываем признак отложенной секции трейлеров
	stream.trailersPending = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Закодированный HPACK-блок трейлеров
		string block = "";
		// Выполняем кодирование трейлеров в HPACK-блок
		this->_encoder.encode(stream.trailers, block, true);
		// Освобождаем память отложенных трейлеров
		stream.trailers.clear();
		// Сверяем размер секции трейлеров с лимитом списка заголовков пира
		this->checkPeerHeaderList(sid);
		// Отправляем блок трейлеров с завершением потока
		h2::frame::serialize::headerBlock(this->_buffer.output, sid, block, true, this->_remote.maxFrameSize);
		// Помечаем что END_STREAM отправлен
		stream.endStreamSent = true;
		// Применяем отправленный END_STREAM (ссылка на поток может стать недействительной)
		this->applyLocalEndStream(stream);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Секция трейлеров отправлена
	return true;
}
/**
 * @brief Метод отправки собранного HPACK-блока заголовков потока
 *
 * @param sid       идентификатор потока
 * @param block     закодированный HPACK-блок заголовков
 * @param endStream флаг завершения потока (тела не будет)
 *
 */
void awh::http::Parser_HTTP2::commitHeaders(const uint32_t sid, const string & block, const bool endStream) {
	// Сверяем размер блока заголовков с лимитом списка заголовков пира
	this->checkPeerHeaderList(sid);
	// Отправляем блок заголовков (с автоматической нарезкой на HEADERS + CONTINUATION)
	h2::frame::serialize::headerBlock(this->_buffer.output, sid, block, endStream, this->_remote.maxFrameSize);
	// Получаем существующий либо создаём новый объект потока
	stream_t & stream = this->stream(sid);
	// Помечаем что блок заголовков потока нами отправлен
	stream.headersSent = true;
	// Если поток ещё не использован - мы инициируем поток
	if(stream.state == h2::stream_state_t::IDLE){
		// Переводим поток в состояние OPEN
		stream.state = h2::stream_state_t::OPEN;
		// Учитываем поток в лимите одновременных потоков, разрешённом нам пиром
		++this->_transfer.localStreamCount;
		// Запоминаем наибольший наш открытый идентификатор потока
		if(sid > this->_transfer.localOpened)
			// Обновляем наибольший наш открытый идентификатор потока
			this->_transfer.localOpened = sid;
	}
	/**
	 * Ответ на собственный push: reserved(local) -> half-closed(remote). Поток уже
	 * учтён в лимите одновременных потоков при резервировании, повторно не считаем
	 */
	else if(stream.state == h2::stream_state_t::RESERVED_LOCAL)
		// Переводим поток в состояние HALF_CLOSED_REMOTE
		stream.state = h2::stream_state_t::HALF_CLOSED_REMOTE;
	// Если блок завершает поток
	if(endStream){
		// Помечаем что END_STREAM отправлен
		stream.endStreamSent = true;
		// Применяем отправленный END_STREAM (ссылка на поток может стать недействительной)
		this->applyLocalEndStream(stream);
	}
}
/**
 * @brief Метод построения провайдера заголовков потока из псевдо-заголовков
 *
 * @param fields  декодированные заголовки блока
 * @param request собирается запрос клиента (true) или ответ сервера (false)
 * @return        собранный провайдер заголовков
 *
 */
unique_ptr <awh::http::provider_t> awh::http::Parser_HTTP2::buildProvider(const vector <h2::hpack::field_view_t> & fields, const bool request) const noexcept {
	// Результат работы функции - собранный провайдер заголовков
	unique_ptr <provider_t> result = nullptr;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если собирается запрос клиента
		if(request){
			// Создаём объект провайдера запроса клиента
			unique_ptr <request_t> provider(new request_t(version_t::HTTP2));
			/**
			 * Выполняем перебор всех заголовков блока
			 */
			for(const h2::hpack::field_view_t & field : fields){
				// Если получен псевдо-заголовок [:method] (имена уже провалидированы, сравнение строгое)
				if(field.name == header::METHOD){
					// Выполняем классификацию метода запроса по его имени
					provider->method = awh::http::classifyMethod(field.value);
					// Если метод запроса синтаксически корректен, но не распознан
					if(provider->method == method_t::NONE){
						// Помечаем метод запроса как нераспознанный
						provider->method = method_t::UNKNOWN;
						// Сохраняем оригинальное написание метода (прозрачное проксирование экзотических методов)
						provider->methodName = field.value;
					}
				// Если получен псевдо-заголовок [:path]
				} else if(field.name == header::PATH)
					// Устанавливаем параметры URI-запроса
					provider->uri = field.value;
				// Если получен псевдо-заголовок [:protocol] расширенного CONNECT (RFC 8441)
				else if(field.name == header::PROTOCOL)
					// Устанавливаем протокол туннеля
					provider->protocol = field.value;
			}
			// Устанавливаем собранный провайдер как результат
			result = ::move(provider);
		// Если собирается ответ сервера
		} else {
			// Создаём объект провайдера ответа сервера
			unique_ptr <response_t> provider(new response_t(version_t::HTTP2));
			/**
			 * Выполняем перебор всех заголовков блока
			 */
			for(const h2::hpack::field_view_t & field : fields){
				// Если получен псевдо-заголовок [:status] (имена уже провалидированы, сравнение строгое)
				if(field.name == header::STATUS){
					// Статус-код ответа сервера
					uint16_t code = 0;
					/**
					 * Выполняем разбор трёх цифр статус-кода (формат провалидирован ранее)
					 */
					for(const char letter : field.value)
						// Накапливаем значение статус-кода
						code = static_cast <uint16_t> ((code * 10) + (letter - '0'));
					// Устанавливаем статус-код ответа сервера
					provider->code = code;
					// Устанавливаем стандартное сообщение сервера (в HTTP/2 reason-phrase отсутствует)
					provider->message = statusMessage(code);
				}
			}
			// Устанавливаем собранный провайдер как результат
			result = ::move(provider);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(fields.size(), request), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод полной очистки всех данных парсера
 *
 * @details Помимо полного сброса состояния соединения возвращает лимиты
 *          безопасности и параметры SETTINGS к значениям по умолчанию
 *          и удаляет установленные функции обратного вызова.
 *
 */
void awh::http::Parser_HTTP2::clear() noexcept {
	// Возвращаем лимиты безопасности к значениям по умолчанию
	this->_limits = limits_t();
	// Возвращаем наши параметры SETTINGS к значениям по умолчанию
	this->_local = settings_t();
	// Удаляем установленные функции обратного вызова
	this->_callbacks = callbacks_t();
	// Возвращаем порог сигнала writable к значению по умолчанию
	this->_transfer.sendLowWater = SEND_LOW_WATER;
	// Возвращаем ёмкость буфера отправки потока к значению по умолчанию
	this->_transfer.sendHighWater = SEND_HIGH_WATER;
	// Возвращаем порог выходного буфера соединения к значению по умолчанию
	this->_transfer.outputHighWater = OUTPUT_HIGH_WATER;
	// Выполняем полный сброс состояния соединения
	this->reset();
}
/**
 * @brief Метод полного сброса состояния соединения
 *
 * @details В отличие от HTTP/1.x, где reset() готовит парсер к следующему
 *          сообщению, для HTTP/2 сбрасывается ВСЁ соединение: HPACK-таблицы,
 *          карта потоков, окна, буферы (семантика нового соединения).
 *          Лимиты безопасности, параметры SETTINGS и функции обратного
 *          вызова сохраняются.
 *
 */
void awh::http::Parser_HTTP2::reset() noexcept {
	/**
	 * Сдвигаем поколение состояния: если сброс пришёл из пользовательской функции,
	 * это признак для всех идущих разборов немедленно свернуться, не обращаясь
	 * к уже освобождённым спискам заголовков и снимкам потоков
	 */
	++this->_epoch;
	// Выполняем сброс состояния базового парсера (итоговый статус разбора)
	parser_t::reset();
	// Сбрасываем код ошибки уровня соединения
	this->_error = error_t::NO_ERROR;
	// Параметры SETTINGS пира - состояние соединения, сбрасываем к значениям по умолчанию
	this->_remote = settings_t();
	/**
	 * По умолчанию протокол лимита одновременных потоков не задаёт (RFC 9113 §6.5.2):
	 * консервативное значение settings_t уместно только для анонсируемых нами параметров,
	 * а для пира оно ограничивало бы нас там, где он ограничений не ставил
	 */
	this->_remote.maxConcurrentStreams = 0xFFFFFFFF;
	// Пир не заявлял отказ от приоритетов RFC 7540, пока не прислал параметр
	this->_remote.noRfc7540Priorities = 0;
	// Пересоздаём HPACK-кодер (свежая динамическая таблица)
	this->_encoder = h2::hpack::encoder_t();
	// Пересоздаём HPACK-декодер (свежая динамическая таблица пира)
	this->_decoder = h2::hpack::decoder_t();
	// Сбрасываем окно приёма соединения
	this->_window.local = h2::proto::DEFAULT_WINDOW_SIZE;
	// Сбрасываем окно отправки соединения
	this->_window.remote = h2::proto::DEFAULT_WINDOW_SIZE;
	// Сбрасываем целевой размер окна приёма соединения
	this->_window.localMax = h2::proto::DEFAULT_WINDOW_SIZE;
	// Сбрасываем анонсированное начальное окно приёма потока
	this->_window.localInit = h2::proto::DEFAULT_WINDOW_SIZE;
	// Очищаем карту активных потоков
	this->_transfer.streams.clear();
	// Очищаем снимок идентификаторов потоков
	this->_transfer.pumpIds.clear();
	// Очищаем очередь готовых к отправке потоков
	this->_transfer.readyIds.clear();
	// Очищаем снимок идентификаторов закрываемых потоков
	this->_transfer.closeIds.clear();
	// Очищаем кольцо приоритетов ещё не открытых потоков
	this->_transfer.pendingPriorities.clear();
	/**
	 * Очищаем список декодированных заголовков: его представления ссылаются
	 * в арену пересоздаваемого декодера и после сброса недействительны
	 */
	this->_fields.clear();
	// Сбрасываем наибольший принятый идентификатор потока
	this->_transfer.lastStreamId = 0;
	// Очищаем кольцо оборванных сбросом потоков
	::std::fill(this->_transfer.resetStreams.begin(), this->_transfer.resetStreams.end(), reset_t());
	// Сбрасываем позицию записи в кольце
	this->_transfer.resetCursor = 0;
	// Сбрасываем наибольший наш открытый идентификатор потока
	this->_transfer.localOpened = 0;
	// Сбрасываем счётчик активных потоков, открытых пиром
	this->_transfer.peerStreamCount = 0;
	// Сбрасываем счётчик активных потоков, открытых нами
	this->_transfer.localStreamCount = 0;
	// Клиент инициирует нечётные потоки (1,3,5...), сервер - чётные (push: 2,4,6...)
	this->_transfer.nextStreamId = ((this->_direct == direct_t::REQUEST) ? 2 : 1);
	// Сервер ожидает клиентский preface; клиент отправляет его сам через sendPreface()
	this->_flags.prefaceReceived = (this->_direct != direct_t::REQUEST);
	// Сбрасываем защиту от реентерабельного pump()
	this->_flags.inPump = false;
	// Сбрасываем флаг отправленного GOAWAY
	this->_flags.goawaySent = false;
	// Сбрасываем флаг предупреждающего GOAWAY плавного завершения
	this->_flags.goawayGraceful = false;
	// Сбрасываем флаг отклонённого потока
	this->_flags.hbcRefused = false;
	// Сбрасываем флаг END_STREAM собираемого блока заголовков
	this->_flags.hbcEndStream = false;
	// Сбрасываем флаг подтверждения нашего SETTINGS
	this->_flags.settingsAcked = false;
	// Сбрасываем счётчик неподтверждённых отправок SETTINGS
	this->_transfer.settingsAckPending = 0;
	// Сбрасываем флаг получения SETTINGS пира
	this->_flags.settingsReceived = false;
	// Сбрасываем фиксацию параметра отказа от приоритетов RFC 7540
	this->_flags.prioritiesLocked = false;
	// Сбрасываем флаг полученного GOAWAY
	this->_flags.goawayReceived = false;
	// Сбрасываем идентификатор потока собираемого блока заголовков
	this->_hbc.stream = 0;
	// Сбрасываем счётчик фреймов блока заголовков
	this->_hbc.frames = 0;
	// Сбрасываем идентификатор обещанного потока
	this->_hbc.promised = 0;
	// Очищаем накопитель блока заголовков
	this->_hbc.buffer.clear();
	// Очищаем буфер неразобранного хвоста входящих данных
	this->clearInput();
	/**
	 * Очищаем отложенный буфер реентрантных вызовов: сброс из пользовательской
	 * функции означает новое соединение, а байты предыдущего к нему не относятся.
	 * Защиту от реентерабельности при этом не снимаем: внешний разбор ещё идёт
	 */
	this->_buffer.deferred.clear();
	// Очищаем буфер исходящих байтов
	this->_buffer.output.clear();
	// Сбрасываем отданный префикс буфера исходящих байтов
	this->_buffer.outputPos = 0;
	// Сбрасываем текущее время rate-лимитов
	this->_ratelims.now = 0;
	// Инициализируем лимит частоты входящих RST_STREAM из лимитов безопасности
	this->_ratelims.rst.init(this->_limits.rstLimitBurst, this->_limits.rstLimitRate);
	// Инициализируем лимит частоты управляющих фреймов из лимитов безопасности
	this->_ratelims.ctrl.init(this->_limits.ctrlLimitBurst, this->_limits.ctrlLimitRate);
	// Инициализируем лимит частоты кадров приоритета из лимитов безопасности
	this->_ratelims.prio.init(this->_limits.prioLimitBurst, this->_limits.prioLimitRate);
}
/**
 * @brief Метод клонирования объекта парсера
 *
 * @details Клон получает те же направление трафика, лимиты безопасности,
 *          параметры SETTINGS и функции обратного вызова, но чистое
 *          состояние соединения ("фабрика с теми же настройками").
 *
 * @return копия объекта парсера
 *
 */
unique_ptr <awh::http::parser_t> awh::http::Parser_HTTP2::clone() const noexcept {
	// Результат работы функции - копия объекта парсера
	unique_ptr <parser_t> result = nullptr;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Создаём новый объект парсера с теми же направлением трафика и инфраструктурой
		unique_ptr <Parser_HTTP2> parser(new Parser_HTTP2(this->_direct, this->_fmk, this->_log));
		// Копируем лимиты безопасности (с применением к rate-лимитам)
		parser->limits(this->_limits);
		// Копируем наши параметры SETTINGS
		parser->_local = this->_local;
		// Копируем порог сигнала writable
		parser->_transfer.sendLowWater = this->_transfer.sendLowWater;
		// Копируем ёмкость буфера отправки потока
		parser->_transfer.sendHighWater = this->_transfer.sendHighWater;
		// Копируем порог выходного буфера соединения
		parser->_transfer.outputHighWater = this->_transfer.outputHighWater;
		// Копируем функции обратного вызова
		parser->_callbacks = this->_callbacks;
		// Устанавливаем созданный объект парсера как результат
		result = ::move(parser);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод уведомления парсера о завершении потока данных (закрытии соединения)
 *
 * @details Если соединение завершено корректно (активных потоков нет, нет
 *          незавершённого фрейма или блока заголовков) - фиксируется статус
 *          COMPLETE. Если соединение закрыто посреди активных потоков или
 *          незавершённого фрейма - фиксируется ошибка PROTOCOL_ERROR
 *          (обрыв соединения).
 *
 */
void awh::http::Parser_HTTP2::eof() noexcept {
	// Если разбор уже завершился ошибкой - состояние не меняем
	if(this->_status == status_t::ERROR)
		// Выходим из метода
		return;
	// Если активных потоков нет, нет незавершённого фрейма и сборки блока заголовков
	if(this->_transfer.streams.empty() && (this->inputPending() == 0) && (this->_hbc.stream == 0))
		// Фиксируем корректное завершение соединения
		this->_status = status_t::COMPLETE;
	// Иначе соединение оборвано посреди работы
	else this->fail(error_t::PROTOCOL_ERROR, "connection closed unexpectedly");
}
/**
 * @brief Метод разбора данных
 *
 * @details Скармливает парсеру очередную порцию входящих байтов соединения.
 *          Неполный хвост фрейма буферизуется внутри до следующего вызова,
 *          поэтому метод всегда потребляет все переданные байты. По ходу
 *          разбора вызываются функции обратного вызова, а обязательные
 *          ответные фреймы уходят в канал записи.
 *          Итоговый статус необходимо контролировать методом status():
 *          - PARTIAL:  соединение живо, разбор продолжается;
 *          - COMPLETE: соединение завершено (обменялись GOAWAY);
 *          - ERROR:    ошибка уровня соединения - причина в методе error().
 *
 * @param buffer буфер данных для разбора
 * @param size   размер данных для разбора
 * @return       количество обработанных байт данных
 *
 */
size_t awh::http::Parser_HTTP2::parse(const void * buffer, const size_t size) noexcept {
	// Если разбор уже завершился ошибкой уровня соединения - данные игнорируются
	if(this->_status == status_t::ERROR)
		// Выводим количество обработанных байт данных
		return size;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если данные для разбора переданы
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Реентерабельный вызов из функции обратного вызова: дописывание во входной
			 * буфер способно перевыделить его память, а наружу уже отданы zero-copy
			 * указатели в этот же буфер (тело DATA, debug-данные GOAWAY) - обработчик
			 * продолжит читать по ним после возврата из вложенного разбора. Складываем
			 * байты отдельно, внешний разбор подхватит их вне пользовательских функций
			 */
			if(this->_flags.inParse)
				// Дописываем данные в отложенный буфер
				this->_buffer.deferred.append(static_cast <const char *> (buffer), size);
			// Иначе дописываем данные сразу во входной буфер
			else this->_buffer.input.append(static_cast <const char *> (buffer), size);
		}
		// Если разбор уже выполняется - подхватывать отложенное будет внешний вызов
		if(this->_flags.inParse)
			// Выводим количество обработанных байт данных
			return size;
		// Помечаем что разбор уже выполняется
		this->_flags.inParse = true;
		// Выполняем разбор накопленного входного буфера
		this->parseInput();
		/**
		 * Байты, накопленные реентрантными вызовами, дописываем здесь: в этой точке
		 * ни одна пользовательская функция не удерживает указателей во входной буфер,
		 * поэтому перевыделение памяти безопасно. Порядок сохраняется - вложенные
		 * байты пришли из сети после всего, что уже лежало во входном буфере
		 */
		while(!this->_buffer.deferred.empty() && (this->_status != status_t::ERROR)){
			// Дописываем отложенные байты во входной буфер
			this->_buffer.input.append(this->_buffer.deferred);
			// Очищаем отложенный буфер
			this->_buffer.deferred.clear();
			// Выполняем разбор накопленного входного буфера
			this->parseInput();
		}
		// Отбрасываем отложенные байты, оставшиеся после ошибки уровня соединения
		this->_buffer.deferred.clear();
		// Помечаем что разбор завершён
		this->_flags.inParse = false;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Очищаем отложенный буфер реентрантных вызовов
		this->_buffer.deferred.clear();
		// Помечаем что разбор завершён
		this->_flags.inParse = false;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
		// Фиксируем внутреннюю ошибку разбора
		this->fail(error_t::INTERNAL_ERROR, "unhandled exception");
	}
	// Если разбор не завершился ошибкой - актуализируем итоговый статус
	if(this->_status != status_t::ERROR){
		// Если соединение помечено на завершение и вся работа выполнена
		if(this->isClosed() && this->_transfer.streams.empty() && (this->_hbc.stream == 0))
			// Фиксируем корректное завершение соединения
			this->_status = status_t::COMPLETE;
		// Иначе соединение живо и разбор продолжается
		else this->_status = status_t::PARTIAL;
	}
	// Передаём исходящие байты сетевому слою
	this->flush();
	// Выводим количество обработанных байт данных
	return size;
}
/**
 * @brief Метод получения кода ошибки уровня соединения
 *
 * @return код ошибки протокола
 *
 */
awh::http::Parser_HTTP2::error_t awh::http::Parser_HTTP2::error() const noexcept {
	// Выводим код ошибки уровня соединения
	return this->_error;
}
/**
 * @brief Метод получения человекочитаемого названия текущей ошибки разбора
 *
 * @return название текущей ошибки разбора
 *
 */
string_view awh::http::Parser_HTTP2::errorName() const noexcept {
	// Выводим название текущей ошибки разбора
	return errorName(this->_error);
}
/**
 * @brief Метод получения человекочитаемого названия кода ошибки
 *
 * @param error код ошибки протокола
 * @return      название кода ошибки
 *
 */
string_view awh::http::Parser_HTTP2::errorName(const error_t error) noexcept {
	// Выводим название кода ошибки
	return h2::errorName(error);
}
/**
 * @brief Метод получения лимитов безопасности
 *
 * @return лимиты безопасности
 *
 */
const awh::http::Parser_HTTP2::limits_t & awh::http::Parser_HTTP2::limits() const noexcept {
	// Выводим лимиты безопасности
	return this->_limits;
}
/**
 * @brief Метод установки лимитов безопасности
 *
 * @param limits лимиты безопасности
 *
 */
void awh::http::Parser_HTTP2::limits(const limits_t & limits) noexcept {
	// Устанавливаем лимиты безопасности
	this->_limits = limits;
	// Применяем новые параметры лимита частоты входящих RST_STREAM
	this->_ratelims.rst.init(limits.rstLimitBurst, limits.rstLimitRate);
	// Применяем новые параметры лимита частоты управляющих фреймов
	this->_ratelims.ctrl.init(limits.ctrlLimitBurst, limits.ctrlLimitRate);
	// Применяем новые параметры лимита частоты кадров приоритета
	this->_ratelims.prio.init(limits.prioLimitBurst, limits.prioLimitRate);
	// Предупреждаем, если лимит распакованного списка заголовков снят полностью
	this->checkHeaderListLimits();
}
/**
 * @brief Метод получения наших параметров SETTINGS
 *
 * @return наши параметры SETTINGS
 *
 */
const awh::http::Parser_HTTP2::settings_t & awh::http::Parser_HTTP2::settings() const noexcept {
	// Выводим наши параметры SETTINGS
	return this->_local;
}
/**
 * @brief Метод установки наших параметров SETTINGS
 *
 * @note Отправка выполняется методами sendPreface()/sendSettings()
 *
 * @param settings наши параметры SETTINGS
 *
 */
void awh::http::Parser_HTTP2::settings(const settings_t & settings) noexcept {
	// Устанавливаем наши параметры SETTINGS
	this->_local = settings;
	/**
	 * Приводим значения к допустимому протоколом диапазону (RFC 9113 §6.5.2). Пир
	 * обязан оборвать соединение на некорректном параметре, поэтому отправить его
	 * означает гарантированно потерять соединение из-за ошибки настройки
	 */
	if((this->_local.windowSize < 0) || (this->_local.windowSize > h2::proto::MAX_WINDOW_SIZE)){
		// Записываем сообщение об ошибке в лог
		this->_log->print("HTTP/2 INITIAL_WINDOW_SIZE %d is out of range, reset to %d", log_t::flag_t::WARNING, this->_local.windowSize, h2::proto::DEFAULT_WINDOW_SIZE);
		// Возвращаем начальное окно потока к значению по умолчанию
		this->_local.windowSize = h2::proto::DEFAULT_WINDOW_SIZE;
	}
	// Если размер фрейма выходит за допустимый протоколом диапазон
	if((this->_local.maxFrameSize < h2::proto::MIN_MAX_FRAME_SIZE) || (this->_local.maxFrameSize > h2::proto::MAX_MAX_FRAME_SIZE)){
		// Записываем сообщение об ошибке в лог
		this->_log->print("HTTP/2 MAX_FRAME_SIZE %u is out of range, reset to %u", log_t::flag_t::WARNING, this->_local.maxFrameSize, h2::proto::DEFAULT_MAX_FRAME_SIZE);
		// Возвращаем максимальный размер фрейма к значению по умолчанию
		this->_local.maxFrameSize = h2::proto::DEFAULT_MAX_FRAME_SIZE;
	}
	// Если разрешение server push задано недопустимым значением
	if(this->_local.enablePush > 1){
		// Записываем сообщение об ошибке в лог
		this->_log->print("HTTP/2 ENABLE_PUSH %u is invalid, reset to 1", log_t::flag_t::WARNING, this->_local.enablePush);
		// Возвращаем разрешение server push к значению по умолчанию
		this->_local.enablePush = 1;
	}
	// Если разрешение расширенного CONNECT задано недопустимым значением
	if(this->_local.enableConnectProtocol > 1){
		// Записываем сообщение об ошибке в лог
		this->_log->print("HTTP/2 ENABLE_CONNECT_PROTOCOL %u is invalid, reset to 0", log_t::flag_t::WARNING, this->_local.enableConnectProtocol);
		// Запрещаем расширенный метод CONNECT
		this->_local.enableConnectProtocol = 0;
	}
	// Если отказ от приоритетов RFC 7540 задан недопустимым значением
	if(this->_local.noRfc7540Priorities > 1){
		// Записываем сообщение об ошибке в лог
		this->_log->print("HTTP/2 NO_RFC7540_PRIORITIES %u is invalid, reset to 1", log_t::flag_t::WARNING, this->_local.noRfc7540Priorities);
		// Возвращаем отказ от приоритетов RFC 7540 к значению по умолчанию
		this->_local.noRfc7540Priorities = 1;
	}
	// Предупреждаем, если лимит распакованного списка заголовков снят полностью
	this->checkHeaderListLimits();
}
/**
 * @brief Метод получения параметров SETTINGS пира
 *
 * @return параметры SETTINGS пира
 *
 */
const awh::http::Parser_HTTP2::settings_t & awh::http::Parser_HTTP2::remoteSettings() const noexcept {
	// Выводим параметры SETTINGS пира
	return this->_remote;
}
/**
 * @brief Метод проверки того, что соединение помечено на завершение
 *
 * @return признак завершения (отправлен или получен GOAWAY)
 *
 */
bool awh::http::Parser_HTTP2::isClosed() const noexcept {
	// Соединение помечено на завершение, если GOAWAY отправлен или получен
	return (this->_flags.goawaySent || this->_flags.goawayReceived);
}
/**
 * @brief Метод проверки того, что наш SETTINGS подтверждён пиром
 *
 * @return признак получения ACK на наш SETTINGS
 *
 */
bool awh::http::Parser_HTTP2::isSettingsAcked() const noexcept {
	// Выводим признак получения ACK на наш SETTINGS
	return this->_flags.settingsAcked;
}
/**
 * @brief Метод отправки исходящего preface соединения
 *
 * @details Клиент отправляет magic-строку + свой SETTINGS, сервер - только SETTINGS.
 *          Обязан быть первым исходящим сообщением соединения.
 *
 */
void awh::http::Parser_HTTP2::sendPreface() noexcept {
	// Если мы - клиент (разбираем ответы сервера) - отправляем magic-строку preface
	if(this->_direct == direct_t::RESPONSE)
		// Дописываем magic-строку preface в буфер исходящих байтов
		this->_buffer.output.append(h2::proto::PREFACE.data(), h2::proto::PREFACE.size());
	// Отправляем наш SETTINGS-фрейм (с передачей исходящих байтов сетевому слою)
	this->sendSettings();
}
/**
 * @brief Метод отправки нашего SETTINGS-фрейма (текущие параметры)
 *
 */
void awh::http::Parser_HTTP2::sendSettings() noexcept {
	/**
	 * Наш декодер обязан держать таблицу не больше, чем мы анонсируем, и отвергать
	 * Dynamic Table Size Update пира свыше этого значения (RFC 7541 §6.3)
	 */
	this->_decoder.setProtocolMaxSize(this->_local.headerTableSize);
	// Ограничиваем размер динамической таблицы декодера анонсируемым значением
	this->_decoder.table().setMaxSize(this->_local.headerTableSize);
	/**
	 * Изменение анонсируемого начального окна приёма сдвигает окна приёма всех уже
	 * открытых потоков на дельту (RFC 9113 §6.9.2): без этого наш учёт принятых байт
	 * разъезжается с учётом пира и приводит к ложным FLOW_CONTROL_ERROR
	 */
	if(this->_local.windowSize != this->_window.localInit){
		// Вычисляем дельту изменения начального окна приёма
		const int64_t delta = (static_cast <int64_t> (this->_local.windowSize) - this->_window.localInit);
		// Запоминаем анонсируемое начальное окно приёма потока
		this->_window.localInit = this->_local.windowSize;
		/**
		 * Выполняем сдвиг окон приёма всех открытых потоков. Переполнение здесь
		 * невозможно: окно приёма потока никогда не превышает анонсированного
		 * начального размера, а сам параметр приведён к допустимому диапазону
		 * методом settings() - поэтому результат не выходит за новое начальное окно
		 */
		for(auto & item : this->_transfer.streams){
			// Вычисляем новое окно приёма потока
			const int64_t window = (static_cast <int64_t> (item.second.localWindow) + delta);
			// Применяем новое окно приёма потока
			item.second.localWindow = static_cast <int32_t> (::min(static_cast <int64_t> (h2::proto::MAX_WINDOW_SIZE), window));
		}
	}
	// Список отправляемых параметров SETTINGS
	h2::frame::setting_entry_t items[8];
	// Количество отправляемых параметров
	size_t count = 0;
	// Добавляем параметр размера динамической таблицы HPACK
	items[count].id = h2::setting_t::HEADER_TABLE_SIZE;
	// Устанавливаем значение параметра
	items[count++].value = this->_local.headerTableSize;
	/**
	 * Параметр разрешения server push анонсирует только клиент: им он сообщает,
	 * готов ли принимать push. Для сервера параметр смысла не имеет, а значение 1
	 * сервер отправлять прямо запрещено (RFC 9113 §6.5.2) - клиент обязан
	 * оборвать такое соединение с PROTOCOL_ERROR
	 */
	if(this->_direct == direct_t::RESPONSE){
		// Добавляем параметр разрешения server push
		items[count].id = h2::setting_t::ENABLE_PUSH;
		// Устанавливаем значение параметра
		items[count++].value = this->_local.enablePush;
	}
	// Добавляем параметр начального окна потока
	items[count].id = h2::setting_t::INITIAL_WINDOW_SIZE;
	// Устанавливаем значение параметра
	items[count++].value = static_cast <uint32_t> (this->_local.windowSize);
	// Добавляем параметр максимального размера фрейма
	items[count].id = h2::setting_t::MAX_FRAME_SIZE;
	// Устанавливаем значение параметра
	items[count++].value = this->_local.maxFrameSize;
	// MAX_CONCURRENT_STREAMS: 0xFFFFFFFF - это "без лимита", анонсировать незачем
	if(this->_local.maxConcurrentStreams != 0xFFFFFFFF){
		// Добавляем параметр лимита одновременных потоков
		items[count].id = h2::setting_t::MAX_CONCURRENT_STREAMS;
		// Устанавливаем значение параметра
		items[count++].value = this->_local.maxConcurrentStreams;
	}
	// MAX_HEADER_LIST_SIZE: 0 у нас означает "без лимита" (нет sentinel-значения в протоколе) - пропускаем
	if(this->_local.maxHeaderListSize != 0){
		// Добавляем параметр лимита размера списка заголовков
		items[count].id = h2::setting_t::MAX_HEADER_LIST_SIZE;
		// Устанавливаем значение параметра
		items[count++].value = this->_local.maxHeaderListSize;
	}
	// Расширенный CONNECT анонсируется только когда включён (RFC 8441 §3: отзыв запрещён)
	if(this->_local.enableConnectProtocol != 0){
		// Добавляем параметр разрешения расширенного CONNECT
		items[count].id = h2::setting_t::ENABLE_CONNECT_PROTOCOL;
		// Устанавливаем значение параметра
		items[count++].value = this->_local.enableConnectProtocol;
	}
	// Анонсируем отказ от приоритетов RFC 7540, если он объявлен (RFC 9218 §2.1)
	if(this->_local.noRfc7540Priorities != 0){
		// Добавляем параметр отказа от приоритетов RFC 7540
		items[count].id = h2::setting_t::NO_RFC7540_PRIORITIES;
		// Устанавливаем значение параметра
		items[count++].value = this->_local.noRfc7540Priorities;
	}
	// Отправляем SETTINGS-фрейм с собранными параметрами
	h2::frame::serialize::settings(this->_buffer.output, items, count, false);
	/**
	 * Ждём подтверждения на каждый отправленный SETTINGS (RFC 9113 §6.5.3).
	 * Флаг подтверждения снимается: новые параметры вступают в силу для пира
	 * только после его ACK, и до тех пор isSettingsAcked() обязан отвечать
	 * отрицательно - иначе таймер SETTINGS_TIMEOUT сторожит уже неактуальную отправку
	 */
	++this->_transfer.settingsAckPending;
	// Снимаем флаг подтверждения нашего SETTINGS
	this->_flags.settingsAcked = false;
	// Передаём исходящие байты сетевому слою
	this->flush();
}
/**
 * @brief Метод отправки RST_STREAM (аварийное закрытие потока)
 *
 * @param sid  идентификатор потока
 * @param code код ошибки, с которым сбрасывается поток
 *
 */
void awh::http::Parser_HTTP2::sendRstStream(const uint32_t sid, const error_t code) noexcept {
	// Отправляем фрейм RST_STREAM
	this->rejectStream(sid, code);
	// Закрываем поток с вызовом функции обратного вызова закрытия (как и при входящем сбросе)
	this->closeStream(sid, code);
	// Передаём исходящие байты сетевому слою
	this->flush();
}
/**
 * @brief Метод отправки GOAWAY (пометка соединения завершаемым)
 *
 * @param code  код ошибки завершения соединения
 * @param debug необязательные отладочные данные
 *
 */
void awh::http::Parser_HTTP2::sendGoaway(const error_t code, string_view debug) noexcept {
	// Отправляем фрейм GOAWAY с наибольшим принятым идентификатором потока
	h2::frame::serialize::goaway(this->_buffer.output, this->_transfer.lastStreamId, code, debug);
	// Помечаем что GOAWAY отправлен
	this->_flags.goawaySent = true;
	// Передаём исходящие байты сетевому слою
	this->flush();
}
/**
 * @brief Метод начала плавного завершения соединения (RFC 9113 §6.8)
 *
 * @param debug необязательные отладочные данные
 *
 */
void awh::http::Parser_HTTP2::sendShutdown(string_view debug) noexcept {
	// Если соединение уже завершается - повторное предупреждение не требуется
	if(this->_flags.goawaySent || this->_flags.goawayGraceful)
		// Выходим из метода
		return;
	/**
	 * Предупреждающий GOAWAY объявляет максимально возможный идентификатор потока:
	 * пир узнаёт о предстоящем закрытии, но ни один поток не считается отклонённым
	 * (RFC 9113 §6.8). Флаг отправленного GOAWAY при этом не выставляется - иначе
	 * мы сами перестали бы открывать потоки, не дав соединению доработать
	 */
	h2::frame::serialize::goaway(this->_buffer.output, h2::proto::MAX_STREAM_ID, error_t::NO_ERROR, debug);
	// Помечаем что предупреждение о завершении отправлено
	this->_flags.goawayGraceful = true;
	// Передаём исходящие байты сетевому слою
	this->flush();
}
/**
 * @brief Метод отправки WINDOW_UPDATE
 *
 * @param sid       идентификатор потока (0 - окно всего соединения)
 * @param increment инкремент окна flow control
 *
 */
void awh::http::Parser_HTTP2::sendPriority(const uint32_t sid, const uint8_t urgency, const bool incremental) noexcept {
	/**
	 * Кадр отправляет только клиент: серверу это запрещено прямо, а получивший
	 * его клиент обязан оборвать соединение (RFC 9218 §7.1). Приоритет своего
	 * ответа сервер объявляет заголовком priority, а не кадром
	 */
	if(this->_direct == direct_t::REQUEST){
		// Записываем сообщение об ошибке в лог
		this->_log->print("HTTP/2 server is not allowed to send PRIORITY_UPDATE", log_t::flag_t::WARNING);
		// Выходим из метода
		return;
	}
	/**
	 * Клиент вправе приоритизировать не только собственный поток запроса, но и
	 * обещанный ему push-поток (RFC 9218 §7.1). Поток пира, о котором мы ничего
	 * не знаем, приоритизировать нельзя: для push это состояние idle, а его
	 * получатель обязан считать ошибкой соединения
	 */
	if(this->peerInitiated(sid) && (this->findStream(sid) == nullptr))
		// Выходим из метода
		return;
	// Формируем значение поля приоритета структурированным словарём (RFC 8941)
	string value = "u=";
	// Дописываем срочность потока, ограниченную допустимым диапазоном
	value.push_back(static_cast <char> ('0' + ::min(urgency, h2::proto::MAX_URGENCY)));
	// Если требуется инкрементальная доставка потока
	if(incremental)
		// Дописываем признак инкрементальной доставки
		value.append(", i");
	// Отправляем фрейм обновления расширенного приоритета
	h2::frame::serialize::priorityUpdate(this->_buffer.output, sid, value);
	// Если поток уже существует - применяем приоритет и к своей стороне
	stream_t * stream = this->findStream(sid);
	// Если поток найден
	if(stream != nullptr){
		// Применяем срочность потока
		stream->urgency = ::min(urgency, h2::proto::MAX_URGENCY);
		// Применяем признак инкрементальной доставки
		stream->incremental = incremental;
	}
	// Передаём исходящие байты сетевому слою
	this->flush();
}
/**
 * @brief Метод отправки WINDOW_UPDATE
 *
 * @details Выданный пиру кредит сразу учитывается в окне приёма: иначе парсер
 *          не помнит собственного разрешения и рвёт соединение на теле,
 *          присланном строго в объявленных границах. Целевой размер окна
 *          не меняется - это разовая добавка, а не новый уровень; поднять
 *          уровень окна соединения можно методом connectionReceiveWindow().
 *
 * @param sid       идентификатор потока (0 - окно всего соединения)
 * @param increment инкремент окна flow control
 *
 */
void awh::http::Parser_HTTP2::sendWindowUpdate(const uint32_t sid, const uint32_t increment) noexcept {
	/**
	 * Инкремент вне диапазона 1..2^31-1 (RFC 9113 §6.9) не отправляем. Нулевой пир
	 * обязан считать ошибкой, а у вышедшего за диапазон сборка кадра снимает
	 * reserved-бит - на провод уйдёт искажённое значение, вплоть до того же нуля.
	 * Проверкой суммы такой инкремент не ловится: окно приёма потока бывает
	 * отрицательным после снижения нашего SETTINGS_INITIAL_WINDOW_SIZE (§6.9.2),
	 * и сумма с ним снова укладывается в допустимый диапазон
	 */
	if((increment == 0) || (increment > static_cast <uint32_t> (h2::proto::MAX_WINDOW_SIZE)))
		// Выходим из метода
		return;
	// Если кредит выдаётся окну всего соединения
	if(sid == 0){
		// Если добавка выводит окно приёма за предельное значение (RFC 9113 §6.9.1)
		if((static_cast <int64_t> (this->_window.local) + increment) > h2::proto::MAX_WINDOW_SIZE)
			// Выходим из метода
			return;
		// Увеличиваем окно приёма соединения на выданный кредит
		this->_window.local += static_cast <int32_t> (increment);
	// Если кредит выдаётся окну конкретного потока
	} else {
		// Выполняем поиск потока
		stream_t * stream = this->findStream(sid);
		// Кредит несуществующему потоку пир вправе считать ошибкой - не отправляем
		if(stream == nullptr)
			// Выходим из метода
			return;
		// Если добавка выводит окно приёма потока за предельное значение (RFC 9113 §6.9.1)
		if((static_cast <int64_t> (stream->localWindow) + increment) > h2::proto::MAX_WINDOW_SIZE)
			// Выходим из метода
			return;
		// Увеличиваем окно приёма потока на выданный кредит
		stream->localWindow += static_cast <int32_t> (increment);
	}
	// Отправляем фрейм WINDOW_UPDATE
	h2::frame::serialize::windowUpdate(this->_buffer.output, sid, increment);
	// Передаём исходящие байты сетевому слою
	this->flush();
}
/**
 * @brief Метод передачи части тела потока для отправки (push-модель, bounded buffer)
 *
 * @details Копирует во внутренний буфер потока столько байт, сколько влезает до
 *          high-water, и возвращает это число (0..size). Если вернулось меньше
 *          size - буфер заполнен: приостановите выдачу и дождитесь функции
 *          обратного вызова writable. Нарезку во фреймы, учёт окон и
 *          автоматическую досылку по WINDOW_UPDATE парсер делает сам.
 *
 * @param sid       идентификатор потока
 * @param buffer    буфер данных тела
 * @param size      размер данных тела
 * @param endStream флаг завершения потока
 * @return          число принятых байт (0..size)
 *
 */
size_t awh::http::Parser_HTTP2::sendData(const uint32_t sid, const void * buffer, const size_t size, const bool endStream) noexcept {
	// Результат работы функции - число принятых байт
	size_t result = 0;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем поиск потока
		stream_t * stream = this->findStream(sid);
		// Если поток не найден (неизвестный/закрытый поток) - данные не принимаются
		if(stream == nullptr)
			// Выводим число принятых байт
			return result;
		/**
		 * Нельзя слать тело после уже поставленного/отправленного END_STREAM, а также
		 * из состояний, где наша половина закрыта (half-closed(local)/closed)
		 */
		if(stream->endStreamPending || stream->endStreamSent)
			// Выводим число принятых байт
			return result;
		/**
		 * Данные тела допустимы только из состояний open и half-closed(remote)
		 * (RFC 9113 §5.1): в зарезервированный под push поток до отправки его
		 * заголовков тело слать нельзя
		 */
		if((stream->state != h2::stream_state_t::OPEN) && (stream->state != h2::stream_state_t::HALF_CLOSED_REMOTE)){
			// Записываем сообщение об ошибке в лог
			this->_log->print("HTTP/2 stream %u does not accept body in its current state", log_t::flag_t::WARNING, sid);
			// Выводим число принятых байт
			return result;
		}
		// Если секция трейлеров уже отложена - тело после неё не принимается
		if(stream->trailersPending)
			// Выводим число принятых байт
			return result;
		/**
		 * Ответ на запрос методом HEAD содержимого не несёт (RFC 9110 §9.3.2): тело
		 * принимается и отбрасывается, а не отвергается. Отказ приёмом нуля байт
		 * приложение прочло бы как заполненный буфер и ждало бы сигнала writable,
		 * которого при пустом буфере не будет
		 */
		if(stream->bodylessSend){
			// Признаём принятым весь фрагмент, не отправляя из него ничего
			result = size;
			// Если фрагмент финальный - поток всё равно обязан завершиться
			if(endStream){
				// Помечаем что на последнем фрагменте нужно выставить END_STREAM
				stream->endStreamPending = true;
				// Ставим поток в очередь готовых к отправке
				this->markReady(* stream);
			}
			// Прокачиваем отправку по всем потокам
			this->pump();
			// Передаём исходящие байты сетевому слою (выход минует общий flush метода)
			this->flush();
			// Выводим число принятых байт
			return result;
		}
		// Вычисляем свободное место в буфере отправки до high-water
		const size_t room = ((stream->pending() < this->_transfer.sendHighWater) ? (this->_transfer.sendHighWater - stream->pending()) : 0);
		// Принимаем столько байт, сколько влезает (частичный приём + счётчик)
		result = ::min(size, room);
		// Если есть что принимать - дописываем данные в буфер отправки потока
		if(result > 0)
			// Дописываем данные в буфер отправки потока
			stream->sendBuffer.append(static_cast <const char *> (buffer), result);
		// END_STREAM помечаем только когда принят весь финальный фрагмент
		if(endStream && (result == size))
			// Помечаем что на последнем фрагменте нужно выставить END_STREAM
			stream->endStreamPending = true;
		// Ставим поток в очередь готовых к отправке
		this->markReady(* stream);
		// Если буфер отправки поднялся выше low-water - взводим сигнал writable снова
		if(stream->pending() > this->_transfer.sendLowWater)
			// Взводим сигнал writable для следующего провала буфера
			stream->writableNotified = false;
		// Прокачиваем отправку по всем потокам
		this->pump();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid, buffer, size, endStream), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Передаём исходящие байты сетевому слою
	this->flush();
	// Выводим число принятых байт
	return result;
}
/**
 * @brief Метод анонса server push (только сервер)
 *
 * @details Отправляет PUSH_PROMISE на потоке клиента и резервирует чётный
 *          push-поток. Дальше ответ отправляется обычным путём:
 *          sendHeaders(promisedSid, ...) + sendData(promisedSid, ...).
 *
 * @param sid    идентификатор потока клиента, в ответ на который выполняется push
 * @param fields заголовки обещанного запроса (псевдо-заголовки как у запроса клиента)
 * @return       идентификатор зарезервированного push-потока либо 0, если push невозможен
 *
 */
uint32_t awh::http::Parser_HTTP2::sendPushPromise(const uint32_t sid, const vector <h2::hpack::field_t> & fields) noexcept {
	// Результат работы функции - идентификатор зарезервированного push-потока
	uint32_t result = 0;
	// Push инициирует только сервер (мы - сервер, если разбираем запросы)
	if(this->_direct != direct_t::REQUEST)
		// Push невозможен
		return result;
	// Клиент запретил push своим SETTINGS_ENABLE_PUSH=0
	if(this->_remote.enablePush == 0)
		// Push невозможен
		return result;
	// Ассоциированный поток должен быть открыт пиром и ещё жив
	stream_t * assoc = this->findStream(sid);
	// Если ассоциированный поток не существует
	if(assoc == nullptr)
		// Push невозможен
		return result;
	// Если ассоциированный поток не находится в допустимом состоянии
	if((assoc->state != h2::stream_state_t::OPEN) && (assoc->state != h2::stream_state_t::HALF_CLOSED_REMOTE))
		// Push невозможен
		return result;
	// Если соединение помечено на завершение - новые потоки на нём не открываются (RFC 9113 §6.8)
	if(this->_flags.goawayReceived || this->_flags.goawaySent)
		// Push невозможен
		return result;
	/**
	 * Зарезервированный нами поток учитывается в лимите одновременных потоков,
	 * разрешённом пиром (RFC 9113 §5.1.2)
	 */
	if(this->_transfer.localStreamCount >= this->_remote.maxConcurrentStreams)
		// Push невозможен
		return result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Резервируем чётный push-поток
		result = this->nextStreamId();
		// Если пространство идентификаторов исчерпано - push невозможен
		if(result == 0)
			// Выводим результат
			return result;
		// Закодированный HPACK-блок заголовков обещанного запроса
		string block = "";
		// Выполняем кодирование заголовков в HPACK-блок
		this->_encoder.encode(fields, block, true);
		// Сверяем размер блока обещанного запроса с лимитом списка заголовков пира
		this->checkPeerHeaderList(sid);
		// Отправляем PUSH_PROMISE (с автоматической нарезкой на PUSH_PROMISE + CONTINUATION)
		h2::frame::serialize::pushPromiseBlock(this->_buffer.output, sid, result, block, this->_remote.maxFrameSize);
		// Получаем объект зарезервированного push-потока
		stream_t & stream = this->stream(result);
		// Переводим push-поток в состояние RESERVED_LOCAL
		stream.state = h2::stream_state_t::RESERVED_LOCAL;
		/**
		 * Учитываем поток в лимите одновременных потоков: без этого счётчик
		 * рассинхронизировался бы с картой потоков, ведь при закрытии push-потока
		 * он уменьшается наравне с остальными нашими потоками
		 */
		++this->_transfer.localStreamCount;
		// Запоминаем наибольший наш открытый идентификатор потока
		if(result > this->_transfer.localOpened)
			// Обновляем наибольший наш открытый идентификатор потока
			this->_transfer.localOpened = result;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid, fields.size()), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
		// Push не выполнен
		return 0;
	}
	// Передаём исходящие байты сетевому слою
	this->flush();
	// Выводим идентификатор зарезервированного push-потока
	return result;
}
/**
 * @brief Метод отправки блока заголовков (запрос/ответ/трейлеры) потока
 *
 * @details Если поток ещё не существует и мы инициатор - поток открывается.
 *          При endStream поток сразу полузакрывается с нашей стороны (тела не будет).
 *          Блок, превышающий SETTINGS_MAX_FRAME_SIZE пира, автоматически режется
 *          на HEADERS + CONTINUATION (RFC 9113 §6.2/§6.10).
 *
 * @param sid       идентификатор потока
 * @param fields    заголовки (псевдо-заголовки :method/:path/... должны идти первыми)
 * @param endStream флаг завершения потока (тела не будет)
 *
 */
void awh::http::Parser_HTTP2::sendHeaders(const uint32_t sid, const vector <h2::hpack::field_t> & fields, const bool endStream) noexcept {
	// Если отправка блока заголовков в этот поток недопустима
	if(!this->canSendHeaders(sid))
		// Выходим из метода
		return;
	/**
	 * Если тело потока ещё не отправлено полностью - это секция трейлеров, и она
	 * обязана уйти строго после данных, которые завершает
	 */
	if(this->deferTrailers(sid, fields, endStream))
		// Выходим из метода
		return;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Закодированный HPACK-блок заголовков
		string block = "";
		// Дописываем отложенный Dynamic Table Size Update (если требуется)
		this->_encoder.begin(block);
		/**
		 * Выполняем перебор всех кодируемых заголовков
		 */
		for(const h2::hpack::field_t & field : fields){
			/**
			 * Connection-specific заголовки в HTTP/2 запрещены (RFC 9113 §8.2.2) и делают
			 * сообщение малформированным на приёмной стороне - пропускаем их
			 */
			if(::isConnectionSpecific(field.name) || ((field.name == header::TE) && !this->_fmk->compare("trailers", field.value))){
				// Записываем сообщение об ошибке в лог
				this->_log->print("HTTP/2 connection-specific header [%s] is not allowed and skipped", log_t::flag_t::WARNING, field.name.c_str());
				// Переходим к следующему заголовку
				continue;
			}
			/**
			 * Названия в HTTP/2 передаются только в нижнем регистре (RFC 9113 §8.2.1).
			 * Эта перегрузка отдаёт байты как есть - она нужна в том числе для построения
			 * нештатного трафика, поэтому название не переписывается, но о нарушении
			 * вызывающий узнаёт из лога
			 */
			if(!::isValidHeaderName(field.name))
				// Записываем сообщение об ошибке в лог
				this->_log->print("HTTP/2 header name [%s] is not a valid lowercase token", log_t::flag_t::WARNING, field.name.c_str());
			// Кодируем очередной заголовок
			this->_encoder.encode(field.name, field.value, block, field.sensitive, true);
		}
		// Отправляем HPACK-блок заголовков потока
		this->commitHeaders(sid, block, endStream);
		/**
		 * Выполняем поиск метода запроса: ответ на HEAD объявляет content-length,
		 * но тела не несёт (RFC 9110 §9.3.2) - сверять их нельзя
		 */
		for(const h2::hpack::field_t & field : fields){
			// Если получен псевдо-заголовок метода запроса
			if(field.name == header::METHOD){
				// Если запрос выполняется методом HEAD
				if(field.value == value::HEAD){
					// Выполняем поиск потока
					stream_t * stream = this->findStream(sid);
					// Если поток найден
					if(stream != nullptr)
						// Помечаем что ответ на этот поток тела не несёт
						stream->bodyless = true;
				}
				// Прекращаем поиск
				break;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid, fields.size(), endStream), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Передаём исходящие байты сетевому слою
	this->flush();
}
/**
 * @brief Метод отправки блока заголовков потока из контейнера заголовков (zero-copy)
 *
 * @details Заголовки кодируются в HPACK напрямую из контейнера, без промежуточных
 *          копий. Псевдо-заголовки формируются автоматически из провайдера контейнера:
 *          для запроса (request_t) - [:method]/[:scheme]/[:authority]/[:path]
 *          (для метода CONNECT - только [:method]/[:authority], RFC 9113 §8.5),
 *          для ответа (response_t) - [:status]. Заголовок Host конвертируется
 *          в [:authority]. Если провайдер контейнера не установлен - блок кодируется
 *          без псевдо-заголовков (трейлеры). Названия заголовков приводятся к нижнему
 *          регистру (RFC 9113 §8.2.1), запрещённые в HTTP/2 connection-specific
 *          заголовки (Connection/Keep-Alive/Proxy-Connection/Transfer-Encoding/Upgrade,
 *          а также TE со значением кроме "trailers") пропускаются (RFC 9113 §8.2.2).
 *
 * @param sid       идентификатор потока
 * @param headers   контейнер заголовков (провайдер контейнера задаёт псевдо-заголовки)
 * @param endStream флаг завершения потока (тела не будет)
 * @param scheme    схема запроса для псевдо-заголовка [:scheme] (для ответа не используется)
 *
 */
void awh::http::Parser_HTTP2::sendHeaders(const uint32_t sid, const headers_t & headers, const bool endStream, string_view scheme) noexcept {
	// Если отправка блока заголовков в этот поток недопустима
	if(!this->canSendHeaders(sid))
		// Выходим из метода
		return;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем поиск потока
		const stream_t * stream = this->findStream(sid);
		/**
		 * Если тело потока ещё не отправлено полностью - это секция трейлеров. Она
		 * обязана уйти после данных, которые завершает, поэтому заголовки собираются
		 * в отдельный список и откладываются. Псевдо-заголовков в трейлерах нет,
		 * поэтому сборка сводится к переносу полей контейнера
		 */
		if((stream != nullptr) && stream->headersSent && ((stream->pending() > 0) || !this->sourceDone(* stream))){
			// Список заголовков секции трейлеров
			vector <h2::hpack::field_t> fields;
			// Резервируем память под заголовки секции трейлеров
			fields.reserve(headers.size());
			// Переиспользуемый буфер для названий заголовков в нижнем регистре
			string buffer = "";
			/**
			 * Выполняем перебор всех заголовков контейнера
			 */
			for(const headers_t::header_t & header : headers){
				// Приводим название заголовка к нижнему регистру (RFC 9113 §8.2.1)
				const string_view name = ::lowerName(header.name, buffer);
				// Пропускаем запрещённые в HTTP/2 connection-specific заголовки (RFC 9113 §8.2.2)
				if(::isConnectionSpecific(name) || (name == header::HOST))
					// Переходим к следующему заголовку
					continue;
				// Заголовок TE допустим только со значением "trailers" (RFC 9113 §8.2.2)
				if((name == header::TE) && !this->_fmk->compare("trailers", header.value))
					// Переходим к следующему заголовку
					continue;
				// Дописываем заголовок в список секции трейлеров
				fields.emplace_back(string(name), header.value);
			}
			// Откладываем секцию трейлеров до конца отправки тела
			if(this->deferTrailers(sid, fields, endStream))
				// Выходим из метода
				return;
		}
		// Получаем объект провайдера контейнера заголовков
		const provider_t * provider = headers.provider();
		// Если провайдер является запросом клиента
		if((provider != nullptr) && (provider->direct == direct_t::REQUEST)){
			// Получаем объект провайдера запроса клиента
			const request_t * request = static_cast <const request_t *> (provider);
			/**
			 * Расширенный CONNECT допустим, только если пир анонсировал его параметром
			 * SETTINGS_ENABLE_CONNECT_PROTOCOL (RFC 8441 §3). Запрос не отправляется целиком,
			 * а не лишается [:protocol]: без этого псевдо-заголовка получился бы обычный
			 * туннель CONNECT - другая семантика, которой приложение не просило.
			 * Проверка стоит до начала кодирования, иначе отложенный Dynamic Table Size Update
			 * ушёл бы в отброшенный блок и рассинхронизировал декодер пира
			 */
			if(!request->protocol.empty() && (request->method == method_t::CONNECT) && (this->_remote.enableConnectProtocol == 0)){
				// Записываем сообщение об отказе в лог
				this->_log->print(
					"HTTP/2 peer does not support extended CONNECT (RFC 8441), request for stream %u is not sent",
					log_t::flag_t::WARNING, sid
				);
				// Выходим из метода
				return;
			}
		}
		// Закодированный HPACK-блок заголовков
		string block = "";
		// Дописываем отложенный Dynamic Table Size Update (если требуется)
		this->_encoder.begin(block);
		// Если провайдер контейнера установлен - формируем псевдо-заголовки (RFC 9113 §8.3)
		if(provider != nullptr){
			/**
			 * Определяем направление трафика провайдера
			 */
			switch(static_cast <uint8_t> (provider->direct)){
				// Если провайдер является запросом клиента
				case static_cast <uint8_t> (direct_t::REQUEST): {
					// Получаем объект провайдера запроса клиента
					const request_t * request = static_cast <const request_t *> (provider);
					// Кодируем псевдо-заголовок [:method]
					this->_encoder.encode(":method", awh::http::methodName(request), block);
					/**
					 * Расширенный CONNECT (RFC 8441 §4) обязан нести [:scheme] и [:path],
					 * тогда как классический CONNECT их запрещает (RFC 9113 §8.5)
					 */
					const bool extended = (!request->protocol.empty() && (request->method == method_t::CONNECT));
					// Если псевдо-заголовки схемы и пути допустимы для этого запроса
					if((request->method != method_t::CONNECT) || extended)
						// Кодируем псевдо-заголовок [:scheme]
						this->_encoder.encode(":scheme", scheme, block);
					// Если контейнер содержит заголовок Host - конвертируем его в псевдо-заголовок [:authority]
					if(headers.has("host"))
						// Кодируем псевдо-заголовок [:authority]
						this->_encoder.encode(":authority", headers.at("host"), block);
					/**
					 * Для метода CONNECT псевдо-заголовок [:authority] обязателен (RFC 9113 §8.5),
					 * а заголовка Host в таком запросе может не быть - берём цель из URI запроса
					 */
					else if((request->method == method_t::CONNECT) && !request->uri.empty())
						// Кодируем псевдо-заголовок [:authority]
						this->_encoder.encode(":authority", request->uri, block);
					// Если псевдо-заголовки схемы и пути допустимы для этого запроса
					if((request->method != method_t::CONNECT) || extended)
						// Кодируем псевдо-заголовок [:path] (пустой путь запрещён - подставляем "/")
						this->_encoder.encode(":path", (request->uri.empty() ? "/" : request->uri), block);
					// Если запрос поднимает туннель расширенным методом CONNECT
					if(extended)
						// Кодируем псевдо-заголовок [:protocol] (RFC 8441 §4)
						this->_encoder.encode(":protocol", request->protocol, block);
				} break;
				// Если провайдер является ответом сервера
				case static_cast <uint8_t> (direct_t::RESPONSE): {
					// Получаем объект провайдера ответа сервера
					const response_t * response = static_cast <const response_t *> (provider);
					// Формируем статус-код ответа сервера (SSO - без выделения памяти)
					const string code = ::to_string(response->code);
					// Кодируем псевдо-заголовок [:status]
					this->_encoder.encode(":status", code, block);
				} break;
			}
		}
		// Переиспользуемый буфер для названий заголовков в нижнем регистре
		string buffer = "";
		/**
		 * Выполняем перебор всех заголовков контейнера
		 */
		for(const headers_t::header_t & header : headers){
			// Приводим название заголовка к нижнему регистру (RFC 9113 §8.2.1)
			const string_view name = ::lowerName(header.name, buffer);
			// Пропускаем Host (конвертирован в [:authority]) и запрещённые connection-specific заголовки (RFC 9113 §8.2.2)
			if((name == "host") || (name == "connection") || (name == "keep-alive") ||
			   (name == "proxy-connection") || (name == "transfer-encoding") || (name == "upgrade"))
				// Переходим к следующему заголовку
				continue;
			// Заголовок TE допустим только со значением "trailers" (RFC 9113 §8.2.2)
			if((name == "te") && (header.value != "trailers"))
				// Переходим к следующему заголовку
				continue;
			// Кодируем заголовок напрямую из контейнера (без копий)
			this->_encoder.encode(name, header.value, block);
		}
		// Отправляем HPACK-блок заголовков потока
		this->commitHeaders(sid, block, endStream);
		// Если провайдер контейнера задаёт запрос методом HEAD
		if((provider != nullptr) && (provider->direct == direct_t::REQUEST) &&
		   (static_cast <const request_t *> (provider)->method == method_t::HEAD)){
			// Выполняем поиск потока
			stream_t * stream = this->findStream(sid);
			// Если поток найден
			if(stream != nullptr)
				// Помечаем что ответ на этот поток тела не несёт (RFC 9110 §9.3.2)
				stream->bodyless = true;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(sid, headers.size(), endStream, scheme), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Передаём исходящие байты сетевому слою
	this->flush();
}
/**
 * @brief Метод выделения идентификатора для нового инициируемого нами потока
 *
 * @details Клиент получает нечётные идентификаторы (1, 3, 5...), выделенный
 *          идентификатор передаётся в sendHeaders() для открытия потока.
 *
 * @return идентификатор нового потока
 *
 */
uint32_t awh::http::Parser_HTTP2::nextStreamId() noexcept {
	// Запоминаем выделяемый идентификатор потока
	const uint32_t result = this->_transfer.nextStreamId;
	/**
	 * Пространство идентификаторов исчерпано (RFC 9113 §5.1.1): новые потоки на этом
	 * соединении невозможны, требуется установить новое. Сообщаем об этом пиру
	 */
	if(result > h2::proto::STREAM_ID_MASK){
		// Если GOAWAY ещё не отправлен
		if(!this->_flags.goawaySent)
			// Помечаем соединение завершаемым
			this->sendGoaway(error_t::NO_ERROR, "stream identifiers exhausted");
		// Выделить идентификатор невозможно
		return 0;
	}
	// Смещаем следующий инициируемый нами идентификатор потока
	this->_transfer.nextStreamId += 2;
	// Выводим выделенный идентификатор потока
	return result;
}
/**
 * @brief Метод назначения pull-источника данных тела потока
 *
 * @param sid    идентификатор потока
 * @param source pull-источник данных тела
 *
 */
void awh::http::Parser_HTTP2::dataSource(const uint32_t sid, data_source_callback_t source) noexcept {
	// Выполняем поиск потока
	stream_t * stream = this->findStream(sid);
	// Если поток не найден - назначать источник данных некому
	if(stream == nullptr)
		// Выходим из метода
		return;
	// Устанавливаем pull-источник данных тела потока
	stream->source = ::move(source);
	// Сбрасываем флаг достижения конца тела источника
	stream->sourceEof = false;
	// Ставим поток в очередь готовых к отправке
	this->markReady(* stream);
	// Прокачиваем отправку по всем потокам
	this->pump();
	// Передаём исходящие байты сетевому слою
	this->flush();
}
/**
 * @brief Метод настройки порога выходного буфера соединения (backpressure от TCP-стадии)
 *
 * @param high порог выходного буфера соединения
 *
 */
void awh::http::Parser_HTTP2::outputHighWater(const size_t high) noexcept {
	// Устанавливаем порог выходного буфера соединения
	this->_transfer.outputHighWater = high;
}
/**
 * @brief Метод настройки порогов буфера отправки потока
 *
 * @param high ёмкость буфера отправки потока (high-water)
 * @param low  порог сигнала writable (low-water)
 *
 */
void awh::http::Parser_HTTP2::sendWaterMarks(const size_t high, const size_t low) noexcept {
	// Устанавливаем порог сигнала writable
	this->_transfer.sendLowWater = low;
	// Устанавливаем ёмкость буфера отправки потока
	this->_transfer.sendHighWater = high;
}
/**
 * @brief Метод сообщения текущего монотонного времени для пополнения rate-лимитов
 *
 * @details Вызывайте периодически (например, перед parse); необязательно -
 *          без обновления времени работает только стартовый запас burst.
 *
 * @param seconds текущее монотонное время (секунды)
 *
 */
void awh::http::Parser_HTTP2::updateTime(const uint64_t seconds) noexcept {
	// Устанавливаем текущее время rate-лимитов
	this->_ratelims.now = seconds;
}
/**
 * @brief Метод увеличения приёмного окна соединения
 *
 * @details По умолчанию окно соединения 65535 байт - узкое место при высокой
 *          пропускной способности. Поднимает целевой размер окна приёма и сразу
 *          отправляет WINDOW_UPDATE(0) на разницу. Только увеличение.
 *
 * @param size новый целевой размер окна приёма соединения
 *
 */
void awh::http::Parser_HTTP2::connectionReceiveWindow(const int32_t size) noexcept {
	// Допускается только увеличение окна
	if(size <= this->_window.localMax)
		// Выходим из метода
		return;
	// Вычисляем дельту увеличения окна
	const int32_t delta = (size - this->_window.localMax);
	// Устанавливаем новый целевой размер окна приёма соединения
	this->_window.localMax = size;
	// Отправляем WINDOW_UPDATE для окна соединения
	h2::frame::serialize::windowUpdate(this->_buffer.output, 0, static_cast <uint32_t> (delta));
	// Увеличиваем текущее окно приёма соединения
	this->_window.local += delta;
	// Передаём исходящие байты сетевому слою
	this->flush();
}
/**
 * @brief Метод получения ещё не отправленных исходящих байтов (pull-модель)
 *
 * @details View действителен до следующего вызова любого метода парсера.
 *          После записи в сокет освободите отправленную часть методом
 *          consumePending(). При установленной функции обратного вызова
 *          записи буфер опустошается автоматически.
 *
 * @return ещё не отправленные исходящие байты (zero-copy view во внутренний буфер)
 *
 */
string_view awh::http::Parser_HTTP2::pending() const noexcept {
	// Выводим ещё не отправленные исходящие байты
	return string_view(this->_buffer.output.data() + this->_buffer.outputPos, this->_buffer.output.size() - this->_buffer.outputPos);
}
/**
 * @brief Метод освобождения отправленных байтов из исходящего буфера (амортизированно O(1))
 *
 * @param size число отправленных байт
 *
 */
void awh::http::Parser_HTTP2::consumePending(const size_t size) noexcept {
	// Сдвигаем отданный префикс вместо удаления; физическую память освобождаем амортизированно
	this->_buffer.outputPos += ::min(size, this->outputPending());
	// Если весь буфер исходящих байтов отдан
	if(this->_buffer.outputPos >= this->_buffer.output.size()){
		// Очищаем буфер исходящих байтов
		this->_buffer.output.clear();
		// Сбрасываем отданный префикс
		this->_buffer.outputPos = 0;
	// Если отданный префикс не меньше остатка - компактифицируем буфер
	} else if(this->_buffer.outputPos >= (this->_buffer.output.size() - this->_buffer.outputPos)) {
		// Удаляем отданный префикс из буфера
		this->_buffer.output.erase(0, this->_buffer.outputPos);
		// Сбрасываем отданный префикс
		this->_buffer.outputPos = 0;
	}
	// Выходной буфер просел - возможно, освободилось место под отложенные данные
	if(this->outputPending() < this->_transfer.outputHighWater)
		// Прокачиваем отправку по всем потокам
		this->pump();
}
/**
 * @brief Метод установки функции обратного вызова для обработки анонса server push
 *
 * @param callback функция обратного вызова для обработки анонса server push
 *
 */
void awh::http::Parser_HTTP2::on(push_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.push = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки фрагмента тела потока
 *
 * @param callback функция обратного вызова для обработки фрагмента тела потока
 *
 */
void awh::http::Parser_HTTP2::on(data_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.data = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки закрытия потока
 *
 * @param callback функция обратного вызова для обработки закрытия потока
 *
 */
void awh::http::Parser_HTTP2::on(close_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.close = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки ошибки уровня соединения
 *
 * @param callback функция обратного вызова для обработки ошибки уровня соединения
 *
 */
void awh::http::Parser_HTTP2::on(error_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.error = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова записи исходящих байтов в сеть
 *
 * @param callback функция обратного вызова записи исходящих байтов в сеть
 *
 */
void awh::http::Parser_HTTP2::on(write_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.write = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки открытия нового потока
 *
 * @param callback функция обратного вызова для обработки открытия нового потока
 *
 */
void awh::http::Parser_HTTP2::on(begin_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.begin = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки фазы приёма сообщения потока
 *
 * @param callback функция обратного вызова для обработки фазы приёма сообщения потока
 *
 */
void awh::http::Parser_HTTP2::on(phase_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.phase = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки полученного GOAWAY
 *
 * @param callback функция обратного вызова для обработки полученного GOAWAY
 *
 */
void awh::http::Parser_HTTP2::on(goaway_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.goaway = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки заголовков или трейлеров потока
 *
 * @param callback функция обратного вызова для обработки заголовков или трейлеров потока
 *
 */
void awh::http::Parser_HTTP2::on(header_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.header = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова о готовности потока принимать данные тела
 *
 * @param callback функция обратного вызова о готовности потока принимать данные тела
 *
 */
void awh::http::Parser_HTTP2::on(writable_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.writable = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки применённого SETTINGS пира
 *
 * @param callback функция обратного вызова для обработки применённого SETTINGS пира
 *
 */
void awh::http::Parser_HTTP2::on(settings_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.settings = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки провайдера заголовков потока
 *
 * @param callback функция обратного вызова для обработки провайдера заголовков потока
 *
 */
void awh::http::Parser_HTTP2::on(provider_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.provider = ::move(callback);
}
/**
 * @brief Конструктор
 *
 * @param direct направление трафика (REQUEST - мы сервер, RESPONSE - мы клиент)
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 *
 */
awh::http::Parser_HTTP2::Parser_HTTP2(const direct_t direct, const fmk_t * fmk, const log_t * log) noexcept :
 parser_t(direct, fmk, log), _epoch(0), _error(error_t::NO_ERROR) {
	// Лимит одновременных потоков пира по умолчанию не задан (RFC 9113 §6.5.2)
	this->_remote.maxConcurrentStreams = 0xFFFFFFFF;
	// Пир не заявлял отказ от приоритетов RFC 7540, пока не прислал параметр
	this->_remote.noRfc7540Priorities = 0;
	// Запоминаем направление трафика
	this->_flags.prefaceReceived = (direct != direct_t::REQUEST);
	// Устанавливаем начальный идентификатор инициируемого нами потока
	this->_transfer.nextStreamId = (direct == direct_t::REQUEST ? 2 : 1);
	// Инициализируем лимит частоты входящих RST_STREAM из лимитов безопасности
	this->_ratelims.rst.init(this->_limits.rstLimitBurst, this->_limits.rstLimitRate);
	// Инициализируем лимит частоты управляющих фреймов из лимитов безопасности
	this->_ratelims.ctrl.init(this->_limits.ctrlLimitBurst, this->_limits.ctrlLimitRate);
	// Инициализируем лимит частоты кадров приоритета из лимитов безопасности
	this->_ratelims.prio.init(this->_limits.prioLimitBurst, this->_limits.prioLimitRate);
}
/**
 * @brief Деструктор
 *
 */
awh::http::Parser_HTTP2::~Parser_HTTP2() noexcept {}
