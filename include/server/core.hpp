/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_CORE_SERVER__
#define __AWH_CORE_SERVER__

/**
 * Наши модули
 */
#include <if.hpp>
#include <core/core.hpp>
#include <server/worker.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Core Класс клиентского ядра биндинга TCP/IP
	 */
	typedef class CoreServer : public core_t {
		private:
			/**
			 * Устанавливаем настройки для *Nix подобных систем
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Объект для работы с сетевым интерфейсом
				ifnet_t ifnet;
			#endif
		private:
			/**
			 * read Метод чтения данных с сокета сервера
			 * @param bev буфер события
			 * @param ctx передаваемый контекст
			 */
			static void read(struct bufferevent * bev, void * ctx) noexcept;
			/**
			 * write Метод записи данных в сокет сервера
			 * @param bev буфер события
			 * @param ctx передаваемый контекст
			 */
			static void write(struct bufferevent * bev, void * ctx) noexcept;
			/**
			 * event Метод обработка входящих событий с сервера
			 * @param bev    буфер события
			 * @param events произошедшее событие
			 * @param ctx    передаваемый контекст
			 */
			static void event(struct bufferevent * bev, const short events, void * ctx) noexcept;
			/**
			 * accept Функция подключения к серверу
			 * @param fd    файловый дескриптор (сокет)
			 * @param event событие на которое сработала функция обратного вызова
			 * @param ctx   объект передаваемый как значение
			 */
			static void accept(const evutil_socket_t fd, const short event, void * ctx) noexcept;
		private:
			/**
			 * tuning Метод тюннинга буфера событий
			 * @param aid идентификатор адъютанта
			 */
			void tuning(const size_t aid) noexcept;
			/**
			 * connect Метод создания подключения к удаленному серверу
			 * @param wid идентификатор воркера
			 */
			void connect(const size_t wid) noexcept;
			/**
			 * close Метод закрытия сокета
			 * @param fd файловый дескриптор (сокет) для закрытия
			 */
			void close(const evutil_socket_t fd) noexcept;
		public:
			/**
			 * removeAll Метод удаления всех воркеров
			 */
			void removeAll() noexcept;
			/**
			 * remove Метод удаления воркера
			 * @param wid идентификатор воркера
			 */
			void remove(const size_t wid) noexcept;
		public:
			/**
			 * run Метод запуска сервера воркером
			 * @param wid идентификатор воркера
			 */
			void run(const size_t wid) noexcept;
			/**
			 * open Метод открытия подключения воркером
			 * @param wid идентификатор воркера
			 */
			void open(const size_t wid) noexcept;
			/**
			 * close Метод закрытия подключения воркера
			 * @param aid идентификатор адъютанта
			 */
			void close(const size_t aid) noexcept;
		public:
			/**
			 * setBandwidth Метод установки пропускной способности сети
			 * @param aid   идентификатор адъютанта
			 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
			 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
			 */
			void setBandwidth(const size_t aid, const string & read, const string & write) noexcept;
		public:
			/**
			 * setIpV6only Метод установки флага использования только сети IPv6
			 * @param mode флаг для установки
			 */
			void setIpV6only(const bool mode) noexcept;
			/**
			 * setTotal Метод установки максимального количества одновременных подключений
			 * @param wid   идентификатор воркера
			 * @param total максимальное количество одновременных подключений
			 */
			void setTotal(const size_t wid, const u_short total) noexcept;
			/**
			 * init Метод инициализации сервера
			 * @param wid  идентификатор воркера
			 * @param port порт сервера
			 * @param host хост сервера
			 */
			void init(const size_t wid, const u_int port, const string & host = "") noexcept;
			/**
			 * setCert Метод установки файлов сертификата
			 * @param cert  корневой сертификат
			 * @param key   приватный ключ сертификата
			 * @param chain файл цепочки сертификатов
			 */
			void setCert(const string & cert, const string & key, const string & chain = "") noexcept;
		public:
			/**
			 * CoreServer Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			CoreServer(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~CoreServer Деструктор
			 */
			~CoreServer() noexcept;
	} coreSrv_t;
};

#endif // __AWH_CORE_SERVER__
