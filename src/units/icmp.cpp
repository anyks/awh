/**
 * @file icmp.cpp
 * @date 2026-03-06
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
 * @brief Реализация модуля ICMP-клиента — формирование и отправка эхо-запросов, приём и разбор ответов,
 *        измерение времени отклика, контроль TTL, номеров последовательности и количества повторов
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cerrno>
#include <vector>
#include <random>
#include <cstdint>
#include <algorithm>

/**
 * Системные заголовочные файлы
 */
/**
 * Для операционной системы MS Windows
 *
 * @note Заголовки эти принадлежат POSIX и у MS Windows отсутствуют.
 *       Соответствующие им объявления приходят там из winsock2.h,
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
	 */
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif
#include <sys/types.h>
/**
 * @note Системные заголовки netinet/ip.h и netinet/ip6.h здесь не подключаются вовсе:
 *       у MS Windows их нет, а заголовки пакетов IPv4 и IPv6 модуль описывает своими
 *       словами по месту разбора ответа - структурами ip4_hdr_min и ip6_hdr_min
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <units/icmp.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Инкапсулируем статические типы данных в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Генератор случайных чисел для перемешивания списка удалённых узлов
	 *
	 */
	random_device __awh_randev__;
};

/**
 * @brief Инкапсулируем статические типы данных в пространство имён
 *
 */
namespace {
	/**
	 * @brief Структура заголовков ICMP
	 *
	 */
	/**
	 * @par Намеренные решения
	 *
	 *      **Поля объединения оставлены без начальных значений.** Прежде каждое из
	 *      них задавалось как `= 0`, отчего объединение переставало быть простым, а
	 *      признак `packed` на всей структуре отвергался. GCC сообщал об этом
	 *      предупреждением, Clang умалчивал, - и заголовок не упаковывался ни там,
	 *      ни там
	 *
	 *      Расхождение это не косметическое: у запроса `echo` восьмибайтовое поле
	 *      полезной нагрузки выравнивалось по восьми байтам и съезжало с четвёртого
	 *      байта на восьмой. В сеть уходил формат, отличный от предписанного RFC 792
	 *
	 *      Обнуление обеспечивается на месте создания - `header_t icmp{}`, - и в
	 *      начальных значениях полей нужды нет
	 *
	 */
	typedef struct IcmpHeader {
		uint8_t type;      // Тип запроса
		uint8_t code;      // Код запроса
		uint16_t checksum; // Контрольная сумма
		/**
		 * @brief Объединение структур запроса
		 *
		 */
		union {
			/**
			 * @brief Структура отправляемого запроса
			 *
			 */
			struct {
				uint16_t identifier; // Идентификатор запроса
				uint16_t sequence;   // Номер последовательности
				uint64_t payload;    // Тело полезной нагрузки
			} __attribute__((packed)) echo;
			/**
			 * @brief Структура указателя запроса
			 *
			 */
			struct ICMP_PACKET_POINTER_HEADER {
				// Указатель пакета
				uint8_t pointer;
			} __attribute__((packed)) pointer;
			/**
			 * @brief Структура адреса ответа
			 *
			 */
			struct ICMP_PACKET_REDIRECT_HEADER {
				// Адрес ответа IPv4
				uint32_t gatewayAddress;
			} __attribute__((packed)) redirect;
			/**
			 * @brief Структура адреса ответа
			 *
			 */
			struct ICMP6_PACKET_REDIRECT_HEADER {
				// Адрес ответа IPv6
				uint32_t gatewayAddress[4];
			} __attribute__((packed)) redirect6;
		} __attribute__((packed)) meta;
	} __attribute__((packed)) header_t;

	/**
	 * Закрепляем разметку заголовка: она предписана RFC 792 и проверяется здесь, а не
	 * доверяется признаку упаковки. Без упаковки поле полезной нагрузки съезжает с
	 * восьмого байта на шестнадцатый, а сам заголовок разрастается с 20 байт до 24
	 */
	static_assert(sizeof(header_t) == 20, "AWH: разметка заголовка ICMP нарушена");
	static_assert(offsetof(header_t, meta) == 4, "AWH: разметка заголовка ICMP нарушена");
	static_assert(offsetof(header_t, meta.echo.sequence) == 6, "AWH: разметка заголовка ICMP нарушена");
	static_assert(offsetof(header_t, meta.echo.payload) == 8, "AWH: разметка заголовка ICMP нарушена");

	/**
	 * @brief Функция генерации уникального идентификатора
	 *
	 * @return уникальный идентификатор
	 *
	 */
	unit::icmp_t::id_t identifier() noexcept {
		// Начинаем с 1 (0 можно оставить как "invalid")
		static atomic_uint16_t id{1};
		// Переменная результата
		unit::icmp_t::id_t result = 0;
		/**
		 * Получаем следующий идентификатор, пропуская нулевое значение
		 */
		do
			// Получаем следующий идентификатор
			result = id.fetch_add(1, memory_order_relaxed);
		/**
		 * Если идентификатор обернулся и стал нулём,
		 * то повторяем попытку получения следующего идентификатора
		 */
		while(result == 0);
		// Возвращаем результат
		return result;
	}

	/**
	 * @brief Генератор случайных чисел для полезной нагрузки ICMP
	 *
	 * @return ссылка на генератор случайных чисел
	 *
	 */
	mt19937 & icmpRng() noexcept {
		// Генератор случайных чисел для текущего потока
		static thread_local mt19937 generator(::__awh_randev__());
		// Возвращаем генератор случайных чисел
		return generator;
	}

	/**
	 * @brief Функция генерации полезной нагрузки ICMP-запроса
	 *
	 * @return случайное значение полезной нагрузки
	 *
	 */
	uint64_t icmpPayload() noexcept {
		// Распределение случайных чисел для полезной нагрузки
		static thread_local uniform_int_distribution <mt19937::result_type> dist(0, numeric_limits <uint32_t>::max() - 1);
		// Возвращаем случайное значение полезной нагрузки
		return static_cast <uint64_t> (dist(::icmpRng()));
	}

	/**
	 * @brief Функция добавления 16-битного слова в контрольную сумму
	 *
	 * @param sum  текущая сумма
	 * @param word добавляемое слово
	 *
	 */
	void checksumAdd(uint32_t & sum, const uint16_t word) noexcept {
		// Добавляем слово в сумму
		sum += word;
		// Если контрольная сумма достигла предела
		if(sum & 0xffff0000)
			// Выполняем свёртку переноса
			sum = ((sum >> 16) + (sum & 0xffff));
	}

	/**
	 * @brief Функция добавления буфера в контрольную сумму
	 *
	 * @param sum    текущая сумма
	 * @param buffer буфер данных
	 * @param size   размер буфера
	 *
	 */
	void checksumAdd(uint32_t & sum, const void * buffer, const size_t size) noexcept {
		// Если данные переданы верные
		if((buffer != nullptr) && (size > 0)){
			// Указатель на данные буфера
			auto data = reinterpret_cast <const uint8_t *> (buffer);
			// Индекс текущего байта
			size_t index = 0;
			// Выполняем перебор всех 16-битных слов буфера
			while((index + 1) < size){
				// Добавляем очередное слово в контрольную сумму
				::checksumAdd(sum, static_cast <uint16_t> ((data[index] << 8) | data[index + 1]));
				// Переходим к следующему слову
				index += 2;
			}
			// Если остался непарный байт
			if(index < size)
				// Добавляем последний байт в контрольную сумму
				::checksumAdd(sum, static_cast <uint16_t> (data[index] << 8));
		}
	}

	/**
	 * @brief Функция подсчёта контрольной суммы ICMPv6
	 *
	 * @param src    адрес источника IPv6
	 * @param dst    адрес назначения IPv6
	 * @param buffer буфер ICMP-сообщения
	 * @param size   размер ICMP-сообщения
	 * @return       подсчитанная контрольная сумма
	 *
	 */
	uint16_t checksum6(const uint8_t src[16], const uint8_t dst[16], const void * buffer, const size_t size) noexcept {
		// Контрольная сумма расчёта
		uint32_t sum = 0;
		// Добавляем адреса и длину псевдозаголовка IPv6
		::checksumAdd(sum, src, 16);
		::checksumAdd(sum, dst, 16);
		// Длина ICMP-сообщения в сетевом порядке байт
		const uint32_t length = htonl(static_cast <uint32_t> (size));
		// Добавляем длину ICMP-сообщения
		::checksumAdd(sum, &length, sizeof(length));
		// Нулевое поле и номер протокола ICMPv6
		const uint8_t proto[4] = {0, 0, 0, IPPROTO_ICMPV6};
		// Добавляем номер протокола ICMPv6
		::checksumAdd(sum, proto, sizeof(proto));
		// Добавляем тело ICMP-сообщения
		::checksumAdd(sum, buffer, size);
		/**
		 * Выполняем финальную свёртку переноса
		 */
		while(sum >> 16)
			// Выполняем свёртку переноса
			sum = ((sum >> 16) + (sum & 0xffff));
		// Возвращаем результат
		return static_cast <uint16_t> (~sum);
	}

	/**
	 * @brief Функция подсчёта контрольной суммы
	 *
	 * @param buffer буфер данных для подсчёта
	 * @param size   размер данных для подсчёта
	 * @return       подсчитанная контрольная сумма
	 *
	 */
	uint16_t checksum(const void * buffer, const size_t size) noexcept {
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
};

/**
 * @brief Инкапсулируем функции разрешения доменных имён в пространство имён
 *
 */
namespace dns {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Функция разрешения доменного имени удалённого сервера
	 *
	 * @param domain доменное имя удалённого сервера
	 * @return       список IP-адресов удалённого сервера
	 *
	 */
	static vector <unique_ptr <net::addr_t>> resolve(string_view domain) noexcept {
		// Список полученных IP-адресов
		vector <unique_ptr <net::addr_t>> ips;
		// Создаём объект IP-адреса для параметров удалённого сервера
		struct addrinfo hints = {};
		// Устанавливаем тип сокета для удалённого сервера (TCP)
		hints.ai_socktype = 0;
		// Устанавливаем семейство протоколов для удалённого сервера (IPv4 + IPv6)
		hints.ai_family = AF_UNSPEC;
		// Результат получения параметров удалённого сервера
		struct addrinfo * result = nullptr;
		/**
		 * Выполняем получение параметров удалённого сервера по его адресу
		 */
		if(::getaddrinfo(string(domain).c_str(), nullptr, &hints, &result) == 0){
			/**
			 * Выполняем перебор всех полученных параметров удалённого сервера и сохраняем их в общий список удалённых серверов
			 */
			for(auto * p = result; p != nullptr; p = p->ai_next){
				/**
				 * Определяем тип адреса удалённого сервера
				 */
				switch(p->ai_family){
					// Если адрес является IPv4
					case AF_INET: {
						// Выполняем инициализацию объекта IP-адреса
						unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv4_t> ();
						// Получаем объект с IPv4-адресом удалённого сервера
						auto * sa = reinterpret_cast <sockaddr_in *> (p->ai_addr);
						// Копируем IPv4-адрес удалённого сервера в объект IP-адреса
						awh_cast <net::addr_net_ipv4_t *> (ip.get())->address = sa->sin_addr.s_addr;
						// Добавляем сервер в общий список удалённых серверов
						ips.push_back(::move(ip));
					} break;
					// Если адрес является IPv6
					case AF_INET6: {
						// Выполняем инициализацию объекта IP-адреса
						unique_ptr <net::addr_t> ip = make_unique <net::addr_net_ipv6_t> ();
						// Получаем объект с IPv6-адресом удалённого сервера
						auto * sa = reinterpret_cast <sockaddr_in6 *> (p->ai_addr);
						// Копируем IPv6-адрес удалённого сервера в объект IP-адреса
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], &sa->sin6_addr.s6_addr[0], 16);
						// Добавляем сервер в общий список удалённых серверов
						ips.push_back(::move(ip));
					} break;
				}
			}
			// Освобождаем память, выделенную для хранения параметров удалённого сервера
			::freeaddrinfo(result);
		}
		// Перемешиваем список полученных IP-адресов
		::shuffle(ips.begin(), ips.end(), ::icmpRng());
		// Возвращаем полученные IP-адреса
		return ips;
	}
};

/**
 * @brief Конструктор
 *
 */
awh::unit::ICMP::Response::Response() noexcept :
 size(0), elapsed(0), sequence(0),
 timeToLive(0), address(nullptr) {}

/**
 * @brief Конструктор
 *
 */
awh::unit::ICMP::Client::Client() noexcept :
 delay(5000), eid(0),
 target(nullptr), source(nullptr) {}

/**
 * @brief Конструктор
 *
 */
awh::unit::ICMP::Transfer::Transfer() noexcept :
 id(0), waiting(false), count(0), sequence(0), timestamp(0) {}

/**
 * @brief Метод уничтожения события ICMP-клиента
 *
 */
void awh::unit::ICMP::destroyClient() noexcept {
	// Если событие ICMP-клиента активно
	if(this->_client.eid > 0){
		// Снимаем функцию обратного вызова на событие чтения данных
		this->_io->on(this->_client.eid, static_cast <engine::callback::read_t> (nullptr));
		// Снимаем функцию обратного вызова на событие получения ошибок
		this->_io->on(this->_client.eid, static_cast <engine::callback::error_t> (nullptr));
		// Снимаем функцию обратного вызова на событие таймаута
		this->_io->on(this->_client.eid, static_cast <engine::callback::timeout_t> (nullptr));
		// Удаляем событие ICMP-клиента
		this->_io->destroy(this->_client.eid);
		// Сбрасываем идентификатор события ICMP-клиента
		this->_client.eid = 0;
	}
}
/**
 * @brief Метод отправки ICMP Echo-запроса
 *
 * @param eid      идентификатор события ICMP-клиента
 * @param id       идентификатор ICMP-запроса
 * @param sequence номер последовательности запроса
 * @return         количество отправленных байт
 *
 */
size_t awh::unit::ICMP::sendEcho(const event::id_t eid, const id_t id, const uint16_t sequence) noexcept {
	// Создаём объект заголовков
	header_t icmp{};
	// Устанавливаем код запроса
	icmp.code = 0;
	// Получаем семейство IP-адресов текущего события ICMP-клиента
	const event::family_t family = this->_io->family(eid);
	/**
	 * Определяем семейство события
	 */
	switch(static_cast <uint8_t> (family)){
		// Для семейства IPv4
		case static_cast <uint8_t> (event::family_t::IPV4):
			// Выполняем установку типа запроса
			icmp.type = 8;
		break;
		// Для семейства IPv6
		case static_cast <uint8_t> (event::family_t::IPV6):
			// Выполняем установку типа запроса
			icmp.type = 128;
		break;
		// Для остальных семейств
		default:
			// Возвращаем значение по умолчанию
			return 0;
	}
	// Устанавливаем идентификатор запроса
	icmp.meta.echo.identifier = htons(id);
	// Устанавливаем номер последовательности
	icmp.meta.echo.sequence = htons(sequence);
	// Устанавливаем данные полезной нагрузки
	icmp.meta.echo.payload = ::icmpPayload();
	// Обнуляем поле контрольной суммы перед пересчётом
	icmp.checksum = 0;
	// Для IPv6 RAW-сокета считаем контрольную сумму с псевдозаголовком
	if((family == event::family_t::IPV6) && (this->_io->type(eid) == event::type_t::RAW) && (this->_client.target != nullptr)){
		// Адрес источника IPv6
		uint8_t src[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		// Адрес назначения IPv6
		uint8_t dst[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		// Если адрес сети для выполнения запроса установлен
		if(this->_client.source != nullptr)
			// Копируем адрес источника IPv6
			::memcpy(src, &awh_cast <net::addr_net_ipv6_t *> (this->_client.source.get())->address[0], 16);
		// Копируем адрес назначения IPv6
		::memcpy(dst, &awh_cast <net::addr_net_ipv6_t *> (this->_client.target.get())->address[0], 16);
		// Выполняем подсчёт контрольной суммы ICMPv6
		icmp.checksum = ::checksum6(src, dst, &icmp, sizeof(icmp));
	// Для остальных типов сокетов
	} else
		// Выполняем подсчёт контрольной суммы ICMP
		icmp.checksum = ::checksum(&icmp, sizeof(icmp));
	// Запоминаем номер последовательности последнего отправленного запроса
	this->_transfer.sequence = sequence;
	// Запоминаем время отправки запроса
	this->_transfer.timestamp = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
	// Отправляем сообщение серверу
	return this->_io->send(eid, &icmp, sizeof(icmp));
}
/**
 * @brief Метод обработки ошибок событий ICMP-клиента
 *
 * @param eid         идентификатор события ICMP-клиента
 * @param error       код ошибки события ICMP-клиента
 * @param description описание ошибки события ICMP-клиента
 *
 */
void awh::unit::ICMP::error(const event::id_t eid, const event::error_t error, const string & description) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const event::id_t, const event::error_t, const string &)> ("error", eid, error, description);
}
/**
 * @brief Метод обработки таймаута ожидания ответа ICMP-сервера
 *
 * @param eid    идентификатор события ICMP-клиента
 * @param action действие события таймера ICMP-клиента
 * @param delay  задержка таймера ICMP-клиента
 * @return       нужно ли завершить клиента после истечения таймаута
 *
 */
bool awh::unit::ICMP::timeout([[maybe_unused]] const event::id_t eid, const event::action_t action, const uint32_t delay) noexcept {
	// Снимаем флаг ожидания ответа от сервера
	this->_transfer.waiting = false;
	// Выполняем получение идентификатора функции обратного вызова
	const callback_t::id_t fid = this->_callback.id("timeout");
	// Если функция обратного вызова установлена
	if(this->_callback.is(fid))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const id_t, const uint16_t, const uint32_t)> (fid, this->_transfer.id, this->_transfer.count, delay);
	// Если функция обратного вызова не установлена
	else {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug(
				"ICMP-client timeout (delay: %u)",
				__PRETTY_FUNCTION__,
				make_tuple(eid, static_cast <uint16_t> (action), delay),
				log_t::flag_t::WARNING, delay
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("ICMP-client timeout (delay: %u)", log_t::flag_t::WARNING, delay);
		#endif
	}
	// Запрещаем завершение клиента после истечения таймаута
	return false;
}
/**
 * @brief Метод обработки ответов удалённого сервера на ICMP-запросы
 *
 * @param eid  идентификатор события чтения ICMP-ответа
 * @param mode режим обработки события чтения ICMP-ответа
 * @param data данные события чтения ICMP-ответа
 * @param size размер данных события чтения ICMP-ответа
 *
 */
/**
 * @brief Метод продолжения ожидания своего ответа
 *
 * @param eid идентификатор события чтения ICMP-ответа
 *
 */
void awh::unit::ICMP::keepWaiting(const event::id_t eid) noexcept {
	// Если ответ удалённого сервера не ожидается, продолжать нечего
	if(!this->_transfer.waiting)
		// Выходим из функции
		return;
	// Выполняем продолжение прерванного чужим пакетом ожидания
	this->_io->rearmTimeout(eid, event::action_t::READ);
}
void awh::unit::ICMP::response(const event::id_t eid, const mode_t mode, const uint8_t * data, const size_t size) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если данные переданы верные
		if((data != nullptr) && (size > 0)){
			// Длина IP-заголовка
			size_t length = 0;
			// Время жизни пакета ответа (TTL/Hop Limit)
			uint32_t timeToLive = 0;
			// Заголовок пакета ICMP протокола
			const header_t * icmp = nullptr;
			// IP-адрес для вывода результата
			unique_ptr <net::addr_t> address = nullptr;
			/**
			 * Определяем версию IP-адреса
			 * 0x40 = IPv4 (0100 0000), 0x60 = IPv6 (0110 0000)
			 */
			switch(((data[0] & 0xF0) >> 4)){
				// Если адрес является IPv4
				case 4: {
					/**
					 * Добавляем выравнивание структуры для корректного чтения данных из буфера
					 */
					#pragma pack(push, 1)
					/**
					 * @brief IPv4 заголовок без списка опций = 20 байт
					 *
					 * @details Заголовок описан здесь своими словами, а не взят из
					 *          системного `netinet/ip.h`: у MS Windows заголовка этого нет
					 *          вовсе. Заодно снимается зависимость от порядка следования
					 *          битовых полей `ip_v`/`ip_hl`, который у системного описания
					 *          задан условной сборкой по порядку октетов машины
					 *
					 */
					struct ip4_hdr_min {
						uint8_t  verihl; // Версия протокола и длина заголовка в 32-битных словах
						uint8_t  tos;    // Тип обслуживания
						uint16_t plen;   // Полная длина пакета
						uint16_t id;     // Идентификатор пакета
						uint16_t frag;   // Флаги и смещение фрагмента
						uint8_t  ttl;    // Лимит времени жизни
						uint8_t  nxt;    // Протокол вышестоящего уровня
						uint16_t sum;    // Контрольная сумма заголовка
						uint32_t src;    // Адрес источника
						uint32_t dst;    // Адрес назначения
					};
					// Удаляем выравнивание структуры для корректного чтения данных из буфера
					#pragma pack(pop)
					// Если размер данных меньше размера заголовка IP
					if(size < sizeof(struct ip4_hdr_min))
						// Ответ не наш - продолжаем ожидание своего
						return this->keepWaiting(eid);
					// Приводим данные к структуре IP-заголовка
					const struct ip4_hdr_min * iph = reinterpret_cast <const struct ip4_hdr_min *> (data);
					// Извлекаем длину IP-заголовка (младшая половина первого октета в 32-битных словах)
					length = ((iph->verihl & 0x0F) * 4);
					// Если заголовок пришёл битый
					if((length < 20) || (size < (length + 8)))
						// Минимум ICMP-заголовок
						return;
					// Минимум 8 байт ICMP
					if(size >= (length + 8)){
						// Приводим данные к структуре ICMP-заголовка
						icmp = reinterpret_cast <const header_t *> (data + length);
						// Извлекаем TTL из IPv4-заголовка
						timeToLive = iph->ttl;
						// Выполняем инициализацию объекта IP-адреса
						address = make_unique <net::addr_net_ipv4_t> ();
						// Устанавливаем IP-адрес
						awh_cast <net::addr_net_ipv4_t *> (address.get())->address = iph->src;
					}
				} break;
				// Если адрес является IPv6
				case 6: {
					/**
					 * Добавляем выравнивание структуры для корректного чтения данных из буфера
					 */
					#pragma pack(push, 1)
					/**
					 * @brief IPv6 заголовок фиксирован = 40 байт
					 *
					 */
					struct ip6_hdr_min {
						uint32_t flow;    // Потоковая метка (version, traffic class, flow label)
						uint16_t plen;    // Длина полезной нагрузки
						uint8_t  nxt;     // Следующий заголовок
						uint8_t  hlim;    // Лимит времени жизни
						uint8_t  src[16]; // Адрес источника
						uint8_t  dst[16]; // Адрес назначения
					};
					// Удаляем выравнивание структуры для корректного чтения данных из буфера
					#pragma pack(pop)
					// Если размер данных меньше размера заголовка IP
					if(size < 40)
						// Ответ не наш - продолжаем ожидание своего
						return this->keepWaiting(eid);
					// Приводим данные к структуре IP-заголовка
					const struct ip6_hdr_min * ip6h = reinterpret_cast <const struct ip6_hdr_min *> (data);
					// Начальная длина IPv6-заголовка
					length = 40;
					// Извлекаем Hop Limit из IPv6-заголовка
					timeToLive = ip6h->hlim;
					// Тип следующего заголовка после базового IPv6-заголовка
					uint8_t next = ip6h->nxt;
					// Признак успешного выхода на ICMPv6-заголовок
					bool hasIcmpv6 = false;
					/**
					 * Проходим цепочку extension headers до ICMPv6.
					 * Если встречаем неподдерживаемый протокол, завершаем обработку.
					 */
					while(length < size){
						/**
						 * Определяем тип текущего IPv6-заголовка в цепочке
						 */
						switch(next){
							// Если достигли ICMPv6-заголовка
							case IPPROTO_ICMPV6:
								// Устанавливаем флаг успешного выхода на ICMPv6-заголовок
								hasIcmpv6 = true;
							break;
							// Заголовки с длиной в 8-байтовых блоках (длина = (Hdr Ext Len + 1) * 8)
							case IPPROTO_HOPOPTS:
							case IPPROTO_ROUTING:
							case IPPROTO_DSTOPTS: {
								// Проверяем, что в буфере есть минимум поле Next Header и Hdr Ext Len
								if(size < (length + 2))
									// Ответ не наш - продолжаем ожидание своего
									return this->keepWaiting(eid);
								// Извлекаем длину extension header из поля Hdr Ext Len
								const uint8_t hdrLen = data[length + 1];
								// Рассчитываем фактическую длину extension header в байтах
								const size_t extLen = (static_cast <size_t> (hdrLen) + 1) * 8;
								// Проверяем корректность длины и границы буфера
								if((extLen < 8) || (size < (length + extLen)))
									// Ответ не наш - продолжаем ожидание своего
									return this->keepWaiting(eid);
								// Переходим к следующему заголовку в цепочке
								next = data[length];
								// Увеличиваем длину на размер текущего extension header
								length += extLen;
							} break;
							// Fragment Header всегда занимает 8 байт
							case IPPROTO_FRAGMENT: {
								// Проверяем, что в буфере помещается весь Fragment Header
								if(size < (length + 8))
									// Ответ не наш - продолжаем ожидание своего
									return this->keepWaiting(eid);
								// Переходим к следующему заголовку в цепочке
								next = data[length];
								// Увеличиваем длину на размер Fragment Header
								length += 8;
							} break;
							// Authentication Header: длина = (Payload Len + 2) * 4
							case IPPROTO_AH: {
								// Проверяем, что в буфере есть минимум поле Next Header и Payload Len
								if(size < (length + 2))
									// Ответ не наш - продолжаем ожидание своего
									return this->keepWaiting(eid);
								// Извлекаем длину AH из поля Payload Len
								const uint8_t hdrLen = data[length + 1];
								// Рассчитываем фактическую длину AH в байтах
								const size_t extLen = (static_cast <size_t> (hdrLen) + 2) * 4;
								// Проверяем корректность длины и границы буфера
								if((extLen < 8) || (size < (length + extLen)))
									// Ответ не наш - продолжаем ожидание своего
									return this->keepWaiting(eid);
								// Переходим к следующему заголовку в цепочке
								next = data[length];
								// Увеличиваем длину на размер AH
								length += extLen;
							} break;
							// Неподдерживаемый заголовок в цепочке
							default:
								// Ответ не наш - продолжаем ожидание своего
								return this->keepWaiting(eid);
						}
						// Если ICMPv6-заголовок найден, прекращаем разбор extension headers
						if(hasIcmpv6)
							// Выходим из цикла
							break;
					}
					// Если ICMPv6-заголовок не найден или буфер слишком мал, завершаем обработку
					if(!hasIcmpv6 || (size < (length + 8)))
						// Ответ не наш - продолжаем ожидание своего
						return this->keepWaiting(eid);
					// Приводим данные к структуре ICMP-заголовка
					icmp = reinterpret_cast <const header_t *> (data + length);
					// Выполняем инициализацию объекта IP-адреса
					address = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (address.get())->address[0], &ip6h->src, 16);
				} break;
				// Если это какой-то другой адрес
				default: {
					// Если размера данных достаточно для чтения заголовка ICMP
					if(size >= sizeof(header_t)){
						// Результат полученных данных
						icmp = reinterpret_cast <const header_t *> (data);
						// Извлекаем IP-адрес установленный в событии
						this->_io->getTarget(eid, address);
						// Извлекаем TTL/Hop Limit последнего принятого пакета
						timeToLive = static_cast <uint32_t> (this->_io->getCountHops(eid));
					// Если размер данных меньше размера заголовка ICMP
					} else return this->keepWaiting(eid);
				}
			}
			// Если ICMP-заголовок не найден
			if(icmp == nullptr)
				// Ответ не наш - продолжаем ожидание своего
				return this->keepWaiting(eid);
			// Получаем семейство IP-адресов текущего события ICMP-клиента
			const event::family_t family = this->_io->family(eid);
			// Ожидаемый тип ICMP Echo Reply
			const uint8_t replyType = (static_cast <uint8_t> (family) == static_cast <uint8_t> (event::family_t::IPV6) ? 129 : 0);
			// Если ответ пришёл вне активной ICMP-сессии
			if(!this->_transfer.waiting)
				// Ответ не наш - продолжаем ожидание своего
				return this->keepWaiting(eid);
			// Если пакет не является корректным ICMP Echo Reply
			if((icmp->type != replyType) || (icmp->code != 0))
				// Ответ не наш - продолжаем ожидание своего
				return this->keepWaiting(eid);
			// Извлекаем идентификатор запроса
			const id_t id = ntohs(icmp->meta.echo.identifier);
			// Извлекаем номер последовательности запроса
			const uint16_t sequence = ntohs(icmp->meta.echo.sequence);
			// Если идентификатор или последовательность не совпадают с активным запросом
			if((id != this->_transfer.id) || (sequence != this->_transfer.sequence))
				// Ответ не наш - продолжаем ожидание своего
				return this->keepWaiting(eid);
			// Получаем метаданные последнего принятого дейтаграммного пакета
			const net::dgram_info_t & info = this->_io->getTrafficInfo(eid);
			// Если TTL/Hop Limit не удалось извлечь из IP-заголовка
			if(timeToLive == 0)
				// Устанавливаем TTL/Hop Limit из метаданных пакета
				timeToLive = static_cast <uint32_t> (info.hops);
			// Получаем текущую метку времени
			const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
			// Если адрес удалённого сервера получен
			if(address != nullptr){
				/**
				 * Определяем тип адреса
				 */
				switch(address->size){
					// Если адрес является IPv4
					case 4: {
						// Выполняем инициализацию объекта IP-адреса
						this->_replyAddress = make_unique <net::addr_net_ipv4_t> ();
						// Копируем IPv4-адрес удалённого сервера
						awh_cast <net::addr_net_ipv4_t *> (this->_replyAddress.get())->address = awh_cast <net::addr_net_ipv4_t *> (address.get())->address;
					} break;
					// Если адрес является IPv6
					case 16: {
						// Выполняем инициализацию объекта IP-адреса
						this->_replyAddress = make_unique <net::addr_net_ipv6_t> ();
						// Копируем IPv6-адрес удалённого сервера
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (this->_replyAddress.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (address.get())->address[0], 16);
					} break;
				}
			}
			// Выполняем получение идентификатора функции обратного вызова
			const callback_t::id_t fid = this->_callback.id("ping");
			// Если функция обратного вызова установлена для получения ответа от удалённого сервера
			if(this->_callback.is(fid)){
				// Создаём объект ответа от удалённого сервера
				response_t response{};
				// Устанавливаем размер полученных данных от удалённого сервера
				response.size = size;
				// Устанавливаем номер последовательности запроса
				response.sequence = sequence;
				// Устанавливаем IP-адрес удалённого сервера
				response.address = this->_replyAddress.get();
				// Устанавливаем время жизни ответа от удалённого сервера
				response.timeToLive = timeToLive;
				// Устанавливаем время ответа от удалённого сервера
				response.elapsed = (now - this->_transfer.timestamp);
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const id_t, const response_t &)> (fid, id, response);
			}
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const id_t, const net::dgram_info_t &)> ("info", id, info);
			// Если выполняется асинхронный режим пинга удалённого сервера
			if(mode == mode_t::ASYNC){
				// Если номер последовательности запроса меньше количества отправленных запросов
				if(sequence < (this->_transfer.count - 1))
					// Отправляем следующий ICMP Echo-запрос
					this->sendEcho(eid, id, sequence + 1);
				// Снимаем флаг ожидания ответа от сервера
				else this->_transfer.waiting = false;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(eid, static_cast <uint16_t> (mode), data, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод инициализации события ICMP-клиента
 *
 * @param family семейство протоколов (например: IPv4 или IPv6)
 * @return       результат инициализации события ICMP-клиента
 *
 */
bool awh::unit::ICMP::init(const event::family_t family) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Удаляем ранее созданное событие ICMP-клиента
		this->destroyClient();
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Добавляем новое событие клиента ICMP
			this->_client.eid = this->_io->event(event::node_t::CLIENT, family, event::type_t::RAW, event::protocol_t::ICMP);
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Выбираем тип сокета в зависимости от привилегий пользователя
			const event::type_t type = (::getuid() > 0 ? event::type_t::DATAGRAM : event::type_t::RAW);
			// Добавляем новое событие клиента ICMP
			this->_client.eid = this->_io->event(event::node_t::CLIENT, family, type, event::protocol_t::ICMP);
		#endif
		// Если событие ICMP-клиента не создано
		if(this->_client.eid == 0)
			// Возвращаем результат
			return false;
		// Если адрес назначения сервера не установлен
		if(this->_client.target == nullptr){
			// Удаляем событие ICMP-клиента
			this->destroyClient();
			// Выполняем получение идентификатора функции обратного вызова
			const callback_t::id_t fid = this->_callback.id("error");
			// Если функция обратного вызова установлена
			if(this->_callback.is(fid))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::id_t, const event::error_t, const string &)> (fid, 0, event::error_t::INVALID_ADDRESS, "Target address is not set");
			// Если функция обратного вызова не установлена
			else {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("ICMP-client target address is not set", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("ICMP-client target address is not set", log_t::flag_t::CRITICAL);
				#endif
			}
			// Возвращаем результат
			return false;
		}
		// Ожидаемый размер адреса назначения
		const uint8_t expectedSize = (static_cast <uint8_t> (family) == static_cast <uint8_t> (event::family_t::IPV6) ? 16 : 4);
		// Если семейство адреса назначения не совпадает с семейством события
		if(this->_client.target->size != expectedSize){
			// Удаляем событие ICMP-клиента
			this->destroyClient();
			// Выполняем получение идентификатора функции обратного вызова
			const callback_t::id_t fid = this->_callback.id("error");
			// Если функция обратного вызова установлена
			if(this->_callback.is(fid))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::id_t, const event::error_t, const string &)> (fid, 0, event::error_t::INVALID_ADDRESS, "Target address family mismatch");
			// Возвращаем результат
			return false;
		}
		// Устанавливаем функцию обратного вызова на событие получения ошибок
		this->_io->on(this->_client.eid, static_cast <engine::callback::error_t> (std::bind(&icmp_t::error, this, _1, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие чтения данных
		this->_io->on(this->_client.eid, static_cast <engine::callback::read_t> (std::bind(&icmp_t::response, this, _1, mode_t::ASYNC, _2, _3)));
		// Устанавливаем функцию обратного вызова на событие таймаута
		this->_io->on(this->_client.eid, static_cast <engine::callback::timeout_t> (std::bind(static_cast <bool (icmp_t::*)(const event::id_t, const event::action_t, const uint32_t)> (&icmp_t::timeout), this, _1, _2, _3)));
		// Если опции события не установлены
		if(!this->_io->setOptions(this->_client.eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::DGRAM_INFO)){
			// Удаляем событие ICMP-клиента
			this->destroyClient();
			// Если функция обратного вызова не установлена
			if(!this->_callback.is("error")){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("Failed to set options for ICMP-client event", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Failed to set options for ICMP-client event", log_t::flag_t::CRITICAL);
				#endif
			}
			// Возвращаем результат
			return false;
		}
		// Устанавливаем адрес сервера назначения
		this->_io->setTarget(this->_client.eid, this->_client.target.get());
		// Если адрес сети для выполнения запроса установлен
		if(this->_client.source != nullptr){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4):
					// Устанавливаем IP-адрес события
					result = this->_io->setAddress(this->_client.eid, event::address_t::IPV4, this->_client.source.get());
				break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6):
					// Устанавливаем IP-адрес события
					result = this->_io->setAddress(this->_client.eid, event::address_t::IPV6, this->_client.source.get());
				break;
			}
			// Если адрес сети для выполнения запроса установить не удалось
			if(!result){
				// Удаляем событие ICMP-клиента
				this->destroyClient();
				// Возвращаем результат
				return false;
			}
		}
		// Устанавливаем время ожидания ответа от ICMP-сервера
		this->_io->setTimeout(this->_client.eid, event::action_t::READ, this->_client.delay);
		// Выполняем фиксацию параметров события и его запуск
		if((result = this->_io->commit(this->_client.eid) && this->_io->launch(this->_client.eid)))
			// Возвращаем результат
			return true;
		// Удаляем событие ICMP-клиента
		this->destroyClient();
		// Если функция обратного вызова не установлена
		if(!this->_callback.is("error")){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("Failed to launch ICMP-client", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Failed to launch ICMP-client", log_t::flag_t::CRITICAL);
			#endif
		}
		// Возвращаем результат
		return false;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Удаляем событие ICMP-клиента
		this->destroyClient();
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return false;
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова (ping, timeout, datagram)
 *
 */
void awh::unit::ICMP::callback(const callback_t & callback) noexcept {
	// Устанавливаем функцию обратного вызова для родительского юнита
	unit_t::callback(callback);
	// Выполняем установку функции обратного вызова при получении метаданных дейтаграммного пакета
	this->_callback.set("info", callback);
	// Выполняем установку функции обратного вызова при получении ответа от удалённого сервера
	this->_callback.set("ping", callback);
	// Выполняем установку функции обратного вызова при наступлении таймаута ожидания ответа от удалённого сервера
	this->_callback.set("timeout", callback);
}
/**
 * @brief Метод установки таймаута для ожидания ответа от сервера
 *
 * @param delay время ожидания ответа от сервера (в миллисекундах)
 *
 */
void awh::unit::ICMP::setTimeout(const uint32_t delay) noexcept {
	// Устанавливаем время ожидания ответа от сервера
	this->_client.delay = delay;
}
/**
 * @brief Метод получения типа события
 *
 * @return тип события
 *
 */
awh::event::type_t awh::unit::ICMP::type() const noexcept {
	// Получаем тип события ICMP-клиента
	return this->_io->type(this->_client.eid);
}
/**
 * @brief Метод получения типа узла события
 *
 * @return тип узла события
 *
 */
awh::event::node_t awh::unit::ICMP::node() const noexcept {
	// Получаем тип узла события ICMP-клиента
	return this->_io->node(this->_client.eid);
}
/**
 * @brief Метод получения семейства события
 *
 * @return семейство адресов
 *
 */
awh::event::family_t awh::unit::ICMP::family() const noexcept {
	// Получаем семейство события ICMP-клиента
	return this->_io->family(this->_client.eid);
}
/**
 * @brief Метод получения статуса события
 *
 * @return статус события
 *
 */
awh::event::status_t awh::unit::ICMP::status() const noexcept {
	// Получаем статус события ICMP-клиента
	return this->_io->status(this->_client.eid);
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 *
 */
bool awh::unit::ICMP::setTarget(string_view target) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес ICMP-сервера передан
		if(!target.empty()){
			/**
			 * Определяем тип IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_addr.host(target))){
				// Если адрес является IPv4
				case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
					// Выполняем парсинг IPv4-адреса
					if((result = this->_addr.parse(target, net_addr_t::type_t::IPV4))){
						// Устанавливаем адрес сервера назначения
						this->_client.target = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Возвращаем результат
						return result;
					}
				} break;
				// Если адрес является IPv6
				case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
					// Выполняем парсинг IPv6-адреса
					if((result = this->_addr.parse(target, net_addr_t::type_t::IPV6))){
						// Устанавливаем адрес сервера назначения
						this->_client.target = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Возвращаем результат
						return result;
					}
				} break;
				// Если мы получили другой тип адреса, то считаем, что это доменное имя
				default: {
					// Получаем список IP-адресов удалённого сервера
					auto ips = ::dns::resolve(target);
					/**
					 * Сначала ищем IPv4-адрес, затем IPv6-адрес
					 */
					for(const uint8_t size : {4, 16}){
						/**
						 * Выполняем перебор всего списка полученных IP-адресов
						 */
						for(auto & ip : ips){
							// Если размер адреса совпадает с искомым семейством
							if(ip->size == size){
								// Устанавливаем адрес сервера назначения
								this->_client.target = ::move(ip);
								// Возвращаем результат
								return true;
							}
						}
					}
				} break;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(target), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 *
 */
bool awh::unit::ICMP::setTarget(const net::addr_t * target) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес ICMP-сервера передан
		if(target != nullptr){
			/**
			 * Определяем тип адреса
			 */
			switch(target->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем инициализацию объекта IP-адреса
					this->_client.target = make_unique <net::addr_net_ipv4_t> ();
					// Устанавливаем IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (this->_client.target.get())->address = awh_cast <const net::addr_net_ipv4_t *> (target)->address;
					// Возвращаем true
					return true;
				}
				// Если адрес является IPv6
				case 16: {
					// Выполняем инициализацию объекта IP-адреса
					this->_client.target = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (this->_client.target.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (target)->address[0], 16);
					// Возвращаем true
					return true;
				}
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 *
 */
bool awh::unit::ICMP::setTarget(const event::family_t family, string_view target) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес ICMP-сервера передан
		if(!target.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем парсинг IPv4-адреса
					if((result = this->_addr.parse(target, net_addr_t::type_t::IPV4))){
						// Устанавливаем адрес сервера назначения
						this->_client.target = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Возвращаем результат
						return result;
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем парсинг IPv6-адреса
					if((result = this->_addr.parse(target, net_addr_t::type_t::IPV6))){
						// Устанавливаем адрес сервера назначения
						this->_client.target = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Возвращаем результат
						return result;
					}
				} break;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), target), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки адреса сети, с которого будет выполняться запрос
 *
 * @param source адрес сети для выполнения запроса
 * @return       результат выполнения установки
 *
 */
bool awh::unit::ICMP::setSource(string_view source) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if(!source.empty()){
			// Выполняем парсинг IP-адреса
			if((result = this->_addr.parse(source))){
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если адрес является IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
						// Получаем IP-адрес в исходном виде
						this->_client.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Возвращаем результат
						return result;
					}
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
						// Получаем IP-адрес в исходном виде
						this->_client.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Возвращаем результат
						return result;
					}
				}
			}
		// Сбрасываем IP-адрес события
		} else this->_client.source.reset(nullptr);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(source), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки адреса сети, с которого будет выполняться запрос
 *
 * @param source адрес сети для выполнения запроса
 * @return       результат выполнения установки
 *
 */
bool awh::unit::ICMP::setSource(const net::addr_t * source) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if(source != nullptr){
			/**
			 * Определяем тип адреса
			 */
			switch(source->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем инициализацию объекта IP-адреса
					this->_client.source = make_unique <net::addr_net_ipv4_t> ();
					// Устанавливаем IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (this->_client.source.get())->address = awh_cast <const net::addr_net_ipv4_t *> (source)->address;
					// Возвращаем true
					return true;
				}
				// Если адрес является IPv6
				case 16: {
					// Выполняем инициализацию объекта IP-адреса
					this->_client.source = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (this->_client.source.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (source)->address[0], 16);
					// Возвращаем true
					return true;
				}
			}
		// Если адрес сети для выполнения запроса не передан
		} else {
			// Сбрасываем IP-адрес события
			this->_client.source.reset(nullptr);
			// Возвращаем true
			return true;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки адреса сети, с которого будет выполняться запрос
 *
 * @param family семейство IP-адресов IPv4/IPv6
 * @param source адрес сети для выполнения запроса
 * @return       результат выполнения установки
 *
 */
bool awh::unit::ICMP::setSource(const event::family_t family, string_view source) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if(!source.empty()){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем парсинг IPv4-адреса
					if((result = this->_addr.parse(source, net_addr_t::type_t::IPV4))){
						// Получаем IP-адрес в исходном виде
						this->_client.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Возвращаем результат
						return result;
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем парсинг IPv6-адреса
					if((result = this->_addr.parse(source, net_addr_t::type_t::IPV6))){
						// Получаем IP-адрес в исходном виде
						this->_client.source = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Возвращаем результат
						return result;
					}
				} break;
			}
		// Сбрасываем IP-адрес события
		} else this->_client.source.reset(nullptr);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (family), source), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения идентификатора ICMP-клиента для выполнения запроса к удалённому серверу
 *
 * @return идентификатор ICMP-клиента для выполнения запроса к удалённому серверу
 *
 */
awh::unit::ICMP::id_t awh::unit::ICMP::issue() const noexcept {
	// Генерируем идентификатор ICMP-запроса
	return ::identifier();
}
/**
 * @brief Метод выполнения пингов удалённого сервера
 *
 * @param id    идентификатор ICMP-клиента для выполнения запроса к удалённому серверу
 * @param count количество выполняемых запросов
 * @param mode  режим выполнения запросов
 * @return      результат выполнения запроса
 *
 */
bool awh::unit::ICMP::ping(const id_t id, const uint16_t count, const mode_t mode) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если количество выполняемых запросов равно нулю
		if(count == 0)
			// Возвращаем результат
			return result;
		/**
		 * Определяем режим выполнения пинга удалённого сервера
		 */
		switch(static_cast <uint8_t> (mode)){
			// Если выполняется синхронный режим пинга удалённого сервера
			case static_cast <uint8_t> (mode_t::SYNC): {
				// Если пинг удалённого сервера ещё не выполняется и адрес назначения установлен
				if(!this->_transfer.waiting && (this->_client.target != nullptr)){
					// Устанавливаем флаг ожидания ответа от сервера
					this->_transfer.waiting = true;
					// Устанавливаем идентификатор ICMP-клиента для выполнения запроса к удалённому серверу
					this->_transfer.id = id;
					// Получаем семейство IP-адресов текущего события ICMP-клиента
					event::family_t family = event::family_t::NONE;
					/**
					 * Определяем семейство события
					 */
					switch(this->_client.target->size){
						// Для семейства IPv4
						case 4:
							// Получаем семейство IP-адресов текущего события ICMP-клиента
							family = event::family_t::IPV4;
						break;
						// Для семейства IPv6
						case 16:
							// Получаем семейство IP-адресов текущего события ICMP-клиента
							family = event::family_t::IPV6;
						break;
					}
					// Идентификатор события
					event::id_t eid = 0;
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Добавляем новое событие клиента ICMP
						eid = this->_io->event(event::node_t::CLIENT, family, event::type_t::RAW, event::protocol_t::ICMP);
					/**
					 * Для операционной системы не являющейся MS Windows
					 */
					#else
						// Если пользователь является непривилегированным
						if(::getuid() > 0)
							// Добавляем новое событие клиента ICMP
							eid = this->_io->event(awh::event::node_t::CLIENT, family, event::type_t::DATAGRAM, event::protocol_t::ICMP);
						// Добавляем новое событие клиента ICMP
						else eid = this->_io->event(event::node_t::CLIENT, family, event::type_t::RAW, event::protocol_t::ICMP);
					#endif
					// Устанавливаем функцию обратного вызова на событие получения ошибок
					this->_io->on(eid, static_cast <engine::callback::error_t> (std::bind(&icmp_t::error, this, _1, _2, _3)));
					// Устанавливаем функцию обратного вызова на событие чтения данных
					this->_io->on(eid, static_cast <engine::callback::read_t> (std::bind(&icmp_t::response, this, _1, mode, _2, _3)));
					// Если опции события не установлены
					if(!this->_io->setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::CLOSE_ON_EXEC | event::options::DGRAM_INFO)){
						// Удаляем событие ICMP-клиента
						this->_io->destroy(eid);
						// Если функция обратного вызова не установлена
						if(!this->_callback.is("error")){
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("Failed to set options for ICMP-client event", __PRETTY_FUNCTION__, make_tuple(id, count, static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL);
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("Failed to set options for ICMP-client event", log_t::flag_t::CRITICAL);
							#endif
						}
						// Снимаем флаг ожидания ответа от сервера
						this->_transfer.waiting = false;
						// Выходим из функции
						return false;
					}
					// Если адрес назначения сервера установлен
					if(this->_client.target != nullptr){
						// Устанавливаем адрес сервера назначения
						this->_io->setTarget(eid, this->_client.target.get());
						// Если адрес сети для выполнения запроса установлен
						if(this->_client.source != nullptr){
							// Получаем семейство IP-адресов текущего события ICMP-клиента
							const event::family_t family = this->_io->family(eid);
							/**
							 * Определяем семейство события
							 */
							switch(static_cast <uint8_t> (family)){
								// Для семейства IPv4
								case static_cast <uint8_t> (event::family_t::IPV4):
									// Устанавливаем IP-адрес события
									this->_io->setAddress(eid, event::address_t::IPV4, this->_client.source.get());
								break;
								// Для семейства IPv6
								case static_cast <uint8_t> (event::family_t::IPV6):
									// Устанавливаем IP-адрес события
									this->_io->setAddress(eid, event::address_t::IPV6, this->_client.source.get());
								break;
							}
						}
						// Устанавливаем таймаут события на запись
						this->_io->setTimeout(eid, event::action_t::WRITE, this->_client.delay);
						// Устанавливаем таймаут события на чтение
						this->_io->setTimeout(eid, event::action_t::READ, this->_client.delay);
						// Выполняем фиксацию параметров события и его запуск
						if(!(result = this->_io->commit(eid) && this->_io->launch(eid))){
							// Удаляем событие ICMP-клиента
							this->_io->destroy(eid);
							// Если функция обратного вызова не установлена
							if(!this->_callback.is("error")){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Записываем ошибку в лог
									this->_log->debug("Failed to launch ICMP-client", __PRETTY_FUNCTION__, make_tuple(id, count, static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL);
								/**
								 * Если режим отладки не включён
								 */
								#else
									// Записываем ошибку в лог
									this->_log->print("Failed to launch ICMP-client", log_t::flag_t::CRITICAL);
								#endif
							}
						// Если фиксация параметров события прошла успешно
						} else {
							// Устанавливаем количество выполняемых запросов
							this->_transfer.count = count;
							// Последовательность
							uint16_t sequence = 0;
							/**
							 * Выполняем пинг указанного количества раз
							 */
							for(uint16_t i = 0; i < count; i++){
								// Отправляем ICMP Echo-запрос
								if(this->sendEcho(eid, id, sequence) > 0){
									// Выполняем чтение ответа
									if(this->_io->recv(eid))
										// Увеличиваем последовательность запроса
										sequence++;
								}
							}
						}
					// Если адрес назначения сервера не установлен
					} else {
						// Формируем текст сообщения об ошибке ICMP-клиента
						const string error = "ICMP-client target address is not set";
						// Выполняем получение идентификатора функции обратного вызова
						const callback_t::id_t fid = this->_callback.id("error");
						// Если функция обратного вызова установлена
						if(this->_callback.is(fid))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (const event::id_t, const event::error_t, const string &)> (fid, this->_client.eid, event::error_t::INVALID, error);
						// Если callback ошибки не установлен
						else {
							/**
							 * Если включён режим отладки
							 */
							#if DEBUG_MODE
								// Записываем ошибку в лог
								this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, count, static_cast <uint16_t> (mode)), log_t::flag_t::WARNING, error.c_str());
							/**
							 * Если режим отладки не включён
							 */
							#else
								// Записываем ошибку в лог
								this->_log->print("%s", log_t::flag_t::WARNING, error.c_str());
							#endif
						}
					}
					// Удаляем событие ICMP-клиента
					this->_io->destroy(eid);
					// Снимаем флаг ожидания ответа от сервера
					this->_transfer.waiting = false;
				}
			} break;
			// Если выполняется асинхронный режим пинга удалённого сервера
			case static_cast <uint8_t> (mode_t::ASYNC): {
				// Если пинг удалённого сервера ещё не выполняется
				if(!this->_transfer.waiting){
					// Устанавливаем флаг ожидания ответа от сервера
					this->_transfer.waiting = true;
					// Устанавливаем идентификатор ICMP-клиента для выполнения запроса к удалённому серверу
					this->_transfer.id = id;
					// Устанавливаем количество выполняемых запросов
					this->_transfer.count = count;
					// Отправляем первый ICMP Echo-запрос
					this->sendEcho(this->_client.eid, id, 0);
					// Подтверждаем запуск ICMP-сессии
					result = true;
				}
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id, count, static_cast <uint16_t> (mode)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::unit::ICMP::ICMP(const fmk_t * fmk, const log_t * log) noexcept : unit_t(fmk, log), _addr(fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::unit::ICMP::~ICMP() noexcept {
	// Удаляем событие ICMP-клиента
	this->destroyClient();
}
