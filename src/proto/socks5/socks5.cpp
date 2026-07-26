/**
 * @file: socks5.cpp
 * @date: 2026-05-24
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация базового класса протокола SOCKS5 (RFC 1928) — общий конечный автомат обмена,
 *        разбор и сборка адресов и UDP-заголовка, обработка кодов команд и статусов
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартный заголовочный файл
 */
#include <climits>

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/socks5/socks5.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем вспомогательные функции в пространство имён
 *
 */
namespace {
	/**
	 * @brief Типы адресации SOCKS5
	 *
	 */
	enum class addr_type_t : uint8_t {
		IPV4 = 0x01,
		FQDN = 0x03,
		IPV6 = 0x04
	};

	/**
	 * @brief Размер SOCKS5-адреса в ответе/запросе CONNECT
	 *
	 * @param data буфер входящих данных
	 * @param size размер буфера входящих данных
	 * @return     0 — кадр неполный; SIZE_MAX — кадр
	 *
	 */
	size_t connectAddrSize(const uint8_t * data, const size_t size) noexcept {
		// Если данных недостаточно для определения типа адреса
		if(size < 4)
			// Возвращаем признак неполного кадра
			return 0;
		// Размер заголовка запроса/ответа CONNECT
		size_t need = 4;
		/**
		 * Определяем тип адреса
		 */
		switch(data[3]){
			// Если тип адреса соответствует IPv4
			case static_cast <uint8_t> (addr_type_t::IPV4):
				// Устанавливаем размер IPv4-адреса с портом
				need = 10;
			break;
			// Если тип адреса соответствует IPv6
			case static_cast <uint8_t> (addr_type_t::IPV6):
				// Устанавливаем размер IPv6-адреса с портом
				need = 22;
			break;
			// Если тип адреса соответствует FQDN
			case static_cast <uint8_t> (addr_type_t::FQDN): {
				// Если данных недостаточно для получения длины доменного имени
				if(size < 5)
					// Возвращаем признак неполного кадра
					return 0;
				// Устанавливаем размер FQDN-адреса с портом
				need = static_cast <size_t> (5) + static_cast <size_t> (data[4]) + 2;
			} break;
			// Если тип адреса не поддерживается
			default:
				// Возвращаем признак некорректного кадра
				return SIZE_MAX;
		}
		// Если данных недостаточно для получения полного адреса
		if(size < need)
			// Возвращаем признак неполного кадра
			return 0;
		// Возвращаем полный размер кадра
		return need;
	}
};

/**
 * @brief Конструктор
 *
 */
awh::proto::Socks5::UDP_Header::UDP_Header() noexcept :
 frag(0x00), size(0), host(nullptr) {}

/**
 * @brief Конструктор
 *
 */
awh::proto::Socks5::Context::Context() noexcept :
 state(state_t::NONE),
 status(status_t::NOSTATUS),
 command(command_t::NONE),
 host(nullptr) {}

/**
 * @brief Метод получения сообщения
 *
 * @param code код статуса
 * @return     текстовое значение кода статуса
 *
 */
string awh::proto::Socks5::statusMessage(const status_t code) noexcept {
	/**
	 * Определяем текстовое значение кода статуса
	 */
	switch(static_cast <uint8_t> (code)){
		// Если код статуса соответствует успешному завершению
		case static_cast <uint8_t> (status_t::SUCCESS):
			// Устанавливаем результат текстового значения кода статуса
			return "Successful completion";
		break;
		// Если код статуса соответствует ошибке SOCKS-сервера
		case static_cast <uint8_t> (status_t::SOCKSERR):
			// Устанавливаем результат текстового значения кода статуса
			return "SOCKS server error";
		break;
		// Если код статуса соответствует запрещённому соединению набором правил
		case static_cast <uint8_t> (status_t::FORBIDDEN):
			// Устанавливаем результат текстового значения кода статуса
			return "Connection forbidden by ruleset";
		break;
		// Если код статуса соответствует недоступности сети
		case static_cast <uint8_t> (status_t::UNAVNET):
			// Устанавливаем результат текстового значения кода статуса
			return "Network unreachable";
		break;
		// Если код статуса соответствует недоступности хоста
		case static_cast <uint8_t> (status_t::UNAVHOST):
			// Устанавливаем результат текстового значения кода статуса
			return "Host unreachable";
		break;
		// Если код статуса соответствует отказу в соединении
		case static_cast <uint8_t> (status_t::DENIED):
			// Устанавливаем результат текстового значения кода статуса
			return "Connection denied";
		break;
		// Если код статуса соответствует истечению TTL
		case static_cast <uint8_t> (status_t::TIMETTL):
			// Устанавливаем результат текстового значения кода статуса
			return "Connection timed out";
		break;
		// Если код статуса соответствует отсутствию поддерживаемой команды
		case static_cast <uint8_t> (status_t::NOCOMMAND):
			// Устанавливаем результат текстового значения кода статуса
			return "Command not supported";
		break;
		// Если код статуса соответствует отсутствию поддерживаемого типа адреса
		case static_cast <uint8_t> (status_t::NOADDR):
			// Устанавливаем результат текстового значения кода статуса
			return "Address type not supported";
		break;
		// Если код статуса соответствует общей ошибке SOCKS-сервера
		case static_cast <uint8_t> (status_t::NOSUPPORT):
			// Устанавливаем результат текстового значения кода статуса
			return "General SOCKS server failure";
		// Если код статуса не соответствует ни одному из известных кодов, устанавливаем результат текстового значения кода статуса как неизвестная ошибка
		default:
			// Устанавливаем результат текстового значения кода статуса
			return "Unknown status";
	}
}
/**
 * @brief Метод определения полного размера SOCKS5-кадра
 *
 * @param state текущее состояние протокола
 * @param data  буфер входящих данных
 * @param size  размер буфера входящих данных
 * @return      0 — кадр неполный; SIZE_MAX — кадр некорректный; иначе размер кадра
 *
 */
size_t awh::proto::Socks5::frameSize(const socks5_t::state_t state, const uint8_t * data, const size_t size) noexcept {
	// Если буфер входящих данных не передан
	if((data == nullptr) || (size == 0))
		// Возвращаем признак неполного кадра
		return 0;
	/**
	 * Определяем текущее состояние протокола
	 */
	switch(static_cast <uint8_t> (state)){
		// Если ожидается приветствие клиента
		case static_cast <uint8_t> (socks5_t::state_t::NONE): {
			// Если данных недостаточно для получения количества методов
			if(size < 2)
				// Возвращаем признак неполного кадра
				return 0;
			// Получаем количество методов авторизации
			const uint8_t count = data[1];
			// Размер приветствия клиента
			const size_t need = static_cast <size_t> (2) + static_cast <size_t> (count);
			// Если данных недостаточно для получения списка методов
			if(size < need)
				// Возвращаем признак неполного кадра
				return 0;
			// Возвращаем полный размер кадра
			return need;
		}
		// Если ожидается пакет авторизации USER/PASS
		case static_cast <uint8_t> (socks5_t::state_t::AUTH): {
			// Если данных недостаточно для получения версии и длины логина
			if(size < 2)
				// Возвращаем признак неполного кадра
				return 0;
			// Если версия соглашения авторизации не соответствует RFC 1929
			if(data[0] != 0x01)
				// Возвращаем признак некорректного кадра
				return SIZE_MAX;
			// Получаем длину логина пользователя
			const uint8_t ulen = data[1];
			// Если данных недостаточно для получения длины пароля
			if(size < (static_cast <size_t> (3) + static_cast <size_t> (ulen)))
				// Возвращаем признак неполного кадра
				return 0;
			// Получаем длину пароля пользователя
			const uint8_t plen = data[static_cast <size_t> (2) + static_cast <size_t> (ulen)];
			// Размер пакета авторизации
			const size_t need = static_cast <size_t> (3) + static_cast <size_t> (ulen) + static_cast <size_t> (plen);
			// Если данных недостаточно для получения пароля
			if(size < need)
				// Возвращаем признак неполного кадра
				return 0;
			// Возвращаем полный размер кадра
			return need;
		}
		// Если ожидается запрос CONNECT/UDP ASSOCIATE
		case static_cast <uint8_t> (socks5_t::state_t::CONNECT):
			// Возвращаем размер запроса CONNECT
			return ::connectAddrSize(data, size);
		// Если ожидается ответ выбора метода
		case static_cast <uint8_t> (socks5_t::state_t::REQUEST):
			// Возвращаем размер ответа выбора метода
			return (size >= 2) ? 2 : 0;
		// Если ожидается ответ авторизации
		case static_cast <uint8_t> (socks5_t::state_t::RESPONSE):
			// Возвращаем размер ответа авторизации
			return (size >= 2) ? 2 : 0;
		// Если ожидается ответ CONNECT/UDP ASSOCIATE
		case static_cast <uint8_t> (socks5_t::state_t::SUCCESS):
			// Возвращаем размер ответа CONNECT
			return ::connectAddrSize(data, size);
	}
	// Возвращаем признак неполного кадра
	return 0;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::proto::Socks5::Socks5(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::proto::Socks5::~Socks5() noexcept {}
