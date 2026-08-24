/**
 * @file csv.cpp
 * @date 2026-08-13
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
 * @brief Инструмент фаззинга кодека таблиц CSV — построение полуструктурированной таблицы
 *        с точечной порчей, подача её чтению целиком и кусками произвольного размера,
 *        сборка таблицы и её перезапись для поиска аварийных завершений, выходов за
 *        границы буфера и расхождений разбора
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
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
#include <codec/csv/csv.hpp>
#include <sys/log.hpp>

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
	 * @brief Учёт проделанной работы
	 *
	 */
	struct Statistic {
		// Количество построенных таблиц
		uint64_t texts;
		// Количество испорченных таблиц
		uint64_t corrupted;
		// Количество таблиц, разобранных чтением до конца
		uint64_t survived;
		// Количество выданных чтением событий
		uint64_t events;
		// Количество собранных таблиц контейнера
		uint64_t documents;
		// Количество выполненных перезаписей таблицы
		uint64_t rewrites;
		// Количество выполненных потоковых выдач записей
		uint64_t streams;
		// Количество таблиц, собранных договором правки
		uint64_t assembled;
		/**
		 * @brief Конструктор
		 *
		 */
		Statistic() noexcept :
		 texts(0), corrupted(0), survived(0), events(0), documents(0), rewrites(0), streams(0),
		 assembled(0) {}
	} totals;

	/**
	 * @brief Событие разбора, запомненное для сличения
	 *
	 * @details Сличается вся выдача разбора целиком: расхождение хотя бы одного признака
	 * означает зависимость разбора от нарезки исходного текста на куски - договор, какой
	 * потоковое чтение нарушает легче всего
	 *
	 */
	struct Event {
		// Разновидность события разбора
		uint8_t event;
		// Содержимое поля события
		string value;
		// Имя столбца, к какому поле относится
		string name;
		// Признаки заключения поля в кавычки и изменения его содержимого разбором
		bool quoted, modified;
		// Место начала события в исходном тексте
		uint32_t line, column;
		// Номера записи и поля в записи
		uint32_t record, field;
		// Смещение начала события от начала исходного текста
		uint64_t position;
		/**
		 * @brief Конструктор
		 *
		 */
		Event() noexcept :
		 event(0), quoted(false), modified(false),
		 line(0), column(0), record(0), field(0), position(0) {}
	};

	/**
	 * @brief Метод вывода содержимого с отменой непечатаемых знаков
	 *
	 * @details Таблица, доведшая разбор до расхождения, обязана попасть в отчёт
	 * пригодной к повторению: знаки конца строки и кавычки в ней значащие, и вывод их
	 * как есть сделал бы отчёт неразбираемым
	 *
	 * @param text выводимое содержимое
	 *
	 */
	void dump(const string & text) noexcept {
		// Выводим начало содержимого
		::fputs("  <<", stderr);
		/**
		 * Выполняем перебор всех знаков выводимого содержимого
		 */
		for(const unsigned char letter : text){
			/**
			 * Определяем выводимый знак
			 */
			switch(letter){
				// Если знаком является перевод строки
				case '\n': ::fputs("\\n", stderr); break;
				// Если знаком является возврат каретки
				case '\r': ::fputs("\\r", stderr); break;
				// Если знаком является горизонтальная табуляция
				case '\t': ::fputs("\\t", stderr); break;
				// Если знаком является обратная косая черта
				case '\\': ::fputs("\\\\", stderr); break;
				/**
				 * Если знаком является любой другой
				 */
				default: {
					/**
					 * Если знак является печатаемым
					 */
					if((letter >= 0x20) && (letter < 0x7F))
						// Выводим знак как есть
						::fputc(static_cast <char> (letter), stderr);
					// Выводим кодовое значение знака
					else ::fprintf(stderr, "\\x%02X", static_cast <uint32_t> (letter));
				}
			}
		}
		// Выводим конец содержимого
		::fputs(">>\n", stderr);
	}
	/**
	 * @brief Метод вывода настроек разбора таблицы
	 *
	 * @param settings выводимые настройки разбора таблицы
	 *
	 */
	void dump(const csv::reader_t::settings_t & settings) noexcept {
		// Выводим настройки разбора таблицы
		::fprintf(stderr, "  settings: separator=%d quote=%d comment=%d escape=%u header=%u trim=%u ragged=%u\n"
			"            strict=%u blanks=%u comments=%u duplicates=%u field=%u record=%u fields=%u detect=%u\n",
			static_cast <int32_t> (settings.separator), static_cast <int32_t> (settings.quote),
			static_cast <int32_t> (settings.comment), static_cast <uint32_t> (settings.escape),
			static_cast <uint32_t> (settings.header), static_cast <uint32_t> (settings.trim),
			static_cast <uint32_t> (settings.ragged), static_cast <uint32_t> (settings.strict),
			static_cast <uint32_t> (settings.emitBlanks), static_cast <uint32_t> (settings.emitComments),
			static_cast <uint32_t> (settings.duplicates), settings.maxField, settings.maxRecord,
			settings.maxFields, settings.detect);
	}
	/**
	 * @brief Метод подачи таблицы чтению и сбора выданных им событий
	 *
	 * @param text     подаваемая таблица
	 * @param settings настройки разбора таблицы
	 * @param chunk    размер куска подачи, ноль - подать таблицу целиком
	 * @param events   перечень собранных событий разбора
	 * @param position место отказа разбора
	 * @return         код ошибки разбора таблицы
	 *
	 */
	csv::error_t consume(const string & text, const csv::reader_t::settings_t & settings, const size_t chunk, vector <Event> & events, csv::location_t * position) noexcept {
		// Чтение таблицы
		csv::reader_t reader(::logger(), settings);
		// Смещение от начала таблицы
		size_t offset = 0;
		/**
		 * Выполняем подачу таблицы кусками заданного размера
		 */
		do {
			// Получаем размер очередного куска подачи
			const size_t size = ((chunk == 0) ? (text.size() - offset) : min(chunk, text.size() - offset));
			// Выполняем подачу очередного куска таблицы
			reader.feed(text.data() + offset, size, ((offset + size) >= text.size()));
			/**
			 * Выполняем перебор всех событий разбора
			 */
			while(reader.next()){
				// Собираемое событие разбора
				Event item;
				// Запоминаем разновидность события разбора
				item.event = static_cast <uint8_t> (reader.event());
				// Запоминаем содержимое поля события
				item.value.assign(reader.field().value);
				// Запоминаем имя столбца, к какому поле относится
				item.name.assign(reader.field().name);
				// Запоминаем признак заключения поля в кавычки
				item.quoted = reader.field().quoted;
				// Запоминаем признак изменения содержимого поля разбором
				item.modified = reader.field().modified;
				// Запоминаем место начала события в исходном тексте
				item.line = reader.location().line;
				// Запоминаем положение начала события в строке
				item.column = reader.location().column;
				// Запоминаем номер записи события
				item.record = reader.location().record;
				// Запоминаем номер поля в записи
				item.field = reader.location().field;
				// Запоминаем смещение начала события от начала исходного текста
				item.position = reader.location().offset;
				// Заносим собранное событие в перечень
				events.push_back(::std::move(item));
				// Выполняем учёт выданного чтением события
				totals.events++;
			}
			// Выполняем смещение по таблице
			offset += size;
		// Подача ведётся, пока таблица не исчерпана
		} while(offset < text.size());
		/**
		 * Если место отказа разбора требуется вернуть
		 */
		if(position != nullptr)
			// Запоминаем место отказа разбора
			(* position) = reader.location();
		// Выводим код ошибки разбора таблицы
		return reader.error();
	}
	/**
	 * @brief Метод сличения перечней событий разбора
	 *
	 * @param whole   перечень событий подачи таблицы целиком
	 * @param chunked перечень событий подачи таблицы кусками
	 * @param chunk   размер куска подачи таблицы
	 * @param text    поданная разбору таблица
	 * @return        результат сличения
	 *
	 */
	bool compare(const vector <Event> & whole, const vector <Event> & chunked, const size_t chunk, const string & text, const bool refused) noexcept {
		/**
		 * Если количество выданных событий разошлось, а разбор окончился отказом
		 *
		 * @note Сколько событий разбор успел выдать ПРЕЖДЕ отказа, договором не
		 *       установлено, и зависеть это вправе от нарезки: подача целиком отвергает
		 *       негодную кодировку по всему буферу разом, а побайтовая успевает выдать
		 *       поля, стоящие до негодного байта, - и выдаёт их верно. Требуется потому
		 *       не равенство длин, а чтобы короткий перечень был началом длинного:
		 *       расхождение внутри общего начала остаётся расхождением
		 *
		 * @note Так же судят и оба стенда сличения - соответствия XML и эталона CSV;
		 *       строгость здесь была не замыслом, а недосмотром
		 */
		if(refused && (whole.size() != chunked.size()))
			// Сличаем лишь общее начало перечней выданных событий
			return compare(
				vector <Event> (whole.begin(), whole.begin() + min(whole.size(), chunked.size())),
				vector <Event> (chunked.begin(), chunked.begin() + min(whole.size(), chunked.size())),
				chunk, text, false
			);
		/**
		 * Если количество выданных событий разошлось
		 */
		if(whole.size() != chunked.size()){
			// Выводим сообщение о расхождении количества выданных событий
			::fprintf(stderr, "csv fuzz: chunk=%zu event count differs: %zu whole against %zu chunked\n",
				chunk, whole.size(), chunked.size());
			// Выводим поданную разбору таблицу
			dump(text);
			// Выводим результат сличения
			return false;
		}
		/**
		 * Выполняем перебор всех выданных разбором событий
		 */
		for(size_t i = 0; i < whole.size(); i++){
			// Получаем событие подачи таблицы целиком
			const Event & first = whole.at(i);
			// Получаем событие подачи таблицы кусками
			const Event & second = chunked.at(i);
			/**
			 * Если события разошлись хотя бы одним признаком
			 */
			if((first.event != second.event) || (first.value != second.value) || (first.name != second.name) ||
			   (first.quoted != second.quoted) || (first.modified != second.modified) ||
			   (first.line != second.line) || (first.column != second.column) ||
			   (first.record != second.record) || (first.field != second.field) ||
			   (first.position != second.position)){
				// Выводим сообщение о расхождении выданных событий
				::fprintf(stderr, "csv fuzz: chunk=%zu event %zu differs:\n"
					"  whole:   event=%u value=[%s] name=[%s] quoted=%u modified=%u at %u:%u record=%u field=%u offset=%llu\n"
					"  chunked: event=%u value=[%s] name=[%s] quoted=%u modified=%u at %u:%u record=%u field=%u offset=%llu\n",
					chunk, i,
					static_cast <uint32_t> (first.event), first.value.c_str(), first.name.c_str(),
					static_cast <uint32_t> (first.quoted), static_cast <uint32_t> (first.modified),
					first.line, first.column, first.record, first.field,
					static_cast <unsigned long long> (first.position),
					static_cast <uint32_t> (second.event), second.value.c_str(), second.name.c_str(),
					static_cast <uint32_t> (second.quoted), static_cast <uint32_t> (second.modified),
					second.line, second.column, second.record, second.field,
					static_cast <unsigned long long> (second.position));
				// Выводим поданную разбору таблицу
				dump(text);
				// Выводим результат сличения
				return false;
			}
		}
		// Выводим результат сличения
		return true;
	}
	/**
	 * @brief Метод построения содержимого одного поля
	 *
	 * @details Содержимое строится из кусков, разбор задевающих: кавычек, разделителей,
	 * знаков конца строки и знаков Юникода. Поле, из одних лишь безобидных знаков
	 * составленное, ни одной ветви разбора не достигает
	 *
	 * @param engine источник псевдослучайных чисел
	 * @return       построенное содержимое поля
	 *
	 */
	string content(mt19937 & engine) noexcept {
		/**
		 * Куски, из каких складывается содержимое поля
		 */
		static const char * const PIECES[] = {
			"a", "bb", "ccc", "", " ", "\t", ",", ";", "|", "\"", "\"\"", "\n", "\r", "\r\n",
			"\\", "\\\"", "#", "=", "0", "-1", "3.14", "true", "null",
			"\xD0\x90", "\xE2\x82\xAC", "\xF0\x9F\x98\x80", "\xC3\xA9"
		};
		// Количество кусков в содержимом поля
		const uint32_t count = (engine() % 5);
		// Построенное содержимое поля
		string result;
		/**
		 * Выполняем набор содержимого поля из кусков
		 */
		for(uint32_t i = 0; i < count; i++)
			// Дописываем очередной кусок к содержимому поля
			result.append(PIECES[engine() % (sizeof(PIECES) / sizeof(PIECES[0]))]);
		// Выводим построенное содержимое поля
		return result;
	}
	/**
	 * @brief Метод построения таблицы
	 *
	 * @details Таблица строится записью, разбору поддающейся: поля, содержащие знаки,
	 * разбор задевающие, берутся в кавычки с их отменой. Порча вносится отдельно и
	 * точечно: таблица, испорченная целиком, отвергается первым же знаком, и глубокие
	 * состояния разбора так не достигаются
	 *
	 * @param engine    источник псевдослучайных чисел
	 * @param separator знак-разделитель полей строимой таблицы
	 * @param quote     знак кавычек строимой таблицы
	 * @return          построенная таблица
	 *
	 */
	string generate(mt19937 & engine, const char separator, const char quote) noexcept {
		/**
		 * Знаки конца строки, какими завершаются записи
		 */
		static const char * const NEWLINES[] = {"\r\n", "\n", "\r"};
		// Построенная таблица
		string result;
		// Количество записей строимой таблицы
		const uint32_t records = (1 + (engine() % 8));
		// Количество полей в записи строимой таблицы
		const uint32_t fields = (1 + (engine() % 5));
		/**
		 * Если таблица начинается с метки порядка байтов
		 */
		if((engine() % 8) == 0)
			// Дописываем метку порядка байтов к таблице
			result.append("\xEF\xBB\xBF");
		/**
		 * Выполняем построение всех записей таблицы
		 */
		for(uint32_t i = 0; i < records; i++){
			/**
			 * Если записи предшествует строка примечания
			 */
			if((engine() % 8) == 0)
				// Дописываем строку примечания к таблице
				result.append("#").append(content(engine)).append(NEWLINES[engine() % 3]);
			/**
			 * Если записи предшествует пустая строка
			 */
			if((engine() % 8) == 0)
				// Дописываем пустую строку к таблице
				result.append(NEWLINES[engine() % 3]);
			// Количество полей текущей записи
			const uint32_t width = (((engine() % 6) == 0) ? (1 + (engine() % 5)) : fields);
			/**
			 * Выполняем построение всех полей записи
			 */
			for(uint32_t j = 0; j < width; j++){
				/**
				 * Если полю предшествует разделитель
				 */
				if(j > 0)
					// Дописываем разделитель к таблице
					result.push_back(separator);
				// Получаем содержимое очередного поля
				const string value = content(engine);
				/**
				 * Если поле следует заключить в кавычки
				 *
				 * @note Поле берётся в кавычки чаще, чем того требует его содержимое:
				 *       кавычки вокруг содержимого безобидного разбор проходит иными
				 *       ветвями, нежели содержимое без кавычек
				 */
				if((engine() % 2) == 0){
					// Дописываем открывающую кавычку к таблице
					result.push_back(quote);
					/**
					 * Выполняем перебор всех знаков содержимого поля
					 */
					for(const char letter : value){
						/**
						 * Если знаком является кавычка
						 */
						if(letter == quote)
							// Дописываем отменяющую кавычку к таблице
							result.push_back(quote);
						// Дописываем знак содержимого поля к таблице
						result.push_back(letter);
					}
					// Дописываем закрывающую кавычку к таблице
					result.push_back(quote);
				/**
				 * Если поле записывается без кавычек
				 */
				} else {
					/**
					 * Выполняем перебор всех знаков содержимого поля
					 */
					for(const char letter : value){
						/**
						 * Если знак разрывает поле либо запись
						 *
						 * @note Знаки эти из поля без кавычек убираются, а не отменяются:
						 *       отмена их - уговор записи с разбором, а не часть договора,
						 *       и вносить её сюда значило бы строить таблицу, разбором по
						 *       умолчанию не читаемую
						 */
						if((letter == separator) || (letter == quote) || (letter == '\r') || (letter == '\n'))
							// Пропускаем знак содержимого поля
							continue;
						// Дописываем знак содержимого поля к таблице
						result.push_back(letter);
					}
				}
			}
			/**
			 * Если запись завершается знаком конца строки
			 *
			 * @note Последняя запись знаком конца строки завершается не всегда: договор
			 *       дозволяет обойтись без него, и путь этот разбор проходит иначе
			 */
			if((i + 1 < records) || ((engine() % 4) != 0))
				// Дописываем знак конца строки к таблице
				result.append(NEWLINES[engine() % 3]);
		}
		// Выводим построенную таблицу
		return result;
	}
	/**
	 * @brief Метод порчи построенной таблицы
	 *
	 * @details Порча вносится точечно: заменой, вставкой либо изъятием одного знака.
	 * Таблица при этом остаётся похожей на таблицу, и разбор доходит по ней далеко -
	 * тогда как таблица, испорченная целиком, отвергается первым же знаком
	 *
	 * @param text   портимая таблица
	 * @param engine источник псевдослучайных чисел
	 *
	 */
	void corrupt(string & text, mt19937 & engine) noexcept {
		/**
		 * Знаки, какими ведётся порча таблицы
		 */
		static const char LETTERS[] = {'"', ',', ';', '\n', '\r', '\\', '#', ' ', '\t', '\0', '\x01', '\x7F', '\xC0', '\xFF'};
		/**
		 * Если портить нечего
		 */
		if(text.empty())
			// Выходим из метода
			return;
		// Количество вносимых порч
		const uint32_t count = (1 + (engine() % 3));
		/**
		 * Выполняем внесение всех порч
		 */
		for(uint32_t i = 0; i < count; i++){
			/**
			 * Если портить стало нечего
			 */
			if(text.empty())
				// Выходим из метода
				return;
			// Место вносимой порчи в таблице
			const size_t offset = (engine() % text.size());
			/**
			 * Определяем разновидность вносимой порчи
			 */
			switch(engine() % 3){
				// Если порча вносится заменой знака
				case 0: text[offset] = LETTERS[engine() % sizeof(LETTERS)]; break;
				// Если порча вносится вставкой знака
				case 1: text.insert(offset, 1, LETTERS[engine() % sizeof(LETTERS)]); break;
				// Если порча вносится изъятием знака
				case 2: text.erase(offset, 1); break;
			}
		}
	}
	/**
	 * @brief Метод проверки кругового хода таблицы через её перезапись
	 *
	 * @details Таблица, собранная контейнером, записывается обратно и разбирается вновь:
	 * записанное обязано прочитаться неизменным, каким бы содержимое полей ни было.
	 * Договор этот и есть основание записи, и проверяется он здесь на содержимом,
	 * какое обыкновенная таблица не содержит
	 *
	 * @param text     разбираемая таблица
	 * @param settings настройки контейнера таблицы
	 * @return         результат проверки кругового хода
	 *
	 */
	bool roundtrip(const string & text, const csv::document_t::settings_t & settings) noexcept {
		// Контейнер разбираемой таблицы
		csv::document_t document(::logger(), settings);
		/**
		 * Если разбор таблицы не удался
		 *
		 * @note Отказ разбора здесь не является расхождением: таблица испорчена намеренно,
		 *       и отвергнуть её - дело разбора. Проверять здесь нечего
		 */
		if(!document.parse(text))
			// Выводим результат проверки кругового хода
			return true;
		// Выполняем учёт собранной таблицы контейнера
		totals.documents++;
		/**
		 * Выполняем перебор всех записей собранной таблицы
		 */
		for(size_t i = 0; i < document.rows(); i++){
			/**
			 * Если запись состоит из единственного пустого поля
			 *
			 * @note Записывается такая запись пустой строкой, а разбор пустые строки
			 *       пропускает: круговой ход её не сохраняет, и это свойство самой
			 *       записи CSV, а не дефект. Случай этот из проверки исключается
			 */
			if((document.size(i) == 1) && document.get(i, size_t(0)).empty())
				// Выводим результат проверки кругового хода
				return true;
		}
		// Выполняем перезапись собранной таблицы
		const string rewritten = document.text();
		// Выполняем учёт перезаписи таблицы
		totals.rewrites++;
		// Контейнер перезаписанной таблицы
		csv::document_t reread(::logger(), settings);
		/**
		 * Если разбор перезаписанной таблицы не удался
		 *
		 * @note Записанное обязано читаться обратно: отказ здесь означает, что запись
		 *       выдала таблицу, собственным разбором не читаемую
		 */
		if(!reread.parse(rewritten)){
			// Выводим сообщение о неразбираемости перезаписанной таблицы
			::fprintf(stderr, "csv fuzz: rewritten table is not readable back: error=%u\n",
				static_cast <uint32_t> (reread.error()));
			// Выводим разобранную таблицу
			dump(text);
			// Выводим перезаписанную таблицу
			dump(rewritten);
			// Выводим результат проверки кругового хода
			return false;
		}
		/**
		 * Если количество записей прочитанной обратно таблицы разошлось
		 */
		if(reread.rows() != document.rows()){
			// Выводим сообщение о расхождении количества записей
			::fprintf(stderr, "csv fuzz: row count differs after round trip: %zu against %zu\n",
				reread.rows(), document.rows());
			// Выводим разобранную таблицу
			dump(text);
			// Выводим перезаписанную таблицу
			dump(rewritten);
			// Выводим результат проверки кругового хода
			return false;
		}
		/**
		 * Выполняем перебор всех записей прочитанной обратно таблицы
		 */
		for(size_t i = 0; i < document.rows(); i++){
			/**
			 * Если количество полей записи разошлось
			 */
			if(reread.size(i) != document.size(i)){
				// Выводим сообщение о расхождении количества полей записи
				::fprintf(stderr, "csv fuzz: field count of row %zu differs after round trip: %zu against %zu\n",
					i, reread.size(i), document.size(i));
				// Выводим разобранную таблицу
				dump(text);
				// Выводим перезаписанную таблицу
				dump(rewritten);
				// Выводим результат проверки кругового хода
				return false;
			}
			/**
			 * Выполняем перебор всех полей записи
			 */
			for(size_t j = 0; j < document.size(i); j++){
				/**
				 * Если содержимое поля разошлось
				 */
				if(reread.get(i, j) != document.get(i, j)){
					// Выводим сообщение о расхождении содержимого поля
					::fprintf(stderr, "csv fuzz: field %zu:%zu differs after round trip\n", i, j);
					// Выводим разобранную таблицу
					dump(text);
					// Выводим перезаписанную таблицу
					dump(rewritten);
					// Выводим содержимое поля разобранной таблицы
					dump(string(document.get(i, j)));
					// Выводим содержимое поля прочитанной обратно таблицы
					dump(string(reread.get(i, j)));
					// Выводим результат проверки кругового хода
					return false;
				}
			}
		}
		// Выводим результат проверки кругового хода
		return true;
	}
	/**
	 * @brief Метод проверки сборки таблицы договором правки
	 *
	 * @details Круговой ход через запись идёт текстом, а этот - мимо текста вовсе: поля
	 * берутся у разобранной таблицы видами и подаются доливом в таблицу пустую.
	 * Собранная обязана совпасть с исходною полем в поле
	 *
	 * @note Поля подаются ВИДАМИ, а не копиями, и вид указывает в хранилище знаков той
	 *       же таблицы, откуда взят: заход этот и есть самый острый у долива - хранилище
	 *       приёмника наращивается, а виды подателя обязаны его пережить. На этом месте
	 *       уже был дефект чтения освобождённой памяти
	 *
	 * @param text     разбираемый текст таблицы
	 * @param settings настройки разбора таблицы
	 * @return         результат проверки сборки таблицы
	 *
	 */
	bool rebuild(const string & text, const csv::document_t::settings_t & settings) noexcept {
		// Контейнер разбираемой таблицы
		csv::document_t document(::logger(), settings);
		/**
		 * Если разбор таблицы не удался
		 */
		if(!document.parse(text))
			// Выводим результат проверки сборки таблицы
			return true;
			// Собираемая таблица, доливом наполняемая
		csv::document_t built(::logger(), settings);
		/**
		 * Если заголовок таблицы объявлен
		 */
		if(document.cols() > 0){
			// Собираемый перечень имён столбцов заголовка
			vector <string> names;
			/**
			 * Выполняем перебор всех имён столбцов заголовка
			 */
			for(auto & name : document.header())
				// Выполняем снятие копии имени очередного столбца
				names.emplace_back(name);
			/**
			 * Если заголовок несёт хотя бы одно имя
			 */
			if(!names.empty())
				// Выполняем объявление заголовка собираемой таблицы
				(void) built.header(names);
		}
		/**
		 * Выполняем перебор всех записей разобранной таблицы
		 */
		for(size_t i = 0; i < document.rows(); i++){
			// Собираемый перечень полей очередной записи
			vector <string_view> fields;
			/**
			 * Выполняем перебор всех полей очередной записи
			 */
			for(size_t j = 0; j < document.size(i); j++)
				// Выполняем добавление вида очередного поля к перечню
				fields.push_back(document.get(i, j));
			// Выполняем долив собранной записи к таблице
			built.append(fields);
		}
		// Выполняем учёт таблицы, договором правки собранной
		totals.assembled++;
		/**
		 * Если количество записей собранной таблицы разошлось
		 */
		if(built.rows() != document.rows()){
			// Выводим сообщение о расхождении количества записей
			::fprintf(stderr, "csv fuzz: assembled row count differs: %zu against %zu\n",
				built.rows(), document.rows());
			// Выводим разобранную таблицу
			dump(text);
			// Выводим результат проверки кругового хода
			return false;
		}
		/**
		 * Выполняем перебор всех записей собранной таблицы
		 */
		for(size_t i = 0; i < document.rows(); i++){
			/**
			 * Если количество полей записи разошлось
			 */
			if(built.size(i) != document.size(i)){
				// Выводим сообщение о расхождении количества полей записи
				::fprintf(stderr, "csv fuzz: assembled field count of row %zu differs: %zu against %zu\n",
					i, built.size(i), document.size(i));
				// Выводим разобранную таблицу
				dump(text);
				// Выводим результат проверки кругового хода
				return false;
			}
			/**
			 * Выполняем перебор всех полей записи
			 */
			for(size_t j = 0; j < document.size(i); j++){
				/**
				 * Если содержимое поля разошлось
				 */
				if(built.get(i, j) != document.get(i, j)){
					// Выводим сообщение о расхождении содержимого поля
					::fprintf(stderr, "csv fuzz: assembled field %zu:%zu differs\n", i, j);
					// Выводим разобранную таблицу
					dump(text);
					// Выводим содержимое поля разобранной таблицы
					dump(string(document.get(i, j)));
					// Выводим содержимое поля собранной таблицы
					dump(string(built.get(i, j)));
					// Выводим результат проверки кругового хода
					return false;
				}
			}
		}
		// Выводим результат проверки сборки таблицы
		return true;
	}
	/**
	 * @brief Метод проверки кругового хода полей через запись
	 *
	 * @details Поля со враждебным содержимым записываются и читаются обратно при всяком
	 * сочетании правил записи: поле, содержащее что угодно, обязано записываться так,
	 * чтобы разобраться обратно неизменным
	 *
	 * @param engine источник псевдослучайных чисел
	 * @return       результат проверки кругового хода
	 *
	 */
	bool hostile(mt19937 & engine) noexcept {
		// Поля записываемой записи
		vector <string> fields;
		// Количество полей записываемой записи
		const uint32_t count = (1 + (engine() % 5));
		/**
		 * Выполняем построение всех полей записываемой записи
		 */
		for(uint32_t i = 0; i < count; i++)
			// Заносим построенное содержимое поля в записываемую запись
			fields.push_back(content(engine));
		// Настройки записи таблицы
		csv::writer_t::settings_t writing;
		// Задаём правило заключения поля в кавычки
		writing.quoting = static_cast <csv::quoting_t> (engine() % 3);
		// Задаём способ записи кавычки внутри поля
		writing.escape = static_cast <csv::escape_t> (engine() % 2);
		// Задаём знак конца строки
		writing.newline = static_cast <csv::newline_t> (engine() % 3);
		// Задаём строгое прочтение кавычек лишь при знаке конца строки договора
		const bool strict = (writing.newline == csv::newline_t::CRLF);
		// Задаём знак-разделитель полей
		writing.separator = ((engine() % 2) == 0 ? ',' : ';');
		// Запись таблицы
		csv::writer_t writer(::logger(), writing);
		// Выполняем запись записи полем за полем
		writer.record(fields);
		/**
		 * Если записанное неотличимо от пустой строки
		 *
		 * @note Запись из единственного пустого поля даёт строку, содержащую один лишь
		 *       знак конца строки, а разбор пустые строки пропускает: круговой ход такой
		 *       записи не сохраняет, и это свойство самой записи CSV, а не дефект. Случай
		 *       этот из проверки исключается, а не выдаётся расхождением
		 */
		if(writer.text().find_first_not_of("\r\n") == string::npos)
			// Выводим результат проверки кругового хода
			return true;
		// Настройки разбора таблицы
		csv::reader_t::settings_t reading;
		// Задаём знак-разделитель полей
		reading.separator = writing.separator;
		// Задаём способ записи кавычки внутри поля
		reading.escape = writing.escape;
		// Задаём строгое прочтение кавычек
		reading.strict = strict;
		// Перечень событий разбора записанной таблицы
		vector <Event> events;
		// Выполняем разбор записанной таблицы
		const csv::error_t error = consume(writer.text(), reading, 0, events, nullptr);
		/**
		 * Если разбор записанной таблицы не удался
		 */
		if(error != csv::error_t::NONE){
			// Выводим сообщение о неразбираемости записанной записи
			::fprintf(stderr, "csv fuzz: hostile record is not readable back: %s\n", csv::message(error));
			// Выводим записанную таблицу
			dump(writer.text());
			// Выводим результат проверки кругового хода
			return false;
		}
		// Полученные обратно поля записи
		vector <string> result;
		/**
		 * Выполняем перебор всех событий разбора записанной таблицы
		 */
		for(const Event & item : events){
			/**
			 * Если событием является поле записи
			 */
			if(item.event == static_cast <uint8_t> (csv::event_t::FIELD))
				// Заносим содержимое поля в полученные обратно поля записи
				result.push_back(item.value);
		}
		/**
		 * Если полученные обратно поля с записанными разошлись
		 *
		 * @note Запись без кавычек вовсе перевода строки внутри поля не передаёт: знак
		 *       этот запись завершает, и отменить его нечем. Случай этот из проверки
		 *       исключается, а не выдаётся расхождением
		 */
		if(result != fields){
			/**
			 * Если запись велась без кавычек вовсе
			 */
			if(writing.quoting == csv::quoting_t::NONE)
				// Выводим результат проверки кругового хода
				return true;
			// Выводим сообщение о расхождении полученных обратно полей
			::fprintf(stderr, "csv fuzz: hostile record differs after round trip: quoting=%u escape=%u newline=%u\n",
				static_cast <uint32_t> (writing.quoting), static_cast <uint32_t> (writing.escape),
				static_cast <uint32_t> (writing.newline));
			// Выводим записанную таблицу
			dump(writer.text());
			/**
			 * Выполняем перебор всех записанных полей
			 */
			for(size_t i = 0; i < max(fields.size(), result.size()); i++){
				// Выводим номер сличаемого поля
				::fprintf(stderr, "  field %zu:\n", i);
				// Выводим записанное содержимое поля
				dump(i < fields.size() ? fields.at(i) : string("<нет поля>"));
				// Выводим полученное обратно содержимое поля
				dump(i < result.size() ? result.at(i) : string("<нет поля>"));
			}
			// Выводим результат проверки кругового хода
			return false;
		}
		// Выводим результат проверки кругового хода
		return true;
	}
	/**
	 * @brief Метод проверки потоковой выдачи записей обработчику
	 *
	 * @details Потоковая выдача обязана давать те же записи, что и сборка таблицы
	 * целиком: путь этот проходит разбор иначе - буфер записи переиспользуется, и
	 * поля живут лишь на время вызова обработчика
	 *
	 * @param text     разбираемая таблица
	 * @param settings настройки контейнера таблицы
	 * @return         результат проверки потоковой выдачи
	 *
	 */
	bool streaming(const string & text, const csv::document_t::settings_t & settings) noexcept {
		// Контейнер таблицы, собираемой целиком
		csv::document_t document(::logger(), settings);
		/**
		 * Если разбор таблицы не удался
		 */
		if(!document.parse(text))
			// Выводим результат проверки потоковой выдачи
			return true;
		// Записи, выданные потоковым разбором
		vector <vector <string>> records;
		// Контейнер таблицы, разбираемой потоково
		csv::document_t stream(::logger(), settings);
		// Выполняем потоковый разбор таблицы записями
		stream.parse(text, [&records](const vector <string_view> & fields) noexcept -> bool {
			// Поля очередной выданной записи
			vector <string> record;
			/**
			 * Выполняем перебор всех полей выданной записи
			 */
			for(const string_view value : fields)
				// Заносим содержимое поля в поля выданной записи
				record.push_back(string(value));
			// Заносим выданную запись в перечень записей
			records.push_back(::std::move(record));
			// Выводим признак продолжения разбора
			return true;
		});
		// Выполняем учёт выполненной потоковой выдачи
		totals.streams++;
		/**
		 * Если количество выданных записей разошлось
		 */
		if(records.size() != document.rows()){
			// Выводим сообщение о расхождении количества выданных записей
			::fprintf(stderr, "csv fuzz: streamed row count differs: %zu against %zu\n",
				records.size(), document.rows());
			// Выводим разбираемую таблицу
			dump(text);
			// Выводим результат проверки потоковой выдачи
			return false;
		}
		/**
		 * Выполняем перебор всех выданных записей
		 */
		for(size_t i = 0; i < records.size(); i++){
			/**
			 * Если количество полей выданной записи разошлось
			 */
			if(records.at(i).size() != document.size(i)){
				// Выводим сообщение о расхождении количества полей выданной записи
				::fprintf(stderr, "csv fuzz: streamed field count of row %zu differs: %zu against %zu\n",
					i, records.at(i).size(), document.size(i));
				// Выводим разбираемую таблицу
				dump(text);
				// Выводим результат проверки потоковой выдачи
				return false;
			}
			/**
			 * Выполняем перебор всех полей выданной записи
			 */
			for(size_t j = 0; j < records.at(i).size(); j++){
				/**
				 * Если содержимое поля выданной записи разошлось
				 */
				if(records.at(i).at(j) != document.get(i, j)){
					// Выводим сообщение о расхождении содержимого поля выданной записи
					::fprintf(stderr, "csv fuzz: streamed field %zu:%zu differs\n", i, j);
					// Выводим разбираемую таблицу
					dump(text);
					// Выводим результат проверки потоковой выдачи
					return false;
				}
			}
		}
		// Выводим результат проверки потоковой выдачи
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
	/**
	 * Если количество проходов задано параметром командной строки
	 */
	if(argc > 1)
		// Выполняем чтение количества проходов из параметра командной строки
		count = static_cast <uint64_t> (::strtoull(argv[1], nullptr, 10));
	// Зерно источника псевдослучайных чисел
	uint32_t seed = 0xC5F00;
	/**
	 * Если зерно задано вторым параметром командной строки
	 *
	 * @note Зерно закреплено по умолчанию: прогон обязан воспроизводиться. Задавать
	 *       его иным стоит затем, что построения одного зерна складываются в один и
	 *       тот же набор, сколько проходов ни задай
	 */
	if(argc > 2)
		// Выполняем чтение зерна из параметра командной строки
		seed = static_cast <uint32_t> (::strtoul(argv[2], nullptr, 10));
	// Создаём источник псевдослучайных чисел с закреплённым зерном
	mt19937 engine(seed);
	// Шаг вывода отчёта о ходе работы генератора
	const uint64_t report = ((count > 20) ? (count / 20) : 0);
	/**
	 * Выполняем проходы генератора
	 */
	for(uint64_t i = 0; i < count; i++){
		/**
		 * Если очередным проходом пришёл черёд отчёта о ходе работы
		 *
		 * @note Прогон на многих миллионах проходов идёт часами, и молчание его
		 *       неотличимо от зависания: отчёт этот показывает, что работа движется,
		 *       и даёт снять пробу на том проходе, на котором прогон встал
		 */
		if((report > 0) && (i > 0) && ((i % report) == 0)){
			// Выводим отчёт о ходе работы генератора
			::fprintf(stderr, "csv fuzz: %llu/%llu passes, %llu events, %llu documents\n",
				static_cast <unsigned long long> (i), static_cast <unsigned long long> (count),
				static_cast <unsigned long long> (totals.events), static_cast <unsigned long long> (totals.documents));
			// Выполняем выталкивание накопленного вывода
			::fflush(stderr);
		}
		// Собираемые настройки разбора таблицы
		csv::reader_t::settings_t settings;
		// Задаём знак-разделитель полей
		settings.separator = ((engine() % 4) == 0 ? ';' : ',');
		// Задаём знак кавычек, обрамляющих поле
		settings.quote = ((engine() % 8) == 0 ? '\'' : '"');
		// Задаём знак начала строки примечания
		settings.comment = ((engine() % 3) == 0 ? '#' : '\0');
		// Задаём способ записи кавычки внутри поля
		settings.escape = static_cast <csv::escape_t> (engine() % 3);
		// Задаём признак наличия заголовка
		settings.header = static_cast <csv::header_t> (engine() % 2);
		// Задаём способ снятия обвязки с содержимого поля
		settings.trim = static_cast <csv::trim_t> (engine() % 3);
		// Задаём обращение с записями разной длины
		settings.ragged = static_cast <csv::ragged_t> (engine() % 3);
		// Задаём строгое прочтение кавычек
		settings.strict = ((engine() % 4) == 0);
		// Задаём выдачу событий пустых строк
		settings.emitBlanks = ((engine() % 3) == 0);
		// Задаём выдачу событий примечаний
		settings.emitComments = ((engine() % 3) == 0);
		// Задаём проверку повторного объявления имён столбцов
		settings.duplicates = ((engine() % 4) != 0);
		/**
		 * Если требуется поджать пределы разбора
		 *
		 * @details Пределы меряются по ходу накопления, и расхождения их проступают
		 * лишь на поджатых значениях: при умолчаниях накопление до предела не доходит
		 * вовсе, и ветви отказа по пределу остаются непройденными
		 */
		if((engine() % 4) == 0){
			// Устанавливаем наибольшую допустимую длину поля
			settings.maxField = (1 + (engine() % 32));
			// Устанавливаем наибольшую допустимую длину записи
			settings.maxRecord = (1 + (engine() % 64));
			// Устанавливаем наибольшее допустимое количество полей записи
			settings.maxFields = (1 + (engine() % 6));
		}
		/**
		 * Если требуется определение разделителя по содержимому
		 */
		if((engine() % 6) == 0){
			// Включаем определение разделителя по содержимому
			settings.separator = '\0';
			// Устанавливаем количество записей, по каким ведётся определение
			settings.detect = (1 + (engine() % 8));
		}
		// Выполняем построение таблицы
		string text = generate(engine,
			(settings.separator != '\0' ? settings.separator : ','),
			settings.quote
		);
		// Выполняем учёт построенной таблицы
		totals.texts++;
		/**
		 * Если требуется испортить построенную таблицу
		 */
		if((engine() % 3) == 0){
			// Выполняем порчу построенной таблицы
			corrupt(text, engine);
			// Выполняем учёт испорченной таблицы
			totals.corrupted++;
		}
		// Перечень событий подачи таблицы целиком
		vector <Event> whole;
		// Место отказа разбора таблицы, поданной целиком
		csv::location_t failure;
		// Выполняем подачу таблицы целиком
		const csv::error_t error = consume(text, settings, 0, whole, &failure);
		/**
		 * Если таблица разобрана до конца
		 */
		if(error == csv::error_t::NONE)
			// Выполняем учёт разобранной до конца таблицы
			totals.survived++;
		/**
		 * Выполняем сличение подачи целиком с подачей кусками нескольких размеров
		 *
		 * @note Размеров берётся несколько, а не один, и первый из них всегда
		 *       однобайтовый: знак, чьё значение зависит от следующего за ним,
		 *       нарушает договор о независимости выдачи именно на однобайтовой подаче,
		 *       а расхождения пределов проступают на размерах покрупнее
		 */
		for(uint32_t attempt = 0; attempt < 3; attempt++){
			// Размер куска подачи таблицы
			const size_t chunk = ((attempt == 0) ? 1 : (1 + (engine() % 24)));
			// Перечень событий подачи таблицы кусками
			vector <Event> chunked;
			// Место отказа разбора таблицы, поданной кусками
			csv::location_t second;
			// Выполняем подачу таблицы кусками
			const csv::error_t reached = consume(text, settings, chunk, chunked, &second);
			/**
			 * Если перечни выданных разбором событий разошлись
			 */
			if(!compare(whole, chunked, chunk, text, ((error != csv::error_t::NONE) || (reached != csv::error_t::NONE)))){
				// Выводим настройки разбора таблицы
				dump(settings);
				// Выходим из приложения с кодом ошибки
				return EXIT_FAILURE;
			}
			/**
			 * Если код ошибки либо место отказа разошлись
			 */
			if((reached != error) || (failure.line != second.line) || (failure.column != second.column) ||
			   (failure.offset != second.offset) || (failure.record != second.record)){
				// Выводим сообщение о расхождении итога разбора
				::fprintf(stderr, "csv fuzz: chunk=%zu outcome differs: error %u/%u at %u:%u/%u:%u offset %llu/%llu record %u/%u\n",
					chunk, static_cast <uint32_t> (error), static_cast <uint32_t> (reached),
					failure.line, failure.column, second.line, second.column,
					static_cast <unsigned long long> (failure.offset),
					static_cast <unsigned long long> (second.offset),
					failure.record, second.record);
				// Выводим поданную разбору таблицу
				dump(text);
				// Выводим настройки разбора таблицы
				dump(settings);
				// Выходим из приложения с кодом ошибки
				return EXIT_FAILURE;
			}
		}
		// Собираемые настройки контейнера таблицы
		csv::document_t::settings_t document;
		// Задаём настройки разбора таблицы
		document.reader = settings;
		// Задаём знак-разделитель полей записи
		document.writer.separator = (settings.separator != '\0' ? settings.separator : ',');
		// Задаём знак кавычек, обрамляющих поле
		document.writer.quote = settings.quote;
		// Задаём знак начала строки примечания, признаваемый разбором
		document.writer.comment = settings.comment;
		// Задаём правило заключения поля в кавычки
		document.writer.quoting = static_cast <csv::quoting_t> (engine() % 3);
		/**
		 * Задаём знак конца строки
		 *
		 * @note Строгое прочтение признаёт лишь возврат каретки с переводом строки,
		 *       названный договором: записав таблицу иным знаком и прочитав её строго,
		 *       мы получили бы отказ по несогласованности настроек, а не по дефекту
		 */
		document.writer.newline = (settings.strict ? csv::newline_t::CRLF : static_cast <csv::newline_t> (engine() % 3));
		/**
		 * Задаём способ записи кавычки внутри поля
		 *
		 * @note Способ записи берётся тем же, что и способ разбора: записанное иным
		 *       способом собственным разбором не читается, и расхождение кругового
		 *       хода означало бы здесь несогласованность настроек, а не дефект
		 */
		document.writer.escape = settings.escape;
		/**
		 * Если круговой ход таблицы через её перезапись нарушен
		 *
		 * @note Проверка ведётся лишь при обвязке, сохраняемой как записана: снятие
		 *       обвязки содержимое поля изменяет намеренно, и прочитанное обратно с
		 *       записанным разойдётся по указанию потребителя, а не по дефекту.
		 *       Заголовок при перезаписи занимает первую запись, а разбор с
		 *       объявленным заголовком её именами и считает - сличать записи так
		 *       нельзя, и случай этот исключается тоже
		 *
		 * @note Пределы разбора меряются по исходному тексту, а не по содержимому полей:
		 *       перезапись добавляет кавычки, и запись, в предел укладывавшаяся, после
		 *       перезаписи его превышает. Поджатые пределы потому из проверки исключаются
		 *
		 * @note Определение разделителя из проверки исключается тоже: перезапись меняет
		 *       расстановку кавычек, и знак, бывший в исходном тексте содержимым поля,
		 *       после перезаписи вправе оказаться пригодным разделителем. Определение
		 *       выберет его - и правильно сделает, ибо иного различения у него нет
		 */
		if((settings.trim == csv::trim_t::NONE) && (settings.header == csv::header_t::NONE) &&
		   (settings.ragged != csv::ragged_t::FILL) && (settings.maxField == csv::MAX_FIELD) &&
		   (settings.maxRecord == csv::MAX_RECORD) && (settings.maxFields == csv::MAX_FIELDS) &&
		   (settings.separator != '\0') && !roundtrip(text, document)){
			// Выводим настройки разбора таблицы
			dump(settings);
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
		}
		/**
		 * Если сборка таблицы договором правки разошлась с разобранной
		 *
		 * @note Проверка эта ведётся при ВСЯКИХ настройках, в отличие от кругового хода
		 *       выше: тот идёт через запись и оттого узкими настройками ограничен, а эта
		 *       текста не касается вовсе - сличаются таблица разобранная и таблица,
		 *       собранная доливом её же полей
		 */
		if(!rebuild(text, document)){
			// Выводим настройки разбора таблицы
			dump(settings);
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
		}
		/**
		 * Если потоковая выдача записей разошлась со сборкой таблицы целиком
		 */
		if(!streaming(text, document)){
			// Выводим настройки разбора таблицы
			dump(settings);
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
		}
		/**
		 * Если круговой ход полей через запись нарушен
		 */
		if(!hostile(engine)){
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
		}
	}
	// Выводим итог работы генератора
	::fprintf(stderr, "csv fuzz: %llu passes, %llu texts (%llu corrupted), %llu survived, %llu events, %llu documents, %llu rewrites, %llu streams, %llu assembled\n",
		static_cast <unsigned long long> (count), static_cast <unsigned long long> (totals.texts),
		static_cast <unsigned long long> (totals.corrupted), static_cast <unsigned long long> (totals.survived),
		static_cast <unsigned long long> (totals.events), static_cast <unsigned long long> (totals.documents),
		static_cast <unsigned long long> (totals.rewrites), static_cast <unsigned long long> (totals.streams),
		static_cast <unsigned long long> (totals.assembled));
	// Выходим из приложения
	return EXIT_SUCCESS;
}
