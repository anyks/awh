/**
 * @file journal.hpp
 * @date 2026-09-13
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
 * \~russian
 * @brief Сторож подписки на журнал для проверок кодеков
 *
 * \~english
 * @brief The guard of the subscription to the log for the tests of the codecs
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <utility>
#include <string_view>

/**
 * Подключаем заголовочные файлы фреймворка
 */
#include <sys/log.hpp>

/**
 * @brief Пространство имён безымянное
 *
 * @details Проверки всех семи кодеков собираются в ОДНУ программу, и тип, из заголовка
 *          пришедший, лежал бы в ней в стольких видах, сколько единиц трансляции его
 *          подключили. Безымянное пространство даёт каждой свой, и правило одного
 *          определения держится
 *
 */
namespace {
	/**
	 * @brief Сторож подписки на журнал, единственный на процесс
	 *
	 * @details Подписка `awh::log::subscribe` живёт в состоянии журнала, единственном на
	 *          процесс, и область видимости сборника сообщений ПЕРЕЖИВАЕТ: замыкание
	 *          держит сборник по ссылке, область кончается, подписка остаётся, и первый
	 *          же отчёт следующей проверки пишет в разрушенную память
	 *
	 * @warning Снятие стоит В ДЕСТРУКТОРЕ, а не зовом в конце проверки: `ASSERT_*`
	 *          выходит из проверки возвратом, и хвостовое снятие при КРАСНОЙ проверке
	 *          пропускалось бы - одна краснота плодила бы порчу памяти у соседей,
	 *          отчего искали бы дефект не там, где он есть
	 *
	 * @note Беда доказана щупом под ASan 13.09.2026: `stack-use-after-scope` приходит на
	 *       первом же отчёте, поданном после конца области сборника. Найдено у кодеков
	 *       INI, TOML и YAML
	 *
	 */
	class Journal {
		private:
			// Собираемые сообщения журнала
			std::vector <std::string> _messages;
			// Собираемые сообщения журнала вместе с уровнем важности
			std::vector <std::pair <awh::log::flag_t, std::string>> _records;
		public:
			/**
			 * @brief Метод получения собранных сообщений журнала
			 *
			 * @return собранные сообщения журнала
			 *
			 */
			const std::vector <std::string> & messages() const noexcept {
				// Выводим собранные сообщения журнала
				return this->_messages;
			}
			/**
			 * @brief Метод получения собранных сообщений журнала с уровнем важности
			 *
			 * @return собранные сообщения журнала с уровнем важности
			 *
			 */
			const std::vector <std::pair <awh::log::flag_t, std::string>> & records() const noexcept {
				// Выводим собранные сообщения журнала с уровнем важности
				return this->_records;
			}
			/**
			 * @brief Метод проверки отсутствия собранных сообщений журнала
			 *
			 * @return признак отсутствия собранных сообщений журнала
			 *
			 */
			bool empty() const noexcept {
				// Выводим признак отсутствия собранных сообщений журнала
				return this->_messages.empty();
			}
			/**
			 * @brief Метод получения последнего собранного сообщения журнала
			 *
			 * @return последнее собранное сообщение журнала
			 *
			 */
			const std::string & back() const noexcept {
				// Выводим последнее собранное сообщение журнала
				return this->_messages.back();
			}
			/**
			 * @brief Метод очистки собранных сообщений журнала
			 *
			 */
			void clear() noexcept {
				// Выполняем очистку собранных сообщений журнала
				this->_messages.clear();
				// Выполняем очистку собранных сообщений журнала с уровнем важности
				this->_records.clear();
			}
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			Journal() noexcept {
				// Выполняем назначение приёмника вывода в функцию обратного вызова
				awh::log::mode({awh::log::mode_t::DEFERRED});
				// Выполняем назначение перехвата сообщений журнала
				awh::log::subscribe([this](const awh::log::flag_t flag, std::string_view text) noexcept -> void {
					// Выполняем сбор очередного сообщения журнала
					this->_messages.push_back(std::string(text));
					// Выполняем сбор очередного сообщения журнала с уровнем важности
					this->_records.emplace_back(flag, std::string(text));
				});
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~Journal() noexcept {
				// Выполняем снятие перехвата сообщений журнала
				awh::log::subscribe(nullptr);
			}
	};
}
