/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_CORE_CLIENT__
#define __AWH_CORE_CLIENT__

/**
 * Наши модули
 */
#include <core/core.hpp>
#include <client/worker.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Core Класс клиентского ядра биндинга TCP/IP
	 */
	typedef class CoreClient : public core_t {
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
			 * tuning Метод тюннинга буфера событий
			 * @param bev буфер события
			 * @param ctx передаваемый контекст
			 */
			static void tuning(struct bufferevent * bev, void * ctx) noexcept;
			/**
			 * event Метод обработка входящих событий с сервера
			 * @param bev    буфер события
			 * @param events произошедшее событие
			 * @param ctx    передаваемый контекст
			 */
			static void event(struct bufferevent * bev, const short events, void * ctx) noexcept;
		private:
			/**
			 * connect Метод создания подключения к удаленному серверу
			 * @param worker воркер для подключения
			 * @return       результат подключения
			 */
			bool connect(const worker_t * worker) noexcept;
		private:
			/**
			 * close Метод закрытия подключения воркера
			 * @param worker воркер для закрытия подключения
			 */
			void close(const worker_t * worker) noexcept;
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
			 * add Метод добавления воркера в биндинг
			 * @param worker воркер для добавления
			 * @return       идентификатор воркера в биндинге
			 */
			size_t add(const worker_t * worker) noexcept;
		public:
			/**
			 * open Метод открытия подключения воркером
			 * @param wid идентификатор воркера
			 */
			void open(const size_t wid) noexcept;
			/**
			 * switchProxy Метод переключения с прокси-сервера
			 * @param wid идентификатор воркера
			 */
			void switchProxy(const size_t wid) noexcept;
			/**
			 * write Метод записи буфера данных воркером
			 * @param buffer буфер для записи данных
			 * @param size   размер записываемых данных
			 * @param wid    идентификатор воркера
			 */
			void write(const char * buffer, const size_t size, const size_t wid) noexcept;
		public:
			/**
			 * CoreClient Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			CoreClient(const fmk_t * fmk, const log_t * log) noexcept : core_t(fmk, log) {}
			/**
			 * ~CoreClient Деструктор
			 */
			~CoreClient() noexcept;
	} ccl_t;
};

#endif // __AWH_CORE_CLIENT__
