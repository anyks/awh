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
#include <sys/log.hpp>

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
			// Количество удавшихся чтений, после какого работа чтения отказывает, −1 - без предела
			int sight = -1;
			// Количество чтений, сделанных с носителя
			int reads = 0;
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
				/**
				 * Если предел удавшихся чтений объявлен и исчерпан
				 *
				 * @note Предел этот нужен поверке отката: отказ чтения обязан приходиться
				 * на середину работы, а не на первое же обращение её к носителю
				 */
				if((this->sight >= 0) && (this->reads >= this->sight))
					// Выводим признак неудачного чтения октетов
					return false;
				// Выполняем учёт сделанного чтения с носителя
				this->reads++;
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
				// Выполняем заведение объекта сжатия данных
				this->_compressor = make_unique <compressor::block_t> ();
				// Выполняем заведение объекта шифрования данных
				this->_crypto = make_unique <crypto_t> ();
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
 * @brief Проверка сохранения подписи уборкой мусора
 *
 * @details Уборка складывает контейнер заново на другом носителе, и подпись ей приходится
 *          вырабатывать наново: тело у убранного контейнера иное, и прежняя подпись к нему
 *          не подходит. Проверяется, что подпись эта на убранном контейнере сходится, а
 *          отпечаток владельца и признак подписанности уборкой сохранены
 *
 */
/**
 * @brief Проверка отката состояния при отказе чтения посреди фиксации
 *
 * @details Фиксация помечает прежнее оглавление мусором и УВЕЛИЧИВАЕТ счёт мусора
 * прежде, чем вычитать кадры для дерева свёрток. Отказ чтения на этом отрезке обязан
 * вернуть состояние к прежнему виду: незакреплённые правки остаются, и повторная
 * фиксация обязана дать тот же итог, что и без отказа
 *
 * @note Поверяется счёт мусора, а не код отказа: код тот же и при откате, и без него,
 * а расхождение видно лишь удвоением счёта по второй фиксации
 *
 */
TEST_F(EditorFixture, CommitRollbackOnReadFailure) {
	// Выполняем заведение ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с тремя записями
	this->build(medium, {"первая", "вторая", "третья"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем объявление подписи правимого контейнера
	ASSERT_TRUE(editor.sign(this->_crypto.get(), "владелец")) << "код отказа: " << abc::message(editor.error());
	// Выполняем снос второй записи контейнера
	ASSERT_TRUE(editor.erase(1)) << "код отказа: " << abc::message(editor.error());
	// Выполняем запоминание счёта мусора до первой фиксации
	const uint64_t garbage = editor.garbage();
	/**
	 * Выполняем перебор пределов удавшихся чтений, накрывая весь отрезок фиксации
	 *
	 * @details Мест чтения у фиксации несколько, и какое из них ляжет за увеличением
	 * счёта мусора, разбором кода не решается: перебор накрывает их все разом
	 *
	 * @note Правщик заводится СВОЙ на всякий предел, и это не расточительство. Перебор
	 * на одном правщике кончался на первом же успехе: удавшаяся фиксация клала правки
	 * на носитель, и круги следом звали её у правщика чистого, где отказывать нечему.
	 * Выглядел он полным, а накрывал лишь начало отрезка - непокрытыми оставались
	 * ИМЕННО глубокие чтения фиксации, ради каких перебор и заведён
	 */
	for(int sight = 0; sight < 24; sight++){
		// Носитель, несущий правимый контейнер этого круга
		Medium probe;
		// Выполняем сборку контейнера с тремя записями
		this->build(probe, {"первая", "вторая", "третья"});
		// Правщик контейнера этого круга
		abc::editor_t retried;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(retried, probe)) << "предел чтений: " << sight;
		// Выполняем объявление подписи правимого контейнера
		ASSERT_TRUE(retried.sign(this->_crypto.get(), "владелец")) << "предел чтений: " << sight;
		/**
		 * Выполняем снос второй записи и закрепление его фиксацией беспрепятственной
		 *
		 * @note Круг этот заводится ради того, чтобы контейнер стал подписанным НА
		 * НОСИТЕЛЕ. Глубокие чтения фиксации - снятие кадра прежней записи подписи и
		 * внесение его в дерево свёрток мусором - идут лишь у контейнера УЖЕ
		 * подписанного, а первая фиксация подпись только кладёт. Без этого круга
		 * перебор накрывал начало отрезка и до откатов глубоких не доходил вовсе
		 */
		ASSERT_TRUE(retried.erase(1)) << "предел чтений: " << sight;
		// Первая фиксация обязана лечь на носитель беспрепятственно
		ASSERT_TRUE(retried.commit()) << "предел чтений: " << sight;
		// Выполняем снос третьей записи контейнера
		ASSERT_TRUE(retried.erase(2)) << "предел чтений: " << sight;
		// Выполняем запоминание счёта мусора до фиксации поверяемой
		const uint64_t counted = retried.garbage();
		// Выполняем объявление предела удавшихся чтений носителя
		probe.sight = sight;
		// Выполняем сброс счёта сделанных чтений носителя
		probe.reads = 0;
		// Если фиксация отказом не отвечена, предел этот отказа не вызывает
		if(retried.commit())
			// Переходим к следующему пределу удавшихся чтений
			continue;
		// Счёт мусора обязан вернуться к прежнему виду откатом
		ASSERT_EQ(retried.garbage(), counted) << "предел чтений: " << sight;
	}
	// Счёт мусора правщика, перебором не тронутого, обязан остаться прежним
	ASSERT_EQ(editor.garbage(), garbage);
	// Выполняем снятие предела удавшихся чтений носителя
	medium.sight = -1;
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	/**
	 * Выполняем проверку того, что правки легли на носитель однажды
	 *
	 * @note Подпись здесь НЕ поверяется: откат, не сумевший собрать дерево свёрток
	 * наново (носитель на тот миг ещё отказывал чтением), снимает подписанта намеренно -
	 * подписывать по наполовину собранному дереву нельзя. Поверка подписи после отказа
	 * чтения ловила бы это намеренное решение, а не двойной учёт
	 */
	// Выборщик записей правленого контейнера
	abc::fetcher_t fetcher;
	// Выполняем открытие правленого контейнера выборщиком
	ASSERT_TRUE(fetcher.open([&medium](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем чтение затребованных октетов контейнера
		return medium.read(offset, size, result);
	})) << "код отказа: " << abc::message(fetcher.error());
	// Правленый контейнер обязан нести три записи: снос вторую лишь помечает
	ASSERT_EQ(fetcher.records(), static_cast <uint64_t> (3));
}
/**
 * @brief Проверка укладки кадром при смене вида содержимого
 *
 * @details Кадр несёт ОДИН вид содержимого: он выбирает и сжатие, и способ укладки, -
 * оттого смена вида посреди накопления обязана уложить накопленное кадром прежде, чем
 * принять запись нового вида. Без того записи двух видов легли бы в один кадр, и вид,
 * объявленный кадром, отвечал бы лишь части их
 *
 * @note Порог укладки поднят намеренно: без того кадр уложился бы по набору размера, и
 * проверка ловила бы не смену вида, а переполнение
 *
 */
TEST_F(EditorFixture, PayloadKindSplitsChunk) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем получение настроек правки контейнера
	abc::editor_t::settings_t settings = editor.settings();
	// Выполняем установку размера кадра, вмещающего все записи разом
	settings.block = 65536;
	// Выполняем установку настроек правки контейнера
	editor.settings(settings);
	// Дописываемые записи знакового текста
	const vector <string> texts = {"вторая", "третья"};
	/**
	 * Выполняем дописывание записей знакового текста
	 */
	for(const string & text : texts){
		// Выполняем сборку очередной дописываемой записи
		const vector <uint8_t> item = abc::value_t(text).dump();
		// Выполняем дописывание очередной записи знакового текста
		ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(editor.error());
	}
	// Дописываемая запись сырых октетов
	const vector <uint8_t> raw = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
	/**
	 * Выполняем дописывание записи сырых октетов: вид содержимого сменился, и
	 * накопленное обязано лечь кадром прежде принятия её
	 */
	ASSERT_TRUE(editor.append(raw.data(), raw.size(), abc::payload_t::BINARY))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	/**
	 * Выполняем проверку того, что записи двух видов легли РАЗНЫМИ кадрами
	 *
	 * @details Утверждается смещение кадра, а не целость записей: без укладки при смене
	 * вида записи прочитались бы исправно всё равно - вид кадра выбирает лишь способ
	 * сжатия, а не разбор, - и проверка на целость проходила бы при снятой укладке,
	 * поверяя ровно ничего. Разными кадрами их делает ИМЕННО она
	 */
	// Строки оглавления правленого контейнера
	const vector <abc::entry_t> & entries = editor.index().entries();
	// Оглавление обязано нести исходную запись, две текстовых и одну двоичную
	ASSERT_EQ(entries.size(), static_cast <size_t> (4));
	// Записи знакового текста легли одним кадром
	ASSERT_EQ(entries.at(1).chunk, entries.at(2).chunk);
	// Запись сырых октетов легла кадром ОТДЕЛЬНЫМ
	ASSERT_NE(entries.at(3).chunk, entries.at(2).chunk);
	// Выборщик записей правленого контейнера
	abc::fetcher_t fetcher;
	// Выполняем открытие правленого контейнера выборщиком
	ASSERT_TRUE(fetcher.open([&medium](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем чтение затребованных октетов контейнера
		return medium.read(offset, size, result);
	})) << "код отказа: " << abc::message(fetcher.error());
	// Правленый контейнер обязан нести исходную запись, две текстовых и одну двоичную
	ASSERT_EQ(fetcher.records(), static_cast <uint64_t> (4));
	// Буфер выбранной записи правленого контейнера
	vector <uint8_t> picked;
	/**
	 * Выполняем перебор дописанных записей знакового текста
	 */
	for(size_t i = 0; i < texts.size(); i++){
		// Выполняем выборку очередной дописанной записи
		ASSERT_TRUE(fetcher.record(static_cast <uint64_t> (i + 1), picked)) << "запись: " << i;
		// Выполняем проверку того, что выбранная запись отвечает дописанной
		ASSERT_EQ(picked, abc::value_t(texts.at(i)).dump()) << "запись: " << i;
	}
	// Выполняем выборку дописанной записи сырых октетов
	ASSERT_TRUE(fetcher.record(static_cast <uint64_t> (3), picked));
	/**
	 * Выполняем проверку того, что запись сырых октетов вышла целой
	 *
	 * @note Записи легли РАЗНЫМИ кадрами, и целость её тем и доказывается: лягши в один
	 * кадр с текстом, она читалась бы видом, объявленным не о ней
	 */
	ASSERT_EQ(picked, raw);
}
/**
 * @brief Проверка возврата перенаправленных строк оглавления при отказе фиксации
 *
 * @details Фиксация вносит строки в оглавление ПО ХОДУ записи кадров, и откат ведётся
 * журналом перенаправленных строк, а не копией оглавления: строк бывают миллионы, а
 * правок за фиксацию немного. Журнал этот прежде не поверялся вовсе - откат звался лишь
 * там, где перенаправить ещё ничего не успели, и тело его не исполнялось ни разу
 *
 * @note Отказ ведётся пределом УДАВШИХСЯ записей, а не отказом всякой: перенаправление
 * случается между записями кадров, и отказ обязан прийти ПОСЛЕ него
 *
 */
TEST_F(EditorFixture, CommitRollbackReturnsRedirected) {
	// Заменяющая запись правимого контейнера
	const vector <uint8_t> replacement = abc::value_t(string{"заменённая"}).dump();
	/**
	 * Выполняем перебор пределов удавшихся записей, накрывая всю фиксацию
	 */
	for(int allow = 1; allow < 8; allow++){
		// Носитель, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера с тремя записями
		this->build(medium, {"первая", "вторая", "третья"});
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "предел записей: " << allow;
		// Выполняем запоминание строки оглавления правимой записи
		const abc::entry_t entry = editor.index().entries().at(1);
		// Выполняем запоминание счёта мусора до фиксации
		const uint64_t garbage = editor.garbage();
		// Выполняем перезапись второй записи контейнера
		ASSERT_TRUE(editor.replace(1, replacement.data(), replacement.size(), abc::payload_t::TEXT))
			<< "предел записей: " << allow;
		// Выполняем объявление предела удавшихся записей носителя
		medium.allow = allow;
		// Если фиксация отказом не отвечена, предел этот отказа не вызывает
		if(editor.commit())
			// Переходим к следующему пределу удавшихся записей
			continue;
		// Строка оглавления правимой записи обязана вернуться к прежнему виду
		const abc::entry_t & returned = editor.index().entries().at(1);
		// Смещение кадра строки обязано вернуться к прежнему
		ASSERT_EQ(returned.chunk, entry.chunk) << "предел записей: " << allow;
		// Смещение записи в кадре обязано вернуться к прежнему
		ASSERT_EQ(returned.offset, entry.offset) << "предел записей: " << allow;
		// Длина записи обязана вернуться к прежней
		ASSERT_EQ(returned.length, entry.length) << "предел записей: " << allow;
		// Счёт мусора обязан вернуться к прежнему
		ASSERT_EQ(editor.garbage(), garbage) << "предел записей: " << allow;
		// Оглавление обязано сохранить прежнее количество строк
		ASSERT_EQ(editor.index().size(), static_cast <size_t> (3)) << "предел записей: " << allow;
	}
}
/**
 * @brief Проверка передачи правщиком модулей сжатия и шифрования укладчику кадров
 *
 * @details Правщик модулей этих не держит - он передаёт их своему укладчику кадров, - и
 * поверить передачу можно лишь тем, что кадр вышел уложенным ИМИ: содержимое, уложенное
 * со сжатием и шифрованием, без тех же модулей не читается вовсе
 *
 * @note Утверждается ОБА исхода: чтение с модулями и отказ без них. Одно лишь удавшееся
 * чтение проходило бы и при правщике, модули отбросившем, - кадр вышел бы неукрытым, а
 * читался бы точно так же
 *
 */
TEST_F(EditorFixture, ModulesReachPacker) {
	// Выполняем установку соли шифрования содержимого кадров
	this->_crypto->salt("соль контейнера");
	// Выполняем установку пароля шифрования содержимого кадров
	this->_crypto->password("пароль владельца");
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем передачу правщику модуля сжатия содержимого кадров
	editor.compressor(this->_compressor.get());
	// Выполняем передачу правщику модуля шифрования содержимого кадров
	editor.crypto(this->_crypto.get());
	// Дописываемая запись, укрываемая сжатием и шифрованием
	const vector <uint8_t> item = abc::value_t(string(512, 'a')).dump();
	// Выполняем дописывание записи в правимый контейнер
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	/**
	 * Выполняем поверку того, что укрытая запись без модулей НЕ читается
	 */
	{
		// Выборщик записей правленого контейнера без модулей
		abc::fetcher_t bare;
		// Выполняем открытие правленого контейнера выборщиком
		ASSERT_TRUE(bare.open([&medium](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
			// Выполняем чтение затребованных октетов контейнера
			return medium.read(offset, size, result);
		})) << "код отказа: " << abc::message(bare.error());
		// Буфер выбранной записи правленого контейнера
		vector <uint8_t> picked;
		// Укрытая запись без модулей читаться не должна
		ASSERT_FALSE(bare.record(1, picked));
	}
	// Выборщик записей правленого контейнера
	abc::fetcher_t fetcher;
	// Выполняем передачу выборщику модуля сжатия содержимого кадров
	fetcher.compressor(this->_compressor.get());
	// Выполняем передачу выборщику модуля шифрования содержимого кадров
	fetcher.crypto(this->_crypto.get());
	// Выполняем открытие правленого контейнера выборщиком
	ASSERT_TRUE(fetcher.open([&medium](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем чтение затребованных октетов контейнера
		return medium.read(offset, size, result);
	})) << "код отказа: " << abc::message(fetcher.error());
	// Буфер выбранной записи правленого контейнера
	vector <uint8_t> picked;
	// Укрытая запись с теми же модулями обязана прочитаться
	ASSERT_TRUE(fetcher.record(1, picked)) << "код отказа: " << abc::message(fetcher.error());
	// Прочитанная запись обязана отвечать дописанной
	ASSERT_EQ(picked, item);
	/**
	 * Полная длина правленого контейнера обязана быть объявлена правщиком
	 *
	 * @note Длина эта ведётся записью и опросом при открытии, а не пересчётом: поверка
	 * её сличением с носителем и стережёт ведение это
	 */
	ASSERT_EQ(editor.length(), static_cast <uint64_t> (medium.data.size()));
}
TEST_F(EditorFixture, CompactOffsetWidthGuard) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Собираемые записи правимого контейнера
	vector <string> items;
	// Выполняем сборку двух десятков записей по сорок октетов
	for(size_t i = 0; i < 20; i++)
		// Выполняем внесение очередной собранной записи
		items.push_back(string(40, static_cast <char> ('a' + (i % 26))));
	// Выполняем сборку контейнера собранными записями
	this->build(medium, items);
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем получение настроек правки контейнера
	abc::editor_t::settings_t settings = editor.settings();
	// Выполняем установку размера кадра, вмещающего все записи разом
	settings.block = 4096;
	// Выполняем установку настроек правки контейнера
	editor.settings(settings);
	// Носитель, куда следует убрать контейнер
	Medium cleaned;
	// Полная длина убранного контейнера
	uint64_t length = 0;
	// Выполняем уборку мусора перестройкой контейнера
	ASSERT_TRUE(editor.compact([&cleaned](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
		// Выполняем запись поданных октетов убранного контейнера
		return cleaned.write(offset, buffer, size);
	}, abc::payload_t::TEXT, length)) << "код отказа: " << abc::message(editor.error());
	// Выборщик записей убранного контейнера
	abc::fetcher_t fetcher;
	// Выполняем открытие убранного контейнера выборщиком
	ASSERT_TRUE(fetcher.open([&cleaned](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем чтение затребованных октетов контейнера
		return cleaned.read(offset, size, result);
	})) << "код отказа: " << abc::message(fetcher.error());
	// Убранный контейнер обязан нести все собранные записи
	ASSERT_EQ(fetcher.records(), static_cast <uint64_t> (items.size()));
	/**
	 * Выполняем перебор всех записей убранного контейнера
	 */
	for(uint64_t i = 0; i < fetcher.records(); i++){
		// Буфер выбранной записи убранного контейнера
		vector <uint8_t> picked;
		// Выполняем выборку очередной записи убранного контейнера
		ASSERT_TRUE(fetcher.record(i, picked)) << "запись: " << i;
		// Выполняем сборку записи того вида, каким она укладывалась контейнером
		const vector <uint8_t> item = abc::value_t(items.at(static_cast <size_t> (i))).dump();
		// Выполняем проверку того, что выбранная запись отвечает собранной
		ASSERT_EQ(picked, item) << "запись: " << i;
	}
}
TEST_F(EditorFixture, CompactKeepsSignature) {
	// Выполняем заведение ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с тремя записями
	this->build(medium, {"первая", "вторая", "третья"});
	// Правщик контейнера
	abc::editor_t editor;
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
	ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", cleaned.data.data(), cleaned.data.size(), error))
		<< "код отказа: " << abc::message(error);
	// Буфер выбранной записи убранного контейнера
	vector <uint8_t> picked;
	// Выполняем выборку уцелевшей записи из убранного контейнера
	ASSERT_TRUE(this->pick(cleaned, 2, picked, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, abc::value_t(string{"третья"}).dump());
	// Правщик убранного контейнера
	abc::editor_t opened;
	// Выполняем открытие убранного контейнера правщиком
	ASSERT_TRUE(this->open(opened, cleaned)) << "код отказа: " << abc::message(opened.error());
	// Выполняем проверку того, что признак подписанности уборкой сохранён
	ASSERT_TRUE(opened.header().is(abc::flag_t::SIGNED));
	/**
	 * Выполняем проверку того, что отпечаток ключа владельца в заголовке не пуст и
	 * отвечает ключу
	 *
	 * @note Заслона укладки отпечатка утверждение это НЕ закрепляет: заголовок убранного
	 *       контейнера наследует отпечаток прежнего, и при ТОМ ЖЕ ключе новая выработка
	 *       от унаследованного значения неотличима. Закрепляет его
	 *       `EditorFixture.CompactionStoresTheFingerprintOfTheCurrentSigner`, где уборка
	 *       идёт ключом ИНЫМ, нежели подписан прежний контейнер
	 */
	{
		// Буфер отпечатка открытого ключа владельца
		vector <uint8_t> print;
		// Выполняем выработку отпечатка ключа владельца
		ASSERT_TRUE(abc::fingerprint(* this->_crypto, "владелец", print))
			<< "отпечаток ключа владельца не выработан";
		// Отпечаток, уложенный уборкой в заголовок убранного контейнера
		const vector <uint8_t> stored(opened.header().fingerprint,
		 opened.header().fingerprint + abc::FINGERPRINT_LENGTH);
		// Выполняем проверку того, что отпечаток в заголовке не пуст
		ASSERT_NE(stored, vector <uint8_t> (abc::FINGERPRINT_LENGTH, 0x00))
			<< "уборка оставила отпечаток ключа пустым";
		// Выполняем проверку того, что отпечаток отвечает ключу владельца
		ASSERT_EQ(stored, vector <uint8_t> (print.begin(), print.begin() + abc::FINGERPRINT_LENGTH));
	}
	// Выполняем проверку того, что мусора в убранном контейнере не осталось
	ASSERT_EQ(opened.garbage(), 0ull);
	// Выполняем порчу одного октета тела убранного контейнера
	cleaned.data.at(abc::HEADER_LENGTH + abc::CHUNK_HEADER + 1) ^= 0xFF;
	// Выполняем проверку отказа поверки подписи после порчи тела
	ASSERT_FALSE(abc::verify(* this->_crypto, "владелец", cleaned.data.data(), cleaned.data.size(), error));
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
	abc::editor_t editor;
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
	abc::editor_t plain;
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
	abc::editor_t opened;
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
	ASSERT_FALSE(abc::verify(* this->_crypto, "владелец", cleaned.data.data(), cleaned.data.size(), error));
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
		abc::editor_t editor;
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
	ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", origin.data.data(), origin.data.size(), error))
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
		abc::editor_t editor;
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
		/**
		 * Выполняем учёт исхода фиксации, оборванной отказом записи
		 *
		 * @note Сличается не один лишь ИСХОД, но и НАЗВАННАЯ причина: отказ носителя
		 *       обязан называться отказом носителя, а не чем попало. До 05.09.2026
		 *       причину здесь не спрашивал никто, и сплошной щуп по местам отказа
		 *       показал четырнадцать мест `UNWRITABLE_SINK`, подмена каких не красила
		 *       ничего. Обход же обрывает запись на РАЗНЫХ шагах, оттого одно и то же
		 *       место назвать нельзя - зато можно потребовать, чтобы причина
		 *       принадлежала записи и была объявлена непременно
		 */
		if(!editor.commit()){
			// Выполняем учёт отказавшей фиксации
			refused++;
			// Выполняем проверку того, что отказ причину объявил
			ASSERT_NE(editor.error(), abc::error_t::NONE) << "обрыв на записи " << allow;
			// Выполняем проверку того, что причина принадлежит записи на носитель
			ASSERT_TRUE((editor.error() == abc::error_t::UNWRITABLE_SINK) ||
			 (editor.error() == abc::error_t::SIGNING_FAILED))
			 << "обрыв на записи " << allow << ", причина: " << abc::message(editor.error());
		}
		// Выполняем снятие предела удавшихся записей на носитель
		medium.allow = -1;
		/**
		 * Выполняем поверку подписи носителя круга: обрыв на любом месте обязан оставить
		 * прежнее поколение целым, ибо новые кадры ложатся за концом прежнего тела
		 */
		ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", medium.data.data(), medium.data.size(), error))
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
	abc::editor_t editor;
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
	abc::editor_t editor;
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
	abc::editor_t editor;
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
	abc::fetcher_t fetcher;
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
		abc::editor_t editor;
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
		abc::fetcher_t fetcher;
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
	abc::editor_t editor;
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
/**
 * @brief Проверка того, что счёт мусора означает мусор НА НОСИТЕЛЕ
 *
 * @details Договор работы `garbage()` гласит «количество октетов мусора на носителе», и
 *          проверка держит его тремя утверждениями сразу: до фиксации счёт НЕ РАСТЁТ ни
 *          от сноса, ни от правки - носитель ими не тронут; фиксация досчитывает и
 *          вытесненную запись, и кадр прежнего оглавления; а свежее открытие того же
 *          носителя счёт ВОССТАНАВЛИВАЕТ, а не начинает с нуля
 *
 * @note Прежде проверка эта закрепляла ИНОЕ - что снос считает при зове, а правка при
 *       закреплении, - и закрепляла честно, «таким, каково оно есть», доводом «замерено,
 *       а не выведено». Расхождение это решено владельцем 02.09.2026 в пользу договора:
 *       считать надлежит носитель. Замер до правки: после `erase` и ДО фиксации счёт
 *       показывал 13 при нетронутом носителе, а повторное открытие того же носителя со
 *       154 октетами мусора отвечало нулём
 *
 * @warning Счёт при открытии - НИЖНЯЯ ГРАНИЦА, и утверждается он неравенством намеренно.
 *          Записей, вытесненных правкой внутри живого кадра, на носителе не сыскать:
 *          строка оглавления вытесненной записи перезаписана новой. Утверждай проверка
 *          равенство - она требовала бы от кодека невозможного без смены вида записи
 *
 */
TEST_F(EditorFixture, GarbageMeansTheWasteOnTheMedium) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с четырьмя записями
	this->build(medium, {"первая", "вторая", "третья", "четвёртая"});
	// Мусор, сосчитанный по закреплении правок
	uint64_t committed = 0;
	// Длина снесённой записи контейнера
	uint64_t second = 0;
	/**
	 * Выполняем правку контейнера и поверку счёта мусора по ходу её
	 */
	{
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Свежесобранный контейнер мусора нести не обязан
		ASSERT_EQ(editor.garbage(), 0ull) << "свежий контейнер объявил мусор";
		// Выполняем снятие длины второй записи по оглавлению
		second = static_cast <uint64_t> (editor.index().entries().at(1).length);
		// Выполняем снятие длины третьей записи по оглавлению
		const uint64_t third = static_cast <uint64_t> (editor.index().entries().at(2).length);
		// Длина второй записи обязана быть отлична от нуля
		ASSERT_GT(second, 0ull);
		// Длина третьей записи обязана быть отлична от нуля
		ASSERT_GT(third, 0ull);
		// Выполняем снос второй записи контейнера
		ASSERT_TRUE(editor.erase(1)) << "код отказа: " << abc::message(editor.error());
		/**
		 * Снос мусора НЕ прибавляет, покуда правка не закреплена: снос копится в
		 * памяти, и носителя он до фиксации не трогает вовсе
		 */
		ASSERT_EQ(editor.garbage(), 0ull) << "снос досчитан мусором до фиксации";
		// Выполняем сборку новой записи взамен третьей
		const vector <uint8_t> item = abc::value_t(string{"третья, правленная"}).dump();
		// Выполняем правку третьей записи контейнера
		ASSERT_TRUE(editor.replace(2, item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(editor.error());
		// Правка мусора не прибавляет по тому же доводу, что и снос
		ASSERT_EQ(editor.garbage(), 0ull) << "правка досчитана мусором до фиксации";
		// Выполняем закрепление накопленных правок на носителе
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
		/**
		 * Закрепление обязано досчитать снесённую запись, вытесненную запись и кадр
		 * прежнего оглавления целиком: величина последнего зависит от сжатия и здесь
		 * не сличается
		 */
		ASSERT_GT(editor.garbage(), second + third) << "фиксация досчитала не всё";
		// Выполняем запоминание счёта мусора по закреплении
		committed = editor.garbage();
	}
	/**
	 * Выполняем поверку счёта мусора при СВЕЖЕМ открытии того же носителя
	 */
	{
		// Правщик того же контейнера
		abc::editor_t opened;
		// Выполняем открытие правленного контейнера правщиком
		ASSERT_TRUE(this->open(opened, medium)) << "код отказа: " << abc::message(opened.error());
		/**
		 * Свежее открытие обязано ВОССТАНОВИТЬ счёт мусора, а не начать с нуля
		 */
		ASSERT_GT(opened.garbage(), 0ull) << "свежее открытие мусора не увидело";
		/**
		 * Восстановленный счёт обязан вобрать снесённую запись: строка её в оглавлении
		 * помечена снесённой, и длина её оттуда читается точно
		 */
		ASSERT_GE(opened.garbage(), second) << "снесённая запись при открытии не сосчитана";
		/**
		 * Восстановленный счёт не вправе ПРЕВЫШАТЬ сосчитанного правкой: он есть нижняя
		 * граница, и завышение гнало бы уборку за мусором, какого нет
		 */
		ASSERT_LE(opened.garbage(), committed) << "открытие насчитало мусора больше правки";
	}
}

/**
 * @brief Проверка того, что правка кладёт оглавление и подпись ЗА телом контейнера
 *
 * @details Раскладку эту заводят ТРИ разных пути записи: сборщик контейнера,
 *          закрепление правок и уборка мусора. Довод поля `index` гласит
 *          «оглавление лежит ЗА телом», и выборка записи на нём и стоит: длину
 *          кадра оглавления она поверяет полем `extent` заголовка, полагая, что
 *          кадр по смещению `index` и есть кадр оглавления. Сборщик закреплён
 *          проверкою `ContainerFixture.LayoutOffsetsFollowBody`, а здесь
 *          закрепляются два пути ПРАВКИ
 *
 * @note Умещение сличается неравенством, а не равенством: носитель правки лишь
 *       растёт, и за концом перезаписанного контейнера остаются октеты прежнего,
 *       более длинного. Уборка же кладёт контейнер на чистый носитель, и её длина
 *       выдаётся отдельно - там сличается равенство
 *
 */
TEST_F(EditorFixture, LayoutOffsetsFollowBody) {
	/**
	 * @brief Функция поверки раскладки уложенного контейнера
	 *
	 * @param data  октеты уложенного контейнера
	 * @param bound предел, каким ограничены октеты контейнера
	 * @param exact признак того, что контейнер кончается ровно пределом
	 *
	 */
	const auto examine = [](const vector <uint8_t> & data, const uint64_t bound,
	 const bool exact) noexcept -> void {
		// Заголовок опознания уложенного контейнера
		abc::header_t header;
		// Код отказа снятия заголовка опознания
		abc::error_t error = abc::error_t::NONE;
		// Выполняем снятие заголовка опознания уложенного контейнера
		ASSERT_TRUE(header.unpack(data.data(), data.size(), error))
			<< "код отказа: " << abc::message(error);
		// Выполняем проверку того, что оглавление лежит СРАЗУ за телом контейнера
		ASSERT_EQ(header.index, static_cast <uint64_t> (abc::HEADER_LENGTH) + header.length)
			<< "оглавление легло не за телом контейнера";
		// Выполняем выведение конца кадра оглавления контейнера
		const uint64_t tail = (header.index +
		 static_cast <uint64_t> (abc::CHUNK_HEADER) + static_cast <uint64_t> (header.extent));
		/**
		 * Если контейнер подписан, подпись обязана лечь СРАЗУ за кадром оглавления
		 */
		if(header.signature > 0){
			// Выполняем проверку того, что подпись лежит за кадром оглавления
			ASSERT_EQ(header.signature, tail) << "подпись легла не за кадром оглавления";
			// Выполняем проверку того, что запись подписи умещается в пределе
			ASSERT_LT(header.signature, bound) << "запись подписи вышла за предел контейнера";
		/**
		 * Если контейнер не подписан, кадром оглавления он и кончается
		 */
		} else if(exact) {
			/**
			 * Выполняем проверку того, что за кадром оглавления лежит ХВОСТОВОЙ заголовок
			 * опознания и более ничего.
			 *
			 * Хвостовой заголовок - разница правки со сборщиком: сборщик кончает контейнер
			 * кадром оглавления, а правка кладёт следом ещё один заголовок и лишь потом
			 * переписывает головной. Порядок этот и позволяет поднять контейнер, у какого
			 * обрыв застал головной заголовок недописанным
			 */
			ASSERT_EQ(tail + static_cast <uint64_t> (abc::HEADER_LENGTH), bound)
				<< "за оглавлением легло не хвостовым заголовком";
			// Заголовок опознания, снятый с ХВОСТА контейнера
			abc::header_t behind;
			// Код отказа снятия хвостового заголовка опознания
			abc::error_t spare = abc::error_t::NONE;
			// Выполняем снятие хвостового заголовка опознания контейнера
			ASSERT_TRUE(behind.unpack(data.data() + static_cast <size_t> (tail),
			 static_cast <size_t> (bound - tail), spare)) << "код отказа: " << abc::message(spare);
			// Выполняем проверку того, что хвостовой заголовок объявляет ту же раскладку
			ASSERT_EQ(behind.index, header.index) << "хвостовой заголовок объявил иное оглавление";
			// Выполняем проверку того, что хвостовой заголовок объявляет ту же длину тела
			ASSERT_EQ(behind.length, header.length) << "хвостовой заголовок объявил иную длину тела";
		}
		// Выполняем проверку того, что кадр оглавления умещается в пределе
		ASSERT_LE(tail, bound) << "кадр оглавления вышел за предел контейнера";
	};
	/**
	 * Выполняем поверку раскладки после ЗАКРЕПЛЕНИЯ правок
	 */
	{
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
		// Выполняем поверку раскладки закреплённого контейнера
		examine(medium.data, static_cast <uint64_t> (medium.data.size()), false);
	}
	/**
	 * Выполняем поверку раскладки после УБОРКИ мусора
	 */
	{
		// Носитель, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера с четырьмя записями
		this->build(medium, {"первая", "вторая", "третья", "четвёртая"});
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем снос третьей записи контейнера
		ASSERT_TRUE(editor.erase(2)) << "код отказа: " << abc::message(editor.error());
		// Выполняем фиксацию накопленных правок на носителе
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
		// Носитель, куда следует убрать контейнер
		Medium cleaned;
		// Полная длина убранного контейнера
		uint64_t length = 0;
		// Выполняем уборку мусора перестройкой контейнера
		ASSERT_TRUE(editor.compact([&cleaned](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
			// Выполняем запись поданных октетов убранного контейнера
			return cleaned.write(offset, buffer, size);
		}, abc::payload_t::TEXT, length)) << "код отказа: " << abc::message(editor.error());
		// Выполняем поверку раскладки убранного контейнера
		examine(cleaned.data, length, true);
	}
}

/**
 * @brief Проверка того, что снятие срока ИЗ РАБОТЫ ЗАПИСИ не валит правку
 *
 * @details Самочинная фиксация по сроку идёт СВОИМ потоком, а работа записи октетов
 *          отдана потребителем - и зовётся она тем самым потоком. Потребителю
 *          естественно снять самочинную фиксацию оттуда же: «легло на носитель - более
 *          не отбивай». Снятие это уходит в остановку отбоя срока, а остановка прежде
 *          дожидалась конца своего потока - того самого, из какого зван вызов. Ожидание
 *          себя самого стандарт запрещает, отказ его есть исключение, а остановка
 *          объявлена `noexcept`: работа снималась целиком
 *
 * @note Проверка эта - о ПУТИ ПОТРЕБИТЕЛЯ, а не о отбое срока: сам отбой закреплён
 *       проверками `CodecAbcSchedule.StopFromCallbackDoesNotAbort` и
 *       `RestartAfterStopFromCallbackSurvives`. Здесь же закрепляется, что путь этот
 *       достижим из работы, какую потребитель ПИШЕТ САМ, а не выдуман щупом
 *
 * @warning Утверждается не одно доживание до конца: фиксация обязана состояться, срок
 *          обязан быть снят, и вторая фиксация обязана не наступить. Утверждай проверка
 *          одно доживание, её прошло бы и снятие, ничего не снявшее
 *
 */
TEST_F(EditorFixture, DroppingDeadlineFromTheSinkSurvives) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Количество обращений к работе записи октетов
	atomic <size_t> writes(0);
	// Признак снятого срока самочинной фиксации
	atomic <bool> dropped(false);
	/**
	 * Выполняем открытие контейнера работами чтения и записи, где ЗАПИСЬ снимает срок
	 */
	ASSERT_TRUE(editor.open([&medium](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем чтение затребованных октетов контейнера
		return medium.read(offset, size, result);
	}, [&medium, &editor, &writes, &dropped](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
		// Выполняем учёт обращения к работе записи октетов
		writes++;
		/**
		 * Если срок самочинной фиксации ещё не снят, снимаем его ОТСЮДА
		 */
		if(!dropped.exchange(true)){
			// Получаем настройки правки контейнера
			abc::editor_t::settings_t settings = editor.settings();
			// Выполняем снятие способа самочинной фиксации
			settings.mode = abc::editor_t::mode_t::MANUAL;
			// Выполняем установку настроек правки контейнера
			editor.settings(settings);
		}
		// Выполняем запись поданных октетов контейнера
		return medium.write(offset, buffer, size);
	}, static_cast <uint64_t> (medium.data.size()))) << "код отказа: " << abc::message(editor.error());
	// Получаем настройки правки контейнера
	abc::editor_t::settings_t settings = editor.settings();
	// Выполняем установку способа фиксации своим потоком
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
	// Выполняем ожидание, многократно превышающее срок самочинной фиксации
	this_thread::sleep_for(chrono::milliseconds(250));
	// Выполняем проверку того, что работа записи звана, то есть фиксация состоялась
	ASSERT_GT(writes.load(), 0ul) << "самочинная фиксация не состоялась";
	// Выполняем проверку того, что срок снят работой записи
	ASSERT_TRUE(dropped.load()) << "работа записи срока не снимала";
	// Выполняем проверку того, что способ фиксации снят
	ASSERT_EQ(editor.settings().mode, abc::editor_t::mode_t::MANUAL);
	// Выполняем получение количества обращений к работе записи
	const size_t counted = writes.load();
	// Выполняем ожидание, многократно превышающее срок самочинной фиксации
	this_thread::sleep_for(chrono::milliseconds(150));
	// Выполняем проверку того, что снятый срок более не отбивается
	ASSERT_EQ(writes.load(), counted) << "снятый срок продолжил отбиваться";
}
/**
 * @brief Проверка того, что отвергнутое накопление не оставляет правку объявленной
 *
 * @details Отказ укладки кадра откатывает внесённую правку да накопленные октеты, но
 *          признак незакреплённых правок оставлял выставленным. Правка оттого числилась
 *          незакреплённой при ПУСТОМ списке правок и ПУСТОМ накоплении, и следующая
 *          фиксация ранним выходом «закреплять нечего» не уходила, а вела всю работу:
 *          метила прежнее оглавление мусором, писала новое, поднимала поколение
 *
 * @note Проверка ведёт ОБЕ половины договора. Первая - что накопление отвергнуто и
 *       состояние возвращено (накопленного ноль, записей столько же). Одной её мало:
 *       она была ЗЕЛЁНОЙ и при дефекте, ибо список правок и октеты откатывались верно,
 *       а утекал один лишь признак. Вторая - что фиксация после того НИЧЕГО не делает,
 *       и вот она дефект и красит
 *
 * @note Мерою взяты мусор, поколение и длина, а не выдача фиксации: фиксация отвечала
 *       УСПЕХОМ и при дефекте - работа-то шла удачно, просто была не нужна. Замер щупом
 *       03.09.2026: контейнер о 197 октетах рос до 349, мусор с нуля до 56, поколение с
 *       нуля до единицы, и круг повторялся сколько угодно
 *
 * @note Отказ укладки достигается настройкой укладчика `encrypt` при НЕОТДАННОМ модуле
 *       шифрования - это самый короткий путь к нему через открытый API. Причина отказа
 *       снимается перед фиксацией намеренно: не сними её, фиксация упала бы по той же
 *       причине, её собственный откат вернул бы всё, и цена утёкшего признака осталась
 *       бы невидимой. На этом первая сборка проверки и обманулась
 *
 */
TEST_F(EditorFixture, RefusedAddLeavesNothingToCommit) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Настройки правки контейнера
	abc::editor_t::settings_t settings = editor.settings();
	// Выполняем установку порога укладки в один октет: укладка пойдёт сразу же
	settings.block = 1;
	// Выполняем установку настроек правки контейнера
	editor.settings(settings);
	// Настройки укладчика кадров правщика
	abc::packer_t::settings_t packing = editor.packer().settings();
	// Выполняем требование шифровать содержимое кадров
	packing.encrypt = true;
	// Выполняем установку настроек укладчика, модуля шифрования НЕ отдавая
	editor.packer().settings(packing);
	// Состояние контейнера до отвергнутого накопления
	const uint64_t garbage = editor.garbage(), generation = editor.header().generation, length = editor.length();
	// Количество записей контейнера до отвергнутого накопления
	const uint64_t records = editor.records();
	// Дописываемая запись
	const vector <uint8_t> item = abc::value_t(string("вторая")).dump();
	// Выполняем проверку того, что дописывание записи отвергнуто
	ASSERT_FALSE(editor.append(item.data(), item.size(), abc::payload_t::TEXT));
	// Выполняем проверку кода отказа дописывания записи
	ASSERT_EQ(editor.error(), abc::error_t::ENCRYPTION_FAILED);
	/**
	 * Выполняем проверку того, что состояние правки возвращено
	 *
	 * @note Половина эта была зелёной и при дефекте: откат списка правок и накопленных
	 *       октетов работал верно, утекал один лишь признак незакреплённых правок
	 */
	ASSERT_EQ(editor.pending(), static_cast <size_t> (0));
	// Выполняем проверку того, что количество записей контейнера не изменилось
	ASSERT_EQ(editor.records(), records);
	/**
	 * Выполняем снятие требования шифровать: причина отказа устранена
	 *
	 * @note Без этого фиксация упала бы по той же причине, её собственный откат вернул бы
	 *       всё, и цена утёкшего признака осталась бы невидимой
	 */
	packing.encrypt = false;
	// Выполняем установку настроек укладчика кадров
	editor.packer().settings(packing);
	/**
	 * Выполняем фиксацию, какой закреплять нечего
	 *
	 * @note Успехом она отвечала и при дефекте: работа шла удачно, просто была не нужна
	 */
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Мусора на носителе прибавиться не должно: фиксации нечего было закреплять
	ASSERT_EQ(editor.garbage(), garbage);
	// Поколение записи контейнера подняться не должно
	ASSERT_EQ(editor.header().generation, generation);
	// Длина контейнера вырасти не должна
	ASSERT_EQ(editor.length(), length);
}
/**
 * @brief Проверка того, что открытие не требует у источника непомерного по подделанной длине
 *
 * @details Длина содержимого кадра оглавления объявлена ДВАЖДЫ: заголовком опознания и
 *          самим кадром. Открытие сличало их между собою и тем полагало длину поверенной -
 *          но заголовок несёт контрольную сумму, а сумму эту подделыватель ПЕРЕСЧИТЫВАЕТ,
 *          и обе длины приходят какими он положит. По ним же вычитывается кадр целиком
 *
 * @note Сторож этот стоял у снятия кадра, у сбора свёрток, у выборки записей и у обхода
 *       счёта мусора - последний десятью строками ниже, в той же самой работе, - а у
 *       самого открытия его не было ни у правки, ни у выборки. Открытие же первым и
 *       встречает недоверенный контейнер
 *
 * @note Мерою взят НАИБОЛЬШИЙ ЗАПРОС К ИСТОЧНИКУ, а не выдача открытия: открытие
 *       отвечало отказом и при дефекте - источник проверки пределы поверяет и отдаёт
 *       меньше затребованного. Вся цена дефекта в том, ЧТО БЫЛО ЗАТРЕБОВАНО прежде
 *       отказа: источник даётся потребителем, и наивный завёл бы 4 ГиБ. Замер щупом
 *       03.09.2026 на контейнере в 197 октетов: 4 294 967 327 октетов у обоих
 *
 * @note Полная длина контейнера подаётся выборке ВТОРЫМ доводом `open`: без неё сторожу
 *       не на что опереться, и он снимается. Ради того у хранилища и заведён близнец
 *       связывания - `bind(fetcher_t &)`, подающий длину сам
 *
 */
TEST_F(EditorFixture, ForgedIndexLengthAsksNothingHuge) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Октеты собранного контейнера
	vector <uint8_t> body = medium.data;
	// Заголовок опознания собранного контейнера
	abc::header_t header;
	// Код отказа снятия заголовка опознания
	abc::error_t error = abc::error_t::NONE;
	// Выполняем снятие заголовка опознания собранного контейнера
	ASSERT_TRUE(header.unpack(body.data(), body.size(), error)) << "код отказа: " << abc::message(error);
	// Выполняем получение смещения кадра оглавления контейнера
	const uint64_t place = header.index;
	/**
	 * Выполняем подделку длины содержимого кадра оглавления в заголовке опознания
	 *
	 * @note Заголовок укладывается ЗАНОВО, оттого контрольная сумма его пересчитывается
	 *       сама - ровно так поступил бы и подделыватель
	 */
	header.extent = 0xFFFFFFFFu;
	// Октеты подделанного заголовка опознания
	vector <uint8_t> forged;
	// Выполняем укладку подделанного заголовка опознания
	header.pack(forged);
	// Выполняем подмену головного заголовка опознания подделанным
	::memcpy(body.data(), forged.data(), abc::HEADER_LENGTH);
	// Выполняем подделку той же длины в самом кадре оглавления: сличение обязано сойтись
	abc::fixed(body.data() + place + 4, 0xFFFFFFFFull, 4);
	// Наибольший размер, затребованный у источника октетов
	size_t biggest = 0;
	/**
	 * @brief Работа чтения октетов, следящая за наибольшим запросом
	 *
	 */
	auto source = [&body, &biggest](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Если запрос превышает наибольший из виденных, запоминаем его
		if(size > biggest)
			// Выполняем запоминание наибольшего запроса к источнику
			biggest = size;
		// Выполняем очистку буфера прочитанных октетов
		result.clear();
		// Если затребованное чтение выходит за пределы контейнера
		if((offset > body.size()) || (size > (body.size() - offset)))
			// Выводим признак неудачного чтения
			return false;
		// Выполняем выдачу затребованных октетов контейнера
		result.assign(body.begin() + static_cast <ptrdiff_t> (offset),
		 body.begin() + static_cast <ptrdiff_t> (offset + size));
		// Выводим признак успешного чтения
		return true;
	};
	/**
	 * Выполняем поверку выборки записей
	 */
	{
		// Выполняем сброс наибольшего запроса к источнику
		biggest = 0;
		// Выборщик записей подделанного контейнера
		abc::fetcher_t fetcher;
		// Выполняем проверку того, что открытие подделанного контейнера отвергнуто
		ASSERT_FALSE(fetcher.open(source, static_cast <uint64_t> (body.size())));
		// Выполняем проверку кода отказа открытия подделанного контейнера
		ASSERT_EQ(fetcher.error(), abc::error_t::INVALID_CHUNK);
		// У источника не должно быть затребовано больше самого контейнера
		ASSERT_LE(biggest, body.size()) << "затребовано октетов: " << biggest;
	}
	/**
	 * Выполняем поверку правки контейнера
	 */
	{
		// Выполняем сброс наибольшего запроса к источнику
		biggest = 0;
		// Правщик подделанного контейнера
		abc::editor_t editor;
		// Выполняем проверку того, что открытие подделанного контейнера отвергнуто
		ASSERT_FALSE(editor.open(source, [](const uint64_t, const void *, const size_t) noexcept -> bool {
			// Выполняем объявление записи успешной: писать проверке нечего
			return true;
		}, static_cast <uint64_t> (body.size())));
		// Выполняем проверку кода отказа открытия подделанного контейнера
		ASSERT_EQ(editor.error(), abc::error_t::INVALID_CHUNK);
		// У источника не должно быть затребовано больше самого контейнера
		ASSERT_LE(biggest, body.size()) << "затребовано октетов: " << biggest;
	}
}

/**
 * @brief Проверка отказов накопления записи, у каких причина своя
 *
 * @details Заслонов у накопления два, и оба отвечают СВОЕЙ причиной: правка неоткрытого
 *          контейнера есть внутренний отказ, а запись пустая - не запись вовсе, и строка
 *          оглавления о нуле октетов указывала бы в никуда
 *
 * @note Заведена находкой 05.09.2026, добытой сплошным щупом по местам отказа: обе
 *       причины стояли ненаблюдаемыми. Дверь та же, что у сборщика контейнера
 *       (`ContainerFixture.EmptyRecordRefusedByTheAssembler`), - и стеречь надлежит обе
 *
 */
TEST_F(EditorFixture, AddNamesItsRefusals) {
	// Собираемая запись, годная сама по себе
	const vector <uint8_t> item = abc::value_t(string{"годная"}).dump();
	/**
	 * Половина первая: правка контейнера, ещё не открытого
	 */
	{
		// Правщик контейнера, контейнера не открывавший
		abc::editor_t editor;
		// Выполняем проверку отказа накопления записи у неоткрытого контейнера
		ASSERT_FALSE(editor.append(item.data(), item.size()));
		// Выполняем проверку названной причины отказа
		ASSERT_EQ(editor.error(), abc::error_t::INTERNAL) << abc::message(editor.error());
		// Вычитываемые октеты записи контейнера
		vector <uint8_t> picked;
		/**
		 * Выборка записи у контейнера неоткрытого - вторая дверь того же заслона
		 */
		ASSERT_FALSE(editor.record(0, picked));
		// Выполняем проверку названной причины отказа
		ASSERT_EQ(editor.error(), abc::error_t::INTERNAL) << abc::message(editor.error());
		/**
		 * Фиксация правок у контейнера неоткрытого - третья дверь того же заслона
		 */
		ASSERT_FALSE(editor.commit());
		// Выполняем проверку названной причины отказа
		ASSERT_EQ(editor.error(), abc::error_t::INTERNAL) << abc::message(editor.error());
	}
	/**
	 * Половина вторая: пустая запись у контейнера ОТКРЫТОГО
	 */
	{
		// Носитель, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера с одной записью
		this->build(medium, {"первая"});
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << abc::message(editor.error());
		// Вычитываемые октеты записи открытого контейнера
		vector <uint8_t> picked;
		// Выполняем проверку отказа накопления записи, поданной пустым указателем
		ASSERT_FALSE(editor.append(nullptr, item.size()));
		// Выполняем проверку названной причины отказа
		ASSERT_EQ(editor.error(), abc::error_t::EMPTY_RECORD) << abc::message(editor.error());
		// Выполняем проверку отказа накопления записи нулевой длины
		ASSERT_FALSE(editor.append(item.data(), 0));
		// Выполняем проверку названной причины отказа
		ASSERT_EQ(editor.error(), abc::error_t::EMPTY_RECORD) << abc::message(editor.error());
		/**
		 * Накопление записи годной при том работает: без этой половины проверка прошла бы
		 * и у правщика, не принимающего ничего вовсе
		 */
		ASSERT_TRUE(editor.append(item.data(), item.size())) << abc::message(editor.error());
		// Выполняем проверку того, что отказа по накоплении годной записи не объявлено
		ASSERT_EQ(editor.error(), abc::error_t::NONE) << abc::message(editor.error());
		/**
		 * Выборка записи, номера какой в оглавлении нет вовсе
		 *
		 * @note Причина здесь ИНАЯ, чем у неоткрытого контейнера: там правка невозможна
		 *       вовсе, здесь же контейнер открыт, а спрошено у него несуществующее
		 */
		ASSERT_FALSE(editor.record(999, picked));
		// Выполняем проверку названной причины отказа
		ASSERT_EQ(editor.error(), abc::error_t::INVALID_INDEX) << abc::message(editor.error());
	}
}

/**
 * @brief Проверка того, что всякий отказ чтения носителя назван причиною чтения
 *
 * @details Обход обрывает чтение на РАЗНЫХ шагах: сперва на первом, потом на втором и
 *          так далее, - оттого одного места назвать нельзя. Зато можно потребовать
 *          того, что договору и принадлежит: причина объявлена непременно, принадлежит
 *          источнику либо порче кадра, и работа отвечает отказом, а не мнимым успехом
 *
 * @note Заведена находкой 05.09.2026, добытой сплошным щупом по местам отказа: мест
 *       `UNREADABLE_SOURCE` у правщика двадцать одно, и подмена причины НИ У ОДНОГО из
 *       них не красила ничего. Проверки обхода по `sight` при том стояли - но спрашивали
 *       исход да сохранность, а не то, о чём работа отчитывается потребителю
 *
 * @note Обход требует, чтобы отказы ЗАСТАЛИСЬ: молчание всего обхода означало бы, что
 *       предел чтений не работает вовсе, и проверка прошла бы вслепую
 *
 */
TEST_F(EditorFixture, EveryReadRefusalNamesItsCause) {
	// Носитель контейнера в памяти
	Medium medium;
	// Выполняем сборку контейнера о шести записях
	this->build(medium, {"первая", "вторая", "третья", "четвёртая", "пятая", "шестая"});
	// Выполняем установку соли шифрования
	this->_crypto->salt("соль контейнера");
	// Выполняем установку пароля шифрования
	this->_crypto->password("пароль владельца");
	// Выполняем выработку ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Количество отказов, застигнутых обходом
	size_t refused = 0;
	/**
	 * Выполняем обход всех мест обрыва чтения носителя
	 */
	for(int sight = 0; sight < 24; sight++){
		// Носитель круга, несущий снимок собранного контейнера
		Medium probe;
		// Выполняем снятие снимка собранного контейнера на носитель круга
		probe.data = medium.data;
		// Правщик контейнера круга
		abc::editor_t editor;
		/**
		 * Открытие контейнера при слепом носителе - тоже отказ, и причину он обязан назвать
		 */
		probe.sight = sight;
		// Признак успешно открытого контейнера
		const bool opened = this->open(editor, probe);
		/**
		 * Объявление подписи собирает свёртки по кадрам тела, а сбор этот ЧИТАЕТ носитель
		 * на своих местах, отличных от мест выборки записи: без него обход слеп к целой
		 * дороге отказов
		 */
		const bool signed_ = (opened && editor.sign(this->_crypto.get(), "владелец"));
		// Вычитываемые октеты записи контейнера
		vector <uint8_t> picked;
		/**
		 * Если контейнер открылся, выполняем выборку записей его
		 */
		bool taken = (signed_ && editor.record(0, picked) && editor.record(5, picked));
		/**
		 * Фиксация правок читает носитель тоже - и обрыв чтения настигает её на СВОИХ
		 * местах, отличных от мест выборки записи
		 */
		if(taken){
			// Собираемая дописываемая запись контейнера
			const vector <uint8_t> item = abc::value_t(string{"седьмая"}).dump();
			// Выполняем дописывание записи в конец контейнера с последующей фиксацией
			taken = (editor.append(item.data(), item.size(), abc::payload_t::TEXT) && editor.commit());
		}
		// Выполняем снятие предела удавшихся чтений носителя
		probe.sight = -1;
		/**
		 * Если работа отвечена отказом, причина обязана быть названной
		 */
		if(!taken){
			// Выполняем учёт застигнутого отказа
			refused++;
			// Выполняем проверку того, что отказ причину объявил
			ASSERT_NE(editor.error(), abc::error_t::NONE) << "предел чтений: " << sight;
			// Выполняем проверку того, что причина принадлежит чтению либо порче кадра
			ASSERT_TRUE((editor.error() == abc::error_t::UNREADABLE_SOURCE) ||
			 (editor.error() == abc::error_t::INVALID_CHUNK) ||
			 (editor.error() == abc::error_t::MISSING_RECORD) ||
			 (editor.error() == abc::error_t::MISSING_INDEX) ||
			 (editor.error() == abc::error_t::INVALID_INDEX) ||
			 (editor.error() == abc::error_t::UNWRITABLE_SINK) ||
			 (editor.error() == abc::error_t::SIGNING_FAILED))
			 << "предел чтений: " << sight << ", причина: " << abc::message(editor.error());
		}
	}
	// Выполняем проверку того, что обход застал отказы чтения
	ASSERT_GT(refused, 0u);
}

/**
 * @brief Проверка отказов открытия контейнера правщиком и объявления подписи
 *
 * @details Заслонов здесь три, и причины у них РАЗНЫЕ: работы чтения либо записи, не
 *          отданные вовсе, суть отказ внутренний; контейнер, оглавления не несущий,
 *          правке не подлежит - править нечего, кроме тела; а объявление подписи у
 *          правщика, контейнера не открывшего, есть тот же внутренний отказ
 *
 * @note Заведена находкой 05.09.2026, добытой сплошным щупом по местам отказа: все три
 *       причины стояли ненаблюдаемыми. Отказ по оглавлению стережёт и `IndexFixture`, но
 *       у ВЫБОРЩИКА, - дверь же правщика своя, и она молчала
 *
 */
TEST_F(EditorFixture, OpeningNamesItsRefusals) {
	/**
	 * Половина первая: работы чтения и записи, не отданные вовсе
	 */
	{
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем проверку отказа открытия контейнера без работ чтения и записи
		ASSERT_FALSE(editor.open(nullptr, nullptr, 0));
		// Выполняем проверку названной причины отказа
		ASSERT_EQ(editor.error(), abc::error_t::INTERNAL) << abc::message(editor.error());
	}
	/**
	 * Половина вторая: объявление подписи у правщика, контейнера не открывшего
	 */
	{
		// Правщик контейнера, контейнера не открывавший
		abc::editor_t editor;
		// Выполняем установку соли шифрования
		this->_crypto->salt("соль контейнера");
		// Выполняем установку пароля шифрования
		this->_crypto->password("пароль владельца");
		// Выполняем выработку ключа владельца контейнера
		ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
		// Выполняем проверку отказа объявления подписи у неоткрытого контейнера
		ASSERT_FALSE(editor.sign(this->_crypto.get(), "владелец"));
		// Выполняем проверку названной причины отказа
		ASSERT_EQ(editor.error(), abc::error_t::INTERNAL) << abc::message(editor.error());
	}
	/**
	 * Половина третья: контейнер, оглавления не несущий
	 */
	{
		// Сборщик контейнера
		abc::assembler_t assembler;
		// Получаем настройки сборки контейнера
		abc::assembler_t::settings_t settings = assembler.settings();
		// Выполняем отключение ведения оглавления собираемого контейнера
		settings.indexed = false;
		// Выполняем установку настроек сборки контейнера
		assembler.settings(settings);
		// Выполняем сборку записи контейнера
		const vector <uint8_t> item = abc::value_t(string{"запись без оглавления"}).dump();
		// Выполняем внесение записи в собираемый контейнер
		ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
		 << abc::message(assembler.error());
		// Носитель, несущий контейнер без оглавления
		Medium medium;
		// Выполняем завершение сборки контейнера
		ASSERT_TRUE(assembler.complete(medium.data)) << abc::message(assembler.error());
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем проверку отказа открытия контейнера без оглавления
		ASSERT_FALSE(this->open(editor, medium));
		// Выполняем проверку названной причины отказа
		ASSERT_EQ(editor.error(), abc::error_t::MISSING_INDEX) << abc::message(editor.error());
	}
}

/**
 * @brief Проверка того, что причина отказа переносится через границу слоёв
 *
 * @details Кадр снимает УКЛАДЧИК, а отвечает потребителю ПРАВЩИК: причина обязана дойти
 *          через границу целой, а не выродиться во внутренний отказ. Порча суммы кадра
 *          опознаётся укладчиком, и правщик обязан назвать ту же причину, что назвал бы
 *          сам укладчик
 *
 * @note Заведена по приёму, отданному владельцем разметки 05.09.2026: щуп подменою причин
 *       слеп к дефекту, где причина ставится верно, а гибнет ПОСЛЕ постановки - по дороге
 *       наружу. Подменять там нечего, ибо подменённая причина гибнет ровно так же.
 *       Стережётся такое сличением причины у ВНЕШНЕГО слоя с причиной у внутреннего
 *
 */
TEST_F(EditorFixture, TheCauseCrossesTheBoundaryOfTheLayers) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с двумя записями
	this->build(medium, {"первая", "вторая"});
	// Выполняем проверку того, что контейнер собран
	ASSERT_GT(medium.data.size(), abc::HEADER_LENGTH + abc::CHUNK_HEADER);
	/**
	 * Выполняем порчу содержимого первого кадра тела контейнера
	 *
	 * @note Портится СОДЕРЖИМОЕ, а не заголовок кадра: длина и вид остаются годными, и
	 *       отказ приходит именно по контрольной сумме, а не по разбору полей
	 */
	medium.data.at(abc::HEADER_LENGTH + abc::CHUNK_HEADER) ^= 0xFF;
	// Правщик контейнера
	abc::editor_t editor;
	// Признак успешно открытого контейнера с испорченным кадром
	const bool opened = this->open(editor, medium);
	// Вычитываемые октеты записи контейнера
	vector <uint8_t> picked;
	// Выполняем проверку того, что порча кадра наружу выходит отказом
	ASSERT_FALSE(opened && editor.record(0, picked));
	// Выполняем проверку того, что отказ причину объявил
	ASSERT_NE(editor.error(), abc::error_t::NONE);
	/**
	 * Выполняем проверку того, что названа причина УКЛАДЧИКА, а не внутренний отказ:
	 * внутренний отказ здесь означал бы, что причина по дороге наружу погибла
	 */
	ASSERT_NE(editor.error(), abc::error_t::INTERNAL) << abc::message(editor.error());
	// Выполняем проверку того, что названа именно порча кадра либо суммы его
	ASSERT_TRUE((editor.error() == abc::error_t::INVALID_CHECKSUM) ||
	 (editor.error() == abc::error_t::INVALID_CHUNK))
	 << "причина: " << abc::message(editor.error());
}

/**
 * @brief Проверка того, что строка оглавления, за тело указывающая, отвергается
 *
 * @details Строка оглавления несёт смещение кадра в теле контейнера. Смещение, за длину
 *          тела уходящее, есть порча: снятие по нему читало бы за концом тела, а то
 *          отвечает усечением, и толковать усечённое как кадр значило бы верить порче
 *
 * @note Заведена находкой 05.09.2026, добытой сплошным щупом по местам отказа: заслон
 *       стоял, а спрашивать у него причину было некому. Подделка ведётся тем же приёмом,
 *       что у `EditorFixture.ForgedIndexLengthAsksNothingHuge`, - правкой заголовка
 *       опознания, - но портится ДЛИНА ТЕЛА, а не размах оглавления: тело объявляется
 *       короче, и всякий кадр за новой границей оказывается за телом
 *
 */
TEST_F(EditorFixture, ChunkBeyondTheBodyIsRefused) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с тремя записями
	this->build(medium, {"первая", "вторая", "третья"});
	// Правщик контейнера, открывающий нетронутый контейнер
	{
		// Правщик контейнера
		abc::editor_t editor;
		// Вычитываемые октеты записи контейнера
		vector <uint8_t> picked;
		// Выполняем проверку того, что нетронутый контейнер открывается и читается
		ASSERT_TRUE(this->open(editor, medium)) << abc::message(editor.error());
		// Выполняем проверку выборки последней записи нетронутого контейнера
		ASSERT_TRUE(editor.record(2, picked)) << abc::message(editor.error());
	}
	// Заголовок опознания собранного контейнера
	abc::header_t header;
	// Код отказа снятия заголовка опознания
	abc::error_t error = abc::error_t::NONE;
	// Выполняем снятие заголовка опознания собранного контейнера
	ASSERT_TRUE(header.unpack(medium.data.data(), medium.data.size(), error))
	 << abc::message(error);
	// Выполняем проверку того, что тело контейнера объявлено непустым
	ASSERT_GT(header.length, 0ull);
	/**
	 * Выполняем объявление тела ВДВОЕ короче настоящего: кадры второй половины его
	 * оказываются за телом, а строки оглавления по-прежнему на них указывают
	 */
	header.length /= 2;
	// Октеты подделанного заголовка опознания
	vector <uint8_t> forged;
	// Выполняем укладку подделанного заголовка опознания
	header.pack(forged);
	// Выполняем подмену головного заголовка опознания подделанным
	::memcpy(medium.data.data(), forged.data(), abc::HEADER_LENGTH);
	// Правщик контейнера с подделанной длиною тела
	abc::editor_t editor;
	// Признак успешно открытого контейнера
	const bool opened = this->open(editor, medium);
	// Вычитываемые октеты записи контейнера
	vector <uint8_t> picked;
	// Выполняем проверку того, что порча наружу выходит отказом, а не усечённой записью
	ASSERT_FALSE(opened && editor.record(2, picked));
	// Выполняем проверку того, что отказ причину объявил
	ASSERT_NE(editor.error(), abc::error_t::NONE);
	/**
	 * Выполняем проверку того, что названа порча раскладки, а не внутренний отказ:
	 * внутренний отказ увёл бы потребителя искать дефект у себя
	 */
	ASSERT_NE(editor.error(), abc::error_t::INTERNAL) << abc::message(editor.error());
}
/**
 * @brief Проверка того, что отказ снятия накопленного кадра назван причиною укладчика
 *
 * @details Накопленное правщик держит УЛОЖЕННЫМИ кадрами и снимает их обратно при всякой
 *          выборке. Оснастку же - сжатие и шифрование - отдаёт зовущий, и отдать её он
 *          волен когда угодно, в том числе ПОСЛЕ укладки. Кадр, уложенный сжимающим
 *          правщиком, снимается тогда правщиком, сжатия уже не знающим, и снятие
 *          отвечается отказом
 *
 * @note Место это щупы числили слепым: набор оснастку по ходу работы не менял вовсе.
 *       Отказ здесь не заводит своей причины, а ПЕРЕНОСИТ причину укладчика - тем и
 *       закрепляется переход причины через границу слоёв (05.09.2026)
 */
TEST_F(EditorFixture, PendingUnpackFailureCarriesThePackerCause) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем установку модуля сжатия правщику
	editor.compressor(this->_compressor.get());
	// Получаем настройки правки контейнера
	abc::editor_t::settings_t settings = editor.settings();
	/**
	 * Выполняем установку порога накопления, укладывающего кадром всякую запись:
	 * накопленное, кадром ещё не уложенное, читается напрямую и снятия не требует вовсе
	 */
	settings.block = 1;
	// Выполняем установку настроек правки контейнера
	editor.settings(settings);
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	/**
	 * Собираем хорошо сжимаемую запись: без сжатия кадр ляжет несжатым, и снятие его
	 * без разжимателя пройдёт, а проверка прошла бы вхолостую
	 */
	const vector <uint8_t> item = abc::value_t(string(4096, 'a')).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	/**
	 * Выполняем проверку того, что накопленная запись оснащённым правщиком выбирается:
	 * иначе отказ ниже принадлежал бы не снятию оснастки, а самой укладке
	 */
	ASSERT_TRUE(editor.record(1, picked)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, item);
	// Выполняем снятие модуля сжатия у правщика
	editor.compressor(nullptr);
	// Выполняем проверку того, что выборка накопленной записи отвечена отказом
	ASSERT_FALSE(editor.record(1, picked));
	// Выполняем проверку того, что причина укладчика перенесена правщику
	ASSERT_EQ(editor.error(), abc::error_t::COMPRESSION_FAILED);
	/**
	 * Выполняем проверку того, что возвращённая оснастка работу возвращает: отказ
	 * принадлежал оснастке, а не порче накопленного
	 */
	editor.compressor(this->_compressor.get());
	// Выполняем проверку того, что накопленная запись выбирается снова
	ASSERT_TRUE(editor.record(1, picked)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, item);
}
/**
 * @brief Проверка того, что отказ снятия оглавления назван причиною укладчика
 *
 * @details Правщик открывает контейнер снятием кадра оглавления, а разжимателя ему, как
 *          и всякому слою, отдаёт зовущий. Сжатый контейнер, поданный правщику без
 *          оснастки, отвечается отказом уже при ОТКРЫТИИ, и причина переносится снизу
 *
 * @note Устье-близнец тому, что у выборщика записей, и закрывается тем же доводом.
 *       Числилось незакреплённым до 05.09.2026
 */
TEST_F(EditorFixture, OpeningCompressedWithoutTheCompressorCarriesThePackerCause) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	{
		// Сборщик контейнера
		abc::assembler_t assembler;
		// Выполняем установку модуля сжатия сборщику
		assembler.compressor(this->_compressor.get());
		/**
		 * Выполняем внесение череды записей: оглавление о четырёх записях сжатие берёт,
		 * и снятие его без разжимателя отвечается отказом
		 */
		for(size_t i = 0; i < 4; i++){
			// Выполняем сборку очередной записи
			const vector <uint8_t> item = abc::value_t(string(4096, 'a')).dump();
			// Выполняем внесение очередной записи в собираемый контейнер
			ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
				<< "код отказа: " << abc::message(assembler.error());
		}
		// Выполняем завершение сборки контейнера
		ASSERT_TRUE(assembler.complete(medium.data)) << "код отказа: " << abc::message(assembler.error());
	}
	{
		// Правщик контейнера, сжатием НЕ оснащённый
		abc::editor_t editor;
		// Выполняем проверку того, что открытие сжатого контейнера отвечено отказом
		ASSERT_FALSE(this->open(editor, medium));
		// Выполняем проверку того, что причина укладчика перенесена правщику
		ASSERT_EQ(editor.error(), abc::error_t::COMPRESSION_FAILED);
	}
	{
		// Правщик контейнера, сжатием оснащённый
		abc::editor_t editor;
		// Выполняем установку модуля сжатия правщику
		editor.compressor(this->_compressor.get());
		/**
		 * Выполняем проверку того, что отказ принадлежал ОСНАСТКЕ: тот же контейнер
		 * правщиком оснащённым открывается и записи его выбираются
		 */
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Буфер выбранной записи контейнера
		vector <uint8_t> picked;
		// Выполняем проверку того, что запись контейнера выбирается
		ASSERT_TRUE(editor.record(2, picked)) << "код отказа: " << abc::message(editor.error());
	}
}
/**
 * @brief Проверка того, что отказы укладки правщика названы причиною укладчика
 *
 * @details Правщик укладывает кадром трижды: накопленное при фиксации, оглавление при
 *          фиксации и то же при уборке мусора. Всякая укладка подчинена настройкам
 *          укладчика, а шифрование, заказанное без модуля шифрования, отвечается отказом.
 *          Причина же переносится правщику снизу: слою правки знать нечего о шифровании
 *
 * @note Заказ ставится ПОСЛЕ накопления: закажи его раньше - отказом ответится укладка
 *       накопленного, и до оглавления дело не дойдёт. Тем и разводятся устья, до
 *       05.09.2026 незакреплённые
 */
TEST_F(EditorFixture, PackFailureCarriesThePackerCause) {
	// Собираемая дописываемая запись контейнера
	const vector <uint8_t> item = abc::value_t(string{"дописанная"}).dump();
	/**
	 * Устье первое: укладка при ФИКСАЦИИ накопленных правок
	 */
	{
		// Носитель, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера о трёх записях
		this->build(medium, {"первая", "вторая", "третья"});
		// Правщик контейнера
		abc::editor_t editor;
		/**
		 * Выполняем установку порога накопления, укладывающего кадром всякую запись:
		 * без того отказом ответится укладка НАКОПЛЕННОГО, и до оглавления дело не дойдёт
		 */
		abc::editor_t::settings_t rules = editor.settings();
		// Выполняем установку порога накопления записей
		rules.block = 1;
		// Выполняем установку настроек правки контейнера
		editor.settings(rules);
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем дописывание записи в конец контейнера
		ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(editor.error());
		// Получаем настройки укладчика кадров
		abc::packer_t::settings_t settings = editor.packer().settings();
		// Выполняем заказ шифрования без модуля шифрования
		settings.encrypt = true;
		// Выполняем установку настроек укладчика кадров
		editor.packer().settings(settings);
		// Выполняем проверку того, что фиксация правок отвечена отказом
		ASSERT_FALSE(editor.commit());
		// Выполняем проверку того, что причина укладчика перенесена правщику
		ASSERT_EQ(editor.error(), abc::error_t::ENCRYPTION_FAILED);
	}
	/**
	 * Устье второе: укладка при УБОРКЕ мусора
	 */
	{
		// Носитель, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера о трёх записях
		this->build(medium, {"первая", "вторая", "третья"});
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем снос записи контейнера, дающий мусор
		ASSERT_TRUE(editor.erase(1)) << "код отказа: " << abc::message(editor.error());
		// Выполняем фиксацию накопленных правок на носителе
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
		// Выполняем проверку того, что мусор правкой накопился
		ASSERT_GT(editor.garbage(), 0ull);
		// Получаем настройки укладчика кадров
		abc::packer_t::settings_t settings = editor.packer().settings();
		// Выполняем заказ шифрования без модуля шифрования
		settings.encrypt = true;
		// Выполняем установку настроек укладчика кадров
		editor.packer().settings(settings);
		// Носитель, куда следует убрать контейнер
		Medium cleaned;
		// Полная длина убранного контейнера
		uint64_t length = 0;
		// Выполняем проверку того, что уборка мусора отвечена отказом
		ASSERT_FALSE(editor.compact([&cleaned](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
			// Выполняем запись поданных октетов убранного контейнера
			return cleaned.write(offset, buffer, size);
		}, abc::payload_t::TEXT, length));
		// Выполняем проверку того, что причина укладчика перенесена правщику
		ASSERT_EQ(editor.error(), abc::error_t::ENCRYPTION_FAILED);
	}
	/**
	 * Устье третье: укладка ОГЛАВЛЕНИЯ при уборке мусора
	 *
	 * @note Уборка перекладывает всякую уцелевшую запись, и укладка их идёт ПРЕЖДЕ укладки
	 *       оглавления: опередить её можно, лишь снеся все записи до единой. Тогда
	 *       перекладывать нечего, и первой укладкой уборки выходит оглавление
	 */
	{
		// Носитель, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера о трёх записях
		this->build(medium, {"первая", "вторая", "третья"});
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		/**
		 * Выполняем снос всех записей контейнера
		 */
		for(uint64_t number = 0; number < 3; number++)
			// Выполняем снос очередной записи контейнера
			ASSERT_TRUE(editor.erase(number)) << "код отказа: " << abc::message(editor.error());
		// Выполняем фиксацию накопленных правок на носителе
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
		// Получаем настройки укладчика кадров
		abc::packer_t::settings_t settings = editor.packer().settings();
		// Выполняем заказ шифрования без модуля шифрования
		settings.encrypt = true;
		// Выполняем установку настроек укладчика кадров
		editor.packer().settings(settings);
		// Носитель, куда следует убрать контейнер
		Medium cleaned;
		// Полная длина убранного контейнера
		uint64_t length = 0;
		// Выполняем проверку того, что уборка пустого контейнера отвечена отказом
		ASSERT_FALSE(editor.compact([&cleaned](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
			// Выполняем запись поданных октетов убранного контейнера
			return cleaned.write(offset, buffer, size);
		}, abc::payload_t::TEXT, length));
		// Выполняем проверку того, что причина укладчика перенесена правщику
		ASSERT_EQ(editor.error(), abc::error_t::ENCRYPTION_FAILED);
	}
	/**
	 * Выполняем проверку того, что отказы принадлежали НЕДОСТАЮЩЕЙ ОСНАСТКЕ: с отданным
	 * модулем шифрования те же работы проходят
	 */
	{
		// Выполняем установку соли шифрования
		this->_crypto->salt("соль контейнера");
		// Выполняем установку пароля шифрования
		this->_crypto->password("пароль владельца");
		// Носитель, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера о трёх записях
		this->build(medium, {"первая", "вторая", "третья"});
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем установку модуля шифрования правщику
		editor.crypto(this->_crypto.get());
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем дописывание записи в конец контейнера
		ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(editor.error());
		// Получаем настройки укладчика кадров
		abc::packer_t::settings_t settings = editor.packer().settings();
		// Выполняем заказ шифрования при отданном модуле шифрования
		settings.encrypt = true;
		// Выполняем установку настроек укладчика кадров
		editor.packer().settings(settings);
		// Выполняем проверку того, что фиксация правок проходит
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	}
}
/**
 * @brief Проверка того, что обрыв носителя назван причиною своего рода
 *
 * @details Прежние обходы обрыва носителя утверждали, что причина НАЗВАНА, а не КАКАЯ:
 *          `ASSERT_NE(error, NONE)` проходит при всякой причине, и договор «обрыв чтения
 *          есть `UNREADABLE_SOURCE`, обрыв записи есть `UNWRITABLE_SINK`» держался ничем.
 *          Девятнадцать устий отказа носителя стояли незакреплёнными по роду причины
 *
 * @note Обход ведётся по ВСЕМ работам, носитель трогающим: открытие, выборка записи,
 *       объявление подписи, фиксация правок и уборка мусора, - у всякой свои места
 *       обращения к носителю, и предел, до одной не дошедший, настигает другую
 *
 * @note Проверяется и ОБРАТНОЕ: при снятом пределе всякая работа проходит. Без того
 *       обход прошёл бы и на работе, отказывающей всегда
 */
TEST_F(EditorFixture, MediumRefusalNamesItsKind) {
	// Выполняем установку соли шифрования
	this->_crypto->salt("соль контейнера");
	// Выполняем установку пароля шифрования
	this->_crypto->password("пароль владельца");
	/**
	 * Выполняем заведение ключа владельца контейнера: без него подпись отказывает СВОЕЮ
	 * причиною, и обход счёл бы её обрывом носителя
	 */
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Количество застигнутых обрывов чтения носителя
	size_t unreadable = 0;
	// Количество застигнутых обрывов записи носителя
	size_t unwritable = 0;
	// Собираемая дописываемая запись контейнера
	const vector <uint8_t> item = abc::value_t(string{"дописанная"}).dump();
	/**
	 * Выполняем обход всех мест обрыва ЧТЕНИЯ носителя
	 */
	/**
	 * Носитель, несущий контейнер УЖЕ ПОДПИСАННЫЙ и зафиксированный
	 *
	 * @note Фиксация подписанного контейнера вносит в дерево свёрток ПРЕЖНЮЮ запись
	 *       подписи, а для того читает её с носителя - и места эти лежат вне дороги
	 *       первой подписи. Оттого обход ведётся по контейнеру, подпись уже несущему:
	 *       три устья чтения были слепы именно потому, что подпись в обходе была первой
	 *       (замерено 05.09.2026)
	 */
	Medium signed_medium;
	{
		// Выполняем сборку контейнера о трёх записях
		this->build(signed_medium, {"первая", "вторая", "третья"});
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, signed_medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем объявление подписи контейнера
		ASSERT_TRUE(editor.sign(this->_crypto.get(), "владелец")) << "код отказа: " << abc::message(editor.error());
		// Выполняем фиксацию подписи на носителе
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	}
	/**
	 * Выполняем прогон контейнера через ещё два поколения правки
	 *
	 * @note Поколения эти нужны по замеру: фиксация вносит в дерево свёрток кадр ПРЕЖНЕЙ
	 *       записи подписи лишь тогда, когда та лежит за новым оглавлением, а у контейнера
	 *       одного поколения такого расположения ещё нет. Три устья чтения прежней подписи
	 *       оттого и стояли слепыми: обход вёлся по контейнеру, подписанному впервые
	 *       (замерено 05.09.2026)
	 */
	for(int round = 0; round < 2; round++){
		// Правщик контейнера круга
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, signed_medium)) << "круг: " << round;
		// Выполняем объявление подписи контейнера
		ASSERT_TRUE(editor.sign(this->_crypto.get(), "владелец")) << "круг: " << round;
		// Собираемая дописываемая запись контейнера
		const vector <uint8_t> more = abc::value_t(string{"круговая"}).dump();
		// Выполняем дописывание записи в конец контейнера
		ASSERT_TRUE(editor.append(more.data(), more.size(), abc::payload_t::TEXT)) << "круг: " << round;
		// Выполняем фиксацию накопленных правок на носителе
		ASSERT_TRUE(editor.commit()) << "круг: " << round;
	}
	/**
	 * Выполняем обход мест обрыва чтения при фиксации ПОДПИСАННОГО контейнера
	 */
	for(int sight = 0; sight < 64; sight++){
		// Носитель круга, несущий подписанный контейнер
		Medium medium;
		// Выполняем снятие снимка подписанного контейнера на носитель круга
		medium.data = signed_medium.data;
		// Выполняем установку предела удавшихся чтений носителя
		medium.sight = sight;
		// Правщик контейнера круга
		abc::editor_t editor;
		// Признак успешно открытого контейнера
		const bool opened = this->open(editor, medium);
		// Признак успешно объявленной подписи контейнера
		const bool signed_ = (opened && editor.sign(this->_crypto.get(), "владелец"));
		// Собираемая дописываемая запись контейнера
		const vector <uint8_t> extra = abc::value_t(string{"добавленная"}).dump();
		// Признак успешно дописанной и зафиксированной записи
		const bool done = (signed_ && editor.append(extra.data(), extra.size(), abc::payload_t::TEXT) && editor.commit());
		/**
		 * Если работа отвечена отказом, причина обязана быть обрывом ЧТЕНИЯ
		 */
		if(!done){
			// Выполняем учёт застигнутого обрыва чтения носителя
			unreadable++;
			// Выполняем проверку того, что причина названа обрывом чтения носителя
			ASSERT_EQ(editor.error(), abc::error_t::UNREADABLE_SOURCE)
				<< "предел чтений подписанного: " << sight << ", причина: " << abc::message(editor.error());
		}
	}
	for(int sight = 0; sight < 32; sight++){
		// Носитель круга, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера о трёх записях
		this->build(medium, {"первая", "вторая", "третья"});
		// Выполняем установку предела удавшихся чтений носителя
		medium.sight = sight;
		// Правщик контейнера круга
		abc::editor_t editor;
		// Признак успешно открытого контейнера
		const bool opened = this->open(editor, medium);
		// Буфер выбранной записи контейнера
		vector <uint8_t> picked;
		// Признак успешно выбранной записи контейнера
		const bool taken = (opened && editor.record(1, picked));
		/**
		 * Объявление подписи собирает свёртки по кадрам тела и читает носитель на СВОИХ
		 * местах, отличных от мест выборки записи
		 */
		const bool signed_ = (taken && editor.sign(this->_crypto.get(), "владелец"));
		// Признак успешно дописанной и зафиксированной записи
		const bool done = (signed_ && editor.append(item.data(), item.size(), abc::payload_t::TEXT) && editor.commit());
		/**
		 * Если работа отвечена отказом, причина обязана быть обрывом ЧТЕНИЯ
		 */
		if(!done){
			// Выполняем учёт застигнутого обрыва чтения носителя
			unreadable++;
			// Выполняем проверку того, что причина названа обрывом чтения носителя
			ASSERT_EQ(editor.error(), abc::error_t::UNREADABLE_SOURCE)
				<< "предел чтений: " << sight << ", причина: " << abc::message(editor.error());
		}
	}
	/**
	 * Выполняем обход всех мест обрыва ЗАПИСИ носителя
	 */
	for(int allow = 0; allow < 32; allow++){
		// Носитель круга, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера о трёх записях
		this->build(medium, {"первая", "вторая", "третья"});
		// Правщик контейнера круга
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "предел записей: " << allow;
		// Выполняем снос записи контейнера, дающий мусор
		ASSERT_TRUE(editor.erase(1)) << "предел записей: " << allow;
		// Выполняем дописывание записи в конец контейнера
		ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT)) << "предел записей: " << allow;
		// Выполняем установку предела удавшихся записей носителя
		medium.allow = allow;
		/**
		 * Если фиксация правок отвечена отказом, причина обязана быть обрывом ЗАПИСИ
		 */
		if(!editor.commit()){
			// Выполняем учёт застигнутого обрыва записи носителя
			unwritable++;
			// Выполняем проверку того, что причина названа обрывом записи носителя
			ASSERT_EQ(editor.error(), abc::error_t::UNWRITABLE_SINK)
				<< "предел записей: " << allow << ", причина: " << abc::message(editor.error());
		}
	}
	/**
	 * Выполняем обход всех мест обрыва записи носителя УБРАННОГО контейнера
	 *
	 * @note Предел здесь ставится ТОЛЬКО носителю уборки, а носитель правки оставляется
	 *       без предела: одним числом на оба носителя отказ фиксации опережал уборку, и
	 *       до записи убранного дело не доходило вовсе. Разведено щупом 05.09.2026,
	 *       назвавшим пять устий уборки слепыми при проходящей проверке
	 */
	for(int allow = 0; allow < 32; allow++){
		// Носитель круга, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера о трёх записях
		this->build(medium, {"первая", "вторая", "третья"});
		// Правщик контейнера круга
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "предел записей: " << allow;
		/**
		 * Выполняем объявление подписи контейнера: уборка ПОДПИСАННОГО контейнера пишет
		 * сверх тела ещё и запись подписи, а у той своё место обрыва - без подписи оно
		 * стояло слепым (замерено 05.09.2026)
		 */
		ASSERT_TRUE(editor.sign(this->_crypto.get(), "владелец")) << "предел записей: " << allow;
		// Выполняем снос записи контейнера, дающий мусор
		ASSERT_TRUE(editor.erase(1)) << "предел записей: " << allow;
		// Выполняем фиксацию накопленных правок на носителе
		ASSERT_TRUE(editor.commit()) << "предел записей: " << allow;
		// Носитель, куда следует убрать контейнер
		Medium cleaned;
		// Выполняем установку предела удавшихся записей носителя уборки
		cleaned.allow = allow;
		// Полная длина убранного контейнера
		uint64_t length = 0;
		/**
		 * Если уборка мусора отвечена отказом, причина обязана быть обрывом ЗАПИСИ
		 */
		if(!editor.compact([&cleaned](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
			// Выполняем запись поданных октетов убранного контейнера
			return cleaned.write(offset, buffer, size);
		}, abc::payload_t::TEXT, length)){
			// Выполняем учёт застигнутого обрыва записи носителя
			unwritable++;
			// Выполняем проверку того, что причина названа обрывом записи носителя
			ASSERT_EQ(editor.error(), abc::error_t::UNWRITABLE_SINK)
				<< "предел записей уборки: " << allow << ", причина: " << abc::message(editor.error());
		}
	}
	// Выполняем проверку того, что обрывы чтения обходом застигнуты
	ASSERT_GT(unreadable, static_cast <size_t> (0));
	// Выполняем проверку того, что обрывы записи обходом застигнуты
	ASSERT_GT(unwritable, static_cast <size_t> (0));
	/**
	 * Выполняем проверку того, что при СНЯТОМ пределе всякая работа проходит: иначе
	 * обход выше прошёл бы и на работе, отказывающей всегда
	 */
	{
		// Носитель, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера о трёх записях
		this->build(medium, {"первая", "вторая", "третья"});
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Буфер выбранной записи контейнера
		vector <uint8_t> picked;
		// Выполняем проверку того, что запись контейнера выбирается
		ASSERT_TRUE(editor.record(1, picked)) << "код отказа: " << abc::message(editor.error());
		// Выполняем снос записи контейнера, дающий мусор
		ASSERT_TRUE(editor.erase(1)) << "код отказа: " << abc::message(editor.error());
		// Выполняем дописывание записи в конец контейнера
		ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(editor.error());
		// Выполняем проверку того, что фиксация правок проходит
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
		// Носитель, куда следует убрать контейнер
		Medium cleaned;
		// Полная длина убранного контейнера
		uint64_t length = 0;
		// Выполняем проверку того, что уборка мусора проходит
		ASSERT_TRUE(editor.compact([&cleaned](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
			// Выполняем запись поданных октетов убранного контейнера
			return cleaned.write(offset, buffer, size);
		}, abc::payload_t::TEXT, length)) << "код отказа: " << abc::message(editor.error());
	}
}
/**
 * @brief Проверка того, что строка оглавления за содержимым кадра отвечена отказом
 *
 * @details Строка оглавления приходит С НОСИТЕЛЯ и недоверенна: смещение и длина в ней
 *          объявлены тем, кто контейнер подал. Строка, указывающая за содержимое снятого
 *          кадра, обязана быть отвергнута, а не вычитана за пределы буфера
 *
 * @note Близнец `IndexFixture.EntryBeyondChunk`, но у ПРАВЩИКА: у выборщика заслон этот
 *       стерегли, у правщика он стоял слепым до 05.09.2026. Оглавление кладётся без
 *       сжатия нарочно - иначе правка строки легла бы в сжатое содержимое, и отказ пришёл
 *       бы вовсе не по той причине
 */
TEST_F(EditorFixture, EntryBeyondChunkIsRefused) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера о восьми записях
	this->build(medium, {"первая", "вторая", "третья", "четвёртая", "пятая", "шестая", "седьмая", "восьмая"});
	// Снятый заголовок опознания контейнера
	abc::header_t header;
	// Код отказа снятия заголовка
	abc::error_t error = abc::error_t::NONE;
	// Выполняем снятие заголовка опознания контейнера
	ASSERT_TRUE(header.unpack(medium.data.data(), medium.data.size(), error))
		<< "код отказа: " << abc::message(error);
	// Выполняем проверку того, что оглавление контейнера объявлено
	ASSERT_GT(header.index, 0u);
	/**
	 * Выполняем проверку того, что оглавление легло в кадр открыто: сжатое содержимое
	 * правке строки не поддаётся, и отказ пришёл бы от разжатия
	 */
	ASSERT_EQ(medium.data.at(static_cast <size_t> (header.index)), 0x00) << "оглавление уложено сжатым";
	// Выполняем получение смещения первой строки оглавления в записи контейнера
	const size_t entry = static_cast <size_t> (header.index) + abc::CHUNK_HEADER;
	// Выполняем проверку того, что строка оглавления в записи контейнера умещается
	ASSERT_LE(entry + abc::ENTRY_LENGTH, medium.data.size());
	{
		// Правщик нетронутого контейнера
		abc::editor_t editor;
		// Выполняем открытие нетронутого контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Буфер выбранной записи контейнера
		vector <uint8_t> picked;
		// Выполняем проверку того, что запись нетронутого контейнера выбирается
		ASSERT_TRUE(editor.record(0, picked)) << "код отказа: " << abc::message(editor.error());
	}
	/**
	 * Выполняем подделку длины первой записи в строке оглавления непомерным числом
	 */
	for(size_t i = 0; i < 4; i++)
		// Выполняем укладку очередного октета объявленной длины записи
		medium.data.at(entry + 12 + i) = 0xFF;
	/**
	 * Выполняем обновление контрольной суммы кадра оглавления: правка кадра НА МЕСТЕ
	 * обязана обновить сумму его, иначе отказ придёт по сумме, не дойдя до строки
	 */
	{
		// Выполняем получение смещения кадра оглавления в записи контейнера
		const size_t place = static_cast <size_t> (header.index);
		// Выполняем получение длины уложенного содержимого кадра оглавления
		const size_t length = static_cast <size_t> (abc::gather(medium.data.data() + place + 4, 4));
		// Выполняем укладку обновлённой контрольной суммы кадра оглавления
		abc::fixed(medium.data.data() + place + abc::CHUNK_DIGEST,
		 abc::digest(medium.data.data() + place, abc::CHUNK_HEADER + length), 8);
	}
	// Правщик поддельного контейнера
	abc::editor_t editor;
	// Выполняем открытие поддельного контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Выполняем проверку того, что выборка поддельной записи отвечена отказом
	ASSERT_FALSE(editor.record(0, picked));
	// Выполняем проверку того, что отказ объявлен повреждённым оглавлением
	ASSERT_EQ(editor.error(), abc::error_t::INVALID_INDEX) << abc::message(editor.error());
	/**
	 * Выполняем проверку того, что соседняя запись контейнера по-прежнему выбирается:
	 * отказ принадлежал ПОДДЕЛАННОЙ строке, а не устройству правщика
	 */
	ASSERT_TRUE(editor.record(1, picked)) << "код отказа: " << abc::message(editor.error());
}
/**
 * @brief Проверка того, что причина отказа переживает откат
 *
 * @details Откат состояния при отказе фиксации собирает дерево свёрток ЗАНОВО, а сбор
 *          этот читает носитель - и чтение отказать может тоже. Причина отката затирала
 *          причину самого отказа: зовущий, встретивший отказ ШИФРОВАНИЯ, читал «работа
 *          чтения октетов отвечена отказом» и чинил бы не то
 *
 * @note Развилка найдена ЗАМЕРОМ 05.09.2026, а не рассуждением. Отказ обязан лечь на
 *       укладку ОГЛАВЛЕНИЯ (оттого порог накопления в единицу: иначе отказом ответится
 *       укладка накопленного, а её путь отката не зовёт вовсе), а предела чтений обязано
 *       хватить фиксации и НЕ хватить откату. Запас в два чтения сверх израсходованных
 *       открытием и подписью - ровно та узкая щель: без запоминания причины выходило 35
 *       (обрыв чтения), с запоминанием выходит 32 (отказ шифрования)
 *
 * @note Щель эта привязана к раскладке контейнера. Обеднеет она - проверка станет
 *       пустой, оттого сверх причины поверяется и ИСЧЕРПАНИЕ предела: без него откат до
 *       носителя не дошёл бы, и стеречь было бы нечего
 */
TEST_F(EditorFixture, TheCauseSurvivesTheRollback) {
	// Выполняем установку соли шифрования
	this->_crypto->salt("соль контейнера");
	// Выполняем установку пароля шифрования
	this->_crypto->password("пароль владельца");
	// Выполняем заведение ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера о трёх записях
	this->build(medium, {"первая", "вторая", "третья"});
	// Правщик контейнера
	abc::editor_t editor;
	// Получаем настройки правки контейнера
	abc::editor_t::settings_t rules = editor.settings();
	/**
	 * Выполняем установку порога накопления, укладывающего кадром всякую запись: отказ
	 * обязан лечь на укладку оглавления, а её путь откат и зовёт
	 */
	rules.block = 1;
	// Выполняем установку настроек правки контейнера
	editor.settings(rules);
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	/**
	 * Выполняем объявление подписи контейнера: без подписанта откат дерева свёрток не
	 * собирает вовсе, и читать ему нечего
	 */
	ASSERT_TRUE(editor.sign(this->_crypto.get(), "владелец")) << "код отказа: " << abc::message(editor.error());
	// Собираемая дописываемая запись контейнера
	const vector <uint8_t> item = abc::value_t(string{"дописанная"}).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Получаем настройки укладчика кадров
	abc::packer_t::settings_t settings = editor.packer().settings();
	/**
	 * Выполняем заказ шифрования БЕЗ модуля шифрования: модуль отдан подписанту, а
	 * укладчику не отдавался, и укладка оглавления отвечается отказом шифрования
	 */
	settings.encrypt = true;
	// Выполняем установку настроек укладчика кадров
	editor.packer().settings(settings);
	// Выполняем установку предела удавшихся чтений с запасом в два чтения
	medium.sight = medium.reads + 2;
	// Выполняем проверку того, что фиксация правок отвечена отказом
	ASSERT_FALSE(editor.commit());
	/**
	 * Выполняем проверку того, что предел чтений исчерпан: без того откат до носителя не
	 * дошёл бы вовсе, и проверка стерегла бы пустоту
	 */
	ASSERT_EQ(medium.reads, medium.sight) << "предел чтений не исчерпан, проверка негодна";
	/**
	 * Выполняем проверку того, что наружу вышла причина ОТКАЗА, а не причина отката
	 */
	ASSERT_EQ(editor.error(), abc::error_t::ENCRYPTION_FAILED)
		<< "вышла причина отката вместо причины отказа: " << abc::message(editor.error());
	/**
	 * Выполняем проверку того, что отказ принадлежал недостающей оснастке: с отданным
	 * укладчику модулем шифрования та же фиксация на исправном носителе проходит
	 */
	{
		// Носитель, несущий правимый контейнер
		Medium again;
		// Выполняем сборку контейнера о трёх записях
		this->build(again, {"первая", "вторая", "третья"});
		// Правщик контейнера
		abc::editor_t fixed;
		// Выполняем установку модуля шифрования правщику
		fixed.crypto(this->_crypto.get());
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(fixed, again)) << "код отказа: " << abc::message(fixed.error());
		// Выполняем объявление подписи контейнера
		ASSERT_TRUE(fixed.sign(this->_crypto.get(), "владелец")) << "код отказа: " << abc::message(fixed.error());
		// Выполняем дописывание записи в конец контейнера
		ASSERT_TRUE(fixed.append(item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(fixed.error());
		// Получаем настройки укладчика кадров
		abc::packer_t::settings_t allowed = fixed.packer().settings();
		// Выполняем заказ шифрования при отданном модуле шифрования
		allowed.encrypt = true;
		// Выполняем установку настроек укладчика кадров
		fixed.packer().settings(allowed);
		// Выполняем проверку того, что фиксация правок проходит
		ASSERT_TRUE(fixed.commit()) << "код отказа: " << abc::message(fixed.error());
	}
}
/**
 * @brief Проверка того, что длина кадра оглавления поверяется заголовком и у правщика
 *
 * @details Длина уложенного содержимого объявлена дважды: заголовком контейнера и самим
 *          кадром оглавления. Контрольную сумму несёт заголовок, и доверять надлежит ему.
 *          Порча четырёх октетов длины ВНУТРИ кадра суммы заголовка не задевает вовсе
 *
 * @note Устьев у правщика два: ОТКРЫТИЕ читает оглавление, и ФИКСАЦИЯ читает его снова -
 *       перед перекладкой. Второе достигается порчею носителя УЖЕ ПОСЛЕ открытия: до
 *       того отказом ответится само открытие. Близнец
 *       `IndexFixture.DeclaredIndexLengthIsCheckedAgainstTheHeader`, у правщика оба
 *       устья стояли слепыми до 05.09.2026
 */
TEST_F(EditorFixture, DeclaredIndexLengthIsCheckedAgainstTheHeader) {
	/**
	 * @brief Функция порчи объявленной длины содержимого кадра оглавления
	 *
	 * @param data октеты правимой записи контейнера
	 */
	const auto spoil = [](vector <uint8_t> & data) noexcept -> void {
		// Снятый заголовок опознания контейнера
		abc::header_t header;
		// Код отказа снятия заголовка
		abc::error_t error = abc::error_t::NONE;
		// Выполняем снятие заголовка опознания контейнера
		ASSERT_TRUE(header.unpack(data.data(), data.size(), error)) << abc::message(error);
		// Выполняем проверку того, что оглавление контейнера объявлено
		ASSERT_GT(header.index, 0u);
		// Выполняем проверку того, что объявленная длина в запись контейнера умещается
		ASSERT_LE(static_cast <size_t> (header.index) + 8, data.size());
		/**
		 * Выполняем порчу объявленной длины содержимого кадра оглавления
		 */
		for(size_t i = 0; i < 4; i++)
			// Выполняем порчу очередного октета объявленной длины содержимого
			data.at(static_cast <size_t> (header.index) + 4 + i) = 0xFF;
	};
	/**
	 * Устье первое: порча, застигнутая ОТКРЫТИЕМ контейнера
	 */
	{
		// Носитель, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера о трёх записях
		this->build(medium, {"первая", "вторая", "третья"});
		// Выполняем порчу объявленной длины содержимого кадра оглавления
		spoil(medium.data);
		// Правщик поддельного контейнера
		abc::editor_t editor;
		// Выполняем проверку того, что открытие поддельного контейнера отвечено отказом
		ASSERT_FALSE(this->open(editor, medium));
		// Выполняем проверку того, что отказ объявлен повреждённым кадром
		ASSERT_EQ(editor.error(), abc::error_t::INVALID_CHUNK) << abc::message(editor.error());
	}
	/**
	 * Устье второе: порча, застигнутая ФИКСАЦИЕЙ правок
	 */
	{
		// Носитель, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера о трёх записях
		this->build(medium, {"первая", "вторая", "третья"});
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие годного контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Собираемая дописываемая запись контейнера
		const vector <uint8_t> item = abc::value_t(string{"дописанная"}).dump();
		// Выполняем дописывание записи в конец контейнера
		ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(editor.error());
		/**
		 * Выполняем порчу носителя УЖЕ ПОСЛЕ открытия: фиксация читает оглавление снова,
		 * и порча застигается ею
		 */
		spoil(medium.data);
		// Выполняем проверку того, что фиксация правок отвечена отказом
		ASSERT_FALSE(editor.commit());
		// Выполняем проверку того, что отказ объявлен повреждённым кадром
		ASSERT_EQ(editor.error(), abc::error_t::INVALID_CHUNK) << abc::message(editor.error());
	}
	/**
	 * Выполняем проверку того, что отказы принадлежали ПОРЧЕ: нетронутый контейнер и
	 * открывается, и правки его фиксируются
	 */
	{
		// Носитель, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера о трёх записях
		this->build(medium, {"первая", "вторая", "третья"});
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Собираемая дописываемая запись контейнера
		const vector <uint8_t> item = abc::value_t(string{"дописанная"}).dump();
		// Выполняем дописывание записи в конец контейнера
		ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(editor.error());
		// Выполняем проверку того, что фиксация правок проходит
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	}
}
/**
 * @brief Проверка того, что смещение кадра за телом контейнера отвечено отказом
 *
 * @details Смещение кадра приходит ИЗ ОГЛАВЛЕНИЯ, а оглавление недоверенно: сложение
 *          `HEADER_LENGTH + origin` при смещении близ предела разрядной сетки завернулось
 *          бы и подало источнику малое смещение вместо непомерного. Оттого сторож стоит
 *          прежде первого же чтения, а тело ограничено заголовком, сумму несущим
 *
 * @note Место стояло слепым до 05.09.2026: подделку строки оглавления у правщика не
 *       наводил никто. Оглавление кладётся без сжатия нарочно - иначе правка строки легла
 *       бы в сжатое содержимое
 */
TEST_F(EditorFixture, ChunkOffsetBeyondTheBodyIsRefused) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера о трёх записях
	this->build(medium, {"первая", "вторая", "третья"});
	// Снятый заголовок опознания контейнера
	abc::header_t header;
	// Код отказа снятия заголовка
	abc::error_t error = abc::error_t::NONE;
	// Выполняем снятие заголовка опознания контейнера
	ASSERT_TRUE(header.unpack(medium.data.data(), medium.data.size(), error)) << abc::message(error);
	// Выполняем проверку того, что оглавление контейнера объявлено
	ASSERT_GT(header.index, 0u);
	/**
	 * Выполняем проверку того, что оглавление легло в кадр открыто: сжатое содержимое
	 * правке строки не поддаётся
	 */
	ASSERT_EQ(medium.data.at(static_cast <size_t> (header.index)), 0x00) << "оглавление уложено сжатым";
	// Выполняем получение смещения первой строки оглавления в записи контейнера
	const size_t entry = static_cast <size_t> (header.index) + abc::CHUNK_HEADER;
	// Выполняем проверку того, что строка оглавления в записи контейнера умещается
	ASSERT_LE(entry + abc::ENTRY_LENGTH, medium.data.size());
	{
		// Правщик нетронутого контейнера
		abc::editor_t editor;
		// Выполняем открытие нетронутого контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Буфер выбранной записи контейнера
		vector <uint8_t> picked;
		// Выполняем проверку того, что запись нетронутого контейнера выбирается
		ASSERT_TRUE(editor.record(0, picked)) << "код отказа: " << abc::message(editor.error());
	}
	/**
	 * Выполняем подделку смещения кадра первой записи непомерным числом: смещение занимает
	 * первые восемь октетов строки оглавления
	 */
	for(size_t i = 0; i < 8; i++)
		// Выполняем укладку очередного октета поддельного смещения кадра
		medium.data.at(entry + i) = 0xFF;
	/**
	 * Выполняем обновление контрольной суммы кадра оглавления: правка кадра НА МЕСТЕ
	 * обязана обновить сумму его, иначе отказ придёт по сумме, не дойдя до строки
	 */
	{
		// Выполняем получение смещения кадра оглавления в записи контейнера
		const size_t place = static_cast <size_t> (header.index);
		// Выполняем получение длины уложенного содержимого кадра оглавления
		const size_t length = static_cast <size_t> (abc::gather(medium.data.data() + place + 4, 4));
		// Выполняем укладку обновлённой контрольной суммы кадра оглавления
		abc::fixed(medium.data.data() + place + abc::CHUNK_DIGEST,
		 abc::digest(medium.data.data() + place, abc::CHUNK_HEADER + length), 8);
	}
	// Правщик поддельного контейнера
	abc::editor_t editor;
	// Выполняем открытие поддельного контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Выполняем проверку того, что выборка поддельной записи отвечена отказом
	ASSERT_FALSE(editor.record(0, picked));
	// Выполняем проверку того, что отказ объявлен повреждённым кадром
	ASSERT_EQ(editor.error(), abc::error_t::INVALID_CHUNK) << abc::message(editor.error());
	/**
	 * Выполняем проверку того, что соседняя запись контейнера по-прежнему выбирается:
	 * отказ принадлежал ПОДДЕЛАННОЙ строке, а не устройству правщика
	 */
	ASSERT_TRUE(editor.record(1, picked)) << "код отказа: " << abc::message(editor.error());
}
/**
 * @brief Проверка того, что длина кадра за телом контейнера отвечена отказом при сборе свёрток
 *
 * @details Сбор свёрток идёт по кадрам тела ПОДРЯД, а длина всякого кадра прочитана из
 *          самого контейнера и недоверенна: по ней вычитывается кадр целиком, а сдвиг
 *          обхода на неё растёт. Длина близ предела разрядной сетки завернула бы сдвиг
 *          к началу тела
 *
 * @note Место стояло слепым до 05.09.2026: подделку длины кадра ТЕЛА при подписании не
 *       наводил никто. Сумма кадра пересчитывается, иначе отказ придёт по ней
 */
TEST_F(EditorFixture, ChunkLengthBeyondTheBodyIsRefusedWhileHarvesting) {
	// Выполняем установку соли шифрования
	this->_crypto->salt("соль контейнера");
	// Выполняем установку пароля шифрования
	this->_crypto->password("пароль владельца");
	// Выполняем заведение ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера о трёх записях
	this->build(medium, {"первая", "вторая", "третья"});
	{
		// Правщик нетронутого контейнера
		abc::editor_t editor;
		// Выполняем открытие нетронутого контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем проверку того, что подпись нетронутого контейнера объявляется
		ASSERT_TRUE(editor.sign(this->_crypto.get(), "владелец")) << "код отказа: " << abc::message(editor.error());
	}
	/**
	 * Выполняем подделку объявленной длины ПЕРВОГО кадра тела непомерным числом: тело
	 * начинается сразу за заголовком опознания, а длина занимает четыре октета за меткою
	 */
	{
		// Выполняем получение смещения первого кадра тела в записи контейнера
		const size_t place = static_cast <size_t> (abc::HEADER_LENGTH);
		// Выполняем проверку того, что кадр тела в запись контейнера умещается
		ASSERT_LE(place + abc::CHUNK_HEADER, medium.data.size());
		/**
		 * Выполняем порчу объявленной длины содержимого кадра
		 */
		for(size_t i = 0; i < 4; i++)
			// Выполняем укладку очередного октета поддельной длины
			medium.data.at(place + 4 + i) = 0xFF;
		/**
		 * Выполняем обновление контрольной суммы кадра: без того отказ придёт по сумме,
		 * не дойдя до сторожа длины
		 *
		 * @note Сумма берётся по ОБЪЯВЛЕННОЙ длине, а та ныне непомерна, - оттого сумма
		 *       считается по заголовку кадра, а сторож стоит прежде чтения содержимого
		 */
		abc::fixed(medium.data.data() + place + abc::CHUNK_DIGEST,
		 abc::digest(medium.data.data() + place, abc::CHUNK_HEADER), 8);
	}
	// Правщик поддельного контейнера
	abc::editor_t editor;
	// Выполняем открытие поддельного контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что объявление подписи отвечено отказом
	ASSERT_FALSE(editor.sign(this->_crypto.get(), "владелец"));
	// Выполняем проверку того, что отказ объявлен повреждённым кадром
	ASSERT_EQ(editor.error(), abc::error_t::INVALID_CHUNK) << abc::message(editor.error());
}
/**
 * @brief Проверка того, что уборка отвергает работу до открытия и без работы записи
 *
 * @details Уборка перестраивает контейнер на иной носитель, и требует она двух вещей:
 *          открытого контейнера, откуда брать, и работы записи, куда класть. Нет одного -
 *          работать не с чем, и отказ объявляется внутренним
 *
 * @note Место стояло слепым до 05.09.2026: уборку до открытия не звал никто
 */
TEST_F(EditorFixture, CompactingRefusesWithoutItsGear) {
	// Полная длина убранного контейнера
	uint64_t length = 0;
	{
		// Правщик контейнера, контейнера не открывший
		abc::editor_t editor;
		// Носитель, куда следует убрать контейнер
		Medium cleaned;
		// Выполняем проверку того, что уборка до открытия отвечена отказом
		ASSERT_FALSE(editor.compact([&cleaned](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
			// Выполняем запись поданных октетов убранного контейнера
			return cleaned.write(offset, buffer, size);
		}, abc::payload_t::TEXT, length));
		// Выполняем проверку того, что отказ объявлен внутренним
		ASSERT_EQ(editor.error(), abc::error_t::INTERNAL) << abc::message(editor.error());
	}
	{
		// Носитель, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера о трёх записях
		this->build(medium, {"первая", "вторая", "третья"});
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем проверку того, что уборка без работы записи отвечена отказом
		ASSERT_FALSE(editor.compact(nullptr, abc::payload_t::TEXT, length));
		// Выполняем проверку того, что отказ объявлен внутренним
		ASSERT_EQ(editor.error(), abc::error_t::INTERNAL) << abc::message(editor.error());
		/**
		 * Выполняем проверку того, что отказы принадлежали НЕДОСТАЮЩЕЙ ОСНАСТКЕ: при
		 * открытом контейнере и отданной работе записи уборка проходит
		 */
		{
			// Носитель, куда следует убрать контейнер
			Medium cleaned;
			// Выполняем проверку того, что уборка проходит
			ASSERT_TRUE(editor.compact([&cleaned](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
				// Выполняем запись поданных октетов убранного контейнера
				return cleaned.write(offset, buffer, size);
			}, abc::payload_t::TEXT, length)) << "код отказа: " << abc::message(editor.error());
			// Выполняем проверку полной длины убранного контейнера
			ASSERT_EQ(length, static_cast <uint64_t> (cleaned.data.size()));
		}
	}
}
/**
 * @brief Проверка того, что запись шире поля оглавления отвергается объявленной длиной
 *
 * @details Строка оглавления держит смещение и длину записи тридцатью двумя разрядами,
 *          и запись шире четырёх гигаоктетов в неё не вмещается. Место это щуп 06.09.2026
 *          называл слепым с доводом «нужен настоящий буфер свыше четырёх гигаоктетов» -
 *          довод оказался шире правды
 *
 * @warning Буфер здесь подаётся МЕЛКИЙ, а длина объявляется заведомо большей, и это не
 *          уловка проверки, а свойство самого заслона: он мерит ОБЪЯВЛЕННУЮ длину и стоит
 *          ДО всякого касания буфера и до всякой правки состояния. Читать поданное здесь
 *          нечем и незачем - усечение длины к `uint32` произошло бы прежде чтения, и кадр
 *          объявил бы длину, содержимому не равную. Оттого проверка законна, а требовать
 *          четыре гигаоктета памяти значило бы поверять не тот заслон
 *
 * @note Заслон, стоящий ДО правки состояния, тем и поверяется, что состояние после отказа
 *       осталось прежним: то и утверждается ниже
 */
TEST_F(EditorFixture, DeclaredRecordLengthBeyondTheEntryFieldIsRefused) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с одной записью
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Количество записей контейнера до отвергнутого накопления
	const uint64_t records = editor.records();
	// Мелкий буфер, чья длина объявляется заведомо большей поля оглавления
	const vector <uint8_t> item = abc::value_t(string("запись")).dump();
	/**
	 * Выполняем проверку того, что запись объявленной длины шире поля оглавления
	 * отвергается
	 */
	ASSERT_FALSE(editor.append(item.data(),
	 static_cast <size_t> (numeric_limits <uint32_t>::max()) + 1, abc::payload_t::TEXT))
		<< "запись шире поля оглавления принята";
	// Выполняем проверку того, что причина названа недопустимой длиной
	ASSERT_EQ(editor.error(), abc::error_t::INVALID_LENGTH) << abc::message(editor.error());
	/**
	 * Выполняем проверку того, что состояние правки осталось прежним: заслон стоит ДО
	 * всякой правки, и отвергнутое накопление следа оставить не вправе
	 */
	ASSERT_EQ(editor.records(), records) << "отвергнутое накопление изменило состав контейнера";
	/**
	 * Выполняем проверку того, что та же запись ПОДЛИННОЙ длины принимается: без этого
	 * проверка прошла бы и при правщике, отвергающем всякое накопление вовсе
	 */
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
}
/**
 * @brief Проверка того, что нулевые пороги настроек поднимаются наименьшими
 *
 * @details Нуль тут не отвергается, а ПОДНИМАЕТСЯ, и различие существенно: настройки
 *          заводятся пустой структурой намеренно, оттого нуль есть обычный вид
 *          «потребитель порога не назначал», а не ошибка его. Порог накопления
 *          в нуль записей означал бы укладку, не наступающую никогда, а порог
 *          самочинной фиксации в нуль - фиксацию на всякой правке либо отсутствие
 *          её вовсе, смотря по сличению; оба исхода суть молчаливая поломка
 *
 * @note Утверждается ОТДАЧА настроек, а не поведение укладки: заслон обещает ровно
 *       подъём числа, и сличать надлежит то, что он обещает. Поведение при пороге в
 *       единицу закреплено соседними проверками и здесь не поверяется
 *
 * @note Пороги поднимаются ПОРОЗНЬ, двумя вызовами: подъём обоих разом прошёл бы и
 *       при единственном работающем заслоне, ибо второй нуль поднялся бы соседом
 *
 */
TEST_F(EditorFixture, ZeroThresholdsAreRaisedToTheSmallest) {
	// Правщик контейнера
	abc::editor_t editor;
	/**
	 * Выполняем проверку подъёма порога накопления записей
	 */
	{
		// Настройки правки контейнера
		abc::editor_t::settings_t settings = editor.settings();
		// Выполняем обнуление порога накопления записей
		settings.block = 0;
		// Выполняем установку настроек правки контейнера
		editor.settings(settings);
		// Выполняем проверку того, что порог накопления поднят наименьшим
		ASSERT_EQ(editor.settings().block, 1ul) << "нулевой порог накопления оставлен нулём";
	}
	/**
	 * Выполняем проверку подъёма порога самочинной фиксации
	 */
	{
		// Настройки правки контейнера
		abc::editor_t::settings_t settings = editor.settings();
		// Выполняем обнуление порога самочинной фиксации
		settings.limit = 0;
		// Выполняем установку настроек правки контейнера
		editor.settings(settings);
		// Выполняем проверку того, что порог самочинной фиксации поднят наименьшим
		ASSERT_EQ(editor.settings().limit, 1ul) << "нулевой порог фиксации оставлен нулём";
	}
}
/**
 * @brief Проверка того, что отвергнутое смещение кадра до носителя не доходит
 *
 * @details Сосед `ChunkOffsetBeyondTheBodyIsRefused` наводит тот же сценарий и утверждает
 *          ту же причину, а заслон СМЕЩЕНИЯ при том не закрепляет: обесточь его - и отказ
 *          придёт ниже по течению, назвавшись тем же `INVALID_CHUNK`, отчего проверка
 *          устоит. Установлено сплошным щупом нужности 07.09.2026, и запись при самом
 *          заслоне, гласившая «закреплено проверкою», была шире правды
 *
 * @note Своё у заслона одно, и утверждается здесь ровно оно: правщик обязан НЕ СПРАШИВАТЬ
 *       у носителя смещения за пределами самого контейнера. Без заслона носителю уходит
 *       запрос на смещении под `2^64 - 1`; отказом он ответит, и причина совпадёт с честной, - но
 *       потребитель, чей носитель есть файл, сеть или чужое устройство, к такому запросу
 *       не обязан быть готов вовсе
 *
 * @note Утверждается МЕСТО запроса, а не число их: количество чтений есть устройство
 *       правщика и вправе меняться, а обещание «не за телом» - договор с потребителем
 *
 */
TEST_F(EditorFixture, RefusedChunkOffsetNeverReachesTheMedium) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера о трёх записях
	this->build(medium, {"первая", "вторая", "третья"});
	// Снятый заголовок опознания контейнера
	abc::header_t header;
	// Код отказа снятия заголовка
	abc::error_t error = abc::error_t::NONE;
	// Выполняем снятие заголовка опознания контейнера
	ASSERT_TRUE(header.unpack(medium.data.data(), medium.data.size(), error)) << abc::message(error);
	// Выполняем проверку того, что оглавление контейнера объявлено
	ASSERT_GT(header.index, 0u);
	// Выполняем проверку того, что оглавление легло в кадр открыто
	ASSERT_EQ(medium.data.at(static_cast <size_t> (header.index)), 0x00) << "оглавление уложено сжатым";
	// Выполняем получение смещения первой строки оглавления в записи контейнера
	const size_t entry = static_cast <size_t> (header.index) + abc::CHUNK_HEADER;
	// Выполняем проверку того, что строка оглавления в записи контейнера умещается
	ASSERT_LE(entry + abc::ENTRY_LENGTH, medium.data.size());
	/**
	 * Выполняем подделку смещения кадра первой записи непомерным числом
	 */
	for(size_t i = 0; i < 8; i++)
		// Выполняем укладку очередного октета поддельного смещения кадра
		medium.data.at(entry + i) = 0xFF;
	/**
	 * Выполняем обновление контрольной суммы кадра оглавления: правка кадра НА МЕСТЕ
	 * обязана обновить сумму его, иначе отказ придёт по сумме, не дойдя до строки
	 */
	{
		// Выполняем получение смещения кадра оглавления в записи контейнера
		const size_t place = static_cast <size_t> (header.index);
		// Выполняем получение длины уложенного содержимого кадра оглавления
		const size_t length = static_cast <size_t> (abc::gather(medium.data.data() + place + 4, 4));
		// Выполняем укладку обновлённой контрольной суммы кадра оглавления
		abc::fixed(medium.data.data() + place + abc::CHUNK_DIGEST,
		 abc::digest(medium.data.data() + place, abc::CHUNK_HEADER + length), 8);
	}
	// Количество обращений правщика к носителю
	size_t reads = 0;
	// Правщик поддельного контейнера
	abc::editor_t editor;
	/**
	 * Выполняем открытие контейнера носителем, записывающим затребованные смещения
	 */
	ASSERT_TRUE(editor.open([&medium, &reads](const uint64_t offset, const size_t size,
	 vector <uint8_t> & result) noexcept -> bool {
		// Выполняем учёт очередного обращения к носителю
		reads++;
		// Выполняем чтение затребованных октетов контейнера
		return medium.read(offset, size, result);
	}, [&medium](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
		// Выполняем запись поданных октетов контейнера
		return medium.write(offset, buffer, size);
	}, static_cast <uint64_t> (medium.data.size()))) << "код отказа: " << abc::message(editor.error());
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Выполняем сброс учёта обращений: открытие читает носитель законно
	reads = 0;
	// Выполняем проверку того, что выборка поддельной записи отвечена отказом
	ASSERT_FALSE(editor.record(0, picked));
	// Выполняем проверку того, что отказ объявлен повреждённым кадром
	ASSERT_EQ(editor.error(), abc::error_t::INVALID_CHUNK) << abc::message(editor.error());
	/**
	 * Выполняем проверку того, что носителя не спрашивали ВОВСЕ
	 *
	 * @note Утверждается ЧИСЛО обращений, а не место их, и это не придирчивость. Два
	 *       написания до этого пали, и оба - на доводе шире правды. Границею тела: за
	 *       телом законно лежат оглавление и подпись, и читать их правщик обязан.
	 *       Границею контейнера: сумма `HEADER_LENGTH + origin` при `origin` под
	 *       `2^64 - 1` ЗАВОРАЧИВАЕТСЯ и даёт смещение 95 - вполне внутри контейнера,
	 *       отчего никакой сторож МЕСТА заслона не различает. Заворот и есть то, чем
	 *       место опасно: без заслона правщик читает не за краем, а НЕ ТАМ, оставаясь
	 *       с виду в границах, и отказ приходит соседом по той же причине
	 */
	ASSERT_EQ(reads, 0ul) << "правщик обратился к носителю " << reads
		<< " раз по строке с поддельным смещением";
	/**
	 * Выполняем проверку того, что соседняя запись по-прежнему выбирается: без этого
	 * проверка прошла бы и при правщике, не спрашивающем носителя вовсе
	 */
	ASSERT_TRUE(editor.record(1, picked)) << "код отказа: " << abc::message(editor.error());
}
/**
 * @brief Проверка того, что снятие подписи снимает её и с носителя
 *
 * @details Ноль вместо модуля шифрования означает СНЯТИЕ подписи, а не отказ объявления:
 *          заслон отвечает успехом и чистит дерево свёрток, не собирая их вовсе. Место
 *          стояло слепым до 07.09.2026 - подписи не снимал никто, сплошной щуп нужности
 *          назвал его ни разу не сработавшим
 *
 * @note Утверждается СОСТОЯНИЕ НОСИТЕЛЯ после фиксации, а не одна отдача метода. Отдача
 *       успехом сама по себе прошла бы и при правщике, снятие пропустившем: подпись
 *       осталась бы объявленной, а заслон обещает именно её отсутствие
 *
 * @note Половина первая утверждает, что подпись до снятия ДЕЙСТВИТЕЛЬНО стояла. Без неё
 *       проверка прошла бы и при правщике, не подписывающем вовсе, - и слепота вышла бы
 *       не меньше прежней
 *
 */
TEST_F(EditorFixture, SigningWithoutTheCryptoRemovesTheSignature) {
	// Выполняем заведение ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера с тремя записями
	this->build(medium, {"первая", "вторая", "третья"});
	// Код отказа поверки подписи владельца
	abc::error_t error = abc::error_t::NONE;
	/**
	 * Половина первая: подпись объявляется и на носителе оказывается
	 */
	{
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем объявление подписи правимого контейнера
		ASSERT_TRUE(editor.sign(this->_crypto.get(), "владелец")) << "код отказа: " << abc::message(editor.error());
		// Выполняем фиксацию накопленных правок на носителе
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
		// Выполняем проверку того, что подпись контейнера сходится
		ASSERT_TRUE(abc::verify(* this->_crypto, "владелец",
		 medium.data.data(), medium.data.size(), error))
			<< "код отказа: " << abc::message(error);
	}
	/**
	 * Половина вторая: подпись снимается нулём вместо модуля шифрования
	 */
	{
		// Правщик подписанного контейнера
		abc::editor_t editor;
		// Выполняем открытие подписанного контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем снятие подписи правимого контейнера
		ASSERT_TRUE(editor.sign(nullptr, "")) << "код отказа: " << abc::message(editor.error());
		// Выполняем фиксацию накопленных правок на носителе
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	}
	/**
	 * Выполняем проверку того, что подписи на носителе более НЕТ
	 */
	ASSERT_FALSE(abc::verify(* this->_crypto, "владелец",
	 medium.data.data(), medium.data.size(), error))
		<< "подпись пережила снятие";
	// Выполняем проверку того, что отказ назван отсутствием подписи
	ASSERT_EQ(error, abc::error_t::UNSIGNED_CONTAINER) << abc::message(error);
	/**
	 * Выполняем проверку того, что содержимое контейнера снятие подписи пережило: снятие
	 * трогает подпись, а не записи
	 */
	{
		// Выборщик записей контейнера
		vector <uint8_t> picked;
		// Правщик контейнера без подписи
		abc::editor_t editor;
		// Выполняем открытие контейнера без подписи правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем проверку количества записей контейнера
		ASSERT_EQ(editor.records(), 3ull);
		// Выполняем проверку того, что запись выбирается
		ASSERT_TRUE(editor.record(1, picked)) << "код отказа: " << abc::message(editor.error());
	}
}
/**
 * @brief Проверка отказа открытия при расхождении длины оглавления с заголовком
 *
 * @details Длина уложенного содержимого объявлена ДВАЖДЫ: кадром оглавления и заголовком
 *          опознания. Заголовок несёт контрольную сумму, кадр её не несёт, - оттого
 *          заголовком и поверяется прочитанное из кадра. Место стояло слепым до
 *          07.09.2026: расхождения двух длин не наводил никто
 *
 * @note Подделанная длина взята УМЕЩАЮЩЕЙСЯ - на единицу меньше объявленной. Довод тут
 *       в различении соседа: десятью строками ниже стоит сторож кадра, в контейнер не
 *       умещающегося, и подделка непомерным числом досталась бы ЕМУ, а не этому. Длина
 *       меньшая проходит соседа насквозь, и отказать вправе один лишь сторож расхождения
 *
 * @note Контрольная сумма кадра оглавления пересобирается по НОВОЙ длине: правка кадра на
 *       месте без пересборки суммы дала бы отказ по сумме, не дойдя до сличения длин
 *
 */
TEST_F(EditorFixture, IndexLengthDisagreeingWithTheHeaderIsRefused) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера о трёх записях
	this->build(medium, {"первая", "вторая", "третья"});
	// Снятый заголовок опознания контейнера
	abc::header_t header;
	// Код отказа снятия заголовка
	abc::error_t error = abc::error_t::NONE;
	// Выполняем снятие заголовка опознания контейнера
	ASSERT_TRUE(header.unpack(medium.data.data(), medium.data.size(), error)) << abc::message(error);
	// Выполняем проверку того, что оглавление контейнера объявлено
	ASSERT_GT(header.index, 0u);
	// Выполняем проверку того, что объём оглавления объявлен заголовком
	ASSERT_GT(header.extent, 1u);
	{
		// Правщик нетронутого контейнера
		abc::editor_t editor;
		/**
		 * Выполняем проверку того, что нетронутый контейнер открывается: без этого
		 * проверка прошла бы и при правщике, не открывающем вовсе
		 */
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	}
	// Выполняем получение смещения кадра оглавления в записи контейнера
	const size_t place = static_cast <size_t> (header.index);
	// Подделанная длина уложенного содержимого кадра оглавления
	const uint32_t spiked = (header.extent - 1);
	/**
	 * Выполняем укладку подделанной длины в заголовок кадра оглавления: длина занимает
	 * четыре октета, начиная с четвёртого
	 */
	abc::fixed(medium.data.data() + place + 4, spiked, 4);
	/**
	 * Выполняем обновление контрольной суммы кадра оглавления по НОВОЙ длине
	 */
	abc::fixed(medium.data.data() + place + abc::CHUNK_DIGEST,
	 abc::digest(medium.data.data() + place, abc::CHUNK_HEADER + static_cast <size_t> (spiked)), 8);
	// Признак того, что тело кадра оглавления вычитывалось
	bool body = false;
	// Правщик поддельного контейнера
	abc::editor_t editor;
	/**
	 * Выполняем открытие контейнера носителем, стерегущим чтение тела оглавления
	 */
	ASSERT_FALSE(editor.open([&medium, &body, place](const uint64_t offset, const size_t size,
	 vector <uint8_t> & result) noexcept -> bool {
		// Если читается кадр оглавления длиною свыше заголовка его, отмечаем чтение тела
		if((offset == static_cast <uint64_t> (place)) && (size > abc::CHUNK_HEADER))
			// Отмечаем состоявшееся чтение тела кадра оглавления
			body = true;
		// Выполняем чтение затребованных октетов контейнера
		return medium.read(offset, size, result);
	}, [&medium](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
		// Выполняем запись поданных октетов контейнера
		return medium.write(offset, buffer, size);
	}, static_cast <uint64_t> (medium.data.size()))) << "контейнер о двух несогласных длинах открыт";
	// Выполняем проверку того, что отказ объявлен повреждённым кадром
	ASSERT_EQ(editor.error(), abc::error_t::INVALID_CHUNK) << abc::message(editor.error());
	/**
	 * Выполняем проверку того, что тела оглавления правщик НЕ читал
	 *
	 * @note Утверждается ДЕЙСТВИЕ, а не причина, и это не придирчивость. Первое написание
	 *       утверждало причину и заслона НЕ закрепило: подделанная длина доходит до
	 *       снятия оглавления, и то отвергает её само, назвав ту же причину. Своё у
	 *       заслона одно - отказать, НЕ ВЫЧИТЫВАЯ тела по недоверенной длине; это
	 *       единственное, чего соседи не дают
	 */
	ASSERT_FALSE(body) << "правщик вычитал тело оглавления по несогласной длине";
}
/**
 * @brief Проверка того, что повторный снос записи мусора не удваивает
 *
 * @details Снос записи прибавляет длину её к счёту мусора, а счёт этот переливается на
 *          носитель фиксацией. Заслон повторного сноса отвечает успехом, НИЧЕГО не
 *          прибавляя: без него длина легла бы в счёт вторично, и уборка обещала бы
 *          мусор, какого на носителе нет. Место стояло слепым до 07.09.2026 - дважды
 *          одну запись не сносил никто
 *
 * @note Утверждается СЧЁТ МУСОРА, а не отдача метода. Отдача успехом прошла бы и без
 *       заслона: повторный снос сработал бы обычным путём и отдал бы тот же успех, -
 *       различает их одно лишь удвоение счёта
 *
 * @note Сличаются ДВА контейнера, собранные одинаково, а не число с числом ожидаемым:
 *       длина записи есть устройство укладки и вправе меняться, а равенство двух счётов
 *       - обещание заслона. Второй контейнер тут не роскошь, а сам довод
 *
 */
TEST_F(EditorFixture, RepeatedErasureDoesNotDoubleTheGarbage) {
	// Счёт мусора на носителе после одного сноса
	uint64_t once = 0;
	// Счёт мусора на носителе после сноса дважды
	uint64_t twice = 0;
	/**
	 * Выполняем снос записи ОДНАЖДЫ
	 */
	{
		// Носитель, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера о трёх записях
		this->build(medium, {"первая", "вторая", "третья"});
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем снос второй записи контейнера
		ASSERT_TRUE(editor.erase(1)) << "код отказа: " << abc::message(editor.error());
		// Выполняем фиксацию накопленных правок на носителе
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
		// Выполняем получение счёта мусора на носителе
		once = editor.garbage();
	}
	/**
	 * Выполняем снос той же записи ДВАЖДЫ
	 */
	{
		// Носитель, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера о трёх записях
		this->build(medium, {"первая", "вторая", "третья"});
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем снос второй записи контейнера
		ASSERT_TRUE(editor.erase(1)) << "код отказа: " << abc::message(editor.error());
		/**
		 * Выполняем повторный снос той же записи: заслон обязан отдать успех, ничего
		 * не прибавив к счёту мусора
		 */
		ASSERT_TRUE(editor.erase(1)) << "код отказа: " << abc::message(editor.error());
		// Выполняем фиксацию накопленных правок на носителе
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
		// Выполняем получение счёта мусора на носителе
		twice = editor.garbage();
	}
	/**
	 * Выполняем проверку того, что снос однажды мусор ВООБЩЕ насчитал: без этого
	 * равенство ниже прошло бы и при правщике, мусора не считающем вовсе
	 */
	ASSERT_GT(once, 0ull) << "снос записи мусора не насчитал";
	// Выполняем проверку того, что повторный снос счёта мусора не удвоил
	ASSERT_EQ(twice, once) << "повторный снос насчитал мусора " << twice << " против " << once;
}
/**
 * @brief Проверка того, что счёт мусора восстанавливается при повторном открытии
 *
 * @details Снос записи копится в памяти и переливается на носитель фиксацией; правщик же,
 *          открывший контейнер ЗАНОВО, о прежних сносах не знает ничего и обязан
 *          восстановить счёт по оглавлению - строки, несущие признак сноса, и есть мусор
 *          на носителе. Место стояло слепым до 07.09.2026: контейнер, уже несущий
 *          снесённые записи, не открывал никто
 *
 * @note Сличается счёт ДВУХ правщиков над одним носителем, а не число с ожидаемым: длина
 *       записи есть устройство укладки и вправе меняться, а равенство счётов - обещание
 *
 */
TEST_F(EditorFixture, GarbageIsRecoveredFromTheIndexOnReopening) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера о трёх записях
	this->build(medium, {"первая", "вторая", "третья"});
	// Счёт мусора, добытый сносом и фиксацией
	uint64_t committed = 0;
	{
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем проверку того, что нетронутый контейнер мусора не несёт
		ASSERT_EQ(editor.garbage(), 0ull) << "нетронутый контейнер объявлен с мусором";
		// Выполняем снос второй записи контейнера
		ASSERT_TRUE(editor.erase(1)) << "код отказа: " << abc::message(editor.error());
		// Выполняем фиксацию накопленных правок на носителе
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
		// Выполняем получение счёта мусора на носителе
		committed = editor.garbage();
	}
	/**
	 * Выполняем проверку того, что снос мусора ВООБЩЕ насчитал: без этого равенство
	 * ниже прошло бы и при правщике, мусора не считающем вовсе
	 */
	ASSERT_GT(committed, 0ull) << "снос записи мусора не насчитал";
	// Правщик, открывающий тот же носитель ЗАНОВО
	abc::editor_t reopened;
	// Выполняем повторное открытие носителя правщиком
	ASSERT_TRUE(this->open(reopened, medium)) << "код отказа: " << abc::message(reopened.error());
	/**
	 * Выполняем проверку того, что счёт мусора восстановлен по оглавлению
	 */
	ASSERT_EQ(reopened.garbage(), committed)
		<< "повторное открытие насчитало мусора " << reopened.garbage() << " против " << committed;
}
/**
 * @brief Проверка того, что мусорный кадр учитывается при повторном открытии
 *
 * @details Подмена записи кладёт новый кадр в конец, а прежний метит признаком мусора:
 *          строка оглавления при этом не сносится, а перенаправляется, - оттого обход
 *          СТРОК такого мусора не видит вовсе, и находит его лишь обход ЗАГОЛОВКОВ
 *          кадров. Место стояло слепым до 07.09.2026: контейнер, несущий мусорный кадр,
 *          заново не открывал никто
 *
 * @note Проверка эта соседке `GarbageIsRecoveredFromTheIndexOnReopening` не двойник:
 *       та идёт сносом и учитывается ОГЛАВЛЕНИЕМ, эта - подменой и учитывается
 *       ЗАГОЛОВКАМИ КАДРОВ. Два источника счёта мусора различны, и один другого не
 *       подпирает
 *
 */
TEST_F(EditorFixture, WastedChunkIsCountedOnReopening) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера о трёх записях
	this->build(medium, {"первая", "вторая", "третья"});
	// Счёт мусора, добытый подменою и фиксацией
	uint64_t committed = 0;
	// Объём оглавления ДО подмены
	uint64_t before = 0;
	// Подменяющая запись контейнера
	const vector <uint8_t> item = abc::value_t(string{"подменённая запись"}).dump();
	{
		// Заголовок опознания контейнера до подмены
		abc::header_t head;
		// Код отказа снятия заголовка
		abc::error_t error = abc::error_t::NONE;
		// Выполняем снятие заголовка опознания контейнера
		ASSERT_TRUE(head.unpack(medium.data.data(), medium.data.size(), error)) << abc::message(error);
		// Выполняем запоминание объёма оглавления до подмены
		before = static_cast <uint64_t> (head.extent);
	}
	{
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем проверку того, что нетронутый контейнер мусора не несёт
		ASSERT_EQ(editor.garbage(), 0ull) << "нетронутый контейнер объявлен с мусором";
		// Выполняем подмену второй записи контейнера
		ASSERT_TRUE(editor.replace(1, item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(editor.error());
		// Выполняем фиксацию накопленных правок на носителе
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
		// Выполняем получение счёта мусора на носителе
		committed = editor.garbage();
	}
	/**
	 * Выполняем проверку того, что подмена мусора ВООБЩЕ насчитала
	 */
	ASSERT_GT(committed, 0ull) << "подмена записи мусора не насчитала";
	// Правщик, открывающий тот же носитель ЗАНОВО
	abc::editor_t reopened;
	// Выполняем повторное открытие носителя правщиком
	ASSERT_TRUE(this->open(reopened, medium)) << "код отказа: " << abc::message(reopened.error());
	// Выполняем проверку того, что мусорный кадр при открытии сосчитан
	/**
	 * Выполняем проверку того, что при открытии сосчитан ПРЕЖНИЙ КАДР ОГЛАВЛЕНИЯ целиком:
	 * фиксация метит его признаком мусора, и обход заголовков находит его по признаку
	 */
	ASSERT_EQ(reopened.garbage(), static_cast <uint64_t> (abc::CHUNK_HEADER) + before)
		<< "повторное открытие насчитало " << reopened.garbage()
		<< " при прежнем оглавлении в " << (abc::CHUNK_HEADER + before) << " октетов";
	/**
	 * Выполняем проверку того, что счёт фиксации БОЛЬШЕ на длину перенаправленной записи
	 *
	 * @note Расхождение это НЕ дефект и не находка проверки: свойство объявлено при самой
	 * работе `garbage()` как НИЖНЯЯ ГРАНИЦА, и причина там же названа - запись, вытесненная
	 * правкой внутри живого кадра, на носителе не сыскивается, ибо строка её перезаписана,
	 * а длина нигде более не хранится. Пометить же кадр целиком нельзя: кадр вправе нести
	 * НЕСКОЛЬКО записей, и пометка отняла бы у потребителя живых соседей
	 *
	 * @warning Закрепляется расхождение ЧИСЛОМ намеренно: объявленное словом свойство
	 * проверкою прежде не поверялось, и занижение могло вырасти молча. Замер 07.09.2026:
	 * 117 при фиксации против 104 по переоткрытии, разница ровно в длину прежней записи
	 */
	ASSERT_EQ(committed, reopened.garbage() + abc::value_t(string{"вторая"}).dump().size())
		<< "фиксация насчитала " << committed << ", переоткрытие " << reopened.garbage();
	/**
	 * Выполняем проверку того, что подменённая запись читается новым содержимым: без
	 * этого проверка прошла бы и при подмене, содержимого не сменившей
	 */
	{
		// Буфер выбранной записи контейнера
		vector <uint8_t> picked;
		// Выполняем выборку подменённой записи
		ASSERT_TRUE(reopened.record(1, picked)) << "код отказа: " << abc::message(reopened.error());
		// Выполняем проверку содержимого подменённой записи
		ASSERT_EQ(picked, item);
	}
}
/**
 * @brief Проверка того, что повторная выборка той же записи носителя не трогает
 *
 * @details Снятый кадр удерживается правщиком, и затребованный наново он отдаётся из
 *          удержания, не читая носителя вовсе. Место стояло слепым до 07.09.2026: одну
 *          и ту же запись дважды подряд не выбирал никто
 *
 * @note Утверждается ДЕЙСТВИЕ, а не отдача: без удержания повторная выборка прошла бы
 *       успехом ровно так же, прочитав носитель заново, - различает их одно лишь число
 *       обращений. Носитель подаётся замыканием, и счёт в нём дёшев
 *
 * @note Утверждается и РАВЕНСТВО выданного: удержание, отдающее чужой кадр, тоже не
 *       читало бы носителя, и счёт обращений один такую поломку пропустил бы
 *
 */
TEST_F(EditorFixture, RepeatedRecordFetchDoesNotTouchTheMedium) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера о трёх записях
	this->build(medium, {"первая", "вторая", "третья"});
	// Количество обращений правщика к носителю
	size_t reads = 0;
	// Правщик контейнера
	abc::editor_t editor;
	/**
	 * Выполняем открытие контейнера носителем, считающим обращения
	 */
	ASSERT_TRUE(editor.open([&medium, &reads](const uint64_t offset, const size_t size,
	 vector <uint8_t> & result) noexcept -> bool {
		// Выполняем учёт очередного обращения к носителю
		reads++;
		// Выполняем чтение затребованных октетов контейнера
		return medium.read(offset, size, result);
	}, [&medium](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
		// Выполняем запись поданных октетов контейнера
		return medium.write(offset, buffer, size);
	}, static_cast <uint64_t> (medium.data.size()))) << "код отказа: " << abc::message(editor.error());
	// Буфер записи, выбранной впервые
	vector <uint8_t> first;
	// Выполняем сброс учёта обращений: открытие читает носитель законно
	reads = 0;
	// Выполняем первую выборку записи контейнера
	ASSERT_TRUE(editor.record(1, first)) << "код отказа: " << abc::message(editor.error());
	/**
	 * Выполняем проверку того, что первая выборка носитель ЧИТАЛА: без этого проверка
	 * ниже прошла бы и при правщике, не читающем носителя вовсе
	 */
	ASSERT_GT(reads, 0ul) << "первая выборка записи носителя не читала";
	// Буфер записи, выбранной повторно
	vector <uint8_t> second;
	// Выполняем сброс учёта обращений перед повторной выборкой
	reads = 0;
	// Выполняем повторную выборку той же записи контейнера
	ASSERT_TRUE(editor.record(1, second)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что повторная выборка носителя не трогала
	ASSERT_EQ(reads, 0ul) << "повторная выборка обратилась к носителю " << reads << " раз";
	// Выполняем проверку того, что удержание отдало ТУ ЖЕ запись
	ASSERT_EQ(second, first) << "удержание отдало запись, отличную от прочитанной";
	// Выполняем проверку того, что выданное есть затребованная запись
	ASSERT_EQ(first, abc::value_t(string{"вторая"}).dump());
}
/**
 * @brief Проверка самочинной фиксации по РАЗМЕРУ накопленных правок
 *
 * @details Способ `SIZE` закрепляет правки, едва накопленных октетов станет не меньше
 *          порога. Место стояло слепым до 08.09.2026: близнец его по КОЛИЧЕСТВУ правок
 *          закреплён `EditorFixture.CommitByTheCountOfEdits`, а по размеру - никем,
 *          хотя способ этот отдан потребителю наравне с прочими
 *
 * @note Порог берётся ЗАМЕРЕННЫМ, а не угаданным: сколько октетов копит одна запись,
 *       решает укладка, и число это вправе меняться. Оттого сперва меряется накопление
 *       одной записи правщиком отдельным, а порог ставится на октет выше замера - тогда
 *       первая запись его заведомо не достаёт, а вторая заведомо переступает
 *
 * @note Утверждается ПОКОЛЕНИЕ заголовка, а не отдача `append`: отдача успехом приходит
 *       и без самочинной фиксации, а поколение растёт лишь удавшейся фиксацией
 *
 */
TEST_F(EditorFixture, CommitBySizeOfPendingEdits) {
	// Первая дописываемая запись
	const vector <uint8_t> first = abc::value_t(string{"вторая"}).dump();
	// Вторая дописываемая запись
	const vector <uint8_t> second = abc::value_t(string{"третья"}).dump();
	// Накопление, какое даёт одна дописанная запись
	size_t single = 0;
	/**
	 * Выполняем замер накопления одной записи правщиком отдельным
	 */
	{
		// Носитель замера
		Medium gauge;
		// Выполняем сборку контейнера об одной записи
		this->build(gauge, {"первая"});
		// Правщик замера
		abc::editor_t probe;
		// Выполняем открытие контейнера правщиком замера
		ASSERT_TRUE(this->open(probe, gauge)) << "код отказа: " << abc::message(probe.error());
		// Выполняем дописывание записи в конец контейнера
		ASSERT_TRUE(probe.append(first.data(), first.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(probe.error());
		// Выполняем получение накопления одной записи
		single = probe.pending();
		// Выполняем проверку того, что накопление вообще состоялось
		ASSERT_GT(single, 0ul) << "дописанная запись ничего не накопила";
	}
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера об одной записи
	this->build(medium, {"первая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Получаем настройки правки контейнера
	abc::editor_t::settings_t settings = editor.settings();
	// Выполняем установку способа фиксации по размеру накопленных правок
	settings.mode = abc::editor_t::mode_t::SIZE;
	// Выполняем установку порога самочинной фиксации на октет выше замера
	settings.limit = (single + 1);
	// Выполняем установку настроек правки контейнера
	editor.settings(settings);
	// Выполняем дописывание первой записи в конец контейнера
	ASSERT_TRUE(editor.append(first.data(), first.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	/**
	 * Выполняем проверку того, что порога одна запись не достала и фиксации не вызвала
	 */
	ASSERT_EQ(editor.header().generation, 0ull) << "фиксация случилась до порога";
	// Выполняем проверку того, что накопленное на месте
	ASSERT_EQ(editor.pending(), single) << "накопление разошлось с замером";
	// Выполняем дописывание второй записи в конец контейнера
	ASSERT_TRUE(editor.append(second.data(), second.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что порог размера вызвал фиксацию сам
	ASSERT_EQ(editor.header().generation, 1ull) << "порог размера фиксации не вызвал";
	// Выполняем проверку того, что накопленных правок не осталось
	ASSERT_EQ(editor.pending(), 0ul) << "фиксация накопленного не забрала";
	/**
	 * Выполняем проверку того, что обе записи легли на носитель: фиксация, ничего не
	 * записавшая, подняла бы поколение и опустошила накопление тем же порядком
	 */
	{
		// Правщик, открывающий тот же носитель заново
		abc::editor_t reopened;
		// Выполняем повторное открытие носителя правщиком
		ASSERT_TRUE(this->open(reopened, medium)) << "код отказа: " << abc::message(reopened.error());
		// Выполняем проверку количества записей контейнера
		ASSERT_EQ(reopened.records(), 3ull) << "самочинная фиксация записей не уложила";
		// Буфер выбранной записи контейнера
		vector <uint8_t> picked;
		// Выполняем выборку последней записи контейнера
		ASSERT_TRUE(reopened.record(2, picked)) << "код отказа: " << abc::message(reopened.error());
		// Выполняем проверку содержимого последней записи
		ASSERT_EQ(picked, second);
	}
}
/**
 * @brief Проверка того, что срок поверяется и ВЫБОРКОЙ, а не одними правками
 *
 * @details Близнец `EditorFixture.DeadlineCommit` наводит наступление срока ДОПИСЫВАНИЕМ,
 *          то есть правкой. Место же на пути ЧТЕНИЯ стояло слепым до 08.09.2026, хотя
 *          стережёт оно случай особый и наиболее вероятный: правки кончились, а
 *          накопленное осталось. Срок, поверяемый одними правками, там не наступает
 *          НИКОГДА, и накопленное лежало бы незакреплённым до самого закрытия
 *
 * @note Правок после наступления срока не вносится вовсе - в том и суть. Единственное
 *       обращение к правщику есть выборка записи, и она обязана срок поверить
 *
 * @note Отказ фиксации выборке не вредит по устройству, и утверждается это отдельно:
 *       запись выдаётся та самая, какую спрашивали
 *
 */
TEST_F(EditorFixture, DeadlineIsCheckedByTheFetchAsWell) {
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
	// Выполняем сборку дописываемой записи
	const vector <uint8_t> item = abc::value_t(string{"вторая"}).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что срок ещё не наступил и фиксации не было
	ASSERT_EQ(editor.header().generation, 0ull) << "фиксация случилась до срока";
	// Выполняем проверку того, что накопленное на месте
	ASSERT_GT(editor.pending(), 0ul) << "дописанная запись ничего не накопила";
	// Выполняем ожидание наступления срока
	this_thread::sleep_for(chrono::milliseconds(60));
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	/**
	 * Выполняем ВЫБОРКУ записи, и более ничего: правок после наступления срока не
	 * вносится вовсе, и поверить срок обязана она одна
	 */
	ASSERT_TRUE(editor.record(0, picked)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что наступивший срок вызвал фиксацию выборкою
	ASSERT_EQ(editor.header().generation, 1ull) << "выборка срока не поверила";
	// Выполняем проверку того, что накопленных правок не осталось
	ASSERT_EQ(editor.pending(), 0ul) << "фиксация накопленного не забрала";
	// Выполняем проверку того, что выборка выдала затребованную запись
	ASSERT_EQ(picked, abc::value_t(string{"первая"}).dump()) << "выборка выдала не ту запись";
	/**
	 * Выполняем проверку того, что дописанное легло на носитель
	 */
	{
		// Буфер записи, выбранной с носителя
		vector <uint8_t> stored;
		// Код отказа выборки записи контейнера
		abc::error_t error = abc::error_t::NONE;
		// Выполняем выборку дописанной записи с носителя
		ASSERT_TRUE(this->pick(medium, 1, stored, error)) << "код отказа: " << abc::message(error);
		// Выполняем проверку выбранной записи контейнера
		ASSERT_EQ(stored, item);
	}
}
/**
 * @brief Проверка того, что уборка копит записи кадром по порогу накопления
 *
 * @details Уборка перестраивает контейнер, укладывая живые записи кадрами, и порог
 *          накопления решает, сколько записей ляжет в один кадр. Место стояло слепым до
 *          08.09.2026: уборку проверяли содержимым, а число кадров не считал никто, -
 *          и накопление, отменённое правкой, прошло бы незамеченным. Убранный контейнер
 *          остался бы верным, но кадр приходился бы на всякую запись, и всякий нёс бы
 *          свои тридцать два октета заголовка да свою свёртку
 *
 * @note Утверждается ЧИСЛО КАДРОВ, а не длина: длина зависит и от сжатия, и от порчи,
 *       обращённой в мусор, а число кадров есть прямое следствие порога накопления
 *
 * @note Сличаются ДВЕ уборки одного и того же контейнера при разных порогах, а не число
 *       с ожидаемым: порог наименьший даёт кадр на запись, порог просторный - один кадр
 *       на все. Сличение это и делает проверку зрячей к отмене накопления
 *
 */
TEST_F(EditorFixture, CompactionBatchesRecordsByTheBlockThreshold) {
	// Записи, какими наполняется правимый контейнер
	const vector <string> records = {"первая", "вторая", "третья", "четвёртая"};
	/**
	 * @brief Работа уборки контейнера при заданном пороге накопления
	 *
	 * @param block порог накопления записей
	 * @return      количество кадров тела убранного контейнера
	 *
	 */
	const auto chunks = [this, &records](const size_t block) noexcept -> size_t {
		// Носитель, несущий правимый контейнер
		Medium medium;
		// Выполняем сборку контейнера с записями
		this->build(medium, records);
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		if(!this->open(editor, medium))
			// Выводим признак неудачной уборки
			return 0;
		// Получаем настройки правки контейнера
		abc::editor_t::settings_t settings = editor.settings();
		// Выполняем установку порога накопления записей
		settings.block = block;
		// Выполняем установку настроек правки контейнера
		editor.settings(settings);
		// Носитель, куда следует убрать контейнер
		Medium cleaned;
		// Полная длина убранного контейнера
		uint64_t length = 0;
		// Выполняем уборку контейнера
		if(!editor.compact([&cleaned](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
			// Выполняем запись поданных октетов убранного контейнера
			return cleaned.write(offset, buffer, size);
		}, abc::payload_t::TEXT, length))
			// Выводим признак неудачной уборки
			return 0;
		// Сниматель убранного контейнера
		abc::loader_t loader;
		// Выполняем подачу убранного контейнера снимателю
		if(!loader.feed(cleaned.data.data(), cleaned.data.size()))
			// Выводим признак неудачной подачи
			return 0;
		// Количество снятых кадров тела контейнера
		size_t result = 0;
		// Содержимое очередного снятого кадра
		vector <uint8_t> payload;
		// Сведения об очередном снятом кадре
		abc::chunk_t chunk;
		/**
		 * Выполняем перебор всех кадров тела убранного контейнера
		 */
		while(loader.next(payload, chunk))
			// Выполняем учёт очередного снятого кадра
			result++;
		// Выводим количество кадров тела убранного контейнера
		return result;
	};
	// Количество кадров при пороге наименьшем
	const size_t tight = chunks(1);
	// Количество кадров при пороге, вмещающем все записи разом
	const size_t roomy = chunks(65536);
	/**
	 * Выполняем проверку того, что порог наименьший дал кадр на всякую запись
	 */
	ASSERT_EQ(tight, records.size()) << "порог в единицу дал кадров " << tight
		<< " при записях " << records.size();
	/**
	 * Выполняем проверку того, что порог просторный собрал все записи одним кадром
	 */
	ASSERT_EQ(roomy, 1ul) << "просторный порог дал кадров " << roomy;
}
/**
 * @brief Проверка того, что уборка кладёт отпечаток НЫНЕШНЕГО подписанта
 *
 * @details Отпечаток есть усечённая свёртка канонической записи открытого ключа, и нужен
 *          он потребителю затем, чтобы выбрать ключ поверки, не перебирая все. Уборка
 *          вырабатывает его наново по нынешнему подписанту, а заголовок убранного
 *          контейнера ведётся от прежнего - и потому отпечаток обязан быть ПЕРЕПИСАН, а
 *          не унаследован
 *
 * @warning Ключи здесь РАЗНЫЕ намеренно, и в этом весь довод. При одном ключе новая
 * выработка от унаследованного значения неотличима, и утверждение отпечатка заслона не
 * закрепляет вовсе: щуп отверг такое написание в
 * `EditorFixture.CompactKeepsSignature` (08.09.2026). Ключ иной разводит два исхода:
 * с укладкой в заголовке лежит отпечаток НОВОГО владельца, без неё - прежнего
 *
 * @note Утверждается и НЕРАВЕНСТВО прежнему: равенство новому доказывало бы то же лишь
 *       при заведомо разных отпечатках, а неравенство прежнему делает довод полным
 *
 */
TEST_F(EditorFixture, CompactionStoresTheFingerprintOfTheCurrentSigner) {
	// Выполняем заведение ключа прежнего владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("прежний", crypto_t::signature_t::ED25519));
	// Выполняем заведение ключа нового владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("новый", crypto_t::signature_t::ED25519));
	// Отпечаток открытого ключа прежнего владельца
	vector <uint8_t> was;
	// Отпечаток открытого ключа нового владельца
	vector <uint8_t> now;
	// Выполняем выработку отпечатка ключа прежнего владельца
	ASSERT_TRUE(abc::fingerprint(* this->_crypto, "прежний", was)) << "отпечаток прежнего не выработан";
	// Выполняем выработку отпечатка ключа нового владельца
	ASSERT_TRUE(abc::fingerprint(* this->_crypto, "новый", now)) << "отпечаток нового не выработан";
	/**
	 * Выполняем проверку того, что отпечатки двух ключей РАЗЛИЧНЫ: без этого проверка
	 * ниже не различала бы укладку от наследования вовсе
	 */
	ASSERT_NE(vector <uint8_t> (was.begin(), was.begin() + abc::FINGERPRINT_LENGTH),
	 vector <uint8_t> (now.begin(), now.begin() + abc::FINGERPRINT_LENGTH))
		<< "отпечатки двух ключей совпали, проверка недоказательна";
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера о трёх записях
	this->build(medium, {"первая", "вторая", "третья"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем объявление подписи ПРЕЖНЕГО владельца
	ASSERT_TRUE(editor.sign(this->_crypto.get(), "прежний")) << "код отказа: " << abc::message(editor.error());
	// Выполняем снос записи, чтобы уборке было что убирать
	ASSERT_TRUE(editor.erase(1)) << "код отказа: " << abc::message(editor.error());
	// Выполняем фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что мусор правкой накопился
	ASSERT_GT(editor.garbage(), 0ull) << "снос мусора не накопил";
	// Выполняем смену подписанта на НОВОГО владельца
	ASSERT_TRUE(editor.sign(this->_crypto.get(), "новый")) << "код отказа: " << abc::message(editor.error());
	// Носитель, куда следует убрать контейнер
	Medium cleaned;
	// Полная длина убранного контейнера
	uint64_t length = 0;
	// Выполняем уборку контейнера ключом нового владельца
	ASSERT_TRUE(editor.compact([&cleaned](const uint64_t offset, const void * buffer, const size_t size) noexcept -> bool {
		// Выполняем запись поданных октетов убранного контейнера
		return cleaned.write(offset, buffer, size);
	}, abc::payload_t::TEXT, length)) << "код отказа: " << abc::message(editor.error());
	// Правщик убранного контейнера
	abc::editor_t opened;
	// Выполняем открытие убранного контейнера правщиком
	ASSERT_TRUE(this->open(opened, cleaned)) << "код отказа: " << abc::message(opened.error());
	// Отпечаток, уложенный уборкой в заголовок убранного контейнера
	const vector <uint8_t> stored(opened.header().fingerprint,
	 opened.header().fingerprint + abc::FINGERPRINT_LENGTH);
	// Выполняем проверку того, что в заголовке лежит отпечаток НОВОГО владельца
	ASSERT_EQ(stored, vector <uint8_t> (now.begin(), now.begin() + abc::FINGERPRINT_LENGTH))
		<< "уборка уложила отпечаток не нынешнего подписанта";
	// Выполняем проверку того, что прежний отпечаток в заголовке не остался
	ASSERT_NE(stored, vector <uint8_t> (was.begin(), was.begin() + abc::FINGERPRINT_LENGTH))
		<< "уборка оставила отпечаток прежнего владельца";
	/**
	 * Выполняем проверку того, что подпись убранного контейнера сходится ключом НОВОГО
	 * владельца: отпечаток без сошедшейся подписи был бы обещанием впустую
	 */
	{
		// Код отказа поверки подписи владельца
		abc::error_t error = abc::error_t::NONE;
		// Выполняем проверку сходимости подписи нового владельца
		ASSERT_TRUE(abc::verify(* this->_crypto, "новый",
		 cleaned.data.data(), cleaned.data.size(), error))
			<< "код отказа: " << abc::message(error);
	}
}
/**
 * @brief Проверка того, что откажнувшая фиксация пересобирает дерево свёрток подписи
 *
 * @details Дерево свёрток растёт лишь вперёд, и кадры, внесённые в него до отказа, изъять
 *          из него нельзя. Оттого откат подписанного контейнера обязан собрать дерево
 *          наново по кадрам тела: без того СЛЕДУЮЩАЯ фиксация уложила бы корень, считанный
 *          по лишним кадрам, и подпись её не сошлась бы на теле - причём отказала бы она
 *          молча, ибо сама фиксация прошла бы успешно
 *
 * @note Утверждается ДЕЙСТВИЕ - схождение подписи после повторной фиксации, а не причина
 *       отказа: причину закрепляет соседняя `TheCauseSurvivesTheRollback`, и утверждать
 *       её здесь значило бы стеречь чужой заслон
 *
 */
TEST_F(EditorFixture, RefusedCommitRebuildsTheSigningTree) {
	// Выполняем установку соли шифрования
	this->_crypto->salt("соль контейнера");
	// Выполняем установку пароля шифрования
	this->_crypto->password("пароль владельца");
	// Выполняем заведение ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера о трёх записях
	this->build(medium, {"первая", "вторая", "третья"});
	// Правщик контейнера
	abc::editor_t editor;
	// Получаем настройки правки контейнера
	abc::editor_t::settings_t rules = editor.settings();
	/**
	 * Выполняем установку порога накопления, укладывающего кадром всякую запись: отказ
	 * обязан лечь на укладку оглавления, а её путь откат и зовёт
	 */
	rules.block = 1;
	// Выполняем установку настроек правки контейнера
	editor.settings(rules);
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Выполняем объявление подписи контейнера
	ASSERT_TRUE(editor.sign(this->_crypto.get(), "владелец")) << "код отказа: " << abc::message(editor.error());
	// Собираемая дописываемая запись контейнера
	const vector <uint8_t> item = abc::value_t(string{"дописанная"}).dump();
	// Выполняем дописывание записи в конец контейнера
	ASSERT_TRUE(editor.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(editor.error());
	// Получаем настройки укладчика кадров
	abc::packer_t::settings_t settings = editor.packer().settings();
	/**
	 * Выполняем заказ шифрования БЕЗ модуля шифрования у укладчика: укладка оглавления
	 * отвечается отказом, и фиксация уходит в откат, успев внести кадры в дерево свёрток
	 */
	settings.encrypt = true;
	// Выполняем установку настроек укладчика кадров
	editor.packer().settings(settings);
	// Выполняем проверку того, что фиксация правок отвечена отказом
	ASSERT_FALSE(editor.commit()) << "фиксация прошла, откат не звучал, проверка негодна";
	// Выполняем отмену заказа шифрования укладчику кадров
	settings.encrypt = false;
	// Выполняем установку настроек укладчика кадров
	editor.packer().settings(settings);
	// Выполняем повторную фиксацию накопленных правок на носителе
	ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	// Код отказа поверки подписи владельца
	abc::error_t error = abc::error_t::NONE;
	/**
	 * Выполняем поверку подписи владельца правленного контейнера: корень обязан быть
	 * считан по кадрам тела, а не по дереву, отравленному отказавшей фиксацией
	 */
	ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", medium.data.data(), medium.data.size(), error))
		<< "код отказа: " << abc::message(error);
}
/**
 * @brief Проверка того, что кадр, в тело не умещающийся, обход мусора прекращает
 *
 * @details Счёт мусора при открытии обходит ЗАГОЛОВКИ кадров тела, а длина кадра взята из
 *          самого кадра и недоверенна. Кадр, объявивший длину сверх тела, обязан обход
 *          прекратить: без заслона счёт вобрал бы объявленную длину целиком и отдал бы
 *          потребителю мусор, какого на носителе нет, - а тот погнал бы уборку за пустотой
 *
 * @note Порча кладётся на кадр МУСОРНЫЙ намеренно: живой кадр в счёт не идёт и при снятом
 *       заслоне, и проверка на нём была бы зелена всегда
 *
 * @note Утверждается ДЕЙСТВИЕ - счёт мусора после порчи, - а не причина: открытие здесь
 *       обязано УДАТЬСЯ, ибо счёт мусора совещателен и отвергать по нему годный контейнер
 *       нельзя, и никакой причины наружу не выходит вовсе
 *
 */
TEST_F(EditorFixture, ChunkLengthBeyondTheBodyStopsTheWasteWalk) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера о трёх записях
	this->build(medium, {"первая", "вторая", "третья"});
	// Подменяющая запись контейнера
	const vector <uint8_t> item = abc::value_t(string{"подменённая запись"}).dump();
	{
		// Правщик контейнера
		abc::editor_t editor;
		// Выполняем открытие контейнера правщиком
		ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
		// Выполняем подмену второй записи контейнера
		ASSERT_TRUE(editor.replace(1, item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(editor.error());
		/**
		 * Выполняем фиксацию накопленных правок: она метит прежнее оглавление мусором,
		 * и обход находит его по признаку
		 */
		ASSERT_TRUE(editor.commit()) << "код отказа: " << abc::message(editor.error());
	}
	// Счёт мусора, добытый переоткрытием НЕТРОНУТОГО носителя
	uint64_t honest = 0;
	{
		// Правщик, открывающий тот же носитель заново
		abc::editor_t reopened;
		// Выполняем повторное открытие носителя правщиком
		ASSERT_TRUE(this->open(reopened, medium)) << "код отказа: " << abc::message(reopened.error());
		// Выполняем получение счёта мусора на носителе
		honest = reopened.garbage();
	}
	/**
	 * Выполняем проверку того, что мусор обходом ВООБЩЕ насчитан: без того порча
	 * отнимала бы нуль от нуля, и проверка стерегла бы пустоту
	 */
	ASSERT_GT(honest, 0ull) << "переоткрытие мусора не насчитало, проверка негодна";
	// Предел тела правленного контейнера
	const uint64_t bound = static_cast <uint64_t> (medium.data.size());
	// Смещение найденного мусорного кадра тела контейнера
	uint64_t found = 0;
	/**
	 * Выполняем обход заголовков кадров тела тем же путём, каким идёт открытие
	 */
	for(uint64_t place = abc::HEADER_LENGTH; (place + abc::CHUNK_HEADER) <= bound;){
		// Выполняем снятие объявленной длины содержимого очередного кадра
		uint64_t length = 0;
		/**
		 * Выполняем сборку длины из четырёх октетов, от младшего к старшему
		 */
		for(uint8_t i = 0; i < 4; i++)
			// Выполняем внесение очередного октета объявленной длины
			length |= (static_cast <uint64_t> (medium.data.at(static_cast <size_t> (place) + 4 + i)) << (i * 8));
		// Если очередной кадр помечен мусором, обход прекращается
		if((medium.data.at(static_cast <size_t> (place) + abc::CHUNK_FLAGS) & abc::CHUNK_WASTE) != 0){
			// Выполняем запоминание смещения найденного мусорного кадра
			found = place;
			// Прекращаем обход заголовков кадров
			break;
		}
		// Выполняем переход к следующему кадру тела контейнера
		place += (static_cast <uint64_t> (abc::CHUNK_HEADER) + length);
	}
	// Выполняем проверку того, что мусорный кадр в теле сыскан
	ASSERT_GT(found, 0ull) << "мусорного кадра в теле нет, проверка негодна";
	/**
	 * Выполняем объявление длины кадра сверх предела тела: четыре октета старшими
	 * единицами дают длину, тела заведомо превосходящую
	 */
	for(uint8_t i = 0; i < 4; i++)
		// Выполняем порчу очередного октета объявленной длины кадра
		medium.data.at(static_cast <size_t> (found) + 4 + i) = 0xFF;
	// Правщик, открывающий испорченный носитель
	abc::editor_t spoiled;
	/**
	 * Выполняем проверку того, что открытие УДАЛОСЬ: счёт мусора совещателен, и годный
	 * контейнер по нему не отвергается
	 */
	ASSERT_TRUE(this->open(spoiled, medium)) << "код отказа: " << abc::message(spoiled.error());
	/**
	 * Выполняем проверку того, что объявленная сверх тела длина в счёт НЕ ВОШЛА: обход
	 * прекращён кадром, и всё, что за ним, осталось несочтённым
	 */
	ASSERT_LT(spoiled.garbage(), honest)
		<< "счёт мусора вобрал объявленную сверх тела длину: " << spoiled.garbage()
		<< " при честном счёте " << honest;
}
/**
 * @brief Проверка того, что воронка отказа доносит причину журналу
 *
 * @details Воронка `fail` заводит код отказа и объявляет его журналу. Объявление это -
 *          единственный путь, каким об отказе узнаёт СТОРОННИЙ наблюдатель: зовущий видит
 *          лишь ложь возврата, а причину спрашивает не всякий. Обесточь объявление - и
 *          всякий отказ правщика стал бы для журнала невидим, притом что набор остался бы
 *          зелен: проверки спрашивают `error()`, а не журнал
 *
 * @note Утверждаются ОБЕ половины условия: причина доносится при отказе и НЕ доносится
 *       при успехе. Вторая половина стережёт запись при самой воронке - сброс кода отказа
 *       в неё не идёт, иначе журнал полнился бы «no error» на всякий удавшийся вызов
 *
 */
TEST_F(EditorFixture, TheRefusalFunnelReportsItsCauseToTheJournal) {
	// Носитель, несущий правимый контейнер
	Medium medium;
	// Выполняем сборку контейнера о двух записях
	this->build(medium, {"первая", "вторая"});
	// Правщик контейнера
	abc::editor_t editor;
	// Выполняем открытие контейнера правщиком
	ASSERT_TRUE(this->open(editor, medium)) << "код отказа: " << abc::message(editor.error());
	// Донесения, снятые с журнала подпискою
	vector <string> journal;
	// Разрешаем отложенный вывод: подписка кормится именно им
	awh::log::mode({awh::log::mode_t::DEFERRED});
	// Выполняем подписку на журнал ради разбора донесений об отказах
	awh::log::subscribe([&journal](const awh::log::flag_t, const string_view text) noexcept -> void {
		// Выполняем накопление очередного донесения журнала
		journal.emplace_back(text);
	});
	// Буфер выбираемой записи контейнера
	vector <uint8_t> picked;
	/**
	 * Выполняем УДАВШУЮСЯ выборку записи: журналу доносить нечего, и молчание его
	 * закрепляется наравне с донесением
	 */
	ASSERT_TRUE(editor.record(0, picked)) << "код отказа: " << abc::message(editor.error());
	// Выполняем проверку того, что удавшаяся работа журнала не тронула
	ASSERT_TRUE(journal.empty()) << "журнал принял донесение об удавшейся работе: " << journal.front();
	/**
	 * Выполняем выборку записи по номеру, оглавлению не отвечающему: работа обязана быть
	 * отвечена отказом
	 */
	ASSERT_FALSE(editor.record(0x1000, picked)) << "выборка за пределом оглавления удалась, проверка негодна";
	// Выполняем получение объявленной причины отказа
	const abc::error_t cause = editor.error();
	// Выполняем проверку того, что причина отказа объявлена
	ASSERT_NE(cause, abc::error_t::NONE) << "отказ причины не объявил, проверка негодна";
	// Выполняем проверку того, что журнал донесение об отказе принял
	ASSERT_FALSE(journal.empty()) << "отказ журналу не объявлен вовсе";
	// Признак того, что донесение несёт объявленную причину отказа
	bool named = false;
	/**
	 * Выполняем перебор всех накопленных донесений журнала
	 */
	for(const string & line : journal)
		// Отмечаем, что очередное донесение несёт объявленную причину отказа
		named = (named || (line.find(abc::message(cause)) != string::npos));
	/**
	 * Выполняем проверку того, что донесение несёт ИМЕННО объявленную причину: без того
	 * проверка прошла бы и при донесении о чём угодно ином
	 */
	ASSERT_TRUE(named) << "донесение не несёт объявленной причины «" << abc::message(cause)
		<< "», первое из принятых: " << journal.front();
	// Выполняем снятие подписки на журнал
	awh::log::subscribe(nullptr);
}
