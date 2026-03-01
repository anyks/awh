/**
 * @file: dns.cpp
 * @date: 2026-02-26
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Стандартные модули
 */
#include <array>
#include <vector>
#include <random>
#include <cerrno>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/locker.hpp>
#include <sys/binbox.hpp>

/**
 * Подключаем заголовочные файл модуля
 */
#include <units/dns.hpp>

/**
 * Если используется модуль IDN и операционная система не MS Windows
 */
#if AWH_IDN && !_WIN32 && !_WIN64
	#include <idn2.h>
#endif

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён плейсхолдеров
 */
using namespace placeholders;

/**
 * Если стандартные DNS-серверы IPv4 не установлены
 */
#ifndef AWH_IPV4_RESOLVERS
	/**
	 * Устанавливаем стандартные DNS-серверы IPv4
	 */
	#define AWH_IPV4_RESOLVERS { \
		"8.8.8.8", \
		"8.8.4.4", \
		"1.1.1.1", \
		"1.0.0.1" \
		"77.88.8.8" \
		"77.88.8.1" \
	}
#endif

/**
 * Если стандартные DNS-серверы IPv6 не установлены
 */
#ifndef AWH_IPV6_RESOLVERS
	/**
	 * Устанавливаем стандартные DNS-серверы IPv6
	 */
	#define AWH_IPV6_RESOLVERS { \
		"[2001:4860:4860::8888]", \
		"[2001:4860:4860::8844]", \
		"[2606:4700:4700::1111]", \
		"[2606:4700:4700::1001]" \
		"[2a02:6b8::feed:0ff]", \
		"[2a02:6b8:0:1::feed:0ff]" \
	}
#endif

/**
 * Инкапсулируем статические типы данных в пространство имён
 */
namespace {
	/**
	 * Подписываемся на пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Структура записи IP-адреса принадлежащего домену
	 *
	 */
	struct RecordIP {
		// Время завершения жизни
		uint64_t life;
		// IP-адрес доменного имени
		unique_ptr <net::addr_t> ip;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit RecordIP() noexcept : life(0), ip(nullptr) {}
	};

	/**
	 * @brief Структура записи доменного имени
	 *
	 */
	struct RecordDomain {
		// Время завершения жизни
		uint64_t life;
		// Доменное имя записи
		string domain;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit RecordDomain() noexcept : life(0), domain{""} {}
	};

	/**
	 * @brief Хэш-функция для IPv6 ключа
	 *
	 * Использует FNV-1a алгоритм — быстрый и с хорошим распределением
	 */
	struct IpV6Hash {
		/**
		 * @brief Оператор генерации числового хэша ключа
		 *
		 * @param key ключ для которого необходима генерация
		 * @return    сгенерированный хэш ключа
		 */
		uint64_t operator()(const array <uint8_t, 16> & key) const noexcept {
			// FNV-1a 64-bit constants
			uint64_t result = 14695981039346656037ULL; // FNV offset basis
			// Выполняем перебор всех байт ключа
			for(uint8_t byte : key){
				// Выполняем инвертирование байт ключа
				result ^= static_cast <uint64_t> (byte);
				// Выполняем компенсацию
				result *= 1099511628211ULL; // FNV prime
			}
			// Выводим результат
			return result;
		}
	};

	/**
	 * @brief Хэш-функция для ключа доменного имени
	 *
	 * Использует FNV-1a алгоритм — быстрый и с хорошим распределением
	 */
	struct DomainHash {
		/**
		 * @brief Оператор генерации числового хэша доменного имени
		 *
		 * @param domain доменное имя для которого необходима генерация
		 * @return       сгенерированный хэш доменного имени
		 */
		uint64_t operator()(string_view domain) const noexcept {
			// FNV-1a 64-bit constants
			uint64_t result = 14695981039346656037ULL; // FNV offset basis
			// Выполняем перебор всех байт доменного имени
			for(char c : domain){
				// Приводим к lowercase на лету (если ещё не нормализовали)
				char lower = static_cast <char> (::tolower(static_cast<uint8_t>(c)));
				// Выполняем инвертирование байт доменного имени
				result ^= static_cast <uint64_t> (lower);
				// Выполняем компенсацию
				result *= 1099511628211ULL; // FNV prime
			}
			// Выводим результат
			return result;
		}
	};

	/**
	 * @brief Объект работы с кэшированием
	 *
	 */
	struct Cache {
		// Мютекс для блокировки потока
		lock_state_t <std::shared_mutex> mtx;
		// Список IPv4-адресов с доменными именами
		unordered_map <uint32_t, vector <RecordDomain>> ipv4;
		// Список IPv6-адресов с доменными именами
		unordered_map <array <uint8_t, 16>, vector <RecordDomain>, IpV6Hash> ipv6;
		// Список доменных имён с IP-адресами
		unordered_map <string_view, vector <RecordIP>, DomainHash> domains;
	} __awh_cache__;

	/**
	 * @brief Структура резолвера DNS
	 *
	 */
	struct Resolver {
		// Префикс для переменных окружения
		string prefix;
		// Мютекс для блокировки потока
		lock_state_t <std::mutex> mtx;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Resolver() noexcept : prefix{""} {}
	} __awh_resolver__;
};

/**
 * Инкапсулируем статические типы данных в пространство имён
 */
namespace {
	/**
	 * Подписываемся на пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Генератор случайных чисел для рандомизации DNS-серверов
	 *
	 */
	random_device __awh_randev__;
	/**
	 * @brief Мютекс для блокировки потока
	 *
	 */
	lock_state_t <std::mutex> __awh_mtx__;
	/**
	 * @brief Режим безопасности работы потоков
	 *
	 */
	event::mode_t __awh_thread_safety__ = event::mode_t::DISABLED;
};

/**
 * @brief Метод уничтожения события DNS-резолвера
 *
 * @param eid идентификатор события DNS-резолвера
 */
void awh::unit::DNS::destroy(const event::id_t eid) noexcept {
	// Выполняем блокировку потокв для уничтожения события DNS-резолвера
	const locker_t <> lock(::__awh_resolver__.mtx);
	// Удаляем событие уведомителя
	this->_io->destroy(eid);
}
/**
 * @brief Метод создания события DNS-резолвера
 *
 * @param family семейстов IP-адресов IPv4/IPv6
 * @return       идентификатор события DNS-резолвера
 */
awh::event::id_t awh::unit::DNS::create(const event::family_t family) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потокв для создания события DNS-резолвера
		const locker_t <> lock(::__awh_resolver__.mtx);
		// Добавляем новое событие клиента UDP
		const event::id_t result = this->_io->event(event::node_t::CLIENT, family, event::type_t::DATAGRAM, event::protocol_t::UDP);
		// Устанавливаем порт события
		this->_io->setPort(result, 53);
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Если префикс для переменных окружения установлен
				if(!::__awh_resolver__.prefix.empty()){
					// Получаем значение переменной
					const char * env = ::getenv(this->_fmk->format("%s_DNS_IPV4_SERVER", ::__awh_resolver__.prefix.c_str()).c_str());
					// Если IP-адрес из переменной окружения получен
					if(env != nullptr)
						// Устанавливаем адрес сервера назначения
						this->_io->setTarget(result, env);
				// Если префикс для переменных окружения не установлен
				} else {
					// Создаём массив стандартных DNS-серверов IPv4
					const array <string_view, 6> resolvers = AWH_IPV4_RESOLVERS;
					// Выбираем случайный DNS-сервер из массива
					const string_view resolver = resolvers[::__awh_randev__() % resolvers.size()];
					// Устанавливаем адрес сервера назначения
					this->_io->setTarget(result, resolver.data());
				}
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Если префикс для переменных окружения установлен
				if(!::__awh_resolver__.prefix.empty()){
					// Получаем значение переменной
					const char * env = ::getenv(this->_fmk->format("%s_DNS_IPV6_SERVER", ::__awh_resolver__.prefix.c_str()).c_str());
					// Если IP-адрес из переменной окружения получен
					if(env != nullptr)
						// Устанавливаем адрес сервера назначения
						this->_io->setTarget(result, env);
				// Если префикс для переменных окружения не установлен
				} else {
					// Создаём массив стандартных DNS-серверов IPv6
					const array <string_view, 6> resolvers = AWH_IPV6_RESOLVERS;
					// Выбираем случайный DNS-сервер из массива
					const string_view resolver = resolvers[::__awh_randev__() % resolvers.size()];
					// Устанавливаем адрес сервера назначения
					this->_io->setTarget(result, resolver.data());
				}
			} break;
		}
		// Выводим результат
		return result;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки безопасности работы потоков
 *
 * @param mode флаг режима безопасности потоков
 */
void awh::unit::DNS::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасности работы потоков
	::__awh_thread_safety__ = (mode ? event::mode_t::ENABLED : event::mode_t::DISABLED);
	// Активируем работу мьютекса блокировки потока при работе с кэшем
	::__awh_cache__.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Активируем работу мьютекса блокировки потока при работе с резолвером
	::__awh_resolver__.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
}
/**
 * @brief Метод кодирования интернационального доменного имени
 *
 * @param domain доменное имя для кодирования
 * @return       результат работы кодирования
 */
string awh::unit::DNS::encode(string_view domain) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если доменное имя передано
		if(!std::empty(domain) && (domain.front() != '-') && (domain.back() != '-')){
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Результирующий буфер данных
				wchar_t buffer[0xff];
				// Выполняем кодирования доменного имени
				if(::IdnToAscii(0, this->_fmk->convert(domain.data()).c_str(), -1, buffer, sizeof(buffer)) == 0){
					// Создаём буфер сообщения ошибки
					wchar_t message[0xff] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xff, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain), log_t::flag_t::CRITICAL, ::convert(message).c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
					#endif
				// Получаем результат кодирования
				} else result = this->_fmk->convert(wstring{buffer});
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#elif AWH_IDN
				// Результирующий буфер данных
				char * buffer = nullptr;
				// Выполняем кодирования доменного имени
				const int32_t rc = ::idn2_to_ascii_8z(domain.data(), &buffer, IDN2_NONTRANSITIONAL);
				// Если кодирование не выполнено
				if(rc != IDNA_SUCCESS){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain), log_t::flag_t::CRITICAL, ::idn2_strerror(rc));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::idn2_strerror(rc));
					#endif
				// Получаем результат кодирования
				} else result = buffer;
				// Если память была выделена
				if(buffer != nullptr)
					// Очищаем буфер данных
					::free(buffer);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод декодирования интернационального доменного имени
 *
 * @param domain доменное имя для декодирования
 * @return       результат работы декодирования
 */
string awh::unit::DNS::decode(string_view domain) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если доменное имя передано
		if(!std::empty(domain) && (domain.front() != '-') && (domain.back() != '-')){
			/**
			 * Для операционной системы MS Windows
			 */
			#if _WIN32 || _WIN64
				// Результирующий буфер данных
				wchar_t buffer[0xff];
				// Выполняем кодирования доменного имени
				if(::IdnToUnicode(0, this->_fmk->convert(domain.data()).c_str(), -1, buffer, sizeof(buffer)) == 0){
					// Создаём буфер сообщения ошибки
					wchar_t message[0xff] = {0};
					// Выполняем формирование текста ошибки
					::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, ::WSAGetLastError(), 0, message, 0xff, 0);
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain), log_t::flag_t::CRITICAL, ::convert(message).c_str());
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::convert(message).c_str());
					#endif
				// Получаем результат кодирования
				} else result = this->_fmk->convert(wstring{buffer});
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#elif AWH_IDN
				// Результирующий буфер данных
				char * buffer = nullptr;
				// Выполняем декодирования доменного имени
				const int32_t rc = ::idn2_to_unicode_8z8z(domain.data(), &buffer, 0);
				// Если кодирование не выполнено
				if(rc != IDNA_SUCCESS){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain), log_t::flag_t::CRITICAL, ::idn2_strerror(rc));
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, ::idn2_strerror(rc));
					#endif
				// Получаем результат декодирования
				} else result = buffer;
				// Если память была выделена
				if(buffer != nullptr)
					// Очищаем буфер данных
					::free(buffer);
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения порта события
 *
 * @param eid идентификатор события
 * @return    порт события
 */
uint16_t awh::unit::DNS::getPort(const event::id_t eid) const noexcept {
	// Получаем порт события
	return this->_io->getPort(eid);
}
/**
 * @brief Метод установки порта события
 *
 * @param eid  идентификатор события
 * @param port порт события
 * @return     результат выполнения установки
 */
bool awh::unit::DNS::setPort(const event::id_t eid, const uint16_t port) noexcept {
	// Выполняем блокировку потокв для установки порта события
	const locker_t <> lock(::__awh_resolver__.mtx);
	// Устанавливаем порт события
	return this->_io->setPort(eid, port);
}
/**
 * @brief Метод установки времени ожидания выполнения запроса
 *
 * @param eid     идентификатор события DNS-резолвера
 * @param timeout значение таймаута в миллисекундах
 */
void awh::unit::DNS::setTimeout(const event::id_t eid, const uint32_t timeout) noexcept {
	// Выполняем блокировку потокв для установки таймаута события
	const locker_t <> lock(::__awh_resolver__.mtx);
	// Устанавливаем время ожидания выполнения запроса
	this->_io->setTimeout(eid, event::action_t::READ, timeout);
}
/**
 * @brief Метод пересортировки адресов в кэше для доменного имени
 *
 * @param family семейстов IP-адресов IPv4/IPv6
 * @param domain доменное имя соответствующее IP-адресу
 */
void awh::unit::DNS::shuffle(const event::family_t family, string_view domain) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потокв для работы с кэшем
		const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Если доменное имя передано
		if(!std::empty(domain)){
			// Выполняем поиск доменного имени в кэше
			auto i = ::__awh_cache__.domains.find(domain);
			// Если в кэше доменное имя найдено
			if(i != ::__awh_cache__.domains.end()){
				// Выбираем стаднарт рандомайзера
				mt19937 generator(::__awh_randev__());
				// Выполняем рандомную сортировку списка DNS-серверов
				::shuffle(i->second.begin(), i->second.end(), generator);
			}
		}
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Если список IPv4-адресов не пустой
				if(!::__awh_cache__.ipv4.empty()){
					// Выбираем стаднарт рандомайзера
					mt19937 generator(::__awh_randev__());
					/**
					 * Выполняем перебор всех IPv4-адресов в кэше
					 */
					for(auto i = ::__awh_cache__.ipv4.begin(); i != ::__awh_cache__.ipv4.end(); ++i)
						// Выполняем рандомную сортировку списка доменных имён
						::shuffle(i->second.begin(), i->second.end(), generator);
				}
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Если список IPv6-адресов не пустой
				if(!::__awh_cache__.ipv6.empty()){
					// Выбираем стаднарт рандомайзера
					mt19937 generator(::__awh_randev__());
					/**
					 * Выполняем перебор всех IPv6-адресов в кэше
					 */
					for(auto i = ::__awh_cache__.ipv6.begin(); i != ::__awh_cache__.ipv6.end(); ++i)
						// Выполняем рандомную сортировку списка доменных имён
						::shuffle(i->second.begin(), i->second.end(), generator);
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод очистки кэша
 *
 */
void awh::unit::DNS::clearCache() noexcept {
	// Выполняем блокировку потокв для работы с кэшем
	const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
	// Если в кэше есть IPv4-адреса
	if(!::__awh_cache__.ipv4.empty())
		// Выполняем удаление всех IPv4-адресов из кэша
		::__awh_cache__.ipv4.clear();
	// Если в кэше есть IPv6-адреса
	if(!::__awh_cache__.ipv6.empty())
		// Выполняем удаление всех IPv6-адресов из кэша
		::__awh_cache__.ipv6.clear();
	// Если в кэше есть доменные имена
	if(!::__awh_cache__.domains.empty())
		// Выполняем удаление всех доменных имён из кэша
		::__awh_cache__.domains.clear();
}
/**
 * @brief Метод очистки кэша
 *
 * @param family семейстов IP-адресов IPv4/IPv6
 */
void awh::unit::DNS::clearCache(const event::family_t family) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потокв для работы с кэшем
		const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
		// Если в кэше есть доменные имена
		if(!::__awh_cache__.domains.empty()){
			/**
			 * Выполняем перебор всех доменных имён в кэше
			 */
			for(auto i = ::__awh_cache__.domains.begin(); i != ::__awh_cache__.domains.end();){
				// Выполняем перебор всех записей доменного имени
				for(auto j = i->second.begin(); j != i->second.end();){
					/**
					 * Определяем семейство события
					 */
					switch(static_cast <uint8_t> (family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Если IP-адрес доменного имени является IPv4, то удаляем его из кэша
							if(j->ip->size == 4)
								// Удаляем запись доменного имени из кэша
								j = i->second.erase(j);
							// Если IP-адрес доменного имени является IPv6, то пропускаем его
							else ++j;
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Если IP-адрес доменного имени является IPv6, то удаляем его из кэша
							if(j->ip->size == 16)
								// Удаляем запись доменного имени из кэша
								j = i->second.erase(j);
							// Если IP-адрес доменного имени является IPv4, то пропускаем его
							else ++j;
						} break;
						// Если это какое-то другое семейство, то выходим из функции
						default: return;
					}
				}
				// Если после удаления всех записей доменного имени в кэше не осталось, то удаляем его из кэша
				if(i->second.empty())
					// Удаляем доменное имя из кэша
					i = ::__awh_cache__.domains.erase(i);
				// Если у доменного имени остались записи, то пропускаем его
				else ++i;
			}
		}
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Если в кэше есть IPv4-адреса
				if(!::__awh_cache__.ipv4.empty())
					// Выполняем удаление всех IPv4-адресов из кэша
					::__awh_cache__.ipv4.clear();
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Если в кэше есть IPv6-адреса
				if(!::__awh_cache__.ipv6.empty())
					// Выполняем удаление всех IPv6-адресов из кэша
					::__awh_cache__.ipv6.clear();
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод очистки кэша для указанного доменного имени
 *
 * @param domain доменное имя для которого выполняется очистка кэша
 */
void awh::unit::DNS::clearCache(string_view domain) noexcept {
	// Если доменное имя передано
	if(!std::empty(domain)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потокв для работы с кэшем
			const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем поиск доменного имени в кэше
			auto i = ::__awh_cache__.domains.find(domain);
			// Если в кэше доменное имя найдено
			if(i != ::__awh_cache__.domains.end())
				// Удаляем доменное имя из кэша
				::__awh_cache__.domains.erase(i);
			// Если список IPv4-адресов не пустой
			if(!::__awh_cache__.ipv4.empty()){
				/**
				 * Выполняем перебор всех IPv4-адресов в кэше
				 */
				for(auto i = ::__awh_cache__.ipv4.begin(); i != ::__awh_cache__.ipv4.end();){
					// Выполняем перебор всех доменных имён IPv4-адреса
					for(auto j = i->second.begin(); j != i->second.end();){
						// Если доменное имя соответствует удаляемому, то удаляем его из кэша
						if(this->_fmk->compare(domain.data(), j->domain.data()))
							// Удаляем доменное имя из кэша
							j = i->second.erase(j);
						// Если доменное имя не соответствует удаляемому, то пропускаем его
						else ++j;
					}
					// Если после удаления всех доменных имён у IPv4-адреса не осталось доменных имён, то удаляем его из кэша
					if(i->second.empty())
						// Удаляем IPv4-адрес из кэша
						i = ::__awh_cache__.ipv4.erase(i);
					// Если у IPv4-адреса остались доменные имена, то пропускаем его
					else ++i;
				}
			}
			// Если список IPv6-адресов не пустой
			if(!::__awh_cache__.ipv6.empty()){
				/**
				 * Выполняем перебор всех IPv6-адресов в кэше
				 */
				for(auto i = ::__awh_cache__.ipv6.begin(); i != ::__awh_cache__.ipv6.end();){
					// Выполняем перебор всех доменных имён IPv6-адреса
					for(auto j = i->second.begin(); j != i->second.end();){
						// Если доменное имя соответствует удаляемому, то удаляем его из кэша
						if(this->_fmk->compare(domain.data(), j->domain.data()))
							// Удаляем доменное имя из кэша
							j = i->second.erase(j);
						// Если доменное имя не соответствует удаляемому, то пропускаем его
						else ++j;
					}
					// Если после удаления всех доменных имён у IPv6-адреса не осталось доменных имён, то удаляем его из кэша
					if(i->second.empty())
						// Удаляем IPv6-адрес из кэша
						i = ::__awh_cache__.ipv6.erase(i);
					// Если у IPv6-адреса остались доменные имена, то пропускаем его
					else ++i;
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод очистки кэша для указанного доменного имени
 *
 * @param family семейстов IP-адресов IPv4/IPv6
 * @param domain доменное имя для которого выполняется очистка кэша
 */
void awh::unit::DNS::clearCache(const event::family_t family, string_view domain) noexcept {
	// Если доменное имя передано
	if(!std::empty(domain)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потокв для работы с кэшем
			const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем поиск доменного имени в кэше
			auto i = ::__awh_cache__.domains.find(domain);
			// Если в кэше доменное имя найдено
			if(i != ::__awh_cache__.domains.end()){
				/**
				 * Выполняем перебор всех записей доменного имени
				 */
				for(auto j = i->second.begin(); j != i->second.end();){
					/**
					 * Определяем семейство события
					 */
					switch(static_cast <uint8_t> (family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Если IP-адрес доменного имени является IPv4
							if(j->ip->size == 4)
								// Удаляем запись доменного имени из кэша
								j = i->second.erase(j);
							// Если IP-адрес доменного имени является IPv6, то пропускаем его
							else ++j;
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Если IP-адрес доменного имени является IPv6
							if(j->ip->size == 16)
								// Удаляем запись доменного имени из кэша
								j = i->second.erase(j);
							// Если IP-адрес доменного имени является IPv4, то пропускаем его
							else ++j;
						} break;
						// Если это какое-то другое семейство, то выходим из функции
						default: return;
					}
				}
				// Если после удаления всех записей доменного имени в кэше не осталось, то удаляем его из кэша
				if(i->second.empty())
					// Удаляем доменное имя из кэша
					::__awh_cache__.domains.erase(i);
			}
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Если список IPv4-адресов не пустой
					if(!::__awh_cache__.ipv4.empty()){
						/**
						 * Выполняем перебор всех IPv4-адресов в кэше
						 */
						for(auto i = ::__awh_cache__.ipv4.begin(); i != ::__awh_cache__.ipv4.end();){
							// Выполняем перебор всех доменных имён IPv4-адреса
							for(auto j = i->second.begin(); j != i->second.end();){
								// Если доменное имя соответствует удаляемому, то удаляем его из кэша
								if(this->_fmk->compare(domain.data(), j->domain.data()))
									// Удаляем доменное имя из кэша
									j = i->second.erase(j);
								// Если доменное имя не соответствует удаляемому, то пропускаем его
								else ++j;
							}
							// Если после удаления всех доменных имён у IPv4-адреса не осталось доменных имён, то удаляем его из кэша
							if(i->second.empty())
								// Удаляем IPv4-адрес из кэша
								i = ::__awh_cache__.ipv4.erase(i);
							// Если у IPv4-адреса остались доменные имена, то пропускаем его
							else ++i;
						}
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Если список IPv6-адресов не пустой
					if(!::__awh_cache__.ipv6.empty()){
						/**
						 * Выполняем перебор всех IPv6-адресов в кэше
						 */
						for(auto i = ::__awh_cache__.ipv6.begin(); i != ::__awh_cache__.ipv6.end();){
							// Выполняем перебор всех доменных имён IPv6-адреса
							for(auto j = i->second.begin(); j != i->second.end();){
								// Если доменное имя соответствует удаляемому, то удаляем его из кэша
								if(this->_fmk->compare(domain.data(), j->domain.data()))
									// Удаляем доменное имя из кэша
									j = i->second.erase(j);
								// Если доменное имя не соответствует удаляемому, то пропускаем его
								else ++j;
							}
							// Если после удаления всех доменных имён у IPv6-адреса не осталось доменных имён, то удаляем его из кэша
							if(i->second.empty())
								// Удаляем IPv6-адрес из кэша
								i = ::__awh_cache__.ipv6.erase(i);
							// Если у IPv6-адреса остались доменные имена, то пропускаем его
							else ++i;
						}
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод получения IP-адреса из кэша
 *
 * @param family семейстов IP-адресов IPv4/IPv6
 * @param domain доменное имя соответствующее IP-адресу
 * @return       IP-адрес находящийся в кэше
 */
string awh::unit::DNS::getFromCache(const event::family_t family, string_view domain) noexcept {
	// Если доменное имя передано
	if(!std::empty(domain)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потокв для работы с кэшем
			const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Выполняем поиск доменного имени в кэше
			auto i = ::__awh_cache__.domains.find(domain);
			// Если в кэше доменное имя найдено
			if(i != ::__awh_cache__.domains.end()){
				// Получаем текущую метку времени
				const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
				/**
				 * Выполняем перебор всех записей доменного имени
				 */
				for(const auto & record : i->second){
					/**
					 * Определяем семейство события
					 */
					switch(static_cast <uint8_t> (family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Если IP-адрес доменного имени является IPv4
							if((record.ip->size == 4) && (now < record.life)){
								// Выполняем блокировку потокв для парсинга IP-адреса
								const locker_t <> lock(::__awh_mtx__);
								// Устанавливаем полученный IP-адрес
								this->_addr.source(record.ip, net_addr_t::endian_t::LITTLE);
								// Выводим результат
								return static_cast <string> (this->_addr);
							}
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Если IP-адрес доменного имени является IPv6
							if((record.ip->size == 16) && (now < record.life)){
								// Выполняем блокировку потокв для парсинга IP-адреса
								const locker_t <> lock(::__awh_mtx__);
								// Устанавливаем полученный IP-адрес
								this->_addr.source(record.ip, net_addr_t::endian_t::LITTLE);
								// Выводим результат
								return static_cast <string> (this->_addr);
							}
						} break;
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод получения IP-адреса из кэша
 *
 * @param family семейстов IP-адресов IPv4/IPv6
 * @param domain доменное имя соответствующее IP-адресу
 * @param value  IP-адрес находящийся в кэше
 * @return       результат выполнения операции
 */
bool awh::unit::DNS::getFromCache(const event::family_t family, string_view domain, unique_ptr <net::addr_t> & value) noexcept {
	// Результат работы функции
	bool result = false;
	// Если доменное имя передано
	if(!std::empty(domain)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потокв для работы с кэшем
			const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::SHARED);
			// Выполняем поиск доменного имени в кэше
			auto i = ::__awh_cache__.domains.find(domain);
			// Если в кэше доменное имя найдено
			if(i != ::__awh_cache__.domains.end()){
				// Получаем текущую метку времени
				const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
				/**
				 * Выполняем перебор всех записей доменного имени
				 */
				for(const auto & record : i->second){
					/**
					 * Определяем семейство события
					 */
					switch(static_cast <uint8_t> (family)){
						// Для семейства IPv4
						case static_cast <uint8_t> (event::family_t::IPV4): {
							// Если IP-адрес доменного имени является IPv4
							if((result = ((record.ip->size == 4) && (now < record.life)))){
								// Если объект результата не инициализирован
								if(value == nullptr)
									// Инициализируем объект результата
									value = make_unique <net::addr_net_ipv4_t> ();
								// Устанавливаем IP-адрес
								awh_cast <net::addr_net_ipv4_t *> (value.get())->address = awh_cast <net::addr_net_ipv4_t *> (record.ip.get())->address;
								// Выводим результат
								return result;
							}
						} break;
						// Для семейства IPv6
						case static_cast <uint8_t> (event::family_t::IPV6): {
							// Если IP-адрес доменного имени является IPv6
							if((result = ((record.ip->size == 16) && (now < record.life)))){
								// Если объект результата не инициализирован
								if(value == nullptr)
									// Инициализируем объект результата
									value = make_unique <net::addr_net_ipv6_t> ();
								// Устанавливаем IP-адрес
								::memcpy(&awh_cast <net::addr_net_ipv6_t *> (value.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (record.ip.get())->address[0], 16);
								// Выводим результат
								return result;
							}
						} break;
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
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (family), domain), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод добавления IP-адреса в кэш
 *
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления к кэш
 * @param ttl    время жизни кэша доменного имени (в секундах)
 */
void awh::unit::DNS::addToCache(string_view domain, string_view ip, const uint32_t ttl) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!std::empty(domain) && !std::empty(ip)){
		// Выполняем блокировку потокв для парсинга IP-адреса
		const locker_t <> lock(::__awh_mtx__);
		// Выполняем парсинг IPv4-адреса
		if(this->_addr.parse(ip.data())){
			// Получаем IP-адрес в исходном виде
			auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
			// Выполняем добавление записи в кэш
			this->addToCache(domain, ip, ttl);
		}
	}
}
/**
 * @brief Метод добавления IP-адреса в кэш
 *
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления к кэш
 * @param ttl    время жизни кэша доменного имени (в секундах)
 */
void awh::unit::DNS::addToCache(string_view domain, const unique_ptr <net::addr_t> & ip, const uint32_t ttl) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!std::empty(domain) && (ip != nullptr)){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем блокировку потокв для работы с кэшем
			const locker_t <std::shared_mutex> lock(::__awh_cache__.mtx, locker_t <std::shared_mutex>::mode_t::EXCLUSIVE);
			// Выполняем поиск доменного имени в кэше
			auto i = ::__awh_cache__.domains.find(domain);
			// Если в кэше доменное имя найдено
			if(i != ::__awh_cache__.domains.end()){
				// Создаём объект записи
				RecordIP record;
				// Если время жизни кэша установлено
				if(ttl > 0)
					// Устанавливаем время жизни
					record.life = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + static_cast <uint64_t>  (ttl * 1000));
				/**
				 * Определяем тип адреса
				 */
				switch(ip->size){
					// Если адрес является IPv4
					case 4: {
						// Выполняем инициализацию объекта IP-адреса
						record.ip = make_unique <net::addr_net_ipv4_t> ();
						// Устанавливаем IP-адрес
						awh_cast <net::addr_net_ipv4_t *> (record.ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (ip.get())->address;
						// Выполняем поиск IP-адреса
						auto i = ::__awh_cache__.ipv4.find(awh_cast <net::addr_net_ipv4_t *> (record.ip.get())->address);
						// Если IP-адрес найден в кэше
						if(i != ::__awh_cache__.ipv4.end()){
							// Создаём объект записи
							RecordDomain record;
							// Устанавливаем доменное имя
							record.domain = domain;
							// Если время жизни кэша установлено
							if(ttl > 0)
								// Устанавливаем время жизни
								record.life = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + static_cast <uint64_t>  (ttl * 1000));
							// Выполняем добавление IP-адреса
							i->second.push_back(::move(record));
						// Если IP-адрес не найден в кэше
						} else {
							// Создаём список записей IP-адресов
							vector <RecordDomain> records(1);
							// Устанавливаем доменное имя
							records.back().domain = domain;
							// Устанавливаем время жизни
							records.back().life = record.life;
							// Добавляем новую запись в кэш IP-адресов
							::__awh_cache__.ipv4.emplace(awh_cast <net::addr_net_ipv4_t *> (record.ip.get())->address, ::move(records));
						}
					} break;
					// Если адрес является IPv6
					case 16: {
						// Выполняем инициализацию объекта IP-адреса
						record.ip = make_unique <net::addr_net_ipv6_t> ();
						// Устанавливаем IP-адрес
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (record.ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], 16);
						// Выполняем поиск IP-адреса
						auto i = ::__awh_cache__.ipv6.find(awh_cast <net::addr_net_ipv6_t *> (record.ip.get())->address);
						// Если IP-адрес найден в кэше
						if(i != ::__awh_cache__.ipv6.end()){
							// Создаём объект записи
							RecordDomain record;
							// Устанавливаем доменное имя
							record.domain = domain;
							// Если время жизни кэша установлено
							if(ttl > 0)
								// Устанавливаем время жизни
								record.life = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + static_cast <uint64_t>  (ttl * 1000));
							// Выполняем добавление IP-адреса
							i->second.push_back(::move(record));
						// Если IP-адрес не найден в кэше
						} else {
							// Создаём список записей IP-адресов
							vector <RecordDomain> records(1);
							// Устанавливаем доменное имя
							records.back().domain = domain;
							// Устанавливаем время жизни
							records.back().life = record.life;
							// Добавляем новую запись в кэш IP-адресов
							::__awh_cache__.ipv6.emplace(awh_cast <net::addr_net_ipv6_t *> (record.ip.get())->address, ::move(records));
						}
					} break;
				}
				// Выполняем добавление IP-адреса
				i->second.push_back(::move(record));
			// Если к хэше доменное имя не найдено
			} else {
				// Создаём список записей IP-адресов
				vector <RecordIP> records(1);
				// Устанавливаем время жизни
				records.back().life = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + static_cast <uint64_t>  (ttl * 1000));
				/**
				 * Определяем тип адреса
				 */
				switch(ip->size){
					// Если адрес является IPv4
					case 4: {
						// Выполняем инициализацию объекта IP-адреса
						records.back().ip = make_unique <net::addr_net_ipv4_t> ();
						// Устанавливаем IP-адрес
						awh_cast <net::addr_net_ipv4_t *> (records.back().ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (ip.get())->address;
						// Выполняем поиск IP-адреса
						auto i = ::__awh_cache__.ipv4.find(awh_cast <net::addr_net_ipv4_t *> (ip.get())->address);
						// Если IP-адрес найден в кэше
						if(i != ::__awh_cache__.ipv4.end()){
							// Создаём объект записи
							RecordDomain record;
							// Устанавливаем доменное имя
							record.domain = domain;
							// Если время жизни кэша установлено
							if(ttl > 0)
								// Устанавливаем время жизни
								record.life = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + static_cast <uint64_t>  (ttl * 1000));
							// Выполняем добавление IP-адреса
							i->second.push_back(::move(record));
						// Если IP-адрес не найден в кэше
						} else {
							// Создаём список записей IP-адресов
							vector <RecordDomain> records(1);
							// Устанавливаем доменное имя
							records.back().domain = domain;
							// Если время жизни кэша установлено
							if(ttl > 0)
								// Устанавливаем время жизни
								records.back().life = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + static_cast <uint64_t>  (ttl * 1000));
							// Добавляем новую запись в кэш IP-адресов
							::__awh_cache__.ipv4.emplace(awh_cast <net::addr_net_ipv4_t *> (ip.get())->address, ::move(records));
						}
					} break;
					// Если адрес является IPv6
					case 16: {
						// Выполняем инициализацию объекта IP-адреса
						records.back().ip = make_unique <net::addr_net_ipv6_t> ();
						// Устанавливаем IP-адрес
						::memcpy(&awh_cast <net::addr_net_ipv6_t *> (records.back().ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], 16);
						// Выполняем поиск IP-адреса
						auto i = ::__awh_cache__.ipv6.find(awh_cast <net::addr_net_ipv6_t *> (ip.get())->address);
						// Если IP-адрес найден в кэше
						if(i != ::__awh_cache__.ipv6.end()){
							// Создаём объект записи
							RecordDomain record;
							// Устанавливаем доменное имя
							record.domain = domain;
							// Если время жизни кэша установлено
							if(ttl > 0)
								// Устанавливаем время жизни
								record.life = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + static_cast <uint64_t>  (ttl * 1000));
							// Выполняем добавление IP-адреса
							i->second.push_back(::move(record));
						// Если IP-адрес не найден в кэше
						} else {
							// Создаём список записей IP-адресов
							vector <RecordDomain> records(1);
							// Устанавливаем доменное имя
							records.back().domain = domain;
							// Если время жизни кэша установлено
							if(ttl > 0)
								// Устанавливаем время жизни
								records.back().life = (this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS) + static_cast <uint64_t>  (ttl * 1000));
							// Добавляем новую запись в кэш IP-адресов
							::__awh_cache__.ipv6.emplace(awh_cast <net::addr_net_ipv6_t *> (ip.get())->address, ::move(records));
						}
					} break;
				}
				// Добавляем новую запись в кэш доменных имён
				::__awh_cache__.domains.emplace(domain, ::move(records));
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(domain, ttl), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод добавления IP-адреса в кэш
 *
 * @param family семейстов IP-адресов IPv4/IPv6
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления к кэш
 * @param ttl    время жизни кэша доменного имени (в секундах)
 */
void awh::unit::DNS::addToCache(const event::family_t family, string_view domain, string_view ip, const uint32_t ttl) noexcept {
	// Если доменное имя и IP-адрес переданы
	if(!std::empty(domain) && !std::empty(ip)){
		/**
		 * Определяем семейство события
		 */
		switch(static_cast <uint8_t> (family)){
			// Для семейства IPv4
			case static_cast <uint8_t> (event::family_t::IPV4): {
				// Выполняем блокировку потокв для парсинга IP-адреса
				const locker_t <> lock(::__awh_mtx__);
				// Выполняем парсинг IPv4-адреса
				if(this->_addr.parse(ip.data(), net_addr_t::type_t::IPV4)){
					// Получаем IP-адрес в исходном виде
					auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
					// Выполняем добавление записи в кэш
					this->addToCache(domain, ip, ttl);
				}
			} break;
			// Для семейства IPv6
			case static_cast <uint8_t> (event::family_t::IPV6): {
				// Выполняем блокировку потокв для парсинга IP-адреса
				const locker_t <> lock(::__awh_mtx__);
				// Выполняем парсинг IPv6-адреса
				if(this->_addr.parse(ip.data(), net_addr_t::type_t::IPV6)){
					// Получаем IP-адрес в исходном виде
					auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
					// Выполняем добавление записи в кэш
					this->addToCache(domain, ip, ttl);
				}
			} break;
		}
	}
}
/**
 * @brief Метод очистки чёрного списка
 *
 * @param domain доменное имя для которого очищается чёрный список
 */
void awh::unit::DNS::clearBlacklist(string_view domain) noexcept {

}
/**
 * @brief Метод очистки чёрного списка
 *
 * @param family семейстов IP-адресов IPv4/IPv6
 * @param domain доменное имя для которого очищается чёрный список
 */
void awh::unit::DNS::clearBlacklist(const event::family_t family, string_view domain) noexcept {

}
/**
 * @brief Метод удаления IP-адреса из чёрного списока
 *
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::unit::DNS::delInBlacklist(string_view domain, string_view ip) noexcept {

}
/**
 * @brief Метод удаления IP-адреса из чёрного списока
 *
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::unit::DNS::delInBlacklist(string_view domain, const unique_ptr <net::addr_t> & ip) noexcept {

}
/**
 * @brief Метод удаления IP-адреса из чёрного списока
 *
 * @param family семейстов IP-адресов IPv4/IPv6
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::unit::DNS::delInBlacklist(const event::family_t family, string_view domain, string_view ip) noexcept {

}
/**
 * @brief Метод добавления IP-адреса в чёрный список
 *
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в чёрный список
 */
void awh::unit::DNS::addToBlacklist(string_view domain, string_view ip) noexcept {

}
/**
 * @brief Метод добавления IP-адреса в чёрный список
 *
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в чёрный список
 */
void awh::unit::DNS::addToBlacklist(string_view domain, const unique_ptr <net::addr_t> & ip) noexcept {

}
/**
 * @brief Метод добавления IP-адреса в чёрный список
 *
 * @param family семейстов IP-адресов IPv4/IPv6
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в чёрный список
 */
void awh::unit::DNS::addToBlacklist(const event::family_t family, string_view domain, string_view ip) noexcept {

}
/**
 * @brief Метод проверки наличия IP-адреса в чёрном списке
 *
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для проверки наличия в чёрном списке
 * @return       результат проверки наличия IP-адреса в чёрном списке
 */
bool awh::unit::DNS::hasInBlacklist(string_view domain, string_view ip) const noexcept {

}
/**
 * @brief Метод проверки наличия IP-адреса в чёрном списке
 *
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для проверки наличия в чёрном списке
 * @return       результат проверки наличия IP-адреса в чёрном списке
 */
bool awh::unit::DNS::hasInBlacklist(string_view domain, const unique_ptr <net::addr_t> & ip) const noexcept {

}
/**
* @brief Метод проверки наличия IP-адреса в чёрном списке
*
* @param family семейстов IP-адресов IPv4/IPv6
* @param domain доменное имя соответствующее IP-адресу
* @param ip     адрес для проверки наличия в чёрном списке
* @return       результат проверки наличия IP-адреса в чёрном списке
*/
bool awh::unit::DNS::hasInBlacklist(const event::family_t family, string_view domain, string_view ip) const noexcept {

}
/**
 * @brief Метод установки фдреса файла локальных хостов
 *
 * @param filename адрес файла для установки
 */
void awh::unit::DNS::setHostsFilename(string_view filename) noexcept {

}
/**
 * @brief Метод установки префикса переменной окружения
 *
 * @param prefix префикс переменной окружения для установки
 */
void awh::unit::DNS::setPrefixSnvironment(string_view prefix) noexcept {
	// Если префикс переменной окружения передан
	if(!std::empty(prefix)){
		// Выполняем блокировку потокв для установки IP-адреса события
		const locker_t <> lock(::__awh_resolver__.mtx);
		// Устанавливаем префикс переменной окружения
		::__awh_resolver__.prefix = this->_fmk->transform(prefix.data(), fmk_t::transform_t::UPPER_CASE);
	}
}
/**
 * @brief Метод добавления сервера DNS
 *
 * @param eid    идентификатор события DNS-резолвера
 * @param server адрес DNS-сервера
 */
void awh::unit::DNS::addServer(const event::id_t eid, string_view server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес DNS-сервера передан
		if((eid > 0) && !std::empty(server)){
			// Выполняем блокировку потокв для парсинга IP-адреса
			const locker_t <> lock(::__awh_mtx__);
			// Выполняем парсинг IPv4-адреса
			if(this->_addr.parse(server.data())){
				// Выполняем блокировку потокв для установки IP-адреса события
				const locker_t <> lock(::__awh_resolver__.mtx);
				// Получаем IP-адрес в исходном виде
				auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				// Устанавливаем IP-адрес события
				this->_io->setTarget(eid, ::move(ip));
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, server), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод добавления сервера DNS
 *
 * @param eid    идентификатор события DNS-резолвера
 * @param server адрес DNS-сервера
 */
void awh::unit::DNS::addServer(const event::id_t eid, const unique_ptr <net::addr_t> & server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес DNS-сервера передан
		if((eid > 0) && (server != nullptr)){
			// Объект для хранения IP-адреса
			unique_ptr <net::addr_t> ip = nullptr;
			/**
			 * Определяем тип адреса
			 */
			switch(server->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем инициализацию объекта IP-адреса
					ip = make_unique <net::addr_net_ipv4_t> ();
					// Устанавливаем IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (server.get())->address;
				} break;
				// Если адрес является IPv6
				case 16: {
					// Выполняем инициализацию объекта IP-адреса
					ip = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (server.get())->address[0], 16);
				} break;
			}
			// Если IP-адрес получен
			if(ip != nullptr){
				// Выполняем блокировку потокв для установки IP-адреса события
				const locker_t <> lock(::__awh_resolver__.mtx);
				// Устанавливаем IP-адрес события
				this->_io->setTarget(eid, ::move(ip));
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод добавления сервера DNS
 *
 * @param eid    идентификатор события DNS-резолвера
 * @param family семейстов IP-адресов IPv4/IPv6
 * @param server адрес DNS-сервера
 */
void awh::unit::DNS::addServer(const event::id_t eid, const event::family_t family, string_view server) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес DNS-сервера передан
		if((eid > 0) && !std::empty(server)){
			// Объект для хранения IP-адреса
			unique_ptr <net::addr_t> ip = nullptr;
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потокв для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv4-адреса
					if(this->_addr.parse(server.data(), net_addr_t::type_t::IPV4))
						// Получаем IP-адрес в исходном виде
						ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потокв для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IPv6-адреса
					if(this->_addr.parse(server.data(), net_addr_t::type_t::IPV6))
						// Получаем IP-адрес в исходном виде
						ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
				} break;
			}
			// Если IP-адрес получен
			if(ip != nullptr){
				// Выполняем блокировку потокв для установки IP-адреса события
				const locker_t <> lock(::__awh_resolver__.mtx);
				// Устанавливаем IP-адрес события
				this->_io->setTarget(eid, ::move(ip));
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (family), server), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод добавления адреса сети с которого будет выполняться запрос
 *
 * @param eid    идентификатор события DNS-резолвера
 * @param source адрес сети для выполнения запроса
 */
void awh::unit::DNS::addSource(const event::id_t eid, string_view source) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if((eid > 0) && !std::empty(source)){
			// Выполняем блокировку потокв для парсинга IP-адреса
			const locker_t <> lock(::__awh_mtx__);
			// Выполняем парсинг IP-адреса
			if(this->_addr.parse(source.data())){
				/**
				 * Определяем тип IP-адреса
				 */
				switch(static_cast <uint8_t> (this->_addr.type())){
					// Если адрес является IPv4
					case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
						// Выполняем блокировку потокв для установки IP-адреса события
						const locker_t <> lock(::__awh_resolver__.mtx);
						// Получаем IP-адрес в исходном виде
						auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Устанавливаем IP-адрес события
						this->_io->setAddress(eid, event::address_t::IPV4, ::move(ip));
					} break;
					// Если адрес является IPv6
					case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
						// Выполняем блокировку потокв для установки IP-адреса события
						const locker_t <> lock(::__awh_resolver__.mtx);
						// Получаем IP-адрес в исходном виде
						auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Устанавливаем IP-адрес события
						this->_io->setAddress(eid, event::address_t::IPV6, ::move(ip));
					} break;
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, source), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод добавления адреса сети с которого будет выполняться запрос
 *
 * @param eid    идентификатор события DNS-резолвера
 * @param source адрес сети для выполнения запроса
 */
void awh::unit::DNS::addSource(const event::id_t eid, const unique_ptr <net::addr_t> & source) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if((eid > 0) && (source != nullptr)){
			/**
			 * Определяем тип адреса
			 */
			switch(source->size){
				// Если адрес является IPv4
				case 4: {
					// Выполняем блокировку потокв для установки IP-адреса события
					const locker_t <> lock(::__awh_resolver__.mtx);
					// Выполняем инициализацию объекта IP-адреса
					auto ip = make_unique <net::addr_net_ipv4_t> ();
					// Устанавливаем IP-адрес
					awh_cast <net::addr_net_ipv4_t *> (ip.get())->address = awh_cast <net::addr_net_ipv4_t *> (source.get())->address;
					// Устанавливаем IP-адрес события
					this->_io->setAddress(eid, event::address_t::IPV4, ::move(ip));
				} break;
				// Если адрес является IPv6
				case 16: {
					// Выполняем блокировку потокв для установки IP-адреса события
					const locker_t <> lock(::__awh_resolver__.mtx);
					// Выполняем инициализацию объекта IP-адреса
					auto ip = make_unique <net::addr_net_ipv6_t> ();
					// Устанавливаем IP-адрес
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (ip.get())->address[0], &awh_cast <net::addr_net_ipv6_t *> (source.get())->address[0], 16);
					// Устанавливаем IP-адрес события
					this->_io->setAddress(eid, event::address_t::IPV6, ::move(ip));
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод добавления адреса сети с которого будет выполняться запрос
 *
 * @param eid    идентификатор события DNS-резолвера
 * @param family семейстов IP-адресов IPv4/IPv6
 * @param source адрес сети для выполнения запроса
 */
void awh::unit::DNS::addSource(const event::id_t eid, const event::family_t family, string_view source) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если адрес сети для выполнения запроса передан
		if((eid > 0) && !std::empty(source)){
			/**
			 * Определяем семейство события
			 */
			switch(static_cast <uint8_t> (family)){
				// Для семейства IPv4
				case static_cast <uint8_t> (event::family_t::IPV4): {
					// Выполняем блокировку потокв для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IP-адреса
					if(this->_addr.parse(source.data(), net_addr_t::type_t::IPV4)){
						// Выполняем блокировку потокв для установки IP-адреса события
						const locker_t <> lock(::__awh_resolver__.mtx);
						// Получаем IP-адрес в исходном виде
						auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Устанавливаем IP-адрес события
						this->_io->setAddress(eid, event::address_t::IPV4, ::move(ip));
					}
				} break;
				// Для семейства IPv6
				case static_cast <uint8_t> (event::family_t::IPV6): {
					// Выполняем блокировку потокв для парсинга IP-адреса
					const locker_t <> lock(::__awh_mtx__);
					// Выполняем парсинг IP-адреса
					if(this->_addr.parse(source.data(), net_addr_t::type_t::IPV6)){
						// Выполняем блокировку потокв для установки IP-адреса события
						const locker_t <> lock(::__awh_resolver__.mtx);
						// Получаем IP-адрес в исходном виде
						auto ip = ::move(this->_addr.source(net_addr_t::endian_t::LITTLE));
						// Устанавливаем IP-адрес события
						this->_io->setAddress(eid, event::address_t::IPV6, ::move(ip));
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(eid, static_cast <uint16_t> (family), source), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод обратного запроса доменного имени соответствующего IP-адресу
 *
 * @param eid идентификатор события DNS-резолвера
 * @param ip  адрес для поиска доменного имени
 * @return    результат выполнения запроса
 */
bool awh::unit::DNS::reverse(const event::id_t eid, string_view ip) noexcept {

}
/**
 * @brief Метод обратного запроса доменного имени соответствующего IP-адресу
 *
 * @param eid идентификатор события DNS-резолвера
 * @param ip  адрес для поиска доменного имени
 * @return    результат выполнения запроса
 */
bool awh::unit::DNS::reverse(const event::id_t eid, const unique_ptr <net::addr_t> & ip) noexcept {

}
/**
 * @brief Метод обратного запроса доменного имени соответствующего IP-адресу
 *
 * @param eid     идентификатор события DNS-резолвера
 * @param family тип интернет-протокола IPv4/IPv6
 * @param ip     адрес для поиска доменного имени
 * @return       результат выполнения запроса
 */
bool awh::unit::DNS::reverse(const event::id_t eid, const event::family_t family, string_view ip) noexcept {

}
/**
 * @brief Метод поиска доменного имени соответствующего IP-адресу
 *
 * @param eid идентификатор события DNS-резолвера
 * @param ip  адрес для поиска доменного имени
 * @return    список найденных доменных имён
 */
vector <string> awh::unit::DNS::search(const event::id_t eid, string_view ip) noexcept {

}
/**
 * @brief Метод поиска доменного имени соответствующего IP-адресу
 *
 * @param eid идентификатор события DNS-резолвера
 * @param ip  адрес для поиска доменного имени
 * @return    список найденных доменных имён
 */
vector <string> awh::unit::DNS::search(const event::id_t eid, const unique_ptr <net::addr_t> & ip) noexcept {

}
/**
 * @brief Метод поиска доменного имени соответствующего IP-адресу
 *
 * @param eid     идентификатор события DNS-резолвера
 * @param family тип интернет-протокола IPv4/IPv6
 * @param ip     адрес для поиска доменного имени
 * @return       список найденных доменных имён
 */
vector <string> awh::unit::DNS::search(const event::id_t eid, const event::family_t family, string_view ip) noexcept {

}
/**
 * @brief Метод ресолвинга домена
 *
 * @param eid    идентификатор события DNS-резолвера
 * @param domain доменное имя сервера
 * @return       полученный IP-адрес
 */
bool awh::unit::DNS::request(const event::id_t eid, string_view domain) noexcept {

}
/**
* @brief Метод ресолвинга домена
*
* @param eid    идентификатор события DNS-резолвера
* @param family тип интернет-протокола IPv4/IPv6
* @param domain доменное имя сервера
* @return       полученный IP-адрес
*/
bool awh::unit::DNS::request(const event::id_t eid, const event::family_t family, string_view domain) noexcept {

}
/**
 * @brief Метод ресолвинга домена
 *
 * @param eid    идентификатор события DNS-резолвера
 * @param domain доменное имя сервера
 * @return       полученный IP-адрес
 */
string awh::unit::DNS::resolve(const event::id_t eid, string_view domain) noexcept {

}
/**
 * @brief Метод ресолвинга домена
 *
 * @param eid    идентификатор события DNS-резолвера
 * @param family тип интернет-протокола IPv4/IPv6
 * @param domain доменное имя сервера
 * @return       полученный IP-адрес
 */
string awh::unit::DNS::resolve(const event::id_t eid, const event::family_t family, string_view domain) noexcept {

}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::unit::DNS::DNS(const fmk_t * fmk, const log_t * log) noexcept : unit_t(fmk, log), _addr(fmk, log) {
	// Активируем работу мьютекса блокировки потока при работе с кэшем
	::__awh_cache__.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
	// Активируем работу мьютекса блокировки потока при работе с резолвером
	::__awh_resolver__.mtx.enabled = (::__awh_thread_safety__ == event::mode_t::ENABLED);
}
/**
 * @brief Деструктор
 *
 */
awh::unit::DNS::~DNS() noexcept {}
