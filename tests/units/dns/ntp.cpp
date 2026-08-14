/**
 * @file ntp.cpp
 * @date 2026-08-04
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
 * @brief Проверка обхождения NTP-клиента с чужими ответами —
 *        ответ не на заданный вопрос ожидания прекращать не должен
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "dns.hpp"
#include "../../../include/units/ntp.hpp"

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>

/**
 * @brief Срок ожидания ответа сервера времени в проверках в миллисекундах
 *
 */
static constexpr uint32_t NTP_DELAY = 300;

/**
 * @brief Проверка приёма своевременного ответа сервера времени
 *
 * @details Проверка задаёт основание для проверки чужого ответа: без неё та прошла
 *          бы и при подставном сервере вовсе неработающем
 *
 */
TEST_F(DNSUnitFixture, NtpOwnAnswerAccepted) {
	// Создаём подставной сервер времени
	NTPStubServer server;
	// Выполняем запуск подставного сервера времени, отвечающего как положено
	ASSERT_TRUE(server.start(false));
	// Создаём объект NTP-клиента
	awh::unit::ntp_t ntp(this->_fmk.get(), this->_log.get());
	// Устанавливаем порт подставного сервера времени
	ntp.setTargetPort(server.port());
	// Устанавливаем срок ожидания ответа сервера времени
	ntp.setTimeout(NTP_DELAY);
	// Устанавливаем число попыток обращения к серверу времени
	ntp.setAttempts(1);
	// Устанавливаем адрес подставного сервера времени
	ntp.setServers(awh::event::family_t::IPV4, {"127.0.0.1"});
	// Выполняем инициализацию NTP-клиента
	ASSERT_TRUE(ntp.init(awh::event::family_t::IPV4));
	// Полученная от сервера метка времени
	uint64_t stamp = 0;
	/**
	 * Устанавливаем функцию обратного вызова на получение метки времени
	 */
	ntp.on <void (const uint64_t)> (
		"timestamp", [&ntp, &stamp](const uint64_t timestamp) noexcept -> void {
			// Запоминаем полученную метку времени
			stamp = timestamp;
			// Выполняем остановку работы клиента
			ntp.stop();
		}, std::placeholders::_1
	);
	/**
	 * Устанавливаем функцию обратного вызова на исчерпание числа попыток
	 */
	ntp.on <void (const uint8_t)> (
		"attempts", [&ntp](const uint8_t) noexcept -> void {
			// Выполняем остановку работы клиента
			ntp.stop();
		}, std::placeholders::_1
	);
	/**
	 * Устанавливаем функцию обратного вызова на событие клиента
	 */
	ntp.on <void (const awh::event::status_t)> (
		"status", [&ntp](const awh::event::status_t status) noexcept -> void {
			// Если клиент запущен
			if(status == awh::event::status_t::LAUNCHED)
				// Выполняем синхронизацию времени с сервером
				ntp.sync();
		}, std::placeholders::_1
	);
	// Выполняем запуск работы клиента
	ntp.start();
	// Выполняем остановку подставного сервера времени
	server.stop();
	// Выполняем проверку того, что сервер вопрос принял
	ASSERT_GE(server.received(), static_cast <uint32_t> (1));
	// Выполняем проверку того, что метка времени получена
	ASSERT_GT(stamp, static_cast <uint64_t> (0));
}

/**
 * @brief Проверка того, что чужой ответ сервера времени ожидания не прекращает
 *
 * @details Сервер отвечает, не переписав в ответ метку из вопроса. Ответ такой
 *          заданному вопросу не принадлежит - примета своего ответа именно в этой
 *          метке, - и принять его клиент не вправе
 *
 *          Прежде он не просто отбрасывал такой ответ, а **завершал им обмен**:
 *          признак ожидания снимался прежде разбора, а вызывающему уходил отказ по
 *          вопросу, ответ на который был ещё в пути. Один заблудившийся пакет губил
 *          законную синхронизацию, а подложным ответом её можно было сорвать нарочно
 *
 * @note Проверяется и время: обмен обязан продлиться отпущенные ему сроки, а не
 *       оборваться мгновением прихода чужого пакета
 *
 */
TEST_F(DNSUnitFixture, NtpForeignAnswerKeepsWaiting) {
	// Создаём подставной сервер времени
	NTPStubServer server;
	// Выполняем запуск подставного сервера времени, отвечающего чужой меткой
	ASSERT_TRUE(server.start(true));
	// Создаём объект NTP-клиента
	awh::unit::ntp_t ntp(this->_fmk.get(), this->_log.get());
	// Устанавливаем порт подставного сервера времени
	ntp.setTargetPort(server.port());
	// Устанавливаем срок ожидания ответа сервера времени
	ntp.setTimeout(NTP_DELAY);
	// Устанавливаем число попыток обращения к серверу времени
	ntp.setAttempts(1);
	// Устанавливаем адрес подставного сервера времени
	ntp.setServers(awh::event::family_t::IPV4, {"127.0.0.1"});
	// Выполняем инициализацию NTP-клиента
	ASSERT_TRUE(ntp.init(awh::event::family_t::IPV4));
	// Полученная от сервера метка времени
	uint64_t stamp = 0;
	// Признак того, что попытки обращения исчерпаны
	bool exhausted = false;
	/**
	 * Устанавливаем функцию обратного вызова на получение метки времени
	 *
	 * @note Вызов её здесь означает, что чужой ответ принят за свой
	 */
	ntp.on <void (const uint64_t)> (
		"timestamp", [&ntp, &stamp](const uint64_t timestamp) noexcept -> void {
			// Запоминаем полученную метку времени
			stamp = timestamp;
			// Выполняем остановку работы клиента
			ntp.stop();
		}, std::placeholders::_1
	);
	/**
	 * Устанавливаем функцию обратного вызова на исчерпание числа попыток
	 */
	ntp.on <void (const uint8_t)> (
		"attempts", [&ntp, &exhausted](const uint8_t) noexcept -> void {
			// Запоминаем, что попытки обращения исчерпаны
			exhausted = true;
			// Выполняем остановку работы клиента
			ntp.stop();
		}, std::placeholders::_1
	);
	/**
	 * Устанавливаем функцию обратного вызова на событие клиента
	 */
	ntp.on <void (const awh::event::status_t)> (
		"status", [&ntp](const awh::event::status_t status) noexcept -> void {
			// Если клиент запущен
			if(status == awh::event::status_t::LAUNCHED)
				// Выполняем синхронизацию времени с сервером
				ntp.sync();
		}, std::placeholders::_1
	);
	// Замеряем время начала обращения к серверу времени
	const auto start = std::chrono::steady_clock::now();
	// Выполняем запуск работы клиента
	ntp.start();
	// Запоминаем время, потраченное на обращение
	const int64_t spent = std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count();
	// Выполняем остановку подставного сервера времени
	server.stop();
	// Выполняем проверку того, что сервер вопрос принял
	ASSERT_GE(server.received(), static_cast <uint32_t> (1));
	// Выполняем проверку того, что чужой ответ за свой не принят
	ASSERT_EQ(stamp, static_cast <uint64_t> (0));
	// Выполняем проверку того, что обмен завершился исчерпанием попыток
	ASSERT_TRUE(exhausted);
	/**
	 * Выполняем проверку того, что обмен продлился отпущенные ему сроки
	 *
	 * @note Нижняя граница берётся в один срок ожидания: оборвись обмен приходом
	 *       чужого пакета, потрачены были бы единицы миллисекунд
	 */
	ASSERT_GE(spent, static_cast <int64_t> (NTP_DELAY));
}
