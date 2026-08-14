/**
 * @file server.cpp
 * @date 2026-05-24
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
 * @brief Реализация серверной стороны протокола SOCKS5 — разбор запросов клиента,
 *        согласование метода авторизации и формирование ответов на команды CONNECT, BIND и UDP ASSOCIATE
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартный заголовочный файл
 */
#include <cstring>
#include <cmath>
#include <array>

/**
 * Системный заголовочный файл
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
	#include <netinet/in.h>
#endif

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/socks5/server.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статические параметры в пространство имён
 *
 */
namespace {
	/**
	 * @brief Типы адресации
	 *
	 */
	enum class addr_type_t : uint8_t {
		NONE = 0x00, // Тип адреса не определён
		IPV4 = 0x01, // Поддерживается IPv4 IP адрес
		FQDN = 0x03, // Поддерживается доменное имя
		IPV6 = 0x04  // Поддерживается IPv6 IP адрес
	};

	/**
	 * @brief Основные методы
	 *
	 */
	enum class method_t : uint8_t {
		NOAUTH   = 0x00, // Аутентификация не требуется
		GSSAPI   = 0x01, // Аутентификация по GSSAPI
		PASSWD   = 0x02, // Аутентификация по USERNAME/PASSWORD
		IANA     = 0x03, // До X'7F' зарезервировано IANA
		RESERVE  = 0x80, // До X'FE' преднозначено для частных методов
		NOMETHOD = 0xFF  // Нет применимых методов
	};

	/**
	 * @brief Структура заголовка пакета для UDP протокола
	 *
	 */
	typedef struct UDP {
		// Зарезервировано, всегда 0x0000
		uint16_t rsv;
		// Номер фрагмента (0x00 = нет фрагментации)
		uint8_t frag;
		// Тип адреса
		uint8_t atyp;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit UDP() noexcept :
		 rsv(0x0000), frag(0x00), atyp(0x00) {}
	} __attribute__((packed)) udp_t;

	/**
	 * @brief Структура заголовка авторизации
	 *
	 */
	typedef struct Auth {
		// Версия прокси-протокола
		uint8_t ver;
		// Статус авторизации на сервере
		uint8_t status;
		/**
		 * @brief Конструктор
		 *
		 */
		Auth() noexcept :
		 ver(0x00), status(0x00) {}
	} __attribute__((packed)) auth_t;

	/**
	 * @brief Структура заголовка пакета
	 *
	 */
	typedef struct Header {
		// Версия прокси-протокола
		uint8_t ver;
		// Выбранный метод сервера
		uint8_t method;
		/**
		 * @brief Конструктор
		 *
		 */
		Header() noexcept :
		 ver(0x00), method(0x00) {}
	} __attribute__((packed)) header_t;

	/**
	 * @brief Структура ip адреса сервера
	 *
	 */
	typedef struct IPv4 {
		// Хост сервера
		uint32_t host;
		// Порт сервера
		uint16_t port;
		/**
		 * @brief Конструктор
		 *
		 */
		IPv4() noexcept : host(0), port(0) {}
	} __attribute__((packed)) ip4_t;

	/**
	 * @brief Структура ip адреса сервера
	 *
	 */
	typedef struct IPv6 {
		// Хост сервера
		array <uint8_t, 16> host;
		// Порт сервера
		uint16_t port;
		/**
		 * @brief Конструктор
		 *
		 */
		IPv6() noexcept : host{0}, port(0) {}
	} __attribute__((packed)) ip6_t;

	/**
	 * @brief Структура запроса
	 *
	 */
	typedef struct Request {
		uint8_t ver;  // Версия прокси-протокола
		uint8_t cmd;  // Код запроса у прокси-сервера
		uint8_t rsv;  // Зарезервированный октет
		uint8_t type; // Тип подключения
		/**
		 * @brief Конструктор
		 *
		 */
		Request() noexcept :
		 ver(0x00), cmd(0x00),
		 rsv(0x00), type(0x00) {}
	} __attribute__((packed)) request_t;
};

/**
 * @brief Инкапсулируем статические параметры в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Размер данных в буфере
	 *
	 */
	thread_local size_t __awh_size__ = 0;

	/**
	 * @brief Буфер временного хранения данных
	 *
	 */
	thread_local uint8_t __awh_buffer__[proto::socks5_t::SOCKS5_TX_BUFFER_SIZE] = {0};
};

/**
 * @brief Инкапсулируем статические функции в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Шаблон функции добавления полезной нагрузки в буфер
	 *
	 * @tparam T тип данных для добавления в буфер
	 *
	 */
	template <typename T>
	/**
	 * @brief Функция добавления полезной нагрузки в буфер
	 *
	 * @param data данные для добавления в буфер
	 *
	 */
	void addPayload(const T & data) noexcept {
		// Получаем размер данных для добавления в буфер
		const size_t size = sizeof(data);
		// Если данных слишком много для буфера
		if((::__awh_size__ + size) > proto::socks5_t::SOCKS5_TX_BUFFER_SIZE)
			// Выходим из функции
			return;
		// Устанавливаем первый октет
		::memcpy(::__awh_buffer__ + ::__awh_size__, &data, size);
		// Возвращаем размер смещения
		::__awh_size__ += size;
	}
	/**
	 * @brief Функция добавления полезной нагрузки в буфер
	 *
	 * @param data данные для добавления в буфер
	 *
	 */
	void addPayload(const string & data) noexcept {
		// Если данных слишком много для буфера
		if((::__awh_size__ + data.length()) > proto::socks5_t::SOCKS5_TX_BUFFER_SIZE)
			// Выходим из функции
			return;
		// Устанавливаем первый октет
		::memcpy(::__awh_buffer__ + ::__awh_size__, data.c_str(), data.length());
		// Возвращаем размер смещения
		::__awh_size__ += data.length();
	}
};

/**
 * @brief Метод парсинга входящих данных
 *
 * @param buffer бинарный буфер входящих данных
 * @param size   размер бинарного буфера входящих данных
 * @param ctx    объект для извлечения параметров сообщения
 * @return       результат парсинга входящих данных
 *
 */
bool awh::proto::Server_Socks5::parse(const void * buffer, const size_t size, ctx_t & ctx) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если данные буфера переданы
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Определяем текущее состояние сервера
			 */
			switch(static_cast <uint8_t> (ctx.state)){
				// Если текущее состояние ещё не определено
				case static_cast <uint8_t> (state_t::NONE): {
					// Если данных достаточно для получения ответа
					if(size > sizeof(uint16_t)){
						// Версия прокси-протокола
						uint8_t version = 0x00;
						// Выполняем чтение версии протокола
						::memcpy(&version, buffer, sizeof(version));
						// Если версия протокола соответствует
						if(version == 0x05){
							// Количество методов авторизации
							uint8_t count = 0x00;
							// Формируем смещение в буфере
							size_t offset = sizeof(version);
							// Выполняем чтение количество методов авторизации
							::memcpy(&count, reinterpret_cast <const uint8_t *> (buffer) + offset, sizeof(count));
							// Увеличиваем размер смещения на размер количества методов авторизации
							offset += sizeof(count);
							// Если количество методов авторизации получено
							if((result = ((count > 0) && (size >= (sizeof(uint16_t) + (sizeof(uint8_t) * count)))))){
								// Устанавливаем статус ожидания ответа на авторизацию
								ctx.state = state_t::AUTH;
								// Устанавливаем статус ошибки
								ctx.status = proto::socks5_t::status_t::DENIED;
								// Временное значение метода для извлечения
								method_t method = method_t::NOMETHOD;
								/**
								 * Переходим по всем методам авторизации
								 */
								for(uint8_t i = 0; i < count; i++){
									// Получаем метод авторизации
									::memcpy(&method, reinterpret_cast <const uint8_t *> (buffer) + offset, sizeof(method));
									// Увеличиваем размер смещения на размер метода авторизации
									offset += sizeof(method);
									// Если поддерживается метод аутентификации по USERNAME/PASSWORD
									if(method == method_t::PASSWD){
										// Если функция обратного вызова для обработки авторизации установлена
										if(this->_callback != nullptr){
											// Устанавливаем статус ожидания ответа авторизации
											ctx.status = proto::socks5_t::status_t::FORBIDDEN;
											// Выходим из цикла обработки методов авторизации
											break;
										}
									// Если поддерживается метод аутентификации "без аутентификации"
									} else if(method == method_t::NOAUTH) {
										// Если функция обратного вызова для обработки авторизации не установлена
										if(this->_callback == nullptr){
											// Устанавливаем статус разрешающего подключения
											ctx.status = proto::socks5_t::status_t::SUCCESS;
											// Выходим из цикла обработки методов авторизации
											break;
										}
									}
								}
							}
						// Если версия прокси-сервера не соответствует
						} else {
							// Устанавливаем состояние клиента как "сломанный"
							ctx.state = state_t::BROKEN;
							// Устанавливаем статус ошибки
							ctx.status = proto::socks5_t::status_t::SOCKSERR;
						}
					}
				} break;
				// Если текущее состояние соответствует этапу аутентификации
				case static_cast <uint8_t> (state_t::AUTH): {
					// Если данных достаточно для получения ответа
					if(size > sizeof(uint16_t)){
						// Версия прокси-протокола
						uint8_t version = 0x00;
						// Получаем смещение в буфере
						size_t offset = sizeof(version);
						// Выполняем чтение версии соглашения авторизации
						::memcpy(&version, buffer, offset);
						// Если версия соглашения авторизации соответствует
						if(version == 0x01){
							// Размер логина пользователя
							uint8_t length = 0x00;
							// Выполняем получение длины логина пользователя
							::memcpy(&length, reinterpret_cast <const uint8_t *> (buffer) + offset, sizeof(length));
							// Увеличиваем размер смещения на размер длины логина пользователя
							offset += sizeof(length);
							// Если количество байт достаточно, чтобы получить логин пользователя
							if(size >= (offset + length)){
								// Устанавливаем статус отправки ответа
								ctx.state = state_t::RESPONSE;
								// Устанавливаем статус запрета на подключение
								ctx.status = proto::socks5_t::status_t::DENIED;
								// Если логин пользователя для авторизации на сервере получен
								if(length > 0){
									// Получаем логин пользователя для авторизации на сервере
									const string username(reinterpret_cast <const char *> (buffer) + offset, length);
									// Увеличиваем размер смещения на размер логина пользователя для авторизации на сервере
									offset += length;
									// Если данных достаточно, чтобы получить размер пароля пользователя для авторизации на сервере
									if(size >= (offset + sizeof(uint8_t))){
										// Выполняем получение длины пароля пользователя
										::memcpy(&length, reinterpret_cast <const uint8_t *> (buffer) + offset, sizeof(length));
										// Увеличиваем размер смещения на размер длины пароля пользователя
										offset += sizeof(length);
										// Если пароль пользователя для авторизации на сервере получен
										if(length > 0){
											// Если данных достаточно, чтобы получить пароль пользователя для авторизации на сервере
											if((result = (size >= (offset + length)))){
												// Получаем пароль пользователя для авторизации на сервере
												const string password(reinterpret_cast <const char *> (buffer) + offset, length);
												// Увеличиваем размер смещения на размер пароля пользователя для авторизации на сервере
												offset += length;
												// Если функция обратного вызова для обработки авторизации установлена
												if(this->_callback != nullptr){
													// Если авторизация на сервере прошла успешно
													if(!this->_callback(username, password))
														// Устанавливаем статус ошибки
														ctx.status = proto::socks5_t::status_t::FORBIDDEN;
													// Если авторизация на сервере прошла успешно, устанавливаем статус успеха
													else ctx.status = proto::socks5_t::status_t::SUCCESS;
												}
											}
										}
									}
								}
							}
						// Если версия прокси-сервера не соответствует
						} else {
							// Устанавливаем состояние клиента как "сломанный"
							ctx.state = state_t::BROKEN;
							// Устанавливаем статус ошибки
							ctx.status = proto::socks5_t::status_t::SOCKSERR;
						}
					}
				} break;
				// Если текущее состояние соответствует ожиданию выполнения подключения
				case static_cast <uint8_t> (state_t::CONNECT): {
					// Если данных достаточно для получения запроса
					if(size > sizeof(request_t)){
						// Создаём объект данных запроса
						request_t request;
						// Выполняем чтение данных
						::memcpy(&request, buffer, sizeof(request));
						// Если версия протокола соответствует
						if(request.ver == 0x05){
							// Устанавливаем стейт рукопожатия
							ctx.state = state_t::HANDSHAKE;
							// Устанавливаем статус запрета на подключение
							ctx.status = proto::socks5_t::status_t::DENIED;
							// Устанавливаем тип команды "команда подключения"
							ctx.command = static_cast <command_t> (request.cmd);
							/**
							 * Определяем запрошенную команду подключения
							 */
							switch(request.cmd){
								// Если запрошенная команда соответствует методу обратного подключения (сервера к клиенту)
								case static_cast <uint8_t> (command_t::BIND): {
									// Устанавливаем результат парсинга
									result = true;
									// Устанавливаем статус неподдерживаемой команды
									ctx.status = proto::socks5_t::status_t::NOCOMMAND;
								} break;
								// Если запрошенная команда соответствует работе с UDP протоколом
								case static_cast <uint8_t> (command_t::UDP):
								// Если запрошенная команда соответствует методу подключения
								case static_cast <uint8_t> (command_t::CONNECT): {
									/**
									 * Определяем тип адреса
									 */
									switch(request.type){
										// Если тип адреса соответствует доменному имени
										case static_cast <uint8_t> (addr_type_t::FQDN): {
											// Если буфер пришел достаточного размера
											if((result = (size >= (sizeof(request_t) + 3)))){
												// Размер доменного имени для подключения
												uint8_t length = 0;
												// Формируем смещение в буфере
												size_t offset = sizeof(request_t);
												// Копируем в буфер размер доменного имени для подключения
												::memcpy(&length, reinterpret_cast <const uint8_t *> (buffer) + offset, sizeof(length));
												// Увеличиваем смещение на размер доменного имени для подключения
												offset += sizeof(length);
												// Если буфер пришел достаточного размера для извлечения доменного имени
												if((result = ((offset + (static_cast <uint16_t> (length) + 2)) <= size))){
													// Выполняем инициализацию объекта хоста
													ctx.host = make_unique <net::attr_fqdn_t> ();
													// Устанавливаем тип адреса события
													ctx.host->type = net::type_t::FQDN;
													// Если доменное имя для подключения не пустое
													if(length > 0){
														// Выделяем память для доменного имени хоста для подключения
														awh_cast <net::attr_fqdn_t *> (ctx.host.get())->domain.resize(length, 0);
														// Копируем в буфер доменное имя хоста для подключения
														::memcpy(&awh_cast <net::attr_fqdn_t *> (ctx.host.get())->domain[0], reinterpret_cast <const uint8_t *> (buffer) + offset, length);
														// Увеличиваем смещение на размер доменного имени для подключения
														offset += length;
													}
													// Порт хоста для подключения
													uint16_t port = 0;
													// Извлекаем порт хоста для подключения
													::memcpy(&port, reinterpret_cast <const uint8_t *> (buffer) + offset, sizeof(port));
													// Увеличиваем смещение на размер порта хоста для подключения
													offset += sizeof(port);
													// Устанавливаем порт хоста для подключения
													awh_cast <net::attr_fqdn_t *> (ctx.host.get())->port = ntohs(port);
													// Устанавливаем статус успеха
													ctx.status = proto::socks5_t::status_t::SUCCESS;
												}
											}
										} break;
										// Если тип адреса соответствует IPv4 IP адресу
										case static_cast <uint8_t> (addr_type_t::IPV4): {
											// Если буфер пришел достаточного размера
											if((result = (size >= (sizeof(request_t) + sizeof(ip4_t))))){
												// Создаём объект данных сервера
												ip4_t server{};
												// Копируем в буфер наши данные IP адреса
												::memcpy(&server, reinterpret_cast <const uint8_t *> (buffer) + sizeof(request_t), sizeof(server));
												// Выполняем инициализацию объекта хоста
												ctx.host = make_unique <net::attr_net_t> ();
												// Устанавливаем тип адреса события
												ctx.host->type = net::type_t::IPV4;
												// Устанавливаем порт хоста для подключения
												awh_cast <net::attr_net_t *> (ctx.host.get())->port = ntohs(server.port);
												// Создаём новый объект адреса клиента IPv4
												awh_cast <net::attr_net_t *> (ctx.host.get())->ip = make_unique <net::addr_net_ipv4_t> ();
												// Устанавливаем IP-адрес хоста для подключения
												awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (ctx.host.get())->ip.get())->address = server.host;
												// Устанавливаем статус успеха
												ctx.status = proto::socks5_t::status_t::SUCCESS;
											}
										} break;
										// Если тип адреса соответствует IPv6 IP адресу
										case static_cast <uint8_t> (addr_type_t::IPV6): {
											// Если буфер пришел достаточного размера
											if((result = (size >= (sizeof(request_t) + sizeof(ip6_t))))){
												// Создаём объект данных сервера
												ip6_t server{};
												// Копируем в буфер наши данные IP адреса
												::memcpy(&server, reinterpret_cast <const uint8_t *> (buffer) + sizeof(request_t), sizeof(server));
												// Выполняем инициализацию объекта хоста
												ctx.host = make_unique <net::attr_net_t> ();
												// Устанавливаем тип адреса события
												ctx.host->type = net::type_t::IPV6;
												// Устанавливаем порт хоста для подключения
												awh_cast <net::attr_net_t *> (ctx.host.get())->port = ntohs(server.port);
												// Создаём новый объект адреса клиента IPv6
												awh_cast <net::attr_net_t *> (ctx.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
												// Устанавливаем IP-адрес хоста для подключения
												awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (ctx.host.get())->ip.get())->address = ::move(server.host);
												// Устанавливаем статус успеха
												ctx.status = proto::socks5_t::status_t::SUCCESS;
											}
										} break;
									}
								} break;
								// Если запрошенная команда не соответствует поддерживаемым командам
								default:
									// Если авторизация не пройдена, устанавливаем статус ошибки
									ctx.status = proto::socks5_t::status_t::NOCOMMAND;
							}
						// Если версия прокси-сервера не соответствует
						} else {
							// Устанавливаем состояние клиента как "сломанный"
							ctx.state = state_t::BROKEN;
							// Устанавливаем статус ошибки
							ctx.status = proto::socks5_t::status_t::SOCKSERR;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод парсинга входящих данных
 *
 * @param buffer бинарный буфер входящих данных
 * @param size   размер бинарного буфера входящих данных
 * @param udp    объект для извлечения параметров UDP заголовка
 * @return       результат парсинга входящих данных
 *
 */
bool awh::proto::Server_Socks5::parse(const void * buffer, const size_t size, udp_head_t & udp) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если данные буфера переданы
		if((buffer != nullptr) && (size > sizeof(udp_t))){
			// Инициализируем объект заголовка пакета
			udp_t header{};
			// Выполняем чтение данных заголовка пакета из буфера входящих данных
			::memcpy(&header, buffer, sizeof(header));
			// Если зарезервированный октет в заголовке пакета соответствует установленному значению
			if((header.rsv == 0x0000) && (header.frag == 0x00)){
				// Устанавливаем фрагментацию в объекте UDP заголовка
				udp.frag = header.frag;
				// Устанавливаем размер данных в объекте UDP заголовка
				udp.size = sizeof(udp_t);
				/**
				 * Определяем тип адреса
				 */
				switch(header.atyp){
					// Если тип адреса соответствует FQDN
					case static_cast <uint8_t> (addr_type_t::FQDN): {
						// Если буфер пришел достаточного размера
						if((result = (size >= (udp.size + 3)))){
							// Размер доменного имени для подключения
							uint8_t length = 0;
							// Копируем в буфер размер доменного имени для подключения
							::memcpy(&length, reinterpret_cast <const uint8_t *> (buffer) + udp.size, sizeof(length));
							// Увеличиваем смещение на размер доменного имени для подключения
							udp.size += sizeof(length);
							// Если буфер пришел достаточного размера для извлечения доменного имени
							if((result = ((udp.size + (static_cast <uint16_t> (length) + 2)) <= size))){
								// Выполняем инициализацию объекта хоста
								udp.host = make_unique <net::attr_fqdn_t> ();
								// Устанавливаем тип адреса события
								udp.host->type = net::type_t::FQDN;
								// Если доменное имя для подключения не пустое
								if(length > 0){
									// Выделяем память для доменного имени хоста для подключения
									awh_cast <net::attr_fqdn_t *> (udp.host.get())->domain.resize(length, 0);
									// Копируем в буфер доменное имя хоста для подключения
									::memcpy(&awh_cast <net::attr_fqdn_t *> (udp.host.get())->domain[0], reinterpret_cast <const uint8_t *> (buffer) + udp.size, length);
									// Увеличиваем смещение на размер доменного имени для подключения
									udp.size += length;
								}
								// Порт хоста для подключения
								uint16_t port = 0;
								// Извлекаем порт хоста для подключения
								::memcpy(&port, reinterpret_cast <const uint8_t *> (buffer) + udp.size, sizeof(port));
								// Увеличиваем смещение на размер порта хоста для подключения
								udp.size += sizeof(port);
								// Устанавливаем порт хоста для подключения
								awh_cast <net::attr_fqdn_t *> (udp.host.get())->port = ntohs(port);
							}
						}
					} break;
					// Если тип адреса соответствует IPv4
					case static_cast <uint8_t> (addr_type_t::IPV4): {
						// Если буфер пришел достаточного размера
						if((result = (size >= (udp.size + sizeof(ip4_t))))){
							// Создаём объект данных сервера
							ip4_t server{};
							// Копируем в буфер наши данные IP адреса
							::memcpy(&server, reinterpret_cast <const uint8_t *> (buffer) + udp.size, sizeof(server));
							// Выполняем инициализацию объекта хоста
							udp.host = make_unique <net::attr_net_t> ();
							// Устанавливаем тип адреса события
							udp.host->type = net::type_t::IPV4;
							// Устанавливаем порт хоста для подключения
							awh_cast <net::attr_net_t *> (udp.host.get())->port = ntohs(server.port);
							// Создаём новый объект адреса клиента IPv4
							awh_cast <net::attr_net_t *> (udp.host.get())->ip = make_unique <net::addr_net_ipv4_t> ();
							// Устанавливаем IP-адрес хоста для подключения
							awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (udp.host.get())->ip.get())->address = server.host;
							// Увеличиваем размер данных в объекте UDP заголовка
							udp.size += sizeof(server);
						}
					} break;
					// Если тип адреса соответствует IPv6
					case static_cast <uint8_t> (addr_type_t::IPV6): {
						// Если буфер пришел достаточного размера
						if((result = (size >= (udp.size + sizeof(ip6_t))))){
							// Создаём объект данных сервера
							ip6_t server{};
							// Копируем в буфер наши данные IP адреса
							::memcpy(&server, reinterpret_cast <const uint8_t *> (buffer) + udp.size, sizeof(server));
							// Выполняем инициализацию объекта хоста
							udp.host = make_unique <net::attr_net_t> ();
							// Устанавливаем тип адреса события
							udp.host->type = net::type_t::IPV6;
							// Устанавливаем порт хоста для подключения
							awh_cast <net::attr_net_t *> (udp.host.get())->port = ntohs(server.port);
							// Создаём новый объект адреса клиента IPv6
							awh_cast <net::attr_net_t *> (udp.host.get())->ip = make_unique <net::addr_net_ipv6_t> ();
							// Устанавливаем IP-адрес хоста для подключения
							awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (udp.host.get())->ip.get())->address = ::move(server.host);
							// Увеличиваем размер данных в объекте UDP заголовка
							udp.size += sizeof(server);
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод извлечения буфера запроса/ответа
 *
 * @param buffer указатель на буфер для извлечения данных
 * @param size   ссылка на размер буфера для извлечения данных
 * @param ctx    объект для установки параметров сообщения
 * @return 	     результат извлечения данных в буфер
 *
 */
bool awh::proto::Server_Socks5::buffer(uint8_t ** buffer, size_t & size, ctx_t & ctx) const noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Обнуляем размер буфера для отправки данных
		::__awh_size__ = 0;
		/**
		 * Определяем текущее состояние сервера
		 */
		switch(static_cast <uint8_t> (ctx.state)){
			// Если текущее состояние соответствует этапу аутентификации
			case static_cast <uint8_t> (state_t::AUTH): {
				// Устанавливаем результат для отправки данных
				result = true;
				// Формируем объект заголовка
				header_t header{};
				// Устанавливаем версию прокси-протокола
				header.ver = 0x05;
				/**
				 * Определяем статус разрешающего подключения к серверу
				 */
				switch(static_cast <uint8_t> (ctx.status)){
					// Если статус соответствует разрешающему подключение к серверу
					case static_cast <uint8_t> (proto::socks5_t::status_t::SUCCESS): {
						// Устанавливаем статус запроса ожидания подключения к серверу
						ctx.state = state_t::CONNECT;
						// Устанавливаем статус разрешающего подключения к серверу
						header.method = static_cast <uint8_t> (method_t::NOAUTH);
					} break;
					// Если статус соответствует ожиданию авторизации на сервере
					case static_cast <uint8_t> (proto::socks5_t::status_t::FORBIDDEN):
						// Устанавливаем статус ожидания авторизации на сервере
						header.method = static_cast <uint8_t> (method_t::PASSWD);
					break;
					// Если статус соответствует запрету на подключение к серверу
					case static_cast <uint8_t> (proto::socks5_t::status_t::DENIED): {
						// Устанавливаем статус запроса повторить попытку подключения к серверу
						ctx.state = state_t::NONE;
						// Устанавливаем статус запрета на подключение к серверу
						header.method = static_cast <uint8_t> (method_t::NOMETHOD);
					} break;
				}
				// Добавляем параметры аутентификации в буфер для отправки данных
				::addPayload(header);
			} break;
			// Если текущее состояние соответствует этапу ответа
			case static_cast <uint8_t> (state_t::RESPONSE): {
				// Устанавливаем результат для отправки данных
				result = true;
				// Формируем объект заголовка
				auth_t auth{};
				// Устанавливаем версию соглашения авторизации
				auth.ver = 0x01;
				// Устанавливаем статус успешной авторизации на сервере
				auth.status = static_cast <uint8_t> (ctx.status);
				// Добавляем параметры авторизации в буфер для отправки данных
				::addPayload(auth);
				// Если статус не соответствует разрешающему подключение к серверу
				if(ctx.status != proto::socks5_t::status_t::SUCCESS)
					// Устанавливаем статус ожидания ответа на авторизацию
					ctx.state = state_t::AUTH;
				// Устанавливаем статус запроса
				else ctx.state = state_t::CONNECT;
			} break;
			// Если текущее состояние соответствует этапу рукопожатия
			case static_cast <uint8_t> (state_t::HANDSHAKE): {
				// Добавляем версию прокси-протокола в буфер для отправки данных
				::addPayload(static_cast <uint8_t> (0x05));
				// Добавляем статус разрешающего подключение к серверу в буфер для отправки данных
				::addPayload(ctx.status);
				// Добавляем зарезервированный октет в буфер для отправки данных
				::addPayload(static_cast <uint8_t> (0x00));
				// Если статус не соответствует разрешающему подключение к серверу
				if(ctx.status != proto::socks5_t::status_t::SUCCESS)
					// Устанавливаем статус запроса ожидания подключения к серверу
					ctx.state = state_t::CONNECT;
				// Если хост для подключения установлен
				if(ctx.host != nullptr){
					/**
					 * Определяем тип адреса хоста для подключения
					 */
					switch(static_cast <uint8_t> (ctx.host->type)){
						// Если тип адреса соответствует FQDN
						case static_cast <uint8_t> (net::type_t::FQDN): {
							// Добавляем тип адреса "доменные имена" в буфер для отправки данных
							::addPayload(addr_type_t::FQDN);
							// Извлекаем доменное имя хоста для подключения
							const string & fqdn = awh_cast <net::attr_fqdn_t *> (ctx.host.get())->domain;
							// Добавляем размер доменного имени хоста для подключения в буфер для отправки данных
							::addPayload(static_cast <uint8_t> (fqdn.length()));
							// Устанавливаем доменное имя хоста для подключения в буфер для отправки данных
							::addPayload(fqdn);
							// Добавляем порт хоста для подключения в буфер для отправки данных
							::addPayload(htons(awh_cast <net::attr_fqdn_t *> (ctx.host.get())->port));
						} break;
						// Если тип адреса соответствует IPv4
						case static_cast <uint8_t> (net::type_t::IPV4): {
							// Добавляем тип адреса "IPv4" в буфер для отправки данных
							::addPayload(addr_type_t::IPV4);
							// Получаем объект атрибутов хоста для подключения
							net::attr_net_t * host = awh_cast <net::attr_net_t *> (ctx.host.get());
							// Если IP-адрес хоста для подключения установлен
							if(host->ip != nullptr)
								// Добавляем IP адрес хоста для подключения в буфер для отправки данных
								::addPayload(awh_cast <net::addr_net_ipv4_t *> (host->ip.get())->address);
							// Добавляем нулевой IP-адрес в буфер для отправки данных, если адрес хоста не установлен (RFC 1928)
							else ::addPayload(static_cast <uint32_t> (0));
							// Добавляем порт хоста для подключения в буфер для отправки данных
							::addPayload(htons(awh_cast <net::attr_net_t *> (ctx.host.get())->port));
						} break;
						// Если тип адреса соответствует IPv6
						case static_cast <uint8_t> (net::type_t::IPV6): {
							// Добавляем тип адреса "IPv6" в буфер для отправки данных
							::addPayload(addr_type_t::IPV6);
							// Получаем объект атрибутов хоста для подключения
							net::attr_net_t * host = awh_cast <net::attr_net_t *> (ctx.host.get());
							// Если IP-адрес хоста для подключения установлен
							if(host->ip != nullptr)
								// Добавляем IP адрес хоста для подключения в буфер для отправки данных
								::addPayload(awh_cast <net::addr_net_ipv6_t *> (host->ip.get())->address);
							// Добавляем нулевой IP-адрес в буфер для отправки данных, если адрес хоста не установлен (RFC 1928)
							else ::addPayload(array <uint8_t, 16> {});
							// Добавляем порт хоста для подключения в буфер для отправки данных
							::addPayload(htons(awh_cast <net::attr_net_t *> (ctx.host.get())->port));
						} break;
					}
					// Устанавливаем положительный результат
					result = true;
				// Если хост не установлен, формируем ответ с пустым адресом (RFC 1928)
				} else if(ctx.status != proto::socks5_t::status_t::SUCCESS){
					// Добавляем тип адреса "IPv4" в буфер для отправки данных
					::addPayload(addr_type_t::IPV4);
					// Добавляем IP-адрес 0.0.0.0 в буфер для отправки данных
					::addPayload(static_cast <uint32_t> (0));
					// Добавляем порт 0 в буфер для отправки данных
					::addPayload(static_cast <uint16_t> (0));
					// Устанавливаем положительный результат
					result = true;
				}
			} break;
		}
		// Устанавливаем результат для отправки размера данных
		size = ::__awh_size__;
		// Устанавливаем результат для отправки данных
		(* buffer) = ::__awh_buffer__;
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
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод извлечения буфера запроса/ответа
 *
 * @param buffer указатель на буфер для извлечения данных
 * @param size   ссылка на размер буфера для извлечения данных
 * @param udp    объект для установки параметров UDP заголовка
 * @return 	     результат извлечения данных в буфер
 *
 */
bool awh::proto::Server_Socks5::buffer(uint8_t ** buffer, size_t & size, const udp_head_t & udp) const noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Обнуляем размер буфера для отправки данных
		::__awh_size__ = 0;
		// Добавляем зарезервированный октет в буфер для отправки данных
		::addPayload(static_cast <uint16_t> (0x0000));
		// Добавляем фрагментацию в буфер для отправки данных
		::addPayload(udp.frag);
		/**
		 * Определяем тип адреса хоста для подключения
		 */
		switch(static_cast <uint8_t> (udp.host->type)){
			// Если тип адреса соответствует FQDN
			case static_cast <uint8_t> (net::type_t::FQDN): {
				// Устанавливаем результат для отправки данных
				result = true;
				// Добавляем тип адреса "доменные имена" в буфер для отправки данных
				::addPayload(addr_type_t::FQDN);
				// Извлекаем доменное имя хоста для подключения
				const string & fqdn = awh_cast <net::attr_fqdn_t *> (udp.host.get())->domain;
				// Добавляем размер доменного имени хоста для подключения в буфер для отправки данных
				::addPayload(static_cast <uint8_t> (fqdn.length()));
				// Устанавливаем доменное имя хоста для подключения в буфер для отправки данных
				::addPayload(fqdn);
				// Добавляем порт хоста для подключения в буфер для отправки данных
				::addPayload(htons(awh_cast <net::attr_fqdn_t *> (udp.host.get())->port));
			} break;
			// Если тип адреса соответствует IPv4
			case static_cast <uint8_t> (net::type_t::IPV4): {
				// Устанавливаем результат для отправки данных
				result = true;
				// Добавляем тип адреса "IPv4" в буфер для отправки данных
				::addPayload(addr_type_t::IPV4);
				// Получаем объект атрибутов хоста для подключения
				net::attr_net_t * host = awh_cast <net::attr_net_t *> (udp.host.get());
				// Если IP-адрес хоста для подключения установлен
				if(host->ip != nullptr)
					// Добавляем IP адрес хоста для подключения в буфер для отправки данных
					::addPayload(awh_cast <net::addr_net_ipv4_t *> (host->ip.get())->address);
				// Добавляем нулевой IP-адрес в буфер для отправки данных, если адрес хоста не установлен (RFC 1928)
				else ::addPayload(static_cast <uint32_t> (0));
				// Добавляем порт хоста для подключения в буфер для отправки данных
				::addPayload(htons(awh_cast <net::attr_net_t *> (udp.host.get())->port));
			} break;
			// Если тип адреса соответствует IPv6
			case static_cast <uint8_t> (net::type_t::IPV6): {
				// Устанавливаем результат для отправки данных
				result = true;
				// Добавляем тип адреса "IPv6" в буфер для отправки данных
				::addPayload(addr_type_t::IPV6);
				// Получаем объект атрибутов хоста для подключения
				net::attr_net_t * host = awh_cast <net::attr_net_t *> (udp.host.get());
				// Если IP-адрес хоста для подключения установлен
				if(host->ip != nullptr)
					// Добавляем IP адрес хоста для подключения в буфер для отправки данных
					::addPayload(awh_cast <net::addr_net_ipv6_t *> (host->ip.get())->address);
				// Добавляем нулевой IP-адрес в буфер для отправки данных, если адрес хоста не установлен (RFC 1928)
				else ::addPayload(array <uint8_t, 16> {});
				// Добавляем порт хоста для подключения в буфер для отправки данных
				::addPayload(htons(awh_cast <net::attr_net_t *> (udp.host.get())->port));
			} break;
		}
		// Устанавливаем результат для отправки размера данных
		size = ::__awh_size__;
		// Устанавливаем результат для отправки данных
		(* buffer) = ::__awh_buffer__;
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
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод добавления функции обработки авторизации
 *
 * @param callback функция обратного вызова для обработки авторизации
 *
 */
void awh::proto::Server_Socks5::on(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки авторизации
	this->_callback = ::move(callback);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::proto::Server_Socks5::Server_Socks5(const fmk_t * fmk, const log_t * log) noexcept :
 socks5_t(fmk, log), _callback(nullptr) {}
/**
 * @brief Деструктор
 *
 */
awh::proto::Server_Socks5::~Server_Socks5() noexcept {}
