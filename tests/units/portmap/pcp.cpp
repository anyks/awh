/**
 * @file pcp.cpp
 * @date 2026-08-05
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
 * @brief Проверка обмена по договору PCP на поддельном маршрутизаторе —
 *        принятие ответа, приведение кодов отказа и отбрасывание негодных ответов
 *
 * @copyright Copyright © 2026
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
#include <chrono>
#include <cstring>

/**
 * Системные заголовочные файлы
 */
/**
 * Для операционной системы MS Windows
 *
 * @note Заголовки эти принадлежат POSIX и у MS Windows отсутствуют: отвечающие им
 *       объявления приходят там из winsock2.h, подключаемого через единую точку
 *       sys/win32.hpp, а недостающее восполняет tests/posix.hpp
 *
 */
#if _WIN32 || _WIN64
	#include <sys/win32.hpp>
/**
 * Для операционных систем Linux, FreeBSD, NetBSD, OpenBSD, macOS и Solaris
 */
#else
	#include <unistd.h>
	#include <sys/time.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

/**
 * Подключаем восполнение средств POSIX, отсутствующих у MS Windows
 */
#include "../../posix.hpp"


/**
 * @brief Порт договора PCP
 *
 */
static constexpr uint16_t PCP_PORT = 5351;

/**
 * @brief Внутренний порт заводимого перенаправления
 *
 */
static constexpr uint16_t PCP_INTERNAL_PORT = 8081;

/**
 * @brief Внешний порт, назначаемый поддельным маршрутизатором
 *
 */
static constexpr uint16_t PCP_EXTERNAL_PORT = 41234;

/**
 * @brief Издание договора PCP
 *
 */
static constexpr uint8_t PCP_VERSION = 0x02;

/**
 * @brief Отметка ответа в коде действия договора
 *
 */
static constexpr uint8_t PCP_RESPONSE_FLAG = 0x80;

/**
 * @brief Размер заголовка сообщения договора
 *
 */
static constexpr size_t PCP_HEADER_SIZE = 0x18;

/**
 * @brief Размер полезной части действия перенаправления
 *
 */
static constexpr size_t PCP_MAP_PAYLOAD_SIZE = 0x24;

/**
 * @brief Класс поддельного маршрутизатора договора PCP
 *
 * @details Отвечает на просьбу так, как отвечал бы маршрутизатор, но код итога и вид
 *          порчи ответа задаются испытанием. Настоящего маршрутизатора для этого не
 *          нужно, и ответ приходит одинаково на любой машине
 *
 * @note Отличительная метка перенаправления переписывается из просьбы в ответ: договор
 *       велит сличать её, и ответ с чужой меткой модуль отбросит - именно этим и
 *       проверяется само сличение
 *
 */
class PcpRouter {
	public:
		/**
		 * @brief Вид порчи ответа маршрутизатора
		 *
		 */
		enum class Damage : uint8_t {
			NONE     = 0x00, // Ответ выдаётся целым
			TRUNCATE = 0x01, // Ответ обрывается на половине заголовка
			VERSION  = 0x02, // В ответе называется неизвестное издание договора
			REQUEST  = 0x03, // Ответ выдаётся без отметки ответа
			NONCE    = 0x04, // В ответе называется чужая отличительная метка
			SILENT   = 0x05  // Ответ не выдаётся вовсе
		};
	private:
		// Дескриптор гнезда поддельного маршрутизатора
		int _socket;
		// Поток обмена поддельного маршрутизатора
		std::thread _thread;
		// Признак продолжения работы поддельного маршрутизатора
		std::atomic <bool> _working;
		// Количество принятых просьб
		std::atomic <int32_t> _requests;
	public:
		// Код итога, выдаваемый в ответе
		std::atomic <uint8_t> result{0};
		// Вид порчи выдаваемого ответа
		std::atomic <Damage> damage{Damage::NONE};
		/**
		 * Отсчёт времени работы, называемый поддельным маршрутизатором
		 *
		 * @note Отсчёт этот испытание задаёт само: откат его означает, что маршрутизатор
		 *       перезагрузился и заведённые перенаправления утратил, и подделать такое
		 *       событие иначе, чем назначив отсчёт, нечем
		 */
		std::atomic <uint32_t> epoch{1000};
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
		 * @brief Метод получения количества принятых просьб
		 *
		 * @return количество принятых просьб
		 *
		 */
		int32_t requests() const noexcept {
			// Выводим количество принятых просьб
			return this->_requests.load();
		}
	public:
		/**
		 * @brief Метод запуска поддельного маршрутизатора
		 *
		 */
		void start() noexcept {
			// Запоминаем, что работа поддельного маршрутизатора начата
			this->_working.store(true);
			// Выполняем запуск потока обмена поддельного маршрутизатора
			this->_thread = std::thread([this]() noexcept -> void {
				// Место под полученную просьбу
				uint8_t buffer[256] = {0};
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
					const ssize_t size = ::recvfrom(this->_socket, reinterpret_cast <char *> (buffer), sizeof(buffer), 0, reinterpret_cast <struct sockaddr *> (&client), &length);
					// Если просьба короче заголовка с полезной частью, ожидаем следующую
					if(size < static_cast <ssize_t> (PCP_HEADER_SIZE + PCP_MAP_PAYLOAD_SIZE)) continue;
					// Запоминаем принятую просьбу
					this->_requests.fetch_add(1);
					// Получаем вид порчи выдаваемого ответа
					const Damage damage = this->damage.load();
					// Если ответ не выдаётся вовсе, ожидаем следующую просьбу
					if(damage == Damage::SILENT) continue;
					// Собираемый ответ маршрутизатора
					uint8_t answer[PCP_HEADER_SIZE + PCP_MAP_PAYLOAD_SIZE] = {0};
					// Записываем издание договора
					answer[0] = ((damage == Damage::VERSION) ? static_cast <uint8_t> (PCP_VERSION + 1) : PCP_VERSION);
					// Записываем действие договора с отметкой ответа
					answer[1] = static_cast <uint8_t> ((buffer[1] & 0x7F) | ((damage == Damage::REQUEST) ? 0 : PCP_RESPONSE_FLAG));
					// Записываем код итога, выданный маршрутизатором
					answer[3] = this->result.load();
					// Записываем назначенный срок жизни перенаправления
					answer[7] = 60;
					// Получаем отсчёт времени работы маршрутизатора
					const uint32_t epoch = this->epoch.load();
					// Записываем отсчёт времени работы маршрутизатора
					answer[8] = static_cast <uint8_t> (epoch >> 24);
					// Записываем отсчёт времени работы маршрутизатора
					answer[9] = static_cast <uint8_t> ((epoch >> 16) & 0xFF);
					// Записываем отсчёт времени работы маршрутизатора
					answer[10] = static_cast <uint8_t> ((epoch >> 8) & 0xFF);
					// Записываем отсчёт времени работы маршрутизатора
					answer[11] = static_cast <uint8_t> (epoch & 0xFF);
					// Выполняем перенос отличительной метки перенаправления из просьбы в ответ
					::memcpy(&answer[PCP_HEADER_SIZE], &buffer[PCP_HEADER_SIZE], 12);
					// Если ответ выдаётся с чужой отличительной меткой, портим её первый байт
					if(damage == Damage::NONCE) answer[PCP_HEADER_SIZE] = static_cast <uint8_t> (answer[PCP_HEADER_SIZE] + 1);
					// Записываем договор перенаправляемого порта
					answer[PCP_HEADER_SIZE + 12] = buffer[PCP_HEADER_SIZE + 12];
					// Записываем внутренний порт перенаправления
					answer[PCP_HEADER_SIZE + 16] = buffer[PCP_HEADER_SIZE + 16];
					// Записываем внутренний порт перенаправления
					answer[PCP_HEADER_SIZE + 17] = buffer[PCP_HEADER_SIZE + 17];
					// Записываем назначенный внешний порт перенаправления
					answer[PCP_HEADER_SIZE + 18] = static_cast <uint8_t> (PCP_EXTERNAL_PORT >> 8);
					// Записываем назначенный внешний порт перенаправления
					answer[PCP_HEADER_SIZE + 19] = static_cast <uint8_t> (PCP_EXTERNAL_PORT & 0xFF);
					/**
					 * Записываем назначенный внешний адрес записью IPv4, вложенной в IPv6
					 *
					 * @note Договор велит называть адрес записью IPv6 всегда, а адрес сети
					 *       IPv4 вкладывать в неё отведённым договором образом
					 */
					answer[PCP_HEADER_SIZE + 30] = 0xFF;
					// Записываем отметку вложения записи IPv4
					answer[PCP_HEADER_SIZE + 31] = 0xFF;
					// Записываем первый октет назначенного внешнего адреса
					answer[PCP_HEADER_SIZE + 32] = 203;
					// Записываем второй октет назначенного внешнего адреса
					answer[PCP_HEADER_SIZE + 33] = 0;
					// Записываем третий октет назначенного внешнего адреса
					answer[PCP_HEADER_SIZE + 34] = 113;
					// Записываем четвёртый октет назначенного внешнего адреса
					answer[PCP_HEADER_SIZE + 35] = 7;
					// Получаем размер выдаваемого ответа
					const size_t length2 = ((damage == Damage::TRUNCATE) ? (PCP_HEADER_SIZE / 2) : sizeof(answer));
					// Выполняем отправку ответа обратившейся машине
					::sendto(this->_socket, reinterpret_cast <const char *> (answer), length2, 0, reinterpret_cast <struct sockaddr *> (&client), length);
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
				::closesocket(this->_socket);
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
		PcpRouter() noexcept : _socket(-1), _working(false), _requests(0) {
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
			address.sin_port = htons(PCP_PORT);
			/**
			 * Слушаем любой адрес машины, а не одну лишь петлю
			 *
			 * @note Модуль заводит на каждую попытку новое гнездо, и повторная просьба
			 *       уходит уже с иного местного адреса. Гнездо, привязанное к одной
			 *       петле, такую просьбу не принимает вовсе - захват на стенде Windows
			 *       ARM64 показал её отброшенной с «transport endpoint was not found»,
			 *       и проверка насчитывала одну просьбу вместо двух. Наружу это гнездо
			 *       ничего не открывает: обмен ведут проверки этой же машины
			 */
			address.sin_addr.s_addr = htonl(INADDR_ANY);
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
				::setReceiveTimeout(this->_socket, 100);
				// Отключаем отчёт о недоступности получателя, теряющий дейтаграммы
				::disableConnectionReset(this->_socket);
			}
			/**
			 * Если привязать гнездо к порту договора не удалось
			 *
			 * @note Порт бывает занят настоящей службой перенаправления, и тогда
			 *       испытание не проводится, а не объявляется неудачным
			 */
			if(::bind(this->_socket, reinterpret_cast <struct sockaddr *> (&address), sizeof(address)) != 0){
				// Выполняем закрытие гнезда поддельного маршрутизатора
				::closesocket(this->_socket);
				// Сбрасываем дескриптор гнезда поддельного маршрутизатора
				this->_socket = -1;
			}
		}
		/**
		 * @brief Деструктор
		 *
		 */
		~PcpRouter() noexcept {
			// Выполняем остановку поддельного маршрутизатора
			this->stop();
		}
};

/**
 * @brief Метод настройки модуля на поддельный маршрутизатор договора PCP
 *
 * @param portmap объект модуля перенаправления портов
 *
 */
static void setup(awh::unit::portmap_t & portmap) noexcept {
	// Устанавливаем вид опроса маршрутизатора
	portmap.setType(awh::unit::portmap_t::type_t::PCP);
	// Устанавливаем адрес поддельного маршрутизатора
	portmap.setRouter("127.0.0.1");
	// Устанавливаем срок ожидания ответа маршрутизатора
	portmap.setTimeout(300);
	// Устанавливаем количество попыток обращения к маршрутизатору
	portmap.setAttempts(2);
}

/**
 * @brief Проверка принятия ответа маршрутизатора договора PCP
 *
 * @details Поддельный маршрутизатор отвечает согласием и называет назначенный внешний
 *          порт. Модуль обязан такой ответ принять и выдать вызывающему назначенное
 *          маршрутизатором, а не запрошенное им самим
 *
 */
TEST_F(PortmapUnitFixture, PortmapPcpMapping) {
	// Создаём поддельный маршрутизатор договора PCP
	PcpRouter router;
	// Если порт договора занят, испытание не проводится
	if(!router.ready()) GTEST_SKIP() << "PCP port is occupied";
	// Выполняем запуск поддельного маршрутизатора
	router.start();
	// Создаём объект модуля перенаправления портов
	awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
	// Выполняем настройку модуля на поддельный маршрутизатор
	::setup(portmap);
	// Выполняем ожидание итога обращения к маршрутизатору
	const outcome_t outcome = this->await(portmap, awh::unit::portmap_t::action_t::OPEN);
	// Выполняем остановку поддельного маршрутизатора
	router.stop();
	// Выполняем проверку того, что маршрутизатор ответил
	ASSERT_TRUE(outcome.answered) << "код отказа " << static_cast <int32_t> (outcome.error);
	// Выполняем проверку того, что обращение отказом не завершилось
	ASSERT_FALSE(outcome.failed);
	// Выполняем проверку договора, по которому получен итог
	ASSERT_EQ(outcome.type, awh::unit::portmap_t::type_t::PCP);
	// Выполняем проверку того, что выдан назначенный маршрутизатором внешний порт
	ASSERT_EQ(outcome.mapping.externalPort, PCP_EXTERNAL_PORT);
	// Выполняем проверку того, что внутренний порт остался запрошенным
	ASSERT_EQ(outcome.mapping.internalPort, PCP_INTERNAL_PORT);
	// Выполняем проверку того, что выдан назначенный маршрутизатором срок жизни
	ASSERT_EQ(outcome.mapping.lifeTime, 60u);
	// Выполняем проверку того, что просьба до маршрутизатора дошла один раз
	ASSERT_EQ(router.requests(), 1);
}

/**
 * @brief Проверка приведения кодов отказа договора PCP
 *
 * @details Маршрутизатор отвечает отказом, и код его договора приводится к коду причины
 *          отказа перенаправления. Приведение это не исполнялось ни одним испытанием:
 *          отказ настоящего маршрутизатора по заказу не получить
 *
 * @note Проверяется и код, договору неизвестный: договор оставляет место под новые коды,
 *       и приниматься за успех они не должны
 *
 */
TEST_F(PortmapUnitFixture, PortmapPcpRefusal) {
	/**
	 * @brief Структура сличаемой пары кодов
	 *
	 */
	struct pair_t {
		// Код итога, выданный маршрутизатором
		uint8_t result;
		// Ожидаемый код причины отказа перенаправления
		awh::unit::portmap_t::error_t error;
	};
	/**
	 * Выполняем перебор кодов итога, выдаваемых маршрутизатором
	 */
	for(const pair_t & pair : {
		pair_t{0x01, awh::unit::portmap_t::error_t::NOT_SUPPORTED},    // Издание договора не поддерживается
		pair_t{0x02, awh::unit::portmap_t::error_t::NOT_AUTHORIZED},   // Просьба отвергнута настройкой
		pair_t{0x03, awh::unit::portmap_t::error_t::MALFORMED},        // Запрос построен ошибочно
		pair_t{0x04, awh::unit::portmap_t::error_t::NOT_SUPPORTED},    // Действие не поддерживается
		pair_t{0x05, awh::unit::portmap_t::error_t::NOT_SUPPORTED},    // Дополнение запроса не поддерживается
		pair_t{0x06, awh::unit::portmap_t::error_t::MALFORMED},        // Дополнение построено ошибочно
		pair_t{0x07, awh::unit::portmap_t::error_t::NETWORK_FAILURE},  // Нет связи с внешней сетью
		pair_t{0x08, awh::unit::portmap_t::error_t::OUT_OF_RESOURCES}, // Не осталось места под перенаправления
		pair_t{0x09, awh::unit::portmap_t::error_t::NOT_SUPPORTED},    // Договор порта не поддерживается
		pair_t{0x0A, awh::unit::portmap_t::error_t::OUT_OF_RESOURCES}, // Исчерпана отведённая доля
		pair_t{0x0B, awh::unit::portmap_t::error_t::REFUSED},          // Внешний адрес выдать невозможно
		pair_t{0x0C, awh::unit::portmap_t::error_t::REFUSED},          // Адрес в запросе не совпал
		pair_t{0x7F, awh::unit::portmap_t::error_t::REFUSED}           // Код итога договору неизвестен
	}){
		// Создаём поддельный маршрутизатор договора PCP
		PcpRouter router;
		// Если порт договора занят, испытание не проводится
		if(!router.ready()) GTEST_SKIP() << "PCP port is occupied";
		// Устанавливаем код итога, выдаваемый маршрутизатором
		router.result.store(pair.result);
		// Выполняем запуск поддельного маршрутизатора
		router.start();
		// Создаём объект модуля перенаправления портов
		awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
		// Выполняем настройку модуля на поддельный маршрутизатор
		::setup(portmap);
		// Выполняем ожидание итога обращения к маршрутизатору
		const outcome_t outcome = this->await(portmap, awh::unit::portmap_t::action_t::OPEN);
		// Выполняем остановку поддельного маршрутизатора
		router.stop();
		// Выполняем проверку того, что обращение завершилось отказом
		ASSERT_TRUE(outcome.failed) << "код итога " << static_cast <int32_t> (pair.result);
		// Выполняем проверку приведённого кода причины отказа перенаправления
		ASSERT_EQ(outcome.error, pair.error) << "код итога " << static_cast <int32_t> (pair.result);
		// Выполняем проверку того, что просьба до маршрутизатора дошла
		ASSERT_GT(router.requests(), 0);
	}
}

/**
 * @brief Проверка обхождения с негодными ответами договора PCP
 *
 * @details Разводятся два повода. Ответ, который кодеку разобрать не удалось, приходит
 *          с гнезда, подключённого к маршрутизатору: иного отправителя ядро на него не
 *          пропустит, и негодным такой ответ делает сам маршрутизатор - обмен обрывается
 *          отказом. Ответ же, разобранный, но принадлежащий чужой просьбе, приходит от
 *          маршрутизатора наравне с ответами прочим машинам, и отказом он не считается:
 *          ожидание продолжается остатком, просьба повторяется, а отказ наступает в свой
 *          черёд по неответу
 *
 * @note Разводом этим закрепляется и продолжение прерванного ожидания: движок снимает
 *       срок приходом данных, не разбирая, ответ это или чужая дейтаграмма, и отброшенный
 *       ответ оставил бы обмен без срока вовсе - ни ответа, ни отказа, ни повтора
 *
 */
TEST_F(PortmapUnitFixture, PortmapPcpDamagedAnswer) {
	/**
	 * @brief Структура сличаемого обхождения с негодным ответом
	 *
	 */
	struct pair_t {
		// Вид порчи выдаваемого ответа
		PcpRouter::Damage damage;
		// Ожидаемый код причины отказа перенаправления
		awh::unit::portmap_t::error_t error;
		// Ожидаемое наименьшее количество принятых просьб
		int32_t requests;
	};
	/**
	 * Выполняем перебор видов порчи ответа маршрутизатора
	 */
	for(const pair_t & pair : {
		// Ответ оборван на половине заголовка: разобрать его не удалось
		pair_t{PcpRouter::Damage::TRUNCATE, awh::unit::portmap_t::error_t::MALFORMED, 1},
		// В ответе названо неизвестное издание договора: разобрать его не удалось
		pair_t{PcpRouter::Damage::VERSION, awh::unit::portmap_t::error_t::MALFORMED, 1},
		// Ответ выдан без отметки ответа: разобрать его не удалось
		pair_t{PcpRouter::Damage::REQUEST, awh::unit::portmap_t::error_t::MALFORMED, 1},
		// В ответе названа чужая отличительная метка: ответ разобран, но не наш
		pair_t{PcpRouter::Damage::NONCE, awh::unit::portmap_t::error_t::NO_RESPONSE, 2},
		// Ответа нет вовсе: обмен завершается по истечении срока
		pair_t{PcpRouter::Damage::SILENT, awh::unit::portmap_t::error_t::NO_RESPONSE, 2}
	}){
		// Создаём поддельный маршрутизатор договора PCP
		PcpRouter router;
		// Если порт договора занят, испытание не проводится
		if(!router.ready()) GTEST_SKIP() << "PCP port is occupied";
		// Устанавливаем вид порчи выдаваемого ответа
		router.damage.store(pair.damage);
		// Выполняем запуск поддельного маршрутизатора
		router.start();
		// Создаём объект модуля перенаправления портов
		awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
		// Выполняем настройку модуля на поддельный маршрутизатор
		::setup(portmap);
		// Выполняем ожидание итога обращения к маршрутизатору
		const outcome_t outcome = this->await(portmap, awh::unit::portmap_t::action_t::OPEN);
		/**
		 * Дожидаемся, пока поддельный маршрутизатор разберёт принятое
		 *
		 * @details Отказ модуль объявляет по своему сроку, а повторная просьба уходит
		 *          прежде того - но принять её маршрутизатор успевает не всегда: поток
		 *          обмена спит в приёме до ста миллисекунд, а остановка закрывает гнездо
		 *          вместе со всем, что в нём лежит непрочитанным. Проверка насчитывала
		 *          тогда одну просьбу вместо двух - примерно один прогон из пяти на
		 *          стенде Windows ARM64. Ожидание здесь ограничено сроком, и просьбу
		 *          неушедшую оно не выдумывает: не дождавшись, проверка отказывает
		 *          так же, как и прежде
		 */
		{
			// Время начала ожидания разбора принятого
			const auto start = std::chrono::steady_clock::now();
			/**
			 * Выполняем ожидание, пока просьбы не будут разобраны
			 */
			while((router.requests() < pair.requests) && (std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 300))
				// Выполняем передышку между проверками количества принятых просьб
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		// Выполняем остановку поддельного маршрутизатора
		router.stop();
		// Выполняем проверку того, что негодный ответ за свой не принят
		ASSERT_FALSE(outcome.answered) << "вид порчи " << static_cast <int32_t> (pair.damage);
		// Выполняем проверку того, что обращение завершилось отказом
		ASSERT_TRUE(outcome.failed) << "вид порчи " << static_cast <int32_t> (pair.damage);
		// Выполняем проверку кода причины отказа перенаправления
		ASSERT_EQ(outcome.error, pair.error) << "вид порчи " << static_cast <int32_t> (pair.damage);
		/**
		 * Выполняем проверку количества просьб, дошедших до маршрутизатора
		 *
		 * @note Ответ, разобрать который не удалось, обмен обрывает сразу, и просьба
		 *       повторяться не должна вовсе: повтор означал бы, что отказ выдан не по
		 *       негодному ответу, а по истечении срока
		 */
		/**
		 * Потраченное время выводится вместе с числом просьб
		 *
		 * @details Отказ здесь плавает - примерно один прогон из десяти, - и по одному
		 *          числу просьб причину не назвать. Настройка проверки: срок ответа 300 мс,
		 *          попыток 2, значит отказ по истечении обеих приходит примерно к 600 мс.
		 *          Время около 300 мс означало бы, что модуль сдался после ПЕРВОЙ попытки
		 *          (дефект модуля), около 600 - что повторная просьба ушла, но до
		 *          маршрутизатора не дошла (потеря дейтаграммы)
		 */
		ASSERT_GE(router.requests(), pair.requests) << "вид порчи " << static_cast <int32_t> (pair.damage) << ", потрачено " << outcome.spent << " мс";
		// Выполняем проверку того, что просьба сверх положенного не повторялась
		if(pair.requests == 1) ASSERT_EQ(router.requests(), 1) << "вид порчи " << static_cast <int32_t> (pair.damage);
	}
}

/**
 * @brief Проверка снятия и продления перенаправления по договору PCP
 *
 * @details Договор велит просить о снятии тем же действием, что и о заведении, но с
 *          нулевым сроком жизни, а ответ на снятие несёт нулевой внешний порт и нулевой
 *          срок. Перенять их значило бы выдать вызывающему не то перенаправление,
 *          которое убиралось, и потому назначенное маршрутизатором перенимается при
 *          заведении и продлении, но не при снятии
 *
 * @note Ходы эти не исполнялись ни одним испытанием: настоящий маршрутизатор для них
 *       обязателен, а поддельный отвечает одинаково на любой машине
 *
 */
TEST_F(PortmapUnitFixture, PortmapPcpCloseAndRenew) {
	/**
	 * Выполняем перебор просьб, с которыми ведётся обращение
	 */
	for(const awh::unit::portmap_t::action_t action : {
		awh::unit::portmap_t::action_t::CLOSE, awh::unit::portmap_t::action_t::RENEW
	}){
		// Создаём поддельный маршрутизатор договора PCP
		PcpRouter router;
		// Если порт договора занят, испытание не проводится
		if(!router.ready()) GTEST_SKIP() << "PCP port is occupied";
		// Выполняем запуск поддельного маршрутизатора
		router.start();
		// Создаём объект модуля перенаправления портов
		awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
		// Выполняем настройку модуля на поддельный маршрутизатор
		::setup(portmap);
		// Выполняем ожидание итога обращения к маршрутизатору
		const outcome_t outcome = this->await(portmap, action);
		// Выполняем остановку поддельного маршрутизатора
		router.stop();
		// Выполняем проверку того, что маршрутизатор ответил
		ASSERT_TRUE(outcome.answered) << "просьба " << static_cast <int32_t> (action);
		// Выполняем проверку того, что обращение отказом не завершилось
		ASSERT_FALSE(outcome.failed) << "просьба " << static_cast <int32_t> (action);
		// Выполняем проверку договора, по которому получен итог
		ASSERT_EQ(outcome.type, awh::unit::portmap_t::type_t::PCP);
		// Выполняем проверку того, что просьба до маршрутизатора дошла один раз
		ASSERT_EQ(router.requests(), 1) << "просьба " << static_cast <int32_t> (action);
		/**
		 * Если продлевался срок заведённого перенаправления
		 */
		if(action == awh::unit::portmap_t::action_t::RENEW){
			// Выполняем проверку того, что перенят назначенный маршрутизатором внешний порт
			ASSERT_EQ(outcome.mapping.externalPort, PCP_EXTERNAL_PORT);
			// Выполняем проверку того, что перенят назначенный маршрутизатором срок жизни
			ASSERT_EQ(outcome.mapping.lifeTime, 60u);
		/**
		 * Если убиралось заведённое перенаправление порта
		 */
		} else {
			/**
			 * Выполняем проверку того, что назначенное маршрутизатором не перенято
			 *
			 * @note Ответ на снятие несёт нулевой внешний порт по договору, и перенять
			 *       его значило бы подменить убранное перенаправление пустым
			 */
			ASSERT_NE(outcome.mapping.externalPort, PCP_EXTERNAL_PORT);
			// Выполняем проверку того, что внутренний порт остался запрошенным
			ASSERT_EQ(outcome.mapping.internalPort, PCP_INTERNAL_PORT);
		}
	}
}


/**
 * @brief Порт машины, принимающий объявления маршрутизатора
 *
 */
static constexpr uint16_t PCP_ANNOUNCE_PORT = 5350;

/**
 * @brief Метод рассылки объявления маршрутизатора договора PCP
 *
 * @details Объявление собирается ответом на действие ANNOUNCE, как его и рассылает
 *          маршрутизатор: полезной части у этого действия нет вовсе, и объявление
 *          занимает один заголовок договора
 *
 * @note Объявление шлётся прямо на порт петли, а не на групповой адрес: групповая
 *       рассылка петлёй ходит не на всякой машине, и испытание зависело бы от
 *       устройства сети, а проверяется здесь разбор объявления, а не доставка
 *
 * @param epoch отсчёт времени работы, называемый в объявлении
 *
 */
static void proclaimPcp(const uint32_t epoch) noexcept {
	// Выполняем заведение гнезда рассылки объявления
	const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
	// Если гнездо завести не удалось, выходим
	if(fd < 0) return;
	// Собираемое объявление маршрутизатора
	uint8_t message[PCP_HEADER_SIZE] = {0};
	// Записываем издание договора
	message[0] = PCP_VERSION;
	// Записываем действие объявления с отметкой ответа
	message[1] = PCP_RESPONSE_FLAG;
	// Записываем отсчёт времени работы маршрутизатора
	message[8] = static_cast <uint8_t> (epoch >> 24);
	// Записываем отсчёт времени работы маршрутизатора
	message[9] = static_cast <uint8_t> ((epoch >> 16) & 0xFF);
	// Записываем отсчёт времени работы маршрутизатора
	message[10] = static_cast <uint8_t> ((epoch >> 8) & 0xFF);
	// Записываем отсчёт времени работы маршрутизатора
	message[11] = static_cast <uint8_t> (epoch & 0xFF);
	// Адрес приёмника объявлений
	struct sockaddr_in target;
	// Выполняем очистку адреса приёмника объявлений
	::memset(&target, 0, sizeof(target));
	// Устанавливаем разновидность сети
	target.sin_family = AF_INET;
	// Устанавливаем порт приёма объявлений
	target.sin_port = htons(PCP_ANNOUNCE_PORT);
	// Устанавливаем адрес петли
	target.sin_addr.s_addr = ::inet_addr("127.0.0.1");
	// Выполняем рассылку объявления маршрутизатора
	::sendto(fd, reinterpret_cast <const char *> (message), sizeof(message), 0, reinterpret_cast <struct sockaddr *> (&target), sizeof(target));
	// Выполняем закрытие гнезда рассылки объявления
	::closesocket(fd);
}

/**
 * @brief Проверка обнаружения утраты состояния маршрутизатором по договору PCP
 *
 * @details Отсчёт времени работы маршрутизатора несут оба дейтаграммных договора, и
 *          сверяется он у каждого ответа: откат означает, что маршрутизатор
 *          перезагрузился и заведённые перенаправления утратил (RFC 6887, раздел 8.5)
 *
 * @note Отсчёт держится отдельно на каждый договор, и проверка эта не повторяет
 *       проверку договора NAT-PMP: маршрутизатор вправе вести договоры разными
 *       службами, и совпадения отсчётов они не требуют
 *
 */
TEST_F(PortmapUnitFixture, PortmapPcpEpochRollback) {
	// Создаём поддельный маршрутизатор договора PCP
	PcpRouter router;
	// Если порт договора занят, испытание не проводится
	if(!router.ready()) GTEST_SKIP() << "PCP port is occupied";
	// Выполняем запуск поддельного маршрутизатора
	router.start();
	// Создаём объект модуля перенаправления портов
	awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
	// Выполняем настройку модуля на поддельный маршрутизатор
	::setup(portmap);
	// Количество объявлений об утрате состояния маршрутизатором
	size_t resets = 0;
	// Устанавливаем функцию обратного вызова на утрату состояния маршрутизатором
	portmap.on <void (const awh::unit::portmap_t::type_t)> (
		"reset", [&resets](const awh::unit::portmap_t::type_t) noexcept -> void {
			// Запоминаем объявление об утрате состояния маршрутизатором
			resets++;
		}, std::placeholders::_1
	);
	// Выполняем первое обращение к маршрутизатору
	const outcome_t first = this->await(portmap, awh::unit::portmap_t::action_t::OPEN);
	// Выполняем проверку того, что первый ответ принят
	ASSERT_TRUE(first.answered);
	// Выполняем проверку того, что первый ответ утратой состояния не признан
	ASSERT_EQ(resets, static_cast <size_t> (0));
	// Задаём отсчёт, откатившийся назад перезагрузкой маршрутизатора
	router.epoch.store(1);
	// Выполняем второе обращение к маршрутизатору
	const outcome_t second = this->await(portmap, awh::unit::portmap_t::action_t::OPEN);
	// Выполняем остановку поддельного маршрутизатора
	router.stop();
	// Выполняем проверку того, что второй ответ принят
	ASSERT_TRUE(second.answered);
	// Выполняем проверку того, что утрата состояния маршрутизатором объявлена
	ASSERT_EQ(resets, static_cast <size_t> (1));
}

/**
 * @brief Проверка приёма объявления маршрутизатора по договору PCP
 *
 * @details Объявления обоих договоров приходят на один и тот же порт и разделяются
 *          изданием в первом октете. Проверяется здесь именно ветка договора PCP:
 *          действие ANNOUNCE полезной части не имеет вовсе, и объявление занимает
 *          один заголовок
 *
 * @note Обращение к маршрутизатору ведётся туда, откуда ответа не будет, и служит оно
 *       лишь пределом: не приди объявления вовсе - работа всё равно прекратится, а
 *       испытание объявит неудачу проверкой, а не зависанием
 *
 */
TEST_F(PortmapUnitFixture, PortmapPcpAnnouncement) {
	// Создаём объект модуля перенаправления портов
	awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
	// Выполняем настройку модуля на поддельный маршрутизатор
	::setup(portmap);
	// Если приём объявлений завести не удалось, испытание не проводится
	if(!portmap.announce(true)) GTEST_SKIP() << "PCP announcement port is occupied";
	// Количество объявлений об утрате состояния маршрутизатором
	std::atomic <size_t> resets{0};
	// Договор, по которому объявлена утрата состояния
	std::atomic <awh::unit::portmap_t::type_t> type{awh::unit::portmap_t::type_t::NONE};
	/**
	 * Устанавливаем функцию обратного вызова на утрату состояния маршрутизатором
	 *
	 * @note Работа модуля прекращается объявлением утраты: цикл базы событий сам собой
	 *       не остановится, а проверяемое к этой поре уже случилось
	 */
	portmap.on <void (const awh::unit::portmap_t::type_t)> (
		"reset", [&portmap, &resets, &type](const awh::unit::portmap_t::type_t protocol) noexcept -> void {
			// Запоминаем договор, по которому объявлена утрата состояния
			type.store(protocol);
			// Запоминаем объявление об утрате состояния маршрутизатором
			resets.fetch_add(1);
			// Выполняем остановку работы модуля
			portmap.stop();
		}, std::placeholders::_1
	);
	// Устанавливаем функцию обратного вызова на отказ перенаправления
	portmap.on <void (const awh::unit::portmap_t::error_t, const awh::unit::portmap_t::type_t)> (
		"failure", [&portmap](const awh::unit::portmap_t::error_t, const awh::unit::portmap_t::type_t) noexcept -> void {
			// Выполняем остановку работы модуля
			portmap.stop();
		}, std::placeholders::_1, std::placeholders::_2
	);
	// Признак продолжения рассылки объявлений
	std::atomic <bool> working{true};
	// Выполняем запуск потока рассылки объявлений маршрутизатора
	std::thread sender([&working]() noexcept -> void {
		/**
		 * Выполняем рассылку объявлений, пока работа не прекращена
		 */
		while(working.load()){
			// Выполняем рассылку объявления с исходным отсчётом времени работы
			::proclaimPcp(1000);
			// Выполняем ожидание перед рассылкой объявления об утрате состояния
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
			// Выполняем рассылку объявления с откатившимся отсчётом времени работы
			::proclaimPcp(1);
			// Выполняем ожидание перед следующей рассылкой
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
		}
	});
	// Заводимое перенаправление порта
	awh::unit::portmap_t::mapping_t mapping;
	// Устанавливаем договор перенаправляемого порта
	mapping.proto = awh::unit::portmap_t::proto_t::TCP;
	// Устанавливаем внутренний порт перенаправления
	mapping.internalPort = PCP_INTERNAL_PORT;
	// Устанавливаем срок жизни перенаправления в секундах
	mapping.lifeTime = 60;
	// Выполняем заведение перенаправления порта
	portmap.open(mapping);
	// Выполняем запуск работы модуля
	portmap.start();
	// Запоминаем, что рассылка объявлений прекращена
	working.store(false);
	// Дожидаемся завершения потока рассылки объявлений
	sender.join();
	// Выполняем отключение приёма объявлений маршрутизатора
	portmap.announce(false);
	// Выполняем проверку того, что утрата состояния маршрутизатором объявлена
	ASSERT_GT(resets.load(), static_cast <size_t> (0));
	// Выполняем проверку того, что утрата объявлена именно по договору PCP
	ASSERT_EQ(type.load(), awh::unit::portmap_t::type_t::PCP);
}

/**
 * @brief Метод рассылки объявления маршрутизатора договора PCP по сети IPv6
 *
 * @details Объявление то же, что и в сети IPv4: договор PCP разновидностями сети
 *          сообщения не разнит, и разбирается объявление одним и тем же ходом
 *
 * @param epoch отсчёт времени работы, называемый в объявлении
 * @return      признак того, что объявление разослано
 *
 */
static bool proclaimPcp6(const uint32_t epoch) noexcept {
	// Выполняем заведение гнезда рассылки объявления
	const int fd = ::socket(AF_INET6, SOCK_DGRAM, 0);
	// Если гнездо завести не удалось, выводим отрицательный результат
	if(fd < 0) return false;
	// Собираемое объявление маршрутизатора
	uint8_t message[PCP_HEADER_SIZE] = {0};
	// Записываем издание договора
	message[0] = PCP_VERSION;
	// Записываем действие объявления с отметкой ответа
	message[1] = PCP_RESPONSE_FLAG;
	// Записываем отсчёт времени работы маршрутизатора
	message[8] = static_cast <uint8_t> (epoch >> 24);
	// Записываем отсчёт времени работы маршрутизатора
	message[9] = static_cast <uint8_t> ((epoch >> 16) & 0xFF);
	// Записываем отсчёт времени работы маршрутизатора
	message[10] = static_cast <uint8_t> ((epoch >> 8) & 0xFF);
	// Записываем отсчёт времени работы маршрутизатора
	message[11] = static_cast <uint8_t> (epoch & 0xFF);
	// Адрес приёмника объявлений
	struct sockaddr_in6 target;
	// Выполняем очистку адреса приёмника объявлений
	::memset(&target, 0, sizeof(target));
	// Устанавливаем разновидность сети
	target.sin6_family = AF_INET6;
	// Устанавливаем порт приёма объявлений
	target.sin6_port = htons(PCP_ANNOUNCE_PORT);
	// Устанавливаем адрес петли
	::inet_pton(AF_INET6, "::1", &target.sin6_addr);
	// Выполняем рассылку объявления маршрутизатора
	const ssize_t size = ::sendto(fd, reinterpret_cast <const char *> (message), sizeof(message), 0, reinterpret_cast <struct sockaddr *> (&target), sizeof(target));
	// Выполняем закрытие гнезда рассылки объявления
	::closesocket(fd);
	// Выводим признак того, что объявление разослано
	return (size == static_cast <ssize_t> (sizeof(message)));
}

/**
 * @brief Проверка приёма объявления маршрутизатора по сети IPv6
 *
 * @details Приёмник объявлений сети IPv6 привязывается неопределённым адресом и
 *          разбирает то же объявление, что и в сети IPv4. Проверяется здесь именно
 *          привязка и приём: разбор объявления закреплён проверкой сети IPv4
 *
 * @note Объявления рассылаются до запуска работы модуля намеренно: гнездо приёмника
 *       к этой поре уже привязано, и присланное на него ложится в очередь гнезда, а
 *       выбирается запущенным циклом базы событий. Так испытание не зависит от того,
 *       успел ли поток рассылки попасть в отведённое ему время
 *
 * @note Сети IPv6 на машине может не быть вовсе - тогда рассылка не проходит, и
 *       испытание не проводится: доказать этим нечего ни в ту, ни в другую сторону
 *
 */
TEST_F(PortmapUnitFixture, PortmapPcpAnnouncementIPv6) {
	// Создаём объект модуля перенаправления портов
	awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
	// Устанавливаем вид опроса маршрутизатора
	portmap.setType(awh::unit::portmap_t::type_t::PCP);
	// Устанавливаем разновидность сети, в которой ведётся обмен
	portmap.setFamily(awh::unit::portmap_t::family_t::IPV6);
	// Устанавливаем адрес поддельного маршрутизатора
	portmap.setRouter("::1");
	// Устанавливаем срок ожидания ответа маршрутизатора
	portmap.setTimeout(300);
	// Устанавливаем количество попыток обращения к маршрутизатору
	portmap.setAttempts(2);
	// Если приём объявлений завести не удалось, испытание не проводится
	if(!portmap.announce(true)) GTEST_SKIP() << "PCP announcement port is occupied";
	// Если объявление разослать не удалось, испытание не проводится
	if(!::proclaimPcp6(1000)) GTEST_SKIP() << "IPv6 network is unavailable";
	// Выполняем рассылку объявления с откатившимся отсчётом времени работы
	::proclaimPcp6(1);
	// Количество объявлений об утрате состояния маршрутизатором
	std::atomic <size_t> resets{0};
	// Устанавливаем функцию обратного вызова на утрату состояния маршрутизатором
	portmap.on <void (const awh::unit::portmap_t::type_t)> (
		"reset", [&portmap, &resets](const awh::unit::portmap_t::type_t) noexcept -> void {
			// Запоминаем объявление об утрате состояния маршрутизатором
			resets.fetch_add(1);
			// Выполняем остановку работы модуля
			portmap.stop();
		}, std::placeholders::_1
	);
	// Устанавливаем функцию обратного вызова на отказ перенаправления
	portmap.on <void (const awh::unit::portmap_t::error_t, const awh::unit::portmap_t::type_t)> (
		"failure", [&portmap](const awh::unit::portmap_t::error_t, const awh::unit::portmap_t::type_t) noexcept -> void {
			// Выполняем остановку работы модуля
			portmap.stop();
		}, std::placeholders::_1, std::placeholders::_2
	);
	// Заводимое перенаправление порта
	awh::unit::portmap_t::mapping_t mapping;
	// Устанавливаем договор перенаправляемого порта
	mapping.proto = awh::unit::portmap_t::proto_t::TCP;
	// Устанавливаем внутренний порт перенаправления
	mapping.internalPort = PCP_INTERNAL_PORT;
	// Устанавливаем срок жизни перенаправления в секундах
	mapping.lifeTime = 60;
	// Выполняем заведение перенаправления порта
	portmap.open(mapping);
	// Выполняем запуск работы модуля
	portmap.start();
	// Выполняем отключение приёма объявлений маршрутизатора
	portmap.announce(false);
	// Выполняем проверку того, что утрата состояния маршрутизатором объявлена
	ASSERT_GT(resets.load(), static_cast <size_t> (0));
}
