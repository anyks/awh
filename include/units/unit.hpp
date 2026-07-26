/**
 * @file: unit.hpp
 * @date: 2026-02-20
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл базового класса модулей — класс unit::Unit,
 *        задающий общий контракт всех модулей библиотеки: привязка к движку ввода-вывода, идентификация события,
 *        управление состоянием и подписка на обратные вызовы
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT__
#define __AWH_UNIT__

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../net/io.hpp"
#include "../sys/signals.hpp"
#include "../sys/callback.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён модулей
	 *
	 */
	namespace unit {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * @brief Класс базового модуля
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Unit {
			protected:
				// Идентификатор процесса
				pid_t _pid;
			private:
				// Таймаут опроса базы событий
				int32_t _timeout;
			private:
				// Флаг активации перехвата сигналов
				event::mode_t _intercep;
			protected:
				// Статус работы модуля
				event::status_t _status;
			private:
				// Объект работы с сигналами
				signals_t _signals;
			protected:
				// Хранилище функций обратного вызова
				callback_t _callback;
			protected:
				// Объект асинхронного сетевого движка
				engine::io_t * _io;
			protected:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			private:
				/**
				 * @brief Метод вывода полученного сигнала
				 *
				 * @param sig идентификатор сигнала
				 *
				 */
				void signal(const int32_t sig) const noexcept;
			public:
				/**
				 * @brief Метод принудительного пинка базе событий
				 *
				 * @return результат выполнения операции
				 *
				 */
				bool kick() noexcept;
			public:
				/**
				 * @brief Метод определения мастер-процесса
				 *
				 * @return результат проверки
				 *
				 */
				bool master() const noexcept;
			public:
				/**
				 * @brief Метод проверки на запуск работы
				 *
				 * @return результат проверки
				 *
				 */
				bool working() const noexcept;
			public:
				/**
				 * @brief Метод очистки базы событий
				 *
				 */
				void clear() noexcept;
			public:
				/**
				 * @brief Метод реинициализации базы событий
				 *
				 */
				void reinit() noexcept;
			public:
				/**
				 * @brief Метод получения количества событий в базе событий
				 *
				 * @return количество событий
				 *
				 */
				size_t events() const noexcept;
			public:
				/**
				 * @brief Метод остановки юнита
				 *
				 * @note База событий едина на весь процесс. Реально цикл событий останавливает только
				 *       юнит-лаунчер (тот, что его запустил): он сбрасывает флаг работы и будит базу.
				 *       Остальные юниты лишь переводят себя в статус DESTROYED, не затрагивая общий цикл.
				 *
				 */
				virtual void stop() noexcept;
				/**
				 * @brief Метод запуска юнита
				 *
				 * @note База событий едина на весь процесс. Запустить цикл событий может только один
				 *       юнит-лаунчер: захват выполняется атомарно (compare_exchange), и для него вызов
				 *       блокирующий — поток крутится в цикле опроса до остановки. Остальные юниты,
				 *       обнаружив, что база уже работает, цикл не запускают, не блокируются и лишь
				 *       переводят себя в статус LAUNCHED.
				 *
				 */
				virtual void start() noexcept;
			public:
				/**
				 * @brief Метод перестройки события: пересоздание нижележащего дескриптора с сохранением события
				 *
				 * @note Дескриптор пересоздаётся, а само событие (его идентификатор, коллбэки,
				 *       параметры) сохраняется - подмена дескриптора приложению незаметна
				 *
				 * @param eid идентификатор события
				 * @return    результат выполнения перестройки
				 *
				 */
				bool rebuild(const event::id_t eid) noexcept;
			public:
				/**
				 * @brief Метод получения типа события
				 *
				 * @param eid идентификатор события
				 * @return    тип события
				 *
				 */
				event::type_t type(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод получения типа узла события
				 *
				 * @param eid идентификатор события
				 * @return    тип узла события
				 *
				 */
				event::node_t node(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод получения семейства события
				 *
				 * @param eid идентификатор события
				 * @return    семейство адресов
				 *
				 */
				event::family_t family(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод получения статуса события
				 *
				 * @param eid идентификатор события
				 * @return    статус события
				 *
				 */
				event::status_t status(const event::id_t eid) const noexcept;
				/**
				 * @brief Метод получения протокола события
				 *
				 * @param id идентификатор события
				 * @return   протокол события
				 *
				 */
				event::protocol_t protocol(const event::id_t id) const noexcept;
			public:
				/**
				 * @brief Метод получения типа внутренних таймеров
				 *
				 * @return тип таймера для событий сетевого движка
				 *
				 */
				event::timer_t getInternalTimer() const noexcept;
				/**
				 * @brief Метод установки типа внутренних таймеров
				 *
				 * @param timer тип таймера для событий сетевого движка
				 *
				 */
				void setInternalTimer(const event::timer_t timer) noexcept;
			public:
				/**
				 * @brief Метод установки пропускной способности события
				 *
				 * @param limiting  режим ограничения пропускной способности события (egress или ingress)
				 * @param bandwidth пропускная способность события для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 *
				 */
				void bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept;
			public:
				/**
				 * @brief Метод установки времени блокировки базы событий в ожидании событий
				 *
				 * @param timeout время ожидания событий в миллисекундах
				 *
				 */
				void rate(const int32_t timeout = -1) noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 *
				 */
				virtual void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 *
				 */
				template <typename T, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param name идентификатор функции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 *
				 */
				auto on(const char * name, Args... args) noexcept -> uint32_t {
					// Если мы получили название функции обратного вызова
					if(name != nullptr)
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (name, args...);
					// Возвращаем значение по умолчанию
					return 0;
				}
				/**
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 *
				 */
				template <typename T, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param name идентификатор функции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 *
				 */
				auto on(string_view name, Args... args) noexcept -> uint32_t {
					// Если мы получили название функции обратного вызова
					if(!name.empty())
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (name, args...);
					// Возвращаем значение по умолчанию
					return 0;
				}
				/**
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 *
				 */
				template <typename T, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param name идентификатор функции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 *
				 */
				auto on(const string & name, Args... args) noexcept -> uint32_t {
					// Если мы получили название функции обратного вызова
					if(!name.empty())
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (name, args...);
					// Возвращаем значение по умолчанию
					return 0;
				}
				/**
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam T    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 *
				 */
				template <typename T, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param fid  идентификатор функции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 *
				 */
				auto on(const uint32_t fid, Args... args) noexcept -> uint32_t {
					// Если мы получили идентификатор функции обратного вызова
					if(fid > 0)
						// Выполняем установку функции обратного вызова
						return this->_callback.on <T> (fid, args...);
					// Возвращаем значение по умолчанию
					return 0;
				}
				/**
				 * @brief Шаблон метода подключения финкции обратного вызова
				 *
				 * @tparam A    тип идентификатора функции
				 * @tparam B    тип функции обратного вызова
				 * @tparam Args аргументы функции обратного вызова
				 *
				 */
				template <typename A, typename B, class... Args>
				/**
				 * @brief Метод подключения финкции обратного вызова
				 *
				 * @param fid  идентификатор функции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 *
				 */
				auto on(const A fid, Args... args) noexcept -> uint32_t {
					// Если мы получили на вход число
					if constexpr (is_arithmetic_v <A> || is_enum_v <A>)
						// Выполняем установку функции обратного вызова
						return this->_callback.on <B> (static_cast <uint32_t> (fid), args...);
					// Возвращаем значение по умолчанию
					return 0;
				}
			public:
				/**
				 * @brief Метод активации/деактивации перехвата сигналов
				 *
				 * @param mode флаг активации
				 *
				 */
				void interception(const event::mode_t mode) noexcept;
			private:
				/**
				 * @brief Конструктор копирования (запрещаем)
				 *
				 */
				Unit(const Unit &) = delete;
				/**
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 */
				Unit & operator = (const Unit &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 */
				explicit Unit(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~Unit() noexcept;
		} unit_t;
	};
};

#endif // __AWH_UNIT__
