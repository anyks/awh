/**
 * @file: callback.hpp
 * @date: 2026-03-10
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
#ifndef __AWH_ENGINE_CALLBACK__
#define __AWH_ENGINE_CALLBACK__

/**
 * Стандартный заголовочный файл
 */
#include <functional>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "net.hpp"
#include "event.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён движков ввода-вывода
	 *
	 */
	namespace engine {
		/**
		 * @brief пространство имён работы с обратными вызовами
		 *
		 */
		namespace callback {
			/**
			 * Для операционной системы Linux или FreeBSD
			 */
			#if __linux__ || __FreeBSD__
				/**
				 * @brief Пространство имён для работы с SCTP
				 *
				 */
				namespace sctp {
					/**
					 * @brief Функция обратного вызова срабатывающая при получении информационных сообщений SCTP
					 *
					 * @param id   идентификатор события
					 * @param info объект информационных метаданных SCTP
					 */
					using minfo_t = function <void (const event::id_t, const net::sctp::minfo_t &)>;
					/**
					 * @brief Функция обратного вызова срабатывающая при получении событий SCTP
					 *
					 * @param id    идентификатор события
					 * @param event объект события SCTP
					 */
					using events_t = function <void (const event::id_t, unique_ptr <net::sctp::event_t>)>;
				};
			#endif
			/**
			 * @brief Функция обратного вызова срабатывающая при подключении события
			 *
			 * @param id идентификатор события
			 * @param ok результат подключения события (true - подключено, false - ошибка)
			 */
			using connect_t = function <void (const event::id_t, const bool)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при записи в событие
			 *
			 * @param id   идентификатор события
			 * @param size размер записанных данных
			 */
			using write_t = function <void (const event::id_t, const size_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при принятии события
			 *
			 * @param id   идентификатор события
			 * @param peer идентификатор подключившегося события
			 */
			using accept_t = function <void (const event::id_t, const event::id_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при общем событии
			 *
			 * @param id     идентификатор события
			 * @param action тип события
			 */
			using event_t = function <void (const event::id_t, const event::action_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при изменении статуса события
			 *
			 * @param id     идентификатор события
			 * @param status новый статус события
			 */
			using status_t = function <void (const event::id_t, const event::status_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при получении информационных метаданных о дейтаграммном пакете
			 *
			 * @param id   идентификатор события
			 * @param info объект информационных метаданных дейтаграммного пакета
			 */
			using traffic_t = function <void (const event::id_t, const net::dgram_info_t &)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при чтении из события
			 *
			 * @param id     идентификатор события
			 * @param buffer буфер прочитанных данных
			 * @param size   размер прочитанных данных
			 */
			using read_t = function <void (const event::id_t, const uint8_t *, const size_t)>;
			/**
			 * @brief Функция обратного вызова инъекции объединённых данных в событие (splice)
			 *
			 * @note Позволяет транспорту, шифрующему данные на уровне соединения (QUIC),
			 *       принять перенаправленные из события-источника байты и отправить их
			 *       собственным потоком, а не записывать их сырьём в сокет
			 *
			 * @param id     идентификатор события-приёмника
			 * @param buffer буфер перенаправляемых данных
			 * @param size   размер перенаправляемых данных
			 * @return       результат приёма данных на инъекцию
			 */
			using inject_t = function <bool (const event::id_t, const uint8_t *, const size_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при ошибке события
			 *
			 * @param id    идентификатор события
			 * @param error код ошибки события
			 * @param text  текстовое описание ошибки события
			 */
			using error_t = function <void (const event::id_t, const event::error_t, const string &)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при доступности очереди на отправку данных
			 *
			 * @param id     идентификатор события
			 * @param status статус события
			 * @param size   размер доступных данных для отправки
			 */
			using available_t = function <void (const event::id_t, const event::status_t, const size_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при истечении таймера события
			 *
			 * @param id     идентификатор события
			 * @param action тип события
			 * @param count  счётчик таймера события
			 */
			using timeout_t = function <bool (const event::id_t, const event::action_t, const uint32_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при определении сессии дейтаграммного пакета
			 *
			 * @note Вызывается до маршрутизации датаграммы и позволяет протоколам с
			 *       собственной адресацией сессий задать ключ маршрутизации самим.
			 *       Отрицательный результат означает, что датаграмма протоколу не
			 *       принадлежит и подлежит отбрасыванию без создания сессии
			 *
			 * @param id     идентификатор события
			 * @param buffer буфер принятой датаграммы
			 * @param size   размер принятой датаграммы
			 * @param key    выводимый ключ сессии
			 * @return       результат определения сессии
			 */
			using origin_t = function <bool (const event::id_t, const uint8_t *, const size_t, net::origin_key_t &)>;
			/**
			 * @brief Функция обратного вызова возвращающая неотправленные данные события
			 *
			 * @param id     идентификатор события
			 * @param buffer буфер данных события
			 * @param size   размер данных события
			 * @return       результат обработки (true - продолжить отправку, false - прервать с ошибкой ABORTED)
			 */
			using spool_t = function <void (const event::id_t, const event::send_error_t, const uint8_t *, const size_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при изменении каталога
			 *
			 * @note Параметр name действителен ТОЛЬКО на время вызова функции обратного вызова
			 *
			 * @param id     идентификатор события
			 * @param action тип события
			 * @param vnode  виртуальный узел события
			 * @param name   имя изменённого файла/каталога события
			 */
			using vnode_t = function <void (const event::id_t, const event::action_t, const event::vnode_t, const string &)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при получении информации о пакетах в туннельном интерфейсе
			 *
			 * @param id     идентификатор события
			 * @param peer   идентификатор удалённого узла
			 * @param action тип события
			 * @param info   информация о пакетах в туннельном интерфейсе
			 */
			using tuninfo_t = function <void (const event::id_t, const event::id_t, const event::action_t, const net::tun_info_t &)>;
		};
	};
};

#endif // __AWH_ENGINE_CALLBACK__
