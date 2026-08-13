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
 * \~russian
 * @brief Заголовочный файл модуля кластера — класс unit::Cluster для запуска и контроля дочерних воркеров,
 *        обмена сообщениями между процессами, перезапуска упавших воркеров и защиты от цикла быстрых перезапусков
 *
 * \~english
 * @brief Header file of the cluster module — the unit::Cluster class for launching and controlling the child workers,
 *        exchanging messages between the processes, restarting the fallen workers and protecting against a loop of fast restarts
 *
 * \~
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
	 * \~russian
	 * @brief Пространство имён модулей
	 *
	 *
	 * \~english
	 * @brief Modules namespace
	 *
	 * \~
	 */
	namespace unit {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * \~russian
		 * @brief Класс узла кластера
		 *
		 * \~english
		 * @brief Cluster unit class
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Cluster : public unit_t {
			public:
				/**
				 * \~russian
				 * @brief События работы кластера
				 *
				 * \~english
				 * @brief Events of the work of the cluster
				 *
				 * \~
				 */
				enum class event_t : uint8_t {
					STOP  = 0x00, // Событие остановки процесса
					START = 0x01  // Событие запуска процесса
				};
				/**
				 * \~russian
				 * @brief Семейство кластера
				 *
				 * \~english
				 * @brief Cluster family
				 *
				 * \~
				 */
				enum class family_t : uint8_t {
					NONE     = 0x00, // Воркер не установлен
					MASTER   = 0x02, // Воркер является мастером
					CHILDREN = 0x01  // Воркер является ребёнком
				};
				/**
				 * \~russian
				 * @brief Тип завершения работы кластера
				 *
				 * \~english
				 * @brief Type of the termination of the work of the cluster
				 *
				 * \~
				 */
				enum class shutdown_t : uint8_t {
					NONE     = 0x00, // Тип завершения работы кластера не определён
					GRACEFUL = 0x01, // Тип завершения работы кластера - плавное завершение
					FORCEFUL = 0x02  // Тип завершения работы кластера - принудительное завершение
				};
			private:
				/**
				 * \~russian
				 * @brief Структура воркера
				 *
				 * @details Содержит идентификатор процесса, время жизни и идентификатор события для обмена сообщениями между процессами.
				 *
				 * \~english
				 * @brief Worker structure
				 * @details Contains the process identifier, the lifetime and the event identifier for exchanging messages between the processes.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Worker {
					// Идентификатор процесса
					pid_t pid;
					// Время начала жизни процесса
					uint64_t life;
					// Идентификаторы события для обмена сообщениями между процессами
					event::id_t eid;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Worker() noexcept;
				} __attribute__((packed)) worker_t;
				/**
				 * \~russian
				 * @brief Структура хранения параметров возрождения процессов
				 *
				 * @details Содержит флаг автоматического возрождения процессов,
				 *          максимальное число подряд идущих быстрых падений воркеров до остановки кластера,
				 *          временное окно «быстрого» (раннего) падения воркера в миллисекундах и счётчик подряд идущих быстрых (ранних) падений воркеров для защиты от цикла перезапусков.
				 *
				 * \~english
				 * @brief Structure for storing the parameters of the revival of the processes
				 * @details Contains the flag of the automatic revival of the processes,
				 *          the maximum number of consecutive fast falls of the workers before the cluster is stopped,
				 *          the time window of a «fast» (early) fall of a worker in milliseconds and the counter of the consecutive fast (early) falls of the workers for protection against a loop of restarts.
				 *
				 * \~
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
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Rebirth() noexcept;
				} __attribute__((packed)) rebirth_t;
			private:
				// Название кластера
				string _name;
			private:
				/**
				 * \~russian
				 * Признак отвергнутого кластера
				 *
				 * @details Кластер в процессе может быть только один: перехватчик сигнала
				 *          о состоянии дочернего процесса заводится один на процесс и несёт
				 *          указатель на кластер, которому эти дочерние процессы принадлежат.
				 *          Второй кластер этот указатель бы перехватил, и события процессов
				 *          первого доставались бы второму
				 *
				 * @note Прежде второй кластер отвергался броском исключения из конструктора,
				 *       объявленного noexcept, - то есть завершал процесс немедленно вместо
				 *       того, чтобы отдать отказ вызывающей стороне. Исключения в движке не
				 *       применяются, а конструктор об отказе сообщить не может вовсе, потому
				 *       отвергнутый кластер помечается признаком: перехватчика он не трогает,
				 *       к запуску не допускается и говорит об этом в журнал
				 *
				 * \~english
				 * Flag of a rejected cluster
				 * @details There can be only one cluster in a process: the interceptor of the signal
				 *          about the state of a child process is created one per process and carries
				 *          a pointer to the cluster to which those child processes belong.
				 *          A second cluster would intercept that pointer, and the events of the processes
				 *          of the first one would go to the second
				 * @note Previously a second cluster was rejected by throwing an exception out of the constructor
				 *       declared noexcept — that is, it terminated the process immediately instead
				 *       of returning a refusal to the calling side. Exceptions are not used in the engine,
				 *       and the constructor cannot report a refusal at all, therefore
				 *       a rejected cluster is marked with a flag: it does not touch the interceptor,
				 *       it is not allowed to launch and it says so in the log
				 *
				 * \~
				 */
				bool _rejected;
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
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					/**
					 * \~russian
					 * Дескриптор объекта родительского процесса (0 у мастера и до захвата)
					 *
					 * @details Хранится числом, а не типом `HANDLE`, чтобы открытый заголовок
					 *          не тянул за собой системные заголовки MS Windows. По этому
					 *          дескриптору дочерний процесс узнаёт, жив ли мастер: у MS Windows
					 *          нет ни `getppid`, ни устойчивой связи «родитель — потомок», и
					 *          номер процесса система переиспользует
					 *
					 * \~english
					 * Handle of the object of the parent process (0 in the master and before the capture)
					 * @details Stored as a number rather than as the `HANDLE` type so that the public header
					 *          does not drag the system headers of MS Windows along with it. By that
					 *          handle the child process learns whether the master is alive: MS Windows has
					 *          neither `getppid` nor a stable «parent — child» link, and
					 *          the system reuses the process number
					 *
					 * \~
					 */
					uintptr_t _master;
				#endif
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
				 * \~russian
				 * @brief Метод проверки, что родительский процесс жив
				 *
				 * @return признак того, что родительский процесс жив
				 *
				 * @details Дочерний процесс, потерявший мастера, обязан самоликвидироваться:
				 *          принимать сообщения ему больше не от кого, а канал обмена
				 *          сообщениями остаётся открытым. Проверка эта — единственное место,
				 *          где такое осиротевание распознаётся
				 *
				 * @note У мастера метод отвечает ложью: собственного родителя кластер не знает
				 *       и не отслеживает, поэтому вызывать метод следует только из ветвей
				 *       дочернего процесса
				 *
				 * \~english
				 * @brief Method of checking that the parent process is alive
				 * @return flag of the parent process being alive
				 * @details A child process that has lost its master is obliged to self-destruct:
				 *          there is no one left for it to receive messages from, while the message
				 *          exchange channel remains open. This check is the only place
				 *          where such an orphaning is recognized
				 * @note In the master the method answers with a lie: the cluster does not know and does not track
				 *       its own parent, therefore the method should be called only from the branches
				 *       of the child process
				 *
				 * \~
				 */
				bool parent() const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод создания дочерних процессов при запуске кластера
				 *
				 * \~english
				 * @brief Method of creating the child processes at the launch of the cluster
				 *
				 * \~
				 */
				void create() noexcept;
				/**
				 * \~russian
				 * @brief Метод размещения нового дочернего процесса
				 *
				 * @param pid идентификатор убитого процесса
				 *
				 * \~english
				 * @brief Method of placing a new child process
				 * @param pid identifier of the killed process
				 *
				 * \~
				 */
				void emplace(const pid_t pid) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод освобождения ресурсов воркера
				 *
				 * @param eid идентификатор события воркера
				 *
				 * \~english
				 * @brief Method of releasing the resources of a worker
				 * @param eid event identifier of the worker
				 *
				 * \~
				 */
				void release(const event::id_t eid) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод запуска/остановки работы кластера
				 *
				 * @param status статус запуска/остановки кластера
				 *
				 * \~english
				 * @brief Method of launching/stopping the work of the cluster
				 * @param status status of the launch/stop of the cluster
				 *
				 * \~
				 */
				void launch(const event::status_t status) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод создания одного дочернего процесса (воркера)
				 *
				 * @param replaced идентификатор замещаемого (упавшего) процесса, либо 0 при первичном создании
				 * @param deferred флаг отложенного запуска события (true — фиксация/запуск выполняются позже пакетно)
				 * @return         семейство процесса: MASTER — родитель, CHILDREN — дочерний, NONE — ошибка создания
				 *
				 * \~english
				 * @brief Method of creating a single child process (worker)
				 * @param replaced identifier of the process being replaced (the fallen one), or 0 at the primary creation
				 * @param deferred flag of the deferred launch of the event (true — the commit/launch are performed later in a batch)
				 * @return         family of the process: MASTER — the parent, CHILDREN — the child, NONE — a creation error
				 *
				 * \~
				 */
				family_t spawn(const pid_t replaced, const bool deferred) noexcept;
			private:
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					/**
					 * \~russian
					 * @brief Метод порождения дочернего процесса повторным запуском образа приложения
					 *
					 * @return идентификатор порождённого процесса, либо 0 при отказе
					 *
					 * @details Метода этого на системах POSIX нет вовсе: дочерний процесс
					 *          достаётся там вызовом `fork`, продолжающим работу с того же
					 *          места и с тем же состоянием. У MS Windows соответствия `fork`
					 *          нет, и дочерний процесс запускается заново - собственным
					 *          образом приложения, с той же строкой доводов и с меткой роли
					 *          в окружении
					 *
					 * @warning Отсюда следует, что код, лежащий в `main` до `cluster.start()`,
					 *          в дочернем процессе исполняется повторно
					 *
					 * \~english
					 * @brief Method of spawning a child process by launching the image of the application anew
					 * @return identifier of the spawned process, or 0 on a refusal
					 * @details There is no such method on the POSIX systems at all: a child process
					 *          is obtained there by the `fork` call, which continues the work from the same
					 *          place and with the same state. MS Windows has no counterpart of `fork`,
					 *          and the child process is launched anew — by its own
					 *          image of the application, with the same argument line and with a mark of the role
					 *          in the environment
					 * @warning It follows from this that the code lying in `main` before `cluster.start()`
					 *          is executed once more in the child process
					 *
					 * \~
					 */
					pid_t execute() noexcept;
					/**
					 * \~russian
					 * @brief Метод распознавания роли дочернего процесса и захвата мастера
					 *
					 * @return признак того, что процесс является дочерним
					 *
					 * @details Дочерний процесс запускается тем же образом и с той же строкой
					 *          доводов, что и мастер, и отличает его лишь метка окружения
					 *          `AWH_CLUSTER_MASTER`, выставленная мастером перед запуском.
					 *          Распознав себя дочерним, процесс перенимает номер мастера в
					 *          поле `_pid` и открывает дескриптор его объекта, по которому
					 *          затем и отвечает метод `parent`
					 *
					 * @note Метка снимается из окружения сразу после разбора: порождай воркер
					 *       собственные процессы, тем метка досталась бы по наследству, и те
					 *       сочли бы себя воркерами несуществующего мастера
					 *
					 * \~english
					 * @brief Method of recognizing the role of the child process and capturing the master
					 * @return flag of the process being a child one
					 * @details A child process is launched by the same image and with the same argument
					 *          line as the master, and it is distinguished only by the environment mark
					 *          `AWH_CLUSTER_MASTER` set by the master before the launch.
					 *          Having recognized itself as a child one, the process takes over the number of the master into
					 *          the `_pid` field and opens the handle of its object, by which
					 *          the `parent` method then answers
					 * @note The mark is removed from the environment right after the parsing: were a worker to spawn
					 *       processes of its own, the mark would be inherited by them, and they
					 *       would consider themselves workers of a non-existent master
					 *
					 * \~
					 */
					bool adopt() noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия своего конца канала обмена сообщениями с мастером
					 *
					 * @return признак того, что канал обмена сообщениями открыт
					 *
					 * @details Соответствия socketpair у MS Windows нет, и пара обмена строится
					 *          именованным каналом. Сторону ожидания заводит мастер, а имя её
					 *          передаёт порождаемому процессу окружением `AWH_CLUSTER_PIPE`:
					 *          дескриптора тот по наследству не получает, проходя main заново,
					 *          зато имя переносимо
					 *
					 * @note Метода этого на системах POSIX нет вовсе: там оба конца пары
					 *       достаются дочернему процессу вызовом fork
					 *
					 * \~english
					 * @brief Method of opening one's own end of the message exchange channel with the master
					 * @return flag of the message exchange channel being open
					 * @details MS Windows has no counterpart of socketpair, and the exchange pair is built
					 *          with a named pipe. The waiting side is created by the master, while its name
					 *          is passed to the spawned process through the `AWH_CLUSTER_PIPE` environment:
					 *          the latter does not receive the descriptor by inheritance, passing through main anew,
					 *          while the name is portable
					 * @note There is no such method on the POSIX systems at all: there both ends of the pair
					 *       go to the child process by the fork call
					 *
					 * \~
					 */
					bool attach() noexcept;
				#endif
			private:
				/**
				 * \~russian
				 * @brief Метод перезапуска упавшего процесса
				 *
				 * @param pid    идентификатор упавшего процесса
				 * @param status статус остановившегося процесса
				 *
				 * \~english
				 * @brief Method of restarting a fallen process
				 * @param pid    identifier of the fallen process
				 * @param status status of the stopped process
				 *
				 * \~
				 */
				void process(const pid_t pid, const int32_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод отложенной обработки завершившихся процессов (выполняется в цикле событий)
				 *
				 * @param eid  идентификатор события пробуждения
				 * @param data данные события пробуждения
				 * @param size размер данных события пробуждения
				 *
				 * \~english
				 * @brief Method of the deferred processing of the terminated processes (performed in the event loop)
				 * @param eid  event identifier of the wake-up
				 * @param data data of the wake-up event
				 * @param size data size of the wake-up event
				 *
				 * \~
				 */
				void reap(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * Для операционных систем, отличных от MS Windows
				 */
				#if !_WIN32 && !_WIN64
					/**
					 * \~russian
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
					 * \~english
					 * @brief Filter function of the signal interceptor
					 * @param signal number of the signal received by the system
					 * @param info   information object received by the system
					 * @param ctx    internal context being passed
					 * @note Under MS Windows the method has no body: the SIGCHLD signal does not
					 *       exist there, and the termination of a child process is announced by
					 *       the waiting on the process object from the system thread pool.
					 *       The wake-up of the event loop is common in both cases — the
					 *       `_wakeup` event, while the parsing of the terminated ones is done by the `reap` method
					 *
					 * \~
					 */
					static void child(int32_t signal, siginfo_t * info, void * ctx) noexcept;
				#endif
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					/**
					 * \~russian
					 * @brief Функция извещения о завершении дочернего процесса
					 *
					 * @param ctx     идентификатор завершившегося процесса
					 * @param timeout признак срабатывания по истечении срока ожидания
					 *
					 * @details Вызывается системным пулом потоков, а не потоком петли
					 *          событий. Поэтому не делает ничего, кроме постановки номера
					 *          процесса в очередь и пробуждения петли: разбор ведёт метод
					 *          `reap` уже в своём потоке. Соображение то же, что и у
					 *          обработчика сигнала SIGCHLD на системах POSIX
					 *
					 * @note Написание доводов повторяет WAITORTIMERCALLBACK, но своими
					 *       словами: `void *` вместо PVOID и `uint8_t` вместо BOOLEAN.
					 *       Сделано так, чтобы открытый заголовок не тянул за собой
					 *       заголовки MS Windows
					 *
					 * \~english
					 * @brief Function of the notification about the termination of a child process
					 * @param ctx     identifier of the terminated process
					 * @param timeout flag of the triggering by the expiration of the waiting term
					 * @details Called by the system thread pool rather than by the thread of the event
					 *          loop. Therefore it does nothing except placing the number of the
					 *          process into the queue and waking the loop up: the parsing is done by the
					 *          `reap` method already in its own thread. The consideration is the same as that of
					 *          the handler of the SIGCHLD signal on the POSIX systems
					 * @note The spelling of the arguments repeats WAITORTIMERCALLBACK, but in its own
					 *       words: `void *` instead of PVOID and `uint8_t` instead of BOOLEAN.
					 *       This has been done so that the public header does not drag the
					 *       headers of MS Windows along with it
					 *
					 * \~
					 */
					static void __stdcall child(void * ctx, uint8_t timeout) noexcept;
				#endif
			private:
				/**
				 * \~russian
				 * @brief Метод обработки событий записи сообщений кластера
				 *
				 * @param eid  идентификатор события
				 * @param size размер сообщения
				 *
				 * \~english
				 * @brief Method of processing cluster message write events
				 * @param eid  event identifier
				 * @param size message size
				 *
				 * \~
				 */
				void write(const event::id_t eid, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки событий чтения сообщений кластера
				 *
				 * @param eid  идентификатор события
				 * @param data данные сообщения
				 * @param size размер сообщения
				 *
				 * \~english
				 * @brief Method of processing cluster message read events
				 * @param eid  event identifier
				 * @param data message data
				 * @param size message size
				 *
				 * \~
				 */
				void read(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки состояния кластера
				 *
				 * @param eid    идентификатор события
				 * @param status статус события
				 *
				 * \~english
				 * @brief Method of processing the cluster state
				 * @param eid    event identifier
				 * @param status event status
				 *
				 * \~
				 */
				void state(const event::id_t eid, const event::status_t status) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки исключений событий кластера
				 *
				 * @param eid     идентификатор события
				 * @param error   тип ошибки
				 * @param message сообщение об ошибке
				 *
				 * \~english
				 * @brief Method of processing cluster event exceptions
				 * @param eid     event identifier
				 * @param error   error type
				 * @param message error message
				 *
				 * \~
				 */
				void error(const event::id_t eid, const event::error_t error, const string & message) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки событий доступного размера очереди события кластера
				 *
				 * @param eid    идентификатор события
				 * @param status статус события
				 * @param size   доступный размер очереди в байтах
				 *
				 * \~english
				 * @brief Method of processing available queue size events of the cluster event
				 * @param eid    event identifier
				 * @param status event status
				 * @param size   available queue size in bytes
				 *
				 * \~
				 */
				void available(const event::id_t eid, const event::status_t status, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @name Разбор состояния завершения процесса
				 *
				 * @details Обработчик события `"exit"` получает состояние завершения процесса
				 *          в том виде, в каком его отдаёт система: у POSIX это упакованное
				 *          состояние ожидания от `waitpid`, у MS Windows - значение
				 *          `GetExitCodeProcess`. Значения эти к общему виду **не приводятся**
				 *          намеренно: оба несут и признак того, как процесс кончился, и то,
				 *          с чем именно, и сведение их к общему знаменателю только отняло бы
				 *          часть сведений. У POSIX пропал бы код возврата, а у MS Windows -
				 *          коды NTSTATUS, какими та помечает падения (`0xC0000005` - обращение
				 *          по недопустимому адресу, `0xC00000FD` - переполнение стека)
				 *
				 *          Методы ниже позволяют задавать состоянию переносимые вопросы, не
				 *          разбирая при этом платформу
				 *
				 * \~english
				 * @name Parsing of the termination state of a process
				 * @details The handler of the `"exit"` event receives the termination state of the process
				 *          in the form in which the system returns it: on POSIX this is the packed
				 *          waiting state from `waitpid`, on MS Windows it is the value of
				 *          `GetExitCodeProcess`. Those values are **not** brought to a common form
				 *          deliberately: both carry the sign of how the process has ended and
				 *          what exactly it has ended with, and reducing them to a common denominator would only take away
				 *          a part of the information. On POSIX the return code would be lost, and on MS Windows —
				 *          the NTSTATUS codes with which it marks the crashes (`0xC0000005` — an access
				 *          at an invalid address, `0xC00000FD` — a stack overflow)
				 *          The methods below make it possible to ask the state portable questions without
				 *          parsing the platform
				 *
				 * \~
				 *
				 * @{
				 *
				 */
				/**
				 * \~russian
				 * @brief Метод проверки, завершился ли процесс сам
				 *
				 * @param status состояние завершения процесса
				 * @return       признак того, что процесс завершился сам
				 *
				 * \~english
				 * @brief Method of checking whether the process has terminated by itself
				 * @param status termination state of the process
				 * @return       flag of the process having terminated by itself
				 *
				 * \~
				 */
				static bool exited(const int32_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения кода возврата завершившегося процесса
				 *
				 * @param status состояние завершения процесса
				 * @return       код возврата процесса, либо EXIT_FAILURE при завершении ненормальном
				 *
				 * \~english
				 * @brief Method of getting the return code of the terminated process
				 * @param status termination state of the process
				 * @return       return code of the process, or EXIT_FAILURE on an abnormal termination
				 *
				 * \~
				 */
				static int32_t exitcode(const int32_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки, снят ли процесс сигналом
				 *
				 * @param status состояние завершения процесса
				 * @return       признак того, что процесс снят сигналом
				 *
				 * @note У MS Windows отвечает ложью всегда: сигналов там нет вовсе.
				 *       Переносимый вопрос «кончился ли процесс ненормально» задаётся
				 *       методом `crashed`, а не этим
				 *
				 * \~english
				 * @brief Method of checking whether the process has been removed by a signal
				 * @param status termination state of the process
				 * @return       flag of the process having been removed by a signal
				 * @note On MS Windows it always answers with a lie: there are no signals there at all.
				 *       The portable question «has the process ended abnormally» is asked by
				 *       the `crashed` method rather than by this one
				 *
				 * \~
				 */
				static bool signaled(const int32_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения номера сигнала, снявшего процесс
				 *
				 * @param status состояние завершения процесса
				 * @return       номер сигнала, либо 0 если процесс снят не сигналом
				 *
				 * @note У MS Windows отвечает нулём всегда - см. пояснение к `signaled`
				 *
				 * \~english
				 * @brief Method of getting the number of the signal that has removed the process
				 * @param status termination state of the process
				 * @return       signal number, or 0 if the process has not been removed by a signal
				 * @note On MS Windows it always answers with zero — see the explanation of `signaled`
				 *
				 * \~
				 */
				static int32_t termsig(const int32_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки, завершился ли процесс ненормально
				 *
				 * @param status состояние завершения процесса
				 * @return       признак ненормального завершения процесса
				 *
				 * @details Вопрос этот переносим, в отличие от `signaled`: у POSIX
				 *          ненормальным считается снятие сигналом краха (SIGSEGV, SIGBUS,
				 *          SIGILL, SIGFPE, SIGABRT), у MS Windows - код завершения с
				 *          признаком ошибки в старших разрядах, каким система помечает
				 *          необработанные исключения
				 *
				 * \~english
				 * @brief Method of checking whether the process has terminated abnormally
				 * @param status termination state of the process
				 * @return       flag of the abnormal termination of the process
				 * @details This question is portable, unlike `signaled`: on POSIX
				 *          a removal by a crash signal (SIGSEGV, SIGBUS,
				 *          SIGILL, SIGFPE, SIGABRT) is considered abnormal, on MS Windows — a termination code with
				 *          the error flag in the high bits, with which the system marks
				 *          the unhandled exceptions
				 *
				 * \~
				 */
				static bool crashed(const int32_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки, снят ли процесс с клавиатуры
				 *
				 * @param status состояние завершения процесса
				 * @return       признак ручной остановки процесса
				 *
				 * @details Ручной остановкой считается прерывание с клавиатуры: сигнал SIGINT
				 *          у POSIX и код завершения STATUS_CONTROL_C_EXIT у MS Windows, каким
				 *          та помечает процесс, снятый по нажатию Ctrl+C или Ctrl+Break
				 *
				 * \~english
				 * @brief Method of checking whether the process has been removed from the keyboard
				 * @param status termination state of the process
				 * @return       flag of the manual stop of the process
				 * @details A manual stop is an interruption from the keyboard: the SIGINT signal
				 *          on POSIX and the STATUS_CONTROL_C_EXIT termination code on MS Windows, with which
				 *          the latter marks a process removed by pressing Ctrl+C or Ctrl+Break
				 *
				 * \~
				 */
				static bool manual(const int32_t status) noexcept;
				/** @} */
			public:
				/**
				 * \~russian
				 * @brief Метод остановки кластера
				 *
				 * \~english
				 * @brief Method of stopping the cluster
				 *
				 * \~
				 */
				void stop() noexcept;
				/**
				 * \~russian
				 * @brief Метод запуска кластера
				 *
				 * \~english
				 * @brief Method of launching the cluster
				 *
				 * \~
				 */
				void start() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки всех выделенных ресурсов
				 *
				 * @param shutdown тип завершения работы кластера
				 *
				 * \~english
				 * @brief Method of clearing all the allocated resources
				 * @param shutdown type of the termination of the work of the cluster
				 *
				 * \~
				 */
				void clear(const shutdown_t shutdown) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод размещения нового дочернего процесса
				 *
				 * \~english
				 * @brief Method of placing a new child process
				 *
				 * \~
				 */
				void emplace() noexcept;
				/**
				 * \~russian
				 * @brief Метод удаления активного процесса
				 *
				 * @param pid      идентификатор процесса
				 * @param shutdown тип завершения работы кластера
				 *
				 * \~english
				 * @brief Method of removing an active process
				 * @param pid      process identifier
				 * @param shutdown type of the termination of the work of the cluster
				 *
				 * \~
				 */
				void erase(const pid_t pid, const shutdown_t shutdown) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения типа протокола передачи данных между воркерами
				 *
				 * @return тип протокола передачи данных между воркерами
				 *
				 * \~english
				 * @brief Method of getting the type of the data transfer protocol between the workers
				 * @return type of the data transfer protocol between the workers
				 *
				 * \~
				 */
				event::type_t getTypeEventMessage() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки типа протокола передачи данных между воркерами
				 *
				 * @param type тип протокола передачи данных между воркерами для установки
				 *
				 * \~english
				 * @brief Method of setting the type of the data transfer protocol between the workers
				 * @param type type of the data transfer protocol between the workers to be set
				 *
				 * \~
				 */
				void setTypeEventMessage(const event::type_t type) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки флага автоматического возрождения процессов
				 *
				 * @param mode флаг возрождения процессов
				 *
				 * \~english
				 * @brief Method of setting the flag of the automatic revival of the processes
				 * @param mode flag of the revival of the processes
				 *
				 * \~
				 */
				void rebirth(const bool mode) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки параметров защиты от цикла перезапусков воркеров
				 *
				 * @param limit  максимальное число подряд идущих быстрых падений до остановки кластера (0 — без ограничения)
				 * @param window временное окно «быстрого» (раннего) падения воркера в миллисекундах
				 *
				 * \~english
				 * @brief Method of setting the parameters of the protection against a loop of restarts of the workers
				 * @param limit  maximum number of consecutive fast falls before the cluster is stopped (0 — without a limit)
				 * @param window time window of a «fast» (early) fall of a worker in milliseconds
				 *
				 * \~
				 */
				void rebirthLimit(const uint16_t limit, const uint64_t window) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки названия кластера
				 *
				 * @param name название кластера для установки
				 *
				 * \~english
				 * @brief Method of setting the name of the cluster
				 * @param name name of the cluster to be set
				 *
				 * \~
				 */
				void name(string_view name) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения максимального количества процессов
				 *
				 * @return максимальное количество процессов
				 *
				 * \~english
				 * @brief Method of getting the maximum number of the processes
				 * @return maximum number of the processes
				 *
				 * \~
				 */
				uint16_t count() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки максимального количества процессов
				 *
				 * @param count максимальное количество процессов
				 *
				 * \~english
				 * @brief Method of setting the maximum number of the processes
				 * @param count maximum number of the processes
				 *
				 * \~
				 */
				void count(const uint16_t count) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения списка дочерних процессов
				 *
				 * @return список дочерних процессов
				 *
				 * \~english
				 * @brief Method of getting the list of the child processes
				 * @return list of the child processes
				 *
				 * \~
				 */
				unordered_set <pid_t> workers() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 *
				 *
				 * \~english
				 * @brief Method of setting the callback functions
				 * @param callback callback functions
				 *
				 * \~
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отправки сообщения родительскому процессу
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 * \~english
				 * @brief Method of sending a message to the parent process
				 * @param buffer binary buffer for sending the message
				 * @param size   size of the binary buffer for sending the message
				 * @return       number of bytes of the sent message
				 *
				 * \~
				 */
				size_t send(const void * buffer, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки сообщения дочернему процессу
				 *
				 * @param pid    идентификатор процесса для получения сообщения
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 * \~english
				 * @brief Method of sending a message to a child process
				 * @param pid    identifier of the process for receiving the message
				 * @param buffer binary buffer for sending the message
				 * @param size   size of the binary buffer for sending the message
				 * @return       number of bytes of the sent message
				 *
				 * \~
				 */
				size_t send(const pid_t pid, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отправки сообщения всем дочерним процессам
				 *
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 * \~english
				 * @brief Method of sending a message to all the child processes
				 * @param buffer binary buffer for sending the message
				 * @param size   size of the binary buffer for sending the message
				 * @return       number of bytes of the sent message
				 *
				 * \~
				 */
				size_t broadcast(const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения размера буфера события
				 *
				 * @param pid    идентификатор процесса
				 * @param action тип действия события
				 * @return       размер буфера события
				 *
				 * \~english
				 * @brief Method of getting the event buffer size
				 * @param pid    process identifier
				 * @param action event action type
				 * @return       event buffer size
				 *
				 * \~
				 */
				size_t getBufferSize(const pid_t pid, const event::action_t action) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки размера буфера события
				 *
				 * @param pid    идентификатор процесса
				 * @param action тип действия события
				 * @param size   размер буфера события
				 * @return       результат выполнения установки
				 *
				 * \~english
				 * @brief Method of setting the event buffer size
				 * @param pid    process identifier
				 * @param action event action type
				 * @param size   event buffer size
				 * @return       result of performing the setting
				 *
				 * \~
				 */
				bool setBufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Конструктор копирования (запрещаем)
				 *
				 *
				 * \~english
				 * @brief Copy constructor (prohibited)
				 *
				 * \~
				 */
				Cluster(const Cluster &) = delete;
				/**
				 * \~russian
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 *
				 * \~english
				 * @brief Copy assignment operator (prohibited)
				 * @return current value of the object
				 *
				 * \~
				 */
				Cluster & operator = (const Cluster &) = delete;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 *
				 * \~english
				 * @brief Constructor
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Cluster(const fmk_t * fmk, const log_t * log) noexcept;
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
				~Cluster() noexcept;
		} cluster_t;
	};
};

#endif // __AWH_UNIT_CLUSTER__
