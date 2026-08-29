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
			// Признак отказа работы чтения октетов
			bool blind = false;
			// Количество удавшихся записей, после какого работа записи отказывает, −1 - без предела
			int allow = -1;
			// Количество записей, сделанных на носитель
			int writes = 0;
			// Наибольший размер, затребованный работою чтения октетов
			size_t largest = 0;
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
				/**
				 * Выполняем учёт наибольшего затребованного размера
				 *
				 * @note Учёт этот нужен поверке сторожей длины кадра: сторож ловится не
				 * кодом отказа, а тем, СКОЛЬКО правка затребует у источника прежде отказа
				 */
				if(size > this->largest)
					// Выполняем запоминание наибольшего затребованного размера
					this->largest = size;
				// Если работа чтения объявлена отказавшей
				if(this->blind)
					// Выводим признак неудачного чтения октетов
					return false;
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
				// Выполняем учёт очередной записи на носитель
				this->writes++;
				// Если предел удавшихся записей объявлен и исчерпан
				if((this->allow >= 0) && (this->writes > this->allow))
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
				abc::assembler_t assembler(this->_log.get());
				// Выполняем перебор всех собираемых записей контейнера
				for(const string & text : records){
					// Выполняем сборку очередной записи
					const vector <uint8_t> item = abc::value_t(text).dump();
					// Выполняем внесение очередной записи в собираемый контейнер
					ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT));
				}
				// Выполняем завершение сборки контейнера
				ASSERT_TRUE(assembler.complete(medium.data));
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
				abc::fetcher_t fetcher(this->_log.get());
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
	abc::editor_t editor(this->_log.get());
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
	abc::editor_t editor(this->_log.get());
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
	abc::editor_t editor(this->_log.get());
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
	abc::editor_t editor(this->_log.get());
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
	abc::editor_t editor(this->_log.get());
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
	abc::editor_t editor(this->_log.get());
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
	abc::editor_t restored(this->_log.get());
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
	abc::editor_t editor(this->_log.get());
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
	abc::loader_t loader(this->_log.get());
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
	abc::editor_t editor(this->_log.get());
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
	abc::editor_t editor(this->_log.get());
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
	abc::editor_t opened(this->_log.get());
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
	abc::editor_t editor(this->_log.get());
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
 * @brief Проверка сохранения подписи уборкой мусора
 *
 * @details Уборка складывает контейнер заново на другом носителе, и подпись ей приходится
 *          вырабатывать наново: тело у убранного контейнера иное, и прежняя подпись к нему
 *          не подходит. Проверяется, что подпись эта на убранном контейнере сходится, а
 *          отпечаток владельца и признак подписанности уборкой сохранены
 *
 */
TEST_F(EditorFixture, CompactKeepsSignature) {
	// Выполняем заведение ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с тремя записями
	this->build(medium, {"первая", "вторая", "третья"});
	// Правщик контейнера
	abc::editor_t editor(this->_log.get());
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем объявление подписи правимого контейнера
	ASSERT_TRUE(editor.sign(this->_crypto.get(), "владелец")) << "код отказа: " << abc::message(editor.error());
	// Выполняем снос второй записи контейнера
	ASSERT_TRUE(editor.erase(1)) << "код отказа: " << abc::message(editor.error());
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
	// Код отказа поверки подписи владельца
	abc::error_t error = abc::error_t::NONE;
	/**
	 * Выполняем поверку подписи владельца убранного контейнера: уборка сложила тело
	 * заново, и подпись обязана сойтись именно на новом теле
	 */
	ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", cleaned.data.data(), cleaned.data.size(), error, this->_log.get()))
		<< "код отказа: " << abc::message(error);
	// Буфер выбранной записи убранного контейнера
	vector <uint8_t> picked;
	// Выполняем выборку уцелевшей записи из убранного контейнера
	ASSERT_TRUE(this->pick(cleaned, 2, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, abc::value_t(string{"третья"}).dump());
	// Правщик убранного контейнера
	abc::editor_t opened(this->_log.get());
	// Выполняем открытие убранного контейнера правщиком
	ASSERT_TRUE(this->open(opened, cleaned)) << "код отказа: " << abc::message(opened.error());
	// Выполняем проверку того, что признак подписанности уборкой сохранён
	ASSERT_TRUE(opened.header().is(abc::flag_t::SIGNED));
	// Выполняем проверку того, что мусора в убранном контейнере не осталось
	ASSERT_EQ(opened.garbage(), 0ull);
	// Выполняем порчу одного октета тела убранного контейнера
	cleaned.data.at(abc::HEADER_LENGTH + abc::CHUNK_HEADER + 1) ^= 0xFF;
	// Выполняем проверку отказа поверки подписи после порчи тела
	ASSERT_FALSE(abc::verify(* this->_crypto, "владелец", cleaned.data.data(), cleaned.data.size(), error, this->_log.get()));
}
/**
 * @brief Проверка снятия подписи уборкой контейнера без подписывающего
 *
 * @details Подпись вырабатывается уборкой наново, а вырабатывать её нечем, если
 *          подписывающий правщику не объявлен. Убранный контейнер обязан выйти
 *          неподписанным честно: с погашенным признаком, обнулённым местом подписи и
 *          вычищенным отпечатком, - а не с признаком подписанности при отсутствующей подписи
 *
 */
TEST_F(EditorFixture, CompactWithoutSignerDropsSignature) {
	// Выполняем заведение ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с двумя записями
	this->build(medium, {"первая", "вторая"});
	// Правщик контейнера
	abc::editor_t editor(this->_log.get());
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем объявление подписи правимого контейнера
	ASSERT_TRUE(editor.sign(this->_crypto.get(), "владелец")) << "код отказа: " << abc::message(editor.error());
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Носитель, несущий подписанный контейнер
	Medium signed_;
	// Выполняем перенос подписанного контейнера на отдельный носитель
	signed_.data = medium.data;
	// Правщик подписанного контейнера, подписывающего которому не объявлено
	abc::editor_t plain(this->_log.get());
	// Выполняем открытие подписанного контейнера правщиком без подписывающего
	ASSERT_TRUE(this->open(plain, signed_)) << "код отказа: " << abc::message(plain.error());
	// Выполняем проверку того, что открытый контейнер подписан
	ASSERT_TRUE(plain.header().is(abc::flag_t::SIGNED));
	// Носитель, куда следует убрать контейнер
	Medium cleaned;
	// Полная длина убранного контейнера
	uint64_t length = 0;
	// Выполняем уборку мусора перестройкой контейнера
	ASSERT_TRUE(plain.compact([&cleaned](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
		// Выполняем запись поданных октетов убранного контейнера
		return cleaned.write(offset, buffer, size);
	}, abc::payload_t::TEXT, length)) << "код отказа: " << abc::message(plain.error());
	// Правщик убранного контейнера
	abc::editor_t opened(this->_log.get());
	// Выполняем открытие убранного контейнера правщиком
	ASSERT_TRUE(this->open(opened, cleaned)) << "код отказа: " << abc::message(opened.error());
	// Выполняем проверку того, что признак подписанности уборкой погашен
	ASSERT_FALSE(opened.header().is(abc::flag_t::SIGNED));
	// Выполняем проверку того, что место подписи убранного контейнера обнулено
	ASSERT_EQ(opened.header().signature, 0ull);
	// Выполняем проверку того, что записи убранного контейнера уцелели
	ASSERT_EQ(opened.records(), 2ull);
	// Код отказа поверки подписи владельца
	abc::error_t error = abc::error_t::NONE;
	// Выполняем проверку отказа поверки подписи у неподписанного контейнера
	ASSERT_FALSE(abc::verify(* this->_crypto, "владелец", cleaned.data.data(), cleaned.data.size(), error, this->_log.get()));
}
/**
 * @brief Проверка отката фиксации, оборванной отказом записи на носитель
 *
 * @details Фиксация переписывает оглавление целиком и вырабатывает подпись наново, а
 *          носитель волен отказать посреди этой работы. Проверяется обрыв на всяком месте
 *          подряд: прежнее поколение контейнера обязано пережить обрыв целиком - подпись
 *          его сходится, а все записи читаются, - ибо новые кадры ложатся ЗА концом
 *          прежнего тела, а заголовок с новым местом оглавления пишется последним
 *
 * @note Обрыв поверяется по одному отказу за проверку: правщик заводится наново на всякий
 *       круг, иначе состояние его тянулось бы от круга к кругу и место отказа терялось бы
 *
 */
TEST_F(EditorFixture, CommitRollbackKeepsPreviousGeneration) {
	// Выполняем заведение ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Носитель, несущий подписанный контейнер прежнего поколения
	Medium origin;
	{
		// Выполняем сборку контейнера с тремя записями
		this->build(origin, {"первая", "вторая", "третья"});
		// Правщик контейнера
		abc::editor_t editor(this->_log.get());
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, origin)) << "код отказа: " << abc::message(editor.error());
		// Выполняем объявление подписи правимого контейнера
		ASSERT_TRUE(editor.sign(this->_crypto.get(), "владелец")) << "код отказа: " << abc::message(editor.error());
		// Выполняем фиксацию подписи прежнего поколения на носителе
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	}
	// Код отказа поверки подписи владельца
	abc::error_t error = abc::error_t::NONE;
	// Выполняем проверку того, что подпись прежнего поколения сходится
	ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", origin.data.data(), origin.data.size(), error, this->_log.get()))
		<< "код отказа: " << abc::message(error);
	// Количество отказавших фиксаций за весь обход
	size_t refused = 0;
	/**
	 * Выполняем обход всех мест обрыва записи на носитель
	 */
	for(int allow = 0; allow < 8; allow++){
		// Носитель круга, несущий снимок прежнего поколения
		Medium medium;
		// Выполняем снятие снимка прежнего поколения на носитель круга
		medium.data = origin.data;
		// Правщик контейнера круга
		abc::editor_t editor(this->_log.get());
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем объявление подписи правимого контейнера
		ASSERT_TRUE(editor.sign(this->_crypto.get(), "владелец")) << "код отказа: " << abc::message(editor.error());
		// Выполняем сборку дописываемой записи
		const vector <uint8_t> item = abc::value_t(string{"четвёртая"}).dump();
		// Выполняем дописывание записи в конец контейнера
		ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(editor.error());
		// Выполняем объявление предела удавшихся записей на носитель
		medium.allow = allow;
		// Выполняем учёт исхода фиксации, оборванной отказом записи
		if(!editor.commit())
			// Выполняем учёт отказавшей фиксации
			refused++;
		// Выполняем снятие предела удавшихся записей на носитель
		medium.allow = -1;
		/**
		 * Выполняем поверку подписи носителя круга: обрыв на любом месте обязан оставить
		 * прежнее поколение целым, ибо новые кадры ложатся за концом прежнего тела
		 */
		ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", medium.data.data(), medium.data.size(), error, this->_log.get()))
			<< "обрыв на записи " << allow << ", код отказа: " << abc::message(error);
		// Буфер выбранной записи контейнера
		vector <uint8_t> picked;
		/**
		 * Выполняем выборку всех записей прежнего поколения
		 */
		for(uint64_t number = 0; number < 3; number++){
			// Выполняем выборку очередной записи с носителя круга
			ASSERT_TRUE(this->pick(medium, number, picked, error))
				<< "обрыв на записи " << allow << ", запись " << number
				<< ", код отказа: " << abc::message(error);
		}
	}
	// Выполняем проверку того, что обход застал отказы фиксации
	ASSERT_GT(refused, 0u);
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
	abc::editor_t editor(this->_log.get());
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
	abc::editor_t editor(this->_log.get());
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
	abc::editor_t editor(this->_log.get());
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
	ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", medium.data.data(), medium.data.size(), error, this->_log.get()))
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
	ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", medium.data.data(), medium.data.size(), error, this->_log.get()))
		<< "код отказа: " << abc::message(error);
	// Выполняем проверку того, что поколение записи контейнера возросло дважды
	ASSERT_EQ(editor.header().generation, 2ull);
	// Выполняем порчу одного октета тела правленного контейнера
	medium.data.at(abc::HEADER_LENGTH + abc::CHUNK_HEADER + 1) ^= 0xFF;
	// Выполняем проверку отказа поверки подписи после порчи тела
	ASSERT_FALSE(abc::verify(* this->_crypto, "владелец", medium.data.data(), medium.data.size(), error, this->_log.get()));
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
	abc::editor_t editor(this->_log.get());
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
	ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", medium.data.data(), medium.data.size(), error, this->_log.get()))
		<< "код отказа: " << abc::message(error);
	// Выполняем порчу одного октета мусорного кадра прежнего оглавления
	medium.data.at(static_cast <size_t> (waste) + abc::CHUNK_HEADER) ^= 0xFF;
	// Выполняем проверку отказа поверки подписи после порчи мусорного кадра
	ASSERT_FALSE(abc::verify(* this->_crypto, "владелец", medium.data.data(), medium.data.size(), error, this->_log.get()));
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
	abc::editor_t closed(this->_log.get());
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
	abc::editor_t editor(this->_log.get());
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
	abc::editor_t editor(this->_log.get());
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

/**
 * @brief Проверка сноса записи, правленной в том же наборе накопленного
 *
 * @details Правки копятся списком, и содержимым записи стоит правка её ПОСЛЕДНЯЯ. Снос,
 *          пришедший вслед за правкой той же записи, обязан её и отменить: правка,
 *          ответившая успехом, обязана изменить содержимое контейнера
 *
 * @note Вскрыто ворошителем 22.08.2026 ходом «erase 0; replace 1; erase 1»: снесённая
 *       запись выдавалась и после фиксации
 *
 */
TEST_F(EditorFixture, EraseAfterReplace) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с тремя записями
	this->build(medium, {"первая", "вторая", "третья"});
	// Правщик контейнера
	abc::editor_t editor(this->_log.get());
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Октеты записи, какою правится вторая запись контейнера
	const vector <uint8_t> item = abc::value_t(string{"правленая"}).dump();
	// Выполняем правку второй записи контейнера
	ASSERT_TRUE(editor.replace(1, item.data(), item.size(), abc::payload_t::MIXED))
	 << "код отказа: " << abc::message(editor.error());
	// Выполняем снос той же второй записи контейнера
	ASSERT_TRUE(editor.erase(1)) << "код отказа: " << abc::message(editor.error());
	// Буфер снятой записи контейнера
	vector <uint8_t> taken;
	// Выполняем проверку того, что снесённая запись не выдаётся и до фиксации
	ASSERT_FALSE(editor.record(1, taken)) << "снесённая запись выдана до фиксации";
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что снесённая запись не выдаётся и после фиксации
	ASSERT_FALSE(editor.record(1, taken)) << "снесённая запись выдана после фиксации";
	// Код отказа выборки записи контейнера
	abc::error_t error = abc::error_t::NONE;
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Выполняем проверку отказа выборки снесённой записи с носителя
	ASSERT_FALSE(this->pick(medium, 1, picked, error));
	// Выполняем проверку кода отказа выборки снесённой записи
	ASSERT_EQ(error, abc::error_t::MISSING_RECORD);
	// Выполняем проверку того, что соседняя запись сносом не задета
	ASSERT_TRUE(this->pick(medium, 2, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку выбранной соседней записи контейнера
	ASSERT_EQ(picked, abc::value_t(string{"третья"}).dump());
}

/**
 * @brief Проверка правки записи, снесённой в том же наборе накопленного
 *
 * @details Порядок обратный: снос, а следом правка той же записи. Побеждать обязано
 *          действие последнее - правка воскрешает снесённое. Направление это работало
 *          и прежде, и закреплено оно ради того, чтобы правка сноса его не сломала
 *
 */
TEST_F(EditorFixture, ReplaceAfterErase) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с тремя записями
	this->build(medium, {"первая", "вторая", "третья"});
	// Правщик контейнера
	abc::editor_t editor(this->_log.get());
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем снос второй записи контейнера
	ASSERT_TRUE(editor.erase(1)) << "код отказа: " << abc::message(editor.error());
	// Октеты записи, какою воскрешается снесённая
	const vector <uint8_t> item = abc::value_t(string{"воскрешённая"}).dump();
	// Выполняем правку снесённой записи контейнера
	ASSERT_TRUE(editor.replace(1, item.data(), item.size(), abc::payload_t::MIXED))
	 << "код отказа: " << abc::message(editor.error());
	// Буфер снятой записи контейнера
	vector <uint8_t> taken;
	// Выполняем проверку выдачи воскрешённой записи до фиксации
	ASSERT_TRUE(editor.record(1, taken)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку содержимого воскрешённой записи до фиксации
	ASSERT_EQ(taken, item);
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Код отказа выборки записи контейнера
	abc::error_t error = abc::error_t::NONE;
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Выполняем проверку выборки воскрешённой записи с носителя
	ASSERT_TRUE(this->pick(medium, 1, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку содержимого воскрешённой записи на носителе
	ASSERT_EQ(picked, item);
}
/**
 * @brief Проверка того, что отвергнутое объявление подписи подписанта не оставляет
 *
 * @details Сбор свёрток по кадрам тела читает носитель, и отказ чтения оставлял дерево
 *          собранным наполовину. Подписант при том оставался объявленным, и следующая
 *          фиксация докладывала УСПЕХ, кладя контейнер, объявляющий себя подписанным,
 *          а поверка подписи его отвечала расхождением. Такой контейнер хуже
 *          неподписанного - он выглядит подделанным
 *
 * @note Проверка требует, чтобы контейнер вышел ЧЕСТНО неподписанным: и признака
 *       подписанности нет, и записи целы. Отказ фиксации годным итогом не считается -
 *       правка обязана довестись до конца, лишившись лишь подписи
 *
 */
TEST_F(EditorFixture, RefusedSignLeavesNoSigner) {
	// Носитель контейнера в памяти
	Medium medium;
	// Выполняем сборку контейнера о шести записях
	this->build(medium, {"первая", "вторая", "третья", "четвёртая", "пятая", "шестая"});
	// Выполняем проверку того, что контейнер собран
	ASSERT_FALSE(medium.data.empty());
	// Выполняем установку соли шифрования
	this->_crypto->salt("соль контейнера");
	// Выполняем установку пароля шифрования
	this->_crypto->password("пароль владельца");
	// Выполняем выработку ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Правщик контейнера
	abc::editor_t editor(this->_log.get());
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем объявление отказа работы чтения октетов
	medium.blind = true;
	/**
	 * Выполняем проверку того, что объявление подписи при слепом носителе отвергается
	 */
	ASSERT_FALSE(editor.sign(this->_crypto.get(), "владелец"));
	// Выполняем возврат работы чтения октетов
	medium.blind = false;
	// Выполняем сборку дописываемой записи
	const vector <uint8_t> item = abc::value_t(string{"дописанная"}).dump();
	// Выполняем дописывание записи в контейнер
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем фиксацию накопленных правок
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Снятый заголовок опознания контейнера
	abc::header_t header;
	// Код отказа снятия заголовка
	abc::error_t error = abc::error_t::NONE;
	// Выполняем снятие заголовка опознания контейнера с носителя
	ASSERT_TRUE(header.unpack(medium.data.data(), medium.data.size(), error))
		<< "код отказа: " << abc::message(error);
	/**
	 * Выполняем проверку того, что контейнер подписанным себя НЕ объявляет
	 */
	ASSERT_FALSE(header.is(abc::flag_t::SIGNED)) << "контейнер объявил себя подписанным";
	// Выборщик записей контейнера
	abc::fetcher_t fetcher(this->_log.get());
	// Выполняем открытие контейнера выборщиком
	ASSERT_TRUE(fetcher.open([&medium](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем чтение затребованных октетов контейнера
		return medium.read(offset, size, result);
	})) << "код отказа: " << abc::message(fetcher.error());
	// Выполняем проверку количества записей открытого контейнера
	ASSERT_EQ(fetcher.records(), static_cast <uint64_t> (7));
	/**
	 * Выполняем проверку того, что всякая запись контейнера цела
	 */
	for(uint64_t i = 0; i < fetcher.records(); i++){
		// Буфер выбранной записи контейнера
		vector <uint8_t> record;
		// Выполняем проверку того, что запись контейнера выбирается
		ASSERT_TRUE(fetcher.record(i, record)) << "код отказа: " << abc::message(fetcher.error())
			<< " на записи " << i;
	}
}
/**
 * @brief Проверка того, что повторная фиксация после отказавшей записей не двоит
 *
 * @details Фиксация вносит строки в оглавление ПО ХОДУ записи кадров: смещение кадра
 *          известно лишь по записи его. Отказ посреди фиксации оставлял внесённое жить,
 *          а накопленные правки при том не снимались, и повторная фиксация вносила их
 *          ЗАНОВО. Шесть записей плюс четыре дописанных давали ЧЕТЫРНАДЦАТЬ, и все
 *          четырнадцать читались целыми - удвоение выходило молчаливым
 *
 * @note Обрыв наводится на всякой по счёту записи носителя: точка обрыва решает, что
 *       успело лечь в оглавление, и одной точки было бы мало
 *
 */
TEST_F(EditorFixture, RecommitDoesNotDouble) {
	// Носитель исходного контейнера
	Medium pristine;
	// Выполняем сборку контейнера о шести записях
	this->build(pristine, {"первая", "вторая", "третья", "четвёртая", "пятая", "шестая"});
	// Выполняем проверку того, что контейнер собран
	ASSERT_FALSE(pristine.data.empty());
	/**
	 * Выполняем наведение обрыва на всякой по счёту записи носителя
	 */
	for(int limit = 1; limit <= 8; limit++){
		// Носитель правимого контейнера
		Medium medium;
		// Выполняем перенос октетов исходного контейнера на носитель
		medium.data = pristine.data;
		// Правщик контейнера
		abc::editor_t editor(this->_log.get());
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "предел записей: " << limit;
		/**
		 * Выполняем дописывание череды записей в контейнер
		 */
		for(size_t i = 0; i < 4; i++){
			// Выполняем сборку очередной дописываемой записи
			const vector <uint8_t> item = abc::value_t(string{"дописанная-"} + to_string(i)).dump();
			// Выполняем дописывание очередной записи в контейнер
			ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
				<< "предел записей: " << limit;
		}
		// Выполняем объявление предела удавшихся записей на носитель
		medium.allow = limit;
		// Выполняем первую фиксацию, какой суждено оборваться либо удаться
		const bool first = editor.commit();
		// Выполняем снятие предела удавшихся записей
		medium.allow = -1;
		/**
		 * Выполняем повторную фиксацию: носитель починен, и правка обязана довестись
		 */
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error())
			<< ", предел записей: " << limit;
		// Выборщик записей контейнера
		abc::fetcher_t fetcher(this->_log.get());
		// Выполняем открытие правленого контейнера выборщиком
		ASSERT_TRUE(fetcher.open([&medium](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
			// Выполняем чтение затребованных октетов контейнера
			return medium.read(offset, size, result);
		})) << "код отказа: " << abc::message(fetcher.error()) << ", предел записей: " << limit;
		/**
		 * Выполняем проверку количества записей: шесть исходных и четыре дописанных,
		 * сколько бы фиксаций ни потребовалось
		 */
		ASSERT_EQ(fetcher.records(), static_cast <uint64_t> (10))
			<< "предел записей: " << limit << ", первая фиксация: " << (first ? "удалась" : "отказала");
		/**
		 * Выполняем проверку того, что всякая запись контейнера цела
		 */
		for(uint64_t i = 0; i < fetcher.records(); i++){
			// Буфер выбранной записи контейнера
			vector <uint8_t> record;
			// Выполняем проверку того, что запись контейнера выбирается
			ASSERT_TRUE(fetcher.record(i, record)) << "код отказа: " << abc::message(fetcher.error())
				<< " на записи " << i << ", предел записей: " << limit;
		}
		// Выполняем проверку того, что поколение записи контейнера возросло единожды
		ASSERT_EQ(fetcher.header().generation, static_cast <uint64_t> (1)) << "предел записей: " << limit;
	}
}
/**
 * @brief Проверка сторожа объявленной длины кадра у правки контейнера
 *
 * @details Длина содержимого кадра прочитана из САМОГО контейнера и недоверенна, а
 *          вычитывается по ней кадр целиком. Сторож этот стоял у выборки записей, а у
 *          правки его не было вовсе, хотя ввод у обеих один: подделка четырёх октетов
 *          длины заставляла правку затребовать у источника октетов 4 294 967 327 -
 *          замерено щупом 29.08.2026 на контейнере в 316 октетов
 *
 * @note Ловится сторож не кодом отказа, а тем, СКОЛЬКО правка затребует у источника
 *       прежде отказа: без сторожа отказ приходит тоже - от источника, отдавшего меньше
 *       затребованного, - и по коду отказа два случая неотличимы. Проверка оттого и
 *       сличает наибольший затребованный размер, а не одно лишь слово отказа
 *
 * @note Подделка контрольной суммы кадра здесь не нужна: длина читается из заголовка
 *       ПРЕЖДЕ сличения суммы, и непомерное чтение происходит до всякой поверки
 *
 */
TEST_F(EditorFixture, ChunkLengthGuard) {
	// Носитель октетов контейнера
	Medium medium;
	// Выполняем сборку контейнера с четырьмя записями
	this->build(medium, {"первая", "вторая", "третья", "четвёртая"});
	// Выполняем проверку того, что контейнер собран
	ASSERT_FALSE(medium.data.empty());
	/**
	 * Выполняем подделку объявленной длины ПЕРВОГО кадра тела контейнера
	 *
	 * @note Первый кадр лежит сразу за заголовком опознания, а длина содержимого его -
	 * четырьмя октетами со смещения 4 в заголовке кадра
	 */
	abc::fixed(medium.data.data() + abc::HEADER_LENGTH + 4, 0xFFFFFFFFull, 4);
	// Правщик контейнера
	abc::editor_t editor(this->_log.get());
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем сброс наибольшего затребованного размера
	medium.largest = 0;
	// Буфер выбираемой записи контейнера
	vector <uint8_t> item;
	// Выполняем проверку того, что выборка записи отвечена отказом
	ASSERT_FALSE(editor.record(0, item));
	// Выполняем проверку того, что отказ объявлен повреждённым кадром
	ASSERT_EQ(editor.error(), abc::error_t::INVALID_CHUNK);
	/**
	 * Выполняем проверку того, что непомерного чтения у источника не затребовано
	 *
	 * @note Предел взят длиною самого контейнера: правка вправе вычитать из него что
	 * угодно, но затребовать БОЛЬШЕ, чем контейнер весит, ей неоткуда - разве что по
	 * длине, подделанной в самом контейнере
	 */
	ASSERT_LE(medium.largest, medium.data.size())
		<< "затребовано у источника: " << medium.largest << " при контейнере в " << medium.data.size();
}
