/**
 * @file: quic.hpp
 * @date: 2026-07-22
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
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT_QUIC__
#define __AWH_UNIT_QUIC__

/**
 * Стандартные заголовочные файлы
 */
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "unit.hpp"
#include "../net/tls/coder.hpp"
#include "../proto/quic/connection.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён модулей
	 *
	 */
	namespace unit {
		/**
		 * Подписываемся на стандартное пространство имён
		 */
		using namespace std;

		/**
		 * @brief Класс модуля сервера транспортного протокола QUIC
		 *
		 * @details Связывает соединения QUIC с асинхронным сетевым движком.
		 *          Соединение адресуется набором идентификаторов, который меняется
		 *          по ходу работы: к выданному при установлении добавляются
		 *          анонсированные фреймами NEW_CONNECTION_ID, а выведенные из
		 *          обращения удаляются. Модуль синхронизирует этот набор с
		 *          маршрутизацией движка после каждой обработанной датаграммы,
		 *          поэтому смена адреса клиента соединение не разрывает
		 *          (RFC 9000 §9), а забыть синхронизацию невозможно.
		 */
		typedef class __AWH_SHARED_EXPORT__ QuicServer : public unit_t {
			private:
				/**
				 * @brief Структура сессии соединения
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Session {
					// Объект соединения QUIC
					unique_ptr <quic::connection_t> connection;
					// Флаг оповещения приложения об установленном соединении
					bool connected;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Session() noexcept;
				} session_t;
			private:
				// Идентификатор события сервера
				event::id_t _eid;
				// Идентификатор события интервала таймеров соединений
				event::id_t _tid;
			private:
				// Флаг проверки адреса клиента через пакет Retry (RFC 9000 §8.1.2)
				bool _retry;
				// Флаг уведомления о перегрузке пути (RFC 9000 §13.4)
				bool _ecn;
				// Семейство адресов события сервера
				event::family_t _family;
				/**
				 * Маркировка, установленная на сокете события сервера. Маркировка
				 * накладывается на сокет целиком, а проверку пути соединения ведут
				 * порознь: значение кешируется, чтобы менять его только при
				 * расхождении с требуемым, а не перед каждой датаграммой
				 */
				event::ecn_t _marking;
			private:
				/**
				 * Общий ключ вывода токенов сброса без сохранения состояния. Генерируется
				 * при запуске сервера, если не задан приложением: сохранённый между
				 * запусками ключ позволяет сбрасывать соединения, о которых сервер
				 * после перезапуска уже ничего не помнит (RFC 9000 §10.3.2)
				 */
				string _resetKey;
			private:
				// Идентификатор шаблона контекста безопасности
				tls::coder_t::id_t _ctx;
				// Объект кодера транспортной безопасности
				const tls::coder_t * _coder;
			private:
				// Локальные транспортные параметры соединений
				quic::params::params_t _params;
			private:
				// Список сессий соединений по идентификаторам событий
				map <event::id_t, session_t> _sessions;
			private:
				/**
				 * @brief Метод получения текущего времени в миллисекундах
				 *
				 * @return текущее время в миллисекундах
				 */
				uint64_t date() const noexcept;
				/**
				 * @brief Метод формирования адреса удалённого эндпоинта сессии
				 *
				 * @param oid идентификатор события сессии
				 * @return    адрес удалённого эндпоинта в виде "адрес:порт"
				 */
				string peer(const event::id_t oid) const noexcept;
			private:
				/**
				 * @brief Метод определения сессии принятой датаграммы (RFC 9000 §17.2)
				 *
				 * @param eid  идентификатор события сервера
				 * @param data данные датаграммы
				 * @param size размер датаграммы
				 * @param key  выводимый ключ сессии
				 * @return     результат определения сессии
				 */
				bool origin(const event::id_t eid, const uint8_t * data, const size_t size, net::origin_key_t & key) noexcept;
				/**
				 * @brief Метод создания сессии нового соединения
				 *
				 * @param eid идентификатор события сервера
				 * @param oid идентификатор события сессии
				 */
				void accept(const event::id_t eid, const event::id_t oid) noexcept;
				/**
				 * @brief Метод обработки принятой датаграммы сессии
				 *
				 * @param oid  идентификатор события сессии
				 * @param data данные датаграммы
				 * @param size размер датаграммы
				 */
				void read(const event::id_t oid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод обработки просроченных таймеров соединений
				 *
				 * @param eid    идентификатор события интервала
				 * @param status статус события интервала
				 */
				void tick(const event::id_t eid, const event::status_t status) noexcept;
			private:
				/**
				 * @brief Метод синхронизации маршрутизации соединения
				 *
				 * @note Идентификаторы, введённые соединением в обращение, привязываются
				 *       к сессии движка, а выведенные - снимаются. Без синхронизации
				 *       датаграмма с новым идентификатором была бы принята за новое
				 *       соединение
				 *
				 * @param oid     идентификатор события сессии
				 * @param session сессия соединения
				 */
				void reroute(const event::id_t oid, session_t & session) noexcept;
				/**
				 * @brief Метод выдачи собранных данных потоков приложения
				 *
				 * @param oid     идентификатор события сессии
				 * @param session сессия соединения
				 */
				void process(const event::id_t oid, session_t & session) noexcept;
				/**
				 * @brief Метод применения маркировки соединения к сокету события сервера
				 *
				 * @param marking требуемая маркировка исходящих датаграмм
				 */
				void mark(const event::ecn_t marking) noexcept;
				/**
				 * @brief Метод отправки готовых исходящих датаграмм соединения
				 *
				 * @param oid     идентификатор события сессии
				 * @param session сессия соединения
				 */
				bool flush(const event::id_t oid, session_t & session) noexcept;
				/**
				 * @brief Метод отправки сброса без сохранения состояния (RFC 9000 §10.3)
				 *
				 * @note Отправляется в ответ на датаграмму, адресованную соединению,
				 *       о котором сервер ничего не помнит: без сброса удалённый узел
				 *       продолжит отправку до самого таймаута простоя
				 *
				 * @param oid  идентификатор события сессии
				 * @param data данные вызвавшей сброс датаграммы
				 * @param size размер вызвавшей сброс датаграммы
				 * @return     результат отправки
				 */
				bool drop(const event::id_t oid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод завершения сессии соединения
				 *
				 * @param oid идентификатор события сессии
				 */
				void erase(const event::id_t oid) noexcept;
			public:
				/**
				 * @brief Метод установки шаблона контекста безопасности соединений
				 *
				 * @note Криптография соединений задаётся целиком на шаблоне контекста:
				 *       сертификаты, доверенные центры, проверка узла и список
				 *       ALPN-протоколов настраиваются там. Кодер обязан пережить модуль
				 *
				 * @param coder объект кодера транспортной безопасности
				 * @param ctx   идентификатор шаблона контекста безопасности
				 */
				void context(const tls::coder_t & coder, const tls::coder_t::id_t ctx) noexcept;
				/**
				 * @brief Метод установки локальных транспортных параметров соединений (RFC 9000 §7.4)
				 *
				 * @param params локальные транспортные параметры
				 */
				void params(const quic::params::params_t & params) noexcept;
				/**
				 * @brief Метод установки проверки адреса клиента через пакет Retry (RFC 9000 §8.1.2)
				 *
				 * @param mode режим проверки адреса клиента
				 */
				void retry(const bool mode) noexcept;
				/**
				 * @brief Метод установки уведомления о перегрузке пути (RFC 9000 §13.4)
				 *
				 * @note Исходящие датаграммы помечаются поддержкой ECN, а маркировка
				 *       входящих сообщается соединениям: маршрутизатор на пути
				 *       сигнализирует о заторе, не отбрасывая пакет, и окно перегрузки
				 *       сокращается раньше и без утраты данных. Режим включается до
				 *       запуска сервера и требует извлечения метаданных каждой
				 *       датаграммы, что снижает пропускную способность приёма
				 *
				 * @param mode режим уведомления о перегрузке пути
				 */
				void ecn(const bool mode) noexcept;
				/**
				 * @brief Метод установки общего ключа вывода токенов сброса (RFC 9000 §10.3.2)
				 *
				 * @note Вызывается до запуска сервера. Без явной установки ключ генерируется
				 *       случайно при запуске: сброс без сохранения состояния будет работать
				 *       в пределах жизни процесса, но не переживёт его перезапуск
				 *
				 * @param key общий ключ вывода токенов сброса
				 */
				void resetKey(string_view key) noexcept;
			public:
				/**
				 * @brief Метод запуска сервера соединений
				 *
				 * @param family семейство адресов события сервера
				 * @param ip     адрес прослушивания события сервера
				 * @param port   порт прослушивания события сервера
				 * @return       идентификатор созданного события сервера
				 */
				event::id_t listen(const event::family_t family, string_view ip, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод открытия потока приложения соединения
				 *
				 * @param oid  идентификатор события сессии
				 * @param mode режим однонаправленного потока
				 * @return     идентификатор открытого потока
				 */
				uint64_t open(const event::id_t oid, const bool mode = false) noexcept;
				/**
				 * @brief Метод отправки данных в поток приложения соединения
				 *
				 * @param oid  идентификатор события сессии
				 * @param sid  идентификатор потока приложения
				 * @param data отправляемые данные
				 * @param fin  флаг завершения потока
				 * @return     результат постановки данных в очередь отправки
				 */
				bool send(const event::id_t oid, const uint64_t sid, string_view data, const bool fin = false) noexcept;
				/**
				 * @brief Метод отправки датаграммы приложения соединению (RFC 9221)
				 *
				 * @note Доставка датаграмм ненадёжна: потерянная датаграмма повторно
				 *       не отправляется, порядок доставки не гарантируется. Отправка
				 *       возможна только когда удалённый узел анонсировал их приём
				 *
				 * @param oid  идентификатор события сессии
				 * @param data данные датаграммы приложения
				 * @return     результат отправки
				 */
				bool datagram(const event::id_t oid, string_view data) noexcept;
				/**
				 * @brief Метод получения предельного размера отправляемой датаграммы (RFC 9221 §3)
				 *
				 * @param oid идентификатор события сессии
				 * @return    предельный размер данных датаграммы в октетах (0 - датаграммы не поддерживаются)
				 */
				size_t datagrams(const event::id_t oid) const noexcept;
				/**
				 * @brief Метод завершения соединения приложением (RFC 9000 §10.2)
				 *
				 * @param oid    идентификатор события сессии
				 * @param code   код ошибки приложения
				 * @param reason человекочитаемая причина завершения
				 */
				void close(const event::id_t oid, const uint64_t code = 0, string_view reason = "") noexcept;
			public:
				/**
				 * @brief Метод получения согласованного ALPN-протокола соединения
				 *
				 * @param oid идентификатор события сессии
				 * @return    согласованный ALPN-протокол
				 */
				tls::coder_t::alpn_t alpn(const event::id_t oid) const noexcept;
				/**
				 * @brief Метод получения адреса удалённого эндпоинта соединения
				 *
				 * @param oid идентификатор события сессии
				 * @return    адрес удалённого эндпоинта в виде "адрес:порт"
				 */
				string address(const event::id_t oid) const noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @note Поддерживаются функции обратного вызова: "open" на установленное
				 *       соединение, "read" на собранные данные потока приложения,
				 *       "datagram" на принятую датаграмму приложения и "close"
				 *       на завершённое соединение
				 *
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			private:
				/**
				 * @brief Конструктор копирования (запрещаем)
				 *
				 */
				QuicServer(const QuicServer &) = delete;
				/**
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 */
				QuicServer & operator = (const QuicServer &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				explicit QuicServer(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~QuicServer() noexcept;
		} quic_server_t;

		/**
		 * @brief Класс модуля клиента транспортного протокола QUIC
		 *
		 * @details Ведёт одно соединение с удалённым сервером поверх асинхронного
		 *          сетевого движка. Билет возобновления, присланный сервером после
		 *          установления соединения, сохраняется модулем и подставляется
		 *          при следующем подключении, поэтому повторное соединение с тем
		 *          же сервером обходится без полного хендшейка (RFC 9001 §4.6)
		 */
		typedef class __AWH_SHARED_EXPORT__ QuicClient : public unit_t {
			private:
				// Идентификатор события клиента
				event::id_t _eid;
				// Идентификатор события интервала таймеров соединения
				event::id_t _tid;
			private:
				// Флаг оповещения приложения об установленном соединении
				bool _connected;
				// Флаг возобновления сессии сохранённым билетом
				bool _resume;
				// Флаг уведомления о перегрузке пути (RFC 9000 §13.4)
				bool _ecn;
			private:
				// Семейство адресов события клиента
				event::family_t _family;
				// Маркировка, установленная на сокете события клиента
				event::ecn_t _marking;
			private:
				// Идентификатор шаблона контекста безопасности
				tls::coder_t::id_t _ctx;
				// Объект кодера транспортной безопасности
				const tls::coder_t * _coder;
			private:
				// Сохранённый билет возобновления сессии (RFC 9001 §4.6)
				string _ticket;
				/**
				 * Сохранённый токен проверки адреса фрейма NEW_TOKEN. Подставляется
				 * в первый пакет следующего соединения и позволяет пропустить обмен
				 * пакетом Retry, сэкономив круг задержки (RFC 9000 §8.1.3)
				 */
				string _token;
				// Адрес удалённого сервера в виде "адрес:порт"
				string _address;
			private:
				// Локальные транспортные параметры соединения
				quic::params::params_t _params;
			private:
				// Объект соединения QUIC
				unique_ptr <quic::connection_t> _connection;
			private:
				/**
				 * @brief Метод получения текущего времени в миллисекундах
				 *
				 * @return текущее время в миллисекундах
				 */
				uint64_t date() const noexcept;
			private:
				/**
				 * @brief Метод обработки принятой датаграммы соединения
				 *
				 * @param eid  идентификатор события клиента
				 * @param data данные датаграммы
				 * @param size размер датаграммы
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод обработки просроченных таймеров соединения
				 *
				 * @param eid    идентификатор события интервала
				 * @param status статус события интервала
				 */
				void tick(const event::id_t eid, const event::status_t status) noexcept;
				/**
				 * @brief Метод обработки завершения подключения к серверу
				 *
				 * @param eid идентификатор события клиента
				 * @param ok  результат подключения к серверу
				 */
				void connected(const event::id_t eid, const bool ok) noexcept;
			private:
				/**
				 * @brief Метод выдачи собранных данных потоков приложения
				 *
				 */
				void process() noexcept;
				/**
				 * @brief Метод оповещения приложения о завершённом соединении
				 *
				 * @note Соединение завершается как удалённым эндпоинтом, так и по
				 *       истечении периода завершения, выдерживаемого после завершения
				 *       соединения самим приложением (RFC 9000 §10.2). Оповещение
				 *       выполняется однократно
				 */
				void complete() noexcept;
				/**
				 * @brief Метод применения маркировки соединения к сокету события клиента
				 *
				 * @param marking требуемая маркировка исходящих датаграмм
				 */
				void mark(const event::ecn_t marking) noexcept;
				/**
				 * @brief Метод отправки готовых исходящих датаграмм соединения
				 *
				 */
				void flush() noexcept;
			public:
				/**
				 * @brief Метод установки шаблона контекста безопасности соединения
				 *
				 * @param coder объект кодера транспортной безопасности
				 * @param ctx   идентификатор шаблона контекста безопасности
				 */
				void context(const tls::coder_t & coder, const tls::coder_t::id_t ctx) noexcept;
				/**
				 * @brief Метод установки уведомления о перегрузке пути (RFC 9000 §13.4)
				 *
				 * @note Исходящие датаграммы помечаются поддержкой ECN, а маркировка
				 *       принятых извлекается из заголовка IP-пакета. Путь, стирающий
				 *       маркировку, соединение выявляет само и маркировку снимает
				 *
				 * @param mode режим уведомления о перегрузке пути
				 */
				void ecn(const bool mode) noexcept;
				/**
				 * @brief Метод установки локальных транспортных параметров соединения (RFC 9000 §7.4)
				 *
				 * @param params локальные транспортные параметры
				 */
				void params(const quic::params::params_t & params) noexcept;
				/**
				 * @brief Метод установки возобновления сессии сохранённым билетом (RFC 9001 §4.6)
				 *
				 * @note Билет сохраняется модулем самостоятельно по завершении
				 *       соединения и подставляется при следующем подключении
				 *
				 * @param mode режим возобновления сессии
				 */
				void resume(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод извлечения сохранённого билета возобновления сессии
				 *
				 * @note Билет переживает соединение и пригоден для восстановления
				 *       между запусками приложения
				 *
				 * @return сериализованный билет возобновления (пусто - билет не получен)
				 */
				const string & session() const noexcept;
				/**
				 * @brief Метод установки сохранённого билета возобновления сессии
				 *
				 * @param session сериализованный билет возобновления
				 */
				void session(string_view session) noexcept;
			public:
				/**
				 * @brief Метод извлечения сохранённого токена проверки адреса (RFC 9000 §8.1.3)
				 *
				 * @note Токен присылается сервером по завершении соединения и сохраняется
				 *       модулем самостоятельно, переживая соединение и пригодный
				 *       к предъявлению между запусками приложения
				 *
				 * @return токен проверки адреса (пусто - токен не получен)
				 */
				const string & token() const noexcept;
				/**
				 * @brief Метод установки сохранённого токена проверки адреса (RFC 9000 §8.1.3)
				 *
				 * @param token токен проверки адреса
				 */
				void token(string_view token) noexcept;
			public:
				/**
				 * @brief Метод подключения к удалённому серверу
				 *
				 * @param family семейство адресов события клиента
				 * @param ip     адрес удалённого сервера
				 * @param port   порт удалённого сервера
				 * @return       идентификатор созданного события клиента
				 */
				event::id_t connect(const event::family_t family, string_view ip, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод открытия потока приложения соединения
				 *
				 * @param mode режим однонаправленного потока
				 * @return     идентификатор открытого потока
				 */
				uint64_t open(const bool mode = false) noexcept;
				/**
				 * @brief Метод отправки данных в поток приложения соединения
				 *
				 * @param sid  идентификатор потока приложения
				 * @param data отправляемые данные
				 * @param fin  флаг завершения потока
				 * @return     результат постановки данных в очередь отправки
				 */
				bool send(const uint64_t sid, string_view data, const bool fin = false) noexcept;
				/**
				 * @brief Метод отправки датаграммы приложения серверу (RFC 9221)
				 *
				 * @note Доставка датаграмм ненадёжна: потерянная датаграмма повторно
				 *       не отправляется, порядок доставки не гарантируется. Отправка
				 *       возможна только когда сервер анонсировал их приём
				 *
				 * @param data данные датаграммы приложения
				 * @return     результат отправки
				 */
				bool datagram(string_view data) noexcept;
				/**
				 * @brief Метод получения предельного размера отправляемой датаграммы (RFC 9221 §3)
				 *
				 * @return предельный размер данных датаграммы в октетах (0 - датаграммы не поддерживаются)
				 */
				size_t datagrams() const noexcept;
				/**
				 * @brief Метод завершения соединения приложением (RFC 9000 §10.2)
				 *
				 * @param code   код ошибки приложения
				 * @param reason человекочитаемая причина завершения
				 */
				void close(const uint64_t code = 0, string_view reason = "") noexcept;
			public:
				/**
				 * @brief Метод получения согласованного ALPN-протокола соединения
				 *
				 * @return согласованный ALPN-протокол
				 */
				tls::coder_t::alpn_t alpn() const noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @note Поддерживаются функции обратного вызова: "open" на установленное
				 *       соединение, "read" на собранные данные потока приложения,
				 *       "datagram" на принятую датаграмму приложения и "close"
				 *       на завершённое соединение
				 *
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			private:
				/**
				 * @brief Конструктор копирования (запрещаем)
				 *
				 */
				QuicClient(const QuicClient &) = delete;
				/**
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 */
				QuicClient & operator = (const QuicClient &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				explicit QuicClient(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~QuicClient() noexcept;
		} quic_client_t;
	};
};

#endif // __AWH_UNIT_QUIC__
