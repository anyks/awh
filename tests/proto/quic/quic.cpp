/**
 * @file quic.cpp
 * @date 2026-07-21
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
 * @brief Реализация тестовой фикстуры протокола QUIC —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>

/**
 * Для операционной системы MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Подключаем единую точку подключения системных заголовков MS Windows
	 */
	#include <sys/macro/win32.hpp>
#endif

/**
 * Подключаем заголовочный файлы проекта
 */
#include "quic.hpp"
#include "../../cert.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void QuicFixture::SetUp(){
	// Инициализируем объект разбора сетевых адресов
	this->_addr = std::make_unique <awh::net_addr_t> ();
	// Инициализируем тестовое окружение транспортной безопасности
	this->_security = std::make_unique <QuicSecurity> ();
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void QuicFixture::TearDown(){}

/**
 * @brief Метод формирования опакового представления пути удалённого эндпоинта
 *
 * @param host адрес хоста удалённого эндпоинта
 * @param port порт удалённого эндпоинта
 * @return     опаковое представление пути соединения
 *
 */
std::string QuicFixture::makePath(const std::string & host, const uint16_t port) const noexcept {
	// Разбираем адрес хоста удалённого эндпоинта
	this->_addr->parse(host);
	// Формируем штатную структуру сетевого адреса удалённого эндпоинта
	const std::unique_ptr <awh::net::addr_t> addr = this->_addr->source(awh::net_addr_t::endian_t::LITTLE);
	// Опаковое представление пути соединения
	std::string result = "";
	// Если штатная структура сетевого адреса сформирована
	if(addr != nullptr){
		/**
		 * Определяем разновидность сетевого адреса по его размеру
		 */
		switch(addr->size){
			// Для сетевого адреса IPv4
			case 4: {
				// Извлекаем IPv4-адрес в чистом виде
				const uint32_t ip = static_cast <const awh::net::addr_net_ipv4_t *> (addr.get())->address;
				// Дописываем байты IPv4-адреса в опаковое представление пути
				result.append(reinterpret_cast <const char *> (&ip), sizeof(ip));
			} break;
			// Для сетевого адреса IPv6
			case 16: {
				// Получаем ссылку на IPv6-адрес в чистом виде
				const auto & ip = static_cast <const awh::net::addr_net_ipv6_t *> (addr.get())->address;
				// Дописываем байты IPv6-адреса в опаковое представление пути
				result.append(reinterpret_cast <const char *> (ip.data()), ip.size());
			} break;
		}
		// Дописываем порт эндпоинта в опаковое представление пути
		result.append(reinterpret_cast <const char *> (&port), sizeof(port));
	}
	// Возвращаем опаковое представление пути соединения
	return result;
}

/**
 * @brief Метод преобразования шестнадцатеричной строки в бинарный буфер
 *
 * @param hex шестнадцатеричная строка
 * @return    бинарный буфер
 *
 */
std::string QuicFixture::unhex(const std::string & hex) const noexcept {
	// Результирующий бинарный буфер
	std::string result;
	// Резервируем память под результат
	result.reserve(hex.size() / 2);
	/**
	 * Перебираем шестнадцатеричную строку парами символов
	 */
	for(size_t i = 0; (i + 1) < hex.size(); i += 2)
		// Преобразуем пару символов в октет
		result.push_back(static_cast <char> (std::stoi(hex.substr(i, 2), nullptr, 16)));
	// Выводим бинарный буфер
	return result;
}
/**
 * @brief Метод преобразования бинарного буфера в шестнадцатеричную строку
 *
 * @param data бинарный буфер
 * @return     шестнадцатеричная строка
 *
 */
std::string QuicFixture::hex(const std::string & data) const noexcept {
	// Таблица шестнадцатеричных символов
	static const char SYMBOLS[] = "0123456789abcdef";
	// Результирующая шестнадцатеричная строка
	std::string result;
	// Резервируем память под результат
	result.reserve(data.size() * 2);
	/**
	 * Перебираем бинарный буфер
	 */
	for(size_t i = 0; i < data.size(); i++){
		// Октет бинарного буфера
		const uint8_t byte = static_cast <uint8_t> (data[i]);
		// Дописываем старший полуоктет
		result.push_back(SYMBOLS[byte >> 4]);
		// Дописываем младший полуоктет
		result.push_back(SYMBOLS[byte & 0x0F]);
	}
	// Выводим шестнадцатеричную строку
	return result;
}
/**
 * @brief Метод создания идентификатора соединения из бинарного буфера
 *
 * @param data бинарный буфер идентификатора
 * @return     сформированный идентификатор соединения
 *
 */
awh::quic::cid_t QuicFixture::makeCid(const std::string & data) const noexcept {
	// Результирующий идентификатор соединения
	awh::quic::cid_t result;
	// Устанавливаем длину идентификатора соединения
	result.size = data.size();
	// Если буфер не пустой
	if(!data.empty())
		// Копируем данные идентификатора соединения
		::memcpy(result.data, data.data(), data.size());
	// Выводим идентификатор соединения
	return result;
}
/**
 * @brief Внутренние вспомогательные средства тестового окружения
 *
 */
namespace {
	/**
	 * @brief Функция получения пути к временному каталогу
	 *
	 * @return путь к временному каталогу
	 *
	 * @note У MS Windows путь берётся у самой системы, а не из окружения. Оболочка MSYS2
	 *       выставляет там TEMP и TMP по правилам POSIX (значением "/tmp"), но собранный
	 *       под MinGW двоичный файл - родной для Windows, и такой путь ему непонятен:
	 *       он раскрывается в "C:\\tmp", какого на машине нет вовсе
	 *
	 */
	static std::string directory() noexcept {
		/**
		 * Для операционной системы MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Буфер под путь к временному каталогу
			char buffer[MAX_PATH + 1]{0};
			// Получаем путь к временному каталогу у системы
			const DWORD size = ::GetTempPathA(static_cast <DWORD> (sizeof(buffer)), buffer);
			// Если путь к временному каталогу получен
			if((size > 0) && (size <= MAX_PATH)){
				// Формируем путь к временному каталогу
				std::string result(buffer, static_cast <size_t> (size));
				// Удаляем завершающий разделитель, добавляемый системой
				while(!result.empty() && ((result.back() == '\\') || (result.back() == '/')))
					// Удаляем завершающий разделитель
					result.pop_back();
				// Выводим путь к временному каталогу
				return result;
			}
			// Выводим запасной путь к временному каталогу
			return std::string(".");
		/**
		 * Для всех остальных операционных систем
		 */
		#else
			// Получаем путь к временному каталогу из окружения
			const char * result = ::getenv("TMPDIR");
			// Выводим путь к временному каталогу
			return ((result != nullptr) ? std::string(result) : std::string("/tmp"));
		#endif
	}
	/**
	 * @brief Функция записи данных во временный файл
	 *
	 * @param path путь к создаваемому файлу
	 * @param data записываемые данные
	 * @return     результат записи
	 *
	 */
	static bool store(const std::string & path, const std::string & data) noexcept {
		// Открываем файл на запись
		FILE * file = ::fopen(path.c_str(), "wb");
		// Если файл не открыт
		if(file == nullptr)
			// Выводим отрицательный результат
			return false;
		// Записываем данные в файл
		const bool result = (::fwrite(data.data(), 1, data.size(), file) == data.size());
		// Закрываем файл
		::fclose(file);
		// Выводим результат записи
		return result;
	}
	/**
	 * @brief Функция получения самоподписанного сертификата тестового узла
	 *
	 * @param certificate сертификат в формате PEM
	 * @param privateKey  приватный ключ в формате PEM
	 * @return            результат получения
	 *
	 */
		static bool __awh_make_certificate__(std::string & certificate, std::string & privateKey) noexcept {
		// Устанавливаем самоподписанный сертификат тестового узла
		certificate = std::string(::testCertificate());
		// Устанавливаем приватный ключ тестового узла
		privateKey = std::string(::testPrivateKey());
		// Выводим положительный результат
		return (!certificate.empty() && !privateKey.empty());
	}
};

/**
 * @brief Метод извлечения шаблона контекста безопасности роли эндпоинта
 *
 * @param endpoint роль эндпоинта
 * @return         идентификатор шаблона контекста безопасности
 *
 */
awh::tls::coder::id_t QuicSecurity::context(const awh::quic::endpoint_t endpoint) const noexcept {
	// Выводим шаблон контекста безопасности запрошенной роли
	return ((endpoint == awh::quic::endpoint_t::CLIENT) ? this->_client : this->_server);
}
/**
 * @brief Метод создания отдельного шаблона контекста безопасности
 *
 * @param endpoint  роль эндпоинта
 * @param protocols список поддерживаемых ALPN-протоколов
 * @param validate  режим проверки сертификата удалённого узла
 * @return          идентификатор созданного шаблона контекста безопасности
 *
 */
awh::tls::coder::id_t QuicSecurity::make(const awh::quic::endpoint_t endpoint, const std::vector <awh::tls::coder::alpn_t> & protocols, const bool validate) noexcept {
	// Определяем роль узла кодера
	const awh::event::node_t node = ((endpoint == awh::quic::endpoint_t::CLIENT) ? awh::event::node_t::CLIENT : awh::event::node_t::SERVER);
	// Создаём шаблон контекста безопасности протокола QUIC
	const awh::tls::coder::id_t result = awh::tls::coder::context(node, awh::event::protocol_t::QUIC);
	// Если шаблон контекста безопасности не создан
	if(result == 0)
		// Выводим отрицательный результат
		return result;
	// Устанавливаем список поддерживаемых ALPN-протоколов
	awh::tls::coder::alpn(result, protocols);
	// Если эндпоинт является сервером
	if(endpoint == awh::quic::endpoint_t::SERVER){
		// Устанавливаем сертификат тестового узла
		awh::tls::coder::certificate(result, this->_certificate);
		// Устанавливаем приватный ключ тестового узла
		awh::tls::coder::privateKey(result, this->_privateKey);
		/**
		 * Снимаем проверку на сервере: шаблон контекста создаётся с включённой
		 * проверкой, а на серверном узле это означает требование клиентского
		 * сертификата, то есть взаимную аутентификацию. Тесты проверяют
		 * односторонний TLS, поэтому требование снимается явно
		 */
		awh::tls::coder::validateServerNameIndication(result, false);
	// Если эндпоинт является клиентом
	} else {
		// Устанавливаем доменное имя удалённого узла
		awh::tls::coder::serverNameIndication(result, "localhost");
		// Устанавливаем доверенный центр сертификации тестового узла
		awh::tls::coder::ca(result, this->_certificate);
		// Устанавливаем режим проверки сертификата удалённого узла
		awh::tls::coder::validateServerNameIndication(result, validate);
	}
	// Выводим созданный шаблон контекста безопасности
	return result;
}
/**
 * @brief Конструктор
 *
 */
QuicSecurity::QuicSecurity() noexcept :
 _client(0), _server(0) {
	// Сертификат тестового узла в формате PEM
	std::string certificate = "";
	// Приватный ключ тестового узла в формате PEM
	std::string privateKey = "";
	// Если генерация самоподписанного сертификата не выполнена
	if(!::__awh_make_certificate__(certificate, privateKey))
		// Выходим из конструктора - контексты останутся несозданными
		return;
	// Формируем основу пути к временным файлам
	const std::string base = (::directory() + "/awh-quic-" + std::to_string(reinterpret_cast <uintptr_t> (this)));
	// Устанавливаем путь к файлу сертификата
	this->_certificate = (base + ".crt");
	// Устанавливаем путь к файлу приватного ключа
	this->_privateKey = (base + ".key");
	// Если запись сертификата и приватного ключа во временные файлы не выполнена
	if(!::store(this->_certificate, certificate) || !::store(this->_privateKey, privateKey))
		// Выходим из конструктора - контексты останутся несозданными
		return;
	// Создаём шаблон контекста безопасности клиента
	this->_client = this->make(awh::quic::endpoint_t::CLIENT, {awh::tls::coder::alpn_t{0, "h3"}});
	// Создаём шаблон контекста безопасности сервера
	this->_server = this->make(awh::quic::endpoint_t::SERVER, {awh::tls::coder::alpn_t{0, "h3"}});
}
/**
 * @brief Деструктор
 *
 */
QuicSecurity::~QuicSecurity() noexcept {
	// Если файл сертификата создан
	if(!this->_certificate.empty())
		// Удаляем файл сертификата
		::remove(this->_certificate.c_str());
	// Если файл приватного ключа создан
	if(!this->_privateKey.empty())
		// Удаляем файл приватного ключа
		::remove(this->_privateKey.c_str());
}
