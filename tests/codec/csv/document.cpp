/**
 * @file document.cpp
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
 * @brief Проверки контейнера CSV — разбор текста таблицы, доступ к полям по номеру и по
 *        имени столбца, чтение и запись файла, потоковая выдача записей обработчику и
 *        круговой проход таблицы через её текст
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
#include <cstdio>
#include <fstream>
#include <csignal>
#include <sys/resource.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
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
using namespace awh;
using namespace awh::codec;

/**
 * @brief Метод получения настроек контейнера с объявленным заголовком
 *
 * @return настройки контейнера с объявленным заголовком
 *
 */
static csv::document_t::settings_t heading() noexcept {
	// Настройки контейнера таблицы
	csv::document_t::settings_t result;
	// Объявляем наличие заголовка в разбираемой таблице
	result.reader.header = csv::header_t::PRESENT;
	// Выводим полученный результат
	return result;
}

/**
 * @brief Метод записи временного файла таблицы
 *
 * @param name имя записываемого временного файла
 * @param text записываемый текст таблицы
 * @return     адрес записанного временного файла
 *
 */
static string temporary(const string & name, const string & text) noexcept {
	// Адрес записываемого временного файла
	const string result = "./" + name;
	// Объект записи временного файла
	ofstream file(result, ios::binary | ios::trunc);
	// Выполняем запись текста таблицы во временный файл
	file.write(text.data(), static_cast <streamsize> (text.size()));
	// Выполняем закрытие записанного временного файла
	file.close();
	// Выводим полученный результат
	return result;
}

/**
 * @brief Проверка разбора текста таблицы без заголовка
 *
 */
TEST(CodecCsvDocument, Parse) {
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	// Выполняем разбор текста таблицы
	ASSERT_TRUE(document.parse("a,b\nc,d\n"));
	// Выполняем проверку отсутствия ошибки разбора
	ASSERT_EQ(document.error(), csv::error_t::NONE);
	// Выполняем проверку количества записей таблицы
	ASSERT_EQ(document.rows(), 2u);
	// Выполняем проверку количества столбцов таблицы
	ASSERT_EQ(document.cols(), 2u);
	// Выполняем проверку содержимого первого поля первой записи
	ASSERT_EQ(document.get(0, size_t(0)), "a");
	// Выполняем проверку содержимого второго поля второй записи
	ASSERT_EQ(document.get(1, size_t(1)), "d");
	// Выполняем проверку того, что заголовок таблицы объявлен не был
	ASSERT_TRUE(document.header().empty());
}

/**
 * @brief Проверка разбора текста таблицы с заголовком
 *
 */
TEST(CodecCsvDocument, Header) {
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	// Выполняем разбор текста таблицы с объявленным заголовком
	ASSERT_TRUE(document.parse("name,value\na,1\nb,2\n", heading()));
	// Выполняем проверку того, что заголовок записью не считается
	ASSERT_EQ(document.rows(), 2u);
	// Выполняем проверку имён столбцов таблицы
	ASSERT_EQ(document.header(), (vector <string_view> {"name", "value"}));
	// Выполняем проверку наличия столбца с заданным именем
	ASSERT_TRUE(document.has("value"));
	// Выполняем проверку отсутствия столбца с незаданным именем
	ASSERT_FALSE(document.has("missing"));
	// Выполняем проверку номера столбца по его имени
	ASSERT_EQ(document.column("value"), 1u);
	// Выполняем проверку признака отсутствия столбца по его имени
	ASSERT_EQ(document.column("missing"), csv::NO_INDEX);
	// Выполняем проверку содержимого поля по номеру записи и имени столбца
	ASSERT_EQ(document.get(1, "value"), "2");
	// Выполняем проверку содержимого поля по имени отсутствующего столбца
	ASSERT_TRUE(document.get(0, "missing").empty());
}

/**
 * @brief Проверка разбора многострочных полей и отмены кавычек
 *
 */
TEST(CodecCsvDocument, Quoted) {
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	// Выполняем разбор текста таблицы с многострочным полем
	ASSERT_TRUE(document.parse("\"a\nb\",\"c\"\"d\"\n\"e,f\",g\n"));
	// Выполняем проверку сохранения перевода строки внутри поля
	ASSERT_EQ(document.get(0, size_t(0)), "a\nb");
	// Выполняем проверку снятия удвоения кавычки внутри поля
	ASSERT_EQ(document.get(0, size_t(1)), "c\"d");
	// Выполняем проверку сохранения разделителя внутри поля
	ASSERT_EQ(document.get(1, size_t(0)), "e,f");
}

/**
 * @brief Проверка получения записи и столбца таблицы целиком
 *
 */
TEST(CodecCsvDocument, RowColumn) {
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	// Выполняем разбор текста таблицы с объявленным заголовком
	ASSERT_TRUE(document.parse("name,value\na,1\nb,2\n", heading()));
	// Выполняем проверку получения записи таблицы целиком
	ASSERT_EQ(document.row(0), (vector <string_view> {"a", "1"}));
	// Выполняем проверку получения столбца таблицы целиком по его номеру
	ASSERT_EQ(document.col(size_t(1)), (vector <string_view> {"1", "2"}));
	// Выполняем проверку получения столбца таблицы целиком по его имени
	ASSERT_EQ(document.col("name"), (vector <string_view> {"a", "b"}));
	// Выполняем проверку получения записи по номеру за пределами таблицы
	ASSERT_TRUE(document.row(2).empty());
	// Выполняем проверку получения столбца по имени отсутствующего столбца
	ASSERT_TRUE(document.col("missing").empty());
}

/**
 * @brief Проверка записей с разным числом полей
 *
 */
TEST(CodecCsvDocument, Ragged) {
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	// Выполняем разбор текста таблицы с записями разной длины
	ASSERT_TRUE(document.parse("a,b,c\nd\ne,f\n"));
	// Выполняем проверку того, что столбцов взято по наибольшей записи
	ASSERT_EQ(document.cols(), 3u);
	// Выполняем проверку количества полей первой записи
	ASSERT_EQ(document.size(0), 3u);
	// Выполняем проверку количества полей второй записи
	ASSERT_EQ(document.size(1), 1u);
	// Выполняем проверку получения отсутствующего поля записи
	ASSERT_TRUE(document.get(1, size_t(2)).empty());
	// Выполняем проверку получения столбца с отсутствующими полями
	ASSERT_EQ(document.col(size_t(2)), (vector <string_view> {"c", "", ""}));
}

/**
 * @brief Проверка отказа разбора при расхождении числа полей
 *
 */
TEST(CodecCsvDocument, RaggedError) {
	// Настройки контейнера таблицы
	csv::document_t::settings_t settings;
	// Устанавливаем прекращение разбора при расхождении числа полей
	settings.reader.ragged = csv::ragged_t::ERROR;
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	// Выполняем разбор текста таблицы с записями разной длины
	ASSERT_FALSE(document.parse("a,b\nc\n", settings));
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(document.error(), csv::error_t::FIELD_COUNT_MISMATCH);
	// Выполняем проверку номера записи, на какой разбор прекращён
	ASSERT_EQ(document.errorLocation().line, 2u);
}

/**
 * @brief Проверка приведения содержимого поля к числу
 *
 */
TEST(CodecCsvDocument, Numeric) {
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	// Выполняем разбор текста таблицы
	ASSERT_TRUE(document.parse("-42,3.5,true,text\n"));
	// Полученное знаковое целое значение
	int32_t number = 0;
	// Выполняем приведение содержимого поля к знаковому целому
	ASSERT_TRUE(document.numeric <int32_t> (0, 0, number));
	// Выполняем проверку полученного знакового целого значения
	ASSERT_EQ(number, -42);
	// Полученное значение с плавающей запятой
	double real = 0.;
	// Выполняем приведение содержимого поля к значению с плавающей запятой
	ASSERT_TRUE(document.numeric <double> (0, 1, real));
	// Выполняем проверку полученного значения с плавающей запятой
	ASSERT_EQ(real, 3.5);
	// Полученное логическое значение
	bool boolean = false;
	// Выполняем приведение содержимого поля к логическому значению
	ASSERT_TRUE(document.numeric <bool> (0, 2, boolean));
	// Выполняем проверку полученного логического значения
	ASSERT_TRUE(boolean);
	// Выполняем проверку отказа приведения содержимого, числом не являющегося
	ASSERT_FALSE(document.numeric <int32_t> (0, 3, number));
	// Выполняем проверку отказа приведения содержимого за пределами таблицы
	ASSERT_FALSE(document.numeric <int32_t> (1, 0, number));
	// Полученное беззнаковое целое значение
	uint32_t unsigned_ = 0;
	// Выполняем проверку того, что знаковое значение беззнаковым не берётся
	ASSERT_FALSE(document.numeric <uint32_t> (0, 0, unsigned_));
}

/**
 * @brief Проверка определения знака-разделителя
 *
 */
TEST(CodecCsvDocument, Detect) {
	// Настройки контейнера таблицы
	csv::document_t::settings_t settings;
	// Включаем определение разделителя по содержимому
	settings.reader.separator = '\0';
	// Объект контейнера таблицы
	csv::document_t document(::logger(), settings);
	// Выполняем разбор текста таблицы с точкой с запятой разделителем
	ASSERT_TRUE(document.parse("a;b;c\nd;e;f\n"));
	// Выполняем проверку количества столбцов таблицы
	ASSERT_EQ(document.cols(), 3u);
	// Выполняем проверку содержимого второго поля первой записи
	ASSERT_EQ(document.get(0, size_t(1)), "b");
	// Выполняем разбор текста таблицы с табуляцией разделителем
	ASSERT_TRUE(document.parse("a\tb\nc\td\n"));
	// Выполняем проверку количества столбцов таблицы
	ASSERT_EQ(document.cols(), 2u);
	// Выполняем проверку содержимого второго поля второй записи
	ASSERT_EQ(document.get(1, size_t(1)), "d");
}

/**
 * @brief Проверка составления таблицы вручную
 *
 */
TEST(CodecCsvDocument, Append) {
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	// Выполняем установку заголовка таблицы
	ASSERT_TRUE(document.header(vector <string> {"name", "value"}));
	// Выполняем добавление первой записи в конец таблицы
	document.append(vector <string> {"a", "1"});
	// Выполняем добавление второй записи в конец таблицы
	document.append(vector <string> {"b", "2"});
	// Выполняем проверку количества записей таблицы
	ASSERT_EQ(document.rows(), 2u);
	// Выполняем проверку содержимого поля по имени столбца
	ASSERT_EQ(document.get(1, "name"), "b");
	// Выполняем проверку собранного текста таблицы
	ASSERT_EQ(document.text(), "name,value\r\na,1\r\nb,2\r\n");
	// Выполняем проверку отказа установки заголовка с повторяющимися именами
	ASSERT_FALSE(document.header(vector <string> {"name", "name"}));
}

/**
 * @brief Проверка размножения записи видами того же документа
 *
 * @details Виды, выданные `row()`, указывают в хранилище знаков самого документа, и
 *          подача их обратно в `append()` есть прямой способ размножить запись. Долив
 *          хранилища по одному полю перераспределял его на первом же поле и обращал
 *          виды остальных в висячие, откуда шло чтение освобождённой памяти
 *
 */
TEST(CodecCsvDocument, AppendSelfViews) {
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	/**
	 * Выполняем набивку таблицы, заведомо перераспределяющей хранилище знаков
	 */
	for(size_t i = 0; i < 4; i++){
		// Поля очередной записи таблицы
		vector <string> fields;
		/**
		 * Выполняем сборку полей очередной записи
		 */
		for(size_t j = 0; j < 8; j++)
			// Заносим очередное поле записи
			fields.push_back("поле" + to_string(i) + "-" + to_string(j));
		// Выполняем добавление записи в конец таблицы
		document.append(fields);
	}
	// Выполняем снятие первой записи таблицы видами
	const vector <string_view> copy = document.row(0);
	// Выполняем добавление снятой записи в конец таблицы
	document.append(copy);
	// Выполняем проверку количества записей таблицы
	ASSERT_EQ(document.rows(), 5u);
	// Выполняем снятие добавленной записи таблицы
	const vector <string_view> back = document.row(document.rows() - 1);
	// Выполняем проверку количества полей добавленной записи
	ASSERT_EQ(back.size(), 8u);
	/**
	 * Выполняем перебор всех полей добавленной записи
	 */
	for(size_t i = 0; i < back.size(); i++)
		// Выполняем проверку содержимого очередного поля
		ASSERT_EQ(back.at(i), "поле0-" + to_string(i));
	// Выполняем проверку сохранности исходной записи
	ASSERT_EQ(document.get(0, 0), "поле0-0");
}

/**
 * @brief Проверка очистки таблицы
 *
 */
TEST(CodecCsvDocument, Clear) {
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	// Выполняем разбор текста таблицы с объявленным заголовком
	ASSERT_TRUE(document.parse("name,value\na,1\n", heading()));
	// Выполняем очистку таблицы
	document.clear();
	// Выполняем проверку того, что записи таблицы очищены
	ASSERT_EQ(document.rows(), 0u);
	// Выполняем проверку того, что заголовок таблицы очищен
	ASSERT_TRUE(document.header().empty());
	// Выполняем проверку того, что соответствие имён столбцов очищено
	ASSERT_FALSE(document.has("name"));
	// Выполняем проверку того, что текст очищенной таблицы пуст
	ASSERT_TRUE(document.text().empty());
}

/**
 * @brief Проверка повторного разбора поверх прежней таблицы
 *
 * @details Разбор обязан замещать прежнее содержимое, а не дописывать к нему: иначе
 * повторное чтение того же контейнера удваивало бы таблицу
 *
 */
TEST(CodecCsvDocument, Reparse) {
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	// Выполняем разбор первого текста таблицы
	ASSERT_TRUE(document.parse("a,b\nc,d\n"));
	// Выполняем разбор второго текста таблицы
	ASSERT_TRUE(document.parse("e,f\n"));
	// Выполняем проверку количества записей таблицы
	ASSERT_EQ(document.rows(), 1u);
	// Выполняем проверку содержимого первого поля первой записи
	ASSERT_EQ(document.get(0, size_t(0)), "e");
}

/**
 * @brief Проверка кругового прохода таблицы через её текст
 *
 */
TEST(CodecCsvDocument, RoundTrip) {
	// Объект контейнера исходной таблицы
	csv::document_t source(::logger());
	// Выполняем разбор текста исходной таблицы с объявленным заголовком
	ASSERT_TRUE(source.parse("name,value\n\"a\nb\",\"c,d\"\n\"e\"\"f\",g\n", heading()));
	// Объект контейнера полученной обратно таблицы
	csv::document_t result(::logger());
	// Выполняем разбор текста, собранного исходной таблицей
	ASSERT_TRUE(result.parse(source.text(), heading()));
	// Выполняем проверку имён столбцов полученной обратно таблицы
	ASSERT_EQ(result.header(), source.header());
	// Выполняем проверку количества записей полученной обратно таблицы
	ASSERT_EQ(result.rows(), source.rows());
	/**
	 * Выполняем перебор записей полученной обратно таблицы
	 */
	for(size_t i = 0; i < result.rows(); i++)
		// Выполняем проверку того, что запись получена обратно неизменной
		ASSERT_EQ(result.row(i), source.row(i)) << i;
}

/**
 * @brief Проверка чтения и записи файла таблицы
 *
 */
TEST(CodecCsvDocument, File) {
	// Выполняем запись временного файла таблицы
	const string & filename = temporary("awh_csv_file.csv", "name,value\r\na,1\r\nb,2\r\n");
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	// Выполняем чтение таблицы из файла
	ASSERT_TRUE(document.load(filename));
	// Выполняем проверку количества прочитанных записей таблицы
	ASSERT_EQ(document.rows(), 3u);
	// Выполняем проверку содержимого поля прочитанной таблицы
	ASSERT_EQ(document.get(2, size_t(1)), "2");
	// Адрес файла, в какой записывается таблица
	const string output = "./awh_csv_file_out.csv";
	// Выполняем запись таблицы в файл
	ASSERT_TRUE(document.save(output));
	// Объект контейнера полученной обратно таблицы
	csv::document_t result(::logger());
	// Выполняем чтение записанной таблицы из файла
	ASSERT_TRUE(result.load(output));
	// Выполняем проверку количества записей полученной обратно таблицы
	ASSERT_EQ(result.rows(), document.rows());
	// Выполняем проверку содержимого поля полученной обратно таблицы
	ASSERT_EQ(result.get(1, size_t(0)), "a");
	// Выполняем удаление записанных временных файлов таблицы
	::remove(filename.c_str());
	// Выполняем удаление записанных временных файлов таблицы
	::remove(output.c_str());
}

/**
 * @brief Проверка отказа чтения отсутствующего файла таблицы
 *
 */
TEST(CodecCsvDocument, FileMissing) {
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	// Выполняем проверку отказа чтения отсутствующего файла таблицы
	ASSERT_FALSE(document.load("./awh_csv_missing.csv"));
}

/**
 * @brief Проверка потоковой выдачи записей обработчику
 *
 * @details Таблица при этом не заполняется вовсе: тем и ведётся чтение файлов, в память
 * не помещающихся
 *
 */
TEST(CodecCsvDocument, Callback) {
	// Количество записей, выданных обработчику
	size_t count = 0;
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	// Выполняем разбор текста таблицы записями
	ASSERT_TRUE(document.parse("name,value\na,1\nb,2\n", [&count](const vector <string_view> & fields) noexcept -> bool {
		// Выполняем подсчёт записей, выданных обработчику
		count++;
		// Выполняем проверку количества полей выданной записи
		EXPECT_EQ(fields.size(), 2u);
		// Выводим признак продолжения разбора
		return true;
	}));
	/**
	 * Заголовок обработчику не выдаётся, а потому записей выдано на одну меньше,
	 * нежели строк в тексте: заголовок здесь настройками не объявлен, и первая
	 * строка записью является наравне с прочими
	 */
	ASSERT_EQ(count, 3u);
	// Выполняем проверку того, что таблица при этом не заполнялась
	ASSERT_EQ(document.rows(), 0u);
}

/**
 * @brief Проверка прекращения потоковой выдачи обработчиком
 *
 */
TEST(CodecCsvDocument, CallbackStop) {
	// Количество записей, выданных обработчику
	size_t count = 0;
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	// Выполняем разбор текста таблицы записями
	document.parse("a\nb\nc\nd\n", [&count](const vector <string_view> &) noexcept -> bool {
		// Выполняем подсчёт записей, выданных обработчику
		count++;
		// Выводим признак прекращения разбора после второй записи
		return (count < 2);
	});
	// Выполняем проверку того, что разбор прекращён обработчиком
	ASSERT_EQ(count, 2u);
}

/**
 * @brief Проверка потокового чтения файла таблицы записями
 *
 * @details Заголовок обработчику не выдаётся, а имена столбцов берутся по окончании
 * чтения: тем отличается настоящий способ от чтения, таблицу удерживающего
 *
 */
TEST(CodecCsvDocument, CallbackFile) {
	// Текст записываемой временной таблицы
	string text = "name,value\r\n";
	/**
	 * Выполняем составление текста временной таблицы
	 */
	for(size_t i = 0; i < 1000; i++)
		// Выполняем добавление очередной записи в текст временной таблицы
		text.append("row" + to_string(i) + ",\"многострочное\nзначение " + to_string(i) + "\"\r\n");
	// Выполняем запись временного файла таблицы
	const string & filename = temporary("awh_csv_stream.csv", text);
	// Количество записей, выданных обработчику
	size_t count = 0;
	// Объект контейнера таблицы
	csv::document_t document(::logger(), heading());
	// Выполняем чтение таблицы из файла записями
	ASSERT_TRUE(document.read(filename, [&count](const vector <string_view> & fields) noexcept -> bool {
		// Выполняем проверку количества полей выданной записи
		EXPECT_EQ(fields.size(), 2u);
		// Выполняем проверку содержимого первого поля выданной записи
		EXPECT_EQ(fields.front(), "row" + to_string(count));
		// Выполняем проверку содержимого второго поля выданной записи
		EXPECT_EQ(fields.back(), "многострочное\nзначение " + to_string(count));
		// Выполняем подсчёт записей, выданных обработчику
		count++;
		// Выводим признак продолжения чтения
		return true;
	}));
	// Выполняем проверку количества записей, выданных обработчику
	ASSERT_EQ(count, 1000u);
	// Выполняем проверку того, что таблица при этом не заполнялась
	ASSERT_EQ(document.rows(), 0u);
	// Выполняем проверку имён столбцов, взятых по окончании чтения
	ASSERT_EQ(document.header(), (vector <string_view> {"name", "value"}));
	// Выполняем удаление записанного временного файла таблицы
	::remove(filename.c_str());
}

/**
 * @brief Проверка вывода таблицы последовательностью знаков
 *
 */
TEST(CodecCsvDocument, Operator) {
	// Объект контейнера таблицы
	csv::document_t document(::logger());
	// Выполняем разбор текста таблицы
	ASSERT_TRUE(document.parse("a,b\n"));
	// Выполняем проверку вывода таблицы последовательностью знаков
	ASSERT_EQ(static_cast <string> (document), "a,b\r\n");
}

/**
 * @brief Проверка соответствия имён столбцов при повторном объявлении заголовка
 *
 * @details Заголовок, объявленный вторично, обязан снести соответствие имён прежнего:
 * иначе имя, из заголовка ушедшее, разыскивалось бы по-прежнему, а столбец выдавался
 * бы чужой. Расхождение это невидимо, когда имена второго заголовка те же самые -
 * оттого второй круг ведётся именами ДРУГИМИ, а имена первого проверяются на отсутствие
 *
 * @note Заход тот же, каким у прочих кодеков проверяется указатель имён вместилища
 *
 */
TEST(CodecCsvDocument, HeaderRefill) {
	// Собираемая таблица
	csv::document_t document(::logger());
	// Выполняем объявление заголовка первого круга
	ASSERT_TRUE(document.header({"первое", "второе", "третье"}));
	// Выполняем проверку того, что имена первого круга разыскиваются
	ASSERT_TRUE(document.has("первое"));
	// Выполняем проверку номера столбца по имени первого круга
	ASSERT_EQ(document.column("третье"), 2);
	// Выполняем долив записи к таблице
	document.append(vector <string> {"a", "b", "c"});
	// Выполняем объявление заголовка второго круга именами другими
	ASSERT_TRUE(document.header({"четвёртое", "пятое", "шестое"}));
	// Выполняем проверку того, что имена второго круга разыскиваются
	ASSERT_TRUE(document.has("пятое"));
	// Выполняем проверку номера столбца по имени второго круга
	ASSERT_EQ(document.column("шестое"), 2);
	/**
	 * Выполняем проверку того, что имён первого круга более нет
	 *
	 * @note Ровно это и ловит проверку: соответствие, от прежнего заголовка уцелевшее,
	 *       выдавало бы столбец по имени, какого в заголовке уже нет
	 */
	ASSERT_FALSE(document.has("первое"));
	// Выполняем проверку того, что имя первого круга столбца не выдаёт
	ASSERT_FALSE(document.has("третье"));
	// Выполняем проверку того, что содержимое записи объявлением не тронуто
	ASSERT_EQ(document.get(0, size_t(0)), "a");
	/**
	 * Выполняем проверку отказа объявления заголовка с повтором имени
	 *
	 * @note Повтор оставил бы один из столбцов недостижимым по имени, а разбор
	 *       заголовка повтор отвергает - отвергать его надлежит и правке
	 */
	ASSERT_FALSE(document.header({"имя", "имя"}));
	// Выполняем проверку того, что отвергнутое объявление заголовка не оставило
	ASSERT_FALSE(document.has("имя"));
	// Выполняем проверку того, что имена прежнего заголовка отказом тоже сняты
	ASSERT_FALSE(document.has("пятое"));
}

/**
 * @brief Проверка отличения ненайденного файла от внутреннего изъяна разбора
 *
 * @details Путь к файлу передаётся извне, и отказ открытия внутренним изъяном
 *          разбора не является. Прежде отвечалось кодом внутренней ошибки, и
 *          потребитель отправлялся искать дефект у нас вместо своего пути
 *
 */
TEST(CodecCsvDocument, MissingFileIsNotInternal) {
	// Таблица значений
	csv::document_t document(::logger());
	// Выполняем проверку отказа чтения несуществующего файла
	ASSERT_FALSE(document.load("/несуществующий/каталог/таблица.csv"));
	// Выполняем проверку кода ошибки чтения
	ASSERT_EQ(document.error(), csv::error_t::FILE_NOT_OPENED);
}

/**
 * @brief Проверка отказа приведения поля к виду, его не вмещающему
 *
 * @details Правило кодека - отказ: разбор записи, в шестьдесят четыре разряда не
 *          вместившейся, отвечает отказом, - и сужение к виду поменьше обязано
 *          отвечать им же. Приведение языка вместо этого переносило разряды и отдавало
 *          значение, ничего общего с содержимым поля не имеющее: поле «300» видом в
 *          один байт без знака выходило числом 44
 *
 * @note Кодеки JSON и XML в том же положении приводят число к пределам вида: договор
 *       извлечения у них так и записан. Расхождение это задано устройством поверхностей,
 *       а не недоделкой - там значение уже разобрано и несёт вид, а здесь числом не
 *       является само содержимое поля, коль скоро оно в затребованный вид не легло
 *
 */
TEST(CodecCsvDocument, NarrowingRefusesInsteadOfWrapping) {
	// Таблица значений
	csv::document_t document(::logger());
	// Выполняем разбор текста таблицы
	ASSERT_TRUE(document.parse("300,-1,70000,127\r\n"));
	// Извлекаемое число видом в один байт без знака
	uint8_t byte = 0;
	// Выполняем проверку отказа приведения числа, в один байт не помещающегося
	ASSERT_FALSE(document.numeric(0, 0, byte));
	// Извлекаемое число видом в один байт со знаком
	int8_t small = 0;
	// Выполняем проверку отказа приведения отрицательного числа к виду без знака
	ASSERT_FALSE(document.numeric(0, 1, byte));
	// Выполняем проверку приведения отрицательного числа к виду со знаком
	ASSERT_TRUE(document.numeric(0, 1, small));
	// Выполняем проверку приведённого значения
	ASSERT_EQ(small, static_cast <int8_t> (-1));
	// Извлекаемое число видом в два байта без знака
	uint16_t word = 0;
	// Выполняем проверку отказа приведения числа, в два байта не помещающегося
	ASSERT_FALSE(document.numeric(0, 2, word));
	// Выполняем проверку приведения помещающегося числа
	ASSERT_TRUE(document.numeric(0, 3, small));
	// Выполняем проверку приведённого значения
	ASSERT_EQ(small, static_cast <int8_t> (127));
	// Выполняем проверку отказа приведения числа «300», в один байт со знаком не помещающегося
	ASSERT_FALSE(document.numeric(0, 0, small));
	// Выполняем проверку приведения того же числа к виду в два байта без знака
	ASSERT_TRUE(document.numeric(0, 0, word));
	// Выполняем проверку приведённого значения
	ASSERT_EQ(word, static_cast <uint16_t> (300));
}

/**
 * @brief Проверка доклада об отказе записи файла таблицы
 *
 * @details Признак успеха снимался с потока ДО закрытия его, а буфер поток сбрасывает
 * именно закрытием: текст, целиком уместившийся в буфер, уходил отказом сброса молча,
 * а вызов отчитывался успехом. Замер дал успех при 64 байтах из 190 в файле. У кодеков
 * JSON и XML то же место чинено тем же порядком
 *
 * @note Код отказа уходит в журнал, а не в `error()`: запись объявлена `const`, и селить
 *       в ней код отказа некуда
 *
 */
TEST(CodecCsvDocument, WriteFailureIsNotSuccess) {
	// Собираемые сообщения журнала
	vector <string> messages;
	// Объект журнала с перехватом вывода
	awh::log_t log(&Silent::framework());
	// Выполняем назначение приёмника вывода в функцию обратного вызова
	log.mode({awh::log_t::mode_t::DEFERRED});
	// Выполняем назначение перехвата сообщений журнала
	log.subscribe([&messages](const awh::log_t::flag_t, string_view text) noexcept -> void {
		// Выполняем сбор очередного сообщения журнала
		messages.push_back(string(text));
	});
	// Собираемый текст таблицы
	string text("имя,второе\n");
	/**
	 * Выполняем сборку текста таблицы крупнее допустимого предела
	 */
	for(uint16_t i = 0; i < 6; i++)
		// Добавляем очередную запись таблицы
		text.append("значение" + std::to_string(i) + ",второе\n");
	// Дерево значений таблицы
	csv::document_t document(&log);
	// Выполняем проверку разбора собранного текста таблицы
	ASSERT_TRUE(document.parse(text));
	/**
	 * @brief Сторож предельного размера файла процесса
	 *
	 * @note Предел этот - настройка ПРОЦЕССА, и снимать её обязательно: оставленная,
	 *       она валила бы соседние проверки, пишущие файлы
	 *
	 */
	struct Guard {
		// Прежний предел размера файла процесса
		struct rlimit limit;
		// Прежний обработчик сигнала превышения предела
		void (* handler)(int32_t);
		/**
		 * @brief Конструктор
		 *
		 */
		Guard() noexcept {
			// Выполняем снятие прежнего предела размера файла
			::getrlimit(RLIMIT_FSIZE, &this->limit);
			// Выполняем отключение сигнала превышения предела
			this->handler = ::signal(SIGXFSZ, SIG_IGN);
			// Предел размера файла, заведомо меньший текста таблицы
			struct rlimit bound = this->limit;
			// Выполняем установку предела размера файла
			bound.rlim_cur = 64;
			// Выполняем назначение предела размера файла процессу
			::setrlimit(RLIMIT_FSIZE, &bound);
		}
		/**
		 * @brief Деструктор
		 *
		 */
		~Guard() noexcept {
			// Выполняем возврат прежнего предела размера файла
			::setrlimit(RLIMIT_FSIZE, &this->limit);
			// Выполняем возврат прежнего обработчика сигнала
			::signal(SIGXFSZ, this->handler);
		}
	} guard;
	// Адрес файла, в который ведётся запись таблицы
	const string filename = "./awh-codec-csv-write-failure.csv";
	// Выполняем снос прежнего файла таблицы
	::remove(filename.c_str());
	// Выполняем проверку отказа записи усечённого файла таблицы
	ASSERT_FALSE(document.save(filename));
	// Выполняем проверку оглашения отказа в журнале
	ASSERT_FALSE(messages.empty());
	// Выполняем проверку упоминания причины отказа в сообщении
	ASSERT_NE(messages.back().find(csv::message(csv::error_t::FILE_NOT_WRITTEN)), string::npos) << messages.back();
	// Выполняем снос усечённого файла таблицы
	::remove(filename.c_str());
}

/**
 * @brief Проверка доклада об отказе открытия файла таблицы на запись
 *
 * @details Чтение файла оглашает отказ открытия кодом `FILE_NOT_OPENED`, а запись
 * прежде возвращала голое отрицание без единого слова: несимметрия эта оставляла
 * потребителя без причины отказа там, где та известна
 *
 */
TEST(CodecCsvDocument, WriteToMissingDirectoryIsReported) {
	// Собираемые сообщения журнала
	vector <string> messages;
	// Объект журнала с перехватом вывода
	awh::log_t log(&Silent::framework());
	// Выполняем назначение приёмника вывода в функцию обратного вызова
	log.mode({awh::log_t::mode_t::DEFERRED});
	// Выполняем назначение перехвата сообщений журнала
	log.subscribe([&messages](const awh::log_t::flag_t, string_view text) noexcept -> void {
		// Выполняем сбор очередного сообщения журнала
		messages.push_back(string(text));
	});
	// Дерево значений таблицы
	csv::document_t document(&log);
	// Выполняем проверку разбора текста таблицы
	ASSERT_TRUE(document.parse("имя\nзначение\n"));
	// Выполняем проверку отказа записи в несуществующий каталог
	ASSERT_FALSE(document.save("/несуществующий/каталог/таблица.csv"));
	// Выполняем проверку оглашения отказа в журнале
	ASSERT_FALSE(messages.empty());
	// Выполняем проверку упоминания причины отказа в сообщении
	ASSERT_NE(messages.back().find(csv::message(csv::error_t::FILE_NOT_OPENED)), string::npos) << messages.back();
}

/**
 * @brief Проверка отказа сужения дробного числа, в затребованный вид не помещающегося
 *
 * @details Приведение к `float` числа «1e308» отдавало бесконечность признаком успеха,
 * тогда как то же число видом `double` разбирается точно, а запись «1e400», в `double`
 * не помещающаяся, отвечает отказом. Утрата числа выдавалась за приведение, и правило
 * кодека - отвергать не поместившееся - соблюдалось для целых видов и обходилось для
 * дробных. Обнаружено покрытием: ветвь дробного приведения не была пройдена вовсе
 *
 * @note Записи «inf» и «nan» отказом отвечать не должны: они пределом вида не ограничены
 *
 */
TEST(CodecCsvDocument, RealNarrowingRefusesInsteadOfInfinity) {
	// Таблица значений
	csv::document_t document(::logger());
	// Выполняем разбор текста таблицы
	ASSERT_TRUE(document.parse("1e308,3.5,-1e300,1e400,inf,-inf,nan\r\n"));
	// Извлекаемое число дробным видом одинарной точности
	float single = 0.f;
	// Извлекаемое число дробным видом двойной точности
	double couple = 0.;
	// Выполняем проверку отказа приведения числа, в одинарную точность не помещающегося
	ASSERT_FALSE(document.numeric(0, 0, single));
	// Выполняем проверку приведения того же числа к двойной точности
	ASSERT_TRUE(document.numeric(0, 0, couple));
	// Выполняем проверку приведённого значения
	ASSERT_DOUBLE_EQ(couple, 1e308);
	// Выполняем проверку приведения помещающегося числа
	ASSERT_TRUE(document.numeric(0, 1, single));
	// Выполняем проверку приведённого значения
	ASSERT_FLOAT_EQ(single, 3.5f);
	// Выполняем проверку отказа приведения отрицательного числа, в вид не помещающегося
	ASSERT_FALSE(document.numeric(0, 2, single));
	// Выполняем проверку отказа приведения числа, и в двойную точность не помещающегося
	ASSERT_FALSE(document.numeric(0, 3, couple));
	// Выполняем проверку отказа того же числа и для одинарной точности
	ASSERT_FALSE(document.numeric(0, 3, single));
	/**
	 * Выполняем перебор записей бесконечности и не-числа
	 */
	for(size_t i = 4; i < 7; i++){
		// Выполняем проверку приведения записи к двойной точности
		ASSERT_TRUE(document.numeric(0, i, couple)) << i;
		// Выполняем проверку приведения записи к одинарной точности
		ASSERT_TRUE(document.numeric(0, i, single)) << i;
	}
	// Выполняем проверку переноса не-числа в одинарную точность
	ASSERT_TRUE(::isnan(single));
}

/**
 * @brief Проверка записи таблицы, собранного текста которой не вмещает буфер
 *
 * @details Запись изымает собранный текст кусками, и ветвь эта покрытием пройдена не
 * была: все проверки записи брали таблицы в десятки байт. Заголовок таблицы записью
 * равно не был затронут ни разу
 *
 */
TEST(CodecCsvDocument, LargeTableSurvivesRoundTrip) {
	// Настройки таблицы
	csv::document_t::settings_t settings;
	// Выполняем указание на присутствие заголовка таблицы
	settings.reader.header = csv::header_t::PRESENT;
	// Исходная таблица значений
	csv::document_t document(::logger(), settings);
	// Собираемый текст таблицы
	string text = "alpha,beta,gamma\r\n";
	/**
	 * Выполняем сбор записей таблицы
	 */
	for(size_t i = 0; i < 20000; i++)
		// Заносим очередную запись таблицы
		text.append(std::to_string(i)).append(",\"поле, с разделителем ").append(std::to_string(i)).append("\",v").append(std::to_string(i)).append("\r\n");
	// Выполняем проверку разбора текста таблицы
	ASSERT_TRUE(document.parse(text));
	// Выполняем проверку количества собранных записей
	ASSERT_EQ(document.rows(), static_cast <size_t> (20000));
	// Адрес файла таблицы
	const string filename = "./csv-large-round-trip.csv";
	// Выполняем проверку записи таблицы в файл
	ASSERT_TRUE(document.save(filename));
	// Полученная обратным чтением таблица значений
	csv::document_t back(::logger(), settings);
	// Выполняем проверку чтения записанного файла таблицы
	ASSERT_TRUE(back.load(filename));
	// Выполняем проверку совпадения заголовков таблиц
	ASSERT_EQ(document.header(), back.header());
	// Выполняем проверку совпадения количества записей
	ASSERT_EQ(document.rows(), back.rows());
	/**
	 * Выполняем перебор всех записей таблицы
	 */
	for(size_t i = 0; i < document.rows(); i++){
		/**
		 * Выполняем перебор всех полей записи
		 */
		for(size_t j = 0; j < 3; j++)
			// Выполняем проверку совпадения содержимого поля
			ASSERT_EQ(document.get(i, j), back.get(i, j)) << i << ':' << j;
	}
	// Выполняем удаление файла таблицы
	::remove(filename.c_str());
}

/**
 * @brief Проверка отказа установки заголовка таблицы с пустым именем столбца
 *
 * @details Обращение к столбцу ведётся по имени, и пустым именем столбец недостижим:
 * принять такое имя значило бы оставить столбец без доступа. Отказ обязан снимать
 * заголовок целиком, а не оставлять его собранным наполовину
 *
 */
TEST(CodecCsvDocument, EmptyColumnNameRefused) {
	// Таблица значений
	csv::document_t document(::logger());
	// Выполняем проверку разбора текста таблицы
	ASSERT_TRUE(document.parse("1,2,3\r\n"));
	// Выполняем проверку установки годного заголовка таблицы
	ASSERT_TRUE(document.header({"альфа", "бета", "гамма"}));
	// Выполняем проверку количества столбцов заголовка
	ASSERT_EQ(document.header().size(), static_cast <size_t> (3));
	// Выполняем проверку отказа установки заголовка с пустым именем столбца
	ASSERT_FALSE(document.header({"альфа", "", "гамма"}));
	// Выполняем проверку снятия заголовка целиком
	ASSERT_TRUE(document.header().empty());
	// Выполняем проверку недостижимости столбца по прежнему имени
	ASSERT_TRUE(document.col("альфа").empty());
}

/**
 * @brief Проверка разбора пустого текста таблицы вместилищем
 *
 * @details Подача пустого текста нужна затем, чтобы разбор объявил его окончание, и
 * ветвь эта покрытием пройдена не была ни разбором текста, ни выдачей записями
 *
 */
TEST(CodecCsvDocument, EmptyTextParsedWholly) {
	// Таблица значений
	csv::document_t document(::logger());
	// Выполняем проверку разбора пустого текста таблицы
	ASSERT_TRUE(document.parse(""));
	// Выполняем проверку отсутствия отказа разбора
	ASSERT_EQ(document.error(), csv::error_t::NONE);
	// Выполняем проверку отсутствия записей таблицы
	ASSERT_EQ(document.rows(), static_cast <size_t> (0));
	// Количество выданных записей таблицы
	size_t count = 0;
	// Выполняем проверку разбора пустого текста таблицы выдачей записями
	ASSERT_TRUE(document.parse("", [&count](const vector <string_view> &) noexcept -> bool {
		// Выполняем учёт очередной выданной записи
		count++;
		// Выводим признак продолжения разбора
		return true;
	}));
	// Выполняем проверку отсутствия выданных записей
	ASSERT_EQ(count, static_cast <size_t> (0));
	// Выполняем проверку отсутствия отказа разбора
	ASSERT_EQ(document.error(), csv::error_t::NONE);
}

/**
 * @brief Проверка отказа выдачи записями без переданного обработчика
 *
 * @details Отказ этот собственный внутренний изъян и означает: обработчик обязателен,
 * а разбор без него смысла не имеет
 *
 */
TEST(CodecCsvDocument, MissingCallbackRefused) {
	// Таблица значений
	csv::document_t document(::logger());
	// Пустой обработчик записей таблицы
	const function <bool (const vector <string_view> &)> empty;
	// Выполняем проверку отказа разбора текста таблицы без обработчика
	ASSERT_FALSE(document.parse("1,2\r\n", empty));
	// Выполняем проверку кода отказа разбора
	ASSERT_EQ(document.error(), csv::error_t::INTERNAL);
	// Выполняем проверку отказа чтения файла таблицы без обработчика
	ASSERT_FALSE(document.read("./csv-нет-такого-файла.csv", empty));
	// Выполняем проверку кода отказа чтения
	ASSERT_EQ(document.error(), csv::error_t::INTERNAL);
	// Выполняем проверку отказа чтения отсутствующего файла таблицы годным обработчиком
	ASSERT_FALSE(document.read("./csv-нет-такого-файла.csv", [](const vector <string_view> &) noexcept -> bool {
		// Выводим признак продолжения чтения
		return true;
	}));
	// Выполняем проверку кода отказа чтения
	ASSERT_EQ(document.error(), csv::error_t::FILE_NOT_OPENED);
}

/**
 * @brief Проверка сбора заголовка таблицы выдачей записями
 *
 * @details Имена столбцов переносятся в контейнер и выдачей записями: записи там не
 * оседают, а заголовок нужен уже после чтения. Ветвь эта покрытием пройдена не была
 *
 */
TEST(CodecCsvDocument, HeaderCollectedByCallbackParsing) {
	// Настройки таблицы
	csv::document_t::settings_t settings;
	// Выполняем указание на присутствие заголовка таблицы
	settings.reader.header = csv::header_t::PRESENT;
	// Таблица значений
	csv::document_t document(::logger(), settings);
	// Количество выданных записей таблицы
	size_t count = 0;
	// Выполняем проверку разбора текста таблицы выдачей записями
	ASSERT_TRUE(document.parse("альфа,бета\r\n1,2\r\n3,4\r\n", [&count](const vector <string_view> &) noexcept -> bool {
		// Выполняем учёт очередной выданной записи
		count++;
		// Выводим признак прекращения разбора после первой же записи
		return false;
	}));
	// Выполняем проверку прекращения разбора обработчиком
	ASSERT_EQ(count, static_cast <size_t> (1));
	// Выполняем проверку сбора заголовка таблицы
	ASSERT_EQ(document.header().size(), static_cast <size_t> (2));
	// Выполняем проверку первого имени столбца заголовка
	ASSERT_EQ(document.header().front(), "альфа");
	// Выполняем проверку записей, в контейнере не осевших
	ASSERT_EQ(document.rows(), static_cast <size_t> (0));
}

/**
 * @brief Проверка прекращения выдачи записями при отказе разбора
 *
 * @details Подача куска чтению отвечает отказом, и обходы обязаны прекращать подачу
 * тут же: продолжение кормило бы отказавшее чтение впустую. Ветви эти покрытием
 * пройдены не были ни разбором текста, ни чтением файла
 *
 */
TEST(CodecCsvDocument, MalformedTextStopsCallbackParsing) {
	// Настройки таблицы
	csv::document_t::settings_t settings;
	// Выполняем указание предела длины поля таблицы
	settings.reader.maxField = 4;
	// Таблица значений
	csv::document_t document(::logger(), settings);
	// Количество выданных записей таблицы
	size_t count = 0;
	// Обработчик очередной записи таблицы
	const auto callback = [&count](const vector <string_view> &) noexcept -> bool {
		// Выполняем учёт очередной выданной записи
		count++;
		// Выводим признак продолжения разбора
		return true;
	};
	// Выполняем проверку отказа разбора текста таблицы выдачей записями
	ASSERT_FALSE(document.parse("аб,вг\r\nслишком длинное поле,вг\r\n", callback));
	// Выполняем проверку оглашения отказа разбора
	ASSERT_NE(document.error(), csv::error_t::NONE);
	// Адрес файла таблицы
	const string filename = "./csv-malformed-callback.csv";
	{
		// Поток записи файла таблицы
		ofstream file(filename, ios::binary);
		// Выполняем запись текста таблицы в файл
		file << "аб,вг\r\nслишком длинное поле,вг\r\n";
	}
	// Сбрасываем количество выданных записей таблицы
	count = 0;
	// Выполняем проверку отказа чтения файла таблицы выдачей записями
	ASSERT_FALSE(document.read(filename, callback));
	// Выполняем проверку оглашения отказа чтения
	ASSERT_NE(document.error(), csv::error_t::NONE);
	// Выполняем проверку отказа чтения файла таблицы целиком
	ASSERT_FALSE(document.load(filename));
	// Выполняем проверку оглашения отказа чтения
	ASSERT_NE(document.error(), csv::error_t::NONE);
	// Выполняем удаление файла таблицы
	::remove(filename.c_str());
}

/**
 * @brief Проверка выдачи настроек контейнера
 *
 * @details Выдача настроек покрытием пройдена не была вовсе: проверки настройки лишь
 * устанавливали. Выданные настройки обязаны отвечать установленным
 *
 */
TEST(CodecCsvDocument, SettingsReadBack) {
	// Таблица значений
	csv::document_t document(::logger());
	// Настройки таблицы
	csv::document_t::settings_t settings;
	// Выполняем указание на присутствие заголовка таблицы
	settings.reader.header = csv::header_t::PRESENT;
	// Выполняем указание предела длины поля таблицы
	settings.reader.maxField = 128;
	// Выполняем установку настроек таблицы
	document.settings(settings);
	// Выполняем проверку выданного признака присутствия заголовка
	ASSERT_EQ(document.settings().reader.header, csv::header_t::PRESENT);
	// Выполняем проверку выданного предела длины поля
	ASSERT_EQ(document.settings().reader.maxField, static_cast <size_t> (128));
}

/**
 * @brief Проверка прекращения чтения файла таблицы обработчиком
 *
 * @details Ложь, выданная обработчиком, прекращает чтение файла, и ветвь эта покрытием
 * пройдена не была: проверки прекращения брали разбор текста, а не чтение файла
 *
 */
TEST(CodecCsvDocument, CallbackStopsFileReading) {
	// Таблица значений
	csv::document_t document(::logger());
	// Адрес файла таблицы
	const string filename = "./csv-callback-stop-file.csv";
	{
		// Поток записи файла таблицы
		ofstream file(filename, ios::binary);
		// Выполняем запись текста таблицы в файл
		file << "1,2\r\n3,4\r\n5,6\r\n";
	}
	// Количество выданных записей таблицы
	size_t count = 0;
	// Выполняем проверку чтения файла таблицы выдачей записями
	ASSERT_TRUE(document.read(filename, [&count](const vector <string_view> &) noexcept -> bool {
		// Выполняем учёт очередной выданной записи
		count++;
		// Выводим признак прекращения чтения после первой же записи
		return false;
	}));
	// Выполняем проверку прекращения чтения обработчиком
	ASSERT_EQ(count, static_cast <size_t> (1));
	// Выполняем проверку отсутствия отказа чтения
	ASSERT_EQ(document.error(), csv::error_t::NONE);
	// Выполняем удаление файла таблицы
	::remove(filename.c_str());
}

/**
 * @brief Проверка установки объекта ведения журнала работы после создания
 *
 * @details Кодеки JSON и XML дают установку журнала всякому своему классу, а кодек CSV
 * не давал её вовсе: журнал принимался лишь конструктором. Расхождение это в договоре
 * трёх кодеков одного устройства, и держать его нечем
 *
 * @note Запись таблицы своих сообщений не имеет и журнал держит ради того же договора:
 *       проверяется здесь лишь то, что установка сборке текста не мешает
 *
 */
TEST(CodecCsvDocument, LoggerSetAfterCreation) {
	// Собираемые сообщения журнала
	vector <string> messages;
	// Объект журнала с перехватом вывода
	awh::log_t log(&Silent::framework());
	// Выполняем назначение приёмника вывода в функцию обратного вызова
	log.mode({awh::log_t::mode_t::DEFERRED});
	// Выполняем назначение перехвата сообщений журнала
	log.subscribe([&messages](const awh::log_t::flag_t, string_view text) noexcept -> void {
		// Выполняем сбор очередного сообщения журнала
		messages.push_back(string(text));
	});
	{
		// Таблица значений без объекта ведения журнала работы
		csv::document_t document(nullptr);
		// Выполняем проверку разбора текста таблицы
		ASSERT_TRUE(document.parse("имя\nзначение\n"));
		// Выполняем проверку отказа записи в несуществующий каталог
		ASSERT_FALSE(document.save("/несуществующий/каталог/таблица.csv"));
		// Выполняем проверку молчания журнала, покуда он не установлен
		ASSERT_TRUE(messages.empty());
		// Выполняем установку объекта ведения журнала работы
		document.setLogger(&log);
		// Выполняем проверку отказа записи в несуществующий каталог
		ASSERT_FALSE(document.save("/несуществующий/каталог/таблица.csv"));
		// Выполняем проверку оглашения отказа в журнале
		ASSERT_FALSE(messages.empty());
	}
	// Очищаем собранные сообщения журнала
	messages.clear();
	{
		// Чтение текста таблицы без объекта ведения журнала работы
		csv::reader_t reader(nullptr);
		// Негодная последовательность знаков UTF-8
		const char broken[] = {'a', ',', 'b', ',', '\xc2', '\xc2'};
		// Выполняем проверку отказа разбора негодного текста таблицы
		ASSERT_FALSE(reader.feed(broken, sizeof(broken), true));
		// Выполняем проверку молчания журнала, покуда он не установлен
		ASSERT_TRUE(messages.empty());
		// Выполняем сброс состояния чтения
		reader.reset();
		// Выполняем установку объекта ведения журнала работы
		reader.setLogger(&log);
		// Выполняем проверку отказа разбора негодного текста таблицы
		ASSERT_FALSE(reader.feed(broken, sizeof(broken), true));
		// Выполняем проверку оглашения отказа в журнале
		ASSERT_FALSE(messages.empty());
	}
	// Очищаем собранные сообщения журнала
	messages.clear();
	{
		// Приведение текста таблицы без объекта ведения журнала работы
		csv::decoder_t decoder(nullptr);
		// Полученный приведением текст таблицы
		string result;
		// Негодная последовательность знаков UTF-8
		const char broken[] = {'a', ',', 'b', ',', '\xc2', '\xc2'};
		// Выполняем проверку отказа приведения негодного текста таблицы
		ASSERT_FALSE(decoder.convert(broken, sizeof(broken), true, result));
		// Выполняем проверку молчания журнала, покуда он не установлен
		ASSERT_TRUE(messages.empty());
		// Выполняем сброс состояния приведения
		decoder.reset();
		// Выполняем установку объекта ведения журнала работы
		decoder.setLogger(&log);
		// Выполняем проверку отказа приведения негодного текста таблицы
		ASSERT_FALSE(decoder.convert(broken, sizeof(broken), true, result));
		// Выполняем проверку оглашения отказа в журнале
		ASSERT_FALSE(messages.empty());
	}
	{
		// Запись таблицы без объекта ведения журнала работы
		csv::writer_t writer(nullptr);
		// Выполняем установку объекта ведения журнала работы
		writer.setLogger(&log);
		// Выполняем запись поля таблицы
		writer.field("значение");
		// Выполняем завершение записи таблицы
		writer.record();
		// Выполняем проверку собранного текста таблицы
		ASSERT_EQ(writer.take(), "значение\r\n");
	}
}
/**
 * @brief Проверка подъёма отказа записи всеми путями выдачи таблицы
 *
 * @details Поле, установленными настройками записи непредставимое, обязано отказом
 *          подниматься и у выдачи текста, и у сохранения в файл, и по заголовку, и по
 *          записям: иначе порченая таблица ушла бы к читающему целой
 *
 * @note Выдача текста отдаёт при отказе ПУСТУЮ строку: вывод её - строка, признака
 *       отказа у неё нет вовсе, а таблица непустая пустого текста не даёт никогда.
 *       Причину оглашает журналом сама запись
 *
 * @note Пути эти карта покрытия показала непройденными: отказ был заведён, а проверен
 *       лишь у самой записи, минуя контейнер
 *
 */
TEST(CodecCsvDocument, UnwritableFieldStopsEveryOutput){
	/**
	 * Выполняем перебор двух путей: по записям и по заголовку
	 */
	for(uint8_t heading = 0; heading < 2; heading++){
		// Объект контейнера таблицы
		csv::document_t document(::logger());
		// Настройки контейнера
		csv::document_t::settings_t settings;
		/**
		 * Если проверяется путь по заголовку
		 */
		if(heading != 0)
			// Устанавливаем признак объявленного заголовка
			settings.reader.header = csv::header_t::PRESENT;
		// Выполняем установку настроек контейнера
		document.settings(settings);
		// Выполняем разбор таблицы, поле которой содержит разделитель
		ASSERT_TRUE(document.parse(string("\"а,б\",в\r\nпервое,второе\r\n"))) << csv::message(document.error());
		// Получаем настройки контейнера для правки
		csv::document_t::settings_t writing = document.settings();
		// Устанавливаем отказ от кавычек вовсе
		writing.writer.quoting = csv::quoting_t::NONE;
		// Устанавливаем способ записи кавычки удвоением
		writing.writer.escape = csv::escape_t::DOUBLE;
		// Выполняем установку настроек контейнера
		document.settings(writing);
		// Выполняем проверку того, что выдача текста отказала пустою строкою
		ASSERT_TRUE(document.text().empty()) << uint32_t(heading);
		// Адрес файла, в какой записывается таблица
		const string output = "./awh_csv_unwritable.csv";
		// Выполняем проверку отказа сохранения таблицы в файл
		ASSERT_FALSE(document.save(output)) << uint32_t(heading);
		// Выполняем снос оставленного файла
		::remove(output.c_str());
	}
}
