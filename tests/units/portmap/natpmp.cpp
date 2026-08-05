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
#include <sys/time.h>
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
	public:
		/**
		 * @brief Порча, вносимая поддельным маршрутизатором в ответ
		 *
		 * @details Ответ, построенный ошибочно, приходит из сети наравне с правильным, и
		 *          отличать одно от другого обязан сам модуль: гнездо обмена договором
		 *          подключено к маршрутизатору, и негодный ответ приходит именно от него,
		 *          а не от постороннего
		 *
		 */
		enum class Damage : uint8_t {
			NONE,      // Ответ выдаётся правильным
			TRUNCATE,  // Ответ обрывается на середине заголовка
			VERSION,   // Издание договора называется неизвестным
			REQUEST,   // Отметка ответа снимается, и ответ выглядит просьбой
			OPCODE,    // Действие в ответе называется чужим
			SILENT     // Ответ не выдаётся вовсе
		};
	private:
		// Дескриптор гнезда поддельного маршрутизатора
		int _socket;
		// Поток обмена поддельного маршрутизатора
		std::thread _thread;
		// Признак продолжения работы поддельного маршрутизатора
		std::atomic <bool> _working;
	public:
		// Код итога, выдаваемый поддельным маршрутизатором
		std::atomic <uint16_t> result{0};
		// Порча, вносимая поддельным маршрутизатором в ответ
		std::atomic <Damage> damage{Damage::NONE};
		// Срок жизни перенаправления, называемый поддельным маршрутизатором
		std::atomic <uint32_t> lifeTime{60};
		// Количество просьб, полученных поддельным маршрутизатором
		std::atomic <size_t> calls{0};
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
					/**
					 * Если просьба не получена либо короче заголовка договора, ожидаем следующую
					 *
					 * @note Просьба о внешнем адресе занимает два байта, а о перенаправлении -
					 *       двенадцать: короче двух не бывает ни одна
					 */
					if(size < 2) continue;
					// Запоминаем полученную просьбу
					this->calls.fetch_add(1);
					// Получаем порчу, вносимую в ответ
					const Damage damage = this->damage.load();
					// Если ответ не выдаётся вовсе, ожидаем следующую просьбу
					if(damage == Damage::SILENT) continue;
					/**
					 * Признак того, что просьба обращена за внешним адресом маршрутизатора
					 *
					 * @note Действие это договор обозначает нулём, а перенаправление - единицей
					 *       у UDP и двойкой у TCP
					 */
					const bool address = (buffer[1] == 0);
					// Собираемый ответ маршрутизатора
					uint8_t answer[16] = {0};
					// Записываем издание договора
					answer[0] = static_cast <uint8_t> ((damage == Damage::VERSION) ? 0xFF : 0);
					// Получаем действие, называемое в ответе
					const uint8_t opcode = static_cast <uint8_t> ((damage == Damage::OPCODE) ? (buffer[1] + 1) : buffer[1]);
					// Записываем действие договора с отметкой ответа
					answer[1] = static_cast <uint8_t> ((damage == Damage::REQUEST) ? opcode : (opcode | RESPONSE_FLAG));
					// Получаем код итога, выдаваемый маршрутизатором
					const uint16_t code = this->result.load();
					// Записываем код итога, выдаваемый маршрутизатором
					answer[2] = static_cast <uint8_t> (code >> 8);
					// Записываем код итога, выдаваемый маршрутизатором
					answer[3] = static_cast <uint8_t> (code & 0xFF);
					// Время работы маршрутизатора в секундах
					answer[7] = 1;
					/**
					 * Если просьба обращена за внешним адресом маршрутизатора
					 *
					 * @note Ответ на неё занимает двенадцать байтов, а внешний адрес лежит
					 *       в последних четырёх
					 */
					if(address){
						// Записываем внешний адрес маршрутизатора
						answer[8] = 203; answer[9] = 0; answer[10] = 113; answer[11] = 7;
						// Выполняем отправку ответа обратившейся машине
						::sendto(this->_socket, answer, ((damage == Damage::TRUNCATE) ? 5 : 12), 0, reinterpret_cast <struct sockaddr *> (&client), length);
						// Ожидаем следующую просьбу
						continue;
					}
					// Записываем внутренний порт, называемый испытанием
					answer[8] = static_cast <uint8_t> (internalPort >> 8);
					// Записываем внутренний порт, называемый испытанием
					answer[9] = static_cast <uint8_t> (internalPort & 0xFF);
					// Записываем назначенный внешний порт
					answer[10] = static_cast <uint8_t> (INTERNAL_PORT >> 8);
					// Записываем назначенный внешний порт
					answer[11] = static_cast <uint8_t> (INTERNAL_PORT & 0xFF);
					// Получаем назначенный срок жизни перенаправления
					const uint32_t life = this->lifeTime.load();
					// Записываем назначенный срок жизни перенаправления
					answer[12] = static_cast <uint8_t> (life >> 24);
					// Записываем назначенный срок жизни перенаправления
					answer[13] = static_cast <uint8_t> ((life >> 16) & 0xFF);
					// Записываем назначенный срок жизни перенаправления
					answer[14] = static_cast <uint8_t> ((life >> 8) & 0xFF);
					// Записываем назначенный срок жизни перенаправления
					answer[15] = static_cast <uint8_t> (life & 0xFF);
					// Выполняем отправку ответа обратившейся машине
					::sendto(this->_socket, answer, ((damage == Damage::TRUNCATE) ? 5 : sizeof(answer)), 0, reinterpret_cast <struct sockaddr *> (&client), length);
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
			 * Устанавливаем срок ожидания приёма просьбы
			 *
			 * @note Срок обязателен: без него поток обмена спит в приёме до первой
			 *       просьбы, и закрытие гнезда из другого потока его не будит - остановка
			 *       поддельного маршрутизатора повисла бы навсегда
			 */
			{
				// Срок ожидания приёма просьбы
				struct timeval timeout;
				// Устанавливаем срок ожидания в секундах
				timeout.tv_sec = 0;
				// Устанавливаем срок ожидания в микросекундах
				timeout.tv_usec = 100000;
				// Выполняем установку срока ожидания приёма просьбы
				::setsockopt(this->_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
			}
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
 * @note Проверкой закрепляется и продолжение прерванного ожидания: движок снимает срок
 *       приходом данных, не разбирая, ответ это или чужая дейтаграмма, и отброшенный
 *       ответ оставил бы обмен без срока вовсе - ни ответа, ни отказа, ни повтора.
 *       Модуль продолжает ожидание остатком, и отказ приходит в свой черёд
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

/**
 * @brief Метод настройки модуля перенаправления портов на поддельный маршрутизатор
 *
 * @param portmap объект модуля перенаправления портов
 *
 */
static void setup(awh::unit::portmap_t & portmap) noexcept {
	// Устанавливаем вид опроса маршрутизатора
	portmap.setType(awh::unit::portmap_t::type_t::NAT_PMP);
	// Устанавливаем адрес поддельного маршрутизатора
	portmap.setRouter("127.0.0.1");
	// Устанавливаем срок ожидания ответа маршрутизатора
	portmap.setTimeout(300);
	// Устанавливаем количество попыток обращения к маршрутизатору
	portmap.setAttempts(2);
}

/**
 * @brief Проверка приведения кодов отказа маршрутизатора
 *
 * @details Договор NAT-PMP несёт шесть кодов итога, и всякий из них модуль обязан
 *          привести к своему коду причины отказа. Приведение это прежде не проверялось
 *          вовсе: поддельный маршрутизатор отвечал только удачей
 *
 */
TEST_F(PortmapUnitFixture, PortmapNatPmpRefusal) {
	/**
	 * @brief Соответствие кода итога договора коду причины отказа модуля
	 *
	 */
	struct Pair {
		// Код итога, выдаваемый маршрутизатором
		uint16_t result;
		// Ожидаемый код причины отказа перенаправления
		awh::unit::portmap_t::error_t error;
	};
	// Перечень сличаемых кодов итога
	const Pair pairs[] = {
		{1, awh::unit::portmap_t::error_t::NOT_SUPPORTED},
		{2, awh::unit::portmap_t::error_t::NOT_AUTHORIZED},
		{3, awh::unit::portmap_t::error_t::NETWORK_FAILURE},
		{4, awh::unit::portmap_t::error_t::OUT_OF_RESOURCES},
		{5, awh::unit::portmap_t::error_t::NOT_SUPPORTED},
		{9, awh::unit::portmap_t::error_t::REFUSED}
	};
	/**
	 * Выполняем перебор всех сличаемых кодов итога
	 */
	for(const Pair & pair : pairs){
		// Создаём поддельный маршрутизатор договора NAT-PMP
		Router router;
		// Если порт договора занят, испытание не проводится
		if(!router.ready()) GTEST_SKIP() << "NAT-PMP port is occupied";
		// Устанавливаем код итога, выдаваемый поддельным маршрутизатором
		router.result.store(pair.result);
		// Выполняем запуск поддельного маршрутизатора с тем же внутренним портом
		router.start(INTERNAL_PORT);
		// Создаём объект модуля перенаправления портов
		awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
		// Выполняем настройку модуля на поддельный маршрутизатор
		setup(portmap);
		// Выполняем ожидание итога обращения к маршрутизатору
		const outcome_t outcome = this->await(portmap, awh::unit::portmap_t::action_t::OPEN);
		// Выполняем остановку поддельного маршрутизатора
		router.stop();
		// Выполняем проверку того, что обращение завершилось отказом
		ASSERT_TRUE(outcome.failed) << "код итога " << pair.result;
		// Выполняем проверку кода причины отказа перенаправления
		ASSERT_EQ(outcome.error, pair.error) << "код итога " << pair.result;
		// Выполняем проверку того, что повторов при осмысленном отказе не было
		ASSERT_EQ(router.calls.load(), static_cast <size_t> (1)) << "код итога " << pair.result;
	}
}

/**
 * @brief Проверка разбора негодного ответа маршрутизатора
 *
 * @details Гнездо обмена договором подключено к маршрутизатору, и негодный ответ приходит
 *          именно от него, а не от постороннего: ответ, разобрать который не удалось,
 *          означает неисправность маршрутизатора, и объявляется он неразобранным сразу,
 *          без повтора просьбы. Ответ же, разобранный успешно, но нашей просьбе не
 *          отвечающий, - дело иное: он попросту не наш, отбрасывается молча, а ожидание
 *          продолжается остатком срока
 *
 * @note Приведение это в точности то же, что у договора PCP: там оно закреплено
 *       проверкой `PortmapPcpDamagedAnswer`, и расхождение между договорами означало бы,
 *       что один из них разобран неверно
 *
 */
TEST_F(PortmapUnitFixture, PortmapNatPmpDamagedAnswer) {
	/**
	 * @brief Соответствие вносимой порчи ожидаемому итогу обращения
	 *
	 */
	struct Pair {
		// Порча, вносимая поддельным маршрутизатором в ответ
		Router::Damage damage;
		// Ожидаемый код причины отказа перенаправления
		awh::unit::portmap_t::error_t error;
		// Ожидаемое количество просьб, полученных маршрутизатором
		size_t calls;
	};
	// Перечень сличаемой порчи
	const Pair pairs[] = {
		// Ответ короче заголовка договора разобрать нечем
		{Router::Damage::TRUNCATE, awh::unit::portmap_t::error_t::MALFORMED, 1},
		// Издание договора в ответе неизвестно
		{Router::Damage::VERSION, awh::unit::portmap_t::error_t::MALFORMED, 1},
		// Ответ без отметки ответом не является
		{Router::Damage::REQUEST, awh::unit::portmap_t::error_t::MALFORMED, 1},
		// Действие в ответе названо чужим
		{Router::Damage::OPCODE, awh::unit::portmap_t::error_t::MALFORMED, 1},
		// Ответа нет вовсе, и просьба повторяется отведённое число раз
		{Router::Damage::SILENT, awh::unit::portmap_t::error_t::NO_RESPONSE, 2}
	};
	/**
	 * Выполняем перебор всей сличаемой порчи
	 */
	for(const Pair & pair : pairs){
		// Получаем порчу, вносимую поддельным маршрутизатором в ответ
		const Router::Damage damage = pair.damage;
		// Создаём поддельный маршрутизатор договора NAT-PMP
		Router router;
		// Если порт договора занят, испытание не проводится
		if(!router.ready()) GTEST_SKIP() << "NAT-PMP port is occupied";
		// Устанавливаем порчу, вносимую поддельным маршрутизатором в ответ
		router.damage.store(damage);
		// Выполняем запуск поддельного маршрутизатора с тем же внутренним портом
		router.start(INTERNAL_PORT);
		// Создаём объект модуля перенаправления портов
		awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
		// Выполняем настройку модуля на поддельный маршрутизатор
		setup(portmap);
		// Выполняем ожидание итога обращения к маршрутизатору
		const outcome_t outcome = this->await(portmap, awh::unit::portmap_t::action_t::OPEN);
		// Выполняем остановку поддельного маршрутизатора
		router.stop();
		// Выполняем проверку того, что негодный ответ за свой не принят
		ASSERT_FALSE(outcome.answered) << "порча " << static_cast <int> (damage);
		// Выполняем проверку того, что обращение завершилось отказом
		ASSERT_TRUE(outcome.failed) << "порча " << static_cast <int> (damage);
		// Выполняем проверку кода причины отказа перенаправления
		ASSERT_EQ(outcome.error, pair.error) << "порча " << static_cast <int> (damage);
		// Выполняем проверку количества просьб, полученных маршрутизатором
		ASSERT_EQ(router.calls.load(), pair.calls) << "порча " << static_cast <int> (damage);
	}
}

/**
 * @brief Проверка получения внешнего адреса маршрутизатора
 *
 * @details Внешний адрес договор выдаёт отдельным действием, и разбор его прежде не
 *          проверялся вовсе: адрес лежит в ответе четырьмя байтами в порядке сети
 *
 */
TEST_F(PortmapUnitFixture, PortmapNatPmpExternal) {
	// Создаём поддельный маршрутизатор договора NAT-PMP
	Router router;
	// Если порт договора занят, испытание не проводится
	if(!router.ready()) GTEST_SKIP() << "NAT-PMP port is occupied";
	// Выполняем запуск поддельного маршрутизатора с тем же внутренним портом
	router.start(INTERNAL_PORT);
	// Создаём объект модуля перенаправления портов
	awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
	// Выполняем настройку модуля на поддельный маршрутизатор
	setup(portmap);
	// Выполняем ожидание итога обращения к маршрутизатору
	const outcome_t outcome = this->await(portmap, awh::unit::portmap_t::action_t::EXTERNAL);
	// Выполняем остановку поддельного маршрутизатора
	router.stop();
	// Выполняем проверку того, что обращение отказом не завершилось
	ASSERT_FALSE(outcome.failed);
	// Выполняем проверку того, что маршрутизатор ответил
	ASSERT_TRUE(outcome.answered);
	// Выполняем проверку договора, по которому получен итог
	ASSERT_EQ(outcome.type, awh::unit::portmap_t::type_t::NAT_PMP);
}

/**
 * @brief Проверка снятия и продления перенаправления
 *
 * @details Обе просьбы договор выражает тем же действием, что и заведение: снятие -
 *          нулевым сроком жизни, продление - обычным. Различает их лишь сам модуль
 *
 */
TEST_F(PortmapUnitFixture, PortmapNatPmpCloseAndRenew) {
	// Перечень проверяемых просьб
	const awh::unit::portmap_t::action_t actions[] = {
		awh::unit::portmap_t::action_t::CLOSE,
		awh::unit::portmap_t::action_t::RENEW
	};
	/**
	 * Выполняем перебор всех проверяемых просьб
	 */
	for(const awh::unit::portmap_t::action_t action : actions){
		// Создаём поддельный маршрутизатор договора NAT-PMP
		Router router;
		// Если порт договора занят, испытание не проводится
		if(!router.ready()) GTEST_SKIP() << "NAT-PMP port is occupied";
		/**
		 * Устанавливаем нулевой срок жизни при снятии перенаправления
		 *
		 * @note Маршрутизатор отвечает на снятие тем же нулём, каким его и просили
		 */
		if(action == awh::unit::portmap_t::action_t::CLOSE) router.lifeTime.store(0);
		// Выполняем запуск поддельного маршрутизатора с тем же внутренним портом
		router.start(INTERNAL_PORT);
		// Создаём объект модуля перенаправления портов
		awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
		// Выполняем настройку модуля на поддельный маршрутизатор
		setup(portmap);
		// Выполняем ожидание итога обращения к маршрутизатору
		const outcome_t outcome = this->await(portmap, action);
		// Выполняем остановку поддельного маршрутизатора
		router.stop();
		// Выполняем проверку того, что обращение отказом не завершилось
		ASSERT_FALSE(outcome.failed) << "просьба " << static_cast <int> (action);
		// Выполняем проверку того, что маршрутизатор ответил
		ASSERT_TRUE(outcome.answered) << "просьба " << static_cast <int> (action);
	}
}
