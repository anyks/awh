/**
 * @file: sample.hpp
 * @date: 2023-10-18
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_SAMPLE_SERVER__
#define __AWH_SAMPLE_SERVER__

/**
 * Наши модули
 */
#include "../core/server.hpp"
#include "../sys/callback.hpp"
#include "../scheme/sample.hpp"

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief серверное пространство имён
	 *
	 */
	namespace server {
		/**
		 * @brief Класс работы с SAMPLE сервером
		 *
		 */
		typedef class AWHSHARED_EXPORT Sample {
			public:
				/**
				 * Режим работы брокера
				 */
				enum class mode_t : uint8_t {
					CONNECT    = 0x01, // Метод подключения брокера
					DISCONNECT = 0x02  // Метод отключения брокера
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					NOT_INFO = 0x01, // Флаг запрещающий вывод информационных сообщений
					NOT_STOP = 0x02, // Флаг запрета остановки работы базы событий
					NOT_PING = 0x03  // Флаг запрещающий выполнение пингов
				};
			private:
				// Идентификатор основного процесса
				pid_t _pid;
			private:
				// Флаг долгоживущего подключения
				bool _alive;
				// Флаг разрешающий выполнение пингов
				bool _pinging;
				// Флаг остановки работы базы событий
				bool _complete;
			private:
				// Порт сервера
				uint32_t _port;
				// Хости сервера
				string _host;
			private:
				// Объект работы с URI
				uri_t _uri;
			private:
				// Ядро для локального таймера
				timer_t _timer;
			protected:
				// Интервал времени на выполнение пингов
				uint32_t _pingInterval;
			private:
				// Хранилище функций обратного вызова
				callback_t _callback;
				// Объект схемы сети
				scheme::sample_t _scheme;
			private:
				// Размер шифрования передаваемых данных
				hash_t::cipher_t _cipher;
			private:
				// Список отключившихся клиентов
				std::map <uint64_t, uint64_t> _disconnected;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
				// Объект сетевого ядра
				const server::core_t * _core;
			private:
				/**
				 * @brief Метод обратного вызова при запуске работы
				 *
				 * @param sid идентификатор схемы сети
				 */
				void openEvent(const uint16_t sid) noexcept;
				/**
				 * @brief Метод обратного вызова при активации ядра сервера
				 *
				 * @param status флаг запуска/остановки
				 */
				void statusEvent(const awh::core_t::status_t status) noexcept;
				/**
				 * @brief Метод обратного вызова при подключении к серверу
				 *
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				void connectEvent(const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * @brief Метод обратного вызова при отключении от сервера
				 *
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				void disconnectEvent(const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * @brief Метод получения события запуска сервера
				 *
				 * @param host хост запущенного сервера
				 * @param port порт запущенного сервера
				 */
				void launchedEvent(const string & host, const uint32_t port) noexcept;
				/**
				 * @brief Метод обратного вызова при чтении сообщения с брокера
				 *
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void readEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * @brief Метод обратного вызова при записи сообщение брокеру
				 *
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void writeEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * @brief Метод обратного вызова при проверке подключения брокера
				 *
				 * @param ip   адрес интернет подключения брокера
				 * @param mac  мак-адрес подключившегося брокера
				 * @param port порт подключившегося брокера
				 * @param sid  идентификатор схемы сети
				 * @return     результат разрешения к подключению брокера
				 */
				bool acceptEvent(const string & ip, const string & mac, const uint32_t port, const uint16_t sid) noexcept;
			private:
				/**
				 * @brief Метод удаления отключившихся клиентов
				 *
				 * @param tid идентификатор таймера
				 */
				void erase(const uint16_t tid) noexcept;
				/**
				 * @brief Метод таймера выполнения пинга клиента
				 *
				 * @param tid идентификатор таймера
				 */
				void pinging(const uint16_t tid) noexcept;
			public:
				/**
				 * @brief Метод инициализации Rest брокера
				 *
				 * @param socket unix-сокет для биндинга
				 */
				void init(const string & socket) noexcept;
				/**
				 * @brief Метод инициализации Rest брокера
				 *
				 * @param port порт сервера
				 * @param host хост сервера
				 */
				void init(const uint32_t port, const string & host = "") noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param name  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const char * name, Args... args) noexcept -> uint64_t {
					// Если мы получили название функции обратного вызова
					if(name != nullptr)
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (name, args...);
					// Выводим результат по умолчанию
					return 0;
				}
				/**
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param name  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const string & name, Args... args) noexcept -> uint64_t {
					// Если мы получили название функции обратного вызова
					if(!name.empty())
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (name, args...);
					// Выводим результат по умолчанию
					return 0;
				}
				/**
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param fid  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const uint64_t fid, Args... args) noexcept -> uint64_t {
					// Если мы получили название функции обратного вызова
					if(fid > 0)
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (fid, args...);
					// Выводим результат по умолчанию
					return 0;
				}
				/**
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam A    тип идентификатора функции
				 * @tparam B    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 */
				template <typename A, typename B, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param fid  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const A fid, Args... args) noexcept -> uint64_t {
					// Если мы получили на вход число
					if(is_integral_v <A> || is_enum_v <A> || is_floating_point_v <A>)
						// Выполняем установку функции обратного вызова
						return this->_callback.on <B> (static_cast <uint64_t> (fid), args...);
					// Выводим результат по умолчанию
					return 0;
				}
			public:
				/**
				 * @brief Метод отправки сообщения брокеру
				 *
				 * @param bid    идентификатор брокера
				 * @param buffer буфер бинарных данных для отправки
				 * @param size   размер бинарных данных для отправки
				 */
				void send(const uint64_t bid, const char * buffer, const size_t size) const noexcept;
			public:
				/**
				 * @brief Метод получения порта подключения брокера
				 *
				 * @param bid идентификатор брокера
				 * @return    порт подключения брокера
				 */
				uint32_t port(const uint64_t bid) const noexcept;
				/**
				 * @brief Метод получения IP-адреса брокера
				 *
				 * @param bid идентификатор брокера
				 * @return    адрес интернет подключения брокера
				 */
				const string & ip(const uint64_t bid) const noexcept;
				/**
				 * @brief Метод получения MAC-адреса брокера
				 *
				 * @param bid идентификатор брокера
				 * @return    адрес устройства брокера
				 */
				const string & mac(const uint64_t bid) const noexcept;
			public:
				/**
				 * @brief Метод установки долгоживущего подключения
				 *
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const bool mode) noexcept;
				/**
				 * @brief Метод установки долгоживущего подключения
				 *
				 * @param bid  идентификатор брокера
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const uint64_t bid, const bool mode) noexcept;
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
				 * @brief Метод закрытия подключения брокера
				 *
				 * @param bid идентификатор брокера
				 */
				void close(const uint64_t bid) noexcept;
			public:
				/**
				 * @brief Метод установки интервала времени выполнения пингов
				 *
				 * @param sec интервал времени выполнения пингов в секундах
				 */
				void pingInterval(const uint16_t sec) noexcept;
			public:
				/**
				 * @brief Метод ожидания входящих сообщений
				 *
				 * @param sec интервал времени в секундах
				 */
				void waitMessage(const uint16_t sec) noexcept;
				/**
				 * @brief Метод детекции сообщений по количеству секунд
				 *
				 * @param read  количество секунд для детекции по чтению
				 * @param write количество секунд для детекции по записи
				 */
				void waitTimeDetect(const uint16_t read, const uint16_t write) noexcept;
			public:
				/**
				 * @brief Метод установки максимального количества одновременных подключений
				 *
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const uint16_t total) noexcept;
				/**
				 * @brief Метод установки флага модуля
				 *
				 * @param flag флаг модуля для установки
				 */
				void mode(const std::set <flag_t> & flags) noexcept;
				/**
				 * @brief Метод установки жизни подключения
				 *
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Sample(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Sample() noexcept {}
		} sample_t;
	};
};

#endif // __AWH_SAMPLE_SERVER__
