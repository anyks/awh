/**
 * @file: cluster.hpp
 * @date: 2026-02-21
 * @license: GPL-3.0
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
#ifndef __AWH_CLUSTER__
#define __AWH_CLUSTER__

/**
 * Стандартные модули
 */
#include <unordered_map>
#include <unordered_set>

/**
 * Наши модули
 */
#include "unit.hpp"
#include "../sys/crypto.hpp"
#include "../sys/compressor.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён узал источника
	 *
	 */
	namespace unit {
		/**
		 * Подписываемся на стандартное пространство имён
		 */
		using namespace std;
		/**
		 * @brief Класс базового узла источника
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Cluster : public unit_t {
			public:
				/**
				 * События работы кластера
				 */
				enum class event_t : uint8_t {
					STOP  = 0x00, // Событие остановки процесса
					START = 0x01  // Событие запуска процесса
				};
				/**
				 * Семейство кластера
				 */
				enum class family_t : uint8_t {
					NONE     = 0x00, // Воркер не установлено
					MASTER   = 0x02, // Воркер является мастером
					CHILDREN = 0x01  // Воркер является ребёнком
				};
				/**
				 * Тип завершения работы кластера
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
				 */
				typedef struct Worker {
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
					explicit Worker() noexcept :
					 pid(0), life(0), eid(0) {}
				} worker_t;
			private:
				// Название кластера
				string _name;
			private:
				// Флаг автоматического возрождения процессов
				bool _rebirth;
			private:
				// Количество воркеров
				uint16_t _count;
			private:
				// Тип протокола передачи данных между воркерами
				event::type_t _type;
			private:
				// Список соответствия идентификаторов событий и идентификаторов процессов
				unordered_map <event::id_t, pid_t> _accord;
				// Список активных воркеров
				unordered_map <pid_t, unique_ptr <worker_t>> _workers;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
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
				 */
				void emplace(const pid_t pid) noexcept;
			private:
				/**
				 * @brief Метод запуска/остановки работы кластера
				 *
				 * @param status статус запуска/остановки кластера
				 */
				void launch(const event::status_t status) noexcept;
			private:
				/**
				 * @brief Метод перезапуска упавшего процесса
				 *
				 * @param pid    идентификатор упавшего процесса
				 * @param status статус остановившегося процесса
				 */
				void process(const pid_t pid, const int32_t status) noexcept;
				/**
				 * @brief Функция фильтр перехватчика сигналов
				 *
				 * @param signal номер сигнала полученного системой
				 * @param info   объект информации полученный системой
				 * @param ctx    передаваемый внутренний контекст
				 */
				static void child(int32_t signal, siginfo_t * info, void * ctx) noexcept;
			private:
				/**
				 * @brief Метод обработки событий записи сообщений кластера
				 *
				 * @param eid  идентификатор события
				 * @param size размер сообщения
				 */
				void write(const event::id_t eid, const size_t size) noexcept;
				/**
				 * @brief Метод обработки событий чтения сообщений кластера
				 *
				 * @param eid  идентификатор события
				 * @param data данные сообщения
				 * @param size размер сообщения
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * @brief Метод обработки событий кластера
				 *
				 * @param eid    идентификатор события
				 * @param status статус события
				 */
				void status(const event::id_t eid, const event::status_t status) noexcept;
			private:
				/**
				 * @brief Метод обработки исключений событий кластера
				 *
				 * @param eid идентификатор события
				 * @param error тип ошибки
				 * @param message сообщение об ошибке
				 */
				void error(const event::id_t eid, const event::error_t error, const string & message) noexcept;
			private:
				/**
				 * @brief Метод обработки событий доступного размера очереди события кластера
				 *
				 * @param eid    идентификатор события
				 * @param status статус события
				 * @param size   доступный размер очереди в байтах
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
				 */
				void erase(const pid_t pid, const shutdown_t shutdown) noexcept;
			public:
				/**
				 * @brief Метод установки флага автоматического возрождения процессов
				 *
				 * @param mode флаг возрождения процессов
				 */
				void rebirth(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод установки названия кластера
				 *
				 * @param name название кластера для установки
				 */
				void name(const string & name) noexcept;
			public:
				/**
				 * @brief Метод получения максимального количества процессов
				 *
				 * @return максимальное количество процессов
				 */
				uint16_t count() const noexcept;
				/**
				 * @brief Метод установки максимального количества процессов
				 *
				 * @param count максимальное количество процессов
				 */
				void count(const uint16_t count) noexcept;
			public:
				/**
				 * @brief Метод получения списка дочерних процессов
				 *
				 * @return список дочерних процессов
				 */
				unordered_set <pid_t> workers() const noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения родительскому процессу
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 */
				size_t send(const void * buffer, const size_t size) noexcept;
				/**
				 * @brief Метод отправки сообщения дочернему процессу
				 *
				 * @param pid    идентификатор процесса для получения сообщения
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 */
				size_t send(const pid_t pid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения всем дочерним процессам
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 */
				size_t broadcast(const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод получения размера буфера события
				 *
				 * @param pid    идентификатор процесса
				 * @param action тип действия события
				 * @return       размер буфера события
				 */
				size_t bufferSize(const pid_t pid, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки размера буфера события
				 *
				 * @param pid    идентификатор процесса
				 * @param action тип действия события
				 * @param size   размер буфера события
				 * @return       результат выполнения установки
				 */
				bool bufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept;
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
				 */
				Cluster & operator = (const Cluster &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
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

#endif // __AWH_CLUSTER__
