/**
 * @file: provider.hpp
 * @date: 2026-07-08
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл провайдеров HTTP-сообщений — классы Provider, Request и Response,
 *        формирующие и хранящие структуру HTTP-запроса клиента и HTTP-ответа сервера поверх контейнера заголовков
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PROVIDER__
#define __AWH_HTTP_PROVIDER__

/**
 * Стандартные заголовочные файлы
 */
#include <memory>
#include <string>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "http.hpp"
#include "../../sys/global.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён HTTP-протокола
	 *
	 */
	namespace http {
		/**
		 * @brief Функция получения стандартного сообщения HTTP-ответа по коду
		 *
		 * @details Таблица сообщений хранится как локальная static constexpr-таблица указателей на строковые
		 *          литералы: данные размещаются в .rodata, без конструкторов при старте программы и без
		 *          динамических аллокаций. Функция помечена inline, поэтому во всей программе существует
		 *          единственный экземпляр таблицы (без inline-переменных, требующих C++17).
		 *
		 * @param code код ответа сервера
		 * @return     стандартное сообщение либо пустое представление, если код неизвестен
		 *
		 */
		inline string_view statusMessage(const uint16_t code) noexcept {
			/**
			 * @brief Структура записи таблицы стандартных сообщений HTTP-ответов сервера
			 *
			 */
			struct Message {
				// Код ответа сервера
				uint16_t code = 0;
				// Стандартное сообщение сервера (указатель на строковый литерал в .rodata)
				const char * text = nullptr;
			};
			// Таблица стандартных сообщений HTTP-ответов сервера
			static constexpr Message messages[] = {
				{0, "Not Answer"},
				{100, "Continue"},
				{101, "Switching Protocols"},
				{102, "Processing"},
				{103, "Early Hints"},
				{200, "OK"},
				{201, "Created"},
				{202, "Accepted"},
				{203, "Non-Authoritative Information"},
				{204, "No Content"},
				{205, "Reset Content"},
				{206, "Partial Content"},
				{300, "Multiple Choice"},
				{301, "Moved Permanently"},
				{302, "Found"},
				{303, "See Other"},
				{304, "Not Modified"},
				{305, "Use Proxy"},
				{306, "Switch Proxy"},
				{307, "Temporary Redirect"},
				{308, "Permanent Redirect"},
				{400, "Bad Request"},
				{401, "Authentication Required"},
				{402, "Payment Required"},
				{403, "Forbidden"},
				{404, "Not Found"},
				{405, "Method Not Allowed"},
				{406, "Not Acceptable"},
				{407, "Proxy Authentication Required"},
				{408, "Request Timeout"},
				{409, "Conflict"},
				{410, "Gone"},
				{411, "Length Required"},
				{412, "Precondition Failed"},
				{413, "Request Entity Too Large"},
				{414, "Request-URI Too Long"},
				{415, "Unsupported Media Type"},
				{416, "Requested Range Not Satisfiable"},
				{417, "Expectation Failed"},
				{500, "Internal Server Error"},
				{501, "Not Implemented"},
				{502, "Bad Gateway"},
				{503, "Service Unavailable"},
				{504, "Gateway Timeout"},
				{505, "HTTP Version Not Supported"}
			};
			/**
			 * Выполняем перебор таблицы стандартных сообщений
			 */
			for(const auto & item : messages){
				// Если код ответа совпадает - возвращаем соответствующее стандартное сообщение
				if(item.code == code)
					// Возвращаем найденное стандартное сообщение
					return item.text;
			}
			// Стандартное сообщение для указанного кода не найдено
			return "";
		}

		/**
		 * @brief Класс провайдера
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Provider {
			public:
				// Версия протокола
				version_t version;
				// Направление трафика (запрос/ответ)
				const direct_t direct;
			public:
				/**
				 * @brief Метод клонирования объекта провайдера
				 *
				 * @details Обеспечивает корректное копирование производного объекта (Request/Response)
				 *          через указатель на базовый класс без срезки (object slicing)
				 *
				 * @return копия объекта провайдера
				 *
				 */
				virtual unique_ptr <Provider> clone() const noexcept = 0;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param direct направление трафика (запрос/ответ)
				 *
				 */
				explicit Provider(const direct_t direct) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param direct  направление трафика (запрос/ответ)
				 * @param version версия протокола
				 *
				 */
				explicit Provider(const direct_t direct, const version_t version) noexcept;
			public:
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~Provider() noexcept = default;
		} provider_t;

		/**
		 * @brief Класс HTTP-запроса клиента
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Request : public provider_t {
			public:
				// Параметры URI-запроса
				string uri;
				// Метод запроса клиента
				method_t method;
				// Оригинальное написание метода запроса (заполняется ТОЛЬКО для method_t::UNKNOWN - прозрачное проксирование экзотических методов)
				string methodName;
				/**
				 * Протокол расширенного метода CONNECT (псевдо-заголовок [:protocol], RFC 8441).
				 * Заполняется ТОЛЬКО для HTTP/2 и HTTP/3: именно так поверх CONNECT
				 * поднимается WebSocket. Для остальных запросов остаётся пустым
				 */
				string protocol;
			public:
				/**
				 * @brief Метод клонирования объекта запроса
				 *
				 * @return копия объекта запроса
				 *
				 */
				unique_ptr <provider_t> clone() const noexcept override;
			public:
				/**
				 * @brief Оператор [=] перемещения параметров запроса клиента
				 *
				 * @param request объект параметров запроса клиента
				 * @return        текущие параметры запроса клиента
				 *
				 */
				Request & operator = (Request && request) noexcept;
				/**
				 * @brief Оператор [=] присванивания параметров запроса клиента
				 *
				 * @param request объект параметров запроса клиента
				 * @return        текущие параметры запроса клиента
				 *
				 */
				Request & operator = (const Request & request) noexcept;
			public:
				/**
				 * @brief Оператор [==] сравнения
				 *
				 * @param request объект параметров запроса клиента
				 * @return        результат сравнения
				 *
				 */
				bool operator == (const Request & request) const noexcept;
				/**
				 * @brief Оператор [!=] сравнения
				 *
				 * @param request объект параметров запроса клиента
				 * @return        результат сравнения
				 *
				 */
				bool operator != (const Request & request) const noexcept;
			public:
				/**
				 * @brief Конструктор перемещения
				 *
				 * @param request объект параметров запроса клиента
				 *
				 */
				Request(Request && request) noexcept;
				/**
				 * @brief Конструктор копирования
				 *
				 * @param request объект параметров запроса клиента
				 *
				 */
				Request(const Request & request) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Request() noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param uri параметры URI-запроса
				 *
				 */
				explicit Request(const string & uri) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param method метод запроса клиента
				 *
				 */
				explicit Request(const method_t method) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 *
				 */
				explicit Request(const version_t version) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param method метод запроса клиента
				 * @param uri    параметры URI-запроса
				 *
				 */
				explicit Request(const method_t method, const string & uri) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 * @param uri     параметры URI-запроса
				 *
				 */
				explicit Request(const version_t version, const string & uri) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 * @param method  метод запроса клиента
				 *
				 */
				explicit Request(const version_t version, const method_t method) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 * @param method  метод запроса клиента
				 * @param uri     параметры URI-запроса
				 *
				 */
				explicit Request(const version_t version, const method_t method, const string & uri) noexcept;
			public:
				/**
				 * @brief Деструктор
				 *
				 */
				~Request() noexcept = default;
		} request_t;

		/**
		 * @brief Класс HTTP-ответа сервера
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Response : public provider_t {
			public:
				// Код ответа сервера
				uint16_t code;
				// Сообщение сервера
				string message;
			public:
				/**
				 * @brief Метод клонирования объекта ответа
				 *
				 * @return копия объекта ответа
				 *
				 */
				unique_ptr <provider_t> clone() const noexcept override;
			public:
				/**
				 * @brief Оператор [=] перемещения параметров ответа сервера
				 *
				 * @param response объект параметров ответа сервера
				 * @return         текущие параметры ответа сервера
				 *
				 */
				Response & operator = (Response && response) noexcept;
				/**
				 * @brief Оператор [=] присванивания параметров ответа сервера
				 *
				 * @param response объект параметров ответа сервера
				 * @return         текущие параметры ответа сервера
				 *
				 */
				Response & operator = (const Response & response) noexcept;
			public:
				/**
				 * @brief Оператор [==] сравнения
				 *
				 * @param response объект параметров ответа сервера
				 * @return         результат сравнения
				 *
				 */
				bool operator == (const Response & response) const noexcept;
				/**
				 * @brief Оператор [!=] сравнения
				 *
				 * @param response объект параметров ответа сервера
				 * @return         результат сравнения
				 *
				 */
				bool operator != (const Response & response) const noexcept;
			public:
				/**
				 * @brief Конструктор перемещения
				 *
				 * @param response объект параметров ответа сервера
				 *
				 */
				Response(Response && response) noexcept;
				/**
				 * @brief Конструктор копирования
				 *
				 * @param response объект параметров ответа сервера
				 *
				 */
				Response(const Response & response) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Response() noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param code код ответа сервера
				 *
				 */
				explicit Response(const uint16_t code) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param message сообщение сервера
				 *
				 */
				explicit Response(const string & message) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 *
				 */
				explicit Response(const version_t version) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param code    код ответа сервера
				 * @param message сообщение сервера
				 *
				 */
				explicit Response(const uint16_t code, const string & message) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 * @param code    код ответа сервера
				 *
				 */
				explicit Response(const version_t version, const uint16_t code) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 * @param message сообщение сервера
				 *
				 */
				explicit Response(const version_t version, const string & message) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param version версия протокола
				 * @param code    код ответа сервера
				 * @param message сообщение сервера
				 *
				 */
				explicit Response(const version_t version, const uint16_t code, const string & message) noexcept;
			public:
				/**
				 * @brief Деструктор
				 *
				 */
				~Response() noexcept = default;
		} response_t;
    };
};

#endif // __AWH_HTTP_PROVIDER__
