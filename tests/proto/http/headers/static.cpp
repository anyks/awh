/**
 * @file static.cpp
 * @date 2026-07-12
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
 * @brief Статические тесты контейнера HTTP-заголовков — проверка создания и сброса объекта модуля,
 *        а также корректности регистронезависимого поиска полей,
 *        работы с множественными значениями и контроля лимитов
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <set>
#include <map>
#include <regex>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

/**
 * Подключаем заголовочный файлы проекта
 */
#include <locale>

#include "headers.hpp"

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Метод создания и сброса контейнера заголовков
 *
 */
TEST_F(HeadersFixture, CreateHeadersTest){
	// Проверяем что объект контейнера заголовков создан
	ASSERT_TRUE(this->_headers != nullptr);
	// Проверяем что новый контейнер пустой
	ASSERT_TRUE(this->_headers->empty());
	// Проверяем что размер нового контейнера нулевой
	ASSERT_EQ(this->_headers->size(), 0u);
	// Проверяем что потребление памяти нового контейнера нулевое
	ASSERT_EQ(this->_headers->memory(), 0u);
	// Выполняем сброс объекта контейнера заголовков
	this->_headers.reset();
	// Проверяем что объект контейнера заголовков удалён
	ASSERT_TRUE(this->_headers == nullptr);
}

/**
 * @brief Метод проверки создания HTTP-заголовка через фабричный метод
 *
 */
TEST_F(HeadersFixture, HeaderFromTest){
	// Создаём заголовок с корректными данными
	headers_t::header_t header1;
	// Заполняем название и значение заголовка
	header1.from("Content-Type", "text/html");
	// Проверяем что название заголовка установлено
	ASSERT_EQ(header1.name, "Content-Type");
	// Проверяем что значение заголовка установлено
	ASSERT_EQ(header1.value, "text/html");
	// Создаём заголовок с пустым значением
	headers_t::header_t header2;
	// Заполняем заголовок с пустым значением (пустое значение допустимо согласно RFC 9110)
	header2.from("Content-Type", "");
	// Проверяем что название заголовка сохранено
	ASSERT_EQ(header2.name, "Content-Type");
	// Проверяем что значение заголовка пустое
	ASSERT_TRUE(header2.value.empty());
	// Создаём заголовок с пустым названием
	headers_t::header_t headerEmpty;
	// Заполняем заголовок с пустым названием (заголовок без названия некорректен, данные должны быть отброшены)
	headerEmpty.from("", "application/json");
	// Проверяем что название заголовка сброшено
	ASSERT_TRUE(headerEmpty.name.empty());
	// Проверяем что значение заголовка сброшено
	ASSERT_TRUE(headerEmpty.value.empty());
	// Создаём заголовок с названием в другом регистре и другим значением
	headers_t::header_t header3;
	// Заполняем заголовок с названием в другом регистре
	header3.from("content-type", "application/json");
	/**
	 * Проверяем что заголовки с одинаковым названием, но разными значениями
	 * равными не считаются: это два разных заголовка, и сравнение по одному лишь
	 * названию считало бы равными [Set-Cookie: a=1] и [Set-Cookie: b=2]
	 */
	ASSERT_FALSE(header1 == header3);
	// Создаём заголовок с тем же значением и названием в другом регистре
	headers_t::header_t header4;
	// Заполняем заголовок с названием в другом регистре
	header4.from("content-type", "text/html");
	// Проверяем что регистр названия на сравнение не влияет: названия полей регистронезависимы
	ASSERT_TRUE(header1 == header4);
	// Создаём заголовок с тем же названием и значением в другом регистре
	headers_t::header_t header5;
	// Заполняем заголовок со значением в другом регистре
	header5.from("Content-Type", "TEXT/HTML");
	// Проверяем что регистр значения на сравнение влияет: значения полей регистрозависимы
	ASSERT_FALSE(header1 == header5);
}

/**
 * @brief Метод проверки добавления и извлечения заголовков
 *
 */
TEST_F(HeadersFixture, EmplaceAndAtTest){
	// Добавляем заголовок и проверяем возвращаемое количество
	ASSERT_EQ(this->_headers->emplace("Content-Type", "text/html"), 1u);
	// Добавляем ещё один заголовок
	ASSERT_EQ(this->_headers->emplace("Host", "example.com"), 2u);
	// Проверяем что контейнер не пустой
	ASSERT_FALSE(this->_headers->empty());
	// Проверяем общее количество заголовков
	ASSERT_EQ(this->_headers->size(), 2u);
	// Проверяем извлечение значения заголовка через метод at
	ASSERT_EQ(this->_headers->at("Content-Type"), "text/html");
	// Проверяем извлечение значения заголовка через оператор индексации
	ASSERT_EQ((* this->_headers)["Host"], "example.com");
	// Проверяем что извлечение несуществующего заголовка возвращает пустую строку
	ASSERT_TRUE(this->_headers->at("X-Missing").empty());
}

/**
 * @brief Метод проверки всех перегрузок метода добавления заголовка
 *
 */
TEST_F(HeadersFixture, EmplaceOverloadsTest){
	// Формируем константные строки для проверки перегрузок с копированием
	const std::string constName = "X-Const";
	// Формируем константное значение для проверки перегрузок с копированием
	const std::string constValue = "const-value";
	// Добавляем заголовок через перегрузку с перемещением обеих строк (string && / string &&)
	ASSERT_EQ(this->_headers->emplace(std::string("Content-Type"), std::string("text/html")), 1u);
	// Проверяем что заголовок добавлен через перегрузку с перемещением обеих строк
	ASSERT_EQ(this->_headers->at("Content-Type"), "text/html");
	// Добавляем заголовок через перегрузку с перемещением названия и копированием содержимого (string && / const string &)
	ASSERT_EQ(this->_headers->emplace(std::string("Accept"), constValue), 2u);
	// Проверяем что заголовок добавлен через перегрузку с перемещением названия и копированием содержимого
	ASSERT_EQ(this->_headers->at("Accept"), constValue);
	// Добавляем заголовок через перегрузку с копированием названия и перемещением содержимого (const string & / string &&)
	ASSERT_EQ(this->_headers->emplace(constName, std::string("moved-value")), 3u);
	// Проверяем что заголовок добавлен через перегрузку с копированием названия и перемещением содержимого
	ASSERT_EQ(this->_headers->at("X-Const"), "moved-value");
	// Добавляем заголовок через перегрузку с копированием обеих строк (const string & / const string &)
	ASSERT_EQ(this->_headers->emplace(constName, constValue), 3u);
	// Проверяем что заголовок с тем же названием заменён при копировании обеих строк
	ASSERT_EQ(this->_headers->at("X-Const"), constValue);
	// Добавляем заголовок через перегрузку со строковыми представлениями (string_view / string_view)
	ASSERT_EQ(this->_headers->emplace(std::string_view("Host"), std::string_view("example.com")), 4u);
	// Проверяем что заголовок добавлен через перегрузку со строковыми представлениями
	ASSERT_EQ(this->_headers->at("Host"), "example.com");
	// Добавляем заголовок через перегрузку с C-строками (const char * / const char *) без неоднозначности
	ASSERT_EQ(this->_headers->emplace("X-Powered-By", "awh"), 5u);
	// Проверяем что заголовок добавлен через перегрузку с C-строками
	ASSERT_EQ(this->_headers->at("X-Powered-By"), "awh");
	// Добавляем заголовок через смешанную перегрузку C-строки и перемещаемой строки (const char * / string &&)
	ASSERT_EQ(this->_headers->emplace("X-Move", std::string("moved")), 6u);
	// Проверяем что заголовок добавлен через смешанную перегрузку C-строки и перемещаемой строки
	ASSERT_EQ(this->_headers->at("X-Move"), "moved");
	// Добавляем заголовок через смешанную перегрузку перемещаемой строки и C-строки (string && / const char *)
	ASSERT_EQ(this->_headers->emplace(std::string("X-Literal"), "literal"), 7u);
	// Проверяем что заголовок добавлен через смешанную перегрузку перемещаемой строки и C-строки
	ASSERT_EQ(this->_headers->at("X-Literal"), "literal");
	// Добавляем заголовок через смешанную перегрузку C-строки и строкового представления (const char * / string_view)
	ASSERT_EQ(this->_headers->emplace("X-View", std::string_view("view")), 8u);
	// Проверяем что заголовок добавлен через смешанную перегрузку C-строки и строкового представления
	ASSERT_EQ(this->_headers->at("X-View"), "view");
}

/**
 * @brief Метод проверки семантики переноса в смешанных перегрузках добавления заголовка
 *
 */
TEST_F(HeadersFixture, EmplaceMixedMoveSemanticsTest){
	// Проверяем перегрузку с перемещением названия и копированием содержимого (string && / const string &)
	std::string movedName(4096, 'n');
	// Запоминаем указатель на внутренний буфер названия до перемещения
	const char * movedNamePointer = movedName.data();
	// Формируем константное содержимое, которое должно быть скопировано
	const std::string keptValue = "kept-value";
	// Добавляем заголовок с перемещением названия и копированием содержимого
	this->_headers->emplace(std::move(movedName), keptValue);
	// Проверяем что исходная строка названия перемещена (опустошена)
	ASSERT_TRUE(movedName.empty());
	// Проверяем что скопированное содержимое сохранено у источника
	ASSERT_EQ(keptValue, "kept-value");
	// Проверяем что заголовок добавлен по перемещённому названию без изменения буфера
	ASSERT_EQ(this->_headers->find(std::string(4096, 'n'))->name.data(), movedNamePointer);
	// Проверяем перегрузку с копированием названия и перемещением содержимого (const string & / string &&)
	const std::string keptName = "X-Kept-Name";
	// Формируем объёмное содержимое для перемещения
	std::string movedValue(4096, 'v');
	// Запоминаем указатель на внутренний буфер содержимого до перемещения
	const char * movedValuePointer = movedValue.data();
	// Добавляем заголовок с копированием названия и перемещением содержимого
	this->_headers->emplace(keptName, std::move(movedValue));
	// Проверяем что скопированное название сохранено у источника
	ASSERT_EQ(keptName, "X-Kept-Name");
	// Проверяем что исходная строка содержимого перемещена (опустошена)
	ASSERT_TRUE(movedValue.empty());
	// Проверяем что содержимое перемещено без изменения буфера
	ASSERT_EQ(this->_headers->at("X-Kept-Name").data(), movedValuePointer);
}

/**
 * @brief Метод проверки сохранения перемещаемых строк при добавлении заголовка
 *
 */
TEST_F(HeadersFixture, EmplaceMoveSemanticsTest){
	// Формируем название заголовка для перемещения
	std::string name = "X-Large-Header";
	// Формируем объёмное значение заголовка для перемещения
	std::string value(4096, 'x');
	// Запоминаем указатель на внутренний буфер значения до перемещения
	const char * pointer = value.data();
	// Добавляем заголовок через перегрузку с перемещением строк
	this->_headers->emplace(std::move(name), std::move(value));
	// Проверяем что заголовок добавлен
	ASSERT_EQ(this->_headers->size(), 1u);
	// Проверяем что значение заголовка сохранено корректно
	ASSERT_EQ(this->_headers->at("X-Large-Header").size(), 4096u);
	// Проверяем что исходная строка названия перемещена (опустошена)
	ASSERT_TRUE(name.empty());
	// Проверяем что исходная строка значения перемещена (опустошена)
	ASSERT_TRUE(value.empty());
	// Проверяем что внутренний буфер значения переиспользован (перенос, а не копирование)
	ASSERT_EQ(this->_headers->at("X-Large-Header").data(), pointer);
}

/**
 * @brief Метод проверки замены заголовка при повторном добавлении
 *
 */
TEST_F(HeadersFixture, EmplaceReplaceTest){
	// Добавляем заголовок
	this->_headers->emplace("Accept", "text/html");
	// Проверяем что заголовок добавлен
	ASSERT_EQ(this->_headers->count("Accept"), 1u);
	// Повторно добавляем заголовок с тем же названием, но другим значением
	this->_headers->emplace("Accept", "application/json");
	// Проверяем что заголовок заменён, а не продублирован
	ASSERT_EQ(this->_headers->count("Accept"), 1u);
	// Проверяем что значение заголовка обновлено
	ASSERT_EQ(this->_headers->at("Accept"), "application/json");
	// Проверяем что общее количество заголовков не изменилось
	ASSERT_EQ(this->_headers->size(), 1u);
}

/**
 * @brief Метод проверки добавления заголовков в режиме APPEND
 *
 */
TEST_F(HeadersFixture, EmplaceAppendModeTest){
	// Добавляем первый заголовок с указанным названием в режиме добавления
	ASSERT_EQ(this->_headers->emplace("Set-Cookie", "a=1", headers_t::mode_t::APPEND), 1u);
	// Добавляем второй заголовок с тем же названием в режиме добавления
	ASSERT_EQ(this->_headers->emplace("Set-Cookie", "b=2", headers_t::mode_t::APPEND), 2u);
	// Добавляем третий заголовок с тем же названием в режиме добавления
	ASSERT_EQ(this->_headers->emplace("Set-Cookie", "c=3", headers_t::mode_t::APPEND), 3u);
	// Проверяем что все одноимённые заголовки сохранены
	ASSERT_EQ(this->_headers->count("Set-Cookie"), 3u);
	// Проверяем общее количество заголовков
	ASSERT_EQ(this->_headers->size(), 3u);
	// Получаем список значений одноимённых заголовков
	std::vector <std::string> values = this->_headers->range("Set-Cookie");
	// Сортируем значения для устойчивого сравнения (порядок добавления сохраняется, но сортируем для надёжности)
	std::sort(values.begin(), values.end());
	// Проверяем количество извлечённых значений
	ASSERT_EQ(values.size(), 3u);
	// Проверяем первое значение
	ASSERT_EQ(values.at(0), "a=1");
	// Проверяем второе значение
	ASSERT_EQ(values.at(1), "b=2");
	// Проверяем третье значение
	ASSERT_EQ(values.at(2), "c=3");
}

/**
 * @brief Метод проверки замены одноимённых заголовков в режиме REPLACE
 *
 */
TEST_F(HeadersFixture, EmplaceReplaceModeTest){
	// Добавляем несколько одноимённых заголовков в режиме добавления
	this->_headers->emplace("Set-Cookie", "a=1", headers_t::mode_t::APPEND);
	// Добавляем ещё один одноимённый заголовок в режиме добавления
	this->_headers->emplace("Set-Cookie", "b=2", headers_t::mode_t::APPEND);
	// Проверяем что оба заголовка сохранены
	ASSERT_EQ(this->_headers->count("Set-Cookie"), 2u);
	// Явно указываем режим замены при добавлении нового значения
	ASSERT_EQ(this->_headers->emplace("Set-Cookie", "z=9", headers_t::mode_t::REPLACE), 1u);
	// Проверяем что все прежние вхождения схлопнуты в одно
	ASSERT_EQ(this->_headers->count("Set-Cookie"), 1u);
	// Проверяем что осталось только новое значение
	ASSERT_EQ(this->_headers->at("Set-Cookie"), "z=9");
	// Проверяем общее количество заголовков
	ASSERT_EQ(this->_headers->size(), 1u);
}

/**
 * @brief Метод проверки режима добавления по умолчанию (REPLACE)
 *
 */
TEST_F(HeadersFixture, EmplaceDefaultModeTest){
	// Добавляем заголовок без явного указания режима
	this->_headers->emplace("Accept", "text/html");
	// Повторно добавляем одноимённый заголовок без явного указания режима
	this->_headers->emplace("Accept", "application/json");
	// Проверяем что по умолчанию используется режим замены (заголовок не продублирован)
	ASSERT_EQ(this->_headers->count("Accept"), 1u);
	// Проверяем что сохранено последнее значение
	ASSERT_EQ(this->_headers->at("Accept"), "application/json");
}

/**
 * @brief Метод проверки режима APPEND в перегрузках метода добавления заголовка
 *
 */
TEST_F(HeadersFixture, EmplaceAppendOverloadsTest){
	// Формируем константные строки для проверки перегрузок с копированием
	const std::string constName = "X-Multi";
	// Формируем константное значение для проверки перегрузок с копированием
	const std::string constValue = "v-const";
	// Добавляем заголовок через перегрузку с C-строками в режиме добавления (const char * / const char *)
	ASSERT_EQ(this->_headers->emplace("X-Multi", "v-cstr", headers_t::mode_t::APPEND), 1u);
	// Добавляем заголовок через перегрузку с перемещением обеих строк в режиме добавления (string && / string &&)
	ASSERT_EQ(this->_headers->emplace(std::string("X-Multi"), std::string("v-move"), headers_t::mode_t::APPEND), 2u);
	// Добавляем заголовок через перегрузку со строковыми представлениями в режиме добавления (string_view / string_view)
	ASSERT_EQ(this->_headers->emplace(std::string_view("X-Multi"), std::string_view("v-view"), headers_t::mode_t::APPEND), 3u);
	// Добавляем заголовок через перегрузку с копированием обеих строк в режиме добавления (const string & / const string &)
	ASSERT_EQ(this->_headers->emplace(constName, constValue, headers_t::mode_t::APPEND), 4u);
	// Проверяем что все одноимённые заголовки сохранены во всех перегрузках
	ASSERT_EQ(this->_headers->count("X-Multi"), 4u);
	// Получаем список значений одноимённых заголовков
	std::vector <std::string> values = this->_headers->range("X-Multi");
	// Сортируем значения для устойчивого сравнения
	std::sort(values.begin(), values.end());
	// Проверяем количество извлечённых значений
	ASSERT_EQ(values.size(), 4u);
	// Проверяем набор сохранённых значений
	ASSERT_EQ(values.at(0), "v-const");
	// Проверяем набор сохранённых значений
	ASSERT_EQ(values.at(1), "v-cstr");
	// Проверяем набор сохранённых значений
	ASSERT_EQ(values.at(2), "v-move");
	// Проверяем набор сохранённых значений
	ASSERT_EQ(values.at(3), "v-view");
}

/**
 * @brief Метод проверки регистронезависимого доступа к заголовкам
 *
 */
TEST_F(HeadersFixture, CaseInsensitiveTest){
	// Добавляем заголовок в смешанном регистре
	this->_headers->emplace("Content-Length", "42");
	// Проверяем существование заголовка в нижнем регистре
	ASSERT_TRUE(this->_headers->has("content-length"));
	// Проверяем существование заголовка в верхнем регистре
	ASSERT_TRUE(this->_headers->has("CONTENT-LENGTH"));
	// Проверяем извлечение значения заголовка в другом регистре
	ASSERT_EQ(this->_headers->at("content-length"), "42");
	// Проверяем что замена по имени в другом регистре заменяет исходный заголовок
	this->_headers->emplace("CONTENT-length", "100");
	// Проверяем что заголовок не был продублирован
	ASSERT_EQ(this->_headers->size(), 1u);
	// Проверяем что значение заголовка обновлено
	ASSERT_EQ(this->_headers->at("Content-Length"), "100");
}

/**
 * @brief Метод проверки множественных заголовков с одинаковым названием
 *
 */
TEST_F(HeadersFixture, MultipleValuesTest){
	// Формируем набор заголовков с повторяющимся названием
	headers_t::fields_t data = {
		this->header("Set-Cookie", "a=1"),
		this->header("Set-Cookie", "b=2"),
		this->header("Set-Cookie", "c=3")
	};
	// Присваиваем набор заголовков (сохраняет допустимые дубликаты имён)
	(* this->_headers) = data;
	// Проверяем что все дубликаты сохранены
	ASSERT_EQ(this->_headers->count("Set-Cookie"), 3u);
	// Получаем список значений заголовков с одинаковым названием
	std::vector <std::string> values = this->_headers->range("Set-Cookie");
	// Сортируем значения для устойчивого сравнения (порядок в контейнере не определён)
	std::sort(values.begin(), values.end());
	// Проверяем количество извлечённых значений
	ASSERT_EQ(values.size(), 3u);
	// Проверяем первое значение
	ASSERT_EQ(values.at(0), "a=1");
	// Проверяем второе значение
	ASSERT_EQ(values.at(1), "b=2");
	// Проверяем третье значение
	ASSERT_EQ(values.at(2), "c=3");
}

/**
 * @brief Метод проверки извлечения списка названий заголовков
 *
 */
TEST_F(HeadersFixture, NamesTest){
	// Добавляем несколько заголовков
	this->_headers->emplace("Host", "example.com");
	// Добавляем ещё один заголовок
	this->_headers->emplace("Accept", "text/html");
	// Добавляем ещё один заголовок
	this->_headers->emplace("User-Agent", "awh");
	// Получаем список названий заголовков
	std::vector <std::string> names = this->_headers->names();
	// Формируем множество названий для устойчивой проверки
	std::set <std::string> unique(names.begin(), names.end());
	// Проверяем количество уникальных названий
	ASSERT_EQ(unique.size(), 3u);
	// Проверяем наличие названия Host
	ASSERT_TRUE(unique.count("Host") > 0);
	// Проверяем наличие названия Accept
	ASSERT_TRUE(unique.count("Accept") > 0);
	// Проверяем наличие названия User-Agent
	ASSERT_TRUE(unique.count("User-Agent") > 0);
}

/**
 * @brief Метод проверки удаления заголовка по названию
 *
 */
TEST_F(HeadersFixture, EraseByNameTest){
	// Добавляем несколько заголовков
	this->_headers->emplace("Host", "example.com");
	// Добавляем ещё один заголовок
	this->_headers->emplace("Accept", "text/html");
	// Проверяем количество заголовков перед удалением
	ASSERT_EQ(this->_headers->size(), 2u);
	// Удаляем заголовок по названию
	this->_headers->erase("Host");
	// Проверяем что заголовок удалён
	ASSERT_FALSE(this->_headers->has("Host"));
	// Проверяем что оставшийся заголовок присутствует
	ASSERT_TRUE(this->_headers->has("Accept"));
	// Проверяем что количество заголовков уменьшилось
	ASSERT_EQ(this->_headers->size(), 1u);
	// Удаляем все дубликаты за один вызов
	this->_headers->erase("accept");
	// Проверяем что контейнер опустел
	ASSERT_TRUE(this->_headers->empty());
}

/**
 * @brief Метод проверки удаления заголовка по итератору
 *
 */
TEST_F(HeadersFixture, EraseByIteratorTest){
	// Добавляем несколько заголовков
	this->_headers->emplace("Host", "example.com");
	// Добавляем ещё один заголовок
	this->_headers->emplace("Accept", "text/html");
	// Получаем итератор первого заголовка
	headers_t::iterator_t it = this->_headers->begin();
	// Проверяем что итератор действителен
	ASSERT_TRUE(it != this->_headers->end());
	// Удаляем заголовок по итератору
	this->_headers->erase(it);
	// Проверяем что количество заголовков уменьшилось
	ASSERT_EQ(this->_headers->size(), 1u);
}

/**
 * @brief Метод проверки поиска заголовка
 *
 */
TEST_F(HeadersFixture, FindTest){
	// Добавляем заголовок
	this->_headers->emplace("Host", "example.com");
	// Ищем существующий заголовок
	headers_t::iterator_t found = this->_headers->find("Host");
	// Проверяем что заголовок найден
	ASSERT_TRUE(found != this->_headers->end());
	// Проверяем значение найденного заголовка
	ASSERT_EQ(found->value, "example.com");
	// Ищем несуществующий заголовок
	headers_t::iterator_t missing = this->_headers->find("X-Missing");
	// Проверяем что несуществующий заголовок не найден
	ASSERT_TRUE(missing == this->_headers->end());
}

/**
 * @brief Метод проверки очистки и полного сброса контейнера
 *
 */
TEST_F(HeadersFixture, ClearResetTest){
	// Добавляем заголовки
	this->_headers->emplace("Host", "example.com");
	// Добавляем ещё один заголовок
	this->_headers->emplace("Accept", "text/html");
	// Проверяем что контейнер не пустой
	ASSERT_FALSE(this->_headers->empty());
	// Выполняем очистку контейнера
	this->_headers->clear();
	// Проверяем что контейнер пустой
	ASSERT_TRUE(this->_headers->empty());
	// Проверяем что потребление памяти сброшено
	ASSERT_EQ(this->_headers->memory(), 0u);
	// Повторно добавляем заголовок
	this->_headers->emplace("Host", "example.com");
	// Выполняем полный сброс контейнера
	this->_headers->reset();
	// Проверяем что контейнер пустой
	ASSERT_TRUE(this->_headers->empty());
	// Проверяем что потребление памяти сброшено
	ASSERT_EQ(this->_headers->memory(), 0u);
}

/**
 * @brief Метод проверки корректности учёта потребляемой памяти
 *
 */
TEST_F(HeadersFixture, MemoryAccountingTest){
	// Проверяем что новый контейнер не потребляет память
	ASSERT_EQ(this->_headers->memory(), 0u);
	// Добавляем заголовок с известным объёмом полезной нагрузки
	this->_headers->emplace("Content-Type", "text/html");
	// Проверяем что учтён объём названия и значения заголовка (12 + 9)
	ASSERT_EQ(this->_headers->memory(), 21u);
	// Добавляем ещё один заголовок
	this->_headers->emplace("Host", "example.com");
	// Проверяем что учтён суммарный объём заголовков (21 + 4 + 11)
	ASSERT_EQ(this->_headers->memory(), 36u);
	// Удаляем один заголовок
	this->_headers->erase("Host");
	// Проверяем что учёт памяти уменьшился на объём удалённого заголовка
	ASSERT_EQ(this->_headers->memory(), 21u);
	// Заменяем существующий заголовок на заголовок другого объёма
	this->_headers->emplace("Content-Type", "application/json");
	// Проверяем что учёт памяти пересчитан корректно (12 + 16)
	ASSERT_EQ(this->_headers->memory(), 28u);
}

/**
 * @brief Метод проверки ограничения на количество заголовков
 *
 */
TEST_F(HeadersFixture, MaxRecordsLimitTest){
	// Устанавливаем ограничение на количество заголовков
	this->_headers->maxRecords(2);
	// Проверяем что ограничение установлено
	ASSERT_EQ(this->_headers->maxRecords(), 2u);
	// Добавляем первый заголовок в пределах лимита
	this->_headers->emplace("A", "1");
	// Добавляем второй заголовок в пределах лимита
	this->_headers->emplace("B", "2");
	// Пытаемся добавить третий заголовок сверх лимита
	this->_headers->emplace("C", "3");
	// Проверяем что третий заголовок не был добавлен
	ASSERT_EQ(this->_headers->size(), 2u);
	// Проверяем что третий заголовок отсутствует
	ASSERT_FALSE(this->_headers->has("C"));
}

/**
 * @brief Метод проверки ограничения на потребляемую память
 *
 */
TEST_F(HeadersFixture, MaxMemoryLimitTest){
	// Устанавливаем ограничение на потребляемую память
	this->_headers->maxMemory(10);
	// Проверяем что ограничение установлено
	ASSERT_EQ(this->_headers->maxMemory(), 10u);
	// Добавляем заголовок в пределах лимита (2 + 2 = 4)
	this->_headers->emplace("AB", "12");
	// Проверяем что заголовок добавлен
	ASSERT_EQ(this->_headers->size(), 1u);
	// Пытаемся добавить заголовок, превышающий лимит (4 + 8 > 10)
	this->_headers->emplace("CD", "abcdef");
	// Проверяем что заголовок сверх лимита не был добавлен
	ASSERT_EQ(this->_headers->size(), 1u);
	// Проверяем что учёт памяти не изменился
	ASSERT_EQ(this->_headers->memory(), 4u);
}

/**
 * @brief Метод проверки установки и получения протокола
 *
 */
TEST_F(HeadersFixture, ProtoGetSetTest){
	// Проверяем протокол по умолчанию
	ASSERT_EQ(this->_headers->proto(), proto_t::NONE);
	// Устанавливаем протокол HTTP/1.1
	this->_headers->proto(proto_t::HTTP1);
	// Проверяем что протокол установлен
	ASSERT_EQ(this->_headers->proto(), proto_t::HTTP1);
	// Устанавливаем протокол HTTP/2 через оператор присваивания
	(* this->_headers) = proto_t::HTTP2;
	// Проверяем что протокол установлен
	ASSERT_EQ(this->_headers->proto(), proto_t::HTTP2);
}

/**
 * @brief Метод проверки установки провайдера без срезки производного класса
 *
 */
TEST_F(HeadersFixture, ProviderSetGetTest){
	// Создаём исходный объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::POST, "/submit");
	// Устанавливаем объект провайдера в контейнер (копирование через клонирование)
	this->_headers->provider(&request);
	// Получаем объект провайдера из контейнера
	const provider_t * provider = this->_headers->provider();
	// Проверяем что провайдер установлен
	ASSERT_TRUE(provider != nullptr);
	// Проверяем что направление трафика соответствует запросу
	ASSERT_EQ(provider->direct, direct_t::REQUEST);
	// Проверяем что производная часть скопирована без срезки через стартовую строку
	ASSERT_EQ(this->_headers->startline(), "POST /submit HTTP/1.1");
	// Изменяем исходный объект запроса после установки провайдера
	request.uri = "/changed";
	// Проверяем что установленный провайдер является независимой копией
	ASSERT_EQ(this->_headers->startline(), "POST /submit HTTP/1.1");
	// Сбрасываем провайдер передачей нулевого указателя
	this->_headers->provider(static_cast <const provider_t *> (nullptr));
	// Проверяем что провайдер сброшен
	ASSERT_TRUE(this->_headers->provider() == nullptr);
}

/**
 * @brief Метод проверки установки провайдера через умный указатель и его извлечения
 *
 */
TEST_F(HeadersFixture, ProviderUniquePtrTest){
	// Создаём объект провайдера через умный указатель
	std::unique_ptr <provider_t> provider = std::make_unique <response_t> (version_t::HTTP1_1, 200, "OK");
	// Передаём владение объектом провайдера контейнеру
	this->_headers->provider(std::move(provider));
	// Проверяем что владение объектом передано
	ASSERT_TRUE(provider == nullptr);
	// Проверяем что провайдер установлен корректно через стартовую строку
	ASSERT_EQ(this->_headers->startline(), "HTTP/1.1 200 OK");
	// Извлекаем копию провайдера через выходной параметр
	std::unique_ptr <provider_t> extracted;
	// Проверяем результат извлечения копии провайдера
	ASSERT_TRUE(this->_headers->provider(extracted));
	// Проверяем что копия провайдера получена
	ASSERT_TRUE(extracted != nullptr);
	// Проверяем что направление трафика копии соответствует ответу
	ASSERT_EQ(extracted->direct, direct_t::RESPONSE);
	// Безопасно приводим копию провайдера к типу ответа (тип подтверждён флагом direct)
	const response_t * response = static_cast <const response_t *> (extracted.get());
	// Проверяем что производная часть скопирована без срезки
	ASSERT_EQ(response->code, 200u);
}

/**
 * @brief Метод проверки формирования стартовой строки запроса клиента
 *
 */
TEST_F(HeadersFixture, StartlineRequestGetTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, "/index.html");
	// Устанавливаем объект провайдера в контейнер
	this->_headers->provider(&request);
	// Проверяем формирование стартовой строки запроса
	ASSERT_EQ(this->_headers->startline(), "GET /index.html HTTP/1.1");
}

/**
 * @brief Метод проверки формирования стартовой строки ответа сервера
 *
 */
TEST_F(HeadersFixture, StartlineResponseGetTest){
	// Создаём объект ответа сервера без явного сообщения
	response_t response(static_cast <uint16_t> (200));
	// Устанавливаем объект провайдера в контейнер
	this->_headers->provider(&response);
	// Проверяем что стартовая строка использует стандартное сообщение по коду ответа
	ASSERT_EQ(this->_headers->startline(), "HTTP/1.1 200 OK");
	// Создаём объект ответа сервера с пользовательским сообщением
	response_t custom(version_t::HTTP1_1, 404, "Custom Not Found");
	// Устанавливаем объект провайдера в контейнер
	this->_headers->provider(&custom);
	// Проверяем что стартовая строка использует пользовательское сообщение
	ASSERT_EQ(this->_headers->startline(), "HTTP/1.1 404 Custom Not Found");
}

/**
 * @brief Метод проверки разбора стартовой строки запроса клиента
 *
 */
TEST_F(HeadersFixture, StartlineParseRequestTest){
	// Устанавливаем стартовую строку запроса
	this->_headers->startline("GET /index.html HTTP/1.1");
	// Получаем объект провайдера
	const provider_t * provider = this->_headers->provider();
	// Проверяем что провайдер сформирован
	ASSERT_TRUE(provider != nullptr);
	// Проверяем что направление трафика соответствует запросу
	ASSERT_EQ(provider->direct, direct_t::REQUEST);
	// Безопасно приводим провайдер к типу запроса (тип подтверждён флагом direct)
	const request_t * request = static_cast <const request_t *> (provider);
	// Проверяем что метод запроса разобран корректно
	ASSERT_EQ(request->method, method_t::GET);
	// Проверяем что URI-адрес разобран корректно
	ASSERT_EQ(request->uri, "/index.html");
	// Проверяем что версия протокола разобрана корректно
	ASSERT_EQ(request->version, version_t::HTTP1_1);
}

/**
 * @brief Метод проверки разбора стартовой строки ответа сервера
 *
 */
TEST_F(HeadersFixture, StartlineParseResponseTest){
	// Устанавливаем стартовую строку ответа
	this->_headers->startline("HTTP/1.1 404 Not Found");
	// Получаем объект провайдера
	const provider_t * provider = this->_headers->provider();
	// Проверяем что провайдер сформирован
	ASSERT_TRUE(provider != nullptr);
	// Проверяем что направление трафика соответствует ответу
	ASSERT_EQ(provider->direct, direct_t::RESPONSE);
	// Безопасно приводим провайдер к типу ответа (тип подтверждён флагом direct)
	const response_t * response = static_cast <const response_t *> (provider);
	// Проверяем что код ответа разобран корректно
	ASSERT_EQ(response->code, 404u);
	// Проверяем что сообщение сервера разобрано корректно
	ASSERT_EQ(response->message, "Not Found");
	// Проверяем что версия протокола разобрана корректно
	ASSERT_EQ(response->version, version_t::HTTP1_1);
}

/**
 * @brief Метод проверки сброса провайдера пустой стартовой строкой
 *
 */
TEST_F(HeadersFixture, StartlineEmptyResetsProviderTest){
	// Устанавливаем стартовую строку запроса
	this->_headers->startline("GET / HTTP/1.1");
	// Проверяем что провайдер установлен
	ASSERT_TRUE(this->_headers->provider() != nullptr);
	// Устанавливаем пустую стартовую строку
	this->_headers->startline("");
	// Проверяем что провайдер сброшен
	ASSERT_TRUE(this->_headers->provider() == nullptr);
}

/**
 * @brief Метод проверки печати заголовков в формате HTTP/1.1
 *
 */
TEST_F(HeadersFixture, PrintHttp1Test){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, "/index.html");
	// Устанавливаем объект провайдера в контейнер
	this->_headers->provider(&request);
	// Добавляем заголовок
	this->_headers->emplace("Host", "example.com");
	// Формируем текстовое представление заголовков в формате HTTP/1.1
	std::string output = this->_headers->print(proto_t::HTTP1);
	// Проверяем что стартовая строка присутствует в начале вывода
	ASSERT_EQ(output.compare(0, 24, "GET /index.html HTTP/1.1"), 0);
	// Проверяем что заголовок присутствует в выводе без изменения регистра
	ASSERT_NE(output.find("Host: example.com\r\n"), std::string::npos);
	// Проверяем что вывод завершается пустой строкой, отделяющей тело сообщения
	ASSERT_EQ(output.compare(output.size() - 4, 4, "\r\n\r\n"), 0);
}

/**
 * @brief Метод проверки печати заголовков в формате HTTP/2 с псевдозаголовками
 *
 */
TEST_F(HeadersFixture, PrintHttp2Test){
	// Создаём объект запроса клиента с URI-адресом в абсолютной форме
	request_t request(version_t::HTTP2, method_t::GET, "https://example.com/path");
	// Устанавливаем объект провайдера в контейнер
	this->_headers->provider(&request);
	// Добавляем заголовок в смешанном регистре
	this->_headers->emplace("Content-Type", "text/html");
	// Формируем текстовое представление заголовков в формате HTTP/2
	std::string output = this->_headers->print(proto_t::HTTP2);
	// Проверяем наличие псевдозаголовка метода запроса
	ASSERT_NE(output.find(":method: GET\r\n"), std::string::npos);
	// Проверяем наличие псевдозаголовка схемы, извлечённой из абсолютного URI
	ASSERT_NE(output.find(":scheme: https\r\n"), std::string::npos);
	// Проверяем наличие псевдозаголовка авторитета, извлечённого из абсолютного URI
	ASSERT_NE(output.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем наличие псевдозаголовка пути запроса, извлечённого из абсолютного URI
	ASSERT_NE(output.find(":path: /path\r\n"), std::string::npos);
	// Проверяем что название обычного заголовка приведено к нижнему регистру
	ASSERT_NE(output.find("content-type: text/html\r\n"), std::string::npos);
	// Проверяем что название обычного заголовка не осталось в исходном регистре
	ASSERT_EQ(output.find("Content-Type: text/html\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки печати конкретного заголовка по названию
 *
 */
TEST_F(HeadersFixture, PrintByNameTest){
	// Формируем набор заголовков с повторяющимся названием
	headers_t::fields_t data = {
		this->header("Set-Cookie", "a=1"),
		this->header("Set-Cookie", "b=2"),
		this->header("Host", "example.com")
	};
	// Присваиваем набор заголовков
	(* this->_headers) = data;
	// Формируем текстовое представление заголовков с указанным названием
	std::string output = this->_headers->print("Set-Cookie", proto_t::HTTP1);
	// Проверяем наличие первого значения заголовка
	ASSERT_NE(output.find("Set-Cookie: a=1\r\n"), std::string::npos);
	// Проверяем наличие второго значения заголовка
	ASSERT_NE(output.find("Set-Cookie: b=2\r\n"), std::string::npos);
	// Проверяем что заголовок с другим названием отсутствует в выводе
	ASSERT_EQ(output.find("Host"), std::string::npos);
}

/**
 * @brief Метод проверки обмена содержимым двух контейнеров
 *
 */
TEST_F(HeadersFixture, SwapTest){
	// Наполняем первый контейнер заголовками
	this->_headers->emplace("Host", "first.com");
	// Устанавливаем протокол первого контейнера
	this->_headers->proto(proto_t::HTTP1);
	// Создаём второй контейнер заголовков
	headers_t other(this->_fmk.get(), this->_log.get());
	// Наполняем второй контейнер заголовками
	other.emplace("Accept", "text/html");
	// Устанавливаем протокол второго контейнера
	other.proto(proto_t::HTTP2);
	// Выполняем обмен содержимым контейнеров
	this->_headers->swap(other);
	// Проверяем что первый контейнер получил заголовки второго
	ASSERT_TRUE(this->_headers->has("Accept"));
	// Проверяем что первый контейнер получил протокол второго
	ASSERT_EQ(this->_headers->proto(), proto_t::HTTP2);
	// Проверяем что второй контейнер получил заголовки первого
	ASSERT_TRUE(other.has("Host"));
	// Проверяем что второй контейнер получил протокол первого
	ASSERT_EQ(other.proto(), proto_t::HTTP1);
	// Проверяем что учёт памяти первого контейнера соответствует полученным заголовкам (6 + 9)
	ASSERT_EQ(this->_headers->memory(), 15u);
	// Проверяем что учёт памяти второго контейнера соответствует полученным заголовкам (4 + 9)
	ASSERT_EQ(other.memory(), 13u);
}

/**
 * @brief Метод проверки слияния заголовков
 *
 */
TEST_F(HeadersFixture, MergeTest){
	// Наполняем первый контейнер заголовками
	this->_headers->emplace("Host", "example.com");
	// Создаём второй контейнер заголовков
	headers_t other(this->_fmk.get(), this->_log.get());
	// Наполняем второй контейнер заголовками
	other.emplace("Accept", "text/html");
	// Добавляем во второй контейнер ещё один заголовок
	other.emplace("User-Agent", "awh");
	// Выполняем слияние заголовков через метод merge
	this->_headers->merge(other);
	// Проверяем что исходный заголовок сохранён
	ASSERT_TRUE(this->_headers->has("Host"));
	// Проверяем что заголовок из второго контейнера добавлен
	ASSERT_TRUE(this->_headers->has("Accept"));
	// Проверяем что второй заголовок из второго контейнера добавлен
	ASSERT_TRUE(this->_headers->has("User-Agent"));
	// Проверяем итоговое количество заголовков
	ASSERT_EQ(this->_headers->size(), 3u);
}

/**
 * @brief Метод проверки оператора слияния заголовков
 *
 */
TEST_F(HeadersFixture, MergeOperatorTest){
	// Наполняем первый контейнер заголовками
	this->_headers->emplace("Host", "example.com");
	// Создаём второй контейнер заголовков
	headers_t other(this->_fmk.get(), this->_log.get());
	// Наполняем второй контейнер заголовками
	other.emplace("Accept", "text/html");
	// Выполняем слияние заголовков через оператор
	(* this->_headers) += other;
	// Проверяем что исходный заголовок сохранён
	ASSERT_TRUE(this->_headers->has("Host"));
	// Проверяем что заголовок из второго контейнера добавлен
	ASSERT_TRUE(this->_headers->has("Accept"));
	// Проверяем итоговое количество заголовков
	ASSERT_EQ(this->_headers->size(), 2u);
}

/**
 * @brief Метод проверки копирования контейнера без срезки провайдера
 *
 */
TEST_F(HeadersFixture, CopyHeadersNoSlicingTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::POST, "/submit");
	// Устанавливаем объект провайдера в контейнер
	this->_headers->provider(&request);
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP1);
	// Наполняем контейнер заголовками
	this->_headers->emplace("Host", "example.com");
	// Копируем контейнер через конструктор копирования
	headers_t copy(* this->_headers);
	// Проверяем что стартовая строка скопированного контейнера сформирована без срезки провайдера
	ASSERT_EQ(copy.startline(), "POST /submit HTTP/1.1");
	// Проверяем что заголовки скопированы
	ASSERT_TRUE(copy.has("Host"));
	// Проверяем что протокол скопирован
	ASSERT_EQ(copy.proto(), proto_t::HTTP1);
	// Проверяем что контейнеры равны
	ASSERT_TRUE(copy == (* this->_headers));
	// Проверяем копирование через оператор присваивания
	headers_t assigned(this->_fmk.get(), this->_log.get());
	// Выполняем присваивание копированием
	assigned = (* this->_headers);
	// Проверяем что стартовая строка присвоенного контейнера сформирована без срезки провайдера
	ASSERT_EQ(assigned.startline(), "POST /submit HTTP/1.1");
}

/**
 * @brief Метод проверки перемещения контейнера заголовков
 *
 */
TEST_F(HeadersFixture, MoveHeadersTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, "/data");
	// Устанавливаем объект провайдера в контейнер
	this->_headers->provider(&request);
	// Наполняем контейнер заголовками
	this->_headers->emplace("Host", "example.com");
	// Перемещаем контейнер через конструктор перемещения
	headers_t moved(std::move(* this->_headers));
	// Проверяем что перемещённый контейнер содержит стартовую строку
	ASSERT_EQ(moved.startline(), "GET /data HTTP/1.1");
	// Проверяем что перемещённый контейнер содержит заголовки
	ASSERT_TRUE(moved.has("Host"));
	// Проверяем перемещение через оператор присваивания
	headers_t movedAssigned(this->_fmk.get(), this->_log.get());
	// Выполняем присваивание перемещением
	movedAssigned = std::move(moved);
	// Проверяем что перемещённый контейнер содержит заголовки
	ASSERT_TRUE(movedAssigned.has("Host"));
}

/**
 * @brief Метод проверки сравнения контейнеров с учётом кратности значений
 *
 */
TEST_F(HeadersFixture, EqualityMultiplicityTest){
	// Создаём первый контейнер заголовков
	headers_t headers1(this->_fmk.get(), this->_log.get());
	// Создаём второй контейнер заголовков
	headers_t headers2(this->_fmk.get(), this->_log.get());
	// Наполняем первый контейнер заголовками с разными значениями одного названия
	headers1 = headers_t::fields_t ({this->header("Set-Cookie", "a=1"), this->header("Set-Cookie", "b=2")});
	// Наполняем второй контейнер теми же заголовками, но в другом порядке
	headers2 = headers_t::fields_t ({this->header("Set-Cookie", "b=2"), this->header("Set-Cookie", "a=1")});
	/**
	 * Проверяем что контейнеры с переставленными одноимёнными заголовками не равны:
	 * порядок полей одного названия несёт смысл (RFC 9110 §5.3), и сообщения
	 * с переставленными Set-Cookie - разные сообщения
	 */
	ASSERT_TRUE(headers1 != headers2);
	// Наполняем второй контейнер теми же заголовками в исходном порядке
	headers2 = headers_t::fields_t ({this->header("Set-Cookie", "a=1"), this->header("Set-Cookie", "b=2")});
	// Проверяем что контейнеры с совпавшим порядком одноимённых заголовков равны
	ASSERT_TRUE(headers1 == headers2);
	// Наполняем первый контейнер заголовками с разными названиями
	headers1 = headers_t::fields_t ({this->header("Host", "example.com"), this->header("Accept", "text/html")});
	// Наполняем второй контейнер теми же заголовками в обратном порядке
	headers2 = headers_t::fields_t ({this->header("Accept", "text/html"), this->header("Host", "example.com")});
	// Проверяем что порядок заголовков с разными названиями на равенство не влияет
	ASSERT_TRUE(headers1 == headers2);
	// Наполняем первый контейнер заголовками с разными значениями одного названия
	headers1 = headers_t::fields_t ({this->header("Set-Cookie", "a=1"), this->header("Set-Cookie", "b=2")});
	// Наполняем второй контейнер заголовками с одинаковыми значениями (нарушаем кратность)
	headers2 = headers_t::fields_t ({this->header("Set-Cookie", "a=1"), this->header("Set-Cookie", "a=1")});
	// Проверяем что контейнеры с разной кратностью значений не равны
	ASSERT_TRUE(headers1 != headers2);
}

/**
 * @brief Метод проверки операторов сравнения контейнеров
 *
 */
TEST_F(HeadersFixture, EqualityTest){
	// Создаём первый контейнер заголовков
	headers_t headers1(this->_fmk.get(), this->_log.get());
	// Наполняем первый контейнер заголовками
	headers1.emplace("Host", "example.com");
	// Добавляем в первый контейнер ещё один заголовок
	headers1.emplace("Accept", "text/html");
	// Создаём второй контейнер заголовков
	headers_t headers2(this->_fmk.get(), this->_log.get());
	// Наполняем второй контейнер такими же заголовками
	headers2.emplace("Accept", "text/html");
	// Добавляем во второй контейнер ещё один заголовок
	headers2.emplace("Host", "example.com");
	// Проверяем что контейнеры с одинаковыми заголовками равны
	ASSERT_TRUE(headers1 == headers2);
	// Изменяем значение заголовка во втором контейнере
	headers2.emplace("Host", "other.com");
	// Проверяем что контейнеры с разными значениями не равны
	ASSERT_TRUE(headers1 != headers2);
	// Удаляем заголовок из второго контейнера
	headers2.erase("Accept");
	// Проверяем что контейнеры с разным количеством заголовков не равны
	ASSERT_TRUE(headers1 != headers2);
}

/**
 * @brief Метод проверки операторов преобразования контейнера
 *
 */
TEST_F(HeadersFixture, ConversionOperatorsTest){
	// Наполняем контейнер заголовками
	this->_headers->emplace("Host", "example.com");
	// Добавляем ещё один заголовок
	this->_headers->emplace("Accept", "text/html");
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP2);
	// Проверяем оператор преобразования в количество заголовков
	ASSERT_EQ(static_cast <size_t> (* this->_headers), 2u);
	// Проверяем оператор преобразования в протокол
	ASSERT_EQ(static_cast <proto_t> (* this->_headers), proto_t::HTTP2);
	// Проверяем оператор преобразования в строку
	std::string text = static_cast <std::string> (* this->_headers);
	// Проверяем что строковое представление содержит заголовок
	ASSERT_NE(text.find("example.com"), std::string::npos);
	// Проверяем оператор преобразования в список заголовков
	headers_t::fields_t vec = static_cast <headers_t::fields_t> (* this->_headers);
	// Проверяем количество заголовков в списке
	ASSERT_EQ(vec.size(), 2u);
	// Проверяем оператор преобразования в набор заголовков
	headers_t::entries_t entries = static_cast <headers_t::entries_t> (* this->_headers);
	// Проверяем количество заголовков в наборе
	ASSERT_EQ(entries.size(), 2u);
	// Проверяем оператор преобразования в карту заголовков
	headers_t::map_t map = static_cast <headers_t::map_t> (* this->_headers);
	// Проверяем количество заголовков в карте
	ASSERT_EQ(map.size(), 2u);
	// Проверяем оператор преобразования в мультикарту заголовков
	headers_t::multimap_t multimap = static_cast <headers_t::multimap_t> (* this->_headers);
	// Проверяем количество заголовков в мультикарте
	ASSERT_EQ(multimap.size(), 2u);
}

/**
 * @brief Метод проверки оператора преобразования контейнера в провайдер
 *
 */
TEST_F(HeadersFixture, ConversionProviderTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, "/data");
	// Устанавливаем объект провайдера в контейнер
	this->_headers->provider(&request);
	// Проверяем оператор преобразования в константный указатель провайдера
	const provider_t * provider = static_cast <const provider_t *> (* this->_headers);
	// Проверяем что провайдер получен
	ASSERT_TRUE(provider != nullptr);
	// Проверяем что направление трафика соответствует запросу
	ASSERT_EQ(provider->direct, direct_t::REQUEST);
	// Проверяем оператор преобразования в умный указатель провайдера
	std::unique_ptr <provider_t> cloned = static_cast <std::unique_ptr <provider_t>> (* this->_headers);
	// Проверяем что копия провайдера получена
	ASSERT_TRUE(cloned != nullptr);
	// Безопасно приводим копию провайдера к типу запроса (тип подтверждён флагом direct)
	const request_t * clonedPtr = static_cast <const request_t *> (cloned.get());
	// Проверяем что производная часть склонирована без срезки
	ASSERT_EQ(clonedPtr->uri, "/data");
}

/**
 * @brief Метод проверки автоматического определения протокола при присваивании
 *
 */
TEST_F(HeadersFixture, DetectProtoTest){
	// Наполняем контейнер обычными заголовками
	(* this->_headers) = headers_t::fields_t ({this->header("Host", "example.com")});
	// Проверяем что определён протокол HTTP/1.1
	ASSERT_EQ(this->_headers->proto(), proto_t::HTTP1);
	// Наполняем контейнер заголовками, содержащими псевдозаголовок HTTP/2
	(* this->_headers) = headers_t::fields_t ({this->header(":method", "GET"), this->header("Host", "example.com")});
	// Проверяем что определён протокол HTTP/2
	ASSERT_EQ(this->_headers->proto(), proto_t::HTTP2);
}

/**
 * @brief Метод проверки присваивания контейнера из мультикарты заголовков
 *
 */
TEST_F(HeadersFixture, AssignMultimapTest){
	// Формируем мультикарту заголовков с повторяющимся названием
	headers_t::multimap_t data = {
		{"Set-Cookie", "a=1"},
		{"Set-Cookie", "b=2"},
		{"Host", "example.com"}
	};
	// Присваиваем контейнеру мультикарту заголовков
	(* this->_headers) = data;
	// Проверяем что все дубликаты сохранены
	ASSERT_EQ(this->_headers->count("Set-Cookie"), 2u);
	// Проверяем что уникальный заголовок присутствует
	ASSERT_TRUE(this->_headers->has("Host"));
	// Проверяем итоговое количество заголовков
	ASSERT_EQ(this->_headers->size(), 3u);
}

/**
 * @brief Метод проверки итерирования по заголовкам
 *
 */
TEST_F(HeadersFixture, IterationTest){
	// Наполняем контейнер заголовками
	this->_headers->emplace("Host", "example.com");
	// Добавляем ещё один заголовок
	this->_headers->emplace("Accept", "text/html");
	// Добавляем ещё один заголовок
	this->_headers->emplace("User-Agent", "awh");
	// Счётчик пройденных заголовков
	size_t counter = 0;
	/**
	 * Обходим все заголовки контейнера через итераторы
	 */
	for(headers_t::iterator_t it = this->_headers->begin(); it != this->_headers->end(); ++it){
		// Проверяем что название текущего заголовка не пустое
		ASSERT_FALSE(it->name.empty());
		// Проверяем что значение текущего заголовка не пустое
		ASSERT_FALSE(it->value.empty());
		// Увеличиваем счётчик пройденных заголовков
		counter++;
	}
	// Проверяем что количество пройденных заголовков совпадает с размером контейнера
	ASSERT_EQ(counter, 3u);
}

/**
 * @brief Метод проверки оператора вывода контейнера в поток
 *
 */
TEST_F(HeadersFixture, OstreamOperatorTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, "/index.html");
	// Устанавливаем объект провайдера в контейнер
	this->_headers->provider(&request);
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP1);
	// Наполняем контейнер заголовками
	this->_headers->emplace("Host", "example.com");
	// Формируем строковый поток для вывода
	std::stringstream ss;
	// Выводим контейнер в поток через оператор вывода (оператор объявлен в пространстве имён awh)
	awh::operator << (ss, * this->_headers);
	// Получаем результат вывода
	std::string output = ss.str();
	// Проверяем что вывод содержит стартовую строку
	ASSERT_NE(output.find("GET /index.html HTTP/1.1"), std::string::npos);
	// Проверяем что вывод содержит заголовок
	ASSERT_NE(output.find("Host: example.com"), std::string::npos);
}

/**
 * @brief Метод проверки конструкторов контейнера с параметрами инициализации
 *
 */
TEST_F(HeadersFixture, ConstructWithDataTest){
	// Создаём контейнер из списка заголовков
	headers_t fromVector(headers_t::fields_t ({this->header("Host", "example.com")}), this->_fmk.get(), this->_log.get());
	// Проверяем что заголовок добавлен
	ASSERT_TRUE(fromVector.has("Host"));
	// Создаём объект провайдера через умный указатель
	std::unique_ptr <provider_t> provider = std::make_unique <request_t> (version_t::HTTP1_1, method_t::GET, "/data");
	// Создаём контейнер из протокола, провайдера и списка заголовков
	headers_t full(proto_t::HTTP1, std::move(provider), headers_t::fields_t ({this->header("Accept", "text/html")}), this->_fmk.get(), this->_log.get());
	// Проверяем что протокол установлен
	ASSERT_EQ(full.proto(), proto_t::HTTP1);
	// Проверяем что провайдер установлен через стартовую строку
	ASSERT_EQ(full.startline(), "GET /data HTTP/1.1");
	// Проверяем что заголовок добавлен
	ASSERT_TRUE(full.has("Accept"));
}

/**
 * @brief Метод проверки установки и получения идентификации сервиса для запроса
 *
 */
TEST_F(HeadersFixture, IdentRequestTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, "/");
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Устанавливаем идентификацию сервиса
	this->_headers->ident("MyID", "MyApp", "1.2.3");
	// Получаем сформированный User-Agent
	const std::string agent = this->_headers->ident();
	// Проверяем что название сервиса присутствует в начале агента
	ASSERT_EQ(agent.find("MyApp ("), 0u);
	// Проверяем что идентификатор и версия присутствуют в агенте
	ASSERT_NE(agent.find("MyID/1.2.3)"), std::string::npos);
	// Проверяем формат агента целиком: «Name (OS; ID/Version)»
	ASSERT_TRUE(std::regex_match(agent, std::regex("^MyApp \\(.+; MyID/1\\.2\\.3\\)$")));
}

/**
 * @brief Метод проверки установки и получения идентификации сервиса для ответа
 *
 */
TEST_F(HeadersFixture, IdentResponseTest){
	// Создаём объект ответа сервера
	response_t response(version_t::HTTP1_1, 200, "OK");
	// Устанавливаем провайдер ответа
	this->_headers->provider(&response);
	// Устанавливаем идентификацию сервиса
	this->_headers->ident("MyID", "MyApp", "1.2.3");
	// Проверяем формат X-Powered-By: «ID/Version»
	ASSERT_EQ(this->_headers->ident(), "MyID/1.2.3");
}

/**
 * @brief Метод проверки идентификации сервиса по умолчанию
 *
 */
TEST_F(HeadersFixture, IdentDefaultTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, "/");
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Получаем идентификацию по умолчанию
	const std::string agent = this->_headers->ident();
	// Проверяем что название сервиса по умолчанию присутствует
	ASSERT_EQ(agent.find(AWH_NAME " ("), 0u);
	// Проверяем что короткий идентификатор и версия по умолчанию присутствуют
	ASSERT_NE(agent.find(AWH_SHORT_NAME "/" AWH_VERSION ")"), std::string::npos);
	// Создаём объект ответа сервера
	response_t response(version_t::HTTP1_1, 200, "OK");
	// Устанавливаем провайдер ответа
	this->_headers->provider(&response);
	// Проверяем идентификацию ответа по умолчанию
	ASSERT_EQ(this->_headers->ident(), AWH_SHORT_NAME "/" AWH_VERSION);
}

/**
 * @brief Метод проверки что пустые аргументы не перезаписывают идентификацию
 *
 */
TEST_F(HeadersFixture, IdentEmptyArgsPreserveTest){
	// Создаём объект ответа сервера
	response_t response(version_t::HTTP1_1, 200, "OK");
	// Устанавливаем провайдер ответа
	this->_headers->provider(&response);
	// Устанавливаем исходную идентификацию сервиса
	this->_headers->ident("KeepID", "KeepName", "9.9.9");
	// Передаём пустые аргументы - значения не должны измениться
	this->_headers->ident("", "", "");
	// Проверяем что идентификация сохранена
	ASSERT_EQ(this->_headers->ident(), "KeepID/9.9.9");
	// Частично обновляем только версию
	this->_headers->ident("", "", "2.0.0");
	// Проверяем что обновилась только версия
	ASSERT_EQ(this->_headers->ident(), "KeepID/2.0.0");
}

/**
 * @brief Метод проверки форматирования HTTP-даты
 *
 */
TEST_F(HeadersFixture, DateFormatTest){
	// Формируем дату для известного Unix Timestamp (1 января 2021 00:00:00 GMT)
	ASSERT_EQ(this->_headers->date(1609459200ull), "Fri, 01 Jan 2021 00:00:00 GMT");
	// Формируем дату для известного Unix Timestamp (18 декабря 2013 12:00:00 GMT)
	ASSERT_EQ(this->_headers->date(1387368000ull), "Wed, 18 Dec 2013 12:00:00 GMT");
	// Формируем дату из миллисекунд - значение должно быть нормализовано до секунд
	ASSERT_EQ(this->_headers->date(1609459200000ull), "Fri, 01 Jan 2021 00:00:00 GMT");
}

/**
 * @brief Метод проверки независимости HTTP-даты от местности приложения
 *
 */
TEST_F(HeadersFixture, DateLocaleIndependenceTest){
	// Запоминаем действующую общую местность приложения
	const std::locale current = std::locale();
	// Количество проверенных местностей с иными названиями дня и месяца
	uint32_t checked = 0;
	/**
	 * Выполняем перебор названий местности с иными названиями дня и месяца
	 *
	 * @note Местность приложение задаёт вызовом std::locale::global, и поток берёт её
	 *       именно оттуда: под «de_DE» вывод put_time становится немецким - «Do., 06
	 *       Aug. 2026», - тогда как RFC 9110 §5.6.7 требует английских сокращений
	 */
	for(const char * name : {"de_DE.UTF-8", "de_DE.utf8", "ru_RU.UTF-8", "German_Germany"}){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Выполняем установку общей местности приложения
			std::locale::global(std::locale(name));
		/**
		 * Если местности в системе нет
		 */
		} catch(const std::exception &) {
			// Выполняем переход к следующей местности
			continue;
		}
		// Выполняем проверку записи даты английскими сокращениями
		ASSERT_EQ(this->_headers->date(1609459200ull), "Fri, 01 Jan 2021 00:00:00 GMT") << name;
		// Выполняем проверку записи даты иной отметки времени
		ASSERT_EQ(this->_headers->date(1387368000ull), "Wed, 18 Dec 2013 12:00:00 GMT") << name;
		// Выполняем учёт проверенной местности
		checked++;
	}
	// Выполняем возврат действующей общей местности приложения
	std::locale::global(current);
	// Если ни одной местности с иными названиями дня и месяца в системе не нашлось
	if(checked == 0)
		// Выполняем пропуск проверки
		GTEST_SKIP() << "no locale with foreign day and month names is available";
}

/**
 * @brief Метод проверки форматирования текущей HTTP-даты
 *
 */
TEST_F(HeadersFixture, DateNowTest){
	// Получаем текущую дату в HTTP-формате
	const std::string now = this->_headers->date();
	// Проверяем что дата не пустая
	ASSERT_FALSE(now.empty());
	// Проверяем соответствие формату RFC 9110 HTTP-date
	ASSERT_TRUE(std::regex_match(now, std::regex("^[A-Z][a-z]{2}, \\d{2} [A-Z][a-z]{2} \\d{4} \\d{2}:\\d{2}:\\d{2} GMT$")));
}

/**
 * @brief Метод проверки всех форм построения контейнера заголовков
 *
 * @details Контейнер строится комбинацией трёх независимых признаков: источник стартовой
 *          строки (нет, протокол, провайдер во владении вызывающей стороны, провайдер во
 *          владении контейнера), форма набора заголовков (нет, вектор, набор, отображение,
 *          список инициализации) и наличие объектов фреймворка и логирования. Комбинаций
 *          получается несколько десятков, и каждая является отдельной точкой входа данных
 *          в объект: набор, попавший внутрь мимо проверок одной из них, отличался бы от
 *          набора, попавшего через остальные
 *
 */
TEST_F(HeadersFixture, ConstructionFormsTest){
	// Создаём объект фреймворка
	awh::fmk_t fmk;
	// Создаём объект логирования
	awh::log_t log(&fmk);
	// Отключаем вывод сообщений контейнера
	log.level(awh::log_t::level_t::NONE);
	// Формируем заголовок узла назначения
	const headers_t::header_t host = headers_t::header_t().from("Host", "anyks.com");
	// Формируем заголовок принимаемых типов содержимого
	const headers_t::header_t accept = headers_t::header_t().from("Accept", "*/*");
	// Формируем набор заголовков вектором
	const headers_t::fields_t fields = {host, accept};
	// Формируем набор заголовков набором
	const headers_t::entries_t entries(fields.begin(), fields.end());
	// Формируем набор заголовков отображением
	const headers_t::multimap_t mapping = {{"Host", "anyks.com"}, {"Accept", "*/*"}};
	/**
	 * @brief Функция проверки построенного контейнера заголовков
	 *
	 * @param headers проверяемый контейнер заголовков
	 * @param filled  признак ожидания заполненного набора заголовков
	 * @param details описание проверяемой формы построения
	 *
	 */
	auto check = [](const headers_t & headers, const bool filled, const char * details) -> void {
		// Проверяем количество заголовков построенного контейнера
		ASSERT_EQ(headers.size(), (filled ? 2u : 0u)) << details;
		// Если набор заголовков ожидается заполненным
		if(filled){
			// Проверяем что заголовок Host попал в контейнер
			ASSERT_TRUE(headers.has("Host")) << details;
			// Проверяем что заголовок Accept попал в контейнер
			ASSERT_TRUE(headers.has("Accept")) << details;
		}
	};
	/**
	 * @brief Функция создания провайдера запроса клиента
	 *
	 * @return созданный провайдер запроса клиента
	 *
	 */
	auto provider = []() -> std::unique_ptr <request_t> {
		// Выводим созданный провайдер запроса клиента
		return std::make_unique <request_t> (version_t::HTTP1_1, method_t::GET, std::string("/index.html"));
	};
	// Создаём провайдер запроса, остающийся во владении вызывающей стороны
	const std::unique_ptr <request_t> external = provider();
	/**
	 * Проверяем формы построения без объектов фреймворка и логирования
	 */
	{
		// Проверяем построение пустого контейнера
		check(headers_t(), false, "пустой контейнер");
		// Проверяем построение от протокола
		check(headers_t(proto_t::HTTP1), false, "протокол");
		// Проверяем построение от провайдера вызывающей стороны
		check(headers_t(external.get()), false, "провайдер вызывающей стороны");
		// Проверяем построение от провайдера во владении контейнера
		check(headers_t(provider()), false, "провайдер во владении");
		// Проверяем построение от вектора заголовков
		check(headers_t(fields), true, "вектор");
		// Проверяем построение от набора заголовков
		check(headers_t(entries), true, "набор");
		// Проверяем построение от отображения заголовков
		check(headers_t(mapping), true, "отображение");
		// Проверяем построение от списка инициализации
		check(headers_t({host, accept}), true, "список инициализации");
	}
	/**
	 * Проверяем формы построения от протокола с набором заголовков
	 */
	{
		// Проверяем построение от протокола и вектора
		check(headers_t(proto_t::HTTP1, fields), true, "протокол и вектор");
		// Проверяем построение от протокола и набора
		check(headers_t(proto_t::HTTP1, entries), true, "протокол и набор");
		// Проверяем построение от протокола и отображения
		check(headers_t(proto_t::HTTP1, mapping), true, "протокол и отображение");
		// Проверяем построение от протокола и списка инициализации
		check(headers_t(proto_t::HTTP1, {host, accept}), true, "протокол и список");
	}
	/**
	 * Проверяем формы построения от провайдера с набором заголовков
	 */
	{
		// Проверяем построение от провайдера вызывающей стороны и вектора
		check(headers_t(external.get(), fields), true, "провайдер и вектор");
		// Проверяем построение от провайдера вызывающей стороны и набора
		check(headers_t(external.get(), entries), true, "провайдер и набор");
		// Проверяем построение от провайдера вызывающей стороны и отображения
		check(headers_t(external.get(), mapping), true, "провайдер и отображение");
		// Проверяем построение от провайдера вызывающей стороны и списка инициализации
		check(headers_t(external.get(), {host, accept}), true, "провайдер и список");
		// Проверяем построение от провайдера во владении и вектора
		check(headers_t(provider(), fields), true, "владение и вектор");
		// Проверяем построение от провайдера во владении и набора
		check(headers_t(provider(), entries), true, "владение и набор");
		// Проверяем построение от провайдера во владении и отображения
		check(headers_t(provider(), mapping), true, "владение и отображение");
		// Проверяем построение от провайдера во владении и списка инициализации
		check(headers_t(provider(), {host, accept}), true, "владение и список");
	}
	/**
	 * Проверяем формы построения от протокола и провайдера с набором заголовков
	 */
	{
		// Проверяем построение от протокола, провайдера вызывающей стороны и вектора
		check(headers_t(proto_t::HTTP1, external.get(), fields), true, "протокол, провайдер и вектор");
		// Проверяем построение от протокола, провайдера вызывающей стороны и набора
		check(headers_t(proto_t::HTTP1, external.get(), entries), true, "протокол, провайдер и набор");
		// Проверяем построение от протокола, провайдера вызывающей стороны и отображения
		check(headers_t(proto_t::HTTP1, external.get(), mapping), true, "протокол, провайдер и отображение");
		// Проверяем построение от протокола, провайдера вызывающей стороны и списка
		check(headers_t(proto_t::HTTP1, external.get(), {host, accept}), true, "протокол, провайдер и список");
		// Проверяем построение от протокола, провайдера во владении и вектора
		check(headers_t(proto_t::HTTP1, provider(), fields), true, "протокол, владение и вектор");
		// Проверяем построение от протокола, провайдера во владении и набора
		check(headers_t(proto_t::HTTP1, provider(), entries), true, "протокол, владение и набор");
		// Проверяем построение от протокола, провайдера во владении и отображения
		check(headers_t(proto_t::HTTP1, provider(), mapping), true, "протокол, владение и отображение");
		// Проверяем построение от протокола, провайдера во владении и списка
		check(headers_t(proto_t::HTTP1, provider(), {host, accept}), true, "протокол, владение и список");
	}
	/**
	 * Проверяем те же формы построения с объектами фреймворка и логирования
	 */
	{
		// Проверяем построение с объектами от фреймворка и логирования
		check(headers_t(&fmk, &log), false, "объекты");
		// Проверяем построение с объектами от протокола
		check(headers_t(proto_t::HTTP1, &fmk, &log), false, "объекты и протокол");
		// Проверяем построение с объектами от провайдера вызывающей стороны
		check(headers_t(external.get(), &fmk, &log), false, "объекты и провайдер");
		// Проверяем построение с объектами от провайдера во владении
		check(headers_t(provider(), &fmk, &log), false, "объекты и владение");
		// Проверяем построение с объектами от вектора
		check(headers_t(fields, &fmk, &log), true, "объекты и вектор");
		// Проверяем построение с объектами от набора
		check(headers_t(entries, &fmk, &log), true, "объекты и набор");
		// Проверяем построение с объектами от отображения
		check(headers_t(mapping, &fmk, &log), true, "объекты и отображение");
		// Проверяем построение с объектами от списка инициализации
		check(headers_t({host, accept}, &fmk, &log), true, "объекты и список");
		// Проверяем построение с объектами от протокола и вектора
		check(headers_t(proto_t::HTTP1, fields, &fmk, &log), true, "объекты, протокол и вектор");
		// Проверяем построение с объектами от протокола и набора
		check(headers_t(proto_t::HTTP1, entries, &fmk, &log), true, "объекты, протокол и набор");
		// Проверяем построение с объектами от протокола и отображения
		check(headers_t(proto_t::HTTP1, mapping, &fmk, &log), true, "объекты, протокол и отображение");
		// Проверяем построение с объектами от протокола и списка
		check(headers_t(proto_t::HTTP1, {host, accept}, &fmk, &log), true, "объекты, протокол и список");
		// Проверяем построение с объектами от провайдера вызывающей стороны и вектора
		check(headers_t(external.get(), fields, &fmk, &log), true, "объекты, провайдер и вектор");
		// Проверяем построение с объектами от провайдера вызывающей стороны и набора
		check(headers_t(external.get(), entries, &fmk, &log), true, "объекты, провайдер и набор");
		// Проверяем построение с объектами от провайдера вызывающей стороны и отображения
		check(headers_t(external.get(), mapping, &fmk, &log), true, "объекты, провайдер и отображение");
		// Проверяем построение с объектами от провайдера вызывающей стороны и списка
		check(headers_t(external.get(), {host, accept}, &fmk, &log), true, "объекты, провайдер и список");
		// Проверяем построение с объектами от провайдера во владении и вектора
		check(headers_t(provider(), fields, &fmk, &log), true, "объекты, владение и вектор");
		// Проверяем построение с объектами от провайдера во владении и набора
		check(headers_t(provider(), entries, &fmk, &log), true, "объекты, владение и набор");
		// Проверяем построение с объектами от провайдера во владении и отображения
		check(headers_t(provider(), mapping, &fmk, &log), true, "объекты, владение и отображение");
		// Проверяем построение с объектами от провайдера во владении и списка
		check(headers_t(provider(), {host, accept}, &fmk, &log), true, "объекты, владение и список");
		// Проверяем построение с объектами от протокола, провайдера вызывающей стороны и вектора
		check(headers_t(proto_t::HTTP1, external.get(), fields, &fmk, &log), true, "объекты, протокол, провайдер и вектор");
		// Проверяем построение с объектами от протокола, провайдера вызывающей стороны и набора
		check(headers_t(proto_t::HTTP1, external.get(), entries, &fmk, &log), true, "объекты, протокол, провайдер и набор");
		// Проверяем построение с объектами от протокола, провайдера вызывающей стороны и отображения
		check(headers_t(proto_t::HTTP1, external.get(), mapping, &fmk, &log), true, "объекты, протокол, провайдер и отображение");
		// Проверяем построение с объектами от протокола, провайдера вызывающей стороны и списка
		check(headers_t(proto_t::HTTP1, external.get(), {host, accept}, &fmk, &log), true, "объекты, протокол, провайдер и список");
		// Проверяем построение с объектами от протокола, провайдера во владении и вектора
		check(headers_t(proto_t::HTTP1, provider(), fields, &fmk, &log), true, "объекты, протокол, владение и вектор");
		// Проверяем построение с объектами от протокола, провайдера во владении и набора
		check(headers_t(proto_t::HTTP1, provider(), entries, &fmk, &log), true, "объекты, протокол, владение и набор");
		// Проверяем построение с объектами от протокола, провайдера во владении и отображения
		check(headers_t(proto_t::HTTP1, provider(), mapping, &fmk, &log), true, "объекты, протокол, владение и отображение");
		// Проверяем построение с объектами от протокола, провайдера во владении и списка
		check(headers_t(proto_t::HTTP1, provider(), {host, accept}, &fmk, &log), true, "объекты, протокол, владение и список");
	}
}

/**
 * @brief Метод проверки разбора стартовой строки контейнером заголовков
 *
 * @details Стартовая строка задаёт направление сообщения, версию протокола, метод либо
 *          код состояния - то есть всё, от чего зависит трактовка остального сообщения.
 *          Строка приходит текстом извне, поэтому проверяются и пригодные формы, и
 *          непригодные: непригодная обязана оставить контейнер без провайдера, а не
 *          собрать провайдер наполовину
 *
 */
TEST_F(HeadersFixture, StartlineParsingTest){
	/**
	 * Проверяем разбор стартовой строки запроса клиента
	 */
	{
		// Создаём контейнер заголовков
		headers_t headers;
		// Устанавливаем стартовую строку запроса клиента
		headers.startline("GET /index.html HTTP/1.1");
		// Проверяем что провайдер запроса создан
		ASSERT_TRUE(headers.provider() != nullptr);
		// Проверяем что направление трафика определено как запрос
		ASSERT_EQ(headers.provider()->direct, direct_t::REQUEST);
		// Проверяем что стартовая строка восстанавливается в исходном виде
		ASSERT_EQ(headers.startline(), "GET /index.html HTTP/1.1");
	}
	/**
	 * Проверяем разбор строки состояния ответа сервера
	 */
	{
		// Создаём контейнер заголовков
		headers_t headers;
		// Устанавливаем строку состояния ответа сервера
		headers.startline("HTTP/1.1 404 Not Found");
		// Проверяем что провайдер ответа создан
		ASSERT_TRUE(headers.provider() != nullptr);
		// Проверяем что направление трафика определено как ответ
		ASSERT_EQ(headers.provider()->direct, direct_t::RESPONSE);
		// Проверяем что стартовая строка восстанавливается в исходном виде
		ASSERT_EQ(headers.startline(), "HTTP/1.1 404 Not Found");
	}
	/**
	 * Проверяем разбор строки состояния ответа без текста состояния
	 */
	{
		// Создаём контейнер заголовков
		headers_t headers;
		// Устанавливаем строку состояния ответа без текста состояния
		headers.startline("HTTP/1.0 204");
		// Проверяем что провайдер ответа создан
		ASSERT_TRUE(headers.provider() != nullptr);
		// Проверяем что направление трафика определено как ответ
		ASSERT_EQ(headers.provider()->direct, direct_t::RESPONSE);
	}
	/**
	 * Проверяем сброс провайдера пустой стартовой строкой
	 */
	{
		// Создаём контейнер заголовков со стартовой строкой запроса
		headers_t headers;
		// Устанавливаем стартовую строку запроса клиента
		headers.startline("GET / HTTP/1.1");
		// Сбрасываем стартовую строку пустым значением
		headers.startline("");
		// Проверяем что провайдер сброшен
		ASSERT_TRUE(headers.provider() == nullptr);
	}
	/**
	 * Проверяем что непригодные стартовые строки провайдера не создают
	 */
	{
		// Формируем перечень непригодных стартовых строк
		const std::vector <std::string> broken = {"GET", "GET ", "HTTP/1.1", "   "};
		/**
		 * Выполняем перебор всех непригодных стартовых строк
		 */
		for(auto & startline : broken){
			// Создаём контейнер заголовков
			headers_t headers;
			// Устанавливаем непригодную стартовую строку
			headers.startline(startline);
			// Проверяем что провайдер не создан
			ASSERT_TRUE(headers.provider() == nullptr) << ("стартовая строка: [" + startline + "]");
		}
	}
	/**
	 * Проверяем разбор версий протокола всех поддерживаемых семейств
	 */
	{
		// Формируем перечень строк состояния разных версий протокола
		const std::vector <std::string> versions = {"HTTP/1.0 200 OK", "HTTP/1.1 200 OK", "HTTP/2 200 OK", "HTTP/3 200 OK"};
		/**
		 * Выполняем перебор всех строк состояния
		 */
		for(auto & startline : versions){
			// Создаём контейнер заголовков
			headers_t headers;
			// Устанавливаем строку состояния ответа сервера
			headers.startline(startline);
			// Проверяем что провайдер ответа создан
			ASSERT_TRUE(headers.provider() != nullptr) << ("стартовая строка: [" + startline + "]");
		}
	}
}

/**
 * @brief Метод проверки преобразования контейнера заголовков в наборы
 *
 * @details Контейнер отдаёт свой набор тремя формами, и каждая является отдельным
 *          обходом внутреннего хранилища: расхождение между ними означало бы, что
 *          вызывающая сторона получает разный набор в зависимости от того, каким
 *          способом она его запросила
 *
 */
TEST_F(HeadersFixture, ConversionFormsTest){
	// Формируем заголовок узла назначения
	const headers_t::header_t host = headers_t::header_t().from("Host", "anyks.com");
	// Формируем заголовок принимаемых типов содержимого
	const headers_t::header_t accept = headers_t::header_t().from("Accept", "*/*");
	// Создаём контейнер заголовков с набором
	const headers_t headers({host, accept});
	// Выполняем преобразование контейнера в вектор заголовков
	const headers_t::fields_t fields = static_cast <headers_t::fields_t> (headers);
	// Выполняем преобразование контейнера в набор заголовков
	const headers_t::entries_t entries = static_cast <headers_t::entries_t> (headers);
	// Выполняем преобразование контейнера в отображение заголовков
	const headers_t::multimap_t mapping = static_cast <headers_t::multimap_t> (headers);
	// Проверяем количество заголовков в векторе
	ASSERT_EQ(fields.size(), 2u);
	// Проверяем количество заголовков в наборе
	ASSERT_EQ(entries.size(), 2u);
	// Проверяем количество заголовков в отображении
	ASSERT_EQ(mapping.size(), 2u);
	// Проверяем что заголовок узла назначения присутствует в наборе
	ASSERT_EQ(entries.count(host), 1u);
	// Проверяем что заголовок узла назначения присутствует в отображении
	ASSERT_EQ(mapping.count("Host"), 1u);
	// Выполняем преобразование пустого контейнера в вектор заголовков
	const headers_t::fields_t empty = static_cast <headers_t::fields_t> (headers_t());
	// Проверяем что преобразование пустого контейнера даёт пустой набор
	ASSERT_TRUE(empty.empty());
}

/**
 * @brief Метод проверки всех форм присваивания контейнеру заголовков
 *
 * @details Присваивание является той же точкой входа данных, что и построение, но
 *          применяется к уже наполненному объекту: прежний набор обязан быть замещён
 *          целиком, а не дополнен - иначе заголовки предыдущего сообщения ушли бы
 *          на провод вместе со следующим
 *
 */
TEST_F(HeadersFixture, AssignmentFormsTest){
	// Формируем заголовок узла назначения
	const headers_t::header_t host = headers_t::header_t().from("Host", "anyks.com");
	// Формируем заголовок принимаемых типов содержимого
	const headers_t::header_t accept = headers_t::header_t().from("Accept", "*/*");
	// Формируем набор заголовков вектором
	const headers_t::fields_t fields = {host, accept};
	// Формируем набор заголовков набором
	const headers_t::entries_t entries(fields.begin(), fields.end());
	// Формируем набор заголовков отображением
	const headers_t::multimap_t mapping = {{"Host", "anyks.com"}, {"Accept", "*/*"}};
	// Создаём провайдер запроса, остающийся во владении вызывающей стороны
	const std::unique_ptr <request_t> external = std::make_unique <request_t> (version_t::HTTP1_1, method_t::GET, std::string("/index.html"));
	// Формируем заголовок прежнего набора
	const headers_t::header_t previous = headers_t::header_t().from("X-Previous", "value");
	/**
	 * @brief Функция проверки присвоенного набора заголовков
	 *
	 * @param headers проверяемый контейнер заголовков
	 * @param details описание проверяемой формы присваивания
	 *
	 */
	auto check = [](const headers_t & headers, const char * details) -> void {
		// Проверяем что прежний набор замещён целиком
		ASSERT_EQ(headers.size(), 2u) << details;
		// Проверяем что заголовок Host попал в контейнер
		ASSERT_TRUE(headers.has("Host")) << details;
		// Проверяем что заголовок предыдущего набора вычищен
		ASSERT_FALSE(headers.has("X-Previous")) << details;
	};
	/**
	 * Выполняем перебор всех проверяемых форм присваивания
	 */
	{
		// Создаём контейнер с прежним набором заголовков
		headers_t headers({previous});
		// Выполняем присваивание вектора заголовков
		headers = fields;
		// Проверяем результат присваивания вектора
		check(headers, "вектор");
	}
	{
		// Создаём контейнер с прежним набором заголовков
		headers_t headers({previous});
		// Выполняем присваивание набора заголовков
		headers = entries;
		// Проверяем результат присваивания набора
		check(headers, "набор");
	}
	{
		// Создаём контейнер с прежним набором заголовков
		headers_t headers({previous});
		// Выполняем присваивание отображения заголовков
		headers = mapping;
		// Проверяем результат присваивания отображения
		check(headers, "отображение");
	}
	{
		// Создаём контейнер с прежним набором заголовков
		headers_t headers({previous});
		// Выполняем присваивание списка инициализации
		headers = {host, accept};
		// Проверяем результат присваивания списка инициализации
		check(headers, "список инициализации");
	}
	{
		// Создаём контейнер с набором заголовков
		headers_t headers(fields);
		// Выполняем присваивание провайдера вызывающей стороны
		headers = external.get();
		// Проверяем что присвоение провайдера набор заголовков не тронуло
		ASSERT_EQ(headers.size(), 2u);
		// Проверяем что провайдер установлен
		ASSERT_TRUE(headers.provider() != nullptr);
	}
	{
		// Создаём контейнер с набором заголовков
		headers_t headers(fields);
		// Выполняем присваивание провайдера во владение контейнера
		headers = std::make_unique <request_t> (version_t::HTTP1_1, method_t::GET, std::string("/index.html"));
		// Проверяем что присвоение провайдера набор заголовков не тронуло
		ASSERT_EQ(headers.size(), 2u);
		// Проверяем что провайдер установлен
		ASSERT_TRUE(headers.provider() != nullptr);
	}
}

/**
 * @brief Метод проверки работы контейнера без объектов фреймворка и логирования
 *
 * @details Контейнер создаётся конструктором от протокола, и оба объекта в этом случае
 *          остаются пустыми - это штатный путь построения, а не вырожденный случай.
 *          Вызов их методов по пустому указателю является неопределённым поведением,
 *          поэтому проверяется весь набор мест, где контейнер к ним обращается
 *
 */
TEST_F(HeadersFixture, NoFrameworkTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, "/index.html");
	// Создаём объект ответа сервера
	response_t response(version_t::HTTP1_1, 200, "OK");
	// Создаём контейнер заголовков без объектов фреймворка и логирования
	headers_t headers(proto_t::HTTP1);
	// Устанавливаем провайдер запроса
	headers.provider(&request);
	// Устанавливаем идентификацию сервиса
	headers.ident("TestID", "TestApp", "3.0.0");
	// Проверяем сформированную идентификацию клиента
	ASSERT_EQ(headers.ident().find("TestApp ("), 0u);
	// Проверяем что идентификация несёт идентификатор и версию сервиса
	ASSERT_NE(headers.ident().find("TestID/3.0.0"), std::string::npos);
	// Устанавливаем непригодную к записи составляющую идентификации
	headers.ident("", "Bad\r\nX-Injected: yes", "");
	// Проверяем что непригодная составляющая не применена
	ASSERT_EQ(headers.ident().find("X-Injected"), std::string::npos);
	// Проверяем получение штампа времени в текстовом виде
	ASSERT_FALSE(headers.date(0).empty());
	// Проверяем добавление заголовков по умолчанию в запрос клиента
	ASSERT_TRUE(headers.addDefaultHeaders());
	// Устанавливаем провайдер ответа
	headers.provider(&response);
	// Проверяем сформированную идентификацию сервера
	ASSERT_EQ(headers.ident(), "TestID/3.0.0");
	// Проверяем добавление заголовков по умолчанию в ответ сервера
	ASSERT_TRUE(headers.addDefaultHeaders());
}

/**
 * @brief Метод проверки отказа от непригодной к записи идентификации сервиса
 *
 * @details Составляющие идентификации попадают в значение поля целиком, а приложение
 *          берёт их извне: возврат каретки либо перевод строки внутри них расщепил бы
 *          сообщение на два. Проверка выполняется в точке входа данных в объект, а не
 *          на сборке сообщения - иначе непригодное значение поселилось бы в объекте
 *          и ушло бы на провод везде, где сборка его не проверяет
 *
 */
TEST_F(HeadersFixture, IdentInjectionTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, "/index.html");
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Устанавливаем пригодную идентификацию сервиса
	this->_headers->ident("TestID", "TestApp", "3.0.0");
	// Запоминаем сформированную идентификацию сервиса
	const std::string expected = this->_headers->ident();
	// Проверяем что пригодная идентификация применена
	ASSERT_NE(expected.find("TestApp"), std::string::npos);
	/**
	 * Набор непригодных к записи в значение поля составляющих
	 *
	 * Возврат каретки и перевод строки расщепляют сообщение на два, а прочие
	 * управляющие символы и DEL получатель вправе отвергнуть (RFC 9110 §5.5)
	 */
	static const char * octets[] = {"\r\n", "\r", "\n", "\x01", "\x7F"};
	/**
	 * Перебираем непригодные к записи октеты
	 */
	for(const char * octet : octets){
		// Формируем непригодную к записи составляющую идентификации
		const std::string injected = (std::string("Bad") + octet + "X-Injected: yes");
		// Пробуем установить непригодный идентификатор сервиса
		this->_headers->ident(injected, "", "");
		// Пробуем установить непригодное название сервиса
		this->_headers->ident("", injected, "");
		// Пробуем установить непригодную версию сервиса
		this->_headers->ident("", "", injected);
		// Проверяем что идентификация осталась прежней
		ASSERT_EQ(this->_headers->ident(), expected);
	}
	/**
	 * Проверяем что непригодная составляющая не отменяет пригодных
	 */
	{
		// Устанавливаем непригодное название вместе с пригодной версией
		this->_headers->ident("", (std::string("Bad\r\nX-Injected: yes")), "4.0.0");
		// Получаем сформированную идентификацию сервиса
		const std::string result = this->_headers->ident();
		// Проверяем что пригодная версия применена
		ASSERT_NE(result.find("4.0.0"), std::string::npos);
		// Проверяем что непригодное название не применено
		ASSERT_EQ(result.find("X-Injected"), std::string::npos);
	}
}

/**
 * @brief Метод проверки добавления заголовков по умолчанию в запрос клиента
 *
 */
TEST_F(HeadersFixture, AddDefaultHeadersRequestTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, "/index.html");
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Устанавливаем идентификацию сервиса
	this->_headers->ident("TestID", "TestApp", "3.0.0");
	// Добавляем заголовки по умолчанию
	ASSERT_TRUE(this->_headers->addDefaultHeaders());
	// Проверяем что заголовок User-Agent добавлен
	ASSERT_TRUE(this->_headers->has("User-Agent"));
	// Проверяем что значение User-Agent совпадает с идентификацией
	ASSERT_EQ(this->_headers->at("User-Agent"), this->_headers->ident());
	// Повторная генерация не должна добавлять заголовок повторно
	ASSERT_FALSE(this->_headers->addDefaultHeaders());
	// Проверяем что количество заголовков не изменилось
	ASSERT_EQ(this->_headers->size(), 1u);
}

/**
 * @brief Метод проверки что существующий User-Agent не перезаписывается
 *
 */
TEST_F(HeadersFixture, AddDefaultHeadersRequestExistingTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, "/");
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Добавляем собственный User-Agent
	this->_headers->emplace("User-Agent", "CustomAgent/1.0");
	// Добавление заголовков по умолчанию не должно изменять существующий User-Agent
	ASSERT_FALSE(this->_headers->addDefaultHeaders());
	// Проверяем что исходный User-Agent сохранён
	ASSERT_EQ(this->_headers->at("User-Agent"), "CustomAgent/1.0");
}

/**
 * @brief Метод проверки добавления заголовков по умолчанию для HTTP-ответа
 *
 */
TEST_F(HeadersFixture, AddDefaultHeadersResponseTest){
	// Создаём объект ответа сервера
	response_t response(version_t::HTTP1_1, 200, "OK");
	// Устанавливаем провайдер ответа
	this->_headers->provider(&response);
	// Устанавливаем идентификацию сервиса
	this->_headers->ident("SvcID", "SvcName", "4.5.6");
	// Добавляем заголовки по умолчанию
	ASSERT_TRUE(this->_headers->addDefaultHeaders());
	// Проверяем что заголовок Server добавлен с названием сервиса
	ASSERT_EQ(this->_headers->at("Server"), "SvcName");
	// Проверяем что заголовок X-Powered-By добавлен с идентификацией
	ASSERT_EQ(this->_headers->at("X-Powered-By"), "SvcID/4.5.6");
	// Проверяем что заголовок Date добавлен
	ASSERT_TRUE(this->_headers->has("Date"));
	// Проверяем формат заголовка Date
	ASSERT_TRUE(std::regex_match(this->_headers->at("Date"), std::regex("^[A-Z][a-z]{2}, \\d{2} [A-Z][a-z]{2} \\d{4} \\d{2}:\\d{2}:\\d{2} GMT$")));
	// Повторная генерация не должна добавлять заголовки повторно
	ASSERT_FALSE(this->_headers->addDefaultHeaders());
	// Проверяем что количество заголовков не изменилось
	ASSERT_EQ(this->_headers->size(), 3u);
}

/**
 * @brief Метод проверки поведения методов идентификации без установленного провайдера
 *
 */
TEST_F(HeadersFixture, NoProviderSafetyTest){
	// Проверяем что провайдер не установлен
	ASSERT_TRUE(this->_headers->provider() == nullptr);
	// Проверяем что идентификация без провайдера возвращает пустую строку
	ASSERT_TRUE(this->_headers->ident().empty());
	// Проверяем что добавление заголовков по умолчанию без провайдера не выполняется
	ASSERT_FALSE(this->_headers->addDefaultHeaders());
	// Проверяем что заголовки не были добавлены
	ASSERT_TRUE(this->_headers->empty());
}

/**
 * @brief Метод проверки частичного добавления заголовков по умолчанию для ответа
 *
 */
TEST_F(HeadersFixture, AddDefaultHeadersResponsePartialTest){
	// Создаём объект ответа сервера
	response_t response(version_t::HTTP1_1, 200, "OK");
	// Устанавливаем провайдер ответа
	this->_headers->provider(&response);
	// Устанавливаем идентификацию сервиса
	this->_headers->ident("SvcID", "SvcName", "1.0.0");
	// Добавляем уже существующий заголовок Server
	this->_headers->emplace("Server", "ExistingServer");
	// Добавляем недостающие заголовки по умолчанию
	ASSERT_TRUE(this->_headers->addDefaultHeaders());
	// Проверяем что существующий Server не перезаписан
	ASSERT_EQ(this->_headers->at("Server"), "ExistingServer");
	// Проверяем что X-Powered-By добавлен
	ASSERT_EQ(this->_headers->at("X-Powered-By"), "SvcID/1.0.0");
	// Проверяем что Date добавлен
	ASSERT_TRUE(this->_headers->has("Date"));
}

/**
 * @brief Метод проверки сохранения нестандартных методов запроса в стартовой строке
 *
 */
TEST_F(HeadersFixture, StartlineExoticMethodTest){
	// Устанавливаем стартовую строку с методом WebDAV
	this->_headers->startline("PROPFIND /calendar HTTP/1.1");
	// Проверяем что провайдер запроса сформирован
	ASSERT_TRUE(this->_headers->provider() != nullptr);
	// Проверяем что стартовая строка выводится в исходном виде
	ASSERT_EQ(this->_headers->startline(), "PROPFIND /calendar HTTP/1.1");
	// Устанавливаем стартовую строку с методом, в таблицу не входящим
	this->_headers->startline("BREW /coffee HTTP/1.1");
	// Проверяем что оригинальное написание нераспознанного метода сохранено
	ASSERT_EQ(this->_headers->startline(), "BREW /coffee HTTP/1.1");
}

/**
 * @brief Метод проверки регистрозависимого разбора метода запроса
 *
 */
TEST_F(HeadersFixture, StartlineMethodCaseTest){
	// Устанавливаем стартовую строку с методом в нижнем регистре
	this->_headers->startline("get / HTTP/1.1");
	/**
	 * Проверяем что метод сохранён в исходном написании: методы запроса -
	 * регистрозависимые токены (RFC 9110 §9.1), приводить их к канонической
	 * форме означало бы принять чужое сообщение за корректный запрос GET
	 */
	ASSERT_EQ(this->_headers->startline(), "get / HTTP/1.1");
}

/**
 * @brief Метод проверки сброса провайдера при некорректной стартовой строке
 *
 */
TEST_F(HeadersFixture, StartlineInvalidResetsProviderTest){
	// Устанавливаем корректную стартовую строку запроса
	this->_headers->startline("GET /first HTTP/1.1");
	// Проверяем что провайдер запроса сформирован
	ASSERT_TRUE(this->_headers->provider() != nullptr);
	// Устанавливаем стартовую строку без пробелов
	this->_headers->startline("GET");
	// Проверяем что прежний провайдер сброшен, а не сохранён
	ASSERT_TRUE(this->_headers->provider() == nullptr);
	// Устанавливаем корректную стартовую строку запроса повторно
	this->_headers->startline("GET /second HTTP/1.1");
	// Устанавливаем стартовую строку без версии протокола
	this->_headers->startline("GET /third");
	// Проверяем что прежний провайдер сброшен
	ASSERT_TRUE(this->_headers->provider() == nullptr);
	// Устанавливаем корректную стартовую строку запроса повторно
	this->_headers->startline("GET /fourth HTTP/1.1");
	// Устанавливаем стартовую строку с методом, токеном не являющимся
	this->_headers->startline("GE(T) /fifth HTTP/1.1");
	// Проверяем что прежний провайдер сброшен
	ASSERT_TRUE(this->_headers->provider() == nullptr);
}

/**
 * @brief Метод проверки переноса идентификации сервиса при копировании и обмене
 *
 */
TEST_F(HeadersFixture, IdentLifetimeTest){
	// Создаём объект ответа сервера
	response_t response(version_t::HTTP1_1, 200, "OK");
	// Устанавливаем провайдер ответа
	this->_headers->provider(&response);
	// Устанавливаем идентификацию сервиса
	this->_headers->ident("SvcID", "SvcName", "1.2.3");
	// Получаем сформированную идентификацию сервиса
	const std::string ident = this->_headers->ident();
	// Создаём копию контейнера заголовков конструктором копирования
	Headers copy(* this->_headers);
	// Проверяем что идентификация сервиса перенесена в копию
	ASSERT_EQ(copy.ident(), ident);
	// Создаём контейнер заголовков для проверки оператора присваивания
	Headers assigned(this->_fmk.get(), this->_log.get());
	// Копируем контейнер заголовков оператором присваивания
	assigned = (* this->_headers);
	// Проверяем что идентификация сервиса перенесена оператором присваивания
	ASSERT_EQ(assigned.ident(), ident);
	// Создаём контейнер заголовков с провайдером ответа для проверки обмена
	Headers swapped(&response, headers_t::fields_t{});
	// Обмениваем содержимое контейнеров местами
	swapped.swap(* this->_headers);
	// Проверяем что идентификация сервиса перешла в принявший контейнер
	ASSERT_EQ(swapped.ident(), ident);
	// Проверяем что идентификация сервиса из принявшего контейнера ушла в отдавший
	ASSERT_NE(this->_headers->ident(), ident);
	// Создаём копию контейнера заголовков конструктором перемещения
	Headers moved(::std::move(swapped));
	// Проверяем что идентификация сервиса перенесена перемещением
	ASSERT_EQ(moved.ident(), ident);
}

/**
 * @brief Метод проверки приведения регистра названий оператором установки протокола
 *
 */
TEST_F(HeadersFixture, ProtoOperatorRecaseTest){
	// Добавляем заголовок с названием в смешанном регистре
	this->_headers->emplace("Content-Type", "text/plain");
	// Устанавливаем протокол HTTP/2 оператором присваивания
	(* this->_headers) = proto_t::HTTP2;
	// Проверяем что название заголовка приведено к нижнему регистру, как и при вызове метода
	ASSERT_EQ(this->_headers->begin()->name, "content-type");
}

/**
 * @brief Метод проверки сохранения заголовка при отказе замены по лимиту
 *
 */
TEST_F(HeadersFixture, ReplaceRejectedKeepsHeaderTest){
	// Добавляем заголовок с коротким значением
	this->_headers->emplace("X-Test", "short");
	// Ограничиваем объём памяти текущим потреблением
	this->_headers->maxMemory(this->_headers->memory());
	// Пытаемся заменить заголовок значением, в ограничение не помещающимся
	this->_headers->emplace("X-Test", "value that is much longer than allowed", headers_t::mode_t::REPLACE);
	// Проверяем что прежний заголовок сохранён, а не потерян вместе с новым
	ASSERT_TRUE(this->_headers->has("X-Test"));
	// Проверяем что прежнее значение осталось нетронутым
	ASSERT_EQ(this->_headers->at("X-Test"), "short");
	// Заменяем заголовок значением, в ограничение помещающимся
	this->_headers->emplace("X-Test", "tiny", headers_t::mode_t::REPLACE);
	// Проверяем что помещающаяся замена по-прежнему выполняется
	ASSERT_EQ(this->_headers->at("X-Test"), "tiny");
}

/**
 * @brief Метод проверки разбора абсолютного URI без пути при печати псевдозаголовков
 *
 */
TEST_F(HeadersFixture, PseudoHeadersAbsoluteUriTest){
	// Создаём объект запроса клиента с абсолютным URI без пути, но со строкой запроса
	request_t request(version_t::HTTP2, method_t::GET, std::string("https://example.com?q=1"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Устанавливаем протокол HTTP/2, при котором печатаются псевдозаголовки
	this->_headers->proto(proto_t::HTTP2);
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	/**
	 * Проверяем что строка запроса осталась в пути, а не ушла в авторитет:
	 * разделителем компонента авторитета является первый из символов '/', '?' и '#'
	 */
	ASSERT_NE(result.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что путь дополнен корневым слешем и сохранил строку запроса
	ASSERT_NE(result.find(":path: /?q=1\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки отсева запрещённых заголовков при печати для HTTP/2
 *
 */
TEST_F(HeadersFixture, PrintBinaryProtoFiltersTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/index.html"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Добавляем заголовок адресата, переносимый в псевдозаголовок авторитета
	this->_headers->emplace("Host", "example.com");
	// Добавляем заголовок управления соединением
	this->_headers->emplace("Connection", "keep-alive");
	// Добавляем заголовок продления соединения
	this->_headers->emplace("Keep-Alive", "timeout=5");
	// Добавляем заголовок обновления протокола
	this->_headers->emplace("Upgrade", "websocket");
	// Добавляем заголовок управления соединением с промежуточным узлом
	this->_headers->emplace("Proxy-Connection", "keep-alive");
	// Добавляем заголовок кодирования передачи
	this->_headers->emplace("Transfer-Encoding", "chunked");
	// Добавляем заголовок расширений передачи с недопустимым значением
	this->_headers->emplace("TE", "gzip");
	// Добавляем обычный заголовок, отсеву не подлежащий
	this->_headers->emplace("Accept", "text/html");
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что заголовок управления соединением отсеян
	ASSERT_EQ(result.find("connection:"), std::string::npos);
	// Проверяем что заголовок продления соединения отсеян
	ASSERT_EQ(result.find("keep-alive:"), std::string::npos);
	// Проверяем что заголовок обновления протокола отсеян
	ASSERT_EQ(result.find("upgrade:"), std::string::npos);
	// Проверяем что заголовок управления соединением с промежуточным узлом отсеян
	ASSERT_EQ(result.find("proxy-connection:"), std::string::npos);
	// Проверяем что заголовок кодирования передачи отсеян
	ASSERT_EQ(result.find("transfer-encoding:"), std::string::npos);
	// Проверяем что заголовок расширений передачи с недопустимым значением отсеян
	ASSERT_EQ(result.find("te:"), std::string::npos);
	// Проверяем что заголовок адресата отсеян как отдельное поле
	ASSERT_EQ(result.find("host:"), std::string::npos);
	// Проверяем что адресат перенесён в псевдозаголовок авторитета, а не потерян
	ASSERT_NE(result.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что обычный заголовок сохранён
	ASSERT_NE(result.find("accept: text/html\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки сохранения допустимых полей при печати для HTTP/2
 *
 */
TEST_F(HeadersFixture, PrintBinaryProtoKeepsAllowedTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Добавляем заголовок расширений передачи с единственным допустимым значением
	this->_headers->emplace("TE", "trailers");
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что допустимое значение поля расширений передачи сохранено
	ASSERT_NE(result.find("te: trailers\r\n"), std::string::npos);
	// Получаем напечатанный набор заголовков для протокола HTTP/1.1
	const std::string plain = this->_headers->print(proto_t::HTTP1);
	// Добавляем заголовок управления соединением
	this->_headers->emplace("Connection", "keep-alive");
	// Проверяем что для HTTP/1.1 отсев не применяется
	ASSERT_NE(this->_headers->print(proto_t::HTTP1).find("Connection: keep-alive\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки подрезки набора при понижении ограничения по количеству
 *
 */
TEST_F(HeadersFixture, LowerRecordsLimitTrimsTest){
	// Добавляем первый заголовок
	this->_headers->emplace("X-First", "1");
	// Добавляем второй заголовок
	this->_headers->emplace("X-Second", "2");
	// Добавляем третий заголовок
	this->_headers->emplace("X-Third", "3");
	// Понижаем ограничение по количеству записей ниже собранного набора
	this->_headers->maxRecords(2);
	// Проверяем что набор приведён к новому ограничению
	ASSERT_EQ(this->_headers->size(), 2u);
	// Проверяем что добавленный первым заголовок сохранён
	ASSERT_TRUE(this->_headers->has("X-First"));
	// Проверяем что добавленный вторым заголовок сохранён
	ASSERT_TRUE(this->_headers->has("X-Second"));
	// Проверяем что отброшен добавленный последним заголовок
	ASSERT_FALSE(this->_headers->has("X-Third"));
	// Проверяем что учёт потребляемой памяти пересчитан по оставшимся заголовкам
	ASSERT_EQ(this->_headers->memory(), (std::string("X-First1").size() + std::string("X-Second2").size()));
}

/**
 * @brief Метод проверки подрезки набора при понижении ограничения по памяти
 *
 */
TEST_F(HeadersFixture, LowerMemoryLimitTrimsTest){
	// Добавляем первый заголовок
	this->_headers->emplace("X-First", "value");
	// Добавляем второй заголовок
	this->_headers->emplace("X-Second", "value");
	// Запоминаем объём полезной нагрузки первого заголовка
	const size_t payload = std::string("X-Firstvalue").size();
	// Понижаем ограничение по памяти до объёма первого заголовка
	this->_headers->maxMemory(payload);
	// Проверяем что в наборе остался единственный заголовок
	ASSERT_EQ(this->_headers->size(), 1u);
	// Проверяем что сохранён добавленный первым заголовок
	ASSERT_TRUE(this->_headers->has("X-First"));
	// Проверяем что учёт потребляемой памяти уложился в новое ограничение
	ASSERT_EQ(this->_headers->memory(), payload);
	// Проверяем что учёт потребляемой памяти не превышает ограничение
	ASSERT_LE(this->_headers->memory(), this->_headers->maxMemory());
}

/**
 * @brief Метод проверки сохранения набора при повышении ограничений
 *
 */
TEST_F(HeadersFixture, RaiseLimitsKeepsHeadersTest){
	// Добавляем заголовок
	this->_headers->emplace("X-First", "value");
	// Повышаем ограничение по количеству записей
	this->_headers->maxRecords(1000);
	// Повышаем ограничение по памяти
	this->_headers->maxMemory(65536);
	// Проверяем что набор не тронут повышением ограничений
	ASSERT_EQ(this->_headers->size(), 1u);
	// Проверяем что заголовок на месте
	ASSERT_EQ(this->_headers->at("X-First"), "value");
}

/**
 * @brief Метод проверки доступа к заголовку через итератор только для чтения
 *
 */
TEST_F(HeadersFixture, IteratorReadOnlyAccessTest){
	// Добавляем заголовок
	this->_headers->emplace("X-First", "value");
	/**
	 * Проверяем что разыменование итератора даёт доступ только для чтения:
	 * правка по месту нарушила бы учёт потребляемой памяти контейнера,
	 * восстановить который извне нечем
	 */
	static_assert(
		std::is_const <std::remove_reference <headers_t::iterator_t::reference>::type>::value,
		"изменяемый итератор контейнера обязан давать доступ к заголовку только для чтения"
	);
	// Проверяем что доступ к указателю заголовка также ограничен чтением
	static_assert(
		std::is_const <std::remove_pointer <headers_t::iterator_t::pointer>::type>::value,
		"указатель заголовка обязан быть указателем на неизменяемый заголовок"
	);
	// Проверяем что чтение через итератор работает
	ASSERT_EQ(this->_headers->begin()->value, "value");
}

/**
 * @brief Метод проверки согласованности печати и операторов преобразования
 *
 */
TEST_F(HeadersFixture, ComposeConsistencyTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/index.html"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP2);
	// Добавляем заголовок адресата
	this->_headers->emplace("Host", "example.com");
	// Добавляем заголовок управления соединением
	this->_headers->emplace("Connection", "keep-alive");
	// Добавляем обычный заголовок
	this->_headers->emplace("Accept", "text/html");
	// Получаем список полей оператором преобразования
	const headers_t::fields_t fields = static_cast <headers_t::fields_t> (* this->_headers);
	// Получаем набор полей оператором преобразования
	const headers_t::entries_t entries = static_cast <headers_t::entries_t> (* this->_headers);
	// Признак наличия запрещённого заголовка в списке полей
	bool forbidden = false;
	// Признак наличия псевдозаголовка авторитета в списке полей
	bool authority = false;
	/**
	 * Выполняем перебор всех полей списка
	 */
	for(const auto & header : fields){
		// Запоминаем наличие запрещённого в HTTP/2 заголовка
		forbidden = (forbidden || (header.name == "connection") || (header.name == "host"));
		// Запоминаем наличие псевдозаголовка авторитета
		authority = (authority || (header.name == ":authority"));
	}
	// Проверяем что запрещённые заголовки в список полей не попали
	ASSERT_FALSE(forbidden);
	// Проверяем что адресат перенесён в псевдозаголовок авторитета
	ASSERT_TRUE(authority);
	// Проверяем что набор полей совпадает по количеству со списком полей
	ASSERT_EQ(entries.size(), fields.size());
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	/**
	 * Проверяем что каждое поле списка присутствует в напечатанном наборе:
	 * расхождение означало бы, что вид набора зависит от способа его получения
	 */
	for(const auto & header : fields)
		// Проверяем что поле присутствует в напечатанном наборе
		ASSERT_NE(result.find(header.name + ": " + header.value + "\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки отсутствия дубликатов псевдозаголовков
 *
 */
TEST_F(HeadersFixture, NoDuplicatePseudoHeadersTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/index.html"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Присваиваем список полей, содержащий псевдозаголовки
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":method", "GET"),
		headers_t::header_t{}.from(":path", "/index.html"),
		headers_t::header_t{}.from("Accept", "text/html")
	};
	// Восстанавливаем провайдер запроса, сброшенный присваиванием набора
	this->_headers->provider(&request);
	// Получаем список полей оператором преобразования
	const headers_t::fields_t fields = static_cast <headers_t::fields_t> (* this->_headers);
	// Количество псевдозаголовков метода запроса в списке полей
	size_t methods = 0;
	// Количество псевдозаголовков пути запроса в списке полей
	size_t paths = 0;
	/**
	 * Выполняем перебор всех полей списка
	 */
	for(const auto & header : fields){
		// Увеличиваем счётчик псевдозаголовков метода запроса
		methods += static_cast <size_t> (header.name == ":method");
		// Увеличиваем счётчик псевдозаголовков пути запроса
		paths += static_cast <size_t> (header.name == ":path");
	}
	// Проверяем что псевдозаголовок метода запроса выведен единожды
	ASSERT_EQ(methods, 1u);
	// Проверяем что псевдозаголовок пути запроса выведен единожды
	ASSERT_EQ(paths, 1u);
}

/**
 * @brief Метод проверки формирования псевдозаголовка протокола туннеля
 *
 */
TEST_F(HeadersFixture, ExtendedConnectPseudoHeadersTest){
	// Создаём объект запроса клиента расширенным методом CONNECT
	request_t request(version_t::HTTP2, method_t::CONNECT, std::string("/chat"));
	// Устанавливаем протокол туннеля, поднимаемого поверх соединения
	request.protocol = "websocket";
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Добавляем заголовок адресата
	this->_headers->emplace("Host", "example.com");
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::WEBSOCKET2);
	// Проверяем что псевдозаголовок протокола туннеля сформирован
	ASSERT_NE(result.find(":protocol: websocket\r\n"), std::string::npos);
	// Проверяем что расширенный метод несёт псевдозаголовок пути запроса
	ASSERT_NE(result.find(":path: /chat\r\n"), std::string::npos);
	// Проверяем что расширенный метод несёт псевдозаголовок схемы
	ASSERT_NE(result.find(":scheme: "), std::string::npos);
	// Создаём объект запроса клиента классическим методом CONNECT
	request_t tunnel(version_t::HTTP2, method_t::CONNECT, std::string("example.com:443"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&tunnel);
	// Получаем напечатанный набор заголовков
	const std::string plain = this->_headers->print(proto_t::HTTP2);
	// Проверяем что классический метод псевдозаголовка протокола туннеля не несёт
	ASSERT_EQ(plain.find(":protocol:"), std::string::npos);
	// Проверяем что классический метод псевдозаголовка пути запроса не несёт
	ASSERT_EQ(plain.find(":path:"), std::string::npos);
	// Проверяем что классический метод псевдозаголовка схемы не несёт
	ASSERT_EQ(plain.find(":scheme:"), std::string::npos);
}

/**
 * @brief Метод проверки формирования схемы для запроса в origin-форме
 *
 */
TEST_F(HeadersFixture, OriginFormSchemeTest){
	// Создаём объект запроса клиента с путём без схемы и авторитета
	request_t request(version_t::HTTP2, method_t::GET, std::string("/index.html"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Добавляем заголовок адресата
	this->_headers->emplace("Host", "example.com");
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	/**
	 * Проверяем что псевдозаголовок схемы сформирован: он обязателен во всяком
	 * запросе, кроме классического CONNECT, а origin-форма схемы в себе не несёт
	 */
	ASSERT_NE(result.find(":scheme: https\r\n"), std::string::npos);
	// Создаём объект запроса клиента с абсолютным URI-адресом
	request_t absolute(version_t::HTTP2, method_t::GET, std::string("http://example.com/index.html"));
	// Устанавливаем провайдер запроса (контейнер хранит копию, а не ссылку на исходный объект)
	this->_headers->provider(&absolute);
	// Проверяем что схема взята из URI-адреса, а не подменена умолчанием
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":scheme: http\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки отсева полей, перечисленных в заголовке Connection
 *
 */
TEST_F(HeadersFixture, HopByHopFieldsTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Добавляем заголовок управления соединением с перечислением полей
	this->_headers->emplace("Connection", "close, X-Custom-Hop");
	// Добавляем поле, действующее только на одном участке передачи
	this->_headers->emplace("X-Custom-Hop", "value");
	// Добавляем обычный заголовок
	this->_headers->emplace("Accept", "text/html");
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что поле, перечисленное в заголовке Connection, отсеяно
	ASSERT_EQ(result.find("x-custom-hop:"), std::string::npos);
	// Проверяем что обычный заголовок сохранён
	ASSERT_NE(result.find("accept: text/html\r\n"), std::string::npos);
	// Проверяем что для HTTP/1.1 поле сохраняется
	ASSERT_NE(this->_headers->print(proto_t::HTTP1).find("X-Custom-Hop: value\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки сброса провайдера при неразбираемом коде ответа
 *
 */
TEST_F(HeadersFixture, StartlineBadStatusResetsProviderTest){
	// Устанавливаем корректную строку состояния ответа
	this->_headers->startline("HTTP/1.1 200 OK");
	// Проверяем что провайдер ответа сформирован
	ASSERT_TRUE(this->_headers->provider() != nullptr);
	// Устанавливаем строку состояния с нечисловым кодом ответа
	this->_headers->startline("HTTP/1.1 abc OK");
	// Проверяем что прежний провайдер сброшен, а не выдаётся за разобранный
	ASSERT_TRUE(this->_headers->provider() == nullptr);
}

/**
 * @brief Метод проверки печати в формате протокола контейнера
 *
 */
TEST_F(HeadersFixture, PrintUsesContainerProtoTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP2);
	// Получаем напечатанный набор заголовков без указания протокола
	const std::string result = this->_headers->print();
	// Проверяем что печать выполнена в формате протокола контейнера
	ASSERT_NE(result.find(":method: GET\r\n"), std::string::npos);
	// Проверяем что стартовая строка HTTP/1.1 не выводится
	ASSERT_EQ(result.find("GET / HTTP/2.0"), std::string::npos);
	// Проверяем что печать без указания протокола совпадает с оператором преобразования в строку
	ASSERT_EQ(result, static_cast <std::string> (* this->_headers));
}

/**
 * @brief Метод проверки признака добавления заголовков по умолчанию при отказе
 *
 */
TEST_F(HeadersFixture, AddDefaultHeadersRejectedTest){
	// Создаём объект ответа сервера
	response_t response(version_t::HTTP1_1, 200, "OK");
	// Устанавливаем провайдер ответа
	this->_headers->provider(&response);
	// Устанавливаем идентификацию сервиса
	this->_headers->ident("SvcID", "SvcName", "1.0.0");
	// Запрещаем добавление заголовков ограничением по количеству записей
	this->_headers->maxRecords(0);
	/**
	 * Проверяем что признак успеха не выставлен: заголовки отвергнуты ограничением,
	 * и признак успеха сообщал бы о наличии обязательных заголовков, которых нет
	 */
	ASSERT_FALSE(this->_headers->addDefaultHeaders());
	// Проверяем что набор остался пустым
	ASSERT_TRUE(this->_headers->empty());
}

/**
 * @brief Метод проверки сброса провайдера при присваивании набора заголовков
 *
 */
TEST_F(HeadersFixture, AssignResetsProviderTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/index.html"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP2);
	// Присваиваем набор полей другого сообщения
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":method", "POST"),
		headers_t::header_t{}.from(":path", "/other")
	};
	/**
	 * Проверяем что прежний провайдер сброшен: иначе псевдозаголовки собирались бы
	 * из прежнего сообщения, а присвоенные пропускались как дубликаты, и наружу
	 * выдавалось бы не то, что было присвоено
	 */
	ASSERT_TRUE(this->_headers->provider() == nullptr);
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что выведен присвоенный метод запроса
	ASSERT_NE(result.find(":method: POST\r\n"), std::string::npos);
	// Проверяем что прежний метод запроса не выводится
	ASSERT_EQ(result.find(":method: GET\r\n"), std::string::npos);
	// Проверяем что выведен присвоенный путь запроса
	ASSERT_NE(result.find(":path: /other\r\n"), std::string::npos);
	// Проверяем что прежний путь запроса не выводится
	ASSERT_EQ(result.find("/index.html"), std::string::npos);
}

/**
 * @brief Метод проверки сохранения протокола HTTP/3 при присваивании набора
 *
 */
TEST_F(HeadersFixture, AssignKeepsBinaryProtoTest){
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP3);
	// Присваиваем набор полей с псевдозаголовками
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":method", "GET"),
		headers_t::header_t{}.from("Accept", "text/html")
	};
	/**
	 * Проверяем что настроенный протокол сохранён: обнаружение по составу заголовков
	 * различает лишь семейства и о HTTP/3 не знает, а замена понизила бы протокол
	 * контейнера на всяком присваивании набора
	 */
	ASSERT_EQ(this->_headers->proto(), proto_t::HTTP3);
	// Присваиваем набор полей без псевдозаголовков
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from("Accept", "text/html")
	};
	/**
	 * Проверяем что настроенный протокол сохранён и здесь: отсутствие псевдозаголовков
	 * не доказывает HTTP/1, поскольку секция трейлеров HTTP/2 их не содержит вовсе
	 */
	ASSERT_EQ(this->_headers->proto(), proto_t::HTTP3);
	// Создаём контейнер заголовков без установленного протокола
	Headers detected(this->_fmk.get(), this->_log.get());
	// Присваиваем ему набор полей без псевдозаголовков
	detected = headers_t::fields_t {
		headers_t::header_t{}.from("Accept", "text/html")
	};
	// Проверяем что ненастроенный контейнер протокол определяет по составу набора
	ASSERT_EQ(detected.proto(), proto_t::HTTP1);
}

/**
 * @brief Метод проверки согласованности печати заголовка по названию
 *
 */
TEST_F(HeadersFixture, PrintNamedFollowsComposeTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Добавляем заголовок адресата
	this->_headers->emplace("Host", "example.com");
	// Добавляем заголовок управления соединением
	this->_headers->emplace("Connection", "keep-alive");
	// Проверяем что заголовок адресата для HTTP/2 по названию не выводится
	ASSERT_TRUE(this->_headers->print("Host", proto_t::HTTP2).empty());
	// Проверяем что заголовок управления соединением для HTTP/2 по названию не выводится
	ASSERT_TRUE(this->_headers->print("Connection", proto_t::HTTP2).empty());
	// Проверяем что вместо него выводится псевдозаголовок авторитета
	ASSERT_EQ(this->_headers->print(":authority", proto_t::HTTP2), ":authority: example.com\r\n");
	// Проверяем что для HTTP/1.1 заголовок адресата выводится как есть
	ASSERT_EQ(this->_headers->print("Host", proto_t::HTTP1), "Host: example.com\r\n");
}

/**
 * @brief Метод проверки разбора версии протокола в стартовой строке
 *
 */
TEST_F(HeadersFixture, StartlineVersionParsingTest){
	// Устанавливаем строку состояния с версией, записанной с лишней цифрой
	this->_headers->startline("HTTP/1.10 200 OK");
	/**
	 * Проверяем что запись отвергнута: версия записывается одной цифрой старшего
	 * номера и одной младшего (RFC 9112 §2.3), а разбор по первому и последнему
	 * символам выдавал бы такую запись за HTTP/1.0
	 */
	ASSERT_TRUE(this->_headers->provider() == nullptr);
	// Устанавливаем строку состояния с версией без младшего номера
	this->_headers->startline("HTTP/2 200 OK");
	// Проверяем что запись без младшего номера принята
	ASSERT_TRUE(this->_headers->provider() != nullptr);
	// Устанавливаем строку запроса с версией, записанной с лишней цифрой
	this->_headers->startline("GET / HTTP/1.10");
	// Проверяем что запись отвергнута и в строке запроса
	ASSERT_TRUE(this->_headers->provider() == nullptr);
	// Устанавливаем корректную строку запроса
	this->_headers->startline("GET / HTTP/1.1");
	// Проверяем что корректная запись принята
	ASSERT_TRUE(this->_headers->provider() != nullptr);
}

/**
 * @brief Метод проверки отсева поля расширений передачи с отступами
 *
 */
TEST_F(HeadersFixture, TrailersValueTrimTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Добавляем поле расширений передачи со значением, записанным с отступами
	this->_headers->emplace("TE", " trailers ");
	// Проверяем что значение с отступами принято, а не отсеяно как недопустимое
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find("te: "), std::string::npos);
	// Заменяем значение поля на недопустимое
	this->_headers->emplace("TE", "gzip", headers_t::mode_t::REPLACE);
	// Проверяем что недопустимое значение по-прежнему отсеивается
	ASSERT_EQ(this->_headers->print(proto_t::HTTP2).find("te: "), std::string::npos);
}

/**
 * @brief Метод проверки разделения вида хранилища и вида сообщения
 *
 */
TEST_F(HeadersFixture, StorageViewVersusWireViewTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP2);
	// Добавляем заголовок адресата
	this->_headers->emplace("Host", "example.com");
	// Добавляем заголовок управления соединением
	this->_headers->emplace("Connection", "keep-alive");
	// Получаем список полей - вид сообщения
	const headers_t::fields_t fields = static_cast <headers_t::fields_t> (* this->_headers);
	// Получаем карту заголовков - вид хранилища
	const headers_t::map_t map = static_cast <headers_t::map_t> (* this->_headers);
	/**
	 * Проверяем что виды различаются намеренно: список полей отдаёт сообщение
	 * таким, каким оно уйдёт на провод, а карта - содержимое хранилища как есть,
	 * и по ней приложение находит то, что само же и положило
	 */
	ASSERT_EQ(map.count("Host"), 1u);
	// Проверяем что заголовок управления соединением в виде хранилища сохранён
	ASSERT_EQ(map.count("Connection"), 1u);
	// Проверяем что вид хранилища псевдозаголовков не содержит
	ASSERT_EQ(map.count(":method"), 0u);
	// Признак наличия заголовка адресата в виде сообщения
	bool host = false;
	// Признак наличия псевдозаголовка метода в виде сообщения
	bool method = false;
	/**
	 * Выполняем перебор всех полей вида сообщения
	 */
	for(const auto & header : fields){
		// Запоминаем наличие заголовка адресата
		host = (host || (header.name == "host"));
		// Запоминаем наличие псевдозаголовка метода запроса
		method = (method || (header.name == ":method"));
	}
	// Проверяем что вид сообщения заголовка адресата не содержит
	ASSERT_FALSE(host);
	// Проверяем что вид сообщения содержит псевдозаголовок метода запроса
	ASSERT_TRUE(method);
}

/**
 * @brief Метод проверки отказа при полезной нагрузке больше ограничения
 *
 */
TEST_F(HeadersFixture, PayloadLargerThanLimitTest){
	// Устанавливаем ограничение по памяти меньше полезной нагрузки одного заголовка
	this->_headers->maxMemory(4);
	/**
	 * Проверяем что заголовок отвергнут: сверка выполняется вычитанием полезной
	 * нагрузки из ограничения, и без предварительной проверки на её превышение
	 * вычитание ушло бы ниже нуля, обернувшись огромным числом и пропустив
	 * заголовок в обход ограничения
	 */
	this->_headers->emplace("X-Header", "value", headers_t::mode_t::APPEND);
	// Проверяем что заголовок в набор не попал
	ASSERT_TRUE(this->_headers->empty());
	// Проверяем что учёт потребляемой памяти не изменился
	ASSERT_EQ(this->_headers->memory(), 0u);
	// Проверяем что замена не помещающимся значением также отвергается
	this->_headers->emplace("X-Header", "value", headers_t::mode_t::REPLACE);
	// Проверяем что набор остался пустым
	ASSERT_TRUE(this->_headers->empty());
	// Повышаем ограничение по памяти до полезной нагрузки заголовка
	this->_headers->maxMemory(std::string("X-Headervalue").size());
	// Добавляем заголовок, помещающийся в ограничение ровно
	this->_headers->emplace("X-Header", "value", headers_t::mode_t::APPEND);
	// Проверяем что помещающийся заголовок принят
	ASSERT_TRUE(this->_headers->has("X-Header"));
	/**
	 * Заменяем заголовок значением, превышающим ограничение целиком: проверка
	 * помещаемости выполняется до удаления прежних вхождений, и без предварительной
	 * проверки на превышение она ушла бы ниже нуля, разрешив удаление, после
	 * которого добавление всё равно отвергается - заголовок пропал бы целиком
	 */
	this->_headers->emplace("X-Header", "value that does not fit the limit", headers_t::mode_t::REPLACE);
	// Проверяем что прежний заголовок сохранён, а не потерян вместе с новым
	ASSERT_EQ(this->_headers->at("X-Header"), "value");
}

/**
 * @brief Метод проверки учёта протокола и провайдера при сравнении контейнеров
 *
 */
TEST_F(HeadersFixture, EqualityAccountsProtoAndProviderTest){
	// Создаём первый контейнер заголовков
	Headers first(this->_fmk.get(), this->_log.get());
	// Создаём второй контейнер заголовков
	Headers second(this->_fmk.get(), this->_log.get());
	// Добавляем заголовок в первый контейнер
	first.emplace("Accept", "text/html");
	// Добавляем такой же заголовок во второй контейнер
	second.emplace("Accept", "text/html");
	// Проверяем что контейнеры с одинаковым набором полей равны
	ASSERT_TRUE(first == second);
	// Устанавливаем обоим контейнерам протокол HTTP/2
	first.proto(proto_t::HTTP2);
	// Устанавливаем второму контейнеру протокол того же семейства
	second.proto(proto_t::HTTP3);
	/**
	 * Проверяем что контейнеры с разными протоколами равными не считаются:
	 * печать даёт для них разный вид одного и того же набора полей
	 */
	ASSERT_FALSE(first == second);
	// Приводим протокол второго контейнера к протоколу первого
	second.proto(proto_t::HTTP2);
	// Проверяем что при совпавших протоколах контейнеры снова равны
	ASSERT_TRUE(first == second);
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/index.html"));
	// Устанавливаем провайдер первому контейнеру
	first.provider(&request);
	// Проверяем что контейнер с провайдером не равен контейнеру без него
	ASSERT_FALSE(first == second);
	// Создаём объект запроса клиента с другой целью
	request_t other(version_t::HTTP2, method_t::GET, std::string("/other"));
	// Устанавливаем провайдер второму контейнеру
	second.provider(&other);
	// Проверяем что контейнеры с разными целями запроса не равны
	ASSERT_FALSE(first == second);
	// Устанавливаем второму контейнеру такой же провайдер
	second.provider(&request);
	// Проверяем что при совпавших провайдерах контейнеры равны
	ASSERT_TRUE(first == second);
}

/**
 * @brief Метод проверки полного сброса контейнера
 *
 */
TEST_F(HeadersFixture, ResetDropsProviderTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Добавляем заголовок
	this->_headers->emplace("Accept", "text/html");
	// Выполняем полный сброс контейнера
	this->_headers->reset();
	// Проверяем что набор заголовков пуст
	ASSERT_TRUE(this->_headers->empty());
	/**
	 * Проверяем что провайдер сброшен: сброс контейнера полный, и оставшийся
	 * провайдер описывал бы сообщение, полей которого больше нет
	 */
	ASSERT_TRUE(this->_headers->provider() == nullptr);
}

/**
 * @brief Метод проверки режимов слияния заголовков
 *
 */
TEST_F(HeadersFixture, MergeModesTest){
	// Создаём контейнер заголовков для слияния
	Headers source(this->_fmk.get(), this->_log.get());
	// Добавляем в него поле, встречающееся в сообщении единожды
	source.emplace("Content-Length", "128");
	// Добавляем в текущий контейнер такое же поле с другим значением
	this->_headers->emplace("Content-Length", "64");
	// Выполняем слияние в режиме по умолчанию
	this->_headers->merge(source);
	// Проверяем что режим по умолчанию сохраняет одноимённые поля
	ASSERT_EQ(this->_headers->count("Content-Length"), 2u);
	// Выполняем слияние в режиме замены
	this->_headers->merge(source, headers_t::mode_t::REPLACE);
	// Проверяем что режим замены оставил единственное поле
	ASSERT_EQ(this->_headers->count("Content-Length"), 1u);
	// Проверяем что оставлено значение из переданного контейнера
	ASSERT_EQ(this->_headers->at("Content-Length"), "128");
}

/**
 * @brief Метод проверки отсева по нескольким заголовкам управления соединением
 *
 */
TEST_F(HeadersFixture, MultipleConnectionHeadersTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Добавляем первый заголовок управления соединением с перечислением полей
	this->_headers->emplace("Connection", "close, X-First-Hop", headers_t::mode_t::APPEND);
	// Добавляем второй заголовок управления соединением с другим перечислением
	this->_headers->emplace("Connection", "X-Second-Hop", headers_t::mode_t::APPEND);
	// Добавляем первое поле одного участка передачи
	this->_headers->emplace("X-First-Hop", "one");
	// Добавляем второе поле одного участка передачи
	this->_headers->emplace("X-Second-Hop", "two");
	// Добавляем обычный заголовок
	this->_headers->emplace("Accept", "text/html");
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что поле из первого перечисления отсеяно
	ASSERT_EQ(result.find("x-first-hop:"), std::string::npos);
	// Проверяем что поле из второго перечисления также отсеяно
	ASSERT_EQ(result.find("x-second-hop:"), std::string::npos);
	// Проверяем что обычный заголовок сохранён
	ASSERT_NE(result.find("accept: text/html\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки удаления нескольких одноимённых заголовков
 *
 */
TEST_F(HeadersFixture, EraseCompactsSetTest){
	// Добавляем заголовок, законно встречающийся несколько раз
	this->_headers->emplace("Set-Cookie", "a=1", headers_t::mode_t::APPEND);
	// Добавляем второе вхождение заголовка
	this->_headers->emplace("Set-Cookie", "b=2", headers_t::mode_t::APPEND);
	// Добавляем третье вхождение заголовка
	this->_headers->emplace("Set-Cookie", "c=3", headers_t::mode_t::APPEND);
	// Добавляем заголовок, удалению не подлежащий
	this->_headers->emplace("Accept", "text/html", headers_t::mode_t::APPEND);
	// Добавляем ещё один заголовок, удалению не подлежащий
	this->_headers->emplace("Host", "example.com", headers_t::mode_t::APPEND);
	// Удаляем все вхождения заголовка
	this->_headers->erase("Set-Cookie");
	// Проверяем что все вхождения удалены
	ASSERT_EQ(this->_headers->count("Set-Cookie"), 0u);
	// Проверяем что прочие заголовки сохранены
	ASSERT_EQ(this->_headers->size(), 2u);
	// Проверяем что порядок сохранённых заголовков не нарушен
	ASSERT_EQ(this->_headers->begin()->name, "Accept");
	// Проверяем что значение первого сохранённого заголовка не повреждено
	ASSERT_EQ(this->_headers->at("Accept"), "text/html");
	// Проверяем что значение второго сохранённого заголовка не повреждено
	ASSERT_EQ(this->_headers->at("Host"), "example.com");
	// Проверяем что учёт потребляемой памяти пересчитан по оставшимся заголовкам
	ASSERT_EQ(this->_headers->memory(), (std::string("Accepttext/html").size() + std::string("Hostexample.com").size()));
}

/**
 * @brief Метод проверки сохранения набора при отказе слияния по ограничению
 *
 */
TEST_F(HeadersFixture, MergeRejectedKeepsHeadersTest){
	// Создаём контейнер заголовков для слияния
	Headers source(this->_fmk.get(), this->_log.get());
	// Добавляем в него заголовок с длинным значением
	source.emplace("X-Field", "value that is much longer than the limit allows");
	// Добавляем в текущий контейнер одноимённый заголовок с коротким значением
	this->_headers->emplace("X-Field", "short");
	// Ограничиваем объём памяти текущим потреблением
	this->_headers->maxMemory(this->_headers->memory());
	// Выполняем слияние в режиме замены
	this->_headers->merge(source, headers_t::mode_t::REPLACE);
	/**
	 * Проверяем что прежний заголовок сохранён: слияние с заменой удаляет прежние
	 * вхождения, и отказ в добавлении после удаления убрал бы поле из набора целиком,
	 * не сообщив об этом вызывающей стороне
	 */
	ASSERT_TRUE(this->_headers->has("X-Field"));
	// Проверяем что прежнее значение осталось нетронутым
	ASSERT_EQ(this->_headers->at("X-Field"), "short");
	// Создаём контейнер заголовков с помещающимся значением
	Headers fitting(this->_fmk.get(), this->_log.get());
	// Добавляем в него одноимённый заголовок с коротким значением
	fitting.emplace("X-Field", "tiny");
	// Выполняем слияние в режиме замены
	this->_headers->merge(fitting, headers_t::mode_t::REPLACE);
	// Проверяем что помещающееся слияние по-прежнему выполняется
	ASSERT_EQ(this->_headers->at("X-Field"), "tiny");
	// Проверяем что одноимённый заголовок остался единственным
	ASSERT_EQ(this->_headers->count("X-Field"), 1u);
}

/**
 * @brief Метод проверки сравнения заголовков в неупорядоченном наборе
 *
 */
TEST_F(HeadersFixture, HeaderEqualityInEntriesTest){
	// Добавляем заголовок, законно встречающийся несколько раз
	this->_headers->emplace("Set-Cookie", "a=1", headers_t::mode_t::APPEND);
	// Добавляем второе вхождение заголовка с другим значением
	this->_headers->emplace("Set-Cookie", "b=2", headers_t::mode_t::APPEND);
	// Получаем набор заголовков оператором преобразования
	const headers_t::entries_t entries = static_cast <headers_t::entries_t> (* this->_headers);
	// Проверяем что оба вхождения попали в набор
	ASSERT_EQ(entries.size(), 2u);
	/**
	 * Проверяем что вхождения различаются по значению: сравнение по одному лишь
	 * названию считало бы их одним и тем же заголовком, и поиск по набору
	 * возвращал бы совпадение для значения, которого там нет
	 */
	ASSERT_EQ(entries.count(headers_t::header_t().from("Set-Cookie", "a=1")), 1u);
	// Проверяем что второе вхождение также находится по своему значению
	ASSERT_EQ(entries.count(headers_t::header_t().from("Set-Cookie", "b=2")), 1u);
	// Проверяем что отсутствующее значение в наборе не находится
	ASSERT_EQ(entries.count(headers_t::header_t().from("Set-Cookie", "c=3")), 0u);
}

/**
 * @brief Метод проверки отбрасывания якоря и сведений о пользователе
 *
 */
TEST_F(HeadersFixture, AbsoluteUriComponentsTest){
	// Создаём объект запроса клиента с якорем в URI-адресе
	request_t fragment(version_t::HTTP2, method_t::GET, std::string("https://example.com/path?q=1#frag"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&fragment);
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	/**
	 * Проверяем что якорь отброшен: он обрабатывается принимающей стороной
	 * и в запрос не отправляется вовсе (RFC 9112 §3.2.1)
	 */
	ASSERT_NE(result.find(":path: /path?q=1\r\n"), std::string::npos);
	// Проверяем что якорь не уехал в путь запроса
	ASSERT_EQ(result.find("#frag"), std::string::npos);
	// Создаём объект запроса клиента со сведениями о пользователе в URI-адресе
	request_t userinfo(version_t::HTTP2, method_t::GET, std::string("https://user:pass@example.com/path"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&userinfo);
	// Получаем напечатанный набор заголовков
	const std::string credentials = this->_headers->print(proto_t::HTTP2);
	/**
	 * Проверяем что сведения о пользователе отброшены: подкомпонент объявлен
	 * устаревшим и в псевдозаголовке авторитета запрещён (RFC 9113 §8.3.1)
	 */
	ASSERT_NE(credentials.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что пароль не уехал на провод
	ASSERT_EQ(credentials.find("pass"), std::string::npos);
	// Создаём объект запроса клиента с адресом IPv6 в URI-адресе
	request_t address(version_t::HTTP2, method_t::GET, std::string("https://[2001:db8::1]:8443/path"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&address);
	// Проверяем что адрес IPv6 отбрасыванием сведений о пользователе не повреждён
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":authority: [2001:db8::1]:8443\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки переноса пустого заголовка адресата
 *
 */
TEST_F(HeadersFixture, EmptyHostNotTransferredTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Добавляем заголовок адресата с пустым значением
	this->_headers->emplace("Host", "");
	/**
	 * Проверяем что пустой авторитет псевдозаголовком не передаётся: он не указывает
	 * ни на какой узел, а принимающая сторона обязана считать такое сообщение некорректным
	 */
	ASSERT_EQ(this->_headers->print(proto_t::HTTP2).find(":authority:"), std::string::npos);
	// Заменяем заголовок адресата непустым значением
	this->_headers->emplace("Host", "example.com", headers_t::mode_t::REPLACE);
	// Проверяем что непустой адресат переносится по-прежнему
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":authority: example.com\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки разбора несуществующих версий семейства HTTP/1
 *
 */
TEST_F(HeadersFixture, UnknownMinorVersionTest){
	// Устанавливаем строку состояния с несуществующей версией семейства HTTP/1
	this->_headers->startline("HTTP/1.2 200 OK");
	/**
	 * Проверяем что запись отвергнута: семейство HTTP/1 знает только две версии,
	 * и приведение прочих к HTTP/1.0 выдавало бы за известную версию несуществующую
	 */
	ASSERT_TRUE(this->_headers->provider() == nullptr);
	// Устанавливаем строку состояния с версией HTTP/1.0
	this->_headers->startline("HTTP/1.0 200 OK");
	// Проверяем что существующая версия принята
	ASSERT_TRUE(this->_headers->provider() != nullptr);
	// Проверяем что версия разобрана верно
	ASSERT_EQ(this->_headers->startline(), "HTTP/1.0 200 OK");
}

/**
 * @brief Метод проверки неделимости слияния в режиме добавления
 *
 */
TEST_F(HeadersFixture, MergeAppendRejectedKeepsSetTest){
	// Создаём контейнер заголовков для слияния
	Headers source(this->_fmk.get(), this->_log.get());
	// Добавляем в него первый заголовок
	source.emplace("X-First", "one", headers_t::mode_t::APPEND);
	// Добавляем в него второй заголовок
	source.emplace("X-Second", "two", headers_t::mode_t::APPEND);
	// Добавляем в текущий контейнер собственный заголовок
	this->_headers->emplace("X-Own", "value");
	// Ограничиваем количество записей так, чтобы поместился лишь один сливаемый заголовок
	this->_headers->maxRecords(2);
	// Выполняем слияние в режиме добавления
	this->_headers->merge(source);
	/**
	 * Проверяем что слияние не выполнено вовсе: набор, принявший часть сливаемых
	 * заголовков, слиянием не является, а вызывающая сторона об этом не узнала бы
	 */
	ASSERT_EQ(this->_headers->size(), 1u);
	// Проверяем что собственный заголовок сохранён
	ASSERT_TRUE(this->_headers->has("X-Own"));
	// Проверяем что первый сливаемый заголовок не добавлен
	ASSERT_FALSE(this->_headers->has("X-First"));
	// Проверяем что второй сливаемый заголовок не добавлен
	ASSERT_FALSE(this->_headers->has("X-Second"));
	// Повышаем ограничение по количеству записей до помещающегося
	this->_headers->maxRecords(3);
	// Выполняем слияние в режиме добавления повторно
	this->_headers->merge(source);
	// Проверяем что помещающееся слияние выполняется целиком
	ASSERT_EQ(this->_headers->size(), 3u);
}

/**
 * @brief Метод проверки переноса адресата при уже заданном авторитете
 *
 */
TEST_F(HeadersFixture, AuthorityAndHostWithoutProviderTest){
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP2);
	// Присваиваем набор полей, содержащий и псевдозаголовок авторитета, и заголовок адресата
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":method", "GET"),
		headers_t::header_t{}.from(":path", "/"),
		headers_t::header_t{}.from(":authority", "example.com"),
		headers_t::header_t{}.from("Host", "other.example.com")
	};
	// Проверяем что провайдер присваиванием набора сброшен
	ASSERT_TRUE(this->_headers->provider() == nullptr);
	// Получаем список полей оператором преобразования
	const headers_t::fields_t fields = static_cast <headers_t::fields_t> (* this->_headers);
	// Количество псевдозаголовков авторитета в собранном наборе
	size_t authority = 0;
	// Признак наличия заголовка адресата в собранном наборе
	bool host = false;
	/**
	 * Выполняем перебор всех полей собранного набора
	 */
	for(const auto & header : fields){
		// Увеличиваем счётчик псевдозаголовков авторитета
		authority += static_cast <size_t> (header.name == ":authority");
		// Запоминаем наличие заголовка адресата
		host = (host || (header.name == "host"));
	}
	/**
	 * Проверяем что псевдозаголовок авторитета выведен единожды: перенос адресата
	 * при уже заданном авторитете дал бы сообщение с двумя авторитетами, которое
	 * принимающая сторона обязана отвергнуть
	 */
	ASSERT_EQ(authority, 1u);
	// Проверяем что заголовок адресата отсеян
	ASSERT_FALSE(host);
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что сохранено значение заданного псевдозаголовка, а не перенесённого адресата
	ASSERT_NE(result.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что значение заголовка адресата в набор не попало
	ASSERT_EQ(result.find("other.example.com"), std::string::npos);
}

/**
 * @brief Метод проверки разбора URI-адреса, состоящего из одного авторитета и якоря
 *
 */
TEST_F(HeadersFixture, UriWithFragmentOnlyTest){
	// Создаём объект запроса клиента с URI-адресом без пути, но с якорем
	request_t request(version_t::HTTP2, method_t::GET, std::string("https://example.com#frag"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	/**
	 * Проверяем что путь принял корневое значение: после отбрасывания якоря
	 * от пути не остаётся ничего, и псевдозаголовок обязан начинаться с '/'
	 */
	ASSERT_NE(result.find(":path: /\r\n"), std::string::npos);
	// Проверяем что авторитет разобран верно
	ASSERT_NE(result.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что якорь на провод не уехал
	ASSERT_EQ(result.find("frag"), std::string::npos);
	// Создаём объект запроса клиента с URI-адресом, оканчивающимся пустым якорем
	request_t empty(version_t::HTTP2, method_t::GET, std::string("https://example.com#"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&empty);
	// Проверяем что пустой якорь обрабатывается так же
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":path: /\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки очистки цели классического метода CONNECT
 *
 */
TEST_F(HeadersFixture, ConnectAuthorityCleanupTest){
	// Создаём объект запроса клиента классическим методом CONNECT со сведениями о пользователе
	request_t request(version_t::HTTP2, method_t::CONNECT, std::string("user:pass@example.com:443"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	/**
	 * Проверяем что сведения о пользователе отброшены и из цели метода CONNECT:
	 * запрет на них в псевдозаголовке авторитета общий для всех форм записи
	 */
	ASSERT_NE(result.find(":authority: example.com:443\r\n"), std::string::npos);
	// Проверяем что пароль на провод не уехал
	ASSERT_EQ(result.find("pass"), std::string::npos);
}

/**
 * @brief Метод проверки строгого разбора кода ответа сервера
 *
 */
TEST_F(HeadersFixture, StrictStatusCodeTest){
	// Устанавливаем строку состояния с кодом ответа, слитым с сообщением
	this->_headers->startline("HTTP/1.1 200OK");
	/**
	 * Проверяем что запись отвергнута: код ответа записывается ровно тремя
	 * цифрами (RFC 9112 §4), а разбор числа из начала участка принимал бы её
	 */
	ASSERT_TRUE(this->_headers->provider() == nullptr);
	// Устанавливаем строку состояния с двузначным кодом ответа
	this->_headers->startline("HTTP/1.1 20 OK");
	// Проверяем что двузначный код отвергнут
	ASSERT_TRUE(this->_headers->provider() == nullptr);
	// Устанавливаем строку состояния с четырёхзначным кодом ответа
	this->_headers->startline("HTTP/1.1 2000 OK");
	// Проверяем что четырёхзначный код отвергнут, а не усечён до типа кода
	ASSERT_TRUE(this->_headers->provider() == nullptr);
	// Устанавливаем корректную строку состояния
	this->_headers->startline("HTTP/1.1 204 No Content");
	// Проверяем что корректная запись принята
	ASSERT_TRUE(this->_headers->provider() != nullptr);
	// Проверяем что код ответа разобран верно
	ASSERT_EQ(this->_headers->startline(), "HTTP/1.1 204 No Content");
}

/**
 * @brief Метод проверки отказа строки запроса без версии протокола
 *
 */
TEST_F(HeadersFixture, RequestWithoutVersionTest){
	// Устанавливаем корректную строку запроса
	this->_headers->startline("GET /path HTTP/1.1");
	// Проверяем что провайдер запроса сформирован
	ASSERT_TRUE(this->_headers->provider() != nullptr);
	// Устанавливаем строку запроса без версии протокола, но с завершающим пробелом
	this->_headers->startline("GET /path ");
	/**
	 * Проверяем что запись отвергнута: версия протокола в строке запроса обязательна
	 * (RFC 9112 §3), а подстановка её по умолчанию выдавала бы за разобранный запрос
	 * строку, которой в обмене быть не может
	 */
	ASSERT_TRUE(this->_headers->provider() == nullptr);
}

/**
 * @brief Метод проверки сохранения поля расширений передачи при перечислении в Connection
 *
 */
TEST_F(HeadersFixture, HopByHopKeepsTrailersTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Добавляем заголовок управления соединением, перечисляющий поле расширений передачи
	this->_headers->emplace("Connection", "TE, close");
	// Добавляем поле расширений передачи с единственным допустимым значением
	this->_headers->emplace("TE", "trailers");
	// Добавляем поле одного участка передачи, перечисленное в заголовке Connection
	this->_headers->emplace("Connection", "X-Custom-Hop", headers_t::mode_t::APPEND);
	// Добавляем само поле одного участка передачи
	this->_headers->emplace("X-Custom-Hop", "value");
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	/**
	 * Проверяем что поле расширений передачи сохранено: оно единственное из полей
	 * управления соединением, которое в HTTP/2 и HTTP/3 разрешено (RFC 9113 §8.2.2),
	 * а перечисление его в заголовке Connection - обычное дело для HTTP/1
	 */
	ASSERT_NE(result.find("te: trailers\r\n"), std::string::npos);
	// Проверяем что прочие перечисленные поля по-прежнему отсеиваются
	ASSERT_EQ(result.find("x-custom-hop:"), std::string::npos);
	// Проверяем что сам заголовок управления соединением отсеян
	ASSERT_EQ(result.find("connection:"), std::string::npos);
	// Заменяем значение поля расширений передачи на недопустимое
	this->_headers->emplace("TE", "gzip", headers_t::mode_t::REPLACE);
	// Проверяем что исключение действует только для допустимого значения
	ASSERT_EQ(this->_headers->print(proto_t::HTTP2).find("te: "), std::string::npos);
}

/**
 * @brief Метод проверки очистки заголовка адресата при переносе в авторитет
 *
 */
TEST_F(HeadersFixture, HostAuthorityCleanupTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP2, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Добавляем заголовок адресата со сведениями о пользователе
	this->_headers->emplace("Host", "user:pass@example.com");
	/**
	 * Проверяем что перенос очищает значение по тем же правилам, что и авторитет
	 * абсолютного URI-адреса: запрет на сведения о пользователе общий
	 */
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что пароль на провод не уехал
	ASSERT_EQ(this->_headers->print(proto_t::HTTP2).find("pass"), std::string::npos);
	// Заменяем заголовок адресата значением с окружающими отступами
	this->_headers->emplace("Host", "  example.com  ", headers_t::mode_t::REPLACE);
	// Проверяем что отступы отброшены: к значению поля они не относятся
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":authority: example.com\r\n"), std::string::npos);
	// Заменяем заголовок адресата значением из одних отступов
	this->_headers->emplace("Host", "   ", headers_t::mode_t::REPLACE);
	// Проверяем что опустевший после очистки авторитет псевдозаголовком не передаётся
	ASSERT_EQ(this->_headers->print(proto_t::HTTP2).find(":authority:"), std::string::npos);
}

/**
 * @brief Метод проверки отсутствия авторитета в ответе сервера
 *
 */
TEST_F(HeadersFixture, ResponseHasNoAuthorityTest){
	// Создаём объект ответа сервера
	response_t response(version_t::HTTP2, 200, std::string("OK"));
	// Устанавливаем провайдер ответа
	this->_headers->provider(&response);
	// Добавляем заголовок адресата: в ответе HTTP/1 он не запрещён
	this->_headers->emplace("Host", "example.com");
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	/**
	 * Проверяем что заголовок адресата в авторитет не перенесён: в ответе сервера
	 * допустим единственный псевдозаголовок [:status] (RFC 9113 §8.3.2), и всякий
	 * другой обязывает принимающую сторону отвергнуть сообщение
	 */
	ASSERT_EQ(result.find(":authority"), std::string::npos);
	// Проверяем что псевдозаголовок кода ответа на месте
	ASSERT_NE(result.find(":status: 200\r\n"), std::string::npos);
	/**
	 * Проверяем что сам заголовок адресата сохранён: снятию подлежат поля, управляющие
	 * соединением (RFC 9113 §8.2.2), а адресат к ним не отнесён. У запроса он снимается
	 * лишь потому, что его место занимает псевдозаголовок авторитета, здесь же авторитет
	 * не формируется, и снятие молча теряло бы поле, передавать которое не запрещено
	 */
	ASSERT_NE(result.find("host: example.com\r\n"), std::string::npos);
	// Проверяем что тот же заголовок адресата в запросе клиента переносится по-прежнему
	request_t request(version_t::HTTP2, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Проверяем что для запроса перенос выполнен
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":authority: example.com\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки текстового вида всех известных версий протокола
 *
 */
TEST_F(HeadersFixture, EveryProtocolVersionPrintsTest){
	// Перечень версий протокола и их ожидаемого текстового вида
	const std::vector <std::pair <version_t, std::string>> versions = {
		{version_t::HTTP1_0, "1.0"}, {version_t::HTTP1_1, "1.1"},
		{version_t::HTTP2, "2.0"}, {version_t::HTTP3, "3.0"},
		{version_t::HTTP4, "4.0"}, {version_t::HTTP5, "5.0"}
	};
	/**
	 * Перебираем все известные версии протокола
	 */
	for(const auto & item : versions){
		// Создаём объект ответа сервера указанной версии
		response_t response(item.first, 200, std::string("OK"));
		// Устанавливаем провайдер ответа
		this->_headers->provider(&response);
		// Проверяем что строка состояния собрана с ожидаемой версией
		ASSERT_EQ(this->_headers->startline(), "HTTP/" + item.second + " 200 OK");
		// Создаём объект запроса клиента той же версии
		request_t request(item.first, method_t::GET, std::string("/x"));
		// Устанавливаем провайдер запроса
		this->_headers->provider(&request);
		// Проверяем что строка запроса собрана с ожидаемой версией
		ASSERT_EQ(this->_headers->startline(), "GET /x HTTP/" + item.second);
	}
}

/**
 * @brief Метод проверки разбора стартовой строки со всеми версиями протокола
 *
 */
TEST_F(HeadersFixture, EveryProtocolVersionParsesTest){
	// Перечень разбираемых записей версии и их ожидаемого вида после разбора
	const std::vector <std::pair <std::string, std::string>> samples = {
		{"HTTP/1.0", "1.0"}, {"HTTP/1.1", "1.1"}, {"HTTP/2.0", "2.0"},
		{"HTTP/3.0", "3.0"}, {"HTTP/4.0", "4.0"}, {"HTTP/5.0", "5.0"},
		{"HTTP/2", "2.0"}, {"HTTP/3", "3.0"}, {"HTTP/4", "4.0"}, {"HTTP/5", "5.0"}
	};
	/**
	 * Перебираем все разбираемые записи версии протокола
	 */
	for(const auto & item : samples){
		// Устанавливаем строку запроса с очередной записью версии
		this->_headers->startline("GET /x " + item.first);
		// Проверяем что версия разобрана и выведена в каноническом виде
		ASSERT_EQ(this->_headers->startline(), "GET /x HTTP/" + item.second);
	}
	// Перечень записей версии, которые версией не являются
	const std::vector <std::string> broken = {
		"HTTP/1.2", "HTTP/1.9", "HTTP/6.0", "HTTP/9.9", "HTTP/1.10",
		"HTTP/x.1", "HTTP/1x1", "HTTP/", "HTTP/11", "HTTPS/1.1"
	};
	/**
	 * Перебираем записи, версией не являющиеся
	 */
	for(const auto & item : broken){
		// Устанавливаем корректный запрос, чтобы провайдер заведомо был установлен
		this->_headers->startline("GET /x HTTP/1.1");
		// Устанавливаем строку запроса с непригодной записью версии
		this->_headers->startline("GET /x " + item);
		// Проверяем что провайдер сброшен: неразобранная строка прежнее сообщение не сохраняет
		ASSERT_EQ(this->_headers->provider(), nullptr);
	}
}

/**
 * @brief Метод проверки отсева всех заголовков управления соединением
 *
 */
TEST_F(HeadersFixture, EveryConnectionSpecificFieldTest){
	// Перечень заголовков, управляющих соединением
	const std::vector <std::string> fields = {
		"Upgrade", "Keep-Alive", "Connection", "Proxy-Connection", "Transfer-Encoding"
	};
	/**
	 * Перебираем все заголовки управления соединением
	 */
	for(const auto & name : fields){
		// Очищаем набор перед очередной проверкой
		this->_headers->clear();
		// Добавляем заголовок управления соединением
		this->_headers->emplace(name, "value");
		// Добавляем обычный заголовок, который отсеян быть не должен
		this->_headers->emplace("X-Custom", "kept");
		// Проверяем что для протокола HTTP/1 заголовок остаётся на месте
		ASSERT_NE(this->_headers->print(proto_t::HTTP1).find("value"), std::string::npos);
		// Получаем вид сообщения под протокол HTTP/2
		const std::string result = this->_headers->print(proto_t::HTTP2);
		// Проверяем что заголовок управления соединением отсеян
		ASSERT_EQ(result.find("value"), std::string::npos);
		// Проверяем что обычный заголовок сохранён
		ASSERT_NE(result.find("kept"), std::string::npos);
	}
}

/**
 * @brief Метод проверки отказа замены по ограничению у всех перегрузок добавления
 *
 */
TEST_F(HeadersFixture, EveryEmplaceOverloadRejectsTest){
	// Название и непомещающееся значение заголовка
	const std::string name = "X-Field";
	// Значение, заведомо не помещающееся в ограничение
	const std::string huge(200, 'z');
	/**
	 * @brief Функция подготовки контейнера к очередной проверке
	 *
	 */
	auto prepare = [this, &name]() noexcept -> void {
		// Снимаем ограничение на время подготовки
		this->_headers->maxMemory(4096);
		// Очищаем набор перед очередной проверкой
		this->_headers->clear();
		// Добавляем заголовок с коротким значением
		this->_headers->emplace(name, std::string("short"));
		// Опускаем ограничение так, чтобы длинное значение в него не поместилось
		this->_headers->maxMemory(64);
	};
	// Проверяем перегрузку с переносом названия и значения
	prepare();
	this->_headers->emplace(std::string(name), std::string(huge), headers_t::mode_t::REPLACE);
	ASSERT_EQ(this->_headers->at(name), "short");
	// Проверяем перегрузку с названием из C-строки и переносом значения
	prepare();
	this->_headers->emplace(name.c_str(), std::string(huge), headers_t::mode_t::REPLACE);
	ASSERT_EQ(this->_headers->at(name), "short");
	// Проверяем перегрузку с переносом названия и значением из C-строки
	prepare();
	this->_headers->emplace(std::string(name), huge.c_str(), headers_t::mode_t::REPLACE);
	ASSERT_EQ(this->_headers->at(name), "short");
	// Проверяем перегрузку с названием и значением из C-строк
	prepare();
	this->_headers->emplace(name.c_str(), huge.c_str(), headers_t::mode_t::REPLACE);
	ASSERT_EQ(this->_headers->at(name), "short");
	// Проверяем перегрузку с названием из C-строки и копированием значения
	prepare();
	this->_headers->emplace(name.c_str(), huge, headers_t::mode_t::REPLACE);
	ASSERT_EQ(this->_headers->at(name), "short");
	// Проверяем перегрузку с копированием названия и значением из C-строки
	prepare();
	this->_headers->emplace(name, huge.c_str(), headers_t::mode_t::REPLACE);
	ASSERT_EQ(this->_headers->at(name), "short");
	// Проверяем перегрузку с переносом названия и копированием значения
	prepare();
	this->_headers->emplace(std::string(name), huge, headers_t::mode_t::REPLACE);
	ASSERT_EQ(this->_headers->at(name), "short");
	// Проверяем перегрузку с копированием названия и переносом значения
	prepare();
	this->_headers->emplace(name, std::string(huge), headers_t::mode_t::REPLACE);
	ASSERT_EQ(this->_headers->at(name), "short");
	// Проверяем перегрузку с участками строк
	prepare();
	this->_headers->emplace(std::string_view(name), std::string_view(huge), headers_t::mode_t::REPLACE);
	ASSERT_EQ(this->_headers->at(name), "short");
	// Проверяем перегрузку с копированием названия и значения
	prepare();
	this->_headers->emplace(name, huge, headers_t::mode_t::REPLACE);
	ASSERT_EQ(this->_headers->at(name), "short");
}

/**
 * @brief Метод проверки защиты перегрузок добавления от нулевого указателя
 *
 */
TEST_F(HeadersFixture, EmplaceHandlesNullPointerTest){
	// Нулевой указатель на C-строку
	const char * empty = nullptr;
	// Добавляем заголовок с нулевым названием и нулевым значением
	this->_headers->emplace(empty, empty);
	// Проверяем что заголовок без названия в набор не попал
	ASSERT_EQ(this->_headers->size(), 0);
	// Добавляем заголовок с нулевым значением
	this->_headers->emplace("X-Null", empty);
	// Проверяем что заголовок добавлен с пустым значением
	ASSERT_EQ(this->_headers->size(), 1);
	// Проверяем что значение пустое
	ASSERT_TRUE(this->_headers->at("X-Null").empty());
	// Добавляем заголовок с нулевым названием и значением из строки
	this->_headers->emplace(empty, std::string("value"));
	// Проверяем что заголовок без названия в набор не попал
	ASSERT_EQ(this->_headers->size(), 1);
	// Добавляем заголовок с названием из строки и нулевым значением
	this->_headers->emplace(std::string("X-Second"), empty);
	// Проверяем что заголовок добавлен
	ASSERT_EQ(this->_headers->size(), 2);
}

/**
 * @brief Метод проверки добавления заголовков по умолчанию для ответа сервера
 *
 */
TEST_F(HeadersFixture, DefaultHeadersForResponseTest){
	// Проверяем что без провайдера заголовки по умолчанию не добавляются
	ASSERT_FALSE(this->_headers->addDefaultHeaders());
	// Создаём объект ответа сервера
	response_t response(version_t::HTTP1_1, 200, std::string("OK"));
	// Устанавливаем провайдер ответа
	this->_headers->provider(&response);
	// Проверяем что заголовки по умолчанию добавлены
	ASSERT_TRUE(this->_headers->addDefaultHeaders());
	// Проверяем что название сервиса на месте
	ASSERT_TRUE(this->_headers->has("Server"));
	// Проверяем что идентификация сервиса на месте
	ASSERT_TRUE(this->_headers->has("X-Powered-By"));
	// Проверяем что штамп времени на месте
	ASSERT_TRUE(this->_headers->has("Date"));
	// Проверяем что идентификация ответа собрана в виде «идентификатор/версия»
	ASSERT_NE(this->_headers->at("X-Powered-By").find('/'), std::string::npos);
	// Проверяем что повторный вызов ничего не меняет: все заголовки уже на месте
	ASSERT_FALSE(this->_headers->addDefaultHeaders());
	// Проверяем что кратность заголовков не выросла
	ASSERT_EQ(this->_headers->count("Server"), 1);
}

/**
 * @brief Метод проверки добавления заголовка по умолчанию для запроса клиента
 *
 */
TEST_F(HeadersFixture, DefaultHeadersForRequestTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Проверяем что заголовок агента добавлен
	ASSERT_TRUE(this->_headers->addDefaultHeaders());
	// Проверяем что заголовок агента на месте
	ASSERT_TRUE(this->_headers->has("User-Agent"));
	// Проверяем что агент запроса несёт название операционной системы в скобках
	ASSERT_NE(this->_headers->at("User-Agent").find('('), std::string::npos);
	// Проверяем что заголовки ответа сервера запросу не достались
	ASSERT_FALSE(this->_headers->has("Server"));
	// Устанавливаем собственную идентификацию сервиса
	this->_headers->ident("ID", "NAME", "1.2.3");
	// Снимаем прежний заголовок агента
	this->_headers->erase("User-Agent");
	// Добавляем заголовки по умолчанию заново
	ASSERT_TRUE(this->_headers->addDefaultHeaders());
	// Проверяем что установленная идентификация попала в агент
	ASSERT_NE(this->_headers->at("User-Agent").find("NAME"), std::string::npos);
	// Проверяем что идентификатор и версия попали в агент
	ASSERT_NE(this->_headers->at("User-Agent").find("ID/1.2.3"), std::string::npos);
}

/**
 * @brief Метод проверки отказа установить непригодную идентификацию сервиса
 *
 */
TEST_F(HeadersFixture, IdentRejectsUnfitPartsTest){
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Устанавливаем пригодную идентификацию сервиса
	this->_headers->ident("ID", "NAME", "1.0");
	// Пробуем установить составляющие с переводом строки
	this->_headers->ident("BAD\r\nID", "BAD\nNAME", "BAD\rVERSION");
	// Проверяем что ни одна непригодная составляющая не применена
	ASSERT_EQ(this->_headers->ident().find("BAD"), std::string::npos);
	// Проверяем что прежняя идентификация сохранена
	ASSERT_NE(this->_headers->ident().find("NAME"), std::string::npos);
	// Проверяем что пустые составляющие прежние значения не сбрасывают
	this->_headers->ident("", "", "");
	// Проверяем что идентификация осталась прежней
	ASSERT_NE(this->_headers->ident().find("ID/1.0"), std::string::npos);
	// Проверяем что непригодной оказывается только испорченная составляющая
	this->_headers->ident("NEW", "BAD\r\nNAME", "");
	// Проверяем что пригодная составляющая применена
	ASSERT_NE(this->_headers->ident().find("NEW"), std::string::npos);
	// Проверяем что непригодная составляющая осталась прежней
	ASSERT_NE(this->_headers->ident().find("NAME"), std::string::npos);
}

/**
 * @brief Метод проверки псевдозаголовков классического метода CONNECT
 *
 */
TEST_F(HeadersFixture, ConnectPseudoHeadersTest){
	// Создаём объект запроса клиента методом CONNECT
	request_t request(version_t::HTTP2, method_t::CONNECT, std::string("example.com:443"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что метод запроса на месте
	ASSERT_NE(result.find(":method: CONNECT\r\n"), std::string::npos);
	// Проверяем что авторитет собран из цели запроса
	ASSERT_NE(result.find(":authority: example.com:443\r\n"), std::string::npos);
	// Проверяем что схема классическому туннелю не положена
	ASSERT_EQ(result.find(":scheme"), std::string::npos);
	// Проверяем что путь классическому туннелю не положен
	ASSERT_EQ(result.find(":path"), std::string::npos);
}

/**
 * @brief Метод проверки псевдозаголовков расширенного метода CONNECT
 *
 */
TEST_F(HeadersFixture, ExtendedConnectCarriesProtocolTest){
	// Создаём объект запроса клиента методом CONNECT
	request_t request(version_t::HTTP2, method_t::CONNECT, std::string("https://example.com/chat"));
	// Устанавливаем протокол туннеля, поднимаемого поверх соединения
	request.protocol = "websocket";
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что метод запроса на месте
	ASSERT_NE(result.find(":method: CONNECT\r\n"), std::string::npos);
	// Проверяем что протокол туннеля передан отдельным псевдозаголовком
	ASSERT_NE(result.find(":protocol: websocket\r\n"), std::string::npos);
	// Проверяем что схема расширенному туннелю положена наравне с обычным запросом
	ASSERT_NE(result.find(":scheme: https\r\n"), std::string::npos);
	// Проверяем что путь расширенному туннелю положен
	ASSERT_NE(result.find(":path: /chat\r\n"), std::string::npos);
	// Проверяем что авторитет извлечён из адреса
	ASSERT_NE(result.find(":authority: example.com\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки подстановки корневого пути и схемы по умолчанию
 *
 */
TEST_F(HeadersFixture, PathAndSchemeDefaultsTest){
	// Перечень адресов запроса и ожидаемых значений пути
	const std::vector <std::pair <std::string, std::string>> samples = {
		{"https://example.com", "/"},
		{"https://example.com?q=1", "/?q=1"},
		{"https://example.com#frag", "/"},
		{"https://example.com/a/b?q=1#frag", "/a/b?q=1"},
		{"http://example.com/", "/"}
	};
	/**
	 * Перебираем все адреса запроса
	 */
	for(const auto & item : samples){
		// Создаём объект запроса клиента с очередным адресом
		request_t request(version_t::HTTP2, method_t::GET, item.first);
		// Устанавливаем провайдер запроса
		this->_headers->provider(&request);
		// Получаем вид сообщения под протокол HTTP/2
		const std::string result = this->_headers->print(proto_t::HTTP2);
		// Проверяем что путь собран ожидаемым образом
		ASSERT_NE(result.find(":path: " + item.second + "\r\n"), std::string::npos) << item.first;
		// Проверяем что якорь на провод не уехал
		ASSERT_EQ(result.find("frag"), std::string::npos) << item.first;
	}
	// Создаём объект запроса клиента в origin-форме, схемы в себе не несущей
	request_t request(version_t::HTTP2, method_t::GET, std::string("/only/path"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Проверяем что подставлена схема по умолчанию
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":scheme: https\r\n"), std::string::npos);
	// Проверяем что путь взят из адреса как есть
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":path: /only/path\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки сборки строки состояния ответа сервера
 *
 */
TEST_F(HeadersFixture, ResponseStartlineTest){
	// Создаём объект ответа сервера без собственного сообщения
	response_t response(version_t::HTTP1_1, 404, std::string(""));
	// Устанавливаем провайдер ответа
	this->_headers->provider(&response);
	// Проверяем что сообщение подставлено по коду ответа
	ASSERT_EQ(this->_headers->startline(), "HTTP/1.1 404 Not Found");
	// Создаём объект ответа сервера с собственным сообщением
	response_t custom(version_t::HTTP1_1, 404, std::string("Ничего не найдено"));
	// Устанавливаем провайдер ответа
	this->_headers->provider(&custom);
	// Проверяем что собственное сообщение сохранено
	ASSERT_EQ(this->_headers->startline(), "HTTP/1.1 404 Ничего не найдено");
	// Создаём объект ответа сервера с кодом, которому сообщения не положено
	response_t unknown(version_t::HTTP1_1, 799, std::string(""));
	// Устанавливаем провайдер ответа
	this->_headers->provider(&unknown);
	// Проверяем что строка состояния собрана и без сообщения
	ASSERT_EQ(this->_headers->startline(), "HTTP/1.1 799 ");
	// Проверяем что печать несёт собранную строку состояния
	ASSERT_NE(this->_headers->print(proto_t::HTTP1).find("HTTP/1.1 799"), std::string::npos);
}

/**
 * @brief Метод проверки псевдозаголовка кода ответа сервера
 *
 */
TEST_F(HeadersFixture, ResponseStatusPseudoHeaderTest){
	// Перечень проверяемых кодов ответа сервера
	const std::vector <uint16_t> codes = {100, 200, 301, 404, 500, 599};
	/**
	 * Перебираем все проверяемые коды ответа сервера
	 */
	for(const auto code : codes){
		// Создаём объект ответа сервера с очередным кодом
		response_t response(version_t::HTTP2, code, std::string("OK"));
		// Устанавливаем провайдер ответа
		this->_headers->provider(&response);
		// Проверяем что код ответа передан псевдозаголовком
		ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":status: " + std::to_string(code) + "\r\n"), std::string::npos);
	}
}

/**
 * @brief Метод проверки сравнения контейнеров по объекту провайдера
 *
 */
TEST_F(HeadersFixture, ProviderParticipatesInEqualityTest){
	// Создаём второй контейнер заголовков
	headers_t other(this->_fmk.get(), this->_log.get());
	// Проверяем что два пустых контейнера без провайдера равны
	ASSERT_TRUE((* this->_headers) == other);
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, std::string("/x"));
	// Устанавливаем провайдер запроса первому контейнеру
	this->_headers->provider(&request);
	// Проверяем что контейнер с провайдером не равен контейнеру без него
	ASSERT_FALSE((* this->_headers) == other);
	// Проверяем что сравнение несимметричным не является
	ASSERT_FALSE(other == (* this->_headers));
	// Устанавливаем тот же провайдер второму контейнеру
	other.provider(&request);
	// Проверяем что контейнеры с одинаковым провайдером равны
	ASSERT_TRUE((* this->_headers) == other);
	// Создаём объект ответа сервера
	response_t response(version_t::HTTP1_1, 200, std::string("OK"));
	// Заменяем провайдер второго контейнера ответом сервера
	other.provider(&response);
	// Проверяем что запрос и ответ равными не являются
	ASSERT_FALSE((* this->_headers) == other);
	// Заменяем провайдер первого контейнера тем же ответом сервера
	this->_headers->provider(&response);
	// Проверяем что два одинаковых ответа сервера равны
	ASSERT_TRUE((* this->_headers) == other);
	// Создаём объект ответа сервера с другим кодом
	response_t another(version_t::HTTP1_1, 404, std::string("OK"));
	// Заменяем провайдер второго контейнера
	other.provider(&another);
	// Проверяем что ответы с разными кодами равными не являются
	ASSERT_FALSE((* this->_headers) == other);
}

/**
 * @brief Метод проверки удаления заголовка по итератору
 *
 */
TEST_F(HeadersFixture, EraseByIteratorAccountsMemoryTest){
	// Добавляем три заголовка
	this->_headers->emplace("X-First", "1");
	// Добавляем второй заголовок
	this->_headers->emplace("X-Second", "22");
	// Добавляем третий заголовок
	this->_headers->emplace("X-Third", "333");
	// Запоминаем объём занимаемой памяти до удаления
	const size_t memory = this->_headers->memory();
	// Удаляем средний заголовок по итератору
	auto next = this->_headers->erase(this->_headers->find("X-Second"));
	// Проверяем что удалённого заголовка в наборе больше нет
	ASSERT_FALSE(this->_headers->has("X-Second"));
	// Проверяем что учёт памяти уменьшен ровно на объём удалённого заголовка
	ASSERT_EQ(this->_headers->memory(), memory - (std::string("X-Second").size() + std::string("22").size()));
	// Проверяем что возвращён итератор следующего заголовка
	ASSERT_EQ(next->name, "X-Third");
	// Проверяем что удаление по конечному итератору набор не меняет
	this->_headers->erase(this->_headers->end());
	// Проверяем что размер набора сохранён
	ASSERT_EQ(this->_headers->size(), 2);
	// Проверяем что поиск отсутствующего заголовка даёт конечный итератор
	ASSERT_TRUE(this->_headers->find("X-Missing") == this->_headers->end());
}

/**
 * @brief Метод проверки перечня названий с учётом кратности и регистра
 *
 */
TEST_F(HeadersFixture, NamesAreUniqueTest){
	// Добавляем два одноимённых заголовка, различающихся регистром названия
	this->_headers->emplace("Set-Cookie", "a=1", headers_t::mode_t::APPEND);
	// Добавляем второй заголовок того же названия в другом регистре
	this->_headers->emplace("SET-COOKIE", "b=2", headers_t::mode_t::APPEND);
	// Добавляем заголовок другого названия
	this->_headers->emplace("X-Custom", "value", headers_t::mode_t::APPEND);
	// Получаем перечень названий заголовков
	const std::vector <std::string> names = this->_headers->names();
	// Проверяем что одноимённые названия сведены в одно
	ASSERT_EQ(names.size(), 2);
	// Проверяем что кратность одноимённых заголовков сохранена
	ASSERT_EQ(this->_headers->count("set-cookie"), 2);
	// Получаем перечень значений одноимённых заголовков
	const std::vector <std::string> values = this->_headers->range("SeT-CooKie");
	// Проверяем что оба значения на месте
	ASSERT_EQ(values.size(), 2);
	// Проверяем что порядок значений сохранён
	ASSERT_EQ(values.front(), "a=1");
	// Проверяем что второе значение на своём месте
	ASSERT_EQ(values.back(), "b=2");
	// Проверяем что перечень значений отсутствующего заголовка пуст
	ASSERT_TRUE(this->_headers->range("X-Missing").empty());
}

/**
 * @brief Метод проверки присваивания мультикарты заголовков
 *
 */
TEST_F(HeadersFixture, MultimapAssignmentTest){
	// Собираем мультикарту заголовков
	headers_t::multimap_t source;
	// Добавляем два одноимённых заголовка
	source.emplace("Set-Cookie", "a=1");
	// Добавляем второй одноимённый заголовок
	source.emplace("Set-Cookie", "b=2");
	// Добавляем заголовок другого названия
	source.emplace("X-Custom", "value");
	// Присваиваем мультикарту контейнеру
	(* this->_headers) = source;
	// Проверяем что все записи попали в набор
	ASSERT_EQ(this->_headers->size(), 3);
	// Проверяем что кратность одноимённых заголовков сохранена
	ASSERT_EQ(this->_headers->count("Set-Cookie"), 2);
	// Проверяем что протокол определён как HTTP/1: псевдозаголовков среди записей нет
	ASSERT_EQ(this->_headers->proto(), proto_t::HTTP1);
	// Собираем мультикарту с псевдозаголовком
	headers_t::multimap_t binary;
	// Добавляем псевдозаголовок метода запроса
	binary.emplace(":method", "GET");
	// Присваиваем мультикарту контейнеру
	(* this->_headers) = binary;
	// Проверяем что протокол определён как HTTP/2
	ASSERT_EQ(this->_headers->proto(), proto_t::HTTP2);
	// Присваиваем пустую мультикарту
	(* this->_headers) = headers_t::multimap_t{};
	// Проверяем что набор опустел
	ASSERT_TRUE(this->_headers->empty());
	// Проверяем что настроенный протокол семейства HTTP/2 присваиванием не понижен
	ASSERT_EQ(this->_headers->proto(), proto_t::HTTP2);
}

/**
 * @brief Метод проверки текстового представления версий протокола
 *
 */
TEST_F(HeadersFixture, VersionRenderingTest){
	/**
	 * @brief Функция получения стартовой строки для указанной версии протокола
	 *
	 * @param version версия протокола ответа сервера
	 * @return        стартовая строка ответа сервера
	 *
	 */
	auto startline = [this](const version_t version) noexcept -> std::string {
		// Создаём объект ответа сервера указанной версии
		response_t response(version, 200, "OK");
		// Устанавливаем провайдер ответа
		this->_headers->provider(&response);
		// Выводим сформированную стартовую строку
		return this->_headers->startline();
	};
	// Проверяем текстовое представление версии HTTP/1.0
	ASSERT_EQ(startline(version_t::HTTP1_0), "HTTP/1.0 200 OK");
	// Проверяем текстовое представление версии HTTP/1.1
	ASSERT_EQ(startline(version_t::HTTP1_1), "HTTP/1.1 200 OK");
	// Проверяем текстовое представление версии HTTP/2
	ASSERT_EQ(startline(version_t::HTTP2), "HTTP/2.0 200 OK");
	// Проверяем текстовое представление версии HTTP/3
	ASSERT_EQ(startline(version_t::HTTP3), "HTTP/3.0 200 OK");
	// Проверяем текстовое представление версии HTTP/4
	ASSERT_EQ(startline(version_t::HTTP4), "HTTP/4.0 200 OK");
	// Проверяем текстовое представление версии HTTP/5
	ASSERT_EQ(startline(version_t::HTTP5), "HTTP/5.0 200 OK");
	// Проверяем текстовое представление неустановленной версии
	ASSERT_EQ(startline(version_t::NONE), "HTTP/0.0 200 OK");
}

/**
 * @brief Метод проверки разбора версий старших семейств протокола
 *
 */
TEST_F(HeadersFixture, HigherVersionParsingTest){
	// Устанавливаем строку состояния с версией HTTP/4
	this->_headers->startline("HTTP/4 200 OK");
	// Проверяем что версия HTTP/4 разобрана
	ASSERT_TRUE(this->_headers->provider() != nullptr);
	// Проверяем что версия сохранена верно
	ASSERT_EQ(this->_headers->startline(), "HTTP/4.0 200 OK");
	// Устанавливаем строку состояния с версией HTTP/5
	this->_headers->startline("HTTP/5.0 200 OK");
	// Проверяем что версия HTTP/5 разобрана
	ASSERT_EQ(this->_headers->startline(), "HTTP/5.0 200 OK");
	// Устанавливаем строку состояния с несуществующим семейством протокола
	this->_headers->startline("HTTP/6 200 OK");
	// Проверяем что несуществующее семейство отвергнуто
	ASSERT_TRUE(this->_headers->provider() == nullptr);
}

/**
 * @brief Метод проверки отбрасывания якоря из компонента авторитета
 *
 */
TEST_F(HeadersFixture, AuthorityFragmentCleanupTest){
	// Создаём объект запроса клиента классическим методом CONNECT с якорем в цели
	request_t request(version_t::HTTP2, method_t::CONNECT, std::string("example.com:443#frag"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Проверяем что якорь отброшен и из цели метода CONNECT
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":authority: example.com:443\r\n"), std::string::npos);
	// Создаём объект запроса клиента
	request_t plain(version_t::HTTP2, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&plain);
	// Добавляем заголовок адресата с якорем
	this->_headers->emplace("Host", "example.com#frag");
	// Проверяем что якорь отброшен и при переносе заголовка адресата
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":authority: example.com\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки разбора абсолютного URI-адреса без пути и строки запроса
 *
 */
TEST_F(HeadersFixture, AbsoluteUriWithoutPathTest){
	// Создаём объект запроса клиента с URI-адресом из одной схемы и авторитета
	request_t request(version_t::HTTP2, method_t::GET, std::string("https://example.com"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что авторитет занял остаток строки
	ASSERT_NE(result.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что путь принял корневое значение
	ASSERT_NE(result.find(":path: /\r\n"), std::string::npos);
	// Проверяем что схема взята из URI-адреса
	ASSERT_NE(result.find(":scheme: https\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки сравнения контейнеров с провайдерами ответа
 *
 */
TEST_F(HeadersFixture, EqualityWithResponseProvidersTest){
	// Создаём первый контейнер заголовков
	Headers first(this->_fmk.get(), this->_log.get());
	// Создаём второй контейнер заголовков
	Headers second(this->_fmk.get(), this->_log.get());
	// Создаём объект ответа сервера
	response_t response(version_t::HTTP1_1, 200, "OK");
	// Устанавливаем провайдер ответа первому контейнеру
	first.provider(&response);
	// Устанавливаем такой же провайдер второму контейнеру
	second.provider(&response);
	// Проверяем что контейнеры с одинаковыми ответами равны
	ASSERT_TRUE(first == second);
	// Создаём объект ответа сервера с другим кодом
	response_t other(version_t::HTTP1_1, 404, "Not Found");
	// Устанавливаем провайдер второму контейнеру
	second.provider(&other);
	// Проверяем что контейнеры с разными кодами ответа не равны
	ASSERT_FALSE(first == second);
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса второму контейнеру
	second.provider(&request);
	// Проверяем что контейнеры с разными направлениями трафика не равны
	ASSERT_FALSE(first == second);
}

/**
 * @brief Метод проверки константных итераторов и их сравнения с изменяемыми
 *
 */
TEST_F(HeadersFixture, IteratorComparisonTest){
	// Добавляем заголовок
	this->_headers->emplace("Host", "example.com");
	// Добавляем ещё один заголовок
	this->_headers->emplace("Accept", "text/html");
	// Получаем константный итератор начала набора
	headers_t::const_iterator_t first = this->_headers->cbegin();
	// Получаем константный итератор конца набора
	const headers_t::const_iterator_t last = this->_headers->cend();
	// Проверяем что константный итератор начала не совпадает с итератором конца
	ASSERT_TRUE(first != last);
	// Проверяем чтение заголовка через константный итератор
	ASSERT_EQ(first->name, "Host");
	// Проверяем разыменование константного итератора
	ASSERT_EQ((* first).value, "example.com");
	// Получаем изменяемый итератор начала набора
	headers_t::iterator_t mutable_ = this->_headers->begin();
	// Проверяем разыменование изменяемого итератора
	ASSERT_EQ((* mutable_).name, "Host");
	// Проверяем сравнение изменяемого итератора с константным
	ASSERT_TRUE(mutable_ == first);
	// Проверяем сравнение константного итератора с изменяемым
	ASSERT_TRUE(first == mutable_);
	// Смещаем константный итератор к следующему заголовку
	++first;
	// Проверяем неравенство изменяемого итератора константному
	ASSERT_TRUE(mutable_ != first);
	// Проверяем неравенство константного итератора изменяемому
	ASSERT_TRUE(first != mutable_);
	// Проверяем что константные итераторы одного положения равны
	ASSERT_TRUE(first == this->_headers->find("Accept"));
	// Проверяем поиск отсутствующего заголовка константным итератором
	ASSERT_TRUE(this->_headers->find("X-Missing") == last);
	// Проверяем преобразование константного итератора в сырой
	ASSERT_EQ(static_cast <headers_t::fields_t::const_iterator> (first)->name, "Accept");
}

/**
 * @brief Метод проверки печати заголовка по названию в формате протокола контейнера
 *
 */
TEST_F(HeadersFixture, PrintNamedUsesContainerProtoTest){
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP2);
	// Добавляем заголовок
	this->_headers->emplace("Accept", "text/html");
	// Проверяем что печать по названию без указания протокола идёт в формате контейнера
	ASSERT_EQ(this->_headers->print("Accept"), "accept: text/html\r\n");
	// Устанавливаем протокол HTTP/1.1
	this->_headers->proto(proto_t::HTTP1);
	// Проверяем что формат печати следует за протоколом контейнера
	ASSERT_EQ(this->_headers->print("Accept"), "Accept: text/html\r\n");
}

/**
 * @brief Метод проверки определения протокола при присваивании прочих форм набора
 *
 */
TEST_F(HeadersFixture, ProtoDetectionFormsTest){
	// Создаём контейнер заголовков для присваивания набора
	Headers entries(this->_fmk.get(), this->_log.get());
	// Присваиваем набор заголовков с псевдозаголовком
	entries = headers_t::entries_t {
		headers_t::header_t().from(":method", "GET"),
		headers_t::header_t().from("Accept", "text/html")
	};
	// Проверяем что протокол определён по составу набора
	ASSERT_EQ(entries.proto(), proto_t::HTTP2);
	// Создаём контейнер заголовков для присваивания списка инициализации
	Headers list(this->_fmk.get(), this->_log.get());
	// Присваиваем список инициализации с псевдозаголовком
	list = {
		headers_t::header_t().from(":status", "200"),
		headers_t::header_t().from("Server", "awh")
	};
	// Проверяем что протокол определён по составу списка
	ASSERT_EQ(list.proto(), proto_t::HTTP2);
	// Создаём контейнер заголовков с псевдозаголовками через конструктор
	Headers built(headers_t::fields_t {headers_t::header_t().from(":method", "GET")}, this->_fmk.get(), this->_log.get());
	// Проверяем что протокол определён конструктором
	ASSERT_EQ(built.proto(), proto_t::HTTP2);
}

/**
 * @brief Метод проверки общего количества заголовков и слияния без изменений
 *
 */
TEST_F(HeadersFixture, CountAndEmptyMergeTest){
	// Добавляем заголовок
	this->_headers->emplace("Set-Cookie", "a=1", headers_t::mode_t::APPEND);
	// Добавляем второе вхождение заголовка
	this->_headers->emplace("Set-Cookie", "b=2", headers_t::mode_t::APPEND);
	// Добавляем заголовок другого названия
	this->_headers->emplace("Accept", "text/html", headers_t::mode_t::APPEND);
	// Проверяем что количество без указания названия равно общему числу заголовков
	ASSERT_EQ(this->_headers->count(""), 3u);
	// Проверяем количество заголовков указанного названия
	ASSERT_EQ(this->_headers->count("Set-Cookie"), 2u);
	// Создаём пустой контейнер заголовков
	Headers empty(this->_fmk.get(), this->_log.get());
	// Выполняем слияние с пустым контейнером
	this->_headers->merge(empty);
	// Проверяем что слияние с пустым контейнером набор не изменило
	ASSERT_EQ(this->_headers->size(), 3u);
	// Выполняем слияние контейнера с самим собой
	this->_headers->merge(* this->_headers);
	// Проверяем что слияние с самим собой набор не изменило
	ASSERT_EQ(this->_headers->size(), 3u);
}

/**
 * @brief Метод проверки отказа идентификации сервиса с недопустимым значением
 *
 */
TEST_F(HeadersFixture, IdentRejectsControlCharactersTest){
	// Создаём объект ответа сервера
	response_t response(version_t::HTTP1_1, 200, "OK");
	// Устанавливаем провайдер ответа
	this->_headers->provider(&response);
	// Устанавливаем корректную идентификацию сервиса
	this->_headers->ident("SvcID", "SvcName", "1.0.0");
	// Запоминаем сформированную идентификацию сервиса
	const std::string ident = this->_headers->ident();
	// Устанавливаем идентификацию сервиса с переводом строки в названии
	this->_headers->ident("SvcID", "Svc\r\nInjected", "1.0.0");
	/**
	 * Проверяем что недопустимое значение отвергнуто: перевод строки в значении
	 * расщепил бы сообщение на два, а идентификация формируется самой библиотекой
	 */
	ASSERT_EQ(this->_headers->ident(), ident);
	// Устанавливаем идентификацию сервиса с управляющим символом в версии
	this->_headers->ident("SvcID", "SvcName", "1.0\x01");
	// Проверяем что управляющий символ также отвергнут
	ASSERT_EQ(this->_headers->ident(), ident);
}

/**
 * @brief Метод проверки разбора метода запроса из знаков препинания
 *
 */
TEST_F(HeadersFixture, PunctuationMethodTest){
	// Устанавливаем стартовую строку с методом из разрешённых знаков препинания
	this->_headers->startline("X-M!*ETHOD~ /path HTTP/1.1");
	// Проверяем что метод принят: разрешённые в токене знаки препинания допустимы
	ASSERT_TRUE(this->_headers->provider() != nullptr);
	// Проверяем что оригинальное написание метода сохранено
	ASSERT_EQ(this->_headers->startline(), "X-M!*ETHOD~ /path HTTP/1.1");
}

/**
 * @brief Метод проверки отказа по пределу количества у всех способов добавления
 *
 */
TEST_F(HeadersFixture, RecordsLimitRejectsEveryWayTest){
	// Названия и значения, передаваемые разными способами
	const std::string name = "X-Field";
	// Значение добавляемого заголовка
	const std::string value = "value";
	// Опускаем предел количества записей до одной
	this->_headers->maxRecords(1);
	// Добавляем единственный помещающийся заголовок
	this->_headers->emplace("X-Kept", "kept", headers_t::mode_t::APPEND);
	// Проверяем что заголовок добавлен
	ASSERT_EQ(this->_headers->size(), 1);
	// Добавляем заголовок переносом названия и значения
	ASSERT_EQ(this->_headers->emplace(std::string(name), std::string(value), headers_t::mode_t::APPEND), 1);
	// Добавляем заголовок переносом названия и копированием значения
	ASSERT_EQ(this->_headers->emplace(std::string(name), value, headers_t::mode_t::APPEND), 1);
	// Добавляем заголовок копированием названия и переносом значения
	ASSERT_EQ(this->_headers->emplace(name, std::string(value), headers_t::mode_t::APPEND), 1);
	// Добавляем заголовок копированием названия и значения
	ASSERT_EQ(this->_headers->emplace(name, value, headers_t::mode_t::APPEND), 1);
	// Проверяем что ни одно добавление предел не обошло
	ASSERT_EQ(this->_headers->size(), 1);
	// Проверяем что отвергнутого заголовка в наборе нет
	ASSERT_FALSE(this->_headers->has(name));
	// Проверяем что прежний заголовок не пострадал
	ASSERT_EQ(this->_headers->at("X-Kept"), "kept");
}

/**
 * @brief Метод проверки отказа по пределу памяти у всех способов добавления
 *
 */
TEST_F(HeadersFixture, MemoryLimitRejectsEveryWayTest){
	// Название добавляемого заголовка
	const std::string name = "X-Field";
	// Значение, заведомо не помещающееся в предел
	const std::string huge(200, 'z');
	// Опускаем предел памяти так, чтобы длинное значение в него не поместилось
	this->_headers->maxMemory(32);
	// Добавляем короткий заголовок, который в предел помещается
	this->_headers->emplace("X-Kept", "kept", headers_t::mode_t::APPEND);
	// Запоминаем объём занимаемой памяти
	const size_t memory = this->_headers->memory();
	// Добавляем заголовок переносом названия и значения
	this->_headers->emplace(std::string(name), std::string(huge), headers_t::mode_t::APPEND);
	// Добавляем заголовок переносом названия и копированием значения
	this->_headers->emplace(std::string(name), huge, headers_t::mode_t::APPEND);
	// Добавляем заголовок копированием названия и переносом значения
	this->_headers->emplace(name, std::string(huge), headers_t::mode_t::APPEND);
	// Добавляем заголовок копированием названия и значения
	this->_headers->emplace(name, huge, headers_t::mode_t::APPEND);
	// Проверяем что ни одно добавление предел не обошло
	ASSERT_EQ(this->_headers->size(), 1);
	// Проверяем что учёт занимаемой памяти не изменился
	ASSERT_EQ(this->_headers->memory(), memory);
	// Проверяем что предел соблюдён
	ASSERT_LE(this->_headers->memory(), this->_headers->maxMemory());
	/**
	 * Проверяем отказ, когда сама полезная нагрузка превышает предел: сложение
	 * счётчика с нагрузкой переполнилось бы и пропустило заголовок в обход проверки
	 */
	this->_headers->maxMemory(16);
	// Добавляем заголовок, чья нагрузка превышает предел целиком
	this->_headers->emplace(name, huge, headers_t::mode_t::APPEND);
	// Проверяем что заголовок отвергнут
	ASSERT_FALSE(this->_headers->has(name));
}

/**
 * @brief Метод проверки добавления заголовка с пустым названием
 *
 */
TEST_F(HeadersFixture, EmptyNameIsNotStoredTest){
	// Пустое название заголовка
	const std::string empty = "";
	// Значение добавляемого заголовка
	const std::string value = "value";
	// Добавляем заголовок переносом пустого названия и значения
	ASSERT_EQ(this->_headers->emplace(std::string(empty), std::string(value), headers_t::mode_t::APPEND), 0);
	// Добавляем заголовок переносом пустого названия и копированием значения
	ASSERT_EQ(this->_headers->emplace(std::string(empty), value, headers_t::mode_t::APPEND), 0);
	// Добавляем заголовок копированием пустого названия и переносом значения
	ASSERT_EQ(this->_headers->emplace(empty, std::string(value), headers_t::mode_t::APPEND), 0);
	// Добавляем заголовок копированием пустого названия и значения
	ASSERT_EQ(this->_headers->emplace(empty, value, headers_t::mode_t::APPEND), 0);
	// Добавляем заголовок с пустым названием в режиме замены
	ASSERT_EQ(this->_headers->emplace(empty, value, headers_t::mode_t::REPLACE), 0);
	// Проверяем что набор остался пустым
	ASSERT_TRUE(this->_headers->empty());
	// Проверяем что учёт занимаемой памяти не сдвинулся
	ASSERT_EQ(this->_headers->memory(), 0);
}

/**
 * @brief Метод проверки присваивания контейнера самому себе
 *
 */
TEST_F(HeadersFixture, SelfAssignmentKeepsSetTest){
	// Добавляем два заголовка
	this->_headers->emplace("X-First", "1");
	// Добавляем второй заголовок
	this->_headers->emplace("Set-Cookie", "a=1");
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, std::string("/x"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Запоминаем вид сообщения до присваивания
	const std::string before = this->_headers->print();
	/**
	 * Получаем ссылку и указатель на контейнер
	 *
	 * @warning Одной ССЫЛКИ здесь мало: записи вида `self = self` компилятор
	 *          отсеивает разбором (`-Wself-assign-overloaded`) и через ссылку тоже,
	 *          считая её опиской. Проверке же нужна именно она, но в работе, оттого
	 *          присваивание идёт через УКАЗАТЕЛЬ: доводы делаются разными на вид и
	 *          остаются одним объектом на деле
	 */
	headers_t & self = (* this->_headers);
	headers_t * alias = this->_headers.get();
	// Присваиваем контейнер самому себе копированием
	(* alias) = self;
	// Проверяем что набор сохранён
	ASSERT_EQ(this->_headers->size(), 2);
	// Проверяем что вид сообщения не изменился
	ASSERT_EQ(this->_headers->print(), before);
	// Проверяем что провайдер на месте
	ASSERT_NE(this->_headers->provider(), nullptr);
	// Присваиваем контейнер самому себе переносом - тем же обходом разбора
	(* alias) = ::std::move(self);
	// Проверяем что набор сохранён и после переноса
	ASSERT_EQ(this->_headers->size(), 2);
	// Проверяем что вид сообщения не изменился
	ASSERT_EQ(this->_headers->print(), before);
	// Выполняем слияние контейнера с самим собой
	this->_headers->merge(self);
	// Проверяем что слияние с самим собой набор не удвоило
	ASSERT_EQ(this->_headers->size(), 2);
}

/**
 * @brief Метод проверки присваивания списка инициализации
 *
 */
TEST_F(HeadersFixture, InitializerListAssignmentTest){
	// Присваиваем контейнеру список инициализации из обычных заголовков
	(* this->_headers) = {this->header("X-First", "1"), this->header("Set-Cookie", "a=1"), this->header("Set-Cookie", "b=2")};
	// Проверяем что все записи попали в набор
	ASSERT_EQ(this->_headers->size(), 3);
	// Проверяем что кратность одноимённых заголовков сохранена
	ASSERT_EQ(this->_headers->count("Set-Cookie"), 2);
	// Проверяем что протокол определён как HTTP/1
	ASSERT_EQ(this->_headers->proto(), proto_t::HTTP1);
	// Присваиваем контейнеру пустой список инициализации
	(* this->_headers) = std::initializer_list <headers_t::header_t> {};
	// Проверяем что набор опустел
	ASSERT_TRUE(this->_headers->empty());
	// Создаём отдельный контейнер для проверки обнаружения по псевдозаголовку
	headers_t binary(this->_fmk.get(), this->_log.get());
	// Присваиваем список инициализации с псевдозаголовком
	binary = {this->header(":method", "GET"), this->header("X-Custom", "value")};
	// Проверяем что протокол определён как HTTP/2
	ASSERT_EQ(binary.proto(), proto_t::HTTP2);
	// Проверяем что названия приведены к нижнему регистру нового протокола
	ASSERT_TRUE(binary.has("x-custom"));
}

/**
 * @brief Метод проверки присваивания пустых наборов
 *
 */
TEST_F(HeadersFixture, EmptyAssignmentResetsSetTest){
	// Добавляем заголовок, который присваивание обязано снять
	this->_headers->emplace("X-First", "1");
	// Присваиваем пустой список полей
	(* this->_headers) = headers_t::fields_t{};
	// Проверяем что набор опустел
	ASSERT_TRUE(this->_headers->empty());
	// Проверяем что протокол сброшен: обнаруживать его не по чему
	ASSERT_EQ(this->_headers->proto(), proto_t::NONE);
	// Добавляем заголовок заново
	this->_headers->emplace("X-First", "1");
	// Присваиваем пустой набор заголовков
	(* this->_headers) = headers_t::entries_t{};
	// Проверяем что набор опустел
	ASSERT_TRUE(this->_headers->empty());
	// Проверяем что протокол сброшен
	ASSERT_EQ(this->_headers->proto(), proto_t::NONE);
	// Проверяем что учёт занимаемой памяти сброшен
	ASSERT_EQ(this->_headers->memory(), 0);
}

/**
 * @brief Метод проверки повторной установки того же протокола
 *
 */
TEST_F(HeadersFixture, SameProtocolIsNotReappliedTest){
	// Добавляем заголовок с названием в нестандартном регистре
	this->_headers->emplace("X-CUSTOM", "value");
	// Проверяем что название приведено к каноническому виду при добавлении
	ASSERT_EQ(this->_headers->begin()->name, "X-Custom");
	// Устанавливаем протокол HTTP/1
	this->_headers->proto(proto_t::HTTP1);
	// Запоминаем название заголовка
	const std::string name = this->_headers->begin()->name;
	// Устанавливаем тот же протокол повторно
	this->_headers->proto(proto_t::HTTP1);
	// Проверяем что название не изменилось
	ASSERT_EQ(this->_headers->begin()->name, name);
	// Переводим контейнер на протокол HTTP/2
	this->_headers->proto(proto_t::HTTP2);
	// Проверяем что название приведено к нижнему регистру
	ASSERT_EQ(this->_headers->begin()->name, "x-custom");
	// Возвращаем контейнер на протокол HTTP/1
	this->_headers->proto(proto_t::HTTP1);
	// Проверяем что название возвращено к «умному» регистру
	ASSERT_EQ(this->_headers->begin()->name, "X-Custom");
}

/**
 * @brief Метод проверки доступа к константному контейнеру заголовков
 *
 */
TEST_F(HeadersFixture, ConstContainerAccessTest){
	// Добавляем заголовок
	this->_headers->emplace("Host", "example.com");
	// Добавляем ещё один заголовок
	this->_headers->emplace("Accept", "text/html");
	// Получаем константную ссылку на контейнер заголовков
	const headers_t & headers = (* this->_headers);
	// Выполняем поиск заголовка по названию у константного контейнера
	const headers_t::const_iterator_t found = headers.find("Accept");
	// Проверяем что заголовок найден
	ASSERT_TRUE(found != headers.cend());
	// Проверяем значение найденного заголовка
	ASSERT_EQ(found->value, "text/html");
	// Проверяем что поиск отсутствующего заголовка даёт итератор конца
	ASSERT_TRUE(headers.find("X-Missing") == headers.cend());
	// Проверяем что перебор константного контейнера начинается с первого заголовка
	ASSERT_EQ(headers.cbegin()->name, "Host");
	// Проверяем сравнение двух константных итераторов одного положения
	ASSERT_TRUE(headers.cbegin() == headers.begin());
	// Извлекаем копию объекта провайдера у контейнера без провайдера
	std::unique_ptr <provider_t> provider = nullptr;
	// Проверяем что извлечение копии провайдера без провайдера не выполняется
	ASSERT_FALSE(headers.provider(provider));
	// Проверяем что объект провайдера остался пустым
	ASSERT_TRUE(provider == nullptr);
	// Создаём объект запроса клиента
	request_t request(version_t::HTTP1_1, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Извлекаем копию объекта провайдера
	ASSERT_TRUE(headers.provider(provider));
	// Проверяем что копия объекта провайдера получена
	ASSERT_TRUE(provider != nullptr);
}

/**
 * @brief Метод проверки табуляции в значении идентификации сервиса
 *
 */
TEST_F(HeadersFixture, IdentAllowsTabulationTest){
	// Создаём объект запроса клиента: его идентификация несёт название сервиса
	request_t request(version_t::HTTP1_1, method_t::GET, std::string("/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Устанавливаем идентификацию сервиса с табуляцией в названии
	this->_headers->ident("SvcID", "Svc\tName", "1.0.0");
	/**
	 * Проверяем что табуляция принята: она разрешена в значении поля наравне
	 * с пробелом (RFC 9110 §5.5), в отличие от прочих управляющих символов
	 */
	ASSERT_NE(this->_headers->ident().find("Svc\tName"), std::string::npos);
}

/**
 * @brief Метод проверки стартовой строки с пустым методом запроса
 *
 */
TEST_F(HeadersFixture, StartlineEmptyMethodTest){
	// Устанавливаем корректную стартовую строку запроса
	this->_headers->startline("GET / HTTP/1.1");
	// Проверяем что провайдер запроса сформирован
	ASSERT_TRUE(this->_headers->provider() != nullptr);
	// Устанавливаем стартовую строку, начинающуюся с пробела
	this->_headers->startline(" / HTTP/1.1");
	/**
	 * Проверяем что запись отвергнута: метод запроса пуст, а пустой участок
	 * токеном не является и методом быть не может
	 */
	ASSERT_TRUE(this->_headers->provider() == nullptr);
}

/**
 * @brief Метод проверки приведения названия к каноническому регистру
 *
 */
TEST_F(HeadersFixture, NameCasingCoversSeparatorsTest){
	// Перечень названий и их ожидаемого вида в «умном» регистре
	const std::vector <std::pair <std::string, std::string>> samples = {
		{"content-type", "Content-Type"},
		{"CONTENT-TYPE", "Content-Type"},
		{"x_custom_field", "X_Custom_Field"},
		{"x custom field", "X Custom Field"},
		{"x-1st-field", "X-1st-Field"},
		{"--double", "--Double"},
		{"_", "_"},
		{"1header", "1header"},
		{"x-\tfield", "X-\tField"}
	};
	/**
	 * Перебираем все проверяемые названия заголовков
	 */
	for(const auto & item : samples){
		// Очищаем набор перед очередной проверкой
		this->_headers->clear();
		// Устанавливаем протокол HTTP/1, при котором применяется «умный» регистр
		this->_headers->proto(proto_t::HTTP1);
		// Добавляем заголовок с очередным названием
		this->_headers->emplace(item.first, "value");
		// Проверяем что название приведено к ожидаемому виду
		ASSERT_EQ(this->_headers->begin()->name, item.second) << item.first;
		// Переводим контейнер на протокол HTTP/2
		this->_headers->proto(proto_t::HTTP2);
		// Собираем ожидаемое название в нижнем регистре
		std::string lower = item.first;
		// Приводим ожидаемое название к нижнему регистру
		for(auto & letter : lower)
			// Приводим очередную букву к нижнему регистру
			letter = static_cast <char> (((letter >= 'A') && (letter <= 'Z')) ? (letter + 32) : letter);
		// Проверяем что для бинарного протокола название приведено к нижнему регистру
		ASSERT_EQ(this->_headers->begin()->name, lower) << item.first;
	}
}

/**
 * @brief Метод проверки разбора перечня полей одного участка передачи
 *
 */
TEST_F(HeadersFixture, ConnectionListParsingTest){
	// Перечень значений заголовка управления соединением
	const std::vector <std::string> samples = {
		"X-Hop", " X-Hop ", "X-Hop,", ",X-Hop", "X-Hop,,X-Other",
		"X-Other, X-Hop", "  ,  X-Hop  ,  ", "x-hop"
	};
	/**
	 * Перебираем все значения заголовка управления соединением
	 */
	for(const auto & value : samples){
		// Очищаем набор перед очередной проверкой
		this->_headers->clear();
		// Добавляем заголовок управления соединением с очередным значением
		this->_headers->emplace("Connection", value);
		// Добавляем перечисленное в нём поле
		this->_headers->emplace("X-Hop", "hop");
		// Добавляем поле, в перечень не входящее
		this->_headers->emplace("X-Kept", "kept");
		// Получаем вид сообщения под протокол HTTP/2
		const std::string result = this->_headers->print(proto_t::HTTP2);
		// Проверяем что перечисленное поле отсеяно
		ASSERT_EQ(result.find("hop"), std::string::npos) << value;
		// Проверяем что не перечисленное поле сохранено
		ASSERT_NE(result.find("kept"), std::string::npos) << value;
	}
	// Очищаем набор перед проверкой пустого перечня
	this->_headers->clear();
	// Добавляем заголовок управления соединением с пустым значением
	this->_headers->emplace("Connection", "");
	// Добавляем обычное поле
	this->_headers->emplace("X-Kept", "kept");
	// Проверяем что пустой перечень ничего не отсеивает
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find("kept"), std::string::npos);
}

/**
 * @brief Метод проверки поля расширений передачи с разными значениями
 *
 */
TEST_F(HeadersFixture, TransferExtensionValuesTest){
	// Перечень значений поля расширений передачи и признака их сохранения
	const std::vector <std::pair <std::string, bool>> samples = {
		{"trailers", true}, {"  trailers  ", true}, {"TRAILERS", true},
		{"gzip", false}, {"trailers, deflate", false}, {"", false}, {"trailer", false}
	};
	/**
	 * Перебираем все значения поля расширений передачи
	 */
	for(const auto & item : samples){
		// Очищаем набор перед очередной проверкой
		this->_headers->clear();
		// Добавляем поле расширений передачи с очередным значением
		this->_headers->emplace("TE", item.second ? item.first : (item.first.empty() ? std::string("x") : item.first));
		// Если значение поля пустое - проверять нечего, поле отсеивается по значению
		if(item.first.empty())
			// Переходим к следующему значению
			continue;
		// Получаем вид сообщения под протокол HTTP/2
		const std::string result = this->_headers->print(proto_t::HTTP2);
		// Проверяем что поле сохранено либо отсеяно согласно ожиданию
		ASSERT_EQ(result.find("te:") != std::string::npos, item.second) << item.first;
	}
	// Очищаем набор перед проверкой перечисления в заголовке управления соединением
	this->_headers->clear();
	// Добавляем заголовок управления соединением, перечисляющий поле расширений
	this->_headers->emplace("Connection", "TE, close");
	// Добавляем поле расширений передачи с допустимым значением
	this->_headers->emplace("TE", "trailers");
	// Проверяем что поле пережило перечисление в заголовке управления соединением
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find("te: trailers"), std::string::npos);
	// Заменяем значение поля расширений передачи на недопустимое
	this->_headers->emplace("TE", "gzip", headers_t::mode_t::REPLACE);
	// Проверяем что поле с недопустимым значением отсеяно
	ASSERT_EQ(this->_headers->print(proto_t::HTTP2).find("te:"), std::string::npos);
}

/**
 * @brief Метод проверки разбора стартовой строки с лишними отступами
 *
 */
TEST_F(HeadersFixture, StartlineSpacingTest){
	// Устанавливаем строку состояния с несколькими пробелами между составляющими
	this->_headers->startline("HTTP/1.1   200   OK");
	// Проверяем что строка состояния разобрана
	ASSERT_EQ(this->_headers->startline(), "HTTP/1.1 200 OK");
	// Устанавливаем строку состояния без сообщения, но с завершающим пробелом
	this->_headers->startline("HTTP/1.1 204 ");
	// Проверяем что сообщение подставлено по коду ответа
	ASSERT_EQ(this->_headers->startline(), "HTTP/1.1 204 No Content");
	// Устанавливаем строку запроса с несколькими пробелами между составляющими
	this->_headers->startline("GET   /x   HTTP/1.1");
	// Проверяем что строка запроса разобрана
	ASSERT_EQ(this->_headers->startline(), "GET /x HTTP/1.1");
	// Перечень строк состояния с непригодным кодом ответа
	const std::vector <std::string> broken = {
		"HTTP/1.1 2x0 OK", "HTTP/1.1 20 OK", "HTTP/1.1 2000 OK", "HTTP/1.1 20O OK", "HTTP/1.1 -20 OK"
	};
	/**
	 * Перебираем строки состояния с непригодным кодом ответа
	 */
	for(const auto & line : broken){
		// Устанавливаем корректную строку, чтобы провайдер заведомо был установлен
		this->_headers->startline("HTTP/1.1 200 OK");
		// Устанавливаем строку состояния с непригодным кодом ответа
		this->_headers->startline(line);
		// Проверяем что провайдер сброшен
		ASSERT_EQ(this->_headers->provider(), nullptr) << line;
	}
	// Устанавливаем строку из одних пробелов после первого токена
	this->_headers->startline("HTTP/1.1    ");
	// Проверяем что провайдер сброшен
	ASSERT_EQ(this->_headers->provider(), nullptr);
	// Устанавливаем строку без второго токена
	this->_headers->startline("GET ");
	// Проверяем что провайдер сброшен
	ASSERT_EQ(this->_headers->provider(), nullptr);
	// Устанавливаем строку из одного токена, начинающегося с записи версии
	this->_headers->startline("HTTP/");
	// Проверяем что провайдер сброшен
	ASSERT_EQ(this->_headers->provider(), nullptr);
}

/**
 * @brief Метод проверки добавления в режиме дополнения у всех перегрузок
 *
 */
TEST_F(HeadersFixture, EveryEmplaceOverloadAppendsTest){
	// Название добавляемого заголовка
	const std::string name = "Set-Cookie";
	// Значение добавляемого заголовка
	const std::string value = "v";
	// Добавляем заголовок переносом названия и значения
	this->_headers->emplace(std::string(name), std::string(value), headers_t::mode_t::APPEND);
	// Добавляем заголовок названием из C-строки и переносом значения
	this->_headers->emplace(name.c_str(), std::string(value), headers_t::mode_t::APPEND);
	// Добавляем заголовок переносом названия и значением из C-строки
	this->_headers->emplace(std::string(name), value.c_str(), headers_t::mode_t::APPEND);
	// Добавляем заголовок участками строк
	this->_headers->emplace(std::string_view(name), std::string_view(value), headers_t::mode_t::APPEND);
	// Добавляем заголовок переносом названия и копированием значения
	this->_headers->emplace(std::string(name), value, headers_t::mode_t::APPEND);
	// Добавляем заголовок копированием названия и переносом значения
	this->_headers->emplace(name, std::string(value), headers_t::mode_t::APPEND);
	// Добавляем заголовок названием и значением из C-строк
	this->_headers->emplace(name.c_str(), value.c_str(), headers_t::mode_t::APPEND);
	// Добавляем заголовок названием из C-строки и копированием значения
	this->_headers->emplace(name.c_str(), value, headers_t::mode_t::APPEND);
	// Добавляем заголовок копированием названия и значением из C-строки
	this->_headers->emplace(name, value.c_str(), headers_t::mode_t::APPEND);
	// Добавляем заголовок копированием названия и значения
	this->_headers->emplace(name, value, headers_t::mode_t::APPEND);
	// Проверяем что все десять способов добавили заголовок, сохранив одноимённые
	ASSERT_EQ(this->_headers->count(name), 10);
	// Перечень пустых названий, передаваемых разными способами
	const std::string empty = "";
	// Добавляем заголовок с пустым названием из C-строки в режиме замены
	this->_headers->emplace(empty.c_str(), value.c_str(), headers_t::mode_t::REPLACE);
	// Добавляем заголовок с пустым названием из C-строки и копированием значения
	this->_headers->emplace(empty.c_str(), value, headers_t::mode_t::REPLACE);
	// Добавляем заголовок с пустым названием и значением из C-строки
	this->_headers->emplace(empty, value.c_str(), headers_t::mode_t::REPLACE);
	// Добавляем заголовок с пустым названием переносом и значением из C-строки
	this->_headers->emplace(std::string(empty), value.c_str(), headers_t::mode_t::REPLACE);
	// Добавляем заголовок с пустым названием из C-строки и переносом значения
	this->_headers->emplace(empty.c_str(), std::string(value), headers_t::mode_t::REPLACE);
	// Добавляем заголовок с пустым названием участками строк
	this->_headers->emplace(std::string_view(empty), std::string_view(value), headers_t::mode_t::REPLACE);
	// Проверяем что ни одно название не пропустило пустое имя в набор
	ASSERT_EQ(this->_headers->size(), 10);
}

/**
 * @brief Метод проверки преобразований при отсутствующем провайдере
 *
 */
TEST_F(HeadersFixture, ConversionsWithoutProviderTest){
	// Добавляем заголовок, чтобы набор не был пустым
	this->_headers->emplace("X-Custom", "value");
	// Получаем копию объекта провайдера при его отсутствии
	std::unique_ptr <provider_t> provider = static_cast <std::unique_ptr <provider_t>> (* this->_headers);
	// Проверяем что копия пуста
	ASSERT_EQ(provider, nullptr);
	// Получаем копию объекта провайдера через метод
	ASSERT_FALSE(this->_headers->provider(provider));
	// Проверяем что указатель на провайдер пуст
	ASSERT_EQ(static_cast <const provider_t *> (* this->_headers), nullptr);
	// Проверяем что стартовая строка при отсутствии провайдера пуста
	ASSERT_TRUE(this->_headers->startline().empty());
	// Проверяем что идентификация при отсутствии провайдера пуста
	ASSERT_TRUE(this->_headers->ident().empty());
	// Создаём копию контейнера без провайдера
	headers_t copy = (* this->_headers);
	// Проверяем что копия равна источнику
	ASSERT_TRUE(copy == (* this->_headers));
	// Присваиваем контейнер без провайдера копированием
	headers_t assigned(this->_fmk.get(), this->_log.get());
	// Выполняем присваивание
	assigned = (* this->_headers);
	// Проверяем что присвоенный контейнер равен источнику
	ASSERT_TRUE(assigned == (* this->_headers));
	// Проверяем что провайдер у присвоенного контейнера отсутствует
	ASSERT_EQ(assigned.provider(), nullptr);
	// Сбрасываем провайдер через установку нулевого указателя
	this->_headers->provider(static_cast <const provider_t *> (nullptr));
	// Проверяем что провайдер отсутствует
	ASSERT_EQ(this->_headers->provider(), nullptr);
}

/**
 * @brief Метод проверки создания контейнера из пустых наборов
 *
 */
TEST_F(HeadersFixture, ConstructionFromEmptyCollectionsTest){
	// Создаём контейнер из пустого списка полей
	headers_t fromFields(headers_t::fields_t{}, this->_fmk.get(), this->_log.get());
	// Проверяем что набор пуст
	ASSERT_TRUE(fromFields.empty());
	// Создаём контейнер из пустого набора заголовков
	headers_t fromEntries(headers_t::entries_t{}, this->_fmk.get(), this->_log.get());
	// Проверяем что набор пуст
	ASSERT_TRUE(fromEntries.empty());
	// Создаём контейнер из непустого списка полей
	headers_t filled(headers_t::fields_t{this->header("X-Custom", "value")}, this->_fmk.get(), this->_log.get());
	// Проверяем что заголовок попал в набор
	ASSERT_EQ(filled.size(), 1);
	// Проверяем что протокол определён по составу полей
	ASSERT_EQ(filled.proto(), proto_t::HTTP1);
	// Создаём контейнер из набора заголовков с псевдозаголовком
	headers_t binary(headers_t::entries_t{this->header(":method", "GET")}, this->_fmk.get(), this->_log.get());
	// Проверяем что протокол определён как HTTP/2
	ASSERT_EQ(binary.proto(), proto_t::HTTP2);
}

/**
 * @brief Метод проверки слияния с частичным совпадением названий
 *
 */
TEST_F(HeadersFixture, MergePartialNameMatchTest){
	// Добавляем три заголовка в текущий контейнер
	this->_headers->emplace("X-First", "1", headers_t::mode_t::APPEND);
	// Добавляем второй заголовок
	this->_headers->emplace("X-Second", "2", headers_t::mode_t::APPEND);
	// Добавляем третий заголовок
	this->_headers->emplace("X-Third", "3", headers_t::mode_t::APPEND);
	// Создаём контейнер для слияния
	headers_t source(this->_fmk.get(), this->_log.get());
	// Добавляем заголовок, совпадающий по названию со вторым
	source.emplace("X-Second", "22", headers_t::mode_t::APPEND);
	// Добавляем заголовок, ни с чем не совпадающий
	source.emplace("X-Fourth", "4", headers_t::mode_t::APPEND);
	// Выполняем слияние с заменой одноимённых
	this->_headers->merge(source, headers_t::mode_t::REPLACE);
	// Проверяем что совпавший заголовок заменён
	ASSERT_EQ(this->_headers->at("X-Second"), "22");
	// Проверяем что кратность совпавшего названия не выросла
	ASSERT_EQ(this->_headers->count("X-Second"), 1);
	// Проверяем что несовпавшие заголовки сохранены
	ASSERT_EQ(this->_headers->at("X-First"), "1");
	// Проверяем что новый заголовок добавлен
	ASSERT_EQ(this->_headers->at("X-Fourth"), "4");
	// Проверяем итоговый размер набора
	ASSERT_EQ(this->_headers->size(), 4);
	// Проверяем что слияние с пустым контейнером набор не меняет
	this->_headers->merge(headers_t(this->_fmk.get(), this->_log.get()), headers_t::mode_t::REPLACE);
	// Проверяем что размер набора сохранён
	ASSERT_EQ(this->_headers->size(), 4);
	// Опускаем предел количества записей так, чтобы слияние в него не поместилось
	this->_headers->maxRecords(4);
	// Создаём контейнер, слияние с которым предел превысит
	headers_t big(this->_fmk.get(), this->_log.get());
	// Добавляем в него заголовок с новым названием
	big.emplace("X-Fifth", "5", headers_t::mode_t::APPEND);
	// Выполняем слияние, которое поместиться не может
	this->_headers->merge(big, headers_t::mode_t::REPLACE);
	// Проверяем что слияние отвергнуто целиком
	ASSERT_EQ(this->_headers->size(), 4);
	// Проверяем что отвергнутый заголовок в набор не попал
	ASSERT_FALSE(this->_headers->has("X-Fifth"));
}

/**
 * @brief Метод проверки пробельных символов в значении поля
 *
 */
TEST_F(HeadersFixture, WhitespaceOctetsAreRecognisedTest){
	// Перечень пробельных символов, отбрасываемых по краям значения
	const std::vector <std::string> spaces = {" ", "\t", "\n", "\v", "\f", "\r"};
	/**
	 * Перебираем все пробельные символы
	 */
	for(const auto & space : spaces){
		// Очищаем набор перед очередной проверкой
		this->_headers->clear();
		// Добавляем заголовок адресата со значением в окружении пробельных символов
		this->_headers->emplace("Host", space + "example.com" + space);
		// Создаём объект запроса клиента
		request_t request(version_t::HTTP2, method_t::GET, std::string("/"));
		// Устанавливаем провайдер запроса
		this->_headers->provider(&request);
		// Проверяем что окружающие пробельные символы при переносе отброшены
		ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":authority: example.com\r\n"), std::string::npos);
	}
	// Проверяем что табуляция в идентификации сервиса допустима наравне с пробелом
	this->_headers->ident("ID", "NA\tME", "1.0");
	// Создаём объект ответа сервера
	response_t response(version_t::HTTP1_1, 200, std::string("OK"));
	// Устанавливаем провайдер ответа
	this->_headers->provider(&response);
	// Проверяем что идентификация с табуляцией применена
	ASSERT_NE(this->_headers->ident().find("ID/1.0"), std::string::npos);
}

/**
 * @brief Метод проверки направления сообщения, собранного без провайдера
 *
 */
TEST_F(HeadersFixture, ResponseWithoutProviderHasNoAuthorityTest){
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP2);
	// Присваиваем набор полей ответа сервера, несущий заголовок адресата
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":status", "200"),
		headers_t::header_t{}.from("Host", "example.com"),
		headers_t::header_t{}.from("Server", "awh")
	};
	// Проверяем что провайдер присваиванием набора сброшен
	ASSERT_TRUE(this->_headers->provider() == nullptr);
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	/**
	 * Проверяем что псевдозаголовок авторитета не сформирован: в ответе сервера
	 * допустим единственный псевдозаголовок [:status] (RFC 9113 §8.3.2), и всякий
	 * другой делает сообщение некорректным. Направление здесь определяется составом
	 * набора, поскольку провайдера нет
	 */
	ASSERT_EQ(result.find(":authority"), std::string::npos);
	/**
	 * Проверяем что заголовок адресата сохранён: снятию подлежат поля, управляющие
	 * соединением (RFC 9113 §8.2.2), а адресат к ним не отнесён. У запроса он снимается
	 * потому, что его место занимает псевдозаголовок авторитета, здесь же авторитет
	 * не формируется, и снятие молча теряло бы поле
	 */
	ASSERT_NE(result.find("host: example.com\r\n"), std::string::npos);
	// Проверяем что псевдозаголовок статуса сохранён
	ASSERT_NE(result.find(":status: 200\r\n"), std::string::npos);
	// Присваиваем набор полей запроса с тем же заголовком адресата
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":method", "GET"),
		headers_t::header_t{}.from(":path", "/"),
		headers_t::header_t{}.from("Host", "example.com")
	};
	// Проверяем что для запроса адресат по-прежнему переносится в псевдозаголовок
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":authority: example.com\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки порядка псевдозаголовков в собранном наборе
 *
 */
TEST_F(HeadersFixture, PseudoHeadersOrderWithoutProviderTest){
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP2);
	// Присваиваем набор полей, где обычное поле стоит впереди псевдозаголовков
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from("Content-Type", "text/plain"),
		headers_t::header_t{}.from(":status", "200"),
		headers_t::header_t{}.from("Server", "awh")
	};
	// Получаем список полей оператором преобразования
	const headers_t::fields_t fields = static_cast <headers_t::fields_t> (* this->_headers);
	// Проверяем что набор собран целиком
	ASSERT_EQ(fields.size(), 3u);
	/**
	 * Проверяем что псевдозаголовок выведен первым: в блоке заголовков HTTP/2
	 * и HTTP/3 псевдозаголовки обязаны предшествовать обычным полям (RFC 9113 §8.3),
	 * а в хранилище они лежат в порядке добавления
	 */
	ASSERT_EQ(fields.front().name, ":status");
	// Признак встреченного обычного поля при переборе набора
	bool plain = false;
	/**
	 * Выполняем перебор всех полей собранного набора
	 */
	for(const auto & header : fields){
		// Определяем принадлежность поля к псевдозаголовкам
		const bool pseudo = (!header.name.empty() && (header.name.front() == ':'));
		// Проверяем что псевдозаголовок не встретился после обычного поля
		ASSERT_FALSE(pseudo && plain);
		// Запоминаем встреченное обычное поле
		plain = (plain || !pseudo);
	}
	// Получаем напечатанный набор заголовков
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что и печать выводит псевдозаголовок раньше обычного поля
	ASSERT_LT(result.find(":status: 200\r\n"), result.find("content-type: text/plain\r\n"));
	// Проверяем что порядок обычных полей между собой сохранён
	ASSERT_LT(result.find("content-type: text/plain\r\n"), result.find("server: awh\r\n"));
}

/**
 * @brief Метод проверки формирования даты по отметке, не умещающейся в тип времени
 *
 * @details Отметка, не умещающаяся в знаковый тип времени, приведением становится
 *          отрицательной. Разложение выдало бы на неё правдоподобную дату вблизи
 *          начала отсчёта - сведения, которых вызывающая сторона не передавала,
 *          и они ушли бы на провод полем [Date]. Пустой результат обозначает
 *          такую отметку честно: поле с ним не формируется вовсе
 *
 */
TEST_F(HeadersFixture, DateOutOfRangeStampTest){
	// Формируем дату по отметке, не умещающейся в знаковый тип времени
	ASSERT_TRUE(this->_headers->date(std::numeric_limits <uint64_t>::max()).empty());
	// Формируем дату по отметке на единицу больше предельной для типа времени
	ASSERT_TRUE(this->_headers->date(static_cast <uint64_t> (std::numeric_limits <time_t>::max()) + 1).empty());
	// Проверяем что предельная отметка типа времени пустой результат уже не даёт
	ASSERT_FALSE(this->_headers->date(static_cast <uint64_t> (std::numeric_limits <time_t>::max())).empty());
	// Проверяем что обычная отметка по-прежнему раскладывается
	ASSERT_EQ(this->_headers->date(1000000000), "Sun, 09 Sep 2001 01:46:40 GMT");
	// Проверяем что отметка в миллисекундах приводится к секундам
	ASSERT_EQ(this->_headers->date(1000000000000), "Sun, 09 Sep 2001 01:46:40 GMT");
}

/**
 * @brief Метод проверки приведения схемы абсолютного URI-адреса к нижнему регистру
 *
 * @details Схема регистронезависима, и канонической считается запись строчными
 *          буквами (RFC 3986 §3.1). Таблица статических полей HPACK и QPACK держит
 *          [http] и [https] строчными, поэтому схема в ином регистре теряет место
 *          в таблице и уходит на провод полной записью
 *
 */
TEST_F(HeadersFixture, SchemeCaseNormalizedTest){
	// Создаём объект запроса клиента с абсолютной формой цели, где схема записана заглавными
	request_t request(version_t::HTTP2, method_t::GET, std::string("HTTPS://example.com/path"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что схема приведена к нижнему регистру
	ASSERT_NE(result.find(":scheme: https\r\n"), std::string::npos);
	// Проверяем что запись заглавными на провод не ушла
	ASSERT_EQ(result.find("HTTPS"), std::string::npos);
	// Проверяем что прочие составляющие адреса разобраны по-прежнему
	ASSERT_NE(result.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что путь отделён от схемы и авторитета
	ASSERT_NE(result.find(":path: /path\r\n"), std::string::npos);
	// Создаём объект запроса со смешанным регистром схемы обмена без шифрования
	request_t plain(version_t::HTTP2, method_t::GET, std::string("HtTp://example.com/"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&plain);
	// Проверяем что смешанный регистр также приведён к нижнему
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":scheme: http\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки приведения пути происхождённой формы цели запроса
 *
 * @details Якорь обрабатывается принимающей стороной и в запрос не отправляется
 *          вовсе (RFC 9112 §3.2.1), а псевдозаголовок пути обязан начинаться
 *          с разделителя и не бывает пустым (RFC 9113 §8.3.1). Происхождённая
 *          форма цели несёт якорь ничуть не реже абсолютной
 *
 */
TEST_F(HeadersFixture, OriginFormPathCleanupTest){
	// Создаём объект запроса клиента с происхождённой формой цели, несущей якорь
	request_t request(version_t::HTTP2, method_t::GET, std::string("/page?q=1#section"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что якорь снят, а строка запроса сохранена
	ASSERT_NE(result.find(":path: /page?q=1\r\n"), std::string::npos);
	// Проверяем что якорь на провод не ушёл
	ASSERT_EQ(result.find("section"), std::string::npos);
	// Создаём объект запроса клиента с целью из одной строки запроса
	request_t query(version_t::HTTP2, method_t::GET, std::string("?q=1"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&query);
	// Проверяем что путь дополнен корневым разделителем
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":path: /?q=1\r\n"), std::string::npos);
	// Создаём объект запроса клиента с целью из одного якоря
	request_t anchor(version_t::HTTP2, method_t::GET, std::string("#section"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&anchor);
	// Проверяем что от цели остался корневой путь: пустым псевдозаголовок пути быть не может
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":path: /\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки цели метода CONNECT, заданной абсолютным URI-адресом
 *
 * @details Цель метода CONNECT записывается authority-формой, но тип цели общий
 *          для всех запросов, и приложение вправе положить в неё абсолютный
 *          URI-адрес. Псевдозаголовок авторитета несёт только узел и порт
 *          (RFC 9113 §8.5), поэтому схема в него попасть не может
 *
 */
TEST_F(HeadersFixture, ConnectAbsoluteUriTargetTest){
	// Создаём объект запроса клиента с целью метода CONNECT, заданной абсолютным URI-адресом
	request_t request(version_t::HTTP2, method_t::CONNECT, std::string("https://example.com:443/path"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что в авторитет попали только узел и порт
	ASSERT_NE(result.find(":authority: example.com:443\r\n"), std::string::npos);
	// Проверяем что схема в авторитет не уехала
	ASSERT_EQ(result.find(":authority: https"), std::string::npos);
	// Проверяем что классический CONNECT псевдозаголовков схемы и пути не несёт
	ASSERT_EQ(result.find(":scheme"), std::string::npos);
	// Проверяем что псевдозаголовок пути также отсутствует
	ASSERT_EQ(result.find(":path"), std::string::npos);
	// Создаём объект запроса клиента с целью метода CONNECT в authority-форме
	request_t plain(version_t::HTTP2, method_t::CONNECT, std::string("example.com:443"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&plain);
	// Проверяем что authority-форма разбирается по-прежнему
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":authority: example.com:443\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки звёздочной формы цели запроса
 *
 * @details Звёздочкой метод OPTIONS обращается к серверу целиком, а не к какому-либо
 *          его ресурсу (RFC 9112 §3.2.4). Псевдозаголовок пути в таком запросе обязан
 *          нести именно звёздочку (RFC 9113 §8.3.1), и дополнение разделителем
 *          превратило бы обращение к серверу в обращение к ресурсу с названием
 *          из звёздочки. Приёмная сторона обоих бинарных протоколов эту форму
 *          проверяет, поэтому звёздочку с приставленным разделителем отвергает
 *
 */
TEST_F(HeadersFixture, AsteriskFormTargetTest){
	// Создаём объект запроса клиента со звёздочной формой цели
	request_t request(version_t::HTTP2, method_t::OPTIONS, std::string("*"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что псевдозаголовок пути несёт именно звёздочку
	ASSERT_NE(result.find(":path: *\r\n"), std::string::npos);
	// Проверяем что звёздочка не дополнена корневым разделителем
	ASSERT_EQ(result.find(":path: /*"), std::string::npos);
	// Проверяем что прочие псевдозаголовки запроса сформированы
	ASSERT_NE(result.find(":method: OPTIONS\r\n"), std::string::npos);
	// Создаём объект запроса клиента с обычной целью того же метода
	request_t regular(version_t::HTTP2, method_t::OPTIONS, std::string("/resource"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&regular);
	// Проверяем что обычная цель того же метода разбирается по-прежнему
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":path: /resource\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки доводов, указывающих внутрь самого набора
 *
 * @details Вызывающая сторона вправе передать представление либо ссылку на строку
 *          этого же набора, добытую поиском по нему. Замена набор перестраивает:
 *          снятие прежних вхождений уплотняет его, перенося сохраняемые заголовки
 *          поверх снимаемых. Довод, указывающий внутрь набора, повис бы посреди
 *          этого переноса, и заголовок собрался бы из освобождённой памяти
 *
 */
TEST_F(HeadersFixture, AliasedArgumentsTest){
	// Наполняем набор так, чтобы уплотнение переносило заголовки поверх снимаемого
	this->_headers->emplace("Host", "example.com", headers_t::mode_t::APPEND);
	// Добавляем поле, которое при уплотнении сдвинется на место снимаемого
	this->_headers->emplace("Accept", "text/plain", headers_t::mode_t::APPEND);
	// Добавляем поле, удлиняющее набор до трёх записей
	this->_headers->emplace("Server", "awh", headers_t::mode_t::APPEND);
	// Заменяем заголовок, передав название представлением на строку самого набора
	this->_headers->emplace(std::string_view(this->_headers->begin()->name), std::string_view("new.example"));
	// Проверяем что замена выполнена по нужному названию
	ASSERT_EQ(this->_headers->at("Host"), "new.example");
	// Проверяем что прочие заголовки уплотнением не пострадали
	ASSERT_EQ(this->_headers->at("Accept"), "text/plain");
	// Проверяем что последний заголовок набора на месте
	ASSERT_EQ(this->_headers->at("Server"), "awh");
	// Заменяем заголовок, передав содержимое ссылкой на значение из самого набора
	this->_headers->emplace("Accept", this->_headers->at("Server"));
	// Проверяем что содержимое перенесено верно
	ASSERT_EQ(this->_headers->at("Accept"), "awh");
	/**
	 * Запоминаем название первого заголовка набора: замена переносит заменённое поле
	 * в конец набора, поэтому первым к этому шагу оказывается не тот заголовок,
	 * с которого набор начинали наполнять
	 */
	const std::string first = this->_headers->begin()->name;
	// Снимаем заголовок, передав название представлением на строку самого набора
	this->_headers->erase(std::string_view(this->_headers->begin()->name));
	// Проверяем что снят именно первый заголовок набора
	ASSERT_FALSE(this->_headers->has(first));
	// Проверяем что уцелевшие заголовки уплотнением не испорчены
	ASSERT_EQ(this->_headers->at("Accept"), "awh");
	// Проверяем что набор уплотнён до двух записей
	ASSERT_EQ(this->_headers->size(), 2u);
}

/**
 * @brief Метод проверки псевдозаголовка авторитета, положенного в набор при провайдере
 *
 * @details Провайдер собирает авторитет только из цели запроса, заданной абсолютной
 *          формой. Для протоколов, работающих псевдозаголовками, обычен запрос,
 *          где авторитет положен полем, а заголовка адресата нет вовсе (RFC 9113 §8.3.1).
 *          Отсев псевдозаголовков набора по одному лишь наличию провайдера терял бы
 *          такой авторитет молча
 *
 */
TEST_F(HeadersFixture, StoredAuthorityWithProviderTest){
	// Создаём объект запроса клиента с целью в происхождённой форме
	request_t request(version_t::HTTP2, method_t::GET, std::string("/index.html"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Кладём авторитет полем набора, не добавляя заголовка адресата
	this->_headers->emplace(":authority", "example.com");
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что положенный в набор авторитет попал в сообщение
	ASSERT_NE(result.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что псевдозаголовки провайдера сформированы наравне с ним
	ASSERT_NE(result.find(":method: GET\r\n"), std::string::npos);
	// Проверяем что путь взят из цели запроса
	ASSERT_NE(result.find(":path: /index.html\r\n"), std::string::npos);
	// Добавляем заголовок адресата с иным значением
	this->_headers->emplace("Host", "other.example");
	// Получаем вид сообщения повторно
	const std::string mixed = this->_headers->print(proto_t::HTTP2);
	// Проверяем что победил уже заданный авторитет, а не перенос заголовка адресата
	ASSERT_NE(mixed.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что второго псевдозаголовка авторитета не появилось
	ASSERT_EQ(mixed.find(":authority: other.example"), std::string::npos);
	// Проверяем что псевдозаголовок пути от провайдера не задвоился
	ASSERT_EQ(mixed.find(":path: /index.html\r\n:path"), std::string::npos);
}

/**
 * @brief Метод проверки допуска псевдозаголовков по методу запроса
 *
 * @details Набор псевдозаголовков запроса закрыт методом: классический туннель несёт
 *          только [:method] и [:authority] (RFC 9113 §8.5), расширенный - вдобавок
 *          схему, путь и протокол туннеля (RFC 8441 §4), обычный запрос протокола
 *          туннеля не несёт вовсе. Псевдозаголовок, методу не положенный, обязывает
 *          принимающую сторону отвергнуть сообщение целиком
 *
 */
TEST_F(HeadersFixture, PseudoHeadersAllowedByMethodTest){
	// Создаём объект запроса клиента с классическим методом туннеля
	request_t tunnel(version_t::HTTP2, method_t::CONNECT, std::string("example.com:443"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&tunnel);
	// Кладём в набор псевдозаголовки, классическому туннелю не положенные
	this->_headers->emplace(":path", "/");
	// Кладём в набор псевдозаголовок схемы
	this->_headers->emplace(":scheme", "https");
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что путь в вид сообщения классического туннеля не попал
	ASSERT_EQ(result.find(":path"), std::string::npos);
	// Проверяем что схема в вид сообщения классического туннеля не попала
	ASSERT_EQ(result.find(":scheme"), std::string::npos);
	// Проверяем что авторитет туннеля на месте
	ASSERT_NE(result.find(":authority: example.com:443\r\n"), std::string::npos);
	// Создаём объект запроса клиента с расширенным методом туннеля
	request_t extended(version_t::HTTP2, method_t::CONNECT, std::string("https://example.com/chat"));
	// Устанавливаем протокол туннеля, делающий метод расширенным
	extended.protocol = "websocket";
	// Устанавливаем провайдер запроса
	this->_headers->provider(&extended);
	// Проверяем что расширенному туннелю схема положена
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":scheme: https\r\n"), std::string::npos);
	// Создаём объект обычного запроса клиента
	request_t regular(version_t::HTTP2, method_t::GET, std::string("/index.html"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&regular);
	// Кладём в набор протокол туннеля, обычному запросу не положенный
	this->_headers->emplace(":protocol", "websocket");
	// Проверяем что протокол туннеля в вид сообщения обычного запроса не попал
	ASSERT_EQ(this->_headers->print(proto_t::HTTP2).find(":protocol"), std::string::npos);
}

/**
 * @brief Метод проверки приведения псевдозаголовков, положенных в набор
 *
 * @details Значение приводится теми же правилами, что и собранное из провайдера:
 *          иначе один и тот же узел, заданный заголовком адресата и псевдозаголовком,
 *          давал бы на проводе разное, а сведения о пользователе уехали бы туда,
 *          откуда их убирают (RFC 9110 §5.5, RFC 3986 §3.1, RFC 9112 §3.2.1)
 *
 */
TEST_F(HeadersFixture, StoredPseudoHeadersNormalizedTest){
	// Создаём объект запроса клиента с целью в происхождённой форме
	request_t request(version_t::HTTP2, method_t::GET, std::string("/index.html"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Кладём авторитет со сведениями о пользователе и окружающими отступами
	this->_headers->emplace(":authority", "  user:pass@example.com  ");
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что авторитет очищен так же, как при переносе заголовка адресата
	ASSERT_NE(result.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что сведения о пользователе на провод не уехали
	ASSERT_EQ(result.find("pass"), std::string::npos);
	/**
	 * Присваиваем набор полей: провайдер при этом сбрасывается, и схему с путём
	 * кладёт сам набор. С провайдером запроса они собираются из него и приведению
	 * из набора взяться неоткуда - положенное набором отброшено бы как дубликат
	 */
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":method", "GET"),
		headers_t::header_t{}.from(":scheme", "HtTpS"),
		headers_t::header_t{}.from(":path", "/page#section")
	};
	// Получаем вид сообщения повторно
	const std::string mixed = this->_headers->print(proto_t::HTTP2);
	// Проверяем что схема приведена к нижнему регистру
	ASSERT_NE(mixed.find(":scheme: https\r\n"), std::string::npos);
	// Проверяем что якорь из пути снят
	ASSERT_NE(mixed.find(":path: /page\r\n"), std::string::npos);
	// Проверяем что якорь на провод не ушёл
	ASSERT_EQ(mixed.find("section"), std::string::npos);
}

/**
 * @brief Метод проверки авторитета, опустевшего после очистки
 *
 * @details Пустой авторитет псевдозаголовком не передаётся: он не указывает ни на какой
 *          узел, а принимающая сторона обязана считать такое сообщение некорректным
 *          (RFC 9113 §8.3.1). Опустеть после очистки значение может, если несло одни
 *          лишь сведения о пользователе. Занявшим место такой авторитет не считается,
 *          иначе перенос заголовка адресата отменялся бы, и сообщение оставалось
 *          вовсе без сведений об узле
 *
 */
TEST_F(HeadersFixture, EmptiedAuthorityFallsBackToHostTest){
	// Создаём объект запроса клиента с целью в происхождённой форме
	request_t request(version_t::HTTP2, method_t::GET, std::string("/index.html"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Кладём в набор авторитет из одних сведений о пользователе
	this->_headers->emplace(":authority", "user:pass@");
	// Кладём заголовок адресата, несущий подлинный узел
	this->_headers->emplace("Host", "example.com");
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что авторитет взят из заголовка адресата
	ASSERT_NE(result.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что сведения о пользователе на провод не уехали
	ASSERT_EQ(result.find("pass"), std::string::npos);
	// Проверяем что пустого псевдозаголовка авторитета в сообщении нет
	ASSERT_EQ(result.find(":authority: \r\n"), std::string::npos);
	// Создаём объект запроса клиента с целью туннеля из одних сведений о пользователе
	request_t tunnel(version_t::HTTP2, method_t::CONNECT, std::string("user:pass@"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&tunnel);
	// Получаем вид сообщения туннеля
	const std::string empty = this->_headers->print(proto_t::HTTP2);
	// Проверяем что пустой псевдозаголовок авторитета туннелю не сформирован
	ASSERT_EQ(empty.find(":authority: \r\n"), std::string::npos);
	// Проверяем что сведения о пользователе на провод не уехали и здесь
	ASSERT_EQ(empty.find("pass"), std::string::npos);
}

/**
 * @brief Метод проверки снятия отступов у псевдозаголовков из набора
 *
 * @details Окружающие отступы к значению поля не относятся (RFC 9110 §5.5), поэтому
 *          снимаются у всех приводимых псевдозаголовков одинаково: у авторитета,
 *          у схемы и у пути
 *
 */
TEST_F(HeadersFixture, StoredPseudoHeadersTrimmedTest){
	// Присваиваем набор полей, где схема и путь несут окружающие отступы
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":method", "GET"),
		headers_t::header_t{}.from(":scheme", "  HTTPS  "),
		headers_t::header_t{}.from(":path", "  /page  ")
	};
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP2);
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что отступы у схемы сняты, а регистр приведён
	ASSERT_NE(result.find(":scheme: https\r\n"), std::string::npos);
	// Проверяем что отступы у пути сняты
	ASSERT_NE(result.find(":path: /page\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки приведения всех псевдозаголовков набора без изъятия
 *
 * @details Окружающие отступы к значению поля не относятся (RFC 9110 §5.5), и снимать
 *          их у одних псевдозаголовков, оставляя у других, значило бы обещать одно,
 *          а делать другое. Пустое значение псевдозаголовком не передаётся вовсе:
 *          оно не указывает ни на что, а принимающая сторона обязана считать такое
 *          сообщение некорректным (RFC 9113 §8.3.1)
 *
 */
TEST_F(HeadersFixture, EveryStoredPseudoHeaderNormalizedTest){
	// Присваиваем набор полей запроса, где отступы несут метод и путь
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":method", "  GET  "),
		headers_t::header_t{}.from(":scheme", "  https  "),
		headers_t::header_t{}.from(":path", "  /page  ")
	};
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP2);
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что отступы сняты и у метода запроса
	ASSERT_NE(result.find(":method: GET\r\n"), std::string::npos);
	// Проверяем что отступы сняты у схемы
	ASSERT_NE(result.find(":scheme: https\r\n"), std::string::npos);
	// Проверяем что отступы сняты у пути
	ASSERT_NE(result.find(":path: /page\r\n"), std::string::npos);
	// Присваиваем набор полей ответа, где отступы несёт код ответа
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":status", "  200  ")
	};
	// Проверяем что отступы сняты и у кода ответа
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":status: 200\r\n"), std::string::npos);
	// Присваиваем набор полей, где схема несёт одни отступы
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":method", "GET"),
		headers_t::header_t{}.from(":scheme", "   "),
		headers_t::header_t{}.from(":path", "/")
	};
	// Проверяем что опустевшая схема псевдозаголовком не передана
	ASSERT_EQ(this->_headers->print(proto_t::HTTP2).find(":scheme"), std::string::npos);
}

/**
 * @brief Метод проверки разновидности туннеля, определяемой без провайдера
 *
 * @details Расширенным туннель делает не наличие псевдозаголовка протокола, а его
 *          значение: пустой протокол ничего не поднимает поверх туннеля (RFC 8441 §4),
 *          и признать по нему туннель расширенным значило бы пропустить в сообщение
 *          схему и путь, которых классический туннель не несёт вовсе (RFC 9113 §8.5).
 *          Название метода при сравнении также берётся без окружающих отступов
 *
 */
TEST_F(HeadersFixture, StoredConnectKindByValueTest){
	// Присваиваем набор полей туннеля, где протокол пуст, а метод несёт отступы
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":method", "  CONNECT  "),
		headers_t::header_t{}.from(":authority", "example.com:443"),
		headers_t::header_t{}.from(":protocol", "   "),
		headers_t::header_t{}.from(":scheme", "https"),
		headers_t::header_t{}.from(":path", "/chat")
	};
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP2);
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что метод с отступами распознан как туннель, и схема отсеяна
	ASSERT_EQ(result.find(":scheme"), std::string::npos);
	// Проверяем что путь классическому туннелю также не передан
	ASSERT_EQ(result.find(":path"), std::string::npos);
	// Проверяем что пустой протокол туннеля на провод не ушёл
	ASSERT_EQ(result.find(":protocol"), std::string::npos);
	// Проверяем что авторитет туннеля на месте
	ASSERT_NE(result.find(":authority: example.com:443\r\n"), std::string::npos);
	// Присваиваем набор полей расширенного туннеля с непустым протоколом
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":method", "CONNECT"),
		headers_t::header_t{}.from(":authority", "example.com:443"),
		headers_t::header_t{}.from(":protocol", "websocket"),
		headers_t::header_t{}.from(":scheme", "https")
	};
	// Проверяем что расширенному туннелю схема положена
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":scheme: https\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки занятости авторитета по всем его вхождениям
 *
 * @details Набор принимает одноимённые поля, и опустевшее после очистки первое
 *          вхождение не отменяет годного второго. Считать авторитет свободным
 *          по одному лишь первому значило бы перенести заголовок адресата рядом
 *          с уже годным авторитетом - и дать два псевдозаголовка авторитета,
 *          сообщение с которыми принимающая сторона обязана отвергнуть
 *
 */
TEST_F(HeadersFixture, AuthorityOccupiedByAnyOccurrenceTest){
	// Присваиваем набор полей, где первый авторитет опустевает после очистки
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":method", "GET"),
		headers_t::header_t{}.from(":path", "/"),
		headers_t::header_t{}.from(":authority", "user:pass@"),
		headers_t::header_t{}.from(":authority", "example.com"),
		headers_t::header_t{}.from("Host", "other.example")
	};
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP2);
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что годный авторитет из набора на месте
	ASSERT_NE(result.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что заголовок адресата рядом с ним в авторитет не перенесён
	ASSERT_EQ(result.find(":authority: other.example"), std::string::npos);
	// Проверяем что сведения о пользователе на провод не уехали
	ASSERT_EQ(result.find("pass"), std::string::npos);
}

/**
 * @brief Метод проверки переноса адресата по всем его вхождениям
 *
 * @details Из вида сообщения запроса снимаются все вхождения заголовка адресата разом,
 *          а переносится в псевдозаголовок одно. Опустевшее после очистки первое
 *          вхождение отменяло бы годное второе, и сообщение осталось бы вовсе
 *          без сведений об узле - при том, что сами заголовки адресата из него
 *          уже сняты (RFC 9113 §8.3.1)
 *
 */
TEST_F(HeadersFixture, HostTransferByAnyOccurrenceTest){
	// Создаём объект запроса клиента с целью в происхождённой форме
	request_t request(version_t::HTTP2, method_t::GET, std::string("/index.html"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Кладём первый заголовок адресата из одних сведений о пользователе
	this->_headers->emplace("Host", "user:pass@", headers_t::mode_t::APPEND);
	// Кладём второй заголовок адресата, несущий подлинный узел
	this->_headers->emplace("Host", "example.com", headers_t::mode_t::APPEND);
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что авторитет перенесён из годного вхождения
	ASSERT_NE(result.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что сведения о пользователе на провод не уехали
	ASSERT_EQ(result.find("pass"), std::string::npos);
	// Проверяем что сами заголовки адресата из вида сообщения запроса сняты
	ASSERT_EQ(result.find("host:"), std::string::npos);
	// Количество псевдозаголовков авторитета в виде сообщения
	size_t authorities = 0;
	/**
	 * Считаем вхождения псевдозаголовка авторитета: искать второе от места первого
	 * нельзя - поиск попал бы внутрь того же самого названия
	 */
	for(size_t i = result.find(":authority"); i != std::string::npos; i = result.find(":authority", i + 10))
		// Увеличиваем количество найденных псевдозаголовков авторитета
		authorities++;
	// Проверяем что авторитет передан единственным псевдозаголовком
	ASSERT_EQ(authorities, 1u);
}

/**
 * @brief Метод проверки вида туннеля, определяемого по первому непустому значению
 *
 * @details Набор принимает одноимённые поля, и пустое первое вхождение метода отменяло бы
 *          годное второе: сообщение уходило бы туннелем, а признаки оставались бы
 *          от обычного запроса, и в вид сообщения проскочили бы схема с путём,
 *          которых классический туннель не несёт вовсе (RFC 9113 §8.5)
 *
 */
TEST_F(HeadersFixture, ConnectKindByAnyOccurrenceTest){
	// Присваиваем набор полей, где первое вхождение метода несёт одни отступы
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":method", "   "),
		headers_t::header_t{}.from(":method", "CONNECT"),
		headers_t::header_t{}.from(":authority", "example.com:443"),
		headers_t::header_t{}.from(":scheme", "https"),
		headers_t::header_t{}.from(":path", "/chat")
	};
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP2);
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что запрос распознан туннелем и схема отсеяна
	ASSERT_EQ(result.find(":scheme"), std::string::npos);
	// Проверяем что путь классическому туннелю также не передан
	ASSERT_EQ(result.find(":path"), std::string::npos);
	// Проверяем что авторитет туннеля на месте
	ASSERT_NE(result.find(":authority: example.com:443\r\n"), std::string::npos);
	// Присваиваем набор полей туннеля, где первое вхождение протокола пусто
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":method", "CONNECT"),
		headers_t::header_t{}.from(":authority", "example.com:443"),
		headers_t::header_t{}.from(":protocol", "   "),
		headers_t::header_t{}.from(":protocol", "websocket"),
		headers_t::header_t{}.from(":scheme", "https")
	};
	// Проверяем что туннель признан расширенным по годному вхождению протокола
	ASSERT_NE(this->_headers->print(proto_t::HTTP2).find(":scheme: https\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки приведения авторитета и протокола на пути провайдера
 *
 * @details Авторитет приходит четырьмя путями - из цели обычного запроса, из цели
 *          туннеля, из заголовка адресата и из псевдозаголовка набора, - и приводиться
 *          обязан одинаково на всех. Разновидность туннеля тем же порядком определяется
 *          по значению протокола, а не по его наличию: протокол из одних отступов
 *          ничего поверх туннеля не поднимает (RFC 8441 §4)
 *
 */
TEST_F(HeadersFixture, ProviderPathNormalizationTest){
	// Создаём объект запроса клиента с целью, где авторитет несёт отступы и сведения о пользователе
	request_t request(version_t::HTTP2, method_t::GET, std::string("https://  user:pass@example.com  /index.html"));
	// Устанавливаем провайдер запроса
	this->_headers->provider(&request);
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что авторитет очищен от отступов и сведений о пользователе
	ASSERT_NE(result.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что сведения о пользователе на провод не уехали
	ASSERT_EQ(result.find("pass"), std::string::npos);
	// Создаём объект запроса клиента с целью туннеля и протоколом из одних отступов
	request_t tunnel(version_t::HTTP2, method_t::CONNECT, std::string("https://example.com:443/chat"));
	// Устанавливаем протокол туннеля из одних отступов
	tunnel.protocol = "   ";
	// Устанавливаем провайдер запроса
	this->_headers->provider(&tunnel);
	// Получаем вид сообщения туннеля
	const std::string classic = this->_headers->print(proto_t::HTTP2);
	// Проверяем что туннель признан классическим и схема не собрана
	ASSERT_EQ(classic.find(":scheme"), std::string::npos);
	// Проверяем что путь классическому туннелю также не собран
	ASSERT_EQ(classic.find(":path"), std::string::npos);
	// Проверяем что протокол туннеля из одних отступов на провод не ушёл
	ASSERT_EQ(classic.find(":protocol"), std::string::npos);
	// Проверяем что авторитет туннеля собран из цели запроса
	ASSERT_NE(classic.find(":authority: example.com:443\r\n"), std::string::npos);
	// Устанавливаем протокол туннеля с окружающими отступами
	tunnel.protocol = "  websocket  ";
	// Устанавливаем провайдер запроса заново
	this->_headers->provider(&tunnel);
	// Получаем вид сообщения расширенного туннеля
	const std::string extended = this->_headers->print(proto_t::HTTP2);
	// Проверяем что протокол туннеля передан без окружающих отступов
	ASSERT_NE(extended.find(":protocol: websocket\r\n"), std::string::npos);
	// Проверяем что расширенному туннелю схема положена
	ASSERT_NE(extended.find(":scheme: https\r\n"), std::string::npos);
}

/**
 * @brief Метод проверки направления сообщения, определяемого по значению кода ответа
 *
 * @details Без провайдера направление берётся из псевдозаголовка кода ответа, и брать
 *          его по наличию поля нельзя: код из одних отступов в вид сообщения не попадёт,
 *          а набор уже будет сочтён ответом. Тогда заголовок адресата в авторитет
 *          не перенесётся и останется полем, и сообщение уйдёт вовсе без псевдозаголовков -
 *          недопустимое ни в одном направлении (RFC 9113 §8.3)
 *
 */
TEST_F(HeadersFixture, DirectionByStatusValueTest){
	// Присваиваем набор полей, где код ответа несёт одни отступы
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":status", "   "),
		headers_t::header_t{}.from("Host", "example.com"),
		headers_t::header_t{}.from("Server", "awh")
	};
	// Устанавливаем протокол контейнера
	this->_headers->proto(proto_t::HTTP2);
	// Получаем вид сообщения под протокол HTTP/2
	const std::string result = this->_headers->print(proto_t::HTTP2);
	// Проверяем что набор сочтён запросом и адресат перенесён в авторитет
	ASSERT_NE(result.find(":authority: example.com\r\n"), std::string::npos);
	// Проверяем что заголовок адресата из вида сообщения запроса снят
	ASSERT_EQ(result.find("host:"), std::string::npos);
	// Проверяем что опустевший код ответа в вид сообщения не попал
	ASSERT_EQ(result.find(":status"), std::string::npos);
	// Присваиваем набор полей с годным кодом ответа
	(* this->_headers) = headers_t::fields_t {
		headers_t::header_t{}.from(":status", "200"),
		headers_t::header_t{}.from("Host", "example.com")
	};
	// Получаем вид сообщения ответа
	const std::string response = this->_headers->print(proto_t::HTTP2);
	// Проверяем что набор с годным кодом сочтён ответом и авторитет не сформирован
	ASSERT_EQ(response.find(":authority"), std::string::npos);
	// Проверяем что заголовок адресата в ответе сохранён
	ASSERT_NE(response.find("host: example.com\r\n"), std::string::npos);
}
