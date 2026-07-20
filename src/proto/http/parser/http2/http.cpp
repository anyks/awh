/**
 * @file: http.cpp
 * @date: 2026-07-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
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
 * Внутренние вспомогательные функции (внутренняя компоновка)
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Функция получения имени метода запроса для псевдо-заголовка [:method]
	 *
	 * @param request провайдер запроса клиента
	 * @return        имя метода запроса (для UNKNOWN - оригинальное написание метода)
	 */
	string_view methodName(const http::request_t * request) noexcept {
		/**
		 * Определяем метод запроса клиента
		 */
		switch(static_cast <uint8_t> (request->method)){
			/**
			 * Основные методы (RFC 7231) + PATCH (RFC 5789)
			 */
			// Если метод запроса установлен как GET
			case static_cast <uint8_t> (http::method_t::GET):
				// Выводим название метода запроса
				return "GET";
			// Если метод запроса установлен как PUT
			case static_cast <uint8_t> (http::method_t::PUT):
				// Выводим название метода запроса
				return "PUT";
			// Если метод запроса установлен как DELETE
			case static_cast <uint8_t> (http::method_t::DEL):
				// Выводим название метода запроса
				return "DELETE";
			// Если метод запроса установлен как POST
			case static_cast <uint8_t> (http::method_t::POST):
				// Выводим название метода запроса
				return "POST";
			// Если метод запроса установлен как HEAD
			case static_cast <uint8_t> (http::method_t::HEAD):
				// Выводим название метода запроса
				return "HEAD";
			// Если метод запроса установлен как PATCH
			case static_cast <uint8_t> (http::method_t::PATCH):
				// Выводим название метода запроса
				return "PATCH";
			// Если метод запроса установлен как TRACE
			case static_cast <uint8_t> (http::method_t::TRACE):
				// Выводим название метода запроса
				return "TRACE";
			// Если метод запроса установлен как OPTIONS
			case static_cast <uint8_t> (http::method_t::OPTIONS):
				// Выводим название метода запроса
				return "OPTIONS";
			// Если метод запроса установлен как CONNECT
			case static_cast <uint8_t> (http::method_t::CONNECT):
				// Выводим название метода запроса
				return "CONNECT";
			/**
			 * WebDAV (RFC 4918) и расширения версионирования (RFC 3253)
			 */
			// Если метод запроса установлен как ACL
			case static_cast <uint8_t> (http::method_t::ACL):
				// Выводим название метода запроса
				return "ACL";
			// Если метод запроса установлен как COPY
			case static_cast <uint8_t> (http::method_t::COPY):
				// Выводим название метода запроса
				return "COPY";
			// Если метод запроса установлен как LOCK
			case static_cast <uint8_t> (http::method_t::LOCK):
				// Выводим название метода запроса
				return "LOCK";
			// Если метод запроса установлен как MOVE
			case static_cast <uint8_t> (http::method_t::MOVE):
				// Выводим название метода запроса
				return "MOVE";
			// Если метод запроса установлен как BIND
			case static_cast <uint8_t> (http::method_t::BIND):
				// Выводим название метода запроса
				return "BIND";
			// Если метод запроса установлен как MKCOL
			case static_cast <uint8_t> (http::method_t::MKCOL):
				// Выводим название метода запроса
				return "MKCOL";
			// Если метод запроса установлен как MERGE
			case static_cast <uint8_t> (http::method_t::MERGE):
				// Выводим название метода запроса
				return "MERGE";
			// Если метод запроса установлен как REPORT
			case static_cast <uint8_t> (http::method_t::REPORT):
				// Выводим название метода запроса
				return "REPORT";
			// Если метод запроса установлен как SEARCH
			case static_cast <uint8_t> (http::method_t::SEARCH):
				// Выводим название метода запроса
				return "SEARCH";
			// Если метод запроса установлен как UNLOCK
			case static_cast <uint8_t> (http::method_t::UNLOCK):
				// Выводим название метода запроса
				return "UNLOCK";
			// Если метод запроса установлен как REBIND
			case static_cast <uint8_t> (http::method_t::REBIND):
				// Выводим название метода запроса
				return "REBIND";
			// Если метод запроса установлен как UNBIND
			case static_cast <uint8_t> (http::method_t::UNBIND):
				// Выводим название метода запроса
				return "UNBIND";
			// Если метод запроса установлен как CHECKOUT
			case static_cast <uint8_t> (http::method_t::CHECKOUT):
				// Выводим название метода запроса
				return "CHECKOUT";
			// Если метод запроса установлен как PROPFIND
			case static_cast <uint8_t> (http::method_t::PROPFIND):
				// Выводим название метода запроса
				return "PROPFIND";
			// Если метод запроса установлен как PROPPATCH
			case static_cast <uint8_t> (http::method_t::PROPPATCH):
				// Выводим название метода запроса
				return "PROPPATCH";
			// Если метод запроса установлен как MKACTIVITY
			case static_cast <uint8_t> (http::method_t::MKACTIVITY):
				// Выводим название метода запроса
				return "MKACTIVITY";
			/**
			 * Прочие распространённые расширения
			 */
			// Если метод запроса установлен как PRI
			case static_cast <uint8_t> (http::method_t::PRI):
				// Выводим название метода запроса
				return "PRI";
			// Если метод запроса установлен как LINK
			case static_cast <uint8_t> (http::method_t::LINK):
				// Выводим название метода запроса
				return "LINK";
			// Если метод запроса установлен как PURGE
			case static_cast <uint8_t> (http::method_t::PURGE):
				// Выводим название метода запроса
				return "PURGE";
			// Если метод запроса установлен как NOTIFY
			case static_cast <uint8_t> (http::method_t::NOTIFY):
				// Выводим название метода запроса
				return "NOTIFY";
			// Если метод запроса установлен как UNLINK
			case static_cast <uint8_t> (http::method_t::UNLINK):
				// Выводим название метода запроса
				return "UNLINK";
			// Если метод запроса установлен как SOURCE
			case static_cast <uint8_t> (http::method_t::SOURCE):
				// Выводим название метода запроса
				return "SOURCE";
			// Если метод запроса установлен как M-SEARCH
			case static_cast <uint8_t> (http::method_t::MSEARCH):
				// Выводим название метода запроса
				return "M-SEARCH";
			// Если метод запроса установлен как SUBSCRIBE
			case static_cast <uint8_t> (http::method_t::SUBSCRIBE):
				// Выводим название метода запроса
				return "SUBSCRIBE";
			// Если метод запроса установлен как MKCALENDAR
			case static_cast <uint8_t> (http::method_t::MKCALENDAR):
				// Выводим название метода запроса
				return "MKCALENDAR";
			// Если метод запроса установлен как UNSUBSCRIBE
			case static_cast <uint8_t> (http::method_t::UNSUBSCRIBE):
				// Выводим название метода запроса
				return "UNSUBSCRIBE";
			// Нераспознанный метод - используем оригинальное написание
			case static_cast <uint8_t> (http::method_t::UNKNOWN):
				// Выводим оригинальное написание метода запроса
				return request->methodName;
		}
		// Метод запроса не установлен
		return "";
	}
	/**
	 * @brief Функция приведения названия заголовка к нижнему регистру (RFC 9113 §8.2.1)
	 *
	 * @details Если название уже в нижнем регистре - возвращается исходная строка без копии
	 *          (горячий путь), иначе выполняется копия в переиспользуемый буфер.
	 *
	 * @param name   название заголовка
	 * @param buffer переиспользуемый буфер для приведённого названия
	 * @return       название заголовка в нижнем регистре
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
	 * @brief Функция классификации метода запроса по значению псевдо-заголовка [:method]
	 *
	 * @note В отличие от HTTP/1.x сравнение выполняется с учётом регистра:
	 *       методы HTTP - регистрозависимые токены (RFC 9110 §9.1).
	 *
	 * @param method значение псевдо-заголовка [:method]
	 * @return       распознанный метод запроса либо method_t::NONE
	 */
	http::method_t classifyMethod(const string & method) noexcept {
		/**
		 * @brief Структура записи таблицы соответствия имён методов запроса
		 *
		 */
		struct Entry {
			// Имя метода запроса
			const char * name;
			// Распознанный метод запроса
			http::method_t method;
		};
		// Таблица соответствия имён методов запроса (указатели на строковые литералы в .rodata)
		static constexpr Entry methods[] = {
			{"GET", http::method_t::GET},
			{"PUT", http::method_t::PUT},
			{"ACL", http::method_t::ACL},
			{"PRI", http::method_t::PRI},
			{"HEAD", http::method_t::HEAD},
			{"POST", http::method_t::POST},
			{"COPY", http::method_t::COPY},
			{"LOCK", http::method_t::LOCK},
			{"MOVE", http::method_t::MOVE},
			{"BIND", http::method_t::BIND},
			{"LINK", http::method_t::LINK},
			{"TRACE", http::method_t::TRACE},
			{"PATCH", http::method_t::PATCH},
			{"MKCOL", http::method_t::MKCOL},
			{"MERGE", http::method_t::MERGE},
			{"PURGE", http::method_t::PURGE},
			{"DELETE", http::method_t::DEL},
			{"SEARCH", http::method_t::SEARCH},
			{"UNLOCK", http::method_t::UNLOCK},
			{"REBIND", http::method_t::REBIND},
			{"UNBIND", http::method_t::UNBIND},
			{"REPORT", http::method_t::REPORT},
			{"NOTIFY", http::method_t::NOTIFY},
			{"SOURCE", http::method_t::SOURCE},
			{"UNLINK", http::method_t::UNLINK},
			{"CONNECT", http::method_t::CONNECT},
			{"OPTIONS", http::method_t::OPTIONS},
			{"PROPFIND", http::method_t::PROPFIND},
			{"CHECKOUT", http::method_t::CHECKOUT},
			{"M-SEARCH", http::method_t::MSEARCH},
			{"PROPPATCH", http::method_t::PROPPATCH},
			{"SUBSCRIBE", http::method_t::SUBSCRIBE},
			{"MKACTIVITY", http::method_t::MKACTIVITY},
			{"MKCALENDAR", http::method_t::MKCALENDAR},
			{"UNSUBSCRIBE", http::method_t::UNSUBSCRIBE}
		};
		/**
		 * Выполняем перебор таблицы соответствия имён методов запроса
		 */
		for(const auto & item : methods){
			// Если имя метода запроса совпадает - выводим распознанный метод запроса
			if(method.compare(item.name) == 0)
				// Выводим распознанный метод запроса
				return item.method;
		}
		// Метод запроса не распознан
		return http::method_t::NONE;
	}
	/**
	 * @brief Функция проверки того, что имя заголовка приведено к нижнему регистру (RFC 9113 §8.2)
	 *
	 * @param name имя заголовка
	 * @return     результат проверки
	 */
	bool isLowercaseName(const string & name) noexcept {
		// Пустое имя заголовка недопустимо
		if(name.empty())
			// Имя заголовка некорректно
			return false;
		/**
		 * Выполняем перебор всех символов имени заголовка
		 */
		for(const char letter : name){
			// Если найден символ в верхнем регистре - имя заголовка некорректно
			if((letter >= 'A') && (letter <= 'Z'))
				// Имя заголовка некорректно
				return false;
		}
		// Имя заголовка корректно
		return true;
	}
	/**
	 * @brief Функция проверки принадлежности заголовка к запрещённым в HTTP/2 connection-specific заголовкам (RFC 9113 §8.2.2)
	 *
	 * @param name имя заголовка
	 * @param fmk  объект фреймворка
	 * @return     результат проверки
	 */
	bool isConnectionSpecific(const string & name, const fmk_t * fmk) noexcept {
		// Выполняем сравнение со списком запрещённых заголовков
		return (
			fmk->compare("upgrade", name) ||
			fmk->compare("keep-alive", name) ||
			fmk->compare("connection", name) ||
			fmk->compare("proxy-connection", name) ||
			fmk->compare("transfer-encoding", name)
		);
	}
	/**
	 * @brief Функция валидации HTTP-семантики блока заголовков (RFC 9113 §8.1-8.3)
	 *
	 * @param fields     декодированные заголовки блока
	 * @param isRequest  блок принадлежит запросу клиента (true) или ответу сервера (false)
	 * @param isTrailers блок является трейлерами
	 * @param fmk        объект фреймворка
	 * @return           код ошибки протокола (NO_ERROR - блок корректен, PROTOCOL_ERROR - malformed)
	 */
	http::h2::error_t validateHeaders(const vector <http::h2::hpack::field_t> & fields, const bool isRequest, const bool isTrailers, const fmk_t * fmk) noexcept {
		// Значение псевдо-заголовка [:method]
		string method{""};
		// Флаг наличия обычного (не псевдо) заголовка
		bool seenRegular = false;
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
			const string & name = field.name;
			// Пустое имя заголовка недопустимо
			if(name.empty())
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
					if(fmk->compare(":method", name)){
						// Повторный псевдо-заголовок недопустим
						if(hasMethod)
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
						// Помечаем что псевдо-заголовок получен
						hasMethod = true;
						// Запоминаем значение метода запроса
						method = field.value;
					// Если получен псевдо-заголовок [:scheme]
					} else if(fmk->compare(":scheme", name)) {
						// Повторный псевдо-заголовок недопустим
						if(hasScheme)
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
						// Помечаем что псевдо-заголовок получен
						hasScheme = true;
					// Если получен псевдо-заголовок [:path]
					} else if(fmk->compare(":path", name)) {
						// Повторный псевдо-заголовок недопустим
						if(hasPath)
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
						// Помечаем что псевдо-заголовок получен
						hasPath = true;
						// Пустое значение [:path] недопустимо (RFC 9113 §8.3.1)
						if(field.value.empty())
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
					// Если получен псевдо-заголовок [:authority]
					} else if(fmk->compare(":authority", name)) {
						// Повторный псевдо-заголовок недопустим
						if(hasAuthority)
							// Блок заголовков некорректен
							return http::h2::error_t::PROTOCOL_ERROR;
						// Помечаем что псевдо-заголовок получен
						hasAuthority = true;
					// Неизвестный/неуместный псевдо-заголовок недопустим
					} else return http::h2::error_t::PROTOCOL_ERROR;
				// Если блок принадлежит ответу сервера
				} else {
					// Если получен псевдо-заголовок [:status]
					if(fmk->compare(":status", name)){
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
				// Имя заголовка обязано быть в нижнем регистре (RFC 9113 §8.2)
				if(!::isLowercaseName(name))
					// Блок заголовков некорректен
					return http::h2::error_t::PROTOCOL_ERROR;
				// Connection-specific заголовки запрещены в HTTP/2 (RFC 9113 §8.2.2)
				if(::isConnectionSpecific(name, fmk))
					// Блок заголовков некорректен
					return http::h2::error_t::PROTOCOL_ERROR;
				// Заголовок [te] допускает только значение [trailers] (RFC 9113 §8.2.2)
				if(fmk->compare("te", name) && !fmk->compare("trailers", field.value))
					// Блок заголовков некорректен
					return http::h2::error_t::PROTOCOL_ERROR;
			}
		}
		// В трейлерах обязательных псевдо-заголовков нет
		if(isTrailers)
			// Блок заголовков корректен
			return http::h2::error_t::NO_ERROR;
		// Если блок принадлежит запросу клиента
		if(isRequest){
			// Если запрос выполняется методом CONNECT
			if(fmk->compare("CONNECT", method)){
				// Метод CONNECT требует наличия [:authority] (RFC 9113 §8.5)
				if(!hasAuthority)
					// Блок заголовков некорректен
					return http::h2::error_t::PROTOCOL_ERROR;
				// И запрещает [:scheme]/[:path]
				if(hasScheme || hasPath)
					// Блок заголовков некорректен
					return http::h2::error_t::PROTOCOL_ERROR;
			// Для остальных методов обязательны [:method]/[:scheme]/[:path]
			} else if(!hasMethod || !hasScheme || !hasPath)
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
 maxConcurrentStreams(h2::proto::MAX_COUNT_STREAMS) {}

/**
 * @brief Метод списания токенов
 *
 * @param value число списываемых токенов
 * @return      результат списания (false - токенов не хватает, превышение лимита)
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
	// Пополняем токены пропорционально прошедшему времени
	this->value += (this->rate * seconds);
	// Ограничиваем количество токенов стартовым запасом
	if(this->value > this->burst)
		// Устанавливаем предельное количество токенов
		this->value = this->burst;
}
/**
 * @brief Метод инициализации лимита
 *
 * @param burst стартовый запас токенов
 * @param rate  пополнение токенов в секунду
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
 */
size_t awh::http::Parser_HTTP2::Stream::pending() const noexcept {
	// Выводим объём буфера отправки без уже отправленного префикса
	return (this->sendBuffer.size() - this->sendOffset);
}
/**
 * @brief Метод снятия учтённого префикса буфера отправки
 *
 * @details Очистка при полном расходе, иначе амортизированная компактификация
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
 writableNotified(false), recvBody(0),
 localWindow(h2::proto::DEFAULT_WINDOW_SIZE),
 remoteWindow(h2::proto::DEFAULT_WINDOW_SIZE),
 sendOffset(0), sendBuffer{""},
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
 localMax(h2::proto::DEFAULT_WINDOW_SIZE) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP2::Buffer::Buffer() noexcept :
 input{""}, output{""}, outputPos(0) {}

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
 goawaySent(false), hbcRefused(false),
 hbcEndStream(false), settingsAcked(true),
 goawayReceived(false), prefaceReceived(false) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP2::Transfer::Transfer() noexcept :
 lastStreamId(0),
 nextStreamId(1),
 peerStreamCount(0),
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
 */
size_t awh::http::Parser_HTTP2::outputPending() const noexcept {
	// Выводим объём буфера исходящих байтов без уже отданного префикса
	return (this->_buffer.output.size() - this->_buffer.outputPos);
}
/**
 * @brief Метод разбора накопленного входного буфера (preface + поток фреймов)
 *
 * @return результат разбора (OK/ERROR)
 */
awh::http::h2::status_t awh::http::Parser_HTTP2::parseInput() noexcept {
	// Позиция разбора во входном буфере
	size_t pos = 0;
	// Если connection preface ещё не получен (мы - сервер)
	if(!this->_flags.prefaceReceived){
		// Если preface целиком ещё не пришёл - ждём больше данных
		if(this->_buffer.input.size() < h2::proto::PREFACE.size())
			// Продолжаем ожидание данных
			return h2::status_t::OK;
		// Если полученные байты не совпадают с magic-строкой preface
		if(string_view(this->_buffer.input.data(), h2::proto::PREFACE.size()) != h2::proto::PREFACE){
			// Очищаем входной буфер
			this->_buffer.input.clear();
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
			this->_buffer.input.clear();
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
			this->_buffer.input.clear();
			// Фиксируем ошибку уровня соединения
			return this->fail(error_t::PROTOCOL_ERROR, "expected CONTINUATION");
		}
		// Выполняем обработку полного фрейма
		const h2::status_t status = this->dispatch(header, payload);
		// Если обработка фрейма завершилась ошибкой уровня соединения
		if(status == h2::status_t::ERROR){
			// Очищаем входной буфер
			this->_buffer.input.clear();
			// Прерываем разбор с ошибкой
			return h2::status_t::ERROR;
		}
		// Пропускаем разобранный фрейм
		pos += (h2::proto::FRAME_HEADER_SIZE + header.length);
	}
	// Убираем разобранный префикс, оставляя неполный хвост во входном буфере
	if(pos > 0)
		// Удаляем разобранный префикс входного буфера
		this->_buffer.input.erase(0, pos);
	// Разбор выполнен успешно
	return h2::status_t::OK;
}
/**
 * @brief Метод декодирования накопленного блока заголовков и вызова функций обратного вызова
 *
 * @return результат обработки (OK/ERROR)
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
	// Декодированные заголовки блока
	vector <h2::hpack::field_t> fields;
	// Лимит суммарного размера распакованного списка заголовков (защита от decompression bomb)
	uint64_t listLimit = this->_limits.maxHeadersTotal;
	// Если лимит SETTINGS_MAX_HEADER_LIST_SIZE задан и строже - применяем его
	if((this->_local.maxHeaderListSize != 0) && (this->_local.maxHeaderListSize < listLimit))
		// Применяем лимит из наших параметров SETTINGS
		listLimit = this->_local.maxHeaderListSize;
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
	const error_t vErr = ::validateHeaders(fields, isRequest, isTrailers, this->_fmk);
	// Если блок заголовков малформирован
	if(vErr != error_t::NO_ERROR){
		// Малформированный запрос/ответ - потоковая ошибка (RFC 9113 §8.1.1), соединение живёт
		h2::frame::serialize::rstStream(this->_buffer.output, streamId, vErr);
		// Закрываем поток с вызовом функции обратного вызова закрытия
		this->closeStream(streamId, vErr);
		// Обработка блока завершена
		return h2::status_t::OK;
	}
	// Если декодированные заголовки превышают лимиты безопасности
	if(!this->checkHeaderLimits(fields)){
		// Превышение лимитов - потоковая ошибка, соединение живёт
		h2::frame::serialize::rstStream(this->_buffer.output, streamId, error_t::ENHANCE_YOUR_CALM);
		// Закрываем поток с вызовом функции обратного вызова закрытия
		this->closeStream(streamId, error_t::ENHANCE_YOUR_CALM);
		// Обработка блока завершена
		return h2::status_t::OK;
	}
	// Если это первый блок заголовков потока - собираем провайдер из псевдо-заголовков
	if(!isTrailers){
		// Выполняем построение провайдера заголовков потока
		stream->headers = this->buildProvider(fields, isRequest);
		// Уведомляем о начале приёма сообщения потока
		if(!this->firePhase(streamId, phase_t::BEGIN, part_t::NONE))
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
		// Определяем часть сообщения, к которой относятся заголовки
		const part_t part = (isTrailers ? part_t::TRAILER : part_t::HEADERS);
		/**
		 * Выполняем доставку всех декодированных заголовков блока
		 */
		for(const h2::hpack::field_t & field : fields){
			// Если функция обратного вызова потребовала сбросить поток
			if(!this->_callbacks.header(streamId, string_view(field.name), string_view(field.value), part)){
				// Если поток ещё существует (функция обратного вызова могла его закрыть)
				if(this->findStream(streamId) != nullptr){
					// Сбрасываем поток с кодом CANCEL
					h2::frame::serialize::rstStream(this->_buffer.output, streamId, error_t::CANCEL);
					// Закрываем поток с вызовом функции обратного вызова закрытия
					this->closeStream(streamId, error_t::CANCEL);
				}
				// Обработка блока завершена (соединение живёт)
				return h2::status_t::OK;
			}
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
	// Помечаем что блок заголовков потока получен (повторный HEADERS = трейлеры)
	stream->headersDone = true;
	// Если функция обратного вызова установлена
	if(this->_callbacks.provider != nullptr){
		// Если функция обратного вызова потребовала сбросить поток
		if(!this->_callbacks.provider(streamId, (isTrailers ? nullptr : stream->headers.get()), endStream)){
			// Если поток ещё существует (функция обратного вызова могла его закрыть)
			if(this->findStream(streamId) != nullptr){
				// Сбрасываем поток с кодом CANCEL
				h2::frame::serialize::rstStream(this->_buffer.output, streamId, error_t::CANCEL);
				// Закрываем поток с вызовом функции обратного вызова закрытия
				this->closeStream(streamId, error_t::CANCEL);
			}
			// Обработка блока завершена (соединение живёт)
			return h2::status_t::OK;
		}
	}
	// Если это первый блок заголовков потока
	if(!isTrailers){
		// Уведомляем о завершении приёма блока заголовков потока
		if(!this->firePhase(streamId, phase_t::END, part_t::HEADERS))
			// Обработка блока завершена (поток сброшен, соединение живёт)
			return h2::status_t::OK;
		// Если END_STREAM не получен - за заголовками последует тело
		if(!endStream){
			// Уведомляем о начале приёма тела потока
			if(!this->firePhase(streamId, phase_t::BEGIN, part_t::BODY))
				// Обработка блока завершена (поток сброшен, соединение живёт)
				return h2::status_t::OK;
		}
	// Если это блок трейлеров
	} else {
		// Уведомляем о завершении приёма трейлеров потока
		if(!this->firePhase(streamId, phase_t::END, part_t::TRAILER))
			// Обработка блока завершена (поток сброшен, соединение живёт)
			return h2::status_t::OK;
	}
	// Если получен END_STREAM - сообщение потока полностью принято
	if(endStream){
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
 */
awh::http::h2::status_t awh::http::Parser_HTTP2::dispatch(const h2::frame::header_t & header, const uint8_t * payload) noexcept {
	// Код ошибки протокола
	error_t err = error_t::NO_ERROR;
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
			// Если получен ACK на наш SETTINGS, помечаем что наш SETTINGS подтверждён
			if((this->_flags.settingsAcked = (header.flags & h2::flag::ACK)))
				// Обработка фрейма завершена
				return h2::status_t::OK;
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
				}
			}
			// Подтверждаем получение SETTINGS пира (ACK)
			h2::frame::serialize::settings(this->_buffer.output, nullptr, 0, true);
			// Если функция обратного вызова установлена - уведомляем о применённом SETTINGS пира
			if(this->_callbacks.settings != nullptr)
				// Уведомляем о применённом SETTINGS пира
				this->_callbacks.settings();
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
			// Если получен PING без флага ACK - требуется ответ
			if((header.flags & h2::flag::ACK) == 0){
				// Пополняем лимит частоты управляющих фреймов по текущему времени
				this->_ratelims.ctrl.update(this->_ratelims.now);
				// Если лимит частоты управляющих фреймов превышен (каждый PING требует ответа - усиление)
				if(!this->_ratelims.ctrl.drain(1))
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::ENHANCE_YOUR_CALM, "PING flood");
				// Отвечаем фреймом PING с флагом ACK
				h2::frame::serialize::ping(this->_buffer.output, opaque, true);
			}
			// Обработка фрейма завершена
			return h2::status_t::OK;
		}
		// Фрейм обновления окна flow control (RFC 9113 §6.9)
		case h2::frame_t::WINDOW_UPDATE: {
			// Инкремент окна flow control
			uint32_t increment = 0;
			// Если разбор полезной нагрузки завершился ошибкой
			if(h2::frame::parser::windowUpdate(header, payload, increment, err) != h2::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(err, "bad WINDOW_UPDATE");
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
				if((stream == nullptr) && (header.streamId > this->_transfer.lastStreamId))
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::PROTOCOL_ERROR, "WINDOW_UPDATE on idle stream");
				// Если поток найден
				if(stream != nullptr){
					// Если новое окно превышает максимально допустимое
					if((static_cast <int64_t> (stream->remoteWindow) + increment) > h2::proto::MAX_WINDOW_SIZE)
						// Фиксируем ошибку уровня соединения
						return this->fail(error_t::FLOW_CONTROL_ERROR, "stream window overflow");
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
			// Если функция обратного вызова установлена - уведомляем о полученном GOAWAY
			if(this->_callbacks.goaway != nullptr)
				// Уведомляем о полученном GOAWAY
				this->_callbacks.goaway(goaway.lastStreamId, goaway.code, goaway.debugData);
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
			if((this->findStream(header.streamId) == nullptr) && (header.streamId > this->_transfer.lastStreamId))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::PROTOCOL_ERROR, "RST_STREAM on idle stream");
			// Пополняем лимит частоты входящих RST_STREAM по текущему времени
			this->_ratelims.rst.update(this->_ratelims.now);
			// Если лимит частоты входящих RST_STREAM превышен (защита от Rapid Reset, CVE-2023-44487)
			if(!this->_ratelims.rst.drain(1))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::ENHANCE_YOUR_CALM, "RST_STREAM flood (Rapid Reset)");
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
			// Флаг отклонённого потока (блок декодируем только для синхронизации HPACK)
			bool refused = false;
			// Выполняем поиск потока
			stream_t * stream = this->findStream(header.streamId);
			// Если поток ещё не существует - пир открывает новый поток
			if(stream == nullptr){
				// Проверяем чётность и монотонность идентификатора нового потока
				if(this->validateNewStream(header.streamId, err) != h2::status_t::OK)
					// Фиксируем ошибку уровня соединения
					return this->fail(err, "invalid new stream id");
				// Запоминаем наибольший принятый идентификатор потока
				this->_transfer.lastStreamId = header.streamId;
				/**
				 * Отклоняем новый поток, если: исчерпан лимит одновременных потоков
				 * (RFC 9113 §5.1.2) либо мы уже отправили GOAWAY (§6.8) - новые потоки
				 * пира после этого не обслуживаются. Это потоковая ошибка REFUSED_STREAM,
				 * соединение остаётся живым; блок заголовков всё равно декодируем
				 */
				if(this->_flags.goawaySent || (this->_transfer.peerStreamCount >= this->_local.maxConcurrentStreams)){
					// Отклоняем поток с кодом REFUSED_STREAM
					h2::frame::serialize::rstStream(this->_buffer.output, header.streamId, error_t::REFUSED_STREAM);
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
					// Если функция обратного вызова потребовала отклонить поток
					if((this->_callbacks.begin != nullptr) && !this->_callbacks.begin(header.streamId)){
						// Сбрасываем поток с кодом CANCEL
						h2::frame::serialize::rstStream(this->_buffer.output, header.streamId, error_t::CANCEL);
						// Закрываем поток с вызовом функции обратного вызова закрытия
						this->closeStream(header.streamId, error_t::CANCEL);
						// Помечаем что поток отклонён
						refused = true;
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
						// Повторный HEADERS - это трейлеры, они обязаны нести END_STREAM (RFC 9113 §8.1)
						if(stream->headersDone && !headers.endStream)
							// Фиксируем ошибку уровня соединения
							return this->fail(error_t::PROTOCOL_ERROR, "trailers without END_STREAM");
					} break;
					// Половина пира закрыта либо поток закрыт
					case h2::stream_state_t::HALF_CLOSED_REMOTE:
					case h2::stream_state_t::CLOSED:
						// Фиксируем ошибку уровня соединения
						return this->fail(error_t::STREAM_CLOSED, "HEADERS on closed stream");
					// Остальные состояния для HEADERS недопустимы
					default:
						// Фиксируем ошибку уровня соединения
						return this->fail(error_t::PROTOCOL_ERROR, "HEADERS in invalid stream state");
				}
			}
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
			// Выполняем поиск потока
			stream_t * stream = this->findStream(header.streamId);
			// Если поток не найден
			if(stream == nullptr){
				// DATA на ещё не открытом (idle) потоке - ошибка соединения (RFC 9113 §5.1)
				if(header.streamId > this->_transfer.lastStreamId)
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::PROTOCOL_ERROR, "DATA on idle stream");
				// DATA на уже закрытом и удалённом потоке - ошибка соединения
				return this->fail(error_t::STREAM_CLOSED, "DATA on closed stream");
			}
			// Данные принимаем только в состояниях OPEN и HALF_CLOSED_LOCAL (RFC 9113 §5.1)
			if((stream->state != h2::stream_state_t::OPEN) && (stream->state != h2::stream_state_t::HALF_CLOSED_LOCAL))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::STREAM_CLOSED, "DATA in non-open stream state");
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
			/**
			 * Flow control приёма: учитывается ПОЛНАЯ длина полезной нагрузки, включая
			 * padding (RFC 9113 §6.9.1). Проверяем окна и соединения, и потока
			 */
			if(static_cast <int32_t> (header.length) > this->_window.local)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::FLOW_CONTROL_ERROR, "connection receive window exhausted");
			// Если окно приёма потока исчерпано
			if(static_cast <int32_t> (header.length) > stream->localWindow)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::FLOW_CONTROL_ERROR, "stream receive window exhausted");
			// Учитываем принятые данные в суммарном размере тела потока
			stream->recvBody += data.data.size();
			// Если суммарный размер тела потока превысил лимит безопасности
			if(stream->recvBody > this->_limits.maxBodySize){
				// Пополняем окно приёма соединения (поток закрывается)
				this->replenishReceiveWindow(nullptr, header.length);
				// Сбрасываем поток с кодом ENHANCE_YOUR_CALM
				h2::frame::serialize::rstStream(this->_buffer.output, header.streamId, error_t::ENHANCE_YOUR_CALM);
				// Закрываем поток с вызовом функции обратного вызова закрытия
				this->closeStream(header.streamId, error_t::ENHANCE_YOUR_CALM);
				// Обработка фрейма завершена (соединение живёт)
				return h2::status_t::OK;
			}
			// Запоминаем флаг END_STREAM (поток может быть удалён функцией обратного вызова)
			const bool endStream = data.endStream;
			// Если функция обратного вызова установлена
			if(this->_callbacks.data != nullptr){
				// Если функция обратного вызова потребовала сбросить поток
				if(!this->_callbacks.data(header.streamId, data.data.data(), data.data.size(), endStream)){
					// Пополняем окно приёма соединения (поток закрывается)
					this->replenishReceiveWindow(nullptr, header.length);
					// Если поток ещё существует (функция обратного вызова могла его закрыть)
					if(this->findStream(header.streamId) != nullptr){
						// Сбрасываем поток с кодом CANCEL
						h2::frame::serialize::rstStream(this->_buffer.output, header.streamId, error_t::CANCEL);
						// Закрываем поток с вызовом функции обратного вызова закрытия
						this->closeStream(header.streamId, error_t::CANCEL);
					}
					// Обработка фрейма завершена (соединение живёт)
					return h2::status_t::OK;
				}
			}
			// Перечитываем указатель на поток (функция обратного вызова могла его удалить)
			stream = this->findStream(header.streamId);
			// Пополняем окно приёма и при просадке шлём WINDOW_UPDATE (потоку - только если он остаётся открыт)
			this->replenishReceiveWindow(((endStream || (stream == nullptr)) ? nullptr : stream), header.length);
			// Если получен END_STREAM и поток ещё существует
			if(endStream && (stream != nullptr)){
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
			// Разобранная полезная нагрузка PRIORITY
			h2::frame::priority_t priority;
			// Если разбор полезной нагрузки завершился ошибкой
			if(h2::frame::parser::priority(header, payload, priority, err) != h2::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(err, "bad PRIORITY");
			// Приоритеты RFC 7540 deprecated - игнорируем
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
			// Если ассоциированный поток не существует либо уже закрыт
			if((assoc == nullptr) || (assoc->state == h2::stream_state_t::CLOSED))
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
				h2::frame::serialize::rstStream(this->_buffer.output, promise.promisedStreamId, error_t::REFUSED_STREAM);
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
 */
awh::http::h2::status_t awh::http::Parser_HTTP2::deliverPushPromise(const uint32_t sid, const uint32_t promisedSid, vector <h2::hpack::field_t> & fields) noexcept {
	// Выполняем поиск обещанного потока
	stream_t * stream = this->findStream(promisedSid);
	// Если обещанный поток исчез - внутренняя ошибка
	if(stream == nullptr)
		// Фиксируем ошибку уровня соединения
		return this->fail(error_t::INTERNAL_ERROR, "promised stream vanished");
	// Обещанный блок - это всегда запрос (псевдо-заголовки запроса), без трейлеров
	const error_t vErr = ::validateHeaders(fields, true, false, this->_fmk);
	// Если блок заголовков малформирован
	if(vErr != error_t::NO_ERROR){
		// Малформированный обещанный запрос - потоковая ошибка, соединение живёт
		h2::frame::serialize::rstStream(this->_buffer.output, promisedSid, vErr);
		// Закрываем обещанный поток с вызовом функции обратного вызова закрытия
		this->closeStream(promisedSid, vErr);
		// Обработка блока завершена
		return h2::status_t::OK;
	}
	// Если декодированные заголовки превышают лимиты безопасности
	if(!this->checkHeaderLimits(fields)){
		// Превышение лимитов - потоковая ошибка, соединение живёт
		h2::frame::serialize::rstStream(this->_buffer.output, promisedSid, error_t::ENHANCE_YOUR_CALM);
		// Закрываем обещанный поток с вызовом функции обратного вызова закрытия
		this->closeStream(promisedSid, error_t::ENHANCE_YOUR_CALM);
		// Обработка блока завершена
		return h2::status_t::OK;
	}
	// Если функция обратного вызова установлена
	if(this->_callbacks.push != nullptr){
		// Если функция обратного вызова потребовала отклонить push
		if(!this->_callbacks.push(sid, promisedSid)){
			// Если обещанный поток ещё существует (функция обратного вызова могла его закрыть)
			if(this->findStream(promisedSid) != nullptr){
				// Отклоняем push с кодом CANCEL
				h2::frame::serialize::rstStream(this->_buffer.output, promisedSid, error_t::CANCEL);
				// Закрываем обещанный поток с вызовом функции обратного вызова закрытия
				this->closeStream(promisedSid, error_t::CANCEL);
			}
			// Обработка блока завершена (соединение живёт)
			return h2::status_t::OK;
		}
	}
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
		for(const h2::hpack::field_t & field : fields){
			// Если функция обратного вызова потребовала сбросить поток
			if(!this->_callbacks.header(promisedSid, string_view(field.name), string_view(field.value), part_t::HEADERS)){
				// Если обещанный поток ещё существует (функция обратного вызова могла его закрыть)
				if(this->findStream(promisedSid) != nullptr){
					// Сбрасываем поток с кодом CANCEL
					h2::frame::serialize::rstStream(this->_buffer.output, promisedSid, error_t::CANCEL);
					// Закрываем обещанный поток с вызовом функции обратного вызова закрытия
					this->closeStream(promisedSid, error_t::CANCEL);
				}
				// Обработка блока завершена (соединение живёт)
				return h2::status_t::OK;
			}
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
	if(this->_callbacks.provider != nullptr){
		// Если функция обратного вызова потребовала сбросить поток
		if(!this->_callbacks.provider(promisedSid, stream->headers.get(), false)){
			// Если обещанный поток ещё существует (функция обратного вызова могла его закрыть)
			if(this->findStream(promisedSid) != nullptr){
				// Сбрасываем поток с кодом CANCEL
				h2::frame::serialize::rstStream(this->_buffer.output, promisedSid, error_t::CANCEL);
				// Закрываем обещанный поток с вызовом функции обратного вызова закрытия
				this->closeStream(promisedSid, error_t::CANCEL);
			}
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
 */
awh::http::Parser_HTTP2::stream_t & awh::http::Parser_HTTP2::stream(const uint32_t id) noexcept {
	// Получаем существующий либо создаём новый объект потока
	stream_t & result = this->_transfer.streams[id];
	// Если поток создан только что (идентификатор ещё не установлен)
	if(result.id == 0){
		// Устанавливаем идентификатор потока
		result.id = id;
		// Устанавливаем окно приёма потока из наших параметров SETTINGS
		result.localWindow = this->_local.windowSize;
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
 */
bool awh::http::Parser_HTTP2::peerInitiated(const uint32_t id) const noexcept {
	// Пир инициирует нечётные потоки, если мы - сервер (разбираем запросы)
	const bool peerOdd = (this->_direct == direct_t::REQUEST);
	// Выполняем сравнение чётности идентификатора потока
	return (((id & 1u) != 0) == peerOdd);
}
/**
 * @brief Метод удаления потока из карты с корректным учётом счётчика встречных потоков
 *
 * @param id идентификатор потока
 */
void awh::http::Parser_HTTP2::eraseStream(const uint32_t id) noexcept {
	// Выполняем поиск потока в карте активных потоков
	const auto i = this->_transfer.streams.find(id);
	// Если поток не найден - удалять нечего
	if(i == this->_transfer.streams.end())
		// Выходим из метода
		return;
	// Освобождаем слот в лимите одновременных потоков, открытых пиром
	if(this->peerInitiated(id) && (this->_transfer.peerStreamCount > 0))
		// Уменьшаем счётчик активных потоков пира
		--this->_transfer.peerStreamCount;
	// Удаляем поток из карты активных потоков
	this->_transfer.streams.erase(i);
}
/**
 * @brief Метод закрытия потока с вызовом функции обратного вызова закрытия
 *
 * @param id   идентификатор потока
 * @param code код ошибки закрытия
 */
void awh::http::Parser_HTTP2::closeStream(const uint32_t id, const error_t code) noexcept {
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
	// Удаляем поток из карты активных потоков
	this->eraseStream(id);
}
/**
 * @brief Метод вызова функции обратного вызова обработки фазы приёма сообщения потока
 *
 * @param id    идентификатор потока
 * @param phase фаза приёма сообщения потока
 * @param part  часть сообщения (заголовки, трейлеры, тело), NONE - сообщение целиком
 * @return      результат обработки (false - поток сброшен)
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
				h2::frame::serialize::rstStream(this->_buffer.output, id, error_t::CANCEL);
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
 * @brief Метод прокачки отправки по всем потокам с учётом окон и порога выходного буфера
 *
 * @details Round-robin: за каждый проход отправляется не более одного DATA-фрейма
 *          с потока, пока хоть один поток делает прогресс - исключает голодание
 *          потоков (head-of-line blocking).
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
		/**
		 * Собираем снимок идентификаторов: прокачка может удалить поток из карты
		 */
		for(const auto & item : this->_transfer.streams)
			// Добавляем идентификатор потока в снимок
			this->_transfer.pumpIds.push_back(item.first);
		/**
		 * Выполняем прокачку отправки по всем потокам снимка
		 */
		for(const uint32_t id : this->_transfer.pumpIds){
			// Выполняем поиск потока (поток мог быть удалён на предыдущей итерации)
			stream_t * stream = this->findStream(id);
			// Если поток существует и его прокачка сделала прогресс
			if((stream != nullptr) && this->pumpStream(* stream))
				// Помечаем что прогресс отправки есть
				progress = true;
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
 */
bool awh::http::Parser_HTTP2::pumpStream(stream_t & stream) noexcept {
	// Запоминаем идентификатор потока
	const uint32_t id = stream.id;
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
	// Определяем является ли фрагмент последним (END_STREAM)
	const bool last = (st.endStreamPending && (size == remaining) && this->sourceDone(st));
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
	// Если отправлен последний фрагмент
	if(last){
		// Помечаем что END_STREAM отправлен
		st.endStreamSent = true;
		// Применяем отправленный END_STREAM (ссылка на поток может стать недействительной)
		this->applyLocalEndStream(st);
	}
	// Прогресс отправки есть
	return true;
}
/**
 * @brief Метод дозагрузки буфера отправки из pull-источника данных (если он задан)
 *
 * @param stream объект потока
 */
void awh::http::Parser_HTTP2::refillFromSource(stream_t & stream) noexcept {
	// Если источник данных не задан либо его тело уже закончилось - дозагружать нечего
	if((stream.source == nullptr) || stream.sourceEof)
		// Выходим из метода
		return;
	/**
	 * Держим буфер наполненным до high-water, запрашивая источник данных порциями
	 */
	while((stream.pending() < this->_transfer.sendHighWater) && !stream.sourceEof){
		// Вычисляем ёмкость запрашиваемой порции (не больше одного DATA-фрейма пира)
		const size_t cap = ::min(static_cast <size_t> (this->_remote.maxFrameSize), this->_transfer.sendHighWater - stream.pending());
		// Запоминаем текущий размер буфера отправки
		const size_t offset = stream.sendBuffer.size();
		// Резервируем место под порцию данных прямо в буфере отправки (без промежуточной копии)
		stream.sendBuffer.resize(offset + cap);
		// Флаг достижения конца тела
		bool eof = false;
		// Результат запроса данных у источника
		int64_t bytes = -1;
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Запрашиваем порцию данных у источника (источник пишет напрямую в буфер отправки)
			bytes = stream.source(stream.id, reinterpret_cast <uint8_t *> (&stream.sendBuffer[offset]), cap, eof);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(stream.id), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
		// Обрезаем буфер отправки до фактически записанного источником размера
		stream.sendBuffer.resize(offset + static_cast <size_t> (((bytes > 0) && (bytes <= static_cast <int64_t> (cap))) ? bytes : 0));
		// Если источник сообщил об ошибке данных либо нарушил контракт (записал больше ёмкости)
		if((bytes < 0) || (bytes > static_cast <int64_t> (cap))){
			// Запоминаем идентификатор потока
			const uint32_t id = stream.id;
			// Сбрасываем поток с кодом INTERNAL_ERROR
			h2::frame::serialize::rstStream(this->_buffer.output, id, error_t::INTERNAL_ERROR);
			// Закрываем поток с вызовом функции обратного вызова закрытия (ссылка на поток недействительна)
			this->closeStream(id, error_t::INTERNAL_ERROR);
			// Выходим из метода
			return;
		}
		// Если достигнут конец тела источника
		if(eof){
			// Помечаем что конец тела источника достигнут
			stream.sourceEof = true;
			// Помечаем что на последнем фрагменте нужно выставить END_STREAM
			stream.endStreamPending = true;
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
		this->_callbacks.writable(stream.id);
	}
}
/**
 * @brief Метод проверки того, что все данные потока для отправки уже получены
 *
 * @param stream объект потока
 * @return       результат проверки (нет источника данных или достигнут его eof)
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
		if(stream->localWindow < (this->_local.windowSize / 2)){
			// Вычисляем инкремент для восстановления окна до начального размера
			const uint32_t delta = static_cast <uint32_t> (this->_local.windowSize - stream->localWindow);
			// Отправляем WINDOW_UPDATE для окна потока
			h2::frame::serialize::windowUpdate(this->_buffer.output, stream->id, delta);
			// Восстанавливаем окно приёма потока
			stream->localWindow += static_cast <int32_t> (delta);
		}
	}
}
/**
 * @brief Метод проверки декодированных заголовков на лимиты безопасности
 *
 * @param fields декодированные заголовки блока
 * @return       результат проверки (false - лимиты превышены)
 */
bool awh::http::Parser_HTTP2::checkHeaderLimits(const vector <h2::hpack::field_t> & fields) const noexcept {
	// Если число заголовков в блоке превышает лимит
	if(fields.size() > this->_limits.maxHeaderCount)
		// Лимиты превышены
		return false;
	/**
	 * Выполняем перебор всех заголовков блока
	 */
	for(const h2::hpack::field_t & field : fields){
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
 * @brief Метод отправки собранного HPACK-блока заголовков потока
 *
 * @details Общая часть перегрузок sendHeaders: нарезка блока на
 *          HEADERS + CONTINUATION и переходы состояния потока.
 *
 * @param sid       идентификатор потока
 * @param block     закодированный HPACK-блок заголовков
 * @param endStream флаг завершения потока (тела не будет)
 */
void awh::http::Parser_HTTP2::commitHeaders(const uint32_t sid, const string & block, const bool endStream) {
	// Отправляем блок заголовков (с автоматической нарезкой на HEADERS + CONTINUATION)
	h2::frame::serialize::headerBlock(this->_buffer.output, sid, block, endStream, this->_remote.maxFrameSize);
	// Получаем существующий либо создаём новый объект потока
	stream_t & stream = this->stream(sid);
	// Если поток ещё не использован - мы инициируем поток
	if(stream.state == h2::stream_state_t::IDLE)
		// Переводим поток в состояние OPEN
		stream.state = h2::stream_state_t::OPEN;
	// Ответ на собственный push: reserved(local) -> half-closed(remote)
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
 */
unique_ptr <awh::http::provider_t> awh::http::Parser_HTTP2::buildProvider(const vector <h2::hpack::field_t> & fields, const bool request) const noexcept {
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
			for(const h2::hpack::field_t & field : fields){
				// Если получен псевдо-заголовок [:method]
				if(this->_fmk->compare(":method", field.name)){
					// Выполняем классификацию метода запроса по его имени
					provider->method = ::classifyMethod(field.value);
					// Если метод запроса синтаксически корректен, но не распознан
					if(provider->method == method_t::NONE){
						// Помечаем метод запроса как нераспознанный
						provider->method = method_t::UNKNOWN;
						// Сохраняем оригинальное написание метода (прозрачное проксирование экзотических методов)
						provider->methodName = field.value;
					}
				// Если получен псевдо-заголовок [:path]
				} else if(this->_fmk->compare(":path", field.name))
					// Устанавливаем параметры URI-запроса
					provider->uri = field.value;
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
			for(const h2::hpack::field_t & field : fields){
				// Если получен псевдо-заголовок [:status]
				if(this->_fmk->compare(":status", field.name)){
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
 */
void awh::http::Parser_HTTP2::reset() noexcept {
	// Выполняем сброс состояния базового парсера (итоговый статус разбора)
	parser_t::reset();
	// Сбрасываем код ошибки уровня соединения
	this->_error = error_t::NO_ERROR;
	// Параметры SETTINGS пира - состояние соединения, сбрасываем к значениям по умолчанию
	this->_remote = settings_t();
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
	// Очищаем карту активных потоков
	this->_transfer.streams.clear();
	// Очищаем снимок идентификаторов потоков
	this->_transfer.pumpIds.clear();
	// Сбрасываем наибольший принятый идентификатор потока
	this->_transfer.lastStreamId = 0;
	// Сбрасываем счётчик активных потоков, открытых пиром
	this->_transfer.peerStreamCount = 0;
	// Клиент инициирует нечётные потоки (1,3,5...), сервер - чётные (push: 2,4,6...)
	this->_transfer.nextStreamId = ((this->_direct == direct_t::REQUEST) ? 2 : 1);
	// Сервер ожидает клиентский preface; клиент отправляет его сам через sendPreface()
	this->_flags.prefaceReceived = (this->_direct != direct_t::REQUEST);
	// Сбрасываем защиту от реентерабельного pump()
	this->_flags.inPump = false;
	// Сбрасываем защиту от реентерабельного parse()
	this->_flags.inParse = false;
	// Сбрасываем флаг отправленного GOAWAY
	this->_flags.goawaySent = false;
	// Сбрасываем флаг отклонённого потока
	this->_flags.hbcRefused = false;
	// Сбрасываем флаг END_STREAM собираемого блока заголовков
	this->_flags.hbcEndStream = false;
	// Сбрасываем флаг подтверждения нашего SETTINGS
	this->_flags.settingsAcked = false;
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
	this->_buffer.input.clear();
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
}
/**
 * @brief Метод клонирования объекта парсера
 *
 * @details Клон получает те же направление трафика, лимиты безопасности,
 *          параметры SETTINGS и функции обратного вызова, но чистое
 *          состояние соединения ("фабрика с теми же настройками").
 *
 * @return копия объекта парсера
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
 */
void awh::http::Parser_HTTP2::eof() noexcept {
	// Если разбор уже завершился ошибкой - состояние не меняем
	if(this->_status == status_t::ERROR)
		// Выходим из метода
		return;
	// Если активных потоков нет, нет незавершённого фрейма и сборки блока заголовков
	if(this->_transfer.streams.empty() && this->_buffer.input.empty() && (this->_hbc.stream == 0))
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
		if((buffer != nullptr) && (size > 0))
			// Дописываем данные во входной буфер
			this->_buffer.input.append(static_cast <const char *> (buffer), size);
		/**
		 * Реентерабельный вызов из функции обратного вызова: данные уже добавлены во
		 * входной буфер, а внешний разбор их подхватит (parseInput перечитывает буфер
		 * на каждой итерации). Это исключает висячий указатель на входной буфер при
		 * перевыделении памяти из вложенного дописывания
		 */
		if(this->_flags.inParse)
			// Выводим количество обработанных байт данных
			return size;
		// Помечаем что разбор уже выполняется
		this->_flags.inParse = true;
		// Выполняем разбор накопленного входного буфера
		this->parseInput();
		// Помечаем что разбор завершён
		this->_flags.inParse = false;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
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
 */
awh::http::Parser_HTTP2::error_t awh::http::Parser_HTTP2::error() const noexcept {
	// Выводим код ошибки уровня соединения
	return this->_error;
}
/**
 * @brief Метод получения человекочитаемого названия текущей ошибки разбора
 *
 * @return название текущей ошибки разбора
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
 */
string_view awh::http::Parser_HTTP2::errorName(const error_t error) noexcept {
	// Выводим название кода ошибки
	return h2::errorName(error);
}
/**
 * @brief Метод получения лимитов безопасности
 *
 * @return лимиты безопасности
 */
const awh::http::Parser_HTTP2::limits_t & awh::http::Parser_HTTP2::limits() const noexcept {
	// Выводим лимиты безопасности
	return this->_limits;
}
/**
 * @brief Метод установки лимитов безопасности
 *
 * @param limits лимиты безопасности
 */
void awh::http::Parser_HTTP2::limits(const limits_t & limits) noexcept {
	// Устанавливаем лимиты безопасности
	this->_limits = limits;
	// Применяем новые параметры лимита частоты входящих RST_STREAM
	this->_ratelims.rst.init(limits.rstLimitBurst, limits.rstLimitRate);
	// Применяем новые параметры лимита частоты управляющих фреймов
	this->_ratelims.ctrl.init(limits.ctrlLimitBurst, limits.ctrlLimitRate);
}
/**
 * @brief Метод получения наших параметров SETTINGS
 *
 * @return наши параметры SETTINGS
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
 */
void awh::http::Parser_HTTP2::settings(const settings_t & settings) noexcept {
	// Устанавливаем наши параметры SETTINGS
	this->_local = settings;
}
/**
 * @brief Метод получения параметров SETTINGS пира
 *
 * @return параметры SETTINGS пира
 */
const awh::http::Parser_HTTP2::settings_t & awh::http::Parser_HTTP2::remoteSettings() const noexcept {
	// Выводим параметры SETTINGS пира
	return this->_remote;
}
/**
 * @brief Метод проверки того, что соединение помечено на завершение
 *
 * @return признак завершения (отправлен или получен GOAWAY)
 */
bool awh::http::Parser_HTTP2::isClosed() const noexcept {
	// Соединение помечено на завершение, если GOAWAY отправлен или получен
	return (this->_flags.goawaySent || this->_flags.goawayReceived);
}
/**
 * @brief Метод отправки исходящего preface соединения
 *
 * @details Клиент отправляет magic-строку + свой SETTINGS, сервер - только SETTINGS.
 *          Обязан быть первым исходящим сообщением соединения.
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
	// Список отправляемых параметров SETTINGS
	h2::frame::setting_entry_t items[6];
	// Количество отправляемых параметров
	size_t count = 0;
	// Добавляем параметр размера динамической таблицы HPACK
	items[count].id = h2::setting_t::HEADER_TABLE_SIZE;
	// Устанавливаем значение параметра
	items[count++].value = this->_local.headerTableSize;
	// Добавляем параметр разрешения server push
	items[count].id = h2::setting_t::ENABLE_PUSH;
	// Устанавливаем значение параметра
	items[count++].value = this->_local.enablePush;
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
	// Отправляем SETTINGS-фрейм с собранными параметрами
	h2::frame::serialize::settings(this->_buffer.output, items, count, false);
	// Передаём исходящие байты сетевому слою
	this->flush();
}
/**
 * @brief Метод отправки RST_STREAM (аварийное закрытие потока)
 *
 * @param sid  идентификатор потока
 * @param code код ошибки, с которым сбрасывается поток
 */
void awh::http::Parser_HTTP2::sendRstStream(const uint32_t sid, const error_t code) noexcept {
	// Отправляем фрейм RST_STREAM
	h2::frame::serialize::rstStream(this->_buffer.output, sid, code);
	// Удаляем поток из карты активных потоков
	this->eraseStream(sid);
	// Передаём исходящие байты сетевому слою
	this->flush();
}
/**
 * @brief Метод отправки GOAWAY (пометка соединения завершаемым)
 *
 * @param code  код ошибки завершения соединения
 * @param debug необязательные отладочные данные
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
 * @brief Метод отправки WINDOW_UPDATE
 *
 * @param sid       идентификатор потока (0 - окно всего соединения)
 * @param increment инкремент окна flow control
 */
void awh::http::Parser_HTTP2::sendWindowUpdate(const uint32_t sid, const uint32_t increment) noexcept {
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
		// Если наша половина потока закрыта - данные не принимаются
		if((stream->state == h2::stream_state_t::HALF_CLOSED_LOCAL) || (stream->state == h2::stream_state_t::CLOSED))
			// Выводим число принятых байт
			return result;
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
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Резервируем чётный push-поток
		result = this->_transfer.nextStreamId;
		// Смещаем следующий инициируемый нами идентификатор потока
		this->_transfer.nextStreamId += 2;
		// Закодированный HPACK-блок заголовков обещанного запроса
		string block = "";
		// Выполняем кодирование заголовков в HPACK-блок
		this->_encoder.encode(fields, block, true);
		// Отправляем PUSH_PROMISE (с автоматической нарезкой на PUSH_PROMISE + CONTINUATION)
		h2::frame::serialize::pushPromiseBlock(this->_buffer.output, sid, result, block, this->_remote.maxFrameSize);
		// Получаем объект зарезервированного push-потока
		stream_t & stream = this->stream(result);
		// Переводим push-поток в состояние RESERVED_LOCAL
		stream.state = h2::stream_state_t::RESERVED_LOCAL;
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
 */
void awh::http::Parser_HTTP2::sendHeaders(const uint32_t sid, const vector <h2::hpack::field_t> & fields, const bool endStream) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Закодированный HPACK-блок заголовков
		string block = "";
		// Выполняем кодирование заголовков в HPACK-блок
		this->_encoder.encode(fields, block, true);
		// Отправляем HPACK-блок заголовков потока
		this->commitHeaders(sid, block, endStream);
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
 */
void awh::http::Parser_HTTP2::sendHeaders(const uint32_t sid, const headers_t & headers, const bool endStream, string_view scheme) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Закодированный HPACK-блок заголовков
		string block = "";
		// Дописываем отложенный Dynamic Table Size Update (если требуется)
		this->_encoder.begin(block);
		// Получаем объект провайдера контейнера заголовков
		const provider_t * provider = headers.provider();
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
					this->_encoder.encode(":method", ::methodName(request), block);
					// Для метода CONNECT псевдо-заголовки [:scheme] и [:path] запрещены (RFC 9113 §8.5)
					if(request->method != method_t::CONNECT)
						// Кодируем псевдо-заголовок [:scheme]
						this->_encoder.encode(":scheme", scheme, block);
					// Если контейнер содержит заголовок Host - конвертируем его в псевдо-заголовок [:authority]
					if(headers.has("host"))
						// Кодируем псевдо-заголовок [:authority]
						this->_encoder.encode(":authority", headers.at("host"), block);
					// Для метода CONNECT псевдо-заголовки [:scheme] и [:path] запрещены (RFC 9113 §8.5)
					if(request->method != method_t::CONNECT)
						// Кодируем псевдо-заголовок [:path] (пустой путь запрещён - подставляем "/")
						this->_encoder.encode(":path", (request->uri.empty() ? "/" : request->uri), block);
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
 */
uint32_t awh::http::Parser_HTTP2::nextStreamId() noexcept {
	// Запоминаем выделяемый идентификатор потока
	const uint32_t result = this->_transfer.nextStreamId;
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
	// Прокачиваем отправку по всем потокам
	this->pump();
	// Передаём исходящие байты сетевому слою
	this->flush();
}
/**
 * @brief Метод настройки порога выходного буфера соединения (backpressure от TCP-стадии)
 *
 * @param high порог выходного буфера соединения
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
 */
string_view awh::http::Parser_HTTP2::pending() const noexcept {
	// Выводим ещё не отправленные исходящие байты
	return string_view(this->_buffer.output.data() + this->_buffer.outputPos, this->_buffer.output.size() - this->_buffer.outputPos);
}
/**
 * @brief Метод освобождения отправленных байтов из исходящего буфера (амортизированно O(1))
 *
 * @param size число отправленных байт
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
 */
void awh::http::Parser_HTTP2::on(push_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.push = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки фрагмента тела потока
 *
 * @param callback функция обратного вызова для обработки фрагмента тела потока
 */
void awh::http::Parser_HTTP2::on(data_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.data = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки закрытия потока
 *
 * @param callback функция обратного вызова для обработки закрытия потока
 */
void awh::http::Parser_HTTP2::on(close_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.close = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки ошибки уровня соединения
 *
 * @param callback функция обратного вызова для обработки ошибки уровня соединения
 */
void awh::http::Parser_HTTP2::on(error_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.error = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова записи исходящих байтов в сеть
 *
 * @param callback функция обратного вызова записи исходящих байтов в сеть
 */
void awh::http::Parser_HTTP2::on(write_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.write = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки открытия нового потока
 *
 * @param callback функция обратного вызова для обработки открытия нового потока
 */
void awh::http::Parser_HTTP2::on(begin_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.begin = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки фазы приёма сообщения потока
 *
 * @param callback функция обратного вызова для обработки фазы приёма сообщения потока
 */
void awh::http::Parser_HTTP2::on(phase_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.phase = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки полученного GOAWAY
 *
 * @param callback функция обратного вызова для обработки полученного GOAWAY
 */
void awh::http::Parser_HTTP2::on(goaway_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.goaway = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки заголовков или трейлеров потока
 *
 * @param callback функция обратного вызова для обработки заголовков или трейлеров потока
 */
void awh::http::Parser_HTTP2::on(header_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.header = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова о готовности потока принимать данные тела
 *
 * @param callback функция обратного вызова о готовности потока принимать данные тела
 */
void awh::http::Parser_HTTP2::on(writable_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.writable = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки применённого SETTINGS пира
 *
 * @param callback функция обратного вызова для обработки применённого SETTINGS пира
 */
void awh::http::Parser_HTTP2::on(settings_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.settings = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова для обработки провайдера заголовков потока
 *
 * @param callback функция обратного вызова для обработки провайдера заголовков потока
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
 */
awh::http::Parser_HTTP2::Parser_HTTP2(const direct_t direct, const fmk_t * fmk, const log_t * log) noexcept :
 parser_t(direct, fmk, log), _error(error_t::NO_ERROR) {
	// Запоминаем направление трафика
	this->_flags.prefaceReceived = (direct != direct_t::REQUEST);
	// Устанавливаем начальный идентификатор инициируемого нами потока
	this->_transfer.nextStreamId = (direct == direct_t::REQUEST ? 2 : 1);
	// Инициализируем лимит частоты входящих RST_STREAM из лимитов безопасности
	this->_ratelims.rst.init(this->_limits.rstLimitBurst, this->_limits.rstLimitRate);
	// Инициализируем лимит частоты управляющих фреймов из лимитов безопасности
	this->_ratelims.ctrl.init(this->_limits.ctrlLimitBurst, this->_limits.ctrlLimitRate);
}
/**
 * @brief Деструктор
 *
 */
awh::http::Parser_HTTP2::~Parser_HTTP2() noexcept {}
