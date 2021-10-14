/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_WEBSOCKET_SERVER__
#define __AWH_WEBSOCKET_SERVER__

/**
 * Наши модули
 */
#include <timer.hpp>
#include <ws/frame.hpp>
#include <ws/server.hpp>
#include <server/core.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * WebSocketServer Класс работы с WebSocket сервером
	 */
	typedef class WebSocketServer {
		public:
			/**
			 * Основные флаги приложения
			 */
			enum class flag_t : uint8_t {
				DEFER     = 0x01, // Флаг отложенных вызовов событий сокета
				NOINFO    = 0x02, // Флаг запрещающий вывод информационных сообщений
				WAITMESS  = 0x04, // Флаг ожидания входящих сообщений
				KEEPALIVE = 0x08  // Флаг автоматического поддержания подключения
			};
		private:
			// Создаём объект работы с URI ссылками
			uri_t uri;
			// Создаём объект для работы с HTTP
			wss_t http;
			// Создаём объект для работы с фреймом WebSocket
			frame_t frame;
			// Создаем объект для работы с сетью
			network_t nwk;
			// Объект рабочего
			workSrv_t worker;
			// Таймер для пинга сервера
			timer_t timerPing;
			// Создаём объект для компрессии-декомпрессии данных
			mutable hash_t hash;
		private:
			// Поддерживаемые сабпротоколы
			vector <string> subs;
			// Данные фрагметрированного сообщения
			vector <char> fragmes;
		private:
			// Флаг шифрования сообщений
			bool crypt = false;
			// Флаг запрета вывода информационных сообщений
			bool noinfo = false;
			// Флаг переданных сжатых данных
			bool compressed = false;
		private:
			// Минимальный размер сегмента
			size_t frameSize = 0xFA000;
		public:
			// Полученный опкод сообщения
			frame_t::opcode_t opcode = frame_t::opcode_t::TEXT;
			// Флаги работы с сжатыми данными
			http_t::compress_t compress = http_t::compress_t::NONE;
		private:
			// Список контекстов передаваемых объектов
			vector <void *> ctx = {nullptr, nullptr, nullptr, nullptr};
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаём объект биндинга TCP/IP
			const coreSrv_t * core = nullptr;
		private:
			/**
			 * chunking Метод обработки получения чанков
			 * @param chunk бинарный буфер чанка
			 * @param ctx   контекст объекта http
			 */
			static void chunking(const vector <char> & chunk, const http_t * ctx) noexcept;
		private:
			/**
			 * openCallback Функция обратного вызова при запуске работы
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 */
			static void openCallback(const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * closeCallback Функция обратного вызова при отключении от сервера
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 */
			static void closeCallback(const size_t wid, core_t * core, void * ctx) noexcept;
			/**
			 * connectCallback Функция обратного вызова при подключении к серверу
			 * @param aid  идентификатор адъютанта
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 */
			static void connectCallback(const size_t aid, core_t * core, void * ctx) noexcept;
			/**
			 * readCallback Функция обратного вызова при чтении сообщения с сервера
			 * @param buffer бинарный буфер содержащий сообщение
			 * @param size   размер бинарного буфера содержащего сообщение
			 * @param aid    идентификатор адъютанта
			 * @param core   объект биндинга TCP/IP
			 * @param ctx    передаваемый контекст модуля
			 */
			static void readCallback(const char * buffer, const size_t size, const size_t aid, core_t * core, void * ctx) noexcept;
			/**
			 * acceptCallback Функция обратного вызова при проверке подключения клиента
			 * @param ip   адрес интернет подключения клиента
			 * @param mac  мак-адрес подключившегося клиента
			 * @param wid  идентификатор воркера
			 * @param core объект биндинга TCP/IP
			 * @param ctx  передаваемый контекст модуля
			 * @return     результат разрешения к подключению клиента
			 */
			static bool acceptCallback(const string & ip, const string & mac, const size_t wid, core_t * core, void * ctx) noexcept;
		public:
			/**
			 * stop Метод остановки клиента
			 */
			void stop() noexcept;
			/**
			 * start Метод запуска клиента
			 */
			void start() noexcept;
		public:
			/**
			 * setSub Метод установки подпротокола поддерживаемого сервером
			 * @param sub подпротокол для установки
			 */
			void setSub(const string & sub) noexcept;
			/**
			 * setSubs Метод установки списка подпротоколов поддерживаемых сервером
			 * @param subs подпротоколы для установки
			 */
			void setSubs(const vector <string> & subs) noexcept;
		public:
			/**
			 * setChunkingFn Метод установки функции обратного вызова для получения чанков
			 * @param callback функция обратного вызова
			 */
			void setChunkingFn(function <void (const vector <char> &, const http_t *)> callback) noexcept;
		public:
			/**
			 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
			 * @param read  количество секунд для детекции по чтению
			 * @param write количество секунд для детекции по записи
			 */
			void setWaitTimeDetect(const time_t read, const time_t write) noexcept;
			/**
			 * setBytesDetect Метод детекции сообщений по количеству байт
			 * @param read  количество байт для детекции по чтению
			 * @param write количество байт для детекции по записи
			 */
			void setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept;
		public:
			/**
			 * setMode Метод установки флага модуля
			 * @param flag флаг модуля для установки
			 */
			void setMode(const u_short flag) noexcept;
			/**
			 * setChunkSize Метод установки размера чанка
			 * @param size размер чанка для установки
			 */
			void setChunkSize(const size_t size) noexcept;
			/**
			 * setFrameSize Метод установки размеров сегментов фрейма
			 * @param size минимальный размер сегмента
			 */
			void setFrameSize(const size_t size) noexcept;
			/**
			 * setCompress Метод установки метода сжатия
			 * @param метод сжатия сообщений
			 */
			void setCompress(const http_t::compress_t compress) noexcept;
			/**
			 * setServ Метод установки данных сервиса
			 * @param id   идентификатор сервиса
			 * @param name название сервиса
			 * @param ver  версия сервиса
			 */
			void setServ(const string & id, const string & name, const string & ver) noexcept;
			/**
			 * setCrypt Метод установки параметров шифрования
			 * @param pass пароль шифрования передаваемых данных
			 * @param salt соль шифрования передаваемых данных
			 * @param aes  размер шифрования передаваемых данных
			 */
			void setCrypt(const string & pass, const string & salt = "", const hash_t::aes_t aes = hash_t::aes_t::AES128) noexcept;
		public:
			/**
			 * WebSocketServer Конструктор
			 * @param core объект биндинга TCP/IP
			 * @param fmk  объект фреймворка
			 * @param log  объект для работы с логами
			 */
			WebSocketServer(const coreSrv_t * core, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~WebSocketServer Деструктор
			 */
			~WebSocketServer() noexcept {}
	} wsSrv_t;
};

#endif // __AWH_WEBSOCKET_SERVER__
