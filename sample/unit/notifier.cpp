/**
 * @file notifier.cpp
 * @date 2026-02-22
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
 * @brief Пример работы с модулем уведомителя —
 *        демонстрация пробуждения цикла событий и доставки пользовательских уведомлений из стороннего потока
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <unit/notifier.hpp>
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
	// Создаём объект узла уведомителя
	unit::notifier_t notifier;
	// Создаём новое событие уведомителя
	event::id_t eid = notifier.create();
	// Устанавливаем функцию обратного вызова на запись в событие
	notifier.on <void (const event::id_t, const size_t)> ("trigger", [](const event::id_t eid, const size_t size) noexcept -> void {
		// Записываем в лог сообщение о записи данных в событие
		awh::log::print("Записано: ID=%u, %zu байт", awh::log::flag_t::INFO, eid, size);
	}, placeholders::_1, placeholders::_2);
	// Устанавливаем функцию обратного вызова на чтение из события
	notifier.on <void (const event::id_t, const uint8_t *, const size_t)> ("notify", [](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
		// Текст входящего сообщения
		const string message(reinterpret_cast <const char *> (data), size);
		// Записываем в лог сообщение о чтении из события
		awh::log::print("Прочитано: ID=%u, %zu байт, сообщение: %s", awh::log::flag_t::INFO, eid, size, message.c_str());
	}, placeholders::_1, placeholders::_2, placeholders::_3);
	// Запускаем дочерний поток для уведомления события
	std::thread([&notifier](const event::id_t eid) noexcept -> void {
		// Задержка перед уведомлением события
		std::this_thread::sleep_for(std::chrono::seconds(3));
		// Текст сообщения
		const string message = "Hello AWH IO Event!";
		// Уведомляем событие
		notifier.trigger(eid, message.c_str(), message.length());
		// Задержка перед уведомлением события
		std::this_thread::sleep_for(std::chrono::seconds(3));
		// Уведомляем событие
		notifier.trigger(eid, message.c_str(), message.length());
	}, eid).detach();
	// Запускаем работу события уведомителя
	notifier.start();
	// Возвращаем результат
	return EXIT_SUCCESS;
}
