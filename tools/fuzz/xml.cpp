/**
 * @file: xml.cpp
 * @date: 2026-08-11
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Инструмент фаззинга кодека разметки XML — построение полуструктурированного
 *        текста разметки с точечной порчей, подача его чтению целиком и кусками
 *        произвольного размера, сборка дерева и его перезапись для поиска аварийных
 *        завершений, выходов за границы буфера и расхождений разбора
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <random>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

/**
 * Подключаем заголовочный файл проекта
 */
#include <codec/xml/xml.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён контейнера разметки
 */
using namespace awh::codec;

/**
 * @brief Внутренние вспомогательные средства генератора (внутренняя компоновка)
 *
 */
namespace {
	/**
	 * @brief Учёт проделанной работы
	 *
	 */
	struct Statistic {
		// Количество построенных текстов разметки
		uint64_t texts;
		// Количество испорченных текстов разметки
		uint64_t corrupted;
		// Количество текстов, разобранных чтением до конца
		uint64_t survived;
		// Количество выданных чтением событий
		uint64_t events;
		// Количество собранных деревьев разметки
		uint64_t trees;
		// Количество перезаписей дерева разметки
		uint64_t rewrites;
		// Количество построенных заведомо правильных текстов разметки
		uint64_t intact;
		// Количество заведомо правильных текстов, разобранных чтением до конца
		uint64_t accepted;
		/**
		 * @brief Конструктор
		 *
		 */
		Statistic() noexcept :
		 texts(0), corrupted(0), survived(0),
		 events(0), trees(0), rewrites(0), intact(0), accepted(0) {}
	/**
	 * Учёт проделанной работы
	 *
	 * @note Имя «stat» тут не годится: у MinGW заголовки объявляют «struct stat»
	 *       в области видимости, и обращение к учёту становится двусмысленным
	 */
	} totals;

	/**
	 * @brief Атрибут узла, запомненный для сличения
	 *
	 */
	struct Property {
		// Префикс имени атрибута и его местное имя
		string prefix, local;
		// Обозначение пространства имён атрибута
		string uri;
		// Значение атрибута, приведённое к окончательному виду
		string value;
		// Признак того, что значение взято из объявления по умолчанию
		bool defaulted;
		/**
		 * @brief Конструктор
		 *
		 */
		Property() noexcept : defaulted(false) {}
		/**
		 * @brief Оператор сравнения
		 *
		 * @param other атрибут для сравнения
		 * @return      результат сравнения
		 *
		 */
		bool operator != (const Property & other) const noexcept {
			// Выполняем сличение всех признаков атрибута
			return (
				(this->prefix != other.prefix) ||
				(this->local != other.local) ||
				(this->uri != other.uri) ||
				(this->value != other.value) ||
				(this->defaulted != other.defaulted)
			);
		}
	};

	/**
	 * @brief Событие разбора, запомненное для сличения
	 *
	 */
	struct Event {
		// Разновидность события
		uint8_t event;
		// Префикс имени узла и его местное имя
		string prefix, local;
		// Обозначение пространства имён узла
		string uri;
		// Содержимое события
		string text;
		// Глубина вложенности узла события
		uint32_t depth;
		// Обращение с пробельным содержимым в узле события
		uint8_t space;
		// Признак того, что узел записан самозакрывающейся меткой
		bool empty;
		// Перечень атрибутов узла события
		vector <Property> attributes;
		// Перечень объявлений пространств имён узла события
		vector <Property> bindings;
		/**
		 * @brief Конструктор
		 *
		 */
		Event() noexcept : event(0), depth(0), space(0), empty(false) {}
		/**
		 * @brief Оператор сравнения
		 *
		 * @param other событие для сравнения
		 * @return      результат сравнения
		 *
		 */
		bool operator != (const Event & other) const noexcept {
			/**
			 * Если разошёлся хоть один признак самого события
			 */
			if((this->event != other.event) || (this->prefix != other.prefix) ||
			   (this->local != other.local) || (this->uri != other.uri) ||
			   (this->text != other.text) || (this->depth != other.depth) ||
			   (this->space != other.space) || (this->empty != other.empty) ||
			   (this->attributes.size() != other.attributes.size()) ||
			   (this->bindings.size() != other.bindings.size()))
				// Выводим результат сличения событий
				return true;
			/**
			 * Выполняем перебор всех атрибутов узла события
			 */
			for(size_t i = 0; i < this->attributes.size(); i++){
				// Если очередной атрибут разошёлся
				if(this->attributes.at(i) != other.attributes.at(i))
					// Выводим результат сличения событий
					return true;
			}
			/**
			 * Выполняем перебор всех объявлений пространств имён узла события
			 */
			for(size_t i = 0; i < this->bindings.size(); i++){
				// Если очередное объявление разошлось
				if(this->bindings.at(i) != other.bindings.at(i))
					// Выводим результат сличения событий
					return true;
			}
			// Выводим результат сличения событий
			return false;
		}
	};

	/**
	 * @brief Метод выборки настроек разбора текста разметки
	 *
	 * @param engine источник псевдослучайных чисел
	 * @return       настройки разбора текста разметки
	 *
	 */
	xml::reader_t::settings_t settings(mt19937 & engine) noexcept {
		// Собираемые настройки разбора текста разметки
		xml::reader_t::settings_t result;
		// Разряды, задающие сочетание признаков разбора
		const uint32_t flags = static_cast <uint32_t> (engine());
		// Устанавливаем флаг разрешения префиксов по договору о пространствах имён
		result.namespaces = ((flags & 0x01) != 0);
		// Устанавливаем флаг подстановки ссылок на объявленные сущности
		result.entities = ((flags & 0x02) != 0);
		// Устанавливаем флаг выдачи примечаний отдельным событием
		result.comments = ((flags & 0x04) != 0);
		// Устанавливаем флаг выдачи указаний обработчику отдельным событием
		result.processing = ((flags & 0x08) != 0);
		// Устанавливаем флаг отделения незначимого пробельного содержимого
		result.separateSpaces = ((flags & 0x10) != 0);
		// Устанавливаем флаг подстановки значений атрибутов, объявленных по умолчанию
		result.defaults = ((flags & 0x20) != 0);
		/**
		 * Устанавливаем флаг склеивания подряд идущих кусков текстового содержимого
		 *
		 * @note Склеивание включено всегда: без него выдача текстового содержимого
		 *       зависела бы от того, где легла граница куска, и договор о равенстве
		 *       подачи целиком подаче кусками проверить было бы нечем
		 */
		result.mergeText = true;
		/**
		 * Если требуется поджать пределы разбора
		 */
		if((flags & 0xC0) == 0xC0){
			// Устанавливаем наибольшую допустимую глубину вложенности узлов
			result.maxDepth = (1 + (engine() % 10));
			// Устанавливаем наибольшую допустимую длину имени в знаках
			result.maxName = (1 + (engine() % 48));
			// Устанавливаем наибольшее допустимое количество атрибутов у одного узла
			result.maxAttributes = (engine() % 6);
			// Устанавливаем наибольшее допустимое количество объявленных сущностей
			result.maxEntities = (engine() % 4);
			// Устанавливаем наибольший допустимый общий объём подстановки сущностей
			result.maxExpansion = (engine() % 1024);
			// Устанавливаем наибольший допустимый объём одного события
			result.maxEvent = (engine() % 512);
		}
		// Выводим собранные настройки разбора текста разметки
		return result;
	}

	/**
	 * @brief Метод построения имени узла разметки
	 *
	 * @param engine источник псевдослучайных чисел
	 * @param valid  признак построения заведомо правильной разметки
	 * @return       построенное имя узла разметки
	 *
	 */
	string title(mt19937 & engine, const bool valid) noexcept {
		// Результат работы функции - построенное имя узла разметки
		string result;
		/**
		 * Выполняем выборку записи имени узла разметки
		 */
		switch(engine() % 16){
			// Дописываем имя с префиксом объявленного пространства имён
			case 0:
			case 1:
			case 2:
			case 3: result.append("ns:"); break;
			// Дописываем имя с префиксом необъявленного пространства имён
			case 4: if(!valid) result.append("nope:"); break;
			// Дописываем имя с отведённым договором префиксом
			case 5: if(!valid) result.append("xml:"); break;
			// Дописываем имя с двумя разделителями префикса
			case 6: if(!valid) result.append("a:b:"); break;
		}
		// Дописываем местное имя узла разметки
		result.append(1, static_cast <char> ('a' + (engine() % 4)));
		/**
		 * Если требуется дописать имя длиной сверх обычного
		 */
		if((engine() % 12) == 0)
			// Дописываем продолжение имени узла разметки
			result.append(40, 'z');
		// Выводим построенное имя узла разметки
		return result;
	}

	/**
	 * @brief Метод построения содержимого узла разметки
	 *
	 * @param engine источник псевдослучайных чисел
	 * @param valid  признак построения заведомо правильной разметки
	 * @return       построенное содержимое узла разметки
	 *
	 */
	string content(mt19937 & engine, const bool valid) noexcept {
		// Результат работы функции - построенное содержимое узла разметки
		string result;
		/**
		 * Выполняем выборку записи содержимого узла разметки
		 */
		switch(engine() % 16){
			// Дописываем простое содержимое
			case 0: result.append("text"); break;
			// Дописываем пробельное содержимое
			case 1: result.append(" \t\r\n "); break;
			// Дописываем содержимое со ссылками на отведённые договором сущности
			case 2: result.append("&lt;&amp;&gt;&quot;&apos;"); break;
			// Дописываем содержимое с числовыми ссылками
			case 3: result.append("&#65;&#x416;&#x10FFFF;&#0;"); break;
			// Дописываем содержимое со ссылкой на объявленную сущность
			case 4: if(!valid) result.append("&plain; &markup; &loop;"); break;
			// Дописываем содержимое со ссылкой на необъявленную сущность
			case 5: if(!valid) result.append("&missing;"); break;
			// Дописываем содержимое с незавершённой ссылкой
			case 6: if(!valid) result.append("&nope tail"); break;
			// Дописываем содержимое со знаками не из набора ASCII
			case 7: result.append("содержимое"); break;
			// Дописываем содержимое с недопустимой последовательностью UTF-8
			case 8: if(!valid) result.append("bad\xC3\x28 tail"); break;
			// Дописываем содержимое с недопустимой для разметки последовательностью
			case 9: if(!valid) result.append("a]]>b"); break;
			// Дописываем длинное содержимое
			case 10: result.append(200, 'x'); break;
			// Дописываем содержимое числом
			case 11: result.append(" -1234 "); break;
			// Дописываем содержимое, набранное произвольными знаками
			case 12: {
				// Если требуется построить заведомо правильную разметку
				if(valid)
					// Выходим из выборки записи содержимого узла разметки
					break;
				// Количество знаков произвольного набора
				const uint32_t length = (engine() % 8);
				/**
				 * Выполняем дозапись знаков произвольного набора
				 */
				for(uint32_t i = 0; i < length; i++)
					// Дописываем произвольный знак
					result.push_back(static_cast <char> (engine() % 256));
			} break;
		}
		// Выводим построенное содержимое узла разметки
		return result;
	}

	/**
	 * @brief Метод построения узла разметки со всем его содержимым
	 *
	 * @param result текст разметки, к которому дописывается построенный узел
	 * @param engine источник псевдослучайных чисел
	 * @param depth  текущая глубина вложенности узла разметки
	 * @param valid  признак построения заведомо правильной разметки
	 *
	 */
	void element(string & result, mt19937 & engine, const uint32_t depth, const bool valid) noexcept {
		// Имя построенного узла разметки
		const string name(::title(engine, valid));
		// Дописываем знак начала открывающей метки узла
		result.push_back('<');
		// Дописываем имя узла разметки
		result.append(name);
		// Количество атрибутов построенного узла разметки
		const uint32_t count = (engine() % 4);
		// Разряды уже записанных узлу разновидностей атрибутов
		uint32_t written = 0;
		/**
		 * Если узел является корневым в заведомо правильной разметке
		 *
		 * @note Префикс пространства имён связывается корневым узлом: без связывания
		 *       всякое имя с префиксом разбор отвергает, и правильной такая разметка
		 *       не была бы вовсе
		 */
		if(valid && (depth == 0)){
			// Дописываем объявление пространства имён для префикса
			result.append(" xmlns:ns=\"urn:named\"");
			// Запоминаем записанную узлу разновидность атрибута
			written |= (1u << 1);
		}
		/**
		 * Выполняем построение атрибутов узла разметки
		 */
		for(uint32_t i = 0; i < count; i++){
			// Разновидность записываемого атрибута узла разметки
			const uint32_t kind = (engine() % 8);
			/**
			 * Если разновидность атрибута узлу уже записана
			 *
			 * @note Повтор имени атрибута договором запрещён, и заведомо правильная
			 *       разметка повторов не несёт: разбор отверг бы её на второй же метке,
			 *       не дойдя ни до разрешения пространств имён, ни до сборки дерева
			 */
			if(valid && ((written & (1u << kind)) != 0))
				// Выполняем переход к следующему атрибуту узла разметки
				continue;
			// Запоминаем записанную узлу разновидность атрибута
			written |= (1u << kind);
			// Дописываем отступ перед именем атрибута
			result.push_back(' ');
			/**
			 * Выполняем выборку записи атрибута узла разметки
			 */
			switch(kind){
				// Дописываем объявление пространства имён по умолчанию
				case 0: result.append("xmlns=\"urn:default\""); break;
				// Дописываем объявление пространства имён для префикса
				case 1: result.append("xmlns:ns=\"urn:named\""); break;
				// Дописываем отмену объявления пространства имён
				case 2: result.append(valid ? "xmlns:other=\"urn:other\"" : "xmlns:ns=\"\""); break;
				// Дописываем атрибут обращения с пробельным содержимым
				case 3: result.append((engine() % 2) == 0 ? "xml:space=\"preserve\"" : "xml:space=\"default\""); break;
				// Дописываем атрибут со значением, требующим приведения
				case 4: result.append(valid ? "attr=\"a\tb\r\nc\rd &amp; e\"" : "attr=\"a\tb\nc &amp; &plain;\""); break;
				// Дописываем атрибут со значением в одиночных кавычках
				case 5: result.append("other='value'"); break;
				// Дописываем атрибут без кавычек вокруг значения
				case 6: if(!valid) result.append("attr=value"); break;
				// Дописываем обычный атрибут
				case 7: {
					// Дописываем имя атрибута
					result.append(1, static_cast <char> ('p' + (engine() % 3)));
					// Дописываем значение атрибута
					result.append("=\"value\"");
				} break;
			}
		}
		/**
		 * Если узел записывается самозакрывающейся меткой
		 */
		if((engine() % 4) == 0){
			// Дописываем завершение самозакрывающейся метки узла
			result.append("/>");
			// Выходим из функции
			return;
		}
		// Дописываем знак завершения открывающей метки узла
		result.push_back('>');
		// Количество вложенных записей узла разметки
		const uint32_t inner = (engine() % 4);
		/**
		 * Выполняем построение вложенных записей узла разметки
		 */
		for(uint32_t i = 0; i < inner; i++){
			/**
			 * Выполняем выборку разновидности вложенной записи
			 */
			switch(engine() % 8){
				// Дописываем текстовое содержимое узла
				case 0:
				case 1:
				case 2: result.append(::content(engine, valid)); break;
				// Дописываем раздел дословного текста
				case 3: result.append(((engine() % 2) == 0) ? "<![CDATA[ a < b & c ]]>" : "<![CDATA[ a\r\n b\r c ]]>"); break;
				// Дописываем примечание
				case 4: result.append(valid ? "<!-- примечание -->" : "<!-- примечание -- хвост -->"); break;
				// Дописываем указание обработчику
				case 5: result.append("<?target данные?>"); break;
				// Дописываем вложенный узел разметки
				case 6:
				case 7: {
					// Если предел глубины вложенности не исчерпан
					if(depth < 6)
						// Выполняем построение вложенного узла разметки
						::element(result, engine, (depth + 1), valid);
					// Иначе дописываем текстовое содержимое узла
					else result.append("deep");
				} break;
			}
		}
		/**
		 * Если требуется оставить узел незакрытым
		 */
		if(!valid && ((engine() % 24) == 0))
			// Выходим из функции, не закрывая узел разметки
			return;
		// Дописываем знак начала закрывающей метки узла
		result.append("</");
		/**
		 * Дописываем имя закрываемого узла разметки
		 *
		 * @note Изредка закрывается имя, отличное от открытого: разбору положено
		 *       отвечать на такое отказом, и путь этот проверять тоже требуется
		 */
		result.append((!valid && ((engine() % 24) == 0)) ? string("other") : name);
		// Дописываем знак завершения закрывающей метки узла
		result.push_back('>');
	}

	/**
	 * @brief Метод построения полуструктурированного текста разметки
	 *
	 * @param engine источник псевдослучайных чисел
	 * @param valid  признак построения заведомо правильной разметки
	 * @return       построенный текст разметки
	 *
	 */
	string generate(mt19937 & engine, const bool valid) noexcept {
		// Результат работы функции - построенный текст разметки
		string result;
		/**
		 * Если требуется дописать метку порядка байтов
		 */
		if((engine() % 12) == 0)
			// Дописываем метку порядка байтов в начало текста разметки
			result.append("\xEF\xBB\xBF");
		/**
		 * Если требуется дописать объявление разметки
		 */
		if((engine() % 3) != 0){
			/**
			 * Выполняем выборку записи объявления разметки
			 */
			switch(engine() % 4){
				// Дописываем объявление разметки с одним изданием
				case 0: result.append("<?xml version=\"1.0\"?>"); break;
				// Дописываем объявление разметки с кодировкой
				case 1: result.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"); break;
				// Дописываем объявление разметки с признаком самодостаточности
				case 2: result.append("<?xml version=\"1.1\" encoding=\"utf-8\" standalone=\"yes\"?>"); break;
				// Дописываем объявление разметки с неизвестной кодировкой
				case 3: result.append(valid ? "<?xml version=\"1.0\"?>" : "<?xml version=\"1.0\" encoding=\"КОИ-8\"?>"); break;
			}
			// Дописываем знак завершения строки
			result.push_back('\n');
		}
		/**
		 * Если требуется дописать описание типа документа
		 */
		if((engine() % 2) == 0){
			// Дописываем начало описания типа документа
			result.append("<!DOCTYPE a");
			/**
			 * Если требуется дописать обозначение внешнего источника
			 */
			if((engine() % 4) == 0)
				// Дописываем обозначение внешнего источника
				result.append(" SYSTEM \"a.dtd\"");
			/**
			 * Если требуется дописать внутреннее подмножество
			 */
			if((engine() % 4) != 0){
				// Дописываем начало внутреннего подмножества
				result.append(" [\n");
				/**
				 * Выполняем построение объявлений внутреннего подмножества
				 */
				switch(engine() % 6){
					// Дописываем объявление простой сущности
					case 0: result.append(valid ? "<!ENTITY plain \"значение\">\n" : "<!ENTITY plain \"зна\r\nче\rние\">\n"); break;
					// Дописываем объявление сущности, содержащей разметку
					case 1: result.append("<!ENTITY plain \"v\">\n<!ENTITY markup \"<b>текст</b>\">\n"); break;
					// Дописываем объявление сущности, ссылающейся на себя
					case 2: if(!valid) result.append("<!ENTITY loop \"&loop;\">\n"); break;
					// Дописываем объявление сущности с многократной подстановкой
					case 3: result.append("<!ENTITY a0 \"xxxxxxxx\">\n<!ENTITY plain \"&a0;&a0;&a0;&a0;\">\n"); break;
					// Дописываем объявление умолчаний атрибутов
					case 4: result.append("<!ELEMENT a ANY>\n<!ATTLIST a q CDATA \"умолчание\" r ID #IMPLIED>\n"); break;
					// Дописываем объявление внешней сущности и указание обработчику
					case 5: result.append("<!ENTITY ext SYSTEM \"e.xml\">\n<?target данные?>\n<!-- примечание -->\n"); break;
				}
				// Дописываем завершение внутреннего подмножества
				result.push_back(']');
			}
			// Дописываем завершение описания типа документа
			result.append(">\n");
		}
		// Количество записей перед корневым узлом разметки
		const uint32_t before = (engine() % 3);
		/**
		 * Выполняем построение записей перед корневым узлом разметки
		 */
		for(uint32_t i = 0; i < before; i++){
			/**
			 * Выполняем выборку разновидности записи
			 */
			switch(engine() % 3){
				// Дописываем примечание
				case 0: result.append("<!-- голова -->\n"); break;
				// Дописываем указание обработчику
				case 1: result.append("<?target голова?>\n"); break;
				// Дописываем пробельное содержимое
				case 2: result.append("  \n"); break;
			}
		}
		// Выполняем построение корневого узла разметки
		::element(result, engine, 0, valid);
		/**
		 * Если требуется дописать второй узел верхнего уровня
		 *
		 * @note Договор допускает лишь один корневой узел, и разбору положено
		 *       отвечать на второй отказом
		 */
		if(!valid && ((engine() % 8) == 0))
			// Выполняем построение второго узла верхнего уровня
			::element(result, engine, 0, valid);
		// Дописываем знак завершения строки
		result.push_back('\n');
		// Выводим построенный текст разметки
		return result;
	}

	/**
	 * @brief Метод порчи построенного текста разметки
	 *
	 * @param text   текст разметки для порчи
	 * @param engine источник псевдослучайных чисел
	 *
	 */
	void corrupt(string & text, mt19937 & engine) noexcept {
		// Если текст разметки пустой, то порчу не выполняем
		if(text.empty())
			// Выходим из функции
			return;
		// Количество вносимых искажений
		const uint32_t count = (1 + (engine() % 4));
		/**
		 * Выполняем внесение искажений в текст разметки
		 */
		for(uint32_t i = 0; i < count; i++){
			// Разновидность вносимого искажения
			const uint32_t kind = (engine() % 4);
			/**
			 * Положение вносимого искажения
			 *
			 * @note Обращения к источнику разнесены по отдельным предложениям
			 *       намеренно: порядок вычисления доводов одного выражения языком
			 *       не определён, и два обращения в одном выражении давали бы разную
			 *       последовательность у разных построителей. Прогон обязан
			 *       воспроизводиться не только на своей машине
			 */
			const size_t place = (engine() % text.length());
			// Знак, вносимый искажением
			const char letter = static_cast <char> (engine() % 256);
			/**
			 * Выполняем выборку разновидности искажения
			 */
			switch(kind){
				// Выполняем замену произвольного знака
				case 0: text.at(place) = letter; break;
				// Выполняем обрезание текста разметки
				case 1: text.resize(place); break;
				// Выполняем вставку произвольного знака
				case 2: text.insert(place, 1, letter); break;
				// Выполняем удаление произвольного знака
				case 3: text.erase(place, 1); break;
			}
			// Если текст разметки опустел, то порчу прекращаем
			if(text.empty())
				// Выходим из цикла внесения искажений
				break;
		}
	}

	/**
	 * @brief Метод разбора текста разметки с запоминанием выданных событий
	 *
	 * @param text     разбираемый текст разметки
	 * @param options  настройки разбора текста разметки
	 * @param chunk    размер куска подачи, ноль - подача текста целиком
	 * @param events   собираемый перечень выданных событий
	 * @return         состояние чтения по завершении разбора
	 *
	 */
	xml::state_t consume(const string & text, const xml::reader_t::settings_t & options, const size_t chunk, vector <Event> & events) noexcept {
		// Создаём объект чтения текста разметки
		xml::reader_t reader(options);
		// Размер куска подачи текста разметки
		const size_t size = (chunk > 0 ? chunk : text.length());
		// Смещение начала очередного куска подачи
		size_t offset = 0;
		/**
		 * Выполняем подачу текста разметки кусками
		 */
		do {
			// Размер подаваемого куска текста разметки
			const size_t length = ((text.length() - offset) < size ? (text.length() - offset) : size);
			// Признак того, что подаётся последний кусок текста разметки
			const bool end = ((offset + length) >= text.length());
			// Если подача куска текста разметки не удалась
			if(!reader.feed(text.data() + offset, length, end))
				// Выходим из цикла подачи текста разметки
				break;
			// Выполняем смещение начала очередного куска подачи
			offset += length;
			/**
			 * Выполняем чтение выданных разбором событий
			 */
			while(reader.next()){
				// Создаём запоминаемое событие разбора
				Event event;
				// Запоминаем разновидность события
				event.event = static_cast <uint8_t> (reader.event());
				// Запоминаем префикс имени узла
				event.prefix.assign(reader.name().prefix);
				// Запоминаем местное имя узла
				event.local.assign(reader.name().local);
				// Запоминаем обозначение пространства имён узла
				event.uri.assign(reader.name().uri);
				// Запоминаем содержимое события
				event.text.assign(reader.text());
				// Запоминаем глубину вложенности узла события
				event.depth = reader.depth();
				// Запоминаем обращение с пробельным содержимым в узле события
				event.space = static_cast <uint8_t> (reader.space());
				// Запоминаем признак записи узла самозакрывающейся меткой
				event.empty = reader.empty();
				/**
				 * Выполняем перебор всех атрибутов узла события
				 */
				for(const xml::attribute_t & attribute : reader.attributes()){
					// Создаём запоминаемый атрибут узла
					Property property;
					// Запоминаем префикс имени атрибута
					property.prefix.assign(attribute.name.prefix);
					// Запоминаем местное имя атрибута
					property.local.assign(attribute.name.local);
					// Запоминаем обозначение пространства имён атрибута
					property.uri.assign(attribute.name.uri);
					// Запоминаем значение атрибута
					property.value.assign(attribute.value);
					// Запоминаем признак получения значения из объявления по умолчанию
					property.defaulted = attribute.defaulted;
					// Выполняем сохранение атрибута в перечне
					event.attributes.push_back(::move(property));
				}
				/**
				 * Выполняем перебор всех объявлений пространств имён узла события
				 */
				for(const xml::binding_t & binding : reader.bindings()){
					// Создаём запоминаемое объявление пространства имён
					Property property;
					// Запоминаем префикс объявленного пространства имён
					property.prefix.assign(binding.prefix);
					// Запоминаем обозначение объявленного пространства имён
					property.uri.assign(binding.uri);
					// Выполняем сохранение объявления в перечне
					event.bindings.push_back(::move(property));
				}
				// Выполняем сохранение события в перечне
				events.push_back(::move(event));
				// Выполняем учёт выданного разбором события
				totals.events++;
			}
			// Если разбор прекращён ошибкой
			if(reader.state() == xml::state_t::FAILED)
				// Выходим из цикла подачи текста разметки
				break;
		// Выполняем подачу до исчерпания текста разметки
		} while(offset < text.length());
		// Выводим состояние чтения по завершении разбора
		return reader.state();
	}

	/**
	 * @brief Метод вывода разбираемого текста разметки в шестнадцатеричном виде
	 *
	 * @param text выводимый текст разметки
	 *
	 */
	void dump(const string & text) noexcept {
		// Выводим начало записи разбираемого текста разметки
		::fprintf(stderr, "xml fuzz: text=\"");
		/**
		 * Выполняем перебор всех знаков разбираемого текста разметки
		 */
		for(size_t i = 0; i < text.length(); i++)
			// Выводим очередной знак разбираемого текста разметки
			::fprintf(stderr, "\\x%02X", static_cast <uint8_t> (text.at(i)));
		// Выводим завершение записи разбираемого текста разметки
		::fprintf(stderr, "\"\n");
	}

	/**
	 * @brief Метод сличения перечней выданных разбором событий
	 *
	 * @param first  перечень событий подачи текста целиком
	 * @param second перечень событий подачи текста кусками
	 * @param chunk  размер куска подачи текста разметки
	 * @param text   разбираемый текст разметки
	 * @return       результат сличения перечней
	 *
	 */
	bool compare(const vector <Event> & first, const vector <Event> & second, const size_t chunk, const string & text, const xml::reader_t::settings_t & options) noexcept {
		/**
		 * Если количество выданных событий разошлось
		 */
		if(first.size() != second.size()){
			// Выводим сообщение о расхождении количества выданных событий
			::fprintf(stderr, "xml fuzz: chunk=%zu events %zu != %zu\n", chunk, first.size(), second.size());
			// Выводим настройки разбора, при которых обнаружено расхождение
			::fprintf(stderr, "xml fuzz: ns=%d ent=%d com=%d pi=%d sp=%d def=%d depth=%u name=%u attrs=%u ents=%u exp=%llu event=%llu\n",
				(int) options.namespaces, (int) options.entities, (int) options.comments, (int) options.processing,
				(int) options.separateSpaces, (int) options.defaults, options.maxDepth, options.maxName,
				options.maxAttributes, options.maxEntities,
				(unsigned long long) options.maxExpansion, (unsigned long long) options.maxEvent);
			// Выводим разбираемый текст разметки
			dump(text);
			// Выводим результат сличения перечней
			return false;
		}
		/**
		 * Выполняем перебор всех выданных разбором событий
		 */
		for(size_t i = 0; i < first.size(); i++){
			// Если очередное событие разошлось
			if(first.at(i) != second.at(i)){
				// Выводим сообщение о расхождении выданного события
				::fprintf(stderr, "xml fuzz: chunk=%zu event %zu differs\n", chunk, i);
				// Выводим настройки разбора, при которых обнаружено расхождение
				::fprintf(stderr, "xml fuzz: ns=%d ent=%d com=%d pi=%d sp=%d def=%d depth=%u name=%u attrs=%u ents=%u exp=%llu event=%llu\n",
					(int) options.namespaces, (int) options.entities, (int) options.comments, (int) options.processing,
					(int) options.separateSpaces, (int) options.defaults, options.maxDepth, options.maxName,
					options.maxAttributes, options.maxEntities,
					(unsigned long long) options.maxExpansion, (unsigned long long) options.maxEvent);
				// Выводим разбираемый текст разметки
				dump(text);
				// Выводим результат сличения перечней
				return false;
			}
		}
		// Выводим результат сличения перечней
		return true;
	}

	/**
	 * @brief Метод обхода узла дерева разметки со всем его содержимым
	 *
	 * @param node обходимый узел дерева разметки
	 *
	 */
	void traverse(const xml::node_t & node) noexcept {
		// Выполняем обращение к имени узла дерева разметки
		const xml::name_t name = node.name();
		// Выполняем обращение к содержимому узла дерева разметки
		const string text = node.text();
		// Выполняем обращение к месту узла в исходном тексте разметки
		node.location();
		/**
		 * Выполняем перебор всех атрибутов узла дерева разметки
		 */
		for(const xml::attribute_t & attribute : node.attributes()){
			// Выполняем обращение к значению атрибута по его имени
			node.attribute(attribute.name.local, attribute.name.uri);
			// Выполняем обращение к признаку наличия атрибута
			node.has(attribute.name.local, attribute.name.uri);
			// Число, полученное разбором значения атрибута
			int64_t number = 0;
			// Выполняем разбор значения атрибута числом
			node.value(number, attribute.name.local, attribute.name.uri);
		}
		// Выполняем обращение к перечню объявлений пространств имён узла
		node.bindings();
		// Выполняем обращение к вложенному узлу по его имени
		node.child(name.local, name.uri);
		// Выполняем поиск вложенного узла по всему поддереву
		node.find(name.local, name.uri);
		/**
		 * Выполняем перебор всех вложенных узлов дерева разметки
		 */
		for(xml::node_t child = node.first(); child.valid(); child = child.next())
			// Выполняем обход вложенного узла дерева разметки
			traverse(child);
	}

	/**
	 * @brief Метод перезаписи дерева разметки
	 *
	 * @param document перезаписываемое дерево разметки
	 * @param options  настройки записи текста разметки
	 * @param result   собираемый текст перезаписи дерева разметки
	 * @return         результат выполнения операции
	 *
	 */
	bool rewrite(const xml::document_t & document, const xml::writer_t::settings_t & options, string & result) noexcept {
		// Создаём объект записи текста разметки
		xml::writer_t writer(options);
		// Выполняем учёт перезаписи дерева разметки
		totals.rewrites++;
		// Если запись дерева разметки не удалась
		if(!writer.element(document.root()) || !writer.complete())
			// Выводим отрицательный результат выполнения операции
			return false;
		// Выполняем сохранение собранного текста перезаписи
		result.assign(writer.text());
		// Выводим положительный результат выполнения операции
		return true;
	}

	/**
	 * @brief Метод проверки дерева разметки на построенном тексте
	 *
	 * @param text    разбираемый текст разметки
	 * @param options настройки разбора текста разметки
	 * @param engine  источник псевдослучайных чисел
	 * @return        результат проверки дерева разметки
	 *
	 */
	bool tree(const string & text, const xml::reader_t::settings_t & options, mt19937 & engine) noexcept {
		// Создаём дерево разметки
		xml::document_t document;
		// Выполняем учёт собранного дерева разметки
		totals.trees++;
		// Если разбор текста разметки не удался, то проверку прекращаем
		if(!document.parse(text, options))
			// Выводим результат проверки дерева разметки
			return true;
		// Выполняем обход всего собранного дерева разметки
		traverse(document.root());
		// Собираемые настройки записи текста разметки
		xml::writer_t::settings_t settings;
		/**
		 * Устанавливаем вид записи собираемого текста разметки
		 *
		 * @note Устойчивость перезаписи проверяется лишь на плотной записи. Отступы
		 *       нарядной записи ложатся в текст пробельным содержимым, дерево принимает
		 *       его наравне с прочим содержимым узла - выпадать из записи оно не вправе,
		 *       - и следующая запись отступает уже от него. Разметка от этого растёт
		 *       отступами, оставаясь равнозначной по смыслу: знак в знак нарядная запись
		 *       не повторяется по устройству, а не по недосмотру
		 */
		settings.format = (((engine() % 4) == 0) ? xml::format_t::PRETTY : xml::format_t::COMPACT);
		// Устанавливаем флаг записи узлов без содержимого самозакрывающейся меткой
		settings.collapse = ((engine() % 2) == 0);
		// Устанавливаем флаг экранирования знаков, выходящих за пределы US-ASCII
		settings.escapeNonAscii = ((engine() % 2) == 0);
		/**
		 * Настройки повторного разбора перезаписанного текста разметки
		 *
		 * @note Пределы разбора снимаются намеренно: запись вправе быть длиннее
		 *       исходного текста, оставаясь равнозначной ему по смыслу. Знаки за
		 *       пределами US-ASCII записываются числовыми ссылками по десятку байтов
		 *       на знак, а префиксы пространств имён назначаются заново. Держать на
		 *       перезаписи пределы исходного текста значило бы отвергать её за то,
		 *       чего сама запись и добавила
		 */
		xml::reader_t::settings_t relaxed = options;
		// Настройки разбора с пределами по умолчанию
		const xml::reader_t::settings_t limits;
		// Снимаем предел глубины вложенности узлов
		relaxed.maxDepth = limits.maxDepth;
		// Снимаем предел длины имени
		relaxed.maxName = limits.maxName;
		// Снимаем предел количества атрибутов узла
		relaxed.maxAttributes = limits.maxAttributes;
		// Снимаем предел количества объявленных сущностей
		relaxed.maxEntities = limits.maxEntities;
		// Снимаем предел объёма подстановки сущностей
		relaxed.maxExpansion = limits.maxExpansion;
		// Снимаем предел объёма одного события
		relaxed.maxEvent = limits.maxEvent;
		// Текст первой перезаписи дерева разметки
		string first;
		// Если перезапись дерева разметки не удалась, то проверку прекращаем
		if(!rewrite(document, settings, first))
			// Выводим результат проверки дерева разметки
			return true;
		// Если перезапись дерева разметки оказалась пустой, то проверку прекращаем
		if(first.empty())
			// Выводим результат проверки дерева разметки
			return true;
		// Создаём дерево разметки для повторного разбора
		xml::document_t repeat;
		/**
		 * Если повторный разбор перезаписанного текста не удался
		 */
		if(!repeat.parse(first, relaxed)){
			// Выводим сообщение об отказе повторного разбора перезаписанного текста
			::fprintf(stderr, "xml fuzz: reparse failed, error=%u\n", static_cast <uint32_t> (repeat.error()));
			// Выводим исходный текст разметки
			dump(text);
			// Выводим перезапись дерева разметки
			dump(first);
			// Выводим результат проверки дерева разметки
			return false;
		}
		// Текст повторной перезаписи дерева разметки
		string second;
		/**
		 * Если повторная перезапись дерева разметки не удалась
		 */
		if(!rewrite(repeat, settings, second)){
			// Выводим сообщение об отказе повторной перезаписи дерева разметки
			::fprintf(stderr, "xml fuzz: second rewrite failed\n");
			// Выводим перезапись дерева разметки
			dump(first);
			// Выводим результат проверки дерева разметки
			return false;
		}
		/**
		 * Если повторная перезапись разошлась с первой
		 */
		if((settings.format == xml::format_t::COMPACT) && (second != first)){
			// Выводим сообщение о расхождении повторной перезаписи
			::fprintf(stderr, "xml fuzz: rewrite unstable, collapse=%d escape=%d ns=%d ent=%d com=%d pi=%d sp=%d def=%d depth=%u name=%u attrs=%u ents=%u exp=%llu event=%llu\n",
				(int) settings.collapse, (int) settings.escapeNonAscii,
				(int) options.namespaces, (int) options.entities, (int) options.comments, (int) options.processing,
				(int) options.separateSpaces, (int) options.defaults, options.maxDepth, options.maxName,
				options.maxAttributes, options.maxEntities,
				(unsigned long long) options.maxExpansion, (unsigned long long) options.maxEvent);
			// Выводим первую перезапись дерева разметки
			dump(first);
			// Выводим повторную перезапись дерева разметки
			dump(second);
			// Выводим результат проверки дерева разметки
			return false;
		}
		// Выводим результат проверки дерева разметки
		return true;
	}
}

/**
 * @brief Функция запуска приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Количество выполняемых проходов генератора
	uint64_t count = 3000;
	// Если количество проходов задано параметром командной строки
	if(argc > 1)
		// Выполняем чтение количества проходов из параметра командной строки
		count = static_cast <uint64_t> (::strtoull(argv[1], nullptr, 10));
	// Создаём источник псевдослучайных чисел с закреплённым зерном
	mt19937 engine(0x5A1CE);
	/**
	 * Выполняем проходы генератора
	 */
	for(uint64_t i = 0; i < count; i++){
		// Выполняем выборку настроек разбора текста разметки
		const xml::reader_t::settings_t options = ::settings(engine);
		/**
		 * Признак построения заведомо правильной разметки
		 *
		 * @note Половина текстов строится правильными намеренно: разметка, отвергаемая
		 *       первой же меткой, до разрешения пространств имён, подстановки умолчаний
		 *       и сборки дерева не доходит вовсе, и пути эти остались бы непройденными
		 */
		const bool valid = ((engine() % 2) == 0);
		// Количество испорченных текстов разметки до построения очередного
		const uint64_t before = totals.corrupted;
		// Выполняем построение текста разметки
		string text = ::generate(engine, valid);
		// Выполняем учёт построенного текста разметки
		totals.texts++;
		/**
		 * Если требуется испортить построенный текст разметки
		 */
		if((engine() % 3) == 0){
			// Выполняем порчу построенного текста разметки
			::corrupt(text, engine);
			// Выполняем учёт испорченного текста разметки
			totals.corrupted++;
		}
		// Признак того, что построенный правильным текст остался неиспорченным
		const bool intact = (valid && (totals.corrupted == before));
		// Если построенный текст разметки остался заведомо правильным
		if(intact)
			// Выполняем учёт построенного заведомо правильного текста разметки
			totals.intact++;
		// Перечень событий подачи текста разметки целиком
		vector <Event> whole;
		// Выполняем подачу текста разметки целиком
		const xml::state_t state = ::consume(text, options, 0, whole);
		/**
		 * Если текст разметки разобран до конца
		 */
		if(state == xml::state_t::FINISHED){
			// Выполняем учёт разобранного до конца текста разметки
			totals.survived++;
			// Если разобранный текст построен заведомо правильным и не испорчен
			if(intact)
				// Выполняем учёт принятого разбором правильного текста разметки
				totals.accepted++;
		}
		// Размер куска подачи текста разметки
		const size_t chunk = (1 + (engine() % 16));
		// Перечень событий подачи текста разметки кусками
		vector <Event> chunked;
		// Выполняем подачу текста разметки кусками
		::consume(text, options, chunk, chunked);
		// Если перечни выданных разбором событий разошлись
		if(!::compare(whole, chunked, chunk, text, options))
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
		// Если проверка дерева разметки не удалась
		if(!::tree(text, options, engine))
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
	}
	// Выводим статистику работы генератора
	::fprintf(
		stdout,
		"xml fuzz: %llu texts (%llu corrupted), %llu events, %llu parsed to the end, %llu trees, %llu rewrites, %llu of %llu intact texts accepted\n",
		static_cast <unsigned long long> (totals.texts),
		static_cast <unsigned long long> (totals.corrupted),
		static_cast <unsigned long long> (totals.events),
		static_cast <unsigned long long> (totals.survived),
		static_cast <unsigned long long> (totals.trees),
		static_cast <unsigned long long> (totals.rewrites),
		static_cast <unsigned long long> (totals.accepted),
		static_cast <unsigned long long> (totals.intact)
	);
	// Выходим из приложения
	return EXIT_SUCCESS;
}
