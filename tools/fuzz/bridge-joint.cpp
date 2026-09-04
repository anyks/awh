/**
 * @file bridge-joint.cpp
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
 * @brief Инструмент сличения образцов сочленения JSON и XML — разбор пары записей,
 *        одно и то же содержимое несущих, и проверка совпадения деревьев вместе с
 *        замкнутостью кругового перевода
 *
 * @details Образцы лежат парами: запись `<имя>.json` и запись `<имя>.xml` выражают
 *          ОДНО И ТО ЖЕ содержимое двумя разными способами. Поверяются два договора:
 *
 *          1. СОВПАДЕНИЕ ДЕРЕВЬЕВ. Разбор обеих записей даёт одно и то же дерево
 *             значений. Расхождение означает, что правила сочленения у двух дорог
 *             разошлись между собою
 *
 *          2. ЗАМКНУТОСТЬ КРУГА. Дерево, из записи JSON собранное, переведённое в
 *             запись XML и прочтённое обратно, даёт то же самое дерево. Договор этот
 *             ловит потерю ТИХУЮ: перевод отвечает успехом, запись выходит годной, а
 *             содержимое меняется
 *
 * @warning Круг замыкается СО ВТОРОГО прохода у записей, чей корень безымянен:
 *          перечень верхнего уровня получает имя корня настройкою, ибо у документа
 *          разметки ровно один корень и безымянным он быть не может. Расхождение это
 *          законно, и средство докладывает его отдельною строкою
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <codec/bridge.hpp>

/**
 * Устанавливаем пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Средства заведения молчащего журнала работы (внутренняя компоновка)
 *
 */
namespace {
	/**
	 * @brief Объект молчащего журнала работы
	 *
	 */
	struct Silent {
		/**
		 * @brief Функция получения объекта фреймворка
		 *
		 * @return объект фреймворка
		 *
		 */
		static awh::fmk_t & framework() noexcept {
			// Объект фреймворка
			static awh::fmk_t fmk;
			// Выводим объект фреймворка
			return fmk;
		}
		// Объект журнала работы
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		Silent() noexcept : log(&Silent::framework()) {
			// Выполняем отключение вывода журнала
			this->log.mode({});
		}
	};
	/**
	 * @brief Функция получения объекта фреймворка
	 *
	 * @return объект фреймворка
	 *
	 */
	awh::fmk_t * framework() noexcept {
		// Выводим объект фреймворка
		return &Silent::framework();
	}
	/**
	 * @brief Функция получения объекта журнала работы
	 *
	 * @return объект журнала работы
	 *
	 */
	awh::log_t * logger() noexcept {
		// Объект молчащего журнала работы
		static Silent silent;
		// Выводим объект журнала работы
		return &silent.log;
	}
	/**
	 * @brief Функция чтения записи из файла
	 *
	 * @param filename адрес файла записи
	 * @return         прочтённая запись
	 *
	 */
	string load(const std::filesystem::path & filename) noexcept {
		// Открываем файл записи
		std::ifstream file(filename, std::ios::binary);
		// Собираемое содержимое файла
		std::stringstream buffer;
		// Выполняем чтение содержимого файла
		buffer << file.rdbuf();
		// Выводим прочтённое содержимое
		return buffer.str();
	}
}

/**
 * @brief Точка входа в приложение
 *
 * @param argc количество доводов запуска
 * @param argv набор доводов запуска
 * @return     код выхода из приложения
 *
 */
int main(int argc, char * argv[]) noexcept {
	// Если каталог образцов не подан
	if(argc < 2){
		// Выводим подсказку по запуску средства
		::fprintf(stderr, "Использование: %s <каталог образцов>\n", argv[0]);
		// Выходим из приложения с признаком отказа
		return 2;
	}
	// Создаём мост между контейнером ABC и текстовыми кодеками
	codec::bridge_t bridge(::framework(), ::logger());
	// Собираемый перечень образцов сличения
	vector <std::filesystem::path> samples;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем обход каталога образцов
		for(auto & entry : std::filesystem::recursive_directory_iterator(argv[1])){
			// Если очередная запись файлом JSON не является
			if(!entry.is_regular_file() || (entry.path().extension() != ".json"))
				// Продолжаем обход каталога дальше
				continue;
			// Собираем адрес парной записи разметки
			std::filesystem::path pair = entry.path();
			// Выполняем замену расширения адреса
			pair.replace_extension(".xml");
			// Если парная запись разметки существует
			if(std::filesystem::exists(pair))
				// Добавляем образец к перечню сличения
				samples.push_back(entry.path());
		}
	/**
	 * Если обход каталога отвечен отказом
	 */
	} catch(const std::exception & error) {
		// Выводим сообщение об отказе обхода каталога
		::fprintf(stderr, "Каталог образцов обходу не поддался: %s\n", error.what());
		// Выходим из приложения с признаком отказа
		return 2;
	}
	// Выполняем упорядочение образцов по их адресам
	std::sort(samples.begin(), samples.end());
	// Число найденных расхождений
	size_t findings = 0;
	// Число расхождений, именем корня вызванных
	size_t named = 0;
	// Выполняем перебор всех образцов сличения
	for(auto & sample : samples){
		// Собираем адрес парной записи разметки
		std::filesystem::path markup = sample;
		// Выполняем замену расширения адреса
		markup.replace_extension(".xml");
		// Собираемые деревья значений обеих записей
		codec::abc::value_t fromJSON, fromXML;
		// Если запись JSON разбору не поддалась
		if(!bridge.decode(::load(sample), fromJSON, codec::bridge_t::format_t::JSON)){
			// Увеличиваем счёт найденных расхождений
			findings++;
			// Выводим сообщение об отказе разбора записи
			::fprintf(stdout, "%s: разбор JSON отвечен отказом\n", sample.c_str());
			// Продолжаем перебор образцов дальше
			continue;
		}
		// Если запись разметки разбору не поддалась
		if(!bridge.decode(::load(markup), fromXML, codec::bridge_t::format_t::XML)){
			// Увеличиваем счёт найденных расхождений
			findings++;
			// Выводим сообщение об отказе разбора записи
			::fprintf(stdout, "%s: разбор XML отвечен отказом\n", markup.c_str());
			// Продолжаем перебор образцов дальше
			continue;
		}
		// Собираемые записи обоих деревьев
		string first = "", second = "";
		// Выполняем перевод обоих деревьев в запись JSON
		static_cast <void> (bridge.encode(fromJSON, first, codec::bridge_t::format_t::JSON));
		static_cast <void> (bridge.encode(fromXML, second, codec::bridge_t::format_t::JSON));
		// Признак совпадения деревьев обеих записей
		const bool same = (first == second);
		// Собираемая запись кругового перевода
		string circle = "";
		// Собираемое дерево кругового перевода
		codec::abc::value_t back;
		// Признак замкнутости кругового перевода
		bool closed = false;
		/**
		 * Если круговой перевод прошёл обе дороги
		 */
		if(bridge.encode(fromJSON, circle, codec::bridge_t::format_t::XML) &&
		   bridge.decode(circle, back, codec::bridge_t::format_t::XML)){
			// Собираемая запись дерева кругового перевода
			string third = "";
			// Выполняем перевод дерева кругового перевода в запись JSON
			static_cast <void> (bridge.encode(back, third, codec::bridge_t::format_t::JSON));
			// Запоминаем признак замкнутости кругового перевода
			closed = (first == third);
		}
		/**
		 * Если круг разомкнут ЛИШЬ именем корня
		 *
		 * @note Признаком тому служит замкнутость круга у дерева, круг уже прошедшего:
		 *       имя корня даётся однажды, и второй проход обязан быть неподвижен
		 */
		bool naming = false;
		// Если круг разомкнут, а дерево его прошедшее собрано
		if(!closed && back.valid()){
			// Собираемая запись второго круга
			string repeat = "";
			// Собираемое дерево второго круга
			codec::abc::value_t twice;
			// Если второй круг прошёл обе дороги
			if(bridge.encode(back, repeat, codec::bridge_t::format_t::XML) &&
			   bridge.decode(repeat, twice, codec::bridge_t::format_t::XML)){
				// Собираемые записи обоих деревьев второго круга
				string before = "", after = "";
				// Выполняем перевод обоих деревьев в запись JSON
				static_cast <void> (bridge.encode(back, before, codec::bridge_t::format_t::JSON));
				static_cast <void> (bridge.encode(twice, after, codec::bridge_t::format_t::JSON));
				// Запоминаем неподвижность круга со второго прохода
				naming = (before == after);
			}
		}
		// Если оба договора соблюдены
		if(same && closed)
			// Выводим сообщение о совпадении образца
			::fprintf(stdout, "%s: деревья совпали, круг замкнут\n", sample.c_str());
		// Если круг разомкнут лишь именем корня
		else if(same && naming) {
			// Увеличиваем счёт расхождений, именем корня вызванных
			named++;
			// Выводим сообщение о расхождении, именем корня вызванном
			::fprintf(stdout, "%s: деревья совпали, круг замкнут СО ВТОРОГО прохода (корень безымянен)\n", sample.c_str());
		// Если договоры соблюдены не оба
		} else {
			// Увеличиваем счёт найденных расхождений
			findings++;
			// Выводим сообщение о найденном расхождении
			::fprintf(stdout, "%s: деревья %s, круг %s\n", sample.c_str(),
			          (same ? "совпали" : "РАЗОШЛИСЬ"), (closed ? "замкнут" : (naming ? "замкнут со второго прохода" : "РАЗОМКНУТ")));
			/**
			 * Если деревья разошлись, показываем ПЕРВОЕ расхождение
			 *
			 * @note Без него доклад говорит лишь «разошлись», и место расхождения
			 *       приходится искать сличением двух записей вручную
			 */
			if(!same){
				// Потоки чтения обеих записей построчно
				std::stringstream one(first), two(second);
				// Читаемые строки обеих записей
				string left = "", right = "";
				// Номер читаемой строки записей
				size_t line = 0;
				/**
				 * Выполняем чтение обеих записей построчно
				 */
				while(std::getline(one, left)){
					// Увеличиваем номер читаемой строки
					line++;
					// Если строка второй записи прочтена и строки совпадают
					if(std::getline(two, right) && (left == right))
						// Продолжаем чтение записей дальше
						continue;
					// Выводим сообщение о первом расхождении записей
					::fprintf(stdout, "    строка %zu: из JSON «%s», из XML «%s»\n", line, left.c_str(), right.c_str());
					// Выходим из чтения записей
					break;
				}
			}
		}
	}
	// Выводим итог сличения образцов
	::fprintf(stdout, "ОБРАЗЦОВ=%zu РАСХОЖДЕНИЙ=%zu ИМЕНЕМ КОРНЯ=%zu\n", samples.size(), findings, named);
	// Выводим код выхода по наличию расхождений
	return ((findings > 0) ? 1 : 0);
}
