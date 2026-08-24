/**
 * @file late.cpp
 * @date 2026-08-03
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
 * @brief Проверка отбрасывания запоздалых ответов DNS-резолвера —
 *        ответ, пришедший после истечения срока ожидания, за настоящий приниматься не должен
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "dns.hpp"

/**
 * Подключаем заголовочный файл узла таймера
 */
#include "../../../include/unit/timer.hpp"

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>

/**
 * @brief Срок ожидания ответа сервера имён в проверках в миллисекундах
 *
 * @note Срок берётся коротким намеренно: проверка ждёт его столько раз, сколько
 *       задано попыток, и длинный превратил бы набор в испытание терпения
 *
 */
static constexpr uint32_t TEST_DELAY = 300;

/**
 * @brief Доменное имя, по которому ведутся проверки
 *
 * @details Область верхнего уровня «test» отведена договором RFC 2606 под
 *          проверки и в движении не встречается: имя это не попадёт ни в файл
 *          узлов, ни в кэш, и разрешить его настоящим сервером нельзя
 *
 */
static constexpr const char * PROMPT_DOMAIN = "prompt.test";

/**
 * @brief Доменное имя, по которому ведётся проверка запоздалого ответа
 *
 * @warning Имя у каждой проверки своё, и общего брать нельзя: разрешённое имя
 *          оседает в кэше, и следующая проверка получила бы ответ оттуда, вопроса
 *          серверу не задав вовсе
 *
 */
static constexpr const char * LATE_DOMAIN = "late.test";

/**
 * @brief Доменное имя, по которому ведётся проверка счёта попыток
 *
 */
static constexpr const char * SILENT_DOMAIN = "silent.test";

/**
 * @brief Доменное имя, по которому ведётся проверка чужого ответа
 *
 */
static constexpr const char * FOREIGN_DOMAIN = "foreign.test";

/**
 * @brief Доменное имя, по которому ведётся проверка переноса порта
 *
 */
static constexpr const char * PORTED_DOMAIN = "ported.test";

/**
 * @brief Адрес, которым подставной сервер отвечает на всякий вопрос
 *
 */
static constexpr const char * STUB_ADDRESS = "192.0.2.77";

/**
 * @brief Проверка приёма своевременного ответа сервера имён
 *
 * @details Проверка эта задаёт основание для проверки запоздалого ответа: без неё
 *          та прошла бы и при подставном сервере вовсе неработающем, поскольку
 *          обе ждут отсутствия адреса. Здесь сервер отвечает сразу, и адрес
 *          обязан быть получен
 *
 */
TEST_F(DNSUnitFixture, DnsPromptAnswerAccepted) {
	// Создаём подставной сервер имён
	DNSStubServer server;
	// Выполняем запуск подставного сервера имён, отвечающего сразу
	ASSERT_TRUE(server.start(0, false));
	// Выполняем проверку того, что порт сервером получен
	ASSERT_GT(server.port(), static_cast <uint16_t> (0));
	// Создаём объект DNS-резолвера
	awh::unit::dns_t dns(awh::event::family_t::IPV4, this->_fmk.get(), this->_log.get());
	// Выполняем настройку резолвера на подставной сервер имён
	this->setup(dns, server, TEST_DELAY, 1);
	// Собираемый итог разрешения доменного имени
	outcome_t result;
	// Объект работы с сетевыми адресами, которым адрес приводится к строке
	awh::net_addr_t addr(this->_fmk.get(), this->_log.get());
	/**
	 * Устанавливаем функцию обратного вызова на получение адреса
	 */
	dns.on <void (const awh::unit::dns_t::id_t, const awh::event::family_t, const std::string &, const awh::net::addr_t *)> (
		"address", [&addr, &result](const awh::unit::dns_t::id_t, const awh::event::family_t, const std::string & domain, const awh::net::addr_t * ip) noexcept -> void {
			// Запоминаем, что адрес получен
			result.resolved = true;
			// Запоминаем доменное имя, по которому получен итог
			result.domain = domain;
			// Если адрес получен
			if(ip != nullptr){
				// Устанавливаем полученный адрес объекту работы с адресами
				addr.source(ip);
				// Запоминаем полученный адрес в виде строки
				result.address = static_cast <std::string> (addr);
			}
		}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4
	);
	/**
	 * Устанавливаем функцию обратного вызова на отказ разрешения
	 */
	dns.on <void (const awh::unit::dns_t::id_t, const awh::unit::dns_t::record_t, const std::string &)> (
		"failure", [&result](const awh::unit::dns_t::id_t, const awh::unit::dns_t::record_t, const std::string & domain) noexcept -> void {
			// Запоминаем, что разрешение завершилось отказом
			result.failed = true;
			// Запоминаем доменное имя, по которому получен итог
			result.domain = domain;
		}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
	);
	/**
	 * Устанавливаем функцию обратного вызова на событие резолвера
	 */
	dns.on <void (const awh::event::status_t)> (
		"status", [&dns](const awh::event::status_t status) noexcept -> void {
			// Если резолвер запущен
			if(status == awh::event::status_t::LAUNCHED)
				// Выполняем разрешение доменного имени
				dns.resolve(dns.issue(), awh::event::family_t::IPV4, PROMPT_DOMAIN);
		}, std::placeholders::_1
	);
	/**
	 * Узел таймера, которым работа резолвера завершается
	 *
	 * @note Останавливать работу из отклика на полученный адрес нельзя: цикл событий
	 *       один на процесс, и не останови его проверка - следующая в нём и застрянет,
	 *       а отклика может не случиться вовсе. Таймер завершает работу в любом исходе
	 */
	awh::unit::timer_t timer(this->_fmk.get(), this->_log.get());
	// Заводим таймер остановки работы резолвера
	const awh::event::id_t tid = timer.timeout(TEST_DELAY);
	// Устанавливаем функцию обратного вызова на срабатывание таймера
	timer.on <void (const awh::event::id_t)> (
		tid, [&dns, &timer](const awh::event::id_t) noexcept -> void {
			// Выполняем остановку работы таймера
			timer.stop();
			// Выполняем остановку работы резолвера
			dns.stop();
		}, std::placeholders::_1
	);
	// Выполняем запуск работы резолвера
	dns.start();
	// Выполняем остановку подставного сервера имён
	server.stop();
	// Выполняем проверку того, что сервер вопрос принял
	ASSERT_GE(server.received(), static_cast <uint32_t> (1));
	// Выполняем проверку того, что адрес получен
	ASSERT_TRUE(result.resolved);
	// Выполняем проверку того, что разрешение отказом не завершилось
	ASSERT_FALSE(result.failed);
	// Выполняем проверку полученного адреса
	ASSERT_EQ(result.address, std::string(STUB_ADDRESS));
}

/**
 * @brief Проверка отбрасывания запоздалого ответа сервера имён
 *
 * @details Сервер отвечает позже, чем истекает срок ожидания у всех заданных
 *          попыток. К мигу прихода ответа резолвер уже отчитался отказом, и
 *          принять запоздалый ответ он не вправе: вопрос снят с учёта, а событие
 *          обмена возвращено в очередь свободных и может быть занято вопросом иным
 *
 * @note Проверка ждёт после отказа особо, давая запоздалому ответу дойти: без
 *       этого ожидания она завершилась бы прежде, чем ответ вообще придёт, и
 *       ничего бы не доказывала
 *
 */
TEST_F(DNSUnitFixture, DnsLateAnswerDiscarded) {
	// Создаём подставной сервер имён
	DNSStubServer server;
	/**
	 * Выполняем запуск подставного сервера имён, отвечающего с запозданием
	 *
	 * @warning Задержка обязана превосходить **всё** время ожидания резолвера, а не
	 *          один его срок: заданное число попыток означает число повторов, то есть
	 *          обращений выходит на одно больше, и ждёт резолвер столько сроков,
	 *          сколько сделал обращений. При сроке в 300 мс и одном повторе отказ
	 *          наступает к 600 мс - и задержка ответа ровно в 600 мс делала исход
	 *          проверки жребием: отказ и ответ приходились на одну миллисекунду, и
	 *          кто из них поспеет первым, решал случай. Проверка падала примерно раз
	 *          на пять прогонов, показывая при этом не дефект, а собственную ничью
	 *
	 * @note Запас взят двукратным: ответ приходит к 1200 мс, отказ наступает к 600 мс,
	 *       а работу завершает таймер к 1800 мс - каждое событие отделено от соседнего
	 *       временем, многократно превосходящим разброс планировщика
	 *
	 */
	ASSERT_TRUE(server.start(TEST_DELAY * 4, false));
	// Создаём объект DNS-резолвера
	awh::unit::dns_t dns(awh::event::family_t::IPV4, this->_fmk.get(), this->_log.get());
	// Выполняем настройку резолвера на подставной сервер имён
	this->setup(dns, server, TEST_DELAY, 1);
	// Собираемый итог разрешения доменного имени
	outcome_t result;
	/**
	 * Устанавливаем функцию обратного вызова на получение адреса
	 *
	 * @note Вызов её здесь и означает дефект: принят ответ, чей срок уже истёк
	 */
	dns.on <void (const awh::unit::dns_t::id_t, const awh::event::family_t, const std::string &, const awh::net::addr_t *)> (
		"address", [&result](const awh::unit::dns_t::id_t, const awh::event::family_t, const std::string & domain, const awh::net::addr_t * ip) noexcept -> void {
			// Запоминаем, что адрес получен
			result.resolved = true;
			// Запоминаем доменное имя, по которому получен итог
			result.domain = domain;
			// Если адрес получен
			if(ip != nullptr)
				// Запоминаем размер полученного адреса
				result.address.assign(static_cast <size_t> (ip->size), '.');
		}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4
	);
	/**
	 * Устанавливаем функцию обратного вызова на исчерпание числа попыток
	 */
	dns.on <void (const awh::unit::dns_t::id_t, const std::string &, const uint8_t)> (
		"attempts", [&result](const awh::unit::dns_t::id_t, const std::string &, const uint8_t attempts) noexcept -> void {
			// Запоминаем число выполненных попыток обращения к серверу имён
			result.attempts = attempts;
		}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
	);
	/**
	 * Устанавливаем функцию обратного вызова на отказ разрешения
	 *
	 * @note Остановка здесь не выполняется: запоздалый ответ ещё в пути, и
	 *       остановившись сразу, проверка не увидела бы, как резолвер с ним
	 *       обойдётся. Останавливает работу таймер
	 */
	dns.on <void (const awh::unit::dns_t::id_t, const awh::unit::dns_t::record_t, const std::string &)> (
		"failure", [&result](const awh::unit::dns_t::id_t, const awh::unit::dns_t::record_t, const std::string & domain) noexcept -> void {
			// Запоминаем, что разрешение завершилось отказом
			result.failed = true;
			// Запоминаем доменное имя, по которому получен итог
			result.domain = domain;
		}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
	);
	/**
	 * Устанавливаем функцию обратного вызова на событие резолвера
	 */
	dns.on <void (const awh::event::status_t)> (
		"status", [&dns](const awh::event::status_t status) noexcept -> void {
			// Если резолвер запущен
			if(status == awh::event::status_t::LAUNCHED)
				// Выполняем разрешение проверяемого доменного имени
				dns.resolve(dns.issue(), awh::event::family_t::IPV4, LATE_DOMAIN);
		}, std::placeholders::_1
	);
	// Создаём объект узла таймера, которым работа резолвера завершается
	awh::unit::timer_t timer(this->_fmk.get(), this->_log.get());
	/**
	 * Заводим таймер завершения работы резолвера
	 *
	 * @note Срок его берётся заведомо больший задержки ответа сервера: к мигу
	 *       срабатывания запоздалый ответ уже дошёл, и видно, как резолвер с ним
	 *       обошёлся. Держать работу вторым доменным именем нельзя - отказ по нему
	 *       приходит вместе с отказом по первому, и до прихода ответа дело не дошло бы
	 */
	const awh::event::id_t eid = timer.timeout(TEST_DELAY * 6);
	// Устанавливаем функцию обратного вызова на срабатывание таймера
	timer.on <void (const awh::event::id_t)> (
		eid, [&dns, &timer](const awh::event::id_t) noexcept -> void {
			// Выполняем остановку работы таймера
			timer.stop();
			// Выполняем остановку работы резолвера
			dns.stop();
		}, std::placeholders::_1
	);
	// Выполняем запуск работы резолвера
	dns.start();
	// Выполняем остановку подставного сервера имён
	server.stop();
	// Выполняем проверку того, что сервер вопросы принял
	ASSERT_GE(server.received(), static_cast <uint32_t> (1));
	// Выполняем проверку того, что разрешение завершилось отказом по истечении срока
	ASSERT_TRUE(result.failed);
	// Выполняем проверку доменного имени, по которому получен отказ
	ASSERT_EQ(result.domain, std::string(LATE_DOMAIN));
	/**
	 * Выполняем проверку того, что запоздалый ответ за настоящий не принят
	 */
	ASSERT_FALSE(result.resolved);
}

/**
 * @brief Проверка счёта попыток обращения к серверу имён
 *
 * @details Сервер не отвечает вовсе, и резолвер обязан обратиться к нему столько
 *          раз, сколько задано попыток, а затем отчитаться отказом
 *
 */
TEST_F(DNSUnitFixture, DnsSilentServerAttempts) {
	// Создаём подставной сервер имён
	DNSStubServer server;
	// Выполняем запуск подставного сервера имён, не отвечающего вовсе
	ASSERT_TRUE(server.start(0, true));
	// Создаём объект DNS-резолвера
	awh::unit::dns_t dns(awh::event::family_t::IPV4, this->_fmk.get(), this->_log.get());
	// Выполняем настройку резолвера на подставной сервер имён
	this->setup(dns, server, TEST_DELAY, 3);
	// Собираемый итог разрешения доменного имени
	outcome_t result;
	/**
	 * Устанавливаем функцию обратного вызова на получение адреса
	 */
	dns.on <void (const awh::unit::dns_t::id_t, const awh::event::family_t, const std::string &, const awh::net::addr_t *)> (
		"address", [&result](const awh::unit::dns_t::id_t, const awh::event::family_t, const std::string &, const awh::net::addr_t *) noexcept -> void {
			// Запоминаем, что адрес получен
			result.resolved = true;
		}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4
	);
	/**
	 * Устанавливаем функцию обратного вызова на исчерпание числа попыток
	 */
	dns.on <void (const awh::unit::dns_t::id_t, const std::string &, const uint8_t)> (
		"attempts", [&result](const awh::unit::dns_t::id_t, const std::string &, const uint8_t attempts) noexcept -> void {
			// Запоминаем число выполненных попыток обращения к серверу имён
			result.attempts = attempts;
		}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
	);
	/**
	 * Устанавливаем функцию обратного вызова на отказ разрешения
	 */
	dns.on <void (const awh::unit::dns_t::id_t, const awh::unit::dns_t::record_t, const std::string &)> (
		"failure", [&dns, &result](const awh::unit::dns_t::id_t, const awh::unit::dns_t::record_t, const std::string & domain) noexcept -> void {
			// Запоминаем, что разрешение завершилось отказом
			result.failed = true;
			// Запоминаем доменное имя, по которому получен итог
			result.domain = domain;
			// Выполняем остановку работы резолвера
			dns.stop();
		}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
	);
	/**
	 * Устанавливаем функцию обратного вызова на событие резолвера
	 */
	dns.on <void (const awh::event::status_t)> (
		"status", [&dns](const awh::event::status_t status) noexcept -> void {
			// Если резолвер запущен
			if(status == awh::event::status_t::LAUNCHED)
				// Выполняем разрешение доменного имени
				dns.resolve(dns.issue(), awh::event::family_t::IPV4, SILENT_DOMAIN);
		}, std::placeholders::_1
	);
	// Выполняем запуск работы резолвера
	dns.start();
	// Выполняем остановку подставного сервера имён
	server.stop();
	// Выполняем проверку того, что адрес получен не был
	ASSERT_FALSE(result.resolved);
	// Выполняем проверку того, что разрешение завершилось отказом
	ASSERT_TRUE(result.failed);
	/**
	 * Выполняем проверку того, что обращений было на одно больше заданных попыток
	 *
	 * @par Намеренные решения
	 * Заданное число попыток означает попытки **дополнительные**, а не общее число
	 * обращений: вопрос задаётся сам собой, а попытка - это разрешение задать его
	 * ещё раз, когда срок ожидания вышел. Оттого при трёх попытках сервер
	 * опрашивается четырежды - первый раз и трижды заново, - а полное время
	 * ожидания вызывающего есть срок, умноженный на число попыток плюс одно
	 *
	 * @warning Сличается точное число, а не нижняя граница. Прежде стояла граница, и
	 *          она пропускала подмену толкования в любую сторону: сойди счёт на
	 *          единицу - и вызывающий ждал бы не то время, которое ему обещано, а
	 *          проверка этого бы не заметила. Ослабьте её до `ASSERT_GE`, и договор
	 *          перестанет стеречься
	 */
	ASSERT_EQ(server.received(), static_cast <uint32_t> (4));
	// Выполняем проверку того, что резолвер отчитался заданным числом попыток
	ASSERT_EQ(result.attempts, static_cast <uint8_t> (3));
}

/**
 * @brief Проверка того, что чужой ответ ожидания не отменяет
 *
 * @details Сервер отвечает на первый вопрос подменённым номером, а дальше молчит.
 *          Ответ такой договору чужой, и отбросить его резолвер обязан - но
 *          отбросить, не прекращая ожидания: вопрос-то остался без ответа
 *
 *          Прежде выходило иначе. Одноразовый срок ожидания снимается приходом
 *          данных, и снимается движком **до** вызова отклика, где чужой номер
 *          только и распознаётся. К мигу отбрасывания срока уже не было, взводить
 *          его заново нечем - и вопрос повисал навсегда, отказом не завершаясь
 *
 * @note Проверяется не только сам отказ, но и **время** его наступления: ожидание
 *       обязано продолжиться с прерванного места, а не начаться заново. Начнись оно
 *       заново, и каждый чужой ответ отодвигал бы отказ, а поток таких ответов не
 *       дал бы наступить ему вовсе - то есть затык вернулся бы, только медленнее
 *
 */
TEST_F(DNSUnitFixture, DnsForeignAnswerKeepsWaiting) {
	// Создаём подставной сервер имён
	DNSStubServer server;
	/**
	 * Выполняем запуск сервера, отвечающего чужим номером на всякий вопрос
	 *
	 * @note Чужим отвечается всегда, а не однажды: у резолвера заданное число попыток
	 *       означает число **повторов**, и ответь сервер на повтор как положено, вопрос
	 *       разрешился бы - проверять было бы нечего
	 */
	ASSERT_TRUE(server.start(0, false, UINT32_MAX));
	// Создаём объект DNS-резолвера
	awh::unit::dns_t dns(awh::event::family_t::IPV4, this->_fmk.get(), this->_log.get());
	// Выполняем настройку резолвера на подставной сервер имён
	this->setup(dns, server, TEST_DELAY, 1);
	// Собираемый итог разрешения доменного имени
	outcome_t result;
	/**
	 * Устанавливаем функцию обратного вызова на получение адреса
	 *
	 * @note Вызов её здесь означает, что чужой ответ принят за свой
	 */
	dns.on <void (const awh::unit::dns_t::id_t, const awh::event::family_t, const std::string &, const awh::net::addr_t *)> (
		"address", [&dns, &result](const awh::unit::dns_t::id_t, const awh::event::family_t, const std::string & domain, const awh::net::addr_t *) noexcept -> void {
			// Запоминаем, что адрес получен
			result.resolved = true;
			// Запоминаем доменное имя, по которому получен итог
			result.domain = domain;
			/**
			 * Выполняем остановку работы резолвера
			 *
			 * @note Останов здесь означает несовпадение ожидаемого: адреса быть не
			 *       должно вовсе. Без него проверка не падала бы, а зависала - разницы
			 *       в исходе никакой, зато читать вывод куда труднее
			 */
			dns.stop();
		}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4
	);
	/**
	 * Устанавливаем функцию обратного вызова на отказ разрешения
	 */
	dns.on <void (const awh::unit::dns_t::id_t, const awh::unit::dns_t::record_t, const std::string &)> (
		"failure", [&dns, &result](const awh::unit::dns_t::id_t, const awh::unit::dns_t::record_t, const std::string & domain) noexcept -> void {
			// Запоминаем, что разрешение завершилось отказом
			result.failed = true;
			// Запоминаем доменное имя, по которому получен итог
			result.domain = domain;
			// Выполняем остановку работы резолвера
			dns.stop();
		}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
	);
	/**
	 * Устанавливаем функцию обратного вызова на событие резолвера
	 */
	dns.on <void (const awh::event::status_t)> (
		"status", [&dns](const awh::event::status_t status) noexcept -> void {
			// Если резолвер запущен
			if(status == awh::event::status_t::LAUNCHED)
				// Выполняем разрешение доменного имени
				dns.resolve(dns.issue(), awh::event::family_t::IPV4, FOREIGN_DOMAIN);
		}, std::placeholders::_1
	);
	// Замеряем время начала обращения к серверу имён
	const auto start = std::chrono::steady_clock::now();
	// Выполняем запуск работы резолвера
	dns.start();
	// Запоминаем время, потраченное на обращение
	const int64_t spent = std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count();
	// Выполняем остановку подставного сервера имён
	server.stop();
	// Выполняем проверку того, что сервер вопрос принял
	ASSERT_GE(server.received(), static_cast <uint32_t> (1));
	// Выполняем проверку того, что чужой ответ за свой не принят
	ASSERT_FALSE(result.resolved);
	// Выполняем проверку того, что разрешение завершилось отказом
	ASSERT_TRUE(result.failed);
	// Выполняем проверку доменного имени, по которому получен отказ
	ASSERT_EQ(result.domain, std::string(FOREIGN_DOMAIN));
	/**
	 * Выполняем проверку того, что отказ наступил в свой срок, а не позже
	 *
	 * @note Вопрос задаётся дважды - сам вопрос да один повтор, - и на каждый уходит
	 *       по сроку ожидания. Верхняя граница берётся с запасом ещё в один срок:
	 *       ожидание продолжается остатком, и чужой ответ отодвинуть отказ не вправе
	 */
	ASSERT_LT(spent, static_cast <int64_t> (TEST_DELAY * 3));
}

/**
 * @brief Проверка того, что порт сервера переносится на заведённые события
 *
 * @details Резолвер заводит события обмена уже в конструкторе, и порт им достаётся
 *          отведённый договором - пятьдесят третий. Заданный позднее обязан
 *          перенестись на них, а не осесть в настройках без последствий
 *
 * @note Проверка ставит порт **последним**, уже после серверов: прежде метод
 *       работал лишь тем, что порт подхватывал `setServers()`, пересоздающий
 *       события, и заданный следом за ним пропадал молча. Вопросы тогда уходили на
 *       порт пятьдесят третий, где их никто не ждал, а распознать это можно было
 *       лишь перехватом движения
 *
 */
TEST_F(DNSUnitFixture, DnsTargetPortAppliesAfterServers) {
	// Создаём подставной сервер имён
	DNSStubServer server;
	// Выполняем запуск подставного сервера имён, отвечающего сразу
	ASSERT_TRUE(server.start(0, false));
	// Создаём объект DNS-резолвера
	awh::unit::dns_t dns(awh::event::family_t::IPV4, this->_fmk.get(), this->_log.get());
	// Устанавливаем срок ожидания ответа сервера имён
	dns.setTimeout(TEST_DELAY);
	// Устанавливаем число попыток обращения к серверу имён
	dns.setAttempts(1);
	// Устанавливаем адрес подставного сервера имён
	dns.setServers(awh::event::family_t::IPV4, {"127.0.0.1"});
	// Устанавливаем порт подставного сервера имён последним
	dns.setTargetPort(server.port());
	// Выполняем проверку того, что порт сервером получен
	ASSERT_GT(server.port(), static_cast <uint16_t> (0));
	// Собираемый итог разрешения доменного имени
	outcome_t result;
	/**
	 * Устанавливаем функцию обратного вызова на получение адреса
	 */
	dns.on <void (const awh::unit::dns_t::id_t, const awh::event::family_t, const std::string &, const awh::net::addr_t *)> (
		"address", [&dns, &result](const awh::unit::dns_t::id_t, const awh::event::family_t, const std::string & domain, const awh::net::addr_t *) noexcept -> void {
			// Запоминаем, что адрес получен
			result.resolved = true;
			// Запоминаем доменное имя, по которому получен итог
			result.domain = domain;
			// Выполняем остановку работы резолвера
			dns.stop();
		}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4
	);
	/**
	 * Устанавливаем функцию обратного вызова на отказ разрешения
	 */
	dns.on <void (const awh::unit::dns_t::id_t, const awh::unit::dns_t::record_t, const std::string &)> (
		"failure", [&dns, &result](const awh::unit::dns_t::id_t, const awh::unit::dns_t::record_t, const std::string & domain) noexcept -> void {
			// Запоминаем, что разрешение завершилось отказом
			result.failed = true;
			// Запоминаем доменное имя, по которому получен итог
			result.domain = domain;
			// Выполняем остановку работы резолвера
			dns.stop();
		}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
	);
	/**
	 * Устанавливаем функцию обратного вызова на событие резолвера
	 */
	dns.on <void (const awh::event::status_t)> (
		"status", [&dns](const awh::event::status_t status) noexcept -> void {
			// Если резолвер запущен
			if(status == awh::event::status_t::LAUNCHED)
				// Выполняем разрешение доменного имени
				dns.resolve(dns.issue(), awh::event::family_t::IPV4, PORTED_DOMAIN);
		}, std::placeholders::_1
	);
	// Выполняем запуск работы резолвера
	dns.start();
	// Выполняем остановку подставного сервера имён
	server.stop();
	/**
	 * Выполняем проверку того, что вопрос дошёл до подставного сервера
	 *
	 * @note Именно это и есть суть проверки: не дойди порт до событий, вопрос ушёл
	 *       бы на пятьдесят третий, и сервер не увидел бы ничего
	 */
	ASSERT_GE(server.received(), static_cast <uint32_t> (1));
	// Выполняем проверку того, что адрес получен
	ASSERT_TRUE(result.resolved);
	// Выполняем проверку того, что разрешение отказом не завершилось
	ASSERT_FALSE(result.failed);
}

/**
 * @brief Проверка того, что номера вопросов выдаются случайными
 *
 * @details Проверка закрепляет намеренное решение, разобранное при `issue()`:
 *          номер выдаётся случайным, а не приращением счётчика. Прежде здесь стоял
 *          счётчик, и заменён он был молча - проверка эта и заведена затем, чтобы
 *          обратная замена не прошла незамеченной
 *
 * @note Сличается не «случайность» как таковая - доказать её проверкой нельзя, -
 *       а её единственное наблюдаемое следствие: подряд выданные номера не идут
 *       подряд по значению. Счётчик с приращением такую проверку не пройдёт
 *
 */
TEST_F(DNSUnitFixture, DnsIdentifiersAreNotSequential) {
	// Создаём объект DNS-резолвера
	awh::unit::dns_t dns(awh::event::family_t::IPV4, this->_fmk.get(), this->_log.get());
	// Число выдаваемых для проверки номеров
	constexpr size_t COUNT = 32;
	// Число пар, идущих подряд по значению
	size_t sequential = 0;
	// Ранее выданный номер вопроса
	awh::unit::dns_t::id_t previous = dns.issue();
	/**
	 * Выполняем выдачу номеров вопросов
	 */
	for(size_t i = 1; i < COUNT; i++){
		// Получаем очередной номер вопроса
		const awh::unit::dns_t::id_t current = dns.issue();
		// Выполняем проверку того, что нулевой номер не выдаётся
		ASSERT_GT(current, static_cast <awh::unit::dns_t::id_t> (0));
		// Если очередной номер идёт следом за прежним по значению
		if(current == static_cast <awh::unit::dns_t::id_t> (previous + 1))
			// Выполняем подсчёт пар, идущих подряд
			sequential++;
		// Запоминаем очередной номер вопроса как прежний
		previous = current;
	}
	/**
	 * Выполняем проверку того, что номера подряд не идут
	 *
	 * @note Граница берётся с запасом, а не в ноль: случайной выдаче ничто не
	 *       мешает выдать соседей и дважды, и вероятность такого при трёх десятках
	 *       номеров не пренебрежимо мала. Счётчик же даст подряд **все** пары, и
	 *       от запаса это его не спасёт
	 */
	ASSERT_LT(sequential, static_cast <size_t> (COUNT / 4));
}
