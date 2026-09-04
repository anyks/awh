/**
 * @file document.cpp
 * @date 2026-09-04
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
 * @brief Автоматические тесты события CEF, удерживаемого целиком — устройства дерева,
 *        замкнутости обхода по пути, повторяющихся ключей перечнем, сведённого именования
 *        вторыми ходами, сброса против сноса и обратимости перевода по деревьям
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/cef/cef.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../../main.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений AWH
 */
#include <sys/macro/suppress.hpp>
#include <sys/log.hpp>

/**
 * @brief Пространство имён проверок этого файла
 *
 * @note Держится оно безымянным намеренно: проверки кодеков собираются одной программою
 *
 */
namespace {
	/**
	 * @brief Объект окружения проверок события CEF
	 *
	 */
	struct EnvCef {
		// Объект фреймворка проверок
		awh::fmk_t fmk;
		// Объект журнала проверок
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		EnvCef() noexcept : log(&this->fmk) {
			// Выполняем отключение вывода логов
			this->log.mode({});
		}
	};
	/**
	 * @brief Функция получения объекта окружения проверок
	 *
	 * @return объект окружения проверок
	 *
	 */
	EnvCef & environment() noexcept {
		// Объект окружения проверок
		static EnvCef env;
		// Выводим объект окружения проверок
		return env;
	}
	/**
	 * @brief Разбираемая запись живого журнала
	 *
	 */
	const ::std::string RECORD =
		"Feb 17 15:30:15 vnetids emerg CEF:0|InfoTeCS|IDS|2.4.3-371989|1:905590:7|ET POLICY RDP connection confirm|1|"
		"cat=1 cn1=25162858 cn1Label=EventID cs3= cs3Label=CVEID dmac=b0:01:86:30:90:05 dst=192.168.59.39 "
		"deviceExternalId=1330334083 deviceExternalId=999 msg=HTTPS post from 1.2.3.4";
}

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::codec;

/**
 * @brief Проверка устройства дерева разобранного события
 *
 */
TEST(CodecCefDocument, Layout) {
	// Объект события CEF
	cef::document_t doc(&::environment().fmk, &::environment().log);
	// Выполняем разбор записи живого журнала
	ASSERT_TRUE(doc.parse(::RECORD));
	// Выполняем проверку приставки syslog
	EXPECT_EQ(doc.at("/syslog").text(), "Feb 17 15:30:15 vnetids emerg");
	// Выполняем проверку поля заголовка поставщика устройства
	EXPECT_EQ(doc.at("/header/vendor").text(), "InfoTeCS");
	// Выполняем проверку поля заголовка изделия поставщика
	EXPECT_EQ(doc.at("/header/product").text(), "IDS");
	// Выполняем проверку поля заголовка редакции изделия
	EXPECT_EQ(doc.at("/header/release").text(), "2.4.3-371989");
	// Выполняем проверку поля заголовка опознавателя события
	EXPECT_EQ(doc.at("/header/signature").text(), "1:905590:7");
	// Выполняем проверку поля заголовка имени события
	EXPECT_EQ(doc.at("/header/name").text(), "ET POLICY RDP connection confirm");
	// Выполняем проверку наличия всех семи полей заголовка
	EXPECT_EQ(doc.keys("/header").size(), static_cast <size_t> (cef::HEADER_FIELDS));
	// Выполняем проверку наличия пары расширения адреса назначения
	EXPECT_EQ(doc.at("/extension/dst").text(), "192.168.59.39");
}

/**
 * @brief Проверка замкнутости обхода дерева по пути
 *
 * @details Замкнутость доказывается ТОЖДЕСТВОМ узлов, а не совпадением количеств:
 *          обход, выдающий звенья, ни к одному потомку не ведущие, совпадению
 *          количеств не противоречит вовсе
 *
 */
TEST(CodecCefDocument, Traversal) {
	// Объект события CEF
	cef::document_t doc(&::environment().fmk, &::environment().log);
	// Выполняем разбор записи живого журнала
	ASSERT_TRUE(doc.parse(::RECORD));
	// Получаем звенья пути расширения записи
	const vector <string> keys = doc.keys("/extension");
	// Выполняем проверку непустоты звеньев пути расширения
	ASSERT_FALSE(keys.empty());
	/**
	 * Выполняем перебор всех звеньев пути расширения
	 */
	for(const auto & key : keys){
		// Выполняем проверку того, что звено ведёт к потомку
		EXPECT_TRUE(doc.has("/extension/" + key)) << "звено: " << key;
		// Выполняем проверку наличия вложенного значения по имени
		EXPECT_TRUE(doc.contains("/extension", key)) << "звено: " << key;
	}
	// Выполняем проверку выдачи пустого перечня у листа дерева
	EXPECT_TRUE(doc.keys("/extension/dst").empty());
	// Выполняем проверку выдачи пустого перечня у пути несуществующего
	EXPECT_TRUE(doc.keys("/несуществующий/путь").empty());
	// Выполняем проверку того, что лист от пути несуществующего отличается наличием
	EXPECT_TRUE(doc.has("/extension/dst"));
	// Выполняем проверку отсутствия значения по пути несуществующему
	EXPECT_FALSE(doc.has("/несуществующий/путь"));
}

/**
 * @brief Проверка укладки повторяющегося ключа перечнем
 *
 * @details Устройство взято у кодека INI: повтор даёт один потомок перечнем, а обход
 *          остаётся замкнутым числовыми звеньями пути
 *
 */
TEST(CodecCefDocument, DuplicateKeys) {
	// Объект события CEF
	cef::document_t doc(&::environment().fmk, &::environment().log);
	// Выполняем разбор записи живого журнала
	ASSERT_TRUE(doc.parse(::RECORD));
	// Получаем звенья пути повторяющегося ключа расширения
	const vector <string> keys = doc.keys("/extension/deviceExternalId");
	// Выполняем проверку количества значений повторяющегося ключа
	ASSERT_EQ(keys.size(), static_cast <size_t> (2));
	// Выполняем проверку числового вида звеньев пути
	EXPECT_EQ(keys.at(0), "0");
	// Выполняем проверку числового вида звеньев пути
	EXPECT_EQ(keys.at(1), "1");
	// Выполняем проверку первого значения повторяющегося ключа
	EXPECT_EQ(doc.at("/extension/deviceExternalId/0").text(), "1330334083");
	// Выполняем проверку второго значения повторяющегося ключа
	EXPECT_EQ(doc.at("/extension/deviceExternalId/1").text(), "999");
	// Выполняем проверку разбора трёх появлений одного ключа
	ASSERT_TRUE(doc.parse("CEF:0|A|B|C|D|E|1|ad.x=1 ad.x=2 ad.x=3"));
	// Выполняем проверку количества значений повторяющегося ключа
	EXPECT_EQ(doc.keys("/extension/ad.x").size(), static_cast <size_t> (3));
	// Выполняем проверку третьего значения повторяющегося ключа
	EXPECT_EQ(doc.at("/extension/ad.x/2").text(), "3");
}

/**
 * @brief Проверка сведённого именования вторыми ходами
 *
 * @details Обход по пути выдаёт СЫРЫЕ ключи записи, а сведённое именование берётся
 *          вторыми ходами: `field` разыскивает значение по полному имени словаря,
 *          `label` выдаёт человеческое имя ключа
 *
 */
TEST(CodecCefDocument, Naming) {
	// Объект события CEF
	cef::document_t doc(&::environment().fmk, &::environment().log);
	// Выполняем разбор записи живого журнала
	ASSERT_TRUE(doc.parse(::RECORD));
	// Выполняем проверку того, что обход выдаёт сырые ключи записи
	EXPECT_TRUE(doc.has("/extension/dmac"));
	// Выполняем проверку того, что полное имя звеном пути не является
	EXPECT_FALSE(doc.has("/extension/deviceMacAddress"));
	// Выполняем проверку розыска значения по полному имени словаря
	EXPECT_EQ(doc.field("deviceMacAddress").text(), "b0:01:86:30:90:05");
	// Выполняем проверку розыска значения по полному имени словаря
	EXPECT_EQ(doc.field("destinationAddress").text(), "192.168.59.39");
	// Выполняем проверку человеческого имени ключа, меткой записи заданного
	EXPECT_EQ(doc.label("cn1"), "EventID");
	// Выполняем проверку человеческого имени ключа, словарём заданного
	EXPECT_EQ(doc.label("dmac"), "deviceMacAddress");
	// Выполняем проверку пустоты имени у ключа без метки и без словаря
	EXPECT_TRUE(doc.label("ad.prog-id").empty());
	// Выполняем проверку вида значения ключа, словарём заданного
	EXPECT_EQ(doc.type("dmac"), cef::type_t::MAC);
	// Выполняем проверку отсутствия вида у ключа, словарю неизвестного
	EXPECT_EQ(doc.type("ad.prog-id"), cef::type_t::NONE);
}

/**
 * @brief Проверка сброса значения против сноса пары
 *
 * @details Сброс оставляет ключ с пустым значением, снос убирает пару из записи вовсе.
 *          Различия «пусто» и «нет вовсе» сама запись CEF не несёт, оттого сброшенное
 *          поле от записанного пустым неотличимо - и это граница формата
 *
 */
TEST(CodecCefDocument, ResetAgainstErase) {
	// Объект события CEF
	cef::document_t doc(&::environment().fmk, &::environment().log);
	// Выполняем разбор записи живого журнала
	ASSERT_TRUE(doc.parse(::RECORD));
	// Выполняем сброс значения пары расширения
	ASSERT_TRUE(doc.reset("/extension/msg"));
	// Выполняем проверку того, что ключ остался на месте
	EXPECT_TRUE(doc.has("/extension/msg"));
	// Выполняем проверку того, что значение стало пустым
	EXPECT_TRUE(doc.at("/extension/msg").text().empty());
	// Выполняем проверку неотличимости сброшенного поля от записанного пустым
	EXPECT_EQ(doc.at("/extension/msg").type(), doc.at("/extension/cs3").type());
	// Выполняем снос пары расширения целиком
	ASSERT_TRUE(doc.erase("/extension/cat"));
	// Выполняем проверку того, что пары в записи не стало
	EXPECT_FALSE(doc.has("/extension/cat"));
	// Выполняем проверку отказа сброса значения по пути несуществующему
	EXPECT_FALSE(doc.reset("/extension/несуществующий"));
	// Выполняем проверку кода отказа сброса значения
	EXPECT_EQ(doc.error(), cef::error_t::UNKNOWN_FIELD);
}

/**
 * @brief Проверка обратимости перевода по ДЕРЕВЬЯМ
 *
 * @details Дословного совпадения записи перевод не обещает - обещает значение. Оттого
 *          обратимость закрепляется сличением деревьев, а не текстов
 *
 */
TEST(CodecCefDocument, Roundtrip) {
	// Объект события CEF
	cef::document_t first(&::environment().fmk, &::environment().log);
	// Выполняем разбор записи живого журнала
	ASSERT_TRUE(first.parse(::RECORD));
	// Выполняем сбор записи CEF из дерева события
	const string text = first.dump();
	// Выполняем проверку непустоты собранной записи
	ASSERT_FALSE(text.empty());
	// Объект события CEF повторного разбора
	cef::document_t second(&::environment().fmk, &::environment().log);
	// Выполняем повторный разбор собранной записи
	ASSERT_TRUE(second.parse(text));
	// Выполняем проверку совпадения деревьев разбора
	EXPECT_EQ(first.root().dump(), second.root().dump());
}

/**
 * @brief Проверка обращения пустого значения по настройке
 *
 */
TEST(CodecCefDocument, EmptyValue) {
	// Настройки разбора записей
	cef::reader_t::settings_t settings;
	// Объект события CEF
	cef::document_t doc(&::environment().fmk, &::environment().log);
	// Выполняем разбор записи с пустым значением
	ASSERT_TRUE(doc.parse("CEF:0|A|B|C|D|E|1|cs3= cs3Label=CVEID"));
	// Выполняем проверку того, что пустое значение есть последовательность знаков
	EXPECT_EQ(doc.at("/extension/cs3").type(), abc::type_t::STRING);
	// Устанавливаем обращение пустого значения в логическую истину
	settings.empty = cef::empty_t::BOOLEAN;
	// Устанавливаем настройки разбора записей
	ASSERT_TRUE(doc.settings(settings));
	// Выполняем разбор записи с пустым значением
	ASSERT_TRUE(doc.parse("CEF:0|A|B|C|D|E|1|cs3= cs3Label=CVEID"));
	// Выполняем проверку того, что пустое значение стало логическим
	EXPECT_EQ(doc.at("/extension/cs3").type(), abc::type_t::BOOL);
	// Устанавливаем пропуск пустого значения вовсе
	settings.empty = cef::empty_t::SKIP;
	// Устанавливаем настройки разбора записей
	ASSERT_TRUE(doc.settings(settings));
	// Выполняем разбор записи с пустым значением
	ASSERT_TRUE(doc.parse("CEF:0|A|B|C|D|E|1|cs3= cs3Label=CVEID"));
	// Выполняем проверку того, что пары с пустым значением не стало
	EXPECT_FALSE(doc.has("/extension/cs3"));
	// Выполняем проверку того, что метка имени в записи осталась
	EXPECT_TRUE(doc.has("/extension/cs3Label"));
}

/**
 * @brief Проверка строгости сличения ключей со словарём
 *
 */
TEST(CodecCefDocument, Strictness) {
	// Настройки разбора записей
	cef::reader_t::settings_t settings;
	// Объект события CEF
	cef::document_t doc(&::environment().fmk, &::environment().log);
	// Устанавливаем строгое сличение ключей расширения со словарём
	settings.mode = cef::mode_t::STRONG;
	// Устанавливаем настройки разбора записей
	ASSERT_TRUE(doc.settings(settings));
	// Выполняем проверку отклонения ключа, словарю неизвестного
	EXPECT_FALSE(doc.parse("CEF:0|A|B|C|D|E|1|ad.prog-id=128394"));
	// Выполняем проверку кода отказа разбора
	EXPECT_EQ(doc.error(), cef::error_t::UNKNOWN_KEY);
	// Выполняем проверку принятия ключа, словарю известного
	EXPECT_TRUE(doc.parse("CEF:0|A|B|C|D|E|1|src=1.2.3.4 dmac=b0:01:86:30:90:05"));
	// Выполняем проверку отклонения значения, виду словаря не отвечающего
	EXPECT_FALSE(doc.parse("CEF:0|A|B|C|D|E|1|src=это не адрес"));
	// Выполняем проверку кода отказа разбора
	EXPECT_EQ(doc.error(), cef::error_t::INVALID_ADDRESS);
	// Устанавливаем сличение имён ключей без проверки видов значений
	settings.mode = cef::mode_t::LOW;
	// Устанавливаем настройки разбора записей
	ASSERT_TRUE(doc.settings(settings));
	// Выполняем проверку принятия ключа, словарю неизвестного, при слабом сличении
	EXPECT_TRUE(doc.parse("CEF:0|A|B|C|D|E|1|ad.prog-id=128394 src=это не адрес"));
}

/**
 * @brief Проверка приведения числовых значений по словарю
 *
 */
TEST(CodecCefDocument, Numbers) {
	// Настройки разбора записей
	cef::reader_t::settings_t settings;
	// Объект события CEF
	cef::document_t doc(&::environment().fmk, &::environment().log);
	// Устанавливаем сличение имён ключей и простых видов значений
	settings.mode = cef::mode_t::MEDIUM;
	// Устанавливаем настройки разбора записей
	ASSERT_TRUE(doc.settings(settings));
	// Выполняем разбор записи с числовыми значениями
	ASSERT_TRUE(doc.parse("CEF:0|A|B|C|D|E|1|cn1=25162858 spt=1232 msg=просто текст"));
	// Выполняем проверку приведения числового значения к целому виду
	EXPECT_TRUE((static_cast <uint32_t> (doc.at("/extension/cn1").type()) & static_cast <uint32_t> (abc::type_t::INT)) > 0);
	// Выполняем проверку приведения числового значения к целому виду
	EXPECT_TRUE((static_cast <uint32_t> (doc.at("/extension/spt").type()) & static_cast <uint32_t> (abc::type_t::INT)) > 0);
	// Выполняем проверку того, что текстовое значение осталось знаками
	EXPECT_EQ(doc.at("/extension/msg").type(), abc::type_t::STRING);
	// Выполняем проверку того, что при выключенном сличении числа остаются знаками
	settings.mode = cef::mode_t::NONE;
	// Устанавливаем настройки разбора записей
	ASSERT_TRUE(doc.settings(settings));
	// Выполняем разбор записи с числовыми значениями
	ASSERT_TRUE(doc.parse("CEF:0|A|B|C|D|E|1|cn1=25162858"));
	// Выполняем проверку того, что числовое значение осталось знаками
	EXPECT_EQ(doc.at("/extension/cn1").type(), abc::type_t::STRING);
}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include <sys/macro/restore.hpp>
