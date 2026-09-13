/**
 * @file ntp.cpp
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
 * @brief Пример работы с NTP-клиентом —
 *        демонстрация синхронизации времени с пулом NTP-серверов и расчёта смещения относительно локальных часов
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <unit/ntp.hpp>
#include <sys/chrono.hpp>
#include <sys/log.hpp>
#include <sys/fmk.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Главная функция приложения
 *
 * @return код выхода из приложения
 *
 */
int32_t main(){
	/**
	 * Выполняем заведение модуля ядра первым делом
	 *
	 * @note Заведение захватывает выдачу памяти процесса и обязано идти
	 *       ДО всякой выдачи и ДО порождения потоков
	 */
	awh::fmk::initialize();
	// Объект работы с датой и временем
	chrono_t chrono;
	// Создаём объект узла NTP
	unit::ntp_t ntp;
	// Устанавливаем количество попыток резолвинга доменного имени
	// ntp.setAttempts(10);
	// Добавляем NTP-сервер для синхронизации времени (фейковый)
	// ntp.addServer("194.190.168.1");
	// Выполняем инициализацию NTP-клиента
	if(ntp.init(event::family_t::IPV4)){
		// Устанавливаем функцию обратного вызова на событие получения времени от NTP-сервера
		ntp.on <void (const uint64_t)> ("timestamp", [&chrono](const uint64_t timestamp) noexcept -> void {
			// Записываем в лог информацию о полученном времени
			awh::log::print("Получено дата от NTP-сервера: %s", awh::log::flag_t::INFO, chrono.format(timestamp, "%H:%M:%S %d.%m.%Y").c_str());
		}, placeholders::_1);
		// Устанавливаем функцию обратного вызова на событие количества попыток запроса времени к NTP-серверу
		ntp.on <void (const uint8_t)> ("attempts", [&ntp](const uint8_t attempts) noexcept -> void {
			// Переинициализируем клиента
			ntp.init(event::family_t::IPV4);
			// Возвращаем количество попыток запроса времени к NTP-серверу
			awh::log::print("Количество попыток запроса времени к NTP-серверу attempts=%d", awh::log::flag_t::WARNING, attempts);
		}, placeholders::_1);
		// Устанавливаем функцию обратного вызова на событие NTP-клиента
		ntp.on <void (const event::status_t)> ("status", [&ntp](const event::status_t status) noexcept -> void {
			/**
			 * В зависимости от статуса события NTP-клиента выполняем определённые действия
			 */
			switch(static_cast <uint8_t> (status)){
				// Если событие NTP-клиента запущено
				case static_cast <uint8_t> (event::status_t::LAUNCHED): {
					// Записываем в лог сообщение о запуске события NTP-клиента
					awh::log::print("Событие NTP-клиента было запущено", awh::log::flag_t::INFO);
					// Выполняем синхронизацию времени с NTP-сервером
					if(!ntp.sync(unit::ntp_t::version_t::V4))
						// Записываем ошибку в лог
						awh::log::print("Не удалось выполнить синхронизацию времени с NTP-сервером", awh::log::flag_t::CRITICAL);
				} break;
				// Если событие NTP-клиента остановлено
				case static_cast <uint8_t> (event::status_t::DESTROYED):
					// Записываем в лог сообщение об остановке события NTP-клиента
					awh::log::print("Событие NTP-клиента было остановлено", awh::log::flag_t::INFO);
				break;
			}
		}, placeholders::_1);
		// Устанавливаем функцию обратного вызова на событие получения ошибок NTP-клиента
		ntp.on <void (const event::id_t, const event::error_t, const string &)> ("error", [](const event::id_t, const event::error_t error, const string & description) noexcept -> void {
			// Записываем в лог информацию об ошибке
			awh::log::print("NTP error: %s (code: %d)", awh::log::flag_t::CRITICAL, description.c_str(), static_cast <uint16_t> (error));
		}, placeholders::_1, placeholders::_2, placeholders::_3);
		// Запускаем NTP-клиент
		ntp.start();
	// Записываем ошибку в лог
	} else awh::log::print("Не удалось запустить событие NTP-клиента", awh::log::flag_t::CRITICAL);
	// Возвращаем результат
	return EXIT_SUCCESS;
}
