/**
 * @file yaml-reader.cpp
 * @date 2026-08-23
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
 * @brief Пример потокового чтения текста YAML — подача текста кусками произвольного
 *        размера, разбор событий потока, выдача примечаний и пустых строк отдельными
 *        событиями, метки, ссылки и метки типов
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные модули
 */
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/yaml/reader.hpp>
#include <sys/log.hpp>

/**
 * @brief Пространство имён образца
 *
 */
namespace {
	/**
	 * @brief Функция получения объекта для работы с логами
	 *
	 * @details Кодек связку берёт конструктором, а построения образца стоят и вне
	 *          main(): объект заводится статикою местною, дабы всякое построение
	 *          образца писало сообщения в один и тот же журнал
	 *
	 * @return объект для работы с логами
	 *
	 */
	const awh::log_t * logger() noexcept {
		// Объект фреймворка
		static awh::fmk_t fmk;
		// Объект для работы с логами
		static awh::log_t log(&fmk);
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
 * @brief Разбираемый текст документа
 *
 */
static const char * TEXT =
 "%YAML 1.2\n"
 "---\n"
 "# настройки службы\n"
 "title: пример\n"
 "\n"
 "server: &основной\n"
 "  host: локальный # хозяин\n"
 "  port: !!int 8080\n"
 "  hosts: [первый, второй]\n"
 "  greeting: >-\n"
 "    строка, свёрнутая\n"
 "    в одну\n"
 "\n"
 "запасной: *основной\n"
 "...\n";

/**
 * @brief Функция запуска приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Блокируем неиспользуемую переменную
	(void) argc;
	// Блокируем неиспользуемую переменную
	(void) argv;
	// Настройки разбора текста документа
	codec::yaml::reader_t::settings_t settings;
	// Просим выдачи примечаний отдельными событиями
	settings.emitComments = true;
	// Просим выдачи пустых строк отдельными событиями
	settings.emitBlanks = true;
	// Создаём объект потокового чтения текста документа
	codec::yaml::reader_t reader(::logger(), settings);
	// Разбираемый текст документа
	const string text = TEXT;
	/**
	 * Выполняем подачу текста документа кусками
	 *
	 * @note Размер куска взят нарочно малым и на границы записей не смотрит: разбор
	 *       от нарезки не зависит, и события выдаются те же, что при подаче целиком
	 */
	for(size_t offset = 0; offset < text.size();){
		// Размер подаваемого куска текста документа
		const size_t size = ((text.size() - offset) < 7 ? (text.size() - offset) : 7);
		// Признак того, что подаётся последний кусок текста документа
		const bool end = ((offset + size) >= text.size());
		/**
		 * Если подача очередного куска текста документа не удалась
		 */
		if(!reader.feed(text.data() + offset, size, end))
			// Выходим из цикла подачи текста документа
			break;
		// Выполняем смещение начала очередного куска подачи
		offset += size;
		/**
		 * Выполняем чтение всех выданных разбором событий
		 */
		while(reader.next()){
			/**
			 * Выводим место выданного события в исходном тексте
			 *
			 * @note Место события берётся у содержимого его, а location() чтения выдаёт
			 *       место отказа разбора - это разные вещи
			 */
			cout << reader.value().location.line << ":" << reader.value().location.column << " ";
			// Выводим название выданного события
			cout << codec::yaml::name(reader.event());
			/**
			 * Если событие несёт содержимое
			 */
			if(!reader.value().text.empty())
				// Выводим содержимое выданного события
				cout << " «" << reader.value().text << "»";
			/**
			 * Если событие несёт метку
			 */
			if(!reader.value().anchor.empty())
				// Выводим метку выданного события
				cout << " &" << reader.value().anchor;
			/**
			 * Если событие несёт метку типа
			 */
			if(!reader.value().tag.empty())
				// Выводим метку типа выданного события
				cout << " <" << reader.value().tag << ">";
			// Выводим перевод строки
			cout << endl;
		}
	}
	/**
	 * Если разбор текста документа завершился отказом
	 */
	if(reader.error() != codec::yaml::error_t::NONE)
		// Выводим сведения об обнаруженной ошибке разбора
		cout << "ошибка: " << codec::yaml::message(reader.error())
		     << " в " << reader.location().line << ":" << reader.location().column << endl;
	/**
	 * Выводим опознанную кодировку исходного текста
	 *
	 * @note Кодировка опознаётся по метке порядка байтов, а при отсутствии её - по
	 *       расположению нулевых байтов в первых четырёх октетах
	 */
	else cout << endl << "кодировка: " << static_cast <uint16_t> (reader.encoding()) << endl;
	// Выводим код успешного выхода из приложения
	return EXIT_SUCCESS;
}
