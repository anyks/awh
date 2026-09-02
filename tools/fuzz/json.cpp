/**
 * @file json.cpp
 * @date 2026-08-15
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
 * @brief Инструмент фаззинга кодека JSON — построение полуструктурированного документа
 *        с точечной порчей, подача его чтению целиком и кусками произвольного размера,
 *        сборка дерева и его перезапись для поиска аварийных завершений, выходов за
 *        границы буфера и расхождений разбора
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cmath>
#include <string>
#include <vector>
#include <random>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>

/**
 * Подключаем заголовочный файл проекта
 */
#include <sys/log.hpp>
#include <codec/json/json.hpp>

/**
 * @brief Пространство имён проверок этого файла
 *
 * @note Держится оно безымянным намеренно: проверки кодеков собираются одной
 *       программою, и одноимённые построения разных файлов иначе сходятся в
 *       одно, порождая порчу вдали от места её причины
 *
 */
namespace {
	/**
	 * @brief Объект журнала проверок с отключённым выводом
	 *
	 * @details Вывод отключается назначением пустого перечня приёмников: отказы
	 *          разбора проверки наводят намеренно, и журнал их засорял бы выдачу
	 *
	 */
	struct Silent {
		/**
		 * @brief Функция получения объекта фреймворка проверок
		 *
		 * @details Объект заводится статикою местною, а не общею файла: заведение его
		 *          порядком построения статики оканчивается падением ещё до входа в
		 *          проверки, ибо фреймворк сам опирается на статику из библиотеки
		 *
		 * @return объект фреймворка проверок
		 *
		 */
		static const awh::fmk_t & framework() noexcept {
			// Объект фреймворка проверок
			static awh::fmk_t fmk;
			// Выводим объект фреймворка проверок
			return fmk;
		}
		// Объект журнала проверок
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
	 * @brief Функция получения объекта журнала проверок
	 *
	 * @return объект журнала проверок
	 *
	 */
	const awh::log_t * logger() noexcept {
		// Объект журнала проверок
		static Silent silent;
		// Выводим объект журнала проверок
		return &silent.log;
	}
}

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён контейнеров данных
 */
using namespace awh::codec;

/**
 * @brief Внутренние вспомогательные средства генератора (внутренняя компоновка)
 *
 */
namespace {
	/**
	 * @brief Перечни всех членов настроек, выбираемых наудачу
	 *
	 * @details Член берётся из перечня по длине его: выбор по остатку от числа,
	 * вписанного рукой, от перечня отставал молча - `number_t` знает два члена, а
	 * остаток брался от трёх, и в настройки уходило значение ВНЕ перечня
	 *
	 * @note Полноту перечней блюдёт собиратель: сторож ниже перебирает члены без
	 *       ветви `default`, и член, в перечень дописанный, отзовётся `-Wswitch`
	 *
	 */
	static const json::duplicate_t DUPLICATES[] = {
		json::duplicate_t::ERROR, json::duplicate_t::FIRST, json::duplicate_t::LAST, json::duplicate_t::KEEP
	};
	// Перечень всех правил преобразования чисел
	static const json::number_t NUMBERS[] = {
		json::number_t::NATIVE, json::number_t::CHECK
	};
	// Перечень всех видов оформления собираемого текста
	static const json::format_t FORMATS[] = {
		json::format_t::COMPACT, json::format_t::PRETTY
	};
	// Перечень всех способов экранирования при записи
	static const json::escape_t ESCAPES[] = {
		json::escape_t::MINIMAL, json::escape_t::SOLIDUS, json::escape_t::ASCII
	};
	/**
	 * @brief Сторож полноты перечней настроек
	 *
	 * @warning Ветвь `default` здесь ставить нельзя: она глушит `-Wswitch`, и сторож
	 *          перестанет кусать
	 *
	 */
	[[maybe_unused]] void guard(const json::duplicate_t duplicates, const json::number_t numbers) noexcept {
		// Перебираем все правила обращения с повторяющимся именем поля объекта
		switch(duplicates){
			case json::duplicate_t::ERROR: case json::duplicate_t::FIRST:
			case json::duplicate_t::LAST: case json::duplicate_t::KEEP: break;
		}
		// Перебираем все правила преобразования чисел
		switch(numbers){
			case json::number_t::NATIVE: case json::number_t::CHECK: break;
		}
	}
	/**
	 * @brief Сторож полноты перечней настроек записи
	 *
	 * @warning Ветвь `default` здесь ставить нельзя: она глушит `-Wswitch`, и сторож
	 *          перестанет кусать
	 *
	 */
	[[maybe_unused]] void guardWriting(const json::format_t format, const json::escape_t escape) noexcept {
		// Перебираем все виды оформления собираемого текста
		switch(format){
			case json::format_t::COMPACT: case json::format_t::PRETTY: break;
		}
		// Перебираем все способы экранирования при записи
		switch(escape){
			case json::escape_t::MINIMAL: case json::escape_t::SOLIDUS: case json::escape_t::ASCII: break;
		}
	}
	/**
	 * @brief Учёт проделанной работы
	 *
	 */
	struct Statistic {
		// Количество построенных текстов документов
		uint64_t texts;
		// Количество испорченных текстов документов
		uint64_t corrupted;
		// Количество текстов, разбор переживших
		uint64_t survived;
		// Количество собранных событий разбора
		uint64_t events;
		// Количество собранных деревьев документов
		uint64_t documents;
		// Количество перезаписей дерева документа
		uint64_t rewrites;
		// Количество потоковых прогонов
		uint64_t streams;
		// Количество чисел, прошедших круговой ход записи
		uint64_t numbers;
		// Количество снятых владеющих значений
		uint64_t values;
		// Количество значений, пересобранных потоковым сборщиком
		uint64_t builds;
		// Количество значений, привитых обратно в дерево документа
		uint64_t grafts;
		// Количество значений, снесённых из дерева документа по указателю
		uint64_t removals;
		// Количество значений, сброшенных в дереве документа по указателю
		uint64_t resets;
		/**
		 * @brief Конструктор
		 *
		 */
		Statistic() noexcept :
		 texts(0), corrupted(0), survived(0), events(0),
		 documents(0), rewrites(0), streams(0), numbers(0),
		 values(0), builds(0), grafts(0), removals(0), resets(0) {}
	};
	/**
	 * @brief Собранное событие разбора
	 *
	 * @details Событие снимается целиком, вместе с местом его в исходном тексте:
	 *          договор о независимости выдачи от нарезки текста на куски требует
	 *          совпадения не только вида и содержимого, но и места
	 *
	 */
	struct Event {
		// Вид собранного события
		json::event_t event;
		// Содержимое собранного события
		string text;
		// Признак изменения содержимого разбором
		bool modified;
		// Смещение события от начала текста
		uint64_t offset;
		// Номер строки события
		uint32_t line;
		// Положение события в строке
		uint32_t column;
		// Глубина вложенности события
		uint32_t depth;
	};
	/**
	 * @brief Итог разбора текста документа
	 *
	 */
	struct Outcome {
		// Собранные события разбора
		vector <Event> events;
		// Код отказа разбора
		json::error_t error;
		// Смещение отказа от начала текста
		uint64_t offset;
		// Номер строки отказа
		uint32_t line;
		// Положение отказа в строке
		uint32_t column;
		/**
		 * @brief Конструктор
		 *
		 */
		Outcome() noexcept : error(json::error_t::NONE), offset(0), line(0), column(0) {}
	};
	/**
	 * @brief Метод записи текста документа в шестнадцатеричном виде
	 *
	 * @details Текст печатается байтами намеренно: испорченный текст содержит знаки,
	 *          какие оболочка съедает либо выводит неразличимо, а по ним и придётся
	 *          повторять найденное расхождение
	 *
	 * @param title заголовок печатаемого текста
	 * @param text  печатаемый текст документа
	 *
	 */
	void dump(const char * title, const string & text) noexcept {
		// Выводим заголовок печатаемого текста
		::fprintf(stderr, "%s (%zu байт):\n", title, text.size());
		/**
		 * Выполняем перебор всех байтов печатаемого текста
		 */
		for(size_t i = 0; i < text.size(); i++){
			// Выводим очередной байт печатаемого текста
			::fprintf(stderr, "%02X ", static_cast <uint8_t> (text[i]));
			/**
			 * Если строка вывода заполнена
			 */
			if(((i + 1) % 32) == 0)
				// Выполняем перевод строки вывода
				::fprintf(stderr, "\n");
		}
		// Выполняем перевод строки вывода
		::fprintf(stderr, "\n");
	}
	/**
	 * @brief Метод записи настроек разбора текста документа
	 *
	 * @param settings печатаемые настройки разбора
	 *
	 */
	void dump(const json::document_t::settings_t & settings) noexcept {
		// Выводим настройки разбора текста документа
		::fprintf(stderr,
			"настройки: strict=%d comments=%d emit=%d commas=%d quotes=%d infnan=%d stream=%d duplicates=%u numbers=%u\n",
			static_cast <int> (settings.reader.strict), static_cast <int> (settings.reader.allowComments),
			static_cast <int> (settings.reader.emitComments), static_cast <int> (settings.reader.allowTrailingCommas),
			static_cast <int> (settings.reader.allowSingleQuotes), static_cast <int> (settings.reader.allowInfinityAndNan),
			static_cast <int> (settings.reader.stream), static_cast <uint32_t> (settings.duplicates),
			static_cast <uint32_t> (settings.numbers));
	}
	/**
	 * @brief Метод построения строкового значения
	 *
	 * @details Строка собирается из знаков, разбор задевающих: кавычки, знак отмены,
	 *          управляющие знаки, записи \\uXXXX вместе с суррогатными парами и знаки
	 *          кодировки UTF-8 всех четырёх длин
	 *
	 * @param engine источник случайных чисел
	 * @return       собранное строковое значение вместе с кавычками
	 *
	 */
	string quoted(mt19937_64 & engine) noexcept {
		// Собираемое строковое значение
		string result(1, '"');
		// Получаем количество знаков строкового значения
		const size_t count = (engine() % 12);
		/**
		 * Выполняем сборку строкового значения
		 */
		for(size_t i = 0; i < count; i++){
			/**
			 * Определяем вид очередного знака строкового значения
			 */
			switch(engine() % 10){
				// Если знак является буквой латиницы
				case 0: case 1: case 2:
					// Выполняем добавление буквы латиницы
					result.push_back(static_cast <char> ('a' + (engine() % 26)));
				break;
				// Если знак является отменяющей последовательностью
				case 3: {
					// Отменяемые знаки, каких стандарт знает восемь
					static const char * escapes = "\"\\/bfnrt";
					// Выполняем добавление знака отмены
					result.push_back('\\');
					// Выполняем добавление отменяемого знака
					result.push_back(escapes[engine() % 8]);
				} break;
				// Если знак является записью Юникода
				case 4: {
					// Хранилище записи знака Юникода
					char buffer[8];
					// Собираем запись знака Юникода
					::snprintf(buffer, sizeof(buffer), "\\u%04X", static_cast <uint32_t> (engine() % 0x10000));
					// Выполняем добавление записи знака Юникода
					result.append(buffer);
				} break;
				// Если знак является суррогатной парой
				case 5: {
					// Хранилище записи суррогатной пары
					char buffer[16];
					// Собираем запись суррогатной пары
					::snprintf(buffer, sizeof(buffer), "\\u%04X\\u%04X",
						static_cast <uint32_t> (0xD800 + (engine() % 0x400)),
						static_cast <uint32_t> (0xDC00 + (engine() % 0x400)));
					// Выполняем добавление записи суррогатной пары
					result.append(buffer);
				} break;
				// Если знак записан двумя байтами кодировки UTF-8
				case 6:
					// Выполняем добавление знака кириллицы
					result.append("\xD0\xB0");
				break;
				// Если знак записан тремя байтами кодировки UTF-8
				case 7:
					// Выполняем добавление знака валюты
					result.append("\xE2\x82\xAC");
				break;
				// Если знак записан четырьмя байтами кодировки UTF-8
				case 8:
					// Выполняем добавление знака вне основной плоскости
					result.append("\xF0\x9F\x98\x80");
				break;
				// Если знак является пробельным
				default:
					// Выполняем добавление пробельного знака
					result.push_back(' ');
			}
		}
		// Выполняем закрытие строкового значения
		result.push_back('"');
		// Выводим собранное строковое значение
		return result;
	}
	/**
	 * @brief Метод построения записи числа
	 *
	 * @param engine источник случайных чисел
	 * @return       собранная запись числа
	 *
	 */
	string numeric(mt19937_64 & engine) noexcept {
		// Хранилище собираемой записи числа
		char buffer[64];
		/**
		 * Определяем вид собираемой записи числа
		 */
		switch(engine() % 8){
			// Если число является целым
			case 0:
				// Собираем запись целого числа
				::snprintf(buffer, sizeof(buffer), "%d", static_cast <int32_t> (engine() % 100000));
			break;
			// Если число является отрицательным целым
			case 1:
				// Собираем запись отрицательного целого числа
				::snprintf(buffer, sizeof(buffer), "-%d", static_cast <int32_t> (engine() % 100000));
			break;
			// Если число является дробным
			case 2: {
				/**
				 * Получаем целую часть дробного числа
				 *
				 * @note Всякий забор случайного числа снимается отдельной переменной
				 *       намеренно: порядок вычисления доводов вызова язык не задаёт, и
				 *       два забора в одном вызове дают у разных собирателей разные
				 *       тексты. Ворошитель тем переставал быть воспроизводимым между
				 *       системами, а сличение счётчиков - доводом
				 */
				const int32_t whole = static_cast <int32_t> (engine() % 1000);
				// Получаем дробную часть числа
				const int32_t fraction = static_cast <int32_t> (engine() % 1000);
				// Собираем запись дробного числа
				::snprintf(buffer, sizeof(buffer), "%d.%d", whole, fraction);
			} break;
			// Если число записано с порядком
			case 3: {
				// Получаем целую часть числа с порядком
				const int32_t whole = static_cast <int32_t> (engine() % 100);
				// Получаем дробную часть числа с порядком
				const int32_t fraction = static_cast <int32_t> (engine() % 100);
				// Получаем знак порядка числа
				const char * sign = (((engine() % 2) != 0) ? "-" : "+");
				// Получаем сам порядок числа
				const int32_t power = static_cast <int32_t> (engine() % 40);
				// Собираем запись числа с порядком
				::snprintf(buffer, sizeof(buffer), "%d.%de%s%d", whole, fraction, sign, power);
			} break;
			// Если число не представимо видом с плавающей запятой
			case 4:
				// Собираем запись непредставимого числа
				::snprintf(buffer, sizeof(buffer), "1e%d", static_cast <int32_t> (300 + (engine() % 200)));
			break;
			// Если число превышает точность вида с плавающей запятой
			case 5:
				// Собираем запись числа, точность превышающего
				::snprintf(buffer, sizeof(buffer), "9007199254740993");
			break;
			// Если число является наибольшим целым без знака
			case 6:
				// Собираем запись наибольшего целого числа без знака
				::snprintf(buffer, sizeof(buffer), "18446744073709551615");
			break;
			// Если число записано длинной чередой цифр
			default: {
				// Собираем запись числа длинной чередой цифр
				size_t length = 0;
				// Выполняем сборку череды цифр
				while(length < (20 + (engine() % 30)))
					// Выполняем добавление очередной цифры
					buffer[length++] = static_cast <char> ('0' + (engine() % 10));
				// Выполняем закрытие записи числа
				buffer[length] = '\0';
			}
		}
		// Выводим собранную запись числа
		return string(buffer);
	}
	/**
	 * @brief Метод построения значения документа
	 *
	 * @param engine источник случайных чисел
	 * @param depth  глубина вложенности собираемого значения
	 * @param result текст документа, к какому дописывается значение
	 *
	 */
	void assemble(mt19937_64 & engine, const json::reader_t::settings_t & settings, const size_t depth, string & result) noexcept {
		/**
		 * Определяем вид собираемого значения
		 */
		/**
		 * Получаем вид собираемого значения
		 *
		 * @note Забор случайного числа снимается переменной прежде выбора: у тернарного
		 *       выбора вычисляется лишь одна ветвь, и число заборов зависело бы от
		 *       глубины вложенности, а с нею расходился бы и весь дальнейший ход
		 */
		const uint64_t choice = engine();
		/**
		 * Определяем вид собираемого значения
		 */
		switch((depth > 5) ? (choice % 5) : (choice % 8)){
			// Если значение является строкой
			case 0:
				// Выполняем добавление строкового значения
				result.append(::quoted(engine));
			break;
			// Если значение является числом
			case 1: case 2:
				// Выполняем добавление записи числа
				result.append(::numeric(engine));
			break;
			/**
			 * Если значение является литералом
			 */
			case 3: {
				// Литералы, какие разбор знает
				static const char * literals[] = {"true", "false", "null", "NaN", "Infinity", "-Infinity"};
				/**
				 * Выполняем добавление литерала
				 *
				 * @note Записи нечисла и бесконечности берутся лишь с дозволения настроек:
				 *       без дозволения текст отвергался бы первым же таким литералом, и
				 *       дерево с перезаписью не проверялись бы вовсе
				 */
				result.append(literals[engine() % (settings.allowInfinityAndNan ? 6 : 3)]);
			} break;
			// Если значение является пустым вместилищем
			case 4:
				// Выполняем добавление пустого вместилища
				result.append(((engine() % 2) != 0) ? "[]" : "{}");
			break;
			/**
			 * Если значение является массивом
			 */
			case 5: case 6: {
				// Получаем количество значений массива
				const size_t count = (1 + (engine() % 4));
				// Выполняем открытие массива
				result.push_back('[');
				/**
				 * Выполняем сборку значений массива
				 */
				for(size_t i = 0; i < count; i++){
					/**
					 * Если значение массива не первое
					 */
					if(i > 0)
						// Выполняем добавление разделителя значений
						result.push_back(',');
					// Выполняем сборку очередного значения массива
					::assemble(engine, settings, (depth + 1), result);
				}
				/**
				 * Если висящая запятая настройками разрешена
				 */
				if(settings.allowTrailingCommas && ((engine() % 4) == 0))
					// Выполняем добавление висящей запятой
					result.push_back(',');
				// Выполняем закрытие массива
				result.push_back(']');
			} break;
			/**
			 * Если значение является объектом
			 */
			default: {
				// Получаем количество полей объекта
				const size_t count = (1 + (engine() % 4));
				// Выполняем открытие объекта
				result.push_back('{');
				/**
				 * Выполняем сборку полей объекта
				 */
				for(size_t i = 0; i < count; i++){
					/**
					 * Если поле объекта не первое
					 */
					if(i > 0)
						// Выполняем добавление разделителя полей
						result.push_back(',');
					/**
					 * Выполняем добавление имени поля объекта
					 *
					 * @note Имена берутся из малого набора намеренно: повтор имени - случай,
					 *       ради какого и заведено правило обращения с повторами, а на именах
					 *       случайных он не выпадал бы вовсе
					 */
					if((engine() % 3) != 0){
						// Хранилище имени поля объекта
						char buffer[16];
						// Собираем имя поля объекта
						::snprintf(buffer, sizeof(buffer), "\"k%d\"", static_cast <int32_t> (engine() % 5));
						// Выполняем добавление имени поля объекта
						result.append(buffer);
					// Выполняем добавление имени поля объекта со знаками, разбор задевающими
					} else result.append(::quoted(engine));
					// Выполняем добавление разделителя имени и значения
					result.push_back(':');
					// Выполняем сборку значения поля объекта
					::assemble(engine, settings, (depth + 1), result);
				}
				/**
				 * Если висящая запятая настройками разрешена
				 */
				if(settings.allowTrailingCommas && ((engine() % 4) == 0))
					// Выполняем добавление висящей запятой
					result.push_back(',');
				// Выполняем закрытие объекта
				result.push_back('}');
			}
		}
	}
	/**
	 * @brief Метод построения текста документа
	 *
	 * @param engine источник случайных чисел
	 * @return       собранный текст документа
	 *
	 */
	string building(mt19937_64 & engine, const json::reader_t::settings_t & settings) noexcept {
		// Собираемый текст документа
		string result;
		/**
		 * Получаем количество документов потока
		 *
		 * @note Поток документов собирается лишь с дозволения настроек: без дозволения
		 *       разбор отвергал бы всё за первым документом
		 */
		const size_t count = (settings.stream ? (1 + (engine() % 3)) : 1);
		/**
		 * Выполняем сборку документов потока
		 */
		for(size_t i = 0; i < count; i++){
			/**
			 * Если документ потока не первый
			 */
			if(i > 0)
				// Выполняем добавление разделителя документов потока
				result.push_back('\n');
			/**
			 * Если текст начинается с примечания
			 */
			if(settings.allowComments && ((engine() % 4) == 0))
				// Выполняем добавление примечания
				result.append(((engine() % 2) != 0) ? "// примечание\n" : "/* примечание */");
			// Выполняем сборку значения документа
			::assemble(engine, settings, 0, result);
			/**
			 * Если текст оканчивается примечанием
			 */
			if(settings.allowComments && ((engine() % 8) == 0))
				// Выполняем добавление примечания
				result.append(" // хвост");
		}
		// Выводим собранный текст документа
		return result;
	}
	/**
	 * @brief Метод точечной порчи текста документа
	 *
	 * @details Порча вносится точечно - заменой, вставкой либо изъятием одного знака:
	 *          текст, испорченный целиком, отвергается первым же знаком, и глубокие
	 *          состояния разбора так не достигаются
	 *
	 * @param engine источник случайных чисел
	 * @param text   портимый текст документа
	 * @return       признак того, что текст был испорчен
	 *
	 */
	bool corrupt(mt19937_64 & engine, string & text) noexcept {
		/**
		 * Если текст документа портить не требуется
		 */
		if(text.empty() || ((engine() % 3) == 0))
			// Выводим признак того, что текст остался нетронутым
			return false;
		// Получаем количество вносимых порч
		const size_t count = (1 + (engine() % 3));
		/**
		 * Выполняем внесение порч в текст документа
		 */
		for(size_t i = 0; (i < count) && !text.empty(); i++){
			// Получаем положение вносимой порчи
			const size_t place = (engine() % text.size());
			/**
			 * Определяем вид вносимой порчи
			 */
			switch(engine() % 4){
				// Если знак заменяется случайным байтом
				case 0:
					// Выполняем замену знака случайным байтом
					text[place] = static_cast <char> (engine() & 0xFF);
				break;
				// Если знак заменяется знаком, разбор задевающим
				case 1: {
					// Знаки, разбор задевающие
					static const char * marks = "{}[]\",:\\'/*.eE+-0x\n\t";
					// Выполняем замену знака знаком, разбор задевающим
					text[place] = marks[engine() % 20];
				} break;
				// Если знак изымается из текста
				case 2:
					// Выполняем изъятие знака из текста
					text.erase(place, 1);
				break;
				// Если в текст вставляется знак
				default:
					// Выполняем вставку знака в текст
					text.insert(place, 1, static_cast <char> (engine() & 0xFF));
			}
		}
		// Выводим признак того, что текст был испорчен
		return true;
	}
	/**
	 * @brief Метод разбора текста документа кусками заданного размера
	 *
	 * @param text     разбираемый текст документа
	 * @param chunk    размер куска, каким подаётся текст, ноль - текст целиком
	 * @param settings настройки разбора текста
	 * @param totals   учёт проделанной работы
	 * @return         итог разбора текста документа
	 *
	 */
	Outcome reading(const string & text, const size_t chunk, const json::reader_t::settings_t & settings, Statistic & totals) noexcept {
		// Итог разбора текста документа
		Outcome result;
		// Объект чтения текста документа
		json::reader_t reader(::logger());
		// Выполняем установку настроек разбора текста
		reader.settings(settings);
		// Получаем размер куска, каким подаётся текст документа
		const size_t size = ((chunk > 0) ? chunk : (text.empty() ? 1 : text.size()));
		/**
		 * Выполняем подачу текста документа кусками заданного размера
		 */
		for(size_t offset = 0; offset <= text.size(); offset += size){
			// Получаем размер очередного подаваемого куска
			const size_t length = (((offset + size) < text.size()) ? size : (text.size() - offset));
			// Выполняем подачу очередного куска текста документа чтению
			const bool ok = reader.feed(text.data() + offset, length, ((offset + length) >= text.size()));
			/**
			 * Если подача куска текста документа завершилась отказом
			 *
			 * @note Место отказа снимается здесь, до вычерпывания очереди событий: метод
			 *       `location()` по договору отдаёт место ТЕКУЩЕГО события, и всякий
			 *       снятый после отказа `next()` его затирает. При подаче кусками очередь
			 *       опустошается до отказа сама собою, а при подаче целиком - нет, и
			 *       снятое после вычерпывания разошлось бы от одной лишь нарезки
			 */
			if(!ok){
				// Запоминаем код отказа разбора
				result.error = reader.error();
				// Запоминаем смещение отказа от начала текста
				result.offset = reader.location().offset;
				// Запоминаем номер строки отказа
				result.line = reader.location().line;
				// Запоминаем положение отказа в строке
				result.column = reader.location().column;
			}
			/**
			 * Выполняем перебор всех собранных событий разбора
			 */
			while(reader.next()){
				// Получаем значение очередного события разбора
				const json::reader_t::value_t value = reader.value();
				// Получаем место очередного события разбора
				const json::location_t place = reader.location();
				// Выполняем добавление собранного события к полученным
				result.events.push_back(Event{
					reader.event(), string(value.text), value.modified,
					place.offset, place.line, place.column, place.depth
				});
				// Увеличиваем счёт собранных событий разбора
				totals.events++;
			}
			/**
			 * Если разбор куска текста документа завершился отказом
			 */
			if(!ok)
				// Прекращаем разбор текста документа
				break;
			/**
			 * Если текст документа исчерпан
			 */
			if((offset + length) >= text.size())
				// Прекращаем разбор текста документа
				break;
		}
		// Выводим итог разбора текста документа
		return result;
	}
	/**
	 * @brief Метод сличения итогов разбора текста документа
	 *
	 * @param sample эталонный итог разбора
	 * @param actual сличаемый итог разбора
	 * @param chunk  размер куска, каким подавался сличаемый текст
	 * @return       признак совпадения итогов разбора
	 *
	 */
	bool matched(const Outcome & sample, const Outcome & actual, const size_t chunk) noexcept {
		/**
		 * Если количество собранных событий разошлось
		 */
		if(sample.events.size() != actual.events.size()){
			// Выводим сообщение о расхождении количества событий
			::fprintf(stderr, "РАСХОЖДЕНИЕ: кусками по %zu собрано %zu событий против %zu\n",
				chunk, actual.events.size(), sample.events.size());
			// Выводим признак расхождения итогов разбора
			return false;
		}
		/**
		 * Выполняем перебор всех собранных событий разбора
		 */
		for(size_t i = 0; i < sample.events.size(); i++){
			// Получаем эталонное событие разбора
			const Event & one = sample.events[i];
			// Получаем сличаемое событие разбора
			const Event & two = actual.events[i];
			/**
			 * Если событие разбора разошлось с эталонным
			 */
			if((one.event != two.event) || (one.text != two.text) || (one.modified != two.modified) ||
			   (one.offset != two.offset) || (one.line != two.line) || (one.column != two.column) || (one.depth != two.depth)){
				// Выводим сообщение о расхождении события разбора
				::fprintf(stderr,
					"РАСХОЖДЕНИЕ: кусками по %zu событие %zu — вид %u против %u, содержимое «%s» против «%s», "
					"изменено %d против %d, место %llu:%u:%u:%u против %llu:%u:%u:%u\n",
					chunk, i, static_cast <uint32_t> (two.event), static_cast <uint32_t> (one.event),
					two.text.c_str(), one.text.c_str(), static_cast <int> (two.modified), static_cast <int> (one.modified),
					static_cast <unsigned long long> (two.offset), two.line, two.column, two.depth,
					static_cast <unsigned long long> (one.offset), one.line, one.column, one.depth);
				// Выводим признак расхождения итогов разбора
				return false;
			}
		}
		/**
		 * Если итог разбора разошёлся с эталонным
		 */
		if((sample.error != actual.error) || (sample.offset != actual.offset) ||
		   (sample.line != actual.line) || (sample.column != actual.column)){
			// Выводим сообщение о расхождении итога разбора
			::fprintf(stderr, "РАСХОЖДЕНИЕ: кусками по %zu отказ «%s» в %llu:%u:%u против «%s» в %llu:%u:%u\n",
				chunk, json::message(actual.error), static_cast <unsigned long long> (actual.offset), actual.line, actual.column,
				json::message(sample.error), static_cast <unsigned long long> (sample.offset), sample.line, sample.column);
			// Выводим признак расхождения итогов разбора
			return false;
		}
		// Выводим признак совпадения итогов разбора
		return true;
	}
	/**
	 * @brief Метод сличения двух деревьев документов
	 *
	 * @param one сличаемое значение первого дерева
	 * @param two сличаемое значение второго дерева
	 * @return    признак совпадения деревьев
	 *
	 */
	bool equal(const json::document_t::value_t & one, const json::document_t::value_t & two) noexcept {
		/**
		 * Если вид значения разошёлся
		 */
		if(one.kind() != two.kind())
			// Выводим признак расхождения деревьев
			return false;
		/**
		 * Если значение является именем поля объекта
		 */
		if(one.name() != two.name())
			// Выводим признак расхождения деревьев
			return false;
		/**
		 * Определяем вид сличаемого значения
		 */
		switch(static_cast <uint8_t> (one.kind())){
			/**
			 * Если значение является вместилищем
			 */
			case static_cast <uint8_t> (json::kind_t::ARRAY):
			case static_cast <uint8_t> (json::kind_t::OBJECT): {
				/**
				 * Если количество детей вместилища разошлось
				 */
				if(one.size() != two.size())
					// Выводим признак расхождения деревьев
					return false;
				// Получаем первое значение первого вместилища
				json::document_t::value_t first = one.begin();
				// Получаем первое значение второго вместилища
				json::document_t::value_t second = two.begin();
				/**
				 * Выполняем перебор всех значений вместилищ
				 */
				while(first.valid() && second.valid()){
					/**
					 * Если значения вместилищ разошлись
					 */
					if(!::equal(first, second))
						// Выводим признак расхождения деревьев
						return false;
					// Выполняем переход к следующему значению первого вместилища
					first = first.next();
					// Выполняем переход к следующему значению второго вместилища
					second = second.next();
				}
				// Выводим признак совпадения детей вместилищ
				return (!first.valid() && !second.valid());
			}
			/**
			 * Если значение является числом
			 *
			 * @note Числа сличаются значением, а не записью: перезапись даёт кратчайшую
			 *       запись числа, и запись «1.0», прочитанная обратно, вернётся как «1»
			 */
			case static_cast <uint8_t> (json::kind_t::NUMBER): {
				// Значение числа первого дерева
				double first = 0.;
				// Значение числа второго дерева
				double second = 0.;
				// Выполняем извлечение числа первого дерева
				const bool ready = one.value(first);
				/**
				 * Если число извлечь не удалось ни у одного из деревьев
				 *
				 * @note Число, вида с плавающей запятой не вмещающееся, извлечению им и
				 *       не поддаётся - таково правило извлечения. Сличать такие числа
				 *       приходится записями, и совпадение отказов извлечения тут само
				 *       по себе значимо
				 */
				if(ready != two.value(second))
					// Выводим признак расхождения деревьев
					return false;
				/**
				 * Если числа извлечению не поддались оба
				 */
				if(!ready)
					// Выводим признак совпадения записей чисел
					return (one.raw() == two.raw());
				/**
				 * Если число не является обычным
				 *
				 * @note Нечисло самому себе не равно по устройству вида с плавающей
				 *       запятой, и сличать его равенством нельзя
				 */
				if(::isnan(first) || ::isnan(second))
					// Выводим признак совпадения, если нечислами являются оба
					return (::isnan(first) && ::isnan(second));
				// Выводим признак совпадения значений чисел
				return (first == second);
			}
			// Если значение является строкой
			case static_cast <uint8_t> (json::kind_t::STRING):
				// Выводим признак совпадения содержимого строк
				return (one.text() == two.text());
			/**
			 * Если значение является логическим
			 */
			case static_cast <uint8_t> (json::kind_t::BOOL): {
				// Логическое значение первого дерева
				bool first = false;
				// Логическое значение второго дерева
				bool second = false;
				// Выполняем извлечение логических значений
				return (one.value(first) && two.value(second) && (first == second));
			}
		}
		// Выводим признак совпадения значений
		return true;
	}
	/**
	 * @brief Метод сборки описи дерева документа
	 *
	 * @details Опись собирается обходом дерева и содержит вид всякого узла, имя поля,
	 *          содержимое строк и значение чисел. Записью документа опись не является:
	 *          сличать пути разбора записью нельзя, ибо запись у нас одна на оба пути,
	 *          и общий её дефект расхождения не дал бы вовсе
	 *
	 * @param value описываемое значение документа
	 * @param result опись, к какой дописывается описанное
	 *
	 */
	void digest(const json::document_t::value_t & value, string & result) noexcept {
		/**
		 * Если значение является полем объекта
		 */
		if(!value.name().empty()){
			// Записываем имя поля объекта
			result.append("K<").append(value.name()).append(">");
		}
		/**
		 * Определяем вид описываемого значения
		 */
		switch(static_cast <uint8_t> (value.kind())){
			// Если значение является пустым
			case static_cast <uint8_t> (json::kind_t::NUL):
				// Записываем пустое значение
				result.append("N;");
			break;
			/**
			 * Если значение является логическим
			 */
			case static_cast <uint8_t> (json::kind_t::BOOL): {
				// Извлекаемое логическое значение
				bool state = false;
				// Записываем логическое значение
				result.append("B<").append((value.value(state) && state) ? "1" : "0").append(">;");
			} break;
			/**
			 * Если значение является числом
			 */
			case static_cast <uint8_t> (json::kind_t::NUMBER): {
				// Хранилище записи числа
				char buffer[64];
				// Извлекаемое значение числа
				double number = 0.;
				// Выполняем извлечение значения числа
				const bool ok = value.value(number);
				// Собираем запись числа
				::snprintf(buffer, sizeof(buffer), "%.17g", (ok ? number : 0.));
				// Записываем значение числа
				result.append("D<").append(buffer).append(">;");
			} break;
			// Если значение является строкой
			case static_cast <uint8_t> (json::kind_t::STRING):
				// Записываем содержимое строки
				result.append("S<").append(value.text()).append(">;");
			break;
			/**
			 * Если значение является вместилищем
			 */
			case static_cast <uint8_t> (json::kind_t::ARRAY):
			case static_cast <uint8_t> (json::kind_t::OBJECT): {
				// Записываем открытие вместилища
				result.append((value.kind() == json::kind_t::ARRAY) ? "[" : "{");
				/**
				 * Выполняем перебор всех значений вместилища
				 */
				for(json::document_t::value_t item = value.begin(); item.valid(); item = item.next())
					// Выполняем описание очередного значения вместилища
					::digest(item, result);
				// Записываем закрытие вместилища
				result.append((value.kind() == json::kind_t::ARRAY) ? "]" : "}");
			} break;
			// Если значение не опознано
			default: result.append("?;");
		}
	}
	/**
	 * @brief Метод проверки кругового хода дерева документа
	 *
	 * @details Собранное дерево записывается обратно и разбирается вновь: деревья
	 *          обязаны совпасть значением всякого узла
	 *
	 * @param document сличаемое дерево документа
	 * @param settings настройки разбора текста документа
	 * @param totals   учёт проделанной работы
	 * @return         признак сохранности кругового хода
	 *
	 */
	bool roundtrip(json::document_t & document, const json::document_t::settings_t & settings, Statistic & totals) noexcept {
		// Выполняем перезапись дерева документа
		const string text = document.dump();
		// Увеличиваем счёт перезаписей дерева документа
		totals.rewrites++;
		// Объект дерева перезаписанного документа
		json::document_t rewritten(::logger());
		// Получаем настройки разбора перезаписанного документа
		json::document_t::settings_t rules = rewritten.settings();
		/**
		 * Устанавливаем разрешение записей нечисла и бесконечности
		 *
		 * @note Разрешение берётся тем же, что и при разборе исходного текста: записанное
		 *       с дозволения нечисло без дозволения обратно не читается, и отказ означал
		 *       бы здесь несогласованность настроек, а не дефект
		 */
		rules.reader.allowInfinityAndNan = settings.reader.allowInfinityAndNan;
		/**
		 * Устанавливаем удержание повторяющихся имён полей объекта
		 *
		 * @note Повторы к этому времени уже разобраны по правилу настроек, и отвергать
		 *       перезаписанное вторично незачем
		 */
		rules.duplicates = json::duplicate_t::KEEP;
		// Выполняем установку настроек разбора перезаписанного документа
		rewritten.settings(rules);
		/**
		 * Если разбор перезаписанного документа завершился отказом
		 */
		if(!rewritten.parse(text)){
			// Выводим сообщение об отказе разбора перезаписанного документа
			::fprintf(stderr, "ПЕРЕЗАПИСЬ НЕ ЧИТАЕТСЯ: «%s» — %s\n", text.c_str(), json::message(rewritten.error()));
			// Выводим признак нарушения кругового хода
			return false;
		}
		/**
		 * Если деревья документов разошлись
		 */
		if(!::equal(document.root(), rewritten.root())){
			// Выводим сообщение о расхождении деревьев документов
			::fprintf(stderr, "РАСХОЖДЕНИЕ КРУГОВОГО ХОДА: «%s»\n", text.c_str());
			// Выводим признак нарушения кругового хода
			return false;
		}
		// Выводим признак сохранности кругового хода
		return true;
	}
	/**
	 * @brief Метод проверки кругового хода чисел через запись
	 *
	 * @details Число записывается писателем и читается обратно документом: запись
	 *          обязана вернуть то же самое число, а не близкое к нему
	 *
	 * @param engine источник случайных чисел
	 * @param totals учёт проделанной работы
	 * @return       признак сохранности кругового хода
	 *
	 */
	bool hostile(mt19937_64 & engine, Statistic & totals) noexcept {
		// Пограничные числа, на каких запись и ломается
		static const double edge[] = {
			0., -0., 1., -1., 0.5, 1e308, 5e-324, 2.2250738585072014e-308,
			9007199254740992., 9007199254740993., 1e-30, 1e30, 100., 1000000., 1e16, 1e17
		};
		// Записываемое число
		double value = 0.;
		/**
		 * Если число берётся пограничным
		 */
		if((engine() % 2) != 0)
			// Получаем пограничное число
			value = edge[engine() % (sizeof(edge) / sizeof(edge[0]))];
		/**
		 * Если число собирается из случайных битов
		 */
		else {
			// Получаем случайные биты числа
			const uint64_t bits = engine();
			// Выполняем сборку числа из случайных битов
			::memcpy(& value, & bits, sizeof(value));
			/**
			 * Если число обычным числом не является
			 *
			 * @note Нечисла и бесконечности стандарт не знает вовсе, и записываются они
			 *       лишь с дозволения настроек: круговой ход их проверяется деревом
			 */
			if(::isnan(value) || ::isinf(value))
				// Выводим признак сохранности кругового хода
				return true;
		}
		// Объект записи текста документа
		json::writer_t writer(::logger());
		// Получаем настройки записи текста документа
		json::writer_t::settings_t rules = writer.settings();
		// Разрешаем запись значения верхнего уровня
		rules.stream = true;
		// Выполняем установку настроек записи текста документа
		writer.settings(rules);
		/**
		 * Если запись числа завершилась отказом
		 */
		if(!writer.value(value)){
			// Выводим сообщение об отказе записи числа
			::fprintf(stderr, "ОТКАЗ ЗАПИСИ ЧИСЛА: %.17g\n", value);
			// Выводим признак нарушения кругового хода
			return false;
		}
		// Объект дерева записанного документа
		json::document_t document(::logger());
		/**
		 * Если разбор записанного числа завершился отказом
		 */
		if(!document.parse(writer.text())){
			// Выводим сообщение об отказе разбора записанного числа
			::fprintf(stderr, "ЗАПИСЬ ЧИСЛА НЕ ЧИТАЕТСЯ: %.17g записано «%s» — %s\n",
				value, writer.text().c_str(), json::message(document.error()));
			// Выводим признак нарушения кругового хода
			return false;
		}
		// Прочитанное обратно число
		double back = 0.;
		/**
		 * Если прочитанное обратно число разошлось с записанным
		 */
		if(!document.root().value(back) || (back != value)){
			// Выводим сообщение о расхождении кругового хода числа
			::fprintf(stderr, "РАСХОЖДЕНИЕ ЧИСЛА: %.17g записано «%s» прочитано %.17g\n",
				value, writer.text().c_str(), back);
			// Выводим признак нарушения кругового хода
			return false;
		}
		// Увеличиваем счёт чисел, прошедших круговой ход записи
		totals.numbers++;
		// Выводим признак сохранности кругового хода
		return true;
	}
	/**
	 * @brief Метод пересборки владеющего значения потоковым сборщиком
	 *
	 * @details Значение обходится узел за узлом, и всякий узел подаётся сборщику тем же
	 *          самым порядком, каким он был бы записан в текст: собранное сборщиком
	 *          обязано совпасть с обходимым
	 *
	 * @param value   пересобираемое владеющее значение
	 * @param builder потоковый сборщик владеющего значения
	 * @return        признак успешности пересборки
	 *
	 */
	bool rebuild(const json::value_t & value, json::builder_t & builder) noexcept {
		/**
		 * Определяем вид пересобираемого значения
		 */
		switch(static_cast <uint8_t> (value.kind())){
			// Если значение является объектом
			case static_cast <uint8_t> (json::kind_t::OBJECT): {
				/**
				 * Если открытие объекта завершилось отказом
				 */
				if(!builder.object())
					// Выводим признак неудачной пересборки
					return false;
				/**
				 * Выполняем перебор полей объекта
				 */
				for(size_t i = 0; i < value.size(); i++){
					/**
					 * Если подача имени поля объекта завершилась отказом
					 *
					 * @note Имя подаётся добавлением, а не установкой: объект вправе нести
					 *       повторяющиеся имена по настройке `duplicate_t::KEEP`, и установка
					 *       свела бы повторы в одно поле
					 */
					if(!builder.append(value.key(i)))
						// Выводим признак неудачной пересборки
						return false;
					/**
					 * Если пересборка значения поля объекта завершилась отказом
					 */
					if(!::rebuild(value[i], builder))
						// Выводим признак неудачной пересборки
						return false;
				}
				// Выводим итог закрытия объекта
				return builder.close();
			}
			// Если значение является массивом
			case static_cast <uint8_t> (json::kind_t::ARRAY): {
				/**
				 * Если открытие массива завершилось отказом
				 */
				if(!builder.array())
					// Выводим признак неудачной пересборки
					return false;
				/**
				 * Выполняем перебор элементов массива
				 */
				for(size_t i = 0; i < value.size(); i++){
					/**
					 * Если пересборка элемента массива завершилась отказом
					 */
					if(!::rebuild(value[i], builder))
						// Выводим признак неудачной пересборки
						return false;
				}
				// Выводим итог закрытия массива
				return builder.close();
			}
			// Если значение является пустым
			case static_cast <uint8_t> (json::kind_t::NUL):
				// Выводим итог подачи пустого значения
				return builder.null();
			// Если значение является логическим
			case static_cast <uint8_t> (json::kind_t::BOOL): {
				// Извлекаемое логическое значение
				bool result = false;
				/**
				 * Если извлечение логического значения завершилось отказом
				 */
				if(!value.value(result))
					// Выводим признак неудачной пересборки
					return false;
				// Выводим итог подачи логического значения
				return builder.value(result);
			}
			// Если значение является строкой
			case static_cast <uint8_t> (json::kind_t::STRING):
				// Выводим итог подачи строкового значения
				return builder.value(value.text());
			// Если значение является числом
			case static_cast <uint8_t> (json::kind_t::NUMBER): {
				/**
				 * Если число со знаком
				 */
				if(value.is(json::type_t::SIGNED)){
					// Извлекаемое число со знаком
					int64_t result = 0;
					/**
					 * Если извлечение числа со знаком завершилось отказом
					 */
					if(!value.value(result))
						// Выводим признак неудачной пересборки
						return false;
					// Выводим итог подачи числа со знаком
					return builder.value(result);
				/**
				 * Если число без знака
				 */
				} else if(value.is(json::type_t::UNSIGNED)) {
					// Извлекаемое число без знака
					uint64_t result = 0;
					/**
					 * Если извлечение числа без знака завершилось отказом
					 */
					if(!value.value(result))
						// Выводим признак неудачной пересборки
						return false;
					// Выводим итог подачи числа без знака
					return builder.value(result);
				/**
				 * Если число дробное
				 */
				} else if(value.is(json::type_t::REAL)) {
					// Извлекаемое дробное число
					double result = 0.;
					/**
					 * Если извлечение дробного числа завершилось отказом
					 */
					if(!value.value(result))
						// Выводим признак неудачной пересборки
						return false;
					// Выводим итог подачи дробного числа
					return builder.value(result);
				}
				/**
				 * Выводим итог подачи значения целиком
				 *
				 * @note Число, ни в один родной вид не вместимое, хранится записью, и
				 *       подавать его надлежит значением целиком: всякое извлечение его
				 *       в родной вид потеряло бы точность записи
				 */
				return builder.value(value);
			}
		}
		// Выводим признак неудачной пересборки
		return false;
	}
	/**
	 * @brief Метод проверки кругового хода владеющего значения
	 *
	 * @details Значение снимается с дерева документа и проверяется тремя ходами:
	 *          пересборкою потоковым сборщиком, прививкою обратно в дерево и записью
	 *          в текст с последующим разбором
	 *
	 * @param document дерево документа, с какого снимается значение
	 * @param settings настройки разбора текста документа
	 * @param totals   учёт проделанной работы
	 * @return         признак сохранности кругового хода
	 *
	 */
	bool owning(json::document_t & document, const json::document_t::settings_t & settings, Statistic & totals) noexcept {
		// Выполняем снятие владеющего значения с дерева документа
		const json::value_t value(document.root());
		// Увеличиваем счёт снятых владеющих значений
		totals.values++;
		/**
		 * Если снятое значение недействительно
		 */
		if(!value.valid())
			// Выводим признак сохранности кругового хода
			return true;
		// Объект потокового сборщика владеющего значения
		json::builder_t builder(::logger());
		/**
		 * Если пересборка значения потоковым сборщиком завершилась отказом
		 */
		if(!::rebuild(value, builder)){
			// Выводим сообщение об отказе пересборки значения
			::fprintf(stderr, "ЗНАЧЕНИЕ НЕ ПЕРЕСОБИРАЕТСЯ: «%s»\n", document.dump().c_str());
			// Выводим признак нарушения кругового хода
			return false;
		}
		// Увеличиваем счёт пересобранных значений
		totals.builds++;
		/**
		 * Если пересобранное значение разошлось со снятым
		 */
		if(!(builder.finish() == value)){
			// Выводим сообщение о расхождении пересобранного значения со снятым
			::fprintf(stderr, "РАСХОЖДЕНИЕ ПЕРЕСБОРКИ: «%s»\n", document.dump().c_str());
			// Выводим признак нарушения кругового хода
			return false;
		}
		/**
		 * Объект дерева документа, принимающего прививку
		 *
		 * @note Прививаемое место обязано существовать: прививка заменяет поддерево,
		 *       а не заводит новое поле, — оттого поле в принимающем дереве и заведено
		 */
		json::document_t host(::logger());
		/**
		 * Если заведение принимающего прививку дерева завершилось успехом
		 */
		if(host.parse("{\"graft\":null}")){
			/**
			 * Если прививка значения в дерево документа завершилась отказом
			 */
			if(!host.graft("/graft", value)){
				// Выводим сообщение об отказе прививки значения
				::fprintf(stderr, "ЗНАЧЕНИЕ НЕ ПРИВИВАЕТСЯ: «%s»\n", document.dump().c_str());
				// Выводим признак нарушения кругового хода
				return false;
			}
			// Увеличиваем счёт привитых значений
			totals.grafts++;
			/**
			 * Если снятое с привитого места значение разошлось с привитым
			 */
			if(!(json::value_t(host.at("/graft")) == value)){
				// Выводим сообщение о расхождении привитого значения с исходным
				::fprintf(stderr, "РАСХОЖДЕНИЕ ПРИВИВКИ: «%s»\n", document.dump().c_str());
				// Выводим признак нарушения кругового хода
				return false;
			}
			/**
			 * Выполняем сброс привитого значения по указателю
			 *
			 * @note Сброс обязан оставить поле на месте, заместив содержимое его пустым
			 *       значением: место сохраняется, а значение при нём становится `null`
			 */
			if(!host.reset("/graft")){
				// Выводим сообщение об отказе сброса значения
				::fprintf(stderr, "ЗНАЧЕНИЕ НЕ СБРАСЫВАЕТСЯ: «%s»\n", document.dump().c_str());
				// Выводим признак нарушения кругового хода
				return false;
			}
			// Увеличиваем счёт сброшенных значений
			totals.resets++;
			/**
			 * Если сброшенное значение пустым не стало либо место его исчезло
			 */
			if(!host.has("/graft") || !host.at("/graft").is(json::type_t::NUL) || (host.root().size() != 1)){
				// Выводим сообщение о расхождении сброшенного значения с пустым
				::fprintf(stderr, "РАСХОЖДЕНИЕ СБРОСА: «%s»\n", host.dump().c_str());
				// Выводим признак нарушения кругового хода
				return false;
			}
			/**
			 * Выполняем снос сброшенного значения по указателю
			 *
			 * @note Снос обязан убрать само место: поле объекта исчезает целиком, и
			 *       опрос наличия его отвечает отсутствием - тем он и отличен от сброса
			 */
			if(!host.erase("/graft")){
				// Выводим сообщение об отказе сноса значения
				::fprintf(stderr, "ЗНАЧЕНИЕ НЕ СНОСИТСЯ: «%s»\n", document.dump().c_str());
				// Выводим признак нарушения кругового хода
				return false;
			}
			// Увеличиваем счёт снесённых значений
			totals.removals++;
			/**
			 * Если снесённое место в дереве уцелело
			 *
			 * @note Сличается и СЧЁТ детей вместилища, а не одна лишь запись его: подмена,
			 *       снявшая убавление счёта при сносе, записи НЕ меняет вовсе - обход
			 *       детей идёт по размаху, а не по счёту, - и проверка по одной записи
			 *       её пропускала. Замер щупом: массив о двух значениях отчитывался
			 *       тремя, а объект о нуле полей - одним
			 */
			if(host.has("/graft") || (host.dump().compare("{}") != 0) || (host.root().size() != 0)){
				// Выводим сообщение о расхождении сноса
				::fprintf(stderr, "РАСХОЖДЕНИЕ СНОСА: «%s»\n", host.dump().c_str());
				// Выводим признак нарушения кругового хода
				return false;
			}
		}
		// Выполняем запись владеющего значения в текст
		const string text = value.dump();
		/**
		 * Если запись значения в текст завершилась отказом
		 *
		 * @note Отказ этот законен: поток записи отвергает нечисло и бесконечность,
		 *       разбором дозволенные, и пустой итог означает здесь отказ, а не усечение
		 */
		if(text.empty())
			// Выводим признак сохранности кругового хода
			return true;
		// Объект дерева документа, записанного значением
		json::document_t written(::logger());
		// Получаем настройки разбора записанного значением документа
		json::document_t::settings_t rules = written.settings();
		// Устанавливаем разрешение записей нечисла и бесконечности
		rules.reader.allowInfinityAndNan = settings.reader.allowInfinityAndNan;
		// Устанавливаем удержание повторяющихся имён полей объекта
		rules.duplicates = json::duplicate_t::KEEP;
		// Выполняем установку настроек разбора записанного значением документа
		written.settings(rules);
		/**
		 * Если разбор записанного значением документа завершился отказом
		 */
		if(!written.parse(text)){
			// Выводим сообщение об отказе разбора записанного значением документа
			::fprintf(stderr, "ЗАПИСЬ ЗНАЧЕНИЯ НЕ ЧИТАЕТСЯ: «%s» — %s\n", text.c_str(), json::message(written.error()));
			// Выводим признак нарушения кругового хода
			return false;
		}
		/**
		 * Если снятое с записи значение разошлось с исходным
		 */
		if(!(json::value_t(written.root()) == value)){
			// Выводим сообщение о расхождении снятого с записи значения с исходным
			::fprintf(stderr, "РАСХОЖДЕНИЕ ЗАПИСИ ЗНАЧЕНИЯ: «%s»\n", text.c_str());
			// Выводим признак нарушения кругового хода
			return false;
		}
		// Выводим признак сохранности кругового хода
		return true;
	}
	/**
	 * @brief Метод проверки потоковой выдачи документов
	 *
	 * @details Выдача обработчику обязана давать те же документы, что и сборка целиком:
	 *          путь этот проходит сборку иначе - хранилище знаков очищается на всяком
	 *          документе, а дерево заводится заново
	 *
	 * @param text     разбираемый текст документа
	 * @param settings настройки разбора текста документа
	 * @param totals   учёт проделанной работы
	 * @return         признак совпадения потоковой выдачи со сборкой целиком
	 *
	 */
	bool streaming(const string & text, const json::document_t::settings_t & settings, Statistic & totals) noexcept {
		// Описи документов, собранных потоковой выдачей
		vector <string> streamed;
		// Объект дерева документа
		json::document_t document(::logger());
		// Выполняем установку настроек разбора текста документа
		document.settings(settings);
		// Выполняем разбор текста документа с потоковой выдачей
		const bool ok = document.parse(text, [&streamed](const json::document_t::value_t & value) noexcept -> bool {
			// Собираемая опись очередного документа
			string item;
			// Выполняем сборку описи очередного документа
			::digest(value, item);
			// Выполняем добавление описи очередного документа к собранным
			streamed.push_back(item);
			// Выводим признак продолжения разбора
			return true;
		});
		// Увеличиваем счёт потоковых прогонов
		totals.streams++;
		/**
		 * Если разбор текста документа завершился отказом
		 *
		 * @note Отказ здесь дефектом не является: текст испорчен намеренно, и отвергнуть
		 *       его - дело правильное. Значение имеет лишь согласие двух путей разбора
		 */
		if(!ok)
			// Выводим признак совпадения путей разбора
			return true;
		// Объект дерева документа, собираемого целиком
		json::document_t whole(::logger());
		// Выполняем установку настроек разбора текста документа
		whole.settings(settings);
		// Получаем признак успешности сборки текста документа целиком
		const bool built = whole.parse(text);
		/**
		 * Если текст несёт более одного документа
		 *
		 * @details Дерево вмещает документ ОДИН, и сборка целиком обязана отвергнуть
		 * поток о нескольких: прежде она удерживала первый документ, а прочие пропадали
		 * молча - ни кода отказа, ни признака потери. Пути разбора здесь расходятся
		 * НАМЕРЕННО, и расходятся они не в согласии, а в возможностях
		 *
		 * @note Прежнее примечание здесь гласило, что сборка целиком удерживает документ
		 *       ПОСЛЕДНИЙ. Утверждение это было неверным: удерживался первый. Ещё один
		 *       довод в пользу того, что записанное убеждение поведения не заменяет
		 */
		if(streamed.size() > 1){
			/**
			 * Если сборка целиком поток о нескольких документах приняла
			 */
			if(built){
				// Выводим сообщение о принятом потоке о нескольких документах
				::fprintf(stderr, "РАСХОЖДЕНИЕ ПУТЕЙ: сборка целиком приняла поток о %zu документах\n", streamed.size());
				// Выводим признак расхождения путей разбора
				return false;
			}
			/**
			 * Если отказ вынесен не тем кодом
			 */
			if(whole.error() != json::error_t::TRAILING_CHARACTERS){
				// Выводим сообщение о неверном коде отказа
				::fprintf(stderr, "РАСХОЖДЕНИЕ ПУТЕЙ: поток о нескольких документах отвергнут кодом «%s»\n",
					json::message(whole.error()));
				// Выводим признак расхождения путей разбора
				return false;
			}
			// Выводим признак совпадения путей разбора
			return true;
		}
		/**
		 * Если разбор текста документа целиком завершился отказом
		 */
		if(!built){
			// Выводим сообщение о расхождении путей разбора
			::fprintf(stderr, "РАСХОЖДЕНИЕ ПУТЕЙ: потоковая выдача приняла текст, а сборка целиком отвергла — %s\n",
				json::message(whole.error()));
			// Выводим признак расхождения путей разбора
			return false;
		}
		/**
		 * Если текст содержит один документ
		 */
		if(streamed.size() == 1){
			// Опись документа, собранного целиком
			string sample;
			// Выполняем сборку описи документа, собранного целиком
			::digest(whole.root(), sample);
			/**
			 * Если записи документов разошлись
			 */
			if(streamed.front() != sample){
				// Выводим сообщение о расхождении описей документов
				::fprintf(stderr, "РАСХОЖДЕНИЕ ПУТЕЙ: потоком «%s», целиком «%s»\n",
					streamed.front().c_str(), sample.c_str());
				// Выводим признак расхождения путей разбора
				return false;
			}
		}
		// Выводим признак совпадения путей разбора
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
int main(int argc, char * argv[]) noexcept {
	// Получаем количество проходов генератора
	const uint64_t count = ((argc > 1) ? static_cast <uint64_t> (::atoll(argv[1])) : 3000);
	// Получаем зерно источника случайных чисел
	const uint64_t seed = ((argc > 2) ? static_cast <uint64_t> (::atoll(argv[2])) : 20260815);
	// Источник случайных чисел
	mt19937_64 engine(seed);
	// Учёт проделанной работы
	Statistic totals;
	/**
	 * Выполняем проходы генератора
	 */
	for(uint64_t pass = 0; pass < count; pass++){
		/**
		 * Настройки разбора текста документа
		 *
		 * @note Настройки выбираются ПРЕЖДЕ сборки текста намеренно: собранный без
		 *       оглядки на них текст отвергался бы первым же запрещённым построением,
		 *       и ни дерево, ни перезапись, ни потоковая выдача не проверялись бы вовсе
		 */
		json::document_t::settings_t settings;
		// Устанавливаем признак строгого следования стандарту
		settings.reader.strict = ((engine() % 4) == 0);
		// Устанавливаем разрешение примечаний
		settings.reader.allowComments = ((engine() % 2) != 0);
		// Устанавливаем затребование выдачи примечаний
		settings.reader.emitComments = ((engine() % 2) != 0);
		// Устанавливаем разрешение висящей запятой
		settings.reader.allowTrailingCommas = ((engine() % 2) != 0);
		// Устанавливаем разрешение одинарных кавычек
		settings.reader.allowSingleQuotes = ((engine() % 2) != 0);
		// Устанавливаем разрешение записей нечисла и бесконечности
		settings.reader.allowInfinityAndNan = ((engine() % 2) != 0);
		// Устанавливаем разрешение разбора потока документов
		settings.reader.stream = ((engine() % 2) != 0);
		// Устанавливаем правило обращения с повторяющимся именем поля объекта
		settings.duplicates = DUPLICATES[engine() % (sizeof(DUPLICATES) / sizeof(DUPLICATES[0]))];
		// Устанавливаем правило преобразования чисел
		settings.numbers = NUMBERS[engine() % (sizeof(NUMBERS) / sizeof(NUMBERS[0]))];
		/**
		 * Устанавливаем настройки записи текста документа
		 *
		 * @note Перебирались прежде ОДНИ настройки разбора: перезапись дерева шла всегда
		 *       умолчаниями, и ни нарядное оформление, ни способы экранирования сверх
		 *       предписанного стандартом не ворошились ни разу
		 */
		settings.writer.format = FORMATS[engine() % (sizeof(FORMATS) / sizeof(FORMATS[0]))];
		// Устанавливаем способ экранирования при записи
		settings.writer.escape = ESCAPES[engine() % (sizeof(ESCAPES) / sizeof(ESCAPES[0]))];
		// Устанавливаем ширину отступа нарядного оформления
		settings.writer.indent = static_cast <uint8_t> (engine() % 5);
		/**
		 * Устанавливаем разрешение записей нечисла и бесконечности
		 *
		 * @note Разрешение берётся тем же, что и у разбора: записанное с дозволения
		 *       нечисло без дозволения обратно не читается, и расхождение означало бы
		 *       здесь несогласованность настроек, а не дефект
		 */
		settings.writer.allowInfinityAndNan = settings.reader.allowInfinityAndNan;
		// Выполняем сборку текста документа
		string text = ::building(engine, settings.reader);
		// Увеличиваем счёт построенных текстов документов
		totals.texts++;
		/**
		 * Если текст документа был испорчен
		 */
		if(::corrupt(engine, text))
			// Увеличиваем счёт испорченных текстов документов
			totals.corrupted++;
		// Выполняем разбор текста документа целиком
		const Outcome sample = ::reading(text, 0, settings.reader, totals);
		/**
		 * Выполняем перебор размеров куска, каким подаётся текст документа
		 *
		 * @note Первый размер всегда однобайтовый: нарезка по одному знаку разрывает
		 *       всякую последовательность, какую разбор мог бы прочесть заглядыванием
		 *       вперёд, и договор о независимости выдачи проверяется ею строже всего
		 */
		for(size_t i = 0; i < 3; i++){
			// Получаем размер куска, каким подаётся текст документа
			const size_t chunk = ((i == 0) ? 1 : (1 + (engine() % 16)));
			// Выполняем разбор текста документа кусками
			const Outcome actual = ::reading(text, chunk, settings.reader, totals);
			/**
			 * Если итог разбора кусками разошёлся с эталонным
			 */
			if(!::matched(sample, actual, chunk)){
				// Выводим настройки разбора текста документа
				::dump(settings);
				// Выводим разбираемый текст документа
				::dump("текст документа", text);
				// Выходим из приложения с кодом ошибки
				return EXIT_FAILURE;
			}
		}
		// Объект дерева документа
		json::document_t document(::logger());
		// Выполняем установку настроек разбора текста документа
		document.settings(settings);
		/**
		 * Если сборка дерева документа завершилась успехом
		 */
		if(document.parse(text)){
			// Увеличиваем счёт текстов, разбор переживших
			totals.survived++;
			// Увеличиваем счёт собранных деревьев документов
			totals.documents++;
			/**
			 * Если круговой ход дерева документа нарушен
			 */
			if(!::roundtrip(document, settings, totals)){
				// Выводим настройки разбора текста документа
				::dump(settings);
				// Выводим разбираемый текст документа
				::dump("текст документа", text);
				// Выходим из приложения с кодом ошибки
				return EXIT_FAILURE;
			}
			/**
			 * Если круговой ход владеющего значения нарушен
			 */
			if(!::owning(document, settings, totals)){
				// Выводим настройки разбора текста документа
				::dump(settings);
				// Выводим разбираемый текст документа
				::dump("текст документа", text);
				// Выходим из приложения с кодом ошибки
				return EXIT_FAILURE;
			}
		}
		/**
		 * Если потоковая выдача документов разошлась со сборкой целиком
		 */
		if(!::streaming(text, settings, totals)){
			// Выводим настройки разбора текста документа
			::dump(settings);
			// Выводим разбираемый текст документа
			::dump("текст документа", text);
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
		}
		/**
		 * Если круговой ход числа через запись нарушен
		 */
		if(!::hostile(engine, totals))
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
	}
	// Выводим итог работы генератора
	::fprintf(stderr, "json fuzz: %llu passes, %llu texts (%llu corrupted), %llu survived, "
		"%llu events, %llu documents, %llu rewrites, %llu streams, %llu numbers, "
		"%llu values, %llu builds, %llu grafts, %llu resets, %llu removals\n",
		static_cast <unsigned long long> (count), static_cast <unsigned long long> (totals.texts),
		static_cast <unsigned long long> (totals.corrupted), static_cast <unsigned long long> (totals.survived),
		static_cast <unsigned long long> (totals.events), static_cast <unsigned long long> (totals.documents),
		static_cast <unsigned long long> (totals.rewrites), static_cast <unsigned long long> (totals.streams),
		static_cast <unsigned long long> (totals.numbers), static_cast <unsigned long long> (totals.values),
		static_cast <unsigned long long> (totals.builds), static_cast <unsigned long long> (totals.grafts),
		static_cast <unsigned long long> (totals.resets), static_cast <unsigned long long> (totals.removals));
	// Выходим из приложения
	return EXIT_SUCCESS;
}
