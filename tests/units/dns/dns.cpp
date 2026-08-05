/**
 * @file: dns.cpp
 * @date: 2026-08-03
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры DNS-резолвера —
 *        подставной сервер имён и настройка резолвера на него
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "dns.hpp"

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * @brief Адрес, которым подставной сервер отвечает на всякий вопрос
 *
 * @details Сеть эта отведена договором RFC 5737 под примеры в описаниях и в
 *          движении не встречается: спутать её с настоящим ответом нельзя
 *
 */
static constexpr uint8_t STUB_ADDRESS[4] = {192, 0, 2, 77};

/**
 * @brief Размер заголовка сообщения DNS в октетах
 *
 */
static constexpr size_t DNS_HEADER_SIZE = 12;

/**
 * @brief Размер сообщения NTP в октетах
 *
 */
static constexpr size_t NTP_PACKET_SIZE = 48;

/**
 * @brief Смещение исходной метки времени в сообщении NTP
 *
 */
static constexpr size_t NTP_ORIGINATE_OFFSET = 24;

/**
 * @brief Смещение метки отправки в сообщении NTP
 *
 */
static constexpr size_t NTP_TRANSMIT_OFFSET = 40;

/**
 * @brief Конструктор
 *
 */
DNSStubServer::DNSStubServer() noexcept :
 _fd(-1), _port(0), _delay(0), _silent(false), _foreign(0), _running(false), _received(0) {}

/**
 * @brief Деструктор
 *
 */
DNSStubServer::~DNSStubServer() noexcept {
	// Выполняем остановку сервера
	this->stop();
}

/**
 * @brief Метод получения порта, на котором сервер принимает вопросы
 *
 * @return порт сервера
 *
 */
uint16_t DNSStubServer::port() const noexcept {
	// Выводим порт сервера
	return this->_port;
}

/**
 * @brief Метод получения числа принятых сервером вопросов
 *
 * @return число принятых вопросов
 *
 */
uint32_t DNSStubServer::received() const noexcept {
	// Выводим число принятых вопросов
	return this->_received.load();
}

/**
 * @brief Метод запуска сервера
 *
 * @param delay   задержка ответа на вопрос в миллисекундах
 * @param silent  признак того, что отвечать на вопросы не следует вовсе
 * @param foreign число ответов, выдаваемых с подменённым номером вопроса
 * @return        результат запуска сервера
 *
 */
bool DNSStubServer::start(const uint32_t delay, const bool silent, const uint32_t foreign) noexcept {
	// Запоминаем задержку ответа на вопрос
	this->_delay = delay;
	// Запоминаем признак того, что отвечать не следует
	this->_silent = silent;
	// Запоминаем число ответов, выдаваемых с подменённым номером вопроса
	this->_foreign = foreign;
	// Выполняем создание дейтаграммного сокета
	this->_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
	// Если сокет создать не удалось
	if(this->_fd < 0)
		// Выводим отрицательный результат запуска сервера
		return false;
	// Адрес, на котором сервер принимает вопросы
	struct sockaddr_in addr{};
	// Устанавливаем семейство адреса
	addr.sin_family = AF_INET;
	// Устанавливаем адрес устройства петли
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	/**
	 * Порт выбирается системой
	 *
	 * @note Занимать порт постоянный нельзя: проверки идут вперемешку с прочими
	 *       наборами, и постоянный порт обернулся бы отказом привязки на машине,
	 *       где он уже занят
	 */
	addr.sin_port = 0;
	// Выполняем привязку сокета к адресу
	if(::bind(this->_fd, reinterpret_cast <struct sockaddr *> (&addr), sizeof(addr)) < 0){
		// Выполняем закрытие сокета
		::close(this->_fd);
		// Сбрасываем дескриптор сокета
		this->_fd = -1;
		// Выводим отрицательный результат запуска сервера
		return false;
	}
	// Размер адреса, на котором сервер принимает вопросы
	socklen_t length = sizeof(addr);
	// Выполняем получение выбранного системой порта
	if(::getsockname(this->_fd, reinterpret_cast <struct sockaddr *> (&addr), &length) < 0){
		// Выполняем закрытие сокета
		::close(this->_fd);
		// Сбрасываем дескриптор сокета
		this->_fd = -1;
		// Выводим отрицательный результат запуска сервера
		return false;
	}
	// Запоминаем выбранный системой порт
	this->_port = ntohs(addr.sin_port);
	/**
	 * Выставляем срок ожидания приёма
	 *
	 * @details Без срока ожидания нить сервера простаивает в приёме безвыходно, и снять
	 *          её оттуда можно лишь надеясь, что закрытие дескриптора разбудит вызов в
	 *          ядре. Надежда эта зависит от системы: macOS и NetBSD вызов будят, а
	 *          OpenBSD закрытием чужую нить из приёма не снимает вовсе, и ожидание её
	 *          завершения стояло там навсегда
	 *
	 * @note Со сроком ожидания приём сам возвращается в круг, где сверяется признак
	 *       работы, и остановка опирается не на поведение системы, а на свой же
	 *       признак - одинаково на всякой из них
	 *
	 */
	struct timeval timeout{};
	// Устанавливаем целые секунды срока ожидания приёма
	timeout.tv_sec = 0;
	// Устанавливаем долю секунды срока ожидания приёма
	timeout.tv_usec = 50000;
	// Выполняем установку срока ожидания приёма
	::setsockopt(this->_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	// Выставляем признак работы нити сервера
	this->_running.store(true);
	// Выполняем запуск нити приёма вопросов
	this->_thread = std::thread(&DNSStubServer::run, this);
	// Выводим положительный результат запуска сервера
	return true;
}

/**
 * @brief Метод остановки сервера
 *
 */
void DNSStubServer::stop() noexcept {
	// Если сервер уже остановлен, выходим
	if(!this->_running.load() && !this->_thread.joinable())
		// Выходим из функции
		return;
	// Снимаем признак работы нити сервера
	this->_running.store(false);
	/**
	 * Сокет закрывается уже после завершения нити
	 *
	 * @note Закрыть его раньше значило бы оставить нити приём по сброшенному
	 *       дескриптору, а то и по чужому, если система успеет выдать тот же
	 *       номер новому сокету. Снимает нить с приёма выставленный ему срок
	 *       ожидания, а не закрытие, потому спешить с ним нужды нет
	 *
	 */
	if(this->_thread.joinable())
		// Выполняем ожидание завершения нити сервера
		this->_thread.join();
	// Если сокет заведён, выполняем его закрытие
	if(this->_fd >= 0){
		// Выполняем закрытие сокета
		::close(this->_fd);
		// Сбрасываем дескриптор сокета
		this->_fd = -1;
	}
}

/**
 * @brief Метод приёма вопросов и выдачи ответов
 *
 */
void DNSStubServer::run() noexcept {
	// Принимаемое сообщение
	uint8_t buffer[1500];
	// Адрес, с которого пришёл вопрос
	struct sockaddr_in from{};
	/**
	 * Выполняем приём вопросов, пока сервер работает
	 */
	while(this->_running.load()){
		// Размер адреса, с которого пришёл вопрос
		socklen_t length = sizeof(from);
		// Выполняем приём вопроса
		const ssize_t size = ::recvfrom(this->_fd, buffer, sizeof(buffer), 0, reinterpret_cast <struct sockaddr *> (&from), &length);
		// Если приём не удался, продолжаем работу
		if(size < static_cast <ssize_t> (DNS_HEADER_SIZE))
			// Переходим к следующему кругу приёма
			continue;
		// Выполняем подсчёт принятых вопросов
		this->_received.fetch_add(1);
		// Если отвечать на вопросы не следует вовсе
		if(this->_silent)
			// Переходим к следующему кругу приёма
			continue;
		// Если ответ следует задержать
		if(this->_delay > 0)
			// Выполняем ожидание перед выдачей ответа
			std::this_thread::sleep_for(std::chrono::milliseconds(this->_delay));
		/**
		 * Если за время ожидания сервер остановлен, ответ не отправляем
		 */
		if(!this->_running.load())
			// Завершаем работу нити сервера
			break;
		// Собираемый ответ
		std::vector <uint8_t> answer(buffer, buffer + size);
		/**
		 * Если ответ следует выдать с подменённым номером вопроса
		 *
		 * @note Номер портится прибавлением единицы к младшему октету: договору такой
		 *       ответ чужой, и отбросить его он обязан, ожидание при этом продолжив
		 */
		if(this->_foreign > 0){
			// Уменьшаем число оставшихся ответов с подменённым номером
			this->_foreign--;
			// Выполняем подмену младшего октета номера вопроса
			answer[1] = static_cast <uint8_t> (answer[1] + 1);
		}
		/**
		 * Выставляем признаки ответа
		 *
		 * @note Первый октет признаков несёт признак ответа и рекурсию, второй -
		 *       разрешённую рекурсию и нулевой код итога
		 */
		answer[2] = 0x81;
		answer[3] = 0x80;
		// Выставляем число записей в ответе
		answer[6] = 0x00;
		answer[7] = 0x01;
		/**
		 * Дописываем запись ответа сжатой ссылкой на имя вопроса
		 *
		 * @note Имя в ответе не повторяется, а указывается смещением к вопросу:
		 *       два старших разряда первого октета несут признак ссылки, прочие
		 *       четырнадцать - смещение, а вопрос лежит сразу за заголовком
		 */
		answer.push_back(0xC0);
		answer.push_back(static_cast <uint8_t> (DNS_HEADER_SIZE));
		// Дописываем вид записи ответа
		answer.push_back(0x00);
		answer.push_back(0x01);
		// Дописываем разряд записи ответа
		answer.push_back(0x00);
		answer.push_back(0x01);
		// Дописываем срок жизни записи ответа
		answer.push_back(0x00);
		answer.push_back(0x00);
		answer.push_back(0x00);
		answer.push_back(0x3C);
		// Дописываем размер данных записи ответа
		answer.push_back(0x00);
		answer.push_back(0x04);
		/**
		 * Дописываем сам адрес записи ответа
		 */
		for(const uint8_t octet : STUB_ADDRESS)
			// Дописываем октет адреса
			answer.push_back(octet);
		// Выполняем отправку ответа
		::sendto(this->_fd, answer.data(), answer.size(), 0, reinterpret_cast <struct sockaddr *> (&from), length);
	}
}

/**
 * @brief Метод настройки тестового окружения
 *
 */
void DNSUnitFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
	// Устанавливаем объект работы с логами
	this->_fmk->setLogger(this->_log.get());
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void DNSUnitFixture::TearDown() {}

/**
 * @brief Метод настройки резолвера на подставной сервер имён
 *
 * @param dns    объект DNS-резолвера
 * @param server подставной сервер имён
 * @param delay  срок ожидания ответа сервера в миллисекундах
 * @param count  число попыток обращения к серверу имён
 *
 */
void DNSUnitFixture::setup(awh::unit::dns_t & dns, const DNSStubServer & server, const uint32_t delay, const uint8_t count) const noexcept {
	/**
	 * Устанавливаем порт подставного сервера имён
	 *
	 * @warning Порядок здесь важен: резолвер заводит события обмена уже в
	 *          конструкторе, а `setTargetPort` лишь запоминает порт, событий не
	 *          пересоздавая. Переносит порт на события `setServers`, пересоздающий
	 *          их, - оттого порт и задаётся первым. Задай его следом, и вопросы
	 *          ушли бы на порт пятьдесят третий
	 */
	dns.setTargetPort(server.port());
	// Устанавливаем срок ожидания ответа сервера имён
	dns.setTimeout(delay);
	// Устанавливаем адрес подставного сервера имён
	dns.setServers(awh::event::family_t::IPV4, {"127.0.0.1"});
	// Устанавливаем число попыток обращения к серверу имён
	dns.setAttempts(count);
}

/**
 * @brief Конструктор
 *
 */
NTPStubServer::NTPStubServer() noexcept :
 _fd(-1), _port(0), _foreign(false), _running(false), _received(0) {}

/**
 * @brief Деструктор
 *
 */
NTPStubServer::~NTPStubServer() noexcept {
	// Выполняем остановку сервера
	this->stop();
}

/**
 * @brief Метод получения порта, на котором сервер принимает вопросы
 *
 * @return порт сервера
 *
 */
uint16_t NTPStubServer::port() const noexcept {
	// Выводим порт сервера
	return this->_port;
}

/**
 * @brief Метод получения числа принятых сервером вопросов
 *
 * @return число принятых вопросов
 *
 */
uint32_t NTPStubServer::received() const noexcept {
	// Выводим число принятых вопросов
	return this->_received.load();
}

/**
 * @brief Метод запуска сервера
 *
 * @param foreign признак того, что метку вопроса в ответ переписывать не следует
 * @return        результат запуска сервера
 *
 */
bool NTPStubServer::start(const bool foreign) noexcept {
	// Запоминаем признак того, что метку переписывать не следует
	this->_foreign = foreign;
	// Выполняем создание дейтаграммного сокета
	this->_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
	// Если сокет создать не удалось
	if(this->_fd < 0)
		// Выводим отрицательный результат запуска сервера
		return false;
	// Адрес, на котором сервер принимает вопросы
	struct sockaddr_in addr{};
	// Устанавливаем семейство адреса
	addr.sin_family = AF_INET;
	// Устанавливаем адрес устройства петли
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	// Порт выбирается системой
	addr.sin_port = 0;
	// Выполняем привязку сокета к адресу
	if(::bind(this->_fd, reinterpret_cast <struct sockaddr *> (&addr), sizeof(addr)) < 0){
		// Выполняем закрытие сокета
		::close(this->_fd);
		// Сбрасываем дескриптор сокета
		this->_fd = -1;
		// Выводим отрицательный результат запуска сервера
		return false;
	}
	// Размер адреса, на котором сервер принимает вопросы
	socklen_t length = sizeof(addr);
	// Выполняем получение выбранного системой порта
	if(::getsockname(this->_fd, reinterpret_cast <struct sockaddr *> (&addr), &length) < 0){
		// Выполняем закрытие сокета
		::close(this->_fd);
		// Сбрасываем дескриптор сокета
		this->_fd = -1;
		// Выводим отрицательный результат запуска сервера
		return false;
	}
	// Запоминаем выбранный системой порт
	this->_port = ntohs(addr.sin_port);
	/**
	 * Выставляем срок ожидания приёма
	 *
	 * @note Довод тот же, что и у сервера DNS выше: снятие нити с приёма закрытием
	 *       дескриптора у OpenBSD не работает, потому остановка опирается на срок
	 *       ожидания и собственный признак работы
	 *
	 */
	struct timeval timeout{};
	// Устанавливаем целые секунды срока ожидания приёма
	timeout.tv_sec = 0;
	// Устанавливаем долю секунды срока ожидания приёма
	timeout.tv_usec = 50000;
	// Выполняем установку срока ожидания приёма
	::setsockopt(this->_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	// Выставляем признак работы нити сервера
	this->_running.store(true);
	// Выполняем запуск нити приёма вопросов
	this->_thread = std::thread(&NTPStubServer::run, this);
	// Выводим положительный результат запуска сервера
	return true;
}

/**
 * @brief Метод остановки сервера
 *
 */
void NTPStubServer::stop() noexcept {
	// Если сервер уже остановлен, выходим
	if(!this->_running.load() && !this->_thread.joinable())
		// Выходим из функции
		return;
	// Снимаем признак работы нити сервера
	this->_running.store(false);
	/**
	 * Сокет закрывается уже после завершения нити
	 *
	 * @note Довод тот же, что и у сервера DNS выше
	 *
	 */
	if(this->_thread.joinable())
		// Выполняем ожидание завершения нити сервера
		this->_thread.join();
	// Если сокет заведён, выполняем его закрытие
	if(this->_fd >= 0){
		// Выполняем закрытие сокета
		::close(this->_fd);
		// Сбрасываем дескриптор сокета
		this->_fd = -1;
	}
}

/**
 * @brief Метод приёма вопросов и выдачи ответов
 *
 */
void NTPStubServer::run() noexcept {
	// Принимаемое сообщение
	uint8_t buffer[128];
	// Адрес, с которого пришёл вопрос
	struct sockaddr_in from{};
	/**
	 * Выполняем приём вопросов, пока сервер работает
	 */
	while(this->_running.load()){
		// Размер адреса, с которого пришёл вопрос
		socklen_t length = sizeof(from);
		// Выполняем приём вопроса
		const ssize_t size = ::recvfrom(this->_fd, buffer, sizeof(buffer), 0, reinterpret_cast <struct sockaddr *> (&from), &length);
		// Если принято меньше целого сообщения, продолжаем работу
		if(size < static_cast <ssize_t> (NTP_PACKET_SIZE))
			// Переходим к следующему кругу приёма
			continue;
		// Выполняем подсчёт принятых вопросов
		this->_received.fetch_add(1);
		// Собираемый ответ
		uint8_t answer[NTP_PACKET_SIZE] = {0};
		/**
		 * Выставляем признаки ответа
		 *
		 * @note Старшие разряды несут версию протокола, младшие - режим, и режим
		 *       серверного ответа договором задан четвёртым
		 */
		answer[0] = 0x24;
		// Выставляем уровень страты часов сервера
		answer[1] = 0x02;
		/**
		 * Переписываем в ответ метку отправки из вопроса
		 *
		 * @note Метка эта и есть примета своего ответа: клиент сличает её с той, что
		 *       ставил в вопрос. Не перепиши её сервер, и ответ клиенту чужой
		 */
		if(!this->_foreign){
			/**
			 * Выполняем перенос метки отправки вопроса в исходную метку ответа
			 */
			for(size_t i = 0; i < 8; i++)
				// Переносим очередной октет метки
				answer[NTP_ORIGINATE_OFFSET + i] = buffer[NTP_TRANSMIT_OFFSET + i];
		}
		/**
		 * Выставляем метку отправки ответа
		 *
		 * @note Отсчёт времени договором ведётся от тысяча девятисотого года, и
		 *       клиент отвергает метки, лежащие раньше эпохи Unix. Берётся заведомо
		 *       большее значение: проверяется не само время, а обхождение с ответом
		 */
		answer[NTP_TRANSMIT_OFFSET + 0] = 0xE8;
		answer[NTP_TRANSMIT_OFFSET + 1] = 0x00;
		answer[NTP_TRANSMIT_OFFSET + 2] = 0x00;
		answer[NTP_TRANSMIT_OFFSET + 3] = 0x00;
		// Выполняем отправку ответа
		::sendto(this->_fd, answer, sizeof(answer), 0, reinterpret_cast <struct sockaddr *> (&from), length);
	}
}
