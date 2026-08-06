/**
 * @file: static.cpp
 * @date: 2025-12-15
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Статические тесты асинхронного движка ввода-вывода — проверка создания и сброса объекта модуля,
 *        а также корректности регистрации событий, управления подписками,
 *        работы таймеров и корректной остановки цикла событий
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Подключаем стандартные модули
 */
#include <cinttypes>
#include <atomic>
#include <set>
#include <thread>

/**
 * Подключаем стандартные модули
 */
/**
 * Для операционной системы MS Windows
 *
 * @note Заголовки эти принадлежат POSIX и у MS Windows отсутствуют:
 *       соответствующие им объявления приходят там из winsock2.h,
 *       подключаемого через единую точку sys/win32.hpp
 *
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/win32.hpp>
/**
 * Для всех остальных операционных систем
 */
#else
	/**
	 * Системные заголовочные файлы
	 */
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
#endif

/**
 * Подключаем восполнение средств POSIX, отсутствующих у MS Windows
 */
#include "../../posix.hpp"

/**
 * Подключаем заголовочный файлы проекта
 */
#include "io.hpp"

/**
 * Снимаем макросы MS Windows, сталкивающиеся с именами членов перечислений AWH
 *
 * @details Проверка эта пишет «awh::event::error_t::INVALID_SOCKET», а MS Windows
 *          заводит имя INVALID_SOCKET макросом. Заголовки AWH защищают **свои**
 *          объявления парой macro_push.hpp и macro_pop.hpp, но возвращают макросы
 *          следом - тем они и оставляют их тому, кто ими пользуется по делу. Потому
 *          всякий, кто называет такие члены в **своём** коде, защищает свой файл
 *          той же парой, и проверка эта не исключение
 *
 * @note Снятие идёт после всех подключений и снимается в конце файла: макросы эти
 *       нужны самим заголовкам MS Windows, и снимать их прежде подключения нельзя
 *
 */
#include <sys/macro_push.hpp>


/**
 * Системные заголовочные файлы
 */

/**
 * @brief Генерация случайного порта в диапазоне 49152-65535
 *
 * @return случайный порт
 *
 */
static uint16_t port() noexcept {
	/**
	 * Инициализация генератора случайных чисел
	 */
	::srandom(::time(nullptr) ^ ::getpid());
	/**
	 * Возвращаем случайный порт из диапазона 49152-65535
	 */
	return (49152 + (::random() % (65535 - 49152 + 1)));
}

/**
 * @brief Тест создания объекта работы со списком параметров URL
 *
 */
TEST_F(IoFixture, CreateIoTest){
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
	// Сбрасываем объект асинхронного движка ввода-вывода
	this->_io.reset();
	// Проверяем, что объект асинхронного движка ввода-вывода сброшен
	ASSERT_TRUE(this->_io == nullptr);
}

/**
 * @brief Тест сброса и повторного создания объекта асинхронного движка ввода-вывода
 *
 */
TEST_F(IoFixture, ResetAndCreateIoTest){
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
	// Сбрасываем объект асинхронного движка ввода-вывода
	this->_io.reset();
	// Проверяем, что объект асинхронного движка ввода-вывода сброшен
	ASSERT_TRUE(this->_io == nullptr);
	// Создаём объект асинхронного движка ввода-вывода заново
	this->_io = std::make_unique <awh::engine::io_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
}

/**
 * @brief Тест повторного создания объекта асинхронного движка ввода-вывода
 *
 */
TEST_F(IoFixture, ReCreateIoTest){
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
	// Создаём объект асинхронного движка ввода-вывода заново
	this->_io = std::make_unique <awh::engine::io_t> (this->_fmk.get(), this->_log.get());
	// Проверяем, что объект асинхронного движка ввода-вывода создан
	ASSERT_TRUE(this->_io != nullptr);
}

/**
 * @note TODO: Полноценный юнит-тест сетевого движка ещё предстоит написать отдельной
 *       задачей (сейчас движок покрыт лишь частично: create/reset, ping, таймеры).
 *       Ниже добавлено точечное покрытие метода rebuild() - пересоздания нижележащего
 *       дескриптора события с сохранением самого события (используется дочерними
 *       процессами кластера для получения собственного сокета вместо унаследованного
 *       от мастера, где SO_REUSEPORT требует отдельного сокета на процесс).
 *
 */

/**
 * @brief Тест перезарядки интервала при нескольких таймерах одновременно
 *
 * @details Интервал проверяется в паре со вторым таймером намеренно. Структура
 *          дедлайнов извлекает сработавший таймер обменом корня с последним
 *          элементом, а обмена при единственном таймере не бывает - и дефект
 *          перезарядки, при котором интервал срабатывал ровно один раз и умолкал,
 *          на одиночном интервале не воспроизводится вовсе
 *
 */
TEST_F(IoFixture, IoTimerPairTest){
	// Количество срабатываний интервала
	uint8_t intervals = 0;
	// Количество срабатываний таймаута
	uint8_t timeouts = 0;
	// Добавляем событие интервала
	const awh::event::id_t interval = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
	// Добавляем событие таймаута
	const awh::event::id_t timeout = this->_io->event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
	// Проверяем что события созданы
	ASSERT_GT(interval, 0u);
	ASSERT_GT(timeout, 0u);
	/**
	 * Задержка таймаута выбрана заведомо больше задержки интервала: тогда в куче
	 * дедлайнов интервал оказывается корнем, а таймаут - последним элементом, то
	 * есть при срабатывании интервала обмен действительно происходит
	 */
	this->_io->setTimeout(interval, awh::event::action_t::NONE, 300);
	this->_io->setTimeout(timeout, awh::event::action_t::NONE, 2000);
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Выполняем фиксацию настроек событий
	ASSERT_TRUE(this->_io->commit(interval));
	ASSERT_TRUE(this->_io->commit(timeout));
	// Устанавливаем функцию обратного вызова на событие интервала
	this->_io->on(interval, [&intervals]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Если статус события успешен, увеличиваем количество срабатываний интервала
		if(status == awh::event::status_t::SUCCESS)
			// Увеличиваем количество срабатываний интервала
			intervals++;
	});
	// Устанавливаем функцию обратного вызова на событие таймаута
	this->_io->on(timeout, [&timeouts]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Если статус события успешен, увеличиваем количество срабатываний таймаута
		if(status == awh::event::status_t::SUCCESS)
			// Увеличиваем количество срабатываний таймаута
			timeouts++;
	});
	// Выполняем запуск событий
	ASSERT_TRUE(this->_io->launch(interval));
	ASSERT_TRUE(this->_io->launch(timeout));
	// Запоминаем время начала опроса событий
	const auto start = std::chrono::steady_clock::now();
	/**
	 * Запускаем опрос событий до накопления пяти срабатываний интервала и
	 * срабатывания таймаута: пять периодов интервала истекают раньше задержки
	 * таймаута, и выходить из опроса только по интервалу было бы рано
	 *
	 * @note Опрос ограничен по времени намеренно. Если интервал перестаёт
	 *       перезаряжаться, ждать накопления срабатываний бессмысленно - без
	 *       ограничения тест не падал бы, а висел
	 */
	while(((intervals < 5) || (timeouts < 1)) && (std::chrono::duration_cast <std::chrono::seconds> (std::chrono::steady_clock::now() - start).count() < 10) && this->_io->poll());
	/**
	 * Проверяем что интервал перезаряжался, а не смолкал после первого срабатывания.
	 * Именно этой проверки и не хватало: одиночного срабатывания хватало любому
	 * тесту, который лишь дожидался первого события
	 */
	ASSERT_GE(intervals, 5);
	// Проверяем что таймаут за это время тоже сработал и ровно один раз
	ASSERT_EQ(timeouts, 1);
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест перестройки серверного события до фиксации (сценарий дочернего процесса кластера)
 *
 */
TEST_F(IoFixture, RebuildServerBeforeCommitTest){
	// Генерируем случайный порт привязки
	const uint16_t listenPort = port();
	// Выполняем инициализацию сетевого движка
	ASSERT_TRUE(this->_io->initialize());
	// Создаём серверное событие TCP
	awh::event::id_t eid = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем, что идентификатор события создан
	ASSERT_GT(eid, 0);
	// Устанавливаем адрес привязки
	ASSERT_TRUE(this->_io->setAddress(eid, awh::event::address_t::IPV4, "127.0.0.1"));
	// Устанавливаем порт привязки
	ASSERT_TRUE(this->_io->setSourcePort(eid, listenPort));
	// Событие ещё не зафиксировано
	ASSERT_EQ(awh::event::status_t::NONE, this->_io->status(eid));
	// Перестраиваем дескриптор до фиксации - должно пройти успешно, статус остаётся неопределённым
	ASSERT_TRUE(this->_io->rebuild(eid));
	// Проверяем, что статус события не изменился
	ASSERT_EQ(awh::event::status_t::NONE, this->_io->status(eid));
	// Фиксируем событие (привязка нового дескриптора)
	ASSERT_TRUE(this->_io->commit(eid));
	// Проверяем, что порт привязки сохранился
	ASSERT_EQ(listenPort, this->_io->getSourcePort(eid));
	// Уничтожаем событие
	this->_io->destroy(eid);
}

/**
 * @brief Тест перестройки серверного события в режиме прослушивания (полный цикл commit -> listen -> launch)
 *
 */
TEST_F(IoFixture, RebuildServerListeningTest){
	// Генерируем случайный порт привязки
	const uint16_t listenPort = port();
	// Выполняем инициализацию сетевого движка
	ASSERT_TRUE(this->_io->initialize());
	// Создаём серверное событие TCP
	awh::event::id_t eid = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем, что идентификатор события создан
	ASSERT_GT(eid, 0);
	// Устанавливаем адрес привязки
	ASSERT_TRUE(this->_io->setAddress(eid, awh::event::address_t::IPV4, "127.0.0.1"));
	// Устанавливаем порт привязки
	ASSERT_TRUE(this->_io->setSourcePort(eid, listenPort));
	// Устанавливаем опции переиспользования адреса и порта
	ASSERT_TRUE(this->_io->setOptions(eid, static_cast <uint16_t> (awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT)));
	// Фиксируем событие (привязка дескриптора)
	ASSERT_TRUE(this->_io->commit(eid));
	// Переводим событие в режим прослушивания
	// (стадию launch, включающую фильтр чтения, не выполняем: ей нужен запущенный
	//  цикл событий - поведение уровня цикла проверяется прогоном кластера, не здесь)
	ASSERT_TRUE(this->_io->listen(eid, 128));
	// Проверяем, что событие переведено в режим прослушивания
	ASSERT_EQ(awh::event::status_t::SUCCESS, this->_io->status(eid));
	// Запоминаем привязанный порт
	const uint16_t boundPort = this->_io->getSourcePort(eid);
	// Перестраиваем дескриптор - статус и порт должны сохраниться
	ASSERT_TRUE(this->_io->rebuild(eid));
	// Проверяем, что событие вновь в режиме прослушивания
	ASSERT_EQ(awh::event::status_t::SUCCESS, this->_io->status(eid));
	// Проверяем, что привязанный порт сохранился
	ASSERT_EQ(boundPort, this->_io->getSourcePort(eid));
	// Проверяем, что опция переиспользования порта сохранилась
	ASSERT_TRUE(this->_io->getOptions(eid) & static_cast <uint16_t> (awh::event::options::REUSE_PORT));
	// Уничтожаем событие
	this->_io->destroy(eid);
}

/**
 * @brief Тест перестройки пары IPC целиком по одному идентификатору
 *
 */
TEST_F(IoFixture, RebuildIpcPairTest){
	// Создаём пару IPC (UNIX-доменный socketpair)
	auto ids = this->_io->events(awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::NONE);
	// Проверяем, что оба идентификатора пары созданы
	ASSERT_GT(ids[0], 0);
	ASSERT_GT(ids[1], 0);
	// Перестраиваем пару целиком по одному из идентификаторов
	ASSERT_TRUE(this->_io->rebuild(ids[0]));
	// Проверяем, что оба узла пары остаются событиями типа IPC
	ASSERT_EQ(awh::event::node_t::IPC, this->_io->node(ids[0]));
	ASSERT_EQ(awh::event::node_t::IPC, this->_io->node(ids[1]));
	// Уничтожаем оба события пары
	this->_io->destroy(ids[0]);
	this->_io->destroy(ids[1]);
}

/**
 * @brief Тест того, что перестройка неприменима к событию без дескриптора (таймер)
 *
 */
TEST_F(IoFixture, RebuildUnsupportedTypeTest){
	// Создаём событие интервального таймера (без сетевого дескриптора)
	awh::event::id_t eid = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
	// Проверяем, что идентификатор события создан
	ASSERT_GT(eid, 0);
	// Перестройка дескриптора неприменима - метод возвращает ложь
	ASSERT_FALSE(this->_io->rebuild(eid));
	// Уничтожаем событие
	this->_io->destroy(eid);
}

/**
 * @brief Тест того, что перестройка несуществующего события возвращает ложь
 *
 */
TEST_F(IoFixture, RebuildUnknownIdTest){
	// Перестройка несуществующего идентификатора события - метод возвращает ложь
	ASSERT_FALSE(this->_io->rebuild(static_cast <awh::event::id_t> (999999999)));
}

/**
 * @brief Тест допустимых комбинаций объединения данных (splice) между узлами
 *
 * @note Проверяется только матрица допустимости комбинаций типов узлов и установка
 *       приёмника: сам перенос данных проверяется прогоном соединения (см.
 *       IoUDPSpliceConnectTest) и прогоном QUIC-прокси (sample server-quic-proxy).
 *       Объединение опирается лишь на наличие узлов в реестре, поэтому фиксация
 *       (commit) и сокеты здесь не требуются
 *
 */
TEST_F(IoFixture, SpliceValidCombinationsTest){
	// Создаём событие клиента TCP (узел-источник)
	awh::event::id_t client = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Создаём второе событие клиента TCP (узел-приёмник)
	awh::event::id_t client2 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Создаём серверное событие TCP (узел-приёмник)
	awh::event::id_t server = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем, что все идентификаторы событий созданы
	ASSERT_GT(client, 0);
	ASSERT_GT(client2, 0);
	ASSERT_GT(server, 0);
	// Объединение клиент -> сервер допустимо
	ASSERT_TRUE(this->_io->splice(client, server));
	// Объединение клиент -> клиент допустимо
	ASSERT_TRUE(this->_io->splice(client, client2));
	// Уничтожаем созданные события
	this->_io->destroy(client);
	this->_io->destroy(client2);
	this->_io->destroy(server);
}

/**
 * @brief Тест того, что серверный узел не может быть источником объединения данных
 *
 */
TEST_F(IoFixture, SpliceServerSourceRejectedTest){
	// Создаём серверное событие TCP (недопустимый узел-источник)
	awh::event::id_t server = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Создаём событие клиента TCP (узел-приёмник)
	awh::event::id_t client = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем, что идентификаторы событий созданы
	ASSERT_GT(server, 0);
	ASSERT_GT(client, 0);
	// Серверный узел не является допустимым источником - объединение отклоняется
	ASSERT_FALSE(this->_io->splice(server, client));
	// Уничтожаем созданные события
	this->_io->destroy(server);
	this->_io->destroy(client);
}

/**
 * @brief Тест того, что объединение с узлом таймера отклоняется
 *
 */
TEST_F(IoFixture, SpliceTimerRejectedTest){
	// Создаём событие клиента TCP (узел-источник)
	awh::event::id_t client = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Создаём событие интервального таймера (недопустимый узел-приёмник)
	awh::event::id_t timer = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
	// Проверяем, что идентификаторы событий созданы
	ASSERT_GT(client, 0);
	ASSERT_GT(timer, 0);
	// Узел таймера не может быть приёмником объединения - объединение отклоняется
	ASSERT_FALSE(this->_io->splice(client, timer));
	// Уничтожаем созданные события
	this->_io->destroy(client);
	this->_io->destroy(timer);
}

/**
 * @brief Тест того, что объединение с несуществующим событием возвращает ложь
 *
 */
TEST_F(IoFixture, SpliceUnknownIdTest){
	// Создаём событие клиента TCP
	awh::event::id_t client = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Проверяем, что идентификатор события создан
	ASSERT_GT(client, 0);
	// Несуществующий узел-источник - объединение отклоняется
	ASSERT_FALSE(this->_io->splice(static_cast <awh::event::id_t> (999999999), client));
	// Несуществующий узел-приёмник - объединение отклоняется
	ASSERT_FALSE(this->_io->splice(client, static_cast <awh::event::id_t> (999999999)));
	// Уничтожаем событие
	this->_io->destroy(client);
}

/**
 * @brief Тест набора сетевых тестов
 *
 */
TEST_F(IoFixture, IoSuiteTest){
	// Устанавливаем логгер
	this->_fmk->setLogger(this->_log.get());
	// Создаём объект работы с Ethernet
	awh::eth_t eth(this->_fmk.get(), this->_log.get());
	// Временный объект для извлечения сетевого интерфейса
	awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
	// Выполняем извлечение сетевых параметров
	eth.addr.fillSource(source);
	// Проверяем, что название сетевого интерфейса получено
	ASSERT_FALSE(source.iface.empty());
	// Устанавливаем тип таймера для событий сетевого движка
	this->_io->setInternalTimer(awh::event::timer_t::DIFFICULT);
	// Проверяем, что тип таймера для событий сетевого движка установлен
	ASSERT_EQ(awh::event::timer_t::DIFFICULT, this->_io->getInternalTimer());
	// Если сетевой интерфейс не принадлежит к VPN
	if(::memcmp("ut", source.iface.c_str(), 2) != 0){
		// MAC-адрес и IP-адрес сетевого интерфейса
		std::string mac = "", ip = "";
		/**
		 * IPv4 событие
		 */
		{
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid1 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid1, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid1, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid1));
			// Устанавливаем MTU события
			// ASSERT_TRUE(this->_io->setMaximumTransmissionUnit(eid1, 1500));
			// Проверяем что MTU получен
			ASSERT_EQ(1500, this->_io->getMaximumTransmissionUnit(eid1));
			// Устанавливаем сетевой интерфейс события
			ASSERT_TRUE(this->_io->setIface(eid1, source.iface));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid1).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid1));

			/**
			 * Для операционной системы FreeBSD
			 */
			#if __FreeBSD__
				// Извлекаем информационные метаданные SCTP сообщения
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid1);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Устанавливаем информационные метаданные SCTP сообщения
				this->_sctp->messageInfo(eid1, minfo);
				// Извлекаем статус SCTP событий SCTP
				const awh::net::sctp::status_t & status = this->_sctp->status(eid1);
				// Возвращаем статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;

				// Текст инициализационных сообщений SCTP
				awh::net::sctp::initmsg_t initmsg;
				// Устанавливаем количество попыток подключения SCTP
				initmsg.attempts = 4;
				// Устанавливаем количество исходящих потоков SCTP
				initmsg.ostreams = 5;
				// Устанавливаем количество входящих потоков SCTP
				initmsg.istreams = 5;
				// Инициализируем сообщения SCTP
				this->_sctp->initMessages(eid1, initmsg);
				// Типы SCTP событий для подписки
				awh::net::sctp::event_types_t types = {
					awh::net::sctp::event_type_t::ASSOC_CHANGE,
					awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
					awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
					awh::net::sctp::event_type_t::REMOTE_ERROR,
					awh::net::sctp::event_type_t::AUTHENTICATION_EVENT
				};
				// Выполняем подписку на SCTP события
				this->_sctp->eventsSubscribe(eid1, types);

				// Проверяем что типы SCTP событий совпадают с установленными ранее (так-как событие не принадлежит к SCTP)
				ASSERT_NE(types, this->_sctp->eventsSubscribed(eid1));

				// Устанавливаем таймаут INIT SCTP события
				ASSERT_FALSE(this->_sctp->setTimeout(eid1, awh::net::sctp::timeout_t::INIT, 3000));
				// Проверяем что таймаут INIT SCTP события получен
				ASSERT_NE(3000, this->_sctp->getTimeout(eid1, awh::net::sctp::timeout_t::INIT));

				// Устанавливаем таймаут DATA SCTP события
				ASSERT_FALSE(this->_sctp->setTimeout(eid1, awh::net::sctp::timeout_t::DATA, 3000));
				// Проверяем что таймаут DATA SCTP события получен
				ASSERT_NE(3000, this->_sctp->getTimeout(eid1, awh::net::sctp::timeout_t::DATA));

				// Устанавливаем таймаут SACK SCTP события
				ASSERT_FALSE(this->_sctp->setTimeout(eid1, awh::net::sctp::timeout_t::SACK, 3000));
				// Проверяем что таймаут SACK SCTP события получен
				ASSERT_NE(3000, this->_sctp->getTimeout(eid1, awh::net::sctp::timeout_t::SACK));

				// Устанавливаем таймаут COOKIE SCTP события
				ASSERT_FALSE(this->_sctp->setTimeout(eid1, awh::net::sctp::timeout_t::COOKIE, 3000));
				// Проверяем что таймаут COOKIE SCTP события получен
				ASSERT_NE(3000, this->_sctp->getTimeout(eid1, awh::net::sctp::timeout_t::COOKIE));

				// Устанавливаем таймаут SHUTDOWN SCTP события
				ASSERT_FALSE(this->_sctp->setTimeout(eid1, awh::net::sctp::timeout_t::SHUTDOWN, 3000));
				// Проверяем что таймаут SHUTDOWN SCTP события получен
				ASSERT_NE(3000, this->_sctp->getTimeout(eid1, awh::net::sctp::timeout_t::SHUTDOWN));

				// Устанавливаем таймаут HEARTBEAT SCTP события
				ASSERT_FALSE(this->_sctp->setTimeout(eid1, awh::net::sctp::timeout_t::HEARTBEAT, 3000));
				// Проверяем что таймаут HEARTBEAT SCTP события получен
				ASSERT_NE(3000, this->_sctp->getTimeout(eid1, awh::net::sctp::timeout_t::HEARTBEAT));

				// Устанавливаем таймаут SHUTDOWNACK SCTP события
				ASSERT_FALSE(this->_sctp->setTimeout(eid1, awh::net::sctp::timeout_t::SHUTDOWNACK, 3000));
				// Проверяем что таймаут SHUTDOWNACK SCTP события получен
				ASSERT_NE(3000, this->_sctp->getTimeout(eid1, awh::net::sctp::timeout_t::SHUTDOWNACK));

				// Устанавливаем ключ аутентификации SCTP-сокета
				ASSERT_FALSE(this->_sctp->authenticateKey(eid1, 1, "0123456789abcdef0123456789abcdef"));
				// Устанавливаем режим использования ключа аутентификации SCTP-сокета
				ASSERT_FALSE(this->_sctp->authenticateKey(eid1, awh::event::mode_t::ENABLED, 1));
				// Устанавливаем поддерживаемые алгоритмы аутентификации SCTP-сокета
				ASSERT_FALSE(this->_sctp->authenticateSupportAlgorithms(eid1, {awh::net::sctp::auth_type_t::HMAC_SHA1, awh::net::sctp::auth_type_t::HMAC_SHA256}));
				// Устанавливаем чанки аутентификации SCTP-сокета
				ASSERT_FALSE(this->_sctp->authenticateChunks(eid1, {awh::net::sctp::auth_chunk_t::DATA, awh::net::sctp::auth_chunk_t::SHUTDOWN}));

				// Извлекаем чанки аутентификации SCTP-сокета
				std::vector <awh::net::sctp::auth_chunk_t> chunks;
				// Выполняем извлечение чанков аутентификации SCTP-сокета
				ASSERT_FALSE(this->_sctp->authenticateChunks(eid1, awh::event::origin_t::LOCAL, chunks));
				// Проверяем что чанки аутентификации SCTP-сокета получены
				ASSERT_TRUE(chunks.empty());
				/**
				 * Перебираем все извлечённые чанки
				 */
				for(auto & chunk : chunks)
					// Записываем в лог информацию о чанках аутентификации SCTP-сокета
					std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
			#endif

			// Извлекаем IP-адрес сетевого интерфейса
			ip = this->_io->getAddress(eid1, awh::event::address_t::IPV4);
			// Извлекаем MAC-адрес сетевого интерфейса
			mac = this->_io->getAddress(eid1, awh::event::address_t::MAC);
			// Проверяем, что IP-адрес получен
			ASSERT_FALSE(ip.empty());
			// Проверяем, что MAC-адрес получен
			ASSERT_FALSE(mac.empty());
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid1).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid1, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid2 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid2, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid2, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid2));
			// Устанавливаем MAC-адрес события
			ASSERT_TRUE(this->_io->setAddress(eid2, awh::event::address_t::MAC, mac));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid2).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid2));
			// Устанавливаем MTU события
			// ASSERT_TRUE(this->_io->setMaximumTransmissionUnit(eid2, 1500));
			// Проверяем что MTU получен
			ASSERT_EQ(1500, this->_io->getMaximumTransmissionUnit(eid2));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->getAddress(eid2, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->getAddress(eid2, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid2).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid2, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid3 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid3, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid3, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid3));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(eid3, awh::event::address_t::IPV4, ip));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid3).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid3));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->getAddress(eid3, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->getAddress(eid3, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid3).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid3, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid4 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid4, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid4, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid4));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(eid4, awh::event::address_t::NETWORK, ip + "/255.255.255.0"));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid4).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid4));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->getAddress(eid4, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->getAddress(eid4, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid4).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid4, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid5 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid5, 0);
			// Устанавливаем порт события
			ASSERT_FALSE(this->_io->setTargetPort(eid5, 8080));
			// Устанавливаем UDS-адрес события
			ASSERT_TRUE(this->_io->setAddress(eid5, awh::event::address_t::UDS, "/tmp/awh.sock"));
			// Проверяем, что название сетевого интерфейса не получено
			ASSERT_TRUE(this->_io->getIface(eid5).empty());
			// Проверяем, что IP-адрес не совпадает с извлечённым ранее
			ASSERT_NE(ip, this->_io->getAddress(eid5, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес не совпадает с извлечённым ранее
			ASSERT_NE(mac, this->_io->getAddress(eid5, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid4).empty());
			// Проверяем, что UDS-адрес установлен и правильный
			ASSERT_EQ("/tmp/awh.sock", this->_io->getAddress(eid5, awh::event::address_t::UDS));
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid6 = this->_io->event(awh::event::node_t::FILE, awh::event::family_t::FSYS);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid6, 0);
			// Устанавливаем порт события
			ASSERT_FALSE(this->_io->setTargetPort(eid6, 8080));
			// Устанавливаем сетевой адрес события
			ASSERT_TRUE(this->_io->setAddress(eid6, awh::event::address_t::FS, "/tmp/awh.txt"));
			// Проверяем, что название сетевого интерфейса не получено
			ASSERT_TRUE(this->_io->getIface(eid6).empty());
			// Проверяем, что IP-адрес не совпадает с извлечённым ранее
			ASSERT_NE(ip, this->_io->getAddress(eid6, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес не совпадает с извлечённым ранее
			ASSERT_NE(mac, this->_io->getAddress(eid6, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid6).empty());
			// Проверяем, что адрес установлен и правильный
			ASSERT_EQ("/tmp/awh.txt", this->_io->getAddress(eid6, awh::event::address_t::FS));
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid7 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid7, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid7, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid7));
			// Устанавливаем IP-адрес назначения для события
			ASSERT_TRUE(this->_io->setTarget(eid7, ip));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid7).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid7));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->getAddress(eid7, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->getAddress(eid7, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен и соответствует
			ASSERT_EQ(ip, this->_io->getTarget(eid7));
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid7, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid8 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::UDS, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid8, 0);
			// Устанавливаем порт события
			ASSERT_FALSE(this->_io->setTargetPort(eid8, 8080));
			// Устанавливаем UDS-адрес назначения для события
			ASSERT_TRUE(this->_io->setTarget(eid8, "/tmp/awh.sock"));
			// Проверяем, что название сетевого интерфейса не получено
			ASSERT_TRUE(this->_io->getIface(eid8).empty());
			// Проверяем, что IP-адрес не совпадает с извлечённым ранее
			ASSERT_NE(ip, this->_io->getAddress(eid8, awh::event::address_t::IPV4));
			// Проверяем, что MAC-адрес не совпадает с извлечённым ранее
			ASSERT_NE(mac, this->_io->getAddress(eid8, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_EQ("/tmp/awh.sock", this->_io->getTarget(eid8));
			// Проверяем, что UDS-адрес установлен и правильный
			ASSERT_EQ("/tmp/awh.sock", this->_io->getAddress(eid8, awh::event::address_t::UDS));
		}
		/**
		 * IPv6 событие
		 */
		{
			/**
			 * @par Намеренные решения
			 *
			 * Проверка ведётся лишь на машине, у которой сеть IPv6 действительно
			 * настроена. Название устройства события выводится из маршрута к
			 * источнику, и там, где маршрута IPv6 по умолчанию нет вовсе, выводить
			 * его не из чего: пустой ответ - правда о такой машине, а не дефект.
			 * Признаком служит опыт - попытка определить источник IPv6, а не
			 * наличие адреса на устройстве: связный адрес есть и у машины без
			 * выхода наружу, а маршрута к ней нет
			 *
			 */
			// Атрибуты источника IPv6 для проверки настроенности сети
			awh::net::src_t probe(std::make_unique <awh::net::addr_net_ipv6_t> ());
			// Создаём объект работы с Ethernet для проверки настроенности сети
			awh::eth_t eth(this->_fmk.get(), this->_log.get());
			// Выполняем извлечение сетевых параметров источника IPv6
			eth.addr.fillSource(probe);
			// Если сеть IPv6 на машине не настроена, проверять нечего
			if(probe.iface.empty())
				// Пропускаем проверку событий IPv6
				GTEST_SKIP() << "IPv6 network is not configured on this machine";
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid1 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid1, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid1, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid1));
			// Устанавливаем сетевой интерфейс события
			ASSERT_TRUE(this->_io->setIface(eid1, source.iface));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid1).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid1));
			// Извлекаем IP-адрес сетевого интерфейса
			ip = this->_io->getAddress(eid1, awh::event::address_t::IPV6);
			// Извлекаем MAC-адрес сетевого интерфейса
			mac = this->_io->getAddress(eid1, awh::event::address_t::MAC);
			// Проверяем, что IP-адрес получен
			ASSERT_FALSE(ip.empty());
			// Проверяем, что MAC-адрес получен
			ASSERT_FALSE(mac.empty());
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid1).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid1, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid2 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid2, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid2, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid2));
			// Устанавливаем MAC-адрес события
			ASSERT_TRUE(this->_io->setAddress(eid2, awh::event::address_t::MAC, mac));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid2).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid2));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->getAddress(eid2, awh::event::address_t::IPV6));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->getAddress(eid2, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid2).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid2, awh::event::address_t::UDS).empty());

			// Добавляем новое событие клиента TCP
			awh::event::id_t eid3 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid3, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid3, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid3));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(eid3, awh::event::address_t::IPV6, ip));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid3).empty());
			// Проверяем, что название сетевого интерфейса совпадает с извлечённым ранее
			ASSERT_EQ(source.iface, this->_io->getIface(eid3));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_EQ(ip, this->_io->getAddress(eid3, awh::event::address_t::IPV6));
			// Проверяем, что MAC-адрес совпадает с извлечённым ранее
			ASSERT_EQ(mac, this->_io->getAddress(eid3, awh::event::address_t::MAC));
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid3).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid3, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid4 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid4, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid4, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid4));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(eid4, awh::event::address_t::NETWORK, ip + "/64"));
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(this->_io->getIface(eid4).empty());
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_FALSE(this->_io->getAddress(eid4, awh::event::address_t::IPV6).empty());
			// Проверяем, что адрес назначения получен
			ASSERT_FALSE(this->_io->getTarget(eid4).empty());
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid4, awh::event::address_t::UDS).empty());
			
			// Добавляем новое событие клиента TCP
			awh::event::id_t eid7 = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV6, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(eid7, 0);
			// Устанавливаем порт события
			ASSERT_TRUE(this->_io->setTargetPort(eid7, 8080));
			// Проверяем что порт получен
			ASSERT_EQ(8080, this->_io->getTargetPort(eid7));
			// Устанавливаем IP-адрес назначения для события
			ASSERT_TRUE(this->_io->setTarget(eid7, ip));
			// Проверяем, что IP-адрес совпадает с извлечённым ранее
			ASSERT_FALSE(this->_io->getAddress(eid4, awh::event::address_t::IPV6).empty());
			// Проверяем, что адрес назначения получен и соответствует
			ASSERT_EQ(ip, this->_io->getTarget(eid7));
			// Проверяем, что UDS-адрес не установлен
			ASSERT_TRUE(this->_io->getAddress(eid7, awh::event::address_t::UDS).empty());
		}
	}
}

/**
 * @brief Тест проверки работы TCP-соединения
 *
 */
TEST_F(IoFixture, IoTCPTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера TCP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
			// Устананавливаем опции события
			ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::AUTO_RECONNECT));
			// Записываем в лог сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
		// Проверяем, что таймаут события на чтение установлен правильно
		ASSERT_EQ(3000, this->_io->getTimeout(events[1], awh::event::action_t::READ));
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
		// Проверяем, что таймаут события на запись установлен правильно
		ASSERT_EQ(3000, this->_io->getTimeout(events[1], awh::event::action_t::WRITE));
		// Устанавливаем режим многократного использования таймаута события на чтение
		this->_io->setUsageReadTimeout(events[1], awh::event::usage_t::REUSABLE);
		// Проверяем, что режим многократного использования таймаута события на чтение установлен правильно
		ASSERT_EQ(awh::event::usage_t::REUSABLE, this->_io->getUsageReadTimeout(events[1]));
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Выполняем прослушивание сервера
		ASSERT_TRUE(this->_io->listen(events[1], 100));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Текст исходящего сообщения
				const std::string message("Hello from async client!");
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, message.c_str(), message.size()))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0]));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(events[0]));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы UDP-событий асинхронного ввода-вывода
 *
 */
TEST_F(IoFixture, IoUDPTest){
	// Флаг остановки теста
	bool stop = false;
	/**
	 * Количество прочитанных сообщений. Счётчик объявлен в области видимости теста:
	 * функция обратного вызова захватывает его по ссылке и вызывается уже из цикла
	 * событий, то есть переживает блок настройки события
	 */
	uint8_t count = 0;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->getAddress(cid, awh::event::address_t::IPV4).c_str(), this->_io->getSourcePort(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&count, &stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Отправляем данные обратно клиенту
			if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
				// Если данные успешно отправлены
				this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			// Останавливаем тест
			stop = (++count >= 10);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(events[0]));
		// Текст исходящего сообщения
		std::string message("Hello from async client message!");
		// Отправляем данные обратно клиенту
		ASSERT_TRUE(this->_io->send(events[0], message.c_str(), message.size()));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы UDP-соединения
 *
 */
TEST_F(IoFixture, IoUDPConnectTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->getAddress(cid, awh::event::address_t::IPV4).c_str(), this->_io->getSourcePort(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Текст исходящего сообщения
				const std::string message("Hello from async client!");
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, message.c_str(), message.size()))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0]));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(events[0]));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы UDS-соединения
 *
 */
TEST_F(IoFixture, IoUDSTest){
	// Флаг остановки теста
	bool stop = false;
	// Добавляем новое событие клиента TCP
	awh::event::id_t cid = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::UDS, awh::event::type_t::STREAM);
	// Добавляем новое событие клиента TCP
	awh::event::id_t sid = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::UDS, awh::event::type_t::STREAM);
	// Проверяем корректность создания событий
	ASSERT_GT(cid, 0);
	ASSERT_GT(sid, 0);
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем опции событий
	ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	ASSERT_TRUE(this->_io->setOptions(sid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->setTarget(cid, "/tmp/awh.sock"));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->setAddress(sid, awh::event::address_t::UDS, "/tmp/awh.sock"));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(sid, static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s", awh::log_t::flag_t::INFO, sid, cid, this->_io->getAddress(cid, awh::event::address_t::UDS).c_str());
			// Устананавливаем опции события
			ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::AUTO_RECONNECT));
			// Записываем в лог сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(sid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(sid, awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(sid));
		// Выполняем прослушивание сервера
		ASSERT_TRUE(this->_io->listen(sid, 100));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(sid));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(cid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(cid, static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Текст исходящего сообщения
				const std::string message("Hello from async client!");
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, message.c_str(), message.size()))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(cid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(cid, awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(cid, awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(cid));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(cid));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(cid));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы UDP UDS-соединения
 *
 */
TEST_F(IoFixture, IoUDPUDSTest){
	// Флаг остановки теста
	bool stop = false;
	// Добавляем новое событие клиента TCP
	awh::event::id_t cid = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::UDS, awh::event::type_t::DATAGRAM);
	// Добавляем новое событие клиента TCP
	awh::event::id_t sid = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::UDS, awh::event::type_t::DATAGRAM);
	// Проверяем корректность создания событий
	ASSERT_GT(cid, 0);
	ASSERT_GT(sid, 0);
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем опции событий
	ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	ASSERT_TRUE(this->_io->setOptions(sid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->setTarget(cid, "/tmp/awh.sock"));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->setAddress(sid, awh::event::address_t::UDS, "/tmp/awh.sock"));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(sid, static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s", awh::log_t::flag_t::INFO, eid, cid, this->_io->getAddress(cid, awh::event::address_t::UDS).c_str());
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(sid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(sid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(sid, awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(sid));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(sid));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(cid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(cid, static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Текст исходящего сообщения
				const std::string message("Hello from async client!");
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, message.c_str(), message.size()))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(cid, awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(cid, awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(cid, awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(cid));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(cid));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(cid));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы широковещательного события
 *
 */
TEST_F(IoFixture, IoBroadcastTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->getAddress(cid, awh::event::address_t::IPV4).c_str(), this->_io->getSourcePort(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::BROADCAST));
		// Создаём объект работы с Ethernet
		awh::eth_t eth(this->_fmk.get(), this->_log.get());
		// Временный объект для извлечения сетевого интерфейса
		awh::net::src_t source(std::make_unique <awh::net::addr_net_ipv4_t> ());
		// Выполняем извлечение сетевых параметров
		eth.addr.fillSource(source);
		// Если сетевой интерфейс не принадлежит к VPN
		if(::memcmp("ut", source.iface.c_str(), 2) != 0){
			// Устанавливаем сетевой интерфейс события
			ASSERT_TRUE(this->_io->setIface(events[0], source.iface));
			// Создаём объект сетевого адреса
			awh::net_addr_t addr(this->_fmk.get(), this->_log.get());
			// Извлекаем IP-адрес сетевого интерфейса
			addr = std::move(this->_io->getAddress(events[0], awh::event::address_t::IPV4));
			// Если IP-адрес не принадлежит к LAN
			if(addr.own() != awh::net_addr_t::own_t::LAN){
				// Уничтожаем все события после получения ответа
				ASSERT_TRUE(this->_io->deinitialize());
				// Выходиим из функции, так как IP-адрес не подходит для тестирования
				return;
			}
			// Проверяем, что название сетевого интерфейса получено
			ASSERT_FALSE(source.iface.empty());
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Формируем адрес Broadcast
			addr.v4((addr.v4(awh::net_addr_t::endian_t::BIG) & 0xFFFFFF00U) | 0x000000FFU, awh::net_addr_t::endian_t::BIG);
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setTarget(events[0], static_cast <std::string> (addr)));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Останавливаем тест
				stop = true;
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Запускаем событие клиента
			ASSERT_TRUE(this->_io->launch(events[0]));
			// Текст исходящего сообщения
			const std::string message("Hello from async client!");
			// Отправляем данные обратно клиенту
			ASSERT_TRUE(this->_io->send(events[0], message.c_str(), message.size()));
		// Если сетевой интерфейс принадлежит к VPN
		} else {
			// Записываем в лог сообщение о пропуске теста для VPN-интерфейса
			this->_log->print("Пропуск теста для VPN-интерфейса: %s", awh::log_t::flag_t::WARNING, source.iface.c_str());
			// Устанавливаем флаг остановки теста
			stop = true;
		}
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы UDP-соединения с перехватом данных из файла
 *
 */
TEST_F(IoFixture, IoUDPSpliceConnectTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие отслеживания файла
	awh::event::id_t fid = this->_io->event(awh::event::node_t::FILE, awh::event::family_t::FSYS);
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++){
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
		// Устанавливаем размер буфера для чтения и записи
		this->_io->setBufferSize(events[i], awh::event::action_t::READ, 10240);
		this->_io->setBufferSize(events[i], awh::event::action_t::WRITE, 10240);
	}
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем адрес текстового файла для чтения
	ASSERT_TRUE(this->_io->setAddress(fid, awh::event::address_t::FS, "../README.md"));
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->getAddress(cid, awh::event::address_t::IPV4).c_str(), this->_io->getSourcePort(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Отправляем данные обратно клиенту
				if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
					// Если данные успешно отправлены
					this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
		// Выполняем объединение двух сокетов
		ASSERT_TRUE(this->_io->splice(fid, events[0]));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Для всех прочих ошибок
				default:
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Код=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, static_cast <uint8_t> (error), description.c_str());
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([fid, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Выполняем фиксацию события файла
				ASSERT_TRUE(this->_io->commit(fid));
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->setOptions(fid, awh::event::options::AUTO_FOLLOW));
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0]));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(events[0]));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы файловой системой I/O событий
 *
 */
TEST_F(IoFixture, IoFsTest){
	// Флаг остановки теста
	bool stop = false;
	// Добавляем новое событие отслеживания каталога
	awh::event::id_t did = this->_io->event(awh::event::node_t::DIR, awh::event::family_t::FSYS);
	// Добавляем новое событие отслеживания файла
	awh::event::id_t fid = this->_io->event(awh::event::node_t::FILE, awh::event::family_t::FSYS);
	// Проверяем идентификаторы созданных событий
	ASSERT_GT(did, 0);
	ASSERT_GT(fid, 0);
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Событие каталога
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(did, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(did, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на изменение события
		this->_io->on(did, [this](const awh::event::id_t eid, const awh::event::action_t action, const awh::event::vnode_t vnode, const std::string & path) noexcept -> void {
			/**
			 * Обрабатываем тип узла события
			 */
			switch(static_cast <uint8_t> (vnode)){
				// Если тип узла не определён
				case static_cast <uint8_t> (awh::event::vnode_t::NONE):
					// Записываем в лог сообщение о типе узла события
					this->_log->print("Тип узла события: Не определён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
				break;
				case static_cast <uint8_t> (awh::event::vnode_t::CHR): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение о изменении события
							this->_log->print("Тип узла события: Символьный узел устройства добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							this->_log->print("Тип узла события: Символьный узел устройства удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				case static_cast <uint8_t> (awh::event::vnode_t::BLK): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение о изменении события
							this->_log->print("Тип узла события: Блочный узел устройства добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							this->_log->print("Тип узла события: Блочный узел устройства удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является каналом FIFO
				case static_cast <uint8_t> (awh::event::vnode_t::FIFO): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение о изменении события
							this->_log->print("Тип узла события: Канал FIFO добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							this->_log->print("Тип узла события: Канал FIFO удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является сокетом
				case static_cast <uint8_t> (awh::event::vnode_t::SOCK): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение о изменении события
							this->_log->print("Тип узла события: Сокет добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							this->_log->print("Тип узла события: Сокет удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является файлом
				case static_cast <uint8_t> (awh::event::vnode_t::FILE): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение о изменении события
							this->_log->print("Тип узла события: Файл добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							this->_log->print("Тип узла события: Файл удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является каталогом
				case static_cast <uint8_t> (awh::event::vnode_t::DIR): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение о изменении события
							this->_log->print("Тип узла события: Каталог добавлен, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение о типе узла события
							this->_log->print("Тип узла события: Каталог удалён, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
				// Если тип узла является символической ссылкой
				case static_cast <uint8_t> (awh::event::vnode_t::LINK): {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение о изменении события
							this->_log->print("Тип узла события: Символическая ссылка добавлена, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение о типе узла события
							this->_log->print("Тип узла события: Символическая ссылка удалена, Путь=%s", awh::log_t::flag_t::INFO, path.c_str());
						break;
					}
				} break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(did, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем путь к отслеживаемому каталогу
		ASSERT_TRUE(this->_io->setAddress(did, awh::event::address_t::FS, "./"));
		// Выполняем фиксацию настроек события каталога
		ASSERT_TRUE(this->_io->commit(did));
		// Запускаем событие каталога
		ASSERT_TRUE(this->_io->launch(did));
	}
	/**
	 * Событие файла
	 */
	{
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(fid, [&stop, this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING): {
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					// Устанавливаем смещение в файле
					// this->_io->getSeek(eid, 1024);
					// Отправляем тестовое сообщение в файл
					this->_io->send(eid, "Hello World!!!", 14);
					// Останавливаем тест
					stop = true;
				} break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(fid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(fid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Удаляем файл
				::remove("tmp.txt");
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#else
				// Удаляем файл
				::unlink("./tmp.txt");
			#endif
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(fid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(did, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем путь к отслеживаемому файлу
		ASSERT_TRUE(this->_io->setAddress(fid, awh::event::address_t::FS, "./tmp.txt"));
		// Выполняем фиксацию настроек события файла
		ASSERT_TRUE(this->_io->commit(fid));
		// Устанавливаем опции события
		ASSERT_TRUE(this->_io->setOptions(fid, awh::event::options::AUTO_FOLLOW));
		// Запускаем событие файла
		ASSERT_TRUE(this->_io->launch(fid));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы пользовательских событий
 *
 */
TEST_F(IoFixture, IoEventsTest){
	// Флаг остановки теста
	bool stop = false;
	// Добавляем новое событие отслеживания каталога
	awh::event::id_t eid = this->_io->event(awh::event::node_t::NOTIFY, awh::event::family_t::USER);
	// Проверяем инициализацию
	ASSERT_GT(eid, 0);
	// Проверяем, что идентификатор события больше нуля
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем функцию обратного вызова на событие таймера
	this->_io->on(eid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		/**
		 * Обрабатываем статус события
		 */
		switch(static_cast <uint8_t> (status)){
			// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
		}
	});
	// Устанавливаем функцию обратного вызова на запись в событие
	this->_io->on(eid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
		// Записываем в лог сообщение о переподключении события
		this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
	}));
	// Устанавливаем функцию обратного вызова на чтение из события
	this->_io->on(eid, [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
		// Текст входящего сообщения
		const std::string message(reinterpret_cast <const char *> (data), size);
		// Записываем в лог сообщение о переподключении события
		this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
		// Останавливаем тест
		stop = true;
	});
	// Устанавливаем функцию обратного вызова на ошибку события
	this->_io->on(eid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
		/**
		 * Обрабатываем статус события
		 */
		switch(static_cast <uint8_t> (error)){
			// Если ошибка неизвестного события
			case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
				// Записываем ошибку в лог неизвестного события
				this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка недопустимой операции
			case static_cast <uint8_t> (awh::event::error_t::INVALID):
				// Записываем ошибку в лог недопустимой операции
				this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка доступа запрещёния
			case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
				// Записываем ошибку в лог доступа запрещёния
				this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка уже существующего объекта
			case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
				// Записываем ошибку в лог уже существующего объекта
				this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка доступа к сокету
			case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
				// Записываем ошибку в лог доступа к сокету
				this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка некорректного адреса
			case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
				// Записываем ошибку в лог некорректного адреса
				this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка ошибки подключения
			case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
				// Записываем ошибку в лог подключения
				this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка недостаточно ресурсов
			case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
				// Записываем ошибку в лог недостаточно ресурсов
				this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка события
			case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
				// Записываем ошибку в лог события
				this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если объект не найден
			case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
				// Записываем ошибку в лог события
				this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
		}
	});
	// Устанавливаем функцию обратного вызова на общее событие
	this->_io->on(eid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
		/**
		 * Обрабатываем действие события
		 */
		switch(static_cast <uint8_t> (action)){
			// Если действие является чтением
			case static_cast <uint8_t> (awh::event::action_t::READ):
				// Записываем в лог сообщение о чтении события
				this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является записью
			case static_cast <uint8_t> (awh::event::action_t::WRITE):
				// Записываем в лог сообщение о записи события
				this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является подключением
			case static_cast <uint8_t> (awh::event::action_t::CONNECT):
				// Записываем в лог сообщение о подключении события
				this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является отключением
			case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
				// Записываем в лог сообщение об отключении события
				this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является переподключением
			case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является закрытием
			case static_cast <uint8_t> (awh::event::action_t::CLOSE):
				// Записываем в лог сообщение о закрытии события
				this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением
			case static_cast <uint8_t> (awh::event::action_t::CHANGE):
				// Записываем в лог сообщение об изменении события
				this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является удалением
			case static_cast <uint8_t> (awh::event::action_t::DELETE):
				// Записываем в лог сообщение об удалении события
				this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является переименованием
			case static_cast <uint8_t> (awh::event::action_t::RENAME):
				// Записываем в лог сообщение о переименовании события
				this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением атрибутов
			case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
				// Записываем в лог сообщение об изменении атрибутов события
				this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является отзывом доступа
			case static_cast <uint8_t> (awh::event::action_t::REVOKE):
				// Записываем в лог сообщение об отзыве доступа события
				this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением счётчика жёстких ссылок
			case static_cast <uint8_t> (awh::event::action_t::HDLINK):
				// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
				this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
		}
	});
	// Выполняем фиксацию настроек события сервера
	ASSERT_TRUE(this->_io->commit(eid));
	// Запускаем событие пользователя
	ASSERT_TRUE(this->_io->launch(eid));
	/**
	 * @par Намеренные решения
	 *
	 * Поток присоединяется, а не отсоединяется. Он обращается к полям
	 * приспособления - объекту журнала и движку, - а живут они ровно столько,
	 * сколько сама проверка. Отсоединённый поток переживал её и продолжал
	 * работать по уже разрушенным объектам: на NetBSD это давало падение с
	 * дампом внутри записи в журнал, а на macOS проходило незамеченным, потому
	 * что освобождённая память там оставалась отображённой. Падение к тому же
	 * возникало не при одиночном запуске, а лишь в общем прогоне - разрушение
	 * приспособления и работа потока сходились по времени по-разному
	 *
	 */
	// Запускаем дочерний поток для уведомления события
	std::thread notifier([this](const awh::event::id_t eid) noexcept -> void {
		// Текст сообщения
		const std::string message = "Hello AWH IO Event!";
		// Уведомляем событие
		this->_io->send(eid, reinterpret_cast <const char *> (message.c_str()), message.length());
	}, eid);
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Дожидаемся завершения дочернего потока уведомления события
	if(notifier.joinable())
		// Выполняем присоединение дочернего потока
		notifier.join();
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы многоадресной передачи UDP
 *
 * @note Тест отключён: многоадресная передача требует, чтобы её пропускал
 *       сетевой интерфейс, а через VPN она не проходит вовсе, поэтому
 *       результат прогона зависит от того, где он выполняется, а не от кода
 */
TEST_F(IoFixture, DISABLED_IoMulticast1Test){
	/**
	 * 1. Peer-to-peer discovery (mDNS, SSDP)
	 * Клиент отправляет запрос в мультикаст (224.0.0.251:5353 для mDNS),
	 * Серверы слушают этот адрес и отвечают либо в мультикаст, либо напрямую клиенту.
	 */
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем TTL для мультикастового события
		ASSERT_TRUE(this->_io->setHops(events[1], awh::event::hops_t::NETWORK));
		// Устанавливаем мультикастовый режим события
		ASSERT_TRUE(this->_io->setDelivery(events[1], awh::event::delivery_mode_t::MULTICAST));
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::MULTICAST_LOOPBACK));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "239.255.1.1"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->membership(events[1], awh::event::mode_t::ENABLED, "239.255.1.1", "0.0.0.0"));
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->getAddress(cid, awh::event::address_t::IPV4).c_str(), this->_io->getSourcePort(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Флаг отправки сообщений
			static bool sending = false;
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [eid, this](const awh::event::id_t cid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, cid, size, message.c_str());
				// Если сообщение ещё не отправлено
				if(!sending){
					// Устанавливаем флаг отправки сообщения
					sending = !sending;
					// Помечаем сообщение
					message.append("1");
					// Отправляем данные обратно клиенту
					if(this->_io->send(eid, message.c_str(), message.length()))
						// Если данные успешно отправлены
						this->_log->print("Отправлено в группу: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.length());
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки в группу: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					// Помечаем сообщение
					message.append("2");
					// Отправляем данные обратно клиенту
					if(this->_io->send(cid, message.c_str(), message.length()))
						// Если данные успешно отправлены
						this->_log->print("Отправлено клиенту: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, message.length());
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки клиенту: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
				}
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[1], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Выполняем запуск события
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем количество хопов события
		ASSERT_TRUE(this->_io->setHops(events[0], awh::event::hops_t::NETWORK));
		// Устанавливаем мультикастовый режим события
		ASSERT_TRUE(this->_io->setDelivery(events[0], awh::event::delivery_mode_t::MULTICAST));
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::MULTICAST_LOOPBACK));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "239.255.1.1"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->membership(events[0], awh::event::mode_t::ENABLED, "239.255.1.1", "0.0.0.0"));
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Останавливаем тест
			stop = true;
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Выполняем запуск события
		ASSERT_TRUE(this->_io->launch(events[0]));
		// Формируем отправляемое сообщение
		const std::string & message = "Hello World!!!";
		// Отправляем данные обратно клиенту
		ASSERT_TRUE(this->_io->send(events[0], message.c_str(), message.length()));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы многоадресной передачи UDP
 *
 * @note Тест отключён: многоадресная передача требует, чтобы её пропускал
 *       сетевой интерфейс, а через VPN она не проходит вовсе, поэтому
 *       результат прогона зависит от того, где он выполняется, а не от кода
 */
TEST_F(IoFixture, DISABLED_IoMulticast3Test){
	/**
	 * 3. Сервер-обнаружение
	 * Клиенты слушают 239.255.1.2,
	 * Сервер периодически рассылает "я здесь",
	 * Клиенты отвечают прямо серверу (unicast), чтобы установить соединение.
	 */
	// Флаг остановки теста
	bool stop = false;
	/**
	 * Количество прочитанных сообщений. Счётчик объявлен в области видимости теста:
	 * функция обратного вызова захватывает его по ссылке и вызывается уже из цикла
	 * событий, то есть переживает блок настройки события
	 */
	uint8_t count = 0;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера UDP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Серверное событие
	 */
	{
		// Добавляем новое событие интервала
		awh::event::id_t tid = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(tid, 0);
		// Добавляем новое событие интервала
		this->_io->setTimeout(tid, awh::event::action_t::NONE, 5000);
		// Выполняем фиксацию настроек события интервала
		ASSERT_TRUE(this->_io->commit(tid));
		// Устанавливаем функцию обратного вызова на событие интервала
		this->_io->on(tid, [&events, this](const awh::event::id_t tid, const awh::event::status_t status) noexcept -> void {
			// Количество отправленных сообщений
			static uint8_t counter = 0;
			// Формируем отправляемое сообщение
			const std::string & message = this->_fmk->format("Message #%d", ++counter);
			// Выполняем отправку сообщения в мультикастовую группу
			if(this->_io->send(events[1], message.c_str(), message.size()))
				// Записываем ошибку в лог отправки сообщения
				this->_log->print("Сообщение отправлено: ID=%u, %s", awh::log_t::flag_t::INFO, events[1], message.c_str());
		});
		// Устанавливаем TTL для мультикастового события
		ASSERT_TRUE(this->_io->setHops(events[1], awh::event::hops_t::NETWORK));
		// Устанавливаем мультикастовый режим события
		ASSERT_TRUE(this->_io->setDelivery(events[1], awh::event::delivery_mode_t::MULTICAST));
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::MULTICAST_LOOPBACK));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "239.255.1.1"));
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на подключение нового клиента
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t eid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u, ADDR=%s:%d", awh::log_t::flag_t::INFO, eid, cid, this->_io->getAddress(cid, awh::event::address_t::IPV4).c_str(), this->_io->getSourcePort(cid));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[1], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Выполняем запуск работы таймера
		ASSERT_TRUE(this->_io->launch(tid));
		// Выполняем запуск работы сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Устанавливаем мультикастовый режим события
		ASSERT_TRUE(this->_io->setDelivery(events[0], awh::event::delivery_mode_t::MULTICAST));
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->membership(events[0], awh::event::mode_t::ENABLED, "239.255.1.1", "0.0.0.0", port));
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [&count, &stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Текст входящего сообщения
			const std::string message(reinterpret_cast <const char *> (data), size);
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
			// Отправляем данные обратно клиенту
			if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
				// Если данные успешно отправлены
				this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			// Останавливаем тест
			stop = (++count >= 3);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Выполняем запуск события
		ASSERT_TRUE(this->_io->launch(events[0]));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы TLS-соединения
 *
 */
TEST_F(IoFixture, IoTLSTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера TCP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Регистрируем объект транспортного уровня безопасности
		awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::SERVER, awh::event::protocol_t::TCP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts, 0);
		// Устанавливаем ALPN протоколы TLS
		this->_coder->alpn(cts, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
		// Устанавливаем файл центра сертификации TLS
		this->_coder->ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		this->_coder->validateServerNameIndication(cts, false);
		// Устанавливаем клиентский сертификат TLS
		this->_coder->certificate(cts, "../sh/certificates/server/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_coder->privateKey(cts, "../sh/certificates/server/key.pem");
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_coder->on(cts, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
			// Записываем в лог сообщение о предупреждающей ошибке TLS
			this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
		});
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([cts, this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
			// Создаём идентификатор транспортного уровня TLS
			awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(ctl, 0);
			// Регистрируем функцию обратного вызова на получение ошибок TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
				// Записываем в лог сообщение о предупреждающей ошибке TLS
				this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
			});
			// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
				/**
				 * Обрабатываем входящие состояния TLS
				 */
				switch(static_cast <uint8_t> (state)){
					// Если состояние ошибки транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
						// Записываем ошибку в лог транспортного уровня TLS
						this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					break;
					// Если состояние уничтожения объекта транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
						// Записываем в лог сообщение об успешном удалении контекста TLS
						this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
					break;
					// Если состояние рукопожатия успешно завершено
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
						// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << this->_coder->serverNameIndication(id) << std::endl << std::endl;
						std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
						std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
						std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
						std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
						std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
						std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
						// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						this->_log->print("Рукопожатие TLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", awh::log_t::flag_t::INFO, id, this->_coder->alpn(id));
					} break;
				}
			});
			// Регистрируем функцию обратного вызова на запись данных TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
						// Записываем в лог сообщение о записи зашифрованных данных TLS
						this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
						// Записываем в лог сообщение о записи дешифрованных данных TLS
						this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
				}
			});
			// Устананавливаем опции события
			ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::AUTO_RECONNECT));
			// Записываем в лог сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем клиента TLS для события
			this->_coder->peer(ctl, this->_io->getAddress(cid, awh::event::address_t::IPV4), this->_io->getSourcePort(cid));
			// Регистрируем функцию обратного вызова на чтение данных TLS
			this->_coder->on(ctl, [cid, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
						// Отправляем данные обратно клиенту
						if(this->_io->send(cid, reinterpret_cast <const char *> (buffer), size))
							// Если данные успешно отправлены
							this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, size);
						// Если данные не отправлены
						else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
					} break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
						// Получаем ответ сервера в расшифрованном виде
						const std::string response(reinterpret_cast <const char *> (buffer), size);
						// Записываем в лог сообщение полученных данных с сервера
						this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
						// Если данные успешно зашифрованы TLS
						if(this->_coder->encrypt(id, response.c_str(), response.size()))
							// Записываем в лог сообщение об успешном шифровании данных TLS
							this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, response.size());
						// Если данные не отправлены
						else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Если данные успешно дешифрованы TLS
				if(this->_coder->decrypt(ctl, data, size))
					// Записываем в лог сообщение об успешном дешифровании данных TLS
					this->_log->print("Успешно дешифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Выполняем прослушивание сервера
		ASSERT_TRUE(this->_io->listen(events[1], 100));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Регистрируем объект транспортного уровня безопасности
		awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::CLIENT, awh::event::protocol_t::TCP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts, 0);
		// Устанавливаем ALPN протоколы TLS
		this->_coder->alpn(cts, {{0,"http/1.1"}});
		// this->_coder->alpn(cts, {{0,"http/1.1"},{2,"h3"}});
		// Устанавливаем файл центра сертификации TLS
		this->_coder->ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		this->_coder->validateServerNameIndication(cts, false);
		// Устанавливаем имя хоста TLS
		this->_coder->serverNameIndication(cts, "anyks.com");
		// Устанавливаем клиентский сертификат TLS
		this->_coder->certificate(cts, "../sh/certificates/client/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_coder->privateKey(cts, "../sh/certificates/client/key.pem");
		// Создаём идентификатор транспортного уровня TLS
		awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(ctl, 0);
		// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
			/**
			 * Обрабатываем входящие состояния TLS
			 */
			switch(static_cast <uint8_t> (state)){
				// Если состояние ошибки транспортного уровня
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
					// Записываем ошибку в лог транспортного уровня TLS
					this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				break;
				// Если состояние уничтожения объекта транспортного уровня
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
					// Записываем в лог сообщение об успешном удалении контекста TLS
					this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
				break;
				// Если состояние рукопожатия успешно завершено
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
					// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
					std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
					std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
					std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
					std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
					std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
					std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
					// Возвращаем данные сертификата TLS
					std::cout << "Certificate data:\n" << this->_coder->certificateExtract(id) << std::endl << std::endl;
					// Записываем в лог информацию о TLS соединении
					std::cout << this->_coder->peerInfo(id) << std::endl;
					// Текст запроса к серверу
					const std::string request =
						"GET / HTTP/1.1\r\n"
						"Host: www.google.com\r\n"
						"Connection: close\r\n"
						"User-Agent: iouring-openssl-sample/1.0\r\n"
						"\r\n";
					// Если данные успешно зашифрованы TLS
					if(this->_coder->encrypt(id, request.c_str(), request.size()))
						// Записываем в лог сообщение об успешном шифровании данных TLS
						this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, request.size());
					// Если данные не отправлены
					else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				} break;
			}
		});
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
			// Записываем в лог сообщение о предупреждающей ошибке TLS
			this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
		});
		// Регистрируем функцию обратного вызова на запись данных TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
					// Записываем в лог сообщение о записи зашифрованных данных TLS
					this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
					// Записываем в лог сообщение о записи дешифрованных данных TLS
					this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
			}
		});
		// Регистрируем функцию обратного вызова на чтение данных TLS
		this->_coder->on(ctl, [&events, &stop, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
					// Отправляем данные обратно клиенту
					if(this->_io->send(events[0], reinterpret_cast <const char *> (buffer), size))
						// Если данные успешно отправлены
						this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, events[0], size);
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, events[0]);
				} break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
					// Получаем ответ сервера в расшифрованном виде
					const std::string response(reinterpret_cast <const char *> (buffer), size);
					// Записываем в лог сообщение полученных данных с сервера
					this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
					// Устанавливаем флаг завершения работы
					stop = true;
				} break;
			}
		});
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Если данные успешно дешифрованы TLS
			if(this->_coder->decrypt(ctl, data, size))
				// Записываем в лог сообщение об успешном дешифровании данных TLS
				this->_log->print("Успешно дешифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([ctl, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Если рукопожатие TLS успешно
				if(this->_coder->handshake(ctl))
					// Записываем в лог сообщение о начале рукопожатия TLS
					this->_log->print("Начинаем процесс рукопожатия: ID=%u", awh::log_t::flag_t::INFO, ctl);
				// Если рукопожатие TLS не выполнено
				else this->_log->print("Ошибка рукопожатия TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, ctl);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0]));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(events[0]));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы мульти TLS-соединения
 *
 */
TEST_F(IoFixture, IoMultiTLSTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера TCP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Регистрируем объекты транспортного уровня безопасности
		awh::tls::coder_t::id_t cts1 = this->_coder->context(awh::event::node_t::SERVER, awh::event::protocol_t::TCP);
		awh::tls::coder_t::id_t cts2 = this->_coder->context(awh::event::node_t::SERVER, awh::event::protocol_t::TCP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts1, 0);
		ASSERT_GT(cts2, 0);
		// Устанавливаем режим работы TLS
		this->_coder->mode(cts1, awh::tls::coder_t::mode_t::MULTICERT);
		this->_coder->mode(cts2, awh::tls::coder_t::mode_t::MULTICERT);
		// Включаем проверку имени хоста TLS
		this->_coder->validateServerNameIndication(cts1, false);
		this->_coder->validateServerNameIndication(cts2, false);
		// Устанавливаем ALPN протоколы TLS
		this->_coder->alpn(cts1, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
		this->_coder->alpn(cts2, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
		// Устанавливаем файл центра сертификации TLS
		this->_coder->ca(cts1, "../sh/certificates", "ca.pem");
		this->_coder->ca(cts2, "../sh/certificates", "ca.pem");
		// Устанавливаем клиентский сертификат TLS
		this->_coder->certificate(cts1, "../sh/certificates/example/cert.pem");
		this->_coder->certificate(cts2, "../sh/certificates/server/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_coder->privateKey(cts1, "../sh/certificates/example/key.pem");
		this->_coder->privateKey(cts2, "../sh/certificates/server/key.pem");
		// Устанавливаем имя хоста TLS (Указывать нужно после установки режима работы мультисертификатного TLS!!!!!!!)
		this->_coder->serverNameIndication(cts2, "anyks.com");
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_coder->on(cts1, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
			// Записываем в лог сообщение о предупреждающей ошибке TLS
			this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
		});
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_coder->on(cts2, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
			// Записываем в лог сообщение о предупреждающей ошибке TLS
			this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
		});
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([cts1, this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
			// Создаём идентификатор транспортного уровня TLS
			awh::tls::coder_t::id_t ctl = this->_coder->transport(cts1);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(ctl, 0);
			// Регистрируем функцию обратного вызова на получение ошибок TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
				// Записываем в лог сообщение о предупреждающей ошибке TLS
				this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
			});
			// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
				/**
				 * Обрабатываем входящие состояния TLS
				 */
				switch(static_cast <uint8_t> (state)){
					// Если состояние ошибки транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
						// Записываем ошибку в лог транспортного уровня TLS
						this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					break;
					// Если состояние уничтожения объекта транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
						// Записываем в лог сообщение об успешном удалении контекста TLS
						this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
					break;
					// Если состояние рукопожатия успешно завершено
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
						// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << this->_coder->serverNameIndication(id) << std::endl << std::endl;
						std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
						std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
						std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
						std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
						std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
						std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
						// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						this->_log->print("Рукопожатие TLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", awh::log_t::flag_t::INFO, id, this->_coder->alpn(id));
					} break;
				}
			});
			// Регистрируем функцию обратного вызова на запись данных TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
						// Записываем в лог сообщение о записи зашифрованных данных TLS
						this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
						// Записываем в лог сообщение о записи дешифрованных данных TLS
						this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
				}
			});
			// Устананавливаем опции события
			ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::AUTO_RECONNECT));
			// Записываем в лог сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем клиента TLS для события
			this->_coder->peer(ctl, this->_io->getAddress(cid, awh::event::address_t::IPV4), this->_io->getSourcePort(cid));
			// Регистрируем функцию обратного вызова на чтение данных TLS
			this->_coder->on(ctl, [cid, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
						// Отправляем данные обратно клиенту
						if(this->_io->send(cid, reinterpret_cast <const char *> (buffer), size))
							// Если данные успешно отправлены
							this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, size);
						// Если данные не отправлены
						else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
					} break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
						// Получаем ответ сервера в расшифрованном виде
						const std::string response(reinterpret_cast <const char *> (buffer), size);
						// Записываем в лог сообщение полученных данных с сервера
						this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
						// Если данные успешно зашифрованы TLS
						if(this->_coder->encrypt(id, response.c_str(), response.size()))
							// Записываем в лог сообщение об успешном шифровании данных TLS
							this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, response.size());
						// Если данные не отправлены
						else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Если данные успешно дешифрованы TLS
				if(this->_coder->decrypt(ctl, data, size))
					// Записываем в лог сообщение об успешном дешифровании данных TLS
					this->_log->print("Успешно дешифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Выполняем прослушивание сервера
		ASSERT_TRUE(this->_io->listen(events[1], 100));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Регистрируем объект транспортного уровня безопасности
		awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::CLIENT, awh::event::protocol_t::TCP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts, 0);
		// Устанавливаем ALPN протоколы TLS
		this->_coder->alpn(cts, {{0,"http/1.1"}});
		// this->_coder->alpn(cts, {{0,"http/1.1"},{2,"h3"}});
		// Устанавливаем файл центра сертификации TLS
		this->_coder->ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		this->_coder->validateServerNameIndication(cts, false);
		// Устанавливаем имя хоста TLS
		this->_coder->serverNameIndication(cts, "anyks.com");
		// Устанавливаем клиентский сертификат TLS
		this->_coder->certificate(cts, "../sh/certificates/client/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_coder->privateKey(cts, "../sh/certificates/client/key.pem");
		// Создаём идентификатор транспортного уровня TLS
		awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(ctl, 0);
		// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
			/**
			 * Обрабатываем входящие состояния TLS
			 */
			switch(static_cast <uint8_t> (state)){
				// Если состояние ошибки транспортного уровня
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
					// Записываем ошибку в лог транспортного уровня TLS
					this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				break;
				// Если состояние уничтожения объекта транспортного уровня
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
					// Записываем в лог сообщение об успешном удалении контекста TLS
					this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
				break;
				// Если состояние рукопожатия успешно завершено
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
					// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
					std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
					std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
					std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
					std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
					std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
					std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
					// Возвращаем данные сертификата TLS
					std::cout << "Certificate data:\n" << this->_coder->certificateExtract(id) << std::endl << std::endl;
					// Записываем в лог информацию о TLS соединении
					std::cout << this->_coder->peerInfo(id) << std::endl;
					// Текст запроса к серверу
					const std::string request =
						"GET / HTTP/1.1\r\n"
						"Host: www.google.com\r\n"
						"Connection: close\r\n"
						"User-Agent: iouring-openssl-sample/1.0\r\n"
						"\r\n";
					// Если данные успешно зашифрованы TLS
					if(this->_coder->encrypt(id, request.c_str(), request.size()))
						// Записываем в лог сообщение об успешном шифровании данных TLS
						this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, request.size());
					// Если данные не отправлены
					else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				} break;
			}
		});
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
			// Записываем в лог сообщение о предупреждающей ошибке TLS
			this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
		});
		// Регистрируем функцию обратного вызова на запись данных TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
					// Записываем в лог сообщение о записи зашифрованных данных TLS
					this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
					// Записываем в лог сообщение о записи дешифрованных данных TLS
					this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
			}
		});
		// Регистрируем функцию обратного вызова на чтение данных TLS
		this->_coder->on(ctl, [&events, &stop, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
					// Отправляем данные обратно клиенту
					if(this->_io->send(events[0], reinterpret_cast <const char *> (buffer), size))
						// Если данные успешно отправлены
						this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, events[0], size);
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, events[0]);
				} break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
					// Получаем ответ сервера в расшифрованном виде
					const std::string response(reinterpret_cast <const char *> (buffer), size);
					// Записываем в лог сообщение полученных данных с сервера
					this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
					// Устанавливаем флаг завершения работы
					stop = true;
				} break;
			}
		});
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Если данные успешно дешифрованы TLS
			if(this->_coder->decrypt(ctl, data, size))
				// Записываем в лог сообщение об успешном дешифровании данных TLS
				this->_log->print("Успешно дешифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([ctl, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Если рукопожатие TLS успешно
				if(this->_coder->handshake(ctl))
					// Записываем в лог сообщение о начале рукопожатия TLS
					this->_log->print("Начинаем процесс рукопожатия: ID=%u", awh::log_t::flag_t::INFO, ctl);
				// Если рукопожатие TLS не выполнено
				else this->_log->print("Ошибка рукопожатия TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, ctl);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0]));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(events[0]));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест проверки работы TLS-соединения
 *
 */
TEST_F(IoFixture, IoDTLSTest){
	// Флаг остановки теста
	bool stop = false;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем новое событие клиента и сервера TCP
	const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM));
	/**
	 * Проверяем, что оба идентификатора события созданы успешно
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Проверяем, что идентификатор события больше нуля
		ASSERT_GT(events[i], 0);
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
	// Устанавливаем порт события
	ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
	// Проверяем что порт получен
	ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выставляем опции и параметры для каждого события
	 */
	for(uint8_t i = 0; i < 2; i++)
		// Устанавливаем опции событий
		ASSERT_TRUE(this->_io->setOptions(events[i], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
	/**
	 * Серверное событие
	 */
	{
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
		// Регистрируем объект транспортного уровня безопасности
		awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::SERVER, awh::event::protocol_t::UDP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts, 0);
		// Устанавливаем ALPN протоколы TLS
		this->_coder->alpn(cts, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
		// Устанавливаем файл центра сертификации TLS
		this->_coder->ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		this->_coder->validateServerNameIndication(cts, false);
		// Устанавливаем клиентский сертификат TLS
		this->_coder->certificate(cts, "../sh/certificates/server/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_coder->privateKey(cts, "../sh/certificates/server/key.pem");
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_coder->on(cts, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
			// Записываем в лог сообщение о предупреждающей ошибке TLS
			this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
		});
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на принятие события
		this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([cts, this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
			// Создаём идентификатор транспортного уровня TLS
			awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(ctl, 0);
			// Регистрируем функцию обратного вызова на получение ошибок TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
				// Записываем в лог сообщение о предупреждающей ошибке TLS
				this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
			});
			// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
				/**
				 * Обрабатываем входящие состояния TLS
				 */
				switch(static_cast <uint8_t> (state)){
					// Если состояние ошибки транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
						// Записываем ошибку в лог транспортного уровня TLS
						this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					break;
					// Если состояние уничтожения объекта транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
						// Записываем в лог сообщение об успешном удалении контекста TLS
						this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
					break;
					// Если состояние рукопожатия успешно завершено
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
						// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << this->_coder->serverNameIndication(id) << std::endl << std::endl;
						std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
						std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
						std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
						std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
						std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
						std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
						// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
						this->_log->print("Рукопожатие TLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", awh::log_t::flag_t::INFO, id, this->_coder->alpn(id));
						// Записываем в лог информацию о DTLS соединении
						std::cout << this->_coder->peerInfo(id) << std::endl;
						// Выполняем повторную передачу данных TLS
						ASSERT_TRUE(this->_coder->retransmit(id));
					} break;
				}
			});
			// Регистрируем функцию обратного вызова на запись данных TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
						// Записываем в лог сообщение о записи зашифрованных данных TLS
						this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
						// Записываем в лог сообщение о записи дешифрованных данных TLS
						this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
				}
			});
			// Записываем в лог сообщение об успешной установке опций события
			this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
			// Устанавливаем клиента TLS для события
			this->_coder->peer(ctl, this->_io->getAddress(cid, awh::event::address_t::IPV4), this->_io->getSourcePort(cid));
			// Регистрируем функцию обратного вызова на чтение данных TLS
			this->_coder->on(ctl, [cid, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события TLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
						// Отправляем данные обратно клиенту
						if(this->_io->send(cid, reinterpret_cast <const char *> (buffer), size))
							// Если данные успешно отправлены
							this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, size);
						// Если данные не отправлены
						else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
					} break;
					// Если событие дешифрования данных TLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
						// Получаем ответ сервера в расшифрованном виде
						const std::string response(reinterpret_cast <const char *> (buffer), size);
						// Записываем в лог сообщение полученных данных с сервера
						this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
						// Если данные успешно зашифрованы TLS
						if(this->_coder->encrypt(id, response.c_str(), response.size()))
							// Записываем в лог сообщение об успешном шифровании данных TLS
							this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, response.size());
						// Если данные не отправлены
						else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(cid, [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Если данные успешно дешифрованы TLS
				if(this->_coder->decrypt(ctl, data, size))
					// Записываем в лог сообщение об успешном дешифровании данных TLS
					this->_log->print("Успешно дешифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(events[1]));
		// Запускаем событие сервера
		ASSERT_TRUE(this->_io->launch(events[1]));
	}
	/**
	 * Клиентское событие
	 */
	{
		// Регистрируем объект транспортного уровня безопасности
		awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::CLIENT, awh::event::protocol_t::UDP);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(cts, 0);
		// Устанавливаем ALPN протоколы TLS
		this->_coder->alpn(cts, {{0,"http/1.1"}});
		// this->_coder->alpn(cts, {{0,"http/1.1"},{2,"h3"}});
		// Устанавливаем файл центра сертификации TLS
		this->_coder->ca(cts, "../sh/certificates", "ca.pem");
		// Включаем проверку имени хоста TLS
		this->_coder->validateServerNameIndication(cts, false);
		// Устанавливаем имя хоста TLS
		this->_coder->serverNameIndication(cts, "anyks.com");
		// Устанавливаем клиентский сертификат TLS
		this->_coder->certificate(cts, "../sh/certificates/client/cert.pem");
		// Устанавливаем приватный ключ TLS
		this->_coder->privateKey(cts, "../sh/certificates/client/key.pem");
		// Создаём идентификатор транспортного уровня TLS
		awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
		// Проверяем, что идентификатор транспортного уровня больше нуля
		ASSERT_GT(ctl, 0);
		// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
			/**
			 * Обрабатываем входящие состояния TLS
			 */
			switch(static_cast <uint8_t> (state)){
				// Если состояние ошибки транспортного уровня
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
					// Записываем ошибку в лог транспортного уровня TLS
					this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				break;
				// Если состояние уничтожения объекта транспортного уровня
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
					// Записываем в лог сообщение об успешном удалении контекста TLS
					this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
				break;
				// Если состояние рукопожатия успешно завершено
				case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
					// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
					std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
					std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
					std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
					std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
					std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
					std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
					std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
					// Возвращаем данные сертификата TLS
					std::cout << "Certificate data:\n" << this->_coder->certificateExtract(id) << std::endl << std::endl;
					// Записываем в лог информацию о TLS соединении
					std::cout << this->_coder->peerInfo(id) << std::endl;
					// Текст запроса к серверу
					const std::string request =
						"GET / HTTP/1.1\r\n"
						"Host: www.google.com\r\n"
						"Connection: close\r\n"
						"User-Agent: iouring-openssl-sample/1.0\r\n"
						"\r\n";
					// Если данные успешно зашифрованы TLS
					if(this->_coder->encrypt(id, request.c_str(), request.size()))
						// Записываем в лог сообщение об успешном шифровании данных TLS
						this->_log->print("Успешно зашифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, request.size());
					// Если данные не отправлены
					else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
				} break;
			}
		});
		// Регистрируем функцию обратного вызова на получение ошибок TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
			// Записываем в лог сообщение о предупреждающей ошибке TLS
			this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
		});
		// Регистрируем функцию обратного вызова на запись данных TLS
		this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
					// Записываем в лог сообщение о записи зашифрованных данных TLS
					this->_log->print("Записаны зашифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
					// Записываем в лог сообщение о записи дешифрованных данных TLS
					this->_log->print("Записаны дешифрованные данные TLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
				break;
			}
		});
		// Регистрируем функцию обратного вызова на чтение данных TLS
		this->_coder->on(ctl, [&events, &stop, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
			/**
			 * Обрабатываем тип события TLS
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие шифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
					// Отправляем данные обратно клиенту
					if(this->_io->send(events[0], reinterpret_cast <const char *> (buffer), size))
						// Если данные успешно отправлены
						this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, events[0], size);
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, events[0]);
				} break;
				// Если событие дешифрования данных TLS
				case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
					// Получаем ответ сервера в расшифрованном виде
					const std::string response(reinterpret_cast <const char *> (buffer), size);
					// Записываем в лог сообщение полученных данных с сервера
					this->_log->print("Получены данные с сервера TLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
					// Устанавливаем флаг завершения работы
					stop = true;
				} break;
			}
		});
		// Устанавливаем IP-адрес события
		ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес сервера назначения
		ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (status)){
				// Если статус принятия
				case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
					// Записываем в лог сообщение о принятии события
					this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус уничтожения
				case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
					// Записываем в лог сообщение об уничтожении события
					this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус инициализации
				case static_cast <uint8_t> (awh::event::status_t::INITIAL):
					// Записываем в лог сообщение об инициализации события
					this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус запуска события
				case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
					// Записываем в лог сообщение о запуске события
					this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус паузы события
				case static_cast <uint8_t> (awh::event::status_t::PAUSED):
					// Записываем в лог сообщение о паузе события
					this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус возобновления события
				case static_cast <uint8_t> (awh::event::status_t::RESUMED):
					// Записываем в лог сообщение о возобновлении события
					this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус успешного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
					// Записываем в лог сообщение о успешном выполнении события
					this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус неудачного выполнения события
				case static_cast <uint8_t> (awh::event::status_t::FAILURE):
					// Записываем в лог сообщение о неудачном выполнении события
					this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				break;
				// Если статус выполнения события в ожидании
				case static_cast <uint8_t> (awh::event::status_t::PENDING):
					// Записываем в лог сообщение о выполнении события в ожидании
					this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус подключения события
				case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус отмены события
				case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
					// Записываем в лог сообщение об отмене события
					this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус переподключения события
				case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если статус прослушивания события
				case static_cast <uint8_t> (awh::event::status_t::LISTENING):
					// Записываем в лог сообщение о прослушивании события
					this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на запись в событие
		this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
			// Записываем в лог сообщение о переподключении события
			this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события
		this->_io->on(events[0], [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Если данные успешно дешифрованы TLS
			if(this->_coder->decrypt(ctl, data, size))
				// Записываем в лог сообщение об успешном дешифровании данных TLS
				this->_log->print("Успешно дешифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
			// Если данные не отправлены
			else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
		});
		// Устанавливаем функцию обратного вызова на ошибку события
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
			/**
			 * Обрабатываем статус события
			 */
			switch(static_cast <uint8_t> (error)){
				// Если ошибка неизвестного события
				case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
					// Записываем ошибку в лог неизвестного события
					this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недопустимой операции
				case static_cast <uint8_t> (awh::event::error_t::INVALID):
					// Записываем ошибку в лог недопустимой операции
					this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа запрещёния
				case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
					// Записываем ошибку в лог доступа запрещёния
					this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка уже существующего объекта
				case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
					// Записываем ошибку в лог уже существующего объекта
					this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка доступа к сокету
				case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
					// Записываем ошибку в лог доступа к сокету
					this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка некорректного адреса
				case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
					// Записываем ошибку в лог некорректного адреса
					this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка ошибки подключения
				case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
					// Записываем ошибку в лог подключения
					this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка недостаточно ресурсов
				case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
					// Записываем ошибку в лог недостаточно ресурсов
					this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если ошибка события
				case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
					// Записываем ошибку в лог события
					this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
				// Если объект не найден
				case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
					// Записываем ошибку в лог события
					this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
				break;
			}
		});
		// Устанавливаем функцию обратного вызова на удачное подключение к серверу
		this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([ctl, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Записываем в лог сообщение о принятии события
			this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
			// Если подключение успешно
			if(ok){
				// Если рукопожатие TLS успешно
				if(this->_coder->handshake(ctl))
					// Записываем в лог сообщение о начале рукопожатия TLS
					this->_log->print("Начинаем процесс рукопожатия: ID=%u", awh::log_t::flag_t::INFO, ctl);
				// Если рукопожатие TLS не выполнено
				else this->_log->print("Ошибка рукопожатия TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, ctl);
			}
		}));
		// Устанавливаем функцию обратного вызова на общее событие
		this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
			/**
			 * Обрабатываем действие события
			 */
			switch(static_cast <uint8_t> (action)){
				// Если действие является чтением
				case static_cast <uint8_t> (awh::event::action_t::READ):
					// Записываем в лог сообщение о чтении события
					this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является записью
				case static_cast <uint8_t> (awh::event::action_t::WRITE):
					// Записываем в лог сообщение о записи события
					this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является подключением
				case static_cast <uint8_t> (awh::event::action_t::CONNECT):
					// Записываем в лог сообщение о подключении события
					this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отключением
				case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
					// Записываем в лог сообщение об отключении события
					this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переподключением
				case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является закрытием
				case static_cast <uint8_t> (awh::event::action_t::CLOSE):
					// Записываем в лог сообщение о закрытии события
					this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением
				case static_cast <uint8_t> (awh::event::action_t::CHANGE):
					// Записываем в лог сообщение об изменении события
					this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является удалением
				case static_cast <uint8_t> (awh::event::action_t::DELETE):
					// Записываем в лог сообщение об удалении события
					this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является переименованием
				case static_cast <uint8_t> (awh::event::action_t::RENAME):
					// Записываем в лог сообщение о переименовании события
					this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением атрибутов
				case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
					// Записываем в лог сообщение об изменении атрибутов события
					this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является отзывом доступа
				case static_cast <uint8_t> (awh::event::action_t::REVOKE):
					// Записываем в лог сообщение об отзыве доступа события
					this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
				// Если действие является изменением счётчика жёстких ссылок
				case static_cast <uint8_t> (awh::event::action_t::HDLINK):
					// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
					this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
				break;
			}
		});
		// Устанавливаем таймаут события на чтение
		this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
		// Устанавливаем таймаут события на запись
		this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
		// Устанавливаем таймаут события на подключение
		this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(events[0]));
		// Выполняем подключение к серверу
		ASSERT_TRUE(this->_io->connect(events[0]));
		// Запускаем событие клиента
		ASSERT_TRUE(this->_io->launch(events[0]));
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * Для операционных систем с поддержкой SCTP: Linux, FreeBSD, Solaris и illumos
 */
#if __linux__ || __FreeBSD__ || __sun
	/**
	 * @brief Тест проверки работы протокола SCTP Stream
	 *
	 */
	TEST_F(IoFixture, IoSCTPStreamTest){
		// Флаг остановки теста
		bool stop = false;
		// Выполняем генерацию порта
		const uint16_t port = ::port();
		// Добавляем новое событие клиента и сервера SCTP Stream
		const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::SCTP));
		/**
		 * Проверяем, что оба идентификатора события созданы успешно
		 */
		for(uint8_t i = 0; i < 2; i++)
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(this->_io->initialize());
		/**
		 * Серверное событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[1], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на принятие события
			this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(cid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info1: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(cid);
				// Возвращаем статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				this->_sctp->on(cid, static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
					// Записываем в лог информацию о сообщении SCTP-сокета
					this->_log->print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				this->_sctp->on(cid, [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
					// Записываем в лог сообщение с идентификатором событий SCTP
					std::cout << " SCTP EVENT ID: " << event->id << std::endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
							// Записываем в лог сообщение о событии DATA IO
							std::cout << "  - DATA IO EVENT " << std::endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
							// Записываем в лог сообщение о событии REMOTE ERROR
							std::cout << "  - REMOTE ERROR EVENT " << std::endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
							// Записываем в лог сообщение о событии ASSOC CHANGE
							std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Записываем в лог сообщение о событии SHUTDOWN EVENT
							std::cout << "  - SHUTDOWN EVENT " << std::endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Записываем в лог сообщение о событии SENDER DRY EVENT
							std::cout << "  - SENDER DRY EVENT " << std::endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Записываем в лог сообщение о событии PEER ADDR CHANGE
							std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Записываем в лог сообщение о событии SEND FAILED EVENT
							std::cout << "  - SEND FAILED EVENT " << std::endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Записываем в лог сообщение о событии STREAM RESET EVENT
							std::cout << "  - STREAM RESET EVENT " << std::endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Записываем в лог сообщение о событии AUTHENTICATION EVENT
							std::cout << "  - AUTHENTICATION EVENT " << std::endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Записываем в лог сообщение о событии ADAPTATION INDICATION
							std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
							std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
						break;
					}
				});
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::AUTO_RECONNECT));
				// Записываем в лог сообщение об успешной установке опций события
				this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
				// Устанавливаем функцию обратного вызова на запись в событие
				this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Текст входящего сообщения
					const std::string message(reinterpret_cast <const char *> (data), size);
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
					// Отправляем данные обратно клиенту
					if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
						// Если данные успешно отправлены
						this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				});
				// Устанавливаем функцию обратного вызова на общее событие
				this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (awh::event::action_t::READ):
							// Записываем в лог сообщение о чтении события
							this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (awh::event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (awh::event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (awh::event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (awh::event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (awh::event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (awh::event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
							this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
					}
				});
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
			// Выполняем фиксацию настроек события сервера
			ASSERT_TRUE(this->_io->commit(events[1]));
			// Выполняем прослушивание сервера
			ASSERT_TRUE(this->_io->listen(events[1], 100));
			// Запускаем событие сервера
			ASSERT_TRUE(this->_io->launch(events[1]));
		}
		/**
		 * Клиентское событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[0], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возрождения события
					case static_cast <uint8_t> (awh::event::status_t::REBIRTHED): {
						// Записываем в лог сообщение об возрождении события
						this->_log->print("Событие возрождено: ID=%u", awh::log_t::flag_t::INFO, eid);
						// Выполняем подписку на SCTP события
						this->_sctp->eventsSubscribe(eid, {
							awh::net::sctp::event_type_t::ASSOC_CHANGE,
							awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
							awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
							awh::net::sctp::event_type_t::REMOTE_ERROR
						});
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
			this->_sctp->on(events[0], static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
				// Записываем в лог информацию о сообщении SCTP-сокета
				this->_log->print(
					"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
					awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
				);
			}));
			// Устанавливаем функцию обратного вызова на создание события
			this->_sctp->on(events[0], [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
				// Записываем в лог сообщение с идентификатором событий SCTP
				std::cout << " SCTP EVENT ID: " << event->id << std::endl;
				/**
				 * Определяем тип события SCTP
				 */
				switch(static_cast <uint8_t> (event->type)){
					// Если требуется уведомление о каждом входящем DATA-пакете
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
						// Записываем в лог сообщение о событии DATA IO
						std::cout << "  - DATA IO EVENT " << std::endl;
					break;
					// Если ошибка удалённого узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
						// Записываем в лог сообщение о событии REMOTE ERROR
						std::cout << "  - REMOTE ERROR EVENT " << std::endl;
					break;
					// Если изменение ассоциации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
						// Записываем в лог сообщение о событии ASSOC CHANGE
						std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
					break;
					// Если событие завершения работы
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
						// Записываем в лог сообщение о событии SHUTDOWN EVENT
						std::cout << "  - SHUTDOWN EVENT " << std::endl;
					break;
					// Если событие "отправитель сухой"
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
						// Записываем в лог сообщение о событии SENDER DRY EVENT
						std::cout << "  - SENDER DRY EVENT " << std::endl;
					break;
					// Если изменение адреса однорангового узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
						// Записываем в лог сообщение о событии PEER ADDR CHANGE
						std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
					break;
					// Если событие ошибки отправки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
						// Записываем в лог сообщение о событии SEND FAILED EVENT
						std::cout << "  - SEND FAILED EVENT " << std::endl;
					break;
					// Если событие сброса потока
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
						// Записываем в лог сообщение о событии STREAM RESET EVENT
						std::cout << "  - STREAM RESET EVENT " << std::endl;
					break;
					// Если событие аутентификации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
						// Записываем в лог сообщение о событии AUTHENTICATION EVENT
						std::cout << "  - AUTHENTICATION EVENT " << std::endl;
					break;
					// Если событие адаптационное указание
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
						// Записываем в лог сообщение о событии ADAPTATION INDICATION
						std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
					break;
					// Если событие частичной доставки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
						// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
						std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info2: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(eid);
				// Возвращаем статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Останавливаем тест
				stop = true;
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на удачное подключение к серверу
			this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
				// Если подключение успешно
				if(ok){
					// Текст исходящего сообщения
					const std::string message("Hello from async client!");
					// Отправляем данные обратно клиенту
					if(this->_io->send(eid, message.c_str(), message.size()))
						// Если данные успешно отправлены
						this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				}
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Выполняем подключение к серверу
			ASSERT_TRUE(this->_io->connect(events[0]));
			// Запускаем событие клиента
			ASSERT_TRUE(this->_io->launch(events[0]));
		}
		/**
		 * Запускаем опрос событий
		 */
		while(!stop && this->_io->poll());
		// Уничтожаем все события после получения ответа
		ASSERT_TRUE(this->_io->deinitialize());
	}

	/**
	 * @brief Тест проверки работы протокола SCTP SEQPACKET
	 *
	 */
	TEST_F(IoFixture, IoSCTPSeqPacketTest){
		// Флаг остановки теста
		bool stop = false;
		// Выполняем генерацию порта
		const uint16_t port = ::port();
		// Добавляем новое событие клиента и сервера SCTP SEQPACKET
		const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::SEQPACKET, awh::event::protocol_t::SCTP));
		/**
		 * Проверяем, что оба идентификатора события созданы успешно
		 */
		for(uint8_t i = 0; i < 2; i++)
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(this->_io->initialize());
		/**
		 * Серверное событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[1], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на принятие события
			this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(cid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info1: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(cid);
				// Возвращаем статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				this->_sctp->on(cid, static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
					// Записываем в лог информацию о сообщении SCTP-сокета
					this->_log->print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				this->_sctp->on(cid, [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
					// Записываем в лог сообщение с идентификатором событий SCTP
					std::cout << " SCTP EVENT ID: " << event->id << std::endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
							// Записываем в лог сообщение о событии DATA IO
							std::cout << "  - DATA IO EVENT " << std::endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
							// Записываем в лог сообщение о событии REMOTE ERROR
							std::cout << "  - REMOTE ERROR EVENT " << std::endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
							// Записываем в лог сообщение о событии ASSOC CHANGE
							std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Записываем в лог сообщение о событии SHUTDOWN EVENT
							std::cout << "  - SHUTDOWN EVENT " << std::endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Записываем в лог сообщение о событии SENDER DRY EVENT
							std::cout << "  - SENDER DRY EVENT " << std::endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Записываем в лог сообщение о событии PEER ADDR CHANGE
							std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Записываем в лог сообщение о событии SEND FAILED EVENT
							std::cout << "  - SEND FAILED EVENT " << std::endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Записываем в лог сообщение о событии STREAM RESET EVENT
							std::cout << "  - STREAM RESET EVENT " << std::endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Записываем в лог сообщение о событии AUTHENTICATION EVENT
							std::cout << "  - AUTHENTICATION EVENT " << std::endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Записываем в лог сообщение о событии ADAPTATION INDICATION
							std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
							std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
						break;
					}
				});
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::AUTO_RECONNECT));
				// Записываем в лог сообщение об успешной установке опций события
				this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
				// Устанавливаем функцию обратного вызова на запись в событие
				this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Текст входящего сообщения
					const std::string message(reinterpret_cast <const char *> (data), size);
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
					// Отправляем данные обратно клиенту
					if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
						// Если данные успешно отправлены
						this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				});
				// Устанавливаем функцию обратного вызова на общее событие
				this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (awh::event::action_t::READ):
							// Записываем в лог сообщение о чтении события
							this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (awh::event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (awh::event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (awh::event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (awh::event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (awh::event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (awh::event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
							this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
					}
				});
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
			// Выполняем фиксацию настроек события сервера
			ASSERT_TRUE(this->_io->commit(events[1]));
			// Текст инициализационных сообщений SCTP
			awh::net::sctp::initmsg_t initmsg;
			// Устанавливаем количество попыток подключения SCTP
			initmsg.attempts = 4;
			// Устанавливаем количество исходящих потоков SCTP
			initmsg.ostreams = 5;
			// Устанавливаем количество входящих потоков SCTP
			initmsg.istreams = 5;
			// Инициализируем сообщения SCTP
			this->_sctp->initMessages(events[1], initmsg);
			// Выполняем прослушивание сервера
			ASSERT_TRUE(this->_io->listen(events[1], 100));
			// Запускаем событие сервера
			ASSERT_TRUE(this->_io->launch(events[1]));
		}
		/**
		 * Клиентское событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[0], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возрождения события
					case static_cast <uint8_t> (awh::event::status_t::REBIRTHED): {
						// Записываем в лог сообщение об возрождении события
						this->_log->print("Событие возрождено: ID=%u", awh::log_t::flag_t::INFO, eid);
						// Выполняем подписку на SCTP события
						this->_sctp->eventsSubscribe(eid, {
							awh::net::sctp::event_type_t::ASSOC_CHANGE,
							awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
							awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
							awh::net::sctp::event_type_t::REMOTE_ERROR
						});
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
			this->_sctp->on(events[0], static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
				// Записываем в лог информацию о сообщении SCTP-сокета
				this->_log->print(
					"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
					awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
				);
			}));
			// Устанавливаем функцию обратного вызова на создание события
			this->_sctp->on(events[0], [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
				// Записываем в лог сообщение с идентификатором событий SCTP
				std::cout << " SCTP EVENT ID: " << event->id << std::endl;
				/**
				 * Определяем тип события SCTP
				 */
				switch(static_cast <uint8_t> (event->type)){
					// Если требуется уведомление о каждом входящем DATA-пакете
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
						// Записываем в лог сообщение о событии DATA IO
						std::cout << "  - DATA IO EVENT " << std::endl;
					break;
					// Если ошибка удалённого узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
						// Записываем в лог сообщение о событии REMOTE ERROR
						std::cout << "  - REMOTE ERROR EVENT " << std::endl;
					break;
					// Если изменение ассоциации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
						// Записываем в лог сообщение о событии ASSOC CHANGE
						std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
					break;
					// Если событие завершения работы
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
						// Записываем в лог сообщение о событии SHUTDOWN EVENT
						std::cout << "  - SHUTDOWN EVENT " << std::endl;
					break;
					// Если событие "отправитель сухой"
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
						// Записываем в лог сообщение о событии SENDER DRY EVENT
						std::cout << "  - SENDER DRY EVENT " << std::endl;
					break;
					// Если изменение адреса однорангового узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
						// Записываем в лог сообщение о событии PEER ADDR CHANGE
						std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
					break;
					// Если событие ошибки отправки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
						// Записываем в лог сообщение о событии SEND FAILED EVENT
						std::cout << "  - SEND FAILED EVENT " << std::endl;
					break;
					// Если событие сброса потока
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
						// Записываем в лог сообщение о событии STREAM RESET EVENT
						std::cout << "  - STREAM RESET EVENT " << std::endl;
					break;
					// Если событие аутентификации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
						// Записываем в лог сообщение о событии AUTHENTICATION EVENT
						std::cout << "  - AUTHENTICATION EVENT " << std::endl;
					break;
					// Если событие адаптационное указание
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
						// Записываем в лог сообщение о событии ADAPTATION INDICATION
						std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
					break;
					// Если событие частичной доставки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
						// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
						std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info2: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(eid);
				// Возвращаем статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Останавливаем тест
				stop = true;
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на удачное подключение к серверу
			this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
				// Если подключение успешно
				if(ok){
					// Текст исходящего сообщения
					const std::string message("Hello from async client!");
					// Отправляем данные обратно клиенту
					if(this->_io->send(eid, message.c_str(), message.size()))
						// Если данные успешно отправлены
						this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				}
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Выполняем подключение к серверу
			ASSERT_TRUE(this->_io->connect(events[0]));
			// Запускаем событие клиента
			ASSERT_TRUE(this->_io->launch(events[0]));
		}
		/**
		 * Запускаем опрос событий
		 */
		while(!stop && this->_io->poll());
		// Уничтожаем все события после получения ответа
		ASSERT_TRUE(this->_io->deinitialize());
	}

	/**
	 * @brief Тест проверки работы протокола SCTP SEQPACKET с аутентификацией
	 *
	 */
	TEST_F(IoFixture, IoSCTPSeqPacketAuthTest){
		// Флаг остановки теста
		bool stop = false;
		// Выполняем генерацию порта
		const uint16_t port = ::port();
		// Добавляем новое событие клиента и сервера SCTP SEQPACKET
		const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::SEQPACKET, awh::event::protocol_t::SCTP));
		/**
		 * Проверяем, что оба идентификатора события созданы успешно
		 */
		for(uint8_t i = 0; i < 2; i++)
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(this->_io->initialize());
		/**
		 * Серверное событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Устанавливаем ключ аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateKey(events[1], 1, "0123456789abcdef0123456789abcdef"));
			// Устанавливаем режим использования ключа аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateKey(events[1], awh::event::mode_t::ENABLED, 1));
			// Устанавливаем поддерживаемые алгоритмы аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateSupportAlgorithms(events[1], {awh::net::sctp::auth_type_t::HMAC_SHA1, awh::net::sctp::auth_type_t::HMAC_SHA256}));
			// Устанавливаем чанки аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateChunks(events[1], {awh::net::sctp::auth_chunk_t::DATA, awh::net::sctp::auth_chunk_t::SHUTDOWN}));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[1], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR,
				awh::net::sctp::event_type_t::AUTHENTICATION_EVENT
			});
			// Извлекаем чанки аутентификации SCTP-сокета
			std::vector <awh::net::sctp::auth_chunk_t> chunks;
			// Выполняем извлечение чанков аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateChunks(events[1], awh::event::origin_t::LOCAL, chunks));
			/**
			 * Перебираем все извлечённые чанки
			 */
			for(auto & chunk : chunks)
				// Записываем в лог информацию о чанках аутентификации SCTP-сокета
				std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
			// Устанавливаем таймаут heartbeat SCTP-сокета
			ASSERT_TRUE(this->_sctp->setTimeout(events[1], awh::net::sctp::timeout_t::HEARTBEAT, 3000));
			// Возвращаем heartbeat timeout SCTP-сокета
			ASSERT_EQ(3000, this->_sctp->getTimeout(events[1], awh::net::sctp::timeout_t::HEARTBEAT));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на принятие события
			this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(cid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info1: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(cid);
				// Возвращаем статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Извлекаем чанки аутентификации SCTP-сокета
				std::vector <awh::net::sctp::auth_chunk_t> chunks;
				// Выполняем извлечение чанков аутентификации SCTP-сокета
				ASSERT_TRUE(this->_sctp->authenticateChunks(cid, awh::event::origin_t::REMOTE, chunks));
				/**
				 * Перебираем все извлечённые чанки
				 */
				for(auto & chunk : chunks)
					// Записываем в лог информацию о чанках аутентификации SCTP-сокета
					std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
				// Устанавливаем таймаут heartbeat SCTP-сокета
				ASSERT_TRUE(this->_sctp->setTimeout(cid, awh::net::sctp::timeout_t::HEARTBEAT, 3000));
				// Возвращаем heartbeat timeout SCTP-сокета
				ASSERT_EQ(3000, this->_sctp->getTimeout(cid, awh::net::sctp::timeout_t::HEARTBEAT));
				ASSERT_EQ(3000, this->_sctp->getTimeout(sid, awh::net::sctp::timeout_t::HEARTBEAT));
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				this->_sctp->on(cid, static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
					// Записываем в лог информацию о сообщении SCTP-сокета
					this->_log->print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				this->_sctp->on(cid, [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
					// Записываем в лог сообщение с идентификатором событий SCTP
					std::cout << " SCTP EVENT ID: " << event->id << std::endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
							// Записываем в лог сообщение о событии DATA IO
							std::cout << "  - DATA IO EVENT " << std::endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
							// Записываем в лог сообщение о событии REMOTE ERROR
							std::cout << "  - REMOTE ERROR EVENT " << std::endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
							// Записываем в лог сообщение о событии ASSOC CHANGE
							std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Записываем в лог сообщение о событии SHUTDOWN EVENT
							std::cout << "  - SHUTDOWN EVENT " << std::endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Записываем в лог сообщение о событии SENDER DRY EVENT
							std::cout << "  - SENDER DRY EVENT " << std::endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Записываем в лог сообщение о событии PEER ADDR CHANGE
							std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Записываем в лог сообщение о событии SEND FAILED EVENT
							std::cout << "  - SEND FAILED EVENT " << std::endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Записываем в лог сообщение о событии STREAM RESET EVENT
							std::cout << "  - STREAM RESET EVENT " << std::endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Записываем в лог сообщение о событии AUTHENTICATION EVENT
							std::cout << "  - AUTHENTICATION EVENT " << std::endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Записываем в лог сообщение о событии ADAPTATION INDICATION
							std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
							std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
						break;
					}
				});
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::AUTO_RECONNECT));
				// Записываем в лог сообщение об успешной установке опций события
				this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
				// Устанавливаем функцию обратного вызова на запись в событие
				this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				this->_io->on(cid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Текст входящего сообщения
					const std::string message(reinterpret_cast <const char *> (data), size);
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
					// Отправляем данные обратно клиенту
					if(this->_io->send(eid, reinterpret_cast <const char *> (data), size))
						// Если данные успешно отправлены
						this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				});
				// Устанавливаем функцию обратного вызова на общее событие
				this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (awh::event::action_t::READ):
							// Записываем в лог сообщение о чтении события
							this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (awh::event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (awh::event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (awh::event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (awh::event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (awh::event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (awh::event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
							this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
					}
				});
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
			// Выполняем фиксацию настроек события сервера
			ASSERT_TRUE(this->_io->commit(events[1]));
			// Текст инициализационных сообщений SCTP
			awh::net::sctp::initmsg_t initmsg;
			// Устанавливаем количество попыток подключения SCTP
			initmsg.attempts = 4;
			// Устанавливаем количество исходящих потоков SCTP
			initmsg.ostreams = 5;
			// Устанавливаем количество входящих потоков SCTP
			initmsg.istreams = 5;
			// Инициализируем сообщения SCTP
			this->_sctp->initMessages(events[1], initmsg);
			// Выполняем прослушивание сервера
			ASSERT_TRUE(this->_io->listen(events[1], 100));
			// Запускаем событие сервера
			ASSERT_TRUE(this->_io->launch(events[1]));
		}
		/**
		 * Клиентское событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Устанавливаем ключ аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateKey(events[0], 1, "0123456789abcdef0123456789abcdef"));
			// Устанавливаем режим использования ключа аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateKey(events[0], awh::event::mode_t::ENABLED, 1));
			// Устанавливаем поддерживаемые алгоритмы аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateSupportAlgorithms(events[0], {awh::net::sctp::auth_type_t::HMAC_SHA1, awh::net::sctp::auth_type_t::HMAC_SHA256}));
			// Устанавливаем чанки аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateChunks(events[0], {awh::net::sctp::auth_chunk_t::DATA, awh::net::sctp::auth_chunk_t::SHUTDOWN}));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[0], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR,
				awh::net::sctp::event_type_t::AUTHENTICATION_EVENT
			});
			// Извлекаем чанки аутентификации SCTP-сокета
			std::vector <awh::net::sctp::auth_chunk_t> chunks;
			// Выполняем извлечение чанков аутентификации SCTP-сокета
			ASSERT_TRUE(this->_sctp->authenticateChunks(events[0], awh::event::origin_t::LOCAL, chunks));
			/**
			 * Перебираем все извлечённые чанки
			 */
			for(auto & chunk : chunks)
				// Записываем в лог информацию о чанках аутентификации SCTP-сокета
				std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
			// Устанавливаем таймаут heartbeat SCTP-сокета
			ASSERT_TRUE(this->_sctp->setTimeout(events[0], awh::net::sctp::timeout_t::HEARTBEAT, 3000));
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возрождения события
					case static_cast <uint8_t> (awh::event::status_t::REBIRTHED): {
						// Записываем в лог сообщение об возрождении события
						this->_log->print("Событие возрождено: ID=%u", awh::log_t::flag_t::INFO, eid);
						// Устанавливаем ключ аутентификации SCTP-сокета
						ASSERT_TRUE(this->_sctp->authenticateKey(eid, 1, "0123456789abcdef0123456789abcdef"));
						// Устанавливаем режим использования ключа аутентификации SCTP-сокета
						ASSERT_TRUE(this->_sctp->authenticateKey(eid, awh::event::mode_t::ENABLED, 1));
						// Устанавливаем поддерживаемые алгоритмы аутентификации SCTP-сокета
						ASSERT_TRUE(this->_sctp->authenticateSupportAlgorithms(eid, {awh::net::sctp::auth_type_t::HMAC_SHA1, awh::net::sctp::auth_type_t::HMAC_SHA256}));
						// Устанавливаем чанки аутентификации SCTP-сокета
						ASSERT_TRUE(this->_sctp->authenticateChunks(eid, {awh::net::sctp::auth_chunk_t::DATA, awh::net::sctp::auth_chunk_t::SHUTDOWN}));
						// Выполняем подписку на SCTP события
						this->_sctp->eventsSubscribe(eid, {
							awh::net::sctp::event_type_t::ASSOC_CHANGE,
							awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
							awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
							awh::net::sctp::event_type_t::REMOTE_ERROR,
							awh::net::sctp::event_type_t::AUTHENTICATION_EVENT
						});
						// Извлекаем чанки аутентификации SCTP-сокета
						std::vector <awh::net::sctp::auth_chunk_t> chunks;
						// Выполняем извлечение чанков аутентификации SCTP-сокета
						ASSERT_TRUE(this->_sctp->authenticateChunks(eid, awh::event::origin_t::LOCAL, chunks));
						/**
						 * Перебираем все извлечённые чанки
						 */
						for(auto & chunk : chunks)
							// Записываем в лог информацию о чанках аутентификации SCTP-сокета
							std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
						// Устанавливаем таймаут heartbeat SCTP-сокета
						ASSERT_TRUE(this->_sctp->setTimeout(eid, awh::net::sctp::timeout_t::HEARTBEAT, 3000));
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
			this->_sctp->on(events[0], static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
				// Записываем в лог информацию о сообщении SCTP-сокета
				this->_log->print(
					"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
					awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
				);
			}));
			// Устанавливаем функцию обратного вызова на создание события
			this->_sctp->on(events[0], [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
				// Записываем в лог сообщение с идентификатором событий SCTP
				std::cout << " SCTP EVENT ID: " << event->id << std::endl;
				/**
				 * Определяем тип события SCTP
				 */
				switch(static_cast <uint8_t> (event->type)){
					// Если требуется уведомление о каждом входящем DATA-пакете
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
						// Записываем в лог сообщение о событии DATA IO
						std::cout << "  - DATA IO EVENT " << std::endl;
					break;
					// Если ошибка удалённого узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
						// Записываем в лог сообщение о событии REMOTE ERROR
						std::cout << "  - REMOTE ERROR EVENT " << std::endl;
					break;
					// Если изменение ассоциации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
						// Записываем в лог сообщение о событии ASSOC CHANGE
						std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
					break;
					// Если событие завершения работы
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
						// Записываем в лог сообщение о событии SHUTDOWN EVENT
						std::cout << "  - SHUTDOWN EVENT " << std::endl;
					break;
					// Если событие "отправитель сухой"
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
						// Записываем в лог сообщение о событии SENDER DRY EVENT
						std::cout << "  - SENDER DRY EVENT " << std::endl;
					break;
					// Если изменение адреса однорангового узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
						// Записываем в лог сообщение о событии PEER ADDR CHANGE
						std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
					break;
					// Если событие ошибки отправки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
						// Записываем в лог сообщение о событии SEND FAILED EVENT
						std::cout << "  - SEND FAILED EVENT " << std::endl;
					break;
					// Если событие сброса потока
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
						// Записываем в лог сообщение о событии STREAM RESET EVENT
						std::cout << "  - STREAM RESET EVENT " << std::endl;
					break;
					// Если событие аутентификации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
						// Записываем в лог сообщение о событии AUTHENTICATION EVENT
						std::cout << "  - AUTHENTICATION EVENT " << std::endl;
					break;
					// Если событие адаптационное указание
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
						// Записываем в лог сообщение о событии ADAPTATION INDICATION
						std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
					break;
					// Если событие частичной доставки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
						// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
						std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [&stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info2: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(eid);
				// Возвращаем статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Извлекаем чанки аутентификации SCTP-сокета
				std::vector <awh::net::sctp::auth_chunk_t> chunks;
				// Выполняем извлечение чанков аутентификации SCTP-сокета
				ASSERT_TRUE(this->_sctp->authenticateChunks(eid, awh::event::origin_t::REMOTE, chunks));
				/**
				 * Перебираем все извлечённые чанки
				 */
				for(auto & chunk : chunks)
					// Записываем в лог информацию о чанках аутентификации SCTP-сокета
					std::cout << " Извлечён чанк аутентификации SCTP-сокета: " << static_cast <uint16_t> (chunk) << std::endl;
				// Возвращаем heartbeat timeout SCTP-сокета
				ASSERT_EQ(3000, this->_sctp->getTimeout(eid, awh::net::sctp::timeout_t::HEARTBEAT));
				// Текст входящего сообщения
				const std::string message(reinterpret_cast <const char *> (data), size);
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, message.c_str());
				// Останавливаем тест
				stop = true;
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на удачное подключение к серверу
			this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
				// Если подключение успешно
				if(ok){
					// Текст исходящего сообщения
					const std::string message("Hello from async client!");
					// Отправляем данные обратно клиенту
					if(this->_io->send(eid, message.c_str(), message.size()))
						// Если данные успешно отправлены
						this->_log->print("Отправлено: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, message.size());
					// Если данные не отправлены
					else this->_log->print("Ошибка отправки: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				}
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Выполняем подключение к серверу
			ASSERT_TRUE(this->_io->connect(events[0]));
			// Запускаем событие клиента
			ASSERT_TRUE(this->_io->launch(events[0]));
		}
		/**
		 * Запускаем опрос событий
		 */
		while(!stop && this->_io->poll());
		// Уничтожаем все события после получения ответа
		ASSERT_TRUE(this->_io->deinitialize());
	}

	/**
	 * @brief Тест проверки работы протокола SCTP SEQPACKET DTLS
	 *
	 */
	TEST_F(IoFixture, IoSCTPSeqPacketDTLSTest){
		// Флаг остановки теста
		bool stop = false;
		// Выполняем генерацию порта
		const uint16_t port = ::port();
		// Добавляем новое событие клиента и сервера SCTP SEQPACKET
		const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::SEQPACKET, awh::event::protocol_t::SCTP));
		/**
		 * Проверяем, что оба идентификатора события созданы успешно
		 */
		for(uint8_t i = 0; i < 2; i++)
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(this->_io->initialize());
		/**
		 * Серверное событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			/**
			 * @note Контекст запрашивается как UDP: слой безопасности выбирается
			 *       по типу сокета, а не по протоколу транспорта. Сокет SCTP
			 *       типа SEQPACKET передаёт сообщения целиком, поэтому поверх
			 *       него работает DTLS - так же, как SCTP типа STREAM запрашивает
			 *       контекст как TCP и работает поверх обычного TLS.
			 */
			// Регистрируем объект транспортного уровня безопасности
			awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::SERVER, awh::event::protocol_t::UDP);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(cts, 0);
			// Устанавливаем ALPN протоколы TLS
			this->_coder->alpn(cts, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
			// Устанавливаем файл центра сертификации DTLS
			this->_coder->ca(cts, "../sh/certificates", "ca.pem");
			// Включаем проверку имени хоста DTLS
			this->_coder->validateServerNameIndication(cts, false);
			// Устанавливаем клиентский сертификат DTLS
			this->_coder->certificate(cts, "../sh/certificates/server/cert.pem");
			// Устанавливаем приватный ключ DTLS
			this->_coder->privateKey(cts, "../sh/certificates/server/key.pem");
			// Регистрируем функцию обратного вызова на получение ошибок DTLS
			this->_coder->on(cts, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
				// Записываем в лог сообщение о предупреждающей ошибке TLS
				this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
			});
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[1], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на принятие события
			this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([cts, this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(cid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info1: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(cid);
				// Возвращаем статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
				// Создаём идентификатор транспортного уровня DTLS
				awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
				// Проверяем, что идентификатор транспортного уровня больше нуля
				ASSERT_GT(ctl, 0);
				// Устанавливаем клиента DTLS для события
				this->_coder->peer(ctl, this->_io->getAddress(cid, awh::event::address_t::IPV4), this->_io->getSourcePort(cid));
				// Регистрируем функцию обратного вызова на получение ошибок DTLS
				this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
					// Записываем в лог сообщение о предупреждающей ошибке TLS
					this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
				});
				// Регистрируем функцию обратного вызова на запись данных DTLS
				this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем тип события DTLS
					 */
					switch(static_cast <uint8_t> (event)){
						// Если событие шифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
							// Записываем в лог сообщение о записи зашифрованных данных DTLS
							this->_log->print("Записаны зашифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
						break;
						// Если событие дешифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
							// Записываем в лог сообщение о записи дешифрованных данных DTLS
							this->_log->print("Записаны дешифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
						break;
					}
				});
				// Регистрируем функцию обратного вызова на успешное завершение рукопожатия DTLS
				this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
					/**
					 * Обрабатываем входящие состояния DTLS
					 */
					switch(static_cast <uint8_t> (state)){
						// Если состояние ошибки транспортного уровня
						case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
							// Записываем ошибку в лог транспортного уровня TLS
							this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
						break;
						// Если состояние уничтожения объекта транспортного уровня
						case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
							// Записываем в лог сообщение об успешном удалении контекста TLS
							this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
						break;
						// Если состояние рукопожатия успешно завершено
						case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
							// Записываем в лог сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
							std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
							std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
							std::cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << this->_coder->serverNameIndication(id) << std::endl << std::endl;
							std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
							std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
							std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
							std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
							std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
							std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
							// Возвращаем данные сертификата DTLS
							std::cout << "Certificate data:\n" << this->_coder->certificateExtract(id) << std::endl << std::endl;
							// Записываем в лог сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
							this->_log->print("Рукопожатие DTLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", awh::log_t::flag_t::INFO, id, this->_coder->alpn(id));
							// Записываем в лог информацию о DTLS соединении
							std::cout << this->_coder->peerInfo(id) << std::endl;
							// Выполняем повторную передачу данных TLS
							ASSERT_TRUE(this->_coder->retransmit(id));
						} break;
					}
				});
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				this->_sctp->on(cid, static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
					// Записываем в лог информацию о сообщении SCTP-сокета
					this->_log->print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				this->_sctp->on(cid, [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
					// Записываем в лог сообщение с идентификатором событий SCTP
					std::cout << " SCTP EVENT ID: " << event->id << std::endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
							// Записываем в лог сообщение о событии DATA IO
							std::cout << "  - DATA IO EVENT " << std::endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
							// Записываем в лог сообщение о событии REMOTE ERROR
							std::cout << "  - REMOTE ERROR EVENT " << std::endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
							// Записываем в лог сообщение о событии ASSOC CHANGE
							std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Записываем в лог сообщение о событии SHUTDOWN EVENT
							std::cout << "  - SHUTDOWN EVENT " << std::endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Записываем в лог сообщение о событии SENDER DRY EVENT
							std::cout << "  - SENDER DRY EVENT " << std::endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Записываем в лог сообщение о событии PEER ADDR CHANGE
							std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Записываем в лог сообщение о событии SEND FAILED EVENT
							std::cout << "  - SEND FAILED EVENT " << std::endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Записываем в лог сообщение о событии STREAM RESET EVENT
							std::cout << "  - STREAM RESET EVENT " << std::endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Записываем в лог сообщение о событии AUTHENTICATION EVENT
							std::cout << "  - AUTHENTICATION EVENT " << std::endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Записываем в лог сообщение о событии ADAPTATION INDICATION
							std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
							std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
						break;
					}
				});
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::AUTO_RECONNECT));
				// Регистрируем функцию обратного вызова на чтение данных DTLS
				this->_coder->on(ctl, [cid, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем тип события DTLS
					 */
					switch(static_cast <uint8_t> (event)){
						// Если событие шифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
							// Отправляем данные обратно клиенту
							if(this->_io->send(cid, reinterpret_cast <const char *> (buffer), size))
								// Если данные успешно отправлены
								this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, size);
							// Если данные не отправлены
							else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
						} break;
						// Если событие дешифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
							// Получаем ответ сервера в расшифрованном виде
							const std::string response(reinterpret_cast <const char *> (buffer), size);
							// Записываем в лог сообщение полученных данных с сервера
							this->_log->print("Получены данные с сервера DTLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
							// Если данные успешно зашифрованы DTLS
							if(this->_coder->encrypt(id, response.c_str(), response.size()))
								// Записываем в лог сообщение об успешном шифровании данных DTLS
								this->_log->print("Успешно зашифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, response.size());
							// Если данные не отправлены
							else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
						} break;
					}
				});
				// Записываем в лог сообщение об успешной установке опций события
				this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
				// Устанавливаем функцию обратного вызова на запись в событие
				this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				this->_io->on(cid, [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Если данные успешно дешифрованы DTLS
					if(this->_coder->decrypt(ctl, data, size))
						// Записываем в лог сообщение об успешном дешифровании данных DTLS
						this->_log->print("Успешно дешифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
					// Если данные не отправлены
					else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				});
				// Устанавливаем функцию обратного вызова на общее событие
				this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (awh::event::action_t::READ):
							// Записываем в лог сообщение о чтении события
							this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (awh::event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (awh::event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (awh::event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (awh::event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (awh::event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (awh::event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
							this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
					}
				});
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
			// Выполняем фиксацию настроек события сервера
			ASSERT_TRUE(this->_io->commit(events[1]));
			// Текст инициализационных сообщений SCTP
			awh::net::sctp::initmsg_t initmsg;
			// Устанавливаем количество попыток подключения SCTP
			initmsg.attempts = 4;
			// Устанавливаем количество исходящих потоков SCTP
			initmsg.ostreams = 5;
			// Устанавливаем количество входящих потоков SCTP
			initmsg.istreams = 5;
			// Инициализируем сообщения SCTP
			this->_sctp->initMessages(events[1], initmsg);
			// Выполняем прослушивание сервера
			ASSERT_TRUE(this->_io->listen(events[1], 100));
			// Запускаем событие сервера
			ASSERT_TRUE(this->_io->launch(events[1]));
		}
		/**
		 * Клиентское событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[0], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			/**
			 * @note Контекст запрашивается как UDP: слой безопасности выбирается
			 *       по типу сокета, а не по протоколу транспорта. Сокет SCTP
			 *       типа SEQPACKET передаёт сообщения целиком, поэтому поверх
			 *       него работает DTLS - так же, как SCTP типа STREAM запрашивает
			 *       контекст как TCP и работает поверх обычного TLS.
			 */
			// Регистрируем объект транспортного уровня безопасности
			awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::CLIENT, awh::event::protocol_t::UDP);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(cts, 0);
			// Устанавливаем ALPN протоколы TLS
			this->_coder->alpn(cts, {{0,"http/1.1"},{2,"h3"}});
			// Устанавливаем файл центра сертификации DTLS
			this->_coder->ca(cts, "../sh/certificates", "ca.pem");
			// Включаем проверку имени хоста DTLS
			this->_coder->validateServerNameIndication(cts, false);
			// Устанавливаем имя хоста DTLS
			this->_coder->serverNameIndication(cts, "server.anyks.com");
			// Устанавливаем клиентский сертификат DTLS
			this->_coder->certificate(cts, "../sh/certificates/client/cert.pem");
			// Устанавливаем приватный ключ DTLS
			this->_coder->privateKey(cts, "../sh/certificates/client/key.pem");
			// Создаём идентификатор транспортного уровня DTLS
			awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(ctl, 0);
			// Регистрируем функцию обратного вызова на успешное завершение рукопожатия DTLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
				/**
				 * Обрабатываем входящие состояния DTLS
				 */
				switch(static_cast <uint8_t> (state)){
					// Если состояние ошибки транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
						// Записываем ошибку в лог транспортного уровня TLS
						this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					break;
					// Если состояние уничтожения объекта транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
						// Записываем в лог сообщение об успешном удалении контекста TLS
						this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
					break;
					// Если состояние рукопожатия успешно завершено
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
						// Записываем в лог сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
						std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
						std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
						std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
						std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
						std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
						std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
						// Возвращаем данные сертификата DTLS
						std::cout << "Certificate data:\n" << this->_coder->certificateExtract(id) << std::endl << std::endl;
						// Записываем в лог информацию о DTLS соединении
						std::cout << this->_coder->peerInfo(id) << std::endl;
						// Текст запроса к серверу
						const std::string request =
							"GET / HTTP/1.1\r\n"
							"Host: www.google.com\r\n"
							"Connection: close\r\n"
							"User-Agent: iouring-openssl-sample/1.0\r\n"
							"\r\n";
						// Если данные успешно зашифрованы DTLS
						if(this->_coder->encrypt(id, request.c_str(), request.size()))
							// Записываем в лог сообщение об успешном шифровании данных DTLS
							this->_log->print("Успешно зашифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, request.size());
						// Если данные не отправлены
						else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					} break;
				}
			});
			// Регистрируем функцию обратного вызова на получение ошибок DTLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
				// Записываем в лог сообщение о предупреждающей ошибке TLS
				this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
			});
			// Регистрируем функцию обратного вызова на запись данных DTLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события DTLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
						// Записываем в лог сообщение о записи зашифрованных данных DTLS
						this->_log->print("Записаны зашифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
					// Если событие дешифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
						// Записываем в лог сообщение о записи дешифрованных данных DTLS
						this->_log->print("Записаны дешифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
				}
			});
			// Регистрируем функцию обратного вызова на чтение данных DTLS
			this->_coder->on(ctl, [&stop, &events, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события DTLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
						// Отправляем данные обратно клиенту
						if(this->_io->send(events[0], reinterpret_cast <const char *> (buffer), size))
							// Если данные успешно отправлены
							this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, events[0], size);
						// Если данные не отправлены
						else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, events[0]);
					} break;
					// Если событие дешифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
						// Получаем ответ сервера в расшифрованном виде
						const std::string response(reinterpret_cast <const char *> (buffer), size);
						// Записываем в лог сообщение полученных данных с сервера
						this->_log->print("Получены данные с сервера DTLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
						// Останавливаем тест
						stop = true;
					} break;
				}
			});
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возрождения события
					case static_cast <uint8_t> (awh::event::status_t::REBIRTHED): {
						// Записываем в лог сообщение об возрождении события
						this->_log->print("Событие возрождено: ID=%u", awh::log_t::flag_t::INFO, eid);
						// Выполняем подписку на SCTP события
						this->_sctp->eventsSubscribe(eid, {
							awh::net::sctp::event_type_t::ASSOC_CHANGE,
							awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
							awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
							awh::net::sctp::event_type_t::REMOTE_ERROR
						});
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
			this->_sctp->on(events[0], static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
				// Записываем в лог информацию о сообщении SCTP-сокета
				this->_log->print(
					"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
					awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
				);
			}));
			// Устанавливаем функцию обратного вызова на создание события
			this->_sctp->on(events[0], [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
				// Записываем в лог сообщение с идентификатором событий SCTP
				std::cout << " SCTP EVENT ID: " << event->id << std::endl;
				/**
				 * Определяем тип события SCTP
				 */
				switch(static_cast <uint8_t> (event->type)){
					// Если требуется уведомление о каждом входящем DATA-пакете
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
						// Записываем в лог сообщение о событии DATA IO
						std::cout << "  - DATA IO EVENT " << std::endl;
					break;
					// Если ошибка удалённого узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
						// Записываем в лог сообщение о событии REMOTE ERROR
						std::cout << "  - REMOTE ERROR EVENT " << std::endl;
					break;
					// Если изменение ассоциации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
						// Записываем в лог сообщение о событии ASSOC CHANGE
						std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
					break;
					// Если событие завершения работы
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
						// Записываем в лог сообщение о событии SHUTDOWN EVENT
						std::cout << "  - SHUTDOWN EVENT " << std::endl;
					break;
					// Если событие "отправитель сухой"
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
						// Записываем в лог сообщение о событии SENDER DRY EVENT
						std::cout << "  - SENDER DRY EVENT " << std::endl;
					break;
					// Если изменение адреса однорангового узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
						// Записываем в лог сообщение о событии PEER ADDR CHANGE
						std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
					break;
					// Если событие ошибки отправки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
						// Записываем в лог сообщение о событии SEND FAILED EVENT
						std::cout << "  - SEND FAILED EVENT " << std::endl;
					break;
					// Если событие сброса потока
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
						// Записываем в лог сообщение о событии STREAM RESET EVENT
						std::cout << "  - STREAM RESET EVENT " << std::endl;
					break;
					// Если событие аутентификации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
						// Записываем в лог сообщение о событии AUTHENTICATION EVENT
						std::cout << "  - AUTHENTICATION EVENT " << std::endl;
					break;
					// Если событие адаптационное указание
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
						// Записываем в лог сообщение о событии ADAPTATION INDICATION
						std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
					break;
					// Если событие частичной доставки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
						// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
						std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info2: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(eid);
				// Возвращаем статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Если данные успешно дешифрованы DTLS
				if(this->_coder->decrypt(ctl, data, size))
					// Записываем в лог сообщение об успешном дешифровании данных DTLS
					this->_log->print("Успешно дешифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на удачное подключение к серверу
			this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([ctl, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
				// Если подключение успешно
				if(ok){
					// Если рукопожатие DTLS успешно
					if(this->_coder->handshake(ctl))
						// Записываем в лог сообщение о начале рукопожатия DTLS
						this->_log->print("Начинаем процесс рукопожатия: ID=%u", awh::log_t::flag_t::INFO, ctl);
					// Если рукопожатие DTLS не выполнено
					else this->_log->print("Ошибка рукопожатия DTLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, ctl);
				}
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Выполняем подключение к серверу
			ASSERT_TRUE(this->_io->connect(events[0]));
			// Запускаем событие клиента
			ASSERT_TRUE(this->_io->launch(events[0]));
		}
		/**
		 * Запускаем опрос событий
		 */
		while(!stop && this->_io->poll());
		// Уничтожаем все события после получения ответа
		ASSERT_TRUE(this->_io->deinitialize());
	}

	/**
	 * @brief Тест проверки работы протокола SCTP STREAM TLS
	 *
	 */
	TEST_F(IoFixture, IoSCTPStreamTLSTest){
		// Флаг остановки теста
		bool stop = false;
		// Выполняем генерацию порта
		const uint16_t port = ::port();
		// Добавляем новое событие клиента и сервера SCTP STREAM
		const auto events = std::move(this->_io->events(awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::SCTP));
		/**
		 * Проверяем, что оба идентификатора события созданы успешно
		 */
		for(uint8_t i = 0; i < 2; i++)
			// Проверяем, что идентификатор события больше нуля
			ASSERT_GT(events[i], 0);
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setTargetPort(events[0], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getTargetPort(events[0]));
		// Устанавливаем порт события
		ASSERT_TRUE(this->_io->setSourcePort(events[1], port));
		// Проверяем что порт получен
		ASSERT_EQ(port, this->_io->getSourcePort(events[1]));
		// Инициализируем асинхронный движок ввода-вывода
		ASSERT_TRUE(this->_io->initialize());
		/**
		 * Серверное событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Регистрируем объект транспортного уровня безопасности
			awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::SERVER, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(cts, 0);
			// Устанавливаем ALPN протоколы TLS
			this->_coder->alpn(cts, {{0,"h2"},{1,"h3"},{2,"http/1.1"}});
			// Устанавливаем файл центра сертификации DTLS
			this->_coder->ca(cts, "../sh/certificates", "ca.pem");
			// Включаем проверку имени хоста DTLS
			this->_coder->validateServerNameIndication(cts, false);
			// Устанавливаем клиентский сертификат DTLS
			this->_coder->certificate(cts, "../sh/certificates/server/cert.pem");
			// Устанавливаем приватный ключ DTLS
			this->_coder->privateKey(cts, "../sh/certificates/server/key.pem");
			// Регистрируем функцию обратного вызова на получение ошибок DTLS
			this->_coder->on(cts, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
				// Записываем в лог сообщение о предупреждающей ошибке TLS
				this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
			});
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[1], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setAddress(events[1], awh::event::address_t::IPV4, "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на принятие события
			this->_io->on(events[1], static_cast <awh::engine::callback::accept_t> ([cts, this](const awh::event::id_t sid, const awh::event::id_t cid) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(cid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info1: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(cid);
				// Возвращаем статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие принято: ID=%u, Клиентский ID=%u", awh::log_t::flag_t::INFO, sid, cid);
				// Создаём идентификатор транспортного уровня DTLS
				awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
				// Проверяем, что идентификатор транспортного уровня больше нуля
				ASSERT_GT(ctl, 0);
				// Устанавливаем клиента DTLS для события
				this->_coder->peer(ctl, this->_io->getAddress(cid, awh::event::address_t::IPV4), this->_io->getSourcePort(cid));
				// Регистрируем функцию обратного вызова на получение ошибок DTLS
				this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
					// Записываем в лог сообщение о предупреждающей ошибке TLS
					this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
				});
				// Регистрируем функцию обратного вызова на запись данных DTLS
				this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем тип события DTLS
					 */
					switch(static_cast <uint8_t> (event)){
						// Если событие шифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
							// Записываем в лог сообщение о записи зашифрованных данных DTLS
							this->_log->print("Записаны зашифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
						break;
						// Если событие дешифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
							// Записываем в лог сообщение о записи дешифрованных данных DTLS
							this->_log->print("Записаны дешифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
						break;
					}
				});
				// Регистрируем функцию обратного вызова на успешное завершение рукопожатия DTLS
				this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
					/**
					 * Обрабатываем входящие состояния DTLS
					 */
					switch(static_cast <uint8_t> (state)){
						// Если состояние ошибки транспортного уровня
						case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
							// Записываем ошибку в лог транспортного уровня TLS
							this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
						break;
						// Если состояние уничтожения объекта транспортного уровня
						case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
							// Записываем в лог сообщение об успешном удалении контекста TLS
							this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
						break;
						// Если состояние рукопожатия успешно завершено
						case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
							// Записываем в лог сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
							std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
							std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
							std::cout << " !!!!!!!!!!!!!!!! HOSTNAME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n" << this->_coder->serverNameIndication(id) << std::endl << std::endl;
							std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
							std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
							std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
							std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
							std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
							std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
							// Возвращаем данные сертификата TLS
							std::cout << "Certificate data:\n" << this->_coder->certificateExtract(id) << std::endl << std::endl;
							// Записываем в лог сообщение об успешном завершении рукопожатия TLS и выводим выбранный ALPN протокол
							this->_log->print("Рукопожатие TLS успешно завершено: ID=%" PRIu64 ", ALPN протокол=%d", awh::log_t::flag_t::INFO, id, this->_coder->alpn(id));
						} break;
					}
				});
				// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
				this->_sctp->on(cid, static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
					// Записываем в лог информацию о сообщении SCTP-сокета
					this->_log->print(
						"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
						awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
					);
				}));
				// Устанавливаем функцию обратного вызова на создание события
				this->_sctp->on(cid, [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
					// Записываем в лог сообщение с идентификатором событий SCTP
					std::cout << " SCTP EVENT ID: " << event->id << std::endl;
					/**
					 * Определяем тип события SCTP
					 */
					switch(static_cast <uint8_t> (event->type)){
						// Если требуется уведомление о каждом входящем DATA-пакете
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
							// Записываем в лог сообщение о событии DATA IO
							std::cout << "  - DATA IO EVENT " << std::endl;
						break;
						// Если ошибка удалённого узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
							// Записываем в лог сообщение о событии REMOTE ERROR
							std::cout << "  - REMOTE ERROR EVENT " << std::endl;
						break;
						// Если изменение ассоциации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
							// Записываем в лог сообщение о событии ASSOC CHANGE
							std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
						break;
						// Если событие завершения работы
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
							// Записываем в лог сообщение о событии SHUTDOWN EVENT
							std::cout << "  - SHUTDOWN EVENT " << std::endl;
						break;
						// Если событие "отправитель сухой"
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
							// Записываем в лог сообщение о событии SENDER DRY EVENT
							std::cout << "  - SENDER DRY EVENT " << std::endl;
						break;
						// Если изменение адреса однорангового узла
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
							// Записываем в лог сообщение о событии PEER ADDR CHANGE
							std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
						break;
						// Если событие ошибки отправки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
							// Записываем в лог сообщение о событии SEND FAILED EVENT
							std::cout << "  - SEND FAILED EVENT " << std::endl;
						break;
						// Если событие сброса потока
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
							// Записываем в лог сообщение о событии STREAM RESET EVENT
							std::cout << "  - STREAM RESET EVENT " << std::endl;
						break;
						// Если событие аутентификации
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
							// Записываем в лог сообщение о событии AUTHENTICATION EVENT
							std::cout << "  - AUTHENTICATION EVENT " << std::endl;
						break;
						// Если событие адаптационное указание
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
							// Записываем в лог сообщение о событии ADAPTATION INDICATION
							std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
						break;
						// Если событие частичной доставки
						case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
							// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
							std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
						break;
					}
				});
				// Устананавливаем опции события
				ASSERT_TRUE(this->_io->setOptions(cid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY | awh::event::options::AUTO_RECONNECT));
				// Регистрируем функцию обратного вызова на чтение данных DTLS
				this->_coder->on(ctl, [cid, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем тип события DTLS
					 */
					switch(static_cast <uint8_t> (event)){
						// Если событие шифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
							// Отправляем данные обратно клиенту
							if(this->_io->send(cid, reinterpret_cast <const char *> (buffer), size))
								// Если данные успешно отправлены
								this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, cid, size);
							// Если данные не отправлены
							else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, cid);
						} break;
						// Если событие дешифрования данных DTLS
						case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
							// Получаем ответ сервера в расшифрованном виде
							const std::string response(reinterpret_cast <const char *> (buffer), size);
							// Записываем в лог сообщение полученных данных с сервера
							this->_log->print("Получены данные с сервера DTLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
							// Если данные успешно зашифрованы DTLS
							if(this->_coder->encrypt(id, response.c_str(), response.size()))
								// Записываем в лог сообщение об успешном шифровании данных DTLS
								this->_log->print("Успешно зашифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, response.size());
							// Если данные не отправлены
							else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
						} break;
					}
				});
				// Записываем в лог сообщение об успешной установке опций события
				this->_log->print("%s", awh::log_t::flag_t::INFO, "Успешно установлены опции события!");
				// Устанавливаем функцию обратного вызова на запись в событие
				this->_io->on(cid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
					// Записываем в лог сообщение о переподключении события
					this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				this->_io->on(cid, [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Если данные успешно дешифрованы TLS
					if(this->_coder->decrypt(ctl, data, size)){
						// Записываем в лог сообщение об успешном дешифровании данных TLS
						this->_log->print("Успешно дешифрованы данные TLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
					// Если данные не отправлены
					} else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
				});
				// Устанавливаем функцию обратного вызова на общее событие
				this->_io->on(cid, [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (awh::event::action_t::READ):
							// Записываем в лог сообщение о чтении события
							this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (awh::event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (awh::event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (awh::event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (awh::event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (awh::event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (awh::event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (awh::event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (awh::event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
							this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
						break;
					}
				});
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[1], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[1], awh::event::action_t::WRITE, 3000);
			// Выполняем фиксацию настроек события сервера
			ASSERT_TRUE(this->_io->commit(events[1]));
			// Выполняем прослушивание сервера
			ASSERT_TRUE(this->_io->listen(events[1], 100));
			// Запускаем событие сервера
			ASSERT_TRUE(this->_io->launch(events[1]));
		}
		/**
		 * Клиентское событие
		 */
		{
			// Устанавливаем опции событий
			ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY));
			// Выполняем подписку на SCTP события
			this->_sctp->eventsSubscribe(events[0], {
				awh::net::sctp::event_type_t::ASSOC_CHANGE,
				awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
				awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
				awh::net::sctp::event_type_t::REMOTE_ERROR
			});
			// Регистрируем объект транспортного уровня безопасности
			awh::tls::coder_t::id_t cts = this->_coder->context(awh::event::node_t::CLIENT, awh::event::protocol_t::TCP);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(cts, 0);
			// Устанавливаем ALPN протоколы TLS
			this->_coder->alpn(cts, {{0,"http/1.1"},{2,"h3"}});
			// Устанавливаем файл центра сертификации DTLS
			this->_coder->ca(cts, "../sh/certificates", "ca.pem");
			// Включаем проверку имени хоста DTLS
			this->_coder->validateServerNameIndication(cts, false);
			// Устанавливаем имя хоста DTLS
			this->_coder->serverNameIndication(cts, "server.anyks.com");
			// Устанавливаем клиентский сертификат TLS
			this->_coder->certificate(cts, "../sh/certificates/client/cert.pem");
			// Устанавливаем приватный ключ TLS
			this->_coder->privateKey(cts, "../sh/certificates/client/key.pem");
			// Создаём идентификатор транспортного уровня TLS
			awh::tls::coder_t::id_t ctl = this->_coder->transport(cts);
			// Проверяем, что идентификатор транспортного уровня больше нуля
			ASSERT_GT(ctl, 0);
			// Регистрируем функцию обратного вызова на успешное завершение рукопожатия TLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::state_t state) noexcept -> void {
				/**
				 * Обрабатываем входящие состояния TLS
				 */
				switch(static_cast <uint8_t> (state)){
					// Если состояние ошибки транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::FAILED):
						// Записываем ошибку в лог транспортного уровня TLS
						this->_log->print("Ошибка транспортного уровня TLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					break;
					// Если состояние уничтожения объекта транспортного уровня
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::DESTROYED):
						// Записываем в лог сообщение об успешном удалении контекста TLS
						this->_log->print("Контекст TLS успешно удалён: ID=%" PRIu64 "", awh::log_t::flag_t::INFO, id);
					break;
					// Если состояние рукопожатия успешно завершено
					case static_cast <uint8_t> (awh::tls::coder_t::state_t::HANDSHAKED): {
						// Записываем в лог сообщение об успешном завершении рукопожатия DTLS и выводим выбранный ALPN протокол
						std::cout << " !!!!!!!!!!!!!!!! HANDSHAKE COMPLETE !!!!!!!!!!!!!!!!!\n\n" << this->_coder->info(id) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!! SELECTED ALPN PROTOCOL !!!!!!!!!!!!!!!!!\n\n" << static_cast <u_short> (this->_coder->alpn(id)) << std::endl;
						std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
						std::cout << "Версия OpenSSL: " << this->_coder->version() << std::endl << std::endl;
						std::cout << "Cipher: " << this->_coder->cipherInfo(id) << std::endl << std::endl;
						std::cout << "Certificate: " << this->_coder->certificateInfo(id) << std::endl << std::endl;
						std::cout << "CRL Info: " << this->_coder->certificateRevocationListInfo(id) << std::endl << std::endl;
						std::cout << "Certificate Validation: " << (this->_coder->validateCertificate(id) ? "Valid" : "Invalid") << std::endl << std::endl;
						// Возвращаем данные сертификата DTLS
						std::cout << "Certificate data:\n" << this->_coder->certificateExtract(id) << std::endl << std::endl;
						// Записываем в лог информацию о DTLS соединении
						std::cout << this->_coder->peerInfo(id) << std::endl;
						// Текст запроса к серверу
						const std::string request =
							"GET / HTTP/1.1\r\n"
							"Host: www.google.com\r\n"
							"Connection: close\r\n"
							"User-Agent: iouring-openssl-sample/1.0\r\n"
							"\r\n";
						// Если данные успешно зашифрованы DTLS
						if(this->_coder->encrypt(id, request.c_str(), request.size()))
							// Записываем в лог сообщение об успешном шифровании данных DTLS
							this->_log->print("Успешно зашифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, id, request.size());
						// Если данные не отправлены
						else this->_log->print("Ошибка шифрования: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, id);
					} break;
				}
			});
			// Регистрируем функцию обратного вызова на получение ошибок DTLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::error_t error, const std::string & message) noexcept -> void {
				// Записываем в лог сообщение о предупреждающей ошибке TLS
				this->_log->print("Ошибка TLS: ID=%" PRIu64 ", Код=%u Сообщение=%s", awh::log_t::flag_t::CRITICAL, id, static_cast <uint8_t> (error), message.c_str());
			});
			// Регистрируем функцию обратного вызова на запись данных DTLS
			this->_coder->on(ctl, [this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события DTLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION):
						// Записываем в лог сообщение о записи зашифрованных данных DTLS
						this->_log->print("Записаны зашифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
					// Если событие дешифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION):
						// Записываем в лог сообщение о записи дешифрованных данных DTLS
						this->_log->print("Записаны дешифрованные данные DTLS: ID=%" PRIu64 ", Размер=%zu байт", awh::log_t::flag_t::INFO, id, size);
					break;
				}
			});
			// Регистрируем функцию обратного вызова на чтение данных DTLS
			this->_coder->on(ctl, [&stop, &events, this](const awh::tls::coder_t::id_t id, const awh::tls::coder_t::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
				/**
				 * Обрабатываем тип события DTLS
				 */
				switch(static_cast <uint8_t> (event)){
					// Если событие шифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::ENCRYPTION): {
						// Отправляем данные обратно клиенту
						if(this->_io->send(events[0], reinterpret_cast <const char *> (buffer), size))
							// Если данные успешно отправлены
							this->_log->print("Отправлено зашифрованных данных: ID=%u, %zu байт", awh::log_t::flag_t::INFO, events[0], size);
						// Если данные не отправлены
						else this->_log->print("Ошибка отправки зашифрованных данных: ID=%u", awh::log_t::flag_t::CRITICAL, events[0]);
					} break;
					// Если событие дешифрования данных DTLS
					case static_cast <uint8_t> (awh::tls::coder_t::event_t::DECRYPTION): {
						// Получаем ответ сервера в расшифрованном виде
						const std::string response(reinterpret_cast <const char *> (buffer), size);
						// Записываем в лог сообщение полученных данных с сервера
						this->_log->print("Получены данные с сервера DTLS: ID=%" PRIu64 ", Размер=%zu байт.\n\n%s", awh::log_t::flag_t::INFO, id, size, response.c_str());
						// Останавливаем тест
						stop = true;
					} break;
				}
			});
			// Устанавливаем IP-адрес события
			ASSERT_TRUE(this->_io->setAddress(events[0], awh::event::address_t::IPV4, "0.0.0.0"));
			// Устанавливаем адрес сервера назначения
			ASSERT_TRUE(this->_io->setTarget(events[0], "127.0.0.1"));
			// Устанавливаем функцию обратного вызова на событие таймера
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (status)){
					// Если статус принятия
					case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
						// Записываем в лог сообщение о принятии события
						this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус уничтожения
					case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
						// Записываем в лог сообщение об уничтожении события
						this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус инициализации
					case static_cast <uint8_t> (awh::event::status_t::INITIAL):
						// Записываем в лог сообщение об инициализации события
						this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус запуска события
					case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
						// Записываем в лог сообщение о запуске события
						this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус паузы события
					case static_cast <uint8_t> (awh::event::status_t::PAUSED):
						// Записываем в лог сообщение о паузе события
						this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возобновления события
					case static_cast <uint8_t> (awh::event::status_t::RESUMED):
						// Записываем в лог сообщение о возобновлении события
						this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус успешного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
						// Записываем в лог сообщение о успешном выполнении события
						this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус неудачного выполнения события
					case static_cast <uint8_t> (awh::event::status_t::FAILURE):
						// Записываем в лог сообщение о неудачном выполнении события
						this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
					break;
					// Если статус выполнения события в ожидании
					case static_cast <uint8_t> (awh::event::status_t::PENDING):
						// Записываем в лог сообщение о выполнении события в ожидании
						this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус подключения события
					case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус отмены события
					case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
						// Записываем в лог сообщение об отмене события
						this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус переподключения события
					case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус прослушивания события
					case static_cast <uint8_t> (awh::event::status_t::LISTENING):
						// Записываем в лог сообщение о прослушивании события
						this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если статус возрождения события
					case static_cast <uint8_t> (awh::event::status_t::REBIRTHED): {
						// Записываем в лог сообщение об возрождении события
						this->_log->print("Событие возрождено: ID=%u", awh::log_t::flag_t::INFO, eid);
						// Выполняем подписку на SCTP события
						this->_sctp->eventsSubscribe(eid, {
							awh::net::sctp::event_type_t::ASSOC_CHANGE,
							awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
							awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
							awh::net::sctp::event_type_t::REMOTE_ERROR
						});
					} break;
				}
			});
			// Устанавливаем функцию обратного вызова на запись в событие
			this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
			}));
			// Устанавливаем функцию обратного вызова на информацию о сообщении SCTP-сокета
			this->_sctp->on(events[0], static_cast <awh::engine::callback::sctp::minfo_t> ([this](const awh::event::id_t eid, const awh::net::sctp::minfo_t & minfo) noexcept -> void {
				// Записываем в лог информацию о сообщении SCTP-сокета
				this->_log->print(
					"CTP Message Info: %d\n  - Stream Number: %d\n  - Payload Protocol ID: %d\n  - Context: %d\n  - Time to Live: %d\n  - Flags: %zu",
					awh::log_t::flag_t::INFO, eid, minfo.num, minfo.ppid, minfo.ctx, minfo.ttl, minfo.flags.size()
				);
			}));
			// Устанавливаем функцию обратного вызова на создание события
			this->_sctp->on(events[0], [this](const awh::event::id_t eid, awh::net::sctp_event_t event) noexcept -> void {
				// Записываем в лог сообщение с идентификатором событий SCTP
				std::cout << " SCTP EVENT ID: " << event->id << std::endl;
				/**
				 * Определяем тип события SCTP
				 */
				switch(static_cast <uint8_t> (event->type)){
					// Если требуется уведомление о каждом входящем DATA-пакете
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::DATA_IO):
						// Записываем в лог сообщение о событии DATA IO
						std::cout << "  - DATA IO EVENT " << std::endl;
					break;
					// Если ошибка удалённого узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::REMOTE_ERROR):
						// Записываем в лог сообщение о событии REMOTE ERROR
						std::cout << "  - REMOTE ERROR EVENT " << std::endl;
					break;
					// Если изменение ассоциации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ASSOC_CHANGE):
						// Записываем в лог сообщение о событии ASSOC CHANGE
						std::cout << "  - ASSOC CHANGE EVENT " << std::endl;
					break;
					// Если событие завершения работы
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SHUTDOWN_EVENT):
						// Записываем в лог сообщение о событии SHUTDOWN EVENT
						std::cout << "  - SHUTDOWN EVENT " << std::endl;
					break;
					// Если событие "отправитель сухой"
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SENDER_DRY_EVENT):
						// Записываем в лог сообщение о событии SENDER DRY EVENT
						std::cout << "  - SENDER DRY EVENT " << std::endl;
					break;
					// Если изменение адреса однорангового узла
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PEER_ADDR_CHANGE):
						// Записываем в лог сообщение о событии PEER ADDR CHANGE
						std::cout << "  - PEER ADDR CHANGE EVENT " << std::endl;
					break;
					// Если событие ошибки отправки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::SEND_FAILED_EVENT):
						// Записываем в лог сообщение о событии SEND FAILED EVENT
						std::cout << "  - SEND FAILED EVENT " << std::endl;
					break;
					// Если событие сброса потока
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::STREAM_RESET_EVENT):
						// Записываем в лог сообщение о событии STREAM RESET EVENT
						std::cout << "  - STREAM RESET EVENT " << std::endl;
					break;
					// Если событие аутентификации
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::AUTHENTICATION_EVENT):
						// Записываем в лог сообщение о событии AUTHENTICATION EVENT
						std::cout << "  - AUTHENTICATION EVENT " << std::endl;
					break;
					// Если событие адаптационное указание
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::ADAPTATION_INDICATION):
						// Записываем в лог сообщение о событии ADAPTATION INDICATION
						std::cout << "  - ADAPTATION INDICATION EVENT " << std::endl;
					break;
					// Если событие частичной доставки
					case static_cast <uint8_t> (awh::net::sctp::event_type_t::PARTIAL_DELIVERY_EVENT):
						// Записываем в лог сообщение о событии PARTIAL DELIVERY EVENT
						std::cout << "  - PARTIAL DELIVERY EVENT " << std::endl;
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на чтение из события
			this->_io->on(events[0], [ctl, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Получаем информацию о сообщении SCTP-сокета
				const awh::net::sctp::minfo_t & minfo = this->_sctp->messageInfo(eid);
				// Записываем в лог информацию о сообщении SCTP-сокета
				std::cout << " SCTP Message Info2: " << std::endl;
				std::cout << "  - Stream Number: " << minfo.num << std::endl;
				std::cout << "  - Payload Protocol ID: " << static_cast <u_short> (minfo.ppid) << std::endl;
				std::cout << "  - Context: " << minfo.ctx << std::endl;
				std::cout << "  - Time to Live: " << minfo.ttl << std::endl;
				std::cout << "  - Flags: " << minfo.flags.size() << std::endl;
				// Получаем статус SCTP-сокета
				const awh::net::sctp::status_t & status = this->_sctp->status(eid);
				// Возвращаем статус SCTP-сокета
				std::cout << " SCTP Status: " << std::endl;
				std::cout << "  - ID: " << status.id << std::endl;
				std::cout << "  - State: " << static_cast <u_short> (status.state) << std::endl;
				std::cout << "  - Outbound Streams: " << status.ostreams << std::endl;
				std::cout << "  - Inbound Streams: " << status.istreams << std::endl;
				std::cout << "  - Fragmentation Point: " << status.fragpoint << std::endl;
				std::cout << "  - Rate Window: " << status.ratewind << std::endl;
				std::cout << "  - Unpack Data: " << status.unackdata << std::endl;
				std::cout << "  - Pending Data: " << status.penddata << std::endl;
				// Если данные успешно дешифрованы DTLS
				if(this->_coder->decrypt(ctl, data, size))
					// Записываем в лог сообщение об успешном дешифровании данных DTLS
					this->_log->print("Успешно дешифрованы данные DTLS: ID=%" PRIu64 ", %zu байт", awh::log_t::flag_t::INFO, ctl, size);
				// Если данные не отправлены
				else this->_log->print("Ошибка дешифрования: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
			});
			// Устанавливаем функцию обратного вызова на ошибку события
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
				/**
				 * Обрабатываем статус события
				 */
				switch(static_cast <uint8_t> (error)){
					// Если ошибка неизвестного события
					case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
						// Записываем ошибку в лог неизвестного события
						this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недопустимой операции
					case static_cast <uint8_t> (awh::event::error_t::INVALID):
						// Записываем ошибку в лог недопустимой операции
						this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа запрещёния
					case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
						// Записываем ошибку в лог доступа запрещёния
						this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка уже существующего объекта
					case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
						// Записываем ошибку в лог уже существующего объекта
						this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка доступа к сокету
					case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
						// Записываем ошибку в лог доступа к сокету
						this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка некорректного адреса
					case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
						// Записываем ошибку в лог некорректного адреса
						this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка ошибки подключения
					case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
						// Записываем ошибку в лог подключения
						this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка недостаточно ресурсов
					case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
						// Записываем ошибку в лог недостаточно ресурсов
						this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если ошибка события
					case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
						// Записываем ошибку в лог события
						this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
					// Если объект не найден
					case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
						// Записываем ошибку в лог события
						this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
					break;
				}
			});
			// Устанавливаем функцию обратного вызова на удачное подключение к серверу
			this->_io->on(events[0], static_cast <awh::engine::callback::connect_t> ([ctl, this](const awh::event::id_t eid, const bool ok) noexcept -> void {
				// Записываем в лог сообщение о принятии события
				this->_log->print("Событие подключения: ID=%u, результат: %s", awh::log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
				// Если подключение успешно
				if(ok){
					// Если рукопожатие DTLS успешно
					if(this->_coder->handshake(ctl))
						// Записываем в лог сообщение о начале рукопожатия DTLS
						this->_log->print("Начинаем процесс рукопожатия: ID=%u", awh::log_t::flag_t::INFO, ctl);
					// Если рукопожатие DTLS не выполнено
					else this->_log->print("Ошибка рукопожатия DTLS: ID=%" PRIu64 "", awh::log_t::flag_t::CRITICAL, ctl);
				}
			}));
			// Устанавливаем функцию обратного вызова на общее событие
			this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
				/**
				 * Обрабатываем действие события
				 */
				switch(static_cast <uint8_t> (action)){
					// Если действие является чтением
					case static_cast <uint8_t> (awh::event::action_t::READ):
						// Записываем в лог сообщение о чтении события
						this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является записью
					case static_cast <uint8_t> (awh::event::action_t::WRITE):
						// Записываем в лог сообщение о записи события
						this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является подключением
					case static_cast <uint8_t> (awh::event::action_t::CONNECT):
						// Записываем в лог сообщение о подключении события
						this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отключением
					case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
						// Записываем в лог сообщение об отключении события
						this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переподключением
					case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является закрытием
					case static_cast <uint8_t> (awh::event::action_t::CLOSE):
						// Записываем в лог сообщение о закрытии события
						this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением
					case static_cast <uint8_t> (awh::event::action_t::CHANGE):
						// Записываем в лог сообщение об изменении события
						this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является удалением
					case static_cast <uint8_t> (awh::event::action_t::DELETE):
						// Записываем в лог сообщение об удалении события
						this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является переименованием
					case static_cast <uint8_t> (awh::event::action_t::RENAME):
						// Записываем в лог сообщение о переименовании события
						this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением атрибутов
					case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
						// Записываем в лог сообщение об изменении атрибутов события
						this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является отзывом доступа
					case static_cast <uint8_t> (awh::event::action_t::REVOKE):
						// Записываем в лог сообщение об отзыве доступа события
						this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
					// Если действие является изменением счётчика жёстких ссылок
					case static_cast <uint8_t> (awh::event::action_t::HDLINK):
						// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
						this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
					break;
				}
			});
			// Устанавливаем таймаут события на чтение
			this->_io->setTimeout(events[0], awh::event::action_t::READ, 3000);
			// Устанавливаем таймаут события на запись
			this->_io->setTimeout(events[0], awh::event::action_t::WRITE, 3000);
			// Устанавливаем таймаут события на подключение
			this->_io->setTimeout(events[0], awh::event::action_t::CONNECT, 5000);
			// Выполняем фиксацию настроек события клиента
			ASSERT_TRUE(this->_io->commit(events[0]));
			// Выполняем подключение к серверу
			ASSERT_TRUE(this->_io->connect(events[0]));
			// Запускаем событие клиента
			ASSERT_TRUE(this->_io->launch(events[0]));
		}
		/**
		 * Запускаем опрос событий
		 */
		while(!stop && this->_io->poll());
		// Уничтожаем все события после получения ответа
		ASSERT_TRUE(this->_io->deinitialize());
	}
#endif

/**
 * @brief Тест маршрутизации дейтаграмм по ключу приложения
 *
 * @details Протоколы с собственной адресацией сессий внутри датаграммы
 *          маршрутизируются не четвёркой сокета, а ключом. Тест проверяет три
 *          свойства такой маршрутизации: две четвёрки с одним ключом попадают
 *          в одну сессию, ответ уходит на адрес последней принятой датаграммы,
 *          а датаграмма без ключа отбрасывается без создания сессии
 *
 */
TEST_F(IoFixture, IoOriginKeyedRoutingTest){
	// Флаг остановки теста
	bool stop = false;
	// Количество созданных сессий
	size_t sessions = 0;
	// Количество отброшенных датаграмм
	size_t dropped = 0;
	// Идентификатор события созданной сессии
	awh::event::id_t session = 0;
	// Буфер принятых сессией датаграмм
	std::vector <std::string> received;
	// Буфер ответов с идентификаторами принявших их клиентов
	std::vector <std::pair <awh::event::id_t, std::string>> answers;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем событие сервера UDP
	const awh::event::id_t server = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что событие сервера создано
	ASSERT_GT(server, 0u);
	// Добавляем первое событие клиента UDP
	const awh::event::id_t first = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Добавляем второе событие клиента UDP
	const awh::event::id_t second = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Добавляем событие таймера принудительной остановки теста
	const awh::event::id_t timer = this->_io->event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
	// Проверяем что события клиентов и таймера созданы
	ASSERT_GT(first, 0u);
	ASSERT_GT(second, 0u);
	ASSERT_GT(timer, 0u);
	// Устанавливаем порт события сервера
	ASSERT_TRUE(this->_io->setSourcePort(server, port));
	// Устанавливаем порт назначения событий клиентов
	ASSERT_TRUE(this->_io->setTargetPort(first, port));
	ASSERT_TRUE(this->_io->setTargetPort(second, port));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем опции события сервера
	ASSERT_TRUE(this->_io->setOptions(server, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
	// Устанавливаем адрес события сервера
	ASSERT_TRUE(this->_io->setAddress(server, awh::event::address_t::IPV4, "127.0.0.1"));
	/**
	 * Устанавливаем функцию обратного вызова определения сессии: ключом служат
	 * первые четыре октета датаграммы, а датаграмма короче ключа объявляется
	 * посторонней
	 */
	this->_io->on(server, static_cast <awh::engine::callback::origin_t> ([&dropped](
		[[maybe_unused]] const awh::event::id_t eid, const uint8_t * data, const size_t size, awh::net::origin_key_t & key
	) noexcept -> bool {
		// Если датаграмма короче ключа сессии
		if(size < 4){
			// Увеличиваем количество отброшенных датаграмм
			dropped++;
			// Выводим отрицательный результат - датаграмма посторонняя
			return false;
		}
		// Формируем ключ сессии из первых октетов датаграммы
		key = awh::net::origin_key_t(data, 4);
		// Выводим положительный результат
		return true;
	}));
	// Устанавливаем функцию обратного вызова на создание сессии
	this->_io->on(server, static_cast <awh::engine::callback::accept_t> ([&](
		[[maybe_unused]] const awh::event::id_t eid, const awh::event::id_t oid
	) noexcept -> void {
		// Увеличиваем количество созданных сессий
		sessions++;
		// Запоминаем идентификатор события созданной сессии
		session = oid;
		// Устанавливаем функцию обратного вызова на чтение из сессии
		this->_io->on(oid, [&](const awh::event::id_t oid, const uint8_t * data, const size_t size) noexcept -> void {
			// Накапливаем принятую сессией датаграмму
			received.emplace_back(reinterpret_cast <const char *> (data), size);
			// Отправляем ответ по адресу последней принятой датаграммы
			this->_io->send(oid, data, size);
		});
	}));
	/**
	 * Устанавливаем функции обратного вызова на чтение ответов клиентами
	 */
	for(auto & client : {first, second}){
		/**
		 * Устанавливаем функцию обратного вызова на подключение события клиента:
		 * до завершения подключения событие к отправке не готово
		 */
		this->_io->on(client, static_cast <awh::engine::callback::connect_t> ([&, client](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Если подключение не выполнено
			if(!ok)
				// Выходим из функции обработки
				return;
			// Если подключено первое событие клиента
			if(client == first){
				// Отправляем посторонную датаграмму короче ключа сессии
				this->_io->send(eid, "xx", 2);
				// Отправляем датаграмму с ключом сессии первым клиентом
				this->_io->send(eid, "KEY0first", 9);
			// Отправляем датаграмму с тем же ключом сессии вторым клиентом
			} else this->_io->send(eid, "KEY0second", 10);
		}));
		// Устанавливаем функцию обратного вызова на чтение из события клиента
		this->_io->on(client, [&](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			// Накапливаем принятый клиентом ответ вместе с идентификатором клиента
			answers.emplace_back(eid, std::string(reinterpret_cast <const char *> (data), size));
			// Останавливаем тест после ответа второму клиенту
			stop = (answers.size() >= 2);
		});
		// Устанавливаем опции события клиента
		ASSERT_TRUE(this->_io->setOptions(client, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
		// Устанавливаем локальный адрес события клиента
		ASSERT_TRUE(this->_io->setAddress(client, awh::event::address_t::IPV4, "0.0.0.0"));
		// Устанавливаем адрес назначения события клиента
		ASSERT_TRUE(this->_io->setTarget(client, "127.0.0.1"));
	}
	// Устанавливаем таймаут принудительной остановки теста
	this->_io->setTimeout(timer, awh::event::action_t::NONE, 3000);
	// Устанавливаем функцию обратного вызова на событие таймера
	this->_io->on(timer, [&stop]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Останавливаем тест по истечении таймаута
		stop = (stop || (status == awh::event::status_t::SUCCESS));
	});
	// Выполняем фиксацию настроек событий
	ASSERT_TRUE(this->_io->commit(server));
	ASSERT_TRUE(this->_io->commit(timer));
	// Выполняем запуск событий
	ASSERT_TRUE(this->_io->launch(server));
	ASSERT_TRUE(this->_io->launch(timer));
	/**
	 * Выполняем подготовку и запуск событий клиентов
	 */
	for(auto & client : {first, second}){
		// Выполняем фиксацию настроек события клиента
		ASSERT_TRUE(this->_io->commit(client));
		// Выполняем подключение события клиента к серверу
		ASSERT_TRUE(this->_io->connect(client));
		// Выполняем запуск события клиента
		ASSERT_TRUE(this->_io->launch(client));
	}
	/**
	 *  Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Проверяем что посторонняя датаграмма отброшена
	ASSERT_EQ(dropped, 1u);
	/**
	 * Проверяем что две четвёрки с одним ключом попали в одну сессию: посторонняя
	 * датаграмма сессии не создала, а вторая четвёрка не создала новой
	 */
	ASSERT_EQ(sessions, 1u);
	// Проверяем что идентификатор события сессии получен
	ASSERT_GT(session, 0u);
	// Проверяем что сессия приняла обе датаграммы
	ASSERT_EQ(received.size(), 2u);
	/**
	 * @par Намеренные решения
	 *
	 * Принятое сличается составом, а не порядком. Датаграммы шлют два разных
	 * клиента, у каждого своё гнездо и своя четвёрка, и очерёдности между
	 * ними не задаёт ничто: она складывается из того, чей обработчик
	 * подключения сработал прежде. На macOS, FreeBSD и NetBSD это выходил
	 * порядок заведения, а на OpenBSD - обратный, и проверка на точный
	 * порядок отказывала там, где ничего не нарушено
	 *
	 * Смысл проверки в том, что обе датаграммы попали в одну сессию и дошли
	 * целыми, - он от очерёдности не зависит вовсе
	 *
	 */
	ASSERT_EQ((std::multiset <std::string> (received.begin(), received.end())),
	          (std::multiset <std::string> ({"KEY0first", "KEY0second"})));
	// Проверяем что оба ответа доставлены
	ASSERT_EQ(answers.size(), 2u);
	/**
	 * Проверяем что ответы ушли по адресам отправителей
	 *
	 * @note Сличается тоже составом, но пара при этом цела: смысл здесь в том, что
	 *       каждому клиенту вернулась именно его датаграмма, а не в том, которая из
	 *       пар оказалась в перечне первой
	 *
	 */
	ASSERT_EQ((std::multiset <std::pair <awh::event::id_t, std::string>> (answers.begin(), answers.end())),
	          (std::multiset <std::pair <awh::event::id_t, std::string>> ({{first, "KEY0first"}, {second, "KEY0second"}})));
}

/**
 * @brief Тест предела количества сессий дейтаграммного события
 *
 * @details Предел защищает от исчерпания памяти потоком датаграмм с разными
 *          ключами: по его достижении новые сессии не создаются
 *
 */
TEST_F(IoFixture, IoOriginLimitTest){
	// Флаг остановки теста
	bool stop = false;
	// Количество созданных сессий
	size_t sessions = 0;
	// Количество отказов в создании сессии
	size_t cancelled = 0;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем событие сервера UDP
	const awh::event::id_t server = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Добавляем событие клиента UDP
	const awh::event::id_t client = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Добавляем событие таймера принудительной остановки теста
	const awh::event::id_t timer = this->_io->event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
	// Проверяем что события созданы
	ASSERT_GT(server, 0u);
	ASSERT_GT(client, 0u);
	ASSERT_GT(timer, 0u);
	// Устанавливаем порт события сервера
	ASSERT_TRUE(this->_io->setSourcePort(server, port));
	// Устанавливаем порт назначения события клиента
	ASSERT_TRUE(this->_io->setTargetPort(client, port));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем опции события сервера
	ASSERT_TRUE(this->_io->setOptions(server, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
	// Устанавливаем адрес события сервера
	ASSERT_TRUE(this->_io->setAddress(server, awh::event::address_t::IPV4, "127.0.0.1"));
	// Проверяем предельное количество сессий по умолчанию
	ASSERT_EQ(this->_io->getMaxConnections(server), 100u);
	// Устанавливаем предельное количество сессий события
	ASSERT_TRUE(this->_io->setMaxConnections(server, 1));
	// Проверяем что предельное количество сессий установлено
	ASSERT_EQ(this->_io->getMaxConnections(server), 1u);
	// Проверяем отказ установки нулевого предела
	ASSERT_FALSE(this->_io->setMaxConnections(server, 0));
	// Устанавливаем функцию обратного вызова определения сессии
	this->_io->on(server, static_cast <awh::engine::callback::origin_t> ([](
		[[maybe_unused]] const awh::event::id_t eid, const uint8_t * data, const size_t size, awh::net::origin_key_t & key
	) noexcept -> bool {
		// Если датаграмма короче ключа сессии
		if(size < 4)
			// Выводим отрицательный результат - датаграмма посторонняя
			return false;
		// Формируем ключ сессии из первых октетов датаграммы
		key = awh::net::origin_key_t(data, 4);
		// Выводим положительный результат
		return true;
	}));
	// Устанавливаем функцию обратного вызова на создание сессии
	this->_io->on(server, static_cast <awh::engine::callback::accept_t> ([&sessions](
		[[maybe_unused]] const awh::event::id_t eid, [[maybe_unused]] const awh::event::id_t oid
	) noexcept -> void {
		// Увеличиваем количество созданных сессий
		sessions++;
	}));
	// Устанавливаем функцию обратного вызова на изменение статуса события сервера
	this->_io->on(server, static_cast <awh::engine::callback::status_t> ([&cancelled](
		[[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status
	) noexcept -> void {
		// Если получен отказ в создании сессии
		if(status == awh::event::status_t::CANCELLED)
			// Увеличиваем количество отказов в создании сессии
			cancelled++;
	}));
	// Устанавливаем опции события клиента
	ASSERT_TRUE(this->_io->setOptions(client, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
	// Устанавливаем локальный адрес события клиента
	ASSERT_TRUE(this->_io->setAddress(client, awh::event::address_t::IPV4, "0.0.0.0"));
	// Устанавливаем адрес назначения события клиента
	ASSERT_TRUE(this->_io->setTarget(client, "127.0.0.1"));
	/**
	 * Устанавливаем функцию обратного вызова на подключение события клиента:
	 * до завершения подключения событие к отправке не готово
	 */
	this->_io->on(client, static_cast <awh::engine::callback::connect_t> ([this](const awh::event::id_t eid, const bool ok) noexcept -> void {
		// Если подключение не выполнено
		if(!ok)
			// Выходим из функции обработки
			return;
		// Отправляем датаграмму с первым ключом сессии
		this->_io->send(eid, "AAAApayload", 11);
		// Отправляем датаграмму со вторым ключом сессии
		this->_io->send(eid, "BBBBpayload", 11);
	}));
	// Устанавливаем таймаут принудительной остановки теста
	this->_io->setTimeout(timer, awh::event::action_t::NONE, 1000);
	// Устанавливаем функцию обратного вызова на событие таймера
	this->_io->on(timer, [&stop]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Останавливаем тест по истечении таймаута
		stop = (stop || (status == awh::event::status_t::SUCCESS));
	});
	// Выполняем фиксацию настроек событий
	ASSERT_TRUE(this->_io->commit(server));
	ASSERT_TRUE(this->_io->commit(timer));
	// Выполняем запуск событий
	ASSERT_TRUE(this->_io->launch(server));
	ASSERT_TRUE(this->_io->launch(timer));
	// Выполняем фиксацию настроек события клиента
	ASSERT_TRUE(this->_io->commit(client));
	// Выполняем подключение события клиента к серверу
	ASSERT_TRUE(this->_io->connect(client));
	// Выполняем запуск события клиента
	ASSERT_TRUE(this->_io->launch(client));
	/**
	 *  Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Проверяем что создана только одна сессия
	ASSERT_EQ(sessions, 1u);
	// Проверяем что в создании второй сессии отказано
	ASSERT_GE(cancelled, 1u);
}

/**
 * @brief Тест разделения сессий между дейтаграммными событиями
 *
 * @details Ключ сессии закреплён за событием, поэтому один и тот же отправитель,
 *          обратившийся к двум серверам, порождает две независимые сессии,
 *          а не одну общую. Датаграммы отправляются одним сокетом, чтобы
 *          четвёрка источника у обоих серверов совпадала
 *
 */
TEST_F(IoFixture, IoOriginNamespaceTest){
	// Флаг остановки теста
	bool stop = false;
	// Количество созданных сессий первого сервера
	size_t first = 0;
	// Количество созданных сессий второго сервера
	size_t second = 0;
	// Выполняем генерацию порта первого сервера
	const uint16_t primary = ::port();
	// Выполняем генерацию порта второго сервера
	const uint16_t secondary = static_cast <uint16_t> ((primary > 49152) ? (primary - 1) : (primary + 1));
	// Добавляем событие первого сервера UDP
	const awh::event::id_t alpha = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Добавляем событие второго сервера UDP
	const awh::event::id_t beta = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Добавляем событие таймера принудительной остановки теста
	const awh::event::id_t timer = this->_io->event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
	// Проверяем что события созданы
	ASSERT_GT(alpha, 0u);
	ASSERT_GT(beta, 0u);
	ASSERT_GT(timer, 0u);
	// Устанавливаем порты событий серверов
	ASSERT_TRUE(this->_io->setSourcePort(alpha, primary));
	ASSERT_TRUE(this->_io->setSourcePort(beta, secondary));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	/**
	 * Выполняем настройку событий серверов
	 */
	for(auto & server : {alpha, beta}){
		// Устанавливаем опции события сервера
		ASSERT_TRUE(this->_io->setOptions(server, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
		// Устанавливаем адрес события сервера
		ASSERT_TRUE(this->_io->setAddress(server, awh::event::address_t::IPV4, "127.0.0.1"));
		// Устанавливаем функцию обратного вызова на создание сессии
		this->_io->on(server, static_cast <awh::engine::callback::accept_t> ([&, server](
			[[maybe_unused]] const awh::event::id_t eid, [[maybe_unused]] const awh::event::id_t oid
		) noexcept -> void {
			// Увеличиваем счётчик сессий соответствующего сервера
			(server == alpha ? first : second)++;
			// Останавливаем тест после создания сессий обоими серверами
			stop = ((first > 0) && (second > 0));
		}));
		// Выполняем фиксацию настроек события сервера
		ASSERT_TRUE(this->_io->commit(server));
		// Выполняем запуск события сервера
		ASSERT_TRUE(this->_io->launch(server));
	}
	// Устанавливаем таймаут принудительной остановки теста
	this->_io->setTimeout(timer, awh::event::action_t::NONE, 2000);
	// Устанавливаем функцию обратного вызова на событие таймера
	this->_io->on(timer, [&stop]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Останавливаем тест по истечении таймаута
		stop = (stop || (status == awh::event::status_t::SUCCESS));
	});
	// Выполняем фиксацию настроек события таймера
	ASSERT_TRUE(this->_io->commit(timer));
	// Выполняем запуск события таймера
	ASSERT_TRUE(this->_io->launch(timer));
	/**
	 * Отправляем датаграммы обоим серверам одним сокетом: разные сокеты дали бы
	 * разные порты источника, и совпадения четвёрок, ради которого ставится
	 * проверка, не возникло бы
	 */
	const int32_t sock = ::socket(AF_INET, SOCK_DGRAM, 0);
	// Проверяем что сокет отправителя создан
	ASSERT_GE(sock, 0);
	// Объект адреса назначения датаграммы
	struct sockaddr_in target{};
	// Устанавливаем семейство адреса назначения
	target.sin_family = AF_INET;
	// Устанавливаем адрес назначения датаграммы
	target.sin_addr.s_addr = ::inet_addr("127.0.0.1");
	/**
	 * Перебираем порты серверов
	 */
	for(auto & port : {primary, secondary}){
		// Устанавливаем порт назначения датаграммы
		target.sin_port = htons(port);
		// Отправляем датаграмму серверу
		ASSERT_GT(::sendto(sock, "SAMEpayload", 11, 0, reinterpret_cast <struct sockaddr *> (&target), sizeof(target)), 0);
	}
	// Закрываем сокет отправителя
	::closesocket(sock);
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	/**
	 * Проверяем что каждый сервер завёл собственную сессию: ключ закреплён за
	 * событием, поэтому общего пространства у них нет, хотя четвёрка источника
	 * у обеих датаграмм одна
	 */
	ASSERT_EQ(first, 1u);
	ASSERT_EQ(second, 1u);
}

/**
 * @brief Тест привязки дополнительных ключей маршрутизации к сессии
 *
 * @details Протоколы со сменой идентификатора на лету адресуют одну сессию
 *          произвольным числом ключей: датаграмма с любым привязанным ключом
 *          обязана попасть в неё, а снятый ключ маршрутизироваться перестаёт
 *
 */
TEST_F(IoFixture, IoOriginBindTest){
	// Флаг остановки теста
	bool stop = false;
	// Количество созданных сессий
	size_t sessions = 0;
	// Идентификатор события созданной сессии
	awh::event::id_t session = 0;
	// Буфер принятых датаграмм с идентификаторами принявших их сессий
	std::vector <std::pair <awh::event::id_t, std::string>> received;
	// Выполняем генерацию порта
	const uint16_t port = ::port();
	// Добавляем событие сервера UDP
	const awh::event::id_t server = this->_io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Добавляем событие таймера принудительной остановки теста
	const awh::event::id_t timer = this->_io->event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
	// Проверяем что события созданы
	ASSERT_GT(server, 0u);
	ASSERT_GT(timer, 0u);
	// Устанавливаем порт события сервера
	ASSERT_TRUE(this->_io->setSourcePort(server, port));
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем опции события сервера
	ASSERT_TRUE(this->_io->setOptions(server, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::REUSE_PORT | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
	// Устанавливаем адрес события сервера
	ASSERT_TRUE(this->_io->setAddress(server, awh::event::address_t::IPV4, "127.0.0.1"));
	// Устанавливаем функцию обратного вызова определения сессии
	this->_io->on(server, static_cast <awh::engine::callback::origin_t> ([](
		[[maybe_unused]] const awh::event::id_t eid, const uint8_t * data, const size_t size, awh::net::origin_key_t & key
	) noexcept -> bool {
		// Если датаграмма короче ключа сессии
		if(size < 4)
			// Выводим отрицательный результат - датаграмма посторонняя
			return false;
		// Формируем ключ сессии из первых октетов датаграммы
		key = awh::net::origin_key_t(data, 4);
		// Выводим положительный результат
		return true;
	}));
	// Устанавливаем функцию обратного вызова на создание сессии
	this->_io->on(server, static_cast <awh::engine::callback::accept_t> ([&](
		[[maybe_unused]] const awh::event::id_t eid, const awh::event::id_t oid
	) noexcept -> void {
		// Увеличиваем количество созданных сессий
		sessions++;
		// Запоминаем идентификатор события созданной сессии
		session = oid;
		// Устанавливаем функцию обратного вызова на чтение из сессии
		this->_io->on(oid, [&](const awh::event::id_t oid, const uint8_t * data, const size_t size) noexcept -> void {
			// Накапливаем принятую датаграмму вместе с идентификатором принявшей сессии
			received.emplace_back(oid, std::string(reinterpret_cast <const char *> (data), size));
			// Останавливаем тест после приёма всех ожидаемых датаграмм
			stop = (received.size() >= 2);
		});
	}));
	// Устанавливаем таймаут принудительной остановки теста
	this->_io->setTimeout(timer, awh::event::action_t::NONE, 2000);
	// Устанавливаем функцию обратного вызова на событие таймера
	this->_io->on(timer, [&stop]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Останавливаем тест по истечении таймаута
		stop = (stop || (status == awh::event::status_t::SUCCESS));
	});
	// Выполняем фиксацию настроек событий
	ASSERT_TRUE(this->_io->commit(server));
	ASSERT_TRUE(this->_io->commit(timer));
	// Выполняем запуск событий
	ASSERT_TRUE(this->_io->launch(server));
	ASSERT_TRUE(this->_io->launch(timer));
	// Создаём сокет отправителя датаграмм
	const int32_t sock = ::socket(AF_INET, SOCK_DGRAM, 0);
	// Проверяем что сокет отправителя создан
	ASSERT_GE(sock, 0);
	// Объект адреса назначения датаграммы
	struct sockaddr_in target{};
	// Устанавливаем семейство адреса назначения
	target.sin_family = AF_INET;
	// Устанавливаем порт назначения датаграммы
	target.sin_port = htons(port);
	// Устанавливаем адрес назначения датаграммы
	target.sin_addr.s_addr = ::inet_addr("127.0.0.1");
	// Отправляем датаграмму с исходным ключом сессии
	ASSERT_GT(::sendto(sock, "AAAAfirst", 9, 0, reinterpret_cast <struct sockaddr *> (&target), sizeof(target)), 0);
	/**
	 * Выполняем опрос событий до создания сессии
	 */
	while(!stop && (sessions == 0) && this->_io->poll());
	// Проверяем что сессия создана
	ASSERT_EQ(sessions, 1u);
	// Проверяем что идентификатор события сессии получен
	ASSERT_GT(session, 0u);
	/**
	 * Запоминаем идентификатор первой сессии отдельно: обработчик создания
	 * сессии перезаписывает его при каждом принятии
	 */
	const awh::event::id_t primary = session;
	// Формируем дополнительный ключ маршрутизации сессии
	const awh::net::origin_key_t extra(reinterpret_cast <const uint8_t *> ("BBBB"), 4);
	// Привязываем дополнительный ключ к созданной сессии
	ASSERT_TRUE(this->_io->bind(primary, extra));
	// Проверяем что повторная привязка того же ключа к той же сессии допустима
	ASSERT_TRUE(this->_io->bind(primary, extra));
	// Проверяем отказ привязки пустого ключа
	ASSERT_FALSE(this->_io->bind(session, awh::net::origin_key_t()));
	// Проверяем отказ привязки ключа к несуществующей сессии
	ASSERT_FALSE(this->_io->bind(0, extra));
	// Отправляем датаграмму с дополнительным ключом маршрутизации
	ASSERT_GT(::sendto(sock, "BBBBsecond", 10, 0, reinterpret_cast <struct sockaddr *> (&target), sizeof(target)), 0);
	/**
	 *  Выполняем опрос событий до приёма датаграммы
	 */
	while(!stop && this->_io->poll());
	/**
	 * Проверяем что датаграмма с дополнительным ключом попала в ту же сессию:
	 * новая сессия под неё создаваться не должна
	 */
	ASSERT_EQ(sessions, 1u);
	// Проверяем что принято две датаграммы
	ASSERT_EQ(received.size(), 2u);
	// Проверяем что обе датаграммы приняты одной и той же сессией
	ASSERT_EQ(received[0].first, primary);
	ASSERT_EQ(received[1].first, primary);
	// Проверяем содержимое принятых сессией датаграмм
	ASSERT_EQ(received[0].second, "AAAAfirst");
	ASSERT_EQ(received[1].second, "BBBBsecond");
	// Снимаем дополнительный ключ с сессии
	ASSERT_TRUE(this->_io->unbind(primary, extra));
	// Проверяем что повторное снятие того же ключа отвергается
	ASSERT_FALSE(this->_io->unbind(primary, extra));
	// Сбрасываем флаг остановки теста
	stop = false;
	// Отправляем датаграмму со снятым с маршрутизации ключом
	ASSERT_GT(::sendto(sock, "BBBBthird", 9, 0, reinterpret_cast <struct sockaddr *> (&target), sizeof(target)), 0);
	/**
	 *  Выполняем опрос событий до создания новой сессии
	 */
	while(!stop && (sessions < 2) && this->_io->poll());
	// Закрываем сокет отправителя
	::closesocket(sock);
	/**
	 * Проверяем что снятый ключ маршрутизироваться перестал: датаграмма с ним
	 * образовала новую сессию, а не попала в прежнюю
	 */
	ASSERT_EQ(sessions, 2u);
	// Проверяем что датаграмма со снятым ключом принята
	ASSERT_EQ(received.size(), 3u);
	// Проверяем что приняла её другая сессия, а не прежняя
	ASSERT_NE(received[2].first, primary);
	// Проверяем содержимое датаграммы со снятым ключом
	ASSERT_EQ(received[2].second, "BBBBthird");
}

/**
 * @brief Продолжительность окна нагрузки тестов ограничения полосы в миллисекундах
 *
 * @details Ведро токенов доливается непрерывно, а расходуется порциями по размеру
 *          полезной нагрузки кадра, поэтому на коротком окне измерялось бы в
 *          основном начальное наполнение ведра, а не работа ограничителя
 *
 */
static constexpr uint32_t BANDWIDTH_WINDOW = 1000;
/**
 * @brief Предельная продолжительность дослива очереди после окна нагрузки в миллисекундах
 *
 * @details После окончания нагрузки очередь отправки остаётся непустой, и её
 *          дослив идёт с той же ограниченной скоростью. Проверка байт-в-байт
 *          требует дождаться именно дослива, а не оборвать передачу по окну
 *
 */
static constexpr uint32_t BANDWIDTH_DRAIN = 15000;
/**
 * @brief Размер блока постановки в очередь тестов ограничения полосы
 *
 */
static constexpr size_t BANDWIDTH_CHUNK = 16384;
/**
 * @brief Глубина запаса очереди отправки тестов ограничения полосы
 *
 * @details Очередь обязана оставаться непустой всё время окна нагрузки, иначе
 *          измерялся бы темп подачи данных тестом, а не работа ограничителя.
 *          Запас при этом ограничен: неограниченная подача выбрала бы всю
 *          память процесса за секунду, потому что ограничитель отдаёт в сокет
 *          медленнее, чем тест ставит в очередь
 *
 */
static constexpr size_t BANDWIDTH_BACKLOG = (BANDWIDTH_CHUNK * 8);

/**
 * @brief Функция получения октета образца передачи по его смещению
 *
 * @details Образец детерминирован и не повторяется на длине блока: приёмник
 *          проверяет по нему не только объём принятого, но и порядок октетов,
 *          так что перестановка или потеря внутри потока была бы обнаружена
 *
 * @param offset смещение октета в потоке передачи
 * @return       значение октета образца
 *
 */
static uint8_t sample(const size_t offset) noexcept {
	// Выводим значение октета образца
	return static_cast <uint8_t> (((offset * 31) + 7) & 0xFF);
}

/**
 * @brief Структура описания одного потребителя теста ограничения полосы
 *
 */
typedef struct Bandwidth_Consumer {
	// Предел на отправку данных, пустая строка - без предела
	std::string egress;
	// Предел на приём данных, пустая строка - без предела
	std::string ingress;
	// Идентификатор события отправителя
	awh::event::id_t sender;
	// Идентификатор события приёмника
	awh::event::id_t receiver;
	// Количество принятых событием отправителя в очередь октетов
	size_t queued;
	// Количество принятых приёмником октетов
	size_t received;
	// Количество вернувшихся отправителю октетов встречного потока
	size_t returned;
	// Количество октетов, поставленных в очередь и ещё не записанных
	size_t backlog;
	// Флаг готовности отправителя к подаче данных
	bool ready;
	// Флаг обнаружения расхождения принятых данных с образцом
	bool corrupted;
	// Достигнутая скорость передачи в октетах в секунду
	double rate;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit Bandwidth_Consumer() noexcept :
	 egress{""}, ingress{""}, sender(0), receiver(0),
	 queued(0), received(0), returned(0), backlog(0), ready(false), corrupted(false), rate(0.0) {}
	/**
	 * @brief Конструктор
	 *
	 * @param egress  предел на отправку данных
	 * @param ingress предел на приём данных
	 *
	 */
	explicit Bandwidth_Consumer(const std::string & egress, const std::string & ingress) noexcept :
	 egress(egress), ingress(ingress), sender(0), receiver(0),
	 queued(0), received(0), returned(0), backlog(0), ready(false), corrupted(false), rate(0.0) {}
} bandwidth_consumer_t;

/**
 * @brief Функция прогона нагрузки с ограничением пропускной способности
 *
 * @details Каждый потребитель это отдельная пара подключений: отправитель
 *          непрерывно держит очередь непустой, приёмник сверяет принятое с
 *          образцом и считает октеты. Нагрузка идёт заданное окно, после чего
 *          подача прекращается и опрос продолжается до полного дослива очередей:
 *          проверка байт-в-байт требует дождаться именно дослива
 *
 * @param io        объект асинхронного движка ввода-вывода
 * @param consumers набор потребителей нагрузки
 * @param echo      флаг возврата принятого приёмником обратно отправителю
 * @return          результат выполнения прогона
 *
 */
static bool bandwidth(awh::engine::io_t * io, std::vector <bandwidth_consumer_t> & consumers, const bool echo = false) noexcept {
	// Флаг окончания окна нагрузки
	bool ceased = false;
	// Флаг остановки прогона
	bool stop = false;
	// Блок передаваемых данных
	static std::vector <uint8_t> chunk;
	// Если блок передаваемых данных ещё не подготовлен
	if(chunk.empty()){
		// Выделяем память под блок передаваемых данных
		chunk.resize(BANDWIDTH_CHUNK, 0);
		/**
		 * Заполняем блок образцом передачи
		 */
		for(size_t i = 0; i < BANDWIDTH_CHUNK; i++)
			// Устанавливаем очередной октет образца
			chunk[i] = ::sample(i);
	}
	// Выполняем генерацию порта
	const uint16_t number = ::port();
	// Набор опций событий прогона
	const uint16_t options = (
		awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE |
		awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK |
		awh::event::options::CLOSE_ON_EXEC | awh::event::options::TCP_NO_DELAY
	);
	// Добавляем событие сервера
	const awh::event::id_t server = io->event(awh::event::node_t::SERVER, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
	// Добавляем событие таймера окончания окна нагрузки
	const awh::event::id_t timer = io->event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
	// Устанавливаем порт события сервера
	io->setSourcePort(server, number);
	// Инициализируем асинхронный движок ввода-вывода
	if(!io->initialize())
		// Выводим отрицательный результат
		return false;
	// Устанавливаем опции события сервера
	io->setOptions(server, options);
	// Устанавливаем адрес события сервера
	io->setAddress(server, awh::event::address_t::IPV4, "127.0.0.1");
	// Количество принятых сервером подключений
	size_t accepted = 0;
	// Устанавливаем функцию обратного вызова на принятие входящего подключения
	io->on(server, static_cast <awh::engine::callback::accept_t> ([io, &consumers, &accepted, options, echo](
		[[maybe_unused]] const awh::event::id_t sid, const awh::event::id_t cid
	) noexcept -> void {
		// Если принято больше подключений, чем заведено потребителей
		if(accepted >= consumers.size())
			// Выходим из функции обработки
			return;
		// Получаем потребителя по порядку принятия его подключения
		bandwidth_consumer_t & consumer = consumers[accepted++];
		// Запоминаем идентификатор события приёмника
		consumer.receiver = cid;
		// Устанавливаем опции принятого подключения
		io->setOptions(cid, options);
		// Если приёмнику задан предел пропускной способности
		if(!consumer.ingress.empty())
			// Устанавливаем предел пропускной способности на приём данных
			io->bandwidth(cid, awh::event::limiting_t::INGRESS, consumer.ingress);
		/**
		 * Устанавливаем функцию обратного вызова на чтение из принятого
		 * подключения: приёмник сверяет принятое с образцом передачи
		 */
		io->on(cid, [io, &consumer, echo](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
			/**
			 * Сверяем принятые октеты с образцом передачи
			 */
			for(size_t i = 0; i < size; i++){
				// Если принятый октет расходится с образцом
				if(data[i] != ::sample((consumer.received + i) % BANDWIDTH_CHUNK)){
					// Отмечаем расхождение принятых данных с образцом
					consumer.corrupted = true;
					// Прекращаем сверку
					break;
				}
			}
			// Накапливаем количество принятых октетов
			consumer.received += size;
			// Если включён встречный поток, возвращаем принятое отправителю
			if(echo)
				// Отправляем принятое обратно
				io->send(eid, data, size);
		});
	}));
	// Выполняем фиксацию настроек события сервера
	io->commit(server);
	// Переводим событие сервера в режим прослушивания
	io->listen(server, 512);
	// Запускаем событие сервера
	io->launch(server);
	/**
	 * Функция подачи данных в очередь отправителя
	 *
	 */
	auto feed = [io, &ceased](bandwidth_consumer_t & consumer) noexcept -> void {
		// Если окно нагрузки истекло, данных больше не подаём
		if(ceased)
			// Выходим из функции подачи
			return;
		/**
		 * Держим очередь непустой, но не выбираем под неё всю память процесса
		 */
		while(consumer.backlog < BANDWIDTH_BACKLOG){
			// Ставим в очередь очередной блок передачи
			const size_t accepted = io->send(consumer.sender, chunk.data(), BANDWIDTH_CHUNK);
			// Если блок в очередь не принят
			if(accepted == 0)
				// Прекращаем подачу
				break;
			// Накапливаем количество поставленных в очередь октетов
			consumer.queued += accepted;
			// Накапливаем объём очереди, ожидающий записи
			consumer.backlog += accepted;
		}
	};
	/**
	 * Заводим отправителей всех потребителей нагрузки
	 */
	for(auto & consumer : consumers){
		// Добавляем событие клиента
		const awh::event::id_t client = io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::STREAM, awh::event::protocol_t::TCP);
		// Запоминаем идентификатор события отправителя
		consumer.sender = client;
		// Устанавливаем порт назначения события клиента
		io->setTargetPort(client, number);
		// Устанавливаем опции события клиента
		io->setOptions(client, options);
		// Устанавливаем адрес привязки события клиента
		io->setAddress(client, awh::event::address_t::IPV4, "0.0.0.0");
		// Устанавливаем адрес назначения события клиента
		io->setTarget(client, "127.0.0.1");
		// Если включён встречный поток, считаем вернувшиеся отправителю октеты
		if(echo)
			// Устанавливаем функцию обратного вызова на чтение из события клиента
			io->on(client, [&consumer]([[maybe_unused]] const awh::event::id_t eid, [[maybe_unused]] const uint8_t * data, const size_t size) noexcept -> void {
				// Накапливаем количество вернувшихся октетов
				consumer.returned += size;
			});
		// Устанавливаем функцию обратного вызова на подключение клиента
		io->on(client, static_cast <awh::engine::callback::connect_t> ([io, &consumer](const awh::event::id_t eid, const bool ok) noexcept -> void {
			// Если подключение не выполнено
			if(!ok)
				// Выходим из функции обработки
				return;
			// Если отправителю задан предел пропускной способности
			if(!consumer.egress.empty())
				// Устанавливаем предел пропускной способности на отправку данных
				io->bandwidth(eid, awh::event::limiting_t::EGRESS, consumer.egress);
			// Отмечаем готовность отправителя к подаче данных
			consumer.ready = true;
		}));
		/**
		 * Устанавливаем функцию обратного вызова на запись в событие клиента:
		 * записанный объём освобождает место в очереди, и подача возобновляется
		 */
		io->on(client, static_cast <awh::engine::callback::write_t> ([&consumer](
			[[maybe_unused]] const awh::event::id_t eid, const size_t size
		) noexcept -> void {
			// Уменьшаем объём очереди, ожидающий записи
			consumer.backlog -= ::std::min(consumer.backlog, size);
		}));
		// Выполняем фиксацию настроек события клиента
		io->commit(client);
		// Выполняем подключение события клиента к серверу
		io->connect(client);
		// Выполняем запуск события клиента
		io->launch(client);
	}
	// Устанавливаем таймаут окончания окна нагрузки
	io->setTimeout(timer, awh::event::action_t::NONE, BANDWIDTH_WINDOW);
	// Устанавливаем функцию обратного вызова на событие таймера
	io->on(timer, [&ceased]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Прекращаем подачу данных по истечении окна нагрузки
		ceased = (ceased || (status == awh::event::status_t::SUCCESS));
	});
	// Выполняем фиксацию настроек события таймера
	io->commit(timer);
	// Запускаем событие таймера
	io->launch(timer);
	// Запоминаем момент начала прогона
	const auto start = std::chrono::steady_clock::now();
	// Момент окончания окна нагрузки
	auto ceasing = start;
	/**
	 * Запускаем опрос событий до полного дослива очередей отправки
	 */
	while(!stop && io->poll()){
		// Если окно нагрузки ещё не истекло
		if(!ceased){
			/**
			 * Подаём данные в очереди отправителей всех потребителей нагрузки
			 */
			for(auto & consumer : consumers){
				// Если отправитель готов к подаче данных
				if(consumer.ready)
					// Подаём данные в очередь отправителя
					feed(consumer);
			}
			// Продолжаем опрос событий
			continue;
		}
		// Если момент окончания окна нагрузки ещё не снят
		if(ceasing == start)
			// Запоминаем момент окончания окна нагрузки
			ceasing = std::chrono::steady_clock::now();
		// Флаг завершённости дослива очередей всех потребителей
		bool drained = true;
		/**
		 * Проверяем дослив очередей всех потребителей нагрузки
		 */
		for(const auto & consumer : consumers)
			// Очередь дослита, когда принято ровно столько, сколько поставлено
			drained = (drained && (consumer.received >= consumer.queued));
		// Останавливаем прогон по завершении дослива очередей
		stop = (drained || (std::chrono::duration <double> (std::chrono::steady_clock::now() - ceasing).count() > (BANDWIDTH_DRAIN / 1000.0)));
	}
	// Вычисляем продолжительность окна нагрузки
	const double seconds = std::chrono::duration <double> (std::chrono::steady_clock::now() - start).count();
	/**
	 * Вычисляем достигнутые скорости передачи потребителей нагрузки
	 */
	for(auto & consumer : consumers)
		// Устанавливаем достигнутую скорость передачи
		consumer.rate = ((seconds > 0.0) ? (static_cast <double> (consumer.received) / seconds) : 0.0);
	// Выводим положительный результат
	return true;
}

/**
 * @brief Функция разбора предела пропускной способности в октеты в секунду
 *
 * @param fmk   объект фреймворка
 * @param limit предел пропускной способности
 * @return      предел пропускной способности в октетах в секунду
 *
 */
static double limitBytes(const awh::fmk_t * fmk, const std::string & limit) noexcept {
	// Выводим предел пропускной способности в октетах в секунду
	return static_cast <double> (fmk->bpsSize(limit));
}

/**
 * @brief Тест ограничения пропускной способности только на отправке
 *
 * @details Ограничение полосы устроено как ведро токенов: они доливаются
 *          пропорционально прошедшему времени, а отправка расходует их по числу
 *          переданных октетов. Проверяется, что достигнутая скорость держится
 *          около заданного предела и что весь поставленный в очередь объём
 *          дошёл до приёмника без потерь и без перестановок
 *
 */
TEST_F(IoFixture, IoBandwidthEgressOnlyTest){
	// Заводим единственного потребителя нагрузки с ограничением отправки
	std::vector <bandwidth_consumer_t> consumers = {bandwidth_consumer_t("8Mbps", "")};
	// Выполняем прогон нагрузки
	ASSERT_TRUE(::bandwidth(this->_io.get(), consumers));
	// Получаем заданный предел пропускной способности в октетах в секунду
	const double limit = ::limitBytes(this->_fmk.get(), consumers[0].egress);
	// Проверяем что передача состоялась
	ASSERT_GT(consumers[0].received, 0u);
	// Проверяем что принятое совпадает с образцом передачи
	ASSERT_FALSE(consumers[0].corrupted);
	// Проверяем что весь поставленный в очередь объём принят до октета
	ASSERT_EQ(consumers[0].received, consumers[0].queued);
	// Проверяем что заданный предел не превышен
	ASSERT_LT(consumers[0].rate, (limit * 1.35));
	// Проверяем что передача не задушена существенно ниже предела
	ASSERT_GT(consumers[0].rate, (limit * 0.65));
}

/**
 * @brief Тест ограничения пропускной способности только на приёме
 *
 */
TEST_F(IoFixture, IoBandwidthIngressOnlyTest){
	// Заводим единственного потребителя нагрузки с ограничением приёма
	std::vector <bandwidth_consumer_t> consumers = {bandwidth_consumer_t("", "8Mbps")};
	// Выполняем прогон нагрузки
	ASSERT_TRUE(::bandwidth(this->_io.get(), consumers));
	// Получаем заданный предел пропускной способности в октетах в секунду
	const double limit = ::limitBytes(this->_fmk.get(), consumers[0].ingress);
	// Проверяем что передача состоялась
	ASSERT_GT(consumers[0].received, 0u);
	// Проверяем что принятое совпадает с образцом передачи
	ASSERT_FALSE(consumers[0].corrupted);
	// Проверяем что весь поставленный в очередь объём принят до октета
	ASSERT_EQ(consumers[0].received, consumers[0].queued);
	// Проверяем что заданный предел не превышен
	ASSERT_LT(consumers[0].rate, (limit * 1.35));
	// Проверяем что передача не задушена существенно ниже предела
	ASSERT_GT(consumers[0].rate, (limit * 0.65));
}

/**
 * @brief Тест ограничения пропускной способности на отправке и на приёме разом
 *
 * @details Оба направления одного узла ограничены одновременно, и по обоим идёт
 *          нагрузка: отправитель шлёт непрерывно, приёмник возвращает принятое
 *          обратно. Ведро токенов у направлений своё, и одно не должно
 *          расходовать токены другого
 *
 */
TEST_F(IoFixture, IoBandwidthDuplexTest){
	// Заводим потребителя нагрузки с ограничением обоих направлений
	std::vector <bandwidth_consumer_t> consumers = {bandwidth_consumer_t("8Mbps", "8Mbps")};
	// Выполняем прогон нагрузки со встречным потоком
	ASSERT_TRUE(::bandwidth(this->_io.get(), consumers, true));
	// Получаем заданный предел пропускной способности в октетах в секунду
	const double limit = ::limitBytes(this->_fmk.get(), consumers[0].egress);
	// Проверяем что передача состоялась в прямом направлении
	ASSERT_GT(consumers[0].received, 0u);
	// Проверяем что передача состоялась во встречном направлении
	ASSERT_GT(consumers[0].returned, 0u);
	// Проверяем что принятое совпадает с образцом передачи
	ASSERT_FALSE(consumers[0].corrupted);
	// Проверяем что весь поставленный в очередь объём принят до октета
	ASSERT_EQ(consumers[0].received, consumers[0].queued);
	// Проверяем что заданный предел не превышен в прямом направлении
	ASSERT_LT(consumers[0].rate, (limit * 1.35));
	// Проверяем что передача не задушена существенно ниже предела
	ASSERT_GT(consumers[0].rate, (limit * 0.65));
}

/**
 * @brief Тест независимости пределов пропускной способности разных узлов
 *
 * @details Ради этого ограничение и заведено на узел, а не на движок: три
 *          одновременно нагруженных узла с разными пределами обязаны выдать
 *          каждый свой, а не поделить между собой общий. Проверяется и то, что
 *          ни один из них не встал и не потерял ни октета
 *
 */
TEST_F(IoFixture, IoBandwidthPerNodeTest){
	// Заводим трёх потребителей нагрузки с разными пределами отправки
	std::vector <bandwidth_consumer_t> consumers = {
		bandwidth_consumer_t("8Mbps", ""),
		bandwidth_consumer_t("16Mbps", ""),
		bandwidth_consumer_t("32Mbps", "")
	};
	// Выполняем прогон нагрузки
	ASSERT_TRUE(::bandwidth(this->_io.get(), consumers));
	/**
	 * Проверяем каждого потребителя нагрузки в отдельности
	 */
	for(const auto & consumer : consumers){
		// Получаем заданный предел пропускной способности в октетах в секунду
		const double limit = ::limitBytes(this->_fmk.get(), consumer.egress);
		// Проверяем что передача состоялась
		ASSERT_GT(consumer.received, 0u) << "предел " << consumer.egress;
		// Проверяем что принятое совпадает с образцом передачи
		ASSERT_FALSE(consumer.corrupted) << "предел " << consumer.egress;
		// Проверяем что весь поставленный в очередь объём принят до октета
		ASSERT_EQ(consumer.received, consumer.queued) << "предел " << consumer.egress;
		// Проверяем что заданный предел не превышен
		ASSERT_LT(consumer.rate, (limit * 1.35)) << "предел " << consumer.egress;
		// Проверяем что передача не задушена существенно ниже предела
		ASSERT_GT(consumer.rate, (limit * 0.65)) << "предел " << consumer.egress;
	}
	// Проверяем что пределы соблюдены в отношении друг к другу, а не совпали
	ASSERT_GT(consumers[1].rate, (consumers[0].rate * 1.5));
	ASSERT_GT(consumers[2].rate, (consumers[1].rate * 1.5));
}

/**
 * @brief Тест передачи без ограничения пропускной способности
 *
 * @details Контрольный прогон: без заданного предела петлевой интерфейс выдаёт
 *          на порядки больше, и совпадение его показателя с ограниченным
 *          означало бы, что предел не применяется вовсе
 *
 */
TEST_F(IoFixture, IoBandwidthUnlimitedTest){
	// Заводим потребителя нагрузки без ограничения
	std::vector <bandwidth_consumer_t> consumers = {bandwidth_consumer_t("", "")};
	// Выполняем прогон нагрузки
	ASSERT_TRUE(::bandwidth(this->_io.get(), consumers));
	// Проверяем что передача состоялась
	ASSERT_GT(consumers[0].received, 0u);
	// Проверяем что принятое совпадает с образцом передачи
	ASSERT_FALSE(consumers[0].corrupted);
	// Проверяем что весь поставленный в очередь объём принят до октета
	ASSERT_EQ(consumers[0].received, consumers[0].queued);
	// Проверяем что без предела передача многократно быстрее ограниченной
	ASSERT_GT(consumers[0].rate, (::limitBytes(this->_fmk.get(), "8Mbps") * 5.0));
}

/**
 * @brief Тест глобального ограничения пропускной способности на отправке
 *
 * @details Глобальное ограничение устроено иначе поузлового: вместо ведра
 *          токенов на событие движок засыпает на время, нужное переданному
 *          объёму при заданной скорости. Ограничение общее на весь движок,
 *          поэтому суммарная скорость всех событий обязана уложиться в предел
 *
 */
TEST_F(IoFixture, IoBandwidthGlobalEgressTest){
	// Устанавливаем глобальный предел пропускной способности на отправку
	this->_io->bandwidth(awh::event::limiting_t::EGRESS, "8Mbps");
	// Заводим потребителя нагрузки без поузлового ограничения
	std::vector <bandwidth_consumer_t> consumers = {bandwidth_consumer_t("", "")};
	// Выполняем прогон нагрузки
	ASSERT_TRUE(::bandwidth(this->_io.get(), consumers));
	// Снимаем глобальный предел пропускной способности
	this->_io->bandwidth(awh::event::limiting_t::EGRESS, "auto");
	// Получаем заданный предел пропускной способности в октетах в секунду
	const double limit = ::limitBytes(this->_fmk.get(), "8Mbps");
	// Проверяем что передача состоялась
	ASSERT_GT(consumers[0].received, 0u);
	// Проверяем что принятое совпадает с образцом передачи
	ASSERT_FALSE(consumers[0].corrupted);
	// Проверяем что весь поставленный в очередь объём принят до октета
	ASSERT_EQ(consumers[0].received, consumers[0].queued);
	// Проверяем что заданный предел не превышен
	ASSERT_LT(consumers[0].rate, (limit * 1.35));
}

/**
 * @brief Тест глобального ограничения пропускной способности на приёме
 *
 */
TEST_F(IoFixture, IoBandwidthGlobalIngressTest){
	// Устанавливаем глобальный предел пропускной способности на приём
	this->_io->bandwidth(awh::event::limiting_t::INGRESS, "8Mbps");
	// Заводим потребителя нагрузки без поузлового ограничения
	std::vector <bandwidth_consumer_t> consumers = {bandwidth_consumer_t("", "")};
	// Выполняем прогон нагрузки
	ASSERT_TRUE(::bandwidth(this->_io.get(), consumers));
	// Снимаем глобальный предел пропускной способности
	this->_io->bandwidth(awh::event::limiting_t::INGRESS, "auto");
	// Получаем заданный предел пропускной способности в октетах в секунду
	const double limit = ::limitBytes(this->_fmk.get(), "8Mbps");
	// Проверяем что передача состоялась
	ASSERT_GT(consumers[0].received, 0u);
	// Проверяем что принятое совпадает с образцом передачи
	ASSERT_FALSE(consumers[0].corrupted);
	// Проверяем что весь поставленный в очередь объём принят до октета
	ASSERT_EQ(consumers[0].received, consumers[0].queued);
	// Проверяем что заданный предел не превышен
	ASSERT_LT(consumers[0].rate, (limit * 1.35));
}

/**
 * @brief Тест глобального ограничения пропускной способности в обоих направлениях
 *
 */
TEST_F(IoFixture, IoBandwidthGlobalDuplexTest){
	// Устанавливаем глобальный предел пропускной способности на отправку
	this->_io->bandwidth(awh::event::limiting_t::EGRESS, "8Mbps");
	// Устанавливаем глобальный предел пропускной способности на приём
	this->_io->bandwidth(awh::event::limiting_t::INGRESS, "8Mbps");
	// Заводим потребителя нагрузки без поузлового ограничения
	std::vector <bandwidth_consumer_t> consumers = {bandwidth_consumer_t("", "")};
	// Выполняем прогон нагрузки
	ASSERT_TRUE(::bandwidth(this->_io.get(), consumers));
	// Снимаем глобальные пределы пропускной способности
	this->_io->bandwidth(awh::event::limiting_t::EGRESS, "auto");
	this->_io->bandwidth(awh::event::limiting_t::INGRESS, "auto");
	// Получаем заданный предел пропускной способности в октетах в секунду
	const double limit = ::limitBytes(this->_fmk.get(), "8Mbps");
	// Проверяем что передача состоялась
	ASSERT_GT(consumers[0].received, 0u);
	// Проверяем что принятое совпадает с образцом передачи
	ASSERT_FALSE(consumers[0].corrupted);
	// Проверяем что весь поставленный в очередь объём принят до октета
	ASSERT_EQ(consumers[0].received, consumers[0].queued);
	// Проверяем что заданный предел не превышен
	ASSERT_LT(consumers[0].rate, (limit * 1.35));
}

/**
 * @brief Тест глобального ограничения пропускной способности на нескольких узлах
 *
 * @details Ограничение это общее на весь движок, и проверять его одним узлом
 *          недостаточно: одиночный узел уложится в предел и при поузловом
 *          ограничителе. Смысл глобального предела в том, что суммарная
 *          скорость всех узлов разом обязана в него уложиться, а поделиться
 *          между ними она может как угодно
 *
 */
TEST_F(IoFixture, IoBandwidthGlobalSharedTest){
	// Устанавливаем глобальный предел пропускной способности на отправку
	this->_io->bandwidth(awh::event::limiting_t::EGRESS, "8Mbps");
	// Заводим три потребителя нагрузки без поузлового ограничения
	std::vector <bandwidth_consumer_t> consumers = {
		bandwidth_consumer_t("", ""),
		bandwidth_consumer_t("", ""),
		bandwidth_consumer_t("", "")
	};
	// Выполняем прогон нагрузки
	ASSERT_TRUE(::bandwidth(this->_io.get(), consumers));
	// Снимаем глобальный предел пропускной способности
	this->_io->bandwidth(awh::event::limiting_t::EGRESS, "auto");
	// Получаем заданный предел пропускной способности в октетах в секунду
	const double limit = ::limitBytes(this->_fmk.get(), "8Mbps");
	// Суммарная достигнутая скорость всех потребителей нагрузки
	double total = 0.0;
	// Роспись объёмов по узлам для сообщения об отказе
	std::string sign = "";
	/**
	 * Собираем роспись объёмов, принятых каждым узлом
	 */
	for(size_t i = 0; i < consumers.size(); i++)
		// Добавляем объём очередного узла в роспись
		sign.append(" узел " + std::to_string(i) + ": " + std::to_string(consumers[i].received) + " октет;");
	/**
	 * Проверяем каждого потребителя нагрузки в отдельности
	 */
	for(size_t i = 0; i < consumers.size(); i++){
		// Получаем очередного потребителя нагрузки
		const auto & consumer = consumers[i];
		// Проверяем что передача состоялась
		ASSERT_GT(consumer.received, 0u) << "узел: " << i << ", роспись:" << sign;
		// Проверяем что принятое совпадает с образцом передачи
		ASSERT_FALSE(consumer.corrupted) << "узел: " << i;
		// Проверяем что весь поставленный в очередь объём принят до октета
		ASSERT_EQ(consumer.received, consumer.queued) << "узел: " << i;
		// Накапливаем суммарную достигнутую скорость
		total += consumer.rate;
	}
	// Проверяем что суммарная скорость всех узлов уложилась в общий предел
	ASSERT_LT(total, (limit * 1.35)) << "роспись:" << sign;
}

/**
 * @brief Функция подбора глухого адреса для проверок срока ожидания подключения
 *
 * @details Адрес берётся из канального блока (RFC 3927): он никуда не
 *          маршрутизируется, и подключение к нему остаётся ожидающим ровно столько,
 *          сколько нужно для истечения срока. Блоки, отведённые под примеры, для
 *          этого не годятся - сеть с туннелем перехватывает их и отвечает согласием
 *
 * @note Адрес складывается из номера процесса и счётчика обращений, и это
 *       существенно. Ядро запоминает неудачную запись соседа и после неё отвечает на
 *       подключение отказом **немедленно**, а не ожиданием, - проверка по такому
 *       адресу проверяла бы уже не то. Номер процесса разводит прогоны между собой,
 *       счётчик - проверки внутри одного прогона: цикл событий у них общий, он один
 *       на весь процесс, и общим оказался бы и адрес
 *
 * @return глухой адрес канального блока
 *
 */
static std::string deafAddress() noexcept {
	// Счётчик обращений за глухим адресом
	static uint32_t count = 0;
	// Получаем номер текущего процесса
	const uint32_t pid = static_cast <uint32_t> (::getpid());
	// Складываем адрес канального блока из номера процесса и счётчика обращений
	return std::string("169.254.") + std::to_string(((pid >> 8) & 0xFF) | 0x01) + "." + std::to_string(((pid + (++count)) & 0xFF) | 0x01);
}

/**
 * @brief Тест прерывания ожидающего подключения по истечении срока ожидания
 *
 * @details Срок ожидания подключения истёк - подключение не состоялось, и попытку
 *          нужно прервать. Узел при этом вправе остаться жить: вызывающий отвечает
 *          на срок ожидания отказом от уничтожения, чтобы событие можно было
 *          использовать под следующий запрос, не создавая нового
 *
 *          Прежде ожидающее подключение в этом случае доводилось до конца уже после
 *          объявленного отказа, и вызывающий получал `ok=true` следом за `ok=false`
 *
 * @note Получатель взят из канального блока (RFC 3927): такой адрес никуда не
 *       маршрутизируется, и подключение к нему остаётся ожидающим ровно столько,
 *       сколько нужно для истечения срока. Блоки, отведённые под примеры, для этого
 *       не годятся: сеть с туннелем перехватывает их и отвечает согласием
 *
 *       Окружение, где такой адрес отвечает отказом сразу, для проверки не годится
 *       вовсе, и тест в нём пропускается: ожидающего подключения там не возникает
 *
 */
TEST_F(IoFixture, IoConnectTimeoutAbandonsPendingTest){
	uint8_t failures = 0, successes = 0, expirations = 0;
	const awh::event::id_t eid = this->_io->event(
		awh::event::node_t::CLIENT, awh::event::family_t::IPV4,
		awh::event::type_t::STREAM, awh::event::protocol_t::TCP
	);
	ASSERT_GT(eid, 0u);
	const awh::event::id_t tick = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
	ASSERT_GT(tick, 0u);
	ASSERT_TRUE(this->_io->initialize());
	ASSERT_TRUE(this->_io->setOptions(eid, awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::TCP_NO_DELAY));
	ASSERT_TRUE(this->_io->setTarget(eid, deafAddress()));
	ASSERT_TRUE(this->_io->setTargetPort(eid, 8080));
	this->_io->setTimeout(eid, awh::event::action_t::CONNECT, 500);
	this->_io->setTimeout(tick, awh::event::action_t::NONE, 200);
	this->_io->on(eid, static_cast <awh::engine::callback::connect_t> (
		[&failures, &successes]([[maybe_unused]] const awh::event::id_t eid, const bool ok) noexcept -> void {
			if(ok) successes++; else failures++;
		}
	));
	this->_io->on(eid, static_cast <awh::engine::callback::timeout_t> (
		[&expirations]([[maybe_unused]] const awh::event::id_t eid, const awh::event::action_t action, [[maybe_unused]] const uint32_t delay) noexcept -> bool {
			if(action == awh::event::action_t::CONNECT) expirations++;
			return false;
		}
	));
	ASSERT_TRUE(this->_io->commit(eid));
	ASSERT_TRUE(this->_io->commit(tick));
	/**
	 * Подключение к глухому адресу должно уйти в **ожидание** - только тогда истечёт
	 * срок. Если ядро отвергает его сразу, значит, оно уже держит неудачную запись
	 * соседа по этому адресу, и опыт поставить не на чем: окружение для него не
	 * годится, и проверка пропускается
	 */
	if(!this->_io->connect(eid)){
		// Уничтожаем заведённые события
		this->_io->destroy(eid);
		this->_io->destroy(tick);
		// Завершаем работу движка
		this->_io->deinitialize();
		// Пропускаем проверку
		GTEST_SKIP() << "ядро отвергает подключение к глухому адресу немедленно";
	}
	ASSERT_TRUE(this->_io->launch(eid));
	ASSERT_TRUE(this->_io->launch(tick));
	const auto start = std::chrono::steady_clock::now();
	while((std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 3000) && this->_io->poll());
	const std::string sign = std::string("сроков=") + std::to_string(expirations) + " отказов=" + std::to_string(failures) + " удач=" + std::to_string(successes);
	/**
	 * Срок ожидания срабатывает по меньшей мере однажды. Больше - если новая попытка,
	 * начатая движком после отказа вызывающего, снова успела уйти в ожидание; ровно
	 * один, если ядро отвергло её сразу, придержав неудачную запись соседа. Оба исхода
	 * законны, и утверждать точное число значило бы утверждать поведение ядра
	 */
	ASSERT_GE(expirations, 1) << sign;
	ASSERT_GE(failures, 1) << sign;
	// Удачного подключения к глухому адресу быть не может ни разу
	ASSERT_EQ(0, successes) << sign;
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест возврата потокового события в работу перестройкой
 *
 * @details Перестройка сохраняет идентификатор события со всеми внесёнными в него
 *          настройками и пересоздаёт нижележащий дескриптор, переигрывая одну лишь
 *          фиксацию. Подключение и запуск остаются за вызывающим, и порядок возврата
 *          таков: `rebuild()` -> `connect()` -> `launch()`
 *
 * @note Проверка ведётся на живом получателе - слушающем сокете на петле, заведённом
 *       прямо здесь. Глухой адрес для этого не годится: ядро запоминает неудачную
 *       запись соседа, и второе подключение к нему отвергается немедленно, отчего
 *       проверялось бы поведение ядра, а не оживление события
 *
 */
/**
 * @brief Тест согласия перегрузок запроса адреса на нулевом источнике
 *
 * @details Клиент, которому источник явно не задавали, держит нулевой адрес, и означает
 *          он не устройство, а согласие отдать выбор ядру. Строковая перегрузка это
 *          понимает и выводит адрес устройства по умолчанию; вторая, отдающая адрес
 *          готовой строением, копировала источник как есть и возвращала голый ноль
 *
 *          Отличить такой ответ от настоящего адреса вызывающему нечем: перегрузка
 *          отвечает согласием и размером в четыре октета. Договор `PCP` требует класть
 *          свой адрес внутрь пакета, маршрутизатор сверяет его с адресом отправителя, и
 *          на нуле любая просьба получала отказ несовпадением адресов
 *
 * @note Проверяется именно **согласие** двух перегрузок, а не само значение: какой адрес
 *       у устройства по умолчанию, знает лишь машина, где идёт проверка, и утверждать
 *       его наперёд значило бы проверять стенд, а не движок
 *
 */
TEST_F(IoFixture, IoAddressOverloadsAgreeOnUnsetSourceTest){
	const awh::event::id_t eid = this->_io->event(
		awh::event::node_t::CLIENT, awh::event::family_t::IPV4,
		awh::event::type_t::STREAM, awh::event::protocol_t::TCP
	);
	ASSERT_GT(eid, 0u);
	ASSERT_TRUE(this->_io->initialize());
	ASSERT_TRUE(this->_io->setOptions(eid, awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK));
	ASSERT_TRUE(this->_io->setTarget(eid, "127.0.0.1"));
	ASSERT_TRUE(this->_io->setTargetPort(eid, 8080));
	ASSERT_TRUE(this->_io->commit(eid));
	// Запрашиваем адрес строкой
	const std::string text = this->_io->getAddress(eid, awh::event::address_t::IPV4);
	// Запрашиваем тот же адрес строением
	std::unique_ptr <awh::net::addr_t> value = nullptr;
	const bool taken = this->_io->getAddress(eid, awh::event::address_t::IPV4, value);
	const std::string sign = std::string("строкой=[") + text + "] строением=" + (taken ? "да" : "нет");
	ASSERT_TRUE(taken) << sign;
	ASSERT_NE(nullptr, value) << sign;
	ASSERT_EQ(4u, value->size) << sign;
	/**
	 * Источник события не задавали, значит обе перегрузки обязаны вывести адрес
	 * устройства по умолчанию. Голый ноль означал бы, что вторая перегрузка отдала
	 * согласие ядру вместо адреса
	 */
	ASSERT_NE(0u, awh_cast <awh::net::addr_net_ipv4_t *> (value.get())->address) << sign;
	// Сличаем ответы обеих перегрузок между собой
	awh::net_addr_t addr(this->_fmk.get(), this->_log.get());
	addr.source(value.get(), awh::net_addr_t::endian_t::LITTLE);
	ASSERT_EQ(text, static_cast <std::string> (addr)) << sign;
	this->_io->destroy(eid);
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест уничтожения события с самостоятельным переподключением по воле вызывающего
 *
 * @details Обрыв связи у события с опцией `AUTO_RECONNECT` разворачивается в подъём, и
 *          разворачивает его само уничтожение - тем и жива круглосуточная связь. Но у
 *          уничтожения, затребованного вызывающим напрямую, смысл обратный: событие
 *          просят убрать, а не поднять
 *
 *          Прежде убрать его было нечем вовсе: обращение к уничтожению возрождало
 *          событие вместо того, чтобы его снять, и так на каждое обращение. Череда
 *          подъёмов при этом бесконечна намеренно - числа попыток движок не ведёт, - и
 *          ручное уничтожение остаётся единственным способом её прекратить
 *
 * @note Признаком служит подъём после уничтожения: без починки взведённый срок
 *       переподключения выстреливает и приводит событие обратно, объявляя возрождение и
 *       новое подключение. Проверка ведётся на живом получателе - слушающем сокете на
 *       петле, - чтобы связь встала наверняка, а обрыв случился по нашей воле
 *
 */
TEST_F(IoFixture, IoDestroyOverridesAutoReconnectTest){
	uint8_t successes = 0, rebirths = 0;
	// Заводим слушающий сокет на петле
	const int32_t listener = ::socket(AF_INET, SOCK_STREAM, 0);
	ASSERT_GT(listener, 0);
	struct sockaddr_in host{};
	/**
	 * Поле длины записи адреса заводят лишь системы происхождения BSD: у Linux и
	 * MS Windows его нет вовсе, а нужным оно не является нигде - длина подаётся
	 * вызовам отдельным доводом
	 */
	#if __APPLE__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
		host.sin_len = sizeof(host);
	#endif
	host.sin_family = AF_INET;
	host.sin_port = 0;
	::inet_pton(AF_INET, "127.0.0.1", &host.sin_addr);
	ASSERT_EQ(0, ::bind(listener, reinterpret_cast <struct sockaddr *> (&host), sizeof(host)));
	socklen_t length = sizeof(host);
	ASSERT_EQ(0, ::getsockname(listener, reinterpret_cast <struct sockaddr *> (&host), &length));
	ASSERT_EQ(0, ::listen(listener, 16));
	// Заводим событие клиента и тикающий интервал
	const awh::event::id_t eid = this->_io->event(
		awh::event::node_t::CLIENT, awh::event::family_t::IPV4,
		awh::event::type_t::STREAM, awh::event::protocol_t::TCP
	);
	ASSERT_GT(eid, 0u);
	const awh::event::id_t tick = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
	ASSERT_GT(tick, 0u);
	ASSERT_TRUE(this->_io->initialize());
	ASSERT_TRUE(this->_io->setOptions(eid, awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::TCP_NO_DELAY | awh::event::options::AUTO_RECONNECT));
	ASSERT_TRUE(this->_io->setTarget(eid, "127.0.0.1"));
	ASSERT_TRUE(this->_io->setTargetPort(eid, ntohs(host.sin_port)));
	this->_io->setTimeout(eid, awh::event::action_t::RECONNECT, 200);
	this->_io->setTimeout(tick, awh::event::action_t::NONE, 100);
	this->_io->on(eid, [&rebirths]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Считаем возрождения события, которыми движок объявляет о подъёме
		if(status == awh::event::status_t::REBIRTHED) rebirths++;
	});
	this->_io->on(eid, static_cast <awh::engine::callback::connect_t> (
		[&successes]([[maybe_unused]] const awh::event::id_t eid, const bool ok) noexcept -> void {
			if(ok) successes++;
		}
	));
	ASSERT_TRUE(this->_io->commit(eid));
	ASSERT_TRUE(this->_io->commit(tick));
	ASSERT_TRUE(this->_io->connect(eid));
	ASSERT_TRUE(this->_io->launch(eid));
	ASSERT_TRUE(this->_io->launch(tick));
	// Дожидаемся подключения
	auto start = std::chrono::steady_clock::now();
	while((successes < 1) && (std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 2000) && this->_io->poll());
	ASSERT_EQ(1, successes);
	// Уничтожаем событие по своей воле, не снимая опции самостоятельного переподключения
	this->_io->destroy(eid);
	// Запоминаем счёт подъёмов на миг уничтожения
	const uint8_t reborn = rebirths, connected = successes;
	// Крутим цикл заведомо дольше срока переподключения, давая подъёму случиться
	start = std::chrono::steady_clock::now();
	while((std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 2000) && this->_io->poll());
	const std::string sign = std::string("возрождений=") + std::to_string(rebirths - reborn) +
		" подключений=" + std::to_string(successes - connected);
	// Уничтоженное по воле вызывающего событие не вправе подняться ни разу
	ASSERT_EQ(reborn, rebirths) << sign;
	ASSERT_EQ(connected, successes) << sign;
	// Закрываем слушающий сокет
	::closesocket(listener);
	this->_io->destroy(tick);
	ASSERT_TRUE(this->_io->deinitialize());
}

TEST_F(IoFixture, IoRebuildRevivesClientTest){
	uint8_t failures = 0, successes = 0;
	// Заводим слушающий сокет на петле
	const int32_t listener = ::socket(AF_INET, SOCK_STREAM, 0);
	ASSERT_GT(listener, 0);
	struct sockaddr_in host{};
	/**
	 * Поле длины записи адреса заводят лишь системы происхождения BSD: у Linux и
	 * MS Windows его нет вовсе, а нужным оно не является нигде - длина подаётся
	 * вызовам отдельным доводом
	 */
	#if __APPLE__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
		host.sin_len = sizeof(host);
	#endif
	host.sin_family = AF_INET;
	host.sin_port = 0;
	::inet_pton(AF_INET, "127.0.0.1", &host.sin_addr);
	ASSERT_EQ(0, ::bind(listener, reinterpret_cast <struct sockaddr *> (&host), sizeof(host)));
	socklen_t length = sizeof(host);
	ASSERT_EQ(0, ::getsockname(listener, reinterpret_cast <struct sockaddr *> (&host), &length));
	ASSERT_EQ(0, ::listen(listener, 16));
	// Заводим событие клиента и тикающий интервал
	const awh::event::id_t eid = this->_io->event(
		awh::event::node_t::CLIENT, awh::event::family_t::IPV4,
		awh::event::type_t::STREAM, awh::event::protocol_t::TCP
	);
	ASSERT_GT(eid, 0u);
	const awh::event::id_t tick = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
	ASSERT_GT(tick, 0u);
	ASSERT_TRUE(this->_io->initialize());
	ASSERT_TRUE(this->_io->setOptions(eid, awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::TCP_NO_DELAY));
	ASSERT_TRUE(this->_io->setTarget(eid, "127.0.0.1"));
	ASSERT_TRUE(this->_io->setTargetPort(eid, ntohs(host.sin_port)));
	this->_io->setTimeout(tick, awh::event::action_t::NONE, 100);
	this->_io->on(eid, static_cast <awh::engine::callback::connect_t> (
		[&failures, &successes]([[maybe_unused]] const awh::event::id_t eid, const bool ok) noexcept -> void {
			if(ok) successes++; else failures++;
		}
	));
	ASSERT_TRUE(this->_io->commit(eid));
	ASSERT_TRUE(this->_io->commit(tick));
	ASSERT_TRUE(this->_io->connect(eid));
	ASSERT_TRUE(this->_io->launch(eid));
	ASSERT_TRUE(this->_io->launch(tick));
	// Дожидаемся первого подключения
	auto start = std::chrono::steady_clock::now();
	while((successes < 1) && (std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 2000) && this->_io->poll());
	ASSERT_EQ(1, successes);
	// Возвращаем событие в работу перестройкой
	const bool rebuilt = this->_io->rebuild(eid);
	const bool reconnected = (rebuilt && this->_io->connect(eid));
	const bool relaunched = (reconnected && this->_io->launch(eid));
	// Дожидаемся второго подключения
	start = std::chrono::steady_clock::now();
	while((successes < 2) && (std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 2000) && this->_io->poll());
	const std::string sign = std::string("перестройка=") + (rebuilt ? "да" : "нет") +
		" подключение=" + (reconnected ? "да" : "нет") + " запуск=" + (relaunched ? "да" : "нет") +
		" удач=" + std::to_string(successes) + " отказов=" + std::to_string(failures);
	ASSERT_TRUE(rebuilt) << sign;
	ASSERT_TRUE(reconnected) << sign;
	ASSERT_TRUE(relaunched) << sign;
	// Перестроенное событие обязано подключиться заново
	ASSERT_EQ(2, successes) << sign;
	this->_io->destroy(eid);
	this->_io->destroy(tick);
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест подъёма события с самостоятельным переподключением после обрыва по сроку ожидания
 *
 * @details Обрыв ожидающего подключения закрывает дескриптор напрямую, минуя
 *          уничтожение. Событию с `AUTO_RECONNECT` это было гибелью: ход
 *          переподключения заводится именно уничтожением, и такое событие
 *          обрывалось, не поднимаясь уже никогда - хотя ради подъёма опция и
 *          ставится. Опыт до починки давал `сроков=1 переподключений=0`
 *
 * @note Ответ на срок переподключения читается **наоборот** ответу на прочие сроки:
 *       отказ отменяет переподключение, а не сохраняет событие
 *
 * @note Попытка подъёма срывается двояко, и обе развязки череду продолжают. Либо
 *       подключение уходит в ожидание и истекает по сроку, либо ядро отвечает отказом
 *       немедленно - так бывает, когда оно уже держит неудачную запись соседа. Прежде
 *       второй случай был для события смертью: подъём обрывался навсегда, хотя опция
 *       ставится ровно ради того, чтобы пробовать снова
 *
 *       Роспись состояний в таком прогоне читается как
 *       `INITIAL -> PENDING -> RECONNECTED -> INITIAL -> REBIRTHED -> FAILURE`, и за
 *       последним отказом теперь следует новая попытка
 *
 */
TEST_F(IoFixture, IoAutoReconnectAfterConnectTimeoutTest){
	uint8_t failures = 0, successes = 0, expirations = 0, reconnects = 0;
	std::string trace = "";
	const awh::event::id_t eid = this->_io->event(
		awh::event::node_t::CLIENT, awh::event::family_t::IPV4,
		awh::event::type_t::STREAM, awh::event::protocol_t::TCP
	);
	ASSERT_GT(eid, 0u);
	const awh::event::id_t tick = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
	ASSERT_GT(tick, 0u);
	ASSERT_TRUE(this->_io->initialize());
	ASSERT_TRUE(this->_io->setOptions(eid, awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::TCP_NO_DELAY | awh::event::options::AUTO_RECONNECT));
	ASSERT_TRUE(this->_io->setTarget(eid, deafAddress()));
	ASSERT_TRUE(this->_io->setTargetPort(eid, 8080));
	this->_io->setTimeout(eid, awh::event::action_t::CONNECT, 500);
	this->_io->setTimeout(eid, awh::event::action_t::RECONNECT, 500);
	this->_io->setTimeout(tick, awh::event::action_t::NONE, 200);
	this->_io->on(eid, [&trace]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		trace.append(std::to_string(static_cast <uint16_t> (status))).append(" ");
	});
	this->_io->on(eid, static_cast <awh::engine::callback::connect_t> (
		[&failures, &successes]([[maybe_unused]] const awh::event::id_t eid, const bool ok) noexcept -> void {
			if(ok) successes++; else failures++;
		}
	));
	this->_io->on(eid, static_cast <awh::engine::callback::timeout_t> (
		[&expirations, &reconnects]([[maybe_unused]] const awh::event::id_t eid, const awh::event::action_t action, [[maybe_unused]] const uint32_t delay) noexcept -> bool {
			if(action == awh::event::action_t::CONNECT){
				// Считаем срабатывание срока ожидания подключения
				expirations++;
				/**
				 * Соглашаемся на уничтожение события: опция самостоятельного
				 * переподключения развернёт его в подъём. Отказ увёл бы в иной путь -
				 * безотлагательную новую попытку силами движка, где переподключение
				 * не участвует
				 */
				return true;
			}
			// Считаем срабатывание срока переподключения
			reconnects++;
			/**
			 * Для переподключения ответ читается наоборот: отказ его отменяет.
			 * Отвечаем согласием, чтобы событие поднималось
			 */
			return true;
		}
	));
	ASSERT_TRUE(this->_io->commit(eid));
	ASSERT_TRUE(this->_io->commit(tick));
	/**
	 * Подключение к глухому адресу должно уйти в **ожидание** - только тогда истечёт
	 * срок. Если ядро отвергает его сразу, значит, оно уже держит неудачную запись
	 * соседа по этому адресу, и опыт поставить не на чем: окружение для него не
	 * годится, и проверка пропускается
	 */
	if(!this->_io->connect(eid)){
		// Уничтожаем заведённые события
		this->_io->destroy(eid);
		this->_io->destroy(tick);
		// Завершаем работу движка
		this->_io->deinitialize();
		// Пропускаем проверку
		GTEST_SKIP() << "ядро отвергает подключение к глухому адресу немедленно";
	}
	ASSERT_TRUE(this->_io->launch(eid));
	ASSERT_TRUE(this->_io->launch(tick));
	const auto start = std::chrono::steady_clock::now();
	while((std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 4000) && this->_io->poll());
	const std::string sign = std::string("сроков=") + std::to_string(expirations) +
		" переподключений=" + std::to_string(reconnects) + " отказов=" + std::to_string(failures) +
		" удач=" + std::to_string(successes) + " состояния: " + trace;
	/**
	 * Событие с самостоятельным переподключением обязано подниматься **безостановочно**:
	 * за четыре секунды при задержке в полсекунды попыток набирается несколько, и ни
	 * одна из них - ни сорвавшаяся немедленным отказом, ни истёкшая по сроку - череду
	 * не прекращает
	 */
	ASSERT_GE(reconnects, 3) << sign;
	// Удачных подключений к глухому адресу быть не может
	ASSERT_EQ(0, successes) << sign;
	/**
	 * Событие снимается с самостоятельного переподключения до уничтожения: иначе оно
	 * поднимается вновь и вновь и достаётся следующему тесту, которому цикл событий
	 * достаётся общий - он один на весь процесс
	 */
	this->_io->setOption(eid, awh::event::options::AUTO_RECONNECT, false);
	// Уничтожаем заведённые события
	this->_io->destroy(eid);
	this->_io->destroy(tick);
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест подъёма события с самостоятельным переподключением при отказе от уничтожения
 *
 * @details Опция `AUTO_RECONNECT` означает, что подключение движок ведёт сам, покуда оно
 *          не встанет. Ответ вызывающего на срок ожидания подключения на этом ничего не
 *          меняет: и согласие, и отказ приводят к одному - сокет пересоздаётся и
 *          начинается новая попытка. Разнятся лишь пути, которыми движок туда приходит,
 *          а исход у них общий, и вызывающему разница не видна
 *
 * @note Согласие проверяется соседним тестом
 *       (`IoAutoReconnectAfterConnectTimeoutTest`), здесь же проверяется **отказ** - тот
 *       самый случай, где событие прежде замирало насовсем: срок ожидания срабатывал
 *       единожды, и дальше не приходило ни возрождения, ни новой попытки
 *
 *       Признак подъёма берётся двойной - возрождение события по росписи состояний и
 *       счёт объявленных отказов подключения. Одного счёта сроков мало: новая попытка
 *       вправе сорваться и немедленным отказом ядра, не дожив до своего срока
 *
 */
TEST_F(IoFixture, IoAutoReconnectOnTimeoutRefusalTest){
	uint8_t failures = 0, successes = 0, expirations = 0, rebirths = 0;
	const awh::event::id_t eid = this->_io->event(
		awh::event::node_t::CLIENT, awh::event::family_t::IPV4,
		awh::event::type_t::STREAM, awh::event::protocol_t::TCP
	);
	ASSERT_GT(eid, 0u);
	const awh::event::id_t tick = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
	ASSERT_GT(tick, 0u);
	ASSERT_TRUE(this->_io->initialize());
	ASSERT_TRUE(this->_io->setOptions(eid, awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::TCP_NO_DELAY | awh::event::options::AUTO_RECONNECT));
	ASSERT_TRUE(this->_io->setTarget(eid, deafAddress()));
	ASSERT_TRUE(this->_io->setTargetPort(eid, 8080));
	this->_io->setTimeout(eid, awh::event::action_t::CONNECT, 500);
	this->_io->setTimeout(eid, awh::event::action_t::RECONNECT, 500);
	this->_io->setTimeout(tick, awh::event::action_t::NONE, 200);
	this->_io->on(eid, [&rebirths]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Считаем возрождения события, которыми движок объявляет о новой попытке
		if(status == awh::event::status_t::REBIRTHED) rebirths++;
	});
	this->_io->on(eid, static_cast <awh::engine::callback::connect_t> (
		[&failures, &successes]([[maybe_unused]] const awh::event::id_t eid, const bool ok) noexcept -> void {
			if(ok) successes++; else failures++;
		}
	));
	this->_io->on(eid, static_cast <awh::engine::callback::timeout_t> (
		[&expirations]([[maybe_unused]] const awh::event::id_t eid, const awh::event::action_t action, [[maybe_unused]] const uint32_t delay) noexcept -> bool {
			// Срок переподключения читается наоборот прочим: отказ отменил бы череду подъёмов
			if(action != awh::event::action_t::CONNECT) return true;
			// Считаем срабатывание срока ожидания подключения
			expirations++;
			/**
			 * Отвечаем **отказом** от уничтожения события: вызывающий ведёт счёт попыток
			 * сам и уничтожит событие своей волей, когда сочтёт нужным. Опция при этом
			 * остаётся в силе, и движок обязан продолжать поднимать событие
			 */
			return false;
		}
	));
	ASSERT_TRUE(this->_io->commit(eid));
	ASSERT_TRUE(this->_io->commit(tick));
	/**
	 * Подключение к глухому адресу должно уйти в **ожидание** - только тогда истечёт
	 * срок. Если ядро отвергает его сразу, значит, оно уже держит неудачную запись
	 * соседа по этому адресу, и опыт поставить не на чем: окружение для него не
	 * годится, и проверка пропускается
	 */
	if(!this->_io->connect(eid)){
		// Уничтожаем заведённые события
		this->_io->destroy(eid);
		this->_io->destroy(tick);
		// Завершаем работу движка
		this->_io->deinitialize();
		// Пропускаем проверку
		GTEST_SKIP() << "ядро отвергает подключение к глухому адресу немедленно";
	}
	ASSERT_TRUE(this->_io->launch(eid));
	ASSERT_TRUE(this->_io->launch(tick));
	const auto start = std::chrono::steady_clock::now();
	uint32_t slowest = 0;
	while((std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 4000)){
		const auto tock = std::chrono::steady_clock::now();
		const bool ok = this->_io->poll();
		const uint32_t spent = static_cast <uint32_t> (std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - tock).count());
		if(spent > slowest) slowest = spent;
		if(!ok) break;
	}
	const uint32_t looped = static_cast <uint32_t> (std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count());
	const std::string sign = std::string("сроков=") + std::to_string(expirations) +
		" возрождений=" + std::to_string(rebirths) + " отказов=" + std::to_string(failures) +
		" удач=" + std::to_string(successes) + " цикл=" + std::to_string(looped) + "мс дольший опрос=" + std::to_string(slowest) + "мс";
	// Ни один опрос цикла событий не вправе задерживаться дольше отведённого сроку ожидания
	ASSERT_LT(slowest, 2000u) << sign;
	ASSERT_LT(looped, 6000u) << sign;
	// Событие обязано возродиться и повести новую попытку, а не замереть после первой же неудачи
	ASSERT_GE(rebirths, 1) << sign;
	// Отказов набирается по меньшей мере два: за первой попыткой обязана последовать вторая
	ASSERT_GE(failures, 2) << sign;
	// Удачных подключений к глухому адресу быть не может
	ASSERT_EQ(0, successes) << sign;
	/**
	 * Событие снимается с самостоятельного переподключения до уничтожения: иначе оно
	 * поднимается вновь и вновь и достаётся следующему тесту, которому цикл событий
	 * достаётся общий - он один на весь процесс
	 */
	this->_io->setOption(eid, awh::event::options::AUTO_RECONNECT, false);
	// Уничтожаем заведённые события
	this->_io->destroy(eid);
	this->_io->destroy(tick);
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест перестройки события с самостоятельным переподключением
 *
 * @details Проверяются два опасных совпадения разом. Первое - у события выставлена
 *          опция `AUTO_RECONNECT`, и перестройка не должна её ломать: событие обязано
 *          остаться работоспособным. Второе, худшее, - перестройку вызывают **в разгар
 *          переподключения**, когда очередной подъём уже назначен сроком
 *
 *          Не сними перестройка назначенный срок, он выстрелил бы следом и повёл
 *          второй подъём поверх того, что ведёт вызывающий: два подключения на одно
 *          событие, каждое со своим дескриптором
 *
 * @note Признаком сдвоенного подъёма служит счёт удачных подключений. Их обязано быть
 *       ровно два - первое и то, что после перестройки. Третье означает, что
 *       назначенное переподключение сработало вдобавок к перестройке
 *
 *       Проверка не пустая: со снятой правкой тест отвечает `удач=3`, то есть ловит
 *       ровно то, ради чего заведён
 *
 */
TEST_F(IoFixture, IoRebuildDuringReconnectTest){
	uint8_t failures = 0, successes = 0;
	// Заводим слушающий сокет на петле
	const int32_t listener = ::socket(AF_INET, SOCK_STREAM, 0);
	ASSERT_GT(listener, 0);
	struct sockaddr_in host{};
	/**
	 * Поле длины записи адреса заводят лишь системы происхождения BSD: у Linux и
	 * MS Windows его нет вовсе, а нужным оно не является нигде - длина подаётся
	 * вызовам отдельным доводом
	 */
	#if __APPLE__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__
		host.sin_len = sizeof(host);
	#endif
	host.sin_family = AF_INET;
	host.sin_port = 0;
	::inet_pton(AF_INET, "127.0.0.1", &host.sin_addr);
	ASSERT_EQ(0, ::bind(listener, reinterpret_cast <struct sockaddr *> (&host), sizeof(host)));
	socklen_t length = sizeof(host);
	ASSERT_EQ(0, ::getsockname(listener, reinterpret_cast <struct sockaddr *> (&host), &length));
	ASSERT_EQ(0, ::listen(listener, 16));
	// Заводим событие клиента и тикающий интервал
	const awh::event::id_t eid = this->_io->event(
		awh::event::node_t::CLIENT, awh::event::family_t::IPV4,
		awh::event::type_t::STREAM, awh::event::protocol_t::TCP
	);
	ASSERT_GT(eid, 0u);
	const awh::event::id_t tick = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
	ASSERT_GT(tick, 0u);
	ASSERT_TRUE(this->_io->initialize());
	ASSERT_TRUE(this->_io->setOptions(eid, awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::TCP_NO_DELAY | awh::event::options::AUTO_RECONNECT));
	ASSERT_TRUE(this->_io->setTarget(eid, "127.0.0.1"));
	ASSERT_TRUE(this->_io->setTargetPort(eid, ntohs(host.sin_port)));
	/**
	 * Задержка переподключения выбрана заведомо большей, чем время до перестройки:
	 * подъём успевает быть назначенным, но не успевает произойти - именно то
	 * совпадение, которое проверяется
	 */
	this->_io->setTimeout(eid, awh::event::action_t::RECONNECT, 1000);
	this->_io->setTimeout(tick, awh::event::action_t::NONE, 100);
	this->_io->on(eid, static_cast <awh::engine::callback::connect_t> (
		[&failures, &successes]([[maybe_unused]] const awh::event::id_t eid, const bool ok) noexcept -> void {
			if(ok) successes++; else failures++;
		}
	));
	this->_io->on(eid, static_cast <awh::engine::callback::timeout_t> (
		[]([[maybe_unused]] const awh::event::id_t eid, [[maybe_unused]] const awh::event::action_t action, [[maybe_unused]] const uint32_t delay) noexcept -> bool {
			// Соглашаемся на любое действие срока
			return true;
		}
	));
	ASSERT_TRUE(this->_io->commit(eid));
	ASSERT_TRUE(this->_io->commit(tick));
	ASSERT_TRUE(this->_io->connect(eid));
	ASSERT_TRUE(this->_io->launch(eid));
	ASSERT_TRUE(this->_io->launch(tick));
	// Дожидаемся первого подключения
	auto start = std::chrono::steady_clock::now();
	while((successes < 1) && (std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 2000) && this->_io->poll());
	ASSERT_EQ(1, successes);
	/**
	 * Рвём связь со стороны получателя: принимаем подключение и закрываем принятый
	 * сокет. Обрыв доходит до события концом файла, а у события с опцией
	 * самостоятельного переподключения обрыв назначает подъём сроком - именно на него
	 * и придётся перестройка
	 *
	 * @note Отключением `disconnect()` такого положения не создать: оно переводит
	 *       событие в отмену, не проходя уничтожением, а подъём назначает именно
	 *       уничтожение. Проверка на отключении оказалась бы пустой
	 */
	const int32_t peer = ::accept(listener, nullptr, nullptr);
	ASSERT_GT(peer, 0);
	ASSERT_EQ(0, ::closesocket(peer));
	// Даём обрыву дойти и назначению состояться, не дожидаясь самого подъёма
	start = std::chrono::steady_clock::now();
	while((std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 400) && this->_io->poll());
	// Возвращаем событие в работу перестройкой поверх назначенного переподключения
	const bool rebuilt = this->_io->rebuild(eid);
	const bool reconnected = (rebuilt && this->_io->connect(eid));
	const bool relaunched = (reconnected && this->_io->launch(eid));
	/**
	 * Опрос ведётся заведомо дольше задержки переподключения: сдвоенный подъём
	 * проявился бы именно после её истечения
	 */
	start = std::chrono::steady_clock::now();
	while((std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 2500) && this->_io->poll());
	const std::string sign = std::string("перестройка=") + (rebuilt ? "да" : "нет") +
		" подключение=" + (reconnected ? "да" : "нет") + " запуск=" + (relaunched ? "да" : "нет") +
		" удач=" + std::to_string(successes) + " отказов=" + std::to_string(failures);
	ASSERT_TRUE(rebuilt) << sign;
	ASSERT_TRUE(reconnected) << sign;
	ASSERT_TRUE(relaunched) << sign;
	// Подключений обязано быть ровно два: сдвоенного подъёма быть не должно
	ASSERT_EQ(2, successes) << sign;
	::closesocket(listener);
	this->_io->setOption(eid, awh::event::options::AUTO_RECONNECT, false);
	this->_io->destroy(eid);
	this->_io->destroy(tick);
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест годности потокового события к работе после обрыва по сроку ожидания
 *
 * @details Обрыв ожидающего подключения закрывает дескриптор - начатое рукопожатие
 *          иначе не прекратить. Но событие, которое вызывающий отказался уничтожать,
 *          обязано остаться годным к работе: движок возвращает ему годность
 *          перестройкой, и следующее подключение начинается простым `connect()` с
 *          `launch()`, без забот о пересоздании дескриптора
 *
 *          Тем самым потоковое событие после обрыва оказывается в том же положении,
 *          что и датаграммное, у которого дескриптор и вовсе не закрывается
 *
 * @note Подключение и запуск движок за вызывающего **не делает**: они взвели бы тот
 *       же срок ожидания и замкнули бы круг без пауз. Кому нужен такой ход, ставит
 *       `AUTO_RECONNECT`
 *
 *       Проверка не пустая: со снятой перестройкой тест отвечает `буфер=0` - читать
 *       размер буфера не с чего, дескриптора нет
 *
 */
TEST_F(IoFixture, IoAbandonedClientStaysUsableTest){
	uint8_t failures = 0, successes = 0, expirations = 0;
	// Заводим событие клиента и тикающий интервал
	const awh::event::id_t eid = this->_io->event(
		awh::event::node_t::CLIENT, awh::event::family_t::IPV4,
		awh::event::type_t::STREAM, awh::event::protocol_t::TCP
	);
	ASSERT_GT(eid, 0u);
	const awh::event::id_t tick = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
	ASSERT_GT(tick, 0u);
	ASSERT_TRUE(this->_io->initialize());
	ASSERT_TRUE(this->_io->setOptions(eid, awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::TCP_NO_DELAY));
	// Первое подключение ведётся к глухому адресу, чтобы истёк срок ожидания
	ASSERT_TRUE(this->_io->setTarget(eid, deafAddress()));
	ASSERT_TRUE(this->_io->setTargetPort(eid, 8080));
	this->_io->setTimeout(eid, awh::event::action_t::CONNECT, 500);
	this->_io->setTimeout(tick, awh::event::action_t::NONE, 100);
	this->_io->on(eid, static_cast <awh::engine::callback::connect_t> (
		[&failures, &successes]([[maybe_unused]] const awh::event::id_t eid, const bool ok) noexcept -> void {
			if(ok) successes++; else failures++;
		}
	));
	this->_io->on(eid, static_cast <awh::engine::callback::timeout_t> (
		[&expirations]([[maybe_unused]] const awh::event::id_t eid, const awh::event::action_t action, [[maybe_unused]] const uint32_t delay) noexcept -> bool {
			// Считаем срабатывание срока ожидания подключения
			if(action == awh::event::action_t::CONNECT) expirations++;
			/**
			 * Отказ от уничтожения даётся лишь однажды. Отказ означает «продолжаем», и
			 * движок тут же начинает новую попытку; отвечай проверка отказом всякий
			 * раз, попытки шли бы без конца, и прогон не завершился бы
			 */
			return (expirations > 1);
		}
	));
	ASSERT_TRUE(this->_io->commit(eid));
	ASSERT_TRUE(this->_io->commit(tick));
	/**
	 * Подключение к глухому адресу должно уйти в **ожидание** - только тогда истечёт
	 * срок. Если ядро отвергает его сразу, значит, оно уже держит неудачную запись
	 * соседа по этому адресу, и опыт поставить не на чем: окружение для него не
	 * годится, и проверка пропускается
	 */
	if(!this->_io->connect(eid)){
		// Уничтожаем заведённые события
		this->_io->destroy(eid);
		this->_io->destroy(tick);
		// Завершаем работу движка
		this->_io->deinitialize();
		// Пропускаем проверку
		GTEST_SKIP() << "ядро отвергает подключение к глухому адресу немедленно";
	}
	ASSERT_TRUE(this->_io->launch(eid));
	ASSERT_TRUE(this->_io->launch(tick));
	// Дожидаемся истечения срока ожидания подключения
	auto start = std::chrono::steady_clock::now();
	while((expirations < 1) && (std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 3000) && this->_io->poll());
	ASSERT_GE(expirations, 1);
	ASSERT_EQ(0, successes);
	/**
	 * Годность проверяется по живому дескриптору: размер буфера приёма читается с
	 * него, и у события с закрытым дескриптором обращение это ничего не даёт
	 *
	 * @note Проверять вторым подключением нельзя. Получателя после обрыва не сменить -
	 *       фиксация уже переиграна перестройкой, а после неё настройки событие не
	 *       принимает; подключаться же снова к глухому адресу бессмысленно: ядро
	 *       держит неудачную запись соседа и отвечает отказом немедленно, так что
	 *       проверялось бы его поведение, а не годность события
	 */
	const size_t buffer = this->_io->getBufferSize(eid, awh::event::action_t::READ);
	const std::string sign = std::string("сроков=") + std::to_string(expirations) +
		" удач=" + std::to_string(successes) + " отказов=" + std::to_string(failures) +
		" буфер=" + std::to_string(buffer);
	// Дескриптор события обязан быть живым: движок вернул годность перестройкой
	ASSERT_GT(buffer, 0u) << sign;
	this->_io->destroy(eid);
	this->_io->destroy(tick);
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест наступления ближнего срока, взведённого вслед за срабатыванием интервала
 *
 * @details Взвод таймера ядра идёт двумя путями, и оба ведут к одному и тому же
 *          таймеру - он единственный. Мгновенный обращается к ядру сразу,
 *          отложенный кладёт изменение в общую очередь, уходящую в ядро следующим
 *          опросом. Здесь проверяется, что ближний срок, взведённый мгновенным
 *          путём сразу за разбором истёкших сроков, наступает в отведённое ему
 *          время, а не отодвигается до дальнего
 *
 * @warning Порядок с **перезаписью** взведённого срока этот стенд не
 *          воспроизводит, и доводом в пользу его снятия служить не может. Для
 *          перезаписи нужно, чтобы отложенный взвод лежал в очереди изменений к
 *          мигу мгновенного взвода, а здесь этого не случается: измерено записью
 *          внутри снятия отложенного взвода - за весь набор проверок оно не
 *          сработало ни разу. Стенд одинаково проходит и со снятием, и без него,
 *          показывая 102-103 мс в обоих случаях
 *
 * @note Чтобы порядок собрался, событие таймера и готовность сокета к чтению
 *       обязаны прийти одной пачкой от ядра: разбор сроков кладёт отложенный
 *       взвод в очередь, а следующее за ним чтение снимает свой срок мгновенным
 *       путём. Стенда на такое совпадение пока нет
 *
 */
TEST_F(IoFixture, IoPromptDeadlineAfterIntervalTest){
	// Количество срабатываний интервала
	uint16_t intervals = 0;
	// Количество срабатываний ближнего срока
	uint8_t prompts = 0;
	// Задержка ближнего срока в миллисекундах
	constexpr uint32_t PROMPT = 60;
	// Добавляем событие интервала, перезаряжаемого отложенным путём
	const awh::event::id_t interval = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
	// Добавляем событие таймаута, которому будет взводиться ближний срок
	const awh::event::id_t prompt = this->_io->event(awh::event::node_t::TIMEOUT, awh::event::family_t::TIMER);
	// Проверяем что события созданы
	ASSERT_GT(interval, 0u);
	ASSERT_GT(prompt, 0u);
	/**
	 * Срок таймаута берётся заведомо дальним: ближний срок ему задаётся уже
	 * перевзведением, и без этого он в куче дедлайнов корнем не станет
	 */
	this->_io->setTimeout(interval, awh::event::action_t::NONE, 40);
	this->_io->setTimeout(prompt, awh::event::action_t::NONE, 5000);
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Выполняем фиксацию настроек событий
	ASSERT_TRUE(this->_io->commit(interval));
	ASSERT_TRUE(this->_io->commit(prompt));
	// Устанавливаем функцию обратного вызова на событие интервала
	this->_io->on(interval, [&intervals]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Если статус события успешен, считаем срабатывание интервала
		if(status == awh::event::status_t::SUCCESS)
			// Увеличиваем количество срабатываний интервала
			intervals++;
	});
	// Устанавливаем функцию обратного вызова на событие ближнего срока
	this->_io->on(prompt, [&prompts]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Если статус события успешен, считаем срабатывание ближнего срока
		if(status == awh::event::status_t::SUCCESS)
			// Увеличиваем количество срабатываний ближнего срока
			prompts++;
	});
	// Выполняем запуск событий
	ASSERT_TRUE(this->_io->launch(interval));
	ASSERT_TRUE(this->_io->launch(prompt));
	/**
	 * Дожидаемся первого срабатывания интервала: к возврату из этого опроса
	 * перезарядка интервала уже лежит в очереди изменений отложенным взводом
	 */
	auto start = std::chrono::steady_clock::now();
	while((intervals < 1) && (std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 3000) && this->_io->poll());
	ASSERT_GE(intervals, 1);
	/**
	 * Взводим ближний срок мгновенным путём
	 *
	 * @note Срок его короче периода интервала, то есть в куче дедлайнов он
	 *       становится корнем, и наступить обязан прежде следующего интервала
	 */
	const auto armed = std::chrono::steady_clock::now();
	ASSERT_TRUE(this->_io->rearmTimeout(prompt, awh::event::action_t::NONE, PROMPT));
	// Дожидаемся наступления ближнего срока
	while((prompts < 1) && (std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - armed).count() < 3000) && this->_io->poll());
	// Запоминаем время, за которое ближний срок наступил
	const int64_t spent = std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - armed).count();
	// Собираем подпись для отчёта об отказе
	const std::string sign = std::string("ближних=") + std::to_string(prompts) +
		" интервалов=" + std::to_string(intervals) + " ушло=" + std::to_string(spent) + "мс";
	// Ближний срок обязан наступить
	ASSERT_GE(prompts, 1) << sign;
	/**
	 * Ближний срок обязан наступить в отведённое ему время
	 *
	 * @note Верхняя граница взята с запасом на разброс планировщика, но заведомо
	 *       меньше дальнего срока таймаута: отодвинься взвод до него, и разница
	 *       была бы не в проценты, а в порядок
	 */
	ASSERT_LT(spent, static_cast <int64_t> (PROMPT * 8)) << sign;
	this->_io->destroy(interval);
	this->_io->destroy(prompt);
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Тест совпадения срабатывания срока и готовности сокета в одной пачке событий
 *
 * @details Стенд собирает порядок, при котором отложенный взвод таймера ядра лежит
 *          в очереди изменений к мигу мгновенного взвода. Событие таймера и
 *          готовность сокета к чтению приходят от ядра одной пачкой: разбор
 *          истёкших сроков кладёт отложенный взвод в очередь, а следующее за ним в
 *          той же пачке чтение снимает свой срок мгновенным путём, обращаясь к ядру
 *          напрямую. Очередь уходит в ядро лишь следующим опросом - и, не сними
 *          мгновенный взвод отложенный, перезаписала бы взведённый срок своим,
 *          уже устаревшим
 *
 * @par Что стенд установил
 * Порядок этот **достижим**: измерено записью внутри снятия отложенного взвода -
 * за прогон оно срабатывает. До этого стенда он не встречался ни в одной из 3396
 * проверок набора, и снятие отложенного взвода выглядело кодом мёртвым. Оно не
 * мёртвое
 *
 * @warning Наблюдаемого отказа отсюда пока не выведено: стенд проходит и со снятием
 *          отложенного взвода, и без него. Причина в подстраховке цикла опроса -
 *          время ожидания опроса ограничивается ближайшим сроком независимо от
 *          того, на что взведено ядро, - и при её работе срок наступает даже при
 *          перезаписанном взводе. Стенд стережёт достижимость порядка, а не вред
 *          от него
 *
 * @note Соотношение величин здесь существенно и подобрано опытом. Срок ожидания
 *       чтения обязан быть **короче** периода интервала: иначе ближайшим сроком
 *       всегда оказывается интервал, снятие срока чтения ближайший срок не трогает,
 *       и мгновенного взвода не выполняется вовсе. При обратном соотношении за
 *       прогон случался ровно ноль мгновенных взводов при пяти сотнях отложенных
 *
 */
TEST_F(IoFixture, IoTimerAndSocketSameBatchTest){
	// Количество срабатываний интервала
	uint16_t intervals = 0;
	// Количество принятых от потока пакетов
	uint16_t packets = 0;
	// Количество наступивших сроков ожидания чтения
	uint16_t expirations = 0;
	// Заводим сокет источника потока
	const int32_t feeder = ::socket(AF_INET, SOCK_DGRAM, 0);
	// Проверяем что сокет источника заведён
	ASSERT_GT(feeder, 0);
	// Адрес источника потока
	struct sockaddr_in origin{};
	// Устанавливаем семейство адреса источника
	origin.sin_family = AF_INET;
	// Устанавливаем адрес устройства петли
	origin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	// Привязываем источник к произвольному свободному порту
	ASSERT_EQ(::bind(feeder, reinterpret_cast <struct sockaddr *> (&origin), sizeof(origin)), 0);
	// Размер адреса источника
	socklen_t length = sizeof(origin);
	// Получаем занятый источником порт
	ASSERT_EQ(::getsockname(feeder, reinterpret_cast <struct sockaddr *> (&origin), &length), 0);
	// Добавляем событие интервала
	const awh::event::id_t interval = this->_io->event(awh::event::node_t::INTERVAL, awh::event::family_t::TIMER);
	// Добавляем событие дейтаграммного клиента, принимающего поток
	const awh::event::id_t reader = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::UDP);
	// Проверяем что события созданы
	ASSERT_GT(interval, 0u);
	ASSERT_GT(reader, 0u);
	/**
	 * Устанавливаем период интервала
	 *
	 * @note Период взят заведомо большим срока ожидания чтения. Иначе ближайшим
	 *       сроком всегда оказывается интервал, снятие срока чтения ближайший срок
	 *       не трогает - и мгновенный взвод не выполняется вовсе, а с ним не
	 *       случается и порядка, ради которого стенд заведён
	 */
	this->_io->setTimeout(interval, awh::event::action_t::NONE, 100);
	/**
	 * Устанавливаем клиенту срок ожидания чтения
	 *
	 * @note Срок этот и снимается мгновенным путём на каждом принятом пакете - ради
	 *       него стенд и заведён. Величина взята заведомо большей шага потока: срок
	 *       обязан сниматься приходом данных, а не истекать сам
	 */
	this->_io->setTimeout(reader, awh::event::action_t::READ, 8);
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем функцию обратного вызова на событие интервала
	this->_io->on(interval, [&intervals]([[maybe_unused]] const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
		// Если статус события успешен, считаем срабатывание интервала
		if(status == awh::event::status_t::SUCCESS)
			// Увеличиваем количество срабатываний интервала
			intervals++;
	});
	// Устанавливаем функцию обратного вызова на событие чтения данных
	this->_io->on(reader, static_cast <awh::engine::callback::read_t> (
		[&packets]([[maybe_unused]] const awh::event::id_t eid, [[maybe_unused]] const uint8_t * data, [[maybe_unused]] const size_t size) noexcept -> void {
			// Считаем принятый от потока пакет
			packets++;
		}
	));
	// Устанавливаем функцию обратного вызова на событие наступления срока ожидания
	this->_io->on(reader, static_cast <awh::engine::callback::timeout_t> (
		[&expirations]([[maybe_unused]] const awh::event::id_t eid, [[maybe_unused]] const awh::event::action_t action, [[maybe_unused]] const uint32_t delay) noexcept -> bool {
			// Считаем наступивший срок ожидания
			expirations++;
			// Запрещаем удаление события по наступлении срока
			return false;
		}
	));
	// Устанавливаем адрес источника потока
	ASSERT_TRUE(this->_io->setTarget(reader, "127.0.0.1"));
	// Устанавливаем порт источника потока
	ASSERT_TRUE(this->_io->setTargetPort(reader, ntohs(origin.sin_port)));
	// Устанавливаем опции события клиента
	ASSERT_TRUE(this->_io->setOptions(reader, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
	// Выполняем фиксацию настроек событий
	ASSERT_TRUE(this->_io->commit(interval));
	ASSERT_TRUE(this->_io->commit(reader));
	// Выполняем запуск событий
	ASSERT_TRUE(this->_io->launch(interval));
	ASSERT_TRUE(this->_io->launch(reader));
	/**
	 * Отправляем источнику первую дейтаграмму
	 *
	 * @note Без неё источнику неоткуда узнать адрес клиента: сокет клиента
	 *       подключённый, порт ему выдан ядром, и назвать его заранее нельзя
	 */
	const uint8_t hello[4] = {0x41, 0x57, 0x48, 0x00};
	ASSERT_GT(this->_io->send(reader, hello, sizeof(hello)), static_cast <size_t> (0));
	// Признак работы потока
	std::atomic <bool> running(true);
	/**
	 * Заводим нить, гонящую поток дейтаграмм клиенту
	 *
	 * @note Шаг потока взят много короче срока ожидания чтения: срок снимается
	 *       приходом данных и наступить не успевает. Событие интервала при этом
	 *       приходит от ядра одной пачкой с готовностью сокета многократно за
	 *       время прогона
	 */
	std::thread feed([&running, feeder]() noexcept -> void {
		// Адрес клиента, узнаваемый из первой его дейтаграммы
		struct sockaddr_in peer{};
		// Размер адреса клиента
		socklen_t size = sizeof(peer);
		// Приёмный буфер источника
		uint8_t buffer[64];
		// Дожидаемся первой дейтаграммы клиента, узнавая его адрес
		if(::recvfrom(feeder, reinterpret_cast <char *> (buffer), sizeof(buffer), 0, reinterpret_cast <struct sockaddr *> (&peer), &size) <= 0)
			// Выходим, так-как адрес клиента узнать не удалось
			return;
		/**
		 * Гоним поток, пока стенд его не остановит
		 */
		while(running.load()){
			// Отправляем очередную дейтаграмму клиенту
			::sendto(feeder, reinterpret_cast <const char *> (buffer), 4, 0, reinterpret_cast <const struct sockaddr *> (&peer), size);
			// Выдерживаем шаг потока
			std::this_thread::sleep_for(std::chrono::microseconds(1000));
		}
	});
	// Запоминаем время начала опроса событий
	const auto start = std::chrono::steady_clock::now();
	// Ведём опрос событий отведённое стенду время
	while((std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 2000) && this->_io->poll());
	// Останавливаем поток
	running.store(false);
	// Дожидаемся завершения нити потока
	feed.join();
	// Освобождаем сокет источника потока
	::closesocket(feeder);
	// Собираем подпись для отчёта об отказе
	const std::string sign = std::string("интервалов=") + std::to_string(intervals) +
		" пакетов=" + std::to_string(packets) + " сроков=" + std::to_string(expirations);
	// Интервал обязан срабатывать всё отведённое время
	ASSERT_GT(intervals, 10u) << sign;
	// Клиент обязан принимать поток всё отведённое время
	ASSERT_GT(packets, 50u) << sign;
	/**
	 * Срок ожидания чтения наступить не вправе
	 *
	 * @note Здесь и вскрылась бы перезапись взведённого срока: поток идёт шагом
	 *       вдвое короче срока ожидания, и снимается срок на каждом пакете. Срок,
	 *       наступивший при живом потоке, означает, что взвод до ядра не дошёл
	 */
	ASSERT_EQ(expirations, 0u) << sign;
	this->_io->destroy(interval);
	this->_io->destroy(reader);
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * Возвращаем снятые макросы MS Windows
 */
#include <sys/macro_pop.hpp>
