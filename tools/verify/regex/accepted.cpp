/**
 * @file: accepted.cpp
 * @date: 2026-07-31
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Порождение набора имён свойств Юникода, принимаемых эталонной реализацией —
 *        имена перебираются по таблицам имён стандарта, а принятие каждого имени
 *        устанавливается сборкой выражения эталонной реализацией
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <set>
#include <string>
#include <cstdio>
#include <fstream>
#include <iostream>

/**
 * Устанавливаем ширину единицы кодирования эталонной реализации
 */
#define PCRE2_CODE_UNIT_WIDTH 8

/**
 * Подключаем заголовочный файл эталонной реализации
 */
#include <pcre2.h>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция проверки принятия имени свойства эталонной реализацией
 *
 * @param name имя проверяемого свойства Юникода
 * @return     результат проверки принятия имени свойства
 *
 */
static bool accepted(const string & name) noexcept {
	// Создаём выражение, состоящее из проверяемого свойства
	const string pattern = ("\\p{" + name + "}");
	// Код ошибки сборки регулярного выражения
	int32_t code = 0;
	// Смещение ошибки сборки регулярного выражения
	PCRE2_SIZE offset = 0;
	// Выполняем сборку регулярного выражения эталонной реализацией
	pcre2_code * result = ::pcre2_compile(
		reinterpret_cast <PCRE2_SPTR> (pattern.c_str()),
		pattern.size(), PCRE2_UTF | PCRE2_UCP, &code, &offset, nullptr
	);
	/**
	 * Если сборка регулярного выражения не выполнена
	 */
	if(result == nullptr)
		// Выводим результат проверки принятия имени свойства
		return false;
	// Выполняем освобождение собранного регулярного выражения
	::pcre2_code_free(result);
	// Выводим результат проверки принятия имени свойства
	return true;
}
/**
 * @brief Функция извлечения имён из таблицы имён стандарта Юникода
 *
 * @param path   путь файла таблицы имён стандарта Юникода
 * @param result набор извлечённых имён
 *
 */
static void collect(const string & path, set <string> & result) noexcept {
	// Выполняем открытие файла таблицы имён стандарта Юникода
	ifstream file(path);
	// Читаемая строка файла таблицы имён
	string line;
	/**
	 * Выполняем чтение файла таблицы имён построчно
	 */
	while(getline(file, line)) {
		// Выполняем отсечение примечания строки
		const size_t comment = line.find('#');
		/**
		 * Если строка содержит примечание
		 */
		if(comment != string::npos)
			// Выполняем отсечение примечания строки
			line.resize(comment);
		// Позиция начала очередного поля строки
		size_t begin = 0;
		/**
		 * Выполняем разбор полей строки, разделённых точкой с запятой
		 */
		while(begin <= line.size()) {
			// Выполняем поиск конца очередного поля строки
			size_t end = line.find(';', begin);
			/**
			 * Если разделитель полей не найден
			 */
			if(end == string::npos)
				// Выполняем установку конца поля концом строки
				end = line.size();
			// Получаем очередное поле строки
			string field = line.substr(begin, (end - begin));
			// Переходим к следующему полю строки
			begin = (end + 1);
			// Выполняем отсечение пробельных символов начала поля
			while(!field.empty() && ((field.front() == ' ') || (field.front() == '\t')))
				// Выполняем удаление пробельного символа начала поля
				field.erase(field.begin());
			// Выполняем отсечение пробельных символов конца поля
			while(!field.empty() && ((field.back() == ' ') || (field.back() == '\t') || (field.back() == '\r')))
				// Выполняем удаление пробельного символа конца поля
				field.pop_back();
			/**
			 * Если поле именем не является
			 */
			if(field.empty() || (field.find(' ') != string::npos) || (field.find("..") != string::npos))
				// Переходим к следующему полю строки
				continue;
			/**
			 * Если поле состоит из шестнадцатеричных цифр
			 */
			if(field.find_first_not_of("0123456789ABCDEF") == string::npos)
				// Переходим к следующему полю строки
				continue;
			// Выполняем размещение имени в наборе извлечённых имён
			result.emplace(field);
		}
	}
}
/**
 * @brief Функция запуска порождения набора имён свойств
 *
 * @param argc количество доводов запуска
 * @param argv набор доводов запуска
 * @return     код возврата порождения набора имён свойств
 *
 */
int main(int argc, char ** argv) noexcept {
	// Получаем путь каталога таблиц стандарта Юникода
	const string tables = ((argc > 1) ? argv[1] : "submodules/pcre2/maint/Unicode.tables");
	// Получаем путь порождаемого набора имён свойств
	const string output = ((argc > 2) ? argv[2] : "sh/unicode.accepted");
	// Создаём набор имён, извлечённых из таблиц стандарта Юникода
	set <string> names;
	// Выполняем извлечение имён из таблицы обозначений свойств
	collect((tables + "/PropertyAliases.txt"), names);
	// Выполняем извлечение имён из таблицы обозначений значений свойств
	collect((tables + "/PropertyValueAliases.txt"), names);
	// Выполняем извлечение имён из таблицы письменностей
	collect((tables + "/Scripts.txt"), names);
	// Выполняем извлечение имён из таблицы расширений письменностей
	collect((tables + "/ScriptExtensions.txt"), names);
	// Создаём набор имён, принимаемых эталонной реализацией
	set <string> result;
	/**
	 * Выполняем перебор извлечённых имён свойств
	 */
	for(const auto & name : names) {
		/**
		 * Если имя свойства эталонной реализацией принято
		 */
		if(accepted(name))
			// Выполняем размещение имени в наборе принимаемых имён
			result.emplace(name);
		/**
		 * Выполняем перебор видов свойств, задаваемых обозначением
		 */
		for(const auto & kind : {"sc=", "scx=", "bc=", "gc="}) {
			// Создаём имя свойства с обозначением его вида
			const string qualified = (kind + name);
			/**
			 * Если имя свойства эталонной реализацией принято
			 */
			if(accepted(qualified))
				// Выполняем размещение имени в наборе принимаемых имён
				result.emplace(qualified);
		}
	}
	// Выполняем открытие порождаемого набора имён свойств
	ofstream file(output);
	/**
	 * Если порождаемый набор имён свойств не открыт
	 */
	if(!file.is_open()) {
		// Выводим сообщение об ошибке открытия набора имён свойств
		cout << "набор имён свойств не открыт: " << output << endl;
		// Выводим код возврата порождения набора имён свойств
		return 1;
	}
	/**
	 * Выполняем запись принимаемых имён свойств
	 */
	for(const auto & name : result)
		// Выполняем запись очередного имени свойства
		file << name << "\n";
	// Выводим количество принимаемых имён свойств
	printf("принимаемых имён свойств: %zu из %zu\n", result.size(), names.size());
	// Выводим код возврата порождения набора имён свойств
	return 0;
}
