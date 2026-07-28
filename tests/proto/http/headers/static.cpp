/**
 * @file: static.cpp
 * @date: 2026-07-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Статические тесты контейнера HTTP-заголовков — проверка создания и сброса объекта модуля,
 *        а также корректности регистронезависимого поиска полей,
 *        работы с множественными значениями и контроля лимитов
 *
 * @copyright: Copyright © 2026
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
	// Проверяем что заголовки с одинаковым названием равны (без учёта регистра)
	headers_t::header_t header3;
	// Заполняем заголовок с названием в другом регистре
	header3.from("content-type", "application/json");
	// Проверяем что заголовки с одинаковым названием равны независимо от регистра и значения
	ASSERT_TRUE(header1 == header3);
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
	// Проверяем что контейнеры равны независимо от порядка значений
	ASSERT_TRUE(headers1 == headers2);
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
