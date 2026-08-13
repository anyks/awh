/**
 * @file: fds.hpp
 * @date: 2025-10-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля партнёрских сокетов — класс Files_Descriptors для создания связанных пар файловых
 *        дескрипторов и передачи дескрипторов между процессами через управляющие сообщения
 *
 * \~english
 * @brief Header file of the module of the partner sockets — the Files_Descriptors class for creating connected pairs of file
 *        descriptors and passing the descriptors between the processes through the control messages
 *
 * \~
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_EVENT_FDS_BASE__
#define __AWH_EVENT_FDS_BASE__

/**
 * Стандартный заголовочный файл
 */
#include <cstdint>

/**
 * Системный заголовочный файл
 */
#include <unistd.h>

/**
 * Подключаем заголовочный файл проекта
 */
#include "../sys/log.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Класс партнёрских сокетов
	 *
	 * @details Ведает пределом одновременно открытых файловых дескрипторов - тем
	 *          самым, в который упирается сервер, держащий много соединений. Каждое
	 *          соединение это дескриптор, и предел по умолчанию у большинства систем
	 *          для сервера мал: тысяча-другая, тогда как принять требуется десятки
	 *          тысяч. Упирается приложение в него не отказом в подключении, а
	 *          отказом `accept()` с невнятной ошибкой, поэтому предел разумно поднять
	 *          на старте и проверить, что он действительно поднялся
	 *
	 * @note Класс различает две разные величины, и путать их не следует. Предел -
	 *       это то, что **разрешено** настройками, а `sockets()` показывает, сколько
	 *       сокетов удаётся открыть **на самом деле**: упереться можно и раньше
	 *       разрешённого - в общесистемный предел, в память, в настройки ядра
	 *
	 * @par Пример: подъём предела на старте приложения
	 *
	 * \~english
	 * @brief Class of the partner sockets
	 * @details Is in charge of the limit of the simultaneously open file descriptors — the very
	 *          one a server holding many connections runs into. Every
	 *          connection is a descriptor, and the limit by default at most of the systems
	 *          is small for a server: a thousand or two, while tens of
	 *          thousands are required to be accepted. The application runs into it not by a refusal of a connection, but by
	 *          a refusal of `accept()` with an unintelligible error, and therefore it is reasonable to raise the limit
	 *          at the startup and to check that it has really been raised
	 * @note The class tells apart two different quantities, and they should not be confused. The limit is
	 *       what is **allowed** by the settings, and `sockets()` shows how many
	 *       sockets it succeeds to open **in reality**: it is possible to run into a wall earlier than
	 *       the allowed one — into the system-wide limit, into the memory, into the settings of the kernel
	 * @par Example: raising the limit at the startup of an application
	 *
	 * \~
	 *
	 * @code{.cpp}
	 * awh::fds_t fds(&log);
	 * // Узнаём, что имеем: мягкий предел и жёсткий потолок
	 * const auto limits = fds.limit();
	 * // Поднимаем мягкий предел до жёсткого потолка
	 * if(!fds.limit(limits.second))
	 *     // Не вышло - выводим в лог, что и почему следует поправить в системе
	 *     fds.help(limits.first, limits.second);
	 * @endcode
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Files_Descriptors {
		private:
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * \~russian
			 * @brief Метод вывода в лог справочной помощи
			 *
			 * @param actual  текущее значение установленных файловых дескрипторов
			 * @param desired желаемое значение для установки файловых дескрипторов
			 *
			 * \~english
			 * @brief Method of yielding the reference help into the log
			 * @param actual  current value of the set file descriptors
			 * @param desired desired value for the setting of the file descriptors
			 *
			 * \~
			 */
			void help(const uint32_t actual, const uint32_t desired) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки нужного количества файловых дескрипторов
			 *
			 * @details Поднимает мягкий предел процесса. Выше жёсткого потолка поднять
			 *          его обычному пользователю нельзя - потолок задаётся системой, и
			 *          изменить его вправе лишь надзиратель, - поэтому запрошенная
			 *          величина сверх потолка приведёт к отказу
			 *
			 * @note Предел наследуется потомками, но не переживает завершение процесса:
			 *       выставлять его следует при каждом запуске, а не однажды
			 *
			 * @param limit желаемое количество файловых дескрипторов
			 * @return      результат установки
			 *
			 * \~english
			 * @brief Method of setting the needed number of the file descriptors
			 * @details Raises the soft limit of the process. Above the hard ceiling an ordinary user
			 *          cannot raise it — the ceiling is set by the system, and
			 *          only the supervisor is free to change it, — and therefore a requested
			 *          value beyond the ceiling will lead to a refusal
			 * @note The limit is inherited by the descendants, but does not outlive the completion of the process:
			 *       it should be set out at every startup, and not once
			 * @param limit desired number of the file descriptors
			 * @return      result of the setting
			 *
			 * \~
			 */
			bool limit(const uint32_t limit) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения лимита файловых дескрипторов установленных в операционной системе
			 *
			 * @details Возвращает пару: первым идёт **мягкий** предел - тот, что
			 *          действует сейчас, - вторым **жёсткий** потолок, до которого
			 *          мягкий разрешено поднимать без особых прав
			 *
			 * @note Неограниченный предел отдаётся как наибольшее представимое
			 *       значение, а не как ноль или признак отсутствия
			 *
			 * @return количество файловых дескрипторов установленных в файловой системе
			 *
			 * \~english
			 * @brief Method of getting the limit of the file descriptors set in the operating system
			 * @details Returns a pair: the first one is the **soft** limit — the one that
			 *          is in force now, — the second one is the **hard** ceiling up to which
			 *          the soft one is allowed to be raised without special rights
			 * @note An unlimited limit is given back as the largest representable
			 *       value, and not as zero or as a sign of an absence
			 * @return number of the file descriptors set in the file system
			 *
			 * \~
			 */
			std::pair <uint32_t, uint32_t> limit() const noexcept;
			/**
			 * \~russian
			 * @brief Метод оценки лимита одновременно открытых сокетов
			 *
			 * @details Проверяет опытом, а не настройками: открывает сокеты один за
			 *          другим, пока открываются, считает удавшиеся и закрывает их все.
			 *          Показывает, сколько соединений приложение сможет держать на
			 *          самом деле, - величина эта бывает ниже разрешённого предела,
			 *          потому что упереться можно и в общесистемные настройки, и в
			 *          память
			 *
			 * @note Проверка не бесплатна: она в самом деле открывает и закрывает
			 *       десятки тысяч сокетов. Уместна на старте или в диагностике, но не
			 *       на рабочем пути
			 *
			 * @param max верхний предел пробинга (0 - использовать значение по умолчанию)
			 * @return    пара значений (оценка доступного количества сокетов, верхний предел пробинга)
			 *
			 * \~english
			 * @brief Method of the estimation of the limit of the simultaneously open sockets
			 * @details Checks by an experiment, and not by the settings: opens the sockets one after
			 *          another, while they open, counts the successful ones and closes them all.
			 *          Shows how many connections the application will be able to hold in
			 *          reality — that value happens to be lower than the allowed limit,
			 *          because it is possible to run into the system-wide settings as well, and into
			 *          the memory
			 * @note The check is not free: it really opens and closes
			 *       tens of thousands of sockets. Is appropriate at the startup or in the diagnostics, but not
			 *       on the working path
			 * @param max upper limit of the probing (0 — use the value by default)
			 * @return    pair of the values (estimation of the available number of the sockets, upper limit of the probing)
			 *
			 * \~
			 */
			std::pair <uint32_t, uint32_t> sockets(const uint32_t max = 0) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param log объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Files_Descriptors(const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			~Files_Descriptors() noexcept;
	} fds_t;
};

#endif // __AWH_EVENT_FDS_BASE__
