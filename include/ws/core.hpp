/**
 * @file: core.hpp
 * @date: 2021-12-19
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

#ifndef __AWH_WS_CORE__
#define __AWH_WS_CORE__

/**
 * Стандартные модули
 */
#include <set>
#include <map>
#include <queue>
#include <mutex>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <random>
#include <algorithm>
#include <functional>

/**
 * Наши модули
 */
#include "../sys/hash.hpp"
#include "../net/socket.hpp"
#include "../http/core.hpp"

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
	 * @brief Класс для работы с WebSocket
	 *
	 */
	typedef class AWH_SHARED_EXPORT WebsocketCore : public http_t {
		protected:
			/**
			 * Версия протокола WebSocket
			 */
			static constexpr uint16_t WS_VERSION = 13;
			/**
			 * Размер минимального значения окна для сжатия данных GZIP
			 */
			static constexpr int16_t GZIP_MIN_WBITS = 8;
			/**
			 * Размер максимального значения окна для сжатия данных GZIP
			 */
			static constexpr int16_t GZIP_MAX_WBITS = 15;
		public:
			/**
			 * Флаги проверок переключения протокола
			 */
			enum class flag_t : uint8_t {
				NONE    = 0x00, // Флаг не установлен
				KEY     = 0x01, // Флаг проверки соответствия ключа запроса
				VERSION = 0x02, // Флаг проверки версии протокола
				UPGRADE = 0x03  // Флаг выполнения переключения протокола
			};
		protected:
			/**
			 * @brief Структура партнёра
			 *
			 */
			typedef struct Partner {
				int16_t wbit;  // Размер скользящего окна
				bool takeover; // Флаг скользящего контекста сжатия
				/**
				 * @brief Конструктор
				 *
				 */
				Partner() noexcept : wbit(GZIP_MAX_WBITS), takeover(false) {}
			} __attribute__((packed)) partner_t;
		protected:
			// Флаг зашифрованных данных
			bool _encryption;
		protected:
			// Объект партнёра клиента
			partner_t _client;
			// Объект партнёра сервера
			partner_t _server;
		protected:
			// Ключ клиента
			mutable string _key;
		protected:
			// Компрессор для жатия данных
			compressors_t _compressors;
		protected:
			// Список поддверживаемых расширений
			vector <vector <string>> _extensions;
		protected:
			// Список выбранных сабпротоколов
			std::unordered_set <string> _selectedProtocols;
			// Список поддерживаемых сабпротоколов
			std::unordered_set <string> _supportedProtocols;
		private:
			/**
			 * @brief Метод инициализации
			 *
			 * @param flag флаг направления передачи данных
			 */
			void init(const process_t flag) noexcept;
		private:
			/**
			 * @brief Метод установки выбранных расширений
			 *
			 * @param flag флаг направления передачи данных
			 */
			void extensions(const process_t flag) noexcept;
		protected:
			/**
			 * @brief Метод генерации ключа
			 *
			 * @return сгенерированный ключ
			 */
			string key() const noexcept;
			/**
			 * @brief Метод генерации хэша SHA1 ключа
			 *
			 * @return сгенерированный хэш ключа клиента
			 */
			string sha1() const noexcept;
		protected:
			/**
			 * @brief Метод извлечения системного расширения из заголовка
			 *
			 * @param extension запись из которой нужно извлечь расширение
			 * @return          результат извлечения
			 */
			bool extract(const string & extension) noexcept;
		public:
			/**
			 * @brief Метод применения полученных результатов
			 *
			 */
			virtual void commit() noexcept = 0;
		public:
			/**
			 * @brief Метод проверки текущего статуса
			 *
			 * @return результат проверки текущего статуса
			 */
			virtual status_t status() noexcept = 0;
		public:
			/**
			 * @brief Метод получения бинарного дампа
			 *
			 * @return бинарный дамп данных
			 */
			buffer_t dump() const noexcept;
			/**
			 * @brief Метод установки бинарного дампа
			 *
			 * @param data бинарный дамп данных
			 */
			void dump(const buffer_t & data) noexcept;
			/**
			 * @brief Метод установки бинарного дампа
			 *
			 * @param buffer буфер бинарных данных
			 * @param size   размер бинарных данных
			 */
			void dump(const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод очистки собранных данных
			 *
			 */
			void clean() noexcept;
		public:
			/**
			 * @brief Метод проверки на зашифрованные данные
			 *
			 * @return флаг проверки на зашифрованные данные
			 */
			bool crypted() const noexcept;
		public:
			/**
			 * @brief Метод активации шифрования
			 *
			 * @param mode флаг активации шифрования
			 */
			void encryption(const bool mode) noexcept;
			/**
			 * @brief Метод установки параметров шифрования
			 *
			 * @param pass   пароль шифрования передаваемых данных
			 * @param salt   соль шифрования передаваемых данных
			 * @param cipher размер шифрования передаваемых данных
			 */
			void encryption(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
		public:
			/**
			 * @brief Метод извлечения выбранного метода компрессии
			 *
			 * @return метод компрессии
			 */
			compressor_t compression() const noexcept;
			/**
			 * @brief Метод установки выбранного метода компрессии
			 *
			 * @param compressor метод компрессии
			 */
			void compression(const compressor_t compressor) noexcept;
			/**
			 * @brief Метод установки списка поддерживаемых компрессоров
			 *
			 * @param compressors методы компрессии данных полезной нагрузки
			 */
			void compressors(const vector <compressor_t> & compressors) noexcept;
		public:
			/**
			 * @brief Метод извлечения списка расширений
			 *
			 * @return список поддерживаемых расширений
			 */
			const vector <vector <string>> & extensions() const noexcept;
			/**
			 * @brief Метод установки списка расширений
			 *
			 * @param extensions список поддерживаемых расширений
			 */
			void extensions(const vector <vector <string>> & extensions) noexcept;
		public:
			/**
			 * @brief Метод выполнения проверки рукопожатия
			 *
			 * @param flag флаг выполняемого процесса
			 * @return     результат выполнения проверки рукопожатия
			 */
			bool handshake(const process_t flag) noexcept;
		public:
			/**
			 * @brief Метод проверки шагов рукопожатия
			 *
			 * @param flag флаг выполнения проверки
			 * @return     результат проверки соответствия
			 */
			virtual bool check(const flag_t flag) noexcept;
		public:
			/**
			 * @brief Метод получения размер скользящего окна
			 *
			 * @param hid тип текущего модуля
			 * @return    размер скользящего окна
			 */
			int16_t wbit(const web_t::hid_t hid) const noexcept;
		public:
			/**
			 * @brief Метод создания отрицательного ответа
			 *
			 * @param req объект параметров REST-ответа
			 * @return    буфер данных ответа в бинарном виде
			 */
			buffer_t reject(const web_t::res_t & res) const noexcept;
			/**
			 * @brief Метод создания отрицательного ответа (для протокола HTTP/2)
			 *
			 * @param req объект параметров REST-ответа
			 * @return    буфер данных ответа в бинарном виде
			 */
			vector <std::pair <string, string>> reject2(const web_t::res_t & res) const noexcept;
		public:
			/**
			 * @brief Метод создания выполняемого процесса в бинарном виде
			 *
			 * @param flag     флаг выполняемого процесса
			 * @param provider параметры провайдера обмена сообщениями
			 * @return         буфер данных в бинарном виде
			 */
			buffer_t process(const process_t flag, const web_t::provider_t & provider) const noexcept;
			/**
			 * @brief Метод создания выполняемого процесса в бинарном виде (для протокола HTTP/2)
			 *
			 * @param flag     флаг выполняемого процесса
			 * @param provider параметры провайдера обмена сообщениями
			 * @return         буфер данных в бинарном виде
			 */
			vector <std::pair <string, string>> process2(const process_t flag, const web_t::provider_t & provider) const noexcept;
		public:
			/**
			 * @brief Метод установки поддерживаемого сабпротокола
			 *
			 * @param subprotocol сабпротокол для установки
			 */
			void subprotocol(const string & subprotocol) noexcept;
			/**
			 * @brief Метод получения списка выбранных сабпротоколов
			 *
			 * @return список выбранных сабпротоколов
			 */
			const std::unordered_set <string> & subprotocols() const noexcept;
			/**
			 * @brief Метод установки списка поддерживаемых сабпротоколов
			 *
			 * @param subprotocols сабпротоколы для установки
			 */
			void subprotocols(const std::unordered_set <string> & subprotocols) noexcept;
		public:
			/**
			 * @brief Метод получения флага переиспользования контекста компрессии
			 *
			 * @param hid тип текущего модуля
			 * @return    флаг запрета переиспользования контекста компрессии
			 */
			bool takeover(const web_t::hid_t hid) const noexcept;
			/**
			 * @brief Метод установки флага переиспользования контекста компрессии
			 *
			 * @param hid  тип текущего модуля
			 * @param flag флаг запрета переиспользования контекста компрессии
			 */
			void takeover(const web_t::hid_t hid, const bool flag) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			WebsocketCore(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~WebsocketCore() noexcept {}
	} ws_core_t;
	/**
	 * Прототип класса сетевого ядра
	 */
	class Core;
	/**
	 * @brief Класс передачи работы между пулом потоков и потоком базы событий
	 *
	 * В режиме многопоточности (multiThreads) функция обратного вызова получения сообщения
	 * исполняется в пуле потоков. Сетевое ядро и сессия HTTP/2 не потокобезопасны, поэтому:
	 * 1. Расшифровка и декомпрессия (общий контекст permessage-deflate) выполняются в потоке базы
	 *    событий строго по порядку, в пул передаётся только готовое сообщение.
	 * 2. Сообщения одного брокера отдаются функции обратного вызова по очереди и не параллельно,
	 *    так порядок сообщений Websocket сохраняется при любом количестве потоков пула, а разные
	 *    брокеры обрабатываются параллельно.
	 * 3. Отправка из стороннего потока (в том числе из функции обратного вызова в пуле потоков)
	 *    передаётся в поток базы событий через межпотоковый передатчик сетевого ядра (upstream).
	 *    Такой вызов sendMessage/send возвращает true, если задача поставлена в очередь: результат
	 *    самой отправки становится известен только в потоке базы событий.
	 */
	typedef class AWH_SHARED_EXPORT WebsocketRelay {
		private:
			/**
			 * @brief Структура очереди задач для потока базы событий
			 *
			 */
			typedef struct Tasks {
				// Мютекс для блокировки очереди
				std::mutex mtx;
				// Очередь задач для исполнения
				std::queue <function <void (void)>> items;
			} tasks_t;
		private:
			// Идентификатор процесса, в котором активирован передатчик
			pid_t _pid;
			// Сокет межпотокового передатчика
			SOCKET _sock;
		private:
			// Идентификатор потока базы событий
			std::thread::id _tid;
		private:
			// База событий, в которой активирован передатчик
			const void * _base;
		private:
			// Мютекс для блокировки параметров передатчика
			mutable std::mutex _mtx;
		private:
			/**
			 * Очередь задач хранится в разделяемом объекте: функция обратного вызова передатчика
			 * держит на него слабую ссылку и после уничтожения модуля ничего не исполняет
			 */
			std::shared_ptr <tasks_t> _tasks;
		private:
			// Мютекс для блокировки очередей сообщений
			std::mutex _locker;
		private:
			// Очереди полученных сообщений брокеров для передачи в пул потоков
			std::map <uint64_t, std::queue <pair <vector <char>, bool>>> _messages;
		public:
			/**
			 * @brief Метод активации межпотокового передатчика
			 *
			 * Вызывается только в потоке базы событий
			 *
			 * @param core объект сетевого ядра
			 */
			void activation(Core * core) noexcept;
			/**
			 * @brief Метод деактивации межпотокового передатчика
			 *
			 * Вызывается только в потоке базы событий
			 *
			 * @param core объект сетевого ядра
			 */
			void deactivation(Core * core) noexcept;
		public:
			/**
			 * @brief Метод проверки необходимости передачи работы в поток базы событий
			 *
			 * @return передатчик активирован, а вызов выполнен не в потоке базы событий
			 */
			bool remote() const noexcept;
			/**
			 * @brief Метод передачи задачи в поток базы событий
			 *
			 * @param core объект сетевого ядра
			 * @param task задача для исполнения в потоке базы событий
			 * @return     результат передачи (false - вызов выполнен в потоке базы событий или передатчик не активирован)
			 */
			bool forward(Core * core, function <void (void)> task) noexcept;
		public:
			/**
			 * @brief Метод добавления полученного сообщения в очередь брокера
			 *
			 * @param bid     идентификатор брокера
			 * @param message буфер полученного сообщения
			 * @param text    сообщение передаётся в текстовом виде
			 * @return        необходимо запустить обработку очереди брокера в пуле потоков
			 */
			bool push(const uint64_t bid, vector <char> && message, const bool text) noexcept;
			/**
			 * @brief Метод извлечения первого сообщения из очереди брокера
			 *
			 * @param bid     идентификатор брокера
			 * @param message буфер извлечённого сообщения
			 * @param text    сообщение передаётся в текстовом виде
			 * @return        результат извлечения сообщения
			 */
			bool pop(const uint64_t bid, vector <char> & message, bool & text) noexcept;
			/**
			 * @brief Метод проверки наличия следующего сообщения в очереди брокера
			 *
			 * Если очередь пуста, она удаляется и обработка очереди брокера завершается
			 *
			 * @param bid идентификатор брокера
			 * @return    в очереди брокера ещё остались сообщения
			 */
			bool next(const uint64_t bid) noexcept;
		public:
			/**
			 * @brief Метод очистки очередей сообщений
			 *
			 * Вызывается после остановки пула потоков, задачи которого удалены без исполнения
			 */
			void clear() noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			WebsocketRelay() noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~WebsocketRelay() noexcept;
	} ws_relay_t;
};

#endif // __AWH_WS_CORE__
