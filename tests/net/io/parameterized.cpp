/**
 * @file: parameterized.cpp
 * @date: 2025-12-17
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Стандартные модули
 */
#include <random>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "io.hpp"
#include "../../include/net/addr.hpp"
#include "../../include/sys/chrono.hpp"

/**
 * @brief Параметры теста выполнения пингования
 *
 */
struct IoPingTestParameter {
	// Сетевой адрес с которого будет выполняться работа
	std::string source = "";
	// Сетевой адрес который будет пинговаться
	std::string target = "";
};

/**
 * @brief Класс параметризованной тестовой фикстуры
 *
 */
class IoPingParameterizedFixture : public IoFixture, public ::testing::WithParamInterface <IoPingTestParameter> {
	public:
		/**
		 * @brief Структура заголовков ICMP
		 *
		 */
		struct IcmpHeader {
			uint8_t type;      // Тип запроса
			uint8_t code;      // Код запроса
			uint16_t checksum; // Контрольная сумма
			/**
			 * Объединение структур запроса
			 */
			union {
				/**
				 * @brief Структура отправляемого запроса
				 *
				 */
				struct {
					uint16_t identifier = 0; // Идентификатор запроса
					uint16_t sequence   = 0; // Номер последовательности
					uint64_t payload    = 0; // Тело полезной нагрузки
				} echo;
				/**
				 * @brief Структура указателя запроса
				 *
				 */
				struct ICMP_PACKET_POINTER_HEADER {
					// Указатель пакета
					uint8_t pointer = 0;
				} pointer;
				/**
				 * @brief Структура адреса ответа
				 *
				 */
				struct ICMP_PACKET_REDIRECT_HEADER {
					// Адрес ответа IPv4
					uint32_t gatewayAddress = 0;
				} redirect;
				/**
				 * @brief Структура адреса ответа
				 *
				 */
				struct ICMP6_PACKET_REDIRECT_HEADER {
					// Адрес ответа IPv6
					uint32_t gatewayAddress[4] = {0,0,0,0};
				} redirect6;
			} meta;
		};
	public:
		/**
		 * @brief Функция подсчёта контрольной суммы
		 *
		 * @param buffer буфер данных для подсчёта
		 * @param size   размер данных для подсчёта
		 * @return       подсчитанная контрольная сумма
		 */
		static uint16_t checksum(const void * buffer, const size_t size) noexcept {
			// Результат работы функции
			uint16_t result = 0;
			// Если данные переданы верные
			if((buffer != nullptr) && (size > 0)){
				// Контрольная сумма расчёта
				uint32_t sum = 0;
				// Устанавливаем длину контрольной суммы
				size_t length = size;
				// Выполняем приведение буфера в нужную нам форму
				auto data = reinterpret_cast <const uint16_t *> (buffer);
				// Если длина буфера всего один байт
				if(length & 1)
					// Выполняем расчёт контрольной суммы
					sum = reinterpret_cast <const uint8_t *> (data)[length - 1];
				// Делим длину байт пополам
				length /= 2;
				/**
				 *  Выполняем перебор буфера байт
				 */
				while(length--){
					// Выполняем расчёт контрольной суммы
					sum += * data++;
					// Если контрольная сумма достигла предела
					if(sum & 0xffff0000)
						// Выполняем смещение на оставшиеся 16 байт
						sum = ((sum >> 16) + (sum & 0xffff));
				}
				// Выполняем получение результата контрольной суммы
				result = static_cast <uint16_t> (~sum);
			}
			// Выводим результат
			return result;
		}
	public:
		// Параметры теста
		IoPingTestParameter _parameter = GetParam();
	public:
		// Создаём объект работы с датами
		std::unique_ptr <awh::chrono_t> chrono = std::make_unique <awh::chrono_t> (this->_fmk.get(), this->_log.get());
		// Создаём объект работы с IP-адресами
		std::unique_ptr <awh::net_addr_t> addr = std::make_unique <awh::net_addr_t> (this->_fmk.get(), this->_log.get());
};

/**
 * @brief Тест параметризованного выполнения работы пингования
 *
 */
TEST_P(IoPingParameterizedFixture, IoPingTest){
	// Идентификатор события
	awh::event::id_t eid = 0;
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Добавляем новое событие клиента ICMP
		eid = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::RAW, awh::event::protocol_t::ICMP);
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		// Если пользователь является привилигированным
		if(::getuid())
			// Добавляем новое событие клиента ICMP
			eid = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::DATAGRAM, awh::event::protocol_t::ICMP);
		// Добавляем новое событие клиента ICMP
		else eid = this->_io->event(awh::event::node_t::CLIENT, awh::event::family_t::IPV4, awh::event::type_t::RAW, awh::event::protocol_t::ICMP);
	#endif
	// Проверяем, что идентификатор события больше нуля
	ASSERT_GT(eid, 0);
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Устанавливаем опции событий
	ASSERT_TRUE(this->_io->options(eid, awh::event::options::NOSIGILL | awh::event::options::NOSIGPIPE | awh::event::options::REUSEADDR));
	// Устанавливаем IP-адрес события
	ASSERT_TRUE(this->_io->address(eid, awh::event::address_t::IPV4, this->_parameter.source));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->target(eid, this->_parameter.target));
	// Устанавливаем функцию обратного вызова на запись в событие
	this->_io->on(eid, static_cast <awh::event::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
		// Выводим сообщение о переподключении события
		this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
	}));
	// Устанавливаем функцию обратного вызова на чтение из события
	this->_io->on(eid, [this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
		// Результат полученных данных
		auto icmpResponseHeader = reinterpret_cast <const struct IoPingParameterizedFixture::IcmpHeader *> (data);
		// Добавляем полученный IP-адрес
		this->addr->v4(icmpResponseHeader->meta.redirect.gatewayAddress);
		// Выводим сообщение о переподключении события
		this->_log->print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, size, static_cast <std::string> (* this->addr.get()).c_str());
	});
	// Устанавливаем функцию обратного вызова на ошибку события
	this->_io->on(eid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
		/**
		 * Обрабатываем статус события
		 */
		switch(static_cast <uint8_t> (error)){
			// Если ошибка неизвестного события
			case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
				// Выводим сообщение об ошибке неизвестного события
				this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка недопустимой операции
			case static_cast <uint8_t> (awh::event::error_t::INVALID):
				// Выводим сообщение об ошибке недопустимой операции
				this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка доступа запрещёния
			case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
				// Выводим сообщение об ошибке доступа запрещёния
				this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка уже существующего объекта
			case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
				// Выводим сообщение об ошибке уже существующего объекта
				this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка доступа к сокету
			case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
				// Выводим сообщение об ошибке доступа к сокету
				this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка некорректного адреса
			case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
				// Выводим сообщение об ошибке некорректного адреса
				this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка ошибки подключения
			case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
				// Выводим сообщение об ошибке подключения
				this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка недостаточно ресурсов
			case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
				// Выводим сообщение об ошибке недостаточно ресурсов
				this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка события
			case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
				// Выводим сообщение об ошибке события
				this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если объект не найден
			case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
				// Выводим сообщение об ошибке события
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
				// Выводим сообщение о чтении события
				this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является записью
			case static_cast <uint8_t> (awh::event::action_t::WRITE):
				// Выводим сообщение о записи события
				this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является подключением
			case static_cast <uint8_t> (awh::event::action_t::CONNECT):
				// Выводим сообщение о подключении события
				this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является отключением
			case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
				// Выводим сообщение об отключении события
				this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является переподключением
			case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
				// Выводим сообщение о переподключении события
				this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является закрытием
			case static_cast <uint8_t> (awh::event::action_t::CLOSE):
				// Выводим сообщение о закрытии события
				this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением
			case static_cast <uint8_t> (awh::event::action_t::CHANGE):
				// Выводим сообщение об изменении события
				this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является удалением
			case static_cast <uint8_t> (awh::event::action_t::DELETE):
				// Выводим сообщение об удалении события
				this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является переименованием
			case static_cast <uint8_t> (awh::event::action_t::RENAME):
				// Выводим сообщение о переименовании события
				this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением атрибутов
			case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
				// Выводим сообщение об изменении атрибутов события
				this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является отзывом доступа
			case static_cast <uint8_t> (awh::event::action_t::REVOKE):
				// Выводим сообщение об отзыве доступа события
				this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением счётчика жёстких ссылок
			case static_cast <uint8_t> (awh::event::action_t::HDLINK):
				// Выводим сообщение о изменении счётчика жёстких ссылок события
				this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
		}
	});
	// Устанавливаем таймаут события на запись
	this->_io->timeout(eid, awh::event::action_t::WRITE, 3000);
	// Устанавливаем таймаут события на подключение
	this->_io->timeout(eid, awh::event::action_t::CONNECT, 5000);
	// Выполняем фиксацию настроек события сервера
	ASSERT_TRUE(this->_io->commit(eid));
	// Выполняем пинг 3 раза 
	for(uint8_t i = 0; i < 3; i++){
		// Выполняем инициализацию генератора
		std::random_device randev;
		// Подключаем устройство генератора
		std::mt19937 generator(randev());
		// Выполняем генерирование случайного числа
		std::uniform_int_distribution <std::mt19937::result_type> dist6(0, std::numeric_limits <uint32_t>::max() - 1);
		// Создаём объект заголовков
		IoPingParameterizedFixture::IcmpHeader icmp{};
		// Выполняем установку типа запроса
		icmp.type = 8; // IPv4
		// icmp.type = 128; // IPv6
		// Устанавливаем код запроса
		icmp.code = 0;
		// Устанавливаем контрольную сумму
		icmp.checksum = 0;
		// Устанавливаем номер последовательности
		icmp.meta.echo.sequence = 1;
		// Устанавливаем идентификатор запроса
		icmp.meta.echo.identifier = ::getpid();
		// Устанавливаем данные полезной нагрузки
		icmp.meta.echo.payload = static_cast <uint64_t> (dist6(generator));
		// Выполняем подсчёт контрольной суммы
		icmp.checksum = IoPingParameterizedFixture::checksum(&icmp, sizeof(icmp));
		// Запоминаем текущее значение времени в миллисекундах
		const uint64_t mseconds = this->chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS);
		// Отправляем сообщение серверу
		ASSERT_TRUE(this->_io->send(eid, reinterpret_cast <char *> (&icmp), sizeof(icmp)));
		// Выполняем чтение ответа
		ASSERT_TRUE(this->_io->recv(eid));
		// Выполняем подсчёт количество прошедшего времени
		ASSERT_GT(this->chrono->timestamp(awh::chrono_t::type_t::MILLISECONDS) - mseconds, 0);
	}
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Инициализация параметров теста
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, IoPingParameterizedFixture,
	::testing::Values(
		IoPingTestParameter({
			"0.0.0.0",
			"5.255.255.242"
		}),
		IoPingTestParameter({
			"0.0.0.0",
			"89.169.31.66"
		}),
		IoPingTestParameter({
			"0.0.0.0",
			"142.250.150.101"
		})
	)
);

/**
 * @brief Параметры теста временного таймера
 *
 */
struct IoTimerTestParameter {
	// Таймаут ожидания в миллисекундах
	uint32_t timeout = 0;
	// Тип узла события
	awh::event::node_t node = awh::event::node_t::NONE;
};

/**
 * @brief Класс параметризованной тестовой фикстуры
 *
 */
class IoTimerParameterizedFixture : public IoFixture, public ::testing::WithParamInterface <IoTimerTestParameter> {
	public:
		// Параметры теста
		IoTimerTestParameter _parameter = GetParam();
	public:
		// Замеряем время начала работы для таймера
		std::chrono::time_point <std::chrono::system_clock> ts;
};

/**
 * @brief Тест параметризованного выполнения работы таймера
 *
 */
TEST_P(IoTimerParameterizedFixture, IoTimerTest){
	// Флаг остановки теста
	bool stop = false;
	// Добавляем новое событие таймера
	awh::event::id_t eid = this->_io->event(this->_parameter.node, awh::event::family_t::TIMER);
	// Проверяем, что идентификатор события больше нуля
	ASSERT_GT(eid, 0);
	// Добавляем новое событие таймера
	this->_io->timeout(eid, awh::event::action_t::NONE, this->_parameter.timeout);
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Выполняем фиксацию настроек события сервера
	ASSERT_TRUE(this->_io->commit(eid));
	// Запоминаем текущее значение времени
	this->ts = std::chrono::system_clock::now();
	// Если таймер является интервалом
	if(this->_parameter.node == awh::event::node_t::INTERVAL){
		// Количество срабатываний интервала
		uint8_t count = 0;
		// Устанавливаем функцию обратного вызова на событие интервала
		this->_io->on(eid, [&count, &stop, this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			// Замеряем время начала работы для интервала времени
			auto shift = std::chrono::system_clock::now();
			// Если статус события успешен
			if(status == awh::event::status_t::SUCCESS){
				// Выводим сообщение о срабатывании интервала
				this->_log->print("Интервал сработал: ID=%u, %u seconds", awh::log_t::flag_t::INFO, eid, std::chrono::duration_cast <std::chrono::seconds> (shift - ts).count());
				// Замеряем время начала работы для интервала времени
				this->ts = std::move(shift);
				// Если таймер отработал 10 раз, выходим
				if((count++) >= 3)
					// Останавливаем тест
					stop = true;
			}
		});
	// Если таймер является таймаутом
	} else {
		// Устанавливаем функцию обратного вызова на событие таймера
		this->_io->on(eid, [&stop, this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			// Замеряем время начала работы для интервала времени
			auto shift = std::chrono::system_clock::now();
			// Если статус события успешен
			if(status == awh::event::status_t::SUCCESS)
				// Выводим сообщение о срабатывании таймера
				this->_log->print("Таймер сработал: ID=%u, %u seconds", awh::log_t::flag_t::INFO, eid, std::chrono::duration_cast <std::chrono::seconds> (shift - this->ts).count());
			// Останавливаем тест
			stop = true;
		});
	}
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
}

/**
 * @brief Инициализация параметров теста
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, IoTimerParameterizedFixture,
	::testing::Values(
		IoTimerTestParameter({
			5000,
			awh::event::node_t::INTERVAL
		}),
		IoTimerTestParameter({
			12000,
			awh::event::node_t::TIMEOUT
		})
	)
);

