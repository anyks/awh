/**
 * @file dump.cpp
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
 * @brief Щуп сличения разбора TOML с эталоном
 *
 * @details Щуп читает текст настроек и выдаёт дерево записью JSON того вида, какого
 *          требует набор сверки toml-test: всякое скалярное значение выдаётся парою
 *          вида `{"type": "integer", "value": "42"}`, а таблицы и перечни - записью
 *          JSON как есть. Годный текст выдаётся с кодом 0, негодный - с кодом 1
 *
 * @note Запись JSON ведётся своею рукою, а не кодеком JSON: щуп обязан мерить один
 *       лишь кодек TOML, и отказ кодека соседнего выглядел бы здесь отказом сличения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы модуля
 */
#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdlib>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/toml/document.hpp>
#include <codec/toml/value.hpp>
#include <sys/log.hpp>

/**
 * Подключаем пространства имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Пространство имён щупа
 *
 * @note Держится оно безымянным намеренно: посредники эти нужны одному лишь файлу
 *
 */
namespace {
	/**
	 * @brief Объект журнала щупа с отключённым выводом
	 *
	 * @details Вывод отключается назначением пустого перечня приёмников: щуп подаёт
	 *          заведомо негодные тексты, и журнал засорял бы ими выдачу сличения
	 *
	 */
	struct Silent {
		/**
		 * @brief Функция получения объекта фреймворка щупа
		 *
		 * @details Объект заводится статикою местною, а не общею файла: заведение его
		 *          порядком построения статики оканчивается падением ещё до входа в
		 *          работу, ибо фреймворк сам опирается на статику из библиотеки
		 *
		 * @return объект фреймворка щупа
		 *
		 */
		static const awh::fmk_t & framework() noexcept {
			// Объект фреймворка щупа
			static awh::fmk_t fmk;
			// Выводим объект фреймворка щупа
			return fmk;
		}
		// Объект журнала щупа
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		Silent() noexcept : log(&Silent::framework()) {
			// Выполняем отключение вывода логов
			this->log.mode({});
		}
	};
	/**
	 * @brief Функция получения объекта журнала щупа
	 *
	 * @return объект журнала щупа
	 *
	 */
	const awh::log_t * logger() noexcept {
		// Объект журнала щупа
		static Silent silent;
		// Выводим объект журнала щупа
		return &silent.log;
	}
	/**
	 * @brief Функция ограждения последовательности знаков для записи JSON
	 *
	 * @param text ограждаемая последовательность знаков
	 * @return     ограждённая последовательность знаков
	 *
	 */
	string escaped(const string_view text) noexcept {
		// Результат ограждения
		string result("\"");
		/**
		 * Выполняем перебор всех знаков последовательности
		 */
		for(const char letter : text){
			/**
			 * Определяем ограждаемый знак
			 */
			switch(letter){
				// Если знаком является кавычка
				case '"': result.append("\\\""); break;
				// Если знаком является обратная косая черта
				case '\\': result.append("\\\\"); break;
				// Если знаком является перевод строки
				case '\n': result.append("\\n"); break;
				// Если знаком является возврат каретки
				case '\r': result.append("\\r"); break;
				// Если знаком является горизонтальная подача
				case '\t': result.append("\\t"); break;
				// Если знаком является подача страницы
				case '\f': result.append("\\f"); break;
				// Если знаком является возврат на знак
				case '\b': result.append("\\b"); break;
				/**
				 * Если знаком является всякий иной
				 */
				default: {
					/**
					 * Если знак является управляющим
					 */
					if(static_cast <uint8_t> (letter) < 0x20){
						// Хранилище записи управляющего знака
						char buffer[8];
						// Выполняем запись управляющего знака
						::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast <uint32_t> (static_cast <uint8_t> (letter)));
						// Выполняем дозапись ограждённого знака
						result.append(buffer);
					// Выполняем дозапись знака как есть
					} else result.push_back(letter);
				}
			}
		}
		// Выполняем закрытие записи
		result.push_back('"');
		// Выводим результат ограждения
		return result;
	}
	/**
	 * @brief Функция получения названия вида значения по договору набора сверки
	 *
	 * @param type вид значения дерева настроек
	 * @return     название вида значения по договору набора сверки
	 *
	 */
	const char * named(const codec::toml::type_t type) noexcept {
		/**
		 * Определяем вид значения дерева настроек
		 */
		switch(static_cast <uint8_t> (type)){
			// Если значение является строковым
			case static_cast <uint8_t> (codec::toml::type_t::STRING): return "string";
			// Если значение является целым числом
			case static_cast <uint8_t> (codec::toml::type_t::INTEGER): return "integer";
			// Если значение является числом с плавающей точкой
			case static_cast <uint8_t> (codec::toml::type_t::FLOAT): return "float";
			// Если значение является логическим
			case static_cast <uint8_t> (codec::toml::type_t::BOOLEAN): return "bool";
			// Если значение является отметкою времени со смещением
			case static_cast <uint8_t> (codec::toml::type_t::OFFSET_DATETIME): return "datetime";
			// Если значение является местной отметкою времени
			case static_cast <uint8_t> (codec::toml::type_t::LOCAL_DATETIME): return "datetime-local";
			// Если значение является местной датой
			case static_cast <uint8_t> (codec::toml::type_t::LOCAL_DATE): return "date-local";
			// Если значение является местным временем
			case static_cast <uint8_t> (codec::toml::type_t::LOCAL_TIME): return "time-local";
		}
		// Выводим отсутствие названия вида значения
		return "";
	}
	/**
	 * @brief Функция выдачи содержимого скалярного значения записью
	 *
	 * @details Отметки времени собираются из полей своих, а не перезаписью значения:
	 * перезапись одиночного значения договором TOML не положена вовсе - запись требует
	 * имени пары, - и выдаёт пустоту. Набор сверки притом ждёт записи приведённой:
	 * разделитель даты и времени заглавною «T», пояс UTC заглавною «Z»
	 *
	 * @param value выдаваемое значение дерева настроек
	 * @return      содержимое значения записью
	 *
	 */
	string content(const codec::toml::value_t & value) noexcept {
		// Результат выдачи содержимого
		string result;
		// Хранилище записи содержимого
		char buffer[64];
		/**
		 * Определяем вид выдаваемого значения
		 */
		switch(static_cast <uint8_t> (value.type())){
			/**
			 * Если значение является строковым
			 */
			case static_cast <uint8_t> (codec::toml::type_t::STRING): {
				// Выполняем получение содержимого значения
				value.value(result);
				// Выводим содержимое значения
				return result;
			}
			/**
			 * Если значение является логическим
			 */
			case static_cast <uint8_t> (codec::toml::type_t::BOOLEAN): {
				// Логическое содержимое значения
				bool boolean = false;
				// Выполняем получение содержимого значения
				value.value(boolean);
				// Выводим содержимое значения
				return (boolean ? "true" : "false");
			}
			/**
			 * Если значение является целым числом
			 */
			case static_cast <uint8_t> (codec::toml::type_t::INTEGER): {
				// Целочисленное содержимое значения
				int64_t number = 0;
				// Выполняем получение содержимого значения
				value.value(number);
				/**
				 * Выполняем запись содержимого значения
				 *
				 * @note Запись ведётся десятичною при всякой системе счисления исходной:
				 *       набор сверки ждёт числа, а не записи его
				 */
				::snprintf(buffer, sizeof(buffer), "%lld", static_cast <long long> (number));
				// Выводим содержимое значения
				return buffer;
			}
			/**
			 * Если значение является числом с плавающей точкой
			 */
			case static_cast <uint8_t> (codec::toml::type_t::FLOAT): {
				// Дробное содержимое значения
				double number = 0.0;
				// Выполняем получение содержимого значения
				value.value(number);
				/**
				 * Выполняем запись содержимого значения
				 *
				 * @note Знаков берётся семнадцать: столько нужно и довольно, чтобы число
				 *       двойной точности прочлось обратно тем же самым. Сличение ведётся
				 *       числом, а не записью, оттого вид записи здесь не значим
				 */
				::snprintf(buffer, sizeof(buffer), "%.17g", number);
				// Выводим содержимое значения
				return buffer;
			}
		}
		// Отметка времени выдаваемого значения
		const codec::toml::stamp_t & stamp = value.stamp();
		// Признак записи даты отметки времени
		const bool dated = (value.is(codec::toml::type_t::LOCAL_DATE) ||
		 value.is(codec::toml::type_t::LOCAL_DATETIME) || value.is(codec::toml::type_t::OFFSET_DATETIME));
		// Признак записи времени отметки
		const bool timed = (value.is(codec::toml::type_t::LOCAL_TIME) ||
		 value.is(codec::toml::type_t::LOCAL_DATETIME) || value.is(codec::toml::type_t::OFFSET_DATETIME));
		/**
		 * Если записывается дата отметки времени
		 */
		if(dated){
			// Выполняем запись даты отметки времени
			::snprintf(buffer, sizeof(buffer), "%04u-%02u-%02u", static_cast <uint32_t> (stamp.date.year),
			 static_cast <uint32_t> (stamp.date.month), static_cast <uint32_t> (stamp.date.day));
			// Выполняем дозапись даты отметки времени
			result.append(buffer);
		}
		/**
		 * Если записываются и дата, и время отметки
		 *
		 * @note Разделителем ставится заглавная «T» при всяком написании исходном:
		 *       набор сверки ждёт записи приведённой, а пробел договором TOML дозволен
		 */
		if(dated && timed)
			// Выполняем дозапись разделителя даты и времени отметки
			result.push_back('T');
		/**
		 * Если записывается время отметки
		 */
		if(timed){
			// Выполняем запись времени отметки
			::snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", static_cast <uint32_t> (stamp.time.hour),
			 static_cast <uint32_t> (stamp.time.minute), static_cast <uint32_t> (stamp.time.second));
			// Выполняем дозапись времени отметки
			result.append(buffer);
			/**
			 * Если время отметки несёт долю секунды
			 */
			if(stamp.time.digits > 0){
				// Выполняем запись доли секунды написанием исходным
				::snprintf(buffer, sizeof(buffer), ".%0*u", static_cast <int32_t> (stamp.time.digits),
				 static_cast <uint32_t> (stamp.time.nanosecond / static_cast <uint32_t> (::pow(10, (9 - stamp.time.digits)))));
				// Выполняем дозапись доли секунды
				result.append(buffer);
			}
		}
		/**
		 * Если отметка времени несёт смещение часового пояса
		 */
		if(stamp.offset != codec::toml::NO_TIMEZONE){
			/**
			 * Если смещением часового пояса является UTC
			 *
			 * @note Знак ставится заглавным при всяком написании исходном по той же
			 *       причине, что и разделитель даты со временем
			 */
			if(stamp.offset == 0)
				// Выполняем дозапись знака часового пояса UTC
				result.push_back('Z');
			/**
			 * Если смещением часового пояса является иное
			 */
			else {
				// Выполняем запись смещения часового пояса
				::snprintf(buffer, sizeof(buffer), "%c%02u:%02u", ((stamp.offset < 0) ? '-' : '+'),
				 static_cast <uint32_t> (::abs(stamp.offset) / 60), static_cast <uint32_t> (::abs(stamp.offset) % 60));
				// Выполняем дозапись смещения часового пояса
				result.append(buffer);
			}
		}
		// Выводим содержимое значения
		return result;
	}
	/**
	 * @brief Функция выдачи значения дерева настроек записью JSON
	 *
	 * @param value  выдаваемое значение дерева настроек
	 * @param result собираемая запись JSON
	 *
	 */
	void render(const codec::toml::value_t & value, string & result) noexcept {
		/**
		 * Если значение является таблицею
		 */
		if(value.is(codec::toml::type_t::TABLE)){
			// Выполняем открытие записи таблицы
			result.push_back('{');
			/**
			 * Выполняем перебор всех пар таблицы
			 */
			for(size_t i = 0; i < value.size(); i++){
				/**
				 * Если пара таблицы не является первою
				 */
				if(i > 0)
					// Выполняем запись разделителя пар
					result.push_back(',');
				// Выполняем запись имени пары
				result.append(escaped(value.key(i)));
				// Выполняем запись разделителя имени и значения
				result.push_back(':');
				// Выполняем запись значения пары
				render(value[i], result);
			}
			// Выполняем закрытие записи таблицы
			result.push_back('}');
		/**
		 * Если значение является перечнем
		 */
		} else if(value.is(codec::toml::type_t::ARRAY)) {
			// Выполняем открытие записи перечня
			result.push_back('[');
			/**
			 * Выполняем перебор всех значений перечня
			 */
			for(size_t i = 0; i < value.size(); i++){
				/**
				 * Если значение перечня не является первым
				 */
				if(i > 0)
					// Выполняем запись разделителя значений
					result.push_back(',');
				// Выполняем запись значения перечня
				render(value[i], result);
			}
			// Выполняем закрытие записи перечня
			result.push_back(']');
		/**
		 * Если значение является скалярным
		 */
		} else {
			// Выполняем открытие записи скалярного значения
			result.append("{\"type\":");
			// Выполняем запись вида скалярного значения
			result.append(escaped(named(value.type())));
			// Выполняем запись разделителя вида и содержимого
			result.append(",\"value\":");
			// Выполняем запись содержимого скалярного значения
			result.append(escaped(content(value)));
			// Выполняем закрытие записи скалярного значения
			result.push_back('}');
		}
	}
}

/**
 * @brief Функция запуска щупа сличения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода щупа сличения
 *
 */
int32_t main(int32_t argc, char ** argv) noexcept {
	/**
	 * Если имя разбираемого файла не передано
	 */
	if(argc < 2){
		// Выводим указание к вызову щупа
		::fprintf(stderr, "usage: dump <file.toml>\n");
		// Выходим из приложения с ошибкой
		return 2;
	}
	/**
	 * Признак выдачи перезаписи вместо записи разобранного дерева
	 *
	 * @details Сличение с эталоном поверяет РАЗБОР: наше дерево против дерева набора.
	 * Запись же поверялась одними нами - кругом через собственный разбор, - и
	 * заблуждение, чтению и записи общее, круг тот пережило бы. Выдача перезаписи
	 * открывает эталону и её: текст, нами записанный, читается заново и сличается с
	 * ЭТАЛОННЫМ деревом набора, а не с нашим. Общим у двух путей остаётся один разбор,
	 * а он эталоном и поверен
	 */
	const bool rewriting = ((argc > 2) && (string(argv[2]).compare("rewrite") == 0));
	// Поток чтения разбираемого файла
	ifstream file(argv[1], ios::binary);
	// Хранилище содержимого разбираемого файла
	stringstream buffer;
	// Выполняем чтение содержимого разбираемого файла
	buffer << file.rdbuf();
	// Дерево настроек разбираемого текста
	codec::toml::document_t document(::logger());
	/**
	 * Если разобрать текст настроек не удалось
	 */
	if(!document.parse(buffer.str())){
		// Выводим сообщение об отказе разбора
		::fprintf(stderr, "%s\n", codec::toml::message(document.error()));
		// Выходим из приложения с ошибкой
		return 1;
	}
	/**
	 * Если затребована выдача перезаписи текста настроек
	 */
	if(rewriting){
		// Получаем перезапись дерева настроек
		const string written = document.text();
		/**
		 * Если запись дерево отвергла
		 */
		if(written.empty() && !document.empty()){
			// Выводим сообщение об отказе записи
			::fprintf(stderr, "%s\n", codec::toml::message(document.error()));
			// Выходим из приложения с ошибкой
			return 1;
		}
		// Выполняем выдачу перезаписи дерева настроек
		::printf("%s", written.c_str());
		// Выходим из приложения
		return 0;
	}
	// Владеющее значение, с дерева настроек снятое
	const codec::toml::value_t lifted(document);
	/**
	 * Выполняем сличение двух путей перезаписи на корпусе соответствия
	 *
	 * @details Дерево и снятое с него владеющее значение переписывают одно и то же
	 *          содержимое, и прочтённое из двух перезаписей обязано совпасть. Примечания
	 *          при обоих путях говорят, что расходиться им нечем, - а слово это ничем не
	 *          подтверждено, покуда не проверено делом. Корпус соответствия для того и
	 *          годен: он несёт то, чего ворошитель не построит, - записи всякого вида в
	 *          настоящих сочетаниях
	 *
	 * @note Сличается ЗНАЧЕНИЕ, а не текст: владеющее значение примечаний не держит
	 *       вовсе, и требовать дословного совпадения значило бы требовать невозможного
	 *
	 * @note Расхождение валит прогон, а не печатается вскользь: молчаливая жалоба на
	 *       поток ошибок утонула бы в выдаче корпуса из сотен случаев
	 */
	{
		// Перезапись снятого значения настройками наречия разбора
		const string written = lifted.dump(document.writing());
		/**
		 * Если перезапись снятого значения не пуста
		 */
		if(!written.empty()){
			// Дерево настроек перезаписи снятого значения
			codec::toml::document_t rebuilt(::logger());
			/**
			 * Если перезапись снятого значения разобрать не удалось
			 */
			if(!rebuilt.parse(written)){
				// Выводим сообщение об отказе разбора перезаписи
				::fprintf(stderr, "перезапись снятого значения не разобрана: %s\n[%s]\n",
				 codec::toml::message(rebuilt.error()), written.c_str());
				// Выходим из приложения с ошибкой
				return 1;
			}
			// Значение, с дерева перезаписи снятое
			const codec::toml::value_t back(rebuilt);
			/**
			 * Если прочтённое из перезаписи со снятым значением разошлось
			 */
			if(!(back == lifted)){
				// Выводим сообщение о расхождении содержимого
				::fprintf(stderr, "содержимое кругового хода разошлось\nдерево:\n[%s]\nзначение:\n[%s]\n",
				 document.text().c_str(), written.c_str());
				// Выходим из приложения с ошибкой
				return 1;
			}
			/**
			 * Если перезапись снятого значения неустойчива
			 *
			 * @note Сличение значений оформления НЕ задевает: равенство их судит по
			 *       содержимому, и потеря ограды либо признака записи перечня им не
			 *       ловится вовсе. Устойчивость же перезаписи ловит именно оформление -
			 *       оба текста собраны одним и тем же путём, и примечаний в них нет ни в
			 *       одном. Проверено подменою признака записи перечня на обратный:
			 *       сличение значений её не заметило, устойчивость перезаписи заметила
			 */
			if(back.dump(document.writing()) != written){
				// Выводим сообщение о неустойчивости перезаписи снятого значения
				::fprintf(stderr, "перезапись снятого значения неустойчива\nпервая:\n[%s]\nвторая:\n[%s]\n",
				 written.c_str(), back.dump(document.writing()).c_str());
				// Выходим из приложения с ошибкой
				return 1;
			}
		}
	}
	// Собираемая запись JSON
	string result;
	// Выполняем выдачу дерева настроек записью JSON
	render(lifted, result);
	// Выполняем вывод собранной записи JSON
	::printf("%s\n", result.c_str());
	// Выходим из приложения
	return 0;
}
