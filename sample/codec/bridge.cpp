/**
 * @file bridge.cpp
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
 * @brief Образец перевода настроек между видами записи
 *
 * \~english
 * @brief Sample of the translation of the settings between the kinds of a record
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
#include <codec/bridge.hpp>
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
 * Используем пространство имён контейнеров данных
 */
using namespace awh::codec;

/**
 * @brief Функция запуска приложения
 *
 * @return код выхода из приложения
 *
 */
int32_t main() noexcept {
	// Создаём объект фреймворка
	const fmk_t fmk;
	// Создаём объект для работы с логами
	const log_t log(&fmk);
	// Создаём мост между контейнером ABC и текстовыми кодеками
	bridge_t bridge(&fmk, &log);
	// Разбираемая запись настроек
	const string text =
		"{\"name\":\"служба\",\"net\":{\"host\":\"localhost\",\"port\":8080},\"list\":[1,2,3]}";
	// Собираемое дерево значений контейнера ABC
	abc::value_t value;
	/**
	 * Выполняем перевод записи JSON в дерево значений
	 *
	 * @details Осью моста служит контейнер ABC: система видов его есть
	 *          надмножество всех текстовых кодеков, оттого перевод в неё не
	 *          теряет ничего, а обратно теряет ровно то, чего в целевом виде
	 *          записи нет вовсе, - и потеря эта видна
	 */
	if(!bridge.decode(text, value, Bridge::format_t::JSON)){
		// Выводим сообщение об отказе перевода
		log.print("Разбор записи JSON отвечен отказом", log_t::flag_t::CRITICAL);
		// Выходим из приложения с кодом отказа
		return EXIT_FAILURE;
	}
	/**
	 * Извлекаемое значение порта
	 *
	 * @warning Спрашивать у числа `text()` нельзя: ход этот отдаёт запись лишь
	 *          у последовательности знаков, а у числа, логического значения и
	 *          пустоты ПУСТ. Это не изъян контейнера, а неверно заданный ему
	 *          вопрос: у числа значение снимается извлечением
	 */
	uint64_t port = 0;
	// Выполняем извлечение значения порта из собранного дерева
	if(value["net"]["port"].value(port))
		// Выводим значение, снятое по пути из собранного дерева
		cout << "Порт: " << port << endl << endl;
	// Перечень видов записи, в которые переводится дерево
	const vector <pair <string, Bridge::format_t>> formats = {
		{"JSON", Bridge::format_t::JSON},
		{"YAML", Bridge::format_t::YAML},
		{"TOML", Bridge::format_t::TOML},
		{"INI",  Bridge::format_t::INI},
		{"XML",  Bridge::format_t::XML}
	};
	// Выполняем перебор всех видов записи
	for(auto & format : formats){
		// Собираемая запись настроек
		string result = "";
		// Выполняем перевод дерева значений в запись кодека
		if(!bridge.encode(value, result, format.second)){
			// Выводим сообщение об отказе перевода
			log.print("Перевод в запись %s отвечен отказом", log_t::flag_t::WARNING, format.first.c_str());
			// Продолжаем перебор видов записи дальше
			continue;
		}
		// Выводим вид записи и собранную запись настроек
		cout << "=== " << format.first << " ===" << endl << result << endl;
	}
	/**
	 * Выполняем настройку обращения с видами, записи кодека неведомыми
	 *
	 * @details Виды BLOB, TIME и UUID контейнера ABC у текстовых кодеков
	 *          отсутствуют, и правило это решает их судьбу: STRICT отвечает
	 *          отказом, TEXT кладёт их последовательностью знаков, SKIP
	 *          пропускает вовсе. Умолчание - TEXT
	 */
	Bridge::settings_t settings = bridge.settings();
	// Устанавливаем строгое обращение с неведомыми видами
	settings.narrow = Bridge::narrow_t::STRICT;
	// Устанавливаем имя корневого узла собираемой записи разметки
	settings.root = "root";
	// Выполняем установку настроек перевода
	bridge.settings(settings);
	/**
	 * Показываем сочленение перечня безымянного с разметкой
	 *
	 * @details Перечень у разметки выражается ПОВТОРОМ одноимённых узлов, и повтор
	 * этот годен лишь при двух звеньях и более, да и то когда перечень лежит полем
	 * отображения. Перечень верхнего уровня и перечень внутри перечня повтора не
	 * дают вовсе - их несёт пометка `array`, обратным чтением снимаемая
	 *
	 * @note Имя корневого узла берётся настройкою: у документа разметки ровно один
	 *       корень, и безымянным он быть не может. Оттого круг замыкается СО ВТОРОГО
	 *       прохода - первый даёт имя
	 */
	{
		// Разбираемая запись перечня безымянного
		const string text = "[[\"год\",\"название\"],[1965,\"Пиксель\"]]";
		// Собираемое дерево значений перечня
		abc::value_t items;
		// Если запись перечня разобрана
		if(bridge.decode(text, items, Bridge::format_t::JSON)){
			// Собираемая запись разметки
			string markup = "";
			// Если дерево перечня переведено в запись разметки
			if(bridge.encode(items, markup, Bridge::format_t::XML)){
				// Выводим собранную запись разметки
				cout << "=== перечень безымянный ===" << endl << text << endl << markup << endl;
				// Собираемое дерево значений обратного хода
				abc::value_t back;
				// Собираемая запись обратного хода
				string again = "";
				// Если круг перевода пройден целиком
				if(bridge.decode(markup, back, Bridge::format_t::XML) && bridge.encode(back, again, Bridge::format_t::JSON))
					// Выводим запись обратного хода
					cout << again << endl;
			}
		}
	}
	// Выводим удачный результат работы приложения
	return EXIT_SUCCESS;
}
