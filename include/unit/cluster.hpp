/**
 * @file cluster.hpp
 * @date 2026-02-21
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
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
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

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
				/**
				 * \~russian
				 * @brief Причины отказа в заведении связи
				 *
				 * @note Отказ называет причину, а не молчит: вызывающей стороне решать,
				 *       повторять ли заказ, и по молчаливому отказу решить это нечем
				 *
				 * \~english
				 * @brief Reasons of the refusal to establish a link
				 *
				 * @note The refusal names the reason rather than keeping silent: it is up to the calling side
				 *       to decide whether to repeat the order, and a silent refusal gives nothing to decide by
				 *
				 * \~
				 */
				enum class reason_t : uint8_t {
					NONE    = 0x00, // Причина отказа не определена
					UNKNOWN = 0x01, // Названного узла в кластере нет
					ITSELF  = 0x02, // Узел заказал связь с самим собой
					REFUSED = 0x03, // Мастер связь запретил
					EXISTS  = 0x04, // Связь между этими узлами уже заведена
					FAILED  = 0x05  // Пара не завелась либо снимок не снялся
				};
				/**
				 * \~russian
				 * @brief Виды служебных сообщений кластера
				 *
				 * @details Идут они своим каналом, отдельным от пользовательского, и
				 *          устройство его задано `SEQPACKET` безусловно: границы сообщений
				 *          держит сам канал, и разбирать заголовок не приходится. Разбор
				 *          устройства целиком - в `src/unit/CLUSTER-LINK.md`.
				 *
				 * \~english
				 * @brief Kinds of the service messages of the cluster
				 *
				 * @details They go by their own channel, separate from the user one, and
				 *          its layout is set to `SEQPACKET` unconditionally: the boundaries of the messages
				 *          are held by the channel itself, and the header does not have to be parsed. The whole
				 *          arrangement is described in `src/unit/CLUSTER-LINK.md`.
				 *
				 * \~
				 */
				enum class control_t : uint8_t {
					NONE     = 0x00, // Вид служебного сообщения не определён
					ONLINE   = 0x01, // Работник поднялся и дошёл до цикла событий
					OFFLINE  = 0x02, // Работник уходит сам
					LINK     = 0x03, // Заказ прямой связи с названным узлом
					UNLINK   = 0x04, // Связь с названным узлом больше не нужна
					RELAY    = 0x05, // Пересылка сообщения через мастера
					GRANT    = 0x06, // Связь заведена, подъём своего конца
					DENY     = 0x07, // Связь не заведена
					DROP     = 0x08, // Связь с названным узлом разорвана
					JOIN     = 0x09, // В кластер вошёл названный узел
					LEAVE    = 0x0A, // Названный узел из кластера выбыл
					SHUTDOWN = 0x0B  // Завершить работу названным кодом
				};
			public:
				/**
				 * \~russian
				 * @brief Код завершения работника, когда мастер кода не назвал
				 *
				 * @details Постоянная эта - кластерная, а не системный номер сигнала:
				 *          `SIGUSR1` у Microsoft Windows не значит ничего, и код завершения
				 *          сложился бы там не тот, какого ждали. Нужен потребителю номер
				 *          сигнала - он волен назвать его сам.
				 *
				 * \~english
				 * @brief Exit code of a worker when the master has not named a code
				 *
				 * @details This constant is the cluster's own one rather than a system number of a signal:
				 *          `SIGUSR1` means nothing on Microsoft Windows, and the termination code
				 *          would come out there not as the one expected. Should the consumer need a number
				 *          of a signal — he is free to name it himself.
				 *
				 * \~
				 */
				static constexpr int32_t SHUTDOWN_CODE = 0x0A;
			private:
				/**
				 * \~russian
				 * @brief Упаковка структуры воркера
				 *
				 * \~english
				 * @brief Packaging the worker structure
				 *
				 * \~
				 */
				__AWH_PACK_BEGIN__
				/**
				 * \~russian
				 * @brief Структура воркера
				 *
				 * @details Содержит идентификатор процесса, время жизни и идентификатор
				 *          события для обмена сообщениями между процессами.
				 *
				 * \~english
				 * @brief Worker structure
				 *
				 * @details Contains the process identifier, the lifetime and the event
				 *          identifier for exchanging messages between the processes.
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
					// Идентификатор события служебного канала обмена между процессами
					event::id_t cid;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Worker() noexcept;
				} __AWH_PACKED__ worker_t;
				__AWH_PACK_END__
				/**
				 * \~russian
				 * @brief Упаковка структуры хранения параметров возрождения процессов
				 *
				 * \~english
				 * @brief Packaging of the storage structure for process-revival parameters
				 *
				 * \~
				 */
				__AWH_PACK_BEGIN__
				/**
				 * \~russian
				 * @brief Структура хранения параметров возрождения процессов
				 *
				 * @details Содержит флаг автоматического возрождения процессов,
				 *          максимальное число подряд идущих быстрых падений воркеров до остановки кластера,
				 *          временное окно «быстрого» (раннего) падения воркера в миллисекундах и счётчик
				 *          подряд идущих быстрых (ранних) падений воркеров для защиты от цикла перезапусков.
				 *
				 * \~english
				 * @brief Structure for storing the parameters of the revival of the processes
				 *
				 * @details Contains the flag of the automatic revival of the processes,
				 *          the maximum number of consecutive fast falls of the workers before the cluster is stopped,
				 *          the time window of a «fast» (early) fall of a worker in milliseconds and the counter of
				 *          the consecutive fast (early) falls of the workers for protection against a loop of restarts.
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
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Rebirth() noexcept;
				} __AWH_PACKED__ rebirth_t;
				__AWH_PACK_END__
			private:
				// Название кластера
				string _name;
			private:
				/**
				 * \~russian
				 * @brief Признак отвергнутого кластера
				 *
				 * @details Кластер в процессе может быть только один: перехватчик сигнала
				 *          о состоянии дочернего процесса заводится один на процесс и несёт
				 *          указатель на кластер, которому эти дочерние процессы принадлежат.
				 *          Второй кластер этот указатель бы перехватил, и события процессов
				 *          первого доставались бы второму.
				 *
				 * @note Прежде второй кластер отвергался броском исключения из конструктора,
				 *       объявленного noexcept, - то есть завершал процесс немедленно вместо
				 *       того, чтобы отдать отказ вызывающей стороне. Исключения в движке не
				 *       применяются, а конструктор об отказе сообщить не может вовсе, потому
				 *       отвергнутый кластер помечается признаком: перехватчика он не трогает,
				 *       к запуску не допускается и говорит об этом в журнал
				 *
				 * \~english
				 * @brief Flag of a rejected cluster
				 *
				 * @details There can be only one cluster in a process: the interceptor of the signal
				 *          about the state of a child process is created one per process and carries
				 *          a pointer to the cluster to which those child processes belong.
				 *          A second cluster would intercept that pointer, and the events of the processes
				 *          of the first one would go to the second.
				 *
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
				/**
				 * \~russian
				 * @brief Идентификатор события пробуждения для отложенной обработки сигнала SIGCHLD
				 *
				 * \~english
				 * @brief Wake-up event identifier for deferred processing of the SIGCHLD signal
				 *
				 * \~
				 */
				event::id_t _wakeup;
				/**
				 * \~russian
				 * @brief Идентификатор своего служебного события у работника
				 *
				 * @note У мастера поле пустует: служебных каналов у него столько же,
				 *       сколько работников, и лежат они у самих работников в поле `cid`
				 *
				 * \~english
				 * @brief Identifier of one's own service event in a worker
				 *
				 * @note In the master the field stays empty: it has as many service channels
				 *       as it has workers, and they lie in the workers themselves in the `cid` field
				 *
				 * \~
				 */
				event::id_t _control;
			private:
				/**
				 * Для операционной системы Microsoft Windows
				 */
				#if defined(_WIN32) || defined(_WIN64) || defined(DOXYGEN)
					/**
					 * \~russian
					 * @brief Дескриптор объекта родительского процесса (0 у мастера и до захвата)
					 *
					 * @details Хранится числом, а не типом `HANDLE`, чтобы открытый заголовок
					 *          не тянул за собой системные заголовки Microsoft Windows. По этому
					 *          дескриптору дочерний процесс узнаёт, жив ли мастер: у Microsoft Windows
					 *          нет ни `getppid`, ни устойчивой связи «родитель — потомок», и
					 *          номер процесса система переиспользует.
					 *
					 * \~english
					 * @brief Handle of the object of the parent process (0 in the master and before the capture)
					 *
					 * @details Stored as a number rather than as the `HANDLE` type so that the public header
					 *          does not drag the system headers of Microsoft Windows along with it. By that
					 *          handle the child process learns whether the master is alive: Microsoft Windows has
					 *          neither `getppid` nor a stable «parent — child» link, and
					 *          the system reuses the process number.
					 *
					 * \~
					 */
					uintptr_t _master;
				#endif
			private:
				// Тип протокола передачи данных между воркерами
				event::type_t _type;
			private:
				// Список известных узлов кластера (у работника)
				unordered_set <pid_t> _nodes;
				/**
				 * \~russian
				 * @brief Список узлов, поднявшихся и дошедших до цикла событий (у мастера)
				 *
				 * @details Порождённый узел работником ещё не является: дошёл ли он до
				 *          цикла, мастер узнаёт извещением `ONLINE`, а не порождением.
				 *          Список этот и есть единственная правда о том, кому извещения доходят.
				 *
				 * @warning Без него извещения о входе двоились: новичку уходил список
				 *          ВСЕХ порождённых узлов, а затем каждый из них получал ещё и
				 *          извещение о новичке - и заказ связи уходил дважды
				 *
				 * \~english
				 * @brief List of the nodes that have risen and reached the event loop (in the master)
				 *
				 * @details A spawned node is not a worker yet: whether it has reached the loop
				 *          the master learns by the `ONLINE` notification rather than by the spawning.
				 *          This list is the only truth about whom the notifications reach.
				 *
				 *
				 * @warning Without it the notifications of an entering were doubled: to a newcomer went a list
				 *         of ALL spawned nodes, and then each of them got also
				 *         a notification about a newcomer - and an order of a connection was spent twice
				 *
				 * \~
				 */
				unordered_set <pid_t> _online;
				// Список узлов, уходящих намеренно, - возрождению они не подлежат (у мастера)
				unordered_set <pid_t> _leaving;
			private:
				// Список заведённых связей работника: узел и его событие обмена
				unordered_map <pid_t, event::id_t> _peers;
			private:
				// Список соответствия идентификаторов событий и идентификаторов процессов
				unordered_map <event::id_t, pid_t> _matching;
				// Список соответствия идентификаторов служебных событий и идентификаторов процессов
				unordered_map <event::id_t, pid_t> _controls;
				// Список соответствия идентификаторов событий связей и идентификаторов узлов
				unordered_map <event::id_t, pid_t> _matchingPeers;
			private:
				// Список заведённых связей между работниками (у мастера)
				unordered_map <pid_t, unordered_set <pid_t>> _links;
				// Список активных рабочих процессов
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
				 *          где такое осиротевание распознаётся.
				 *
				 * @note У мастера метод отвечает ложью: собственного родителя кластер не знает
				 *       и не отслеживает, поэтому вызывать метод следует только из ветвей
				 *       дочернего процесса
				 *
				 * \~english
				 * @brief Method of checking that the parent process is alive
				 *
				 * @return flag of the parent process being alive
				 *
				 * @details A child process that has lost its master is obliged to self-destruct:
				 *          there is no one left for it to receive messages from, while the message
				 *          exchange channel remains open. This check is the only place
				 *          where such an orphaning is recognized.
				 *
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
				 *
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
				 *
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
				 *
				 * @param status status of the launch/stop of the cluster
				 *
				 * \~
				 */
				void launch(const event::status_t status) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод разбора разорванной прямой связи
				 *
				 * @note Разбор идемпотентен намеренно: признаков разрыва два, и приходят они
				 *       оба - вторым заходом связи по событию уже не находится, и потребитель
				 *       извещается ровно однажды
				 *
				 * @param eid идентификатор события связи
				 *
				 * \~english
				 * @brief Method of the disassembling of a broken direct link
				 *
				 * @note A parsing is idempotent on purpose: there are two signs of a rupture, and both of them
				 *      come - by a second coming a connection by an event is already not found, and a consumer
				 *      is notified exactly once
				 *
				 * @param eid identifier of the link event
				 *
				 * \~
				 */
				void sever(const event::id_t eid) noexcept;
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
				 *
				 * @param replaced identifier of the process being replaced (the fallen one), or 0 at the primary creation
				 * @param deferred flag of the deferred launch of the event (true — the commit/launch are performed later in a batch)
				 * @return         family of the process: MASTER — the parent, CHILDREN — the child, NONE — a creation error
				 *
				 * \~
				 */
				family_t spawn(const pid_t replaced, const bool deferred) noexcept;
				/**
				 * Для операционной системы Microsoft Windows
				 */
				#if defined(_WIN32) || defined(_WIN64) || defined(DOXYGEN)
				private:
					/**
					 * \~russian
					 * @brief Метод распознавания роли дочернего процесса и захвата мастера
					 *
					 * @details Дочерний процесс запускается тем же образом и с той же строкой
					 *          доводов, что и мастер, и отличает его лишь метка окружения
					 *          `AWH_CLUSTER_MASTER`, выставленная мастером перед запуском.
					 *          Распознав себя дочерним, процесс перенимает номер мастера в
					 *          поле `_pid` и открывает дескриптор его объекта, по которому
					 *          затем и отвечает метод `parent`.
					 *
					 * @note Метка снимается из окружения сразу после разбора: порождай воркер
					 *       собственные процессы, тем метка досталась бы по наследству, и те
					 *       сочли бы себя воркерами несуществующего мастера
					 *
					 * @return признак того, что процесс является дочерним
					 *
					 * \~english
					 * @brief Method of recognizing the role of the child process and capturing the master
					 *
					 * @details A child process is launched by the same image and with the same argument
					 *          line as the master, and it is distinguished only by the environment mark
					 *          `AWH_CLUSTER_MASTER` set by the master before the launch.
					 *          Having recognized itself as a child one, the process takes over the number of the master into
					 *          the `_pid` field and opens the handle of its object, by which
					 *          the `parent` method then answers.
					 *
					 * @note The mark is removed from the environment right after the parsing: were a worker to spawn
					 *       processes of its own, the mark would be inherited by them, and they
					 *       would consider themselves workers of a non-existent master
					 *
					 * @return flag of the process being a child one
					 *
					 * \~
					 */
					bool adopt() noexcept;
					/**
					 * \~russian
					 * @brief Метод открытия своего конца канала обмена сообщениями с мастером
					 *
					 * @details Соответствия socketpair у Microsoft Windows нет, и пара обмена строится
					 *          именованным каналом. Сторону ожидания заводит мастер, а имя её
					 *          передаёт порождаемому процессу окружением `AWH_CLUSTER_PIPE`:
					 *          дескриптора тот по наследству не получает, проходя main заново,
					 *          зато имя переносимо.
					 *
					 * @note Метода этого на системах POSIX нет вовсе: там оба конца пары
					 *       достаются дочернему процессу вызовом fork
					 *
					 * @return признак того, что канал обмена сообщениями открыт
					 *
					 * \~english
					 * @brief Method of opening one's own end of the message exchange channel with the master
					 *
					 * @details Microsoft Windows has no counterpart of socketpair, and the exchange pair is built
					 *          with a named pipe. The waiting side is created by the master, while its name
					 *          is passed to the spawned process through the `AWH_CLUSTER_PIPE` environment:
					 *          the latter does not receive the descriptor by inheritance, passing through main anew,
					 *          while the name is portable.
					 *
					 * @note There is no such method on the POSIX systems at all: there both ends of the pair
					 *       go to the child process by the fork call
					 *
					 * @return flag of the message exchange channel being open
					 *
					 * \~
					 */
					bool attach() noexcept;
					/**
					 * \~russian
					 * @brief Метод порождения дочернего процесса повторным запуском образа приложения
					 *
					 * @details Метода этого на системах POSIX нет вовсе: дочерний процесс
					 *          достаётся там вызовом `fork`, продолжающим работу с того же
					 *          места и с тем же состоянием. У Microsoft Windows соответствия `fork`
					 *          нет, и дочерний процесс запускается заново - собственным
					 *          образом приложения, с той же строкой доводов и с меткой роли
					 *          в отдельной копии окружения без изменения переменных мастера.
					 *
					 * @warning Отсюда следует, что код, лежащий в `main` до `cluster.start()`,
					 *          в дочернем процессе исполняется повторно
					 *
					 * @param eid идентификатор пользовательского канала
					 * @param cid идентификатор служебного канала
					 * @return    идентификатор порождённого процесса, либо 0 при отказе
					 *
					 * \~english
					 * @brief Method of spawning a child process by launching the image of the application anew
					 *
					 * @details There is no such method on the POSIX systems at all: a child process
					 *          is obtained there by the `fork` call, which continues the work from the same
					 *          place and with the same state. Microsoft Windows has no counterpart of `fork`,
					 *          and the child process is launched anew — by its own
					 *          image of the application, with the same argument line and with a mark of the role
					 *          in a separate environment copy without changing the master variables.
					 *
					 * @warning It follows from this that the code lying in `main` before `cluster.start()`
					 *          is executed once more in the child process
					 *
					 * @param eid identifier of the user channel
					 * @param cid identifier of the control channel
					 * @return    identifier of the spawned process, or 0 on a refusal
					 *
					 * \~
					 */
					pid_t execute(const event::id_t eid, const event::id_t cid) noexcept;
					/**
					 * \~russian
					 * @brief Метод заведения своего конца канала обмена для порождаемого процесса
					 *
					 * @details Сводит в одно место весь порядок, каким конец канала доводится
					 *          до ожидания подключения ПРЕЖДЕ порождения процесса: заведение
					 *          пары, проверку имени канала, уничтожение конца работника,
					 *          подписку откликов, фиксацию, запуск и ожидание готовности.
					 *          Порядок этот тонкий и добыт щупом на стендах, и разводить его
					 *          по двум каналам - пользовательскому и служебному - значило бы
					 *          завести второе место, где его надо помнить.
					 *
					 * @note Служебный канал отличается одними лишь откликами: наружу он не
					 *       выходит вовсе, и пользовательские отклики его не касаются
					 *
					 * @note Метода этого на системах POSIX нет вовсе: там оба конца пары
					 *       достаются дочернему процессу вызовом fork
					 *
					 * @param type    устройство обмена канала
					 * @param service признак служебного канала
					 * @return        идентификатор заведённого события, либо 0 при отказе
					 *
					 * \~english
					 * @brief Method of the establishment of one's own end of the exchange channel for the spawned process
					 *
					 * @details It gathers into one place the whole order by which an end of a channel is brought
					 *         up to a waiting of a connection BEFORE a spawning of a process: a starting
					 *         of a pair, a check of the channel name, a destruction of an end of a worker,
					 *         a subscribing of the callbacks, a fixation, a starting and a waiting of a readiness.
					 *         This order is thin and was got by a probe on the stands, and to spread it
					 *         over two channels - a user one and a service one - would mean to start a second place,
					 *         where it has to be remembered.
					 *
					 * @note A service channel differs by the callbacks only: outward it does not
					 *       go at all, and the user callbacks do not touch it
					 *
					 * @note There is no such method at all on the POSIX systems: there both ends of a pair
					 *       are got by a child process by a call of fork
					 *
					 * @param type    layout of the exchange of the channel
					 * @param service flag of the service channel
					 * @return        identifier of the established event, or 0 on a refusal
					 *
					 * \~
					 */
					event::id_t provision(const event::type_t type, const bool service) noexcept;
				#endif
			private:
				/**
				 * \~russian
				 * @brief Метод обработки состояния прямой связи с соседом
				 *
				 * @param eid    идентификатор события связи
				 * @param status статус события
				 *
				 * \~english
				 * @brief Method of processing the state of a direct link with a neighbour
				 *
				 * @param eid    identifier of the link event
				 * @param status status of the event
				 *
				 * \~
				 */
				void broken(const event::id_t eid, const event::status_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод обработки закрытия прямой связи с соседом
				 *
				 * @details Признаков разрыва у движка два, и приходят они в разные мгновения:
				 *          действие `CLOSE` - сразу, в самый миг закрытия, а состояние
				 *          `DESTROYED` - позже, когда движок доберётся до изъятия узла.
				 *          Подписаны оба: первый даёт скорость, второй ловит те пути изъятия,
				 *          где действия не было. Оба ведут к одному разбору, и повторного
				 *          извещения потребителя не выходит - разбор ищет связь по событию, а
				 *          вторым заходом уже не находит.
				 *
				 * @param eid    идентификатор события связи
				 * @param action действие, случившееся с событием
				 *
				 * \~english
				 * @brief Method of processing the closing of a direct link with a neighbour
				 *
				 * @details An engine has two signs of a rupture, and they come in different moments:
				 *         an action `CLOSE` - at once, in the very moment of a closing, and a state
				 *         `DESTROYED` - later, when an engine reaches a taking out of a node.
				 *         Both are subscribed: a first one gives a speed, a second one catches those paths of a taking out,
				 *         where there was no action. Both lead to one parsing, and a repeated
				 *         notification of a consumer does not come out - a parsing searches a connection by an event, and
				 *         by a second coming it already does not find.
				 *
				 * @param eid    identifier of the link event
				 * @param action action that has happened to the event
				 *
				 * \~
				 */
				void closed(const event::id_t eid, const event::action_t action) noexcept;
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
				 *
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
				 *
				 * @param eid  event identifier of the wake-up
				 * @param data data of the wake-up event
				 * @param size data size of the wake-up event
				 *
				 * \~
				 */
				void reap(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * Для операционных систем, отличных от Microsoft Windows
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
					/**
					 * \~russian
					 * @brief Функция фильтр перехватчика сигналов
					 *
					 * @note Под Microsoft Windows тела у метода нет: сигнала SIGCHLD там не
					 *       существует, и о завершении дочернего процесса извещает
					 *       ожидание объекта процесса из системного пула потоков.
					 *       Пробуждение цикла событий в обоих случаях общее — событие
					 *       `_wakeup`, а разбор завершившихся ведёт метод `reap`
					 *
					 * @param signal номер сигнала полученного системой
					 * @param info   объект информации полученный системой
					 * @param ctx    передаваемый внутренний контекст
					 *
					 * \~english
					 * @brief Filter function of the signal interceptor
					 *
					 * @note Under Microsoft Windows the method has no body: the SIGCHLD signal does not
					 *       exist there, and the termination of a child process is announced by
					 *       the waiting on the process object from the system thread pool.
					 *       The wake-up of the event loop is common in both cases — the
					 *       `_wakeup` event, while the parsing of the terminated ones is done by the `reap` method
					 *
					 * @param signal number of the signal received by the system
					 * @param info   information object received by the system
					 * @param ctx    internal context being passed
					 *
					 * \~
					 */
					static void child(int32_t signal, siginfo_t * info, void * ctx) noexcept;
				/**
				 * Для операционной системы Microsoft Windows
				 */
				#elif defined(_WIN32) || defined(_WIN64) || defined(DOXYGEN)
					/**
					 * \~russian
					 * @brief Функция извещения о завершении дочернего процесса
					 *
					 * @details Вызывается системным пулом потоков, а не потоком петли
					 *          событий. Поэтому не делает ничего, кроме постановки номера
					 *          процесса в очередь и пробуждения петли: разбор ведёт метод
					 *          `reap` уже в своём потоке. Соображение то же, что и у
					 *          обработчика сигнала SIGCHLD на системах POSIX.
					 *
					 * @note Написание доводов повторяет WAITORTIMERCALLBACK, но своими
					 *       словами: `void *` вместо PVOID и `uint8_t` вместо BOOLEAN.
					 *       Сделано так, чтобы открытый заголовок не тянул за собой
					 *       заголовки Microsoft Windows
					 *
					 * @param ctx     идентификатор завершившегося процесса
					 * @param timeout признак срабатывания по истечении срока ожидания
					 *
					 * \~english
					 * @brief Function of the notification about the termination of a child process
					 *
					 * @details Called by the system thread pool rather than by the thread of the event
					 *          loop. Therefore it does nothing except placing the number of the
					 *          process into the queue and waking the loop up: the parsing is done by the
					 *          `reap` method already in its own thread. The consideration is the same as that of
					 *          the handler of the SIGCHLD signal on the POSIX systems.
					 *
					 * @note The spelling of the arguments repeats WAITORTIMERCALLBACK, but in its own
					 *       words: `void *` instead of PVOID and `uint8_t` instead of BOOLEAN.
					 *       This has been done so that the public header does not drag the
					 *       headers of Microsoft Windows along with it
					 *
					 * @param ctx     identifier of the terminated process
					 * @param timeout flag of the triggering by the expiration of the waiting term
					 *
					 * \~
					 */
					static void __stdcall child(void * ctx, uint8_t timeout) noexcept;
				#endif
			private:
				/**
				 * \~russian
				 * @brief Метод извещения работников о входе либо выбытии узла
				 *
				 * @note Извещение уходит всем работникам, КРОМЕ названного: себе о своём
				 *       входе работник вести счёт не обязан
				 *
				 * @param type вид служебного сообщения
				 * @param pid  узел, о котором идёт речь
				 *
				 * \~english
				 * @brief Method of the notification of the workers about the entering or the leaving of a node
				 *
				 * @note The notification goes to all the workers EXCEPT the named one: a worker is not obliged
				 *       to keep count of his own entering
				 *
				 * @param type kind of the service message
				 * @param pid  node in question
				 *
				 * \~
				 */
				void announce(const control_t type, const pid_t pid) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора служебного сообщения кластера
				 *
				 * @param eid  идентификатор служебного события
				 * @param data данные служебного сообщения
				 * @param size размер служебного сообщения
				 *
				 * \~english
				 * @brief Method of the parsing of a service message of the cluster
				 *
				 * @param eid  identifier of the service event
				 * @param data data of the service message
				 * @param size size of the service message
				 *
				 * \~
				 */
				void control(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод разрыва всех связей выбывающего узла
				 *
				 * @param pid выбывающий узел
				 *
				 * \~english
				 * @brief Method of the breaking of all the links of a leaving node
				 *
				 * @param pid leaving node
				 *
				 * \~
				 */
				void dissolve(const pid_t pid) noexcept;
				/**
				 * \~russian
				 * @brief Метод заведения прямой связи между двумя работниками
				 *
				 * @param initiator узел, заказавший связь
				 * @param peer      узел, с которым связь заказана
				 * @param data      тело заказа: имя канала у Microsoft Windows, пустое у систем POSIX
				 * @param size      размер тела заказа
				 *
				 * \~english
				 * @brief Method of the establishment of a direct link between two workers
				 *
				 * @param initiator node that has ordered the link
				 * @param peer      node the link with which has been ordered
				 * @param data      body of an order: a name of a channel at Microsoft Windows, empty at the POSIX systems
				 * @param size      a size of a body of an order
				 *
				 * \~
				 */
				void establish(const pid_t initiator, const pid_t peer, const uint8_t * data, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод обработки событий чтения по прямой связи с соседом
				 *
				 * @param eid  идентификатор события связи
				 * @param data данные сообщения
				 * @param size размер сообщения
				 *
				 * \~english
				 * @brief Method of processing the read events on a direct link with a neighbour
				 *
				 * @param eid  identifier of the link event
				 * @param data data of the message
				 * @param size size of the message
				 *
				 * \~
				 */
				void incoming(const event::id_t eid, const uint8_t * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки служебного сообщения кластера
				 *
				 * @param eid    идентификатор служебного события
				 * @param type   вид служебного сообщения
				 * @param pid    узел, которого сообщение касается
				 * @param buffer тело служебного сообщения
				 * @param size   размер тела служебного сообщения
				 * @return       количество отправленных байт
				 *
				 * \~english
				 * @brief Method of the sending of a service message of the cluster
				 *
				 * @param eid    identifier of the service event
				 * @param type   kind of the service message
				 * @param pid    node the message concerns
				 * @param buffer body of the service message
				 * @param size   size of the body of the service message
				 * @return       number of the sent bytes
				 *
				 * \~
				 */
				size_t dispatch(const event::id_t eid, const control_t type, const pid_t pid, const void * buffer = nullptr, const size_t size = 0) noexcept;
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
				 *
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
				 *
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
				 *
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
				 *
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
				 *
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
				 * @brief Метод проверки, завершился ли процесс сам
				 *
				 * @param status состояние завершения процесса
				 * @return       признак того, что процесс завершился сам
				 *
				 * \~english
				 * @brief Method of checking whether the process has terminated by itself
				 *
				 * @param status termination state of the process
				 * @return       flag of the process having terminated by itself
				 *
				 * \~
				 */
				static bool exited(const int32_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки, снят ли процесс с клавиатуры
				 *
				 * @details Ручной остановкой считается прерывание с клавиатуры: сигнал SIGINT
				 *          у POSIX и код завершения STATUS_CONTROL_C_EXIT у Microsoft Windows, каким
				 *          та помечает процесс, снятый по нажатию Ctrl+C или Ctrl+Break.
				 *
				 * @param status состояние завершения процесса
				 * @return       признак ручной остановки процесса
				 *
				 * \~english
				 * @brief Method of checking whether the process has been removed from the keyboard
				 *
				 * @details A manual stop is an interruption from the keyboard: the SIGINT signal
				 *          on POSIX and the STATUS_CONTROL_C_EXIT termination code on Microsoft Windows, with which
				 *          the latter marks a process removed by pressing Ctrl+C or Ctrl+Break.
				 *
				 * @param status termination state of the process
				 * @return       flag of the manual stop of the process
				 *
				 * \~
				 */
				static bool manual(const int32_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки, завершился ли процесс ненормально
				 *
				 * @details Вопрос этот переносим, в отличие от `signaled`: у POSIX
				 *          ненормальным считается снятие сигналом краха (SIGSEGV, SIGBUS,
				 *          SIGILL, SIGFPE, SIGABRT), у Microsoft Windows - код завершения с
				 *          признаком ошибки в старших разрядах, каким система помечает
				 *          необработанные исключения.
				 *
				 * @param status состояние завершения процесса
				 * @return       признак ненормального завершения процесса
				 *
				 * \~english
				 * @brief Method of checking whether the process has terminated abnormally
				 *
				 * @details This question is portable, unlike `signaled`: on POSIX
				 *          a removal by a crash signal (SIGSEGV, SIGBUS,
				 *          SIGILL, SIGFPE, SIGABRT) is considered abnormal, on Microsoft Windows — a termination code with
				 *          the error flag in the high bits, with which the system marks
				 *          the unhandled exceptions.
				 *
				 * @param status termination state of the process
				 * @return       flag of the abnormal termination of the process
				 *
				 * \~
				 */
				static bool crashed(const int32_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки, снят ли процесс сигналом
				 *
				 * @note У Microsoft Windows отвечает ложью всегда: сигналов там нет вовсе.
				 *       Переносимый вопрос «кончился ли процесс ненормально» задаётся
				 *       методом `crashed`, а не этим
				 *
				 * @param status состояние завершения процесса
				 * @return       признак того, что процесс снят сигналом
				 *
				 * \~english
				 * @brief Method of checking whether the process has been removed by a signal
				 *
				 * @note On Microsoft Windows it always answers with a lie: there are no signals there at all.
				 *       The portable question «has the process ended abnormally» is asked by
				 *       the `crashed` method rather than by this one
				 *
				 * @param status termination state of the process
				 * @return       flag of the process having been removed by a signal
				 *
				 * \~
				 */
				static bool signaled(const int32_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения кода возврата завершившегося процесса
				 *
				 * @param status состояние завершения процесса
				 * @return       код возврата процесса, либо EXIT_FAILURE при завершении ненормальном
				 *
				 * \~english
				 * @brief Method of getting the return code of the terminated process
				 *
				 * @param status termination state of the process
				 * @return       return code of the process, or EXIT_FAILURE on an abnormal termination
				 *
				 * \~
				 */
				static int32_t exitcode(const int32_t status) noexcept;
				/**
				 * \~russian
				 * @brief Метод получения номера сигнала, снявшего процесс
				 *
				 * @note У Microsoft Windows отвечает нулём всегда - см. пояснение к `signaled`
				 *
				 * @param status состояние завершения процесса
				 * @return       номер сигнала, либо 0 если процесс снят не сигналом
				 *
				 * \~english
				 * @brief Method of getting the number of the signal that has removed the process
				 *
				 * @note On Microsoft Windows it always answers with zero — see the explanation of `signaled`
				 *
				 * @param status termination state of the process
				 * @return       signal number, or 0 if the process has not been removed by a signal
				 *
				 * \~
				 */
				static int32_t termsig(const int32_t status) noexcept;
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
				 * @brief Метод определения роли процесса до запуска кластера
				 *
				 * @details Отвечает на вопрос «запущен ли этот процесс работником» ещё
				 *          до того, как кластер начал работу. Нужен он там, где порядок
				 *          подготовки у мастера и у работника расходится: мастер обязан
				 *          подготовиться ПРЕЖДЕ порождения работников, а работник -
				 *          ПОСЛЕ того, как связался с мастером.
				 *
				 * @note Роль здесь лишь спрашивается и не перенимается: перенимает её
				 *       запуск кластера, и метку роли снимает тоже он
				 *
				 * @return признак того, что процесс запущен работником кластера
				 *
				 * \~english
				 * @brief Method of the determination of the role of the process before the start of the cluster
				 *
				 * @details It answers a question `is this process started as a worker`
				 *         yet before a cluster began the work. It is needed there, where an order
				 *         of a preparation at a master and at a worker diverges: a master is obliged
				 *         to prepare BEFORE a spawning of the workers,
				 *         and a worker - AFTER it connected with a master.
				 *
				 * @note A role is only asked here and is not taken over: a starting of a cluster takes it over,
				 *      and a removal of a role mark is also performed by it
				 *
				 * @return flag that the process is started as a worker of the cluster
				 *
				 * \~
				 */
				bool worker() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения предельного размера пересылаемого сообщения
				 *
				 * @return предельный размер тела пересылаемого сообщения в байтах
				 *
				 * \~english
				 * @brief Method of getting the limiting size of a relayed message
				 *
				 * @return limiting size of the body of a relayed message in bytes
				 *
				 * \~
				 */
				size_t maximumRelay() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки всех выделенных ресурсов
				 *
				 * @param shutdown тип завершения работы кластера
				 *
				 * \~english
				 * @brief Method of clearing all the allocated resources
				 *
				 * @param shutdown type of the termination of the work of the cluster
				 *
				 * \~
				 */
				void clear(const shutdown_t shutdown) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения события обмена с процессом кластера
				 *
				 * @details Событие это и есть путь к названному процессу: им называется
				 *          получатель там, где одного номера процесса мало, - прежде
				 *          всего при передаче событий между процессами (`snapshot`).
				 *          Родительский процесс спрашивает событие обмена с дочерним по
				 *          его номеру, дочерний - своё, назвав собственный номер.
				 *
				 * @note Событие живёт ровно столько же, сколько и сам процесс кластера:
				 *       у завершившегося процесса его нет, и метод отвечает нулём
				 *
				 * @param pid идентификатор процесса кластера
				 * @return    идентификатор события обмена с процессом кластера
				 *
				 * \~english
				 * @brief Method of the getting of the exchange event with a process of the cluster
				 *
				 * @details This event is exactly a path to a named process: it is called a recipient there,
				 *         where one process number is not enough - above all at a passing of the events
				 *         between the processes (`snapshot`).
				 *         A parent process asks an exchange event with a child one by
				 *         its number, a child one - its own, having named a own number.
				 *
				 * @note An event lives exactly as long as a process of a cluster itself: at
				 *      a finished process it has none, and a method answers with a zero
				 *
				 * @param pid identifier of the process of the cluster
				 * @return    identifier of the exchange event with the process of the cluster
				 *
				 * \~
				 */
				event::id_t channel(const pid_t pid) const noexcept;
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
				 *
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
				 *
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
				 *
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
				 *
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
				 *
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
				 *
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
				 *
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
				 *
				 * @param count maximum number of the processes
				 *
				 * \~
				 */
				void count(const uint16_t count) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения списка известных работнику узлов кластера
				 *
				 * @details Список этот наполняется извещениями мастера о входе и выбытии
				 *          узлов: собственного способа узнать соседей у работника нет.
				 *
				 * @return список известных узлов кластера
				 *
				 * \~english
				 * @brief Method of getting the list of the nodes of the cluster known to the worker
				 *
				 * @details This list is filled by the notifications of a master about an entering and a leaving
				 *         of the nodes: an own way to learn the neighbours a worker has none.
				 *
				 * @return list of the known nodes of the cluster
				 *
				 * \~
				 */
				unordered_set <pid_t> nodes() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения списка дочерних процессов
				 *
				 * @return список дочерних процессов
				 *
				 * \~english
				 * @brief Method of getting the list of the child processes
				 *
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
				 * \~english
				 * @brief Method of setting the callback functions
				 *
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
				 *
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
				 *
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
				 *
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
				 * @brief Метод заказа прямой связи с соседним работником
				 *
				 * @details Заказ уходит мастеру; тот заводит пару и раздаёт её концы обоим
				 *          работникам, после чего в обмене не участвует вовсе. Готовность
				 *          связи приходит откликом `"linked"`, отказ - откликом
				 *          `"unlinked"` с причиной.
				 *
				 * @note Связь между парой узлов одна: повторный заказ отвечает отказом
				 *       `EXISTS`, а не заводит вторую
				 *
				 * @param pid узел, с которым заказывается связь
				 * @return    признак того, что заказ отправлен мастеру
				 *
				 * \~english
				 * @brief Method of ordering a direct link with a neighbouring worker
				 *
				 * @details An order goes to a master; that one starts a pair and distributes its ends to both
				 *         workers, after which in an exchange it does not participate at all. A readiness
				 *         of a connection comes by a callback `"linked"`, a refusal - by a callback
				 *         `"unlinked"` with a reason.
				 *
				 * @note A connection between a pair of nodes is one: a repeated order answers by a refusal
				 *      `EXISTS`, and does not start a second one
				 *
				 * @param pid node the link with which is ordered
				 * @return    flag of the order being sent to the master
				 *
				 * \~
				 */
				bool link(const pid_t pid) noexcept;
				/**
				 * \~russian
				 * @brief Метод разрыва прямой связи с соседним работником
				 *
				 * @param pid узел, связь с которым разрывается
				 * @return    признак того, что разрыв отправлен мастеру
				 *
				 * \~english
				 * @brief Method of breaking a direct link with a neighbouring worker
				 *
				 * @param pid node the link with which is broken
				 * @return    flag of the breaking being sent to the master
				 *
				 * \~
				 */
				bool unlink(const pid_t pid) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод ухода работника из кластера по своей воле
				 *
				 * @details Работник извещает мастера и выходит названным кодом, дождавшись
				 *          отдачи извещения каналу. Возрождению он не подлежит.
				 *
				 * @param code код завершения работы
				 *
				 * \~english
				 * @brief Method of a worker leaving the cluster of its own will
				 *
				 * @details A worker notifies a master and leaves by a named code, having waited for a giving
				 *         of a notification to a channel. A revival is not due to it.
				 *
				 * @param code termination code of the work
				 *
				 * \~
				 */
				void leave(const int32_t code = 0) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод приказа работнику завершить работу
				 *
				 * @details Приказ этот - не сигнал: работник доводит своё дело до конца,
				 *          извещает мастера и выходит названным кодом сам. Убийство извне
				 *          осталось за `erase(pid, FORCEFUL)`.
				 *
				 * @note Возрождению ушедший по приказу работник не подлежит, и счётчик
				 *       быстрых падений его уход не задевает
				 *
				 * @param pid  узел, которому велено завершить работу
				 * @param code код завершения работы
				 * @return     признак того, что приказ отправлен работнику
				 *
				 * \~english
				 * @brief Method of ordering a worker to terminate its work
				 *
				 * @details This order is not a signal: a worker brings its own deed to an end,
				 *         notifies a master and leaves by a named code itself. A killing from outside
				 *         stayed behind `erase(pid, FORCEFUL)`.
				 *
				 * @note A revival is not due to a worker that left by an order, and a counter
				 *      of the fast falls is not touched by its leaving
				 *
				 * @param pid  node ordered to terminate its work
				 * @param code termination code of the work
				 * @return     flag of the order being sent to the worker
				 *
				 * \~
				 */
				bool shutdown(const pid_t pid, const int32_t code = SHUTDOWN_CODE) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод пересылки сообщения соседу через мастера
				 *
				 * @details Связи для пересылки не требуется, и заводить её ради одного
				 *          короткого сообщения дороже самого сообщения. Платой идут две
				 *          передачи вместо одной и предел размера - `maximumRelay`.
				 *
				 * @param pid    узел, которому предназначено сообщение
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 * \~english
				 * @brief Method of relaying a message to a neighbour through the master
				 *
				 * @details A connection for a forwarding is not required, and to start it for the sake of one
				 *         short message is dearer than the message itself. A price is two
				 *         transfers instead of one and a limit of a size - `maximumRelay`.
				 *
				 * @param pid    node the message is meant for
				 * @param buffer binary buffer for sending the message
				 * @param size   size of the binary buffer for sending the message
				 * @return       number of the bytes of the sent message
				 *
				 * \~
				 */
				size_t relay(const pid_t pid, const void * buffer, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод отправки сообщения соседу по прямой связи
				 *
				 * @param pid    узел, которому предназначено сообщение
				 * @param buffer бинарный буфер для отправки сообщения
				 * @param size   размер бинарного буфера для отправки сообщения
				 * @return       количество байт отправленного сообщения
				 *
				 * \~english
				 * @brief Method of sending a message to a neighbour over a direct link
				 *
				 * @param pid    node the message is meant for
				 * @param buffer binary buffer for sending the message
				 * @param size   size of the binary buffer for sending the message
				 * @return       number of the bytes of the sent message
				 *
				 * \~
				 */
				size_t transmit(const pid_t pid, const void * buffer, const size_t size) noexcept;
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
				 *
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
				 *
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
				 * \~english
				 * @brief Copy constructor (prohibited)
				 *
				 * \~
				 */
				explicit Cluster(const Cluster &) = delete;
				/**
				 * \~russian
				 * @brief Оператор копирования (запрещаем)
				 *
				 * @return текущее значение объекта
				 *
				 * \~english
				 * @brief Copy assignment operator (prohibited)
				 *
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
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Cluster() noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
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
