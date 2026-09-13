/**
 * @file link.cpp
 * @date 2026-09-11
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
 * @brief Живая проверка прямой связи работников кластера и ухода работника по приказу
 *
 * @details Проверки эти поднимают НАСТОЯЩИЙ кластер и прогоняют по нему то, чего звезда
 *          «мастер и работники» не давала вовсе: обмен между двумя работниками напрямую,
 *          пересылку через мастера и уход работника по приказу мастера. Устройство
 *          разобрано в `src/unit/CLUSTER-LINK.md`
 *
 * @note Работник под MS Windows проходит `main` заново и добирается до тех же проверок.
 *       Чтобы он не прогонял весь набор, мастер оставляет ему в окружении отбор
 *       `GTEST_FILTER` по этой самой проверке: тело её служит обеим ролям, а роль
 *       спрашивается у самого кластера
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем системные заголовочные файлы
 */
#include <atomic>
#include <chrono>
#include <thread>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/unit/cluster.hpp"
#include <sys/log.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * @brief Тест прямой связи между двумя работниками кластера
 *
 * @details Порядок проверки таков: работники узнают друг о друге извещениями мастера,
 *          младший по номеру заказывает связь, шлёт по ней посылку, старший отвечает по
 *          той же связи, младший пересылает старшему подтверждение уже через мастера, и
 *          старший докладывает мастеру об успехе своим пользовательским каналом
 *
 * @note Заказ связи ведёт МЛАДШИЙ по номеру процесса намеренно: узнают друг о друге оба,
 *       и закажи связь оба - второй заказ получил бы отказ `EXISTS`. Правило это
 *       произвольно, но однозначно, и оттого годится обеим сторонам без уговора
 *
 */
TEST(ClusterLinkFixture, ClusterWorkerLinkTest){
	// Создаём объект для работы с логами
	// Отключаем вывод журнала: обе роли пишут в один поток
	awh::log::mode({});
	// Создаём объект кластера
	unit::cluster_t cluster;
	// Признак того, что мастер получил доклад об удавшейся связи
	std::atomic_bool linked{false};
	/**
	 * Оставляем работнику отбор проверки в окружении
	 */
	#if defined(_WIN32) || defined(_WIN64)
		::_putenv("GTEST_FILTER=ClusterLinkFixture.ClusterWorkerLinkTest");
	#endif
	// Запрещаем перезапуск упавших работников: работники этой проверки кончают работу сами
	cluster.rebirth(false);
	// Устанавливаем количество дочерних процессов в кластере
	cluster.count(2);
	/**
	 * Устанавливаем функцию обратного вызова на вход узла в кластер
	 *
	 * @note Заказ связи идёт отсюда, а не от запуска работника: своего способа узнать
	 *       соседей у работника нет вовсе, и до извещения мастера связываться не с кем
	 */
	cluster.on <void (const pid_t)> ("join", [&cluster](const pid_t pid) noexcept -> void {
		// Если узел о соседе узнал работник и номер его младше соседского
		if(!cluster.master() && (static_cast <pid_t> (::getpid()) < pid))
			// Заказываем прямую связь с соседом
			cluster.link(pid);
	}, placeholders::_1);
	// Устанавливаем функцию обратного вызова на заведение прямой связи
	cluster.on <void (const pid_t, const event::id_t)> ("linked", [&cluster](const pid_t pid, const event::id_t eid) noexcept -> void {
		/**
		 * Посылку шлёт ЗАКАЗАВШИЙ связь, а не оба конца
		 *
		 * @note Извещение о заведённой связи получают оба, и шли посылку оба - обмен
		 *       пошёл бы навстречу двумя одинаковыми парами, а проверка перестала бы
		 *       различать, чей ответ дошёл
		 */
		if(static_cast <pid_t> (::getpid()) < pid){
			// Текст посылки по прямой связи
			const string message = "ping";
			// Отправляем посылку соседу напрямую
			cluster.transmit(pid, message.c_str(), message.length());
		}
		// Снимаем неиспользуемый довод
		(void) eid;
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на получение сообщения по прямой связи
	cluster.on <void (const pid_t, const uint8_t *, const size_t)> ("peer", [&cluster](const pid_t pid, const uint8_t * data, const size_t size) noexcept -> void {
		// Текст входящего сообщения
		const string message(reinterpret_cast <const char *> (data), size);
		// Если посылку получил заказавший связь работник
		if(message.compare("pong") == 0){
			// Текст подтверждения, пересылаемого через мастера
			const string done = "done";
			// Пересылаем подтверждение соседу через мастера
			cluster.relay(pid, done.c_str(), done.length());
		// Если посылку получил сосед
		} else if(message.compare("ping") == 0) {
			// Текст ответа по прямой связи
			const string answer = "pong";
			// Отвечаем соседу по той же прямой связи
			cluster.transmit(pid, answer.c_str(), answer.length());
		}
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Устанавливаем функцию обратного вызова на получение пересылки через мастера
	cluster.on <void (const pid_t, const uint8_t *, const size_t)> ("relay", [&cluster](const pid_t pid, const uint8_t * data, const size_t size) noexcept -> void {
		// Текст входящей пересылки
		const string message(reinterpret_cast <const char *> (data), size);
		// Если пересылка донесла подтверждение
		if(message.compare("done") == 0){
			// Текст доклада мастеру
			const string report = "ok";
			// Докладываем мастеру об удавшейся связи пользовательским каналом
			cluster.send(report.c_str(), report.length());
		}
		// Снимаем неиспользуемый довод
		(void) pid;
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Устанавливаем функцию обратного вызова на событие получения сообщений
	cluster.on <void (const pid_t, const uint8_t *, const size_t)> ("message", [&cluster, &linked](const pid_t pid, const uint8_t * data, const size_t size) noexcept -> void {
		// Текст входящего сообщения
		const string message(reinterpret_cast <const char *> (data), size);
		// Если доклад получил мастер
		if(cluster.master() && (message.compare("ok") == 0)){
			// Запоминаем, что доклад получен
			linked.store(true);
			/**
			 * Снимаем работников принудительно, прежде остановки кластера
			 *
			 * @warning Одной остановки кластера НЕДОСТАТОЧНО: измерено на macOS, что
			 *          работник переживает и остановку, и уход самого мастера
			 */
			for(const pid_t worker : cluster.workers())
				// Снимаем работника принудительно
				cluster.erase(worker, unit::cluster_t::shutdown_t::FORCEFUL);
			// Останавливаем работу кластера
			cluster.stop();
		}
		// Снимаем неиспользуемый довод
		(void) pid;
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Признак того, что работа кластера кончилась
	std::atomic_bool finished{false};
	/**
	 * Заводим поток сторожа срока работы
	 *
	 * @note Связь не заведшаяся обязана кончиться отказом проверки, а не вечным
	 *       ожиданием: работа кластера ведёт свой цикл событий и сама не кончается
	 */
	std::thread guard([&cluster, &finished]() noexcept -> void {
		/**
		 * Выполняем обороты ожидания, покуда не выйдет срок
		 */
		for(uint16_t round = 0; (round < 300) && !finished.load(); ++round)
			// Выполняем ожидание одного оборота
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		// Если работа кластера ещё не кончилась
		if(!finished.load())
			// Останавливаем работу кластера по сроку
			cluster.stop();
	});
	// Запускаем работу кластера
	cluster.start();
	// Отмечаем, что работа кластера кончилась
	finished.store(true);
	// Дожидаемся завершения сторожа срока работы
	guard.join();
	/**
	 * Снимаем отбор проверки из окружения
	 */
	#if defined(_WIN32) || defined(_WIN64)
		::_putenv("GTEST_FILTER=");
	#endif
	/**
	 * Доклад об удавшейся связи обязан дойти до мастера
	 *
	 * @note Утверждение спрашивается только у мастера: работников снимает он сам,
	 *       и до собственных утверждений те не доходят
	 */
	if(cluster.master())
		// Доклад об удавшейся связи обязан дойти до мастера
		ASSERT_TRUE(linked.load()) << "доклад работника не дошёл до мастера: прямая связь работников не состоялась";
}

/**
 * @brief Тест ухода работника по приказу мастера
 *
 * @details Закрепляет то, что уход по приказу отличим от падения: мастер велит работнику
 *          уйти кодом, работник уходит сам, а мастер при РАЗРЕШЁННОМ возрождении взамен
 *          его не поднимает и счётчик быстрых падений не крутит
 *
 * @note Возрождение здесь разрешено намеренно: с запрещённым возрождением проверка не
 *       доказывала бы ничего - замены не появилось бы и без признака намеренного ухода
 *
 */
TEST(ClusterLinkFixture, ClusterWorkerShutdownTest){
	// Создаём объект для работы с логами
	// Отключаем вывод журнала: обе роли пишут в один поток
	awh::log::mode({});
	// Создаём объект кластера
	unit::cluster_t cluster;
	// Число узлов, вошедших в кластер за время проверки
	std::atomic_uint32_t joined{0};
	// Код, каким завершился ушедший по приказу работник
	std::atomic_int32_t code{0};
	// Признак того, что работник завершился
	std::atomic_bool exited{false};
	/**
	 * Оставляем работнику отбор проверки в окружении
	 */
	#if defined(_WIN32) || defined(_WIN64)
		::_putenv("GTEST_FILTER=ClusterLinkFixture.ClusterWorkerShutdownTest");
	#endif
	// Разрешаем перезапуск упавших работников: им и проверяется отличие ухода от падения
	cluster.rebirth(true);
	// Устанавливаем количество дочерних процессов в кластере
	cluster.count(1);
	// Устанавливаем функцию обратного вызова на вход узла в кластер
	cluster.on <void (const pid_t)> ("join", [&cluster, &joined](const pid_t pid) noexcept -> void {
		// Если узел вошёл в кластер у мастера
		if(cluster.master()){
			// Считаем вошедший узел
			joined.fetch_add(1);
			// Велим работнику завершить работу условленным кодом
			cluster.shutdown(pid, 42);
		}
	}, placeholders::_1);
	// Устанавливаем функцию обратного вызова на завершение работы процесса
	cluster.on <void (const pid_t, const int32_t)> ("exit", [&cluster, &code, &exited](const pid_t pid, const int32_t status) noexcept -> void {
		// Если процесс завершился у мастера
		if(cluster.master()){
			// Запоминаем код завершения работы процесса
			code.store(unit::cluster_t::exitcode(status));
			// Отмечаем завершение работы процесса
			exited.store(true);
		}
		// Снимаем неиспользуемый довод
		(void) pid;
	}, placeholders::_1, placeholders::_2);
	// Признак того, что работа кластера кончилась
	std::atomic_bool finished{false};
	/**
	 * Заводим поток сторожа срока работы
	 *
	 * @note Останавливать кластер здесь приходится по сроку и в исправном случае:
	 *       доказывается ОТСУТСТВИЕ замены, а отсутствие события ничем, кроме
	 *       выдержки, не подтверждается
	 */
	std::thread guard([&cluster, &finished, &exited]() noexcept -> void {
		// Число оборотов ожидания, отмеренных после завершения работника
		uint16_t settle = 0;
		/**
		 * Выполняем обороты ожидания, покуда не выйдет срок
		 */
		for(uint16_t round = 0; (round < 300) && !finished.load(); ++round){
			// Выполняем ожидание одного оборота
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			// Если работник завершился, отмеряем выдержку на возможную замену
			if(exited.load() && (++settle > 20))
				// Прекращаем ожидание: замены за выдержку не появилось
				break;
		}
		// Если работа кластера ещё не кончилась
		if(!finished.load())
			// Останавливаем работу кластера
			cluster.stop();
	});
	// Запускаем работу кластера
	cluster.start();
	// Отмечаем, что работа кластера кончилась
	finished.store(true);
	// Дожидаемся завершения сторожа срока работы
	guard.join();
	/**
	 * Снимаем отбор проверки из окружения
	 */
	#if defined(_WIN32) || defined(_WIN64)
		::_putenv("GTEST_FILTER=");
	#endif
	// Если проверка идёт у мастера
	if(cluster.master()){
		// Работник обязан был завершиться
		ASSERT_TRUE(exited.load()) << "работник не завершился: приказ мастера до него не дошёл";
		// Работник обязан был завершиться условленным кодом
		ASSERT_EQ(42, code.load()) << "работник завершился чужим кодом: приказ мастера кода не донёс";
		/**
		 * Замены ушедшему по приказу работнику быть не должно
		 *
		 * @note Считаются именно ВХОДЫ узлов: поднятая замена дошла бы до цикла событий
		 *       и известила бы мастера о своём подъёме, как и всякий работник
		 */
		ASSERT_EQ(1u, joined.load()) << "мастер поднял замену работнику, ушедшему по приказу";
	}
}

/**
 * @brief Тест отказов в заведении прямой связи и предела пересылки
 *
 * @details Проверяются три отказа разом, и все три обязаны быть РАЗЛИЧИМЫ: связь с самим
 *          собой отвергается на месте, связь с несуществующим узлом - мастером, а
 *          пересылка сверх предела не отправляет ни октета
 *
 * @note Работник утверждений не спрашивает: снимает его мастер, и до собственных
 *       утверждений тот не доходит. Оттого итоги свои работник шлёт мастеру строкой, а
 *       утверждает их уже мастер - там же, где и все прочие
 *
 */
TEST(ClusterLinkFixture, ClusterLinkRefusalTest){
	// Создаём объект для работы с логами
	// Отключаем вывод журнала: обе роли пишут в один поток
	awh::log::mode({});
	// Создаём объект кластера
	unit::cluster_t cluster;
	// Доклад работника об итогах отказов
	string report;
	/**
	 * Оставляем работнику отбор проверки в окружении
	 */
	#if defined(_WIN32) || defined(_WIN64)
		::_putenv("GTEST_FILTER=ClusterLinkFixture.ClusterLinkRefusalTest");
	#endif
	// Запрещаем перезапуск упавших работников
	cluster.rebirth(false);
	// Устанавливаем количество дочерних процессов в кластере
	cluster.count(1);
	/**
	 * Номер узла, которого в кластере заведомо нет
	 *
	 * @note Число взято заведомо большим предела номеров процессов у всех поддерживаемых
	 *       систем: занятый номер увёл бы проверку к иному отказу
	 */
	static constexpr pid_t MISSING = static_cast <pid_t> (0x3FFFFFFF);
	// Устанавливаем функцию обратного вызова на события работы кластера
	cluster.on <void (const pid_t, const unit::cluster_t::event_t)> ("events", [&cluster](const pid_t pid, const unit::cluster_t::event_t event) noexcept -> void {
		// Если процесс запущен и работником
		if((event == unit::cluster_t::event_t::START) && !cluster.master()){
			/**
			 * Связь с самим собой отвергается на месте, не доходя до мастера
			 *
			 * @note Отказ этот немедленный: заказ такой бессмыслен, и гонять его до
			 *       мастера незачем
			 */
			const bool itself = cluster.link(static_cast <pid_t> (::getpid()));
			// Буфер, заведомо превышающий предел пересылки
			const string oversized(cluster.maximumRelay() + 1, 'x');
			// Пересылка сверх предела не отправляет ни октета
			const size_t relayed = cluster.relay(MISSING, oversized.c_str(), oversized.length());
			// Заказываем связь с несуществующим узлом: отказ придёт от мастера
			cluster.link(MISSING);
			// Складываем итоги немедленных отказов
			const string message = string("itself=") + (itself ? "1" : "0") + ";relayed=" + to_string(relayed);
			// Отправляем итоги мастеру
			cluster.send(message.c_str(), message.length());
		}
		// Снимаем неиспользуемый довод
		(void) pid;
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на разрыв прямой связи
	cluster.on <void (const pid_t, const unit::cluster_t::reason_t)> ("unlinked", [&cluster](const pid_t pid, const unit::cluster_t::reason_t reason) noexcept -> void {
		// Если отказ пришёл работнику по заказу несуществующего узла
		if(!cluster.master() && (pid == MISSING)){
			// Складываем причину отказа
			const string message = string("reason=") + to_string(static_cast <uint16_t> (reason));
			// Отправляем причину отказа мастеру
			cluster.send(message.c_str(), message.length());
		}
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на событие получения сообщений
	cluster.on <void (const pid_t, const uint8_t *, const size_t)> ("message", [&cluster, &report](const pid_t pid, const uint8_t * data, const size_t size) noexcept -> void {
		// Если доклад получил мастер
		if(cluster.master()){
			// Дополняем доклад работника
			report.append(reinterpret_cast <const char *> (data), size);
			// Если доклад собран целиком
			if(report.find("reason=") != string::npos){
				// Снимаем работника принудительно
				cluster.erase(pid, unit::cluster_t::shutdown_t::FORCEFUL);
				// Останавливаем работу кластера
				cluster.stop();
			}
		}
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Признак того, что работа кластера кончилась
	std::atomic_bool finished{false};
	// Заводим поток сторожа срока работы
	std::thread guard([&cluster, &finished]() noexcept -> void {
		/**
		 * Выполняем обороты ожидания, покуда не выйдет срок
		 */
		for(uint16_t round = 0; (round < 300) && !finished.load(); ++round)
			// Выполняем ожидание одного оборота
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		// Если работа кластера ещё не кончилась
		if(!finished.load())
			// Останавливаем работу кластера по сроку
			cluster.stop();
	});
	// Запускаем работу кластера
	cluster.start();
	// Отмечаем, что работа кластера кончилась
	finished.store(true);
	// Дожидаемся завершения сторожа срока работы
	guard.join();
	/**
	 * Снимаем отбор проверки из окружения
	 */
	#if defined(_WIN32) || defined(_WIN64)
		::_putenv("GTEST_FILTER=");
	#endif
	// Если проверка идёт у мастера
	if(cluster.master()){
		// Доклад работника обязан дойти
		ASSERT_FALSE(report.empty()) << "доклад работника не дошёл до мастера";
		// Связь с самим собой обязана быть отвергнута на месте
		ASSERT_NE(string::npos, report.find("itself=0")) << "заказ связи с самим собой не был отвергнут";
		// Пересылка сверх предела не отправляет ни октета
		ASSERT_NE(string::npos, report.find("relayed=0")) << "пересылка сверх предела отправила данные вместо отказа";
		/**
		 * Причина отказа обязана быть названа ИМЕННО отсутствием узла
		 *
		 * @note Сличается точное значение, а не «хоть какой-нибудь отказ»: отказ без
		 *       различимой причины оставил бы вызывающую сторону гадать, повторять ли
		 *       заказ, и проверка на «просто отказ» прошла бы при всякой путанице причин
		 */
		ASSERT_NE(string::npos, report.find("reason=" + to_string(static_cast <uint16_t> (unit::cluster_t::reason_t::UNKNOWN))))
		 << "отказ в связи с несуществующим узлом назвал чужую причину: " << report;
	}
}

/**
 * @brief Тест запрета связи мастером
 *
 * @details Сводит работников мастер, и право отказать принадлежит ему. Проверяется, что
 *          отклик `"linking"`, ответивший отрицанием, связь ДЕЙСТВИТЕЛЬНО не заводит, а
 *          заказавший узел узнаёт о запрете причиной `REFUSED`
 *
 * @note Проверка эта - обратная к `ClusterWorkerLinkTest`: там связь обязана завестись,
 *       здесь обязана НЕ завестись. Одной положительной было бы мало - право отказать
 *       могло бы не работать вовсе, и обе проверки прошли бы
 *
 */
TEST(ClusterLinkFixture, ClusterLinkVetoTest){
	// Создаём объект для работы с логами
	// Отключаем вывод журнала: обе роли пишут в один поток
	awh::log::mode({});
	// Создаём объект кластера
	unit::cluster_t cluster;
	// Признак того, что мастер получил доклад о запрете
	std::atomic_bool refused{false};
	// Признак того, что связь всё же завелась вопреки запрету
	std::atomic_bool linked{false};
	/**
	 * Оставляем работнику отбор проверки в окружении
	 */
	#if defined(_WIN32) || defined(_WIN64)
		::_putenv("GTEST_FILTER=ClusterLinkFixture.ClusterLinkVetoTest");
	#endif
	// Запрещаем перезапуск упавших работников
	cluster.rebirth(false);
	// Устанавливаем количество дочерних процессов в кластере
	cluster.count(2);
	// Устанавливаем функцию обратного вызова на запрос разрешения связи
	cluster.on <bool (const pid_t, const pid_t)> ("linking", [](const pid_t initiator, const pid_t peer) noexcept -> bool {
		// Снимаем неиспользуемые доводы
		(void) initiator;
		(void) peer;
		// Запрещаем заведение всякой связи
		return false;
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на вход узла в кластер
	cluster.on <void (const pid_t)> ("join", [&cluster](const pid_t pid) noexcept -> void {
		// Если узел о соседе узнал работник и номер его младше соседского
		if(!cluster.master() && (static_cast <pid_t> (::getpid()) < pid))
			// Заказываем прямую связь с соседом
			cluster.link(pid);
	}, placeholders::_1);
	// Устанавливаем функцию обратного вызова на заведение прямой связи
	cluster.on <void (const pid_t, const event::id_t)> ("linked", [&cluster](const pid_t pid, const event::id_t eid) noexcept -> void {
		// Докладываем мастеру о заведённой вопреки запрету связи
		const string message = "linked";
		// Отправляем доклад мастеру
		cluster.send(message.c_str(), message.length());
		// Снимаем неиспользуемые доводы
		(void) pid;
		(void) eid;
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на разрыв прямой связи
	cluster.on <void (const pid_t, const unit::cluster_t::reason_t)> ("unlinked", [&cluster](const pid_t pid, const unit::cluster_t::reason_t reason) noexcept -> void {
		// Если отказ пришёл работнику
		if(!cluster.master()){
			// Складываем причину отказа
			const string message = string("reason=") + to_string(static_cast <uint16_t> (reason));
			// Отправляем причину отказа мастеру
			cluster.send(message.c_str(), message.length());
		}
		// Снимаем неиспользуемый довод
		(void) pid;
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на событие получения сообщений
	cluster.on <void (const pid_t, const uint8_t *, const size_t)> ("message", [&cluster, &refused, &linked](const pid_t pid, const uint8_t * data, const size_t size) noexcept -> void {
		// Текст входящего сообщения
		const string message(reinterpret_cast <const char *> (data), size);
		// Если доклад получил мастер
		if(cluster.master()){
			// Если связь завелась вопреки запрету
			if(message.compare("linked") == 0)
				// Отмечаем заведённую вопреки запрету связь
				linked.store(true);
			// Если работник доложил о запрете
			else if(message.compare("reason=" + to_string(static_cast <uint16_t> (unit::cluster_t::reason_t::REFUSED))) == 0)
				// Отмечаем полученный доклад о запрете
				refused.store(true);
			// Если доклад получен
			if(refused.load() || linked.load()){
				/**
				 * Переходим по всему списку работников
				 */
				for(const pid_t worker : cluster.workers())
					// Снимаем работника принудительно
					cluster.erase(worker, unit::cluster_t::shutdown_t::FORCEFUL);
				// Останавливаем работу кластера
				cluster.stop();
			}
		}
		// Снимаем неиспользуемый довод
		(void) pid;
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Признак того, что работа кластера кончилась
	std::atomic_bool finished{false};
	// Заводим поток сторожа срока работы
	std::thread guard([&cluster, &finished]() noexcept -> void {
		/**
		 * Выполняем обороты ожидания, покуда не выйдет срок
		 */
		for(uint16_t round = 0; (round < 300) && !finished.load(); ++round)
			// Выполняем ожидание одного оборота
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		// Если работа кластера ещё не кончилась
		if(!finished.load())
			// Останавливаем работу кластера по сроку
			cluster.stop();
	});
	// Запускаем работу кластера
	cluster.start();
	// Отмечаем, что работа кластера кончилась
	finished.store(true);
	// Дожидаемся завершения сторожа срока работы
	guard.join();
	/**
	 * Снимаем отбор проверки из окружения
	 */
	#if defined(_WIN32) || defined(_WIN64)
		::_putenv("GTEST_FILTER=");
	#endif
	// Если проверка идёт у мастера
	if(cluster.master()){
		// Связь, запрещённая мастером, заводиться не вправе
		ASSERT_FALSE(linked.load()) << "связь завелась вопреки запрету мастера";
		// Заказавший узел обязан узнать о запрете названной причиной
		ASSERT_TRUE(refused.load()) << "работник не узнал о запрете связи либо узнал с чужой причиной";
	}
}

/**
 * @brief Тест разрыва связи при выбытии соседа
 *
 * @details Работник, потерявший соседа, обязан узнать о разрыве от мастера: сам он о
 *          смерти встречной стороны не знает, а событие связи у него остаётся живым.
 *          Проверяется весь путь - мастер замечает выбытие, разрывает связи выбывшего и
 *          извещает его соседей
 *
 * @note Сосед снимается ПРИНУДИТЕЛЬНО, а не приказом: приказ ушёл бы служебным каналом и
 *       проверял бы иной путь. Здесь же проверяется разбор выбытия, какой ведёт мастер
 *       сам, не спрашивая уходящего
 *
 */
TEST(ClusterLinkFixture, ClusterLinkDropTest){
	// Создаём объект для работы с логами
	// Отключаем вывод журнала: обе роли пишут в один поток
	awh::log::mode({});
	// Создаём объект кластера
	unit::cluster_t cluster;
	// Признак того, что переживший работник узнал о разрыве
	std::atomic_bool dropped{false};
	/**
	 * Оставляем работнику отбор проверки в окружении
	 */
	#if defined(_WIN32) || defined(_WIN64)
		::_putenv("GTEST_FILTER=ClusterLinkFixture.ClusterLinkDropTest");
	#endif
	// Запрещаем перезапуск упавших работников
	cluster.rebirth(false);
	// Устанавливаем количество дочерних процессов в кластере
	cluster.count(2);
	// Устанавливаем функцию обратного вызова на вход узла в кластер
	cluster.on <void (const pid_t)> ("join", [&cluster](const pid_t pid) noexcept -> void {
		// Если узел о соседе узнал работник и номер его младше соседского
		if(!cluster.master() && (static_cast <pid_t> (::getpid()) < pid))
			// Заказываем прямую связь с соседом
			cluster.link(pid);
	}, placeholders::_1);
	// Устанавливаем функцию обратного вызова на заведение прямой связи
	cluster.on <void (const pid_t, const event::id_t)> ("linked", [&cluster](const pid_t pid, const event::id_t eid) noexcept -> void {
		/**
		 * Докладывает о связи МЛАДШИЙ по номеру, и снимать мастер будет СТАРШЕГО
		 *
		 * @note Роли разведены намеренно: снимись докладчик - о разрыве узнавать стало бы
		 *       некому, и проверка прошла бы по сроку, ничего не доказав
		 */
		if(static_cast <pid_t> (::getpid()) < pid){
			// Складываем доклад о заведённой связи с номером соседа
			const string message = string("linked=") + to_string(static_cast <uint64_t> (pid));
			// Отправляем доклад мастеру
			cluster.send(message.c_str(), message.length());
		}
		// Снимаем неиспользуемый довод
		(void) eid;
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на разрыв прямой связи
	cluster.on <void (const pid_t, const unit::cluster_t::reason_t)> ("unlinked", [&cluster](const pid_t pid, const unit::cluster_t::reason_t reason) noexcept -> void {
		// Если о разрыве узнал работник
		if(!cluster.master()){
			// Складываем доклад о разрыве связи
			const string message = string("dropped=") + to_string(static_cast <uint64_t> (pid));
			// Отправляем доклад мастеру
			cluster.send(message.c_str(), message.length());
		}
		// Снимаем неиспользуемый довод
		(void) reason;
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на событие получения сообщений
	cluster.on <void (const pid_t, const uint8_t *, const size_t)> ("message", [&cluster, &dropped](const pid_t pid, const uint8_t * data, const size_t size) noexcept -> void {
		// Текст входящего сообщения
		const string message(reinterpret_cast <const char *> (data), size);
		// Если доклад получил мастер
		if(cluster.master()){
			// Если работник доложил о заведённой связи
			if(message.compare(0, 7, "linked=") == 0)
				// Снимаем СОСЕДА доложившего работника принудительно
				cluster.erase(static_cast <pid_t> (::stoull(message.substr(7))), unit::cluster_t::shutdown_t::FORCEFUL);
			// Если работник доложил о разрыве связи
			else if(message.compare(0, 8, "dropped=") == 0) {
				// Отмечаем полученный доклад о разрыве
				dropped.store(true);
				/**
				 * Переходим по всему списку работников
				 */
				for(const pid_t worker : cluster.workers())
					// Снимаем работника принудительно
					cluster.erase(worker, unit::cluster_t::shutdown_t::FORCEFUL);
				// Останавливаем работу кластера
				cluster.stop();
			}
		}
		// Снимаем неиспользуемый довод
		(void) pid;
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Признак того, что работа кластера кончилась
	std::atomic_bool finished{false};
	// Заводим поток сторожа срока работы
	std::thread guard([&cluster, &finished]() noexcept -> void {
		/**
		 * Выполняем обороты ожидания, покуда не выйдет срок
		 */
		for(uint16_t round = 0; (round < 300) && !finished.load(); ++round)
			// Выполняем ожидание одного оборота
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		// Если работа кластера ещё не кончилась
		if(!finished.load())
			// Останавливаем работу кластера по сроку
			cluster.stop();
	});
	// Запускаем работу кластера
	cluster.start();
	// Отмечаем, что работа кластера кончилась
	finished.store(true);
	// Дожидаемся завершения сторожа срока работы
	guard.join();
	/**
	 * Снимаем отбор проверки из окружения
	 */
	#if defined(_WIN32) || defined(_WIN64)
		::_putenv("GTEST_FILTER=");
	#endif
	// Если проверка идёт у мастера
	if(cluster.master())
		// Переживший работник обязан узнать о разрыве связи
		ASSERT_TRUE(dropped.load()) << "переживший работник не узнал о разрыве связи с выбывшим соседом";
}
