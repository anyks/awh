/**
 * @file: socks5.cpp
 * @date: 2026-05-24
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
 * Подключаем заголовочный файл
 */
#include <proto/socks5/socks5.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод получения сообщения
 *
 * @param code код статуса
 * @return     текстовое значение кода статуса
 */
string awh::proto::Socks5::statusMessage(const status_t code) const noexcept {
	// Переменная результата
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем текстовое значение кода статуса
		 */
		switch(static_cast <uint8_t> (code)){
			// Если код статуса соответствует успешному завершению
			case static_cast <uint8_t> (status_t::SUCCESS):
				// Устанавливаем результат текстового значения кода статуса
				result = "Successful completion";
			break;
			// Если код статуса соответствует ошибке SOCKS-сервера
			case static_cast <uint8_t> (status_t::SOCKSERR):
				// Устанавливаем результат текстового значения кода статуса
				result = "SOCKS server error";
			break;
			// Если код статуса соответствует запрещённому соединению набором правил
			case static_cast <uint8_t> (status_t::FORBIDDEN):
				// Устанавливаем результат текстового значения кода статуса
				result = "Connection forbidden by ruleset";
			break;
			// Если код статуса соответствует недоступности сети
			case static_cast <uint8_t> (status_t::UNAVNET):
				// Устанавливаем результат текстового значения кода статуса
				result = "Network unreachable";
			break;
			// Если код статуса соответствует недоступности хоста
			case static_cast <uint8_t> (status_t::UNAVHOST):
				// Устанавливаем результат текстового значения кода статуса
				result = "Host unreachable";
			break;
			// Если код статуса соответствует отказу в соединении
			case static_cast <uint8_t> (status_t::DENIED):
				// Устанавливаем результат текстового значения кода статуса
				result = "Connection denied";
			break;
			// Если код статуса соответствует истечению TTL
			case static_cast <uint8_t> (status_t::TIMETTL):
				// Устанавливаем результат текстового значения кода статуса
				result = "Connection timed out";
			break;
			// Если код статуса соответствует отсутствию поддерживаемой команды
			case static_cast <uint8_t> (status_t::NOCOMMAND):
				// Устанавливаем результат текстового значения кода статуса
				result = "Command not supported";
			break;
			// Если код статуса соответствует отсутствию поддерживаемого типа адреса
			case static_cast <uint8_t> (status_t::NOADDR):
				// Устанавливаем результат текстового значения кода статуса
				result = "Address type not supported";
			break;
			// Если код статуса соответствует общей ошибке SOCKS-сервера
			case static_cast <uint8_t> (status_t::NOSUPPORT):
				// Устанавливаем результат текстового значения кода статуса
				result = "General SOCKS server failure";
			// Если код статуса не соответствует ни одному из известных кодов, устанавливаем результат текстового значения кода статуса как неизвестная ошибка
			default:
				// Устанавливаем результат текстового значения кода статуса
				result = "Unknown status";
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (code)), log_t::flag_t::CRITICAL, error.what());
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
 */
awh::proto::Socks5::Socks5(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::proto::Socks5::~Socks5() noexcept {}
