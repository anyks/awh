/**
 * @file: dns.hpp
 * @date: 2026-08-03
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры DNS-резолвера —
 *        подставной сервер имён и ожидание итога разрешения доменного имени
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT_DNS_TESTS__
#define __AWH_UNIT_DNS_TESTS__

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/units/dns.hpp"

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>
#include <thread>
#include <vector>
#include <string>

/**
 * @brief Подставной сервер имён для проверки резолвера
 *
 * @details Сервер держит дейтаграммный сокет на устройстве петли и отвечает на
 *          вопросы сам, своей нитью. Настоящий сервер имён для проверки не годится:
 *          проверяется поведение резолвера при ответе запоздалом, а заставить
 *          настоящий сервер запоздать нечем
 *
 * @note Ответ собирается сжатой ссылкой на вопрос, как того требует RFC 1035:
 *       имя в ответе не повторяется, а указывается смещением к вопросу
 *
 */
class DNSStubServer {
	private:
		// Дескриптор дейтаграммного сокета сервера
		int32_t _fd;
		// Порт, на котором сервер принимает вопросы
		uint16_t _port;
	private:
		// Задержка ответа на вопрос в миллисекундах
		uint32_t _delay;
		// Признак того, что отвечать на вопросы не следует вовсе
		bool _silent;
		/**
		 * Число ответов, выдаваемых с подменённым номером вопроса
		 *
		 * @note Ответ с чужим номером договором положено отбрасывать, а ожидание при
		 *       этом продолжать. Столько первых ответов сервер и портит, а дальше
		 *       отвечает как обычно
		 */
		uint32_t _foreign;
	private:
		// Признак работы нити сервера
		std::atomic <bool> _running;
		// Число принятых сервером вопросов
		std::atomic <uint32_t> _received;
	private:
		// Нить, которой ведётся приём вопросов
		std::thread _thread;
	private:
		/**
		 * @brief Метод приёма вопросов и выдачи ответов
		 *
		 */
		void run() noexcept;
	public:
		/**
		 * @brief Метод получения порта, на котором сервер принимает вопросы
		 *
		 * @return порт сервера
		 *
		 */
		uint16_t port() const noexcept;
		/**
		 * @brief Метод получения числа принятых сервером вопросов
		 *
		 * @return число принятых вопросов
		 *
		 */
		uint32_t received() const noexcept;
	public:
		/**
		 * @brief Метод запуска сервера
		 *
		 * @param delay   задержка ответа на вопрос в миллисекундах
		 * @param silent  признак того, что отвечать на вопросы не следует вовсе
		 * @param foreign число ответов, выдаваемых с подменённым номером вопроса
		 * @return        результат запуска сервера
		 *
		 */
		bool start(const uint32_t delay, const bool silent, const uint32_t foreign = 0) noexcept;
		/**
		 * @brief Метод остановки сервера
		 *
		 */
		void stop() noexcept;
	public:
		/**
		 * @brief Конструктор
		 *
		 */
		DNSStubServer() noexcept;
		/**
		 * @brief Деструктор
		 *
		 */
		~DNSStubServer() noexcept;
};

/**
 * @brief Класс фикстуры для проверок DNS-резолвера
 *
 * @details Резолвер ведёт обмен циклом базы событий, и цикл этот один на процесс:
 *          запускает его тот, кто первым позвал `start()`, а останавливает он же.
 *          Оттого каждая проверка обязана цикл остановить - иначе следующая в нём
 *          и застрянет
 *
 */
class DNSUnitFixture : public testing::Test {
	protected:
		// Объект фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
	public:
		/**
		 * @brief Метод настройки тестового окружения
		 *
		 */
		void SetUp();
		/**
		 * @brief Метод очистки тестового окружения
		 *
		 */
		void TearDown();
	protected:
		/**
		 * @brief Структура итога разрешения доменного имени
		 *
		 */
		typedef struct Outcome {
			// Признак того, что адрес получен
			bool resolved;
			// Признак того, что разрешение завершилось отказом
			bool failed;
			// Полученный адрес в виде строки
			std::string address;
			// Доменное имя, по которому получен итог
			std::string domain;
			// Число выполненных попыток обращения к серверу имён
			uint8_t attempts;
			/**
			 * @brief Конструктор
			 *
			 */
			Outcome() noexcept :
			 resolved(false), failed(false), address{""}, domain{""}, attempts(0) {}
		} outcome_t;
	protected:
		/**
		 * @brief Метод настройки резолвера на подставной сервер имён
		 *
		 * @param dns    объект DNS-резолвера
		 * @param server подставной сервер имён
		 * @param delay  срок ожидания ответа сервера в миллисекундах
		 * @param count  число попыток обращения к серверу имён
		 *
		 */
		void setup(awh::unit::dns_t & dns, const DNSStubServer & server, const uint32_t delay, const uint8_t count) const noexcept;
};

#endif // __AWH_UNIT_DNS_TESTS__
