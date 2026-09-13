/**
 * @file syslog-document.cpp
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
 * @brief Образец сообщения системного журнала, удерживаемого целиком — обхода дерева по пути,
 *        правки события на лету и оборота записи
 *
 * @copyright Copyright © 2026
 *
 */
#include <codec/syslog/syslog.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <iostream>

/**
 * Подключаем заголовочный файл проекта
 */
#include <sys/fmk.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Разбираемая запись системного журнала
 *
 */
static const char * RECORD =
	"<165>1 2023-04-11T23:29:33.003Z mymachine.example.com evntslog 1093 ID47 "
	"[exampleSDID@32473 iut=\"3\" eventSource=\"Application\" eventID=\"1011\"]"
	"[examplePriority@32473 class=\"high\"] An application event log entry";

/**
 * @brief Функция запуска приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код полученного результата
 *
 */
int32_t main(int32_t argc, char * argv[]) noexcept {
	/**
	 * Выполняем заведение модуля ядра первым делом
	 *
	 * @note Заведение захватывает выдачу памяти процесса и обязано идти
	 *       ДО всякой выдачи и ДО порождения потоков
	 */
	fmk::initialize();
	// Отключаем неиспользуемые переменные
	(void) argc;
	(void) argv;
	// Объект события, удерживаемого целиком
	codec::syslog::document_t document;
	// Если разбор записи отказом завершился
	if(!document.parse(RECORD)){
		// Выводим сообщение об ошибке разбора записи
		cout << "ОТКАЗ разбора: " << codec::syslog::message(document.error()) << endl;
		// Выводим результат работы приложения
		return EXIT_FAILURE;
	}
	// Выводим описание, каким запись прочтена
	cout << "Описание: "
	     << ((document.standard() == codec::syslog::standard_t::RFC5424) ? "RFC 5424" : "RFC 3164") << endl;
	/**
	 * Если приоритет записью объявлен
	 */
	if(document.prioritized()){
		// Выводим приоритет записи с источником и важностью
		cout << "Приоритет: " << document.priority()
		     << " (" << codec::syslog::name(document.facility())
		     << "." << codec::syslog::name(document.severity()) << ")" << endl;
		// Выводим человеческое название источника сообщения
		cout << "Источник: " << document.label() << endl;
	}
	/**
	 * Выводим дату сообщения заданным видом
	 *
	 * @note Дерево держит дату тем видом, каким она стояла в записи, а потребителю
	 *       нужен свой: ход `timestamp` разбирает её и выдаёт заданным видом
	 */
	cout << "Дата: " << document.timestamp("%d.%m.%Y %H:%M:%S") << endl;
	// Выводим текст сообщения
	cout << "Текст: " << document.at("/message").text() << endl;
	/**
	 * Выполняем перебор всех полей заголовка записи
	 *
	 * @note Обход по пути замкнут: имя, ходом `keys` выданное, находится ходом `at`
	 */
	cout << endl << "Поля заголовка:" << endl;
	// Перебираем имена полей заголовка записи
	for(auto & field : document.keys("/header")){
		// Получаем значение очередного поля заголовка
		const codec::abc::value_t & value = document.at("/header/" + field);
		/**
		 * Если поле заголовка знаками записано
		 *
		 * @note Вид значения спрашивается ПРЕЖДЕ извлечения: номер описания записи
		 *       лежит в дереве ЧИСЛОМ, и ход `text` выдал бы у него пустоту - поле
		 *       выглядело бы отсутствующим, каковым оно не является
		 */
		if(value.type() == codec::abc::type_t::STRING)
			// Выводим имя и значение поля заголовка знаками
			cout << "  " << field << " = " << value.text() << endl;
		// Если поле заголовка числом записано
		else {
			// Значение поля заголовка, из дерева извлекаемое
			uint64_t number = 0;
			// Выводим имя и значение поля заголовка числом
			cout << "  " << field << " = " << (value.value(number) ? ::std::to_string(number) : string("?")) << endl;
		}
	}
	/**
	 * Выполняем перебор всех блоков структурированных данных
	 */
	cout << endl << "Структурированные данные (" << document.size() << " блока):" << endl;
	// Перебираем опознаватели блоков структурированных данных
	for(auto & block : document.keys("/structures")){
		// Выводим опознаватель очередного блока структурированных данных
		cout << "  [" << block << "]" << endl;
		// Перебираем имена полей очередного блока структурированных данных
		for(auto & param : document.keys("/structures/" + block))
			// Выводим имя и значение очередного поля блока
			cout << "    " << param << " = " << document.at("/structures/" + block + "/" + param).text() << endl;
	}
	/**
	 * Выполняем правку события на лету
	 *
	 * @note Сброс значения и снос его - РАЗНОЕ: сброс оставляет поле с пустым
	 *       значением, а снос убирает его из записи вовсе. У записи нынешнего описания
	 *       различие это видно - снесённое поле пишется знаком «-»
	 */
	cout << endl << "Правка события:" << endl;
	// Заменяем имя узла записи
	document.set("/header/hostname", codec::abc::value_t(string("relay.example.com")));
	// Добавляем поле в блок структурированных данных
	document.set("/structures/exampleSDID@32473/relayed", codec::abc::value_t(string("yes")));
	// Сносим опознаватель работы из записи
	document.erase("/header/process");
	// Собираем запись из правленого дерева события
	const string built = document.dump();
	// Если сборка записи отказом завершилась
	if(built.empty()){
		// Выводим сообщение об ошибке сборки записи
		cout << "  ОТКАЗ сборки: " << codec::syslog::message(document.error()) << endl;
		// Выводим результат работы приложения
		return EXIT_FAILURE;
	}
	// Выводим собранную запись системного журнала
	cout << "  " << built;
	/**
	 * Выполняем поверку оборота записи
	 *
	 * @note Дословного совпадения оборот НЕ обещает - обещает значение: дата выдаётся
	 *       видом, описанию отвечающим, а порядок блоков берётся порядком дерева.
	 *       Обратимость закрепляется сличением деревьев, а не текстов
	 */
	codec::syslog::document_t again;
	// Если повторный разбор собранной записи отказом завершился
	if(!again.parse(built)){
		// Выводим сообщение об ошибке повторного разбора
		cout << "  ОТКАЗ повторного разбора: " << codec::syslog::message(again.error()) << endl;
		// Выводим результат работы приложения
		return EXIT_FAILURE;
	}
	// Выводим итог поверки оборота записи
	cout << endl << "Оборот: имя узла [" << again.at("/header/hostname").text()
	     << "], опознаватель работы "
	     << (again.has("/header/process") ? "объявлен" : "снесён")
	     << ", поле relayed [" << again.at("/structures/exampleSDID@32473/relayed").text() << "]" << endl;
	// Выводим результат работы приложения
	return EXIT_SUCCESS;
}
