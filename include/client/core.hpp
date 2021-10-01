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
			 * event Метод обработка входящих событий с сервера
			 * @param bev    буфер события
			 * @param events произошедшее событие
			 * @param ctx    передаваемый контекст
			 */
			static void event(struct bufferevent * bev, const short events, void * ctx) noexcept;
		private:
			/**
			 * connect Метод создания подключения к удаленному серверу
			 * @param wid идентификатор воркера
			 * @return    результат подключения
			 */
			bool connect(const size_t wid) noexcept;
		private:
			/**
			 * tuning Метод тюннинга буфера событий
			 * @param adj объект текущего адъютанта
			 */
			void tuning(const worker_t::adj_t * adj) noexcept;
		public:
			/**
			 * closeAll Метод отключения всех воркеров
			 */
			void closeAll() noexcept;
		public:
			/**
			 * open Метод открытия подключения воркером
			 * @param wid идентификатор воркера
			 */
			void open(const size_t wid) noexcept;
			/**
			 * close Метод закрытия подключения воркера
			 * @param adj объект текущего адъютанта
			 */
			void close(const worker_t::adj_t * adj) noexcept;
			/**
			 * switchProxy Метод переключения с прокси-сервера
			 * @param adj объект текущего адъютанта
			 */
			void switchProxy(const worker_t::adj_t * adj) noexcept;
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
	} ccli_t;
};

#endif // __AWH_CORE_CLIENT__
