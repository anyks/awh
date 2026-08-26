/**
 * @file parameterized.cpp
 * @date 2025-12-17
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
 * @brief Параметризованные тесты асинхронного движка ввода-вывода —
 *        прогон подготовленных наборов входных данных через методы модуля с проверкой регистрации событий,
 *        управления подписками, работы таймеров и корректной остановки цикла событий
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Подключаем стандартные модули
 */
#include <random>
#include <thread>
#include <chrono>
#include <cstring>
/**
 * Для операционной системы MS Windows
 *
 * @note Заголовки эти принадлежат POSIX и у MS Windows отсутствуют:
 *       соответствующие им объявления приходят там из winsock2.h,
 *       подключаемого через единую точку sys/win32.hpp
 *
 */
#if _WIN32 || _WIN64
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/win32.hpp>
/**
 * Для всех остальных операционных систем
 */
#else
	/**
	 * Системные заголовочные файлы
	 *
	 * @note Заголовок netinet/in.h подключается ЯВНО ради IPPROTO_ICMP: у macOS и
	 *       Linux объявление приходит попутно через прочие заголовки, а у FreeBSD -
	 *       нет, и проба доступности сокета там не собирается
	 */
	#include <arpa/inet.h>
	#include <netinet/in.h>
	/**
	 * @note Ожидание дочернего процесса и разбор его состояния нужны проверке обмена
	 *       между процессами: родитель обязан дождаться работника, а не выждать срок
	 */
	#include <sys/wait.h>
#endif

/**
 * Подключаем восполнение средств POSIX, отсутствующих у MS Windows
 */
#include "../../posix.hpp"

/**
 * Подключаем заголовочный файлы проекта
 */
#include "io.hpp"
#include "../../include/net/addr.hpp"
#include "../../include/sys/chrono.hpp"

/**
 * Снимаем макросы MS Windows, сталкивающиеся с именами членов перечислений AWH
 *
 * @details Проверка эта пишет «awh::event::error_t::INVALID_SOCKET», а MS Windows
 *          заводит имя INVALID_SOCKET макросом. Заголовки AWH защищают **свои**
 *          объявления парой macro_push.hpp и macro_pop.hpp, но возвращают макросы
 *          следом - тем они и оставляют их тому, кто ими пользуется по делу. Потому
 *          всякий, кто называет такие члены в **своём** коде, защищает свой файл
 *          той же парой, и проверка эта не исключение
 *
 * @note Снятие идёт после всех подключений и снимается в конце файла: макросы эти
 *       нужны самим заголовкам MS Windows, и снимать их прежде подключения нельзя
 *
 */
#include <sys/macro_push.hpp>


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
		 *
		 */
		static uint16_t checksum(const void * buffer, const size_t size) noexcept {
			// Переменная результата
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
			// Возвращаем результат
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
 * @note Права суперпользователя нужны НЕ везде: сырой сокет требует их всюду, но
 *       непривилегированному пользователю доступен дейтаграммный сокет ICMP - у
 *       macOS, Linux и MS Windows он работает и без прав. Системы, где такого
 *       сокета нет вовсе, отсеиваются ниже пропуском, а не отказом
 *
 */
TEST_P(IoPingParameterizedFixture, IoPingTest){
	/**
	 * Если непривилегированному пользователю дейтаграммный сокет ICMP недоступен
	 *
	 * @warning Судить о доступности по списку систем НЕЛЬЗЯ: это свойство НАСТРОЙКИ,
	 *          а не системы. Замерено: у macOS сокет доступен, у FreeBSD и illumos
	 *          его нет вовсе (`Protocol not supported`), а у Linux он зависит от
	 *          `net.ipv4.ping_group_range` - на стенде Debian там `1 0`, пустой
	 *          диапазон, и сокет запрещён, тогда как у другой машины с тем же ядром
	 *          он открывается. Поэтому доступность именно ПРОБУЕТСЯ
	 *
	 * @note Отказ здесь означал бы вину кода, тогда как дело в правах, поэтому
	 *       пропускаем с указанием причины, а не проваливаем
	 */
	#if !(_WIN32 || _WIN64)
		// Если пользователь является непривилигированным
		if(::getuid()){
			// Выполняем пробу создания дейтаграммного сокета ICMP
			const int32_t probe = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
			// Если сокет создать не удалось
			if(probe < 0)
				// Пропускаем тест с указанием причины
				GTEST_SKIP() << "Дейтаграммный сокет ICMP недоступен без прав суперпользователя: " << ::strerror(errno);
			// Выполняем закрытие пробного сокета
			::close(probe);
		}
	#endif
	// Идентификатор события
	awh::event::id_t eid = 0;
	// Количество принятых откликов на эхо-запрос
	uint8_t replies = 0;
	// Опознаватель, которым метятся наши запросы
	const uint16_t identifier = htons(static_cast <uint16_t> (::getpid() & 0xFFFF));
	// Номер последовательности, отклика на который мы ждём
	uint16_t expected = 0;
	/**
	 * Признак сырого сокета
	 *
	 * @note Различие существенно для разбора отклика: сырой сокет отдаёт пакет
	 *       ВМЕСТЕ с заголовком IP, а дейтаграммный сокет ICMP - с заголовка ICMP.
	 *       Наложить структуру ICMP на заголовок IP значит разобрать чужие байты
	 */
	#if _WIN32 || _WIN64
		const bool raw = true;
	#else
		const bool raw = (::getuid() == 0);
	#endif
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
		// Если пользователь является непривилигированным
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
	ASSERT_TRUE(this->_io->setOptions(eid, awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::REUSE_ADDR));
	// Устанавливаем IP-адрес события
	ASSERT_TRUE(this->_io->setAddress(eid, awh::event::address_t::IPV4, this->_parameter.source));
	// Устанавливаем адрес сервера назначения
	ASSERT_TRUE(this->_io->setTarget(eid, this->_parameter.target));
	// Устанавливаем функцию обратного вызова на запись в событие
	this->_io->on(eid, static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
		// Записываем в лог сообщение о переподключении события
		this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
	}));
	// Устанавливаем функцию обратного вызова на чтение из события
	this->_io->on(eid, [this, raw, identifier, &expected, &replies](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
		// Если принятого не хватает даже на заголовок
		if((data == nullptr) || (size == 0))
			// Выходим из разбора отклика
			return;
		// Смещение заголовка ICMP от начала принятого
		size_t offset = 0;
		/**
		 * Если отклик пришёл вместе с заголовком IP
		 *
		 * @note Длина заголовка IP берётся из младшей половины первого октета в
		 *       четырёхоктетных словах и постоянной величиной НЕ является: заголовок
		 *       вправе нести дополнения
		 *
		 * @warning Судить о наличии заголовка IP по разновидности сокета НЕЛЬЗЯ:
		 *          системы расходятся. Сырой сокет отдаёт заголовок везде, а вот
		 *          дейтаграммный ICMP у macOS отдаёт его тоже, тогда как у Linux -
		 *          нет. Поэтому смотрим сами октеты: версия 4 в старшей половине
		 *          первого октета у отклика ICMP встретиться не может, там лежит
		 *          тип, а типы 64-79 не назначены
		 */
		if((size >= 20) && ((data[0] >> 4) == 4) && ((static_cast <size_t> (data[0] & 0x0F) * 4) <= size))
			// Получаем смещение заголовка ICMP от начала принятого
			offset = (static_cast <size_t> (data[0] & 0x0F) * 4);
		// Если принятого не хватает на заголовок ICMP
		if((offset + 4) > size)
			// Выходим из разбора отклика
			return;
		// Получаем заголовок ICMP принятого отклика
		auto icmp = reinterpret_cast <const struct IoPingParameterizedFixture::IcmpHeader *> (data + offset);
		// Записываем в лог сведения о принятом отклике
		this->_log->print(
			"Прочитано: ID=%u, %zu байт, заголовок IP %zu байт, тип %u, код %u",
			awh::log_t::flag_t::INFO, eid, size, offset,
			static_cast <uint32_t> (icmp->type), static_cast <uint32_t> (icmp->code)
		);
		/**
		 * Если принят отклик на эхо-запрос
		 *
		 * @note Тип 0 - это именно отклик на эхо-запрос. Прочие типы приходят на тот
		 *       же сырой сокет от чужого обмена, и засчитывать их за наш отклик нельзя
		 */
		if(icmp->type != 0)
			// Выходим из разбора отклика
			return;
		// Если принятого не хватает на тело эхо-запроса
		if((offset + sizeof(struct IoPingParameterizedFixture::IcmpHeader)) > size)
			// Выходим из разбора отклика
			return;
		/**
		 * Если отклик пришёл не на наш запрос
		 *
		 * @note Опознаватель сличается ТОЛЬКО у сырого сокета: дейтаграммный сокет
		 *       ICMP подменяет его своими силами - ядро проставляет туда собственную
		 *       величину, отчего сличение с нашей было бы заведомо ложным
		 */
		if(raw && (icmp->meta.echo.identifier != identifier))
			// Выходим из разбора отклика
			return;
		// Если отклик пришёл не на текущий запрос
		if(icmp->meta.echo.sequence != htons(expected))
			// Выходим из разбора отклика
			return;
		// Увеличиваем количество принятых откликов на эхо-запрос
		replies++;
	});
	// Устанавливаем функцию обратного вызова на ошибку события
	this->_io->on(eid, [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
		/**
		 * Обрабатываем статус события
		 */
		switch(static_cast <uint8_t> (error)){
			// Если ошибка неизвестного события
			case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
				// Записываем ошибку в лог неизвестного события
				this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка недопустимой операции
			case static_cast <uint8_t> (awh::event::error_t::INVALID):
				// Записываем ошибку в лог недопустимой операции
				this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка доступа запрещёния
			case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
				// Записываем ошибку в лог доступа запрещёния
				this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка уже существующего объекта
			case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
				// Записываем ошибку в лог уже существующего объекта
				this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка доступа к сокету
			case static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET):
				// Записываем ошибку в лог доступа к сокету
				this->_log->print("Ошибка доступа к сокету события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка некорректного адреса
			case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
				// Записываем ошибку в лог некорректного адреса
				this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка ошибки подключения
			case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
				// Записываем ошибку в лог подключения
				this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка недостаточно ресурсов
			case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
				// Записываем ошибку в лог недостаточно ресурсов
				this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если ошибка события
			case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
				// Записываем ошибку в лог события
				this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
			break;
			// Если объект не найден
			case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
				// Записываем ошибку в лог события
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
				// Записываем в лог сообщение о чтении события
				this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является записью
			case static_cast <uint8_t> (awh::event::action_t::WRITE):
				// Записываем в лог сообщение о записи события
				this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является подключением
			case static_cast <uint8_t> (awh::event::action_t::CONNECT):
				// Записываем в лог сообщение о подключении события
				this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является отключением
			case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
				// Записываем в лог сообщение об отключении события
				this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является переподключением
			case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
				// Записываем в лог сообщение о переподключении события
				this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является закрытием
			case static_cast <uint8_t> (awh::event::action_t::CLOSE):
				// Записываем в лог сообщение о закрытии события
				this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением
			case static_cast <uint8_t> (awh::event::action_t::CHANGE):
				// Записываем в лог сообщение об изменении события
				this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является удалением
			case static_cast <uint8_t> (awh::event::action_t::DELETE):
				// Записываем в лог сообщение об удалении события
				this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является переименованием
			case static_cast <uint8_t> (awh::event::action_t::RENAME):
				// Записываем в лог сообщение о переименовании события
				this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением атрибутов
			case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
				// Записываем в лог сообщение об изменении атрибутов события
				this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является отзывом доступа
			case static_cast <uint8_t> (awh::event::action_t::REVOKE):
				// Записываем в лог сообщение об отзыве доступа события
				this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
			// Если действие является изменением счётчика жёстких ссылок
			case static_cast <uint8_t> (awh::event::action_t::HDLINK):
				// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
				this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
			break;
		}
	});
	// Устанавливаем таймаут события на запись
	this->_io->setTimeout(eid, awh::event::action_t::WRITE, 3000);
	/**
	 * Срок на чтение обязателен: сокет блокирующий, а читаем мы его сами
	 *
	 * @details Отклика на эхо-запрос может не быть вовсе - узел вправе молчать, а
	 *          заслон вправе его отбросить. Без срока обращение к чтению у такого
	 *          сокета не возвращается никогда, и проверка не отказывает, а висит
	 *
	 * @warning Установлено дважды: на стенде Solaris (13.08.2026) прогон отнимал
	 *          2 часа 45 минут, и то же повторилось на Windows ARM64 (20.08.2026) -
	 *          85 минут до снятия процесса вручную. Прежде отказ этот приписывали
	 *          запуску события, а запуск лишь открывал к нему дорогу: без него
	 *          проверка падала раньше, на отправке, и до чтения не доходила вовсе
	 *
	 * @note Величина взята вчетверо больше срока на запись: обмен ICMP укладывается
	 *       в сорок миллисекунд на запрос, и двенадцати секунд хватает с запасом
	 *       даже медленному стенду
	 */
	this->_io->setTimeout(eid, awh::event::action_t::READ, 12000);
	// Устанавливаем таймаут события на подключение
	this->_io->setTimeout(eid, awh::event::action_t::CONNECT, 5000);
	// Выполняем фиксацию настроек события сервера
	ASSERT_TRUE(this->_io->commit(eid));
	/**
	 * Запускаем событие вслед за фиксацией, как это делает настоящий потребитель
	 *
	 * @details Порядок этот задан юнитом ICMP: там `commit` и `launch` стоят одним
	 *          выражением (`icmp.cpp`), и всякий иной порядок испытывает движок в
	 *          состоянии, в каком его никто не использует. Запуск переводит узел из
	 *          состояния заведения в состояние выполнения
	 *
	 * @warning Прежде запуск здесь НЕ выполнялся, и это скрывало дефект движка: у
	 *          MS Windows разбор отправки требовал состояния выполнения, тогда как у
	 *          прочих движков его переделали на «узел не удалён». Проверка испытывала
	 *          узел в состоянии заведения - у трёх движков это проходило, у четвёртого
	 *          отвечало отказом «клиент ещё не готов к отправке данных»
	 *
	 * @warning Прежнее пояснение запрещало запуск доводом замера на стенде Solaris
	 *          (13.08.2026): с запуском проверка отнимала 2 часа 45 минут при трёх
	 *          запросах против сорока миллисекунд на запрос у самого обмена. Довод
	 *          этот перепроверен, потому что запрещал он верный порядок вызова
	 *
	 * @note Цикл опроса проверка по-прежнему НЕ запускает: обмен ниже идёт вручную,
	 *       `send` и `recv` зовутся прямо. Запуск события и запуск цикла - разные
	 *       вещи, и прежнее пояснение их смешивало
	 */
	ASSERT_TRUE(this->_io->launch(eid));
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
	// Последовательность
	uint16_t sequence = 0;
	/**
	 * Выполняем пинг 3 раза 
	 */
	for(uint8_t i = 0; i < 3; i++){
		// Запоминаем номер последовательности, отклика на который ждём
		expected = sequence;
		// Устанавливаем номер последовательности
		icmp.meta.echo.sequence = htons(sequence);
		// Устанавливаем идентификатор запроса
		icmp.meta.echo.identifier = identifier;
		// Устанавливаем данные полезной нагрузки
		icmp.meta.echo.payload = static_cast <uint64_t> (dist6(generator));
		// Обнуляем структуру (ОЧЕНЬ ВАЖНО ТАК-КАК РАСЧЁТ КОНТРОЛЬНОЙ СУММЫ НАЧИНАЕТСЯ С НУЛЯ!!!)
		icmp.checksum = 0;
		// Выполняем подсчёт контрольной суммы
		icmp.checksum = IoPingParameterizedFixture::checksum(&icmp, sizeof(icmp));
		// Отправляем сообщение серверу
		ASSERT_TRUE(this->_io->send(eid, reinterpret_cast <char *> (&icmp), sizeof(icmp)));
		/**
		 * Выполняем чтение ответа
		 *
		 * @warning Утверждать, что чтение заняло не меньше миллисекунды, НЕЛЬЗЯ: время
		 *          это ничего об обмене не говорит. Отклик приходит обработчиком, а
		 *          само обращение управление возвращает сразу - у Solaris проверка на
		 *          этом и валилась, тогда как отклики приходили исправно. У Linux то
		 *          же утверждение проходило по случайности. Работу обмена доказывает
		 *          счёт разобранных откликов ниже, а не часы
		 */
		ASSERT_TRUE(this->_io->recv(eid));
		// Увеличиваем последовательность запроса
		sequence++;
	}
	// Уничтожаем все события после получения ответа
	ASSERT_TRUE(this->_io->deinitialize());
	/**
	 * Проверяем, что отклики на эхо-запросы приняты и разобраны
	 *
	 * @warning Утверждение это - единственное, что отличает проверку работающего
	 *          обмена ICMP от проверки, которая пройдёт при любых принятых байтах.
	 *          Прежде тело её лишь печатало принятое, причём накладывало заголовок
	 *          ICMP прямо на заголовок IP и выдавало из него несуществующий адрес
	 *          шлюза, - и прошла бы она даже если бы движок вернул мусор
	 */
	ASSERT_EQ(replies, 3);
}

/**
 * @brief Инициализация параметров теста
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, IoPingParameterizedFixture,
	::testing::Values(
		/**
		 * @note Петля отвечает всегда и связи наружу не требует: на ней проверка
		 *       держится там, где выхода в интернет нет вовсе
		 */
		IoPingTestParameter({
			"0.0.0.0",
			"127.0.0.1"
		}),
		/**
		 * @note Узел внешний, и он проверяет путь до настоящей сети - тот, какого
		 *       петля не проходит вовсе
		 *
		 * @warning Адреса сюда ставить только отвечающие. Прежде их было три, и
		 *          все три оказались негодны: узлы Google из России недоступны по
		 *          запрету и не ответят никогда, а два прочих отзывались через раз -
		 *          проверка падала от потери пакетов, а не от дефекта движка
		 */
		IoPingTestParameter({
			"0.0.0.0",
			"77.88.8.8"
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
	/**
	 * Количество срабатываний интервала. Счётчик объявлен в области видимости теста:
	 * функция обратного вызова захватывает его по ссылке и вызывается уже из цикла
	 * событий, то есть переживает ветку настройки события
	 */
	uint8_t count = 0;
	// Добавляем новое событие таймера
	awh::event::id_t eid = this->_io->event(this->_parameter.node, awh::event::family_t::TIMER);
	// Проверяем, что идентификатор события больше нуля
	ASSERT_GT(eid, 0);
	// Добавляем новое событие таймера
	this->_io->setTimeout(eid, awh::event::action_t::NONE, this->_parameter.timeout);
	// Инициализируем асинхронный движок ввода-вывода
	ASSERT_TRUE(this->_io->initialize());
	// Выполняем фиксацию настроек события сервера
	ASSERT_TRUE(this->_io->commit(eid));
	// Запоминаем текущее значение времени
	this->ts = std::chrono::system_clock::now();
	/**
	 * Запоминаем время начала теста отдельно: обработчик интервала сдвигает метку
	 * времени на каждом срабатывании, а проверить нужно суммарную длительность
	 */
	const auto start = this->ts;
	// Если таймер является интервалом
	if(this->_parameter.node == awh::event::node_t::INTERVAL){
		// Устанавливаем функцию обратного вызова на событие интервала
		this->_io->on(eid, [&count, &stop, this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			// Замеряем время начала работы для интервала времени
			auto shift = std::chrono::system_clock::now();
			// Если статус события успешен
			if(status == awh::event::status_t::SUCCESS){
				// Записываем в лог сообщение о срабатывании интервала
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
		/**
		 * Устанавливаем функцию обратного вызова на событие таймера
		 *
		 * @note Тест останавливается только по успешному статусу. Останавливаться на
		 *       любом статусе нельзя: обработчик получает и служебные уведомления, и
		 *       тест завершался на первом же из них за одну миллисекунду, ни разу не
		 *       дождавшись срабатывания самого таймаута
		 */
		this->_io->on(eid, [&count, &stop, this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
			// Замеряем время начала работы для интервала времени
			auto shift = std::chrono::system_clock::now();
			// Если статус события успешен
			if(status == awh::event::status_t::SUCCESS){
				// Записываем в лог сообщение о срабатывании таймера
				this->_log->print("Таймер сработал: ID=%u, %u seconds", awh::log_t::flag_t::INFO, eid, std::chrono::duration_cast <std::chrono::seconds> (shift - this->ts).count());
				// Увеличиваем количество срабатываний таймаута
				count++;
				// Останавливаем тест
				stop = true;
			}
		});
	}
	// Выполняем запуск события
	ASSERT_TRUE(this->_io->launch(eid));
	/**
	 * Запускаем опрос событий
	 */
	while(!stop && this->_io->poll());
	// Проверяем, что таймер действительно срабатывал, а не просто завершился опрос
	ASSERT_GT(count, 0);
	/**
	 * Проверяем, что таймер отработал заданную задержку, а не сработал немедленно.
	 * Допускаем погрешность планировщика в четверть заданной задержки
	 */
	ASSERT_GE(std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::system_clock::now() - start).count(), (this->_parameter.timeout - (this->_parameter.timeout / 4)));
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

/**
 * @brief Параметры теста IPC-сообщений
 *
 */
struct IoIPCTestParameter {
	// Тип события
	awh::event::type_t type = awh::event::type_t::NONE;
	// Семейство события
	awh::event::family_t family = awh::event::family_t::NONE;
};

/**
 * @brief Класс параметризованной тестовой фикстуры
 *
 */
class IoIPCTestParameterizedFixture : public IoFixture, public ::testing::WithParamInterface <IoIPCTestParameter> {
	public:
		// Параметры теста
		IoIPCTestParameter _parameter = GetParam();
};

/**
 * @brief Тест параметризованного выполнения работы IPC-сообщений
 *
 */
TEST_P(IoIPCTestParameterizedFixture, IoIPCTest){
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		/**
		 * Обмен между процессами у MS Windows работает: он лёг на именованный канал в
		 * строе сообщений, и подтверждён опытом на модуле кластера - мастер и работники
		 * обмениваются сообщениями через настоящие процессы
		 *
		 * Пропуск держится иным: проверка эта строит второй процесс вызовом fork, а
		 * средства этого у MS Windows нет вовсе. Отвечает ему там повторный запуск себя
		 * же с передачей имени канала через окружение - так порождает работников модуль
		 * кластера, - и переписывать проверку надлежит именно на этот лад
		 *
		 * @note Довод пропуска сменился: прежде недоставало самого канала, теперь
		 *       недостаёт лишь переписывания проверки под иной способ порождения
		 *
		 */
		/**
		 * Обмен по сокетам домена UNIX проверить здесь нечем
		 *
		 * @details Из трёх видов сокета домена UNIX у MS Windows есть один потоковый, и
		 *          тот подключается обращением системы, а не наложенным `ConnectEx`:
		 *          домен UNIX его не поддерживает вовсе. Видов же с сохранением границ и
		 *          дейтаграммных там нет как таковых
		 *
		 * @note Пропуск этот - правда о системе, а не о движке, и снять его переписыванием
		 *       проверки нельзя. Проверять под MS Windows остаётся обмен по каналу, он
		 *       ниже и проверяется
		 */
		if(this->_parameter.family != awh::event::family_t::UDS){
			// Флаг остановки проверки
			bool stop = false;
			/**
			 * Заводим пару концов канала обмена
			 *
			 * @note Второй конец нужен лишь ради имени: у MS Windows встреча процессов идёт
			 *       ПО ИМЕНИ канала, а не наследованием описателя, как у систем POSIX. Оттого
			 *       имя снимается с него и уходит работнику окружением, а сам он сносится
			 */
			const auto & channels = this->_io->events(awh::event::family_t::PIPE, awh::event::type_t::SEQPACKET);
			// Проверяем, что идентификаторы событий заведены
			ASSERT_GT(channels[0], 0);
			ASSERT_GT(channels[1], 0);
			// Инициализируем асинхронный движок ввода-вывода
			ASSERT_TRUE(this->_io->initialize());
			// Получаем имя канала обмена, каким работник нас найдёт
			const std::string pipe = this->_io->getTarget(channels[1]);
			// Имя канала обмена обязано быть известно
			ASSERT_FALSE(pipe.empty());
			// Сносим второй конец: работник заведёт свой сам, по имени
			ASSERT_TRUE(this->_io->destroy(channels[1]));
			// Опознаватель процесса, доложившийся работником
			uint32_t reported = 0;
			// Устанавливаем функцию обратного вызова на получение доклада работника
			this->_io->on(channels[0], [&reported, &stop](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
				// Если доклад пришёл целиком
				if((data != nullptr) && (size == sizeof(uint32_t))){
					// Запоминаем опознаватель процесса работника
					::memcpy(&reported, data, sizeof(uint32_t));
					// Отмечаем полученный доклад
					stop = true;
				}
			});
			// Выполняем фиксацию настроек своего конца канала обмена
			ASSERT_TRUE(this->_io->commit(channels[0]));
			// Выполняем запуск своего конца канала обмена
			ASSERT_TRUE(this->_io->launch(channels[0]));
			// Буфер под путь к своему двоичному файлу
			wchar_t executable[MAX_PATH]{0};
			// Получаем путь к своему двоичному файлу: работник это тот же файл
			ASSERT_GT(::GetModuleFileNameW(nullptr, executable, static_cast <DWORD> (sizeof(executable) / sizeof(executable[0]))), 0u);
			// Передаём имя канала обмена порождаемому процессу через окружение
			ASSERT_TRUE(::SetEnvironmentVariableW(L"AWH_IO_IPC_PIPE", this->_fmk->convert(pipe).c_str()));
			// Собираем строку запуска работника: своя проверка отключена именем и зовётся особо
			std::wstring command = L"\"" + std::wstring(executable) + L"\" --gtest_also_run_disabled_tests --gtest_filter=IoFixture.DISABLED_IoIPCWorkerTest";
			// Настройки порождаемого процесса
			STARTUPINFOW startup{};
			// Устанавливаем размер записи настроек
			startup.cb = sizeof(startup);
			// Сведения о порождённом процессе
			PROCESS_INFORMATION process{};
			// Выполняем порождение работника
			const BOOL spawned = ::CreateProcessW(executable, command.data(), nullptr, nullptr, 0 /* FALSE: макрос снят macro_push.hpp ради членов перечислений AWH */, 0, nullptr, nullptr, &startup, &process);
			// Работник обязан быть порождён
			ASSERT_TRUE(spawned) << "Дочерний процесс создать не удалось";
			// Отсчёт времени ожидания доклада
			const auto start = std::chrono::steady_clock::now();
			/**
			 * Крутим опрос, покуда работник не доложится
			 */
			while(!stop && (std::chrono::duration_cast <std::chrono::seconds> (std::chrono::steady_clock::now() - start).count() < 20))
				// Выполняем оборот опроса
				ASSERT_TRUE(this->_io->poll(100));
			// Дожидаемся завершения работника
			::WaitForSingleObject(process.hProcess, 5000);
			// Закрываем описатели порождённого процесса
			::CloseHandle(process.hThread);
			::CloseHandle(process.hProcess);
			// Доклад работника обязан прийти
			ASSERT_TRUE(stop) << "работник не доложился за отведённый срок";
			/**
			 * Опознаватель работника обязан отличаться от своего
			 *
			 * @note Проверка эта не придирка: без неё доклад, пришедший от самого себя,
			 *       выглядел бы межпроцессным обменом, каким он не является
			 */
			ASSERT_NE(reported, static_cast <uint32_t> (::GetCurrentProcessId()));
			ASSERT_GT(reported, 0u);
			// Сносим свой конец канала обмена
			ASSERT_TRUE(this->_io->destroy(channels[0]));
			// Сворачиваем движок
			ASSERT_TRUE(this->_io->deinitialize());
			// Оканчиваем проверку
			return;
		}
		GTEST_SKIP() << "MS Windows has no datagram or seqpacket UNIX domain sockets, and its stream one does not take overlapped connect";
	#else
	// Флаг остановки теста
	bool stop = false;
	/**
	 * Обрабатываем семейство события
	 */
	switch(static_cast <uint8_t> (this->_parameter.family)){
		// Если семейство события является каналом
		case static_cast <uint8_t> (awh::event::family_t::PIPE): {
			// Добавляем новое события межпроцессного взаимодействия для родительского процесса
			const auto & mfds = this->_io->events(this->_parameter.family);
			// Добавляем новое события межпроцессного взаимодействия для дочернего процесса
			const auto & cfds = this->_io->events(this->_parameter.family);
			// Проверяем, что идентификаторы событий больше нуля
			ASSERT_GT(mfds[0], 0);
			ASSERT_GT(mfds[1], 0);
			ASSERT_GT(cfds[0], 0);
			ASSERT_GT(cfds[1], 0);
			// Инициализируем асинхронный движок ввода-вывода
			ASSERT_TRUE(this->_io->initialize());
			// Получаем идентификатор родительского процесса
			const pid_t mpid = ::getpid();
			// Устанавливаем идентификатор процесса
			pid_t pid = -1;
			/**
			 * Определяем тип потока
			 */
			switch((pid = ::fork())){
				// Если поток не создан
				case -1: {
					// Записываем в лог сообщение
					this->_log->print("Child process could not be created", awh::log_t::flag_t::CRITICAL);
					// Выходим из приложения
					::exit(EXIT_FAILURE);
				} break;
				// Если процесс является дочерним
				case 0: {
					/**
					 * Сторож завершения дочернего процесса
					 *
					 * @warning Ставится ПЕРВЫМ и устроен разрушителем, а не вызовом в конце
					 *          ветви, намеренно: отказ утверждения `ASSERT_*` выходит из
					 *          ТЕЛА ПРОВЕРКИ, минуя всякий код после себя, и завершение,
					 *          дописанное в конец, его бы не поймало. Разрушитель же
					 *          срабатывает на любом выходе из блока - и на обычном, и на
					 *          принудительном
					 *
					 * @warning Прежде завершения не было ВОВСЕ: дочерний процесс выходил из
					 *          `switch` и доигрывал набор проверок до конца наравне с
					 *          родительским. У четырёх случаев этой проверки это давало
					 *          ШЕСТНАДЦАТЬ процессов, каждый со своими событиями и портами;
					 *          отсюда же и шестнадцать итоговых сводок в выводе набора.
					 *          Столкновения портов, что мы ловили как «плавающие», росли
					 *          отсюда
					 *
					 * @note Завершение идёт через `_exit`, а не `exit`: обработчики выхода
					 *       принадлежат родителю, и через них дочерний процесс печатал бы
					 *       вторую сводку
					 *
					 */
					struct child_guard_t {
						/**
						 * @brief Деструктор
						 *
						 */
						~child_guard_t() noexcept {
							// Завершаем дочерний процесс, отдавая исход утверждений родителю
							::_exit(::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
						}
					} childGuard;
					// Выполняем переинициализацию асинхронного движка ввода-вывода
					ASSERT_TRUE(this->_io->reinitialize());
					// Уничтожаем событие родительского процесса для чтения
					ASSERT_TRUE(this->_io->destroy(mfds[0]));
					// Уничтожаем событие дочернего процесса для записи
					ASSERT_TRUE(this->_io->destroy(cfds[1]));
					// Устананавливаем опции события
					ASSERT_TRUE(this->_io->setOptions(cfds[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
					// Устананавливаем опции события
					ASSERT_TRUE(this->_io->setOptions(mfds[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
					// Устанавливаем функцию обратного вызова на получение статуса события
					this->_io->on(cfds[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
						/**
						 * Обрабатываем статус события
						 */
						switch(static_cast <uint8_t> (status)){
							// Если статус принятия
							case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
								// Записываем в лог сообщение о принятии события
								this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус уничтожения
							case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
								// Записываем в лог сообщение об уничтожении события
								this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус инициализации
							case static_cast <uint8_t> (awh::event::status_t::INITIAL):
								// Записываем в лог сообщение об инициализации события
								this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус запуска события
							case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
								// Записываем в лог сообщение о запуске события
								this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус паузы события
							case static_cast <uint8_t> (awh::event::status_t::PAUSED):
								// Записываем в лог сообщение о паузе события
								this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус возобновления события
							case static_cast <uint8_t> (awh::event::status_t::RESUMED):
								// Записываем в лог сообщение о возобновлении события
								this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус успешного выполнения события
							case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
								// Записываем в лог сообщение о успешном выполнении события
								this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус неудачного выполнения события
							case static_cast <uint8_t> (awh::event::status_t::FAILURE):
								// Записываем в лог сообщение о неудачном выполнении события
								this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
							break;
							// Если статус выполнения события в ожидании
							case static_cast <uint8_t> (awh::event::status_t::PENDING):
								// Записываем в лог сообщение о выполнении события в ожидании
								this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус подключения события
							case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
								// Записываем в лог сообщение о подключении события
								this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус отмены события
							case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
								// Записываем в лог сообщение об отмене события
								this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус переподключения события
							case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
								// Записываем в лог сообщение о переподключении события
								this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус прослушивания события
							case static_cast <uint8_t> (awh::event::status_t::LISTENING):
								// Записываем в лог сообщение о прослушивании события
								this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
						}
					});
					// Устанавливаем функцию обратного вызова на запись в событие
					this->_io->on(mfds[1], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
					}));
					// Устанавливаем функцию обратного вызова на чтение из события
					this->_io->on(cfds[0], [mpid, &stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
						// Текст входящего сообщения
						const std::string message(reinterpret_cast <const char *> (data), size);
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Прочитано: ID=%u, MPID=%u, PID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, mpid, ::getpid(), size, message.c_str());
						// Останавливаем тест
						stop = true;
					});
					// Устанавливаем функцию обратного вызова на ошибку события
					this->_io->on(cfds[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
						/**
						 * Обрабатываем статус события
						 */
						switch(static_cast <uint8_t> (error)){
							// Если ошибка неизвестного события
							case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
								// Записываем ошибку в лог неизвестного события
								this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка недопустимой операции
							case static_cast <uint8_t> (awh::event::error_t::INVALID):
								// Записываем ошибку в лог недопустимой операции
								this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка доступа запрещёния
							case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
								// Записываем ошибку в лог доступа запрещёния
								this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка уже существующего объекта
							case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
								// Записываем ошибку в лог уже существующего объекта
								this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка некорректного адреса
							case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
								// Записываем ошибку в лог некорректного адреса
								this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка ошибки подключения
							case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
								// Записываем ошибку в лог подключения
								this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка недостаточно ресурсов
							case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
								// Записываем ошибку в лог недостаточно ресурсов
								this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка события
							case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
								// Записываем ошибку в лог события
								this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если объект не найден
							case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
								// Записываем ошибку в лог события
								this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
						}
					});
					// Устанавливаем функцию обратного вызова на общее событие
					this->_io->on(cfds[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
						/**
						 * Обрабатываем действие события
						 */
						switch(static_cast <uint8_t> (action)){
							// Если действие является чтением
							case static_cast <uint8_t> (awh::event::action_t::READ):
								// Записываем в лог сообщение о чтении события
								this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является записью
							case static_cast <uint8_t> (awh::event::action_t::WRITE):
								// Записываем в лог сообщение о записи события
								this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является подключением
							case static_cast <uint8_t> (awh::event::action_t::CONNECT):
								// Записываем в лог сообщение о подключении события
								this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является отключением
							case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
								// Записываем в лог сообщение об отключении события
								this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является переподключением
							case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
								// Записываем в лог сообщение о переподключении события
								this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является закрытием
							case static_cast <uint8_t> (awh::event::action_t::CLOSE):
								// Записываем в лог сообщение о закрытии события
								this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является изменением
							case static_cast <uint8_t> (awh::event::action_t::CHANGE):
								// Записываем в лог сообщение об изменении события
								this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является удалением
							case static_cast <uint8_t> (awh::event::action_t::DELETE):
								// Записываем в лог сообщение об удалении события
								this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является переименованием
							case static_cast <uint8_t> (awh::event::action_t::RENAME):
								// Записываем в лог сообщение о переименовании события
								this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является изменением атрибутов
							case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
								// Записываем в лог сообщение об изменении атрибутов события
								this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является отзывом доступа
							case static_cast <uint8_t> (awh::event::action_t::REVOKE):
								// Записываем в лог сообщение об отзыве доступа события
								this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является изменением счётчика жёстких ссылок
							case static_cast <uint8_t> (awh::event::action_t::HDLINK):
								// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
								this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
						}
					});
					// Выполняем фиксацию настроек событий дочернего процесса
					ASSERT_TRUE(this->_io->commit(cfds[0]));
					ASSERT_TRUE(this->_io->commit(mfds[1]));
					// Выполняем запуск события
					ASSERT_TRUE(this->_io->launch(cfds[0]));
					// Сообщение для отправки родительскому процессу
					const std::string message = "Hello from child process!";
					// Отправляем сообщение родительскому процессу
					this->_io->send(mfds[1], reinterpret_cast <const char *> (message.c_str()), message.length());
					/**
					 * Запускаем опрос событий
					 */
					while(!stop && this->_io->poll());
					// Уничтожаем все события после получения ответа
					ASSERT_TRUE(this->_io->deinitialize());
				} break;
				// Если процесс является родительским
				default: {
					// Уничтожаем событие родительского процесса для записи
					ASSERT_TRUE(this->_io->destroy(mfds[1]));
					// Уничтожаем событие дочернего процесса для чтения
					ASSERT_TRUE(this->_io->destroy(cfds[0]));
					// Устананавливаем опции события
					ASSERT_TRUE(this->_io->setOptions(mfds[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
					// Устананавливаем опции события
					ASSERT_TRUE(this->_io->setOptions(cfds[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
					// Устанавливаем функцию обратного вызова на получение статуса события
					this->_io->on(mfds[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
						/**
						 * Обрабатываем статус события
						 */
						switch(static_cast <uint8_t> (status)){
							// Если статус принятия
							case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
								// Записываем в лог сообщение о принятии события
								this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус уничтожения
							case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
								// Записываем в лог сообщение об уничтожении события
								this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус инициализации
							case static_cast <uint8_t> (awh::event::status_t::INITIAL):
								// Записываем в лог сообщение об инициализации события
								this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус запуска события
							case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
								// Записываем в лог сообщение о запуске события
								this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус паузы события
							case static_cast <uint8_t> (awh::event::status_t::PAUSED):
								// Записываем в лог сообщение о паузе события
								this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус возобновления события
							case static_cast <uint8_t> (awh::event::status_t::RESUMED):
								// Записываем в лог сообщение о возобновлении события
								this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус успешного выполнения события
							case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
								// Записываем в лог сообщение о успешном выполнении события
								this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус неудачного выполнения события
							case static_cast <uint8_t> (awh::event::status_t::FAILURE):
								// Записываем в лог сообщение о неудачном выполнении события
								this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
							break;
							// Если статус выполнения события в ожидании
							case static_cast <uint8_t> (awh::event::status_t::PENDING):
								// Записываем в лог сообщение о выполнении события в ожидании
								this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус подключения события
							case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
								// Записываем в лог сообщение о подключении события
								this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус отмены события
							case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
								// Записываем в лог сообщение об отмене события
								this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус переподключения события
							case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
								// Записываем в лог сообщение о переподключении события
								this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус прослушивания события
							case static_cast <uint8_t> (awh::event::status_t::LISTENING):
								// Записываем в лог сообщение о прослушивании события
								this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
						}
					});
					// Устанавливаем функцию обратного вызова на запись в событие
					this->_io->on(cfds[1], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
					}));
					// Устанавливаем функцию обратного вызова на чтение из события
					this->_io->on(mfds[0], [mpid, &stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
						// Текст входящего сообщения
						const std::string message(reinterpret_cast <const char *> (data), size);
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Прочитано: ID=%u, MPID=%u, PID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, mpid, ::getpid(), size, message.c_str());
						// Останавливаем тест
						stop = true;
					});
					// Устанавливаем функцию обратного вызова на ошибку события
					this->_io->on(mfds[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
						/**
						 * Обрабатываем статус события
						 */
						switch(static_cast <uint8_t> (error)){
							// Если ошибка неизвестного события
							case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
								// Записываем ошибку в лог неизвестного события
								this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка недопустимой операции
							case static_cast <uint8_t> (awh::event::error_t::INVALID):
								// Записываем ошибку в лог недопустимой операции
								this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка доступа запрещёния
							case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
								// Записываем ошибку в лог доступа запрещёния
								this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка уже существующего объекта
							case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
								// Записываем ошибку в лог уже существующего объекта
								this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка некорректного адреса
							case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
								// Записываем ошибку в лог некорректного адреса
								this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка ошибки подключения
							case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
								// Записываем ошибку в лог подключения
								this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка недостаточно ресурсов
							case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
								// Записываем ошибку в лог недостаточно ресурсов
								this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка события
							case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
								// Записываем ошибку в лог события
								this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если объект не найден
							case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
								// Записываем ошибку в лог события
								this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
						}
					});
					// Устанавливаем функцию обратного вызова на общее событие
					this->_io->on(mfds[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
						/**
						 * Обрабатываем действие события
						 */
						switch(static_cast <uint8_t> (action)){
							// Если действие является чтением
							case static_cast <uint8_t> (awh::event::action_t::READ):
								// Записываем в лог сообщение о чтении события
								this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является записью
							case static_cast <uint8_t> (awh::event::action_t::WRITE):
								// Записываем в лог сообщение о записи события
								this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является подключением
							case static_cast <uint8_t> (awh::event::action_t::CONNECT):
								// Записываем в лог сообщение о подключении события
								this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является отключением
							case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
								// Записываем в лог сообщение об отключении события
								this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является переподключением
							case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
								// Записываем в лог сообщение о переподключении события
								this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является закрытием
							case static_cast <uint8_t> (awh::event::action_t::CLOSE):
								// Записываем в лог сообщение о закрытии события
								this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является изменением
							case static_cast <uint8_t> (awh::event::action_t::CHANGE):
								// Записываем в лог сообщение об изменении события
								this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является удалением
							case static_cast <uint8_t> (awh::event::action_t::DELETE):
								// Записываем в лог сообщение об удалении события
								this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является переименованием
							case static_cast <uint8_t> (awh::event::action_t::RENAME):
								// Записываем в лог сообщение о переименовании события
								this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является изменением атрибутов
							case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
								// Записываем в лог сообщение об изменении атрибутов события
								this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является отзывом доступа
							case static_cast <uint8_t> (awh::event::action_t::REVOKE):
								// Записываем в лог сообщение об отзыве доступа события
								this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является изменением счётчика жёстких ссылок
							case static_cast <uint8_t> (awh::event::action_t::HDLINK):
								// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
								this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
						}
					});
					// Выполняем фиксацию настроек события сервера
					ASSERT_TRUE(this->_io->commit(mfds[0]));
					ASSERT_TRUE(this->_io->commit(cfds[1]));
					// Выполняем запуск события
					ASSERT_TRUE(this->_io->launch(mfds[0]));
					// Сообщение для отправки дочернему процессу
					const std::string message = "Hello from parent process!";
					// Отправляем сообщение родительскому процессу
					this->_io->send(cfds[1], reinterpret_cast <const char *> (message.c_str()), message.length());
					/**
					 * Запускаем опрос событий
					 */
					while(!stop && this->_io->poll());
					/**
					 * Дожидаемся завершения дочернего процесса
					 *
					 * @warning Прежде здесь стояла ВЫДЕРЖКА в три секунды. Выдержка не
					 *          дожидается ничего - она лишь надеется, что работник за это
					 *          время управится, - и на занятой машине надежда не сбывается.
					 *          Ожидание же возвращается ровно тогда, когда он кончил
					 *
					 * @note Заодно снимается запись о завершившемся процессе: без ожидания
					 *       он оставался бы в таблице процессов до конца набора
					 *
					 */
					int32_t status = 0;
					// Ожидаем завершения дочернего процесса
					ASSERT_EQ(::waitpid(pid, &status, 0), pid) << "дочерний процесс не дождался";
					// Проверяем, что дочерний процесс завершился сам, а не снят сигналом
					ASSERT_TRUE(WIFEXITED(status)) << "дочерний процесс снят сигналом";
					// Проверяем, что утверждения дочернего процесса прошли
					ASSERT_EQ(WEXITSTATUS(status), EXIT_SUCCESS) << "утверждения в дочернем процессе не прошли";
					// Уничтожаем все события после получения ответа
					ASSERT_TRUE(this->_io->deinitialize());
				}
			}
		} break;
		// Если семейство события является сокетом UDS
		case static_cast <uint8_t> (awh::event::family_t::UDS): {
			// Добавляем новое пользовательское событие
			const auto & events = this->_io->events(awh::event::family_t::UDS, this->_parameter.type);
			// Проверяем, что идентификаторы событий больше нуля
			ASSERT_GT(events[0], 0);
			ASSERT_GT(events[1], 0);
			// Инициализируем асинхронный движок ввода-вывода
			ASSERT_TRUE(this->_io->initialize());
			// Получаем идентификатор родительского процесса
			const pid_t mpid = ::getpid();
			// Устанавливаем идентификатор процесса
			pid_t pid = -1;
			/**
			 * Определяем тип потока
			 */
			switch((pid = ::fork())){
				// Если поток не создан
				case -1: {
					// Записываем в лог сообщение
					this->_log->print("Child process could not be created", awh::log_t::flag_t::CRITICAL);
					// Выходим из приложения
					::exit(EXIT_FAILURE);
				} break;
				// Если процесс является дочерним
				case 0: {
					/**
					 * Сторож завершения дочернего процесса
					 *
					 * @warning Ставится ПЕРВЫМ и устроен разрушителем, а не вызовом в конце
					 *          ветви, намеренно: отказ утверждения `ASSERT_*` выходит из
					 *          ТЕЛА ПРОВЕРКИ, минуя всякий код после себя, и завершение,
					 *          дописанное в конец, его бы не поймало. Разрушитель же
					 *          срабатывает на любом выходе из блока - и на обычном, и на
					 *          принудительном
					 *
					 * @warning Прежде завершения не было ВОВСЕ: дочерний процесс выходил из
					 *          `switch` и доигрывал набор проверок до конца наравне с
					 *          родительским. У четырёх случаев этой проверки это давало
					 *          ШЕСТНАДЦАТЬ процессов, каждый со своими событиями и портами;
					 *          отсюда же и шестнадцать итоговых сводок в выводе набора.
					 *          Столкновения портов, что мы ловили как «плавающие», росли
					 *          отсюда
					 *
					 * @note Завершение идёт через `_exit`, а не `exit`: обработчики выхода
					 *       принадлежат родителю, и через них дочерний процесс печатал бы
					 *       вторую сводку
					 *
					 */
					struct child_guard_t {
						/**
						 * @brief Деструктор
						 *
						 */
						~child_guard_t() noexcept {
							// Завершаем дочерний процесс, отдавая исход утверждений родителю
							::_exit(::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS);
						}
					} childGuard;
					// Выполняем переинициализацию асинхронного движка ввода-вывода
					ASSERT_TRUE(this->_io->reinitialize());
					// Уничтожаем событие родительского процесса
					ASSERT_TRUE(this->_io->destroy(events[0]));
					// Устананавливаем опции события
					ASSERT_TRUE(this->_io->setOptions(events[1], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
					// Устанавливаем функцию обратного вызова на событие таймера
					this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
						/**
						 * Обрабатываем статус события
						 */
						switch(static_cast <uint8_t> (status)){
							// Если статус принятия
							case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
								// Записываем в лог сообщение о принятии события
								this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус уничтожения
							case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
								// Записываем в лог сообщение об уничтожении события
								this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус инициализации
							case static_cast <uint8_t> (awh::event::status_t::INITIAL):
								// Записываем в лог сообщение об инициализации события
								this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус запуска события
							case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
								// Записываем в лог сообщение о запуске события
								this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус паузы события
							case static_cast <uint8_t> (awh::event::status_t::PAUSED):
								// Записываем в лог сообщение о паузе события
								this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус возобновления события
							case static_cast <uint8_t> (awh::event::status_t::RESUMED):
								// Записываем в лог сообщение о возобновлении события
								this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус успешного выполнения события
							case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
								// Записываем в лог сообщение о успешном выполнении события
								this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус неудачного выполнения события
							case static_cast <uint8_t> (awh::event::status_t::FAILURE):
								// Записываем в лог сообщение о неудачном выполнении события
								this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
							break;
							// Если статус выполнения события в ожидании
							case static_cast <uint8_t> (awh::event::status_t::PENDING):
								// Записываем в лог сообщение о выполнении события в ожидании
								this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус подключения события
							case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
								// Записываем в лог сообщение о подключении события
								this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус отмены события
							case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
								// Записываем в лог сообщение об отмене события
								this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус переподключения события
							case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
								// Записываем в лог сообщение о переподключении события
								this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус прослушивания события
							case static_cast <uint8_t> (awh::event::status_t::LISTENING):
								// Записываем в лог сообщение о прослушивании события
								this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
						}
					});
					// Устанавливаем функцию обратного вызова на запись в событие
					this->_io->on(events[1], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
					}));
					// Устанавливаем функцию обратного вызова на чтение из события
					this->_io->on(events[1], [mpid, &stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
						// Текст входящего сообщения
						const std::string message(reinterpret_cast <const char *> (data), size);
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Прочитано: ID=%u, MPID=%u, PID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, mpid, ::getpid(), size, message.c_str());
						// Останавливаем тест
						stop = true;
					});
					// Устанавливаем функцию обратного вызова на ошибку события
					this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
						/**
						 * Обрабатываем статус события
						 */
						switch(static_cast <uint8_t> (error)){
							// Если ошибка неизвестного события
							case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
								// Записываем ошибку в лог неизвестного события
								this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка недопустимой операции
							case static_cast <uint8_t> (awh::event::error_t::INVALID):
								// Записываем ошибку в лог недопустимой операции
								this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка доступа запрещёния
							case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
								// Записываем ошибку в лог доступа запрещёния
								this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка уже существующего объекта
							case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
								// Записываем ошибку в лог уже существующего объекта
								this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка некорректного адреса
							case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
								// Записываем ошибку в лог некорректного адреса
								this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка ошибки подключения
							case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
								// Записываем ошибку в лог подключения
								this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка недостаточно ресурсов
							case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
								// Записываем ошибку в лог недостаточно ресурсов
								this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка события
							case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
								// Записываем ошибку в лог события
								this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если объект не найден
							case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
								// Записываем ошибку в лог события
								this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
						}
					});
					// Устанавливаем функцию обратного вызова на общее событие
					this->_io->on(events[1], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
						/**
						 * Обрабатываем действие события
						 */
						switch(static_cast <uint8_t> (action)){
							// Если действие является чтением
							case static_cast <uint8_t> (awh::event::action_t::READ):
								// Записываем в лог сообщение о чтении события
								this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является записью
							case static_cast <uint8_t> (awh::event::action_t::WRITE):
								// Записываем в лог сообщение о записи события
								this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является подключением
							case static_cast <uint8_t> (awh::event::action_t::CONNECT):
								// Записываем в лог сообщение о подключении события
								this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является отключением
							case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
								// Записываем в лог сообщение об отключении события
								this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является переподключением
							case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
								// Записываем в лог сообщение о переподключении события
								this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является закрытием
							case static_cast <uint8_t> (awh::event::action_t::CLOSE):
								// Записываем в лог сообщение о закрытии события
								this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является изменением
							case static_cast <uint8_t> (awh::event::action_t::CHANGE):
								// Записываем в лог сообщение об изменении события
								this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является удалением
							case static_cast <uint8_t> (awh::event::action_t::DELETE):
								// Записываем в лог сообщение об удалении события
								this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является переименованием
							case static_cast <uint8_t> (awh::event::action_t::RENAME):
								// Записываем в лог сообщение о переименовании события
								this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является изменением атрибутов
							case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
								// Записываем в лог сообщение об изменении атрибутов события
								this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является отзывом доступа
							case static_cast <uint8_t> (awh::event::action_t::REVOKE):
								// Записываем в лог сообщение об отзыве доступа события
								this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является изменением счётчика жёстких ссылок
							case static_cast <uint8_t> (awh::event::action_t::HDLINK):
								// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
								this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
						}
					});
					// Выполняем фиксацию настроек события сервера
					ASSERT_TRUE(this->_io->commit(events[1]));
					// Выполняем запуск события
					ASSERT_TRUE(this->_io->launch(events[1]));
					// Сообщение для отправки родительскому процессу
					const std::string message = "Hello from child process!";
					// Отправляем сообщение родительскому процессу
					this->_io->send(events[1], reinterpret_cast <const char *> (message.c_str()), message.length());
					/**
					 * Запускаем опрос событий
					 */
					while(!stop && this->_io->poll());
					// Уничтожаем все события после получения ответа
					ASSERT_TRUE(this->_io->deinitialize());
				} break;
				// Если процесс является родительским
				default: {
					// Уничтожаем событие дочернего процесса
					ASSERT_TRUE(this->_io->destroy(events[1]));
					// Устананавливаем опции события
					ASSERT_TRUE(this->_io->setOptions(events[0], awh::event::options::NO_SIGILL | awh::event::options::NO_SIGPIPE | awh::event::options::NO_IO_BLOCK | awh::event::options::CLOSE_ON_EXEC));
					// Устанавливаем функцию обратного вызова на событие таймера
					this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::status_t status) noexcept -> void {
						/**
						 * Обрабатываем статус события
						 */
						switch(static_cast <uint8_t> (status)){
							// Если статус принятия
							case static_cast <uint8_t> (awh::event::status_t::ACCEPTED):
								// Записываем в лог сообщение о принятии события
								this->_log->print("Событие принято: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус уничтожения
							case static_cast <uint8_t> (awh::event::status_t::DESTROYED):
								// Записываем в лог сообщение об уничтожении события
								this->_log->print("Событие подлежит уничтожению: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус инициализации
							case static_cast <uint8_t> (awh::event::status_t::INITIAL):
								// Записываем в лог сообщение об инициализации события
								this->_log->print("Событие инициализировано: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус запуска события
							case static_cast <uint8_t> (awh::event::status_t::LAUNCHED):
								// Записываем в лог сообщение о запуске события
								this->_log->print("Событие запущено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус паузы события
							case static_cast <uint8_t> (awh::event::status_t::PAUSED):
								// Записываем в лог сообщение о паузе события
								this->_log->print("Событие на паузе: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус возобновления события
							case static_cast <uint8_t> (awh::event::status_t::RESUMED):
								// Записываем в лог сообщение о возобновлении события
								this->_log->print("Событие возобновлено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус успешного выполнения события
							case static_cast <uint8_t> (awh::event::status_t::SUCCESS):
								// Записываем в лог сообщение о успешном выполнении события
								this->_log->print("Событие успешно выполнено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус неудачного выполнения события
							case static_cast <uint8_t> (awh::event::status_t::FAILURE):
								// Записываем в лог сообщение о неудачном выполнении события
								this->_log->print("Событие выполнено с ошибкой: ID=%u", awh::log_t::flag_t::CRITICAL, eid);
							break;
							// Если статус выполнения события в ожидании
							case static_cast <uint8_t> (awh::event::status_t::PENDING):
								// Записываем в лог сообщение о выполнении события в ожидании
								this->_log->print("Событие в ожидании: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус подключения события
							case static_cast <uint8_t> (awh::event::status_t::CONNECTED):
								// Записываем в лог сообщение о подключении события
								this->_log->print("Событие подключено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус отмены события
							case static_cast <uint8_t> (awh::event::status_t::CANCELLED):
								// Записываем в лог сообщение об отмене события
								this->_log->print("Событие отменено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус переподключения события
							case static_cast <uint8_t> (awh::event::status_t::RECONNECTED):
								// Записываем в лог сообщение о переподключении события
								this->_log->print("Событие переподключено: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если статус прослушивания события
							case static_cast <uint8_t> (awh::event::status_t::LISTENING):
								// Записываем в лог сообщение о прослушивании события
								this->_log->print("Событие прослушивается: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
						}
					});
					// Устанавливаем функцию обратного вызова на запись в событие
					this->_io->on(events[0], static_cast <awh::engine::callback::write_t> ([this](const awh::event::id_t eid, const size_t size) noexcept -> void {
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Записано: ID=%u, %zu байт", awh::log_t::flag_t::INFO, eid, size);
					}));
					// Устанавливаем функцию обратного вызова на чтение из события
					this->_io->on(events[0], [mpid, &stop, this](const awh::event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
						// Текст входящего сообщения
						const std::string message(reinterpret_cast <const char *> (data), size);
						// Записываем в лог сообщение о переподключении события
						this->_log->print("Прочитано: ID=%u, MPID=%u, PID=%u, %zu байт, сообщение: %s", awh::log_t::flag_t::INFO, eid, mpid, ::getpid(), size, message.c_str());
						// Останавливаем тест
						stop = true;
					});
					// Устанавливаем функцию обратного вызова на ошибку события
					this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::error_t error, const std::string & description) noexcept -> void {
						/**
						 * Обрабатываем статус события
						 */
						switch(static_cast <uint8_t> (error)){
							// Если ошибка неизвестного события
							case static_cast <uint8_t> (awh::event::error_t::UNKNOWN):
								// Записываем ошибку в лог неизвестного события
								this->_log->print("Неизвестная ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка недопустимой операции
							case static_cast <uint8_t> (awh::event::error_t::INVALID):
								// Записываем ошибку в лог недопустимой операции
								this->_log->print("Недопустимая операция события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка доступа запрещёния
							case static_cast <uint8_t> (awh::event::error_t::ACCESS_DENIED):
								// Записываем ошибку в лог доступа запрещёния
								this->_log->print("Доступ к событию запрещён: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка уже существующего объекта
							case static_cast <uint8_t> (awh::event::error_t::ALREADY_EXISTS):
								// Записываем ошибку в лог уже существующего объекта
								this->_log->print("Объект события уже существует: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка некорректного адреса
							case static_cast <uint8_t> (awh::event::error_t::INVALID_ADDRESS):
								// Записываем ошибку в лог некорректного адреса
								this->_log->print("Некорректный адрес события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка ошибки подключения
							case static_cast <uint8_t> (awh::event::error_t::CONNECTION_FAIL):
								// Записываем ошибку в лог подключения
								this->_log->print("Ошибка подключения события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка недостаточно ресурсов
							case static_cast <uint8_t> (awh::event::error_t::INSUFFICIENT_RES):
								// Записываем ошибку в лог недостаточно ресурсов
								this->_log->print("Недостаточно ресурсов для события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если ошибка события
							case static_cast <uint8_t> (awh::event::error_t::EVENT_FAIL):
								// Записываем ошибку в лог события
								this->_log->print("Ошибка события: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
							// Если объект не найден
							case static_cast <uint8_t> (awh::event::error_t::NOT_FOUND):
								// Записываем ошибку в лог события
								this->_log->print("Объект события не найден: ID=%u, Описание=%s", awh::log_t::flag_t::CRITICAL, eid, description.c_str());
							break;
						}
					});
					// Устанавливаем функцию обратного вызова на общее событие
					this->_io->on(events[0], [this](const awh::event::id_t eid, const awh::event::action_t action) noexcept -> void {
						/**
						 * Обрабатываем действие события
						 */
						switch(static_cast <uint8_t> (action)){
							// Если действие является чтением
							case static_cast <uint8_t> (awh::event::action_t::READ):
								// Записываем в лог сообщение о чтении события
								this->_log->print("Событие на чтение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является записью
							case static_cast <uint8_t> (awh::event::action_t::WRITE):
								// Записываем в лог сообщение о записи события
								this->_log->print("Событие на запись: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является подключением
							case static_cast <uint8_t> (awh::event::action_t::CONNECT):
								// Записываем в лог сообщение о подключении события
								this->_log->print("Событие на подключение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является отключением
							case static_cast <uint8_t> (awh::event::action_t::DISCONNECT):
								// Записываем в лог сообщение об отключении события
								this->_log->print("Событие на отключение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является переподключением
							case static_cast <uint8_t> (awh::event::action_t::RECONNECT):
								// Записываем в лог сообщение о переподключении события
								this->_log->print("Событие на переподключение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является закрытием
							case static_cast <uint8_t> (awh::event::action_t::CLOSE):
								// Записываем в лог сообщение о закрытии события
								this->_log->print("Событие на закрытие подключения: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является изменением
							case static_cast <uint8_t> (awh::event::action_t::CHANGE):
								// Записываем в лог сообщение об изменении события
								this->_log->print("Событие на изменение: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является удалением
							case static_cast <uint8_t> (awh::event::action_t::DELETE):
								// Записываем в лог сообщение об удалении события
								this->_log->print("Событие на удаление: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является переименованием
							case static_cast <uint8_t> (awh::event::action_t::RENAME):
								// Записываем в лог сообщение о переименовании события
								this->_log->print("Событие на переименование: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является изменением атрибутов
							case static_cast <uint8_t> (awh::event::action_t::ATTRIB):
								// Записываем в лог сообщение об изменении атрибутов события
								this->_log->print("Событие на изменение атрибутов: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является отзывом доступа
							case static_cast <uint8_t> (awh::event::action_t::REVOKE):
								// Записываем в лог сообщение об отзыве доступа события
								this->_log->print("Событие на отзыв доступа: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
							// Если действие является изменением счётчика жёстких ссылок
							case static_cast <uint8_t> (awh::event::action_t::HDLINK):
								// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
								this->_log->print("Событие на изменение счётчика жёстких ссылок: ID=%u", awh::log_t::flag_t::INFO, eid);
							break;
						}
					});
					// Выполняем фиксацию настроек события сервера
					ASSERT_TRUE(this->_io->commit(events[0]));
					// Выполняем запуск события
					ASSERT_TRUE(this->_io->launch(events[0]));
					// Сообщение для отправки дочернему процессу
					const std::string message = "Hello from parent process!";
					// Отправляем сообщение родительскому процессу
					this->_io->send(events[0], reinterpret_cast <const char *> (message.c_str()), message.length());
					/**
					 * Запускаем опрос событий
					 */
					while(!stop && this->_io->poll());
					/**
					 * Дожидаемся завершения дочернего процесса
					 *
					 * @warning Прежде здесь стояла ВЫДЕРЖКА в три секунды. Выдержка не
					 *          дожидается ничего - она лишь надеется, что работник за это
					 *          время управится, - и на занятой машине надежда не сбывается.
					 *          Ожидание же возвращается ровно тогда, когда он кончил
					 *
					 * @note Заодно снимается запись о завершившемся процессе: без ожидания
					 *       он оставался бы в таблице процессов до конца набора
					 *
					 */
					int32_t status = 0;
					// Ожидаем завершения дочернего процесса
					ASSERT_EQ(::waitpid(pid, &status, 0), pid) << "дочерний процесс не дождался";
					// Проверяем, что дочерний процесс завершился сам, а не снят сигналом
					ASSERT_TRUE(WIFEXITED(status)) << "дочерний процесс снят сигналом";
					// Проверяем, что утверждения дочернего процесса прошли
					ASSERT_EQ(WEXITSTATUS(status), EXIT_SUCCESS) << "утверждения в дочернем процессе не прошли";
					// Уничтожаем все события после получения ответа
					ASSERT_TRUE(this->_io->deinitialize());
				}
			}
		} break;
	}
	#endif
}

/**
 * @brief Инициализация параметров теста
 *
 */
/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * @brief Работник проверки межпроцессного обмена
	 *
	 * @details Обращения `fork` у MS Windows нет вовсе, и второй процесс поднимается
	 *          повторным запуском СВОЕГО ЖЕ двоичного файла - так порождает работников
	 *          и модуль кластера. Оттого работник живёт отдельной проверкой, отключённой
	 *          именем: сам по себе набор её не запускает, а родитель зовёт её особо
	 *
	 * @note Встреча процессов идёт ПО ИМЕНИ канала, названному родителем и переданному
	 *       окружением: наследовать описатель, как то делает ветвление у систем POSIX,
	 *       здесь нечем
	 *
	 */
	TEST_F(IoFixture, DISABLED_IoIPCWorkerTest){
		// Буфер под имя канала обмена, названного родителем
		wchar_t buffer[256]{0};
		// Получаем имя канала обмена из окружения
		const DWORD length = ::GetEnvironmentVariableW(L"AWH_IO_IPC_PIPE", buffer, static_cast <DWORD> (sizeof(buffer) / sizeof(buffer[0])));
		// Если имени канала в окружении нет, работать нечем
		if((length == 0) || (length >= (sizeof(buffer) / sizeof(buffer[0]))))
			// Выходим отказом: без имени канала родителя не найти
			::_exit(EXIT_FAILURE);
		/**
		 * Ставим работнику предел по времени
		 *
		 * @note Обращения `alarm` у MS Windows нет вовсе, а опрос событий ждёт их без
		 *       срока: не встретивший родителя работник ждал бы вечно вместо отказа
		 */
		std::thread([]() noexcept -> void {
			// Ждём предельный срок работы
			std::this_thread::sleep_for(std::chrono::seconds(20));
			// Выходим отказом: за отведённый срок встречи не состоялось
			::_exit(EXIT_FAILURE);
		}).detach();
		// Выполняем инициализацию сетевого движка
		ASSERT_TRUE(this->_io->initialize());
		// Заводим событие канала обмена, каким доложимся родителю
		const awh::event::id_t channel = this->_io->event(awh::event::node_t::IPC, awh::event::family_t::PIPE, awh::event::type_t::SEQPACKET);
		// Проверяем, что идентификатор события канала заведён
		ASSERT_GT(channel, 0);
		// Устанавливаем имя канала обмена, названное родителем
		ASSERT_TRUE(this->_io->setTarget(channel, this->_fmk->convert(std::wstring(buffer))));
		// Выполняем фиксацию настроек канала обмена
		ASSERT_TRUE(this->_io->commit(channel));
		// Выполняем подключение канала обмена к родителю
		ASSERT_TRUE(this->_io->connect({channel}));
		// Выполняем запуск канала обмена
		ASSERT_TRUE(this->_io->launch(channel));
		// Свой опознаватель процесса: им и доказывается, что процессов было два
		const uint32_t self = static_cast <uint32_t> (::GetCurrentProcessId());
		// Докладываем родителю свой опознаватель процесса
		ASSERT_TRUE(this->_io->send(channel, reinterpret_cast <const char *> (&self), sizeof(self)));
		// Отсчёт времени на доставку доклада
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Крутим опрос, давая докладу уйти
		 */
		while((std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::steady_clock::now() - start).count() < 1000))
			// Выполняем оборот опроса
			this->_io->poll(50);
		// Сворачиваем движок
		this->_io->destroy(channel);
		this->_io->deinitialize();
		/**
		 * Выходим успехом
		 *
		 * @note Выход здесь обязан быть немедленным: работник это тот же двоичный файл
		 *       набора, и обычное возвращение увело бы его в отчёт о прогоне
		 */
		::_exit(EXIT_SUCCESS);
	}
#endif

INSTANTIATE_TEST_SUITE_P(TestParameters, IoIPCTestParameterizedFixture,
	::testing::Values(
		IoIPCTestParameter({
			awh::event::type_t::NONE,
			awh::event::family_t::PIPE
		}),
		IoIPCTestParameter({
			awh::event::type_t::STREAM,
			awh::event::family_t::UDS
		}),
		IoIPCTestParameter({
			awh::event::type_t::SEQPACKET,
			awh::event::family_t::UDS
		}),
		IoIPCTestParameter({
			awh::event::type_t::DATAGRAM,
			awh::event::family_t::UDS
		})
	)
);

/**
 * Возвращаем снятые макросы MS Windows
 */
#include <sys/macro_pop.hpp>
