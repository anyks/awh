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
				if((reader.event() == yaml::event_t::SCALAR) || (reader.event() == yaml::event_t::COMMENT) ||
				   (reader.event() == yaml::event_t::ALIAS))
					// Выполняем запись содержимого события
					result.append(" «").append(reader.value().text).append("»");
				/**
				 * Если событию предпослана метка
				 */
				if(!reader.value().anchor.empty())
					// Выполняем запись имени метки события
					result.append(" &").append(reader.value().anchor);
				/**
				 * Если событию предпослана метка типа
				 */
				if(!reader.value().tag.empty())
					// Выполняем запись метки типа события
					result.append(" <").append(reader.value().tag).append(">");
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
 * @brief Проверка выдачи пустоты на месте значения записи перечня
 *
 * @details Черта без содержимого за нею есть запись перечня с пустым значением, и выдать
 *          её надлежит именно так. Ожидание значения записи от ожидания значения пары
 *          отличается одним: черта на отступе ожидания есть для пары значение её, а для
 *          записи - запись следующая, и пустоту прежней надлежит выдать прежде неё
 *
 * @note Нашёл это ворошитель сличением перезаписи: пустая запись пропадала из выдачи
 *       вовсе, и дерево документа теряло её вместе с местом её в перечне
 *
 */
TEST(CodecYamlReader, EmptyEntries) {
	// Выполняем проверку выдачи пустоты первой записью перечня
	ASSERT_EQ(events("- \n- beta\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\nSCALAR «»\nSCALAR «beta»\n"
		"SEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку выдачи пустоты по черте без пробела за нею
	ASSERT_EQ(events("-\n- beta\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\nSCALAR «»\nSCALAR «beta»\n"
		"SEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку выдачи пустоты последней записью перечня
	ASSERT_EQ(events("- alpha\n- \n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\nSCALAR «alpha»\nSCALAR «»\n"
		"SEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку того, что содержимое строкою ниже пустотою не является
	 *
	 * @note Отступ глубже черты знаменует значение записи, а не пустоту вместо него
	 */
	ASSERT_EQ(events("- \n  alpha\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\nSCALAR «alpha»\n"
		"SEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку выдачи пустоты внутри вложенного перечня
	ASSERT_EQ(events("- \n  - \n  - beta\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\nSEQUENCE_START\n"
		"SCALAR «»\nSCALAR «beta»\nSEQUENCE_END\nSEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку того, что пустота записи закрывается следующей парой отображения
	ASSERT_EQ(events("hosts:\n- \nport: 80\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «hosts»\n"
		"SEQUENCE_START\nSCALAR «»\nSEQUENCE_END\nSCALAR «port»\nSCALAR «80»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
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
			"? составное\n: значение\n"
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
		"a: [1, 2, три]\n",
		"a: {x: 1, y: два}\n",
		"a: [[1, 2], {k: v}]\n",
		"a: []\nb: {}\n",
		"text: |\n  первая\n  вторая\nnext: 1\n",
		"text: >\n  первая\n  вторая\n\n  третья\n",
		"text: |-\n  одна\n\nnext: 1\n",
		"text: |+\n  одна\n\n",
		"text: |2\n    два пробела\n",
		"- |\n  одна\n- две\n",
		"a: [1, 2\n",
		"a: [\n  1,\n  2\n]\n",
		"a: {\n  x: 1,\n  y: два\n}\nb: 3\n",
		"a: [\n  [1,\n   2],\n  {k: v}\n]\n",
		"a: [ # сбоку\n  1, # ещё\n  2\n]\n",
		"a: [&я 1, *я]\n",
		"a: [\n  1,\n  2\n",
		"key: длинное\n  продолжение\nnext: 1\n",
		"key:\n  первая\n  вторая\n",
		"key: первая\n\n  вторая\n",
		"- один\n  продолжение\n- два\n",
		"a:\n  - один\n    продолжение\n  - два\n",
		"key: первая\n  вторая # сбоку\nnext: 1\n",
		"просто строка\n  продолжение\n",
		"key: одна\n\n\nnext: 1\n",
		"key: первая\n  вторая: 1\n",
		"key: первая\n  - один\n",
		"a: &я 1\nb: *я\n",
		"a: &я\n  x: 1\nb: *я\n",
		"- &я один\n- *я\n",
		"a: !!str 12\nb: !!int 7\n",
		"a: !<tag:x,2000:mine> 12\n",
		"%YAML 1.1\n---\na: yes\n",
		"%TAG !e! tag:example.com,2000:\n---\na: !e!mine 1\n",
		"a: *нет\n",
		"a: !!int строка\n",
		"a: !e!mine 1\n",
		"%YAML 2.0\n---\na: 1\n",
		"%YAML 1.2\na: 1\n"
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
/**
 * @brief Проверка разбора поточных построений
 *
 * @details Поточные построения записываются скобками, как в JSON, и правила окончания
 * значений внутри них иные, нежели в блочных: значение оканчивается запятой либо
 * закрывающей скобкой, а не одним лишь концом строки
 *
 */
TEST(CodecYamlReader, Flow) {
	// Выполняем проверку разбора поточного перечня
	ASSERT_EQ(events("a: [1, 2, три]\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «a»\n"
		"SEQUENCE_START\nSCALAR «1»\nSCALAR «2»\nSCALAR «три»\nSEQUENCE_END\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку разбора поточного отображения
	ASSERT_EQ(events("a: {x: 1, y: два}\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «a»\n"
		"MAPPING_START\nSCALAR «x»\nSCALAR «1»\nSCALAR «y»\nSCALAR «два»\nMAPPING_END\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку разбора вложенных поточных построений
	ASSERT_EQ(events("a: [[1, 2], {k: v}]\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «a»\n"
		"SEQUENCE_START\nSEQUENCE_START\nSCALAR «1»\nSCALAR «2»\nSEQUENCE_END\n"
		"MAPPING_START\nSCALAR «k»\nSCALAR «v»\nMAPPING_END\nSEQUENCE_END\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку разбора пустых поточных построений
	ASSERT_EQ(events("a: []\nb: {}\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSEQUENCE_START\nSEQUENCE_END\n"
		"SCALAR «b»\nMAPPING_START\nMAPPING_END\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку имени пары поточного отображения без значения
	 *
	 * @note Отображение `{a, b}` описанием дозволено: значения пар пусты, и выдать их
	 *       надлежит пустыми значениями, ибо пара без значения есть пара
	 */
	ASSERT_EQ(events("a: {x, y}\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «a»\n"
		"MAPPING_START\nSCALAR «x»\nSCALAR «»\nSCALAR «y»\nSCALAR «»\nMAPPING_END\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку ограды внутри поточного построения
	ASSERT_EQ(events("a: ['1', \"два, три\"]\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «a»\n"
		"SEQUENCE_START\nSCALAR «1»\nSCALAR «два, три»\nSEQUENCE_END\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку незакрытого поточного построения
	 *
	 * @note Построение, на многие строки растянутое, заводится следующим этапом работ:
	 *       пока скобка обязана закрыться в той же строке, где открылась
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора незакрытого построения
		ASSERT_FALSE(reader.feed("a: [1, 2\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::UNCLOSED_FLOW);
	}
	/**
	 * Выполняем проверку закрытия построения скобкой чужого вида
	 *
	 * @note Скобка чужого вида, стоя за значением, отвергается ожиданием запятой либо
	 *       своей закрывающей скобки: так место отказа названо точнее, нежели одним лишь
	 *       незакрытым построением
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора построения, закрытого чужой скобкой
		ASSERT_FALSE(reader.feed("a: [1, 2}\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::EXPECTED_COMMA);
	}
}
/**
 * @brief Проверка разбора блочных значений
 *
 * @details Блочное значение собирается многими строками, и завершает его строка,
 * отступом не глубже заголовка. Оттого событие его выдаётся не там, где заголовок
 * прочитан, а там, где содержимое окончилось
 *
 */
TEST(CodecYamlReader, BlockScalars) {
	// Выполняем проверку дословного блочного значения
	ASSERT_EQ(events("text: |\n  первая\n  вторая\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «text»\n"
		"SCALAR «первая\nвторая\n»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку блочного значения со свёрткой строк
	 *
	 * @note Свёртка складывает строки пробелом, а последовательность переводов
	 *       уменьшает на один: `первая` и `вторая` складываются в одну, а два
	 *       перевода перед `третья` обращаются в один. Полностью правило это
	 *       закреплено проверкой `FoldedBreaks`
	 */
	ASSERT_EQ(events("text: >\n  первая\n  вторая\n\n  третья\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «text»\n"
		"SCALAR «первая вторая\nтретья\n»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку усечения переводов строк
	ASSERT_EQ(events("text: |-\n  одна\n\nnext: 1\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «text»\n"
		"SCALAR «одна»\nSCALAR «next»\nSCALAR «1»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку сохранения переводов строк
	ASSERT_EQ(events("text: |+\n  одна\n\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «text»\n"
		"SCALAR «одна\n\n»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку указателя отступа содержимого
	 *
	 * @note Указатель отсчитывается от отступа строки, заголовок несущей: при указателе
	 *       в два пробела содержимое `    два пробела` несёт два пробела своих
	 */
	ASSERT_EQ(events("text: |2\n    два пробела\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «text»\n"
		"SCALAR «  два пробела\n»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку блочного значения внутри перечня
	ASSERT_EQ(events("- |\n  одна\n- две\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\n"
		"SCALAR «одна\n»\nSCALAR «две»\n"
		"SEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку того, что блочное значение выдаётся строкой всегда
	 *
	 * @note Содержимое `12`, записанное блочным значением, есть строка: ограду ему
	 *       заменяет сам вид записи
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем подачу текста с блочным значением из одних цифр
		ASSERT_TRUE(reader.feed("text: |\n  12\n"));
		// Вид значения, разрешённый разбором
		yaml::type_t type = yaml::type_t::UNDEFINED;
		// Вид записи значения в исходном тексте
		yaml::style_t style = yaml::style_t::PLAIN;
		/**
		 * Выполняем перебор всех событий разбора
		 */
		while(reader.next()){
			/**
			 * Если событие несёт скалярное значение с содержимым числа
			 */
			if((reader.event() == yaml::event_t::SCALAR) && (reader.value().text.compare("12\n") == 0)){
				// Запоминаем вид разрешённого значения
				type = reader.value().type;
				// Запоминаем вид записи значения
				style = reader.value().style;
			}
		}
		// Выполняем проверку того, что значение выдано строкой
		ASSERT_EQ(type, yaml::type_t::STRING);
		// Выполняем проверку того, что вид записи сохранён
		ASSERT_EQ(style, yaml::style_t::LITERAL);
	}
	/**
	 * Выполняем проверку ошибочного построения заголовка блочного значения
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора заголовка с двумя правилами усечения
		ASSERT_FALSE(reader.feed("text: |-+\n  одна\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::INVALID_BLOCK_HEADER);
	}
}
/**
 * @brief Проверка разбора меток и ссылок на них
 *
 * @note Ссылки чтением не раскрываются: событие ссылки выдаётся как есть, а раскрытие
 *       её есть забота держащего документ целиком
 *
 */
TEST(CodecYamlReader, Anchors) {
	// Выполняем проверку метки над скалярным значением и ссылки на неё
	ASSERT_EQ(events("a: &я 1\nb: *я\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSCALAR «1» &я\nSCALAR «b»\nALIAS «я»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку метки над построением
	 *
	 * @note Метка стоит строкою выше построения своего, и дожидается она его: событие
	 *       открытия отображения забирает её себе
	 */
	ASSERT_EQ(events("a: &я\n  x: 1\nb: *я\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nMAPPING_START &я\nSCALAR «x»\nSCALAR «1»\nMAPPING_END\n"
		"SCALAR «b»\nALIAS «я»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку метки над значением перечня
	ASSERT_EQ(events("- &я один\n- *я\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\n"
		"SCALAR «один» &я\nALIAS «я»\n"
		"SEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку имени метки со знаками, ей дозволенными
	 *
	 * @note Описание запрещает имени метки лишь пробельные знаки да знаки, поточные
	 *       построения размечающие: восклицательный знак ему дозволен, и `&я!!str` есть
	 *       имя целиком, а не метка со слитою меткой типа
	 */
	ASSERT_EQ(events("a: &я!!str 1\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSCALAR «1» &я!!str\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку отказа ссылки на метку, ещё не объявленную
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора ссылки вперёд объявления
		ASSERT_FALSE(reader.feed("a: *нет\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::UNKNOWN_ALIAS);
	}
	/**
	 * Выполняем проверку отказа ссылки на метку другого документа
	 *
	 * @note Метка живёт ровно столько, сколько живёт документ, её объявивший: ссылка из
	 *       второго документа на метку первого описанием запрещена
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора ссылки через границу документа
		ASSERT_FALSE(reader.feed("---\na: &я 1\n---\nb: *я\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::UNKNOWN_ALIAS);
	}
	/**
	 * Выполняем проверку отказа свойства узла над ссылкой
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора свойства над ссылкой
		ASSERT_FALSE(reader.feed("a: &я 1\nb: &б *я\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::INVALID_CHARACTER);
	}
}
/**
 * @brief Проверка разбора меток типов
 *
 */
TEST(CodecYamlReader, Tags) {
	/**
	 * Выполняем проверку метки типа описания
	 *
	 * @note Сокращение `!!` раскрывается началом, описанием заданным, и наружу выдаётся
	 *       уже раскрытым: потребителю незачем знать, каким сокращением метка писана
	 */
	ASSERT_EQ(events("a: !!str 12\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSCALAR «12» <tag:yaml.org,2002:str>\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку метки типа местной
	ASSERT_EQ(events("a: !mine 12\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSCALAR «12» <!mine>\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку метки типа, записанной указателем дословно
	ASSERT_EQ(events("a: !<tag:x,2000:mine> 12\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSCALAR «12» <tag:x,2000:mine>\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку метки типа вместе с меткой узла
	ASSERT_EQ(events("a: &я !!str 12\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSCALAR «12» &я <tag:yaml.org,2002:str>\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку перебивания схемы меткой типа
	 *
	 * @note Схема разрешает вид по записи значения и оттого лишь угадывает, а метка типа
	 *       сказывает вид прямо: `!!str 12` есть строка, схеме вопреки
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку разбора значения с меткой типа строки
		ASSERT_TRUE(reader.feed("a: !!str 12\n"));
		/**
		 * Выполняем перебор событий разбора до значения пары
		 */
		while(reader.next() && (reader.value().text.compare("12") != 0));
		// Выполняем проверку вида значения, меткой типа заданного
		ASSERT_EQ(reader.value().type, yaml::type_t::STRING);
	}
	/**
	 * Выполняем проверку отказа содержимого, метке типа не отвечающего
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора строки под меткой целого
		ASSERT_FALSE(reader.feed("a: !!int строка\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::TAG_MISMATCH);
	}
	/**
	 * Выполняем проверку отказа скалярной метки типа над построением
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора отображения под меткой строки
		ASSERT_FALSE(reader.feed("a: !!str\n  x: 1\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::TAG_MISMATCH);
	}
	/**
	 * Выполняем проверку отказа необъявленного сокращения метки типа
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора необъявленного сокращения
		ASSERT_FALSE(reader.feed("a: !e!mine 1\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::UNKNOWN_TAG_HANDLE);
	}
	/**
	 * Выполняем проверку отказа знака, записи метки типа не принадлежащего
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора метки типа с письмом иным
		ASSERT_FALSE(reader.feed("a: !своё 1\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::INVALID_TAG);
	}
}
/**
 * @brief Проверка разбора директив, документу предпосланных
 *
 */
TEST(CodecYamlReader, Directives) {
	// Выполняем проверку директивы наречия текста
	ASSERT_EQ(events("%YAML 1.2\n---\na: 1\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSCALAR «1»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку раскрытия сокращения, директивой объявленного
	ASSERT_EQ(events("%TAG !e! tag:example.com,2000:\n---\na: !e!mine 1\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSCALAR «1» <tag:example.com,2000:mine>\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку правки схемы разрешения директивой наречия
	 *
	 * @details Наречие 1.1 разрешает виды иначе: `yes` там есть истина. Объявив наречие,
	 * текст сказал об этом прямо, и читать его схемою ядровой значило бы прочесть не то,
	 * что писано
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку разбора текста наречия 1.1
		ASSERT_TRUE(reader.feed("%YAML 1.1\n---\na: yes\n"));
		/**
		 * Выполняем перебор событий разбора до значения пары
		 */
		while(reader.next() && (reader.value().text.compare("yes") != 0));
		// Выполняем проверку вида значения, схемою наречия 1.1 разрешённого
		ASSERT_EQ(reader.value().type, yaml::type_t::BOOL);
	}
	/**
	 * Выполняем проверку возврата схемы разрешения по закрытии документа
	 *
	 * @note Наречие объявляется документом и живёт ровно столько, сколько живёт он сам:
	 *       второй документ читается схемою, настройками назначенной
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку разбора двух документов с разными наречиями
		ASSERT_TRUE(reader.feed("%YAML 1.1\n---\na: yes\n---\nb: yes\n"));
		/**
		 * Выполняем перебор событий разбора до значения второго документа
		 */
		while(reader.next() && (reader.value().text.compare("b") != 0));
		// Выполняем переход к значению второй пары
		ASSERT_TRUE(reader.next());
		// Выполняем проверку вида значения, схемою ядровой разрешённого
		ASSERT_EQ(reader.value().type, yaml::type_t::STRING);
	}
	/**
	 * Выполняем проверку отказа неподдерживаемого наречия текста
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора неподдерживаемого наречия
		ASSERT_FALSE(reader.feed("%YAML 2.0\n---\na: 1\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::UNSUPPORTED_VERSION);
	}
	/**
	 * Выполняем проверку отказа документа с директивами, чертою не открытого
	 *
	 * @note Без черты неведомо, где кончаются директивы и начинается содержимое
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора директивы без черты за нею
		ASSERT_FALSE(reader.feed("%YAML 1.2\na: 1\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::INVALID_DIRECTIVE);
	}
	/**
	 * Выполняем проверку отказа директивы посреди документа
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора директивы посреди документа
		ASSERT_FALSE(reader.feed("a: 1\n%YAML 1.2\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::INVALID_DIRECTIVE);
	}
	/**
	 * Выполняем проверку отказа повторного объявления сокращения
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора повторного объявления сокращения
		ASSERT_FALSE(reader.feed("%TAG !e! tag:a\n%TAG !e! tag:b\n---\na: 1\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::INVALID_DIRECTIVE);
	}
	/**
	 * Выполняем проверку пропуска директивы, чьё имя не опознано
	 *
	 * @note Описание велит пропускать такие без отказа: наречия последующие вправе завести
	 *       свои, и текст, ими писанный, читающему прежнему понятен остаётся
	 */
	ASSERT_EQ(events("%НЕВЕДОМО что\n---\na: 1\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSCALAR «1»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
}
/**
 * @brief Проверка разбора поточных построений, на многие строки растянутых
 *
 * @details Внутри скобок отступ не значит ничего, и строка, построению принадлежащая,
 * разбирается целиком им, а не отступом своим. Стопа открытых скобок держится полем
 * разбора, а не возвратностью вызовов: иначе прервать разбор концом строки и продолжить
 * его со следующей было бы нечем
 *
 */
TEST(CodecYamlReader, FlowLines) {
	// Выполняем проверку разбора перечня, на многие строки растянутого
	ASSERT_EQ(events("a: [\n  1,\n  2\n]\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSEQUENCE_START\nSCALAR «1»\nSCALAR «2»\nSEQUENCE_END\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку разбора отображения, на многие строки растянутого
	 *
	 * @note Строка за закрывающей скобкой разбирается вновь обычным порядком, и пара `b`
	 *       принадлежит тому же отображению, что и пара `a`
	 */
	ASSERT_EQ(events("a: {\n  x: 1,\n  y: два\n}\nb: 3\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nMAPPING_START\nSCALAR «x»\nSCALAR «1»\nSCALAR «y»\nSCALAR «два»\nMAPPING_END\n"
		"SCALAR «b»\nSCALAR «3»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку разбора вложенных построений, на многие строки растянутых
	ASSERT_EQ(events("a: [\n  [1,\n   2],\n  {k: v}\n]\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSEQUENCE_START\n"
		"SEQUENCE_START\nSCALAR «1»\nSCALAR «2»\nSEQUENCE_END\n"
		"MAPPING_START\nSCALAR «k»\nSCALAR «v»\nMAPPING_END\n"
		"SEQUENCE_END\nMAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку примечаний внутри поточного построения
	 *
	 * @note Примечание внутри скобок дозволено, и оканчивается им строка: остаток её
	 *       содержимым построения не является
	 */
	{
		// Настройки разбора текста
		yaml::reader_t::settings_t settings;
		// Устанавливаем признак выдачи примечаний отдельным событием
		settings.emitComments = true;
		// Объект потокового чтения текста
		yaml::reader_t reader(settings);
		// Выполняем проверку разбора построения с примечаниями внутри
		ASSERT_TRUE(reader.feed("a: [ # сбоку\n  1\n]\n"));
		// Собираемый ряд названий событий разбора
		string result;
		/**
		 * Выполняем перебор всех собранных событий разбора
		 */
		while(reader.next()){
			// Выполняем запись названия очередного события
			result.append(yaml::name(reader.event()));
			/**
			 * Если событие несёт примечание
			 */
			if(reader.event() == yaml::event_t::COMMENT)
				// Выполняем запись содержимого примечания
				result.append(" «").append(reader.value().text).append("»");
			// Выполняем запись разделителя событий
			result.append("\n");
		}
		// Выполняем проверку выдачи примечания внутри построения
		ASSERT_NE(result.find("COMMENT «сбоку»"), string::npos) << result;
	}
	/**
	 * Выполняем проверку отказа построения, скобкой так и не закрытого
	 *
	 * @note Отказ объявляется концом текста, а не закрытием документа: закрытие выдало бы
	 *       события закрытия построений, которых текст не содержит
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора незакрытого построения
		ASSERT_FALSE(reader.feed("a: [\n  1,\n  2\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::UNCLOSED_FLOW);
	}
	// Выполняем проверку метки и ссылки внутри поточного построения
	ASSERT_EQ(events("a: [&я 1, *я]\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSEQUENCE_START\nSCALAR «1» &я\nALIAS «я»\nSEQUENCE_END\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку метки типа внутри поточного построения
	ASSERT_EQ(events("a: [!!str 1]\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSEQUENCE_START\nSCALAR «1» <tag:yaml.org,2002:str>\nSEQUENCE_END\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
}
/**
 * @brief Проверка разбора простых значений, на многие строки растянутых
 *
 * @details Простое значение, добежавшее до конца строки, вправе продолжиться строкою
 * ниже, и знать о том по самой строке нечем: узнаётся это лишь отступом строки
 * следующей. Оттого выдача всякого такого значения откладывается до неё
 *
 * @note Отступ, продолжение задающий, берётся у построения, значение объемлющего, а не
 *       у строки, в которой значение началось: запись `ключ:` со значением строкою ниже
 *       ставит продолжение на тот же отступ, что и начало
 *
 */
TEST(CodecYamlReader, PlainLines) {
	// Выполняем проверку значения, продолженного строкою ниже
	ASSERT_EQ(events("key: длинное\n  продолжение\nnext: 1\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «key»\nSCALAR «длинное продолжение»\nSCALAR «next»\nSCALAR «1»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку значения, целиком стоящего под именем своей пары
	ASSERT_EQ(events("key:\n  первая\n  вторая\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «key»\nSCALAR «первая вторая»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку пустой строки внутри значения
	 *
	 * @note Свёртка описанием задана так: перевод строки один обращается пробелом, а
	 *       каждый следующий остаётся переводом
	 */
	ASSERT_EQ(events("key: первая\n\n  вторая\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «key»\nSCALAR «первая\nвторая»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку хвостовых пустых строк значения
	 *
	 * @note Пустые строки, содержимого так и не дождавшиеся, пропадают: перевод строки
	 *       обращается содержимым лишь приходом содержимого за ним
	 */
	ASSERT_EQ(events("key: одна\n\n\nnext: 1\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «key»\nSCALAR «одна»\nSCALAR «next»\nSCALAR «1»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку значения перечня, продолженного строкою ниже
	ASSERT_EQ(events("- один\n  продолжение\n- два\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\n"
		"SCALAR «один продолжение»\nSCALAR «два»\n"
		"SEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку значения вложенного перечня, продолженного строкою ниже
	ASSERT_EQ(events("a:\n  - один\n    продолжение\n  - два\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSEQUENCE_START\nSCALAR «один продолжение»\nSCALAR «два»\nSEQUENCE_END\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку содержимого документа, продолженного строкою ниже
	ASSERT_EQ(events("просто строка\n  продолжение\n"),
		"STREAM_START\nDOCUMENT_START\n"
		"SCALAR «просто строка продолжение»\n"
		"DOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку примечания, значение завершающего
	 *
	 * @note Примечание значение завершает: описание примечаний внутри значения не знает
	 *       вовсе. Выдаётся оно за значением, которым завершено
	 */
	{
		// Настройки разбора текста
		yaml::reader_t::settings_t settings;
		// Устанавливаем признак выдачи примечаний отдельным событием
		settings.emitComments = true;
		// Объект потокового чтения текста
		yaml::reader_t reader(settings);
		// Выполняем проверку разбора значения, примечанием завершённого
		ASSERT_TRUE(reader.feed("key: первая\n  вторая # сбоку\nnext: 1\n"));
		// Собираемый ряд названий событий разбора
		string result;
		/**
		 * Выполняем перебор всех собранных событий разбора
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
		// Выполняем проверку выдачи значения прежде примечания, его завершившего
		ASSERT_EQ(result,
			"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
			"SCALAR «key»\nSCALAR «первая вторая»\nCOMMENT «сбоку»\nSCALAR «next»\nSCALAR «1»\n"
			"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	}
	/**
	 * Выполняем проверку положения значения, на многие строки растянутого
	 *
	 * @note Событие ставится там, где значение окончилось, а стоит оно там, где началось:
	 *       потребителю указывать надлежит на начало записи, а не на конец её
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку разбора значения, на две строки растянутого
		ASSERT_TRUE(reader.feed("key: длинное\n  продолжение\n"));
		/**
		 * Выполняем перебор событий разбора до значения пары
		 */
		while(reader.next() && (reader.value().text.compare("длинное продолжение") != 0));
		// Выполняем проверку строки, где значение началось
		ASSERT_EQ(reader.value().location.line, 1u);
		// Выполняем проверку положения начала значения в строке
		ASSERT_EQ(reader.value().location.column, 6u);
	}
	/**
	 * Выполняем проверку отказа пары внутри простого значения
	 *
	 * @note Простое значение разделителя имени пары нести не вправе вовсе: неведомо, пара
	 *       ли это внутри значения либо значение с двоеточием
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора пары внутри значения
		ASSERT_FALSE(reader.feed("key: первая\n  вторая: 1\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::INVALID_CHARACTER);
	}
	/**
	 * Выполняем проверку отказа перечня внутри простого значения
	 */
	{
		// Объект потокового чтения текста
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора перечня внутри значения
		ASSERT_FALSE(reader.feed("key: первая\n  - один\n"));
		// Выполняем проверку кода ошибки разбора
		ASSERT_EQ(reader.error(), yaml::error_t::INVALID_CHARACTER);
	}
}
/**
 * @brief Проверка отказа приведения кодировки посреди текста
 *
 * @details Приведение переносит в накопитель всё, что успело прочесть до битой
 * последовательности, и строки те разобрать надлежит прежде отказа: при подаче по байту
 * они разобраны были бы кусками прежними, и отказ пришёл бы уже за ними
 *
 * @note Расхождение это нашёл ворошитель нарезки: подача целиком выдавала одно событие,
 *       а подача по байту - тринадцать
 *
 */
TEST(CodecYamlReader, BrokenEncoding) {
	// Разбираемый текст с битой последовательностью посреди его
	const string text("a: 1\nb: 2\nc: \xE6\xBB\x64\n");
	// Объект потокового чтения текста
	yaml::reader_t reader;
	// Выполняем проверку отказа разбора битой последовательности
	ASSERT_FALSE(reader.feed(text));
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(reader.error(), yaml::error_t::INVALID_ENCODING);
	// Собираемый ряд названий событий разбора
	string result;
	/**
	 * Выполняем перебор всех собранных событий разбора
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
	/**
	 * Выполняем проверку выдачи строк, прочитанных до битой последовательности
	 *
	 * @note Строка последняя, битую последовательность несущая, не выдаётся: прочитана
	 *       она не была. Не выдаются и события закрытия - текст окончен отказом, а не
	 *       концом своим, и закрывать построения было бы неправдой
	 *
	 * @note Значение `2` не выдаётся тоже, и это верно: простое значение, до конца строки
	 *       добежавшее, вправе прирасти строкою ниже, и выдача его отложена. Строка
	 *       следующая пришла битой, отложенное так и не выдано, а отказ выдать его не
	 *       вправе - неведомо, чем оно кончилось бы
	 */
	ASSERT_EQ(result,
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSCALAR «1»\nSCALAR «b»\n");
	// Признак успешного разбора текста, поданного по байту
	bool piece = false;
	// Выполняем проверку того, что нарезка исход разбора не изменила
	ASSERT_EQ(events(text, 1, piece), events(text, text.size(), piece));
}
/**
 * @brief Проверка отсчёта отступа блочного значения от объемлющего построения
 *
 * @details Содержимое блочного значения обязано стоять глубже объемлющего построения, а
 *          не глубже начала строки: написание `- имя: |` кладёт отображение на отступ два,
 *          строку же открывает черта на отступе ноль. Отсчёт от строки брал бы содержимым
 *          и следующую пару того же отображения. Указатель отступа заголовка отсчитывается
 *          оттуда же
 *
 * @note Нашёл это ворошитель сличением перезаписи: пара, за пустым блочным значением
 *       стоящая, уходила содержимым его, а указатель отступа съезжал круг от круга
 *
 */
TEST(CodecYamlReader, BlockOuterIndent) {
	// Выполняем проверку окончания пустого блочного значения парой того же отображения
	ASSERT_EQ(events("- b: |+\n  port: 1\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\nMAPPING_START\n"
		"SCALAR «b»\nSCALAR «»\nSCALAR «port»\nSCALAR «1»\n"
		"MAPPING_END\nSEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку окончания непустого блочного значения парой того же отображения
	ASSERT_EQ(events("- b: |\n    x\n  port: 1\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\nMAPPING_START\n"
		"SCALAR «b»\nSCALAR «x\n»\nSCALAR «port»\nSCALAR «1»\n"
		"MAPPING_END\nSEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку отсчёта указателя отступа от объемлющего построения
	 *
	 * @note Отображение стоит на отступе два, указатель задаёт ещё два, и содержимое
	 *       начинается с отступа четыре: три пробела перед записью суть содержимое её
	 */
	ASSERT_EQ(events("- b: |2\n       99\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\nMAPPING_START\n"
		"SCALAR «b»\nSCALAR «   99\n»\n"
		"MAPPING_END\nSEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
}
/**
 * @brief Проверка отказа записи, глубже завершённой пары стоящей
 *
 * @details Отображение глубже открытого построения заводится лишь значением пары,
 *          объявленной прежде: написание `имя:` со значением строкою ниже того и
 *          требует. Пара же, стоящая глубже уже завершённой пары, значением быть
 *          некому, и описание такое написание запрещает
 *
 * @note Нашёл это ворошитель правкой дерева, а сличение подтвердило: libyaml и
 *       libfyaml отвечают отказом «did not find expected key», а чтение выдумывало
 *       вложенное отображение молча - дерево выходило исходному тексту не отвечающим
 *
 */
TEST(CodecYamlReader, DeeperEntry) {
	// Выполняем проверку отказа пары глубже завершённой пары с поточным значением
	ASSERT_EQ(events("a:\n  b: {x: 1}\n      c: 2\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «a»\nMAPPING_START\nSCALAR «b»\n"
		"MAPPING_START\nSCALAR «x»\nSCALAR «1»\nMAPPING_END\n"
		"ОТКАЗ отступ не отвечает ни одному из открытых уровней строка 3 знак 7\n");
	// Выполняем проверку отказа пары глубже завершённой пары через пустую строку
	ASSERT_EQ(events("a:\n  b: {x: 1}\n\n      c: 2\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «a»\nMAPPING_START\nSCALAR «b»\n"
		"MAPPING_START\nSCALAR «x»\nSCALAR «1»\nMAPPING_END\n"
		"ОТКАЗ отступ не отвечает ни одному из открытых уровней строка 4 знак 7\n");
	// Выполняем проверку отказа пары глубже завершённой пары с перечнем значением
	ASSERT_EQ(events("a:\n  b: [1]\n    c: 2\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «a»\nMAPPING_START\nSCALAR «b»\n"
		"SEQUENCE_START\nSCALAR «1»\nSEQUENCE_END\n"
		"ОТКАЗ отступ не отвечает ни одному из открытых уровней строка 3 знак 5\n");
	/**
	 * Выполняем проверку принятия вложенности в четыре уровня
	 *
	 * @note Отказ обязан отделять написание запрещённое от вложенности обыкновенной:
	 *       всякий уровень её стоит глубже прежнего, и запрет без этого разбора съел
	 *       бы вложенность целиком
	 */
	ASSERT_EQ(events("a:\n  b:\n    c: 1\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «a»\nMAPPING_START\nSCALAR «b»\n"
		"MAPPING_START\nSCALAR «c»\nSCALAR «1»\n"
		"MAPPING_END\nMAPPING_END\nMAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку принятия пары внутри записи перечня
	ASSERT_EQ(events("- a: 1\n  b: 2\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\nMAPPING_START\n"
		"SCALAR «a»\nSCALAR «1»\nSCALAR «b»\nSCALAR «2»\n"
		"MAPPING_END\nSEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
}
/**
 * @brief Проверка свёртки переводов строк блочного значения
 *
 * @details Свёртка складывает строки пробелом, а последовательность переводов
 *          уменьшает на один. Применяется она лишь тогда, когда обе строки,
 *          переводом разделяемые, стоят ровно на отступе содержимого: строка,
 *          стоящая глубже, свёртке не подлежит ни собою, ни соседкой своей, и
 *          переводы вокруг неё сохраняются все до единого
 *
 * @note Нашлось это сличением со стендами соперников: выдача расходилась с libyaml
 *       и libfyaml, а те между собою сходились. Расхождение оказалось изъяном
 *       разбора - пустая строка давала два перевода вместо одного, а перевод за
 *       строкой, глубже стоящей, сворачивался пробелом, - и все шесть случаев,
 *       здесь закреплённых, сличены с выдачею обеих эталонных реализаций
 *
 */
TEST(CodecYamlReader, FoldedBreaks) {
	// Выполняем проверку свёртки одной пустой строки в один перевод
	ASSERT_EQ(events(">\n  a\n  b\n\n  c\n"),
		"STREAM_START\nDOCUMENT_START\nSCALAR «a b\nc\n»\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку сохранения переводов вокруг строки, глубже стоящей
	ASSERT_EQ(events(">\n  a\n   b\n  c\n"),
		"STREAM_START\nDOCUMENT_START\nSCALAR «a\n b\nc\n»\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку свёртки двух пустых строк в два перевода
	ASSERT_EQ(events(">\n  a\n\n\n  b\n"),
		"STREAM_START\nDOCUMENT_START\nSCALAR «a\n\nb\n»\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку сохранения пустой строки между строками, глубже стоящими
	ASSERT_EQ(events(">\n  a\n   b\n\n   c\n"),
		"STREAM_START\nDOCUMENT_START\nSCALAR «a\n b\n\n c\n»\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку свёртки при усечении переводов, содержимое завершающих
	ASSERT_EQ(events(">-\n  a\n\n  b\n"),
		"STREAM_START\nDOCUMENT_START\nSCALAR «a\nb»\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку сохранения пустых строк, содержимому предпосланных
	ASSERT_EQ(events(">\n\n  a\n"),
		"STREAM_START\nDOCUMENT_START\nSCALAR «\na\n»\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку сохранения переводов у значения, переводы хранящего
	 *
	 * @note Свёртка вида этого не касается вовсе: переводы сохраняются все, и правка
	 *       свёртки обязана оставить их нетронутыми
	 */
	ASSERT_EQ(events("|\n  a\n\n  b\n"),
		"STREAM_START\nDOCUMENT_START\nSCALAR «a\n\nb\n»\nDOCUMENT_END\nSTREAM_END\n");
}
/**
 * @brief Проверка сличения метки типа с видом поточного построения
 *
 * @details Сличение это едино для построений блочных и поточных: метка `!!str` над
 *          перечнем есть расхождение объявленного с записанным где угодно
 *
 * @note Нашёл это ворошитель сличением перезаписи: поточное построение метку не сличало
 *       вовсе, и текст, чтением принятый, перезаписью отвергался
 *
 */
TEST(CodecYamlReader, FlowTags) {
	// Выполняем проверку отказа метки скалярного значения над поточным перечнем
	ASSERT_EQ(events("- !!str [ 1 ]\n"),
		"STREAM_START\n"
		"ОТКАЗ содержимое не отвечает виду, заданному меткой типа строка 1 знак 9\n");
	// Выполняем проверку отказа метки перечня над поточным отображением
	ASSERT_EQ(events("- !!seq { a: 1 }\n"),
		"STREAM_START\n"
		"ОТКАЗ содержимое не отвечает виду, заданному меткой типа строка 1 знак 9\n");
	// Выполняем проверку принятия метки перечня над поточным перечнем
	ASSERT_EQ(events("- !!seq [ 1 ]\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\nSEQUENCE_START <tag:yaml.org,2002:seq>\nSCALAR «1»\n"
		"SEQUENCE_END\nSEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку отказа метки скалярного значения над вложенным поточным перечнем
	ASSERT_EQ(events("- [ !!str [ 1 ] ]\n"),
		"STREAM_START\n"
		"ОТКАЗ содержимое не отвечает виду, заданному меткой типа строка 1 знак 11\n");
}
