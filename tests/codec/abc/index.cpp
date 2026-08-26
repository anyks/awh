/**
 * @file index.cpp
 * @date 2026-08-19
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки оглавления бинарного контейнера ABC — круговой обход строк, выборка
 *        записи по номеру из большого контейнера и удержание снятого кадра
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
#include <cstdint>
#include <limits>

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
	 * @brief Класс опоры проверок оглавления контейнера
	 *
	 */
	class IndexFixture : public testing::Test {
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
				// Выполняем установку соли шифрования
				this->_crypto->salt("соль контейнера");
				// Выполняем установку пароля шифрования
				this->_crypto->password("пароль владельца");
			}
	};
	/**
	 * @brief Класс источника октетов контейнера с учётом вычитанного
	 *
	 * @details Источник этот считает вычитанное, чем и доказывается, что выборка не
	 *          читает контейнера целиком: без счёта проверка прошла бы и при чтении всего
	 *
	 */
	class Source {
		public:
			// Октеты контейнера, отданные источнику
			vector <uint8_t> data;
			// Количество вычитанных источником октетов
			size_t taken = 0;
			// Количество обращений к источнику
			size_t calls = 0;
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
				// Выполняем увеличение количества обращений к источнику
				this->calls++;
				// Если затребованные октеты за концом отданных источнику
				if((offset + size) > static_cast <uint64_t> (this->data.size()))
					// Выводим признак неудачного чтения октетов
					return false;
				// Выполняем увеличение количества вычитанных октетов
				this->taken += size;
				// Выполняем выдачу затребованных октетов
				result.assign(this->data.begin() + static_cast <ptrdiff_t> (offset),
				 this->data.begin() + static_cast <ptrdiff_t> (offset + size));
				// Выводим признак успешного чтения октетов
				return true;
			}
	};
	/**
	 * @brief Функция сборки записи из поданного текста
	 *
	 * @param text укладываемый в запись текст
	 * @return     собранная запись
	 *
	 */
	vector <uint8_t> record(const string & text) noexcept {
		// Выполняем сборку записи из поданного текста
		return abc::value_t(text).dump();
	}
};

/**
 * @brief Проверка кругового обхода строк оглавления
 *
 */
TEST_F(IndexFixture, EntriesRoundtrip) {
	// Собираемое оглавление контейнера
	abc::index_t index(this->_log.get());
	// Собираемая строка оглавления
	abc::entry_t entry;
	// Выполняем установку смещения кадра от начала тела контейнера
	entry.chunk = 0x0102030405060708ull;
	// Выполняем установку смещения записи в содержимом кадра
	entry.offset = 0x090A0B0Cu;
	// Выполняем установку длины записи
	entry.length = 0x0D0E0F10u;
	// Выполняем внесение строки в собираемое оглавление
	index.add(entry);
	// Буфер уложенного оглавления
	vector <uint8_t> buffer;
	// Выполняем укладку оглавления в октеты
	index.pack(buffer);
	// Выполняем проверку длины уложенного оглавления
	ASSERT_EQ(buffer.size(), abc::ENTRY_LENGTH);
	// Снимаемое оглавление контейнера
	abc::index_t taken(this->_log.get());
	// Код отказа снятия оглавления
	abc::error_t error = abc::error_t::NONE;
	// Выполняем снятие оглавления с октетов
	ASSERT_TRUE(taken.unpack(buffer.data(), buffer.size(), error))
		<< "код отказа: " << abc::message(error);
	// Выполняем проверку количества снятых строк оглавления
	ASSERT_EQ(taken.size(), 1ul);
	// Выполняем проверку смещения кадра от начала тела контейнера
	ASSERT_EQ(taken.entries().front().chunk, 0x0102030405060708ull);
	// Выполняем проверку смещения записи в содержимом кадра
	ASSERT_EQ(taken.entries().front().offset, 0x090A0B0Cu);
	// Выполняем проверку длины записи
	ASSERT_EQ(taken.entries().front().length, 0x0D0E0F10u);
}
/**
 * @brief Проверка отказа снятия повреждённого оглавления
 *
 */
TEST_F(IndexFixture, CorruptedEntries) {
	// Снимаемое оглавление контейнера
	abc::index_t index(this->_log.get());
	// Код отказа снятия оглавления
	abc::error_t error = abc::error_t::NONE;
	// Октеты, длиною не кратные длине строки оглавления
	const vector <uint8_t> ragged(abc::ENTRY_LENGTH + 3, 0x11);
	// Выполняем проверку отказа снятия оглавления рваной длины
	ASSERT_FALSE(index.unpack(ragged.data(), ragged.size(), error));
	// Выполняем проверку кода отказа снятия оглавления
	ASSERT_EQ(error, abc::error_t::INVALID_INDEX);
	// Октеты строки оглавления с нулевой длиною записи
	const vector <uint8_t> empty(abc::ENTRY_LENGTH, 0x00);
	// Выполняем проверку отказа снятия строки с нулевой длиною записи
	ASSERT_FALSE(index.unpack(empty.data(), empty.size(), error));
	// Выполняем проверку кода отказа снятия оглавления
	ASSERT_EQ(error, abc::error_t::INVALID_INDEX);
	// Выполняем проверку того, что повреждённое оглавление снятым не осталось
	ASSERT_EQ(index.size(), 0ul);
	/**
	 * Выполняем проверку отказа снятия строки с неведомыми разрядами свойств.
	 *
	 * Разряды свойств строки опознаются наравне с разрядами кадра: оглавление
	 * целостностью не защищено, и порча старших разрядов ушла бы обратно в запись
	 */
	{
		// Строка оглавления с ведомыми разрядами свойств
		abc::entry_t entry;
		// Выполняем установку смещения кадра от начала тела контейнера
		entry.chunk = 0;
		// Выполняем установку смещения записи в содержимом кадра
		entry.offset = 0;
		// Выполняем установку длины записи
		entry.length = 16;
		// Выполняем установку разрядов свойств строки оглавления
		entry.marks = static_cast <uint32_t> (abc::mark_t::NONE);
		// Укладываемое оглавление контейнера
		abc::index_t source(this->_log.get());
		// Выполняем внесение строки оглавления
		source.add(entry);
		// Октеты уложенного оглавления
		vector <uint8_t> record;
		// Выполняем укладку оглавления в октеты
		source.pack(record);
		// Выполняем проверку длины уложенного оглавления
		ASSERT_EQ(record.size(), abc::ENTRY_LENGTH);
		// Выполняем проверку того, что оглавление с ведомыми разрядами снимается
		ASSERT_TRUE(index.unpack(record.data(), record.size(), error))
			<< "код отказа: " << abc::message(error);
		/**
		 * Выполняем перебор всех неведомых разрядов свойств строки оглавления
		 */
		for(uint8_t bit = 1; bit < 32; bit++){
			// Октеты повреждаемого оглавления
			vector <uint8_t> damaged = record;
			// Выполняем установку неведомого разряда свойств строки
			damaged.at(16 + (bit / 8)) |= static_cast <uint8_t> (1u << (bit % 8));
			// Выполняем проверку отказа снятия строки с неведомыми разрядами
			ASSERT_FALSE(index.unpack(damaged.data(), damaged.size(), error))
				<< "разряд свойств: " << static_cast <uint16_t> (bit);
			// Выполняем проверку кода отказа снятия оглавления
			ASSERT_EQ(error, abc::error_t::INVALID_INDEX) << "разряд свойств: " << static_cast <uint16_t> (bit);
			// Выполняем проверку того, что повреждённое оглавление снятым не осталось
			ASSERT_EQ(index.size(), 0ul) << "разряд свойств: " << static_cast <uint16_t> (bit);
		}
	}
}
/**
 * @brief Проверка выборки всякой записи контейнера по номеру
 *
 */
TEST_F(IndexFixture, FetchByNumber) {
	// Сборщик контейнера
	abc::assembler_t assembler(this->_log.get());
	// Выполняем установку модуля сжатия сборщику контейнера
	assembler.compressor(this->_compressor.get());
	// Получаем настройки сборки контейнера
	abc::assembler_t::settings_t settings = assembler.settings();
	// Выполняем установку порога накопления записей, дающего несколько кадров
	settings.block = 256;
	// Выполняем установку настроек сборки контейнера
	assembler.settings(settings);
	// Внесённые в контейнер записи
	vector <vector <uint8_t>> records;
	/**
	 * Выполняем внесение сотни записей в собираемый контейнер
	 */
	for(size_t i = 0; i < 100; i++){
		// Выполняем сборку очередной записи
		const vector <uint8_t> item = record(string{"запись номер "} + to_string(i));
		// Выполняем внесение очередной записи в собираемый контейнер
		ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(assembler.error());
		// Выполняем накопление внесённой записи
		records.push_back(item);
	}
	// Источник октетов собранного контейнера
	Source source;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(source.data)) << "код отказа: " << abc::message(assembler.error());
	// Выполняем проверку количества строк оглавления собранного контейнера
	ASSERT_EQ(assembler.index().size(), records.size());
	// Выборщик записей контейнера
	abc::fetcher_t fetcher(this->_log.get());
	// Выполняем установку модуля сжатия выборщику записей
	fetcher.compressor(this->_compressor.get());
	// Выполняем открытие контейнера отданной работой чтения
	ASSERT_TRUE(fetcher.open([&source](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем чтение затребованных октетов контейнера
		return source.read(offset, size, result);
	})) << "код отказа: " << abc::message(fetcher.error());
	// Выполняем проверку количества записей открытого контейнера
	ASSERT_EQ(fetcher.records(), static_cast <uint64_t> (records.size()));
	/**
	 * Выполняем выборку всякой записи открытого контейнера вразнобой, от конца к
	 * началу: выборка по порядку прошла бы и при чтении подряд
	 */
	for(size_t i = records.size(); i > 0; i--){
		// Буфер выбранной записи контейнера
		vector <uint8_t> item;
		// Выполняем выборку очередной записи контейнера по номеру
		ASSERT_TRUE(fetcher.record(static_cast <uint64_t> (i - 1), item))
			<< "код отказа: " << abc::message(fetcher.error()) << " на записи " << (i - 1);
		// Выполняем проверку выбранной записи контейнера
		ASSERT_EQ(item, records.at(i - 1));
	}
	// Буфер выбранной записи контейнера
	vector <uint8_t> item;
	// Выполняем проверку отказа выборки записи за оглавлением контейнера
	ASSERT_FALSE(fetcher.record(static_cast <uint64_t> (records.size()), item));
	// Выполняем проверку кода отказа выборки записи
	ASSERT_EQ(fetcher.error(), abc::error_t::INVALID_INDEX);
}
/**
 * @brief Проверка выборки записи из шифрованного контейнера
 *
 */
TEST_F(IndexFixture, FetchFromSecured) {
	// Сборщик контейнера
	abc::assembler_t assembler(this->_log.get());
	// Выполняем установку модуля сжатия сборщику контейнера
	assembler.compressor(this->_compressor.get());
	// Выполняем установку модуля шифрования сборщику контейнера
	assembler.crypto(this->_crypto.get());
	// Получаем настройки укладки кадра
	abc::packer_t::settings_t packing = assembler.packer().settings();
	// Выполняем установку признака шифрования содержимого кадра
	packing.encrypt = true;
	// Выполняем установку настроек укладки кадра
	assembler.packer().settings(packing);
	// Внесённые в контейнер записи
	vector <vector <uint8_t>> records;
	/**
	 * Выполняем внесение десятка записей в собираемый контейнер
	 */
	for(size_t i = 0; i < 10; i++){
		// Выполняем сборку очередной записи
		const vector <uint8_t> item = record(string{"тайная запись "} + to_string(i));
		// Выполняем внесение очередной записи в собираемый контейнер
		ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(assembler.error());
		// Выполняем накопление внесённой записи
		records.push_back(item);
	}
	// Источник октетов собранного контейнера
	Source source;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(source.data)) << "код отказа: " << abc::message(assembler.error());
	// Выборщик записей контейнера
	abc::fetcher_t fetcher(this->_log.get());
	// Выполняем установку модуля сжатия выборщику записей
	fetcher.compressor(this->_compressor.get());
	// Выполняем установку модуля шифрования выборщику записей
	fetcher.crypto(this->_crypto.get());
	// Выполняем открытие контейнера отданной работой чтения
	ASSERT_TRUE(fetcher.open([&source](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем чтение затребованных октетов контейнера
		return source.read(offset, size, result);
	})) << "код отказа: " << abc::message(fetcher.error());
	// Буфер выбранной записи контейнера
	vector <uint8_t> item;
	// Выполняем выборку последней записи шифрованного контейнера
	ASSERT_TRUE(fetcher.record(static_cast <uint64_t> (records.size() - 1), item))
		<< "код отказа: " << abc::message(fetcher.error());
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(item, records.back());
}
/**
 * @brief Проверка того, что выборка не читает контейнера целиком
 *
 * @details Ради того оглавление и заведено: до одной записи большого контейнера
 *          добираются чтением заголовка, оглавления и одного кадра, а не всего тела
 *
 */
TEST_F(IndexFixture, FetchReadsPart) {
	// Сборщик контейнера
	abc::assembler_t assembler(this->_log.get());
	// Получаем настройки сборки контейнера
	abc::assembler_t::settings_t settings = assembler.settings();
	// Выполняем установку порога накопления записей, дающего множество кадров
	settings.block = 128;
	// Выполняем установку настроек сборки контейнера
	assembler.settings(settings);
	/**
	 * Выполняем внесение тысячи записей в собираемый контейнер
	 */
	for(size_t i = 0; i < 1000; i++){
		// Выполняем сборку очередной записи
		const vector <uint8_t> item = record(string{"запись довольно длинная, чтобы кадров вышло много, номер "} + to_string(i));
		// Выполняем внесение очередной записи в собираемый контейнер
		ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(assembler.error());
	}
	// Источник октетов собранного контейнера
	Source source;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(source.data)) << "код отказа: " << abc::message(assembler.error());
	// Выборщик записей контейнера
	abc::fetcher_t fetcher(this->_log.get());
	// Выполняем открытие контейнера отданной работой чтения
	ASSERT_TRUE(fetcher.open([&source](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем чтение затребованных октетов контейнера
		return source.read(offset, size, result);
	})) << "код отказа: " << abc::message(fetcher.error());
	// Выполняем получение количества октетов, вычитанных открытием контейнера
	const size_t opened = source.taken;
	// Буфер выбранной записи контейнера
	vector <uint8_t> item;
	// Выполняем выборку записи из середины контейнера
	ASSERT_TRUE(fetcher.record(500, item)) << "код отказа: " << abc::message(fetcher.error());
	// Выполняем получение количества октетов, вычитанных выборкой записи
	const size_t fetched = (source.taken - opened);
	/**
	 * Выполняем проверку того, что выборка вычитала долю тела, а не всё тело:
	 * порогом взята десятая доля его, чего кадру с запасом довольно
	 */
	ASSERT_LT(fetched, static_cast <size_t> (fetcher.header().length / 10));
	// Выполняем получение количества обращений к источнику до повторной выборки
	const size_t calls = source.calls;
	// Буфер соседней записи контейнера
	vector <uint8_t> neighbour;
	// Выполняем выборку соседней записи контейнера
	ASSERT_TRUE(fetcher.record(501, neighbour)) << "код отказа: " << abc::message(fetcher.error());
	/**
	 * Выполняем проверку удержания снятого кадра: соседняя запись лежит в том же
	 * кадре, и обращаться к источнику наново за нею не приходится
	 */
	ASSERT_EQ(source.calls, calls);
	// Выполняем проверку того, что соседняя запись выбрана непустой
	ASSERT_FALSE(neighbour.empty());
}
/**
 * @brief Проверка отказа открытия контейнера без оглавления
 *
 */
TEST_F(IndexFixture, MissingIndex) {
	// Сборщик контейнера
	abc::assembler_t assembler(this->_log.get());
	// Получаем настройки сборки контейнера
	abc::assembler_t::settings_t settings = assembler.settings();
	// Выполняем отключение ведения оглавления собираемого контейнера
	settings.indexed = false;
	// Выполняем установку настроек сборки контейнера
	assembler.settings(settings);
	// Выполняем сборку записи
	const vector <uint8_t> item = record("запись без оглавления");
	// Выполняем внесение записи в собираемый контейнер
	ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(assembler.error());
	// Источник октетов собранного контейнера
	Source source;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(source.data)) << "код отказа: " << abc::message(assembler.error());
	// Выполняем проверку того, что оглавление заголовком не объявлено
	ASSERT_EQ(assembler.index().size(), 0ul);
	// Выборщик записей контейнера
	abc::fetcher_t fetcher(this->_log.get());
	// Выполняем проверку отказа открытия контейнера без оглавления
	ASSERT_FALSE(fetcher.open([&source](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем чтение затребованных октетов контейнера
		return source.read(offset, size, result);
	}));
	// Выполняем проверку кода отказа открытия контейнера
	ASSERT_EQ(fetcher.error(), abc::error_t::MISSING_INDEX);
	// Сниматель контейнера
	abc::loader_t loader(this->_log.get());
	// Выполняем подачу собранного контейнера снимателю
	ASSERT_TRUE(loader.feed(source.data.data(), source.data.size()));
	// Содержимое снятого кадра
	vector <uint8_t> payload;
	// Сведения о снятом кадре
	abc::chunk_t chunk;
	/**
	 * Выполняем проверку того, что контейнер без оглавления читается подряд:
	 * оглавление служит выборке по номеру, а не чтению вовсе
	 */
	ASSERT_TRUE(loader.next(payload, chunk)) << "код отказа: " << abc::message(loader.error());
	// Выполняем проверку снятого содержимого
	ASSERT_EQ(payload, item);
}
/**
 * @brief Проверка отказа открытия контейнера при отказе работы чтения
 *
 */
TEST_F(IndexFixture, UnreadableSource) {
	// Выборщик записей контейнера
	abc::fetcher_t fetcher(this->_log.get());
	// Выполняем проверку отказа открытия контейнера отказавшей работой чтения
	ASSERT_FALSE(fetcher.open([](const uint64_t, const size_t, vector <uint8_t> &) noexcept -> bool {
		// Выводим признак отказа чтения октетов контейнера
		return false;
	}));
	// Выполняем проверку кода отказа открытия контейнера
	ASSERT_EQ(fetcher.error(), abc::error_t::UNREADABLE_SOURCE);
	// Выполняем проверку отказа открытия контейнера без работы чтения
	ASSERT_FALSE(fetcher.open(nullptr));
	// Выполняем проверку кода отказа открытия контейнера
	ASSERT_EQ(fetcher.error(), abc::error_t::INTERNAL);
}
/**
 * @brief Проверка отказа выборки по строке оглавления, указывающей за кадр
 *
 * @details Строка оглавления приходит с провода наравне с содержимым и целостностью
 *          не защищена: кадр контрольной суммы не несёт, а подпись необязательна.
 *          Смещение и длина из повреждённой строки увели бы выборку за содержимое
 *          снятого кадра, и читалась бы чужая память
 *
 */
TEST_F(IndexFixture, EntryBeyondChunk) {
	/**
	 * @brief Функция сборки контейнера без сжатия и шифрования
	 *
	 * @details Сжатие здесь выключено нарочно: оглавление обязано лечь в кадр открыто,
	 *          иначе правка строки в записи контейнера невозможна
	 *
	 * @param result буфер, куда следует уложить собранный контейнер
	 *
	 */
	const auto assemble = [this](vector <uint8_t> & result) noexcept -> void {
		// Сборщик контейнера
		abc::assembler_t assembler(this->_log.get());
		/**
		 * Выполняем внесение череды записей в собираемый контейнер
		 */
		for(size_t i = 0; i < 8; i++){
			// Выполняем сборку очередной записи
			const vector <uint8_t> item = record(string{"запись номер "} + to_string(i));
			// Выполняем внесение очередной записи в собираемый контейнер
			ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT));
		}
		// Выполняем завершение сборки контейнера
		ASSERT_TRUE(assembler.complete(result));
	};
	// Октеты собранного контейнера
	vector <uint8_t> pristine;
	// Выполняем сборку контейнера
	assemble(pristine);
	// Выполняем проверку того, что контейнер собран
	ASSERT_FALSE(pristine.empty());
	// Снятый заголовок опознания контейнера
	abc::header_t header;
	// Код отказа снятия заголовка
	abc::error_t error = abc::error_t::NONE;
	// Выполняем снятие заголовка опознания контейнера
	ASSERT_TRUE(header.unpack(pristine.data(), pristine.size(), error))
		<< "код отказа: " << abc::message(error);
	// Выполняем проверку того, что оглавление контейнера объявлено
	ASSERT_GT(header.index, 0u);
	// Выполняем получение смещения первой строки оглавления в записи контейнера
	const size_t entry = static_cast <size_t> (header.index) + abc::CHUNK_HEADER;
	// Выполняем проверку того, что строка оглавления в записи контейнера умещается
	ASSERT_LE(entry + abc::ENTRY_LENGTH, pristine.size());
	/**
	 * Выполняем проверку того, что оглавление легло в кадр открыто.
	 *
	 * Без этого правка строки легла бы в сжатое содержимое, разбор кадра отвечал бы
	 * отказом сжатия, и выборка отказывалась бы вовсе не по той причине
	 */
	ASSERT_EQ(pristine.at(static_cast <size_t> (header.index)), 0x00) << "оглавление уложено сжатым";
	/**
	 * @brief Функция выборки первой записи из поданной записи контейнера
	 *
	 * @param data   октеты записи контейнера
	 * @param result буфер, куда следует положить выбранную запись
	 * @param error  код отказа выборки записи
	 * @return       признак успешно выбранной записи
	 *
	 */
	const auto fetch = [this](const vector <uint8_t> & data, vector <uint8_t> & result, abc::error_t & error) noexcept -> bool {
		// Выборщик записей контейнера
		abc::fetcher_t fetcher(this->_log.get());
		/**
		 * Если открыть контейнер не вышло
		 */
		if(!fetcher.open([&data](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
			// Если затребованные октеты за концом записи контейнера
			if((offset + static_cast <uint64_t> (size)) > static_cast <uint64_t> (data.size()))
				// Выводим признак неудачного чтения октетов
				return false;
			// Выполняем выдачу затребованных октетов
			result.assign(data.begin() + static_cast <ptrdiff_t> (offset),
			 data.begin() + static_cast <ptrdiff_t> (offset + size));
			// Выводим признак успешного чтения октетов
			return true;
		})){
			// Выполняем установку кода отказа открытия контейнера
			error = fetcher.error();
			// Выводим признак неудачной выборки записи
			return false;
		}
		// Выполняем выборку первой записи контейнера
		const bool result2 = fetcher.record(0, result);
		// Выполняем установку кода отказа выборки записи
		error = fetcher.error();
		// Выводим признак успешности выборки записи
		return result2;
	};
	// Буфер выбранной записи контейнера
	vector <uint8_t> item;
	// Выполняем проверку выборки записи из нетронутого контейнера
	ASSERT_TRUE(fetch(pristine, item, error)) << "код отказа: " << abc::message(error);
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(item, record("запись номер 0"));
	// Выполняем получение длины первой записи из строки оглавления
	const uint32_t length = static_cast <uint32_t> (pristine.at(entry + 12)) |
	 (static_cast <uint32_t> (pristine.at(entry + 13)) << 8) |
	 (static_cast <uint32_t> (pristine.at(entry + 14)) << 16) |
	 (static_cast <uint32_t> (pristine.at(entry + 15)) << 24);
	// Выполняем проверку того, что длина записи снята верно
	ASSERT_EQ(length, static_cast <uint32_t> (item.size())) << "строка оглавления найдена не та";
	/**
	 * @brief Функция правки поля строки оглавления
	 *
	 * @param data   октеты правимой записи контейнера
	 * @param shift  смещение правимого поля в строке оглавления
	 * @param value  устанавливаемое значение поля
	 *
	 */
	const auto spike = [entry, &header](vector <uint8_t> & data, const size_t shift, const uint32_t value) noexcept -> void {
		/**
		 * Выполняем перебор всех октетов правимого поля
		 */
		for(size_t i = 0; i < 4; i++)
			// Выполняем укладку очередного октета поля
			data.at(entry + shift + i) = static_cast <uint8_t> ((value >> (i * 8)) & 0xFF);
		/**
		 * Выполняем обновление контрольной суммы кадра оглавления
		 *
		 * @note Правка кадра НА МЕСТЕ обязана обновить сумму его, иначе снятие кадра
		 *       ответит отказом суммы, а проверка эта поверяет не сумму, а строку
		 *       оглавления, указывающую за кадр
		 */
		{
			// Выполняем получение смещения кадра оглавления в записи контейнера
			const size_t place = static_cast <size_t> (header.index);
			// Выполняем получение длины уложенного содержимого кадра оглавления
			const size_t length = static_cast <size_t> (abc::gather(data.data() + place + 4, 4));
			// Выполняем укладку обновлённой контрольной суммы кадра оглавления
			abc::fixed(data.data() + place + abc::CHUNK_DIGEST,
			 abc::digest(data.data() + place, abc::CHUNK_HEADER + length), 8);
		}
	};
	/**
	 * Выполняем проверку отказа выборки по непомерной длине записи
	 */
	{
		// Октеты правимой записи контейнера
		vector <uint8_t> damaged = pristine;
		// Выполняем установку непомерной длины записи
		spike(damaged, 12, numeric_limits <uint32_t>::max());
		// Буфер выбранной записи контейнера
		vector <uint8_t> item;
		// Выполняем проверку отказа выборки записи
		ASSERT_FALSE(fetch(damaged, item, error));
		// Выполняем проверку кода отказа выборки записи
		ASSERT_EQ(error, abc::error_t::INVALID_INDEX);
		// Выполняем проверку того, что выбранного наружу не ушло
		ASSERT_TRUE(item.empty());
	}
	/**
	 * Выполняем проверку отказа выборки по непомерному смещению записи
	 */
	{
		// Октеты правимой записи контейнера
		vector <uint8_t> damaged = pristine;
		// Выполняем установку непомерного смещения записи
		spike(damaged, 8, numeric_limits <uint32_t>::max());
		// Буфер выбранной записи контейнера
		vector <uint8_t> item;
		// Выполняем проверку отказа выборки записи
		ASSERT_FALSE(fetch(damaged, item, error));
		// Выполняем проверку кода отказа выборки записи
		ASSERT_EQ(error, abc::error_t::INVALID_INDEX);
		// Выполняем проверку того, что выбранного наружу не ушло
		ASSERT_TRUE(item.empty());
	}
	/**
	 * Выполняем проверку отказа выборки по длине, вышедшей за кадр на один октет.
	 *
	 * Правка эта отделяет сличение по краю от сличения приблизительного: строка,
	 * вышедшая за содержимое кадра на единый октет, годной не является
	 */
	{
		// Октеты правимой записи контейнера
		vector <uint8_t> damaged = pristine;
		// Выполняем установку длины записи, вышедшей за кадр на один октет
		spike(damaged, 12, length + 1);
		// Буфер выбранной записи контейнера
		vector <uint8_t> item;
		// Выполняем выборку первой записи контейнера
		const bool fetched = fetch(damaged, item, error);
		/**
		 * Если запись выбрана, кадр несёт октеты за нею: отказ здесь необязателен,
		 * а вот выход за содержимое кадра - невозможен
		 */
		if(fetched)
			// Выполняем проверку длины выбранной записи
			ASSERT_EQ(item.size(), static_cast <size_t> (length + 1));
		// Иначе сличаем код отказа выборки записи
		else ASSERT_EQ(error, abc::error_t::INVALID_INDEX);
	}
}
/**
 * @brief Проверка того, что объявленная длина кадра места не заводит
 *
 * @details Длина уложенного содержимого кадра прочитана из САМОГО контейнера и
 *          недоверенна. Порча четырёх октетов её на `0xFFFFFFFF` заставляла выборку
 *          требовать у источника 4 294 967 319 октетов - и заводить их - лишь затем,
 *          чтобы следом ответить отказом по недочитанному. Тело же ограничено
 *          заголовком, и кадр, за него выходящий, негоден заведомо
 *
 * @note Источник здесь заводит место ПО ЗАТРЕБОВАННОМУ, а границы блюдёт после:
 *       так ведёт себя всякий наивный источник, и сторож обязан стоять выше него
 *
 */
TEST_F(IndexFixture, ChunkLengthAllocation) {
	// Октеты собранного контейнера
	vector <uint8_t> data;
	{
		// Сборщик контейнера
		abc::assembler_t assembler(this->_log.get());
		// Получаем настройки сборки контейнера
		abc::assembler_t::settings_t settings = assembler.settings();
		// Выполняем установку порога накопления, дающего кадр на всякую запись
		settings.block = 1;
		// Выполняем установку настроек сборки контейнера
		assembler.settings(settings);
		/**
		 * Выполняем внесение череды записей в собираемый контейнер
		 */
		for(size_t i = 0; i < 4; i++){
			// Выполняем сборку очередной записи
			const vector <uint8_t> item = record(string{"запись номер "} + to_string(i));
			// Выполняем внесение очередной записи в собираемый контейнер
			ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
				<< "код отказа: " << abc::message(assembler.error());
		}
		// Выполняем завершение сборки контейнера
		ASSERT_TRUE(assembler.complete(data)) << "код отказа: " << abc::message(assembler.error());
	}
	// Наибольшее место, заведённое источником за прогон
	size_t peak = 0;
	/**
	 * Источник октетов контейнера, заводящий место по затребованному
	 *
	 * @param offset смещение читаемых октетов
	 * @param size   размер читаемых октетов
	 * @param result буфер, куда следует положить прочитанное
	 * @return       признак успешности чтения
	 */
	const auto source = [&data, &peak](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем очистку буфера прочитанных октетов
		result.clear();
		// Выполняем учёт наибольшего затребованного размера чтения
		peak = ((size > peak) ? size : peak);
		// Если затребованное чтение выходит за пределы записи контейнера
		if((offset + static_cast <uint64_t> (size)) > static_cast <uint64_t> (data.size()))
			// Выводим признак неудачного чтения
			return false;
		// Выполняем выдачу затребованных октетов записи контейнера
		result.assign(data.begin() + static_cast <ptrdiff_t> (offset),
		 data.begin() + static_cast <ptrdiff_t> (offset) + static_cast <ptrdiff_t> (size));
		// Выводим признак успешного чтения
		return true;
	};
	// Смещение объявленной длины кадра второй записи контейнера
	uint64_t place = 0;
	{
		// Выборщик записей контейнера
		abc::fetcher_t fetcher(this->_log.get());
		// Выполняем открытие годного контейнера
		ASSERT_TRUE(fetcher.open(source)) << "код отказа: " << abc::message(fetcher.error());
		// Буфер выбранной записи контейнера
		vector <uint8_t> item;
		// Выполняем проверку того, что запись годного контейнера выбирается
		ASSERT_TRUE(fetcher.record(2, item)) << "код отказа: " << abc::message(fetcher.error());
		// Выполняем получение смещения объявленной длины кадра второй записи
		place = abc::HEADER_LENGTH + fetcher.index().entries().at(2).chunk + 4;
	}
	// Выполняем проверку того, что длина кадра в запись контейнера умещается
	ASSERT_LE(place + 4, static_cast <uint64_t> (data.size()));
	/**
	 * Выполняем порчу объявленной длины кадра второй записи наибольшим числом
	 */
	for(size_t i = 0; i < 4; i++)
		// Выполняем порчу очередного октета объявленной длины кадра
		data.at(static_cast <size_t> (place) + i) = 0xFF;
	// Выполняем сброс наибольшего заведённого места
	peak = 0;
	// Выборщик записей испорченного контейнера
	abc::fetcher_t fetcher(this->_log.get());
	// Выполняем открытие испорченного контейнера
	ASSERT_TRUE(fetcher.open(source)) << "код отказа: " << abc::message(fetcher.error());
	// Буфер выбранной записи контейнера
	vector <uint8_t> item;
	// Выполняем проверку того, что испорченная запись отвечена отказом
	ASSERT_FALSE(fetcher.record(2, item));
	// Выполняем проверку того, что отказ объявлен неопознанным кадром
	ASSERT_EQ(fetcher.error(), abc::error_t::INVALID_CHUNK);
	/**
	 * Выполняем проверку того, что места по объявленной длине заведено не было.
	 *
	 * Порог взят с запасом: кадр годной записи невелик, а порча требовала 4 ГиБ,
	 * и промах меж ними ни с чем не спутать
	 */
	ASSERT_LT(peak, static_cast <size_t> (64 * 1024)) << "заведено место по объявленной длине: " << peak;
	// Выполняем проверку того, что соседняя запись контейнера по-прежнему выбирается
	ASSERT_TRUE(fetcher.record(3, item)) << "код отказа: " << abc::message(fetcher.error());
}
