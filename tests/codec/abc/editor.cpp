/**
 * @file editor.cpp
 * @date 2026-08-19
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки правки бинарного контейнера ABC на месте — дописывание, правка и снос
 *        записей, фиксация накопленного, откат к прежнему поколению и способы фиксации
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/abc/abc.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Пространство имён работ, доступных лишь этому файлу
 *
 */
namespace {
	/**
	 * @brief Класс носителя контейнера в памяти
	 *
	 * @details Носитель этот заменяет файл: правка ведётся теми же работами чтения и
	 *          записи, а проверке остаётся смотреть на октеты его прямо
	 *
	 */
	class Medium {
		public:
			// Октеты контейнера, лежащие на носителе
			vector <uint8_t> data;
			// Признак отказа работы записи октетов
			bool broken = false;
		public:
			/**
			 * @brief Метод чтения октетов контейнера
			 *
			 * @param offset смещение вычитываемых октетов
			 * @param size   размер вычитываемых октетов
			 * @param result буфер, куда следует положить вычитанные октеты
			 * @return       признак успешного чтения октетов
			 *
			 */
			bool read(const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept {
				// Если затребованные октеты за концом лежащих на носителе
				if((offset + size) > static_cast <uint64_t> (this->data.size()))
					// Выводим признак неудачного чтения октетов
					return false;
				// Выполняем выдачу затребованных октетов
				result.assign(this->data.begin() + static_cast <ptrdiff_t> (offset),
				 this->data.begin() + static_cast <ptrdiff_t> (offset + size));
				// Выводим признак успешного чтения октетов
				return true;
			}
			/**
			 * @brief Метод записи октетов контейнера
			 *
			 * @param offset смещение записываемых октетов
			 * @param buffer буфер записываемых октетов
			 * @param size   размер записываемых октетов
			 * @return       признак успешной записи октетов
			 *
			 */
			bool write(const uint64_t offset, const void * buffer, const size_t size) noexcept {
				// Если работа записи объявлена отказавшей
				if(this->broken)
					// Выводим признак неудачной записи октетов
					return false;
				// Если записываемые октеты за концом лежащих на носителе
				if((offset + size) > static_cast <uint64_t> (this->data.size()))
					// Выполняем расширение носителя под записываемые октеты
					this->data.resize(static_cast <size_t> (offset + size), 0);
				// Выполняем запись поданных октетов на носитель
				::memcpy(this->data.data() + offset, buffer, size);
				// Выводим признак успешной записи октетов
				return true;
			}
	};
	/**
	 * @brief Класс опоры проверок правки контейнера
	 *
	 */
	class EditorFixture : public testing::Test {
		protected:
			// Объект фреймворка
			unique_ptr <fmk_t> _fmk;
			// Объект журнала
			unique_ptr <log_t> _log;
			// Объект сжатия данных
			unique_ptr <compressor::block_t> _compressor;
			// Объект шифрования данных
			unique_ptr <crypto_t> _crypto;
		public:
			/**
			 * @brief Метод заведения опоры проверок
			 *
			 */
			void SetUp() override {
				// Выполняем заведение объекта фреймворка
				this->_fmk = make_unique <fmk_t> ();
				// Выполняем заведение объекта журнала
				this->_log = make_unique <log_t> (this->_fmk.get());
				// Выполняем заведение объекта сжатия данных
				this->_compressor = make_unique <compressor::block_t> (this->_log.get());
				// Выполняем заведение объекта шифрования данных
				this->_crypto = make_unique <crypto_t> (this->_fmk.get(), this->_log.get());
			}
		public:
			/**
			 * @brief Метод сборки контейнера с поданными записями
			 *
			 * @param medium  носитель, куда следует уложить собранный контейнер
			 * @param records собираемые записи контейнера
			 *
			 */
			void build(Medium & medium, const vector <string> & records) noexcept {
				// Сборщик контейнера
				abc::assembler_t assembler;
				// Выполняем перебор всех собираемых записей контейнера
				for(const string & text : records){
					// Выполняем сборку очередной записи
					const vector <uint8_t> item = abc::value_t(text).dump();
					// Выполняем внесение очередной записи в собираемый контейнер
					assembler.append(item.data(), item.size(), abc::payload_t::TEXT);
				}
				// Выполняем завершение сборки контейнера
				assembler.complete(medium.data);
			}
			/**
			 * @brief Метод открытия контейнера правщиком
			 *
			 * @param editor открываемый правщик контейнера
			 * @param medium носитель, несущий контейнер
			 * @return       признак успешно открытого контейнера
			 *
			 */
			bool open(abc::editor_t & editor, Medium & medium) noexcept {
				// Выполняем открытие контейнера отданными работами чтения и записи
				return editor.open([&medium](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
					// Выполняем чтение затребованных октетов контейнера
					return medium.read(offset, size, result);
				}, [&medium](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
					// Выполняем запись поданных октетов контейнера
					return medium.write(offset, buffer, size);
				}, static_cast <uint64_t> (medium.data.size()));
			}
			/**
			 * @brief Метод выборки записи контейнера с носителя
			 *
			 * @param medium носитель, несущий контейнер
			 * @param number порядковый номер выбираемой записи
			 * @param result буфер, куда следует положить выбранную запись
			 * @param error  код отказа выборки записи
			 * @return       признак успешно выбранной записи
			 *
			 */
			bool pick(Medium & medium, const uint64_t number, vector <uint8_t> & result, abc::error_t & error) noexcept {
				// Выборщик записей контейнера
				abc::fetcher_t fetcher;
				// Выполняем открытие контейнера отданной работой чтения
				if(!fetcher.open([&medium](const uint64_t offset, const size_t size, vector <uint8_t> & data) noexcept -> bool {
					// Выполняем чтение затребованных октетов контейнера
					return medium.read(offset, size, data);
				})){
					// Выполняем установку кода отказа открытия контейнера
					error = fetcher.error();
					// Выводим признак неудачной выборки записи
					return false;
				}
				// Выполняем выборку затребованной записи контейнера
				const bool result2 = fetcher.record(number, result);
				// Выполняем установку кода отказа выборки записи
				error = fetcher.error();
				// Выводим признак выборки записи
				return result2;
			}
	};
};

/**
 * @brief Проверка дописывания записей в конец контейнера
 *
 */
TEST_F(EditorFixture, AppendCommit) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с двумя записями
	this->build(medium, {"первая", "вторая"});
	// Выполняем получение длины собранного контейнера
	const size_t origin = medium.data.size();
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку количества записей открытого контейнера
	ASSERT_EQ(editor.records(), 2ull);
	// Выполняем сборку дописываемой записи
	const vector <uint8_t> item = abc::value_t(string{"третья"}).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем проверку количества записей контейнера с накопленной правкой
	ASSERT_EQ(editor.records(), 3ull);
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что поколение записи контейнера возросло
	ASSERT_EQ(editor.header().generation, 1ull);
	// Выполняем проверку того, что контейнер на носителе вырос, а не переписан
	ASSERT_GT(medium.data.size(), origin);
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Код отказа выборки записи контейнера
	abc::error_t error = abc::error_t::NONE;
	// Выполняем выборку дописанной записи с носителя
	ASSERT_TRUE(this->pick(medium, 2, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, item);
	// Выполняем выборку записи, лежавшей в контейнере прежде правки
	ASSERT_TRUE(this->pick(medium, 0, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку того, что прежняя запись правкой не тронута
	ASSERT_EQ(picked, abc::value_t(string{"первая"}).dump());
}
/**
 * @brief Проверка правки записи контейнера дописыванием
 *
 */
TEST_F(EditorFixture, ReplaceRedirects) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с тремя записями
	this->build(medium, {"первая", "вторая", "третья"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем сборку новой записи
	const vector <uint8_t> item = abc::value_t(string{"вторая, правленная"}).dump();
	// Выполняем правку второй записи контейнера
	ASSERT_TRUE(editor.replace(1, item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что количество записей правкой не изменилось
	ASSERT_EQ(editor.records(), 3ull);
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что прежние октеты записи обращены в мусор
	ASSERT_GT(editor.garbage(), 0ull);
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Код отказа выборки записи контейнера
	abc::error_t error = abc::error_t::NONE;
	// Выполняем выборку правленной записи с носителя
	ASSERT_TRUE(this->pick(medium, 1, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку того, что строка оглавления указывает на новую запись
	ASSERT_EQ(picked, item);
	// Выполняем выборку соседней записи контейнера
	ASSERT_TRUE(this->pick(medium, 2, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку того, что правка соседей не тронула
	ASSERT_EQ(picked, abc::value_t(string{"третья"}).dump());
}
/**
 * @brief Проверка сноса записи контейнера
 *
 */
TEST_F(EditorFixture, EraseMarks) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с тремя записями
	this->build(medium, {"первая", "вторая", "третья"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем снос второй записи контейнера
	ASSERT_TRUE(editor.erase(1)) << "код отказа: " << abc::message(editor.error());
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Код отказа выборки записи контейнера
	abc::error_t error = abc::error_t::NONE;
	// Выполняем проверку отказа выборки снесённой записи
	ASSERT_FALSE(this->pick(medium, 1, picked, error));
	// Выполняем проверку кода отказа выборки снесённой записи
	ASSERT_EQ(error, abc::error_t::MISSING_RECORD);
	/**
	 * Выполняем проверку того, что номера соседей сносом не сдвинулись: строка
	 * снесённой записи остаётся в оглавлении помеченной, а не изымается
	 */
	ASSERT_TRUE(this->pick(medium, 2, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, abc::value_t(string{"третья"}).dump());
}
/**
 * @brief Проверка чтения накопленного до фиксации
 *
 * @details Решение владельца: данные уже в памяти, и отдавать их до фиксации ничего не
 *          стоит, а не отдавать - значит обязать потребителя держать их вторым списком
 *
 */
TEST_F(EditorFixture, PendingReadable) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем сборку дописываемой записи
	const vector <uint8_t> item = abc::value_t(string{"накопленная"}).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем сборку новой записи взамен прежней
	const vector <uint8_t> edited = abc::value_t(string{"правленная, но не закреплённая"}).dump();
	// Выполняем правку первой записи контейнера
	ASSERT_TRUE(editor.replace(0, edited.data(), edited.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Выполняем выборку накопленной, но ещё не закреплённой записи
	ASSERT_TRUE(editor.record(1, picked)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, item);
	// Выполняем выборку правленной, но ещё не закреплённой записи
	ASSERT_TRUE(editor.record(0, picked)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что выдана новая запись, а не лежащая на носителе
	ASSERT_EQ(picked, edited);
	// Код отказа выборки записи контейнера
	abc::error_t error = abc::error_t::NONE;
	/**
	 * Выполняем проверку того, что на носителе накопленного ещё нет: накопленное
	 * читается правщиком, а фиксация его на носитель ещё не сходила
	 */
	ASSERT_FALSE(this->pick(medium, 1, picked, error));
}
/**
 * @brief Проверка сохранности накопленного при отказе фиксации
 *
 */
TEST_F(EditorFixture, CommitFailureKeepsPending) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем сборку дописываемой записи
	const vector <uint8_t> item = abc::value_t(string{"накопленная"}).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем объявление отказа работы записи октетов
	medium.broken = true;
	// Выполняем проверку отказа фиксации накопленных правок
	ASSERT_FALSE(editor.commit());
	// Выполняем проверку кода отказа фиксации накопленных правок
	ASSERT_EQ(editor.error(), abc::error_t::UNWRITABLE_SINK);
	// Выполняем проверку того, что накопленное отказом не сброшено
	ASSERT_EQ(editor.records(), 2ull);
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Выполняем проверку того, что накопленная запись цела
	ASSERT_TRUE(editor.record(1, picked)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, item);
	// Выполняем снятие объявления отказа работы записи октетов
	medium.broken = false;
	// Выполняем повторную фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Код отказа выборки записи контейнера
	abc::error_t error = abc::error_t::NONE;
	// Выполняем выборку накопленной записи с носителя
	ASSERT_TRUE(this->pick(medium, 1, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, item);
}
/**
 * @brief Проверка отката к прежнему поколению по хвостовому заголовку
 *
 */
TEST_F(EditorFixture, TailHeaderRecovery) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем сборку дописываемой записи
	const vector <uint8_t> item = abc::value_t(string{"вторая"}).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	/**
	 * Выполняем порчу головного заголовка опознания: так выглядит обрыв посреди
	 * правки его, ибо головной заголовок правится последним
	 */
	medium.data.at(4) ^= 0xFF;
	// Правщик контейнера, открываемый после порчи головного заголовка
	abc::editor_t restored;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(restored, medium)) << "код отказа: " << abc::message(restored.error());
	/**
	 * Выполняем проверку того, что взят хвостовой заголовок: поколение его равно
	 * поколению, какое положила фиксация
	 */
	ASSERT_EQ(restored.header().generation, 1ull);
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Выполняем выборку дописанной записи по хвостовому заголовку
	ASSERT_TRUE(restored.record(1, picked)) << "код отказа: " << abc::message(restored.error());
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, item);
}
/**
 * @brief Проверка пропуска прежнего оглавления подрядным чтением
 *
 * @details Прежнее оглавление остаётся на носителе внутри тела нового поколения, и
 *          подрядное чтение обязано его пропустить, а не выдать записью
 *
 */
TEST_F(EditorFixture, WasteSkipped) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем сборку дописываемой записи
	const vector <uint8_t> item = abc::value_t(string{"вторая"}).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Сниматель контейнера
	abc::loader_t loader;
	// Выполняем подачу правленного контейнера снимателю
	ASSERT_TRUE(loader.feed(medium.data.data(), medium.data.size()));
	// Собираемое содержимое всех снятых кадров
	vector <uint8_t> payload;
	// Содержимое очередного снятого кадра
	vector <uint8_t> chunked;
	// Сведения об очередном снятом кадре
	abc::chunk_t chunk;
	// Количество снятых кадров
	size_t count = 0;
	// Выполняем вычитывание всех кадров правленного контейнера
	while(loader.next(chunked, chunk)){
		// Выполняем увеличение количества снятых кадров
		count++;
		// Выполняем внесение содержимого снятого кадра
		payload.insert(payload.end(), chunked.begin(), chunked.end());
	}
	// Выполняем проверку того, что снято два кадра записей, а не три с оглавлением
	ASSERT_EQ(count, 2ul);
	// Собираемое ожидаемое содержимое кадров правленного контейнера
	vector <uint8_t> expected = abc::value_t(string{"первая"}).dump();
	// Выполняем внесение дописанной записи в ожидаемое содержимое
	expected.insert(expected.end(), item.begin(), item.end());
	// Выполняем проверку содержимого снятых кадров
	ASSERT_EQ(payload, expected);
}
/**
 * @brief Проверка самочинной фиксации по количеству накопленных правок
 *
 */
TEST_F(EditorFixture, AutomaticCommit) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Получаем настройки правки контейнера
	abc::editor_t::settings_t settings = editor.settings();
	// Выполняем установку способа фиксации по количеству накопленных правок
	settings.mode = abc::editor_t::mode_t::RECORDS;
	// Выполняем установку порога самочинной фиксации в две правки
	settings.limit = 2;
	// Выполняем установку настроек правки контейнера
	editor.settings(settings);
	// Выполняем сборку первой дописываемой записи
	const vector <uint8_t> first = abc::value_t(string{"вторая"}).dump();
	// Выполняем дописывание первой записи в конец контейнера
	ASSERT_TRUE(editor.append(first.data(), first.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что одна правка фиксации ещё не вызвала
	ASSERT_EQ(editor.header().generation, 0ull);
	// Выполняем сборку второй дописываемой записи
	const vector <uint8_t> second = abc::value_t(string{"третья"}).dump();
	// Выполняем дописывание второй записи в конец контейнера
	ASSERT_TRUE(editor.append(second.data(), second.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что порог правок вызвал фиксацию сам
	ASSERT_EQ(editor.header().generation, 1ull);
	// Выполняем проверку того, что накопленных правок не осталось
	ASSERT_EQ(editor.pending(), 0ul);
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Код отказа выборки записи контейнера
	abc::error_t error = abc::error_t::NONE;
	// Выполняем выборку второй дописанной записи с носителя
	ASSERT_TRUE(this->pick(medium, 2, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, second);
}
/**
 * @brief Проверка уборки мусора перестройкой контейнера
 *
 */
TEST_F(EditorFixture, CompactRebuilds) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с четырьмя записями
	this->build(medium, {"первая", "вторая", "третья", "четвёртая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем сборку новой записи взамен прежней
	const vector <uint8_t> item = abc::value_t(string{"вторая, правленная"}).dump();
	// Выполняем правку второй записи контейнера
	ASSERT_TRUE(editor.replace(1, item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем снос третьей записи контейнера
	ASSERT_TRUE(editor.erase(2)) << "код отказа: " << abc::message(editor.error());
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что мусор правкой накопился
	ASSERT_GT(editor.garbage(), 0ull);
	// Носитель, куда следует убрать контейнер
	Medium cleaned;
	// Полная длина убранного контейнера
	uint64_t length = 0;
	// Выполняем уборку мусора перестройкой контейнера
	ASSERT_TRUE(editor.compact([&cleaned](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
		// Выполняем запись поданных октетов убранного контейнера
		return cleaned.write(offset, buffer, size);
	}, abc::payload_t::TEXT, length)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку полной длины убранного контейнера
	ASSERT_EQ(length, static_cast <uint64_t> (cleaned.data.size()));
	// Выполняем проверку того, что убранный контейнер вышел короче правленного
	ASSERT_LT(cleaned.data.size(), medium.data.size());
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Код отказа выборки записи контейнера
	abc::error_t error = abc::error_t::NONE;
	// Выполняем выборку правленной записи из убранного контейнера
	ASSERT_TRUE(this->pick(cleaned, 1, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку того, что уборка перенесла нынешнее содержимое записи
	ASSERT_EQ(picked, item);
	/**
	 * Выполняем проверку того, что номера записей уборкой не сдвинулись: строка
	 * снесённой записи сохранена пустою, а номера живут и вне контейнера
	 */
	ASSERT_TRUE(this->pick(cleaned, 3, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, abc::value_t(string{"четвёртая"}).dump());
	// Выполняем проверку отказа выборки снесённой записи из убранного контейнера
	ASSERT_FALSE(this->pick(cleaned, 2, picked, error));
	// Выполняем проверку кода отказа выборки снесённой записи
	ASSERT_EQ(error, abc::error_t::MISSING_RECORD);
	// Правщик убранного контейнера
	abc::editor_t opened;
	// Выполняем открытие убранного контейнера правщиком
	ASSERT_TRUE(this->open(opened, cleaned)) << "код отказа: " << abc::message(opened.error());
	// Выполняем проверку того, что мусора в убранном контейнере не осталось
	ASSERT_EQ(opened.garbage(), 0ull);
	// Выполняем проверку того, что количество записей уборкой сохранено
	ASSERT_EQ(opened.records(), 4ull);
}
/**
 * @brief Проверка закрепления накопленных правок уборкой
 *
 */
TEST_F(EditorFixture, CompactCommitsPending) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем сборку дописываемой записи
	const vector <uint8_t> item = abc::value_t(string{"накопленная"}).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Носитель, куда следует убрать контейнер
	Medium cleaned;
	// Полная длина убранного контейнера
	uint64_t length = 0;
	// Выполняем уборку мусора перестройкой контейнера
	ASSERT_TRUE(editor.compact([&cleaned](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
		// Выполняем запись поданных октетов убранного контейнера
		return cleaned.write(offset, buffer, size);
	}, abc::payload_t::TEXT, length)) << "код отказа: " << abc::message(editor.error());
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Код отказа выборки записи контейнера
	abc::error_t error = abc::error_t::NONE;
	/**
	 * Выполняем проверку того, что накопленная правка уборкой не потеряна:
	 * уборка обязана закрепить её прежде, чем убирать по оглавлению
	 */
	ASSERT_TRUE(this->pick(cleaned, 1, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, item);
}
/**
 * @brief Проверка фиксации по сроку, поверяемому при обращении
 *
 */
TEST_F(EditorFixture, DeadlineCommit) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Получаем настройки правки контейнера
	abc::editor_t::settings_t settings = editor.settings();
	// Выполняем установку способа фиксации по сроку, поверяемому при обращении
	settings.mode = abc::editor_t::mode_t::DEADLINE;
	// Выполняем установку срока самочинной фиксации
	settings.delay = 40;
	// Выполняем установку настроек правки контейнера
	editor.settings(settings);
	// Выполняем сборку первой дописываемой записи
	const vector <uint8_t> first = abc::value_t(string{"вторая"}).dump();
	// Выполняем дописывание первой записи в конец контейнера
	ASSERT_TRUE(editor.append(first.data(), first.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что срок ещё не наступил и фиксации не было
	ASSERT_EQ(editor.header().generation, 0ull);
	// Выполняем ожидание наступления срока
	this_thread::sleep_for(chrono::milliseconds(60));
	// Выполняем сборку второй дописываемой записи
	const vector <uint8_t> second = abc::value_t(string{"третья"}).dump();
	/**
	 * Выполняем дописывание второй записи: срок поверяется тем же вызовом, каким
	 * вносится запись, оттого фиксация обязана произойти именно здесь
	 */
	ASSERT_TRUE(editor.append(second.data(), second.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что наступивший срок вызвал фиксацию
	ASSERT_EQ(editor.header().generation, 1ull);
	// Выполняем проверку того, что накопленных правок не осталось
	ASSERT_EQ(editor.pending(), 0ul);
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Код отказа выборки записи контейнера
	abc::error_t error = abc::error_t::NONE;
	// Выполняем выборку второй дописанной записи с носителя
	ASSERT_TRUE(this->pick(medium, 2, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, second);
}
/**
 * @brief Проверка фиксации по сроку, отбиваемому своим потоком
 *
 * @details Отбой своим потоком тем и отличен от поверки при обращении, что срок
 *          наступает и в тишине: записей больше не вносят, а накопленное закрепляется
 *
 */
TEST_F(EditorFixture, ThreadedCommit) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Получаем настройки правки контейнера
	abc::editor_t::settings_t settings = editor.settings();
	// Выполняем установку способа фиксации по сроку, отбиваемому своим потоком
	settings.mode = abc::editor_t::mode_t::THREAD;
	// Выполняем установку срока самочинной фиксации
	settings.delay = 30;
	// Выполняем установку настроек правки контейнера
	editor.settings(settings);
	// Выполняем сборку дописываемой записи
	const vector <uint8_t> item = abc::value_t(string{"вторая"}).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	/**
	 * Выполняем ожидание наступления срока, не обращаясь к правщику вовсе:
	 * обращение обратило бы проверку в проверку поверки при обращении
	 */
	this_thread::sleep_for(chrono::milliseconds(150));
	// Выполняем проверку того, что срок вызвал фиксацию в тишине
	ASSERT_EQ(editor.header().generation, 1ull);
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Код отказа выборки записи контейнера
	abc::error_t error = abc::error_t::NONE;
	// Выполняем выборку дописанной записи с носителя
	ASSERT_TRUE(this->pick(medium, 1, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, item);
	// Получаем настройки правки контейнера
	settings = editor.settings();
	// Выполняем возврат к ручному способу фиксации
	settings.mode = abc::editor_t::mode_t::MANUAL;
	/**
	 * Выполняем установку настроек правки контейнера: остановка отбоя срока
	 * дожидается конца своего потока, и затяжки замка тут быть не должно
	 */
	editor.settings(settings);
	// Выполняем сборку второй дописываемой записи
	const vector <uint8_t> second = abc::value_t(string{"третья"}).dump();
	// Выполняем дописывание второй записи в конец контейнера
	ASSERT_TRUE(editor.append(second.data(), second.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем ожидание, вдвое превышающее срок отбоя
	this_thread::sleep_for(chrono::milliseconds(100));
	// Выполняем проверку того, что остановленный отбой срока фиксации не вызывает
	ASSERT_EQ(editor.header().generation, 1ull);
}
/**
 * @brief Проверка подписи, положенной фиксацией правок
 *
 * @details Всякая фиксация кладёт свою подпись: поколение сменилось, а подпись прежнего
 *          поколения на новое тело не сходится и сходиться не должна
 *
 */
TEST_F(EditorFixture, SignedCommit) {
	// Выполняем заведение ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с двумя записями
	this->build(medium, {"первая", "вторая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем объявление подписи правимого контейнера
	ASSERT_TRUE(editor.sign(this->_crypto.get(), "владелец")) << "код отказа: " << abc::message(editor.error());
	// Выполняем сборку дописываемой записи
	const vector <uint8_t> item = abc::value_t(string{"третья"}).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Код отказа поверки подписи владельца
	abc::error_t error = abc::error_t::NONE;
	/**
	 * Выполняем поверку подписи владельца правленного контейнера: правка положила
	 * свою подпись, и та обязана сойтись на новом теле
	 */
	ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", medium.data.data(), medium.data.size(), error))
		<< "код отказа: " << abc::message(error);
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Выполняем выборку дописанной записи с носителя
	ASSERT_TRUE(this->pick(medium, 2, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, item);
	// Выполняем сборку второй дописываемой записи
	const vector <uint8_t> second = abc::value_t(string{"четвёртая"}).dump();
	// Выполняем дописывание второй записи в конец контейнера
	ASSERT_TRUE(editor.append(second.data(), second.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем повторную фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	/**
	 * Выполняем поверку подписи после второй фиксации: дерево свёрток ведётся
	 * дописыванием, и вторая фиксация обязана сойтись наравне с первой
	 */
	ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", medium.data.data(), medium.data.size(), error))
		<< "код отказа: " << abc::message(error);
	// Выполняем проверку того, что поколение записи контейнера возросло дважды
	ASSERT_EQ(editor.header().generation, 2ull);
	// Выполняем порчу одного октета тела правленного контейнера
	medium.data.at(abc::HEADER_LENGTH + abc::CHUNK_HEADER + 1) ^= 0xFF;
	// Выполняем проверку отказа поверки подписи после порчи тела
	ASSERT_FALSE(abc::verify(* this->_crypto, "владелец", medium.data.data(), medium.data.size(), error));
	// Выполняем проверку кода отказа поверки подписи
	ASSERT_EQ(error, abc::error_t::REFUSED_SIGNATURE);
}
/**
 * @brief Проверка того, что мусорные кадры подписью учтены
 *
 * @details Прежнее оглавление остаётся мусором внутри тела нового поколения, и подпись
 *          обязана считать его наравне с прочим: иначе подмена мусорного кадра прошла
 *          бы мимо поверки
 *
 */
TEST_F(EditorFixture, SignedWasteCounted) {
	// Выполняем заведение ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ECDSA));
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем объявление подписи правимого контейнера
	ASSERT_TRUE(editor.sign(this->_crypto.get(), "владелец")) << "код отказа: " << abc::message(editor.error());
	// Выполняем получение смещения прежнего оглавления контейнера
	const uint64_t waste = editor.header().index;
	// Выполняем сборку дописываемой записи
	const vector <uint8_t> item = abc::value_t(string{"вторая"}).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Код отказа поверки подписи владельца
	abc::error_t error = abc::error_t::NONE;
	// Выполняем поверку подписи владельца правленного контейнера
	ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", medium.data.data(), medium.data.size(), error))
		<< "код отказа: " << abc::message(error);
	// Выполняем порчу одного октета мусорного кадра прежнего оглавления
	medium.data.at(static_cast <size_t> (waste) + abc::CHUNK_HEADER) ^= 0xFF;
	// Выполняем проверку отказа поверки подписи после порчи мусорного кадра
	ASSERT_FALSE(abc::verify(* this->_crypto, "владелец", medium.data.data(), medium.data.size(), error));
	// Выполняем проверку кода отказа поверки подписи
	ASSERT_EQ(error, abc::error_t::REFUSED_SIGNATURE);
}
/**
 * @brief Проверка отказов правки контейнера
 *
 * @details Молчаливое согласие правщика на негодный довод опаснее отказа: строка
 * оглавления увела бы выборку в произвольное место, а пустая запись легла бы
 * кадром, неотличимым от оборванного
 *
 */
TEST_F(EditorFixture, Refusals) {
	// Правщик неоткрытого контейнера
	abc::editor_t closed;
	// Выполняем сборку записи правки
	const vector <uint8_t> item = abc::value_t(string{"запись"}).dump();
	// Выполняем проверку отказа дописывания в неоткрытый контейнер
	ASSERT_FALSE(closed.append(item.data(), item.size(), abc::payload_t::TEXT));
	// Выполняем проверку отказа правки записи неоткрытого контейнера
	ASSERT_FALSE(closed.replace(0, item.data(), item.size(), abc::payload_t::TEXT));
	// Выполняем проверку отказа сноса записи неоткрытого контейнера
	ASSERT_FALSE(closed.erase(0));
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Выполняем проверку отказа выборки записи неоткрытого контейнера
	ASSERT_FALSE(closed.record(0, picked));
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с двумя записями
	this->build(medium, {"первая", "вторая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку количества записей открытого контейнера
	ASSERT_EQ(editor.records(), 2ull);
	/**
	 * Выполняем проверку отказов правки записи, какой в контейнере нет: номер
	 * записи приходит извне, и доверять ему нельзя
	 */
	ASSERT_FALSE(editor.replace(2, item.data(), item.size(), abc::payload_t::TEXT));
	// Выполняем проверку отказа сноса записи, какой в контейнере нет
	ASSERT_FALSE(editor.erase(2));
	// Выполняем проверку отказа выборки записи, какой в контейнере нет
	ASSERT_FALSE(editor.record(2, picked));
	// Выполняем проверку отказа дописывания несуществующего буфера
	ASSERT_FALSE(editor.append(nullptr, item.size(), abc::payload_t::TEXT));
	// Выполняем проверку отказа дописывания пустой записи
	ASSERT_FALSE(editor.append(item.data(), 0, abc::payload_t::TEXT));
	// Выполняем снос первой записи контейнера
	ASSERT_TRUE(editor.erase(0)) << "код отказа: " << abc::message(editor.error());
	// Выполняем получение количества октетов, обращённых сносом в мусор
	const uint64_t garbage = editor.garbage();
	/**
	 * Выполняем проверку того, что повторный снос снесённой записи безобиден:
	 * разряд сноса уже стоит, и отказом такой снос отвечать незачем
	 */
	ASSERT_TRUE(editor.erase(0)) << "код отказа: " << abc::message(editor.error());
	/**
	 * Выполняем проверку того, что повторный снос мусора не приписал: длина
	 * снесённого, посчитанная дважды, увела бы уборку по ложному порогу
	 */
	ASSERT_EQ(editor.garbage(), garbage);
	// Выполняем проверку отказа выборки снесённой записи
	ASSERT_FALSE(editor.record(0, picked));
	// Выполняем проверку кода отказа выборки снесённой записи
	ASSERT_EQ(editor.error(), abc::error_t::MISSING_RECORD);
	/**
	 * Выполняем проверку того, что правка снесённой записи дозволена: снос ставит
	 * разряд, а не изымает строку, и правка возвращает запись к жизни
	 */
	ASSERT_TRUE(editor.replace(0, item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что правленная запись читается
	ASSERT_TRUE(editor.record(0, picked)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, item);
}
/**
 * @brief Проверка сброса правщика контейнера
 *
 * @details Сброс возвращает правщик к неоткрытому виду: накопленное отбрасывается,
 * а работы чтения и записи забываются - иначе следующее открытие писало бы на
 * прежний носитель
 *
 */
TEST_F(EditorFixture, ResetForgets) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Полная длина контейнера до правки
	const size_t length = medium.data.size();
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем сборку дописываемой записи
	const vector <uint8_t> item = abc::value_t(string{"накопленная"}).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем проверку количества записей вместе с накопленной
	ASSERT_EQ(editor.records(), 2ull);
	// Выполняем сброс правщика контейнера
	editor.reset();
	// Выполняем проверку того, что количество записей сброшено
	ASSERT_EQ(editor.records(), 0ull);
	// Выполняем проверку того, что мусор сброшен
	ASSERT_EQ(editor.garbage(), 0ull);
	// Выполняем проверку отказа дописывания в сброшенный правщик
	ASSERT_FALSE(editor.append(item.data(), item.size(), abc::payload_t::TEXT));
	/**
	 * Выполняем проверку того, что накопленное сбросом на носитель не ушло:
	 * фиксации не было, и носитель обязан остаться прежним
	 */
	ASSERT_EQ(medium.data.size(), length);
	// Выполняем открытие контейнера сброшенным правщиком наново
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что открыт прежний контейнер без накопленного
	ASSERT_EQ(editor.records(), 1ull);
}
