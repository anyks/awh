/**
 * @file: sctp.cpp
 * @date: 2026-06-17
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Тесты управления протоколом SCTP — проверка настройки параметров ассоциаций, входящих и исходящих потоков,
 *        heartbeat и подписки на уведомления сокета
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "eth.hpp"

/**
 * Модуль SCTP доступен на системах с поддержкой протокола: Linux, FreeBSD, Solaris и illumos
 */
#if __linux__ || __FreeBSD__ || __sun

/**
 * Подключаем стандартные заголовочные файлы
 */
#include <string>
#include <vector>

/**
 * Подключаем системные заголовочные файлы
 */
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>

/**
 * @brief Внутренние служебные функции тестов
 *
 */
namespace {
	/**
	 * @brief RAII-обёртка над сетевым SCTP-сокетом
	 *
	 * @note Создаёт одиночный (one-to-one) SCTP-сокет и автоматически закрывает его в деструкторе.
	 *       Если ядро не поддерживает SCTP, дескриптор будет невалидным (valid() вернёт false).
	 *
	 */
	class SctpSocket {
		public:
			// Файловый дескриптор сокета
			awh::net::socket_t fd;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit SctpSocket() noexcept : fd(::socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) {}
			/**
			 * @brief Деструктор
			 *
			 */
			~SctpSocket() noexcept {
				// Если сокет был успешно создан
				if(this->fd >= 0)
					// Закрываем сокет
					::close(this->fd);
			}
		public:
			/**
			 * @brief Метод проверки валидности сокета
			 *
			 * @return флаг валидности сокета
			 *
			 */
			bool valid() const noexcept {
				// Сокет валиден, если дескриптор неотрицательный
				return (this->fd >= 0);
			}
	};
}

/**
 * @brief Тест инициализации параметров рукопожатия SCTP сокета
 *
 */
TEST_F(EthFixture, SctpInitMessages){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Формируем параметры инициализации SCTP сокета
	awh::net::sctp::initmsg_t initmsg;
	// Устанавливаем количество попыток инициализации
	initmsg.attempts = 3;
	// Устанавливаем таймаут инициализации в миллисекундах
	initmsg.timeout = 5000;
	// Устанавливаем количество исходящих потоков
	initmsg.ostreams = 8;
	// Устанавливаем количество входящих потоков
	initmsg.istreams = 8;
	// Инициализация параметров рукопожатия должна завершиться успешно
	ASSERT_TRUE(this->_eth->sctp.initMessages(sock.fd, initmsg));
}
/**
 * @brief Тест круговой записи/чтения таймаута INIT через SCTP_INITMSG
 *
 * @note Проверяет, что таймаут INIT устанавливается и считывается именно через SCTP_INITMSG,
 *       а не через SCTP_RTOINFO (исправление разделения INIT/DATA)
 *
 */
TEST_F(EthFixture, SctpTimeoutInitRoundTrip){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Новое значение таймаута INIT в миллисекундах
	const uint32_t value = 4000;
	// Устанавливаем таймаут INIT
	ASSERT_TRUE(this->_eth->sctp.timeout(sock.fd, 0, awh::net::sctp::timeout_t::INIT, value));
	// Считанный таймаут INIT должен совпадать с установленным
	ASSERT_EQ(this->_eth->sctp.timeout(sock.fd, 0, awh::net::sctp::timeout_t::INIT), value);
}
/**
 * @brief Тест круговой записи/чтения таймаута DATA через SCTP_RTOINFO
 *
 */
TEST_F(EthFixture, SctpTimeoutDataRoundTrip){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Новое значение начального таймаута RTO в миллисекундах (в пределах диапазона min/max по умолчанию)
	const uint32_t value = 5000;
	// Устанавливаем таймаут DATA
	ASSERT_TRUE(this->_eth->sctp.timeout(sock.fd, 0, awh::net::sctp::timeout_t::DATA, value));
	// Считанный таймаут DATA должен совпадать с установленным
	ASSERT_EQ(this->_eth->sctp.timeout(sock.fd, 0, awh::net::sctp::timeout_t::DATA), value);
}
/**
 * @brief Тест круговой записи/чтения таймаута COOKIE через SCTP_ASSOCINFO
 *
 */
TEST_F(EthFixture, SctpTimeoutCookieRoundTrip){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Новое значение времени жизни cookie в миллисекундах
	const uint32_t value = 30000;
	// Устанавливаем таймаут COOKIE
	ASSERT_TRUE(this->_eth->sctp.timeout(sock.fd, 0, awh::net::sctp::timeout_t::COOKIE, value));
	// Считанное время жизни cookie должно совпадать с установленным
	ASSERT_EQ(this->_eth->sctp.timeout(sock.fd, 0, awh::net::sctp::timeout_t::COOKIE), value);
}
/**
 * @brief Тест обработки неподдерживаемых типов таймаутов SCTP
 *
 * @note Для SACK/SHUTDOWN/SHUTDOWNACK сеттер должен возвращать false, а геттер - 0
 *
 */
TEST_F(EthFixture, SctpTimeoutUnsupported){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Установка таймаута SACK не поддерживается
	ASSERT_FALSE(this->_eth->sctp.timeout(sock.fd, 0, awh::net::sctp::timeout_t::SACK, 1000));
	// Чтение таймаута SACK не поддерживается и возвращает 0
	ASSERT_EQ(this->_eth->sctp.timeout(sock.fd, 0, awh::net::sctp::timeout_t::SACK), 0U);
	// Установка таймаута SHUTDOWN не поддерживается
	ASSERT_FALSE(this->_eth->sctp.timeout(sock.fd, 0, awh::net::sctp::timeout_t::SHUTDOWN, 1000));
	// Чтение таймаута SHUTDOWN не поддерживается и возвращает 0
	ASSERT_EQ(this->_eth->sctp.timeout(sock.fd, 0, awh::net::sctp::timeout_t::SHUTDOWN), 0U);
	// Установка таймаута SHUTDOWNACK не поддерживается
	ASSERT_FALSE(this->_eth->sctp.timeout(sock.fd, 0, awh::net::sctp::timeout_t::SHUTDOWNACK, 1000));
	// Чтение таймаута SHUTDOWNACK не поддерживается и возвращает 0
	ASSERT_EQ(this->_eth->sctp.timeout(sock.fd, 0, awh::net::sctp::timeout_t::SHUTDOWNACK), 0U);
}
/**
 * @brief Тест подписки на устаревшие (legacy) события SCTP через SCTP_EVENTS
 *
 */
TEST_F(EthFixture, SctpEventsSubscribeLegacy){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Подписка на набор устаревших событий должна завершиться успешно
	ASSERT_TRUE(this->_eth->sctp.eventsSubscribe(sock.fd, {
		awh::net::sctp::event_type_t::DATA_IO,
		awh::net::sctp::event_type_t::ASSOC_CHANGE,
		awh::net::sctp::event_type_t::SHUTDOWN_EVENT,
		awh::net::sctp::event_type_t::SEND_FAILED_EVENT,
		awh::net::sctp::event_type_t::REMOTE_ERROR,
		awh::net::sctp::event_type_t::STREAM_RESET_EVENT
	}));
}
/**
 * @brief Тест подписки на современные события SCTP через SCTP_EVENT
 *
 * @note Проверяет события RFC 6525 (сброс ассоциации и изменение потоков),
 *       которые отсутствуют в устаревшей структуре sctp_event_subscribe
 *
 */
TEST_F(EthFixture, SctpEventsSubscribeModern){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Подписка на современные события должна завершиться успешно
	ASSERT_TRUE(this->_eth->sctp.eventsSubscribe(sock.fd, {
		awh::net::sctp::event_type_t::ASSOC_RESET_EVENT,
		awh::net::sctp::event_type_t::STREAM_CHANGE_EVENT
	}));
}
/**
 * @brief Тест смешанной подписки на устаревшие и современные события SCTP
 *
 */
TEST_F(EthFixture, SctpEventsSubscribeMixed){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Подписка на смешанный набор событий должна завершиться успешно
	ASSERT_TRUE(this->_eth->sctp.eventsSubscribe(sock.fd, {
		awh::net::sctp::event_type_t::ASSOC_CHANGE,
		awh::net::sctp::event_type_t::ASSOC_RESET_EVENT,
		awh::net::sctp::event_type_t::STREAM_RESET_EVENT,
		awh::net::sctp::event_type_t::STREAM_CHANGE_EVENT
	}));
}
/**
 * @brief Тест подписки на пустой список событий SCTP
 *
 * @note Пустой список не должен обнулять подписку и обязан детерминированно вернуть false
 *
 */
TEST_F(EthFixture, SctpEventsSubscribeEmpty){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Подписка на пустой список событий должна завершиться неудачей
	ASSERT_FALSE(this->_eth->sctp.eventsSubscribe(sock.fd, {}));
}
/**
 * @brief Тест установки поддерживаемых алгоритмов аутентификации SCTP сокета
 *
 */
TEST_F(EthFixture, SctpAuthenticateSupportAlgorithms){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Установка поддерживаемых алгоритмов аутентификации должна завершиться успешно
	ASSERT_TRUE(this->_eth->sctp.authenticateSupportAlgorithms(sock.fd, {
		awh::net::sctp::auth_type_t::HMAC_SHA1,
		awh::net::sctp::auth_type_t::HMAC_SHA256
	}));
}
/**
 * @brief Тест установки пустого списка алгоритмов аутентификации SCTP сокета
 *
 */
TEST_F(EthFixture, SctpAuthenticateSupportAlgorithmsEmpty){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Установка пустого списка алгоритмов аутентификации должна завершиться неудачей
	ASSERT_FALSE(this->_eth->sctp.authenticateSupportAlgorithms(sock.fd, {}));
}
/**
 * @brief Тест установки ключа аутентификации SCTP сокета
 *
 */
TEST_F(EthFixture, SctpAuthenticateKey){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Установка ключа аутентификации должна завершиться успешно
	ASSERT_TRUE(this->_eth->sctp.authenticateKey(sock.fd, 1, "super-secret-sctp-key"));
}
/**
 * @brief Тест установки пустого ключа аутентификации SCTP сокета
 *
 */
TEST_F(EthFixture, SctpAuthenticateKeyEmpty){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Установка пустого ключа аутентификации должна завершиться неудачей
	ASSERT_FALSE(this->_eth->sctp.authenticateKey(sock.fd, 1, ""));
}
/**
 * @brief Тест активации и деактивации ключа аутентификации SCTP сокета
 *
 */
TEST_F(EthFixture, SctpAuthenticateKeyActivateDeactivate){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Устанавливаем ключ аутентификации, который будем активировать
	ASSERT_TRUE(this->_eth->sctp.authenticateKey(sock.fd, 5, "active-key-material"));
	// Активация ключа аутентификации должна завершиться успешно
	ASSERT_TRUE(this->_eth->sctp.authenticateKey(sock.fd, awh::net::socket_mode_t::ENABLED, 0, 5));
	// Деактивация (удаление) ранее активного ключа допустима не на всех ядрах, поэтому проверяем лишь отсутствие падения
	(void) this->_eth->sctp.authenticateKey(sock.fd, awh::net::socket_mode_t::DISABLED, 0, 5);
}
/**
 * @brief Тест установки чанков аутентификации SCTP сокета
 *
 */
TEST_F(EthFixture, SctpAuthenticateChunksSet){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Установка чанков аутентификации должна завершиться успешно
	ASSERT_TRUE(this->_eth->sctp.authenticateChunks(sock.fd, {
		awh::net::sctp::auth_chunk_t::DATA,
		awh::net::sctp::auth_chunk_t::COOKIE_ECHO
	}));
}
/**
 * @brief Тест установки пустого списка чанков аутентификации SCTP сокета
 *
 */
TEST_F(EthFixture, SctpAuthenticateChunksSetEmpty){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Установка пустого списка чанков аутентификации должна завершиться неудачей
	ASSERT_FALSE(this->_eth->sctp.authenticateChunks(sock.fd, std::vector <awh::net::sctp::auth_chunk_t> {}));
}
/**
 * @brief Тест извлечения локальных чанков аутентификации SCTP сокета
 *
 * @note Основная цель - проверить безопасность чтения переменной длины (исправление переполнения буфера):
 *       вызов не должен приводить к порче памяти, а количество чанков должно укладываться в выделенный буфер
 *
 */
TEST_F(EthFixture, SctpAuthenticateChunksGetLocal){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Контейнер для извлечённых чанков аутентификации
	std::vector <awh::net::sctp::auth_chunk_t> chunks;
	// Извлекаем локальные чанки аутентификации (вызов должен корректно завершиться без порчи памяти)
	const bool result = this->_eth->sctp.authenticateChunks(sock.fd, awh::event::origin_t::LOCAL, 0, chunks);
	// Если извлечение выполнено успешно
	if(result)
		// Количество извлечённых чанков не должно превышать ёмкость выделенного буфера
		ASSERT_LE(chunks.size(), static_cast <size_t> (256));
}
/**
 * @brief Тест извлечения удалённых чанков аутентификации SCTP сокета
 *
 * @note На неустановленной ассоциации извлечение удалённых чанков обычно невозможно;
 *       тест проверяет лишь корректное (безопасное) завершение вызова
 *
 */
TEST_F(EthFixture, SctpAuthenticateChunksGetRemote){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Контейнер для извлечённых чанков аутентификации
	std::vector <awh::net::sctp::auth_chunk_t> chunks;
	// Извлекаем удалённые чанки аутентификации (вызов должен корректно завершиться без порчи памяти)
	const bool result = this->_eth->sctp.authenticateChunks(sock.fd, awh::event::origin_t::REMOTE, 0, chunks);
	// Если извлечение выполнено успешно
	if(result)
		// Количество извлечённых чанков не должно превышать ёмкость выделенного буфера
		ASSERT_LE(chunks.size(), static_cast <size_t> (256));
}
/**
 * @brief Тест получения статуса SCTP сокета на неустановленной ассоциации
 *
 * @note На неподключённом сокете статус может быть недоступен; тест проверяет отсутствие падения
 *       и валидность возвращаемого состояния при успешном чтении
 *
 */
TEST_F(EthFixture, SctpStatusUnconnected){
	// Создаём SCTP-сокет
	SctpSocket sock;
	// Если ядро не поддерживает SCTP
	if(!sock.valid())
		// Пропускаем тест
		GTEST_SKIP() << "SCTP is not supported by the kernel";
	// Создаём объект статуса SCTP сокета
	awh::net::sctp::status_t status;
	// Получаем статус SCTP сокета
	if(this->_eth->sctp.status(sock.fd, status))
		// Возвращаемое состояние должно быть валидным значением перечисления
		ASSERT_LE(static_cast <uint8_t> (status.state), static_cast <uint8_t> (awh::net::sctp::state_status_t::SHUTDOWN_ACK_SENT));
}

#endif // __linux__ || __FreeBSD__ || __sun
