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
#include <string>
#include <vector>
#include <cstdio>
#include <fstream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/csv/csv.hpp>

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
	csv::document_t document;
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
	csv::document_t document;
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
	csv::document_t document;
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
	csv::document_t document;
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
	csv::document_t document;
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
	csv::document_t document;
	// Выполняем разбор текста таблицы с записями разной длины
	ASSERT_FALSE(document.parse("a,b\nc\n", settings));
	// Выполняем проверку кода ошибки разбора
	ASSERT_EQ(document.error(), csv::error_t::FIELD_COUNT_MISMATCH);
	// Выполняем проверку номера записи, на какой разбор прекращён
	ASSERT_EQ(document.location().line, 2u);
}

/**
 * @brief Проверка приведения содержимого поля к числу
 *
 */
TEST(CodecCsvDocument, Numeric) {
	// Объект контейнера таблицы
	csv::document_t document;
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
	csv::document_t document(settings);
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
	csv::document_t document;
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
	csv::document_t document;
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
	csv::document_t document;
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
	csv::document_t document;
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
	csv::document_t source;
	// Выполняем разбор текста исходной таблицы с объявленным заголовком
	ASSERT_TRUE(source.parse("name,value\n\"a\nb\",\"c,d\"\n\"e\"\"f\",g\n", heading()));
	// Объект контейнера полученной обратно таблицы
	csv::document_t result;
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
	csv::document_t document;
	// Выполняем чтение таблицы из файла
	ASSERT_TRUE(document.read(filename));
	// Выполняем проверку количества прочитанных записей таблицы
	ASSERT_EQ(document.rows(), 3u);
	// Выполняем проверку содержимого поля прочитанной таблицы
	ASSERT_EQ(document.get(2, size_t(1)), "2");
	// Адрес файла, в какой записывается таблица
	const string output = "./awh_csv_file_out.csv";
	// Выполняем запись таблицы в файл
	ASSERT_TRUE(document.write(output));
	// Объект контейнера полученной обратно таблицы
	csv::document_t result;
	// Выполняем чтение записанной таблицы из файла
	ASSERT_TRUE(result.read(output));
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
	csv::document_t document;
	// Выполняем проверку отказа чтения отсутствующего файла таблицы
	ASSERT_FALSE(document.read("./awh_csv_missing.csv"));
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
	csv::document_t document;
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
	csv::document_t document;
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
	csv::document_t document(heading());
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
	csv::document_t document;
	// Выполняем разбор текста таблицы
	ASSERT_TRUE(document.parse("a,b\n"));
	// Выполняем проверку вывода таблицы последовательностью знаков
	ASSERT_EQ(static_cast <string> (document), "a,b\r\n");
}
