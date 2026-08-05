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
		__AWH_SHARED_EXPORT__ string_view statusMessage(const uint16_t code) noexcept;
		/**
		 * @brief Функция классификации метода запроса по значению псевдо-заголовка [:method]
		 *
		 * @note В отличие от HTTP/1.x сравнение выполняется с учётом регистра:
		 *       методы HTTP - регистрозависимые токены (RFC 9110 §9.1).
		 *
		 * @param method значение псевдо-заголовка [:method]
		 * @return       распознанный метод запроса либо method_t::NONE
		 *
		 */
		__AWH_SHARED_EXPORT__ http::method_t classifyMethod(string_view method) noexcept;
		/**
		 * @brief Функция получения имени метода запроса для псевдо-заголовка [:method]
		 *
		 * @param request провайдер запроса клиента
		 * @return        имя метода запроса (для UNKNOWN - оригинальное написание метода)
		 *
		 */
		__AWH_SHARED_EXPORT__ string_view methodName(const http::request_t * request) noexcept;
		/**
		 * @brief Функция приведения пути цели запроса к виду псевдо-заголовка [:path]
		 *
		 * @details Псевдо-заголовок пути не бывает пустым и обязан начинаться с '/'
		 *          (RFC 9113 §8.3.1, RFC 9114 §4.3.1). Буфер задействуется только
		 *          когда путь требует дополнения, то есть в редком случае цели вида
		 *          [https://example.com?q=1]; в остальных выдаётся представление
		 *          на исходную строку без копирования.
		 *
		 *          Звёздочная форма цели выдаётся без изменений: ею метод OPTIONS
		 *          обращается к серверу целиком, а не к его ресурсу (RFC 9112 §3.2.4),
		 *          и псевдо-заголовок пути в таком запросе обязан нести именно её.
		 *
		 * @param path   путь цели запроса
		 * @param buffer буфер под дополненный путь
		 * @return       путь, пригодный для псевдо-заголовка
		 *
		 */
		__AWH_SHARED_EXPORT__ string_view targetPath(const string_view path, string & buffer) noexcept;
		/**
		 * @brief Функция разбора цели запроса, заданной в абсолютной форме (RFC 9112 §3.2.2)
		 *
		 * @details Абсолютная форма цели законна в HTTP/1 и обязательна в запросе
		 *          к прокси, а провайдер запроса общий для всех протоколов. Протоколы
		 *          же, работающие псевдо-заголовками, требуют разнесённых схемы,
		 *          авторитета и пути (RFC 9113 §8.3.1, RFC 9114 §4.3.1), поэтому цель
		 *          перед отправкой обязана быть разобрана.
		 *
		 *          Конец компонента авторитета - первый из символов '/', '?' и '#'
		 *          (RFC 3986 §3.2), а не один только '/': иначе строка запроса цели
		 *          без пути уехала бы внутрь авторитета.
		 *
		 *          Выданный путь может оказаться пустым либо начинаться со строки
		 *          запроса - привести его к виду псевдо-заголовка позволяет targetPath().
		 *
		 * @param target    цель запроса
		 * @param scheme    схема цели (представление в target)
		 * @param authority авторитет цели (представление в target)
		 * @param path      путь цели вместе со строкой запроса (представление в target)
		 * @return          признак того, что цель задана в абсолютной форме
		 *
		 */
		__AWH_SHARED_EXPORT__ bool splitTarget(const string_view target, string_view & scheme, string_view & authority, string_view & path) noexcept;
    };
};

#endif // __AWH_HTTP_PROVIDER__
