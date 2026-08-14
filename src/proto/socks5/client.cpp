/**
 * @file client.cpp
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
 * @brief Реализация клиентской стороны протокола SOCKS5 — формирование запросов приветствия,
 *        авторизации и команд подключения и разбор ответов прокси-сервера
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
#include <proto/socks5/client.hpp>

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
	 * @brief Структура ответа
	 *
	 */
	typedef struct Response {
		// Версия прокси-протокола
		uint8_t ver;
		// Код ответа прокси-сервера
		uint8_t rep;
		// Зарезервированный октет
		uint8_t rsv;
		// Тип подключения
		uint8_t type;
		/**
		 * @brief Конструктор
		 *
		 */
		Response() noexcept :
		 ver(0x00), rep(0x00),
		 rsv(0x00), type(0x00) {}
	} __attribute__((packed)) response_t;
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
bool awh::proto::Client_Socks5::parse(const void * buffer, const size_t size, ctx_t & ctx) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если данные буфера переданы
		if((buffer != nullptr) && (size > 0)){
			/**
			 * Определяем текущее состояние клиента
			 */
			switch(static_cast <uint8_t> (ctx.state)){
				// Если текущее состояние соответствует этапу запроса
				case static_cast <uint8_t> (state_t::REQUEST): {
					// Если размер данных буфера хватает для извлечения заголовка пакета
					if((result = (size >= sizeof(header_t)))){
						// Инициализируем объект заголовка пакета
						header_t header{};
						// Выполняем чтение данных заголовка пакета из буфера входящих данных
						::memcpy(&header, buffer, sizeof(header));
						// Если версия прокси-протокола в заголовке пакета соответствует установленной версии прокси-протокола
						if(header.ver == 0x05){
							/**
							 * Определяем выбранный метод сервера
							 */
							switch(header.method){
								// Если выбранный метод сервера соответствует методу "без аутентификации"
								case static_cast <uint8_t> (method_t::NOAUTH): {
									// Устанавливаем статус ожидания ответа
									ctx.state = state_t::CONNECT;
									// Устанавливаем тип команды "команда подключения" в объекте сообщения
									ctx.command = command_t::CONNECT;
								} break;
								// Если выбранный метод сервера соответствует методу "аутентификация по паролю"
								case static_cast <uint8_t> (method_t::PASSWD): {
									// Проверяем установлен ли имя и пароль пользователя для авторизации на сервере
									if(!this->_username.empty() && !this->_password.empty())
										// Устанавливаем статус ожидания ответа на авторизацию
										ctx.state = state_t::AUTH;
									// Если логин и пароль не установлены
									else {
										// Устанавливаем статус ошибки
										ctx.state = state_t::BROKEN;
										// Устанавливаем статус ошибки
										ctx.status = proto::socks5_t::status_t::FORBIDDEN;
									}
								} break;
								// Если выбранный метод сервером не поддерживается
								case static_cast <uint8_t> (method_t::NOMETHOD): {
									// Устанавливаем статус ошибки
									ctx.state = state_t::BROKEN;
									// Устанавливаем статус ошибки
									ctx.status = proto::socks5_t::status_t::NOSUPPORT;
								} break;
								// Если выбранный метод сервера не соответствует поддерживаемым методам аутентификации
								default: {
									// Устанавливаем статус ошибки
									ctx.state = state_t::BROKEN;
									// Устанавливаем статус ошибки
									ctx.status = proto::socks5_t::status_t::FORBIDDEN;
								}
							}
						// Если версия прокси-сервера не соответствует
						} else {
							// Устанавливаем состояние клиента как "сломанный"
							ctx.state = state_t::BROKEN;
							// Устанавливаем статус ошибки
							ctx.status = proto::socks5_t::status_t::FORBIDDEN;
						}
					}
				} break;
				// Если текущее состояние соответствует этапу ответа
				case static_cast <uint8_t> (state_t::RESPONSE): {
					// Если размер данных буфера хватает для извлечения заголовка авторизации
					if((result = (size >= sizeof(auth_t)))){
						// Инициализируем объект заголовка авторизации
						auth_t auth{};
						// Выполняем чтение данных заголовка авторизации из буфера входящих данных
						::memcpy(&auth, buffer, sizeof(auth));
						// Если версия соглашения авторизации соответствует установленной версии соглашения авторизации
						if(auth.ver == 0x01){
							// Если авторизация на сервере прошла успешно
							if(auth.status == static_cast <uint8_t> (status_t::SUCCESS)){
								// Устанавливаем статус ожидания ответа
								ctx.state = state_t::CONNECT;
								// Устанавливаем тип команды "команда подключения" в объекте сообщения
								ctx.command = command_t::CONNECT;
							// Если авторизация не пройдена
							} else {
								// Устанавливаем состояние клиента как "сломанный"
								ctx.state = state_t::BROKEN;
								// Устанавливаем код ошибки полученного ответа
								ctx.status = static_cast <proto::socks5_t::status_t> (auth.status);
							}
						// Если версия прокси-сервера не соответствует
						} else {
							// Устанавливаем состояние клиента как "сломанный"
							ctx.state = state_t::BROKEN;
							// Устанавливаем код ошибки "несовпадение версии" в объекте сообщения
							ctx.status = proto::socks5_t::status_t::FORBIDDEN;
						}
					}
				} break;
				// Если текущее состояние соответствует этапу разрешения подключения
				case static_cast <uint8_t> (state_t::SUCCESS): {
					// Если данных достаточно для получения ответа
					if((result = (size >= sizeof(response_t)))){
						// Создаём объект данных ответа
						response_t response;
						// Выполняем чтение данных
						::memcpy(&response, buffer, sizeof(response));
						// Если версия прокси-протокола в заголовке пакета соответствует установленной версии прокси-протокола
						if(response.ver == 0x05){
							// Если рукопожатие выполнено
							if(response.rep == static_cast <uint8_t> (status_t::SUCCESS)){
								/**
								 * Определяем тип адреса
								 */
								switch(static_cast <uint8_t> (response.type)){
									// Если тип адреса соответствует FQDN
									case static_cast <uint8_t> (addr_type_t::FQDN): {
										// Если буфер пришел достаточного размера
										if((result = (size >= (sizeof(response_t) + 3)))){
											// Размер доменного имени для подключения
											uint8_t length = 0;
											// Формируем смещение в буфере
											size_t offset = sizeof(response_t);
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
												// Устанавливаем стейт рукопожатия
												ctx.state = state_t::HANDSHAKE;
											}
										}
									} break;
									// Если тип адреса соответствует IPv4
									case static_cast <uint8_t> (addr_type_t::IPV4): {
										// Если буфер пришел достаточного размера
										if((result = (size >= (sizeof(response_t) + sizeof(ip4_t))))){
											// Создаём объект данных сервера
											ip4_t server{};
											// Копируем в буфер наши данные IP адреса
											::memcpy(&server, reinterpret_cast <const uint8_t *> (buffer) + sizeof(response_t), sizeof(server));
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
											// Устанавливаем стейт рукопожатия
											ctx.state = state_t::HANDSHAKE;
										}
									} break;
									// Если тип адреса соответствует IPv6
									case static_cast <uint8_t> (addr_type_t::IPV6): {
										// Если буфер пришел достаточного размера
										if((result = (size >= (sizeof(response_t) + sizeof(ip6_t))))){
											// Создаём объект данных сервера
											ip6_t server{};
											// Копируем в буфер наши данные IP адреса
											::memcpy(&server, reinterpret_cast <const uint8_t *> (buffer) + sizeof(response_t), sizeof(server));
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
											// Устанавливаем стейт рукопожатия
											ctx.state = state_t::HANDSHAKE;
										}
									} break;
								}
							// Если авторизация не пройдена
							} else {
								// Устанавливаем состояние клиента как "сломанный"
								ctx.state = state_t::BROKEN;
								// Устанавливаем код ошибки полученного ответа
								ctx.status = static_cast <proto::socks5_t::status_t> (response.rep);
							}
						// Если версия прокси-сервера не соответствует
						} else {
							// Устанавливаем состояние клиента как "сломанный"
							ctx.state = state_t::BROKEN;
							// Устанавливаем код ошибки "несовпадение версии" в объекте сообщения
							ctx.status = proto::socks5_t::status_t::FORBIDDEN;
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
bool awh::proto::Client_Socks5::parse(const void * buffer, const size_t size, udp_head_t & udp) noexcept {
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
bool awh::proto::Client_Socks5::buffer(uint8_t ** buffer, size_t & size, ctx_t & ctx) const noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Обнуляем размер буфера для отправки данных
		::__awh_size__ = 0;
		/**
		 * Определяем текущее состояние клиента
		 */
		switch(static_cast <uint8_t> (ctx.state)){
			// Если текущее состояние ещё не определено
			case static_cast <uint8_t> (state_t::NONE): {
				// Устанавливаем результат для отправки данных
				result = true;
				// Устанавливаем состояние клиента как "выполнение запроса"
				ctx.state = state_t::REQUEST;
				// Добавляем версию прокси-протокола в буфер для отправки данных
				::addPayload(static_cast <uint8_t> (0x05));
				// Если имя пользователя для авторизации на сервере или пароль пользователя для авторизации на сервере не установлены
				if(this->_username.empty() || this->_password.empty()) {
					// Добавляем количество поддерживаемых методов аутентификации в буфер для отправки данных
					::addPayload(static_cast <uint8_t> (0x01));
					// Добавляем метод аутентификации "без аутентификации" в буфер для отправки данных
					::addPayload(method_t::NOAUTH);
				// Если имя пользователя для авторизации на сервере и пароль пользователя для авторизации на сервере установлены
				} else {
					// Добавляем количество поддерживаемых методов аутентификации в буфер для отправки данных
					::addPayload(static_cast <uint8_t> (0x02));
					// Добавляем метод аутентификации "без аутентификации" в буфер для отправки данных
					::addPayload(method_t::NOAUTH);
					// Добавляем метод аутентификации "по USERNAME/PASSWORD" в буфер для отправки данных
					::addPayload(method_t::PASSWD);
				}
			} break;
			// Если текущее состояние соответствует этапу аутентификации
			case static_cast <uint8_t> (state_t::AUTH): {
				// Если имя и пароль пользователя для авторизации на сервере установлены
				if((result = (!this->_username.empty() && !this->_password.empty() && (this->_username.length() <= 0xFF) && (this->_password.length() <= 0xFF)))){
					// Устанавливаем состояние клиента как "ожидание получения ответа от сервера"
					ctx.state = state_t::RESPONSE;
					// Добавляем версию соглашения авторизации в буфер для отправки данных
					::addPayload(static_cast <uint8_t> (0x01));
					// Добавляем размер имени пользователя для авторизации на сервере в буфер для отправки данных
					::addPayload(static_cast <uint8_t> (this->_username.length()));
					// Устанавливаем имя пользователя для авторизации на сервере в буфер для отправки данных
					::addPayload(this->_username);
					// Добавляем размер пароля пользователя для авторизации на сервере в буфер для отправки данных
					::addPayload(static_cast <uint8_t> (this->_password.length()));
					// Устанавливаем пароль пользователя для авторизации на сервере в буфер для отправки данных
					::addPayload(this->_password);
				}
			} break;
			// Если текущее состояние соответствует ожиданию выполнения подключения
			case static_cast <uint8_t> (state_t::CONNECT): {
				// Если хост для подключения установлен
				if((result = (ctx.host != nullptr))){
					// Устанавливаем состояние клиента как "ожидание разрешения на подключение"
					ctx.state = state_t::SUCCESS;
					// Добавляем версию прокси-протокола в буфер для отправки данных
					::addPayload(static_cast <uint8_t> (0x05));
					// Добавляем код команду запроса к серверу в буфер для отправки данных
					::addPayload(ctx.command);
					// Добавляем зарезервированный октет в буфер для отправки данных
					::addPayload(static_cast <uint8_t> (0x00));
					/**
					 * Определяем тип адреса хоста для подключения
					 */
					switch(static_cast <uint8_t> (ctx.host->type)){
						// Если тип адреса соответствует FQDN
						case static_cast <uint8_t> (net::type_t::FQDN): {
							// Извлекаем доменное имя хоста для подключения
							const string & fqdn = awh_cast <net::attr_fqdn_t *> (ctx.host.get())->domain;
							// Если длина доменного имени превышает допустимый размер
							if(fqdn.length() > 0xFF){
								// Устанавливаем отрицательный результат
								result = false;
								// Выходим из обработки
								break;
							}
							// Устанавливаем результат для отправки данных
							result = true;
							// Добавляем тип адреса "доменные имена" в буфер для отправки данных
							::addPayload(addr_type_t::FQDN);
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
bool awh::proto::Client_Socks5::buffer(uint8_t ** buffer, size_t & size, const udp_head_t & udp) const noexcept {
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
 * @brief Метод установки параметров авторизации
 *
 * @param username имя пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 *
 */
void awh::proto::Client_Socks5::setUser(const string & username, const string & password) noexcept {
	// Устанавливаем имя пользователя для авторизации на сервере
	this->_username = (username.length() <= 0xFF) ? username : username.substr(0, 0xFF);
	// Устанавливаем пароль пользователя для авторизации на сервере
	this->_password = (password.length() <= 0xFF) ? password : password.substr(0, 0xFF);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::proto::Client_Socks5::Client_Socks5(const fmk_t * fmk, const log_t * log) noexcept :
 socks5_t(fmk, log), _username{""}, _password{""} {}
/**
 * @brief Деструктор
 *
 */
awh::proto::Client_Socks5::~Client_Socks5() noexcept {}
