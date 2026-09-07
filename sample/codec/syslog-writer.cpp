/**
 * @file syslog-writer.cpp
 * @date 2026-09-07
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
 * @brief Образец записи событий в сообщение системного журнала — сборки записи из дерева
 *        контейнера ABC обоими описаниями
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/syslog/writer.hpp>
#include <sys/log.hpp>

/**
 * @brief Пространство имён образца
 *
 */
namespace {
	/**
	 * @brief Функция получения объекта фреймворка
	 *
	 * @return объект фреймворка
	 *
	 */
	const awh::fmk_t * framework() noexcept {
		// Объект фреймворка
		static awh::fmk_t fmk;
		// Выводим объект фреймворка
		return &fmk;
	}

	/**
	 * @brief Функция получения объекта для работы с логами
	 *
	 * @return объект для работы с логами
	 *
	 */
	const awh::log_t * logger() noexcept {
		// Объект для работы с логами
		static awh::log_t log(::framework());
		// Выводим объект для работы с логами
		return &log;
	}
}

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция сборки и вывода записи из дерева события
 *
 * @param title название собираемой записи
 * @param value дерево события контейнером ABC
 *
 */
static void build(const char * title, const codec::abc::value_t & value) noexcept {
	// Объект записи событий
	codec::syslog::writer_t writer(::framework(), ::logger());
	// Настройки записи событий
	codec::syslog::writer_t::settings_t settings;
	// Выключаем запись знака конца строки: вывод и без него строку завершает
	settings.terminate = false;
	// Устанавливаем настройки записи событий
	writer.settings(settings);
	// Собранная запись системного журнала
	string result = "";
	// Выводим название собираемой записи
	cout << title << ":" << endl;
	// Если сборка записи отказом завершилась
	if(!writer.write(value, result)){
		// Выводим сообщение об ошибке сборки записи
		cout << "  ОТКАЗ: " << codec::syslog::message(writer.error()) << endl << endl;
		// Выходим из функции
		return;
	}
	// Выводим собранную запись системного журнала
	cout << "  " << result << endl;
	// Выводим описание, каким запись собрана
	cout << "  (описание " << ((writer.standard() == codec::syslog::standard_t::RFC5424) ? "RFC 5424" : "RFC 3164")
	     << ")" << endl << endl;
}

/**
 * @brief Функция запуска приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код полученного результата
 *
 */
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Отключаем неиспользуемые переменные
	(void) argc;
	(void) argv;
	/**
	 * Собираем запись устаревшего описания
	 *
	 * @note Описание записи назначается СОСТАВОМ дерева: RFC 5424 берётся тогда, когда
	 *       дерево несёт то, чего RFC 3164 не знает вовсе - номер описания,
	 *       опознаватель сообщения либо структурированные данные
	 */
	{
		// Дерево собираемого события
		codec::abc::value_t tree(codec::abc::kind_t::MAP);
		// Ставим приоритет записи в дерево события
		tree.place("/priority") = codec::abc::value_t(static_cast <uint64_t> (13));
		// Ставим дату сообщения в дерево события
		tree.place("/header/timestamp") = codec::abc::value_t(string("Oct 22 12:34:56"));
		// Ставим имя узла в дерево события
		tree.place("/header/hostname") = codec::abc::value_t(string("myhostname"));
		// Ставим название приложения в дерево события
		tree.place("/header/application") = codec::abc::value_t(string("myapp"));
		// Ставим опознаватель работы в дерево события
		tree.place("/header/process") = codec::abc::value_t(string("1234"));
		// Ставим текст сообщения в дерево события
		tree.place("/message") = codec::abc::value_t(string("This is a sample syslog message."));
		// Выполняем сборку и вывод записи устаревшего описания
		::build("Запись RFC 3164", tree);
	}
	/**
	 * Собираем запись нынешнего описания со структурированными данными
	 */
	{
		// Дерево собираемого события
		codec::abc::value_t tree(codec::abc::kind_t::MAP);
		/**
		 * Ставим источник и важность сообщения ИМЕНАМИ в дерево события
		 *
		 * @note Имена принимаются наравне с числами: службы журналов пишут правила
		 *       отбора именами, и держать таблицу кодов потребителю незачем - она у
		 *       кодека уже есть. Приоритет собирается из частей лишь за неимением
		 *       целого: объявленный полем `/priority` старше
		 */
		tree.place("/facility") = codec::abc::value_t(string("local4"));
		// Ставим важность сообщения именем в дерево события
		tree.place("/severity") = codec::abc::value_t(string("notice"));
		// Ставим номер описания записи в дерево события
		tree.place("/header/version") = codec::abc::value_t(static_cast <uint64_t> (1));
		// Ставим дату сообщения в дерево события
		tree.place("/header/timestamp") = codec::abc::value_t(string("2023-04-11T23:29:33.003Z"));
		// Ставим имя узла в дерево события
		tree.place("/header/hostname") = codec::abc::value_t(string("mymachine.example.com"));
		// Ставим название приложения в дерево события
		tree.place("/header/application") = codec::abc::value_t(string("evntslog"));
		// Ставим опознаватель сообщения в дерево события
		tree.place("/header/messageId") = codec::abc::value_t(string("ID47"));
		// Ставим поле блока структурированных данных в дерево события
		tree.place("/structures/exampleSDID@32473/iut") = codec::abc::value_t(string("3"));
		// Ставим второе поле блока структурированных данных в дерево события
		tree.place("/structures/exampleSDID@32473/eventSource") = codec::abc::value_t(string("Application"));
		// Ставим текст сообщения в дерево события
		tree.place("/message") = codec::abc::value_t(string("An application event log entry"));
		// Выполняем сборку и вывод записи нынешнего описания
		::build("Запись RFC 5424", tree);
	}
	/**
	 * Собираем запись с полями, деревом не объявленными
	 *
	 * @note Поля заголовка нынешнего описания ПОЗИЦИОННЫ: отсутствующее поле пишется
	 *       знаком «-», а не пропускается - пропуск сдвинул бы все следующие, и запись
	 *       разбиралась бы, но означала иное
	 */
	{
		// Дерево собираемого события
		codec::abc::value_t tree(codec::abc::kind_t::MAP);
		// Ставим приоритет записи в дерево события
		tree.place("/priority") = codec::abc::value_t(static_cast <uint64_t> (165));
		// Ставим номер описания записи в дерево события
		tree.place("/header/version") = codec::abc::value_t(static_cast <uint64_t> (1));
		// Ставим дату сообщения в дерево события
		tree.place("/header/timestamp") = codec::abc::value_t(string("2023-04-11T23:29:33.003Z"));
		// Ставим название приложения в дерево события
		tree.place("/header/application") = codec::abc::value_t(string("MyApp"));
		// Ставим текст сообщения в дерево события
		tree.place("/message") = codec::abc::value_t(string("Message"));
		// Выполняем сборку и вывод записи с отсутствующими полями
		::build("Запись RFC 5424 с отсутствующими полями", tree);
	}
	/**
	 * Собираем запись с отменой знаков в значениях структурированных данных
	 *
	 * @note Отменяются РОВНО ТРИ знака, описанием названные: кавычка, закрывающая
	 *       скобка и сама обратная косая. Ни заголовок, ни текст сообщения отмены не
	 *       знают вовсе
	 */
	{
		// Дерево собираемого события
		codec::abc::value_t tree(codec::abc::kind_t::MAP);
		// Ставим приоритет записи в дерево события
		tree.place("/priority") = codec::abc::value_t(static_cast <uint64_t> (10));
		// Ставим номер описания записи в дерево события
		tree.place("/header/version") = codec::abc::value_t(static_cast <uint64_t> (1));
		// Ставим дату сообщения в дерево события
		tree.place("/header/timestamp") = codec::abc::value_t(string("2023-12-25T15:29:22-07:00"));
		// Ставим имя узла в дерево события
		tree.place("/header/hostname") = codec::abc::value_t(string("srv-demo"));
		// Ставим поле блока с кавычкой в дерево события
		tree.place("/structures/event@23668/quoted") = codec::abc::value_t(string("a\"b"));
		// Ставим поле блока с закрывающей скобкой в дерево события
		tree.place("/structures/event@23668/bracket") = codec::abc::value_t(string("c]d"));
		/**
		 * Ставим текст сообщения, знаками ASCII не исчерпываемый
		 *
		 * @note Метка порядка байтов ставится перед ним сама: описание требует её
		 *       признаком записи в UTF-8. Перед текстом же знаками ASCII она не
		 *       ставится - наращивала бы три октета на всякую запись без нужды
		 */
		tree.place("/message") = codec::abc::value_t(string("Текст события"));
		// Выполняем сборку и вывод записи с отменой знаков
		::build("Запись RFC 5424 с отменой знаков", tree);
	}
	// Выводим результат работы приложения
	return EXIT_SUCCESS;
}
