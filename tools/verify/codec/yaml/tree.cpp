/**
 * @file: tree.cpp
 * @date: 2026-09-01
 *
 * @brief Щуп сличения дерева документа YAML с эталонным деревом набора сверки
 *
 * @details Сличение по потоку событий поверяет ЧТЕНИЕ: порядок событий, места их и
 * оформление. Дерево же, чтением собранное, им не поверяется вовсе - а именно деревом
 * пользуется потребитель. Набор сверки несёт для того эталонную запись JSON у 248 случаев
 * из 351, и щуп этот выдаёт наше дерево тою же записью
 *
 * Вторым доводом щуп поверяет ЗАПИСЬ: с признаком `rewrite` дерево переписывается,
 * читается заново и выдаётся уже с перезаписи. Расхождение с эталоном тогда означает, что
 * запись поменяла смысл, - а поверялась она прежде одними нами, кругом через собственный
 * разбор, и заблуждение, чтению и записи общее, круг тот переживало
 *
 * @copyright: Copyright © 2026
 */

#include <codec/yaml/yaml.hpp>
#include <sys/log.hpp>

#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;
using namespace awh;

/**
 * @brief Функция получения объекта логирования
 *
 * @return объект логирования
 *
 */
static log_t * logger() noexcept {
	// Объект фреймворка
	static fmk_t fmk;
	// Объект логирования
	static log_t log(&fmk);
	// Выводим объект логирования
	return &log;
}
/**
 * @brief Функция экранирования записи строки по правилам JSON
 *
 * @param text экранируемая запись строки
 * @return     экранированная запись строки
 *
 */
static string escaped(const string_view text) noexcept {
	// Собираемая запись строки
	string result("\"");
	/**
	 * Выполняем перебор всех знаков записи строки
	 */
	for(size_t i = 0; i < text.size(); i++){
		// Получаем беззнаковое значение очередного знака
		const uint8_t letter = static_cast <uint8_t> (text[i]);
		/**
		 * Определяем очередной знак записи строки
		 */
		switch(letter){
			// Если знаком является двойная кавычка
			case '"': result.append("\\\""); break;
			// Если знаком является обратная косая черта
			case '\\': result.append("\\\\"); break;
			// Если знаком является перевод строки
			case '\n': result.append("\\n"); break;
			// Если знаком является возврат каретки
			case '\r': result.append("\\r"); break;
			// Если знаком является табуляция
			case '\t': result.append("\\t"); break;
			/**
			 * Если знаком является знак иной
			 */
			default: {
				/**
				 * Если знак управляющим является
				 */
				if(letter < 0x20){
					// Хранилище записи кодового значения знака
					char buffer[8];
					// Выполняем сборку записи кодового значения знака
					::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast <uint32_t> (letter));
					// Выполняем добавление записи кодового значения к строке
					result.append(buffer);
				// Выполняем добавление знака к записи строки
				} else result.push_back(text[i]);
			}
		}
	}
	// Выводим собранную запись строки
	return result.append("\"");
}
/**
 * @brief Функция выдачи владеющего значения записью JSON договора набора сверки
 *
 * @details Набор сверки выдаёт скалярное значение видом его: число - числом, логическое -
 * логическим, пустое - null, прочее - строкой. Разрешение вида ведётся ядровой схемой,
 * и берётся оно у самого дерева, а не решается здесь заново
 *
 * @param value выдаваемое владеющее значение
 * @param result строка, куда ведётся выдача
 *
 */
static void render(const codec::yaml::value_t & value, string & result) noexcept {
	/**
	 * Определяем вид выдаваемого значения
	 */
	switch(static_cast <uint8_t> (value.kind())){
		/**
		 * Если значение отображением пар является
		 */
		case static_cast <uint8_t> (codec::yaml::kind_t::MAPPING): {
			// Выполняем открытие записи отображения
			result.push_back('{');
			/**
			 * Выполняем перебор всех пар отображения
			 *
			 * @note Перебор ведётся номером, а не именем: отображение вправе нести
			 *       повторяющиеся имена, и обход по имени слил бы две пары в одну
			 */
			for(size_t i = 0; i < value.size(); i++){
				/**
				 * Если пара первою не является
				 */
				if(i > 0)
					// Выполняем запись разделителя пар
					result.push_back(',');
				// Выполняем запись имени пары
				result.append(escaped(value.key(i)));
				// Выполняем запись разделителя имени со значением
				result.push_back(':');
				// Выполняем выдачу значения пары
				render(value[i], result);
			}
			// Выполняем закрытие записи отображения
			result.push_back('}');
		} break;
		/**
		 * Если значение перечнем является
		 */
		case static_cast <uint8_t> (codec::yaml::kind_t::SEQUENCE): {
			// Выполняем открытие записи перечня
			result.push_back('[');
			/**
			 * Выполняем перебор всех значений перечня
			 */
			for(size_t i = 0; i < value.size(); i++){
				/**
				 * Если значение первым не является
				 */
				if(i > 0)
					// Выполняем запись разделителя значений
					result.push_back(',');
				// Выполняем выдачу очередного значения перечня
				render(value[i], result);
			}
			// Выполняем закрытие записи перечня
			result.push_back(']');
		} break;
		/**
		 * Если значение скалярным является
		 */
		default: {
			/**
			 * Если значение пустым является
			 */
			if(value.is(codec::yaml::type_t::NUL))
				// Выполняем запись пустого значения
				result.append("null");
			/**
			 * Если значение логическим является
			 */
			else if(value.is(codec::yaml::type_t::BOOL)) {
				// Извлекаемое логическое значение
				bool flag = false;
				// Выполняем извлечение логического значения
				value.value(flag);
				// Выполняем запись логического значения
				result.append(flag ? "true" : "false");
			/**
			 * Если значение целым со знаком является
			 */
			} else if(value.is(codec::yaml::type_t::INT8) || value.is(codec::yaml::type_t::INT16) ||
			          value.is(codec::yaml::type_t::INT32) || value.is(codec::yaml::type_t::INT64)) {
				// Извлекаемое целое значение со знаком
				int64_t number = 0;
				// Выполняем извлечение целого значения со знаком
				value.value(number);
				// Выполняем запись целого значения со знаком
				result.append(std::to_string(number));
			/**
			 * Если значение целым без знака является
			 */
			} else if(value.is(codec::yaml::type_t::UINT8) || value.is(codec::yaml::type_t::UINT16) ||
			          value.is(codec::yaml::type_t::UINT32) || value.is(codec::yaml::type_t::UINT64)) {
				// Извлекаемое целое значение без знака
				uint64_t number = 0;
				// Выполняем извлечение целого значения без знака
				value.value(number);
				// Выполняем запись целого значения без знака
				result.append(std::to_string(number));
			/**
			 * Если значение дробным является
			 *
			 * @note Запись дробного берётся ИСХОДНАЯ, а не собранная заново: набор сверки
			 *       хранит её так, как она стоит в тексте, и «0.278», собранное печатью
			 *       двойной точности, вышло бы «0.27800000000000002»
			 */
			} else if(value.is(codec::yaml::type_t::FLOAT) || value.is(codec::yaml::type_t::DOUBLE)) {
				// Выполняем запись дробного значения исходной записью его
				result.append(value.text());
			/**
			 * Если значение записано строкою
			 */
			} else result.append(escaped(value.text()));
		}
	}
}
/**
 * @brief Функция запуска приложения
 *
 * @param argc количество параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int main(int argc, char ** argv){
	/**
	 * Если имя разбираемого файла не передано
	 */
	if(argc < 2){
		// Выводим указание к вызову щупа
		::fprintf(stderr, "usage: tree <file.yaml> [rewrite]\n");
		// Выходим из приложения с ошибкой
		return 2;
	}
	// Признак выдачи дерева, с перезаписи прочтённого
	const bool rewriting = ((argc > 2) && (string(argv[2]).compare("rewrite") == 0));
	// Поток чтения разбираемого файла
	ifstream file(argv[1], ios::binary);
	// Хранилище содержимого разбираемого файла
	stringstream buffer;
	// Выполняем чтение содержимого разбираемого файла
	buffer << file.rdbuf();
	// Объект дерева документа
	codec::yaml::document_t document(::logger());
	/**
	 * Если разобрать текст документа не удалось
	 */
	if(!document.parse(buffer.str())){
		// Выводим сообщение об отказе разбора
		::fprintf(stderr, "%s\n", codec::yaml::message(document.error()));
		// Выходим из приложения с ошибкой
		return 1;
	}
	/**
	 * Если затребована выдача дерева, с перезаписи прочтённого
	 */
	if(rewriting){
		// Получаем перезапись дерева документа
		const string written = document.dump();
		/**
		 * Если запись дерево отвергла
		 */
		if(written.empty() && (document.documents() > 0)){
			// Выводим сообщение об отказе записи
			::fprintf(stderr, "запись отвергла дерево: %s\n", codec::yaml::message(document.error()));
			// Выходим из приложения с ошибкой
			return 3;
		}
		// Объект дерева документа, перезапись читающий обратно
		codec::yaml::document_t back(::logger());
		/**
		 * Если разобрать перезапись не удалось
		 */
		if(!back.parse(written)){
			// Выводим сообщение об отказе разбора перезаписи
			::fprintf(stderr, "перезапись не читается: %s\n", codec::yaml::message(back.error()));
			// Выходим из приложения с ошибкой
			return 4;
		}
		// Собираемая запись дерева
		string result;
		/**
		 * Выполняем перебор всех документов потока
		 */
		for(size_t i = 0; i < back.documents(); i++)
			// Выполняем выдачу очередного документа потока
			render(codec::yaml::value_t(back.root(i)), result);
		// Выполняем вывод собранной записи дерева
		::printf("%s\n", result.c_str());
		// Выходим из приложения
		return 0;
	}
	// Собираемая запись дерева
	string result;
	/**
	 * Выполняем перебор всех документов потока
	 */
	for(size_t i = 0; i < document.documents(); i++)
		// Выполняем выдачу очередного документа потока
		render(codec::yaml::value_t(document.root(i)), result);
	// Выполняем вывод собранной записи дерева
	::printf("%s\n", result.c_str());
	// Выходим из приложения
	return 0;
}
