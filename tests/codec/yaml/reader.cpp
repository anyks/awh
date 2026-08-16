/**
 * @file reader.cpp
 * @date 2026-08-17
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки потокового чтения текста YAML — разбор блочных построений стопою
 *        отступов, снятие ограды со значений и независимость выдачи от нарезки текста
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/yaml/yaml.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Пространство имён помощников проверок потокового чтения
 *
 * @note Помощники объявлены безымянным пространством имён намеренно: проверки всех
 *       кодеков собираются одной программой, и одноимённые помощники двух кодеков
 *       нарушили бы правило одного определения
 *
 */
namespace {
	/**
	 * @brief Функция сборки ряда событий разбора подачей текста кусками
	 *
	 * @details Ряд собирается тем же способом, каким печатает его эталонный набор
	 *          `yaml-test-suite`: название события, а за ним содержимое скалярного
	 *          значения. Сличение с эталоном пойдёт по этой самой записи
	 *
	 * @param text  разбираемый текст
	 * @param chunk размер куска подачи
	 * @param ok    признак успешного разбора текста
	 * @return      собранный ряд событий разбора
	 *
	 */
	string events(const string & text, const size_t chunk, bool & ok) noexcept {
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Собираемый ряд событий разбора
		string result;
		// Запоминаем признак успешного разбора текста
		ok = true;
		// Смещение очередного подаваемого куска
		size_t offset = 0;
		/**
		 * @brief Функция вычитывания всех накопленных событий разбора
		 *
		 */
		const auto drain = [&reader, &result]() noexcept -> void {
			/**
			 * Выполняем перебор всех накопленных событий разбора
			 */
			while(reader.next()){
				// Выполняем запись названия очередного события
				result.append(yaml::name(reader.event()));
				/**
				 * Если событие несёт скалярное значение либо примечание
				 */
				if((reader.event() == yaml::event_t::SCALAR) || (reader.event() == yaml::event_t::COMMENT))
					// Выполняем запись содержимого события
					result.append(" «").append(reader.value().text).append("»");
				// Выполняем запись разделителя событий
				result.append("\n");
			}
		};
		/**
		 * Выполняем подачу текста до его окончания
		 */
		do {
			// Получаем размер очередного подаваемого куска
			const size_t size = (((offset + chunk) > text.size()) ? (text.size() - offset) : chunk);
			/**
			 * Если разобрать очередной кусок не удалось
			 */
			if(!reader.feed(text.data() + offset, size, ((offset + size) >= text.size()))){
				// Запоминаем признак неудачного разбора текста
				ok = false;
				// Выполняем вычитывание событий, собранных до отказа
				drain();
				// Выполняем запись описания отказа разбора
				result.append("ОТКАЗ ").append(yaml::message(reader.error()));
				// Выполняем запись строки, где отказ произошёл
				result.append(" строка ").append(to_string(reader.location().line));
				// Выполняем запись положения отказа в строке
				result.append(" знак ").append(to_string(reader.location().column)).append("\n");
				// Выводим собранный ряд событий разбора
				return result;
			}
			// Выполняем вычитывание накопленных событий разбора
			drain();
			// Выполняем переход к следующему куску текста
			offset += size;
		// Выполняем подачу до исчерпания текста
		} while(offset < text.size());
		// Выводим собранный ряд событий разбора
		return result;
	}
	/**
	 * @brief Функция сборки ряда событий разбора подачей текста целиком
	 *
	 * @param text разбираемый текст
	 * @return     собранный ряд событий разбора
	 *
	 */
	string events(const string & text) noexcept {
		// Признак успешного разбора текста
		bool ok = false;
		// Выводим собранный ряд событий разбора
		return events(text, text.size(), ok);
	}
}

/**
 * @brief Проверка разбора отображения пар
 *
 */
TEST(CodecYamlReader, Mapping) {
	// Выполняем проверку разбора отображения из одной пары
	ASSERT_EQ(events("key: value\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «key»\nSCALAR «value»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку разбора отображения из двух пар
	ASSERT_EQ(events("first: 1\nsecond: два\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «first»\nSCALAR «1»\nSCALAR «second»\nSCALAR «два»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку разбора пары, значения не несущей
	 *
	 * @note Пустое значение описанием дозволено, и выдать его надлежит событием, а не
	 *       молчанием: пара `ключ:` есть пара, а не отсутствие её
	 */
	ASSERT_EQ(events("empty:\nnext: 1\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «empty»\nSCALAR «»\nSCALAR «next»\nSCALAR «1»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
}
/**
 * @brief Проверка разбора вложенных отображений
 *
 */
TEST(CodecYamlReader, Nesting) {
	// Выполняем проверку разбора вложенного отображения
	ASSERT_EQ(events("server:\n  host: localhost\n  port: 8080\nlevel: debug\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «server»\nMAPPING_START\n"
		"SCALAR «host»\nSCALAR «localhost»\nSCALAR «port»\nSCALAR «8080»\n"
		"MAPPING_END\nSCALAR «level»\nSCALAR «debug»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку разбора вложенности в четыре уровня
	ASSERT_EQ(events("a:\n  b:\n    c:\n      d: 1\ne: 2\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nMAPPING_START\nSCALAR «b»\nMAPPING_START\nSCALAR «c»\nMAPPING_START\n"
		"SCALAR «d»\nSCALAR «1»\n"
		"MAPPING_END\nMAPPING_END\nMAPPING_END\n"
		"SCALAR «e»\nSCALAR «2»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
}
/**
 * @brief Проверка разбора перечней значений
 *
 */
TEST(CodecYamlReader, Sequence) {
	// Выполняем проверку разбора перечня значений
	ASSERT_EQ(events("- один\n- два\n- 3\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\n"
		"SCALAR «один»\nSCALAR «два»\nSCALAR «3»\n"
		"SEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку разбора перечня отображений
	ASSERT_EQ(events("- name: первый\n  port: 1\n- name: второй\n  port: 2\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\n"
		"MAPPING_START\nSCALAR «name»\nSCALAR «первый»\nSCALAR «port»\nSCALAR «1»\nMAPPING_END\n"
		"MAPPING_START\nSCALAR «name»\nSCALAR «второй»\nSCALAR «port»\nSCALAR «2»\nMAPPING_END\n"
		"SEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку разбора перечня, стоящего значением пары с отступом
	ASSERT_EQ(events("hosts:\n  - alpha\n  - beta\nport: 80\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «hosts»\nSEQUENCE_START\nSCALAR «alpha»\nSCALAR «beta»\nSEQUENCE_END\n"
		"SCALAR «port»\nSCALAR «80»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
}
/**
 * @brief Проверка разбора перечня, стоящего на отступе имени своей пары
 *
 * @details Построение это описанием дозволено и в обиходе встречается сплошь и рядом.
 * Закрытием по отступу такой перечень не снимается - отступ у него тот же, что у имён
 * отображения, - и снять его обязана следующая пара того же отображения
 *
 * @note Первое построение разбора его отвергало смешением перечня с отображением, и
 *       нашёл это щуп сличения нарезок, а не набор проверок
 *
 */
TEST(CodecYamlReader, ImpliedSequence) {
	// Выполняем проверку разбора перечня на отступе имени своей пары
	ASSERT_EQ(events("hosts:\n- alpha\n- beta\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «hosts»\nSEQUENCE_START\nSCALAR «alpha»\nSCALAR «beta»\nSEQUENCE_END\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку того, что следующая пара перечень закрывает
	ASSERT_EQ(events("hosts:\n- alpha\nport: 80\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «hosts»\nSEQUENCE_START\nSCALAR «alpha»\nSEQUENCE_END\n"
		"SCALAR «port»\nSCALAR «80»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку смешения перечня с отображением на одном уровне
	 *
	 * @note Перечень, значением пары не являющийся, на отступе отображения стоять не
	 *       вправе: это уже не построение, а ошибка
	 */
	ASSERT_EQ(events("port: 80\n- alpha\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «port»\nSCALAR «80»\n"
		"ОТКАЗ перечень и отображение смешаны на одном уровне строка 2 знак 1\n");
}
/**
 * @brief Проверка снятия ограды со скалярных значений
 *
 */
TEST(CodecYamlReader, Quoting) {
	// Выполняем проверку снятия одинарной ограды
	ASSERT_EQ(events("text: 'одинарная'\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «text»\nSCALAR «одинарная»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку удвоения кавычки внутри одинарной ограды
	 *
	 * @note Отменяющих последовательностей одинарная ограда не знает вовсе, и кавычка
	 *       записывается в ней удвоением своим
	 */
	ASSERT_EQ(events("text: 'кавычка '' внутри'\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «text»\nSCALAR «кавычка ' внутри»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку отменяющих последовательностей двойной ограды
	ASSERT_EQ(events("text: \"с\\tотменой\\nи \\\"кавычкой\\\"\"\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «text»\nSCALAR «с\tотменой\nи \"кавычкой\"»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку записи знака Юникода отменяющей последовательностью
	ASSERT_EQ(events("text: \"\\u0417\\u043D\\u0430\\u043A\"\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «text»\nSCALAR «Знак»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку того, что ограда отменяет разрешение вида
	 *
	 * @note Ограда для того и ставится, чтобы `12` осталось строкой
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем подачу текста с оградою вокруг числа
		ASSERT_TRUE(reader.feed("plain: 12\nquoted: '12'\n"));
		// Виды значений, разрешённые разбором
		vector <yaml::type_t> types;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Если событие несёт скалярное значение
			 */
			if(reader.event() == yaml::event_t::SCALAR)
				// Выполняем запоминание вида разрешённого значения
				types.emplace_back(reader.value().type);
		}
		// Выполняем проверку количества прочитанных значений
		ASSERT_EQ(types.size(), 4u);
		// Выполняем проверку того, что значение без ограды разрешено числом
		ASSERT_EQ(types.at(1), yaml::type_t::NUMBER);
		// Выполняем проверку того, что значение в ограде осталось строкой
		ASSERT_EQ(types.at(3), yaml::type_t::STRING);
	}
}
/**
 * @brief Проверка разбора потока из многих документов
 *
 */
TEST(CodecYamlReader, Documents) {
	// Выполняем проверку разбора потока из двух документов
	ASSERT_EQ(events("---\nfirst: 1\n---\nsecond: 2\n"),
		"STREAM_START\n"
		"DOCUMENT_START\nMAPPING_START\nSCALAR «first»\nSCALAR «1»\nMAPPING_END\nDOCUMENT_END\n"
		"DOCUMENT_START\nMAPPING_START\nSCALAR «second»\nSCALAR «2»\nMAPPING_END\nDOCUMENT_END\n"
		"STREAM_END\n");
	// Выполняем проверку явного объявления конца документа
	ASSERT_EQ(events("first: 1\n...\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «first»\nSCALAR «1»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку разбора скалярного значения документом целиком
	ASSERT_EQ(events("просто строка\n"),
		"STREAM_START\nDOCUMENT_START\nSCALAR «просто строка»\nDOCUMENT_END\nSTREAM_END\n");
}
/**
 * @brief Проверка выдачи примечаний и пустых строк
 *
 */
TEST(CodecYamlReader, Comments) {
	/**
	 * Выполняем проверку того, что примечания молчанием отбрасываются
	 *
	 * @note Выдача их - настройка, а не умолчание: потребитель, дерево собирающий,
	 *       примечаний не ждёт вовсе
	 */
	ASSERT_EQ(events("# сверху\nkey: value # сбоку\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «key»\nSCALAR «value»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Настройки разбора текста с выдачей примечаний
	yaml::reader_t::settings_t settings;
	// Задаём выдачу примечаний отдельным событием
	settings.emitComments = true;
	// Задаём выдачу пустых строк отдельным событием
	settings.emitBlanks = true;
	// Объект потокового чтения текста
	yaml::reader_t reader(settings);
	// Выполняем подачу текста с примечаниями и пустой строкой
	ASSERT_TRUE(reader.feed("# сверху\nkey: value # сбоку\n\nnext: 1\n"));
	// Собираемый ряд событий разбора
	string result;
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next()){
		// Выполняем запись названия очередного события
		result.append(yaml::name(reader.event()));
		/**
		 * Если событие несёт скалярное значение либо примечание
		 */
		if((reader.event() == yaml::event_t::SCALAR) || (reader.event() == yaml::event_t::COMMENT))
			// Выполняем запись содержимого события
			result.append(" «").append(reader.value().text).append("»");
		// Выполняем запись разделителя событий
		result.append("\n");
	}
	// Выполняем проверку собранного ряда событий разбора
	ASSERT_EQ(result,
		"STREAM_START\nCOMMENT «сверху»\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «key»\nSCALAR «value»\nCOMMENT «сбоку»\nBLANK\n"
		"SCALAR «next»\nSCALAR «1»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
}
/**
 * @brief Проверка отказов разбора
 *
 */
TEST(CodecYamlReader, Refusals) {
	/**
	 * Выполняем проверку знака горизонтальной подачи в отступе
	 *
	 * @note Описание запрещает его прямо: ширина его толкуется по-разному, и смысл
	 *       текста зависел бы от настроек показывающего его
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора отступа со знаком подачи
		ASSERT_FALSE(reader.feed("a:\n\tb: 1\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::TAB_IN_INDENTATION);
		// Выполняем проверку строки, где отказ произошёл
		ASSERT_EQ(reader.location().line, 2u);
		// Выполняем проверку состояния прекращения разбора
		ASSERT_EQ(reader.state(), yaml::state_t::FAILED);
	}
	/**
	 * Выполняем проверку незакрытой ограды скалярного значения
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора незакрытой ограды
		ASSERT_FALSE(reader.feed("a: 'не закрыта\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::UNTERMINATED_SCALAR);
	}
	/**
	 * Выполняем проверку неопознанной отменяющей последовательности
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора неопознанной последовательности
		ASSERT_FALSE(reader.feed("a: \"\\q\"\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::INVALID_ESCAPE);
	}
	/**
	 * Выполняем проверку записи суррогата отменяющей последовательностью
	 *
	 * @note Записать суррогат последовательностью UTF-8 нельзя вовсе, и принять такую
	 *       запись значило бы выдать наружу битую последовательность
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора записи суррогата
		ASSERT_FALSE(reader.feed("a: \"\\uD800\"\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::INVALID_UNICODE);
	}
	/**
	 * Выполняем проверку построений, ещё не заведённых
	 *
	 * @note Отвечать на них молчаливым разбором наугад нельзя: разбор выдал бы дерево,
	 *       исходному тексту не отвечающее, и потребитель узнал бы об этом не сразу.
	 *       Проверка эта подлежит замене, когда построения эти будут заведены
	 */
	{
		/**
		 * Построения, заводимые следующими этапами работ
		 */
		const vector <string> pending = {
			"a: [1, 2]\n", "a: {b: 1}\n", "a: |\n  текст\n", "a: >\n  текст\n",
			"a: &метка 1\n", "a: *метка\n", "a: !!str 1\n", "%YAML 1.2\n---\na: 1\n"
		};
		/**
		 * Выполняем перебор всех ещё не заведённых построений
		 */
		for(const string & text : pending){
			// Объект потокового чтения текста
			yaml::reader_t reader;
			// Выполняем проверку отказа разбора построения
			ASSERT_FALSE(reader.feed(text)) << "построение: " << text;
			// Выполняем проверку кода ошибки разбора
			ASSERT_EQ(reader.error(), yaml::error_t::INVALID_CHARACTER) << "построение: " << text;
		}
	}
}
/**
 * @brief Проверка независимости выдачи от нарезки текста на куски
 *
 * @details Проверка эта - главная у потокового чтения: исход разбора не вправе зависеть
 * от того, как текст нарезан при подаче. Нарезка перебирается всеми размерами куска, а
 * не одним, и сличается ряд событий целиком, а не итог его
 *
 * @note Первое построение разбора её не выдерживало: события выдавались по мере разбора
 *       строки, и отказ посреди строки оставлял потребителя с началом построения без
 *       конца его. Строка собирается теперь целиком и лишь затем переносится в очередь
 *       выдачи
 *
 */
TEST(CodecYamlReader, Chunking) {
	/**
	 * Образцы текстов, охватывающие все заведённые построения и отказы
	 */
	const vector <string> samples = {
		"key: value\n",
		"first: 1\nsecond: два\n",
		"server:\n  host: localhost\n  port: 8080\nlevel: debug\n",
		"- один\n- два\n- 3\n",
		"- name: первый\n  port: 1\n- name: второй\n  port: 2\n",
		"hosts:\n  - alpha\n  - beta\nport: 80\n",
		"hosts:\n- alpha\n- beta\n",
		"text: 'одинарная'\nother: \"двойная\\tс отменой\"\nplain: 12\n",
		"empty:\nnext: 1\n",
		"# сверху\nkey: value # сбоку\n",
		"---\nfirst: 1\n---\nsecond: 2\n...\n",
		"просто строка\n",
		"a:\n  b:\n    c:\n      d: 1\ne: 2\n",
		"текст без перевода строки в конце",
		"a:\n\tb: 1\n",
		"a: 'не закрыта\n",
		"a: [1, 2]\n"
	};
	/**
	 * Выполняем перебор всех образцов текстов
	 */
	for(const string & text : samples){
		// Признак успешного разбора текста, поданного целиком
		bool whole = false;
		// Получаем ряд событий разбора текста, поданного целиком
		const string expected = events(text, text.size(), whole);
		/**
		 * Выполняем перебор всех размеров куска подачи
		 */
		for(size_t chunk = 1; chunk < text.size(); chunk++){
			// Признак успешного разбора текста, поданного кусками
			bool piece = false;
			// Получаем ряд событий разбора текста, поданного кусками
			const string result = events(text, chunk, piece);
			// Выполняем проверку того, что нарезка исход разбора не изменила
			ASSERT_EQ(result, expected) << "текст «" << text << "», кусок " << chunk;
			// Выполняем проверку того, что нарезка признак успеха не изменила
			ASSERT_EQ(piece, whole) << "текст «" << text << "», кусок " << chunk;
		}
	}
}
/**
 * @brief Проверка сброса состояния потокового чтения
 *
 */
TEST(CodecYamlReader, Cleared) {
	// Объект потокового чтения текста
	yaml::reader_t reader;
	// Выполняем подачу текста, разбор которого отказывает
	ASSERT_FALSE(reader.feed("a:\n\tb: 1\n"));
	// Выполняем проверку состояния прекращения разбора
	ASSERT_EQ(reader.state(), yaml::state_t::FAILED);
	// Выполняем сброс состояния потокового чтения
	reader.clear();
	// Выполняем проверку сброшенного состояния чтения
	ASSERT_EQ(reader.state(), yaml::state_t::READY);
	// Выполняем проверку сброшенного кода ошибки разбора
	ASSERT_EQ(reader.error(), yaml::error_t::NONE);
	/**
	 * Выполняем проверку того, что чтение годно к работе после сброса
	 *
	 * @note Круг разбора кодека TOML нашёл ровно такой дефект: указатель дерева сброс
	 *       переживал, и следующий разбор шёл на памяти прежнего
	 */
	ASSERT_TRUE(reader.feed("key: value\n"));
	// Собираемый ряд событий разбора
	string result;
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next()){
		// Выполняем запись названия очередного события
		result.append(yaml::name(reader.event()));
		/**
		 * Если событие несёт скалярное значение
		 */
		if(reader.event() == yaml::event_t::SCALAR)
			// Выполняем запись содержимого события
			result.append(" «").append(reader.value().text).append("»");
		// Выполняем запись разделителя событий
		result.append("\n");
	}
	// Выполняем проверку собранного ряда событий разбора
	ASSERT_EQ(result,
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «key»\nSCALAR «value»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку состояния окончания разбора
	ASSERT_EQ(reader.state(), yaml::state_t::FINISHED);
}
