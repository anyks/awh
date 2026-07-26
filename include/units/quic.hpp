/**
 * @file: quic.hpp
 * @date: 2026-07-22
 * @license: LicenseRef-AWH-1.0
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
#include "cluster.hpp"
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
					 * Идентификатор события-приёмника объединения данных (splice): собранные
					 * данные потоков этой сессии перенаправляются в это событие вместо
					 * выдачи приложению (0 - объединение не установлено)
					 */
					event::id_t dest;
					/**
					 * Идентификатор туннельного потока для входящих объединённых данных:
					 * байты, поступающие в эту сессию из события-источника, отправляются
					 * этим потоком приложения (INVALID_STREAM - поток ещё не открыт)
					 */
					uint64_t tunnel;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Session() noexcept;
				} session_t;
				/**
				 * @brief Структура параметров кластера
				 *
				 * @details Параметры кластера задаются при его создании и не могут быть изменены в процессе работы.
				 */
				typedef struct __AWH_SHARED_EXPORT__ ClusterParams {
					// Имя кластера
					string name;
					// Флаг пересоздания процесса при его завершении
					bool rebirth;
					// Максимальное количество процессов в кластере
					uint16_t count;
					// Максимальное число подряд идущих быстрых падений процессов до остановки кластера (0 — без ограничения, по умолчанию 10)
					uint16_t restartLimit;
					// Временное окно «быстрого» (раннего) падения процесса в миллисекундах (по умолчанию 30000)
					uint64_t restartWindow;
					// Режим активации кластера (по умолчанию event::mode_t::DISABLED)
					event::mode_t mode;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit ClusterParams() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~ClusterParams() noexcept = default;
				} cluster_params_t;
			private:
				// Идентификатор события сервера
				event::id_t _eid;
				// Идентификатор события интервала таймеров соединений
				event::id_t _tid;
				/**
				 * Унаследованный до fork идентификатор события сервера. В режиме кластера
				 * дочерний процесс пересоздаёт собственное событие сервера с новым
				 * идентификатором, а фасад продолжает обращаться к унаследованному: значение
				 * сохраняется для приведения унаследованного идентификатора к собственному
				 * (см. actual())
				 */
				event::id_t _inheritedEid;
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
				// Объект работы с кластером
				unique_ptr <cluster_t> _cluster;
			private:
				// Параметры кластера
				cluster_params_t _clusterParams;
			private:
				/**
				 * Идентификаторы дочерних процессов, не получивших порт прослушивания при
				 * раздаче родительским процессом (работают в холостую без сокета сервера).
				 * Заполняется только на системах, где порт выделяется дочернему процессу
				 * монопольно (macOS/Solaris/OpenBSD/NetBSD), и позволяет доотправить порт
				 * такому процессу вручную (clusterIdle()/clusterAssign())
				 */
				unordered_set <pid_t> _idle;
				/**
				 * Порт прослушивания, выделенный каждому дочернему процессу при раздаче
				 * родительским процессом. Позволяет автоматически вернуть возрождённому
				 * (после падения) дочернему процессу тот же порт, что был у него до гибели
				 */
				unordered_map <pid_t, uint16_t> _ports;
			private:
				/**
				 * Параметры прослушивания, кэшируемые для пересоздания сокета в дочернем
				 * процессе кластера. Фасад настраивает событие сервера в родительском
				 * процессе, а каждый дочерний процесс поднимает собственный сокет после
				 * fork, поэтому адрес, порт и размер очереди сохраняются заранее
				 */
				// Тип адреса прослушивания (IPv4 или IPv6)
				event::address_t _listenType;
				// Флаг ожидания дочерним процессом выделенного родительским процессом порта
				bool _awaitingPort;
				// Максимальный размер очереди ожидания соединений
				uint32_t _backlog;
				// Порт прослушивания сервера
				uint16_t _listenPort;
				// Начальный порт диапазона выделения портов дочерним процессам (0 - использовать порт прослушивания)
				uint16_t _portBegin;
				// Конечный порт диапазона выделения портов дочерним процессам (0 - использовать порт прослушивания)
				uint16_t _portEnd;
				// Хост прослушивания сервера
				string _listenHost;
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
				/**
				 * @brief Метод установки адреса удалённого эндпоинта соединению из движка
				 *
				 * @note Адрес источника извлекается штатной структурой сетевого адреса
				 *       фреймворка net::addr_t, порт - штатным методом движка; смена
				 *       адреса при установленном соединении означает миграцию пути
				 *       (RFC 9000 §9)
				 *
				 * @param oid        идентификатор события сессии
				 * @param connection соединение, которому устанавливается адрес эндпоинта
				 */
				void endpoint(const event::id_t oid, quic::connection_t * connection) const noexcept;
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
				 * @param oid идентификатор события сессии
				 */
				void process(const event::id_t oid) noexcept;
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
				 * @brief Метод отправки объединённых данных в туннельный поток сессии
				 *
				 * @note Байты, поступившие из события-источника объединения (splice) - будь
				 *       то обычный сокет через сетевой движок либо другая QUIC-сессия -
				 *       отправляются туннельным потоком сессии-приёмника с их шифрованием
				 *       на уровне соединения (RFC 9000 §2.1)
				 *
				 * @param oid  идентификатор события сессии-приёмника
				 * @param data данные для отправки в туннельный поток
				 * @param size размер данных для отправки
				 * @return     результат постановки данных в очередь отправки
				 */
				bool inject(const event::id_t oid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод перенаправления собранных данных сессии в событие-приёмник объединения
				 *
				 * @note Если приёмник - другая QUIC-сессия, данные перешифровываются в её
				 *       туннельный поток; если обычное событие движка (TCP/UDP) - отправляются
				 *       сырыми байтами
				 *
				 * @param dest идентификатор события-приёмника объединения данных
				 * @param data данные для перенаправления
				 */
				void forward(const event::id_t dest, string_view data) noexcept;
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
			private:
				/**
				 * @brief Метод обработки события пересоздания процесса кластера
				 *
				 * @param old старый идентификатор процесса
				 * @param pid текущий идентификатор процесса
				 */
				void rebase(const pid_t old, const pid_t pid) noexcept;
				/**
				 * @brief Метод получения события завершения работы процесса кластера
				 *
				 * @param pid    идентификатор процесса
				 * @param signal сигнал с которым завершился процесс
				 */
				void exit(const pid_t pid, const int32_t signal) noexcept;
				/**
				 * @brief Метод обработки события отправки сообщения процессу кластера
				 *
				 * @param pid  идентификатор процесса
				 * @param size размер отправленного сообщения
				 */
				void sending(const pid_t pid, const size_t size) noexcept;
				/**
				 * @brief Метод получения событий активации/деактивации кластера
				 *
				 * @note На событие START дочернего процесса поднимается независимый
				 *       сервер QUIC: сокет создаётся и привязывается уже в самом
				 *       дочернем процессе, поэтому каждый воркер владеет собственным
				 *       сокетом (SO_REUSEPORT на общем порту для Linux/FreeBSD либо
				 *       выделенный из диапазона порт для прочих систем)
				 *
				 * @param pid   идентификатор процесса
				 * @param event флаг события кластера
				 */
				void cluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept;
				/**
				 * @brief Метод обработки события получения сообщения от процесса кластера
				 *
				 * @param pid  идентификатор процесса
				 * @param data данные полученного сообщения
				 * @param size размер данных полученного сообщения
				 */
				void message(const pid_t pid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод обработки событий изменения статуса процесса кластера
				 *
				 * @param pid    идентификатор процесса
				 * @param status новый статус процесса кластера
				 */
				void status(const pid_t pid, const event::status_t status) noexcept;
				/**
				 * @brief Метод обработки событий ошибок кластера
				 *
				 * @param pid         идентификатор процесса
				 * @param error       тип ошибки
				 * @param description описание ошибки
				 */
				void error(const pid_t pid, const event::error_t error, const string & description) noexcept;
				/**
				 * @brief Метод обработки события доступности/недоступности очереди исходящих сообщений кластера
				 *
				 * @param pid    идентификатор процесса
				 * @param status статус доступности очереди
				 * @param size   размер доступных данных очереди
				 */
				void available(const pid_t pid, const event::status_t status, const size_t size) noexcept;
				/**
				 * @brief Метод отправки выделенного порта прослушивания дочернему процессу кластера
				 *
				 * @param pid  идентификатор дочернего процесса
				 * @param port выделяемый порт прослушивания
				 * @return     результат отправки
				 */
				bool sendPort(const pid_t pid, const uint16_t port) noexcept;
			private:
				/**
				 * @brief Метод запуска/остановки работы сервера
				 *
				 * @param status статус запуска/остановки сервера
				 */
				void launch(const event::status_t status) noexcept;
			public:
				/**
				 * @brief Метод проверки актуальности события сервера
				 *
				 * @note Событие актуально, если это событие сервера либо активная сессия
				 *       соединения (адресуемая идентификатором события сессии)
				 *
				 * @param eid идентификатор события
				 * @return    результат проверки актуальности события
				 */
				bool isActual(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод приведения переданного фасадом идентификатора события сервера к актуальному
				 *
				 * @note В режиме кластера дочерний процесс пересоздаёт собственное событие
				 *       сервера с новым идентификатором (см. message()), а фасад продолжает
				 *       обращаться к унаследованному до fork идентификатору. Метод подменяет
				 *       унаследованный идентификатор на собственный идентификатор события
				 *       сервера дочернего процесса; активная сессия с таким же идентификатором
				 *       (при повторном выделении идентификатора) имеет приоритет
				 *
				 * @param eid идентификатор события, переданный фасадом
				 * @return    актуальный идентификатор события сервера
				 */
				event::id_t actual(const event::id_t eid) const noexcept;
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
				 * @brief Метод создания события сервера QUIC поверх UDP
				 *
				 * @note Транспорт QUIC работает поверх дейтаграммного UDP-сокета: тип и
				 *       протокол принудительно приводятся к DATAGRAM/UDP независимо от
				 *       переданных значений (RFC 9000)
				 *
				 * @param family   семейство адресов события сервера
				 * @param type     тип события (игнорируется, приводится к DATAGRAM)
				 * @param protocol протокол события (игнорируется, приводится к UDP)
				 * @return         идентификатор созданного события сервера
				 */
				event::id_t issue(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
			public:
				/**
				 * @brief Метод фиксации настроек события сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    результат выполнения фиксации
				 */
				bool commit(const event::id_t eid) noexcept;
				/**
				 * @brief Метод запуска работы события сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    результат выполнения запуска
				 */
				bool launch(const event::id_t eid) noexcept;
				/**
				 * @brief Метод прослушивания порта сервера для приёма входящих соединений
				 *
				 * @param eid идентификатор события сервера
				 * @param max максимальный размер очереди ожидания соединений
				 * @return    результат выполнения прослушивания
				 */
				bool listen(const event::id_t eid, const uint32_t max) noexcept;
				/**
				 * @brief Метод приостановки работы события сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    результат выполнения приостановки работы
				 */
				bool pause(const event::id_t eid) noexcept;
				/**
				 * @brief Метод возобновления работы события сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    результат выполнения возобновления работы
				 */
				bool resume(const event::id_t eid) noexcept;
				/**
				 * @brief Метод получения данных от клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат получения данных
				 */
				bool recv(const event::id_t eid) noexcept;
				/**
				 * @brief Метод отправки данных клиенту
				 *
				 * @note Отправляет прикладные данные сессии соединения потоком по
				 *       умолчанию через открытие двунаправленного потока (RFC 9000 §2.1);
				 *       для явного выбора потока используется перегрузка send(oid, sid, data, fin)
				 *
				 * @param eid    идентификатор события сессии
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт, поставленных в очередь отправки
				 */
				size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
				/**
				 * @brief Метод объединения потоков данных между двумя событиями
				 *
				 * @note Для транспорта QUIC объединение на уровне сокета не поддерживается:
				 *       данные шифруются на уровне соединения, поэтому метод всегда
				 *       возвращает отрицательный результат
				 *
				 * @param eid  идентификатор события-источника
				 * @param dest идентификатор события-приёмника
				 * @return     результат объединения
				 */
				bool splice(const event::id_t eid, const event::id_t dest) noexcept;
				/**
				 * @brief Метод установки контекста события клиента
				 *
				 * @note Для транспорта QUIC контекст сессии ведётся самим модулем по
				 *       идентификатору события сессии, поэтому внешний контекст не
				 *       устанавливается
				 *
				 * @param eid идентификатор события клиента
				 * @param ctx контекст события клиента
				 * @return    результат выполнения установки
				 */
				bool setContext(const event::id_t eid, void * ctx) noexcept;
				/**
				 * @brief Метод уничтожения события сервера
				 *
				 * @param eid идентификатор события для уничтожения
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод получения опций события сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    опции события сервера
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки опций события сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param options опции события сервера для установки
				 * @return        результат выполнения установки
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * @brief Метод установки опции события сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param option опция события сервера для установки
				 * @param mode   режим установки опции события сервера
				 * @return       результат выполнения установки
				 */
				bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
				 *
				 * @param eid идентификатор события сервера
				 * @return    метаданные последнего принятого дейтаграммного пакета
				 */
				net::dgram_info_t getTrafficInfo(const event::id_t eid) const noexcept;
			public:
				/**
				 * @brief Метод получения количества хопов последнего принятого пакета
				 *
				 * @param eid идентификатор события сервера
				 * @return    количество хопов последнего принятого пакета
				 */
				uint8_t getCountHops(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки количества хопов последнего принятого пакета
				 *
				 * @param eid  идентификатор события сервера
				 * @param hops количество хопов последнего принятого пакета
				 * @return     результат выполнения установки
				 */
				bool setCountHops(const event::id_t eid, const uint8_t hops) noexcept;
				/**
				 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid идентификатор события сервера
				 * @return    максимальное количество хопов
				 */
				event::hops_t getHops(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid  идентификатор события сервера
				 * @param hops максимальное количество хопов
				 * @return     результат работы функции
				 */
				bool setHops(const event::id_t eid, const event::hops_t hops) noexcept;
			public:
				/**
				 * @brief Метод получения сетевого интерфейса сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    сетевой интерфейс сервера
				 */
				string getIface(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки сетевого интерфейса сервера
				 *
				 * @param eid  идентификатор события сервера
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 */
				bool setIface(const event::id_t eid, string_view name) noexcept;
			public:
				/**
				 * @brief Метод получения семейства адресов события сервера
				 *
				 * @note Переопределяет базовый метод для приведения унаследованного дочерним
				 *       процессом кластера идентификатора события к собственному (см. actual())
				 *
				 * @param eid идентификатор события сервера
				 * @return    семейство адресов события сервера
				 */
				event::family_t family(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод получения порта сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    порт сервера
				 */
				uint16_t getPort(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки порта сервера
				 *
				 * @param eid  идентификатор события сервера
				 * @param port порт сервера для установки
				 * @return     результат выполнения установки
				 */
				bool setPort(const event::id_t eid, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод получения адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @return        значение адреса сервера
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
				/**
				 * @brief Метод установки адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   значение адреса сервера
				 * @return        результат выполнения установки
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept;
				/**
				 * @brief Метод установки адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   структура сетевого адреса сервера
				 * @return        результат выполнения установки
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
				/**
				 * @brief Метод получения адреса сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param address тип адреса сервера
				 * @param value   объект для извлечения адреса сервера
				 * @return        результат выполнения извлечения адреса сервера
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * @brief Метод получения MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события сервера
				 * @return    MTU сетевого интерфейса
				 */
				uint16_t getMaximumTransmissionUnit(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события сервера
				 * @param mtu размер MTU интерфейса
				 * @return    результат установки MTU сетевого интерфейса
				 */
				bool setMaximumTransmissionUnit(const event::id_t eid, const uint16_t mtu) const noexcept;
				/**
				 * @brief Метод получения режима обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid идентификатор события сервера
				 * @return    текущий режим обнаружения MTU
				 */
				event::mtu_discover_t getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки режима обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid  идентификатор события сервера
				 * @param mode режим обнаружения максимального размера пакета (MTU)
				 * @return     результат работы функции
				 */
				bool setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept;
			public:
				/**
				 * @brief Метод получения режима трансляции пакетов сервера
				 *
				 * @param eid идентификатор события сервера
				 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
				 */
				event::delivery_mode_t getDelivery(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки режима трансляции пакетов сервера
				 *
				 * @param eid      идентификатор события сервера
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @return         результат выполнения установки
				 */
				bool setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept;
			public:
				/**
				 * @brief Метод получения размера буфера сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param action тип действия сервера
				 * @return       размер буфера сервера
				 */
				size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки размера буфера сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param action тип действия сервера
				 * @param size   размер буфера сервера
				 * @return       результат выполнения установки
				 */
				bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод получения режима использования таймаута на чтение события
				 *
				 * @param eid идентификатор события
				 * @return    режим использования таймаута на чтение события
				 */
				event::usage_t getUsageReadTimeout(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки режима использования таймаута на чтение события
				 *
				 * @param eid   идентификатор события
				 * @param usage режим использования таймаута на чтение события (reusable или disposable)
				 */
				void setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept;
				/**
				 * @brief Метод получения таймаута сервера
				 *
				 * @param eid    идентификатор события сервера
				 * @param action тип действия сервера
				 * @return       значение таймаута в миллисекундах
				 */
				uint32_t getTimeout(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки таймаута сервера
				 *
				 * @param eid     идентификатор события сервера
				 * @param action  тип действия сервера
				 * @param timeout значение таймаута в миллисекундах
				 */
				void setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept;
			public:
				/**
				 * @brief Метод установки пропускной способности сервера
				 *
				 * @param eid       идентификатор события сервера
				 * @param limiting  режим ограничения пропускной способности сервера (egress или ingress)
				 * @param bandwidth пропускная способность сервера для установки
				 * @return          результат выполнения установки
				 */
				bool bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept;
				/**
				 * @brief Метод установки параметров keep-alive для сервера
				 *
				 * @param eid   идентификатор события сервера
				 * @param cnt   количество пакетов keep-alive
				 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
				 * @param intvl интервал между пакетами keep-alive в секундах
				 * @return      результат выполнения установки
				 */
				bool keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid идентификатор события сервера
				 * @return    значение DSCP
				 */
				event::dscp_t getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid  идентификатор события сервера
				 * @param dscp значение DSCP
				 * @return     результат работы функции
				 */
				bool setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept;
			public:
				/**
				 * @brief Метод активации/деактивации мультикаст-группы
				 *
				 * @param eid    идентификатор события сервера
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, string_view group, string_view source, const uint16_t port) noexcept;
				/**
				 * @brief Метод активации/деактивации мультикаст-группы
				 *
				 * @param eid    идентификатор события сервера
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод установки названия кластера
				 *
				 * @param name название кластера для установки
				 */
				void clusterName(string_view name) noexcept;
			public:
				/**
				 * @brief Метод получения семейства кластера
				 *
				 * @return семейство к которому принадлежит кластер (MASTER или CHILDREN)
				 */
				cluster_t::family_t clusterFamily() const noexcept;
			public:
				/**
				 * @brief Метод получения режима активации кластера
				 *
				 * @return режим активации кластера
				 */
				event::mode_t clusterMode() const noexcept;
				/**
				 * @brief Метод установки режима работы кластера
				 *
				 * @param mode режим активации/деактивации кластера
				 */
				void clusterMode(const event::mode_t mode) noexcept;
			public:
				/**
				 * @brief Метод получения максимального количества процессов
				 *
				 * @return максимальное количество процессов
				 */
				uint16_t clusterCount() const noexcept;
				/**
				 * @brief Метод установки максимального количества процессов
				 *
				 * @param count максимальное количество процессов
				 */
				void clusterCount(const uint16_t count) noexcept;
			public:
				/**
				 * @brief Метод получения списка дочерних процессов
				 *
				 * @return список дочерних процессов
				 */
				unordered_set <pid_t> clusterWorkers() const noexcept;
			public:
				/**
				 * @brief Метод установки диапазона портов для выделения дочерним процессам кластера
				 *
				 * @note Родительский процесс раздаёт порты из диапазона дочерним процессам,
				 *       каждый из которых поднимает собственный сокет сервера. На Linux/FreeBSD
				 *       порт может повторяться (несколько процессов делят его через SO_REUSEPORT),
				 *       на прочих системах порт выделяется дочернему процессу монопольно.
				 *       Пустой диапазон означает использование единственного порта прослушивания
				 *
				 * @param begin начальный порт диапазона (0 - использовать порт прослушивания)
				 * @param end   конечный порт диапазона (0 - использовать порт прослушивания)
				 */
				void clusterRange(const uint16_t begin, const uint16_t end) noexcept;
			public:
				/**
				 * @brief Метод получения списка дочерних процессов, не получивших порт прослушивания
				 *
				 * @note На системах, где порт выделяется дочернему процессу монопольно
				 *       (macOS/Solaris/OpenBSD/NetBSD), при нехватке портов диапазона часть
				 *       дочерних процессов остаётся без сокета сервера. Их идентификаторы
				 *       возвращаются здесь, чтобы приложение могло доотправить им порт
				 *       вручную методом clusterAssign()
				 *
				 * @return список идентификаторов дочерних процессов, работающих в холостую
				 */
				unordered_set <pid_t> clusterIdle() const noexcept;
				/**
				 * @brief Метод отправки порта прослушивания конкретному дочернему процессу кластера
				 *
				 * @note Позволяет вручную поднять сервер на дочернем процессе, которому при
				 *       автоматической раздаче порт не достался (см. clusterIdle()). Дочерний
				 *       процесс поднимает собственный сокет на полученном порту
				 *
				 * @param pid  идентификатор дочернего процесса
				 * @param port порт прослушивания для дочернего процесса
				 * @return     результат отправки порта дочернему процессу
				 */
				bool clusterAssign(const pid_t pid, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения родительскому процессу
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 */
				size_t clusterSend(const void * buffer, const size_t size) noexcept;
				/**
				 * @brief Метод отправки сообщения дочернему процессу
				 *
				 * @param pid    идентификатор процесса для получения сообщения
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 */
				size_t clusterSend(const pid_t pid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения всем дочерним процессам
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 */
				size_t clusterBroadcast(const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод установки флага автоматического возрождения процессов
				 *
				 * @param mode флаг возрождения процессов
				 */
				void clusterRebirth(const bool mode) noexcept;
				/**
				 * @brief Метод установки параметров защиты от цикла перезапусков процессов кластера
				 *
				 * @param limit  максимальное число подряд идущих быстрых падений до остановки кластера (0 — без ограничения)
				 * @param window временное окно «быстрого» (раннего) падения процесса в миллисекундах
				 */
				void clusterRebirthLimit(const uint16_t limit, const uint64_t window) noexcept;
			public:
				/**
				 * @brief Метод получения типа протокола передачи данных между воркерами
				 *
				 * @return тип протокола передачи данных между воркерами
				 */
				event::type_t clusterGetTypeEventMessage() const noexcept;
				/**
				 * @brief Метод установки типа протокола передачи данных между воркерами
				 *
				 * @param type тип протокола передачи данных между воркерами для установки
				 */
				void clusterSetTypeEventMessage(const event::type_t type) noexcept;
			public:
				/**
				 * @brief Метод получения размера буфера события
				 *
				 * @param pid    идентификатор процесса
				 * @param action тип действия события
				 * @return       размер буфера события
				 */
				size_t clusterGetBufferSize(const pid_t pid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки размера буфера события
				 *
				 * @param pid    идентификатор процесса
				 * @param action тип действия события
				 * @param size   размер буфера события
				 * @return       результат выполнения установки
				 */
				bool clusterSetBufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод остановки сервера
				 *
				 */
				void stop() noexcept;
				/**
				 * @brief Метод запуска сервера
				 *
				 */
				void start() noexcept;
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
				// Флаг выполненного оповещения приложения о завершённом соединении
				bool _notified;
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
				/**
				 * Сохранённый токен проверки адреса фрейма NEW_TOKEN. Подставляется
				 * в первый пакет следующего соединения и позволяет пропустить обмен
				 * пакетом Retry, сэкономив круг задержки (RFC 9000 §8.1.3)
				 */
				string _token;
			private:
				// Локальные транспортные параметры соединения
				quic::params::params_t _params;
			private:
				// Порт удалённого сервера (устанавливается фасадом до подключения)
				uint16_t _targetPort;
				// Структура сетевого адреса удалённого сервера (устанавливается фасадом до подключения)
				unique_ptr <net::addr_t> _target;
			private:
				// Объект соединения QUIC
				unique_ptr <quic::connection_t> _connection;
			private:
				/**
				 * Идентификатор события-приёмника объединения данных (splice): собранные
				 * данные потоков соединения перенаправляются в это событие вместо выдачи
				 * приложению (0 - объединение не установлено)
				 */
				event::id_t _dest;
				/**
				 * Идентификатор туннельного потока для входящих объединённых данных:
				 * байты, поступающие в соединение из события-источника, отправляются
				 * этим потоком приложения (INVALID_STREAM - поток ещё не открыт)
				 */
				uint64_t _tunnel;
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
				/**
				 * @brief Метод отправки объединённых данных в туннельный поток соединения
				 *
				 * @note Байты, поступившие из события-источника объединения (splice),
				 *       отправляются туннельным потоком соединения с их шифрованием
				 *       на уровне соединения (RFC 9000 §2.1)
				 *
				 * @param data данные для отправки в туннельный поток
				 * @param size размер данных для отправки
				 * @return     результат постановки данных в очередь отправки
				 */
				bool inject(const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод перенаправления собранных данных соединения в событие-приёмник объединения
				 *
				 * @param data данные для перенаправления
				 */
				void forward(string_view data) noexcept;
			private:
				/**
				 * @brief Метод проверки актуальности события клиента
				 *
				 * @param eid идентификатор события
				 * @return    результат проверки актуальности события
				 */
				bool isActual(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод запуска/остановки работы клиента
				 *
				 * @param status статус запуска/остановки клиента
				 */
				void launch(const event::status_t status) noexcept;
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
				 * @brief Метод проверки принятия ранних данных удалённым сервером (RFC 9001 §4.6.2)
				 *
				 * @note Сервер вправе отказать в ранних данных, и тогда отправленное
				 *       ими содержимое передаётся заново после хендшейка. Проверка
				 *       нужна приложению, которому важно знать, состоялся ли 0-RTT
				 *
				 * @return результат проверки
				 */
				bool early() const noexcept;
			public:
				/**
				 * @brief Метод создания события клиента QUIC поверх UDP
				 *
				 * @note Транспорт QUIC работает поверх дейтаграммного UDP-сокета: тип и
				 *       протокол принудительно приводятся к DATAGRAM/UDP независимо от
				 *       переданных значений (RFC 9000)
				 *
				 * @param family   семейство адресов события клиента
				 * @param type     тип события (игнорируется, приводится к DATAGRAM)
				 * @param protocol протокол события (игнорируется, приводится к UDP)
				 * @return         идентификатор созданного события клиента
				 */
				event::id_t issue(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
			public:
				/**
				 * @brief Метод фиксации настроек события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения фиксации
				 */
				bool commit(const event::id_t eid) noexcept;
				/**
				 * @brief Метод запуска работы события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения запуска
				 */
				bool launch(const event::id_t eid) noexcept;
				/**
				 * @brief Метод приостановки работы события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения приостановки работы
				 */
				bool pause(const event::id_t eid) noexcept;
				/**
				 * @brief Метод возобновления работы события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения возобновления работы
				 */
				bool resume(const event::id_t eid) noexcept;
				/**
				 * @brief Метод отключения клиента от удалённого сервера
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения отключения
				 */
				bool disconnect(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод подключения клиента к удалённому серверу
				 *
				 * @note Создаёт соединение QUIC на шаблоне контекста безопасности и
				 *       начинает хендшейк (RFC 9001). Адрес и порт удалённого сервера
				 *       берутся из ранее установленной штатной структуры сетевого адреса
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат выполнения подключения
				 */
				bool connect(const event::id_t eid) noexcept;
				/**
				 * @brief Метод подключения клиента к удалённому серверу
				 *
				 * @param ids список идентификаторов событий для подключения
				 * @return    результат выполнения подключения
				 */
				bool connect(const vector <event::id_t> & ids) noexcept;
				/**
				 * @brief Метод получения данных от удалённого сервера
				 *
				 * @param eid идентификатор события клиента
				 * @return    результат получения данных
				 */
				bool recv(const event::id_t eid) noexcept;
				/**
				 * @brief Метод отправки данных удалённому серверу
				 *
				 * @note Отправляет прикладные данные потоком по умолчанию через открытие
				 *       двунаправленного потока (RFC 9000 §2.1); для явного выбора потока
				 *       используется перегрузка send(sid, data, fin)
				 *
				 * @param eid    идентификатор события клиента
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт, поставленных в очередь отправки
				 */
				size_t send(const event::id_t eid, const void * buffer, const size_t size) noexcept;
				/**
				 * @brief Метод объединения потоков данных между двумя событиями
				 *
				 * @note Для транспорта QUIC объединение на уровне сокета не поддерживается:
				 *       данные шифруются на уровне соединения, поэтому метод всегда
				 *       возвращает отрицательный результат
				 *
				 * @param eid  идентификатор события-источника
				 * @param dest идентификатор события-приёмника
				 * @return     результат объединения
				 */
				bool splice(const event::id_t eid, const event::id_t dest) noexcept;
				/**
				 * @brief Метод уничтожения события клиента
				 *
				 * @param eid идентификатор события для уничтожения
				 */
				void destroy(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод получения опций события клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    опции события клиента
				 */
				uint16_t getOptions(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки опций события клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param options опции события клиента для установки
				 * @return        результат выполнения установки
				 */
				bool setOptions(const event::id_t eid, const uint16_t options) noexcept;
				/**
				 * @brief Метод установки опции события клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param option опция события клиента для установки
				 * @param mode   режим установки опции события клиента
				 * @return       результат выполнения установки
				 */
				bool setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
				 *
				 * @param eid идентификатор события клиента
				 * @return    метаданные последнего принятого дейтаграммного пакета
				 */
				net::dgram_info_t getTrafficInfo(const event::id_t eid) const noexcept;
			public:
				/**
				 * @brief Метод получения количества хопов последнего принятого пакета
				 *
				 * @param eid идентификатор события клиента
				 * @return    количество хопов последнего принятого пакета
				 */
				uint8_t getCountHops(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки количества хопов последнего принятого пакета
				 *
				 * @param eid  идентификатор события клиента
				 * @param hops количество хопов последнего принятого пакета
				 * @return     результат выполнения установки
				 */
				bool setCountHops(const event::id_t eid, const uint8_t hops) noexcept;
				/**
				 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid идентификатор события клиента
				 * @return    максимальное количество хопов
				 */
				event::hops_t getHops(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
				 *
				 * @param eid  идентификатор события клиента
				 * @param hops максимальное количество хопов
				 * @return     результат работы функции
				 */
				bool setHops(const event::id_t eid, const event::hops_t hops) noexcept;
			public:
				/**
				 * @brief Метод получения сетевого интерфейса клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    сетевой интерфейс клиента
				 */
				string getIface(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки сетевого интерфейса клиента
				 *
				 * @param eid  идентификатор события клиента
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 */
				bool setIface(const event::id_t eid, string_view name) noexcept;
			public:
				/**
				 * @brief Метод получения внутреннего порта события
				 *
				 * @param eid идентификатор события
				 * @return    внутренний порт события
				 */
				uint16_t getSourcePort(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки внутреннего порта события
				 *
				 * @param eid  идентификатор события
				 * @param port внутренний порт события
				 * @return     результат выполнения установки
				 */
				bool setSourcePort(const event::id_t eid, const uint16_t port) noexcept;
				/**
				 * @brief Метод получения порта удалённого сервера
				 *
				 * @param eid идентификатор события клиента
				 * @return    порт удалённого сервера
				 */
				uint16_t getTargetPort(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки порта удалённого сервера
				 *
				 * @param eid  идентификатор события клиента
				 * @param port порт удалённого сервера для установки
				 * @return     результат выполнения установки
				 */
				bool setTargetPort(const event::id_t eid, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid идентификатор события клиента
				 * @return    адрес хоста целевой машины
				 */
				string getTarget(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 */
				bool setTarget(const event::id_t eid, string_view target) noexcept;
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target структура сетевого адреса хоста целевой машины
				 * @return       результат выполнения установки
				 */
				bool setTarget(const event::id_t eid, const net::addr_t * target) noexcept;
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param eid    идентификатор события клиента
				 * @param target объект для извлечения адреса хоста целевой машины
				 * @return       результат выполнения извлечения адреса хоста целевой машины
				 */
				bool getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept;
			public:
				/**
				 * @brief Метод получения адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @return        значение адреса клиента
				 */
				string getAddress(const event::id_t eid, const event::address_t address) const noexcept;
				/**
				 * @brief Метод установки адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @param value   значение адреса клиента
				 * @return        результат выполнения установки
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept;
				/**
				 * @brief Метод установки адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @param value   структура сетевого адреса клиента
				 * @return        результат выполнения установки
				 */
				bool setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept;
				/**
				 * @brief Метод получения адреса клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param address тип адреса клиента
				 * @param value   объект для извлечения адреса клиента
				 * @return        результат выполнения извлечения адреса клиента
				 */
				bool getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
			public:
				/**
				 * @brief Метод получения MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события клиента
				 * @return    MTU сетевого интерфейса
				 */
				uint16_t getMaximumTransmissionUnit(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки MTU сетевого интерфейса
				 *
				 * @param eid идентификатор события клиента
				 * @param mtu размер MTU интерфейса
				 * @return    результат установки MTU сетевого интерфейса
				 */
				bool setMaximumTransmissionUnit(const event::id_t eid, const uint16_t mtu) const noexcept;
				/**
				 * @brief Метод получения режима обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid идентификатор события клиента
				 * @return    текущий режим обнаружения MTU
				 */
				event::mtu_discover_t getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки режима обнаружения максимального размера пакета (MTU)
				 *
				 * @param eid  идентификатор события клиента
				 * @param mode режим обнаружения максимального размера пакета (MTU)
				 * @return     результат работы функции
				 */
				bool setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept;
			public:
				/**
				 * @brief Метод получения режима трансляции пакетов клиента
				 *
				 * @param eid идентификатор события клиента
				 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
				 */
				event::delivery_mode_t getDelivery(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки режима трансляции пакетов клиента
				 *
				 * @param eid      идентификатор события клиента
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @return         результат выполнения установки
				 */
				bool setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept;
			public:
				/**
				 * @brief Метод получения размера буфера клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @return       размер буфера клиента
				 */
				size_t getBufferSize(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки размера буфера клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @param size   размер буфера клиента
				 * @return       результат выполнения установки
				 */
				bool setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод получения режима использования таймаута на чтение события
				 *
				 * @param eid идентификатор события
				 * @return    режим использования таймаута на чтение события
				 */
				event::usage_t getUsageReadTimeout(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки режима использования таймаута на чтение события
				 *
				 * @param eid   идентификатор события
				 * @param usage режим использования таймаута на чтение события (reusable или disposable)
				 */
				void setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept;
				/**
				 * @brief Метод получения таймаута клиента
				 *
				 * @param eid    идентификатор события клиента
				 * @param action тип действия клиента
				 * @return       значение таймаута в миллисекундах
				 */
				uint32_t getTimeout(const event::id_t eid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки таймаута клиента
				 *
				 * @param eid     идентификатор события клиента
				 * @param action  тип действия клиента
				 * @param timeout значение таймаута в миллисекундах
				 */
				void setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept;
			public:
				/**
				 * @brief Метод установки пропускной способности клиента
				 *
				 * @param eid       идентификатор события клиента
				 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
				 * @param bandwidth пропускная способность клиента для установки
				 * @return          результат выполнения установки
				 */
				bool bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept;
				/**
				 * @brief Метод установки параметров keep-alive для клиента
				 *
				 * @param eid   идентификатор события клиента
				 * @param cnt   количество пакетов keep-alive
				 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
				 * @param intvl интервал между пакетами keep-alive в секундах
				 * @return      результат выполнения установки
				 */
				bool keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid идентификатор события клиента
				 * @return    значение DSCP
				 */
				event::dscp_t getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param eid  идентификатор события клиента
				 * @param dscp значение DSCP
				 * @return     результат работы функции
				 */
				bool setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept;
			public:
				/**
				 * @brief Метод активации/деактивации мультикаст-группы
				 *
				 * @param eid    идентификатор события клиента
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, string_view group, string_view source, const uint16_t port) noexcept;
				/**
				 * @brief Метод активации/деактивации мультикаст-группы
				 *
				 * @param eid    идентификатор события клиента
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 */
				bool membership(const event::id_t eid, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод остановки клиента
				 *
				 */
				void stop() noexcept;
				/**
				 * @brief Метод запуска клиента
				 *
				 */
				void start() noexcept;
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
