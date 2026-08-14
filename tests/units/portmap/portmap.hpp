/**
 * @file portmap.hpp
 * @date 2026-08-03
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
 * @brief Заголовочный файл тестовой фикстуры модуля перенаправления портов —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_UNIT_PORTMAP_TESTS__
#define __AWH_UNIT_PORTMAP_TESTS__

/**
 * Подключаем заголовочные файлы проекта
 */
#include <vector>

#include "../../main.hpp"
#include "../../../include/units/portmap.hpp"

/**
 * Подключаем поддельное устройство доступа в сеть
 *
 * @note Подключается оно не ради самого устройства, а ради названия устройства петли
 *       (`LOOPBACK_IFACE`): имя это разнится по системам и нужно всем проверкам
 *       обращения к маршрутизатору, а не одним лишь тем, что поднимают подделку
 *
 */
#include "igd.hpp"

/**
 * @brief Класс фикстуры для тестов модуля перенаправления портов
 *
 * @details Модуль ведёт обмен циклом базы событий, и цикл этот один на процесс:
 * запускает его тот, кто первым позвал `start()`, а останавливает он же. Оттого
 * каждый тест обязан цикл остановить - иначе следующий в нём и застрянет
 *
 * @note Ответа маршрутизатора тесты не ждут: проверяется поведение модуля тогда,
 * когда ответа нет вовсе, - счёт попыток и повторная отправка. Обмен потому и
 * направляется туда, откуда ответ прийти не может
 *
 */
class PortmapUnitFixture : public testing::Test {
	protected:
		// Объект фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
	public:
		/**
		 * @brief Метод настройки тестового окружения
		 *
		 */
		void SetUp();
		/**
		 * @brief Метод очистки тестового окружения
		 *
		 */
		void TearDown();
	protected:
		/**
		 * @brief Структура итога обращения к маршрутизатору
		 *
		 */
		typedef struct Outcome {
			// Признак того, что обращение завершилось отказом
			bool failed;
			// Признак того, что маршрутизатор ответил
			bool answered;
			// Код причины отказа перенаправления
			awh::unit::portmap_t::error_t error;
			// Договор, по которому получен итог
			awh::unit::portmap_t::type_t type;
			// Время, потраченное на обращение, в миллисекундах
			int64_t spent;
			/**
			 * Перенаправление, выданное маршрутизатором
			 *
			 * @note Держится итогом, а не отдельной подпиской испытания: подписка ведётся
			 * по имени, и заведённая испытанием заместила бы подписку ожидания - итога
			 * тогда не пришло бы вовсе
			 */
			awh::unit::portmap_t::mapping_t mapping;
			// Перечень заведённых перенаправлений, прочитанный у маршрутизатора
			std::vector <awh::unit::portmap_t::mapping_t> mappings;
			/**
			 * @brief Конструктор
			 *
			 */
			Outcome() noexcept :
			 failed(false), answered(false),
			 error(awh::unit::portmap_t::error_t::NONE),
			 type(awh::unit::portmap_t::type_t::NONE), spent(0) {}
		} outcome_t;
	protected:
		/**
		 * @brief Метод ожидания итога обращения к маршрутизатору
		 *
		 * @details Подписывается на итоги, запускает цикл базы событий и останавливает
		 * его, как только итог получен
		 *
		 * @param portmap объект модуля перенаправления портов
		 * @param action  просьба, с которой ведётся обращение
		 * @return        итог обращения к маршрутизатору
		 *
		 */
		outcome_t await(awh::unit::portmap_t & portmap, const awh::unit::portmap_t::action_t action) const noexcept;
};

#endif // __AWH_UNIT_PORTMAP_TESTS__
