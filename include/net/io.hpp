/**
 * @file: io.hpp
 * @date: 2025-11-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл асинхронного движка ввода-вывода — класс engine::IO, реализующий цикл событий,
 *        работу с сокетами всех поддерживаемых семейств и протоколов, таймеры, наблюдение за файлами и каталогами,
 *        списки контроля доступа и поддержку SCTP
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_IO_ENGINE__
#define __AWH_IO_ENGINE__

/**
 * Подключаем заголовочный файл проекта
 */
#include "engine.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён движков ввода-вывода
	 *
	 */
	namespace engine {
		/**
		 * Используем стандартное пространство имён
		 */
		using namespace std;

		/**
		 * Для операционных систем с поддержкой SCTP: Linux, FreeBSD, Solaris и illumos
		 */
		#if __linux__ || __FreeBSD__ || __sun
			/**
			 * @brief Класс управления протоколом передачи с управлением потоком
			 *
			 * @details Спутник движка, обслуживающий то, что есть только у SCTP и не
			 *          укладывается в общую поверхность `engine::io_t`. Само событие
			 *          SCTP-сокета заводится движком обычным порядком - с протоколом
			 *          `event::protocol_t::SCTP`, - а этот класс правит уже
			 *          заведённое событие по его идентификатору.
			 *
			 *          Разделение сделано намеренно. SCTP - протокол с ассоциациями,
			 *          многопоточностью внутри одного соединения, собственными
			 *          таймерами и аутентификацией чанков; у TCP и UDP аналогов этому
			 *          нет. Свести всё в один интерфейс значило бы обвесить движок
			 *          методами, недействительными для девяти протоколов из десяти,
			 *          поэтому особенности SCTP вынесены отдельно, а общее -
			 *          подключение, приём, отправка, освобождение - остаётся за
			 *          движком.
			 *
			 *          Класс покрывает четыре области:
			 *
			 *          - **метаданные сообщения** - `messageInfo()`: поток, номер
			 *            последовательности, признак упорядоченности; у SCTP сообщение
			 *            несёт их само, в отличие от потока октетов у TCP;
			 *          - **параметры ассоциации** - `initMessages()` и `status()`:
			 *            количество потоков в каждую сторону, число попыток
			 *            установления, и текущее состояние ассоциации;
			 *          - **подписка на события протокола** - `eventsSubscribe()`:
			 *            какие уведомления протокола доставлять прикладному коду;
			 *          - **аутентификация по RFC 4895** - `authenticateKey()`,
			 *            `authenticateChunks()`, `authenticateSupportAlgorithms()`.
			 *
			 * @note    Класс объявлен для Linux, FreeBSD, Solaris и illumos, но
			 *          **отлажен и работает только под FreeBSD**. У macOS и OpenBSD
			 *          SCTP в ядре нет вовсе, а NetBSD держит заголовок без поддержки
			 *          в ядре; класс там не существует - обращения к нему не соберутся,
			 *          а не откажут во время работы. Прикладной код, рассчитанный на
			 *          переносимость, обязан заворачивать обращения в ту же проверку
			 *          `#if __linux__ || __FreeBSD__ || __sun`.
			 *
			 * @note    Рабочие примеры лежат в [`sample/net/sctp/`](../../sample/net/sctp):
			 *          потоковый и последовательно-пакетный режимы, поверх TLS и
			 *          DTLS, с аутентификацией чанков - по паре клиент и сервер на
			 *          каждый случай. Смотреть следует их, а не пример ниже: он
			 *          показывает только порядок вызовов.
			 *
			 * @note    Объект создаётся отдельно от движка и своего состояния о
			 *          событиях не держит: он лишь переводит вызовы в параметры
			 *          сокета по идентификатору события. Поэтому один объект
			 *          обслуживает сколько угодно событий, а порядок его создания
			 *          относительно движка не важен.
			 *
			 * @note    Таймауты `setTimeout()` - это таймеры **самого протокола**
			 *          (`INIT`, `DATA`, `SACK`, `SHUTDOWN`, `HEARTBEAT`, `COOKIE`,
			 *          `SHUTDOWNACK`), а не таймауты события движка. Путать их с
			 *          `engine::io_t::setTimeout()` нельзя: те отсчитывает движок и
			 *          сообщает о них функциями обратного вызова, эти отсчитывает ядро
			 *          и действует по ним само
			 *
			 * @par Пример: SCTP-клиент с несколькими потоками в ассоциации
			 * @code{.cpp}
			 * #if __linux__ || __FreeBSD__ || __sun
			 *     awh::engine::io_t io(&fmk, &log);
			 *     awh::engine::sctp_t sctp(&fmk, &log);
			 *     // Заводим событие SCTP-сокета обычным порядком движка
			 *     const awh::event::id_t client = io.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::SCTP);
			 *     io.setTargetPort(client, 9899);
			 *     io.setTarget(client, "127.0.0.1");
			 *     // Задаём параметры ассоциации: по четыре потока в каждую сторону
			 *     awh::net::sctp::initmsg_t initmsg;
			 *     initmsg.outputStreams = 4;
			 *     initmsg.inputStreams = 4;
			 *     sctp.initMessages(client, initmsg);
			 *     // Продлеваем таймер контрольных сообщений протокола до тридцати секунд
			 *     sctp.setTimeout(client, awh::net::sctp::timeout_t::HEARTBEAT, 30000);
			 *     // Подписываемся на метаданные принятых сообщений
			 *     sctp.on(client, static_cast <awh::engine::callback::sctp::minfo_t> ([](const awh::event::id_t id, const awh::net::sctp::minfo_t & info) noexcept -> void {
			 *         // Здесь известно, каким потоком ассоциации пришло сообщение
			 *     }));
			 *     io.initialize();
			 *     io.commit(client);
			 *     io.connect(client);
			 *     io.launch(client);
			 *     while(io.poll(100));
			 *     io.deinitialize();
			 * #endif
			 * @endcode
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Stream_Control_Transmission_Protocol {
				private:
					// Объект работы с сетью
					eth_t _eth;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				public:
					/**
					 * @brief Метод получения информационных метаданных SCTP сообщения
					 *
					 * @param id идентификатор события
					 * @return   информационные метаданные SCTP сообщения
					 *
					 */
					net::sctp::minfo_t messageInfo(const event::id_t id) const noexcept;
					/**
					 * @brief Метод установки информационных метаданных SCTP сообщения
					 *
					 * @param id   идентификатор события
					 * @param info информационные метаданные SCTP сообщения
					 *
					 */
					void messageInfo(const event::id_t id, const net::sctp::minfo_t & info) noexcept;
				public:
					/**
					 * @brief Метод получения параметров статуса инициализации SCTP
					 *
					 * @param id идентификатор события
					 * @return   параметры статуса инициализации SCTP
					 *
					 */
					net::sctp::status_t status(const event::id_t id) const noexcept;
					/**
					 * @brief Метод установки параметров инициализации SCTP
					 *
					 * @param id      идентификатор события
					 * @param initmsg параметры инициализации SCTP события
					 *
					 */
					void initMessages(const event::id_t id, const net::sctp::initmsg_t & initmsg) noexcept;
				public:
					/**
					 * @brief Метод получения опций подписки SCTP событий
					 *
					 * @param id идентификатор события
					 * @return   список событий SCTP на которые выполнена подписка
					 *
					 */
					const net::sctp::event_types_t & eventsSubscribed(const event::id_t id) const noexcept;
					/**
					 * @brief Метод установки опций подписки SCTP событий
					 *
					 * @param id     идентификатор события
					 * @param events список событий SCTP для подписки
					 *
					 */
					void eventsSubscribe(const event::id_t id, const net::sctp::event_types_t & events) noexcept;
				public:
					/**
					 * @brief Метод получения таймаута SCTP события
					 *
					 * @param id   идентификатор события
					 * @param type тип таймаута
					 * @return     значение таймаута в миллисекундах
					 *
					 */
					uint32_t getTimeout(const event::id_t id, const net::sctp::timeout_t type) const noexcept;
					/**
					 * @brief Метод установки таймаута SCTP события
					 *
					 * @param id      идентификатор события
					 * @param type    тип таймаута
					 * @param timeout значение таймаута в миллисекундах
					 * @return        результат работы функции
					 *
					 */
					bool setTimeout(const event::id_t id, const net::sctp::timeout_t type, const uint32_t timeout) noexcept;
				public:
					/**
					 * @brief Метод установки ключа аутентификации SCTP сокета
					 *
					 * @param id  идентификатор события
					 * @param num номер ключа аутентификации
					 * @param key ключ аутентификации
					 * @return    результат работы функции
					 *
					 */
					bool authenticateKey(const event::id_t id, const uint16_t num, string_view key) noexcept;
					/**
					 * @brief Метод активации/деактивации ключа аутентификации SCTP сокета
					 *
					 * @param id   идентификатор события
					 * @param mode режим установки действия события
					 * @param num  номер ключа аутентификации
					 * @return     результат работы функции
					 *
					 */
					bool authenticateKey(const event::id_t id, const event::mode_t mode, const uint16_t num) noexcept;
				public:
					/**
					 * @brief Метод установки чанков аутентификации SCTP сокета
					 *
					 * @param id     идентификатор события
					 * @param chunks список чанков подлежащих аутентификации
					 * @return       результат работы функции
					 *
					 */
					bool authenticateChunks(const event::id_t id, const vector <net::sctp::auth_chunk_t> & chunks) noexcept;
					/**
					 * @brief Метод извлечения чанков аутентификации SCTP сокета
					 *
					 * @param id     идентификатор события
					 * @param origin источник события
					 * @param chunks список чанков подлежащих аутентификации
					 * @return       результат работы функции
					 *
					 */
					bool authenticateChunks(const event::id_t id, const event::origin_t origin, vector <net::sctp::auth_chunk_t> & chunks) const noexcept;
				public:
					/**
					 * @brief Метод установки поддерживаемых алгоритмов аутентификации SCTP сокета
					 *
					 * @param id    идентификатор события
					 * @param types список поддерживаемых алгоритмов аутентификации
					 * @return      результат работы функции
					 *
					 */
					bool authenticateSupportAlgorithms(const event::id_t id, const vector <net::sctp::auth_type_t> & types) noexcept;
				public:
					/**
					 * @brief Метод установки функции обратного вызова для получения метаданных SCTP-сообщения
					 *
					 * @param id идентификатор события
					 * @param cb функция обратного вызова
					 *
					 */
					void on(const event::id_t id, engine::callback::sctp::minfo_t cb) noexcept;
					/**
					 * @brief Метод установки функции обратного вызова для получения SCTP-событий
					 *
					 * @param id идентификатор события
					 * @param cb функция обратного вызова
					 *
					 */
					void on(const event::id_t id, engine::callback::sctp::events_t cb) noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект работы с логами
					 *
					 */
					explicit Stream_Control_Transmission_Protocol(const fmk_t * fmk, const log_t * log) noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Stream_Control_Transmission_Protocol() noexcept;
			} sctp_t;
		#endif
		/**
		 * @brief Тип асинхронного движка ввода-вывода
		 *
		 * @details Движок обслуживает сокеты, файлы, каталоги, межпроцессное
		 *          взаимодействие, туннели и таймеры единым циклом событий. Поверх
		 *          механизма опроса операционной системы - kqueue у BSD и macOS,
		 *          epoll и io_uring у Linux, event ports у Solaris, IOCP у Windows -
		 *          лежит одна и та же модель, поэтому прикладной код от выбора
		 *          механизма не зависит.
		 *
		 *          **Событие вместо дескриптора.** Наружу движок отдаёт не
		 *          дескриптор, а числовой идентификатор события `event::id_t`.
		 *          Разница не косметическая: идентификатор нельзя разыменовать, а
		 *          освобождённое событие по нему просто не находится. Прикладной код
		 *          не держит указателей на внутренние объекты движка и потому не
		 *          может обратиться к уже уничтоженному соединению - самая частая
		 *          ошибка при работе с сырыми дескрипторами здесь невозможна.
		 *          Разрешение идентификатора обходится в три с половиной наносекунды
		 *          и того стоит.
		 *
		 *          **Порядок работы с событием.** Событие проходит четыре шага, и
		 *          порядок их обязателен:
		 *
		 *          1. `event()` - событие создаётся, под него заводится дескриптор,
		 *             событие переходит в состояние `INITIAL`;
		 *          2. настройка - адреса, порты, опции, таймауты, функции обратного
		 *             вызова; всё это только запоминается в событии;
		 *          3. `commit()` - настройки закрепляются, дальше менять их нельзя;
		 *          4. `launch()` - событие включается в опрос и начинает работать.
		 *
		 *          У подключающегося клиента между третьим и четвёртым шагом стоит
		 *          `connect()`, а у сервера - `listen()`. Освобождается событие
		 *          `destroy()` из любого состояния.
		 *
		 *          **Цикл событий ведёт вызывающий.** Движок своего потока не
		 *          создаёт: `poll()` выполняет один оборот и возвращает управление.
		 *          Крутить цикл - дело прикладного кода, и это позволяет вести его в
		 *          своём потоке, встраивать в чужой цикл событий и останавливать
		 *          когда угодно.
		 *
		 * @note    Функции обратного вызова вызываются **внутри** `poll()`. Пока
		 *          выполняется обратный вызов, оборот цикла не завершён, поэтому
		 *          долгая работа в нём задерживает все прочие события. Тяжёлое надо
		 *          уносить в свой поток.
		 *
		 * @note    Освобождать событие изнутри его же обратного вызова допустимо:
		 *          `destroy()` помечает событие и откладывает освобождение на два
		 *          оборота цикла. Отсрочка нужна не для удобства, а по необходимости
		 *          - записи подписки уходят в ядро вместе с ожиданием следующего
		 *          оборота, и закрой движок дескриптор раньше, его номер
		 *          операционная система успела бы выдать другому объекту.
		 *
		 * @note    Движок рассчитан на один поток опроса. Обращаться к событиям из
		 *          других потоков нельзя; для передачи работы в поток цикла заведено
		 *          пользовательское событие `event::node_t::NOTIFY`.
		 *
		 * @par Пример: клиент
		 * @code{.cpp}
		 * awh::engine::io_t io(&fmk, &log);
		 * // Заводим событие клиента и настраиваем его
		 * const awh::event::id_t client = io.event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		 * io.setTargetPort(client, 80);
		 * io.setTarget(client, "93.184.216.34");
		 * io.setTimeout(client, awh::event::action_t::CONNECT, 5000);
		 * // Подписываемся на завершение подключения и на приём данных
		 * io.on(client, static_cast <awh::engine::callback::connect_t> ([](const awh::event::id_t id, const bool ok) noexcept -> void {
		 *     // Здесь известно, состоялось подключение или нет
		 * }));
		 * io.on(client, static_cast <awh::engine::callback::read_t> ([](const awh::event::id_t id, const uint8_t * buffer, const size_t size) noexcept -> void {
		 *     // Принятые данные лежат в буфере и действительны только до выхода отсюда
		 * }));
		 * // Инициализируем движок, закрепляем настройки и запускаем событие
		 * io.initialize();
		 * io.commit(client);
		 * io.connect(client);
		 * io.launch(client);
		 * // Крутим цикл событий, пока он выполняется без ошибок
		 * while(io.poll(100));
		 * io.deinitialize();
		 * @endcode
		 *
		 * @par Пример: сервер
		 * @code{.cpp}
		 * awh::engine::io_t io(&fmk, &log);
		 * // Заводим событие сервера и настраиваем его
		 * const awh::event::id_t server = io.event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		 * io.setSourcePort(server, 8080);
		 * io.setAddress(server, awh::event::address_t::IPV4, "0.0.0.0");
		 * // Принятое подключение приходит готовым событием, отдельной настройки не требует
		 * io.on(server, static_cast <awh::engine::callback::accept_t> ([&io](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
		 *     // Подписываемся на приём данных уже принятого подключения
		 *     io.on(cid, static_cast <awh::engine::callback::read_t> ([&io](const awh::event::id_t id, const uint8_t * buffer, const size_t size) noexcept -> void {
		 *         // Возвращаем принятое отправителю
		 *         io.send(id, buffer, size);
		 *     }));
		 * }));
		 * io.initialize();
		 * io.commit(server);
		 * io.listen(server, 1024);
		 * io.launch(server);
		 * while(io.poll(100));
		 * io.deinitialize();
		 * @endcode
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ IO : public engine_t {
			public:
				/**
				 * @brief Структура управления списками контроля доступа
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ Control_List {
					private:
						// Объект работы с сетевыми адресами
						net_addr_t _addr;
					private:
						// Тип списка контроля доступа
						event::control_list_t _type;
					private:
						// Объект фреймворка
						const fmk_t * _fmk;
						// Объект работы с логами
						const log_t * _log;
					public:
						/**
						 * @brief Метод очистки контрольного списка события
						 *
						 * @param id идентификатор события
						 * @return   результат выполнения очистки
						 *
						 */
						bool clear(const event::id_t id) noexcept;
						/**
						 * @brief Метод добавления адреса в контрольный список события
						 *
						 * @param id    идентификатор события
						 * @param value значение адреса события
						 * @return      результат выполнения установки
						 *
						 */
						bool add(const event::id_t id, string_view value) noexcept;
						/**
						 * @brief Метод удаления адреса из контрольного списка события
						 *
						 * @param id    идентификатор события
						 * @param value адрес для удаления из контрольного списка
						 * @return      результат выполнения удаления
						 *
						 */
						bool remove(const event::id_t id, string_view value) noexcept;
						/**
						 * @brief Метод получения контрольного списка события
						 *
						 * @param id идентификатор события
						 * @return   контрольный список события
						 *
						 */
						const unordered_map <string, event::address_t> & get(const event::id_t id) const noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 * @param type тип контрольного списка
						 *
						 */
						explicit Control_List(const event::control_list_t type, const fmk_t * fmk, const log_t * log) noexcept;
						/**
						 * @brief Деструктор
						 *
						 */
						virtual ~Control_List() noexcept;
				} control_list_t;
			public:
				// Объект управления белым списком
				control_list_t whitelist;
				// Объект управления чёрным списком
				control_list_t blacklist;
			public:
				/**
				 * @brief Метод фиксации настроек события
				 *
				 * @details Закрепляет всё, что было выставлено событию после `event()`:
				 *          адреса, порты, опции сокета, размеры буферов, сроки. До этого
				 *          вызова настройки лежат в самом событии и до сокета не доходят, а
				 *          после - применены, и событие переходит из состояния «заведено» в
				 *          «инициализировано»
				 *
				 * @details Состояния события образуют последовательность, и каждый шаг
				 *          требует предыдущего:
				 *
				 *          | Вызов | Требует состояния | Оставляет состояние |
				 *          |---|---|---|
				 *          | `event()` | - | `NONE` |
				 *          | `commit()` | `NONE` | `INITIAL` |
				 *          | `connect()` | `INITIAL` | `SUCCESS` |
				 *          | `listen()` | `INITIAL` | `SUCCESS` |
				 *          | `launch()` | `INITIAL` или `SUCCESS` | `LAUNCHED` / `LISTENING` |
				 *
				 *          Отсюда следует, что `connect()` и `listen()` ставятся **между**
				 *          фиксацией и запуском, а не до фиксации и не после запуска.
				 *
				 * @note Повторная фиксация уже инициализированного события ничего не делает
				 *       и возвращает отрицательный результат: состояние `NONE` бывает у
				 *       события лишь однажды. Настройки, изменённые после фиксации,
				 *       применяются своими методами сразу, фиксации не требуя
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения фиксации
				 *
				 */
				bool commit(const event::id_t id) noexcept;
				/**
				 * @brief Метод перестройки события: пересоздание нижележащего дескриптора с сохранением самого события
				 *
				 * @note Приложение работает с идентификатором события, а не с дескриптором,
				 *       поэтому дескриптор пересоздаётся, а событие (его идентификатор,
				 *       коллбэки, адрес/порт, опции и таймеры) сохраняется - подмена
				 *       дескриптора приложению незаметна. Всё состояние, живущее на
				 *       дескрипторе (регистрации (kqueue, epoll, ...), размеры буферов, DSCP/ECN/MTU,
				 *       интерфейс), снимается до закрытия и переприменяется на новый
				 *       дескриптор, а пройденные стадии подъёма (commit/listen/launch)
				 *       переигрываются по исходному статусу события. Поддерживается для
				 *       событий типа SERVER
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения перестройки
				 *
				 */
				bool rebuild(const event::id_t id) noexcept;
			public:
				/**
				 * @brief Метод получения сетевого интерфейса события
				 *
				 * @param id идентификатор события
				 * @return   сетевой интерфейс события
				 *
				 */
				string getIface(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки сетевого интерфейса события
				 *
				 * @param id   идентификатор события
				 * @param name имя сетевого интерфейса для установки
				 * @return     результат выполнения установки
				 *
				 */
				bool setIface(const event::id_t id, string_view name) noexcept;
			public:
				/**
				 * @brief Метод получения локального порта события
				 *
				 * @param id идентификатор события
				 * @return   локальный порт события
				 *
				 */
				uint16_t getSourcePort(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки локального порта события
				 *
				 * @param id   идентификатор события
				 * @param port локальный порт события
				 * @return     результат выполнения установки
				 *
				 */
				bool setSourcePort(const event::id_t id, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод получения порта назначения события
				 *
				 * @param id идентификатор события
				 * @return   порт назначения события
				 *
				 */
				uint16_t getTargetPort(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки порта назначения события
				 *
				 * @param id   идентификатор события
				 * @param port порт назначения события
				 * @return     результат выполнения установки
				 *
				 */
				bool setTargetPort(const event::id_t id, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param id идентификатор события
				 * @return   адрес хоста целевой машины
				 *
				 */
				string getTarget(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param id     идентификатор события
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 */
				bool setTarget(const event::id_t id, string_view target) noexcept;
			public:
				/**
				 * @brief Метод получения адреса хоста целевой машины
				 *
				 * @param id     идентификатор события
				 * @param target объект для извлечения адреса хоста целевой машины
				 * @return       результат выполнения извлечения адреса хоста целевой машины
				 *
				 */
				bool getTarget(const event::id_t id, unique_ptr <net::addr_t> & target) const noexcept;
				/**
				 * @brief Метод установки адреса хоста целевой машины
				 *
				 * @param id     идентификатор события
				 * @param target адрес хоста целевой машины
				 * @return       результат выполнения установки
				 *
				 */
				bool setTarget(const event::id_t id, const net::addr_t * target) noexcept;
			public:
				/**
				 * @brief Метод получения адреса события
				 *
				 * @param id      идентификатор события
				 * @param address тип адреса события
				 * @return        значение адреса события
				 *
				 */
				string getAddress(const event::id_t id, const event::address_t address) const noexcept;
				/**
				 * @brief Метод установки адреса события
				 *
				 * @param id      идентификатор события
				 * @param address тип адреса события
				 * @param value   значение адреса события
				 * @return        результат выполнения установки
				 *
				 */
				bool setAddress(const event::id_t id, const event::address_t address, string_view value) noexcept;
			public:
				/**
				 * @brief Метод получения адреса события
				 *
				 * @param id      идентификатор события
				 * @param address тип адреса события
				 * @param value   объект для извлечения адреса события
				 * @return        результат выполнения извлечения адреса события
				 *
				 */
				bool getAddress(const event::id_t id, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept;
				/**
				 * @brief Метод установки адреса события
				 *
				 * @param id      идентификатор события
				 * @param address тип адреса события
				 * @param value   значение адреса события
				 * @return        результат выполнения установки
				 *
				 */
				bool setAddress(const event::id_t id, const event::address_t address, const net::addr_t * value) noexcept;
			public:
				/**
				 * @brief Метод получения MTU сетевого интерфейса
				 *
				 * @param id идентификатор события
				 * @return   MTU сетевого интерфейса
				 *
				 */
				uint16_t getMaximumTransmissionUnit(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки MTU сетевого интерфейса
				 *
				 * @param id  идентификатор события
				 * @param mtu размер MTU интерфейса
				 * @return    результат установки MTU сетевого интерфейса
				 *
				 */
				bool setMaximumTransmissionUnit(const event::id_t id, const uint32_t mtu) const noexcept;
			public:
				/**
				 * @brief Метод получения признака выдачи системой поля Explicit Congestion Notification (ECN) принятых пакетов
				 *
				 * @details Отметку перегрузки пути ставит маршрутизатор в заголовок пакета,
				 *          а выдаёт её принимающему уже ядро - служебным сообщением при
				 *          приёме дейтаграммы. Выдают её не все системы: NetBSD и OpenBSD
				 *          по IPv4 не выдают вовсе, и опции запроса такой выдачи у них не
				 *          заведено. По IPv6 выдают обе
				 *
				 *          Вызывающему это знать необходимо. Обмен, помечающий свои пакеты
				 *          поддержкой отметок и не получающий отметок обратно, обязан по
				 *          договору признать проверку несостоявшейся и отметки отключить -
				 *          то есть проделать лишний круг там, где исход известен заранее
				 *
				 * @note Признак решается наличием средства запроса выдачи, а не перечнем
				 *       систем поимённо: перечень устареет с первым же выпуском, который
				 *       средство добавит
				 *
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       признак выдачи системой отметок перегрузки пути
				 *
				 */
				bool availableExplicitCongestionNotification(const event::family_t family) const noexcept;
			public:
				/**
				 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       значение DSCP
				 *
				 */
				event::dscp_t getDifferentiatedServicesCodePoint(const event::id_t id, const event::family_t family) const noexcept;
				/**
				 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param dscp   значение DSCP
				 * @return       результат работы функции
				 *
				 */
				bool setDifferentiatedServicesCodePoint(const event::id_t id, const event::family_t family, const event::dscp_t dscp) const noexcept;
			public:
				/**
				 * @brief Метод получения значения поля Explicit Congestion Notification (ECN) в заголовке IP-пакета
				 *
				 * @note Выдаёт значение, устанавливаемое на исходящих пакетах. Признак
				 *       перегрузки принятых пакетов приходит отдельно для каждой
				 *       датаграммы и извлекается методом getTrafficInfo
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       значение ECN
				 *
				 */
				event::ecn_t getExplicitCongestionNotification(const event::id_t id, const event::family_t family) const noexcept;
				/**
				 * @brief Метод установки значения поля Explicit Congestion Notification (ECN) в заголовке IP-пакета
				 *
				 * @note Класс обслуживания (DSCP) сохраняется: оба поля занимают один
				 *       октет заголовка, поэтому установка затрагивает только младшие
				 *       два бита
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param ecn    значение ECN
				 * @return       результат работы функции
				 *
				 */
				bool setExplicitCongestionNotification(const event::id_t id, const event::family_t family, const event::ecn_t ecn) const noexcept;
			public:
				/**
				 * @brief Метод получения обнаружения максимального размера пакета (MTU)
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @return       режим обнаружения максимального размера пакета (MTU)
				 *
				 */
				event::mtu_discover_t getMaximumTransmissionUnitDiscover(const event::id_t id, const event::family_t family) const noexcept;
				/**
				 * @brief Метод установки обнаружения максимального размера пакета (MTU)
				 *
				 * @param id     идентификатор события
				 * @param family семейство протоколов (IPv4 или IPv6)
				 * @param mode   режим обнаружения максимального размера пакета (MTU)
				 * @return       результат работы функции
				 *
				 */
				bool setMaximumTransmissionUnitDiscover(const event::id_t id, const event::family_t family, const event::mtu_discover_t mode) const noexcept;
			public:
				/**
				 * @brief Метод активации/деактивации мультикаст группы события
				 *
				 * @param id     идентификатор события
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 */
				bool membership(const event::id_t id, const event::mode_t mode, string_view group, string_view source, const uint16_t port = 0) noexcept;
				/**
				 * @brief Метод активации/деактивации мультикаст группы события
				 *
				 * @param id     идентификатор события
				 * @param mode   режим активации/деактивации
				 * @param group  мультикаст-группа для активации/деактивации
				 * @param source адрес сетевого интерфейса с которого выполняется подписка
				 * @param port   порт мультикаст-группы с которого выполняется подписка
				 * @return       результат выполнения установки
				 *
				 */
				bool membership(const event::id_t id, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port = 0) noexcept;
			public:
				/**
				 * @brief Метод привязки дополнительного ключа маршрутизации к сессии
				 *
				 * @note Одна сессия адресуется произвольным числом ключей: протоколы,
				 *       меняющие идентификатор по ходу работы, обращаются к ней по
				 *       любому из привязанных. Ключи снимаются автоматически при
				 *       уничтожении сессии
				 *
				 * @param id  идентификатор события сессии
				 * @param key привязываемый ключ сессии
				 * @return    результат привязки (false - ключ занят другой сессией)
				 *
				 */
				bool bind(const event::id_t id, const net::origin_key_t & key) noexcept;
				/**
				 * @brief Метод снятия ключа маршрутизации с сессии
				 *
				 * @param id  идентификатор события сессии
				 * @param key снимаемый ключ сессии
				 * @return    результат снятия (false - ключ сессии не принадлежит)
				 *
				 */
				bool unbind(const event::id_t id, const net::origin_key_t & key) noexcept;
			public:
				/**
				 * @brief Метод получения предельного количества одновременных подключений события
				 *
				 * @param id идентификатор события
				 * @return   предельное количество одновременных подключений
				 *
				 */
				uint32_t getMaxConnections(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки предельного количества одновременных подключений события
				 *
				 * @note Для потоковых событий ограничивает число принятых подключений,
				 *       для дейтаграммных - число сессий. Достижение предела означает
				 *       отказ в создании новой сессии, поэтому предел служит защитой
				 *       от исчерпания памяти потоком датаграмм от чужих отправителей
				 *
				 * @param id  идентификатор события
				 * @param max предельное количество одновременных подключений
				 * @return    результат установки
				 *
				 */
				bool setMaxConnections(const event::id_t id, const uint32_t max) noexcept;
			public:
				/**
				 * @brief Метод удаления события
				 *
				 * @details Освобождает событие из любого состояния: настраиваемого,
				 *          работающего, приостановленного. Снимает таймауты, закрывает
				 *          дескриптор и убирает событие из опроса.
				 *
				 *          Освобождение выполняется **не сразу**, а откладывается на
				 *          два оборота цикла событий. Отсрочка нужна по необходимости,
				 *          а не для удобства: записи подписки уходят в ядро вместе с
				 *          ожиданием следующего оборота, и закрой движок дескриптор
				 *          раньше, его номер операционная система успела бы выдать
				 *          другому объекту - записи легли бы на чужой дескриптор.
				 *
				 *          Для вызывающего отсрочка незаметна: обращения по этому
				 *          идентификатору перестают действовать сразу, а функции
				 *          обратного вызова по нему больше не приходят.
				 *
				 * @note    Вызывать изнутри функции обратного вызова этого же события
				 *          **допустимо и безопасно** - ровно из-за отсрочки. Это
				 *          обычный способ закрыть соединение по ошибке разбора или по
				 *          завершении обмена.
				 *
				 * @note    Повторный вызов по тому же идентификатору отказывает, а не
				 *          освобождает узел дважды: событие уже помечено, и найти его
				 *          по идентификатору больше нельзя.
				 *
				 * @note    Освобождение события сервера не освобождает принятые им
				 *          подключения - у каждого свой идентификатор и свой срок
				 *          жизни. Закрывать их следует своими вызовами
				 *
				 * @par Пример: закрытие соединения из обратного вызова
				 * @code{.cpp}
				 * io.on(client, static_cast <awh::engine::callback::read_t> ([&io](const awh::event::id_t id, const uint8_t * buffer, const size_t size) noexcept -> void {
				 *     // Если разбор принятых данных не удался, закрываем соединение
				 *     if(!parse(buffer, size))
				 *         // Освобождение отложится на два оборота цикла и выполнится безопасно
				 *         io.destroy(id);
				 * }));
				 * @endcode
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения удаления
				 *
				 */
				bool destroy(const event::id_t id) noexcept;
			public:
				/**
				 * @brief Метод получения пары событий для сокета
				 *
				 * @param family   семейство адресов
				 * @param type     тип сокета
				 * @param protocol протокол сокета
				 * @return         пара идентификаторов созданных событий
				 *
				 */
				std::array <event::id_t, 2> events(const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
			public:
				/**
				 * @brief Метод создания нового события
				 *
				 * @details Первый шаг работы с событием. Заводит узел события,
				 *          создаёт под него дескриптор операционной системы и
				 *          переводит событие в состояние `INITIAL`, в котором оно
				 *          принимает настройки, но ещё не работает.
				 *
				 *          Дескриптор создаётся **сразу**, а не при запуске, потому
				 *          что настройка события - адреса, порты, опции - выполняется
				 *          над готовым дескриптором. Отсюда следует, что созданное и
				 *          брошенное событие удерживает дескриптор до `destroy()`.
				 *
				 *          Тип узла определяет, чем событие будет: `CLIENT` и
				 *          `SERVER` - сокеты, `PEER` заводится движком сам при приёме
				 *          подключения, `TIMEOUT` и `INTERVAL` - таймеры, `FILE` и
				 *          `DIR` - наблюдение за файловой системой, `IPC` -
				 *          межпроцессное взаимодействие, `NOTIFY` - пользовательское
				 *          событие для передачи работы в поток цикла.
				 *
				 * @note    Для таймеров семейство адресов задаётся значением
				 *          `event::family_t::TIMER`, а тип и протокол не нужны вовсе.
				 *
				 * @note    Событие `PEER` через этот метод не создаётся: принятые
				 *          подключения движок заводит сам и отдаёт их идентификатор
				 *          в функцию обратного вызова приёма подключения уже готовым
				 *          и подписанным на чтение.
				 *
				 * @note    Нулевой идентификатор означает отказ создания. Проверять
				 *          его следует до настройки: обращения по недействительному
				 *          идентификатору молча ничего не делают, и без проверки
				 *          отказ обнаружился бы только отсутствием событий
				 *
				 * @par Пример: таймер
				 * @code{.cpp}
				 * // Заводим событие таймера и задаём ему задержку в две секунды
				 * const awh::event::id_t timer = io.event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
				 * io.setTimeout(timer, awh::event::action_t::NONE, 2000);
				 * io.commit(timer);
				 * io.launch(timer);
				 * @endcode
				 *
				 * @param node     узел события
				 * @param family   семейство адресов
				 * @param type     тип сокета
				 * @param protocol протокол сокета
				 * @return         идентификатор созданного события, нулевой при отказе
				 *
				 */
				event::id_t event(const event::node_t node, const event::family_t family, const event::type_t type = event::type_t::NONE, const event::protocol_t protocol = event::protocol_t::NONE) noexcept;
			public:
				/**
				 * @brief Метод получения смещения в файле события
				 *
				 * @param id   идентификатор события
				 * @param seek тип смещения в файле события
				 * @return     смещение в файле события
				 *
				 */
				size_t getSeek(const event::id_t id, const event::seek_t seek) noexcept;
				/**
				 * @brief Метод установки смещения в файле события
				 *
				 * @param id     идентификатор события
				 * @param seek   тип смещения в файле события
				 * @param offset смещение в файле события
				 * @return       результат выполнения установки
				 *
				 */
				bool setSeek(const event::id_t id, const event::seek_t seek, const size_t offset) noexcept;
			public:
				/**
				 * @brief Метод получения опций события
				 *
				 * @param id идентификатор события
				 * @return   опции события
				 *
				 */
				uint16_t getOptions(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки опций события
				 *
				 * @param id      идентификатор события
				 * @param options опции события для установки
				 * @return        результат выполнения установки
				 *
				 */
				bool setOptions(const event::id_t id, const uint16_t options) noexcept;
				/**
				 * @brief Метод установки опции события
				 *
				 * @param id     идентификатор события
				 * @param option опция события для установки
				 * @param mode   режим установки опции события
				 * @return       результат выполнения установки
				 *
				 */
				bool setOption(const event::id_t id, const uint16_t option, const bool mode) noexcept;
			public:
				/**
				 * @brief Метод объединения данных между событиями
				 *
				 * @param eid  идентификатор события-источника
				 * @param dest идентификатор события-приёмника
				 * @return     результат выполнения объединения
				 *
				 */
				bool splice(const event::id_t eid, const event::id_t dest) noexcept;
			public:
				/**
				 * @brief Метод запуска события
				 *
				 * @details Последний шаг подготовки: с него событие начинает участвовать в
				 *          опросе. До запуска событие заведено, настроено и, возможно,
				 *          подключено, но обратные вызовы ему не приходят.
				 *
				 *          Метод различает **два пути** по состоянию события, и оба
				 *          законны. Из состояния `INITIAL` запускается событие, которому
				 *          подключаться не нужно: таймер, наблюдение за файлом,
				 *          дейтаграммный сокет. Из состояния `SUCCESS` - событие, прошедшее
				 *          через `connect()` или `listen()`; для него запуск заодно
				 *          применяет накопившиеся изменения к ядру.
				 *
				 * @note Дейтаграммный сервер можно запускать и без `listen()`: слушать
				 *       очередь входящих соединений ему незачем. Потоковому `listen()`
				 *       обязателен, иначе запуск откажет
				 *
				 * @note Повторный запуск уже запущенного события отказывает: требуемых
				 *       состояний у него больше нет
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения запуска
				 *
				 */
				bool launch(const event::id_t id) noexcept;
			public:
				/**
				 * @brief Метод отключения события
				 *
				 * @details Разрывает соединение: закрывает дескриптор и переводит событие в
				 *          состояние отмены, после чего вызывается подписка `event_t` с
				 *          действием `DISCONNECT`. Само событие при этом **остаётся
				 *          живым** - его идентификатор действителен, подписки сохранены.
				 *          Этим отключение и отличается от `destroy()`, который событие
				 *          уничтожает.
				 *
				 * @note Дескриптор закрыт, поэтому просто запустить событие снова нельзя:
				 *       вернуть его в работу можно через `rebirth()`, пересоздающий
				 *       дескриптор с сохранением самого события
				 *
				 * @note Отключение должно быть событию разрешено соответствующим действием.
				 *       Если оно запрещено, метод молча ничего не делает и возвращает
				 *       отрицательный результат
				 *
				 * @note Событие, уже помеченное к уничтожению или отключённое, повторно не
				 *       отключается
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения отключения
				 *
				 */
				bool disconnect(const event::id_t id) noexcept;
				/**
				 * @brief Шаблон метода мультиподключения события к удалённым хостам
				 *
				 * @tparam Args список идентификаторов событий для подключения
				 *
				 */
				template <typename... Args>
				/**
				 * @brief Метод мультиподключения события к удалённым хостам
				 *
				 * @param args список идентификаторов событий для подключения
				 * @return     результат выполнения подключения
				 *
				 */
				bool connect(Args&&... args) noexcept {
					// Выполняем подключение к списку удалённых серверов
					return this->connect({args...});
				}
				/**
				 * @brief Метод мультиподключения события к удалённым хостам
				 *
				 * @details Начинает подключение и **возвращается сразу**, не дожидаясь его
				 *          исхода: соединение на неблокирующем сокете устанавливается за
				 *          несколько оборотов цикла. Положительный результат означает лишь
				 *          то, что попытка начата успешно. Об исходе сообщает подписка
				 *          `connect_t`, и до её вызова отправлять данные некуда.
				 *
				 *          Список идентификаторов позволяет начать несколько подключений
				 *          одним вызовом - они пойдут одновременно, а не по очереди, и
				 *          каждое сообщит о себе своим вызовом `connect_t`.
				 *
				 * @note Ставится **между** `commit()` и `launch()`: до фиксации адрес ещё не
				 *       применён, а запуск ожидает событие уже подключающимся
				 *
				 * @note Предел времени на установление соединения задаётся через
				 *       `setTimeout()` с действием `CONNECT`. Без него неудачная попытка
				 *       может висеть столько, сколько отведёт система
				 *
				 * @par Пример: клиент
				 * @code{.cpp}
				 * io.on(client, static_cast <awh::engine::callback::connect_t> ([&io](const awh::event::id_t id, const bool ok) noexcept -> void {
				 *     // Отправлять можно только отсюда: раньше соединения ещё нет
				 *     if(ok)
				 *         io.send(id, request.data(), request.size());
				 * }));
				 * io.setTimeout(client, awh::event::action_t::CONNECT, 5000);
				 * if(io.commit(client) && io.connect(client) && io.launch(client))
				 *     while(io.poll(100));
				 * @endcode
				 *
				 * @param ids список идентификаторов событий для подключения
				 * @return    результат выполнения подключения
				 *
				 */
				bool connect(const vector <event::id_t> & ids) noexcept;
			public:
				/**
				 * @brief Метод перевода события в режим прослушивания входящих соединений
				 *
				 * @details Открывает очередь входящих соединений: с этого момента ядро
				 *          принимает их и складывает в очередь, а разбирать её событие
				 *          начнёт с вызова `launch()`. Второй параметр задаёт предел
				 *          одновременно ожидающих соединений - тот самый backlog.
				 *
				 * @note Требуется **только потоковым** серверам. Дейтаграммному серверу
				 *       очередь соединений не нужна, и он обходится одним `launch()`
				 *
				 * @note Принятые соединения приходят подпиской `accept_t` уже заведёнными
				 *       событиями, и заводить их своими вызовами не требуется
				 *
				 * @par Пример: потоковый сервер
				 * @code{.cpp}
				 * const awh::event::id_t server = io.event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
				 * io.setSourcePort(server, 8080);
				 * io.setAddress(server, awh::event::address_t::IPV4, "0.0.0.0");
				 * io.on(server, static_cast <awh::engine::callback::accept_t> (onAccept));
				 * // Фиксация, очередь входящих, запуск - именно в этом порядке
				 * if(io.commit(server) && io.listen(server, 1024) && io.launch(server))
				 *     while(io.poll(100));
				 * @endcode
				 *
				 * @param id  идентификатор события
				 * @param max максимальное количество входящих соединений
				 * @return    результат выполнения перевода в режим прослушивания
				 *
				 */
				bool listen(const event::id_t id, const uint32_t max) noexcept;
			public:
				/**
				 * @brief Метод получения данных события
				 *
				 * @param id идентификатор события
				 * @return   результат получения данных
				 *
				 */
				bool recv(const event::id_t id) noexcept;
				/**
				 * @brief Метод отправки данных события
				 *
				 * @param id     идентификатор события
				 * @param buffer буфер данных для отправки
				 * @param size   размер данных для отправки
				 * @return       количество байт данных, отправленных событием
				 *
				 */
				size_t send(const event::id_t id, const void * buffer, const size_t size) noexcept;
				/**
				 * @brief Метод перенаправления объединённых данных в событие-приёмник (splice)
				 *
				 * @note Если на событии-приёмнике установлена функция инъекции (транспорт
				 *       шифрует данные на уровне соединения, напр. QUIC), данные передаются
				 *       ей для отправки собственным потоком; иначе выполняется обычная
				 *       отправка байт в сокет
				 *
				 * @note Дейтаграммный приёмник отправляет каждую запись очереди
				 *       отдельным сообщением, и запись, превышающую предел системы,
				 *       отправить нельзя вовсе. Такие данные переносятся частями:
				 *       порция источника устроена иначе, чем сообщение приёмника -
				 *       файл, например, читается страницами, которые предельную
				 *       дейтаграмму превышают. Делится только то, что иначе не
				 *       прошло бы ни одним октетом, поэтому границы сообщений у
				 *       проходящих целиком дейтаграмм сохраняются
				 *
				 * @param id     идентификатор события-приёмника
				 * @param buffer буфер перенаправляемых данных
				 * @param size   размер перенаправляемых данных
				 * @return       количество принятых на перенаправление байт
				 *
				 */
				size_t relay(const event::id_t id, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод установки глубины очереди принятия входящих соединений события
				 *
				 * @param id       идентификатор события
				 * @param depth    глубина очереди принятия входящих соединений
				 * @param adaptive флаг адаптивной глубины очереди принятия входящих соединений
				 *
				 */
				void backlog(const event::id_t id, const uint16_t depth, const bool adaptive = false) noexcept;
			public:
				/**
				 * @brief Метод получения размера буфера события
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @return       размер буфера события
				 *
				 */
				size_t getBufferSize(const event::id_t id, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки размера буфера события
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @param size   размер буфера события
				 * @return       результат выполнения установки
				 *
				 */
				bool setBufferSize(const event::id_t id, const event::action_t action, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод установки пропускной способности события
				 *
				 * @param limiting  режим ограничения пропускной способности события (egress или ingress)
				 * @param bandwidth пропускная способность события для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 *
				 */
				void bandwidth(const event::limiting_t limiting, string_view bandwidth) noexcept;
				/**
				 * @brief Метод установки пропускной способности события для события
				 *
				 * @param id        идентификатор события
				 * @param limiting  режим ограничения пропускной способности события (egress или ingress)
				 * @param bandwidth пропускная способность события для установки (например, "65536bps", "1280kbps", "100Mbps", "1Gbps", "10Gbps" или "auto")
				 * @return          результат выполнения установки
				 *
				 */
				bool bandwidth(const event::id_t id, const event::limiting_t limiting, string_view bandwidth) noexcept;
			public:
				/**
				 * @brief Метод получения режима трансляции пакетов для события
				 *
				 * @param id идентификатор события
				 * @return   режим трансляции пакетов (unicast, multicast, broadcast)
				 *
				 */
				event::delivery_mode_t getDelivery(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки режима трансляции пакетов для события
				 *
				 * @param id       идентификатор события
				 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
				 * @return         результат выполнения установки
				 *
				 */
				bool setDelivery(const event::id_t id, const event::delivery_mode_t delivery) noexcept;
			public:
				/**
				 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
				 *
				 * @param id идентификатор события
				 * @return   метаданные последнего принятого дейтаграммного пакета
				 *
				 */
				net::dgram_info_t getTrafficInfo(const event::id_t id) const noexcept;
			public:
				/**
				 * @brief Метод получения количества хопов последнего принятого пакета
				 *
				 * @param id идентификатор события
				 * @return   количество хопов последнего принятого пакета
				 *
				 */
				uint8_t getCountHops(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки количества хопов последнего принятого пакета
				 *
				 * @param id   идентификатор события
				 * @param hops количество хопов последнего принятого пакета
				 * @return     результат выполнения установки
				 *
				 */
				bool setCountHops(const event::id_t id, const uint8_t hops) noexcept;
			public:
				/**
				 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
				 *
				 * @param id идентификатор события
				 * @return   максимальное количество хопов
				 *
				 */
				event::hops_t getHops(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
				 *
				 * @param id   идентификатор события
				 * @param hops максимальное количество хопов
				 * @return     результат работы функции
				 *
				 */
				bool setHops(const event::id_t id, const event::hops_t hops) noexcept;
			public:
				/**
				 * @brief Метод получения режима использования таймаута для обработки события чтения
				 *
				 * @param id идентификатор события
				 * @return   режим использования таймаута для обработки события чтения
				 *
				 */
				event::usage_t getUsageReadTimeout(const event::id_t id) const noexcept;
				/**
				 * @brief Метод установки режима использования таймаута для обработки события чтения
				 *
				 * @details Определяет, что делать со сроком чтения, заданным через
				 *          `setTimeout()` с действием `READ`, после того как данные пришли.
				 *          Два режима отвечают двум разным по смыслу задачам.
				 *
				 *          `REUSABLE` - срок взводится заново после каждого чтения. Это
				 *          постоянный страж простоя: соединение обязано подавать признаки
				 *          жизни не реже заданного срока, иначе срабатывает таймаут.
				 *          Подходит потокам данных и долгоживущим подпискам.
				 *
				 *          `DISPOSABLE` - срок снимается, как только данные пришли, и
				 *          взводится заново при успешной отправке. То есть это не предел
				 *          простоя, а **ожидание ответа**: отправили запрос - пошёл отсчёт,
				 *          получили ответ - отсчёт снят. Подходит обмену «запрос-ответ», где
				 *          молчание в паузе между запросами законно.
				 *
				 * @note Режимом по умолчанию является `DISPOSABLE`. Ожидающим постоянной
				 *       активности соединениям режим следует менять явно, иначе простой
				 *       между запросами замечен не будет
				 *
				 * @note Действует только на событиях с неблокирующим или частично
				 *       блокирующим вводом-выводом: на блокирующих сроки держит сам сокет
				 *
				 * @param id    идентификатор события
				 * @param usage режим использования таймаута для обработки события чтения (reusable или disposable)
				 *
				 */
				void setUsageReadTimeout(const event::id_t id, const event::usage_t usage) noexcept;
			public:
				/**
				 * @brief Метод получения таймаута события
				 *
				 * @details Возвращает **заданный** срок, а не остаток до срабатывания:
				 *          сколько времени таймеру осталось, отсюда узнать нельзя. Нулевое
				 *          значение означает, что срок не выставлен или снят.
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @return       значение таймаута в миллисекундах
				 *
				 */
				uint32_t getTimeout(const event::id_t id, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки таймаута события
				 *
				 * @details Один метод обслуживает два разных по смыслу случая, и различает
				 *          их по типу узла.
				 *
				 *          **Узлы-таймеры** (`TIMEOUT`, `INTERVAL`) - здесь значение задаёт
				 *          саму задержку срабатывания, а действие не участвует и передаётся
				 *          как `action_t::NONE`. Разница между двумя типами узлов лишь в
				 *          том, срабатывает таймер однажды или повторяется. О срабатывании
				 *          сообщает подписка на `status_t` со статусом `SUCCESS` -
				 *          отдельной функции у таймеров нет.
				 *
				 *          **Узлы соединений** (`CLIENT`, `PEER`, `ORIGIN`, `MEDIATOR`) -
				 *          здесь значение задаёт предел простоя, а действие говорит, простой
				 *          в чём считать. По истечении срока вызывается подписка на
				 *          `timeout_t`, и если её нет, соединение уничтожается безусловно.
				 *
				 * @note Нулевое значение **снимает** срок: заведённый таймер разоружается,
				 *       а событие возвращается в исходное состояние. Это и есть способ
				 *       отменить ранее выставленный таймаут - отдельного метода для отмены
				 *       нет
				 *
				 * @note Выставлять можно в любой момент, в том числе уже работающему
				 *       событию: живой таймер перевзводится тут же, с новым сроком
				 *
				 * @note На **блокирующих** событиях сроки чтения и записи ставятся опциями
				 *       сокета, а не таймерами движка. Наблюдаемое поведение то же, но
				 *       подписка `timeout_t` в этом случае не работает - ждёт сам системный
				 *       вызов
				 *
				 * @par Допустимые действия
				 * | Действие | Для кого | Смысл |
				 * |---|---|---|
				 * | `NONE` | таймеры | задержка срабатывания |
				 * | `READ` | соединения | сколько ждать входящих данных |
				 * | `WRITE` | соединения | сколько ждать возможности отправить |
				 * | `CONNECT` | только `CLIENT` | сколько ждать установления соединения |
				 * | `RECONNECT` | только `CLIENT` | пауза перед повторной попыткой |
				 *
				 * Действие, узлу не подходящее, срок не выставляет: в лог уходит
				 * предупреждение, а подписка на `status_t` получает статус `FAILURE`.
				 *
				 * @par Пример: таймер и предел простоя
				 * @code{.cpp}
				 * // Интервал, срабатывающий каждые пять секунд
				 * const awh::event::id_t timer = io.event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
				 * io.setTimeout(timer, awh::event::action_t::NONE, 5000);
				 * // Клиенту - пять секунд на подключение и тридцать на молчание
				 * io.setTimeout(client, awh::event::action_t::CONNECT, 5000);
				 * io.setTimeout(client, awh::event::action_t::READ, 30000);
				 * // Передумали: снимаем предел простоя, оставив предел подключения
				 * io.setTimeout(client, awh::event::action_t::READ, 0);
				 * @endcode
				 *
				 * @param id      идентификатор события
				 * @param action  тип действия события
				 * @param timeout значение таймаута в миллисекундах
				 *
				 */
				void setTimeout(const event::id_t id, const event::action_t action, const uint32_t timeout) noexcept;
				/**
				 * @brief Метод продолжения прерванного ожидания
				 *
				 * @details Одноразовый срок ожидания чтения (`usage_t::DISPOSABLE`) снимается
				 *          приходом данных, и снимается **до** вызова отклика. Движку этого
				 *          довольно: данные пришли, ожидание кончилось. Договору - не всегда:
				 *          на дейтаграммном обмене прийти вправе что угодно и от кого угодно,
				 *          и пришедшее бывает не ответом на заданный вопрос, а чужим ответом,
				 *          ответом запоздалым либо шумом сети. Разобрать это способен лишь
				 *          сам договор, и лишь он вправе решить, что ожидание не кончилось
				 *
				 *          Решив так, договор зовёт этот метод, и ожидание продолжается.
				 *          Без него вопрос повисает навсегда: срок снят, взводить его заново
				 *          нечем, а другого срабатывания не будет
				 *
				 *          Задержка нулевая означает продолжение **с того места, где ожидание
				 *          прервано**: движок помнит остаток снятого срока и взводит ожидание
				 *          ровно на него. Тем и отличается продолжение от нового ожидания:
				 *          чужой ответ не дарит вопросу лишнего времени, и сколько бы их ни
				 *          пришло, отказ наступит в свой черёд
				 *
				 *          Задержка ненулевая задаёт ожидание заново, на указанный срок.
				 *          Нужна там, где договор знает больше движка: ответ пришёл частью,
				 *          и остаток разумно ждать иначе, чем ждали целое
				 *
				 * @note Продолжать нечего, если ожидание не прерывалось, прервано у события
				 *       иного, у срока иного либо успело истечь. Во всех этих случаях метод
				 *       отвечает отказом, ничего не взводя
				 *
				 * @warning Продолжение нулевой задержкой осмысленно **только внутри отклика**,
				 *          вызванного тем самым чтением, что прервало ожидание. Движок помнит
				 *          один прерванный срок, а не все: обращение позднее застанет запись
				 *          уже чужой и получит отказ. Обходить это, запоминая остаток у себя,
				 *          не следует - лучше позвать метод там, где решение и принимается
				 *
				 * @par Пример: чужой ответ ожидания не прерывает
				 * @code{.cpp}
				 * // Разбираем пришедший ответ
				 * if(header.id != this->_awaiting){
				 *     // Ответ не на наш вопрос - продолжаем ожидание с прерванного места
				 *     io.rearmTimeout(id, awh::event::action_t::READ);
				 *     // Ответ чужой, разбирать его нечего
				 *     return;
				 * }
				 * @endcode
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @param delay  задержка в миллисекундах, либо ноль для продолжения с остатка
				 * @return       результат продолжения ожидания
				 *
				 */
				bool rearmTimeout(const event::id_t id, const event::action_t action, const uint32_t delay = 0) noexcept;
			public:
				/**
				 * @brief Метод получения действия события
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @return       режим действия события
				 *
				 */
				event::mode_t getAction(const event::id_t id, const event::action_t action) const noexcept;
				/**
				 * @brief Метод установки действия события
				 *
				 * @param id     идентификатор события
				 * @param action тип действия события
				 * @param mode   режим установки действия события
				 * @return       результат выполнения установки
				 *
				 */
				bool setAction(const event::id_t id, const event::action_t action, const event::mode_t mode) noexcept;
			public:
				/**
				 * @brief Метод установки параметров keep-alive для события
				 *
				 * @param id    идентификатор события
				 * @param cnt   количество пакетов keep-alive
				 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
				 * @param intvl интервал между пакетами keep-alive в секундах
				 * @return      результат выполнения установки
				 *
				 */
				bool keepAlive(const event::id_t id, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * @brief Метод приостановки события
				 *
				 * @details Отключает чтение, не разрывая соединения: данные остаются в
				 *          приёмном буфере ядра, отправитель упирается в исчерпание окна и
				 *          сам сбавляет темп. Это штатный способ придержать поток, когда
				 *          принимающая сторона не успевает разбирать принятое, - в отличие
				 *          от `disconnect()`, соединение при этом цело.
				 *
				 * @note Снятие чтения выполняется **немедленно**, не дожидаясь очередного
				 *       оборота цикла: иначе успела бы прийти ещё порция данных. А вот
				 *       возобновление откладывается до следующего оборота, и это
				 *       расхождение намеренное
				 *
				 * @note Приостановить можно только запущенное событие, а возобновить -
				 *       только приостановленное. Повторные вызовы отказывают
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения приостановки
				 *
				 */
				bool pause(const event::id_t id) noexcept;
				/**
				 * @brief Метод возобновления события
				 *
				 * @details Возвращает чтение приостановленному событию. Накопившееся в
				 *          приёмном буфере ядра придёт обычными подписками на чтение,
				 *          начиная с очередного оборота цикла/
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения возобновления
				 *
				 */
				bool resume(const event::id_t id) noexcept;
			public:
				/**
				 * @brief Метод проверки состояния события
				 *
				 * @param id идентификатор события
				 * @return   состояние события
				 *
				 */
				bool isAlive(const event::id_t id) const noexcept;
			public:
				/**
				 * @brief Метод очистки сетевого движка
				 *
				 */
				void clear() noexcept;
			public:
				/**
				 * @brief Метод принудительного пинка базе событий
				 *
				 * @return результат выполнения операции
				 *
				 */
				bool kick() noexcept;
				/**
				 * @brief Метод инициализации сетевого движка
				 *
				 * @return результат выполнения инициализации
				 *
				 */
				bool initialize() noexcept;
				/**
				 * @brief Метод реинициализации сетевого движка
				 *
				 * @return результат выполнения реинициализации
				 *
				 */
				bool reinitialize() noexcept;
				/**
				 * @brief Метод деинициализации сетевого движка
				 *
				 * @return результат выполнения деинициализации
				 *
				 */
				bool deinitialize() noexcept;
			public:
				/**
				 * @brief Метод проверки состояния инициализации сетевого движка
				 *
				 * @return состояние инициализации
				 *
				 */
				bool isInitialized() const noexcept;
			public:
				/**
				 * @brief Метод получения количества событий в сетевом движке
				 *
				 * @return количество событий
				 *
				 */
				size_t eventsCount() const noexcept;
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
				 * @details Выбирает структуру, в которой движок держит сроки событий.
				 *          `SIMPLE` - упорядоченное множество с хэш-таблицей положений:
				 *          скромна по памяти, но каждая постановка срока выделяет по узлу в
				 *          обеих. `DIFFICULT` - двоичная куча со страничной таблицей слотов:
				 *          к аллокатору не обращается вовсе и на постановке быстрее в разы,
				 *          зато таблица слотов выделяется чанками по тысяче событий.
				 *
				 *          Умолчанием служит `SIMPLE` - как и прочие умолчания, оно
				 *          рассчитано на самую слабую машину и самый общий случай.
				 *          Приложению, которое держит много сроков и ставит их часто,
				 *          переключение выгодно, и выигрыш измеряется разами.
				 *
				 * @note Выбор общий для всего движка, а не для отдельного события, и менять
				 *       его следует **до** заведения событий: переключение сбрасывает уже
				 *       заведённые таймеры
				 *
				 * @par Пример: включить структуру для большого числа сроков
				 * @code{.cpp}
				 * awh::engine::io_t io(&fmk, &log);
				 * // Переключаем до заведения событий и до initialize()
				 * io.setInternalTimer(awh::event::timer_t::DIFFICULT);
				 * io.initialize();
				 * @endcode
				 *
				 * @param timer тип таймера для событий сетевого движка
				 *
				 */
				void setInternalTimer(const event::timer_t timer) noexcept;
			public:
				/**
				 * @brief Метод получения размера отслеживаемого файла
				 *
				 * @param id идентификатор события
				 * @return   размер файла
				 *
				 */
				size_t size(const event::id_t id) const noexcept;
			public:
				/**
				 * @brief Метод получения количества байт, доступных для записи в очередь события
				 *
				 * @param id идентификатор события
				 * @return   количество байт, доступных для записи
				 *
				 */
				size_t available(const event::id_t id) const noexcept;
			public:
				/**
				 * @brief Метод получения типа события
				 *
				 * @param id идентификатор события
				 * @return   тип события
				 *
				 */
				event::type_t type(const event::id_t id) const noexcept;
				/**
				 * @brief Метод получения типа узла события
				 *
				 * @param id идентификатор события
				 * @return   тип узла события
				 *
				 */
				event::node_t node(const event::id_t id) const noexcept;
				/**
				 * @brief Метод получения семейства события
				 *
				 * @param id идентификатор события
				 * @return   семейство адресов
				 *
				 */
				event::family_t family(const event::id_t id) const noexcept;
				/**
				 * @brief Метод получения статуса события
				 *
				 * @param id идентификатор события
				 * @return   статус события
				 *
				 */
				event::status_t status(const event::id_t id) const noexcept;
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
				 * @brief Метод опроса событий
				 *
				 * @details Выполняет **один оборот** цикла событий и возвращает
				 *          управление. Своего потока движок не создаёт и сам себя не
				 *          крутит - цикл ведёт вызывающий:
				 *
				 *          @code{.cpp}
				 *          while(io.poll(100));
				 *          @endcode
				 *
				 *          За один оборот выполняется: освобождение узлов,
				 *          отложенных позапрошлым оборотом; отправка накопленного
				 *          пакета изменений подписки вместе с ожиданием - одним
				 *          обращением к ядру, а не двумя; разбор полученных событий с
				 *          вызовом функций обратного вызова; разбор истёкших
				 *          дедлайнов таймеров.
				 *
				 *          Все функции обратного вызова вызываются **внутри** этого
				 *          метода. Пока они выполняются, оборот не завершён, поэтому
				 *          долгая работа в обратном вызове задерживает и остальные
				 *          события, и срабатывание таймеров.
				 *
				 * @note    Время ожидания ограничивается не только переданным
				 *          таймаутом, но и сроком ближайшего внутреннего таймера:
				 *          движок обязан проснуться к дедлайну, даже если вызывающий
				 *          просил ждать дольше или бесконечно.
				 *
				 * @note    Отрицательный таймаут означает ожидание без предела, но с
				 *          учётом таймеров; нулевой - опрос без ожидания вовсе,
				 *          пригодный для встраивания в чужой цикл событий.
				 *
				 * @note    Отрицательный результат означает отказ опроса, а не
				 *          отсутствие событий: оборот без единого события - это
				 *          обычный успех. Прерывание системного вызова сигналом
				 *          отказом не считается, движок продолжает работу.
				 *
				 * @note    Метод обязан вызываться из одного и того же потока. Первый
				 *          вызов запоминает поток опроса, и обращения к событиям из
				 *          других потоков после этого недопустимы
				 *
				 * @par Встраивание в чужой цикл событий
				 * @code{.cpp}
				 * // Опрос без ожидания: управление возвращается сразу
				 * while(running){
				 *     io.poll(0);
				 *     foreignLoopIteration();
				 * }
				 * @endcode
				 *
				 * @param timeout таймаут опроса в миллисекундах: отрицательный - без
				 *                предела, нулевой - без ожидания
				 * @return        результат выполнения опроса
				 *
				 */
				bool poll(const int32_t timeout = -1) noexcept;
			public:
				/**
				 * @brief Метод установки функции обратного вызова для обработки события чтения
				 *
				 * @details Подписка на приём данных. Буфер, приходящий в функцию,
				 *          принадлежит движку и действителен **только на время вызова**:
				 *          он переиспользуется под следующее чтение, поэтому данные,
				 *          нужные позже, следует скопировать.
				 *
				 * @par Общие правила для всех перегрузок `on()`
				 * Все перегрузки устроены одинаково, и сказанное здесь относится к
				 * каждой из них.
				 *
				 * Подписка выполняется **присваиванием**: повторный вызов с тем же типом
				 * функции заменяет прежнюю, не добавляя вторую. Двух обработчиков одного
				 * события одного вида быть не может, а передача пустой функции подписку
				 * снимает.
				 *
				 * Подписываться можно в любой момент, а не только до `commit()`. В
				 * частности, принятое подключение приходит уже заведённым событием, и
				 * подписки ему выставляются прямо в функции приёма - как в примере к
				 * описанию класса.
				 *
				 * Неизвестный идентификатор и событие, помеченное к уничтожению,
				 * **игнорируются молча**: ни исключения, ни возвращаемого признака здесь
				 * нет. Если же тип функции узлу не подходит - скажем, чтение для узла
				 * сервера, - в лог уходит предупреждение, а подписка не выставляется.
				 * Поэтому список поддерживаемых типов узлов указан у каждой перегрузки
				 * отдельно, и сверяться с ним стоит: опечатка в типе узла тихо оставит
				 * событие без обработчика.
				 *
				 * Вызывать `on()` изнутри функции обратного вызова безопасно, включая
				 * замену обработчика на самого себя.
				 *
				 * @note Приведение через `static_cast` требуется там, где по одной лямбде
				 *       перегрузку не выбрать однозначно. Так происходит с парой
				 *       `write_t` и `connect_t`: их второй параметр - `size_t` и `bool`, а
				 *       они приводятся друг к другу неявно. Перегрузки, различающиеся
				 *       типами перечислений или типом возврата, выбираются сами, и
				 *       приведения не требуют
				 *
				 * @par Поддерживаемые типы узлов
				 * `FILE`, `NOTIFY`, `IPC`, `PEER`, `ORIGIN`, `MEDIATOR`, `CLIENT`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::read_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки события записи
				 *
				 * @details Сообщает, сколько байт ушло в сокет. Полезно там, где скорость
				 *          отправки нужно согласовать с источником данных: размер
				 *          записанного - это и есть освободившееся место в очереди.
				 *
				 * @note Требует приведения через `static_cast`, иначе перегрузка
				 *       неотличима от `connect_t`
				 *
				 * @par Поддерживаемые типы узлов
				 * `FILE`, `NOTIFY`, `IPC`, `PEER`, `ORIGIN`, `CLIENT`, `SERVER`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::write_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки возврата неотправленных данных
				 *
				 * @details Срабатывает, когда отправить данные не удалось, и возвращает их
				 *          вызывающей стороне: движок их не сохраняет и после выхода из
				 *          функции освобождает. Решение о судьбе байтов - повторить
				 *          позже, отложить в свой буфер или отбросить - остаётся за
				 *          вызывающей стороной. Второй параметр говорит, откуда данные
				 *          вернулись: из самого события или из его очереди отправки.
				 *
				 * @note Без этой подписки неотправленные данные теряются без следа.
				 *       Событиям, где потеря недопустима, подписку следует выставлять
				 *       наравне с чтением
				 *
				 * @par Поддерживаемые типы узлов
				 * `FILE`, `IPC`, `PEER`, `ORIGIN`, `CLIENT`, `SERVER`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::spool_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки общего события
				 *
				 * @details Единая точка наблюдения за происходящим с событием: в функцию
				 *          приходит тип действия, а не его последствия. Нужна там, где
				 *          важен сам факт - для журналирования, счётчиков, отладки - а
				 *          разбирать данные незачем.
				 *
				 * @par Поддерживаемые типы узлов
				 * `NOTIFY`, `DIR`, `FILE`, `IPC`, `PEER`, `ORIGIN`, `MEDIATOR`, `CLIENT`,
				 * `SERVER`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::event_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки ошибки события
				 *
				 * @details Получает код ошибки и её текстовое описание. Подписка эта
				 *          заодно **подавляет вывод ошибок в лог**: пока она не
				 *          выставлена, движок печатает ошибки сам, а с ней - передаёт их
				 *          целиком на усмотрение вызывающей стороны.
				 *
				 * @note Поддерживается почти всеми типами узлов, включая таймеры, и
				 *       выставлять её стоит всегда: без неё причина отказа события
				 *       остаётся только в логе
				 *
				 * @par Поддерживаемые типы узлов
				 * `NOTIFY`, `TIMEOUT`, `INTERVAL`, `DIR`, `FILE`, `IPC`, `PEER`,
				 * `ORIGIN`, `TUNNEL`, `MEDIATOR`, `CLIENT`, `SERVER`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::error_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки изменений события
				 *
				 * @details Наблюдение за файловой системой: сообщает, что именно
				 *          произошло с файлом или каталогом, и с каким именно.
				 *
				 * @note Имя, приходящее в функцию, действительно ТОЛЬКО на время вызова
				 *
				 * @par Поддерживаемые типы узлов
				 * `DIR`, `FILE`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::vnode_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова инъекции объединённых данных (splice)
				 *
				 * @details Позволяет транспорту, шифрующему данные на уровне соединения,
				 *          принять перенаправленные из события-источника байты и отправить
				 *          их собственным потоком, а не записывать сырьём в сокет.
				 *          Отрицательный результат означает отказ принять данные.
				 *
				 * @par Поддерживаемые типы узлов
				 * `PEER`, `ORIGIN`, `MEDIATOR`, `CLIENT`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::inject_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обновления статуса события
				 *
				 * @details Сообщает о смене состояния события - подключено, отключено,
				 *          отказ и так далее.
				 *
				 * @note Через эту же подписку сообщают о срабатывании **узлы-таймеры**:
				 *       отдельной функции у них нет, и сработавший `TIMEOUT` или
				 *       `INTERVAL` приходит сюда со статусом `event::status_t::SUCCESS`.
				 *       Это единственный способ узнать о срабатывании таймера, и в
				 *       функции статус следует проверять: приходят и остальные состояния
				 *
				 * @par Пример: срабатывание интервала
				 * @code{.cpp}
				 * const awh::event::id_t timer = io.event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
				 * io.setTimeout(timer, awh::event::action_t::NONE, 5000);
				 * io.on(timer, static_cast <awh::engine::callback::status_t> ([](const awh::event::id_t id, const awh::event::status_t status) noexcept -> void {
				 *     // Интервал сработал, и сработает снова через те же пять секунд
				 *     if(status == awh::event::status_t::SUCCESS)
				 *         tick();
				 * }));
				 * io.commit(timer);
				 * io.launch(timer);
				 * @endcode
				 *
				 * @par Поддерживаемые типы узлов
				 * `NOTIFY`, `TIMEOUT`, `INTERVAL`, `DIR`, `FILE`, `IPC`, `PEER`,
				 * `ORIGIN`, `TUNNEL`, `MEDIATOR`, `CLIENT`, `SERVER`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::status_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для приёма входящего подключения
				 *
				 * @details Принятое подключение приходит **уже заведённым событием**: его
				 *          идентификатор передаётся вторым параметром, и заводить его
				 *          через `event()`, настраивать и запускать не требуется. Всё, что
				 *          нужно сделать в этой функции - выставить принятому событию
				 *          подписки, иначе принимаемые им данные обрабатывать будет некому.
				 *
				 * @note Время жизни принятого события движку не принадлежит: закрывать его
				 *       следует своим вызовом `destroy()`
				 *
				 * @par Поддерживаемые типы узлов
				 * `SERVER`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::accept_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для определения сессии дейтаграммного пакета
				 *
				 * @note Поддерживается только серверными узлами. Установка функции
				 *       переводит событие на маршрутизацию датаграмм по ключу
				 *       приложения вместо адреса отправителя
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::origin_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова на получение информационных метаданных о дейтаграммном пакете
				 *
				 * @details Сопутствующие сведения о датаграмме - откуда пришла, каким
				 *          интерфейсом принята, что несёт в заголовках. Сами данные
				 *          приходят обычной подпиской на чтение, а сюда попадает то, что в
				 *          них не содержится.
				 *
				 * @par Поддерживаемые типы узлов
				 * `CLIENT`, `SERVER`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::traffic_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки подключения
				 *
				 * @details Сообщает об исходе попытки подключения: признак говорит,
				 *          состоялось соединение или нет. До этого вызова отправлять данные
				 *          некуда, поэтому первая отправка клиента обычно делается именно
				 *          отсюда.
				 *
				 * @note Требует приведения через `static_cast`, иначе перегрузка
				 *       неотличима от `write_t`
				 *
				 * @par Поддерживаемые типы узлов
				 * `CLIENT`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::connect_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова на получение информации о пакетах в туннельном интерфейсе
				 *
				 * @details Сведения о пакетах, прошедших через туннельный интерфейс, вместе
				 *          с идентификатором удалённого узла, которому они принадлежат.
				 *
				 * @par Поддерживаемые типы узлов
				 * `TUNNEL`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::tuninfo_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки таймаута события
				 *
				 * @details Срабатывает, когда истёк срок, заданный через `setTimeout()`:
				 *          соединение не приняло данных (`READ`), не смогло их отправить
				 *          (`WRITE`), не установилось (`CONNECT`) или подошла пора повторить
				 *          попытку (`RECONNECT`).
				 *
				 * @warning Смысл возвращаемого признака **зависит от действия**. Для
				 *          `READ`, `WRITE` и `CONNECT` положительный признак узел
				 *          уничтожает, а отрицательный оставляет жить. Для `RECONNECT` всё
				 *          наоборот: положительный означает «переподключаться», а прервать
				 *          попытку нужно отрицательным. Одна функция обслуживает все
				 *          действия сразу, и различать эти случаи обязана она
				 *
				 * @note Если подписка не выставлена, узел по истечении срока `READ`,
				 *       `WRITE` или `CONNECT` уничтожается **безусловно**. То есть она
				 *       нужна ровно затем, чтобы обрыв предотвратить или обставить своими
				 *       действиями
				 *
				 * @note По истечении срока `CONNECT` дополнительно вызывается подписка
				 *       `connect_t` с отрицательным исходом, и происходит это **до** вызова
				 *       этой функции
				 *
				 * @par Пример: разные действия - разный смысл ответа
				 * @code{.cpp}
				 * io.setTimeout(client, awh::event::action_t::READ, 30000);
				 * io.setTimeout(client, awh::event::action_t::RECONNECT, 5000);
				 * io.on(client, static_cast <awh::engine::callback::timeout_t> ([&attempts](const awh::event::id_t id, const awh::event::action_t action, const uint32_t delay) noexcept -> bool {
				 *     // Переподключение: положительный ответ означает «пробовать снова»
				 *     if(action == awh::event::action_t::RECONNECT)
				 *         return (attempts++ < 3);
				 *     // Простой: положительный ответ означает «рвать соединение»
				 *     return true;
				 * }));
				 * @endcode
				 *
				 * @par Поддерживаемые типы узлов
				 * `PEER`, `ORIGIN`, `CLIENT`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::timeout_t cb) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки доступности очереди события
				 *
				 * @details Сообщает, что в очереди отправки освободилось место, и передаёт
				 *          доступный размер. Это обратная связь для источника данных:
				 *          отправлять следующую порцию имеет смысл отсюда, а не вслепую -
				 *          так очередь не растёт без предела, а отправка идёт со скоростью,
				 *          которую держит соединение.
				 *
				 * @par Поддерживаемые типы узлов
				 * `NOTIFY`, `TIMEOUT`, `INTERVAL`, `DIR`, `FILE`, `IPC`, `PEER`,
				 * `ORIGIN`, `TUNNEL`, `MEDIATOR`, `CLIENT`, `SERVER`
				 *
				 * @param id идентификатор события
				 * @param cb функция обратного вызова
				 *
				 */
				void on(const event::id_t id, engine::callback::available_t cb) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект работы с логами
				 *
				 */
				explicit IO(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~IO() noexcept;
		} io_t;
	};
};

#endif // __AWH_IO_ENGINE__
