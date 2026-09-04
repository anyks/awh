/**
 * @file args.cpp
 * @date 2026-09-04
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
 * @brief Образец работы с параметрами запуска приложения
 *
 * \~english
 * @brief Sample of the work with the launch parameters of an application
 *
 * \~
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
#include <args/args.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён фреймворка
 */
using namespace awh;

/**
 * Используем пространство имён параметров запуска
 */
using namespace awh::args;

/**
 * @brief Функция запуска приложения
 *
 * @param argc количество передаваемых аргументов
 * @param argv буфер параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект для работы с логами
	const log_t log(&fmk);
	// Создаём объект сбора параметров запуска приложения
	args_t args(&fmk, &log);
	/**
	 * Выполняем описание ожидаемых параметров запуска
	 *
	 * @details Описание это не обязательно: без него принимается всякий
	 *          параметр, а вид значения выводится из самой записи. Заведено оно
	 *          ради двух вещей - подсказки по запуску и проверки обязательных
	 */
	static_cast <void> (args.schema().add("config", 'c', Schema::value_t::REQUIRED, "путь к файлу настроек"));
	static_cast <void> (args.schema().add("port", 'p', Schema::value_t::REQUIRED, "порт, на котором поднимается служба"));
	static_cast <void> (args.schema().add("verbose", 'v', Schema::value_t::NONE, "подробный вывод в журнал"));
	/**
	 * Выполняем разбор доводов запуска приложения
	 *
	 * @details Записи `--name=значение` и `-name значение` признаются обе, и
	 *          положение их произвольно
	 */
	if(!args.parse(argc, const_cast <const char **> (argv))){
		// Выполняем перебор всех отказов разбора
		for(auto & error : args.errors())
			// Выводим сообщение об отказе разбора
			log.print("%s", log_t::flag_t::WARNING, message(error.first));
		// Выводим подсказку по запуску приложения
		cout << args.usage() << endl;
		// Выходим из приложения с кодом отказа
		return EXIT_FAILURE;
	}
	/**
	 * Выполняем чтение настроек из окружения
	 *
	 * @details Приставка отделяет наши переменные от чужих: при приставке
	 *          «AWH» переменная `AWH_PORT` ложится параметром `port`. Старшинство
	 *          источников таково, что довод запуска окружение перекрывает, а не
	 *          наоборот, - и порядок подачи на это не влияет
	 */
	args.prefix("AWH");
	// Выполняем чтение настроек из переменных окружения
	static_cast <void> (args.env());
	// Если задан файл настроек
	if(args.has("config")){
		/**
		 * Выполняем чтение файла настроек
		 *
		 * @note Вид записи выводится расширением имени: `.json`, `.yaml`, `.toml`,
		 *       `.ini` либо `.xml`. Знай потребитель вид заранее - задал бы его
		 *       вторым доводом и имя файла ему было бы не указ
		 */
		if(!args.filename(args.get <string> ("config")))
			// Выводим сообщение об отказе чтения файла настроек
			log.print("Файл настроек прочитать не удалось", log_t::flag_t::WARNING);
	}
	// Выполняем проверку обязательных параметров запуска
	if(!args.verify()){
		// Выводим подсказку по запуску приложения
		cout << args.usage() << endl;
		// Выходим из приложения с кодом отказа
		return EXIT_FAILURE;
	}
	// Выводим порт, на котором поднимается служба
	cout << "Порт: " << args.get <uint16_t> ("port") << endl;
	// Выводим признак подробного вывода в журнал
	cout << "Подробный вывод: " << (args.has("verbose") ? "да" : "нет") << endl;
	// Собираемая запись настроек
	string text = "";
	/**
	 * Выполняем выдачу собранных настроек записью YAML
	 *
	 * @details Вид записи здесь произволен: те же настройки выдаются любым из
	 *          пяти видов - JSON, YAML, XML, TOML, INI, - и принимаются обратно
	 */
	if(args.dump(text, codec::Bridge::format_t::YAML))
		// Выводим собранную запись настроек
		cout << endl << text << endl;
	// Выводим удачный результат работы приложения
	return EXIT_SUCCESS;
}
