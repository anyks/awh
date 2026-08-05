/**
 * @file: cluster.hpp
 * @date: 2026-02-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл модуля кластера — класс unit::Cluster для запуска и контроля дочерних воркеров,
 *        обмена сообщениями между процессами, перезапуска упавших воркеров и защиты от цикла быстрых перезапусков
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT_CLUSTER__
#define __AWH_UNIT_CLUSTER__

/**
 * Стандартные заголовочные файлы
 */
#include <unordered_map>
#include <unordered_set>

/**
 * Подключаем заголовочный файл проекта
 */
#include "unit.hpp"

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
		 * @brief Класс узла кластера
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Cluster : public unit_t {
			public:
				/**
				 * @brief События работы кластера
				 *
				 */
				enum class event_t : uint8_t {
					STOP  = 0x00, // Событие остановки процесса
					START = 0x01  // Событие запуска процесса
				};
				/**
				 * @brief Семейство кластера
				 *
				 */
				enum class family_t : uint8_t {
					NONE     = 0x00, // Воркер не установлен
					MASTER   = 0x02, // Воркер является мастером
					CHILDREN = 0x01  // Воркер является ребёнком
				};
				/**
				 * @brief Тип завершения работы кластера
				 *
				 */
				enum class shutdown_t : uint8_t {
					NONE     = 0x00, // Тип завершения работы кластера не определён
					GRACEFUL = 0x01, // Тип завершения работы кластера - плавное завершение
					FORCEFUL = 0x02  // Тип завершения работы кластера - принудительное завершение
				};
			private:
				/**
				 * @brief Структура воркера
				 *
				 * @details Содержит идентификатор процесса, время жизни и идентификатор события для обмена сообщениями между процессами.
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Worker {
					// Идентификатор процесса
					pid_t pid;
					// Время начала жизни процесса
					uint64_t life;
					// Идентификаторы события для обмена сообщениями между процессами
					event::id_t eid;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Worker() noexcept;
				} __attribute__((packed)) worker_t;
				/**
				 * @brief Структура хранения параметров возрождения процессов
				 *
				 * @details Содержит флаг автоматического возрождения процессов,
				 *          максимальное число подряд идущих быстрых падений воркеров до остановки кластера,
				 *          временное окно «быстрого» (раннего) падения воркера в миллисекундах и счётчик подряд идущих быстрых (ранних) падений воркеров для защиты от цикла перезапусков.
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Rebirth {
					// Флаг автоматического возрождения процессов (по умолчанию false)
					bool mode;
					// Максимальное число подряд идущих быстрых падений воркеров до остановки кластера (0 — без ограничения, по умолчанию 10)
					uint16_t limit;
					// Временное окно «быстрого» (раннего) падения воркера в миллисекундах (по умолчанию 30000)
					uint64_t window;
					// Счётчик подряд идущих быстрых (ранних) падений воркеров для защиты от цикла перезапусков (по умолчанию 0)
					uint16_t restarts;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Rebirth() noexcept;
				} __attribute__((packed)) rebirth_t;
			private:
				// Название кластера
				string _name;
			private:
				// Количество воркеров
				uint16_t _count;
			private:
				// Параметры возрождения процессов
				rebirth_t _rebirth;
			private:
				// Идентификатор события пробуждения для отложенной обработки сигнала SIGCHLD
				event::id_t _wakeup;
			private:
				// Тип протокола передачи данных между воркерами
				event::type_t _type;
			private:
				// Список соответствия идентификаторов событий и идентификаторов процессов
				unordered_map <event::id_t, pid_t> _matching;
				// Список активных воркеров
				unordered_map <pid_t, unique_ptr <worker_t>> _workers;
			private:
				/**
				 * @brief Метод создания дочерних процессов при запуске кластера
				 *
				 */
				void create() noexcept;
				/**
				 * @brief Метод размещения нового дочернего процесса
				 *
				 * @param pid идентификатор убитого процесса
				 *
				 */
				void emplace(const pid_t pid) noexcept;
			private:
				/**
				 * @brief Метод освобождения ресурсов воркера
				 *
				 * @param eid идентификатор события воркера
				 *
				 */
				void release(const event::id_t eid) noexcept;
			private:
				/**
				 * @brief Метод запуска/остановки работы кластера
				 *
				 * @param status статус запуска/остановки кластера
				 *
				 */
				void launch(const event::status_t status) noexcept;
			private:
				/**
				 * @brief Метод создания одного дочернего процесса (воркера)
				 *
				 * @param replaced идентификатор замещаемого (упавшего) процесса, либо 0 при первичном создании
				 * @param deferred флаг отложенного запуска события (true — фиксация/запуск выполняются позже пакетно)
				 * @return         семейство процесса: MASTER — родитель, CHILDREN — дочерний, NONE — ошибка создания
				 *
				 */
				family_t spawn(const pid_t replaced, const bool deferred) noexcept;
			private:
				/**
				 * @brief Метод перезапуска упавшего процесса
				 *
				 * @param pid    идентификатор упавшего процесса
				 * @param status статус остановившегося процесса
				 *
				 */
				void process(const pid_t pid, const int32_t status) noexcept;
				/**
				 * @brief Метод отложенной обработки завершившихся процессов (выполняется в цикле событий)
				 *
				 * @param eid  идентификатор события пробуждения
				 * @param data данные события пробуждения
				 * @param size размер данных события пробуждения
				 *
				 */
				void reap(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * Для операционных систем, отличных от MS Windows
				 */
				#if !_WIN32 && !_WIN64
					/**
					 * @brief Функция фильтр перехватчика сигналов
					 *
					 * @param signal номер сигнала полученного системой
					 * @param info   объект информации полученный системой
					 * @param ctx    передаваемый внутренний контекст
					 *
					 * @note Под MS Windows тела у метода нет: сигнала SIGCHLD там не
					 *       существует, и о завершении дочернего процесса извещает
					 *       ожидание объекта процесса из системного пула потоков.
					 *       Пробуждение цикла событий в обоих случаях общее — событие
					 *       `_wakeup`, а разбор завершившихся ведёт метод `reap`
					 *
					 */
					static void child(int32_t signal, siginfo_t * info, void * ctx) noexcept;
				#endif
			private:
				/**
				 * @brief Метод обработки событий записи сообщений кластера
				 *
				 * @param eid  идентификатор события
				 * @param size размер сообщения
				 *
				 */
				void write(const event::id_t eid, const size_t size) noexcept;
				/**
				 * @brief Метод обработки событий чтения сообщений кластера
				 *
				 * @param eid  идентификатор события
				 * @param data данные сообщения
				 * @param size размер сообщения
				 *
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * @brief Метод обработки состояния кластера
				 *
				 * @param eid    идентификатор события
				 * @param status статус события
				 *
				 */
				void state(const event::id_t eid, const event::status_t status) noexcept;
			private:
				/**
				 * @brief Метод обработки исключений событий кластера
				 *
				 * @param eid     идентификатор события
				 * @param error   тип ошибки
				 * @param message сообщение об ошибке
				 *
				 */
				void error(const event::id_t eid, const event::error_t error, const string & message) noexcept;
			private:
				/**
				 * @brief Метод обработки событий доступного размера очереди события кластера
				 *
				 * @param eid    идентификатор события
				 * @param status статус события
				 * @param size   доступный размер очереди в байтах
				 *
				 */
				void available(const event::id_t eid, const event::status_t status, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод остановки кластера
				 *
				 */
				void stop() noexcept;
				/**
				 * @brief Метод запуска кластера
				 *
				 */
				void start() noexcept;
			public:
				/**
				 * @brief Метод очистки всех выделенных ресурсов
				 *
				 * @param shutdown тип завершения работы кластера
				 *
				 */
				void clear(const shutdown_t shutdown) noexcept;
			public:
				/**
				 * @brief Метод размещения нового дочернего процесса
				 *
				 */
				void emplace() noexcept;
				/**
				 * @brief Метод удаления активного процесса
				 *
				 * @param pid      идентификатор процесса
				 * @param shutdown тип завершения работы кластера
				 *
				 */
				void erase(const pid_t pid, const shutdown_t shutdown) noexcept;
			public:
				/**
				 * @brief Метод получения типа протокола передачи данных между воркерами
				 *
				 * @return тип протокола передачи данных между воркерами
				 *
				 */
				event::type_t getTypeEventMessage() const noexcept;
				/**
				 * @brief Метод установки типа протокола передачи данных между воркерами
				 *
				 * @param type тип протокола передачи данных между воркерами для установки
				 *
				 */
				void setTypeEventMessage(const event::type_t type) noexcept;
			public:
				/**
				 * @brief Метод установки флага автоматического возрождения процессов
				 *
				 * @param mode флаг возрождения процессов
				 *
				 */
				void rebirth(const bool mode) noexcept;
				/**
				 * @brief Метод установки параметров защиты от цикла перезапусков воркеров
				 *
				 * @param limit  максимальное число подряд идущих быстрых падений до остановки кластера (0 — без ограничения)
				 * @param window временное окно «быстрого» (раннего) падения воркера в миллисекундах
				 *
				 */
				void rebirthLimit(const uint16_t limit, const uint64_t window) noexcept;
			public:
				/**
				 * @brief Метод установки названия кластера
				 *
				 * @param name название кластера для установки
				 *
				 */
				void name(string_view name) noexcept;
			public:
				/**
				 * @brief Метод получения максимального количества процессов
				 *
				 * @return максимальное количество процессов
				 *
				 */
				uint16_t count() const noexcept;
				/**
				 * @brief Метод установки максимального количества процессов
				 *
				 * @param count максимальное количество процессов
				 *
				 */
				void count(const uint16_t count) noexcept;
			public:
				/**
				 * @brief Метод получения списка дочерних процессов
				 *
				 * @return список дочерних процессов
				 *
				 */
				unordered_set <pid_t> workers() const noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 *
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения родительскому процессу
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 */
				size_t send(const void * buffer, const size_t size) noexcept;
				/**
				 * @brief Метод отправки сообщения дочернему процессу
				 *
				 * @param pid    идентификатор процесса для получения сообщения
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 */
				size_t send(const pid_t pid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения всем дочерним процессам
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 */
				size_t broadcast(const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод получения размера буфера события
				 *
				 * @param pid    идентификатор процесса
				 * @param action тип действия события
				 * @return       размер буфера события
				 *
				 */
				size_t getBufferSize(const pid_t pid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки размера буфера события
				 *
				 * @param pid    идентификатор процесса
				 * @param action тип действия события
				 * @param size   размер буфера события
				 * @return       результат выполнения установки
				 *
				 */
				bool setBufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept;
			private:
				/**
				 * @brief Конструктор копирования (запрещаем)
				 *
				 */
				Cluster(const Cluster &) = delete;
				/**
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 */
				Cluster & operator = (const Cluster &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 */
				explicit Cluster(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Cluster() noexcept;
		} cluster_t;
	};
};

#endif // __AWH_UNIT_CLUSTER__
