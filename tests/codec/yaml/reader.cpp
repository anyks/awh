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
	ASSERT_EQ(events("a: [\n  1,\n  2\n  ]\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nSEQUENCE_START\nSCALAR «1»\nSCALAR «2»\nSEQUENCE_END\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку разбора отображения, на многие строки растянутого
	 *
	 * @note Строка за закрывающей скобкой разбирается вновь обычным порядком, и пара `b`
	 *       принадлежит тому же отображению, что и пара `a`
	 */
	ASSERT_EQ(events("a: {\n  x: 1,\n  y: два\n  }\nb: 3\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\n"
		"SCALAR «a»\nMAPPING_START\nSCALAR «x»\nSCALAR «1»\nSCALAR «y»\nSCALAR «два»\nMAPPING_END\n"
		"SCALAR «b»\nSCALAR «3»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	// Выполняем проверку разбора вложенных построений, на многие строки растянутых
	ASSERT_EQ(events("a: [\n  [1,\n   2],\n  {k: v}\n  ]\n"),
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
		ASSERT_TRUE(reader.feed("a: [ # сбоку\n  1\n  ]\n"));
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
	 * Выполняем проверку принятия черты записи перечня внутри простого значения
	 *
	 * @details Запрет черты касается лишь первого знака простого значения: описание
	 * ограничивает `ns-plain-first`, а строки последующие собираются знаками
	 * `ns-plain-char`, среди коих черта дозволена. Строка глубже отступа объемлющего
	 * построения записью перечня быть не может - записи стоят отступом своим
	 *
	 * @note Прежде тут стоял отказ с доводом, будто записи эти описанием запрещены
	 *       прямо. Довод проверки не выдержал: libyaml и libfyaml обе складывают такую
	 *       строку в значение. Двоеточие же внутри значения обе отвергают - оттого
	 *       проверка выше отказ свой и сохраняет
	 */
	ASSERT_EQ(events("key: первая\n  - один\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «key»\nSCALAR «первая - один»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
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
 * @brief Проверка отказа записи перечня в строке имени пары
 *
 * @details Перечень блочного построения строки имени своей пары не занимает: записи
 *          его отделяются переводом строки и стоят отступом своим
 *
 * @note Нашёл это ворошитель правкой дерева, а сличение подтвердило: libyaml отвечает
 *       отказом «block sequence entries are not allowed in this context», а чтение
 *       выдавало перечень, отступ какого исходному тексту не отвечает
 *
 */
TEST(CodecYamlReader, SequenceOnKeyLine) {
	// Выполняем проверку отказа черты записи перечня за именем пары
	ASSERT_EQ(events("key: - a\n"),
		"STREAM_START\n"
		"ОТКАЗ перечень и отображение смешаны на одном уровне строка 1 знак 6\n");
	// Выполняем проверку отказа одинокой черты за именем пары
	ASSERT_EQ(events("key: -\n"),
		"STREAM_START\n"
		"ОТКАЗ перечень и отображение смешаны на одном уровне строка 1 знак 6\n");
	/**
	 * Выполняем проверку принятия перечня строкою ниже имени пары
	 *
	 * @note Написание это описанием дозволено: перечень стоит на отступе имени своей
	 *       пары, и отказ обязан отделять его от черты в строке имени
	 */
	ASSERT_EQ(events("key:\n- a\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «key»\nSEQUENCE_START\nSCALAR «a»\n"
		"SEQUENCE_END\nMAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку принятия отрицательного числа значением пары
	 *
	 * @note Черта записи перечня отделяется пробелом, а черта числа - нет: отказ обязан
	 *       различать их, иначе всякое отрицательное число стало бы отказом
	 */
	ASSERT_EQ(events("key: -5\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «key»\nSCALAR «-5»\n"
		"MAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку принятия поточного перечня в строке имени пары
	 *
	 * @note Поточное построение строку имени занимать вправе: скобки отступом не ведают
	 */
	ASSERT_EQ(events("key: [1]\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «key»\nSEQUENCE_START\nSCALAR «1»\n"
		"SEQUENCE_END\nMAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку принятия сжатого написания записи перечня
	 *
	 * @note Черта за чертою дозволена: перечень внутри перечня строку делить вправе,
	 *       и запрет касается лишь строки имени пары
	 */
	ASSERT_EQ(events("- - a\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\nSEQUENCE_START\nSCALAR «a»\n"
		"SEQUENCE_END\nSEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
}
/**
 * @brief Проверка отказа значения на отступе открытого перечня
 *
 * @details Перечень несёт одни записи свои, чертою объявленные: значение, на отступе
 *          его стоящее и чертою не объявленное, ни записью перечня не является, ни
 *          объемлющему построению не принадлежит — отступ у него не тот
 *
 * @note Нашёл это ворошитель правкой дерева, а сличение подтвердило: libyaml отвечает
 *       отказом «could not find expected ':'», libfyaml — «invalid scalar at the end
 *       of block sequence», а чтение выдавало значение записью перечня молча
 *
 */
TEST(CodecYamlReader, SequenceScalar) {
	// Выполняем проверку отказа значения, чертою записи не объявленного
	ASSERT_EQ(events("a:\n  - x\n  y\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «a»\nSEQUENCE_START\nSCALAR «x»\n"
		"ОТКАЗ отступ не отвечает ни одному из открытых уровней строка 3 знак 3\n");
	/**
	 * Выполняем проверку отказа значения, чертою открывающегося
	 *
	 * @note Черта, пробелом не отделённая, записи перечня не объявляет: `-e` есть
	 *       значение простое, и отказ обязан прийти ему наравне с прочими
	 */
	ASSERT_EQ(events("a:\n  - x\n  -e\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «a»\nSEQUENCE_START\nSCALAR «x»\n"
		"ОТКАЗ отступ не отвечает ни одному из открытых уровней строка 3 знак 3\n");
	/**
	 * Выполняем проверку отказа значения глубже отступа перечня
	 *
	 * @note Отступ, ни одному уровню не отвечающий, libyaml отвергает отказом «did not
	 *       find expected '-' indicator»: значение глубже перечня есть либо значение
	 *       записи его, либо продолжение простого значения, а тут ни то, ни другое
	 */
	ASSERT_EQ(events("-\n  -\n 9\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\nSEQUENCE_START\n"
		"ОТКАЗ отступ не отвечает ни одному из открытых уровней строка 3 знак 2\n");
	/**
	 * Выполняем проверку принятия значения записи перечня строкою ниже черты
	 *
	 * @note Значение это стоит глубже отступа перечня по праву: черта его объявила, и
	 *       отказ обязан отделять его от значения, чертою не объявленного
	 */
	ASSERT_EQ(events("-\n  x\n"),
		"STREAM_START\nDOCUMENT_START\nSEQUENCE_START\nSCALAR «x»\n"
		"SEQUENCE_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку принятия продолжения простого значения
	 *
	 * @note Простое значение вправе продолжиться строкою ниже, и отступ продолжения
	 *       глубже отступа перечня: отказ обязан отделять продолжение от значения,
	 *       чертою не объявленного
	 */
	ASSERT_EQ(events("a:\n  - x\n    y\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «a»\nSEQUENCE_START\nSCALAR «x y»\n"
		"SEQUENCE_END\nMAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
	/**
	 * Выполняем проверку принятия пары с пустым именем внутри записи перечня
	 *
	 * @note Эталонные реализации тут расходятся: libfyaml запись принимает, libyaml
	 *       отвечает отказом «did not find expected key». Описание наречия 1.2 пару с
	 *       пустым именем дозволяет, и принимается она вслед за libfyaml
	 */
	ASSERT_EQ(events("a:\n  - :\n"),
		"STREAM_START\nDOCUMENT_START\nMAPPING_START\nSCALAR «a»\nSEQUENCE_START\nMAPPING_START\n"
		"SCALAR «»\nSCALAR «»\nMAPPING_END\nSEQUENCE_END\nMAPPING_END\nDOCUMENT_END\nSTREAM_END\n");
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
/**
 * @brief Проверка отказа скалярного значения, парою отображения не объявленного
 *
 * @details Значение, на отступе открытого отображения стоящее и двоеточия за собою не
 *          имеющее, ни именем пары не является, ни значением её. Разбор ронял его молча,
 *          и строка пропадала из дерева бесследно. Отказ этот отвечает rapidyaml с
 *          libyaml: обе отвергают такое написание. Нашёл это ворошитель правкой дерева
 *
 */
TEST(CodecYamlReader, KeylessScalar) {
	// Объект дерева документа
	yaml::document_t doc;
	// Выполняем проверку отказа значения без имени под парою отображения
	ASSERT_FALSE(doc.parse("a: 1\nb\n"));
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(doc.error(), yaml::error_t::INVALID_INDENTATION);
	// Выполняем проверку отказа значения без имени внутри записи перечня
	ASSERT_FALSE(doc.parse("- a: 1\n  b\n"));
	// Выполняем проверку отказа значения без имени за вложенным отображением
	ASSERT_FALSE(doc.parse("- value:\n   имя: текст\n  о\n"));
	// Выполняем проверку того, что продолжение простого значения отказа не даёт
	ASSERT_TRUE(doc.parse("a: текст\n  продолжение\n"));
	// Выполняем проверку собранного простого значения
	ASSERT_EQ(doc.root().at("/a").text(), "текст продолжение");
	// Выполняем проверку того, что вложенное отображение отказа не даёт
	ASSERT_TRUE(doc.parse("a:\n  b: 1\n"));
	// Выполняем проверку того, что примечание между парами отказа не даёт
	ASSERT_TRUE(doc.parse("a: 1\n\n# примечание\nb: 2\n"));
}
/**
 * @brief Проверка строгости чтения, сличением с эталоном вскрытой
 *
 * @details Случаи эти взяты из набора yaml-test-suite: тексты негодные, какие чтение
 *          принимало молча. Ворошитель их не ловил и поймать не мог - он сличает нас с
 *          нами же, а тут мы неверно понимали стандарт
 *
 */
TEST(CodecYamlReader, Strictness) {
	// Объект дерева документа
	yaml::document_t doc;
	/**
	 * Выполняем проверку отказа содержимого за чертой конца документа
	 *
	 * @note Черта конца занимает строку свою целиком: описанием за нею дозволено одно
	 *       примечание. Прежде слово `invalid` пропадало из потока бесследно
	 */
	ASSERT_FALSE(doc.parse("---\nkey: value\n... invalid\n"));
	// Выполняем проверку того, что примечание за чертою конца дозволено
	ASSERT_TRUE(doc.parse("---\nkey: value\n... # примечание\n"));
	// Выполняем проверку того, что черта конца сама по себе дозволена
	ASSERT_TRUE(doc.parse("---\nkey: value\n...\n"));
	/**
	 * Выполняем проверку отказа примечания без пробела перед ним
	 *
	 * @note Знак примечания дозволен лишь за пробельным знаком либо с начала строки:
	 *       запись `]#invalid` примечания не открывает
	 */
	ASSERT_FALSE(doc.parse("---\n[ a, b, c, ]#invalid\n"));
	// Выполняем проверку того, что примечание за пробелом дозволено
	ASSERT_TRUE(doc.parse("---\n[ a, b, c, ] # примечание\n"));
	/**
	 * Выполняем проверку отказа значения, знаком примечания открытого
	 *
	 * @note Внутри построения знак этот значения не открывает, а примечанием там он
	 *       тоже не является - пробела перед ним нет
	 */
	ASSERT_FALSE(doc.parse("---\n[ a, b, c,#invalid\n]\n"));
	// Выполняем проверку того, что примечание внутри построения дозволено
	ASSERT_TRUE(doc.parse("---\n[ a, b, c, # примечание\n]\n"));
	/**
	 * Выполняем проверку того, что знак примечания внутри значения дозволен
	 *
	 * @note Запись `this is#not` есть простое значение целиком: примечания знак тот не
	 *       открывает, пробела перед ним нет
	 */
	ASSERT_TRUE(doc.parse("this is#not: a comment\n"));
	// Выполняем проверку собранного значения с знаком примечания внутри
	ASSERT_EQ(doc.root().at("/this is#not").text(), "a comment");
}
/**
 * @brief Проверка строгости поточных построений и директив
 *
 * @details Случаи взяты из набора yaml-test-suite и сверены с rapidyaml: тексты
 *          негодные, какие чтение принимало молча
 *
 */
TEST(CodecYamlReader, FlowStrictness) {
	// Объект дерева документа
	yaml::document_t doc;
	/**
	 * Выполняем проверку отказа пустой записи построения
	 *
	 * @note Разделитель отделяет записи друг от друга, и двум разделителям подряд
	 *       отделять нечего. Запятая перед скобкою закрывающей дозволена - она записи
	 *       не открывает, а закрывает последнюю
	 */
	ASSERT_FALSE(doc.parse("[ a, b, c, , ]\n"));
	// Выполняем проверку отказа построения об одной запятой
	ASSERT_FALSE(doc.parse("[ , ]\n"));
	// Выполняем проверку отказа пустой записи отображения
	ASSERT_FALSE(doc.parse("{ a: 1, , }\n"));
	// Выполняем проверку того, что запятая перед скобкою дозволена
	ASSERT_TRUE(doc.parse("[ a, b, c, ]\n"));
	// Выполняем проверку количества записей построения
	ASSERT_EQ(doc.root().size(), 3);
	/**
	 * Выполняем проверку отказа продолжения построения на отступе объемлющего
	 *
	 * @note Содержимое построения обязано стоять глубже построения блочного, его
	 *       объемлющего: иначе строка читалась бы и продолжением, и парою нового
	 *       отображения. Отвечает rapidyaml отказом «bad indentation»
	 */
	ASSERT_FALSE(doc.parse("flow: [a,\nb,\nc]\n"));
	// Выполняем проверку того, что продолжение глубже отступа дозволено
	ASSERT_TRUE(doc.parse("flow: [a,\n  b,\n  c]\n"));
	// Выполняем проверку количества записей построения
	ASSERT_EQ(doc.root().at("/flow").size(), 3);
	/**
	 * Выполняем проверку отказа черты документа внутри построения
	 *
	 * @note Черты эти внутри скобок содержимым не являются и построения не закрывают
	 */
	ASSERT_FALSE(doc.parse("[\n--- ,\n...\n]\n"));
	/**
	 * Выполняем проверку отказа директив без документа за ними
	 *
	 * @note Описание велит документу с директивами открываться чертою прямо
	 */
	ASSERT_FALSE(doc.parse("%YAML 1.2\n"));
	// Выполняем проверку отказа директивы с одною чертою конца за нею
	ASSERT_FALSE(doc.parse("%YAML 1.2\n...\n"));
	// Выполняем проверку того, что директива с чертою начала дозволена
	ASSERT_TRUE(doc.parse("%YAML 1.2\n---\n"));
}
/**
 * @brief Проверка строгости имён пар, примечаний и черт записей
 *
 * @details Случаи взяты из набора yaml-test-suite и сверены с rapidyaml
 *
 */
TEST(CodecYamlReader, ColonStrictness) {
	// Объект дерева документа
	yaml::document_t doc;
	/**
	 * Выполняем проверку отказа двух имён пар в одной строке
	 *
	 * @note Запись `a: b: c` двусмысленна: неведомо, пара ли это со значением `b: c`
	 *       либо отображение внутри пары. Отвечает rapidyaml отказом «two consecutive
	 *       keys»
	 */
	ASSERT_FALSE(doc.parse("a: b: c: d\n"));
	// Выполняем проверку отказа второго имени пары в ограде
	ASSERT_FALSE(doc.parse("---\na: 'b': c\n"));
	/**
	 * Выполняем проверку того, что двоеточие без пробела разделителем не является
	 */
	ASSERT_TRUE(doc.parse("время: 12:30\nадрес: http://example.com\n"));
	// Выполняем проверку собранного значения с двоеточием внутри
	ASSERT_EQ(doc.root().at("/адрес").text(), "http://example.com");
	// Выполняем проверку того, что пара внутри записи перечня дозволена
	ASSERT_TRUE(doc.parse("- a: b\n"));
	// Выполняем проверку того, что поточное отображение в строке имени дозволено
	ASSERT_TRUE(doc.parse("a: {b: c}\n"));
	/**
	 * Выполняем проверку отказа примечания без пробела за оградою значения
	 */
	ASSERT_FALSE(doc.parse("key: \"value\"# invalid\n"));
	// Выполняем проверку того, что примечание за пробелом дозволено
	ASSERT_TRUE(doc.parse("key: \"value\" # примечание\n"));
	/**
	 * Выполняем проверку отказа примечания, к заголовку блочного значения приклеенного
	 */
	ASSERT_FALSE(doc.parse("block: ># comment\n  scalar\n"));
	// Выполняем проверку того, что примечание за заголовком дозволено
	ASSERT_TRUE(doc.parse("block: > # примечание\n  scalar\n"));
	/**
	 * Выполняем проверку отказа одинокой черты записи внутри построения
	 *
	 * @note Внутри скобок черта смысла не имеет - записи там отделяет запятая, - а
	 *       простым значением она быть не вправе. Отвечает rapidyaml отказом «invalid
	 *       scalar»
	 */
	ASSERT_FALSE(doc.parse("[-]\n"));
	// Выполняем проверку отказа черт записей построения перечня
	ASSERT_FALSE(doc.parse("[-, -]\n"));
	// Выполняем проверку того, что отрицательное число под правило не подпадает
	ASSERT_TRUE(doc.parse("[a, -1]\n"));
	// Выполняем проверку собранного отрицательного числа
	ASSERT_EQ(doc.root()[1].text(), "-1");
}
/**
 * @brief Проверка строгости отступа пустых строк блочного значения
 *
 * @details Случаи взяты из набора yaml-test-suite (5LLU, S98Z, W9L4) и сверены с rapidyaml
 *
 */
TEST(CodecYamlReader, BlockPadding) {
	// Объект дерева документа
	yaml::document_t doc;
	/**
	 * Выполняем проверку отказа пустой строки, первой непустой строки глубже
	 *
	 * @note Отступ содержимого, заголовком не заданный, берётся по первой непустой
	 *       строке; строка пустая, её глубже стоящая, оказалась бы содержимым с
	 *       отступом, ничем не подтверждённым. Отвечает rapidyaml отказом «first
	 *       non-empty block line should have at least the original indentation»
	 */
	ASSERT_FALSE(doc.parse("block scalar: >\n \n  \n   \n invalid\n"));
	// Выполняем проверку отказа того же написания с примечанием содержимым
	ASSERT_FALSE(doc.parse("empty block scalar: >\n \n  \n   \n # comment\n"));
	// Выполняем проверку отказа того же написания у дословного блочного значения
	ASSERT_FALSE(doc.parse("---\nblock scalar: |\n     \n  more spaces\n  are invalid\n"));
	/**
	 * Выполняем проверку того, что пустая строка мельче содержимого дозволена
	 */
	ASSERT_TRUE(doc.parse("block: |\n\n  a\n\n  b\n"));
	// Выполняем проверку собранного содержимого с пустыми строками
	ASSERT_EQ(doc.root().at("/block").text(), "\na\n\nb\n");
	/**
	 * Выполняем проверку того, что правило не касается строк за содержимым
	 *
	 * @note Пустая строка, стоящая глубже содержимого, но за первою непустою строкой
	 *       его, отступа не задаёт и содержимому принадлежит пустотою своей
	 */
	ASSERT_TRUE(doc.parse("block: |\n  a\n      \n  b\n"));
	// Выполняем проверку того, что заданный заголовком отступ правилу не подлежит
	ASSERT_TRUE(doc.parse("block: |2\n   \n  a\n"));
}
/**
 * @brief Проверка строгости меток типа да строки черты начала документа
 *
 * @details Случаи взяты из набора yaml-test-suite (U99R, CXX2, 9KBC) и сверены с rapidyaml
 *
 */
TEST(CodecYamlReader, HeaderStrictness) {
	// Объект дерева документа
	yaml::document_t doc;
	/**
	 * Выполняем проверку отказа запятой в записи сокращения метки типа
	 *
	 * @note Знаки ограды поточных построений записи сокращения не принадлежат: вне
	 *       построения запятая свойству чужая
	 */
	ASSERT_FALSE(doc.parse("- !!str, xxx\n"));
	/**
	 * Выполняем проверку того, что внутри построения запятая метку типа завершает
	 */
	ASSERT_TRUE(doc.parse("[!!str, xxx]\n"));
	// Выполняем проверку того, что запись, одною меткою занятая, значением выдана
	ASSERT_EQ(doc.root().size(), 2);
	// Выполняем проверку пустоты значения, одною меткою занятого
	ASSERT_TRUE(doc.root()[0].text().empty());
	// Выполняем проверку второго значения построения
	ASSERT_EQ(doc.root()[1].text(), "xxx");
	// Выполняем проверку того, что метка перед скобкой закрывающей значение даёт
	ASSERT_TRUE(doc.parse("[a, !!str]\n"));
	// Выполняем проверку количества записей построения
	ASSERT_EQ(doc.root().size(), 2);
	/**
	 * Выполняем проверку отказа блочного отображения на строке черты начала документа
	 *
	 * @note Отступ построения отсчитывался бы от черты, а не от начала строки, и
	 *       продолжение строкою ниже читать нечем
	 */
	ASSERT_FALSE(doc.parse("--- key1: value1\n    key2: value2\n"));
	// Выполняем проверку отказа отображения на строке черты и без продолжения
	ASSERT_FALSE(doc.parse("--- a: b\n"));
	// Выполняем проверку отказа отображения с меткою на строке черты
	ASSERT_FALSE(doc.parse("--- &метка a: b\n"));
	// Выполняем проверку отказа записи перечня на строке черты
	ASSERT_FALSE(doc.parse("--- - a\n"));
	/**
	 * Выполняем проверку того, что простой скаляр на строке черты дозволен
	 */
	ASSERT_TRUE(doc.parse("--- простая\n"));
	// Выполняем проверку собранного скаляра
	ASSERT_EQ(doc.root().text(), "простая");
	// Выполняем проверку того, что поточное построение на строке черты дозволено
	ASSERT_TRUE(doc.parse("--- [a]\n"));
	// Выполняем проверку собранного построения
	ASSERT_EQ(doc.root()[0].text(), "a");
}
/**
 * @brief Проверка строгости свойств узла перед блочным построением
 *
 * @details Случаи взяты из набора yaml-test-suite (SY6V, GT5M)
 *
 */
TEST(CodecYamlReader, PropertyStrictness) {
	// Объект дерева документа
	yaml::document_t doc;
	/**
	 * Выполняем проверку отказа свойств узла в строке черты записи перечня
	 *
	 * @details Свойства блочного построения отделяются от него переводом строки: правило
	 * `s-l+block-collection` описания ставит между ними `s-l-comments`, а тот перевода
	 * строки требует
	 *
	 * @note Отвечает rapidyaml отказом. Написание `- &метка - запись` она принимает, чем
	 *       себе же и противоречит: правило описания места стояния не различает, и мы
	 *       отвергаем оба
	 */
	ASSERT_FALSE(doc.parse("&метка - запись перечня\n"));
	// Выполняем проверку отказа метки перед вложенным перечнем в той же строке
	ASSERT_FALSE(doc.parse("- &метка - запись\n"));
	// Выполняем проверку отказа метки типа перед чертою записи
	ASSERT_FALSE(doc.parse("!!seq - запись\n"));
	/**
	 * Выполняем проверку того, что свойства строкою выше черты дозволены
	 */
	ASSERT_TRUE(doc.parse("&метка\n- запись\n"));
	// Выполняем проверку собранной записи перечня
	ASSERT_EQ(doc.root()[0].text(), "запись");
	/**
	 * Выполняем проверку отказа свойств узла при уже открытом перечне
	 *
	 * @note Строка перечня на отступе его обязана открываться чертою записи, и свойства,
	 *       строку эту занявшие, узлу принадлежать не могут
	 */
	ASSERT_FALSE(doc.parse("- первая\n&метка\n- вторая\n"));
	/**
	 * Выполняем проверку того, что свойства строкою выше значения пары дозволены
	 *
	 * @note Строка, одними свойствами занятая, узла не несёт, и ожидание значения пары
	 *       обязано её пережить
	 */
	ASSERT_TRUE(doc.parse("имя:\n  &метка\n  - запись\n"));
	// Выполняем проверку собранного значения пары
	ASSERT_EQ(doc.root().at("/имя")[0].text(), "запись");
	// Выполняем проверку того, что свойства перед простым значением пары дозволены
	ASSERT_TRUE(doc.parse("имя:\n  &метка значение\n"));
	// Выполняем проверку собранного значения пары
	ASSERT_EQ(doc.root().at("/имя").text(), "значение");
}
/**
 * @brief Проверка строгости знака горизонтальной подачи в отступе
 *
 * @details Случаи взяты из набора yaml-test-suite (Y79Y)
 *
 */
TEST(CodecYamlReader, TabStrictness) {
	// Объект дерева документа
	yaml::document_t doc;
	/**
	 * Выполняем проверку отказа подачи прежде содержимого блочного значения
	 *
	 * @note Отступ строки описанием набирается пробелами, и подача пустою строку эту не
	 *       делает; содержимым же ей быть некем - стоит она не глубже заголовка
	 */
	ASSERT_FALSE(doc.parse("foo: |\n\t\nbar: 1\n"));
	/**
	 * Выполняем проверку того, что подача за отступом содержимым является
	 */
	ASSERT_TRUE(doc.parse("foo: |\n \t\nbar: 1\n"));
	// Выполняем проверку собранного содержимого блочного значения
	ASSERT_EQ(doc.root().at("/foo").text(), "\t\n");
	// Выполняем проверку того, что подача за собранным содержимым дозволена
	ASSERT_TRUE(doc.parse("foo: |\n  a\n\t\nbar: 1\n"));
	/**
	 * Выполняем проверку отказа подачи в отступе строки поточного построения
	 *
	 * @note Продолжение построения обязано стоять глубже построения блочного, а подача в
	 *       счёт отступа не идёт
	 */
	ASSERT_FALSE(doc.parse("- [\n\t\t\t\tfoo,\n foo\n ]\n"));
	// Выполняем проверку того, что строка из одних подач пустою является
	ASSERT_TRUE(doc.parse("- [\n\t\t\t\t\n foo\n ]\n"));
	// Выполняем проверку того, что пробел отступа подачу за собою дозволяет
	ASSERT_TRUE(doc.parse("- [\n \tfoo,\n foo\n ]\n"));
	// Выполняем проверку того, что построение без блочных уровней над собою вольно
	ASSERT_TRUE(doc.parse("[\n\tfoo\n]\n"));
	/**
	 * Выполняем проверку отказа подачи в отступе вложенного построения
	 *
	 * @note Отступ вложенного построения задан правилом `s-indent` описания, а тот
	 *       пробелами набирается
	 */
	ASSERT_FALSE(doc.parse("-\t\t\t-\n"));
	// Выполняем проверку отказа подачи за пробелом перед вложенным перечнем
	ASSERT_FALSE(doc.parse("- \t\t-\n"));
	// Выполняем проверку отказа подачи перед именем пары вложенного отображения
	ASSERT_FALSE(doc.parse("-\tимя: значение\n"));
	/**
	 * Выполняем проверку того, что скаляру подача дозволена
	 *
	 * @note Скаляр отступа не задаёт, и подача перед ним отделяет его от черты записи
	 */
	ASSERT_TRUE(doc.parse("-\t\t\t-1\n"));
	// Выполняем проверку собранного значения записи перечня
	ASSERT_EQ(doc.root()[0].text(), "-1");
	// Выполняем проверку того, что вложенный перечень за пробелом дозволен
	ASSERT_TRUE(doc.parse("- - x\n"));
	// Выполняем проверку собранного вложенного перечня
	ASSERT_EQ(doc.root()[0][0].text(), "x");
}
/**
 * @brief Проверка отступа, знаком горизонтальной подачи отделённого
 *
 * @details Случаи взяты из набора yaml-test-suite (DK95, 6CA3)
 *
 */
TEST(CodecYamlReader, TabSeparation) {
	// Объект дерева документа
	yaml::document_t doc;
	/**
	 * Выполняем проверку того, что подача за отступом скаляр от него отделяет
	 *
	 * @note Описание задаёт отступ правилом `s-indent`, пробелами набираемым, а отделять
	 *       им содержимое дозволяет правилом `s-separate-in-line`
	 */
	ASSERT_TRUE(doc.parse("foo:\n \tbar\n"));
	// Выполняем проверку собранного значения пары
	ASSERT_EQ(doc.root().at("/foo").text(), "bar");
	// Выполняем проверку того, что подача без пробелов отступа перед оградой дозволена
	ASSERT_TRUE(doc.parse("\t[\n\t]\n"));
	// Выполняем проверку собранного построения
	ASSERT_EQ(doc.root().size(), 0);
	/**
	 * Выполняем проверку отказа построения блочного за подачей
	 *
	 * @note Отступа подача не даёт, и построению взять его неоткуда
	 */
	ASSERT_FALSE(doc.parse("\tfoo: bar\n"));
	// Выполняем проверку отказа записи перечня за подачей
	ASSERT_FALSE(doc.parse("foo:\n  \t- x\n"));
	/**
	 * Выполняем проверку отказа метки порядка байтов в простом значении
	 *
	 * @details Описание изымает метку из знаков содержимого правилом `nb-char`: метка
	 * принадлежит тексту целиком, а не значению в нём. Без правила этого метка,
	 * значением проглоченная, при перезаписи выходила началом текста и обращалась
	 * меткой настоящей
	 *
	 * @note Нашёл это ворошитель
	 */
	ASSERT_FALSE(doc.parse("a: b\xEF\xBB\xBF" "c\n"));
	/**
	 * Выполняем проверку того, что внутри ограды метка дозволена
	 *
	 * @note Содержимое ограды строится правилом `nb-json`, метки не изымающим
	 */
	ASSERT_TRUE(doc.parse("a: \"b\xEF\xBB\xBF" "c\"\n"));
	// Выполняем проверку собранного значения с меткою внутри
	ASSERT_EQ(doc.root().at("/a").text(), "b\xEF\xBB\xBF" "c");
	// Выполняем проверку того, что метка началом текста дозволена
	ASSERT_TRUE(doc.parse("\xEF\xBB\xBF" "a: b\n"));
	// Выполняем проверку собранного значения за меткою начала текста
	ASSERT_EQ(doc.root().at("/a").text(), "b");
}
/**
 * @brief Проверка принадлежности свойств узла имени пары
 *
 * @details Случаи взяты из набора yaml-test-suite (74H7, 2SXE, 26DV)
 *
 */
TEST(CodecYamlReader, KeyProperties) {
	// Объект дерева документа
	yaml::document_t doc;
	/**
	 * Выполняем проверку метки типа, имени пары предпосланной
	 *
	 * @details Описание берёт имя пары правилом `ns-s-implicit-yaml-key`, а то есть узел
	 * со своими свойствами: метка принадлежит имени, а не отображению, им открываемому.
	 * Прежде метка типа сличалась с видом отображения и написание это отказ получало
	 */
	ASSERT_TRUE(doc.parse("!!str имя: значение\n"));
	// Выполняем проверку собранного значения пары
	ASSERT_EQ(doc.root().at("/имя").text(), "значение");
	// Выполняем проверку метки типа у значения пары
	ASSERT_TRUE(doc.parse("имя: !!int 42\n"));
	// Собранное числовое значение пары
	int64_t number = 0;
	// Выполняем получение числового значения пары
	ASSERT_TRUE(doc.root().at("/имя").value(number));
	// Выполняем проверку собранного значения пары
	ASSERT_EQ(number, 42);
	/**
	 * Выполняем проверку того, что отображение свойств имени не берёт
	 *
	 * @note Прежде написание `&метка имя: значение` метку отдавало отображению
	 */
	ASSERT_TRUE(doc.parse("&метка имя: значение\n"));
	/**
	 * @note Ссылка на метку имени пары дерева документа ещё не проходит: имя узлом
	 *       дерева не является, и объявить метку не на чем. Потоковое чтение выдаёт
	 *       метку при имени верно, а раскрытие ссылки на неё - работа отдельная
	 */
	/**
	 * Выполняем проверку того, что свойства строкою выше отображению принадлежат
	 *
	 * @note Отображение берёт свойства лишь тогда, когда они отделены от него переводом
	 *       строки: правило `s-l+block-collection` описания того и требует
	 */
	ASSERT_TRUE(doc.parse("&метка\nимя: значение\n"));
	// Выполняем проверку того, что ссылка указывает на отображение целиком
	ASSERT_TRUE(doc.parse("верх:\n  &метка\n  имя: значение\nниз: *метка\n"));
	// Выполняем проверку раскрытия ссылки на отображение
	ASSERT_EQ(doc.root().at("/низ").at("/имя").text(), "значение");
}
/**
 * @brief Проверка значения, стоящего содержимым документа целиком
 *
 * @details Случаи взяты из набора yaml-test-suite (9YRD, EX5H, HS5T, FP8R, DK3J)
 *
 */
TEST(CodecYamlReader, RootScalar) {
	// Объект дерева документа
	yaml::document_t doc;
	/**
	 * Выполняем проверку продолжения простого значения с начала строки
	 *
	 * @details Построений над значением корневым нет вовсе, и описание берёт правилом
	 * `s-l+block-node` отступ мельче нулевого: продолжение его вправе стоять с начала
	 * строки. Прежде написание это отказ получало
	 */
	ASSERT_TRUE(doc.parse("a\nb\n"));
	// Выполняем проверку собранного значения, свёрткою сложенного
	ASSERT_EQ(doc.root().text(), "a b");
	// Выполняем проверку свёртки с пустыми строками да отступами
	ASSERT_TRUE(doc.parse("a\nb  \n  c\nd\n\ne\n"));
	// Выполняем проверку собранного значения
	ASSERT_EQ(doc.root().text(), "a b c d\ne");
	// Выполняем проверку продолжения значения за чертою начала документа
	ASSERT_TRUE(doc.parse("---\na\nb\n"));
	// Выполняем проверку собранного значения
	ASSERT_EQ(doc.root().text(), "a b");
	/**
	 * Выполняем проверку того, что черта документа значения не продолжает
	 *
	 * @note Описание запрещает черты внутри содержимого правилом `c-forbidden`: без
	 *       правила этого значение проглатывало черту вместе с документом за нею
	 */
	ASSERT_TRUE(doc.parse("12\n...\n---\nимя: значение\n"));
	// Выполняем проверку количества собранных документов
	ASSERT_EQ(doc.documents(), 2);
	// Выполняем проверку собранного значения первого документа
	ASSERT_EQ(doc.root(0).text(), "12");
	// Выполняем проверку собранного значения второго документа
	ASSERT_EQ(doc.root(1).at("/имя").text(), "значение");
	/**
	 * Выполняем проверку содержимого блочного значения с начала строки
	 *
	 * @note Отступа блочному значению корневому взять неоткуда, и содержимое его вправе
	 *       стоять с начала строки
	 */
	ASSERT_TRUE(doc.parse("--- >\nline1\nline2\n"));
	// Выполняем проверку собранного содержимого, свёрткою сложенного
	ASSERT_EQ(doc.root().text(), "line1 line2\n");
	// Выполняем проверку блочного значения без черты начала документа
	ASSERT_TRUE(doc.parse(">\nline1\nline2\n"));
	// Выполняем проверку собранного содержимого
	ASSERT_EQ(doc.root().text(), "line1 line2\n");
	/**
	 * Выполняем проверку того, что черта документа блочное значение завершает
	 */
	ASSERT_TRUE(doc.parse("--- >\nline1\n--- >\nline2\n"));
	// Выполняем проверку количества собранных документов
	ASSERT_EQ(doc.documents(), 2);
	// Выполняем проверку собранного содержимого второго документа
	ASSERT_EQ(doc.root(1).text(), "line2\n");
}

/**
 * @brief Проверка огранённого значения, в несколько строк стоящего
 *
 * @details Описание дозволяет значению огранённому стоять в несколько строк, и переводы
 *          внутри него свёртке подлежат правилом `s-flow-folded`: пробельная обвязка по
 *          обе стороны перевода снимается, один перевод обращается пробелом, а каждый
 *          следующий остаётся переводом. Ограда двойная знает вдобавок отмену самого
 *          перевода обратною косой чертой
 *
 */
TEST(CodecYamlReader, StretchedScalar) {
	/**
	 * @brief Строение проверяемого случая
	 *
	 */
	struct Sample {
		// Разбираемый текст
		string text;
		// Ожидаемое содержимое значения
		string value;
	};
	// Перечень проверяемых случаев
	const vector <Sample> samples = {
		// Перевод строки один обращается пробелом
		{"имя: \"первая\n  вторая\"\n", "первая вторая"},
		// Пробельная обвязка по обе стороны перевода снимается
		{"имя: \"первая   \n     вторая\"\n", "первая вторая"},
		// Два перевода подряд дают один перевод
		{"имя: \"первая\n\n  вторая\"\n", "первая\nвторая"},
		// Три перевода подряд дают два перевода
		{"имя: \"первая\n\n\n  вторая\"\n", "первая\n\nвторая"},
		// Ограда одинарная свёртке подчиняется наравне с двойной
		{"имя: 'первая\n  вторая'\n", "первая вторая"},
		// Ограда одинарная удвоенную кавычку через строку держит
		{"имя: 'первая''я\n  вторая'\n", "первая'я вторая"},
		/**
		 * Отменяющая последовательность своё даёт, а перевод строки за нею - своё
		 *
		 * @note Запись `\\n` даёт перевод строки, а перевод строки следом за нею свёртке
		 *       подлежит и даёт пробел: два знака эти - разные, и складываются они оба
		 */
		{"имя: \"первая\\n\n  вторая\"\n", "первая\n вторая"},
		// Обратная косая черта перед переводом отменяет свёртку его
		{"имя: \"первая\\\n  вторая\"\n", "перваявторая"},
		// Черта удвоенная отменою перевода не является
		{"имя: \"первая\\\\\n  вторая\"\n", "первая\\ вторая"},
		// Значение в три строки свёртывается целиком
		{"имя: \"одна\n  две\n  три\"\n", "одна две три"},
		// Кавычка внутри простого значения ограды не открывает
		{"имя: не тронь don't\n", "не тронь don't"}
	};
	/**
	 * Выполняем перебор всех проверяемых случаев
	 */
	for(auto & sample : samples){
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse(sample.text)) << "текст [" << sample.text << "]: " << yaml::message(doc.error());
		// Выполняем проверку собранного содержимого значения
		ASSERT_EQ(doc.root()["имя"].text(), sample.value) << "текст [" << sample.text << "]";
	}
	/**
	 * Выполняем проверку значения, содержимым документа целиком стоящего
	 *
	 * @note Построений над ним нет вовсе, и продолжению его дозволено стоять с начала
	 *       строки: правило `s-l+block-node` берёт там отступ мельче нулевого
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("\"свёрнуто \nв пробел\"\n")) << yaml::message(doc.error());
		// Выполняем проверку собранного содержимого значения
		ASSERT_EQ(doc.root().text(), "свёрнуто в пробел");
	}
	/**
	 * Выполняем проверку огранённого значения внутри поточного построения
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("- { \"в несколько\n  строк\": значение}\n")) << yaml::message(doc.error());
		// Выполняем проверку собранного имени пары
		ASSERT_EQ(doc.root()[0]["в несколько строк"].text(), "значение");
	}
}

/**
 * @brief Проверка строгости огранённого значения, в несколько строк стоящего
 *
 * @details Две ограды здесь. Первая: продолжение значения обязано стоять глубже
 *          построения, значение объемлющего, - описание берёт его правилом
 *          `s-flow-line-prefix`. Вторая: имя пары описание берёт правилом
 *          `ns-s-implicit-yaml-key`, а тому дозволена одна строка и только одна, - имя, в
 *          несколько строк стоящее, именем пары быть не вправе. Внутри скобок правило
 *          иное, и там многострочное имя дозволено
 *
 * @note Случаи QB6E, JKF3, 7LBH и D49Q набора yaml-test-suite
 *
 */
TEST(CodecYamlReader, StretchedStrictness) {
	// Перечень текстов, отказа заслуживающих
	const vector <string> samples = {
		// Продолжение стоит отступом объемлющего построения, а не глубже
		"---\nимя: \"а\nб\nв\"\n",
		// Продолжение стоит мельче отступа объемлющего построения
		"  имя: \"а\n б\"\n",
		// Продолжение вложенного перечня стоит с начала строки
		"- - \"а\nа\": х\n",
		// Имя пары стоит в несколько строк под оградою двойной
		"\"а\\nб\": 1\n\"в\n г\": 1\n",
		// Имя пары стоит в несколько строк под оградою одинарной
		"'а': 1\n'в\n г': 1\n",
		/**
		 * Продолжение отделено от начала строки одними подачами
		 *
		 * @note Знак горизонтальной подачи отступом не является вовсе - описание задаёт
		 *       отступ правилом `s-indent`, а тот пробелами набирается. Случай DK95
		 *       набора yaml-test-suite
		 */
		"имя: \"а\n\t\tб\"\n"
	};
	/**
	 * Выполняем перебор всех проверяемых текстов
	 */
	for(auto & sample : samples){
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем проверку отказа разбора текста
		ASSERT_FALSE(doc.parse(sample)) << "текст [" << sample << "]";
	}
	/**
	 * Выполняем проверку того, что имя пары внутри скобок многострочным быть вправе
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("{ \"а\n  б\": 1 }\n")) << yaml::message(doc.error());
		// Выполняем проверку собранного значения пары
		ASSERT_EQ(doc.root()["а б"].text(), "1");
	}
}

/**
 * @brief Проверка имени пары, вопросом объявленного
 *
 * @details Имя, вопросом объявленное, есть узел наравне со значением своим: описание берёт
 *          его правилом `c-l-block-map-explicit-key`, а значение - правилом
 *          `c-l-block-map-explicit-value`, двоеточием на том же отступе объявленным.
 *          Событиями выдаётся оно так же, как имя обычной пары, оттого дерево держит его
 *          наравне с прочими
 *
 */
TEST(CodecYamlReader, ExplicitKey) {
	/**
	 * Выполняем проверку имени со значением, двоеточием объявленным
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("? имя\n: значение\n")) << yaml::message(doc.error());
		// Выполняем проверку количества пар отображения
		ASSERT_EQ(doc.root().size(), 1u);
		// Выполняем проверку собранного значения пары
		ASSERT_EQ(doc.root()["имя"].text(), "значение");
	}
	/**
	 * Выполняем проверку имени, значения своего не дождавшегося
	 *
	 * @note Имя без значения пустоту получает: описание того дозволяет прямо, и запись
	 *       эта есть обычный способ записи множества
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("? первое\n? второе\n")) << yaml::message(doc.error());
		// Выполняем проверку количества пар отображения
		ASSERT_EQ(doc.root().size(), 2u);
		// Выполняем проверку вида значения первой пары
		ASSERT_EQ(doc.root()["первое"].kind(), yaml::kind_t::NUL);
		// Выполняем проверку вида значения второй пары
		ASSERT_EQ(doc.root()["второе"].kind(), yaml::kind_t::NUL);
	}
	/**
	 * Выполняем проверку двоеточия, вопроса не дождавшегося
	 *
	 * @note Двоеточие без вопроса объявляет пару с именем пустым: описание того дозволяет,
	 *       и имя такое выдаётся пустым скалярным значением
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse(": значение\n")) << yaml::message(doc.error());
		// Выполняем проверку количества пар отображения
		ASSERT_EQ(doc.root().size(), 1u);
		// Выполняем проверку собранного значения пары с именем пустым
		ASSERT_EQ(doc.root()[""].text(), "значение");
	}
	/**
	 * Выполняем проверку значения, перечнем стоящего
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("? имя\n: - один\n  - два\n")) << yaml::message(doc.error());
		// Выполняем проверку вида значения пары
		ASSERT_EQ(doc.root()["имя"].kind(), yaml::kind_t::SEQUENCE);
		// Выполняем проверку количества записей перечня
		ASSERT_EQ(doc.root()["имя"].size(), 2u);
		// Выполняем проверку первой записи перечня
		ASSERT_EQ(doc.root()["имя"][0].text(), "один");
	}
	/**
	 * Выполняем проверку смешения имён, вопросом объявленных, с именами обычными
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("? один\n: раз\nдва: 2\n")) << yaml::message(doc.error());
		// Выполняем проверку количества пар отображения
		ASSERT_EQ(doc.root().size(), 2u);
		// Выполняем проверку значения пары, вопросом объявленной
		ASSERT_EQ(doc.root()["один"].text(), "раз");
		// Выполняем проверку значения пары обычной
		ASSERT_EQ(doc.root()["два"].text(), "2");
	}
	/**
	 * Выполняем проверку вопроса, за каким пробельного знака нет
	 *
	 * @note Вопрос составное имя объявляет лишь тогда, когда за ним стоит пробельный знак
	 *       либо конец строки: запись `?имя` есть простое значение целиком
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("?имя: значение\n")) << yaml::message(doc.error());
		// Выполняем проверку собранного имени пары
		ASSERT_EQ(doc.root()["?имя"].text(), "значение");
	}
	/**
	 * Выполняем проверку строгости знака горизонтальной подачи за вопросом
	 *
	 * @note Подача отступом не является - описание задаёт отступ правилом `s-indent`, а
	 *       тот пробелами набирается. Случай Y79Y набора yaml-test-suite
	 */
	{
		// Перечень текстов, отказа заслуживающих
		const vector <string> samples = {"?\t-\n", "? -\n:\t-\n", "?\tимя:\n"};
		/**
		 * Выполняем перебор всех проверяемых текстов
		 */
		for(auto & sample : samples){
			// Объект дерева документа
			yaml::document_t doc;
			// Выполняем проверку отказа разбора текста
			ASSERT_FALSE(doc.parse(sample)) << "текст [" << sample << "]";
		}
	}
}

/**
 * @brief Проверка пустого узла документа, чертою открытого
 *
 * @details Документ, чертою начала открытый и содержимого не получивший, узел свой всё же
 *          имеет: описание берёт его правилом `e-node` - пустым узлом. Документ же, черты
 *          не имеющий, без содержимого не заводится вовсе
 *
 * @note Случаи 6XDY, ZYU8, PUW8 и W4TN набора yaml-test-suite
 *
 */
TEST(CodecYamlReader, EmptyDashedDocument) {
	/**
	 * Выполняем проверку одного документа, чертою открытого
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("---\n")) << yaml::message(doc.error());
		// Выполняем проверку количества документов текста
		ASSERT_EQ(doc.documents(), 1u);
		// Выполняем проверку того, что корень документа действителен
		ASSERT_TRUE(doc.root().valid());
		// Выполняем проверку вида корня документа
		ASSERT_EQ(doc.root().kind(), yaml::kind_t::NUL);
	}
	/**
	 * Выполняем проверку двух документов подряд, оба чертою открыты
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("---\n---\n")) << yaml::message(doc.error());
		// Выполняем проверку количества документов текста
		ASSERT_EQ(doc.documents(), 2u);
		// Выполняем проверку вида корня первого документа
		ASSERT_EQ(doc.root(0).kind(), yaml::kind_t::NUL);
		// Выполняем проверку вида корня второго документа
		ASSERT_EQ(doc.root(1).kind(), yaml::kind_t::NUL);
	}
	/**
	 * Выполняем проверку пустого документа за документом содержательным
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("---\nимя: значение\n---\n")) << yaml::message(doc.error());
		// Выполняем проверку количества документов текста
		ASSERT_EQ(doc.documents(), 2u);
		// Выполняем проверку значения пары первого документа
		ASSERT_EQ(doc.root(0)["имя"].text(), "значение");
		// Выполняем проверку вида корня второго документа
		ASSERT_EQ(doc.root(1).kind(), yaml::kind_t::NUL);
	}
	/**
	 * Выполняем проверку того, что текст без черты документа не заводит
	 *
	 * @note Документ без черты и без содержимого не заводится вовсе: пустого узла ему
	 *       давать неоткуда, и поток остаётся без документов
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста, одними примечаниями занятого
		ASSERT_TRUE(doc.parse("# одно примечание\n# и другое\n")) << yaml::message(doc.error());
		// Выполняем проверку количества документов текста
		ASSERT_EQ(doc.documents(), 0u);
	}
}

/**
 * @brief Проверка пары внутри поточного построения
 *
 * @details Описание берёт запись `[ имя: значение ]` правилом `ns-flow-pair`: запись
 *          перечня там есть отображение, одну пару несущее. Скобок своих отображение это
 *          не имеет вовсе, и закрывается оно запятой либо скобкой перечня. Вопрос
 *          составного имени внутри скобок объявляет имя пары ровно так же, как объявляет
 *          он его построением блочным
 *
 */
TEST(CodecYamlReader, FlowPair) {
	/**
	 * Выполняем проверку пары внутри перечня
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("- [ имя: значение ]\n")) << yaml::message(doc.error());
		// Выполняем проверку вида записи перечня
		ASSERT_EQ(doc.root()[0].kind(), yaml::kind_t::SEQUENCE);
		// Выполняем проверку вида записи вложенного перечня
		ASSERT_EQ(doc.root()[0][0].kind(), yaml::kind_t::MAPPING);
		// Выполняем проверку количества пар отображения
		ASSERT_EQ(doc.root()[0][0].size(), 1u);
		// Выполняем проверку собранного значения пары
		ASSERT_EQ(doc.root()[0][0]["имя"].text(), "значение");
	}
	/**
	 * Выполняем проверку двух пар внутри перечня
	 *
	 * @note Отображение закрывается запятой: каждая запись перечня несёт своё отображение,
	 *       а не одно на всех
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("[ один: 1, два: 2 ]\n")) << yaml::message(doc.error());
		// Выполняем проверку количества записей перечня
		ASSERT_EQ(doc.root().size(), 2u);
		// Выполняем проверку значения первой записи перечня
		ASSERT_EQ(doc.root()[0]["один"].text(), "1");
		// Выполняем проверку значения второй записи перечня
		ASSERT_EQ(doc.root()[1]["два"].text(), "2");
	}
	/**
	 * Выполняем проверку пары с именем пустым внутри перечня
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("[ : значение ]\n")) << yaml::message(doc.error());
		// Выполняем проверку вида записи перечня
		ASSERT_EQ(doc.root()[0].kind(), yaml::kind_t::MAPPING);
		// Выполняем проверку собранного значения пары с именем пустым
		ASSERT_EQ(doc.root()[0][""].text(), "значение");
	}
	/**
	 * Выполняем проверку вопроса составного имени внутри отображения
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("{ ? имя: значение }\n")) << yaml::message(doc.error());
		// Выполняем проверку собранного значения пары
		ASSERT_EQ(doc.root()["имя"].text(), "значение");
	}
	/**
	 * Выполняем проверку вопроса составного имени внутри перечня
	 *
	 * @note Вопрос заводит отображение об одной паре наравне с двоеточием, и значение её
	 *       пусто: описание того дозволяет
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("[ ? имя ]\n")) << yaml::message(doc.error());
		// Выполняем проверку вида записи перечня
		ASSERT_EQ(doc.root()[0].kind(), yaml::kind_t::MAPPING);
		// Выполняем проверку вида значения пары
		ASSERT_EQ(doc.root()[0]["имя"].kind(), yaml::kind_t::NUL);
	}
	/**
	 * Выполняем проверку записи, вложенным построением занятой
	 *
	 * @note Место начала записи запоминается у всякой записи, а не у одной скалярной:
	 *       место, у прошлой записи оставшееся, вставило бы открытие отображения невесть куда
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("- [ [ вложенный ], имя: значение ]\n")) << yaml::message(doc.error());
		// Выполняем проверку вида первой записи перечня
		ASSERT_EQ(doc.root()[0][0].kind(), yaml::kind_t::SEQUENCE);
		// Выполняем проверку содержимого вложенного перечня
		ASSERT_EQ(doc.root()[0][0][0].text(), "вложенный");
		// Выполняем проверку вида второй записи перечня
		ASSERT_EQ(doc.root()[0][1].kind(), yaml::kind_t::MAPPING);
		// Выполняем проверку значения пары второй записи
		ASSERT_EQ(doc.root()[0][1]["имя"].text(), "значение");
	}
}

/**
 * @brief Проверка простого значения, в несколько строк внутри скобок стоящего
 *
 * @details Записи поточного построения строкою не отделены, и простое значение внутри
 *          скобок вправе стоять в несколько строк: обрывает его лишь запятая, скобка либо
 *          двоеточие. Переводы свёртываются правилом `s-flow-folded` наравне со значениями
 *          огранёнными
 *
 */
TEST(CodecYamlReader, FlowStretchedPlain) {
	/**
	 * @brief Строение проверяемого случая
	 *
	 */
	struct Sample {
		// Разбираемый текст
		string text;
		// Путь к проверяемому значению
		string path;
		// Ожидаемое содержимое значения
		string value;
	};
	// Перечень проверяемых случаев
	const vector <Sample> samples = {
		// Имя пары стоит в несколько строк, за ним запятая
		{"{ первая\n  вторая, имя: значение}\n", "/первая вторая", ""},
		// Имя пары стоит в несколько строк, за ним двоеточие
		{"{ первая\n  вторая: значение}\n", "/первая вторая", "значение"},
		// Значение пары стоит в несколько строк
		{"{ имя: первая\n  вторая}\n", "/имя", "первая вторая"},
		// Значение перечня стоит в несколько строк
		{"[ первая\n  вторая ]\n", "/0", "первая вторая"},
		// Пустая строка внутри значения даёт перевод строки
		{"[ первая\n\n  вторая ]\n", "/0", "первая\nвторая"},
		// Значение стоит в три строки
		{"[ одна\n  две\n  три ]\n", "/0", "одна две три"}
	};
	/**
	 * Выполняем перебор всех проверяемых случаев
	 */
	for(auto & sample : samples){
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse(sample.text)) << "текст [" << sample.text << "]: " << yaml::message(doc.error());
		// Выполняем проверку собранного содержимого значения
		ASSERT_EQ(doc.root().at(sample.path).text(), sample.value) << "текст [" << sample.text << "]";
	}
	/**
	 * Выполняем проверку того, что значение не теряется скобкой строкою ниже
	 *
	 * @note Разделитель разбирает сам перебор знаков построения, и до разбора значения он
	 *       не доходит вовсе: без выдачи собранного запись теряла значение `2`
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем разбор текста в дерево документа
		ASSERT_TRUE(doc.parse("a: [\n  1,\n  2\n  ]\n")) << yaml::message(doc.error());
		// Выполняем проверку количества записей перечня
		ASSERT_EQ(doc.root()["a"].size(), 2u);
		// Выполняем проверку второй записи перечня
		ASSERT_EQ(doc.root()["a"][1].text(), "2");
	}
	/**
	 * Выполняем проверку строгости имени пары внутри перечня
	 *
	 * @details Имя это принадлежит отображению об одной паре, а описание берёт его правилом
	 *          `ns-s-implicit-yaml-key` - тому дозволена одна строка. Внутри отображения
	 *          правило иное: имя от двоеточия отделяется правилом `s-separate`, а тому
	 *          перевод строки дозволен
	 *
	 * @note Случай DK4H набора yaml-test-suite, а обратные ему - 4MUZ и VJP3
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем проверку отказа имени, в несколько строк стоящего, внутри перечня
		ASSERT_FALSE(doc.parse("---\n[ имя\n  : значение ]\n"));
		/**
		 * Выполняем проверку дозволенности того же имени внутри отображения
		 */
		{
			// Объект дерева документа
			yaml::document_t other;
			// Выполняем разбор текста в дерево документа
			ASSERT_TRUE(other.parse("{имя\n: значение}\n")) << yaml::message(other.error());
			// Выполняем проверку собранного значения пары
			ASSERT_EQ(other.root()["имя"].text(), "значение");
		}
	}
}

/**
 * @brief Проверка метки, вместилищу строкою выше предпосланной
 *
 * @details Свойства, узлу строкою выше предпосланные, принадлежат вместилищу, какое
 * строка нижняя открывает, а свойства той же строки - имени пары её. Написание
 * `top: &m` со строкою `&k имя: значение` ниже несёт метку вместилища и метку имени
 * разом, и держать их одним полем нельзя
 *
 * @note Строка, вместилища не открывшая, несёт узел один, и двух меток он не несёт:
 *       случай 4JVG набора yaml-test-suite отказа требует, а соседний 7BMT тем же
 *       написанием законен - разнятся они лишь тем, открывает ли нижняя строка
 *       отображение
 *
 */
TEST(CodecYamlReader, DelayedAnchor) {
	// Объект дерева документа
	yaml::document_t doc;
	// Получаем настройки разбора документа
	yaml::document_t::settings_t settings = doc.settings();
	// Устанавливаем удержание всех пар отображения
	settings.duplicates = yaml::duplicate_t::KEEP;
	// Выполняем установку настроек разбора документа
	doc.settings(settings);
	// Выполняем проверку разбора метки вместилища вместе с меткой имени пары
	ASSERT_TRUE(doc.parse("top1: &node1\n  &k1 key1: one\n"));
	// Выполняем проверку разысканного значения вложенной пары
	ASSERT_EQ(doc.root()["top1"]["key1"].text(), "one");
	// Выполняем проверку разбора метки типа вместилища вместе с меткой имени пары
	ASSERT_TRUE(doc.parse("---\n&a4 !!map\n&a5 !!str key5: value4\n"));
	// Выполняем проверку разысканного значения пары
	ASSERT_EQ(doc.root()["key5"].text(), "value4");
	/**
	 * Выполняем проверку отказа двух меток одному простому значению
	 *
	 * @note Строка нижняя вместилища не открывает, и обе метки достались бы узлу одному
	 */
	ASSERT_FALSE(doc.parse("top2: &node2\n  &v2 val2\n"));
	// Выполняем проверку причины отказа разбора
	ASSERT_EQ(doc.error(), yaml::error_t::INVALID_CHARACTER);
	// Выполняем проверку отказа двух меток одному узлу в одной строке
	ASSERT_FALSE(doc.parse("---\n&one &two значение\n"));
	// Выполняем проверку причины отказа разбора
	ASSERT_EQ(doc.error(), yaml::error_t::INVALID_CHARACTER);
	// Выполняем проверку разбора метки вместилища строкою выше без метки имени
	ASSERT_TRUE(doc.parse("top3:\n  &node3\n  key3: three\n"));
	// Выполняем проверку разысканного значения вложенной пары
	ASSERT_EQ(doc.root()["top3"]["key3"].text(), "three");
}

/**
 * @brief Проверка отступа блочного значения, заголовком строкою ниже открытого
 *
 * @details Указатель отступа описанием отсчитывается от узла объемлющего, а не от строки,
 * заголовок несущей: написание `a:` со строкою `  >1` ниже кладёт содержимое на отступ
 * один. Прежде отсчёт шёл от самой строки, и содержимое ждали на отступе три
 *
 * @note Примечание, блочному значению не принадлежащее, вправе стоять на всяком отступе:
 *       примечание содержимым не является и уровня собою не задаёт. Случаи DWX9, T26H и
 *       M5C3 набора yaml-test-suite
 *
 */
TEST(CodecYamlReader, BlockHeaderIndent) {
	// Объект дерева документа
	yaml::document_t doc;
	// Выполняем проверку разбора заголовка, строкою ниже пары стоящего
	ASSERT_TRUE(doc.parse("a:\n  >1\n значение\n"));
	// Выполняем проверку собранного содержимого блочного значения
	ASSERT_EQ(doc.root()["a"].text(), "значение\n");
	// Выполняем проверку разбора заголовка вместе с меткой типа строкою выше
	ASSERT_TRUE(doc.parse("a:\n  !foo\n  >1\n значение\n"));
	// Выполняем проверку собранного содержимого блочного значения
	ASSERT_EQ(doc.root()["a"].text(), "значение\n");
	/**
	 * Выполняем проверку отсчёта от объемлющего построения, а не от строки
	 *
	 * @note Написание это отсчёт от строки принимал бы за содержимое и следующую пару
	 *       того же отображения
	 */
	ASSERT_TRUE(doc.parse("- имя: |\n    текст\n- второе: 1\n"));
	// Выполняем проверку количества записей перечня
	ASSERT_EQ(doc.root().size(), 2u);
	// Выполняем проверку собранного содержимого блочного значения
	ASSERT_EQ(doc.root()[0]["имя"].text(), "текст\n");
	/**
	 * Выполняем проверку примечания, отступом между заголовком и содержимым стоящего
	 */
	ASSERT_TRUE(doc.parse("a: |\n  текст\n # примечание\n"));
	// Выполняем проверку собранного содержимого блочного значения
	ASSERT_EQ(doc.root()["a"].text(), "текст\n");
	/**
	 * Выполняем проверку отказа строки, содержимым не являющейся
	 *
	 * @note Строка эта стоит глубже заголовка и мельче содержимого разом, и примечанием
	 *       не является: разобрать её нечем
	 */
	ASSERT_FALSE(doc.parse("a: |\n  текст\n значение\n"));
	// Выполняем проверку причины отказа разбора
	ASSERT_EQ(doc.error(), yaml::error_t::INVALID_INDENTATION);
}

/**
 * @brief Проверка имени пары, записью составною являющегося
 *
 * @details Именем пары описание дозволяет быть и построению поточному, и ссылке на
 * метку: правило `ns-s-implicit-yaml-key` требует от такого имени лишь одной строки.
 * Потоковое чтение читает его событиями своими, а дерево держать его не может вовсе -
 * имя пары там записью хранится, а не поддеревом, - и отвечает отказом
 *
 * @note Разделение это не выбор наш, а расхождение слоёв: событие имени выразимо, а
 *       запись его в дерево - нет. Случаи LX3P, Q9WF и E76Z набора yaml-test-suite
 *
 */
TEST(CodecYamlReader, CompositeKey) {
	/**
	 * Выполняем проверку чтения имени, построением поточным являющегося
	 */
	{
		// Объект потокового чтения
		yaml::reader_t reader;
		// Выполняем проверку разбора текста с именем, перечнем являющимся
		ASSERT_TRUE(reader.feed("[поток]: значение\n"));
		// Перечень выданных событий разбора
		vector <yaml::event_t> events;
		/**
		 * Выполняем перебор всех выданных событий разбора
		 */
		while(reader.next())
			// Добавляем очередное событие к перечню
			events.push_back(reader.event());
		// Выполняем проверку количества выданных событий
		ASSERT_EQ(events.size(), 10u);
		// Выполняем проверку того, что отображение открыто прежде имени своего
		ASSERT_EQ(events.at(2), yaml::event_t::MAPPING_START);
		// Выполняем проверку того, что имя пары перечнем и является
		ASSERT_EQ(events.at(3), yaml::event_t::SEQUENCE_START);
	}
	/**
	 * Выполняем проверку чтения имени, ссылкою на метку являющегося
	 */
	{
		// Объект потокового чтения
		yaml::reader_t reader;
		// Выполняем проверку разбора текста с именем, ссылкою являющимся
		ASSERT_TRUE(reader.feed("&м имя: один\n*м : два\n"));
	}
	/**
	 * Выполняем проверку отказа дерева на имя составное
	 *
	 * @note Отказ поставлен вместо молчаливой порчи: прежде имя такое становилось
	 *       пустым, а построение его - значением пары, и настоящее значение пропадало
	 */
	{
		// Объект дерева документа
		yaml::document_t doc;
		// Выполняем проверку отказа дерева на имя, перечнем являющееся
		ASSERT_FALSE(doc.parse("[поток]: значение\n"));
		// Выполняем проверку причины отказа разбора
		ASSERT_EQ(doc.error(), yaml::error_t::COMPLEX_KEY);
		// Выполняем проверку отказа дерева на имя, ссылкою являющееся
		ASSERT_FALSE(doc.parse("&м имя: один\n*м : два\n"));
		// Выполняем проверку причины отказа разбора
		ASSERT_EQ(doc.error(), yaml::error_t::COMPLEX_KEY);
		// Выполняем проверку отказа дерева на имя, вопросом объявленное и перечнем являющееся
		ASSERT_FALSE(doc.parse("? - один\n: значение\n"));
		// Выполняем проверку причины отказа разбора
		ASSERT_EQ(doc.error(), yaml::error_t::COMPLEX_KEY);
		// Выполняем проверку того, что имя простое отказа не получает
		ASSERT_TRUE(doc.parse("имя: значение\n"));
	}
}

/**
 * @brief Проверка знаков, значение открывающих лишь по соседу своему
 *
 * @details Два знака описанием читаются по тому, что стоит рядом с ними, а не сами по
 * себе: знак примечания открывает примечание лишь за пробельным знаком либо с начала
 * строки, а вопрос объявляет составное имя лишь тогда, когда за ним стоит пробельный
 * знак либо конец записи
 *
 * @note Прежде оба отвергались всюду, где стояли в начале содержимого, и вместе с
 *       записями негодными отвергались законные. Случаи W42U и 652Z набора
 *       yaml-test-suite
 *
 */
TEST(CodecYamlReader, NeighbourIndicators) {
	// Объект дерева документа
	yaml::document_t doc;
	/**
	 * Выполняем проверку записи перечня, одним примечанием заполненной
	 *
	 * @note Значение записи такой пусто, а примечание стоит за нею
	 */
	ASSERT_TRUE(doc.parse("- # пусто\n- один\n"));
	// Выполняем проверку количества записей перечня
	ASSERT_EQ(doc.root().size(), 2u);
	// Выполняем проверку пустоты первой записи перечня
	ASSERT_TRUE(doc.root()[0].is(yaml::type_t::NUL));
	// Выполняем проверку второй записи перечня
	ASSERT_EQ(doc.root()[1].text(), "один");
	/**
	 * Выполняем проверку простого значения, вопросом открытого
	 *
	 * @note Правило `ns-plain-first` вопрос простому значению дозволяет, коль скоро за
	 *       ним стоит знак непробельный
	 */
	ASSERT_TRUE(doc.parse("{ ?имя: значение }\n"));
	// Выполняем проверку имени пары, вопросом открытого
	ASSERT_EQ(doc.root()["?имя"].text(), "значение");
	/**
	 * Выполняем проверку отказа знака примечания без пробела перед ним
	 *
	 * @note Запись `[ a,#негодное ]` примечания не открывает - пробела перед знаком
	 *       нет, - а простым значением он тоже не является
	 */
	ASSERT_FALSE(doc.parse("[ a,#негодное ]\n"));
	// Выполняем проверку причины отказа разбора
	ASSERT_EQ(doc.error(), yaml::error_t::INVALID_CHARACTER);
}

/**
 * @brief Проверка пустых значений внутри поточного построения
 *
 * @details Пара, двоеточие несущая, значение своё вправе не иметь вовсе: правило `e-node`
 * пустоту значением признаёт. Запись `{ имя:, }` тем и разнится с записью `[ a, , b ]`,
 * что там запятая стоит на месте значения объявленной пары, а здесь - на месте целой
 * записи
 *
 * @note Запятая перед скобкою закрывающей запись закрывает, а новой не открывает: прежде
 *       скобка за нею рождала лишнюю пустую пару, ибо признак наполнения объявлял, что
 *       построение значения несло когда-либо, а нужно было знать, начата ли запись сейчас.
 *       Случаи 4ABK и FRK4 набора yaml-test-suite
 *
 */
TEST(CodecYamlReader, FlowEmptyValues) {
	// Количество событий, чтением выданных
	const auto counting = [](const string & text) noexcept -> size_t {
		// Объект потокового чтения
		yaml::reader_t reader;
		/**
		 * Если разобрать текст не удалось
		 */
		if(!reader.feed(text))
			// Выводим нулевое количество событий
			return 0;
		// Количество выданных событий разбора
		size_t result = 0;
		/**
		 * Выполняем перебор всех выданных событий разбора
		 */
		while(reader.next())
			// Выполняем учёт очередного события разбора
			result++;
		// Выводим количество выданных событий разбора
		return result;
	};
	/**
	 * Выполняем проверку пустого значения пары перед разделителем записей
	 *
	 * @note Событий здесь восемь: поток, документ, отображение, две пары и закрытия их
	 */
	ASSERT_EQ(counting("{ имя:, второе: 1 }\n"), 10u);
	// Выполняем проверку того, что запятая перед скобкой пары не рождает
	ASSERT_EQ(counting("{ имя: значение, }\n"), 8u);
	// Выполняем проверку того, что имя без значения пустое значение получает
	ASSERT_EQ(counting("{ имя }\n"), 8u);
	/**
	 * Выполняем проверку отказа запятой на месте целой записи
	 *
	 * @note Разделитель отделяет записи друг от друга, и двум разделителям подряд
	 *       отделять нечего
	 */
	{
		// Объект потокового чтения
		yaml::reader_t reader;
		// Выполняем проверку отказа разбора двух разделителей подряд
		ASSERT_FALSE(reader.feed("[ a, , b ]\n"));
		// Выполняем проверку причины отказа разбора
		ASSERT_EQ(reader.error(), yaml::error_t::EXPECTED_VALUE);
	}
}
