/**
 * @file: ws.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_WORKER_WSS_SERVER__
#define __AWH_WORKER_WSS_SERVER__

/**
 * Стандартная библиотека
 */
#include <map>
#include <ctime>
#include <vector>

/**
 * Наши модули
 */
#include <ws/frame.hpp>
#include <ws/server.hpp>
#include <worker/server.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * WorkerWS Структура WebSocket сервера воркера
		 */
		typedef struct WorkerWebSocket : public worker_t {
			public:
				/**
				 * Основные экшены
				 */
				enum class action_t : uint8_t {
					NONE       = 0x01, // Отсутствие события
					READ       = 0x02, // Событие чтения с сервера
					CONNECT    = 0x03, // Событие подключения к серверу
					DISCONNECT = 0x04  // Событие отключения от сервера
				};
			public:
				/**
				 * Buffer Структура буфера данных
				 */
				typedef struct Buffer {
					vector <char> payload; // Бинарный буфер полезной нагрузки
					vector <char> fragmes; // Данные фрагметрированного сообщения
				} buffer_t;
				/**
				 * Locker Структура локера
				 */
				typedef struct Locker {
					bool mode;           // Флаг блокировки
					recursive_mutex mtx; // Мютекс для блокировки потока
					/**
					 * Locker Конструктор
					 */
					Locker() noexcept : mode(false) {}
				} locker_t;
				/**
				 * Allow Структура флагов разрешения обменом данных
				 */
				typedef struct Allow {
					bool send;    // Флаг разрешения отправки данных
					bool receive; // Флаг разрешения чтения данных
					/**
					 * Allow Конструктор
					 */
					Allow() noexcept : send(true), receive(true) {}
				} allow_t;
			public:
				/**
				 * Settings Структура параметров адъютанта
				 */
				typedef struct Settings {
					bool crypt;                  // Флаг шифрования сообщений
					bool close;                  // Флаг требования закрыть адъютанта
					bool stopped;                // Флаг принудительной остановки
					bool compressed;             // Флаг переданных сжатых данных
					action_t action;             // Экшен активного события
					time_t checkPoint;           // Контрольная точка ответа на пинг
					hash_t hash;                 // Создаём объект для компрессии-декомпрессии данных
					allow_t allow;               // Объект разрешения обмена данными
					locker_t locker;             // Объект блокировщика
					buffer_t buffer;             // Объект буфера данных
					server::wss_t http;          // Создаём объект для работы с HTTP
					recursive_mutex mtx;         // Мютекс для блокировки потока
					frame_t::opcode_t opcode;    // Полученный опкод сообщения
					http_t::compress_t compress; // Метод компрессии данных
					/**
					 * Settings Конструктор
					 */
					Settings(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept :
					 crypt(false),
					 close(false),
					 stopped(false),
					 compressed(false),
					 action(action_t::NONE),
					 checkPoint(0),
					 hash(fmk, log),
					 http(fmk, log, uri),
					 opcode(frame_t::opcode_t::TEXT),
					 compress(http_t::compress_t::NONE) {}
					/**
					 * ~Settings Деструктор
					 */
					~Settings() noexcept {}
				} settings_t;
			public:
				// Создаём объект работы с URI ссылками
				uri_t uri;
				// Создаем объект для работы с сетью
				network_t nwk;
			public:
				// Флаги работы с сжатыми данными
				http_t::compress_t compress;
			private:
				// Параметры подключения адъютантов
				map <size_t, unique_ptr <settings_t>> settings;
			private:
				// Создаём объект фреймворка
				const fmk_t * fmk;
				// Создаём объект работы с логами
				const log_t * log;
			public:
				/**
				 * clear Метод очистки
				 */
				void clear() noexcept;
			public:
				/**
				 * set Метод создания параметров адъютанта
				 * @param aid идентификатор адъютанта
				 */
				void set(const size_t aid) noexcept;
				/**
				 * rm Метод удаления параметров подключения адъютанта
				 * @param aid идентификатор адъютанта
				 */
				void rm(const size_t aid) noexcept;
				/**
				 * get Метод получения параметров подключения адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    параметры подключения адъютанта
				 */
				const settings_t * get(const size_t aid) const noexcept;
			public:
				/**
				 * WorkerWebSocket Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				WorkerWebSocket(const fmk_t * fmk, const log_t * log) noexcept :
				 worker_t(fmk, log), nwk(fmk),
				 compress(http_t::compress_t::NONE),
				 uri(fmk, &nwk), fmk(fmk), log(log) {}
				/**
				 * ~WorkerWebSocket Деструктор
				 */
				~WorkerWebSocket() noexcept {}
		} ws_worker_t;
	};
};

#endif // __AWH_WORKER_WSS_SERVER__
