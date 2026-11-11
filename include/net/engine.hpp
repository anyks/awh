/**
 * @file: engine.hpp
 * @date: 2022-08-03
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

#ifndef __AWH_ENGINE__
#define __AWH_ENGINE__

/**
 * Настройка системы для высокой нагрузки:
 * url: https://romantelychko.com/blog/1300
 * количество занятых сокетов: lsof | grep 69080 | wc -l
 */

/**
 * Активация протокола SCTP:
 * 
 * Под Ubuntu Linux:
 * # sudo apt install libsctp-dev
 * # sudo modprobe sctp
 * # sudo sysctl -w net.sctp.auth_enable=1
 * 
 * Под FreeBSD:
 * # sudo kldload sctp
 */

/**
 * Стандартные модули
 */
#include <random>
#include <thread>
#include <cstdio>
#include <string>
#include <vector>
#include <memory>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <sys/types.h>

/**
 * Для операционной системы не являющейся MS Windows
 */
#if !_WIN32 && !_WIN64
	#include <sys/un.h>
#endif

/**
 * Наши модули
 */
#include "if.hpp"
#include "uri.hpp"
#include "socket.hpp"
#include "../sys/fs.hpp"
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс для работы с двигателем передачи данных
	 *
	 */
	typedef class AWH_SHARED_EXPORT Engine {
		public:
			/**
			 * Режим работы
			 */
			enum class mode_t : uint8_t {
				NONE     = 0x00, // Экшен не установлен
				ENABLED  = 0x01, // Экшен активации
				DISABLED = 0x02  // Экшен деактивации
			};
			/**
			 * Типы ошибок валидации
			 */
			enum class validate_t : uint8_t {
				NONE                 = 0x00, // Не установлено
				Error                = 0x01, // Ошибка валидации
				MatchFound           = 0x02, // Валидация пройдена
				NoSANPresent         = 0x03, // Сеть не распознана
				MatchNotFound        = 0x04, // Валидация не пройдена
				MalformedCertificate = 0x05  // Неверный сертификат
			};
			/**
			 * Основные поддерживаемые протоколы
			 */
			enum class proto_t : uint8_t {
				NONE    = 0x00, // Протокол не установлен
				RAW     = 0x01, // Протокол является бинарным
				SPDY1   = 0x02, // Протокол соответствует SPDY/1
				HTTP1   = 0x03, // Протокол соответствует HTTP/1
				HTTP1_1 = 0x04, // Протокол соответствует HTTP/1.1
				HTTP2   = 0x05, // Протокол соответствует HTTP/2
				HTTP3   = 0x06  // Протокол соответствует HTTP/3
			};
			/**
			 * Основные методы режимов работы
			 */
			enum class method_t : uint8_t {
				READ       = 0x01, // Метод чтения из сокета
				WRITE      = 0x02, // Метод записи в сокет
				ACCEPT     = 0x03, // Метод запроса на подключение
				CONNECT    = 0x04, // Метод подключения к серверу
				DISCONNECT = 0x00  // Метод отключения от сервера
			};
			/**
			 * Тип активного приложения
			 */
			enum class type_t : uint8_t {
				NONE   = 0x00, // Приложение не инициализированно
				CLIENT = 0x01, // Приложение является клиентом
				SERVER = 0x02  // Приложение является сервером
			};
		private:
			/**
			 * @brief Структура подключения
			 *
			 */
			typedef struct Peer {
				socklen_t size;                 // Размер объекта подключения
				struct sockaddr_storage client; // Параметры подключения клиента
				struct sockaddr_storage server; // Параметры подключения сервера
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Peer() noexcept :
				 size(0), client{}, server{} {}
			} peer_t;
		public:
			__AWH_PACK_BEGIN__
			/**
			 * @brief Структура с параметрами для постоянного подключения
			 *
			 */
			typedef struct KeepAlive {
				int32_t cnt;   // Максимальное количество попыток
				int32_t idle;  // Интервал времени в секундах через которое происходит проверка подключения
				int32_t intvl; // Интервал времени в секундах между попытками
				/**
				 * @brief Конструктор
				 *
				 */
				explicit KeepAlive() noexcept :
				 cnt(3), idle(1), intvl(2) {}
			} __AWH_PACKED__ alive_t;
			__AWH_PACK_END__
			/**
			 * @brief Класс сетевого пространства
			 *
			 */
			typedef class AWH_SHARED_EXPORT Address {
				public:
					/**
					 * Статус подключения
					 */
					enum class status_t : uint8_t {
						ACCEPTED     = 0x01, // Статус подключения (разрешено)
						ATTACHED     = 0x03, // Статус подключения (прикреплено)
						CONNECTED    = 0x02, // Статус подключения (подключено)
						DISCONNECTED = 0x00  // Статус подключения (отключено)
					};
				private:
					/**
					 * @brief Устанавливаем дружбу с классом двигателя
					 *
					 */
					friend class Engine;
				public:
					// Сетевой сокет
					SOCKET sock;
				private:
					// Тип сокета (SOCK_STREAM | SOCK_DGRAM)
					int32_t _type;
					// Протокол сокета (IPPROTO_TCP | IPPROTO_UDP | IPPROTO_SCTP)
					int32_t _protocol;
				private:
					// Флаг асинхронного режима работы сокета
					bool _async;
					// Флаг инициализации шифрования
					bool _encrypted;
				public:
					// Статус подключения
					status_t status;
				public:
					// Порт клиента
					uint32_t port;
					// Адрес интернет подключения и аппаратный
					string ip, mac;
				public:
					// Параметры постоянного подключения
					alive_t alive;
				private:
					// Объект работы с файловой системой
					fs_t _fs;
					// Объект подключения
					peer_t _peer;
					// Объект для работы с сетевым интерфейсом
					ifnet_t _ifnet;
					// Объект для работы с сокетами
					socket_t _socket;
				private:
					// Выполняем инициализацию генератора
					std::random_device _randev;
				public:
					// Список сетевых интерфейсов
					vector <string> network;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				private:
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Объект данных запроса
						WSADATA _wsaData;
						// Флаг инициализации WinSocksAPI
						bool _winSockInit;
					#endif
				private:
					/**
					 * @brief Метод закрытия подключения
					 *
					 */
					bool close() noexcept;
					/**
					 * @brief Метод извлечения данных клиента
					 *
					 */
					void client() noexcept;
				public:
					/**
					 * @brief Метод активации прослушивания сокета
					 *
					 * @return результат выполнения операции
					 */
					bool list() noexcept;
					/**
					 * @brief Метод очистки параметров подключения
					 *
					 * @return результат выполнения операции
					 */
					bool clear() noexcept;
				public:
					/**
					 * @brief Метод выполнения подключения
					 *
					 * @return результат выполнения операции
					 */
					bool connect() noexcept;
				private:
					/**
					 * @brief Метод извлечения хоста компьютера
					 *
					 * @param family семейство сокета (AF_INET / AF_INET6)
					 * @return       хост компьютера с которого производится запрос
					 */
					string host(const int32_t family) const noexcept;
				public:
					/**
					 * @brief Метод прикрепления клиента к серверу
					 *
					 * @param addr объект подключения сервера
					 */
					bool attach(Address & addr) noexcept;
				public:
					/**
					 * @brief Метод согласования подключения
					 *
					 * @param addr объект подключения сервера
					 * @return     результат выполнения операции
					 */
					bool accept(Address & addr) noexcept;
					/**
					 * @brief Метод согласования подключения
					 *
					 * @param sock   сетевой сокет сервера
					 * @param family семейство сокета (AF_INET / AF_INET6 / AF_UNIX)
					 * @return       результат выполнения операции
					 */
					bool accept(const SOCKET sock, const int32_t family) noexcept;
				public:
					/**
					 * @brief Метод установки параметров сокета
					 *
					 * @param type     тип сокета (SOCK_STREAM / SOCK_DGRAM)
					 * @param protocol протокол сокета (IPPROTO_TCP / IPPROTO_UDP / IPPROTO_SCTP)
					 */
					void sonet(const int32_t type = SOCK_STREAM, const int32_t protocol = IPPROTO_TCP) noexcept;
				public:
					/**
					 * @brief Метод инициализации адресного пространства сокета
					 *
					 * @param unixsocket адрес unxi-сокета в файловой системе
					 * @param type       тип приложения (клиент или сервер)
					 */
					void init(const string & unixsocket, const type_t type) noexcept;
					/**
					 * @brief Метод инициализации адресного пространства сокета
					 *
					 * @param ip     адрес для которого нужно создать сокет
					 * @param port   порт сервера для которого нужно создать сокет
					 * @param family семейство сокета (AF_INET / AF_INET6 / AF_UNIX)
					 * @param type   тип приложения (клиент или сервер)
					 * @param onlyV6 флаг разрешающий использовать только IPv6 подключение
					 * @return       параметры подключения к серверу
					 */
					void init(const string & ip, const uint32_t port, const int32_t family, const type_t type, const bool onlyV6 = false) noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					explicit Address(const fmk_t * fmk, const log_t * log) noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~Address() noexcept;
			} addr_t;
		private:
			/**
			 * @brief Структура адресов файлов сертификатов
			 *
			 */
			typedef struct Cert {
				// Сертификат центра сертификации (CA-файл)
				string ca;
				// Сертификат отозванных сертификатов (CRL-файл)
				string crl;
				// Приватный ключ сертификата
				string key;
				// Основной сертификат или цепочка сертификатов
				string pem;
				// Каталог с сертификатами центра сертификации (CA-файлами)
				string path;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Cert() noexcept :
				 ca{SSL_CA_FILE}, crl{""},
				 key{""}, pem{""}, path{""} {}
			} cert_t;
		public:
			/**
			 * @brief Класс контекста двигателя
			 *
			 */
			typedef class AWH_SHARED_EXPORT Context {
				private:
					/**
					 * @brief Класс внутреннего состояния связи
					 *
					 */
					class Tls;
				private:
					/**
					 * @brief Устанавливаем дружбу с родительским объектом двигателя
					 *
					 */
					friend class Engine;
				private:
					// Флаг вывода информации о связи
					bool _verb;
					// Флаг инициализации шифрования
					bool _encrypted;
				private:
					// Тип активного приложения
					type_t _type;
				private:
					// Протокол подключения
					proto_t _proto;
				private:
					// Состояние связи внешнего модуля шифрования (реализация — в src/net/engine.cpp)
					std::unique_ptr <Tls> _tls;
					// Первая дейтаграмма, принятая слушающим сокетом: её подаёт связи насос после заведения
					vector <uint8_t> _greeting;
				public:
					// Список поддерживаемых протоколов
					vector <uint8_t> protocols;
				private:
					// Объект подключения
					addr_t * _addr;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				private:
					/**
					 * @brief Метод извлечения прочитанных связью байтов в буфер чтения
					 *
					 */
					void drain() noexcept;
				private:
					/**
					 * @brief Метод отправки накопленных связи байтов в сокет
					 *
					 * @return результат работы функции (false - сокет не принял всё, повторить позже)
					 *
					 */
					bool flush() noexcept;
					/**
					 * @brief Метод проверки временной ошибки отправки датаграммы
					 *
					 * @return результат проверки (true - отправку нужно повторить позже)
					 */
					bool retry() const noexcept;
				private:
					/**
					 * @brief Метод проверки наличия байтов, которые связь должна отправить
					 *
					 * @return результат проверки (true - в накопителе связи лежат неотправленные байты)
					 *
					 */
					bool pending() const noexcept;
					/**
					 * @brief Метод проверки пройденности рукопожатия обеими сторонами
					 *
					 * @return результат проверки
					 *
					 */
					bool established() const noexcept;
				private:
					/**
					 * @brief Метод приёма байтов из сокета и подачи их связи
					 *
					 * @return количество принятых байт, 0 - собеседник закрылся, -1 - данных нет, -2 - связь отвергла байты
					 *
					 */
					int64_t feed() noexcept;
					/**
					 * @brief Метод перекачки байтов связи одним шагом
					 *
					 * @param want флаг, что прикладные данные нужны сейчас
					 * @return     результат работы функции (1 - рукопожатие завершено, -1 - ждёт данных, -2 - ждёт записи, 0 - не удалось)
					 *
					 */
					int32_t pump(const bool want) noexcept;
				private:
					/**
					 * @brief Метод вывода информации об ошибке
					 *
					 * @param status статус ошибки
					 * @return       результат работы фукнции
					 */
					int32_t error(const int32_t status) const noexcept;
				public:
					/**
					 * @brief Метод очистки контекста
					 *
					 */
					void clear() noexcept;
					/**
					 * @brief Метод вывода информации о сертификате
					 *
					 */
					void info() const noexcept;
				public:
					/**
					 * @brief Метод выполнения шага рукопожатия без передачи данных
					 *
					 * @return результат работы функции (1 - рукопожатие завершено, -1 - ждёт данных, -2 - ждёт записи, 0 - не удалось)
					 */
					int32_t negotiate() noexcept;
				public:
					/**
					 * @brief Метод чтения данных из сокета
					 *
					 * @param buffer буфер данных для чтения
					 * @param size   размер буфера данных
					 * @return       количество считанных байт
					 */
					int64_t read(char * buffer, const size_t size) noexcept;
					/**
					 * @brief Метод записи данных в сокет
					 *
					 * @param buffer буфер данных для записи
					 * @param size   размер буфера данных
					 * @return       количество записанных байт
					 */
					int64_t write(const char * buffer, const size_t size) noexcept;
				public:
					/**
					 * @brief Метод перепосылки сообщений рукопожатия DTLS по истечении таймера
					 *
					 * @return результат работы функции (false - рукопожатие не удалось)
					 */
					bool retransmit() noexcept;
					/**
					 * @brief Метод получения времени до перепосылки рукопожатия DTLS
					 *
					 * @return время в миллисекундах до срабатывания таймера DTLS (0 - таймер не запущен)
					 */
					uint32_t retransmission() const noexcept;
				public:
					/**
					 * @brief Метод проверки на то, является ли сокет заблокированным
					 *
					 * @return результат работы функции
					 */
					bool blocking() noexcept;
					/**
					 * @brief Метод активации/деактивации блокирующего сокета
					 *
					 * @param mode режим применимой операции
					 * @return     результат работы функции
					 */
					bool blocking(const mode_t mode) noexcept;
				public:
					/**
					 * @brief Метод отключения/включения алгоритма TCP/CORK
					 *
					 * @param mode режим применимой операции
					 * @return     результат выполенния операции
					 */
					bool cork(const mode_t mode) noexcept;
					/**
					 * @brief Метод отключения/включения алгоритма Нейгла
					 *
					 * @param mode режим применимой операции
					 * @return     результат выполенния операции
					 */
					bool nodelay(const mode_t mode) noexcept;
				public:
					/**
					 * @brief Метод извлечения поддерживаемого протокола
					 *
					 * @return поддерживаемый протокол подключения
					 */
					proto_t proto() const noexcept;
					/**
					 * @brief Метод установки поддерживаемого протокола
					 *
					 * @param proto устанавливаемый протокол
					 */
					void proto(const proto_t proto) noexcept;
				public:
					/**
					 * @brief Метод установки таймаута
					 *
					 * @param msec   количество миллисекунд
					 * @param method метод для установки таймаута
					 * @return       результат установки таймаута
					 */
					bool timeout(const uint32_t msec, const method_t method) noexcept;
				public:
					/**
					 * @brief Метод получения размеров буфера
					 *
					 * @param method метод для выполнения операции с буфером
					 * @return       размер буфера
					 */
					int32_t buffer(const method_t method) const noexcept;
					/**
					 * @brief Метод установки размеров буфера
					 *
					 * @param read  размер буфера на чтение
					 * @param write размер буфера на запись
					 * @return      результат работы функции
					 */
					bool buffer(const int32_t read, const int32_t write) noexcept;
				public:
					/**
					 * @brief Метод проверки количества находящихся байт в сокете
					 *
					 * @param method метод для выполнения операции
					 * @return       количество байт в сокете
					 */
					int32_t availability(const method_t method) const noexcept;
				private:
					/**
					 * @brief Метод выполнения выбора следующего протокола
					 *
					 * @param out     буфер назначения
					 * @param outSize размер буфера назначения
					 * @param in      буфер входящих данных
					 * @param inSize  размер буфера входящих данных
					 * @param key     ключ копирования
					 * @param keySize размер ключа для копирования
					 * @return        результат переключения протокола
					 */
					bool selectProto(uint8_t ** out, uint8_t * outSize, const uint8_t * in, uint32_t inSize, const char * key, uint32_t keySize) const noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 *
					 * @note Реализация в src/net/engine.cpp: состояние связи описано там же,
					 *       и разрушитель unique_ptr требует, чтобы тип был полным
					 */
					explicit Context(const fmk_t * fmk, const log_t * log) noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~Context() noexcept;
			} ctx_t;
		private:
			// Флаг проверки сертификата доменного имени
			bool _verify;
		private:
			// Объект работы с файловой системой
			fs_t _fs;
		private:
			// Список алгоритмов шифрования
			string _cipher;
		private:
			// Объект файлов сертификатов
			mutable cert_t _cert;
			// Сертификаты сервера по доменным именам (SNI)
			std::unordered_map <string, cert_t> _sni;
		private:
			/**
			 * @brief Класс запаса подготовленных подлинностей
			 *
			 */
			class Keys;
			// Запас подготовленных подлинностей (реализация — в src/net/engine.cpp)
			std::unique_ptr <Keys> _keys;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
			// Объект работы с URI
			const uri_t * _uri;
		private:
			/**
			 * @brief Метод прогрева запаса подготовленных подлинностей по зарегистрированным именам
			 *
			 */
			void warm() noexcept;
		private:
			/**
			 * @brief Метод инициализации запаса доверенных сертификатов
			 *
			 * @param target контекст назначения
			 * @return       результат инициализации
			 */
			bool storeCA(ctx_t & target) const noexcept;
			/**
			 * @brief Метод инициализации запаса списков отзыва сертификатов
			 *
			 * @param target контекст назначения
			 * @return       результат инициализации
			 *
			 */
			bool storeCRL(ctx_t & target) const noexcept;
			/**
			 * @brief Метод установки подлинности стороны связи по файлам сертификатов
			 *
			 * @details Подготовленный ключ хранится в запасе движка: подготовка ключа RSA стоит
			 *          вдвое дороже самой подписи, и повторять её на каждое подключение нельзя.
			 *
			 * @param target контекст назначения
			 * @param cert   адреса файлов сертификатов
			 * @return       результат установки
			 *
			 */
			bool credential(ctx_t & target, const cert_t & cert) noexcept;
			/**
			 * @brief Метод заведения связи наружного модуля шифрования
			 *
			 * @param target контекст назначения
			 * @param type   тип активного приложения
			 * @param host   хост удалённого сервера
			 * @return       результат заведения связи
			 */
			bool wrapTLS(ctx_t & target, const type_t type, const string & host) noexcept;
		private:
			/**
			 * @brief Метод проверки доменного имени по цепочке сертификатов собеседника
			 *
			 * @param host  доменное имя
			 * @param chain цепочка сертификатов собеседника записями DER
			 * @return      результат проверки
			 */
			validate_t validateHostname(const string & host, const vector <vector <uint8_t>> & chain) const noexcept;
		public:
			/**
			 * @brief Метод ожидания рукопожатия
			 *
			 * @param target контекст назначения
			 * @return       результат проверки
			 */
			bool wait(ctx_t & target) noexcept;
		public:
			/**
			 * @brief Метод проверки на активацию режима шифрования
			 *
			 * @param ctx контекст подключения
			 * @return    результат проверки
			 */
			bool encrypted(ctx_t & ctx) const noexcept;
			/**
			 * @brief Метод установки флага режима шифрования
			 *
			 * @param mode флаг режима шифрования
			 * @param ctx  контекст подключения
			 */
			void encrypted(const bool mode, ctx_t & ctx) noexcept;
		public:
			/**
			 * @brief Метод извлечения активного протокола
			 *
			 * @param target контекст назначения
			 * @return       метод активного протокола
			 */
			proto_t proto(ctx_t & target) const noexcept;
		private:
			/**
			 * @brief Метод активации протокола HTTP
			 *
			 * @param target контекст назначения
			 */
			void httpUpgrade(ctx_t & target) const noexcept;
		public:
			/**
			 * @brief Метод прикрепления контекста клиента к контексту сервера
			 *
			 * @param target  контекст назначения
			 * @param address объект подключения
			 */
			void attach(ctx_t & target, addr_t * address) noexcept;
		public:
			/**
			 * @brief Метод обертывания сетевого сокета для сервера
			 *
			 * @param target контекст назначения
			 * @param source исходный контекст
			 */
			void wrap(ctx_t & target, ctx_t & source) noexcept;
			/**
			 * @brief Метод обертывания сетевого сокета для сервера
			 *
			 * @param target  контекст назначения
			 * @param address объект подключения
			 */
			void wrap(ctx_t & target, addr_t * address) noexcept;
		public:
			/**
			 * @brief Метод обертывания сетевого сокета для клиента/сервера
			 *
			 * @param target  контекст назначения
			 * @param address объект подключения
			 * @param type    тип активного приложения
			 */
			void wrap(ctx_t & target, addr_t * address, const type_t type) noexcept;
			/**
			 * @brief Метод обертывания сетевого сокета для клиента/сервера
			 *
			 * Для клиента DTLS хост удалённого сервера передаётся в расширение SNI
			 * и используется для проверки соответствия сертификата доменному имени
			 *
			 * @param target  контекст назначения
			 * @param address объект подключения
			 * @param type    тип активного приложения
			 * @param host    хост удалённого сервера
			 */
			void wrap(ctx_t & target, addr_t * address, const type_t type, const string & host) noexcept;
		public:
			/**
			 * @brief Метод обертывания сетевого сокета для клиента
			 *
			 * @param target контекст назначения
			 * @param source исходный контекст
			 * @param host   хост удалённого сервера
			 */
			void wrap(ctx_t & target, ctx_t & source, const string & host) noexcept;
			/**
			 * @brief Метод обертывания сетевого сокета для клиента
			 *
			 * @param target  контекст назначения
			 * @param address объект подключения
			 * @param host    хост удалённого сервера
			 */
			void wrap(ctx_t & target, addr_t * address, const string & host) noexcept;
		public:
			/**
			 * @brief Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
			 *
			 * @param mode флаг состояния разрешения проверки
			 */
			void verify(const bool mode) noexcept;
		public:
			/**
			 * @brief Метод установки алгоритмов шифрования
			 *
			 * @param ciphers список алгоритмов шифрования для установки
			 */
			void ciphers(const vector <string> & ciphers) noexcept;
		public:
			/**
			 * @brief Метод установки CRL-файла отозванных сертификатов центром сертификации
			 *
			 * @param crl адрес CRL-файла отозванных сертификатов центром сертификации
			 */
			void crl(const string & crl) noexcept;
			/**
			 * @brief Метод установки сертификата центра сертификации (CA-файла)
			 *
			 * @param ca   адрес сертификата центра сертификации (CA-файла)
			 * @param path адрес каталога где находится сертификат (CA-файл)
			 */
			void ca(const string & ca, const string & path = "") noexcept;
			/**
			 * @brief Метод установки файлов сертификата
			 *
			 * @param pem файл цепочки сертификатов
			 * @param key приватный ключ сертификата (если требуется)
			 */
			void certificate(const string & pem, const string & key = "") noexcept;
			/**
			 * @brief Метод установки файлов сертификата сервера для доменного имени (SNI)
			 *
			 * Если сертификат по умолчанию ещё не задан, этот сертификат становится и сертификатом по умолчанию
			 *
			 * @param host доменное имя (допускается маска вида *.example.com)
			 * @param pem  файл цепочки сертификатов
			 * @param key  приватный ключ сертификата
			 */
			void certificate(const string & host, const string & pem, const string & key) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 */
			explicit Engine(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Engine() noexcept;
	} engine_t;
};

#endif // __AWH_ENGINE__
