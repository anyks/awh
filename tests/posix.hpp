/**
 * @file: posix.hpp
 * @date: 2026-08-06
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Восполнение средств POSIX, отсутствующих у MS Windows, для тестового окружения
 *
 * @details Заголовок этот принадлежит **только набору тестов** и в поставку библиотеки
 *          не идёт. Сама библиотека средствами POSIX не подменяется - там каждая система
 *          обслуживается своими средствами, - а вот наборы тестов писались от POSIX, и
 *          переписывать их целиком ради одной системы было бы неразумно
 *
 *          Определения выкладываются в глобальное пространство имён намеренно: обращения
 *          в тестах записаны как `::setenv`, `::gmtime_r`, и подмена пространства
 *          потребовала бы править каждое место обращения
 *
 * @warning Восполняется здесь лишь то, чем пользуются тесты, и лишь в той мере, в какой
 *          они этим пользуются. Полными заменами средств POSIX эти определения не
 *          являются и таковыми считаться не должны
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_TESTS_POSIX__
#define __AWH_TESTS_POSIX__

/**
 * Для операционных систем Linux, FreeBSD, NetBSD, OpenBSD, macOS и Solaris
 */
#if !_WIN32 && !_WIN64
	/**
	 * Стандартные заголовочные файлы работы с файловыми дескрипторами и гнёздами
	 *
	 * @note Заголовок разрядных целых подключается явно: BSD-системы и macOS выдают
	 *       эти имена попутно, заголовками гнёзд, а стандартная библиотека GNU их
	 *       попутно не выдаёт
	 */
	#include <cstdint>
	#include <unistd.h>
	#include <sys/time.h>
	#include <sys/socket.h>

	/**
	 * @brief Функция закрытия гнезда
	 *
	 * @param fd закрываемое гнездо
	 * @return   ноль при успехе, иначе -1
	 *
	 * @details Средство это принадлежит MS Windows, где гнёзда и файловые дескрипторы
	 *          разведены порознь: гнёзда закрываются closesocket, файлы - _close, и
	 *          подмена одного другим отвечает отказом. У систем POSIX разделения этого
	 *          нет, и закрывает то и другое один close
	 *
	 *          Заведено оно **у POSIX**, а не наоборот, намеренно. Обратный путь -
	 *          подменить у MS Windows close - невозможен: библиотека MinGW объявляет
	 *          его сама, и своё определение столкнулось бы с её объявлением. Проверено
	 *          опытом: сборка отвечала отказом «conflicting declaration of 'int
	 *          close(int)' with 'C' linkage»
	 *
	 *          Оттого обращения наборов приведены к closesocket - имени, у MS Windows
	 *          настоящему, - а восполняется недостающее здесь
	 *
	 */
	inline int32_t closesocket(const int32_t fd) noexcept {
		/**
		 * Средство закрытия зовётся здесь без указания глобального пространства имён
		 * намеренно
		 *
		 * @warning Запись «::close» выглядела бы строже, но именно она и оказалась
		 *          ловушкой: определение это лежит в глобальном пространстве имён, и
		 *          поточная правка обращений, прошедшая по всем файлам наборов, задела
		 *          и его собственное тело - «::close» стало «::closesocket», и средство
		 *          принялось звать само себя. Уход вглубь без конца, переполнение стека,
		 *          отказ доступа к памяти. Тот же вид ошибки уже встречался у
		 *          __awh_pagesize__ в sys/fs.cpp
		 *
		 */
		return close(fd);
	}
	/**
	 * @brief Функция установки предела ожидания приёма на гнезде
	 *
	 * @param fd      гнездо, которому ставится предел ожидания
	 * @param timeout предел ожидания в миллисекундах
	 * @return        ноль при успехе, иначе -1
	 *
	 * @details Средство это заведено оттого, что настройка SO_RCVTIMEO у систем разнится
	 *          не одними лишь типами доводов, а самим смыслом значения: у POSIX она
	 *          принимает структуру времени `timeval`, у MS Windows - число миллисекунд.
	 *          Приведение указателя одного к другому собралось бы, но задало бы предел,
	 *          ничего общего с заданным не имеющий
	 *
	 */
	inline int32_t setReceiveTimeout(const int32_t fd, const uint32_t timeout) noexcept {
		// Предел ожидания приёма в виде, принимаемом системой
		struct timeval value;
		// Устанавливаем целые секунды предела ожидания
		value.tv_sec = static_cast <time_t> (timeout / 1000);
		// Устанавливаем доли секунды предела ожидания
		value.tv_usec = static_cast <suseconds_t> ((timeout % 1000) * 1000);
		// Выполняем установку предела ожидания приёма
		return ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &value, sizeof(value));
	}
#endif

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Стандартные заголовочные файлы
	 */
	#include <ctime>
	#include <new>
	#include <vector>
	#include <cstdint>
	#include <cstdlib>
	#include <cstring>

	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/win32.hpp>

	/**
	 * Заголовочный файл перечисления сетевых устройств машины
	 */
	#include <iphlpapi.h>

	/**
	 * @brief Функция установки переменной окружения
	 *
	 * @param name      название переменной окружения
	 * @param value     значение переменной окружения
	 * @param overwrite признак замены уже установленного значения
	 * @return          ноль при успехе, иначе -1
	 *
	 * @note Довод overwrite соблюдается: без него набор, выставляющий временную зону
	 *       поверх уже выставленной, работал бы не так, как на POSIX
	 *
	 */
	inline int32_t setenv(const char * name, const char * value, const int32_t overwrite) noexcept {
		// Если название переменной окружения не передано
		if((name == nullptr) || (name[0] == '\0'))
			// Выводим признак отказа
			return -1;
		// Если замена уже установленного значения запрещена
		if(overwrite == 0){
			// Получаем текущее значение переменной окружения
			const char * current = ::getenv(name);
			// Если переменная окружения уже установлена - оставляем её нетронутой
			if((current != nullptr) && (current[0] != '\0'))
				// Выводим признак успеха
				return 0;
		}
		// Устанавливаем значение переменной окружения
		return ((::_putenv_s(name, ((value != nullptr) ? value : "")) == 0) ? 0 : -1);
	}
	/**
	 * @brief Функция снятия переменной окружения
	 *
	 * @param name название переменной окружения
	 * @return     ноль при успехе, иначе -1
	 *
	 * @note Снятие выполняется присвоением пустого значения: отдельного средства для
	 *       того у MS Windows нет, а пустое значение считается там отсутствием
	 *
	 */
	inline int32_t unsetenv(const char * name) noexcept {
		// Снимаем переменную окружения
		return ((::_putenv_s(((name != nullptr) ? name : ""), "") == 0) ? 0 : -1);
	}
	/**
	 * Разложение времени по временным зонам заводится лишь при отсутствии
	 *
	 * @details Заголовок MinGW «time.h» объявляет gmtime_r и localtime_r сам, но лишь
	 *          под признаком _POSIX_THREAD_SAFE_FUNCTIONS. Сейчас признак этот не
	 *          выставлен, и определения ниже нужны; выставь его сборка - свои
	 *          определения столкнулись бы с объявлениями библиотеки. Условие взято то
	 *          же, что и у неё, оттого верны обе поры
	 *
	 * @note Тот же вид столкновения уже случался дважды - с close и с usleep, - и оба
	 *       раза его находила лишь сборка. Проверять наличие средства у библиотеки
	 *       прежде заведения своего надлежит всегда
	 *
	 */
	#ifndef _POSIX_THREAD_SAFE_FUNCTIONS
	/**
	 * @brief Функция разложения времени по всемирной временной зоне
	 *
	 * @param value  раскладываемый момент времени
	 * @param result объект разложенного времени
	 * @return       объект разложенного времени, либо nullptr при отказе
	 *
	 * @note Порядок доводов у gmtime_s обратен порядку доводов gmtime_r, и путать их
	 *       нельзя: оба принимают указатели, и подмена собралась бы без единого
	 *       предупреждения
	 *
	 */
	inline std::tm * gmtime_r(const time_t * value, std::tm * result) noexcept {
		// Выполняем разложение времени по всемирной временной зоне
		return ((::gmtime_s(result, value) == 0) ? result : nullptr);
	}
	/**
	 * @brief Функция разложения времени по местной временной зоне
	 *
	 * @param value  раскладываемый момент времени
	 * @param result объект разложенного времени
	 * @return       объект разложенного времени, либо nullptr при отказе
	 *
	 */
	inline std::tm * localtime_r(const time_t * value, std::tm * result) noexcept {
		// Выполняем разложение времени по местной временной зоне
		return ((::localtime_s(result, value) == 0) ? result : nullptr);
	}
	#endif

	/**
	 * @brief Функция сборки момента времени из разложения по всемирной временной зоне
	 *
	 * @param value объект разложенного времени
	 * @return      собранный момент времени
	 *
	 */
	inline time_t timegm(std::tm * value) noexcept {
		// Выполняем сборку момента времени
		return ::_mkgmtime(value);
	}
	/**
	 * @brief Функция получения действующего идентификатора пользователя
	 *
	 * @return ноль, если процесс работает с полномочиями надзорными, иначе единица
	 *
	 * @details Разделения на пользователей по номерам у MS Windows нет вовсе, и точного
	 *          соответствия geteuid там не существует. Тесты пользуются им ровно для
	 *          одного - узнать, работает ли процесс с полномочиями надзорными, - и
	 *          ответ на этот вопрос система дать умеет
	 *
	 * @note Число возвращается затем, что обращения в тестах записаны сличением с нулём.
	 *       Правами пользователя число это не является и в ином качестве применяться
	 *       не должно
	 *
	 */
	inline uint32_t geteuid() noexcept {
		// Дескриптор маркера доступа процесса
		HANDLE token = nullptr;
		// Если маркер доступа процесса получить не удалось
		if(!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token))
			// Сообщаем, что полномочий надзорных нет
			return 1;
		// Сведения об уровне полномочий процесса
		TOKEN_ELEVATION elevation{};
		// Размер полученных сведений
		DWORD size = 0;
		// Признак работы с полномочиями надзорными
		bool elevated = false;
		// Если сведения об уровне полномочий получены
		if(::GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size))
			// Запоминаем признак работы с полномочиями надзорными
			elevated = (elevation.TokenIsElevated != 0);
		// Закрываем дескриптор маркера доступа процесса
		::CloseHandle(token);
		// Сообщаем об уровне полномочий процесса
		return (elevated ? 0 : 1);
	}
	/**
	 * @brief Функция получения идентификатора пользователя
	 *
	 * @return ноль, если процесс работает с полномочиями надзорными, иначе единица
	 *
	 * @note Различия между действующим и настоящим идентификаторами пользователя у
	 *       MS Windows нет, поэтому обе замены отвечают одинаково - см. пояснение
	 *       к geteuid
	 *
	 */
	inline uint32_t getuid() noexcept {
		// Выводим идентификатор пользователя
		return ::geteuid();
	}
	/**
	 * @brief Функция засева генератора псевдослучайных чисел
	 *
	 * @param seed засеваемое значение
	 *
	 * @note Пара srandom и random принадлежит POSIX; у MS Windows ей отвечает пара
	 *       srand и rand. Разрядность выдачи у последней меньше, но наборы пользуются
	 *       ею лишь для выбора номера порта, и разрядности этой там достаёт
	 *
	 */
	inline void srandom(const uint32_t seed) noexcept {
		// Выполняем засев генератора псевдослучайных чисел
		::srand(static_cast <unsigned int> (seed));
	}
	/**
	 * @brief Функция получения псевдослучайного числа
	 *
	 * @return псевдослучайное число
	 *
	 */
	inline int64_t random() noexcept {
		/**
		 * Собираем число из двух выдач: у MS Windows предел выдачи rand равен 32767,
		 * и одной выдачи не хватает на диапазоны, какими пользуются наборы
		 */
		return ((static_cast <int64_t> (::rand()) << 15) | static_cast <int64_t> (::rand()));
	}
	/**
	 * @brief Функция разбора записи адреса IPv4
	 *
	 * @param value  разбираемая запись адреса
	 * @param result объект разобранного адреса
	 * @return       единица при успехе, иначе ноль
	 *
	 * @warning Замена эта строже подменяемого: inet_aton принимает записи неполные и
	 *          восьмеричные ("127.1", "0177.0.0.1"), а inet_pton - только запись из
	 *          четырёх десятичных полей. Для набора, сличающего разборщик модуля с
	 *          системным на записях полных, разницы нет, но полной заменой считать
	 *          это нельзя
	 *
	 */
	inline int32_t inet_aton(const char * value, struct in_addr * result) noexcept {
		// Выполняем разбор записи адреса IPv4
		return ((::inet_pton(AF_INET, value, result) == 1) ? 1 : 0);
	}
	/**
	 * @brief Функция установки предела ожидания приёма на гнезде
	 *
	 * @param fd      гнездо, которому ставится предел ожидания
	 * @param timeout предел ожидания в миллисекундах
	 * @return        ноль при успехе, иначе -1
	 *
	 * @details Настройка SO_RCVTIMEO принимает у MS Windows число миллисекунд, тогда
	 *          как у POSIX - структуру времени. Пояснение смотрите у той же функции
	 *          в ветви систем POSIX
	 *
	 */
	inline int32_t setReceiveTimeout(const int32_t fd, const uint32_t timeout) noexcept {
		// Предел ожидания приёма в виде, принимаемом системой
		const DWORD value = static_cast <DWORD> (timeout);
		// Выполняем установку предела ожидания приёма
		return ::setsockopt(static_cast <SOCKET> (fd), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast <const char *> (&value), sizeof(value));
	}
	/**
	 * @brief Функция опроса готовности гнёзд
	 *
	 * @param events  перечень опрашиваемых гнёзд
	 * @param count   количество опрашиваемых гнёзд
	 * @param timeout предел ожидания в миллисекундах
	 * @return        количество готовых гнёзд, ноль по истечении предела, иначе -1
	 *
	 * @warning Замена эта опрашивает **только гнёзда**: WSAPoll иного и не умеет, тогда
	 *          как poll у POSIX опрашивает всякий файловый дескриптор. Наборам этого
	 *          достаёт - опрашиваются там одни лишь гнёзда, - но полной заменой считать
	 *          её нельзя
	 *
	 */
	inline int32_t poll(struct pollfd * events, const uint32_t count, const int32_t timeout) noexcept {
		// Выполняем опрос готовности гнёзд
		return ::WSAPoll(events, count, timeout);
	}
	/**
	 * @brief Признак устройства, являющегося петлёй
	 *
	 * @note Заводится он лишь при отсутствии: заголовок ws2ipdef.h объявляет признаки
	 *       эти сам, и притом иными значениями - IFF_LOOPBACK равен там 0x4, а не 0x8,
	 *       принятому у систем POSIX. Своё определение поверх системного расходилось бы
	 *       с тем, чем пользуются прочие средства той же системы
	 *
	 */
	#ifndef IFF_LOOPBACK
		#define IFF_LOOPBACK 0x8
	#endif
	/**
	 * @brief Признак поднятого устройства
	 *
	 */
	#ifndef IFF_UP
		#define IFF_UP 0x1
	#endif

	/**
	 * @brief Запись сетевого устройства машины
	 *
	 * @details Составом полей запись эта повторяет одноимённую запись POSIX ровно в той
	 *          мере, в какой ею пользуются наборы: название устройства, его признаки и
	 *          адрес. Полей ifa_netmask, ifa_dstaddr и ifa_data здесь нет - наборы их
	 *          не читают, а собрать их значило бы завести подобие, ничем не проверяемое
	 *
	 */
	struct ifaddrs {
		// Следующая запись перечня
		struct ifaddrs * ifa_next;
		// Название сетевого устройства
		char * ifa_name;
		// Признаки сетевого устройства
		uint32_t ifa_flags;
		// Адрес сетевого устройства
		struct sockaddr * ifa_addr;
	};

	/**
	 * @brief Внутреннее хранилище записей перечня сетевых устройств
	 *
	 * @details Записи и всё, на что они указывают, отводятся одним куском: перечень
	 *          POSIX освобождается единственным вызовом freeifaddrs, и раздельное
	 *          отведение имён и адресов пришлось бы обходить при освобождении
	 *
	 */
	struct __awh_ifaddrs_entry {
		// Запись, выдаваемая наружу
		struct ifaddrs entry;
		// Название сетевого устройства
		char name[256];
		// Место под адрес сетевого устройства любого семейства
		struct sockaddr_storage address;
	};
	/**
	 * @brief Функция получения перечня сетевых устройств машины
	 *
	 * @param result перечень сетевых устройств машины
	 * @return       ноль при успехе, иначе -1
	 *
	 * @details Средства ifaddrs у MS Windows нет вовсе, а отвечает ему там
	 *          GetAdaptersAddresses. Перебор ведётся по устройствам, а внутри каждого -
	 *          по его адресам передачи одному: запись POSIX несёт один адрес, и
	 *          устройство с несколькими адресами даёт несколько записей, как и у POSIX
	 *
	 * @note Признаки устройства собираются из двух его примет: IFF_UP ставится по
	 *       рабочему состоянию, IFF_LOOPBACK - по виду устройства. Числовые значения
	 *       признаков взяты те же, что у POSIX, чтобы наборы сличали их одинаково
	 *
	 */
	inline int32_t getifaddrs(struct ifaddrs ** result) noexcept {
		// Если перечень сетевых устройств принять некуда
		if(result == nullptr)
			// Выводим признак отказа
			return -1;
		// Сбрасываем перечень сетевых устройств
		(* result) = nullptr;
		// Размер буфера сведений о сетевых устройствах
		ULONG size = 0;
		/**
		 * Размер буфера запрашивается отдельным вызовом: он зависит от числа устройств
		 * машины и числа их адресов, а наперёд то и другое не известно
		 */
		if(::GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr, nullptr, &size) != ERROR_BUFFER_OVERFLOW)
			// Выводим признак отказа
			return -1;
		// Буфер сведений о сетевых устройствах
		std::vector <uint8_t> buffer(static_cast <size_t> (size), 0);
		// Начало перечня сведений о сетевых устройствах
		IP_ADAPTER_ADDRESSES * adapters = reinterpret_cast <IP_ADAPTER_ADDRESSES *> (buffer.data());
		// Если сведения о сетевых устройствах получить не удалось
		if(::GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr, adapters, &size) != NO_ERROR)
			// Выводим признак отказа
			return -1;
		// Последняя собранная запись перечня
		struct ifaddrs * last = nullptr;
		/**
		 * Выполняем перебор всех сетевых устройств машины
		 */
		for(IP_ADAPTER_ADDRESSES * adapter = adapters; adapter != nullptr; adapter = adapter->Next){
			/**
			 * Выполняем перебор всех адресов передачи одному у текущего устройства
			 */
			for(IP_ADAPTER_UNICAST_ADDRESS * address = adapter->FirstUnicastAddress; address != nullptr; address = address->Next){
				// Если адрес устройства негоден, пропускаем его
				if((address->Address.lpSockaddr == nullptr) || (address->Address.iSockaddrLength <= 0))
					// Переходим к следующему адресу устройства
					continue;
				// Если длина адреса в отведённое место не умещается, пропускаем его
				if(static_cast <size_t> (address->Address.iSockaddrLength) > sizeof(struct sockaddr_storage))
					// Переходим к следующему адресу устройства
					continue;
				// Отводим место под очередную запись перечня
				__awh_ifaddrs_entry * item = new (std::nothrow) __awh_ifaddrs_entry();
				// Если отвести место под запись не удалось, прекращаем сбор
				if(item == nullptr)
					// Прекращаем сбор перечня
					break;
				// Выполняем обнуление отведённой записи
				::memset(item, 0, sizeof(__awh_ifaddrs_entry));
				// Запоминаем название сетевого устройства
				::strncpy(item->name, ((adapter->AdapterName != nullptr) ? adapter->AdapterName : ""), (sizeof(item->name) - 1));
				// Копируем адрес сетевого устройства
				::memcpy(&item->address, address->Address.lpSockaddr, static_cast <size_t> (address->Address.iSockaddrLength));
				// Устанавливаем название сетевого устройства
				item->entry.ifa_name = item->name;
				// Устанавливаем адрес сетевого устройства
				item->entry.ifa_addr = reinterpret_cast <struct sockaddr *> (&item->address);
				// Собираем признаки сетевого устройства
				item->entry.ifa_flags = 0;
				// Если сетевое устройство поднято
				if(adapter->OperStatus == IfOperStatusUp)
					// Помечаем сетевое устройство поднятым
					item->entry.ifa_flags |= IFF_UP;
				// Если сетевое устройство является петлёй
				if(adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
					// Помечаем сетевое устройство петлёй
					item->entry.ifa_flags |= IFF_LOOPBACK;
				// Если запись перечня является первой
				if(last == nullptr)
					// Устанавливаем начало перечня
					(* result) = &item->entry;
				// Если запись перечня первой не является
				else last->ifa_next = &item->entry;
				// Запоминаем последнюю собранную запись перечня
				last = &item->entry;
			}
		}
		// Выводим признак успеха
		return 0;
	}
	/**
	 * @brief Функция освобождения перечня сетевых устройств машины
	 *
	 * @param list освобождаемый перечень сетевых устройств
	 *
	 */
	inline void freeifaddrs(struct ifaddrs * list) noexcept {
		/**
		 * Выполняем перебор всех записей перечня сетевых устройств
		 */
		while(list != nullptr){
			// Запоминаем следующую запись перечня прежде снятия текущей
			struct ifaddrs * next = list->ifa_next;
			/**
			 * Запись выдавалась первым полем хранилища, и приведение это обратно тому,
			 * каким она наружу и выдавалась
			 */
			delete reinterpret_cast <__awh_ifaddrs_entry *> (list);
			// Переходим к следующей записи перечня
			list = next;
		}
	}
#endif

#endif // __AWH_TESTS_POSIX__
