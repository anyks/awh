/**
 * @file: websocket.hpp
 * @date: 2022-09-03
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

#ifndef __AWH_SCHEME_WEBSOCKET_SERVER__
#define __AWH_SCHEME_WEBSOCKET_SERVER__

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
#include <scheme/server.hpp>

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
		 * SchemeWS Структура схемы сети WebSocket сервера
		 */
		typedef struct SchemeWebSocket : public scheme_t {
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
				 * Coffer Структура сундука параметров
				 */
				typedef struct Coffer {
					bool crypt;                  // Флаг шифрования сообщений
					bool close;                  // Флаг требования закрыть адъютанта
					bool stopped;                // Флаг принудительной остановки
					bool compressed;             // Флаг переданных сжатых данных
					short wbitClient;            // Размер скользящего окна клиента
					short wbitServer;            // Размер скользящего окна сервера
					action_t action;             // Экшен активного события
					time_t checkPoint;           // Контрольная точка ответа на пинг
					hash_t hash;                 // Создаём объект для компрессии-декомпрессии данных
					allow_t allow;               // Объект разрешения обмена данными
					locker_t locker;             // Объект блокировщика
					buffer_t buffer;             // Объект буфера данных
					server::ws_t ws;             // Создаём объект для работы с HTTP
					recursive_mutex mtx;         // Мютекс для блокировки потока
					frame_t::opcode_t opcode;    // Полученный опкод сообщения
					http_t::compress_t compress; // Метод компрессии данных
					/**
					 * Coffer Конструктор
					 */
					Coffer(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept :
					 crypt(false), close(false),
					 stopped(false), compressed(false),
					 wbitClient(-1), wbitServer(-1),
					 action(action_t::NONE), checkPoint(0),
					 hash(log), ws(fmk, log, uri),
					 opcode(frame_t::opcode_t::TEXT),
					 compress(http_t::compress_t::NONE) {}
					/**
					 * ~Coffer Деструктор
					 */
					~Coffer() noexcept {}
				} coffer_t;
			public:
				// Создаем объект для работы с сетью
				net_t net;
				// Создаём объект работы с URI ссылками
				uri_t uri;
			public:
				// Флаги работы с сжатыми данными
				http_t::compress_t compress;
			private:
				// Параметры подключения адъютантов
				map <size_t, unique_ptr <coffer_t>> _coffers;
			private:
				// Создаём объект фреймворка
				const fmk_t * _fmk;
				// Создаём объект работы с логами
				const log_t * _log;
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
				const coffer_t * get(const size_t aid) const noexcept;
			public:
				/**
				 * SchemeWebSocket Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				SchemeWebSocket(const fmk_t * fmk, const log_t * log) noexcept :
				 scheme_t(fmk, log), uri(fmk),
				 compress(http_t::compress_t::NONE),
				 _fmk(fmk), _log(log) {}
				/**
				 * ~SchemeWebSocket Деструктор
				 */
				~SchemeWebSocket() noexcept {}
		} websocket_scheme_t;
	};
};

#endif // __AWH_SCHEME_WEBSOCKET_SERVER__