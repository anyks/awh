/**
 * @file: natpmp.cpp
 * @date: 2026-08-03
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Проверка опознавания ответов договора NAT-PMP —
 *        принятие ответа на свою просьбу и отбрасывание ответа на чужую
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "portmap.hpp"

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>
#include <thread>
#include <cstring>

/**
 * Системные заголовочные файлы
 */
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * @brief Порт договора NAT-PMP
 *
 */
static constexpr uint16_t NATPMP_PORT = 5351;

/**
 * @brief Внутренний порт заводимого перенаправления
 *
 */
static constexpr uint16_t INTERNAL_PORT = 8081;

/**
 * @brief Отметка ответа в коде действия договора
 *
 */
static constexpr uint8_t RESPONSE_FLAG = 0x80;

/**
 * @brief Класс поддельного маршрутизатора договора NAT-PMP
 *
 * @details Отвечает на просьбу так, как отвечал бы маршрутизатор, но внутренний порт
 *          в ответе задаётся испытанием. Настоящего маршрутизатора для этого не нужно,
 *          и ответ приходит одинаково на любой машине
 *
 * @note Обмен ведётся обычными системными вызовами, а не движком: движок занят циклом
 *       базы событий модуля, и вмешиваться в него отсюда незачем
 *
 */
class Router {
	private:
		// Дескриптор гнезда поддельного маршрутизатора
		int _socket;
		// Поток обмена поддельного маршрутизатора
		std::thread _thread;
		// Признак продолжения работы поддельного маршрутизатора
		std::atomic <bool> _working;
	public:
		/**
		 * @brief Метод проверки готовности поддельного маршрутизатора
		 *
		 * @return признак готовности поддельного маршрутизатора
		 *
		 */
		bool ready() const noexcept {
			// Выводим признак того, что гнездо заведено
			return (this->_socket >= 0);
		}
		/**
		 * @brief Метод запуска поддельного маршрутизатора
		 *
		 * @param internalPort внутренний порт, называемый в ответе
		 *
		 */
		void start(const uint16_t internalPort) noexcept {
			// Запоминаем, что работа поддельного маршрутизатора начата
			this->_working.store(true);
			// Выполняем запуск потока обмена поддельного маршрутизатора
			this->_thread = std::thread([this, internalPort]() noexcept -> void {
				// Место под полученную просьбу
				uint8_t buffer[64] = {0};
				/**
				 * Выполняем обмен, пока работа не прекращена
				 */
				while(this->_working.load()){
					// Адрес обратившейся машины
					struct sockaddr_in client;
					// Размер адреса обратившейся машины
					socklen_t length = sizeof(client);
					// Выполняем очистку адреса обратившейся машины
					::memset(&client, 0, sizeof(client));
					// Выполняем чтение просьбы обратившейся машины
					const ssize_t size = ::recvfrom(this->_socket, buffer, sizeof(buffer), 0, reinterpret_cast <struct sockaddr *> (&client), &length);
					// Если просьба короче заголовка договора, ожидаем следующую
					if(size < 12) continue;
					// Собираемый ответ маршрутизатора
					uint8_t answer[16] = {0};
					// Записываем издание договора
					answer[0] = 0;
					// Записываем действие договора с отметкой ответа
					answer[1] = static_cast <uint8_t> (buffer[1] | RESPONSE_FLAG);
					// Записываем внутренний порт, называемый испытанием
					answer[8] = static_cast <uint8_t> (internalPort >> 8);
					// Записываем внутренний порт, называемый испытанием
					answer[9] = static_cast <uint8_t> (internalPort & 0xFF);
					// Записываем назначенный внешний порт
					answer[10] = static_cast <uint8_t> (INTERNAL_PORT >> 8);
					// Записываем назначенный внешний порт
					answer[11] = static_cast <uint8_t> (INTERNAL_PORT & 0xFF);
					// Записываем назначенный срок жизни перенаправления
					answer[15] = 60;
					// Выполняем отправку ответа обратившейся машине
					::sendto(this->_socket, answer, sizeof(answer), 0, reinterpret_cast <struct sockaddr *> (&client), length);
				}
			});
		}
		/**
		 * @brief Метод остановки поддельного маршрутизатора
		 *
		 */
		void stop() noexcept {
			// Запоминаем, что работа поддельного маршрутизатора прекращена
			this->_working.store(false);
			/**
			 * Если гнездо поддельного маршрутизатора заведено
			 */
			if(this->_socket >= 0){
				// Выполняем закрытие гнезда поддельного маршрутизатора
				::close(this->_socket);
				// Сбрасываем дескриптор гнезда поддельного маршрутизатора
				this->_socket = -1;
			}
			// Если поток обмена запущен, дожидаемся его завершения
			if(this->_thread.joinable()) this->_thread.join();
		}
	public:
		/**
		 * @brief Конструктор
		 *
		 */
		Router() noexcept : _socket(-1), _working(false) {
			// Выполняем заведение гнезда поддельного маршрутизатора
			this->_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
			// Если гнездо завести не удалось, выходим
			if(this->_socket < 0) return;
			// Адрес поддельного маршрутизатора
			struct sockaddr_in address;
			// Выполняем очистку адреса поддельного маршрутизатора
			::memset(&address, 0, sizeof(address));
			// Устанавливаем разновидность сети
			address.sin_family = AF_INET;
			// Устанавливаем порт договора
			address.sin_port = htons(NATPMP_PORT);
			// Устанавливаем адрес петли
			address.sin_addr.s_addr = ::inet_addr("127.0.0.1");
			/**
			 * Если привязать гнездо к порту договора не удалось
			 *
			 * @note Порт бывает занят настоящей службой перенаправления, и тогда
			 *       испытание не проводится, а не объявляется неудачным
			 */
			if(::bind(this->_socket, reinterpret_cast <struct sockaddr *> (&address), sizeof(address)) != 0){
				// Выполняем закрытие гнезда поддельного маршрутизатора
				::close(this->_socket);
				// Сбрасываем дескриптор гнезда поддельного маршрутизатора
				this->_socket = -1;
			}
		}
		/**
		 * @brief Деструктор
		 *
		 */
		~Router() noexcept {
			// Выполняем остановку поддельного маршрутизатора
			this->stop();
		}
};

/**
 * @brief Проверка принятия ответа на свою просьбу
 *
 * @details Поддельный маршрутизатор называет в ответе тот же внутренний порт, что был
 *          в просьбе, и модуль обязан такой ответ принять
 *
 */
TEST_F(PortmapUnitFixture, PortmapNatPmpOwnAnswer) {
	// Создаём поддельный маршрутизатор договора NAT-PMP
	Router router;
	// Если порт договора занят, испытание не проводится
	if(!router.ready()) GTEST_SKIP() << "NAT-PMP port is occupied";
	// Выполняем запуск поддельного маршрутизатора с тем же внутренним портом
	router.start(INTERNAL_PORT);
	// Создаём объект модуля перенаправления портов
	awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
	// Устанавливаем вид опроса маршрутизатора
	portmap.setType(awh::unit::portmap_t::type_t::NAT_PMP);
	// Устанавливаем адрес поддельного маршрутизатора
	portmap.setRouter("127.0.0.1");
	// Устанавливаем срок ожидания ответа маршрутизатора
	portmap.setTimeout(300);
	// Устанавливаем количество попыток обращения к маршрутизатору
	portmap.setAttempts(2);
	// Выполняем ожидание итога обращения к маршрутизатору
	const outcome_t outcome = this->await(portmap, awh::unit::portmap_t::action_t::OPEN);
	// Выполняем остановку поддельного маршрутизатора
	router.stop();
	// Выполняем проверку того, что ответ принят
	ASSERT_TRUE(outcome.answered);
	// Выполняем проверку того, что обращение отказом не завершилось
	ASSERT_FALSE(outcome.failed);
	// Выполняем проверку договора, по которому получен итог
	ASSERT_EQ(outcome.type, awh::unit::portmap_t::type_t::NAT_PMP);
}

/**
 * @brief Проверка отбрасывания ответа на чужую просьбу
 *
 * @details Отличительной метки договор NAT-PMP не имеет вовсе, и опознаётся ответ по
 *          внутреннему порту и договору перенаправления. Поддельный маршрутизатор
 *          называет в ответе чужой внутренний порт, и принять такой ответ за свой
 *          значило бы выдать вызывающему перенаправление, которого он не просил
 *
 * @note Ответ приходит на открытый порт, и прийти он может на чужую просьбу: проверка
 *       эта того же рода, что сличение отличительной метки у договора PCP
 *
 */
TEST_F(PortmapUnitFixture, PortmapNatPmpForeignAnswer) {
	// Создаём поддельный маршрутизатор договора NAT-PMP
	Router router;
	// Если порт договора занят, испытание не проводится
	if(!router.ready()) GTEST_SKIP() << "NAT-PMP port is occupied";
	// Выполняем запуск поддельного маршрутизатора с чужим внутренним портом
	router.start(INTERNAL_PORT + 1);
	// Создаём объект модуля перенаправления портов
	awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
	// Устанавливаем вид опроса маршрутизатора
	portmap.setType(awh::unit::portmap_t::type_t::NAT_PMP);
	// Устанавливаем адрес поддельного маршрутизатора
	portmap.setRouter("127.0.0.1");
	// Устанавливаем срок ожидания ответа маршрутизатора
	portmap.setTimeout(300);
	// Устанавливаем количество попыток обращения к маршрутизатору
	portmap.setAttempts(2);
	// Выполняем ожидание итога обращения к маршрутизатору
	const outcome_t outcome = this->await(portmap, awh::unit::portmap_t::action_t::OPEN);
	// Выполняем остановку поддельного маршрутизатора
	router.stop();
	// Выполняем проверку того, что чужой ответ за свой не принят
	ASSERT_FALSE(outcome.answered);
	// Выполняем проверку того, что обращение завершилось отказом
	ASSERT_TRUE(outcome.failed);
	// Выполняем проверку кода причины отказа перенаправления
	ASSERT_EQ(outcome.error, awh::unit::portmap_t::error_t::NO_RESPONSE);
}
