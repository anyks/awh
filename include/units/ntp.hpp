/**
 * @file: ntp.hpp
 * @date: 2026-03-05
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля NTP-клиента — класс unit::NTP,
 *        выполняющий синхронизацию времени с пулом NTP-серверов,
 *        расчёт смещения относительно локальных часов и контроль таймаутов запросов
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Защита от повторного включения заголовочного файла
 */
#ifndef __AWH_UNIT_NTP__
#define __AWH_UNIT_NTP__

/**
 * Подключаем заголовочный файл проекта
 */
#include "unit.hpp"

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
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * @brief Класс NTP-клиента
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ NTP : public unit_t {
			public:
				/**
				 * @brief Версии протокола NTP
				 *
				 */
				enum class version_t : uint8_t {
					V1 = 0x01, // Версия 1
					V2 = 0x02, // Версия 2
					V3 = 0x03, // Версия 3
					V4 = 0x04  // Версия 4 (наиболее распространённая)
				};
			private:
				/**
				 * @brief Класс для управления списком NTP-серверов
				 *
				 * @details Содержит методы для инициализации, сброса и получения текущего NTP-сервера из списка.
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ Servers {
					private:
						// Индекс текущего NTP-сервера для выполнения запроса IPv4 (для раунд-робин распределения нагрузки)
						size_t _indexIPv4;
						// Индекс текущего NTP-сервера для выполнения запроса IPv6 (для раунд-робин распределения нагрузки)
						size_t _indexIPv6;
					private:
						// Флаг инициализации списка NTP-серверов IPv4
						bool _initializedIPv4;
						// Флаг инициализации списка NTP-серверов IPv6
						bool _initializedIPv6;
					private:
						// Список NTP-серверов для выполнения запросов (IPv4)
						vector <unique_ptr <net::addr_t>> _ipv4;
						// Список NTP-серверов для выполнения запросов (IPv6)
						vector <unique_ptr <net::addr_t>> _ipv6;
					public:
						/**
						 * @brief Метод инициализации списка NTP-серверов из переменных окружения или стандартных значений
						 *
						 */
						void init() noexcept;
					public:
						/**
						 * @brief Метод сброса списка NTP-серверов
						 *
						 * @param family семейство IP-адресов IPv4/IPv6
						 *
						 */
						void reset(const event::family_t family) noexcept;
					public:
						/**
						 * @brief Метод получения текущего NTP-сервера
						 *
						 * @param family семейство IP-адресов IPv4/IPv6
						 * @return       объект NTP-сервера для выполнения запроса
						 *
						 */
						const net::addr_t * get(const event::family_t family) noexcept;
					public:
						/**
						 * @brief Метод добавления NTP-сервера в список
						 *
						 * @param server объект NTP-сервера для добавления в список
						 *
						 */
						void push(const net::addr_t * server) noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Servers() noexcept;
						/**
						 * @brief Деструктор
						 *
						 */
						~Servers() noexcept = default;
				} servers_t;
				/**
				 * @brief Структура для управления состоянием NTP-клиента
				 *
				 * @details Содержит параметры для выполнения запросов к NTP-серверам и хранения состояния NTP-клиента.
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Client {
					// Префикс для переменных окружения (по умолчанию: AWH_SHORT_NAME)
					string prefix;
					// Порт NTP-сервера (по умолчанию: 123)
					uint16_t port;
					// Задержка ожидания ответа от NTP-сервера (в миллисекундах, по умолчанию: 5000)
					uint32_t delay;
					// Идентификатор события для NTP-клиента
					event::id_t eid;
					// Адрес NTP-сервера для выполнения запросов
					servers_t servers;
					// Адрес сети для выполнения запроса
					unique_ptr <net::addr_t> source;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Client() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~Client() noexcept = default;
				} client_t;
				/**
				 * @brief Структура для управления передачей данных при выполнении запросов NTP-клиента
				 *
				 * @details Содержит параметры для управления передачей данных при выполнении запросов NTP-клиента.
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Transfer {
					// Флаг ожидания ответа от NTP-сервера
					bool waiting;
					// Количество попыток получения ответа от NTP-сервера
					uint8_t attempt;
					// Количество попыток получения ответа от NTP-сервера (по умолчанию: 3)
					uint8_t attempts;
					// Метка transmit из последнего запроса (сетевой порядок байтов)
					uint32_t origSec;
					// Дробная часть метки transmit из последнего запроса (сетевой порядок байтов)
					uint32_t origFrac;
					// Версия протокола NTP для выполнения запроса (по умолчанию: version_t::V4)
					version_t version;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Transfer() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~Transfer() noexcept = default;
				} __attribute__((packed)) transfer_t;
			private:
				// Объект работы с сетевыми адресами
				net_addr_t _addr;
			private:
				// Состояние NTP-клиента
				client_t _client;
				// Объект управления передачей данных при выполнении запросов NTP-клиента
				transfer_t _transfer;
			private:
				/**
				 * @brief Метод обработки ошибок событий NTP-клиента
				 *
				 * @param eid         идентификатор события NTP-клиента
				 * @param error       код ошибки события NTP-клиента
				 * @param description описание ошибки события NTP-клиента
				 *
				 */
				void error(const event::id_t eid, const event::error_t error, const string & description) noexcept;
			private:
				/**
				 * @brief Метод продолжения ожидания своего ответа
				 *
				 * @details Дейтаграммный обмен принимает что угодно и от кого угодно, и
				 *          пришедшее бывает не ответом на заданный вопрос: отклик сервера
				 *          прежнего, чужой пакет, подложный ответ со стороны. Отбросить
				 *          такое обмен обязан, но отбросить пакет и прекратить ожидание -
				 *          разные вещи
				 *
				 *          Движок же прекращает: приход данных ожидание чтения завершает, и
				 *          решает он это раньше, чем пакет разобран. Оттого чужой пакет
				 *          прежде обрывал обмен, а вызывающий получал отказ по вопросу,
				 *          ответ на который был ещё в пути
				 *
				 * @note Ожидание продолжается **остатком**, а не сроком полным: иначе поток
				 *       чужих пакетов отодвигал бы отказ без конца, а подложными пакетами
				 *       обмен можно было бы держать открытым сколь угодно долго
				 *
				 * @param eid идентификатор события чтения
				 *
				 */
				void keepWaiting(const event::id_t eid) noexcept;
				/**
				 * @brief Метод обработки ответов от NTP-сервера на запросы NTP-клиента
				 *
				 * @param eid  идентификатор события чтения
				 * @param data данные, полученные от NTP-сервера
				 * @param size размер полученных данных
				 *
				 */
				void response(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * @brief Метод обработки истечения таймаута NTP-запроса
				 *
				 * @param eid    идентификатор события NTP-клиента
				 * @param action тип действия для истекшего таймаута
				 * @param delay  длительность таймаута в миллисекундах
				 * @return       нужно ли завершить обработчик после истечения таймаута
				 *
				 */
				bool timeout(const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept;
			public:
				/**
				 * @brief Метод инициализации события NTP-клиента
				 *
				 * @param family семейство протоколов (например: IPv4 или IPv6)
				 * @return       результат инициализации события NTP-клиента
				 *
				 */
				bool init(const event::family_t family) noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 *
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Метод установки таймаута для ожидания ответа от NTP-сервера
				 *
				 * @param delay время ожидания ответа от NTP-сервера (в миллисекундах)
				 *
				 */
				void setTimeout(const uint32_t delay) noexcept;
			public:
				/**
				 * @brief Метод установки количества попыток получения ответа от NTP-сервера
				 *
				 * @par Намеренные решения
				 * Попытка здесь - **дополнительная**, а не общее число обращений: вопрос
				 * задаётся сам собой, а попытка есть разрешение задать его заново, когда
				 * срок ожидания вышел. Оттого при трёх попытках сервер опрашивается
				 * четырежды - первый раз и трижды снова, - а полное время ожидания
				 * вызывающего есть срок, умноженный на число попыток плюс одно
				 *
				 * @note Повтором это не является. Повтор - когда ответ получен, а вопрос
				 *       задаётся ещё раз; попытка же случается лишь тогда, когда ответа
				 *       не пришло вовсе
				 *
				 * @param attempts количество попыток получения ответа от NTP-сервера
				 *
				 */
				void setAttempts(const uint8_t attempts) noexcept;
			public:
				/**
				 * @brief Метод установки префикса переменной окружения
				 *
				 * @param prefix префикс переменной окружения для установки
				 *
				 */
				void setPrefixEnvironment(string_view prefix) noexcept;
			public:
				/**
				 * @brief Метод получения типа события
				 *
				 * @return тип события
				 *
				 */
				event::type_t type() const noexcept;
				/**
				 * @brief Метод получения типа узла события
				 *
				 * @return тип узла события
				 *
				 */
				event::node_t node() const noexcept;
				/**
				 * @brief Метод получения семейства события
				 *
				 * @return семейство адресов
				 *
				 */
				event::family_t family() const noexcept;
				/**
				 * @brief Метод получения статуса события
				 *
				 * @return статус события
				 *
				 */
				event::status_t status() const noexcept;
			public:
				/**
				 * @brief Метод получения порта NTP-сервера
				 *
				 * @return порт NTP-сервера
				 *
				 */
				uint16_t getTargetPort() const noexcept;
				/**
				 * @brief Метод установки порта NTP-сервера
				 *
				 * @param port порт NTP-сервера для установки
				 *
				 */
				void setTargetPort(const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод установки адреса NTP-сервера
				 *
				 * @param server адрес NTP-сервера для установки
				 *
				 */
				void setServer(string_view server) noexcept;
				/**
				 * @brief Метод установки адреса NTP-сервера
				 *
				 * @param server адрес NTP-сервера для установки
				 *
				 */
				void setServer(const net::addr_t * server) noexcept;
				/**
				 * @brief Метод установки адреса NTP-сервера
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param server адрес NTP-сервера для установки
				 *
				 */
				void setServer(const event::family_t family, string_view server) noexcept;
			public:
				/**
				 * @brief Метод добавления адреса NTP-сервера
				 *
				 * @param server адрес NTP-сервера для добавления
				 *
				 */
				void addServer(string_view server) noexcept;
				/**
				 * @brief Метод добавления адреса NTP-сервера
				 *
				 * @param server адрес NTP-сервера для добавления
				 *
				 */
				void addServer(const net::addr_t * server) noexcept;
				/**
				 * @brief Метод добавления адреса NTP-сервера
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param server адрес NTP-сервера для добавления
				 *
				 */
				void addServer(const event::family_t family, string_view server) noexcept;
			public:
				/**
				 * @brief Метод установки списка адресов NTP-серверов
				 *
				 * @param servers адреса NTP-серверов для установки
				 *
				 */
				void setServers(const vector <string> & servers) noexcept;
				/**
				 * @brief Метод установки списка адресов NTP-серверов
				 *
				 * @param servers адреса NTP-серверов для установки
				 *
				 */
				void setServers(const vector <const net::addr_t *> & servers) noexcept;
				/**
				 * @brief Метод установки списка адресов NTP-серверов
				 *
				 * @param family  семейство IP-адресов IPv4/IPv6
				 * @param servers адреса NTP-серверов для установки
				 *
				 */
				void setServers(const event::family_t family, const vector <string> & servers) noexcept;
			public:
				/**
				 * @brief Метод установки локального адреса для выполнения запроса
				 *
				 * @param source адрес сети для выполнения запроса
				 *
				 */
				void setSource(string_view source) noexcept;
				/**
				 * @brief Метод установки локального адреса для выполнения запроса
				 *
				 * @param source адрес сети для выполнения запроса
				 *
				 */
				void setSource(const net::addr_t * source) noexcept;
				/**
				 * @brief Метод установки локального адреса для выполнения запроса
				 *
				 * @param family семейство IP-адресов IPv4/IPv6
				 * @param source адрес сети для выполнения запроса
				 *
				 */
				void setSource(const event::family_t family, string_view source) noexcept;
			public:
				/**
				 * @brief Метод синхронизации времени с NTP-сервером
				 *
				 * @param version версия протокола NTP для выполнения запроса
				 * @return        результат выполнения запроса
				 *
				 */
				bool sync(const version_t version = version_t::V4) noexcept;
			private:
				/**
				 * @brief Конструктор копирования (запрещаем)
				 *
				 */
				NTP(const NTP &) = delete;
				/**
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 */
				NTP & operator = (const NTP &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 */
				explicit NTP(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~NTP() noexcept;
		} ntp_t;
	};
};

#endif // __AWH_UNIT_NTP__
