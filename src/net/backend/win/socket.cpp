/**
 * @file socket.cpp
 * @date 2026-08-07
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
 * @brief Реализация бэкенда низкоуровневой работы с сокетами для MS Windows —
 *        сроки ожидания, размеры накопителей, поддержание связи, число переходов,
 *        обнаружение MTU, групповая рассылка и заведение сокетов
 *
 * @details Слой этот отвечает эталонным backend/bsd/socket.cpp и backend/gnu/socket.cpp.
 *          Средства те же самые - setsockopt и getsockopt, - но набор имён опций у
 *          MS Windows свой, а часть их несёт иное устройство доводов: срок ожидания
 *          задаётся числом миллисекунд, а не структурой timeval, накопители же
 *          принимаются указателем на char вместо void
 *
 * @note Расхождения, какие снисходительностью не лечатся, помечены `@note` у самого
 *       метода: скрывать их за общим видом нельзя, вызывающая сторона вправе знать,
 *       чего система не умеет вовсе
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <vector>
#include <cstring>
#include <atomic>
#include <mutex>
#include <string>
#include <shared_mutex>

/**
 * Подключаем единую точку подключения системных заголовков MS Windows
 */
#include <sys/win32.hpp>

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/eth/eth.hpp>
#include <net/backend/win/qos.hpp>

/**
 * @brief Средства перевода названия сетевого устройства в его номер
 *
 * @details Объявлены они у MS Windows в отдельном заголовке netioapi.h, а не там же,
 *          где прочие средства сокетов, как то заведено у систем POSIX заголовком
 *          net/if.h. Подключается он здесь, а не через единую точку win32.hpp:
 *          средства эти нужны одному лишь этому файлу
 *
 */
#include <netioapi.h>

/**
 * @brief Заголовок перечня сетевых устройств машины
 *
 * @details Нужен ради номера устройства: название устройства у MS Windows -
 *          GUID, а перевод его в номер средствами сокетов не выполняется
 *
 */
#include <iphlpapi.h>

/**
 * @brief Заголовок UNIX-доменных сокетов
 *
 * @details Объявлены они у MS Windows отдельно от прочих средств сокетов, и
 *          доступны с 2018 года. Потоковыми они у этой системы и остаются
 *
 */
#include <afunix.h>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Название бэкенда для записей в журнале
 *
 */
static constexpr const char * __AWH_SOCKET_BACKEND__ = "MS Windows socket backend";

/**
 * @brief Инкапсулируем состояние слоя в пространство имён
 *
 */
namespace {
	// Режим безопасности работы потоков
	awh::event::mode_t __awh_socket_thread_safety__ = awh::event::mode_t::DISABLED;
	// Замок согласования доступа к настройкам сокетов
	std::shared_mutex __awh_socket_mutex__;

	/**
	 * @brief Признак того, что средства сокетов системой уже подняты
	 *
	 */
	std::once_flag __awh_winsock_once__;

	/**
	 * @brief Описание отказа подъёма средств сокетов, пустое при успехе
	 *
	 * @note Держится оно строкой, а не выдаётся в журнал на месте: подъём идёт и на
	 *       загрузке слоя, когда объекта журнала нет ещё вовсе. Выдают отказ те, у
	 *       кого журнал есть, - и выдают его там, где вызывающая сторона отказа и ждёт
	 *
	 */
	std::string __awh_winsock_failure__;

	/**
	 * @brief Функция проверки поднятости средств сокетов системы
	 *
	 * @details Отдельного обращения об этом система не имеет вовсе, оттого поднятость
	 *          и выясняется пробой - заведением сокета на выброс. Поднятость - ответ
	 *          `10093` (`WSANOTINITIALISED`) на всякое обращение к сокетам, и проба
	 *          получает его первой
	 *
	 * @return результат проверки поднятости средств сокетов системы
	 *
	 */
	bool __awh_winsock_raised__() noexcept {
		// Выполняем заведение сокета на выброс
		const SOCKET sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		// Если сокет завести не удалось, средства сокетов не подняты
		if(sock == INVALID_SOCKET)
			// Выводим признак неподнятых средств сокетов
			return false;
		// Выполняем закрытие заведённого на выброс сокета
		::closesocket(sock);
		// Выводим признак поднятых средств сокетов
		return true;
	}

	/**
	 * @brief Функция подъёма средств сокетов системы
	 *
	 * @details Средства сокетов у MS Windows требуют подъёма на процесс: до вызова
	 *          `WSAStartup` всякое обращение к ним отвечает отказом 10093
	 *          (`WSANOTINITIALISED`). У систем POSIX подъёма этого нет вовсе, оттого
	 *          и вся забота о нём здесь - в слое одной лишь MS Windows
	 *
	 * @details Поднимает средства сама библиотека, а не тот, кто ею пользуется.
	 *          Требовать подъёма от вызывающей стороны значило бы вынести
	 *          своенравность одной системы в общий договор: код, работающий на прочих
	 *          системах, под MS Windows отказывал бы - и отказывал бы не в месте
	 *          ошибки, а на первом же сокете. Установлено прогоном: без подъёма
	 *          проверки сокетов отказывают все до единой
	 *
	 * @details Поднимает она их лишь тогда, когда НИКТО не поднял их раньше. Вызывающая
	 *          сторона вправе поднять средства и не зная о том: за неё это делает,
	 *          к примеру, `boost::asio` на всяком своём `io_context`
	 *
	 * @warning НАМЕРЕННОЕ РЕШЕНИЕ владельца, принятое по опыту живых приложений:
	 *          повторный подъём средств поверх чужого приводил к путанице, из которой
	 *          отпуск уже не выводил. Договор системы обещает иное - счёт ссылок, где
	 *          повторный подъём безобиден, - и на стенде простой парный случай так себя
	 *          и ведёт; но опыт весит больше обещания, и слой повторно не поднимает
	 *
	 * @warning Плата за это известна и принята: не взяв своей ссылки, слой зависит от
	 *          чужой. Отпусти её поднявший раньше, чем слой доработает, - счёт упадёт
	 *          в ноль, средства снимутся под работающим слоем, и сокеты его умрут
	 *          отказом 10093 посреди работы. Замерено на стенде двумя единицами
	 *          трансляции, друг о друге не знающими
	 *
	 * @note Вторая сторона той же платы - и она в пользу пробы: отпуск здесь заведомо
	 *       не бывает лишним. Отпустить больше, чем поднял, значит уронить счёт в ноль
	 *       досрочно и снять средства у всех разом; слой, не берущий ссылки, такого не
	 *       сделает никогда
	 *
	 * @warning Отказ подъёма НЕ уводит из приложения, хотя прежде уводил
	 *          (`exit(EXIT_FAILURE)`). Решать судьбу приложения библиотека не вправе:
	 *          то, что для одного приложения смертельно, для другого - отказ одной
	 *          лишь возможности из многих. Отказ выходит наружу отказом заведения
	 *          движка - там, где вызывающая сторона просит сеть явно
	 *
	 * @return результат подъёма средств сокетов системы
	 *
	 */
	bool __awh_winsock__() noexcept {
		// Выполняем подъём средств сокетов системы единожды на процесс
		std::call_once(::__awh_winsock_once__, []() noexcept -> void {
			/**
			 * Если средства сокетов уже подняты кем-то ещё, поднимать их незачем
			 *
			 * @note Отпуск при этом не вешается вовсе: отпускать нечего, своей ссылки
			 *       слой не брал. Отпусти он чужую - отнял бы средства у того, кто их
			 *       поднял и ими пользуется
			 */
			if(::__awh_winsock_raised__())
				// Выходим из функции: средства сокетов годны
				return;
			// Сведения о поднятых средствах сокетов
			WSADATA data{};
			// Выполняем подъём средств сокетов системы
			const int32_t error = ::WSAStartup(MAKEWORD(2, 2), &data);
			// Если поднять средства сокетов не удалось
			if(error != 0){
				// Запоминаем описание отказа подъёма средств сокетов
				::__awh_winsock_failure__ = ("socket subsystem startup failed with error " + std::to_string(error));
				// Выходим из функции: заводить сокеты будет нечем
				return;
			}
			/**
			 * Сверяем выданную системой версию средств сокетов с запрошенной
			 *
			 * @note Отказом система отвечает не на всякое расхождение: запрошенную
			 *       версию она вправе понизить до той, какую умеет, и сообщить об этом
			 *       одними лишь выданными сведениями. Наложенный обмен, на котором
			 *       стоит движок, младше версии 2.2 не работает вовсе
			 */
			if((LOBYTE(data.wVersion) != 2) || (HIBYTE(data.wVersion) != 2)){
				// Запоминаем описание расхождения версий средств сокетов
				::__awh_winsock_failure__ = (
					"socket subsystem version " + std::to_string(static_cast <uint32_t> (LOBYTE(data.wVersion))) +
					"." + std::to_string(static_cast <uint32_t> (HIBYTE(data.wVersion))) + " is provided instead of requested 2.2"
				);
				// Отдаём взятую ссылку: пользоваться средствами этой версии нельзя
				::WSACleanup();
				// Выходим из функции: заводить сокеты будет нечем
				return;
			}
			/**
			 * Отдаём взятую ссылку на выходе из процесса
			 *
			 * @note Снятие вешается на выход, а не на разрушение объекта: сокеты
			 *       заводятся и закрываются много раз за жизнь процесса, и объекта,
			 *       который жил бы ровно столько же, у слоя этого нет вовсе.
			 *       Вешается оно лишь тогда, когда ссылка ВЗЯТА - то есть когда слой
			 *       поднял средства сам
			 */
			::atexit([]() noexcept -> void {
				// Выполняем отпуск взятой ссылки на средства сокетов системы
				::WSACleanup();
			});
		});
		// Выводим результат подъёма средств сокетов системы
		return ::__awh_winsock_failure__.empty();
	}

	/**
	 * @brief Подъём средств сокетов системы на загрузке слоя
	 *
	 * @note Заведён отдельным объектом, а не одними лишь обращениями из методов:
	 *       обращения покрывают заведение сокета, но не покрывают обращений к
	 *       средствам системы в обход слоя - разбор имён, сведения об устройствах,
	 *       - а те требуют поднятых средств ровно так же
	 *
	 */
	const struct Winsock {
		/**
		 * @brief Конструктор
		 *
		 */
		Winsock() noexcept {
			// Выполняем подъём средств сокетов системы
			::__awh_winsock__();
		}
	} __awh_winsock_starter__;

	/**
	 * @brief Функция получения описания последней ошибки сокета
	 *
	 * @details Соответствия strerror у MS Windows для ошибок сокетов нет: коды их лежат
	 *          в отдельном пространстве WSA, а сообщения достаются FormatMessageW.
	 *          Сообщение выводится на языке системы, оттого код приводится рядом с ним -
	 *          по коду и следует искать, читая чужой журнал
	 *
	 * @return описание последней ошибки сокета вместе с её кодом
	 *
	 */
	std::string __awh_socket_error__() noexcept {
		// Получаем код последней ошибки сокета
		const int32_t code = ::WSAGetLastError();
		// Буфер сообщения, отводимый самой системой
		wchar_t * buffer = nullptr;
		// Получаем описание ошибки сокета
		const DWORD size = ::FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, static_cast <DWORD> (code), 0, reinterpret_cast <wchar_t *> (&buffer), 0, nullptr
		);
		// Собираемое описание ошибки сокета
		std::string result = ("error " + std::to_string(code));
		// Если описание ошибки получено
		if((size > 0) && (buffer != nullptr)){
			// Отводим место под описание ошибки в однобайтовой записи
			std::vector <char> text(static_cast <size_t> (size) * 4 + 1, 0);
			// Переводим описание ошибки в однобайтовую запись
			const int32_t length = ::WideCharToMultiByte(CP_UTF8, 0, buffer, static_cast <int32_t> (size), text.data(), static_cast <int32_t> (text.size() - 1), nullptr, nullptr);
			// Если перевод описания ошибки выполнен
			if(length > 0){
				// Собираем описание ошибки
				std::string message(text.data(), static_cast <size_t> (length));
				/**
				 * Снимаем перевод строки, каким система оканчивает описание
				 */
				while(!message.empty() && ((message.back() == '\r') || (message.back() == '\n')))
					// Снимаем последний знак описания
					message.pop_back();
				// Если описание не опустело - дополняем им результат
				if(!message.empty())
					// Дополняем результат описанием ошибки
					result.append(": ").append(message);
			}
		}
		// Если буфер сообщения системой отведён
		if(buffer != nullptr)
			// Освобождаем буфер сообщения
			::LocalFree(buffer);
		// Возвращаем собранное описание ошибки сокета
		return result;
	}
};

/**
 * @brief Метод установки режима безопасной работы с потоками
 *
 * @param mode режим безопасной работы с потоками
 *
 */
void awh::eth::Socket::threadSafety(const bool mode) noexcept {
	// Устанавливаем режим безопасной работы с потоками
	::__awh_socket_thread_safety__ = (mode ? event::mode_t::ENABLED : event::mode_t::DISABLED);
}

/**
 * @brief Метод получения кода ошибки сокета
 *
 * @param sock сокет для получения кода ошибки
 * @return     код ошибки сокета
 *
 */
int32_t awh::eth::Socket::getError(const net::socket_t sock) const noexcept {
	// Переменная результата
	int32_t result = -1;
	// Размер кода ошибки
	int32_t size = static_cast <int32_t> (sizeof(result));
	// Если код ошибки сокета получить не удалось
	if(::getsockopt(static_cast <SOCKET> (sock), SOL_SOCKET, SO_ERROR, reinterpret_cast <char *> (&result), &size) != 0)
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
	// Возвращаем результат
	return result;
}

/**
 * @brief Метод получения срока ожидания сокета
 *
 * @param sock  сокет для получения срока ожидания
 * @param event событие сокета
 * @return      срок ожидания в миллисекундах
 *
 * @note Срок ожидания у MS Windows задаётся числом миллисекунд типа DWORD, а не
 *       структурой timeval, как то предписано POSIX. Расхождение это уровня самой
 *       системы, и приведение к общему виду ведётся здесь
 *
 */
uint32_t awh::eth::Socket::getTimeout(const net::socket_t sock, const net::socket_event_t event) const noexcept {
	// Срок ожидания, снятый у системы
	DWORD msec = 0;
	// Размер срока ожидания
	int32_t size = static_cast <int32_t> (sizeof(msec));
	// Имя опции срока ожидания
	const int32_t option = ((event == net::socket_event_t::READ) ? SO_RCVTIMEO : SO_SNDTIMEO);
	// Если срок ожидания получить не удалось
	if(::getsockopt(static_cast <SOCKET> (sock), SOL_SOCKET, option, reinterpret_cast <char *> (&msec), &size) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем пустой срок ожидания
		return 0;
	}
	// Возвращаем снятый срок ожидания
	return static_cast <uint32_t> (msec);
}

/**
 * @brief Метод установки срока ожидания сокета
 *
 * @param sock  сокет для установки срока ожидания
 * @param event событие сокета
 * @param msec  срок ожидания в миллисекундах
 * @return      результат выполнения установки
 *
 * @note Нулевой срок означает ожидание бессрочное - так же, как и у систем POSIX
 *
 */
bool awh::eth::Socket::setTimeout(const net::socket_t sock, const net::socket_event_t event, const uint32_t msec) const noexcept {
	// Срок ожидания в понимании системы
	DWORD timeout = static_cast <DWORD> (msec);
	// Имя опции срока ожидания
	const int32_t option = ((event == net::socket_event_t::READ) ? SO_RCVTIMEO : SO_SNDTIMEO);
	// Если срок ожидания установить не удалось
	if(::setsockopt(static_cast <SOCKET> (sock), SOL_SOCKET, option, reinterpret_cast <const char *> (&timeout), static_cast <int32_t> (sizeof(timeout))) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем отрицательный результат установки
		return false;
	}
	// Возвращаем положительный результат установки
	return true;
}

/**
 * @brief Метод получения свободного места в буфере сокета
 *
 * @note MS Windows занятое место буферов НЕ сообщает: ни параметров вроде SO_NWRITE,
 *       ни запросов вроде SIOCOUTQ там нет. Потребитель обязан обходиться свободным
 *       местом собственной очереди
 *
 * @param sock  сетевой сокет
 * @param event событие сокета (чтение либо запись)
 * @return      признак того, что свободное место неизвестно
 *
 */
int32_t awh::eth::Socket::getBufferAvailable([[maybe_unused]] const net::socket_t sock, [[maybe_unused]] const net::socket_event_t event) const noexcept {
	// Выводим признак того, что свободное место неизвестно
	return -1;
}
/**
 * @brief Метод получения размера накопителя сокета
 *
 * @param sock  сокет для получения размера накопителя
 * @param event событие сокета
 * @return      размер накопителя сокета
 *
 */
int32_t awh::eth::Socket::getBufferSize(const net::socket_t sock, const net::socket_event_t event) const noexcept {
	// Размер накопителя сокета
	int32_t result = 0;
	// Размер значения размера накопителя
	int32_t size = static_cast <int32_t> (sizeof(result));
	// Имя опции размера накопителя
	const int32_t option = ((event == net::socket_event_t::READ) ? SO_RCVBUF : SO_SNDBUF);
	// Если размер накопителя получить не удалось
	if(::getsockopt(static_cast <SOCKET> (sock), SOL_SOCKET, option, reinterpret_cast <char *> (&result), &size) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем пустой размер накопителя
		return 0;
	}
	// Возвращаем снятый размер накопителя
	return result;
}

/**
 * @brief Метод установки размера накопителя сокета
 *
 * @param sock  сокет для установки размера накопителя
 * @param event событие сокета
 * @param size  размер накопителя сокета
 * @return      установленный размер накопителя сокета
 *
 * @note Ответ снимается у системы повторным опросом, а не берётся из просьбы: система
 *       вправе отвести накопитель иного размера, и знать надлежит отведённый
 *
 */
int32_t awh::eth::Socket::setBufferSize(const net::socket_t sock, const net::socket_event_t event, const int32_t size) const noexcept {
	// Имя опции размера накопителя
	const int32_t option = ((event == net::socket_event_t::READ) ? SO_RCVBUF : SO_SNDBUF);
	// Если размер накопителя установить не удалось
	if(::setsockopt(static_cast <SOCKET> (sock), SOL_SOCKET, option, reinterpret_cast <const char *> (&size), static_cast <int32_t> (sizeof(size))) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем пустой размер накопителя
		return 0;
	}
	// Возвращаем размер накопителя, отведённый системой
	return this->getBufferSize(sock, event);
}

/**
 * @brief Метод установки числа переходов пакета
 *
 * @param sock     сокет для установки числа переходов
 * @param family   семейство адресов
 * @param delivery способ доставки
 * @param hops     число переходов
 * @return         результат выполнения установки
 *
 */
bool awh::eth::Socket::setHops(const net::socket_t sock, const event::family_t family, const event::delivery_mode_t delivery, const uint8_t hops) const noexcept {
	// Уровень опции числа переходов
	int32_t level = 0;
	// Имя опции числа переходов
	int32_t option = 0;
	/**
	 * Определяем семейство адресов сокета
	 */
	switch(static_cast <uint8_t> (family)){
		// Если семейство адресов является IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			// Устанавливаем уровень опции числа переходов
			level = IPPROTO_IP;
			// Устанавливаем имя опции числа переходов
			option = ((delivery == event::delivery_mode_t::MULTICAST) ? IP_MULTICAST_TTL : IP_TTL);
		} break;
		// Если семейство адресов является IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			// Устанавливаем уровень опции числа переходов
			level = IPPROTO_IPV6;
			// Устанавливаем имя опции числа переходов
			option = ((delivery == event::delivery_mode_t::MULTICAST) ? IPV6_MULTICAST_HOPS : IPV6_UNICAST_HOPS);
		} break;
		// Если семейство адресов неизвестно
		default: return false;
	}
	// Число переходов в понимании системы
	DWORD value = static_cast <DWORD> (hops);
	// Если число переходов установить не удалось
	if(::setsockopt(static_cast <SOCKET> (sock), level, option, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем отрицательный результат установки
		return false;
	}
	// Возвращаем положительный результат установки
	return true;
}

/**
 * @brief Метод получения числа переходов пакета
 *
 * @param sock     сокет для получения числа переходов
 * @param family   семейство адресов
 * @param delivery способ доставки
 * @return         число переходов пакета
 *
 */
uint8_t awh::eth::Socket::getHops(const net::socket_t sock, const event::family_t family, const event::delivery_mode_t delivery) const noexcept {
	// Уровень опции числа переходов
	int32_t level = 0;
	// Имя опции числа переходов
	int32_t option = 0;
	/**
	 * Определяем семейство адресов сокета
	 */
	switch(static_cast <uint8_t> (family)){
		// Если семейство адресов является IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			// Устанавливаем уровень опции числа переходов
			level = IPPROTO_IP;
			// Устанавливаем имя опции числа переходов
			option = ((delivery == event::delivery_mode_t::MULTICAST) ? IP_MULTICAST_TTL : IP_TTL);
		} break;
		// Если семейство адресов является IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			// Устанавливаем уровень опции числа переходов
			level = IPPROTO_IPV6;
			// Устанавливаем имя опции числа переходов
			option = ((delivery == event::delivery_mode_t::MULTICAST) ? IPV6_MULTICAST_HOPS : IPV6_UNICAST_HOPS);
		} break;
		// Если семейство адресов неизвестно
		default: return 0;
	}
	// Число переходов, снятое у системы
	DWORD value = 0;
	// Размер числа переходов
	int32_t size = static_cast <int32_t> (sizeof(value));
	// Если число переходов получить не удалось
	if(::getsockopt(static_cast <SOCKET> (sock), level, option, reinterpret_cast <char *> (&value), &size) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем пустое число переходов
		return 0;
	}
	// Возвращаем снятое число переходов
	return static_cast <uint8_t> (value);
}

/**
 * @brief Метод установки поддержания подключения в живых
 *
 * @param sock  сокет для установки поддержания подключения
 * @param cnt   максимальное количество попыток
 * @param idle  время простоя подключения в секундах
 * @param intvl интервал времени в секундах между попытками
 * @return      результат выполнения установки
 *
 * @note Число попыток у MS Windows задать нельзя вовсе: SIO_KEEPALIVE_VALS принимает
 *       лишь время простоя и промежуток между попытками, а число их система держит
 *       своё. Довод cnt потому не отбрасывается молча - о нём заносится
 *       предупреждение, когда он задан
 *
 */
bool awh::eth::Socket::setKeepalive(const net::socket_t sock, int32_t cnt, int32_t idle, int32_t intvl) const noexcept {
	// Признак поддержания подключения в живых
	DWORD mode = 1;
	// Если признак поддержания подключения установить не удалось
	if(::setsockopt(static_cast <SOCKET> (sock), SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast <const char *> (&mode), static_cast <int32_t> (sizeof(mode))) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем отрицательный результат установки
		return false;
	}
	// Если число попыток задано - сообщаем, что система его не принимает
	if(cnt > 0)
		// Заносим в журнал предупреждение о неприменимости числа попыток
		this->_log->print("%s: keepalive probe count is not settable on MS Windows, the value %d is ignored", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, cnt);
	// Если ни время простоя, ни промежуток между попытками не заданы
	if((idle <= 0) && (intvl <= 0))
		// Возвращаем положительный результат установки
		return true;
	// Настройки поддержания подключения в живых
	struct tcp_keepalive settings{};
	// Включаем поддержание подключения в живых
	settings.onoff = 1;
	// Устанавливаем время простоя подключения в миллисекундах
	settings.keepalivetime = static_cast <ULONG> ((idle > 0) ? (static_cast <ULONG> (idle) * 1000UL) : 7200000UL);
	// Устанавливаем промежуток между попытками в миллисекундах
	settings.keepaliveinterval = static_cast <ULONG> ((intvl > 0) ? (static_cast <ULONG> (intvl) * 1000UL) : 1000UL);
	// Количество байт, отданных управляющим вызовом
	DWORD returned = 0;
	// Если настройки поддержания подключения применить не удалось
	if(::WSAIoctl(static_cast <SOCKET> (sock), SIO_KEEPALIVE_VALS, &settings, static_cast <DWORD> (sizeof(settings)), nullptr, 0, &returned, nullptr, nullptr) == SOCKET_ERROR){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем отрицательный результат установки
		return false;
	}
	// Возвращаем положительный результат установки
	return true;
}

/**
 * @brief Метод установки сетевого устройства групповой рассылки
 *
 * @param sock   сокет для установки сетевого устройства
 * @param family семейство адресов
 * @param ifname название сетевого устройства
 * @return       результат выполнения установки
 *
 * @note Устройство задаётся номером, а не названием: у MS Windows опции с названием
 *       устройства нет вовсе, и перевод названия в номер ведётся здесь -
 *       if_nametoindex объявлен там же, где и прочие средства Winsock
 *
 */

/**
 * @brief Функция добычи номера сетевого устройства по его названию
 *
 * @details Название устройства у MS Windows - GUID вида `{F49A2CB0-...}`, и
 *          `if_nametoindex` его НЕ понимает: тот принимает синтетические названия
 *          вида `ethernet_6`, какие выдаёт `if_indextoname`. Номер оттого берётся
 *          из перечня устройств машины, где он лежит рядом с названием.
 *
 *          Установлено прогоном: набор терял `IoMulticastMembershipAfterCommitTest`,
 *          а в журнале стояло «устройство не найдено» при живом устройстве
 *
 * @note Сличение названий ведётся БЕЗ учёта регистра: система выдаёт GUID
 *       заглавными, а слои выше приводят названия к нижнему регистру - у систем
 *       POSIX названия устройств в нижнем регистре и живут
 *
 * @note Отступление к `if_nametoindex` оставлено: названия синтетического вида оно
 *       разбирает, а перечень устройств такого названия не содержит
 *
 * @param ifname название сетевого устройства
 * @param v6     признак семейства адресов IPv6
 * @return       номер сетевого устройства либо ноль, если устройство не найдено
 *
 */
static uint32_t __awh_iface_index__(const std::string_view ifname, const bool v6) noexcept {
	// Если название сетевого устройства не передано
	if(ifname.empty())
		// Выводим отсутствие номера устройства
		return 0;
	// Состав запрашиваемых сведений
	const ULONG flags = (GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME);
	// Объём буфера под перечень устройств
	ULONG size = 16384;
	// Буфер под перечень сетевых устройств
	std::vector <uint8_t> buffer(static_cast <size_t> (size));
	// Итог опроса перечня устройств
	ULONG code = ::GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, reinterpret_cast <PIP_ADAPTER_ADDRESSES> (buffer.data()), &size);
	// Если буфера не хватило - опрашиваем повторно с запрошенным объёмом
	if(code == ERROR_BUFFER_OVERFLOW){
		// Выполняем расширение буфера под перечень устройств
		buffer.resize(static_cast <size_t> (size));
		// Выполняем повторный опрос перечня устройств
		code = ::GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, reinterpret_cast <PIP_ADAPTER_ADDRESSES> (buffer.data()), &size);
	}
	// Если перечень устройств получить не удалось
	if(code != NO_ERROR)
		// Отступаем к переводу названия средствами сокетов
		return static_cast <uint32_t> (::if_nametoindex(std::string(ifname).c_str()));
	/**
	 * Выполняем перебор всех сетевых устройств машины
	 */
	for(PIP_ADAPTER_ADDRESSES adapter = reinterpret_cast <PIP_ADAPTER_ADDRESSES> (buffer.data()); adapter != nullptr; adapter = adapter->Next){
		// Если название устройства не задано
		if(adapter->AdapterName == nullptr)
			// Переходим к устройству следующему
			continue;
		// Признак совпадения названий
		bool same = true;
		// Порядковый номер сличаемого символа
		size_t i = 0;
		/**
		 * Выполняем сличение названий посимвольно, свёртывая регистр
		 *
		 * @note Свёртка своя, ASCII, а не средствами языка: `tolower` зависит от
		 *       местности, а GUID - данные протокольные
		 */
		for(; (i < ifname.size()) && (adapter->AdapterName[i] != '\0'); i++){
			// Получаем очередной символ искомого названия
			char first = ifname.at(i);
			// Получаем очередной символ названия, выданного системой
			char second = adapter->AdapterName[i];
			// Свёртываем регистр символа искомого названия
			if((first >= 'A') && (first <= 'Z'))
				// Приводим символ к нижнему регистру
				first = static_cast <char> (first + ('a' - 'A'));
			// Свёртываем регистр символа названия, выданного системой
			if((second >= 'A') && (second <= 'Z'))
				// Приводим символ к нижнему регистру
				second = static_cast <char> (second + ('a' - 'A'));
			// Если символы разошлись
			if(first != second){
				// Отмечаем названия разошедшимися
				same = false;
				// Завершаем сличение
				break;
			}
		}
		// Если названия совпали целиком
		if(same && (i == ifname.size()) && (adapter->AdapterName[i] == '\0'))
			// Выводим номер устройства того семейства, какое запрошено
			return static_cast <uint32_t> (v6 ? adapter->Ipv6IfIndex : adapter->IfIndex);
	}
	// Отступаем к переводу названия средствами сокетов
	return static_cast <uint32_t> (::if_nametoindex(std::string(ifname).c_str()));
}

bool awh::eth::Socket::setMulticastIface(const net::socket_t sock, const event::family_t family, string_view ifname) const noexcept {
	// Если название сетевого устройства не передано
	if(ifname.empty())
		// Возвращаем отрицательный результат установки
		return false;
	// Получаем номер сетевого устройства по его названию
	const uint32_t index = ::__awh_iface_index__(ifname, (family == event::family_t::IPV6));
	// Если номер сетевого устройства получить не удалось
	if(index == 0){
		// Заносим в журнал предупреждение об отсутствии сетевого устройства
		this->_log->print("%s: network interface \"%s\" could not be found", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, string(ifname).c_str());
		// Возвращаем отрицательный результат установки
		return false;
	}
	/**
	 * Определяем семейство адресов сокета
	 */
	switch(static_cast <uint8_t> (family)){
		// Если семейство адресов является IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): {
			/**
			 * Номер устройства передаётся адресом особого вида
			 *
			 * @details Опция IP_MULTICAST_IF у IPv4 принимает адрес устройства, а не его
			 *          номер. MS Windows принимает взамен номер, записанный в порядке
			 *          сети на месте адреса, - приём этот описан самой системой и иного
			 *          способа назвать устройство у IPv4 там нет
			 *
			 */
			// Номер устройства в порядке октетов сети
			const DWORD value = ::htonl(index);
			// Если сетевое устройство установить не удалось
			if(::setsockopt(static_cast <SOCKET> (sock), IPPROTO_IP, IP_MULTICAST_IF, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) != 0){
				// Записываем ошибку в лог
				this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
				// Возвращаем отрицательный результат установки
				return false;
			}
		} break;
		// Если семейство адресов является IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			// Номер устройства в понимании системы
			const DWORD value = static_cast <DWORD> (index);
			// Если сетевое устройство установить не удалось
			if(::setsockopt(static_cast <SOCKET> (sock), IPPROTO_IPV6, IPV6_MULTICAST_IF, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) != 0){
				// Записываем ошибку в лог
				this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
				// Возвращаем отрицательный результат установки
				return false;
			}
		} break;
		// Если семейство адресов неизвестно
		default: return false;
	}
	// Возвращаем положительный результат установки
	return true;
}

/**
 * @brief Метод получения способа обнаружения MTU
 *
 * @param sock   сокет для получения способа обнаружения MTU
 * @param family семейство адресов
 * @return       способ обнаружения MTU
 *
 * @note Способов у MS Windows четыре против десяти у самой библиотеки, и приводятся
 *       они к общему виду здесь: IP_PMTUDISC_DONT отвечает DONT, IP_PMTUDISC_DO - DO,
 *       IP_PMTUDISC_PROBE - PROBE. Остальным ответа у системы нет вовсе
 *
 */
awh::event::mtu_discover_t awh::eth::Socket::getMaximumTransmissionUnitDiscover(const net::socket_t sock, const event::family_t family) const noexcept {
	// Уровень опции обнаружения MTU
	const int32_t level = ((family == event::family_t::IPV6) ? IPPROTO_IPV6 : IPPROTO_IP);
	// Имя опции обнаружения MTU
	const int32_t option = ((family == event::family_t::IPV6) ? IPV6_MTU_DISCOVER : IP_MTU_DISCOVER);
	// Способ обнаружения MTU, снятый у системы
	DWORD value = 0;
	// Размер способа обнаружения MTU
	int32_t size = static_cast <int32_t> (sizeof(value));
	// Если способ обнаружения MTU получить не удалось
	if(::getsockopt(static_cast <SOCKET> (sock), level, option, reinterpret_cast <char *> (&value), &size) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем неопределённый способ обнаружения MTU
		return event::mtu_discover_t::NONE;
	}
	/**
	 * Определяем снятый способ обнаружения MTU
	 */
	switch(value){
		// Если обнаружение MTU выполнять не следует
		case IP_PMTUDISC_DONT: return event::mtu_discover_t::DONT;
		// Если обнаружение MTU выполнять следует
		case IP_PMTUDISC_DO: return event::mtu_discover_t::DO;
		// Если следует отправлять пробные пакеты
		case IP_PMTUDISC_PROBE: return event::mtu_discover_t::PROBE;
	}
	// Возвращаем неопределённый способ обнаружения MTU
	return event::mtu_discover_t::NONE;
}

/**
 * @brief Метод установки способа обнаружения MTU
 *
 * @param sock   сокет для установки способа обнаружения MTU
 * @param family семейство адресов
 * @param mode   способ обнаружения MTU
 * @return       результат выполнения установки
 *
 * @note Способы, ответа у системы не имеющие, отказом не отвечают молча: о них
 *       заносится предупреждение, а сама установка не ведётся
 *
 */
bool awh::eth::Socket::setMaximumTransmissionUnitDiscover(const net::socket_t sock, const event::family_t family, const event::mtu_discover_t mode) const noexcept {
	// Способ обнаружения MTU в понимании системы
	DWORD value = 0;
	/**
	 * Определяем заданный способ обнаружения MTU
	 */
	switch(static_cast <uint8_t> (mode)){
		// Если обнаружение MTU выполнять не следует
		case static_cast <uint8_t> (event::mtu_discover_t::DONT): value = IP_PMTUDISC_DONT; break;
		// Если обнаружение MTU выполнять следует
		case static_cast <uint8_t> (event::mtu_discover_t::WANT):
		case static_cast <uint8_t> (event::mtu_discover_t::DO): value = IP_PMTUDISC_DO; break;
		// Если следует отправлять пробные пакеты
		case static_cast <uint8_t> (event::mtu_discover_t::PROBE): value = IP_PMTUDISC_PROBE; break;
		// Если способ обнаружения MTU ответа у системы не имеет
		default: {
			// Заносим в журнал предупреждение об отсутствии ответа у системы
			this->_log->print("%s: MTU discover mode %u has no counterpart on MS Windows", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, static_cast <uint16_t> (mode));
			// Возвращаем отрицательный результат установки
			return false;
		}
	}
	// Уровень опции обнаружения MTU
	const int32_t level = ((family == event::family_t::IPV6) ? IPPROTO_IPV6 : IPPROTO_IP);
	// Имя опции обнаружения MTU
	const int32_t option = ((family == event::family_t::IPV6) ? IPV6_MTU_DISCOVER : IP_MTU_DISCOVER);
	// Если способ обнаружения MTU установить не удалось
	if(::setsockopt(static_cast <SOCKET> (sock), level, option, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) != 0){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем отрицательный результат установки
		return false;
	}
	// Возвращаем положительный результат установки
	return true;
}

/**
 * @brief Метод заведения сокета
 *
 * @param family семейство адресов
 * @param type   тип сокета
 * @param proto  протокол передачи данных
 * @return       заведённый сокет
 *
 * @note Сокет заводится наложенным (WSA_FLAG_OVERLAPPED), и иначе быть не может: у
 *       дескриптора без наложения система выстраивает операции в очередь, и запись из
 *       одного потока дожидается чтения из другого. Проверено опытом на именованном
 *       канале - обмен вставал намертво. Наложение же есть то самое устройство, на
 *       каком стоит порт завершения ввода-вывода
 *
 */
/**
 * @brief Метод проверки готовности средств сокетов системы
 *
 * @details Подъём ведётся здесь же, а не одной лишь проверкой уже поднятого: спросить
 *          систему, годна ли она заводить сокеты, иначе как подняв средства, нельзя -
 *          отдельного обращения об этом у неё нет вовсе
 *
 * @note Обращение это зовёт заведение движка, и отказ его движок выдаёт наружу отказом
 *       заведения: сеть там просят явно, и молчать об её негодности нельзя
 *
 * @return результат проверки готовности средств сокетов системы
 *
 */
bool awh::eth::Socket::ready() const noexcept {
	// Если поднять средства сокетов системы не удалось
	if(!::__awh_winsock__()){
		// Заносим отказ подъёма средств сокетов в журнал
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_winsock_failure__.c_str());
		// Выводим результат с ошибкой
		return false;
	}
	// Сообщаем, что средства сокетов системы годны
	return true;
}

awh::net::socket_t awh::eth::Socket::issue(const event::family_t family, const event::type_t type, const event::protocol_t proto, const uint16_t options) const noexcept {
	// Если поднять средства сокетов системы не удалось
	if(!::__awh_winsock__()){
		// Заносим отказ подъёма средств сокетов в журнал
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_winsock_failure__.c_str());
		// Возвращаем признак отсутствия заведённого сокета
		return static_cast <net::socket_t> (INVALID_SOCKET);
	}
	// Опции при создании сокета эта система не принимает, они накладываются отдельно
	(void) options;
	// Семейство адресов сокета в понимании системы
	int32_t domain = 0;
	/**
	 * Определяем семейство адресов сокета
	 */
	switch(static_cast <uint8_t> (family)){
		// Если семейство адресов является IPv4
		case static_cast <uint8_t> (event::family_t::IPV4): domain = AF_INET; break;
		// Если семейство адресов является IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): domain = AF_INET6; break;
		/**
		 * Если семейство адресов является UNIX-доменным
		 *
		 * @details Средства эти у MS Windows появились в 2018 году и с тех пор
		 *          доступны наравне с прочими: сокет заводится AF_UNIX, путь его
		 *          задаётся `struct sockaddr_un`, а объявлено всё это отдельным
		 *          заголовком `afunix.h`
		 *
		 * @warning Потоковыми они у MS Windows и остаются: дейтаграммного
		 *          UNIX-доменного сокета система не несёт вовсе, и отказ по нему -
		 *          не заглушка, а свойство системы. Отсекается он ниже, разбором
		 *          вида сокета
		 *
		 */
		case static_cast <uint8_t> (event::family_t::UDS): domain = AF_UNIX; break;
		// Если семейство адресов неизвестно
		default: {
			// Заносим в журнал предупреждение о неподдерживаемом семействе адресов
			this->_log->print("%s: address family %u is not supported", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, static_cast <uint16_t> (family));
			// Возвращаем признак отсутствия заведённого сокета
			return static_cast <net::socket_t> (INVALID_SOCKET);
		}
	}
	// Тип сокета в понимании системы
	int32_t kind = 0;
	/**
	 * Определяем тип сокета
	 */
	switch(static_cast <uint8_t> (type)){
		// Если сокет является потоковым
		case static_cast <uint8_t> (event::type_t::STREAM): kind = SOCK_STREAM; break;
		// Если сокет является дейтаграммным
		case static_cast <uint8_t> (event::type_t::DATAGRAM): kind = SOCK_DGRAM; break;
		// Если сокет является сырым
		case static_cast <uint8_t> (event::type_t::RAW): kind = SOCK_RAW; break;
		/**
		 * Сокета последовательных пакетов у MS Windows нет вовсе
		 *
		 * @details Держится он на SCTP, какого система не несёт ни в каком виде: ни
		 *          заголовков, ни поддержки в ядре. Отказ здесь потому не временный, и
		 *          заглушкой он не является
		 *
		 */
		case static_cast <uint8_t> (event::type_t::SEQPACKET): {
			// Заносим в журнал предупреждение об отсутствии типа сокета у системы
			this->_log->print("%s: sequenced packet sockets are not available on MS Windows", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__);
			// Возвращаем признак отсутствия заведённого сокета
			return static_cast <net::socket_t> (INVALID_SOCKET);
		}
		// Если тип сокета неизвестен
		default: {
			// Заносим в журнал предупреждение о неподдерживаемом типе сокета
			this->_log->print("%s: socket type %u is not supported", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, static_cast <uint16_t> (type));
			// Возвращаем признак отсутствия заведённого сокета
			return static_cast <net::socket_t> (INVALID_SOCKET);
		}
	}
	/**
	 * Если сокет UNIX-доменный - разбираем его особо
	 *
	 * @details Протокол ему не задаётся вовсе: у семейства AF_UNIX его нет, и
	 *          передача сюда IPPROTO_TCP оканчивается отказом заведения
	 *
	 * @warning Дейтаграммного UNIX-доменного сокета у MS Windows нет: система несёт
	 *          лишь потоковый. Отказ этот не временный и заглушкой не является -
	 *          подменять его потоковым сокетом было бы подлогом, границы сообщений
	 *          на потоке не держатся
	 *
	 */
	if(domain == AF_UNIX){
		// Если сокет не потоковый
		if(kind != SOCK_STREAM){
			// Заносим в журнал предупреждение об отсутствии такого сокета у системы
			this->_log->print("%s: only stream UNIX-domain sockets are available on MS Windows", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__);
			// Возвращаем признак отсутствия заведённого сокета
			return static_cast <net::socket_t> (INVALID_SOCKET);
		}
		// Выполняем заведение наложенного UNIX-доменного сокета
		const SOCKET single = ::WSASocketW(domain, kind, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
		// Если сокет завести не удалось
		if(single == INVALID_SOCKET)
			// Записываем ошибку в лог
			this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Выводим заведённый сокет
		return static_cast <net::socket_t> (single);
	}
	// Протокол передачи данных в понимании системы
	int32_t protocol = 0;
	/**
	 * Определяем протокол передачи данных
	 */
	switch(static_cast <uint8_t> (proto)){
		// Если протоколом передачи данных является TCP
		case static_cast <uint8_t> (event::protocol_t::TCP): protocol = IPPROTO_TCP; break;
		// Если протоколом передачи данных является UDP
		case static_cast <uint8_t> (event::protocol_t::UDP): protocol = IPPROTO_UDP; break;
		/**
		 * Если протоколом передачи данных является ICMP
		 *
		 * @details Протокол этот выбирается по семейству адресов: у IPv4 он свой, у
		 *          IPv6 - отдельный, и подменять один другим нельзя. Прочие платформы
		 *          заводят его так же (`bsd`, `gnu`, `sun`)
		 *
		 * @warning Прежде ICMP попадал в ветвь по умолчанию и сокет заводился с
		 *          протоколом 0. Система такой сокет создаёт, отправку принимает и
		 *          отвечает успехом, но пакет никуда не уходит и откликов не приходит:
		 *          установлено щупом на стенде Windows ARM64 (20.08.2026), где голый
		 *          сокет с `IPPROTO_ICMP` получал отклик за миллисекунды, а движок не
		 *          получал ничего за двенадцать секунд
		 *
		 */
		case static_cast <uint8_t> (event::protocol_t::ICMP):
			// Устанавливаем протокол ICMP по семейству адресов
			protocol = ((domain == AF_INET6) ? IPPROTO_ICMPV6 : IPPROTO_ICMP);
		break;
		// Если протокол передачи данных не задан - система выбирает его сама по типу
		default: protocol = 0;
	}
	// Выполняем заведение наложенного сокета
	const SOCKET result = ::WSASocketW(domain, kind, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
	// Если сокет завести не удалось
	if(result == INVALID_SOCKET)
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
	/**
	 * Если сокет заведён дейтаграммным - отключаем отчёт о недоступности получателя
	 *
	 * @details Своенравность эта у MS Windows своя, и у систем POSIX ей нет
	 *          соответствия вовсе: получив ICMP о недоступности порта в ответ на
	 *          СВОЮ прежнюю отправку, дейтаграммный сокет отвечает отказом
	 *          `WSAECONNRESET` на следующий ПРИЁМ - тем самым выдавая чужую беду за
	 *          свою. Дейтаграммный обмен от этого рвётся на ровном месте: сервер,
	 *          ответивший ушедшему клиенту, перестаёт принимать от прочих
	 *
	 * @details Замерено щупом на стенде: без отключения `recvfrom` отвечает отказом
	 *          10054 сразу после отправки на закрытый порт, с отключением - ждёт
	 *          данных, как ему и положено
	 *
	 * @note Отключение ставится при заведении сокета, а не отдельной настройкой:
	 *       выбора здесь нет - поведение это ошибочно для всякого дейтаграммного
	 *       обмена, и оставлять его включённым незачем ни в одном случае
	 *
	 */
	else if(kind == SOCK_DGRAM){
		// Признак отчёта о недоступности получателя
		BOOL report = FALSE;
		// Размер ответа обращения, обращению обязательный
		DWORD bytes = 0;
		// Выполняем отключение отчёта о недоступности получателя
		if(::WSAIoctl(result, SIO_UDP_CONNRESET, &report, sizeof(report), nullptr, 0, &bytes, nullptr, nullptr) != 0)
			// Записываем ошибку в лог
			this->_log->print("%s: cannot disable datagram connection reset reporting: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
	}
	// Возвращаем заведённый сокет
	return static_cast <net::socket_t> (result);
}

/**
 * @brief Метод получения опций, принимаемых при создании сокета
 *
 * @note Эта система опций при создании сокета не принимает вовсе, оттого
 * набор всегда пуст, и опции накладываются отдельными обращениями
 *
 * @param options набор опций события
 * @return        подмножество опций, наложенных при создании сокета
 *
 */
uint16_t awh::eth::Socket::inborn(const uint16_t options) const noexcept {
	// Опции при создании сокета эта система не принимает
	(void) options;
	// Выводим пустой набор опций
	return event::options::NONE;
}


/**
 * @brief Метод заведения пары сокетов для обмена сообщениями между процессами
 *
 * @param family семейство адресов сокета
 * @param type   тип сокета
 * @param proto  протокол сокета
 * @return       пара заведённых сокетов
 *
 * @details Соответствия socketpair у MS Windows нет вовсе, и пара строится в обход:
 *          заводится слушатель на петлевом устройстве с нулевым портом, к нему
 *          выполняется подключение, а принятое подключение и даёт вторую сторону
 *          пары. Слушатель затем закрывается - он был нужен лишь для сведения сторон
 *
 * @note Порт слушателю не задаётся: система выбирает свободный сама, и узнаётся он
 *       затем через getsockname. Оттого пара заводится без всякой оглядки на то,
 *       занят ли какой-либо порт
 *
 * @details Тип пары соблюдается, а не подменяется молча:
 *
 *          | Тип | Устройство под MS Windows | Границы | Надёжность |
 *          |---|---|---|---|
 *          | `STREAM` | петлевая пара TCP | нет | да |
 *          | `DATAGRAM` | петлевая пара UDP | да | петлевая доставка |
 *          | `SEQPACKET` | именованный канал в строе сообщений | да | да |
 *
 * @warning Пара последовательных пакетов стоит **не на сокете**, и это следует держать
 *          в уме всюду, где она попадает в чужие руки:
 *
 *          - обмен ведётся ReadFile и WriteFile, а не recv и send;
 *          - закрывается дескриптор через CloseHandle, а не через closesocket;
 *          - опции сокета к нему неприменимы вовсе.
 *
 *          Различать пути обязан тот, кто дескриптором пользуется, и признака для того
 *          заводить не требуется: тип узла он уже несёт в своём состоянии, а вид пары
 *          определяется им однозначно
 *
 * @note Численно дескриптор канала в net::socket_t помещается: под MS Windows тип этот
 *       заведён как uintptr_t, а не как int32_t, каким он бывает у POSIX
 *
 * @warning Сокеты заводятся наложенными: у дескриптора без наложения система
 *          выстраивает операции в очередь, и запись из одного потока дожидается
 *          чтения из другого. Проверено опытом на именованном канале - обмен вставал
 *          намертво
 *
 */
array <awh::net::socket_t, 2> awh::eth::Socket::ipc(const event::family_t family, const event::type_t type, const event::protocol_t proto) const noexcept {
	// Имя заведённого канала, вызывающей стороне не нужное
	string name;
	// Выводим пару связанных концов обмена
	return this->ipc(family, type, proto, name);
}
/**
 * @brief Метод создания пары связанных концов обмена с выдачей имени канала
 *
 * @param family семейство адресов
 * @param type   тип сокета
 * @param proto  протокол сокета
 * @param name   имя заведённого канала
 * @return       пара связанных концов обмена
 *
 */
awh::net::socket_t awh::eth::Socket::channel(const string & name) const noexcept {
	// Если имя канала не передано
	if(name.empty()){
		// Заносим отсутствие имени канала в журнал
		this->_log->print("%s: named pipe cannot be opened without a name", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__);
		// Выводим незаведённый описатель
		return net::invalid_socket_t;
	}
	// Получаем имя канала в понимании системы
	const std::wstring pipe = this->_fmk->convert(name);
	/**
	 * Срок ожидания освобождения экземпляра канала в миллисекундах
	 *
	 * @details Ждать приходится оттого, что стороны выходят на канал не по порядку:
	 *          назвавшая сторона освобождает свой экземпляр не мгновенно, а дойдя до
	 *          подписки на приём, тогда как порождённый процесс стучится сразу, едва
	 *          запустившись. Отказ в такой миг означает не занятость чужим, а лишь
	 *          то, что встречная сторона ещё не готова
	 */
	static constexpr uint32_t TIMEOUT = 5000;
	// Описатель открытого конца канала
	HANDLE result = INVALID_HANDLE_VALUE;
	/**
	 * Открываем свой конец канала наложенным
	 *
	 * @note Наложение обязательно: обмен ведётся портом завершений, а у описателя без
	 *       наложения система выстраивает обращения в очередь, и запись из одного
	 *       потока дожидается чтения из другого
	 */
	while((result = ::CreateFileW(pipe.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr)) == INVALID_HANDLE_VALUE){
		// Если отказ вызван не занятостью всех экземпляров канала - открывать нечего
		if(::GetLastError() != ERROR_PIPE_BUSY)
			// Прерываем ожидание освобождения экземпляра канала
			break;
		// Если экземпляр канала за отведённый срок не освободился
		if(!::WaitNamedPipeW(pipe.c_str(), static_cast <DWORD> (TIMEOUT))){
			// Заносим истечение срока ожидания в журнал
			this->_log->print("%s: named pipe [%s] is busy, error %lu", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, name.c_str(), ::GetLastError());
			// Выводим незаведённый описатель
			return net::invalid_socket_t;
		}
	}
	// Если открыть свой конец канала не удалось
	if(result == INVALID_HANDLE_VALUE){
		// Заносим отказ открытия канала в журнал
		this->_log->print("%s: named pipe [%s] could not be opened, error %lu", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, name.c_str(), ::GetLastError());
		// Выводим незаведённый описатель
		return net::invalid_socket_t;
	}
	// Строй чтения открытого конца канала
	DWORD mode = PIPE_READMODE_MESSAGE;
	/**
	 * Переводим открытый конец канала в строй сообщений
	 *
	 * @note Строй этот обязан совпадать со строем заведённой стороны: подача готовности
	 *       к приёму держится именно на нём - чтение нулевой длины ждёт прихода
	 *       сообщения лишь у канала, работающего сообщениями, а у канала, работающего
	 *       потоком октетов, оно отвечает сразу и обращается в холостой оборот
	 */
	if(!::SetNamedPipeHandleState(result, &mode, nullptr, nullptr))
		// Заносим отказ перевода канала в строй сообщений в журнал
		this->_log->print("%s: named pipe [%s] could not be switched to the message mode, error %lu", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, name.c_str(), ::GetLastError());
	// Выводим описатель открытого конца канала
	return reinterpret_cast <net::socket_t> (result);
}
/**
 * @brief Метод создания пары связанных концов обмена с выдачей имени канала
 *
 * @param family семейство адресов
 * @param type   тип сокета
 * @param proto  протокол сокета
 * @param name   имя заведённого канала
 * @return       пара связанных концов обмена
 *
 */
array <awh::net::socket_t, 2> awh::eth::Socket::ipc(const event::family_t family, const event::type_t type, const event::protocol_t proto, string & name) const noexcept {
	// Выполняем сброс имени заведённого канала
	name.clear();
	// Переменная результата
	array <net::socket_t, 2> result = {
		net::invalid_socket_t,
		net::invalid_socket_t
	};
	// Если поднять средства сокетов системы не удалось
	if(!::__awh_winsock__()){
		// Заносим отказ подъёма средств сокетов в журнал
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_winsock_failure__.c_str());
		// Возвращаем пару незаведённых сокетов
		return result;
	}
	/**
	 * Если пара запрошена для семейств IP - выдаём два НЕСВЯЗАННЫХ сокета
	 *
	 * @details Связанной пары семейства эти не просят вовсе, и строить её здесь нельзя.
	 *          Обращение это зовут заведением пары событий «клиент и сервер», и всякий
	 *          из них дальше распоряжается своим сокетом сам: сервер привязывает его к
	 *          своему адресу и слушает, клиент подключается к цели. Отдай мы им пару
	 *          связанную - сокеты пришли бы уже привязанными и подключёнными, и
	 *          привязка сервера отвергалась бы системой
	 *
	 * @note Установлено прогоном: сокет приходил привязанным к порту, выбранному
	 *       построением пары, и `bind` отвечал отказом 10022 (`WSAEINVAL`). Отказ этот
	 *       валил ВСЕ проверки сокетов разом - и потоковые, и дейтаграммные
	 *
	 * @note Так же поступают и эталонные слои: у них семейства IP выдают два сокета
	 *       обычным заведением, а связанную пару строят одни лишь PIPE и UDS
	 */
	if((family == event::family_t::IPV4) || (family == event::family_t::IPV6)){
		/**
		 * Заводим нужное количество сокетов
		 */
		for(net::socket_t & sock : result)
			// Выполняем заведение сокета по заданным доводам
			sock = this->issue(family, type, proto);
		// Возвращаем заведённые сокеты
		return result;
	}
	// Тип сокетов пары в понимании системы
	int32_t kind = 0;
	// Протокол сокетов пары в понимании системы
	int32_t protocol = 0;
	/**
	 * Определяем запрошенный тип пары
	 */
	switch(static_cast <uint8_t> (type)){
		// Если пара запрошена потоковой
		case static_cast <uint8_t> (event::type_t::NONE):
		case static_cast <uint8_t> (event::type_t::STREAM): {
			// Устанавливаем потоковый тип сокетов пары
			kind = SOCK_STREAM;
			// Устанавливаем протокол сокетов пары
			protocol = IPPROTO_TCP;
		} break;
		// Если пара запрошена дейтаграммной
		case static_cast <uint8_t> (event::type_t::DATAGRAM): {
			// Устанавливаем дейтаграммный тип сокетов пары
			kind = SOCK_DGRAM;
			// Устанавливаем протокол сокетов пары
			protocol = IPPROTO_UDP;
		} break;
		/**
		 * Если пара запрошена последовательными пакетами - строим её именованным каналом
		 *
		 * @details Сокетом последовательные пакеты у MS Windows не выходят вовсе: они
		 *          требуют границ сообщений и надёжности разом. Даёт то и другое лишь
		 *          именованный канал в строе сообщений, и пара этого вида стоит на нём.
		 *          Подставлять взамен дейтаграммную пару, как то делает эталонный
		 *          бэкенд на macOS и OpenBSD, здесь не потребовалось: там подстановка
		 *          вынужденная, а тут есть устройство с той же семантикой
		 *
		 */
		case static_cast <uint8_t> (event::type_t::SEQPACKET): {
			/**
			 * Составляем имя именованного канала
			 *
			 * @details Имя обязано быть неповторимым в пределах всей машины: пространство
			 *          имён каналов у MS Windows общее, и совпадение имён свело бы вместе
			 *          два несвязанных обмена. Неповторимость даётся парой «номер процесса
			 *          - порядковый номер пары»: первый разводит процессы между собой,
			 *          второй разводит пары внутри одного процесса
			 *
			 */
			// Порядковый номер заводимой пары
			static std::atomic_uint64_t counter{0};
			// Показания высокоточного счётчика системы
			LARGE_INTEGER ticks{};
			// Выполняем снятие показаний высокоточного счётчика
			::QueryPerformanceCounter(&ticks);
			/**
			 * Составляем однозначное имя именованного канала
			 *
			 * @details Пространство имён каналов общее на всю систему, и однозначность
			 *          имени здесь не удобство, а условие работы. Складывается оно из
			 *          номера процесса, порядкового номера пары внутри него и показаний
			 *          высокоточного счётчика: первое разводит приложения, второе - пары
			 *          внутри приложения, третье - приложения, запущенные разом
			 *
			 * @note Обычных часов для последнего мало: ход их у MS Windows зернист -
			 *       шагом в 15.6 мс, - и два приложения, запущенные в один шаг, получили
			 *       бы совпадающее значение
			 */
			const std::wstring pipe = (
				L"\\\\.\\pipe\\awh-ipc-" +
				std::to_wstring(static_cast <uint32_t> (::GetCurrentProcessId())) + L"-" +
				std::to_wstring(++counter) + L"-" +
				std::to_wstring(static_cast <uint64_t> (ticks.QuadPart))
			);
			/**
			 * Заводим сторону канала, ожидающую подключения
			 *
			 * @note Обмен ведётся наложенным: у дескриптора без наложения система
			 *       выстраивает операции в очередь, и запись из одного потока дожидается
			 *       чтения из другого. Проверено опытом - обмен вставал намертво
			 *
			 */
			HANDLE server = ::CreateNamedPipeW(
				pipe.c_str(),
				/**
				 * Признак первого экземпляра здесь обязателен
				 *
				 * @note Без него заведение канала с уже занятым именем НЕ отвергается:
				 *       система заводит ещё один экземпляр того же канала, и чужой
				 *       работник вправе подключиться к нашему концу. С признаком
				 *       занятость имени оборачивается честным отказом
				 */
				PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
				PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
				2, 65536, 65536, 0, nullptr
			);
			// Если сторону канала завести не удалось
			if(server == INVALID_HANDLE_VALUE){
				// Записываем ошибку в лог
				this->_log->print("%s: named pipe could not be created, error %lu", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::GetLastError());
				// Возвращаем пустую пару
				return result;
			}
			// Открываем встречный конец канала
			HANDLE client = ::CreateFileW(pipe.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
			// Если встречный конец канала открыть не удалось
			if(client == INVALID_HANDLE_VALUE){
				// Записываем ошибку в лог
				this->_log->print("%s: named pipe could not be opened, error %lu", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::GetLastError());
				// Закрываем заведённую сторону канала
				::CloseHandle(server);
				// Возвращаем пустую пару
				return result;
			}
			// Строй чтения встречного конца канала
			DWORD mode = PIPE_READMODE_MESSAGE;
			/**
			 * Переводим встречный конец канала в строй сообщений
			 *
			 * @note Отказ здесь не смертелен, но оглашается: канал заведён строем сообщений,
			 *       и встречный конец, оставшийся строем октетов, читал бы поток без границ
			 *       сообщений - расхождение это всплыло бы далеко от места
			 */
			if(!::SetNamedPipeHandleState(client, &mode, nullptr, nullptr))
				// Записываем ошибку в лог
				this->_log->print("%s: named pipe could not be switched to message mode, error %lu", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::GetLastError());
			// Выдаём имя заведённого канала вызывающей стороне
			name = this->_fmk->convert(pipe);
			// Запоминаем сторону канала, ожидающую подключения
			result[0] = reinterpret_cast <net::socket_t> (server);
			// Запоминаем встречный конец канала
			result[1] = reinterpret_cast <net::socket_t> (client);
			// Возвращаем заведённую пару
			return result;
		}
		// Если пара запрошена типом, какого у системы нет вовсе
		default: {
			// Заносим в журнал предупреждение об отсутствии типа пары у системы
			this->_log->print("%s: socket pair of type %u is not available on MS Windows", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, static_cast <uint16_t> (type));
			// Возвращаем пустую пару сокетов
			return result;
		}
	}
	/**
	 * Дейтаграммная пара строится встречной привязкой, без слушателя
	 *
	 * @details Слушателя дейтаграммам не нужно вовсе: оба сокета привязываются к
	 *          петлевому устройству с нулевым портом, узнают выбранные системой порты
	 *          друг друга и подключаются встречно. Подключение дейтаграммного сокета
	 *          обмена не заводит, а лишь закрепляет за ним собеседника - тем и
	 *          выходит пара
	 *
	 * @note Границы сообщений такая пара сохраняет, а вот доставку обещает лишь в той
	 *       мере, в какой её обещает петлевое устройство: потерь там не бывает, но
	 *       правилом это не закреплено
	 *
	 */
	if(kind == SOCK_DGRAM){
		// Адреса сторон пары
		struct sockaddr_in first{}, second{};
		// Размеры адресов сторон пары
		int32_t firstSize = static_cast <int32_t> (sizeof(first)), secondSize = static_cast <int32_t> (sizeof(second));
		// Заводим первую сторону пары наложенным сокетом
		const SOCKET one = ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
		// Заводим вторую сторону пары наложенным сокетом
		const SOCKET two = ((one != INVALID_SOCKET) ? ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED) : INVALID_SOCKET);
		/**
		 * Заводим дейтаграммную пару
		 *
		 * @note Цикл здесь однопроходный и заведён ради единой точки уборки
		 *
		 */
		do {
			// Если хоть одну из сторон пары завести не удалось
			if((one == INVALID_SOCKET) || (two == INVALID_SOCKET))
				// Выходим из цикла заведения пары
				break;
			// Выполняем обнуление адреса первой стороны пары
			::memset(&first, 0, sizeof(first));
			// Устанавливаем семейство адресов первой стороны пары
			first.sin_family = AF_INET;
			// Устанавливаем петлевое сетевое устройство первой стороне пары
			first.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
			// Копируем заготовку адреса второй стороне пары
			second = first;
			// Если привязать первую сторону пары не удалось
			if(::bind(one, reinterpret_cast <struct sockaddr *> (&first), static_cast <int32_t> (sizeof(first))) == SOCKET_ERROR)
				// Выходим из цикла заведения пары
				break;
			// Если привязать вторую сторону пары не удалось
			if(::bind(two, reinterpret_cast <struct sockaddr *> (&second), static_cast <int32_t> (sizeof(second))) == SOCKET_ERROR)
				// Выходим из цикла заведения пары
				break;
			// Если узнать адрес первой стороны пары не удалось
			if(::getsockname(one, reinterpret_cast <struct sockaddr *> (&first), &firstSize) == SOCKET_ERROR)
				// Выходим из цикла заведения пары
				break;
			// Если узнать адрес второй стороны пары не удалось
			if(::getsockname(two, reinterpret_cast <struct sockaddr *> (&second), &secondSize) == SOCKET_ERROR)
				// Выходим из цикла заведения пары
				break;
			/**
			 * Восполняем поля, каких система не заполняет
			 *
			 * @details Getsockname у MS Windows вправе выставить один лишь номер порта,
			 *          оставив семейство адресов и сам адрес нетронутыми
			 *
			 */
			// Устанавливаем семейство адресов первой стороны пары
			first.sin_family = AF_INET;
			// Устанавливаем петлевое сетевое устройство первой стороне пары
			first.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
			// Устанавливаем семейство адресов второй стороны пары
			second.sin_family = AF_INET;
			// Устанавливаем петлевое сетевое устройство второй стороне пары
			second.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
			// Если закрепить за первой стороной пары вторую не удалось
			if(::connect(one, reinterpret_cast <struct sockaddr *> (&second), static_cast <int32_t> (sizeof(second))) == SOCKET_ERROR)
				// Выходим из цикла заведения пары
				break;
			// Если закрепить за второй стороной пары первую не удалось
			if(::connect(two, reinterpret_cast <struct sockaddr *> (&first), static_cast <int32_t> (sizeof(first))) == SOCKET_ERROR)
				// Выходим из цикла заведения пары
				break;
			// Запоминаем первую сторону пары
			result[0] = static_cast <net::socket_t> (one);
			// Запоминаем вторую сторону пары
			result[1] = static_cast <net::socket_t> (two);
			// Возвращаем заведённую пару сокетов
			return result;
		} while(false);
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Если первая сторона пары заведена - закрываем её
		if(one != INVALID_SOCKET)
			// Закрываем первую сторону пары
			::closesocket(one);
		// Если вторая сторона пары заведена - закрываем её
		if(two != INVALID_SOCKET)
			// Закрываем вторую сторону пары
			::closesocket(two);
		// Возвращаем пустую пару сокетов
		return result;
	}

	/**
	 * Объединение видов адреса слушателя
	 *
	 * @note Объединение нужно затем, что bind и getsockname принимают адрес общего
	 *       вида, а заполняется он видом IPv4: приведение указателем обошлось бы
	 *       нарушением правил обращения к памяти
	 *
	 */
	union {
		struct sockaddr_in inaddr; // Адрес слушателя вида IPv4
		struct sockaddr addr;      // Адрес слушателя общего вида
	} address;
	// Размер адреса слушателя
	int32_t length = static_cast <int32_t> (sizeof(address.inaddr));
	// Заводим сокет слушателя
	const SOCKET listener = ::socket(AF_INET, kind, protocol);
	// Если сокет слушателя завести не удалось
	if(listener == INVALID_SOCKET){
		// Записываем ошибку в лог
		this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Возвращаем пустую пару сокетов
		return result;
	}
	// Выполняем обнуление адреса слушателя
	::memset(&address, 0, sizeof(address));
	// Устанавливаем семейство адресов слушателя
	address.inaddr.sin_family = AF_INET;
	// Устанавливаем петлевое сетевое устройство
	address.inaddr.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
	// Порт слушателю не задаём - система выберет свободный сама
	address.inaddr.sin_port = 0;
	// Сокеты пары, заводимые ниже
	SOCKET first = INVALID_SOCKET, second = INVALID_SOCKET;
	/**
	 * Заводим пару сокетов
	 *
	 * @note Цикл здесь однопроходный и заведён ради единой точки уборки: всякий отказ
	 *       выходит из него, а уборка ведётся один раз ниже
	 *
	 */
	do {
		// Признак разрешения повторного использования адреса
		const int32_t reuse = 1;
		// Если признак повторного использования адреса установить не удалось
		if(::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast <const char *> (&reuse), static_cast <int32_t> (sizeof(reuse))) == SOCKET_ERROR)
			// Выходим из цикла заведения пары
			break;
		// Если привязать слушателя к адресу не удалось
		if(::bind(listener, &address.addr, static_cast <int32_t> (sizeof(address.inaddr))) == SOCKET_ERROR)
			// Выходим из цикла заведения пары
			break;
		// Выполняем обнуление адреса слушателя перед его опросом
		::memset(&address, 0, sizeof(address));
		// Если узнать адрес слушателя не удалось
		if(::getsockname(listener, &address.addr, &length) == SOCKET_ERROR)
			// Выходим из цикла заведения пары
			break;
		/**
		 * Восполняем поля, каких система не заполняет
		 *
		 * @details Getsockname у MS Windows вправе выставить один лишь номер порта,
		 *          оставив семейство адресов и сам адрес нетронутыми. Описано это самой
		 *          системой; полагаться на заполнение их нельзя
		 *
		 */
		// Устанавливаем семейство адресов слушателя
		address.inaddr.sin_family = AF_INET;
		// Устанавливаем петлевое сетевое устройство
		address.inaddr.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
		// Если запустить прослушивание не удалось
		if(::listen(listener, 1) == SOCKET_ERROR)
			// Выходим из цикла заведения пары
			break;
		// Заводим первую сторону пары наложенным сокетом
		first = ::WSASocketW(AF_INET, kind, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
		// Если первую сторону пары завести не удалось
		if(first == INVALID_SOCKET)
			// Выходим из цикла заведения пары
			break;
		// Если подключить первую сторону пары к слушателю не удалось
		if(::connect(first, &address.addr, static_cast <int32_t> (sizeof(address.inaddr))) == SOCKET_ERROR)
			// Выходим из цикла заведения пары
			break;
		// Принимаем подключение - оно и даёт вторую сторону пары
		second = ::accept(listener, nullptr, nullptr);
		// Если вторую сторону пары принять не удалось
		if(second == INVALID_SOCKET)
			// Выходим из цикла заведения пары
			break;
		// Закрываем сокет слушателя - сведя стороны, он больше не нужен
		::closesocket(listener);
		// Запоминаем первую сторону пары
		result[0] = static_cast <net::socket_t> (first);
		// Запоминаем вторую сторону пары
		result[1] = static_cast <net::socket_t> (second);
		// Возвращаем заведённую пару сокетов
		return result;
	} while(false);
	// Записываем ошибку в лог
	this->_log->print("%s: %s", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
	// Закрываем сокет слушателя
	::closesocket(listener);
	// Если первая сторона пары заведена - закрываем её
	if(first != INVALID_SOCKET)
		// Закрываем первую сторону пары
		::closesocket(first);
	// Если вторая сторона пары заведена - закрываем её
	if(second != INVALID_SOCKET)
		// Закрываем вторую сторону пары
		::closesocket(second);
	// Возвращаем пустую пару сокетов
	return result;
}

/**
 * @brief Метод получения класса обслуживания в заголовке IP-пакета
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов
 * @return       класс обслуживания
 *
 * @warning Отдаётся здесь запрошенное, а не показание системы. Настройка IP_TOS у MS
 *          Windows существует, но с версии Windows XP SP2 не исполняется: запись
 *          отвечает согласием, а в заголовок пакета не попадает ничего. Отметка
 *          ставится подсистемой качества обслуживания qWAVE, а обратного чтения она
 *          не даёт вовсе - оттого выдаётся то, что запрашивали
 *
 */
awh::event::dscp_t awh::eth::Socket::getDifferentiatedServicesCodePoint(const net::socket_t sock, [[maybe_unused]] const event::family_t family) const noexcept {
	// Выводим запрошенный для сокета класс обслуживания
	return win::qos::mark(sock);
}
/**
 * @brief Метод установки класса обслуживания в заголовке IP-пакета
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов
 * @param dscp   устанавливаемый класс обслуживания
 * @return       результат выполнения установки
 *
 * @note Отметка ставится сразу, если сокет уже соединён - подсистема берёт назначение
 *       у него самого. Иначе она запоминается и будет применена, едва назначение
 *       станет известно: назначения требует сама подсистема, а порядок вызовов ей не
 *       подчинён
 *
 * @warning Заведённый подсистемой поток обязан быть снят прежде закрытия сокета
 *
 */
bool awh::eth::Socket::setDifferentiatedServicesCodePoint(const net::socket_t sock, [[maybe_unused]] const event::family_t family, const event::dscp_t dscp) const noexcept {
	// Выводим результат установки класса обслуживания сокету
	return win::qos::mark(sock, dscp, this->_log);
}

/**
 * @brief Метод получения признака перегрузки в заголовке IP-пакета
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов
 * @return       признак перегрузки
 *
 * @note У систем POSIX признак этот занимает два младших разряда того же октета, где
 *       стоит класс обслуживания, и читается вместе с ним настройкой IP_TOS. MS
 *       Windows развела их порознь: признак перегрузки читается своей настройкой
 *       IP_ECN и класса обслуживания при себе не несёт - оттого прикрывать чтение
 *       разрядной маской здесь не приходится
 *
 */
awh::event::ecn_t awh::eth::Socket::getExplicitCongestionNotification(const net::socket_t sock, const event::family_t family) const noexcept {
	// Если сокет не передан
	if(sock == net::invalid_socket_t)
		// Выводим отсутствие признака перегрузки
		return event::ecn_t::NOT_ECT;
	// Считываемое значение признака перегрузки
	int32_t value = 0;
	// Размер считываемого значения
	int32_t length = static_cast <int32_t> (sizeof(value));
	// Уровень настройки и её название
	int32_t level = IPPROTO_IP, option = IP_ECN;
	// Если сокет работает по протоколу IPv6
	if(family == event::family_t::IPV6){
		// Устанавливаем уровень настройки
		level = IPPROTO_IPV6;
		// Устанавливаем название настройки
		option = IPV6_ECN;
	}
	// Если считать признак перегрузки не удалось
	if(::getsockopt(sock, level, option, reinterpret_cast <char *> (&value), &length) != 0){
		// Выводим в журнал сообщение о невозможности чтения признака перегрузки
		this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Выводим отсутствие признака перегрузки
		return event::ecn_t::NOT_ECT;
	}
	// Выводим считанный признак перегрузки
	return static_cast <event::ecn_t> (value & 0x03);
}
/**
 * @brief Функция получения расширенного вызова отправки сообщения с управляющими данными
 *
 * @details Вызов этот у MS Windows не объявлен наперёд: адрес его берётся у самого
 *          сокета управляющим запросом - тем же порядком, каким берутся `AcceptEx`,
 *          `ConnectEx` и `WSARecvMsg`. Берётся он однажды и запоминается: адрес общий
 *          для всех сокетов библиотеки, а спрашивать его на каждую датаграмму значило
 *          бы платить обращением к ядру за каждую отправку
 *
 * @param sock сокет, у которого спрашивается вызов
 * @return     адрес расширенного вызова, либо пустое значение
 *
 */
static LPFN_WSASENDMSG __awh_wsa_sendmsg__(const SOCKET sock) noexcept {
	// Запомненный адрес расширенного вызова
	static LPFN_WSASENDMSG result = nullptr;
	// Если адрес уже взят
	if(result != nullptr)
		// Выводим запомненный адрес
		return result;
	// Опознаватель расширенного вызова отправки сообщения
	GUID guid = WSAID_WSASENDMSG;
	// Размер отданного адреса
	DWORD size = 0;
	// Если взять адрес расширенного вызова не удалось
	if(::WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &result, sizeof(result), &size, nullptr, nullptr) != 0)
		// Сбрасываем адрес расширенного вызова
		result = nullptr;
	// Выводим адрес расширенного вызова
	return result;
}
/**
 * @brief Метод отправки датаграммы с меткой перегрузки в управляющих данных
 *
 * @details Метка едет вместе со своей датаграммой, а не ставится настройкой сокета: у
 *          датаграммных серверов сокет один на все соединения, и датаграмма, полежавшая
 *          в очереди отправки, ушла бы под меткой, выставленной уже другим соединением
 *
 * @note У MS Windows метка кладётся настройкой `IP_ECN` / `IPV6_ECN`, а не `IP_TOS` /
 *       `IPV6_TCLASS`, как то заведено у систем POSIX. Настройки эти несут ТОЛЬКО два
 *       разряда самой метки и класса обслуживания при себе не имеют - оттого в
 *       управляющие данные и кладётся `traffic & 0x03`, а не весь октет. Класс
 *       обслуживания задаётся здесь отдельным средством подсистемы качества
 *       обслуживания (`win::qos`), настройкой сокета он не выставляется вовсе
 *
 * @note Отправка эта СИНХРОННА: расширенный вызов зовётся без наложения (`OVERLAPPED`
 *       не подаётся), и к его возврату система с управляющими данными уже разобралась.
 *       Оттого держать их на стеке здесь можно - требование пережить завершение
 *       операции относится лишь к подаче с перекрытием
 *
 * @param sock    сетевой сокет
 * @param buffer  буфер отправляемых данных
 * @param size    размер буфера отправляемых данных
 * @param flags   признаки отправки
 * @param addr    адрес удалённого узла
 * @param length  размер адреса удалённого узла
 * @param family  семейство протоколов (IPv4 или IPv6)
 * @param traffic значение поля класса обслуживания вместе с меткой перегрузки
 * @return        количество отправленных октетов либо -1 при отказе
 *
 */
ssize_t awh::eth::Socket::datagram(const net::socket_t sock, const void * buffer, const size_t size, const int32_t flags, const struct sockaddr * addr, const socklen_t length, const event::family_t family, const uint8_t traffic) const noexcept {
	/**
	 * Если метить датаграмму не требуется - отправляем обычным обращением
	 *
	 * @note Нулевое значение означает и нулевой класс обслуживания, и отсутствие метки
	 *       перегрузки - строить ради него управляющие данные незачем
	 */
	if(traffic == 0)
		// Выполняем отправку датаграммы обычным обращением
		return static_cast <ssize_t> (::sendto(sock, reinterpret_cast <const char *> (buffer), static_cast <int32_t> (size), flags, addr, static_cast <int32_t> (length)));
	/**
	 * Если метка означает состоявшуюся перегрузку - отправляем датаграмму без неё
	 *
	 * @note Метку эту MS Windows отправителю ставить не даёт: управляющие данные с нею
	 *       отвергаются отказом 10022 (WSAEINVAL) - проверено щупом на обоих семействах
	 *       адресов. Право её ставить система оставляет за узлами пути, каким перегрузка
	 *       и встретилась, а отправителю оставляет лишь заявить о своей готовности её
	 *       принять. Отказывать в отправке из-за этого было бы неверно: датаграмма
	 *       пропала бы вовсе, а расхождение прячется в свойствах системы, не в данных.
	 *       Тем же порядком обходится у BSD и отсутствие метки IPv4 у NetBSD
	 */
	if((traffic & 0x03) == 0x03)
		// Выполняем отправку датаграммы обычным обращением
		return static_cast <ssize_t> (::sendto(sock, reinterpret_cast <const char *> (buffer), static_cast <int32_t> (size), flags, addr, static_cast <int32_t> (length)));
	// Уровень управляющих данных и их название
	int32_t level = IPPROTO_IP, option = IP_ECN;
	/**
	 * Определяем семейство протоколов события
	 */
	switch(static_cast <uint8_t> (family)){
		// Для семейства IPv6
		case static_cast <uint8_t> (event::family_t::IPV6): {
			// Устанавливаем уровень управляющих данных
			level = IPPROTO_IPV6;
			// Устанавливаем название управляющих данных
			option = IPV6_ECN;
		} break;
		// Для семейства IPv4 остаются взятые по умолчанию значения
		case static_cast <uint8_t> (event::family_t::IPV4):
		break;
		// Для остальных семейств метить датаграмму нечем
		default:
			// Выполняем отправку датаграммы обычным обращением
			return static_cast <ssize_t> (::sendto(sock, reinterpret_cast <const char *> (buffer), static_cast <int32_t> (size), flags, addr, static_cast <int32_t> (length)));
	}
	// Получаем расширенный вызов отправки сообщения
	LPFN_WSASENDMSG send = ::__awh_wsa_sendmsg__(sock);
	// Если расширенный вызов получить не удалось
	if(send == nullptr)
		// Выполняем отправку датаграммы обычным обращением
		return static_cast <ssize_t> (::sendto(sock, reinterpret_cast <const char *> (buffer), static_cast <int32_t> (size), flags, addr, static_cast <int32_t> (length)));
	// Буфер отправляемых данных
	WSABUF data{};
	// Устанавливаем размер буфера отправляемых данных
	data.len = static_cast <ULONG> (size);
	// Устанавливаем буфер отправляемых данных
	data.buf = const_cast <CHAR *> (reinterpret_cast <const CHAR *> (buffer));
	// Буфер управляющих данных отправки
	uint8_t control[WSA_CMSG_SPACE(sizeof(INT))]{0};
	// Устройство отправляемого сообщения
	WSAMSG message{};
	// Устанавливаем адрес удалённого узла
	message.name = const_cast <LPSOCKADDR> (addr);
	// Устанавливаем размер адреса удалённого узла
	message.namelen = static_cast <INT> (length);
	// Устанавливаем буфер отправляемых данных
	message.lpBuffers = &data;
	// Устанавливаем количество буферов отправляемых данных
	message.dwBufferCount = 1;
	// Устанавливаем буфер управляющих данных
	message.Control.buf = reinterpret_cast <CHAR *> (control);
	// Устанавливаем размер буфера управляющих данных
	message.Control.len = static_cast <ULONG> (sizeof(control));
	// Получаем заголовок управляющих данных
	LPWSACMSGHDR cmsg = WSA_CMSG_FIRSTHDR(&message);
	// Если заголовок управляющих данных получить не удалось
	if(cmsg == nullptr)
		// Выполняем отправку датаграммы обычным обращением
		return static_cast <ssize_t> (::sendto(sock, reinterpret_cast <const char *> (buffer), static_cast <int32_t> (size), flags, addr, static_cast <int32_t> (length)));
	/**
	 * Значение метки перегрузки
	 *
	 * @note Кладутся лишь два младших разряда: настройка эта класса обслуживания при
	 *       себе не несёт, и весь октет она приняла бы за метку целиком
	 */
	const INT value = static_cast <INT> (traffic & 0x03);
	// Устанавливаем уровень управляющих данных
	cmsg->cmsg_level = level;
	// Устанавливаем тип управляющих данных
	cmsg->cmsg_type = option;
	// Устанавливаем размер управляющих данных
	cmsg->cmsg_len = WSA_CMSG_LEN(sizeof(value));
	// Устанавливаем значение метки перегрузки
	::memcpy(WSA_CMSG_DATA(cmsg), &value, sizeof(value));
	// Устанавливаем размер занятого буфера управляющих данных
	message.Control.len = static_cast <ULONG> (WSA_CMSG_SPACE(sizeof(value)));
	// Количество отправленных октетов
	DWORD bytes = 0;
	// Если отправить датаграмму с меткой перегрузки не удалось
	if(send(sock, &message, static_cast <DWORD> (flags), &bytes, nullptr, nullptr) != 0)
		// Выводим признак отказа отправки
		return -1;
	// Выводим количество отправленных октетов
	return static_cast <ssize_t> (bytes);
}
/**
 * @brief Метод установки признака перегрузки в заголовке IP-пакета
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов
 * @param ecn    устанавливаемый признак перегрузки
 * @return       результат выполнения установки
 *
 * @note Читать прежнее значение перед записью, как то делают эталонные бэкенды, здесь
 *       не нужно: класс обслуживания настройкой этой не задевается вовсе
 *
 * @warning ДАТАГРАММЫ НАСТРОЙКА ЭТА НЕ МЕТИТ. Замерено щупом 25.08.2026 на MS Windows
 *          10 (ARM64 x86-64, MinGW): настройка принимается с успехом, а класс
 *          обслуживания у получателя приходит НУЛЁМ. Проверено тремя способами:
 *
 *          | способ отправки                              | метка у получателя |
 *          |----------------------------------------------|--------------------|
 *          | настройка сокета, `sendto`                    | не применена       |
 *          | настройка сокета после `connect`, `send`      | не применена       |
 *          | управляющее сообщение НА ДАТАГРАММУ           | применена          |
 *
 *          Движок из-за этого настройкой и не пользуется: для датаграммных узлов он
 *          держит метку на узле и кладёт её управляющими данными отправки, а сюда
 *          обращается лишь для потоковых сокетов, где управляющих данных нет вовсе.
 *
 *          Отказом настройка при этом НЕ отвечает, оттого несоответствие молчаливое:
 *          внешний потребитель, позвавший метод на датаграммном сокете, получит успех
 *          и непомеченные датаграммы. Возврат метода здесь не изменён намеренно -
 *          отказ развёл бы договор метода по системам, а решение это владельца
 *
 */
bool awh::eth::Socket::setExplicitCongestionNotification(const net::socket_t sock, const event::family_t family, const event::ecn_t ecn) const noexcept {
	// Если сокет не передан
	if(sock == net::invalid_socket_t)
		// Выводим отрицательный результат установки
		return false;
	// Устанавливаемое значение признака перегрузки
	const int32_t value = static_cast <int32_t> (ecn);
	// Уровень настройки и её название
	int32_t level = IPPROTO_IP, option = IP_ECN;
	// Если сокет работает по протоколу IPv6
	if(family == event::family_t::IPV6){
		// Устанавливаем уровень настройки
		level = IPPROTO_IPV6;
		// Устанавливаем название настройки
		option = IPV6_ECN;
	}
	// Если установить признак перегрузки не удалось
	if(::setsockopt(sock, level, option, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) != 0){
		// Выводим в журнал сообщение о невозможности установки признака перегрузки
		this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Выводим отрицательный результат установки
		return false;
	}
	// Выводим положительный результат установки
	return true;
}
/**
 * @brief Метод включения либо выключения выдачи сведений о принятых пакетах
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов
 * @param mode   режим включения либо выключения
 * @return       результат выполнения переключения
 *
 * @details Выдача эта прикладывает к каждому принятому пакету его сопутствующие
 *          сведения - число переходов, какое пакет ещё вправе сделать, устройство,
 *          каким он принят, и класс обслуживания, с каким он пришёл
 *
 * @note Настройки числа переходов и сведений об устройстве выставляются заодно, и
 *       успехом считается лишь общий их успех: выдача сведений наполовину для разбора
 *       пакета непригодна
 *
 * @warning Класс обслуживания выставляется ОТДЕЛЬНО и обязательным не считается:
 *          настройка `IP_RECVTOS` появилась у MS Windows лишь в поздних сборках, и
 *          отказ её не должен рушить выдачу остальных сведений. Довод взят у
 *          эталонного бэкенда, где та же настройка отсутствует у NetBSD и OpenBSD и
 *          обходится предупреждением
 *
 *          Прежде настройка эта не выставлялась ВОВСЕ, и следствие было молчаливым:
 *          управляющее сообщение с классом обслуживания не приходило никогда, разбор
 *          его в движке оставался мёртвым, а потребитель, запросивший `DGRAM_INFO`,
 *          получал метку перегрузки всегда пустой - при исправно идущих данных.
 *          Найдено 25.08.2026 прогоном набора `eth` под MS Windows: проверка
 *          `SocketEcnDeliveryTest` отступала по доводу, который к тому дню устарел
 *
 */
bool awh::eth::Socket::trafficInfoGeneration(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode) const noexcept {
	// Если сокет не передан
	if(sock == net::invalid_socket_t)
		// Выводим отрицательный результат переключения
		return false;
	// Устанавливаемое значение настроек
	const int32_t value = (mode == net::socket_mode_t::ENABLED ? 1 : 0);
	// Уровень настроек и их названия
	int32_t level = IPPROTO_IP, hops = IP_RECVTTL, info = IP_PKTINFO, service = IP_RECVTOS;
	// Если сокет работает по протоколу IPv6
	if(family == event::family_t::IPV6){
		// Устанавливаем уровень настроек
		level = IPPROTO_IPV6;
		// Устанавливаем название настройки выдачи числа переходов
		hops = IPV6_HOPLIMIT;
		// Устанавливаем название настройки выдачи сведений об устройстве
		info = IPV6_PKTINFO;
		// Устанавливаем название настройки выдачи класса обслуживания
		service = IPV6_RECVTCLASS;
	}
	// Результат выставления настройки выдачи числа переходов
	const bool first = (::setsockopt(sock, level, hops, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) == 0);
	// Если выставить настройку выдачи числа переходов не удалось
	if(!first)
		// Выводим в журнал сообщение о невозможности выставления настройки
		this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
	// Результат выставления настройки выдачи сведений об устройстве
	const bool second = (::setsockopt(sock, level, info, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) == 0);
	// Если выставить настройку выдачи сведений об устройстве не удалось
	if(!second)
		// Выводим в журнал сообщение о невозможности выставления настройки
		this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
	/**
	 * Выставляем настройку выдачи класса обслуживания
	 *
	 * @note Итог её в общий результат НЕ входит: без неё пропадает лишь метка
	 *       перегрузки, тогда как число переходов и устройство приёма приходят исправно
	 */
	if(::setsockopt(sock, level, service, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) != 0)
		// Выводим в журнал сообщение о невозможности выставления настройки
		this->_log->print("%s: traffic class of received packets is not reported: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
	// Выводим общий результат переключения
	return (first && second);
}
/**
 * @brief Метод переключения настройки сокета
 *
 * @param sock   сетевой сокет
 * @param family семейство протоколов
 * @param mode   режим включения либо выключения
 * @param option переключаемая настройка
 * @param proto  протокол сокета, NONE - протокол не назван
 * @return       результат выполнения переключения
 *
 * @details Часть настроек у MS Windows зовётся иначе, чем у систем POSIX, часть
 *          достигается вовсе не настройкой сокета, а часть недостижима
 *
 *          | Настройка | Чем достигается у MS Windows |
 *          |---|---|
 *          | HDRINCL | IP_HDRINCL, у IPv6 соответствия нет |
 *          | TCP_NO_DELAY | TCP_NODELAY |
 *          | IPV6_ONLY | IPV6_V6ONLY |
 *          | BROADCAST | SO_BROADCAST |
 *          | REUSE_ADDR | SO_REUSEADDR |
 *          | MULTICAST_LOOPBACK | IP_MULTICAST_LOOP либо IPV6_MULTICAST_LOOP |
 *          | HARD_CLOSE | SO_LINGER с нулевой выдержкой |
 *          | NO_IO_BLOCK, SM_IO_BLOCK | ioctlsocket с FIONBIO |
 *          | CLOSE_ON_EXEC | SetHandleInformation без наследования |
 *          | NO_SIGILL, NO_SIGPIPE | достигнуто само собой |
 *          | TCP_CORKING | недостижимо |
 *          | REUSE_PORT | недостижимо |
 *
 * @note Заглушение сигналов настройки не требует вовсе: сокеты MS Windows сигналов не
 *       поднимают, и запрошенное состояние здесь уже стоит. Отвечать отказом на то,
 *       чего добивались и что уже есть, было бы неверно, оттого настройки эти
 *       отвечают согласием, не трогая сокета
 *
 */
bool awh::eth::Socket::switchOption(const net::socket_t sock, const event::family_t family, const net::socket_mode_t mode, const uint16_t option, [[maybe_unused]] const event::protocol_t proto) const noexcept {
	// Если сокет не передан
	if(sock == net::invalid_socket_t)
		// Выводим отрицательный результат переключения
		return false;
	/**
	 * Если настройка запрошена у именованного канала
	 *
	 * @details Описатель канала сокетом не является, и сокетные обращения отвечают ему
	 *          отказом 10038 (WSAENOTSOCK). Настройки, какие каналу вообще осмысленны,
	 *          задаются при заведении описателя и позже не меняются: наложенный обмен
	 *          ставится признаком FILE_FLAG_OVERLAPPED, а наследование - оснасткой
	 *          прав описателя. Отвечать отказом на достигнутое было бы неверно, а
	 *          выполнять сокетное обращение - вредно
	 *
	 * @note Изъян обнаружен щупом живого запуска кластера: пара каналов заводилась,
	 *       а настройка узла валилась отказом 10038 на неблокирующем обмене
	 *
	 */
	if(family == event::family_t::PIPE){
		/**
		 * Определяем переключаемую настройку
		 */
		switch(option){
			// Если переключается неблокирующий обмен
			case event::options::NO_IO_BLOCK:
			// Если переключается блокирующий обмен
			case event::options::SM_IO_BLOCK:
			// Если переключается закрытие описателя при замещении образа
			case event::options::CLOSE_ON_EXEC:
			// Если переключается заглушение сигнала неверного действия
			case event::options::NO_SIGILL:
			// Если переключается заглушение сигнала обрыва канала
			case event::options::NO_SIGPIPE:
				// Выводим положительный результат: состояние это стоит с заведения
				return true;
			/**
			 * Для остальных настроек
			 *
			 * @details Настройка, каналу не свойственная, обходится молча и с согласием:
			 *          набор настроек задаёт потребитель одним и тем же вызовом для
			 *          всякого своего события, и канал обязан принимать его целиком.
			 *          Отвечать отказом значило бы валить заведение узла из-за настройки,
			 *          которая каналу и не нужна, а писать о том в журнал - шуметь на
			 *          каждом узле обо всяком расхождении устройств
			 *
			 * @note Умолчание это касается лишь канала: сокету всякая неподдерживаемая
			 *       настройка по-прежнему отвечает отказом с записью в журнал
			 */
			default:
				// Выводим положительный результат: настройка каналу не свойственна
				return true;
		}
	}
	// Устанавливаемое значение настройки
	const int32_t value = (mode == net::socket_mode_t::ENABLED ? 1 : 0);
	// Уровень настройки и её название
	int32_t level = 0, name = 0;
	/**
	 * Определяем переключаемую настройку
	 */
	switch(option){
		// Если переключается настройка самостоятельной сборки заголовков пакета
		case event::options::HDRINCL: {
			/**
			 * Если сокет работает по протоколу IPv6
			 *
			 * @details Своей головы у IPv6 не даёт ни одна система: опции IP_HDRINCL
			 *          для него нет ни у BSD, ни у Linux, ни у систем Sun. Договор
			 *          движков оттого един - отвечать УСПЕХОМ на пустое действие, и
			 *          движки POSIX так и делают
			 *
			 * @warning Прежде здесь стоял отказ, и он расходился с прочими движками:
			 *          отсутствие опции у ВСЕХ систем выдавалось за особенность MS
			 *          Windows. Проверка SocketSwitchOptionIPv6Test падала оттого на
			 *          всякой машине Windows, закрепляя общий договор
			 */
			if(family == event::family_t::IPV6)
				// Выводим положительный результат: делать нечего, и это не отказ
				return true;
			// Устанавливаем уровень настройки
			level = IPPROTO_IP;
			// Устанавливаем название настройки
			name = IP_HDRINCL;
		} break;
		// Если переключается настройка немедленной отправки
		case event::options::TCP_NO_DELAY: {
			// Устанавливаем уровень настройки
			level = IPPROTO_TCP;
			// Устанавливаем название настройки
			name = TCP_NODELAY;
		} break;
		// Если переключается настройка работы одним лишь IPv6
		case event::options::IPV6_ONLY: {
			// Устанавливаем уровень настройки
			level = IPPROTO_IPV6;
			// Устанавливаем название настройки
			name = IPV6_V6ONLY;
		} break;
		// Если переключается настройка широковещательной рассылки
		case event::options::BROADCAST: {
			// Устанавливаем уровень настройки
			level = SOL_SOCKET;
			// Устанавливаем название настройки
			name = SO_BROADCAST;
		} break;
		/**
		 * Если переключается настройка повторного занятия адреса
		 *
		 * @warning UNIX-доменному сокету настройка эта НЕ ставится, и заслон здесь
		 *          обязателен. Своенравность MS Windows тут особого рода: обращение
		 *          отвечает УСПЕХОМ, а сокет после него испорчен - привязка отвечает
		 *          отказом 10045 (`WSAEOPNOTSUPP`), и повод её из отказа не читается
		 *          никак. Установлено щупом на стенде: чистый сокет привязывается, а
		 *          тот же сокет после `SO_REUSEADDR` - нет; то же делает и
		 *          `SO_EXCLUSIVEADDRUSE`
		 *
		 * @note Потери в этом нет: повторное занятие адреса нужно там, где адрес
		 *       держится ядром после закрытия (состояние TIME_WAIT у TCP), а
		 *       UNIX-доменный сокет держится файлом, и занятость его снимается
		 *       удалением этого файла
		 *
		 */
		case event::options::REUSE_ADDR: {
			// Если сокет является UNIX-доменным
			if(family == event::family_t::UDS){
				// Выводим в журнал сообщение об оставленной без внимания настройке
				this->_log->print("%s: address reuse is not applicable to UNIX-domain sockets on MS Windows", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__);
				/**
				 * Выводим положительный результат переключения
				 *
				 * @note Отказ здесь был бы неверен: настройка не нужна, а не
				 *       недоступна, - и обрывать из-за неё заведение узла незачем
				 */
				return true;
			}
			// Устанавливаем уровень настройки
			level = SOL_SOCKET;
			// Устанавливаем название настройки
			name = SO_REUSEADDR;
		} break;
		// Если переключается настройка возврата рассылки самому себе
		case event::options::MULTICAST_LOOPBACK: {
			// Устанавливаем уровень настройки
			level = (family == event::family_t::IPV6 ? IPPROTO_IPV6 : IPPROTO_IP);
			// Устанавливаем название настройки
			name = (family == event::family_t::IPV6 ? IPV6_MULTICAST_LOOP : IP_MULTICAST_LOOP);
		} break;
		/**
		 * Если переключается настройка обрыва связи без доводки
		 *
		 * @note Достигается она выдержкой закрытия нулевой длины: сокет закрывается
		 *       немедленно, недоставленное отбрасывается, а тому концу уходит сброс
		 *       связи взамен обыкновенного её завершения
		 *
		 */
		case event::options::HARD_CLOSE: {
			// Настройки выдержки закрытия сокета
			struct linger value{};
			// Устанавливаем признак применения выдержки
			value.l_onoff = static_cast <u_short> (mode == net::socket_mode_t::ENABLED ? 1 : 0);
			// Устанавливаем длину выдержки
			value.l_linger = 0;
			// Если выставить выдержку закрытия не удалось
			if(::setsockopt(sock, SOL_SOCKET, SO_LINGER, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) != 0){
				// Выводим в журнал сообщение о невозможности выставления настройки
				this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
				// Выводим отрицательный результат переключения
				return false;
			}
			// Выводим положительный результат переключения
			return true;
		}
		// Если переключается настройка работы сокета без задержек
		case event::options::NO_IO_BLOCK:
		// Если переключается настройка работы сокета с задержками
		case event::options::SM_IO_BLOCK: {
			/**
			 * Настройка эта у MS Windows задаётся не сокету, а его управлению
			 *
			 * @note Значение здесь обратное по смыслу: единица означает работу без
			 *       задержек, оттого настройка задержек и настройка их отсутствия
			 *       разводятся сами собой
			 *
			 */
			u_long value = static_cast <u_long> (option == event::options::NO_IO_BLOCK ? (mode == net::socket_mode_t::ENABLED ? 1 : 0) : (mode == net::socket_mode_t::ENABLED ? 0 : 1));
			// Если выставить настройку не удалось
			if(::ioctlsocket(sock, FIONBIO, &value) != 0){
				// Выводим в журнал сообщение о невозможности выставления настройки
				this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
				// Выводим отрицательный результат переключения
				return false;
			}
			// Выводим положительный результат переключения
			return true;
		}
		/**
		 * Если переключается настройка закрытия сокета при запуске другого приложения
		 *
		 * @note Наследования у MS Windows нет вовсе - взамен него дочернему процессу
		 *       передаётся перечень описателей, помеченных наследуемыми. Снятие
		 *       пометки и есть то самое, чего добиваются закрытием при запуске
		 *
		 */
		case event::options::CLOSE_ON_EXEC: {
			// Если снять либо поставить пометку наследования не удалось
			if(!::SetHandleInformation(reinterpret_cast <HANDLE> (sock), HANDLE_FLAG_INHERIT, (mode == net::socket_mode_t::ENABLED ? 0 : HANDLE_FLAG_INHERIT))){
				// Выводим в журнал сообщение о невозможности выставления настройки
				this->_log->print("%s: handle inheritance could not be changed, error %lu", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::GetLastError());
				// Выводим отрицательный результат переключения
				return false;
			}
			// Выводим положительный результат переключения
			return true;
		}
		// Если переключается заглушение сигнала неверного действия
		case event::options::NO_SIGILL:
		// Если переключается заглушение сигнала обрыва канала
		case event::options::NO_SIGPIPE:
			/**
			 * Сигналов этих сокеты MS Windows не поднимают вовсе
			 *
			 * @note Запрошенное состояние здесь стоит само собой, и отвечать отказом
			 *       на достигнутое было бы неверно
			 *
			 */
			return true;
		// Если переключается настройка придержки отправки
		case event::options::TCP_CORKING: {
			/**
			 * Придержка отправки изображается алгоритмом Нейгла
			 *
			 * @details Ни TCP_CORK, ни TCP_NOPUSH у MS Windows нет, и ближайшее, что
			 *          даёт там сам TCP, - алгоритм Нейгла: назначение у него то же,
			 *          копить мелкие отправки до заполнения сегмента. Придержка
			 *          включается снятием TCP_NODELAY, снимается - его возвратом
			 *
			 * @details Ровно так же поступает движок kqueue у NetBSD, где нет ни того,
			 *          ни другого: наружу выставлено одно имя опции, а движок
			 *          подставляет то, что даёт система
			 *
			 * @warning Равенством это не является: придержка держит до явного снятия,
			 *          Нейгл - лишь до подтверждения предыдущего сегмента. Обмен,
			 *          которому придержка нужна как строгая, получит здесь
			 *          приближение, а не её саму
			 *
			 * @warning Прежде здесь стоял отказ, и он расходился с движками POSIX:
			 *          проверка SocketSwitchOptionTcpTest падала на всякой машине
			 *          Windows, а изобразить придержку было чем
			 */
			// Придержке отправки соответствует включённый алгоритм Нейгла, снятию - выключенный
			const int32_t nodelay = (mode == net::socket_mode_t::ENABLED ? 0 : 1);
			// Включаем либо отключаем алгоритм Нейгла вместо придержки отправки
			if(::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast <const char *> (&nodelay), static_cast <int32_t> (sizeof(nodelay))) != 0){
				// Выводим в журнал сообщение об ошибке
				this->_log->print("%s: TCP output corking could not be emulated by the Nagle algorithm, %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
				// Выводим отрицательный результат переключения
				return false;
			}
			// Выводим положительный результат переключения
			return true;
		}
		/**
		 * Если переключается настройка повторного занятия порта
		 *
		 * @details Соответствия ей у MS Windows нет: SO_REUSEADDR здесь позволяет занять
		 *          уже занятый порт и без неё, отчего подменять одно другим означало бы
		 *          выдать иное поведение за запрошенное - разведения нагрузки по сокетам,
		 *          какое даёт SO_REUSEPORT у Linux и FreeBSD, эта система не даёт ничем
		 *
		 * @note Настройка неприложима к системе ПО УСТРОЙСТВУ, а не по недостатку
		 *       поддержки, и оттого отвечает успехом, не обращаясь к ядру: тем же
		 *       порядком отвечают эталонные слои на настройку повторного занятия порта
		 *       для UNIX-доменного сокета, где порта нет вовсе. Отвечай она отказом -
		 *       отказ уносил бы с собой ВСЁ обращение выставления настроек, и код,
		 *       работающий на прочих системах, не выставил бы под MS Windows ни одной
		 *       настройки из запрошенных. Установлено прогоном: на этом падали все
		 *       проверки дейтаграмм разом
		 *
		 * @warning Разведение нагрузки, которого настройка не даёт, добирается уровнем
		 *          выше: кластер под MS Windows держит один общий слушающий сокет, а
		 *          воркеры расхватывают подключения наперегонки. Опираться на успех
		 *          этого обращения как на признак разведения нельзя ни на одной системе
		 *
		 */
		case event::options::REUSE_PORT: {
			// Выводим в журнал сообщение об отсутствии соответствия
			this->_log->print("%s: port reuse has no counterpart on MS Windows", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__);
			// Выводим успешный результат, не обращаясь к ядру
			return true;
		}
		// Если настройка модулю неизвестна
		default: {
			// Выводим в журнал сообщение о неизвестной настройке
			this->_log->print("%s: socket option %u is not supported", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, static_cast <uint32_t> (option));
			// Выводим отрицательный результат переключения
			return false;
		}
	}
	// Если выставить настройку сокета не удалось
	if(::setsockopt(sock, level, name, reinterpret_cast <const char *> (&value), static_cast <int32_t> (sizeof(value))) != 0){
		// Выводим в журнал сообщение о невозможности выставления настройки
		this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
		// Выводим отрицательный результат переключения
		return false;
	}
	// Выводим положительный результат переключения
	return true;
}
/**
 * @brief Метод входа в группу рассылки либо выхода из неё
 *
 * @param sock   сетевой сокет
 * @param mode   режим входа либо выхода
 * @param group  адрес группы рассылки
 * @param source адрес устройства, каким выполняется вход
 * @return       результат выполнения
 *
 * @note Устройство у IPv6 задаётся не адресом, а номером, и номер этот берётся из
 *       поля зоны переданного адреса. Пустая зона означает выбор устройства самой
 *       системой - тем поведение приводится к общему с эталонными бэкендами виду
 *
 */
bool awh::eth::Socket::membership(const net::socket_t sock, const net::socket_mode_t mode, const net::addr_net_t * group, const net::addr_net_t * source) const noexcept {
	// Если сокет либо адреса не переданы
	if((sock == net::invalid_socket_t) || (group == nullptr) || (source == nullptr)){
		// Выводим в журнал сообщение о непереданных адресах
		this->_log->print("%s: multicast group or source address is not initialized", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__);
		// Выводим отрицательный результат выполнения
		return false;
	}
	// Если виды переданных адресов не совпали
	if(group->size != source->size){
		// Выводим в журнал сообщение о несовпадении видов адресов
		this->_log->print("%s: multicast group and source addresses belong to different families", log_t::flag_t::CRITICAL, ::__AWH_SOCKET_BACKEND__);
		// Выводим отрицательный результат выполнения
		return false;
	}
	// Признак входа в группу рассылки
	const bool join = (mode == net::socket_mode_t::ENABLED);
	/**
	 * Определяем вид переданных адресов
	 */
	switch(group->size){
		// Если адреса являются адресами IPv4
		case 4: {
			// Запрос на вход в группу рассылки
			struct ip_mreq request{};
			// Устанавливаем адрес группы рассылки
			request.imr_multiaddr.s_addr = awh_cast <const net::addr_net_ipv4_t *> (group)->address;
			// Устанавливаем адрес устройства, каким выполняется вход
			request.imr_interface.s_addr = awh_cast <const net::addr_net_ipv4_t *> (source)->address;
			// Если выполнить вход в группу рассылки либо выход из неё не удалось
			if(::setsockopt(sock, IPPROTO_IP, (join ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP), reinterpret_cast <const char *> (&request), static_cast <int32_t> (sizeof(request))) != 0){
				// Выводим в журнал сообщение о невозможности выполнения
				this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
				// Выводим отрицательный результат выполнения
				return false;
			}
			// Выводим положительный результат выполнения
			return true;
		}
		// Если адреса являются адресами IPv6
		case 16: {
			// Запрос на вход в группу рассылки
			struct ipv6_mreq request{};
			// Устанавливаем адрес группы рассылки
			::memcpy(&request.ipv6mr_multiaddr, &awh_cast <const net::addr_net_ipv6_t *> (group)->address[0], sizeof(request.ipv6mr_multiaddr));
			// Устанавливаем номер устройства, каким выполняется вход
			request.ipv6mr_interface = static_cast <ULONG> (awh_cast <const net::addr_net_ipv6_t *> (source)->zone);
			// Если выполнить вход в группу рассылки либо выход из неё не удалось
			if(::setsockopt(sock, IPPROTO_IPV6, (join ? IPV6_ADD_MEMBERSHIP : IPV6_DROP_MEMBERSHIP), reinterpret_cast <const char *> (&request), static_cast <int32_t> (sizeof(request))) != 0){
				// Выводим в журнал сообщение о невозможности выполнения
				this->_log->print("%s: %s", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__, ::__awh_socket_error__().c_str());
				// Выводим отрицательный результат выполнения
				return false;
			}
			// Выводим положительный результат выполнения
			return true;
		}
	}
	// Выводим в журнал сообщение о неподдерживаемом виде адресов
	this->_log->print("%s: only IPv4 and IPv6 multicast groups are supported", log_t::flag_t::WARNING, ::__AWH_SOCKET_BACKEND__);
	// Выводим отрицательный результат выполнения
	return false;
}
