/**
 * @file static.cpp
 * @date 2025-12-12
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
 * @brief Статические тесты модуля логирования — проверка создания и сброса объекта модуля,
 *        а также корректности форматирования сообщений по уровням важности, работы приёмников вывода и ротации файлов
 *
 * @copyright Copyright © 2025
 *
 */
#include "log.hpp"
#include <sys/log.hpp>

/**
 * Подключаем заголовочный файлы проекта
 */

/**
 * @brief Проверка состояния модуля логов, единственного на процесс
 *
 * @details Прежде эти проверки испытывали заведение и сброс ОБЪЕКТА логирования.
 *          Объекта не стало: состояние модуля единственно на процесс, и закреплять
 *          нужно именно это свойство, а не построение
 *
 */
TEST_F(LogFixture, SingleStateLogTest){
	// Запоминаем набор режимов вывода, заведённый по умолчанию
	const size_t initial = awh::log::mode().size();
	// Утверждаем, что состояние заведено само, без построения объекта
	ASSERT_GT(initial, static_cast <size_t> (0))
	 << "состояние модуля логов не заведено по первому обращению";
	// Выполняем установку названия службы
	awh::log::name("SINGLE-STATE");
	// Выполняем установку единственного режима вывода
	awh::log::mode({awh::log::mode_t::CONSOLE});
	// Утверждаем, что установка видна при следующем обращении
	ASSERT_EQ(awh::log::mode().size(), static_cast <size_t> (1))
	 << "состояние модуля логов не сохранилось между обращениями";
	// Утверждаем, что видна именно установленная настройка
	ASSERT_TRUE(awh::log::mode().count(awh::log::mode_t::CONSOLE) > 0)
	 << "сохранился не тот режим вывода, который был установлен";
}

/**
 * @brief Проверка неизменности состояния модуля логов при повторном заведении
 *
 * @details Заведение модуля ленивое и одноразовое: повторное обращение обязано
 *          отдать УЖЕ настроенное состояние, а не завести его заново с умолчаниями
 *
 */
TEST_F(LogFixture, StateSurvivesLogTest){
	// Выполняем установку опознаваемого уровня логирования
	awh::log::level(awh::log::level_t::CRITICAL);
	// Выполняем установку опознаваемого формата даты
	awh::log::format("%Y-%m-%d");
	// Выполняем обращение к иной настройке модуля
	static_cast <void> (awh::log::mode());
	// Утверждаем, что установленный формат уцелел
	ASSERT_EQ(awh::log::format(), std::string{"%Y-%m-%d"})
	 << "обращение к модулю сбросило ранее установленный формат даты";
}

/**
 * @brief Тесты установки режимов логов
 *
 */
TEST_F(LogFixture, ModeLogTest){
	/**
	 * Устанавливаем режимы формирвоания логов
	 */
	awh::log::mode({
		awh::log::mode_t::FILE,
		awh::log::mode_t::SYSLOG,
		awh::log::mode_t::CONSOLE,
		awh::log::mode_t::DEFERRED
	});
	// Проверяем установленные режимы логов
	ASSERT_TRUE(awh::log::mode().size() == 4);
	/**
	 * Проверяем корректность установленных режимов логов
	 */
	for(auto & mode : awh::log::mode())
		// Проверяем корректность режима логов
		ASSERT_TRUE(
			(mode == awh::log::mode_t::FILE) ||
			(mode == awh::log::mode_t::SYSLOG) ||
			(mode == awh::log::mode_t::CONSOLE) ||
			(mode == awh::log::mode_t::DEFERRED)
		);
}

/**
 * @brief Тесты установки формата логов
 *
 */
TEST_F(LogFixture, FormatLogTest){
	// Устанавливаем формат лога
	awh::log::format("%a %h %e %Y %H:%M:%S");
	// Проверяем установленный формат лога
	ASSERT_EQ("%a %h %e %Y %H:%M:%S", awh::log::format());
}

/**
 * @brief Тесты других методов логов
 *
 */
TEST_F(LogFixture, OtherLogTest){
	// Активируем асинхронный режим работы логов
	awh::log::async(true);
	// Деактивируем асинхронный режим работы логов
	awh::log::async(false);
	// Устанавливаем название сервиса для вывода лога
	awh::log::name("anyks");
	// Устанавливаем максимальный размер файла логов
	awh::log::maxSize(4096);
	// Устанавливаем размер текста для формирования разделителя
	awh::log::sepSize(1024);
	// Устанавливаем уровень логирования
	awh::log::level(awh::log::level_t::ALL);
	// Устанавливаем путь к файлу для сохранения логов
	awh::log::filename("/tmp/test.log");
	// Устанавливаем разделитель сообщений логирования
	awh::log::separator(awh::log::separator_t::ALWAYS);
	// Проверяем успешное выполнение методов
	ASSERT_TRUE(true);
}

/**
 * @brief Тест доводов отладочной записи, несущих знаки формата
 *
 * @details Название метода и его доводы прежде вклеивались в строку формата, и довод
 *          со знаком процента (адрес "/tmp/100%d%d%x") разбирался vsnprintf как
 *          переменные: те съедали настоящий довод сообщения, печатали мусор со стека,
 *          а переменная самого сообщения читала произвольный указатель - замерено
 *          щупом: запись выходила с "(null)" вместо адреса. Так же подстановка по
 *          списку подменяла "$1" внутри доводов. Проверка ведётся по всем четырём
 *          видам отладочной записи: узкому и широкому, с доводами «...» и списком
 *
 */
TEST_F(LogFixture, DebugArgumentsAreNotFormatLogTest){
	// Последняя дошедшая до подписчика запись
	std::string text;
	// Счётчик записей, дошедших до подписчика
	uint16_t received = 0;
	// Выводим записи синхронно, чтобы запись дошла до выхода из вызова
	awh::log::async(false);
	// Разрешаем вывод записей в функцию обратного вызова
	awh::log::mode({awh::log::mode_t::DEFERRED});
	// Подписываемся на получение записей
	awh::log::subscribe([&](const awh::log::flag_t, std::string_view record) noexcept -> void {
		// Запоминаем, что запись до подписчика дошла
		received++;
		// Запоминаем текст записи
		text.assign(record);
	});
	// Узкая запись с доводами «...»
	awh::log::debug("Path: \"%s\"", "method%d()", {"/tmp/100%d%d%x"}, awh::log::flag_t::WARNING, "/tmp/message%s");
	// Запись обязана дойти
	ASSERT_EQ(received, static_cast <uint16_t> (1));
	// Название метода обязано выйти дословно
	ASSERT_NE(text.find("method%d()"), std::string::npos) << text;
	// Довод метода обязан выйти дословно
	ASSERT_NE(text.find("(/tmp/100%d%d%x)"), std::string::npos) << text;
	// Сообщение обязано сформироваться по своему формату
	ASSERT_NE(text.find("Path: \"/tmp/message%s\""), std::string::npos) << text;
	// Широкая запись с доводами «...»
	awh::log::debug(L"Path: \"%ls\"", "method()", {"/tmp/100%d%x"}, awh::log::flag_t::WARNING, L"/tmp/wide");
	// Запись обязана дойти
	ASSERT_EQ(received, static_cast <uint16_t> (2));
	// Довод метода обязан выйти дословно
	ASSERT_NE(text.find("(/tmp/100%d%x)"), std::string::npos) << text;
	// Сообщение обязано сформироваться по своему формату
	ASSERT_NE(text.find("Path: \"/tmp/wide\""), std::string::npos) << text;
	// Узкая запись со списком подстановки
	awh::log::debug("Path: $1", "method()", {"/tmp/$1$2"}, awh::log::flag_t::WARNING, std::vector <std::string> {"list"});
	// Запись обязана дойти
	ASSERT_EQ(received, static_cast <uint16_t> (3));
	// Довод метода обязан выйти дословно
	ASSERT_NE(text.find("(/tmp/$1$2)"), std::string::npos) << text;
	// Сообщение обязано сформироваться по своему формату
	ASSERT_NE(text.find("Path: list"), std::string::npos) << text;
	// Широкая запись со списком подстановки
	awh::log::debug(L"Path: $1", "method()", {"/tmp/$1"}, awh::log::flag_t::WARNING, std::vector <std::wstring> {L"wide"});
	// Запись обязана дойти
	ASSERT_EQ(received, static_cast <uint16_t> (4));
	// Довод метода обязан выйти дословно
	ASSERT_NE(text.find("(/tmp/$1)"), std::string::npos) << text;
	// Сообщение обязано сформироваться по своему формату
	ASSERT_NE(text.find("Path: wide"), std::string::npos) << text;
}
