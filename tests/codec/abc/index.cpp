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
	 * @brief Класс опоры проверок оглавления контейнера
	 *
	 */
	class IndexFixture : public testing::Test {
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
	abc::index_t index;
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
	abc::index_t taken;
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
	abc::index_t index;
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
		abc::index_t source;
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
	abc::assembler_t assembler;
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
	abc::fetcher_t fetcher;
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
	abc::assembler_t assembler;
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
	abc::fetcher_t fetcher;
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
	abc::assembler_t assembler;
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
	abc::fetcher_t fetcher;
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
	abc::assembler_t assembler;
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
	abc::fetcher_t fetcher;
	// Выполняем проверку отказа открытия контейнера без оглавления
	ASSERT_FALSE(fetcher.open([&source](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем чтение затребованных октетов контейнера
		return source.read(offset, size, result);
	}));
	// Выполняем проверку кода отказа открытия контейнера
	ASSERT_EQ(fetcher.error(), abc::error_t::MISSING_INDEX);
	// Сниматель контейнера
	abc::loader_t loader;
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
	abc::fetcher_t fetcher;
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
	const auto assemble = [](vector <uint8_t> & result) noexcept -> void {
		// Сборщик контейнера
		abc::assembler_t assembler;
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
	const auto fetch = [](const vector <uint8_t> & data, vector <uint8_t> & result, abc::error_t & error) noexcept -> bool {
		// Выборщик записей контейнера
		abc::fetcher_t fetcher;
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
		abc::assembler_t assembler;
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
		abc::fetcher_t fetcher;
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
	abc::fetcher_t fetcher;
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
/**
 * @brief Проверка отказа снятия строки с занятыми хвостовыми октетами
 *
 * @details Строка оглавления объявлена длиною 24 октета, а занято ею ДВАДЦАТЬ: четыре
 *          хвостовых оставлены впрок. Укладка кладёт их нулями, и снятие нулевыми их
 *          требует - равняясь на сличение разрядов свойств, стоящее рядом: оба поля
 *          приходят с провода, и оба обязаны быть опознаны
 *
 * @note Без этого требования занятые кем-то октеты проходили бы молча, а строка
 *       числилась бы понятой целиком. Проверка перебирает ВСЕ тридцать два разряда
 *       поодиночке: требование нулей обязано держать всякий, а не один лишь младший
 *
 */
TEST_F(IndexFixture, ReservedOctets) {
	// Оглавление контейнера
	abc::index_t index;
	// Заводимая строка оглавления
	abc::entry_t entry;
	// Выполняем установку смещения кадра строки
	entry.chunk = 64;
	// Выполняем установку смещения записи в кадре
	entry.offset = 8;
	// Выполняем установку длины записи
	entry.length = 16;
	// Выполняем внесение строки в оглавление
	index.add(entry);
	// Октеты уложенного оглавления
	vector <uint8_t> packed;
	// Выполняем укладку оглавления в октеты
	index.pack(packed);
	// Выполняем проверку длины уложенной строки оглавления
	ASSERT_EQ(packed.size(), abc::ENTRY_LENGTH);
	/**
	 * Выполняем проверку того, что хвостовые октеты уложены нулями
	 */
	for(size_t i = 20; i < abc::ENTRY_LENGTH; i++)
		// Выполняем проверку нулевого хвостового октета
		ASSERT_EQ(packed.at(i), 0u) << "хвостовой октет: " << i;
	// Код отказа снятия оглавления
	abc::error_t error = abc::error_t::NONE;
	// Выполняем проверку того, что целая строка снимается
	ASSERT_TRUE(index.unpack(packed.data(), packed.size(), error))
		<< "код отказа: " << abc::message(error);
	/**
	 * Выполняем перебор всех разрядов хвостовых октетов строки
	 */
	for(uint8_t bit = 0; bit < 32; bit++){
		// Порченые октеты уложенного оглавления
		vector <uint8_t> damaged = packed;
		// Выполняем занятие очередного разряда хвостовых октетов
		damaged.at(20 + (bit / 8)) |= static_cast <uint8_t> (1u << (bit % 8));
		// Выполняем проверку отказа снятия строки с занятыми хвостовыми октетами
		ASSERT_FALSE(index.unpack(damaged.data(), damaged.size(), error))
			<< "хвостовой разряд: " << static_cast <uint16_t> (bit);
		// Выполняем проверку кода отказа снятия оглавления
		ASSERT_EQ(error, abc::error_t::INVALID_INDEX) << "хвостовой разряд: " << static_cast <uint16_t> (bit);
		// Выполняем проверку того, что повреждённое оглавление снятым не осталось
		ASSERT_EQ(index.size(), 0ul) << "хвостовой разряд: " << static_cast <uint16_t> (bit);
	}
}

/**
 * @brief Проверка усечения оглавления, откату служащего
 *
 * @details Работа эта служит ОТКАТУ: фиксация вносит строки по ходу записи кадров, а
 *          отказ посреди неё обязан вернуть оглавление к тому, чем оно было. Без отката
 *          повторная фиксация вносила строки заново, и записи двоились
 *
 * @note Проверка заведена 04.09.2026 сличением состава: ход этот - один из двух
 *       ПУБЛИЧНЫХ ходов кодека, не поминаемых ни единой проверкой. Довод его подробен и
 *       верен, а стерегло его ничто, - и порча его сказалась бы не здесь, а вдали:
 *       двоящимися записями после отказа фиксации
 *
 * @warning Сличение состава ходов дважды солгало по дороге, и оба раза - шаблоном
 * поиска. Первый шаблон брал лишь вызовы вида `.ход(` и объявил незвучащими `verify`,
 * `probe` и `compose`, зовомые тридцатью одним, двенадцатью и десятью местами. Второй
 * счёл публичными `clone` и ещё пятерых, стоящих в закрытой части. Приём отвечает «не
 * поминается МОИМ шаблоном», и всякий его список надлежит переспросить глазами
 *
 */
TEST_F(IndexFixture, TruncationServesTheRollback) {
	// Собираемое оглавление контейнера
	abc::index_t index;
	/**
	 * Выполняем внесение трёх строк в собираемое оглавление
	 */
	for(uint64_t i = 0; i < 3; i++){
		// Собираемая строка оглавления
		abc::entry_t entry;
		// Выполняем установку смещения кадра от начала тела контейнера
		entry.chunk = (0x100ull + i);
		// Выполняем установку смещения записи в содержимом кадра
		entry.offset = static_cast <uint32_t> (0x10u + i);
		// Выполняем установку длины записи
		entry.length = static_cast <uint32_t> (0x20u + i);
		// Выполняем внесение строки в собираемое оглавление
		index.add(entry);
	}
	// Выполняем проверку количества внесённых строк оглавления
	ASSERT_EQ(index.size(), 3ul);
	/**
	 * Выполняем усечение оглавления до одной строки
	 */
	index.truncate(1);
	// Выполняем проверку количества оставшихся строк оглавления
	ASSERT_EQ(index.size(), 1ul);
	/**
	 * Выполняем проверку того, что уцелела строка ПЕРВАЯ, а не последняя
	 *
	 * @note Половина эта и делает проверку зрячей: усечение с иного конца оставило бы
	 *       то же ЧИСЛО строк, и проверка по одному лишь счёту была бы зелена. Откат же
	 *       обязан вернуть оглавление к тому, чем оно было ДО фиксации, то есть уцелеть
	 *       должно начало
	 */
	ASSERT_EQ(index.entries().front().chunk, 0x100ull);
	// Выполняем проверку смещения записи уцелевшей строки
	ASSERT_EQ(index.entries().front().offset, 0x10u);
	/**
	 * Выполняем проверку того, что количество, большее нынешнего, оглавления не трогает
	 *
	 * @note Правило это записано доводом при работе, а стерегомо не было: усечение,
	 *       заводящее строки из ничего либо очищающее оглавление при избыточном
	 *       количестве, прошло бы незамеченным
	 */
	index.truncate(16);
	// Выполняем проверку неизменности количества строк оглавления
	ASSERT_EQ(index.size(), 1ul);
	// Выполняем проверку неизменности уцелевшей строки оглавления
	ASSERT_EQ(index.entries().front().chunk, 0x100ull);
	// Выполняем проверку того, что усечение до нуля оглавление опустошает
	index.truncate(0);
	// Выполняем проверку опустошённости оглавления
	ASSERT_EQ(index.size(), 0ul);
}

/**
 * @brief Проверка отказа снятия оглавления по буферу, поданному пустым указателем
 *
 * @details Октеты, объявленные длиною при пустом указателе, суть ошибка ЗОВУЩЕГО, а не
 *          порча оглавления: отвечать на неё кодом порчи значило бы указать потребителю
 *          не туда. Оттого причина здесь внутренняя
 *
 * @note Заведена находкой 05.09.2026, добытой щупом НУЖНОСТИ (заслон снимается, гонится
 *       весь набор): щуп ПУТИ показал, что дорогу эту проходят 47 проверок, а щуп
 *       нужности - что заслон не ловит НИ ОДНОЙ. То есть место хожено, а порчу его не
 *       видел никто. Различение двух щупов указано разбором сетевых движков в тот же
 *       день: молчание у них значит противоположное
 *
 */
TEST_F(IndexFixture, UnpackRefusesTheAbsentBuffer) {
	// Оглавление контейнера
	abc::index_t index;
	// Код отказа снятия оглавления
	abc::error_t error = abc::error_t::NONE;
	// Выполняем проверку отказа снятия оглавления по пустому указателю при длине
	ASSERT_FALSE(index.unpack(nullptr, 32, error));
	// Выполняем проверку названной причины отказа
	ASSERT_EQ(error, abc::error_t::INTERNAL) << abc::message(error);
	/**
	 * Пустой указатель при НУЛЕВОЙ длине ошибкою не является: оглавления просто нет, и
	 * это законный исход
	 */
	ASSERT_TRUE(index.unpack(nullptr, 0, error)) << abc::message(error);
	// Выполняем проверку отсутствия отказа при нулевой длине
	ASSERT_EQ(error, abc::error_t::NONE) << abc::message(error);
	// Выполняем проверку пустоты снятого оглавления
	ASSERT_EQ(index.size(), 0ul);
}
/**
 * @brief Проверка того, что длина кадра оглавления поверяется заголовком
 *
 * @details Длина уложенного содержимого объявлена ДВАЖДЫ: заголовком контейнера и самим
 *          кадром оглавления. Заголовок несёт контрольную сумму, кадр её не несёт, и
 *          доверять надлежит заголовку. Порча четырёх октетов длины ВНУТРИ кадра
 *          контрольной суммы заголовка не задевает вовсе, и без сличения проходила бы
 *          незамеченной
 *
 * @note Заслон этот щуп пути числил слепым: подделка внутри кадра оглавления ни одной
 *       проверке набора не встречалась. Место опознано 05.09.2026 и закрыто здесь
 */
TEST_F(IndexFixture, DeclaredIndexLengthIsCheckedAgainstTheHeader) {
	// Октеты собранного контейнера
	vector <uint8_t> data;
	{
		// Сборщик контейнера
		abc::assembler_t assembler;
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
	/**
	 * Источник октетов контейнера
	 *
	 * @param offset смещение читаемых октетов
	 * @param size   размер читаемых октетов
	 * @param result буфер, куда следует положить прочитанное
	 * @return       признак успешности чтения
	 */
	const auto source = [&data](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем очистку буфера прочитанных октетов
		result.clear();
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
	// Смещение объявленной длины содержимого внутри кадра оглавления
	uint64_t place = 0;
	{
		// Выборщик записей контейнера
		abc::fetcher_t fetcher;
		// Выполняем проверку того, что годный контейнер открывается
		ASSERT_TRUE(fetcher.open(source)) << "код отказа: " << abc::message(fetcher.error());
		// Выполняем проверку того, что оглавление у контейнера объявлено
		ASSERT_GT(fetcher.header().index, static_cast <uint64_t> (0));
		// Выполняем получение смещения объявленной длины содержимого кадра оглавления
		place = fetcher.header().index + 4;
	}
	// Выполняем проверку того, что объявленная длина в запись контейнера умещается
	ASSERT_LE(place + 4, static_cast <uint64_t> (data.size()));
	/**
	 * Выполняем порчу объявленной длины содержимого кадра оглавления
	 */
	for(size_t i = 0; i < 4; i++)
		// Выполняем порчу очередного октета объявленной длины содержимого
		data.at(static_cast <size_t> (place) + i) = 0xFF;
	{
		// Выборщик записей испорченного контейнера
		abc::fetcher_t fetcher;
		// Выполняем проверку того, что испорченный контейнер отвечен отказом
		ASSERT_FALSE(fetcher.open(source));
		// Выполняем проверку того, что отказ объявлен неопознанным кадром
		ASSERT_EQ(fetcher.error(), abc::error_t::INVALID_CHUNK);
	}
	/**
	 * Выполняем проверку того, что заслон стережёт именно подделку: возвращённая
	 * длина открывает контейнер снова, и отказ выше принадлежал порче, а не устройству
	 * самой проверки
	 */
	{
		// Выборщик записей возвращённого контейнера
		abc::fetcher_t fetcher;
		// Буфер выбранной записи контейнера
		vector <uint8_t> item;
		// Выполняем восстановление испорченной длины сборкой контейнера заново
		vector <uint8_t> again;
		{
			// Сборщик контейнера
			abc::assembler_t assembler;
			/**
			 * Выполняем внесение той же череды записей в собираемый контейнер
			 */
			for(size_t i = 0; i < 4; i++){
				// Выполняем сборку очередной записи
				const vector <uint8_t> entry = record(string{"запись номер "} + to_string(i));
				// Выполняем внесение очередной записи в собираемый контейнер
				ASSERT_TRUE(assembler.append(entry.data(), entry.size(), abc::payload_t::TEXT))
					<< "код отказа: " << abc::message(assembler.error());
			}
			// Выполняем завершение сборки контейнера
			ASSERT_TRUE(assembler.complete(again)) << "код отказа: " << abc::message(assembler.error());
		}
		// Выполняем подмену октетов контейнера на годные
		data = again;
		// Выполняем проверку того, что годный контейнер открывается
		ASSERT_TRUE(fetcher.open(source)) << "код отказа: " << abc::message(fetcher.error());
		// Выполняем проверку того, что запись годного контейнера выбирается
		ASSERT_TRUE(fetcher.record(2, item)) << "код отказа: " << abc::message(fetcher.error());
	}
}
/**
 * @brief Проверка того, что отказ снятия кадра назван причиною укладчика
 *
 * @details Оснастка выборщику не принадлежит: разжиматель отдаёт зовущий, а метод сжатия
 *          кадр несёт в себе. Оттого сжатый контейнер может попасть выборщику, сжатия не
 *          знающему вовсе, и снятие кадра отвечается отказом. Причина здесь не заводится
 *          своя, а ПЕРЕНОСИТСЯ от укладчика: слою оглавления знать нечего о разжимателе
 *
 * @note Устьев два, и достижимы они ПОРОЗНЬ - разнятся числом записей: оглавление о
 *       четырёх записях сжимается и валит само ОТКРЫТИЕ, а оглавление об одной коротко,
 *       сжатия не берёт, открытие проходит, и отказ настигает ВЫБОРКУ. Замерено щупом
 *       05.09.2026: до него оба устья числились незакреплёнными
 */
TEST_F(IndexFixture, UnpackFailureCarriesThePackerCause) {
	/**
	 * Собиратель сжатого контейнера о заданном числе записей
	 *
	 * @param count число собираемых записей контейнера
	 * @param data  буфер собираемых октетов контейнера
	 */
	const auto build = [this](const size_t count, vector <uint8_t> & data) noexcept -> void {
		// Сборщик контейнера
		abc::assembler_t assembler;
		// Выполняем установку модуля сжатия
		assembler.compressor(this->_compressor.get());
		/**
		 * Выполняем внесение череды хорошо сжимаемых записей
		 */
		for(size_t i = 0; i < count; i++){
			// Выполняем сборку очередной записи
			const vector <uint8_t> item = record(string(4096, 'a'));
			// Выполняем внесение очередной записи в собираемый контейнер
			ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
				<< "код отказа: " << abc::message(assembler.error());
		}
		// Выполняем завершение сборки контейнера
		ASSERT_TRUE(assembler.complete(data)) << "код отказа: " << abc::message(assembler.error());
	};
	/**
	 * Устье первое: сжатое ОГЛАВЛЕНИЕ валит открытие контейнера
	 */
	{
		// Октеты собранного контейнера
		vector <uint8_t> data;
		// Выполняем сборку контейнера о четырёх записях
		build(4, data);
		/**
		 * Источник октетов контейнера
		 *
		 * @param offset смещение читаемых октетов
		 * @param size   размер читаемых октетов
		 * @param result буфер, куда следует положить прочитанное
		 * @return       признак успешности чтения
		 */
		const auto source = [&data](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
			// Выполняем очистку буфера прочитанных октетов
			result.clear();
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
		{
			// Выборщик записей, сжатием НЕ оснащённый
			abc::fetcher_t fetcher;
			// Выполняем проверку того, что открытие сжатого контейнера отвечено отказом
			ASSERT_FALSE(fetcher.open(source));
			// Выполняем проверку того, что причина укладчика перенесена выборщику
			ASSERT_EQ(fetcher.error(), abc::error_t::COMPRESSION_FAILED);
		}
		{
			// Выборщик записей, сжатием оснащённый
			abc::fetcher_t fetcher;
			// Выполняем установку модуля сжатия
			fetcher.compressor(this->_compressor.get());
			/**
			 * Выполняем проверку того, что отказ принадлежал ОСНАСТКЕ: тот же контейнер
			 * выборщиком оснащённым открывается и записи его выбираются
			 */
			ASSERT_TRUE(fetcher.open(source)) << "код отказа: " << abc::message(fetcher.error());
			// Буфер выбранной записи контейнера
			vector <uint8_t> item;
			// Выполняем проверку того, что запись контейнера выбирается
			ASSERT_TRUE(fetcher.record(2, item)) << "код отказа: " << abc::message(fetcher.error());
		}
	}
	/**
	 * Устье второе: оглавление об одной записи сжатия не берёт, открытие проходит,
	 * и отказ настигает выборку записи
	 */
	{
		// Октеты собранного контейнера
		vector <uint8_t> data;
		// Выполняем сборку контейнера об одной записи
		build(1, data);
		/**
		 * Источник октетов контейнера
		 *
		 * @param offset смещение читаемых октетов
		 * @param size   размер читаемых октетов
		 * @param result буфер, куда следует положить прочитанное
		 * @return       признак успешности чтения
		 */
		const auto source = [&data](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
			// Выполняем очистку буфера прочитанных октетов
			result.clear();
			// Если затребованное чтение выходит за пределы записи контейнера
			if((offset + static_cast <uint64_t> (size)) > static_cast <uint64_t> (data.size()))
				// Выводим признак неудачного чтения
				return false;
			// Выполняем выдачу затребованных октетов записи контейнера
			result.assign(data.begin() + static_cast <ptrdiff_t> (offset),
			 data.begin() + static_cast <ptrdiff_t> (offset) + static_cast <ptrdiff_t> (size));
			// Выполняем выдачу признака успешного чтения
			return true;
		};
		// Выборщик записей, сжатием НЕ оснащённый
		abc::fetcher_t fetcher;
		// Выполняем проверку того, что открытие контейнера проходит
		ASSERT_TRUE(fetcher.open(source)) << "код отказа: " << abc::message(fetcher.error());
		// Буфер выбранной записи контейнера
		vector <uint8_t> item;
		// Выполняем проверку того, что выборка сжатой записи отвечена отказом
		ASSERT_FALSE(fetcher.record(0, item));
		// Выполняем проверку того, что причина укладчика перенесена выборщику
		ASSERT_EQ(fetcher.error(), abc::error_t::COMPRESSION_FAILED);
		// Выполняем установку модуля сжатия выборщику
		fetcher.compressor(this->_compressor.get());
		/**
		 * Выполняем проверку того, что возвращённая оснастка работу возвращает: отказ
		 * принадлежал ей, а не порче контейнера
		 */
		ASSERT_TRUE(fetcher.record(0, item)) << "код отказа: " << abc::message(fetcher.error());
	}
}
/**
 * @brief Проверка того, что обрыв чтения назван причиною своего рода
 *
 * @details Выборщик читает носитель на четырёх местах: заголовок кадра оглавления, кадр
 *          оглавления целиком, заголовок кадра записи и кадр записи целиком. Всякий обрыв
 *          обязан объявляться `UNREADABLE_SOURCE`, а не причиною соседнего слоя
 *
 * @note Прежние проверки выборщика утверждали, что отказ СЛУЧИЛСЯ, а не какою причиною
 *       назван, - и четыре места чтения стояли слепыми по роду причины до 05.09.2026.
 *       Обход ведётся по числу удавшихся чтений: предел, до одного места не дошедший,
 *       настигает другое
 *
 * @note Проверяется и обратное: при снятом пределе выборка проходит. Без того обход
 *       прошёл бы и на выборщике, отказывающем всегда
 */
TEST_F(IndexFixture, ReadRefusalNamesItsKind) {
	// Октеты собранного контейнера
	vector <uint8_t> data;
	{
		// Сборщик контейнера
		abc::assembler_t assembler;
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
	// Количество застигнутых обрывов чтения
	size_t refused = 0;
	/**
	 * Выполняем обход всех мест обрыва чтения носителя
	 */
	for(int sight = 0; sight < 24; sight++){
		// Количество удавшихся чтений круга
		int reads = 0;
		/**
		 * Источник октетов контейнера, отказывающий по исчерпании предела
		 *
		 * @param offset смещение читаемых октетов
		 * @param size   размер читаемых октетов
		 * @param result буфер, куда следует положить прочитанное
		 * @return       признак успешности чтения
		 */
		const auto source = [&data, &reads, sight](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
			// Выполняем очистку буфера прочитанных октетов
			result.clear();
			// Если предел удавшихся чтений исчерпан
			if(reads >= sight)
				// Выводим признак неудачного чтения
				return false;
			// Выполняем учёт удавшегося чтения
			reads++;
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
		// Выборщик записей контейнера круга
		abc::fetcher_t fetcher;
		// Признак успешно открытого контейнера
		const bool opened = fetcher.open(source);
		// Буфер выбранной записи контейнера
		vector <uint8_t> item;
		// Признак успешно выбранных записей контейнера
		const bool taken = (opened && fetcher.record(0, item) && fetcher.record(3, item));
		/**
		 * Если работа отвечена отказом, причина обязана быть обрывом чтения
		 */
		if(!taken){
			// Выполняем учёт застигнутого обрыва чтения
			refused++;
			// Выполняем проверку того, что причина названа обрывом чтения носителя
			ASSERT_EQ(fetcher.error(), abc::error_t::UNREADABLE_SOURCE)
				<< "предел чтений: " << sight << ", причина: " << abc::message(fetcher.error());
		}
	}
	// Выполняем проверку того, что обрывы чтения обходом застигнуты
	ASSERT_GT(refused, static_cast <size_t> (0));
	/**
	 * Выполняем проверку того, что при СНЯТОМ пределе выборка проходит
	 */
	{
		/**
		 * Источник октетов контейнера без предела чтений
		 *
		 * @param offset смещение читаемых октетов
		 * @param size   размер читаемых октетов
		 * @param result буфер, куда следует положить прочитанное
		 * @return       признак успешности чтения
		 */
		const auto source = [&data](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
			// Выполняем очистку буфера прочитанных октетов
			result.clear();
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
		// Выборщик записей контейнера
		abc::fetcher_t fetcher;
		// Выполняем проверку того, что контейнер открывается
		ASSERT_TRUE(fetcher.open(source)) << "код отказа: " << abc::message(fetcher.error());
		// Буфер выбранной записи контейнера
		vector <uint8_t> item;
		// Выполняем проверку того, что первая запись контейнера выбирается
		ASSERT_TRUE(fetcher.record(0, item)) << "код отказа: " << abc::message(fetcher.error());
		// Выполняем проверку того, что последняя запись контейнера выбирается
		ASSERT_TRUE(fetcher.record(3, item)) << "код отказа: " << abc::message(fetcher.error());
	}
}
/**
 * @brief Проверка того, что выборщик отвергает работу до открытия и подделку смещения
 *
 * @details Выборка у выборщика, контейнера не открывшего, отвечается отказом внутренним:
 *          оглавления у него ещё нет, и брать запись неоткуда. Смещение же кадра приходит
 *          ИЗ ОГЛАВЛЕНИЯ и недоверенно: сложение его с длиной заголовка при смещении близ
 *          предела разрядной сетки завернулось бы, подав источнику малое смещение
 *
 * @note Оба места стояли слепыми до 05.09.2026. Первое - потому что до открытия выборщика
 *       не тревожил никто, второе - потому что подделку строки оглавления у выборщика не
 *       наводили. Близнец второго у правщика:
 *       `EditorFixture.ChunkOffsetBeyondTheBodyIsRefused`
 */
TEST_F(IndexFixture, RefusalsOfTheFetcher) {
	/**
	 * Устье первое: выборка до открытия контейнера
	 */
	{
		// Выборщик записей контейнера
		abc::fetcher_t fetcher;
		// Буфер выбранной записи контейнера
		vector <uint8_t> item;
		// Выполняем проверку того, что выборка до открытия отвечена отказом
		ASSERT_FALSE(fetcher.record(0, item));
		// Выполняем проверку того, что отказ объявлен внутренним
		ASSERT_EQ(fetcher.error(), abc::error_t::INTERNAL) << abc::message(fetcher.error());
	}
	// Октеты собранного контейнера
	vector <uint8_t> data;
	{
		// Сборщик контейнера
		abc::assembler_t assembler;
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
	/**
	 * Источник октетов контейнера
	 *
	 * @param offset смещение читаемых октетов
	 * @param size   размер читаемых октетов
	 * @param result буфер, куда следует положить прочитанное
	 * @return       признак успешности чтения
	 */
	const auto source = [&data](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем очистку буфера прочитанных октетов
		result.clear();
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
	// Снятый заголовок опознания контейнера
	abc::header_t header;
	// Код отказа снятия заголовка
	abc::error_t error = abc::error_t::NONE;
	// Выполняем снятие заголовка опознания контейнера
	ASSERT_TRUE(header.unpack(data.data(), data.size(), error)) << abc::message(error);
	// Выполняем проверку того, что оглавление контейнера объявлено
	ASSERT_GT(header.index, 0u);
	/**
	 * Выполняем проверку того, что оглавление легло в кадр открыто: сжатое содержимое
	 * правке строки не поддаётся
	 */
	ASSERT_EQ(data.at(static_cast <size_t> (header.index)), 0x00) << "оглавление уложено сжатым";
	// Выполняем получение смещения первой строки оглавления в записи контейнера
	const size_t entry = static_cast <size_t> (header.index) + abc::CHUNK_HEADER;
	// Выполняем проверку того, что строка оглавления в записи контейнера умещается
	ASSERT_LE(entry + abc::ENTRY_LENGTH, data.size());
	/**
	 * Выполняем подделку смещения кадра первой записи непомерным числом: смещение занимает
	 * первые восемь октетов строки оглавления
	 */
	for(size_t i = 0; i < 8; i++)
		// Выполняем укладку очередного октета поддельного смещения кадра
		data.at(entry + i) = 0xFF;
	/**
	 * Выполняем обновление контрольной суммы кадра оглавления: иначе отказ придёт по сумме
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
	// Выборщик записей поддельного контейнера
	abc::fetcher_t fetcher;
	// Выполняем открытие поддельного контейнера
	ASSERT_TRUE(fetcher.open(source)) << "код отказа: " << abc::message(fetcher.error());
	// Буфер выбранной записи контейнера
	vector <uint8_t> item;
	// Выполняем проверку того, что выборка поддельной записи отвечена отказом
	ASSERT_FALSE(fetcher.record(0, item));
	// Выполняем проверку того, что отказ объявлен повреждённой записью оглавления
	ASSERT_EQ(fetcher.error(), abc::error_t::INVALID_CHUNK) << abc::message(fetcher.error());
	/**
	 * Выполняем проверку того, что соседняя запись по-прежнему выбирается: отказ
	 * принадлежал ПОДДЕЛАННОЙ строке, а не устройству выборщика
	 */
	ASSERT_TRUE(fetcher.record(1, item)) << "код отказа: " << abc::message(fetcher.error());
}
/**
 * @brief Проверка того, что правка строки за пределом оглавления отвергается
 *
 * @details Заслон этот - ЕДИНСТВЕННЫЙ отказ работы правки, и на нём стоит довод, каким
 *          девятнадцать мест правщика и сборщика объявлены последними руками. Довод
 *          опирался на чтение кода, а сам заслон не звался никем: щуп нужности 07.09.2026
 *          обесточил его, и не покраснело ничего. Опора довода закрепляется здесь
 */
TEST_F(IndexFixture, ReplaceRefusesTheNumberBeyondTheIndex) {
	// Оглавление контейнера
	abc::index_t index;
	// Заводимая строка оглавления
	abc::entry_t entry;
	// Выполняем установку смещения кадра строки оглавления
	entry.chunk = 0;
	// Выполняем установку смещения записи в содержимом кадра
	entry.offset = 0;
	// Выполняем установку длины записи
	entry.length = 16;
	/**
	 * Выполняем проверку того, что правка ПУСТОГО оглавления отвергается: строки с
	 * номером ноль в нём нет
	 */
	ASSERT_FALSE(index.replace(0, entry)) << "правка пустого оглавления принята";
	// Выполняем внесение строки в оглавление
	index.add(entry);
	// Выполняем проверку того, что строк в оглавлении ровно одна
	ASSERT_EQ(index.entries().size(), 1ul);
	// Правимая строка оглавления
	abc::entry_t other = entry;
	// Выполняем установку иной длины записи
	other.length = 32;
	/**
	 * Выполняем проверку того, что правка строки ЗА пределом оглавления отвергается:
	 * строка первая есть, а второй нет
	 */
	ASSERT_FALSE(index.replace(1, other)) << "правка строки за пределом оглавления принята";
	/**
	 * Выполняем проверку того, что отвергнутая правка оглавления НЕ ТРОНУЛА: заслон стоит
	 * до всякой правки, и следа отказ оставить не вправе
	 */
	ASSERT_EQ(index.entries().size(), 1ul);
	ASSERT_EQ(index.entries().at(0).length, entry.length) << "отвергнутая правка изменила строку";
	/**
	 * Выполняем проверку того, что правка строки СУЩЕСТВУЮЩЕЙ работает: без этой половины
	 * проверка прошла бы и у оглавления, не принимающего никакой правки вовсе
	 */
	ASSERT_TRUE(index.replace(0, other)) << "правка существующей строки отвергнута";
	// Выполняем проверку того, что правка легла
	ASSERT_EQ(index.entries().at(0).length, other.length) << "правка не легла";
}
/**
 * @brief Проверка того, что длина оглавления, не кратная строке, отвергается
 *
 * @details Оглавление есть череда строк равной длины, и длина, строке не кратная, означает
 *          обрыв либо порчу. Заслон этот стоит ПРЕЖДЕ отведения памяти под строки: без
 *          него деление дало бы неполное число строк, а чтение последней ушло бы за
 *          поданное. Место щуп нужности 07.09.2026 назвал не ловившим ничего
 */
TEST_F(IndexFixture, IndexLengthNotAMultipleOfTheEntryIsRefused) {
	// Оглавление контейнера
	abc::index_t index;
	// Код отказа снятия оглавления
	abc::error_t error = abc::error_t::NONE;
	/**
	 * Выполняем сборку подлинного оглавления об ОДНОЙ строке и добавляем лишний октет
	 *
	 * @warning Некратность наводится ИЗЛИШКОМ поверх целой строки, а не отсечением октета
	 *          у двух строк. Разница решает всё: при отсечении обесточенный заслон дал бы
	 *          неполную строку, та отверглась бы как порченая, и причина вышла бы ТА ЖЕ -
	 *          проверка устояла бы и без заслона. С излишком же обесточенный заслон снял
	 *          бы одну целую строку УСПЕХОМ, лишний октет отбросив молча. Замерено щупом
	 *          нужности 07.09.2026: первый вид проверки его молчания не менял
	 */
	abc::index_t single;
	// Заводимая строка подлинного оглавления
	abc::entry_t only;
	// Выполняем установку смещения кадра строки оглавления
	only.chunk = 0;
	// Выполняем установку смещения записи в содержимом кадра
	only.offset = 0;
	// Выполняем установку длины записи
	only.length = 16;
	// Выполняем внесение строки в оглавление
	single.add(only);
	// Октеты уложенного оглавления об одной строке
	vector <uint8_t> octets;
	// Выполняем укладку подлинного оглавления
	single.pack(octets);
	// Выполняем проверку того, что уложена ровно одна строка
	ASSERT_EQ(octets.size(), static_cast <size_t> (abc::ENTRY_LENGTH));
	// Выполняем добавление лишнего октета, делающего длину некратной
	octets.push_back(0x00);
	// Выполняем проверку того, что оглавление некратной длины отвергается
	ASSERT_FALSE(index.unpack(octets.data(), octets.size(), error)) << "оглавление некратной длины снято";
	// Выполняем проверку того, что причина названа негодным оглавлением
	ASSERT_EQ(error, abc::error_t::INVALID_INDEX) << abc::message(error);
	// Выполняем проверку того, что строк в оглавлении не завелось
	ASSERT_TRUE(index.entries().empty()) << "отвергнутое оглавление завело строки";
	/**
	 * Выполняем проверку того, что оглавление КРАТНОЙ длины снимается: без этой половины
	 * проверка прошла бы и у оглавления, отвергающего всякое снятие вовсе
	 *
	 * @note Октеты берутся УКЛАДКОЙ подлинного оглавления, а не чередою нулей: строка из
	 *       нулей объявляет нулевую длину записи и отвергается как порченая - тогда обе
	 *       половины проверки отвечали бы отказом, и кратность длины не поверялась бы
	 *       вовсе. Замерено 07.09.2026: череда нулей давала отказ «строка оглавления
	 *       повреждена», а не успех
	 */
	abc::index_t source;
	// Заводимая строка подлинного оглавления
	abc::entry_t row;
	// Выполняем установку смещения кадра строки оглавления
	row.chunk = 0;
	// Выполняем установку смещения записи в содержимом кадра
	row.offset = 0;
	// Выполняем установку длины записи
	row.length = 16;
	// Выполняем внесение первой строки в оглавление
	source.add(row);
	// Выполняем установку смещения второй записи в содержимом кадра
	row.offset = 16;
	// Выполняем внесение второй строки в оглавление
	source.add(row);
	// Октеты уложенного оглавления
	vector <uint8_t> whole;
	// Выполняем укладку подлинного оглавления
	source.pack(whole);
	// Выполняем проверку того, что длина уложенного кратна строке
	ASSERT_EQ(whole.size() % abc::ENTRY_LENGTH, 0ul) << "уложенное оглавление некратно строке";
	ASSERT_TRUE(index.unpack(whole.data(), whole.size(), error)) << abc::message(error);
	// Выполняем проверку того, что строк снялось ровно две
	ASSERT_EQ(index.entries().size(), 2ul);
}
/**
 * @brief Проверка того, что строка оглавления, указывающая за тело, отвергается
 *
 * @details Смещение кадра приходит из оглавления, а оно НЕДОВЕРЕННО. Заслон стоит прежде
 *          первого же чтения: сложение `HEADER_LENGTH + entry.chunk` при смещении близ
 *          предела разрядной сетки завернулось бы и подало источнику октетов смещение
 *          малое вместо непомерного. Место щуп нужности 07.09.2026 назвал не ловившим
 *          ничего: строку с непомерным смещением кадра не подделывал никто
 */
TEST_F(IndexFixture, EntryChunkBeyondTheBodyIsRefused) {
	// Октеты собранного контейнера
	vector <uint8_t> pristine;
	{
		// Сборщик контейнера
		abc::assembler_t assembler;
		/**
		 * Выполняем внесение череды записей в собираемый контейнер
		 */
		for(size_t i = 0; i < 4; i++){
			// Выполняем сборку очередной записи
			const vector <uint8_t> item = record(string{"запись номер "} + to_string(i));
			// Выполняем внесение очередной записи в собираемый контейнер
			ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT));
		}
		// Выполняем завершение сборки контейнера
		ASSERT_TRUE(assembler.complete(pristine));
	}
	// Снятый заголовок опознания контейнера
	abc::header_t header;
	// Код отказа снятия заголовка
	abc::error_t error = abc::error_t::NONE;
	// Выполняем снятие заголовка опознания контейнера
	ASSERT_TRUE(header.unpack(pristine.data(), pristine.size(), error)) << abc::message(error);
	// Выполняем получение смещения первой строки оглавления в записи контейнера
	const size_t entry = static_cast <size_t> (header.index) + abc::CHUNK_HEADER;
	// Выполняем проверку того, что строка оглавления в записи контейнера умещается
	ASSERT_LE(entry + abc::ENTRY_LENGTH, pristine.size());
	// Выполняем проверку того, что оглавление легло в кадр открыто
	ASSERT_EQ(pristine.at(static_cast <size_t> (header.index)), 0x00) << "оглавление уложено сжатым";
	/**
	 * @brief Функция выборки первой записи из поданной записи контейнера
	 *
	 * @param data  октеты записи контейнера
	 * @param error код отказа выборки записи
	 * @return      признак успешно выбранной записи
	 *
	 */
	const auto fetch = [](const vector <uint8_t> & data, abc::error_t & error, size_t * reads = nullptr) noexcept -> bool {
		// Выборщик записей контейнера
		abc::fetcher_t fetcher;
		/**
		 * Если открыть контейнер не вышло
		 */
		if(!fetcher.open([&data, reads](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
			// Если счётчик чтений затребован, выполняем его наращивание
			if(reads != nullptr) (*reads)++;
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
		/**
		 * Выполняем сброс счётчика чтений ПОСЛЕ открытия контейнера
		 *
		 * @note Открытие читает оглавление, и чтения его к делу не идут. Считаются лишь
		 *       чтения САМОЙ ВЫБОРКИ: заслон стоит прежде чтения кадра, и потому выборка,
		 *       отказавшая по нему, не читает НИ РАЗУ
		 */
		if(reads != nullptr) (*reads) = 0;
		// Буфер выбранной записи контейнера
		vector <uint8_t> item;
		// Выполняем выборку первой записи контейнера
		const bool taken = fetcher.record(0, item);
		// Выполняем установку кода отказа выборки записи
		error = fetcher.error();
		// Выводим признак успешности выборки записи
		return taken;
	};
	/**
	 * Выполняем проверку выборки записи из НЕТРОНУТОГО контейнера: без этой половины
	 * проверка прошла бы и у выборщика, отвергающего всякую выборку вовсе
	 */
	size_t honest = 0;
	ASSERT_TRUE(fetch(pristine, error, &honest)) << "код отказа: " << abc::message(error);
	/**
	 * Выполняем проверку того, что выборка из целого контейнера источник ЧИТАЛА: без
	 * этого утверждение о нуле чтений ниже держалось бы и у выборщика, не читающего вовсе
	 */
	ASSERT_GT(honest, 0ul) << "выборка из целого контейнера источника не читала";
	// Запись контейнера с поддельным смещением кадра
	vector <uint8_t> forged = pristine;
	/**
	 * Выполняем подделку смещения кадра первой строки оглавления: смещение это лежит
	 * восемью октетами от начала строки, и берётся оно НЕПОМЕРНЫМ - близ предела
	 * разрядной сетки, чтобы поверить и заворот сложения
	 */
	for(size_t i = 0; i < 8; i++)
		// Выполняем установку очередного октета непомерного смещения кадра
		forged.at(entry + i) = 0xFF;
	/**
	 * Выполняем обновление контрольной суммы кадра оглавления
	 *
	 * @note Правка кадра НА МЕСТЕ обязана обновить сумму его: без обновления снятие
	 *       кадра отвечает отказом СУММЫ, и заслон смещения не достигается вовсе.
	 *       Замерено 07.09.2026 - первый заход проверки дал причину «сумма заголовка
	 *       не сошлась» вместо ожидаемой, и это был отказ соседа, а не искомого
	 */
	{
		// Выполняем получение смещения кадра оглавления в записи контейнера
		const size_t place = static_cast <size_t> (header.index);
		// Выполняем получение длины уложенного содержимого кадра оглавления
		const size_t length = static_cast <size_t> (abc::gather(forged.data() + place + 4, 4));
		// Выполняем укладку обновлённой контрольной суммы кадра оглавления
		abc::fixed(forged.data() + place + abc::CHUNK_DIGEST,
		 abc::digest(forged.data() + place, abc::CHUNK_HEADER + length), 8);
	}
	/**
	 * Количество чтений источника, сделанных ПОСЛЕ открытия контейнера
	 *
	 * @warning Счёт этот и есть суть проверки. Утверждать одну лишь ПРИЧИНУ отказа
	 *          недостаточно: обесточенный заслон даёт заворот сложения `HEADER_LENGTH +
	 *          chunk`, источник читает по малому смещению, кадр там негоден - и отказ
	 *          приходит с ТОЮ ЖЕ причиной от соседа. Проверка устояла бы, ничего не
	 *          стерегя (замерено щупом нужности 07.09.2026). Заслон же обещает большее:
	 *          он стоит прежде чтения КАДРА, и потому чтений выходит МЕНЬШЕ, чем у
	 *          выборки из целого контейнера, - а такого соседний заслон дать не может,
	 *          он-то отказывает уже ПОСЛЕ чтения
	 *
	 * @note Считаются чтения ОДНОЙ ЛИШЬ ВЫБОРКИ: счётчик сбрасывается после открытия.
	 *       Первый заход считал все чтения подряд и требовал, чтобы их вышло меньше, чем
	 *       у выборки из целого контейнера, - и это тоже не различало: без заслона чтений
	 *       выходит меньше полного успеха всё равно, ибо разбор обрывается на негодном
	 *       кадре. Щуп нужности молчал и на такой проверке (07.09.2026)
	 */
	size_t reads = 0;
	// Выполняем проверку того, что выборка по поддельной строке отвергается
	ASSERT_FALSE(fetch(forged, error, &reads)) << "запись выбрана по строке, указывающей за тело";
	// Выполняем проверку того, что причина названа повреждённым кадром
	ASSERT_EQ(error, abc::error_t::INVALID_CHUNK) << abc::message(error);
	/**
	 * Выполняем проверку того, что выборка не читала источник НИ РАЗУ: заслон стоит
	 * прежде чтения кадра, и такого соседний заслон дать не может - он отказывает уже
	 * ПОСЛЕ чтения
	 */
	ASSERT_EQ(reads, 0ul) << "выборка по строке за телом читала источник " << reads << " раз";
}
/**
 * @brief Проверка того, что воронка отказа выборщика доносит причину журналу
 *
 * @details Воронка `Fetcher::fail` заводит код отказа и объявляет его журналу. Объявление
 *          это - единственный путь, каким об отказе узнаёт СТОРОННИЙ наблюдатель: зовущий
 *          видит лишь ложь возврата, а причину спрашивает не всякий
 *
 * @note Близнец её у правщика (`Editor::fail`) молчал тем же молчанием и закреплён
 *       `EditorFixture.TheRefusalFunnelReportsItsCauseToTheJournal`. Воронки эти РАЗНЫЕ и
 *       живут в разных работах: закрепление одной другую не стережёт, и щуп судил их
 *       порознь
 *
 */
TEST_F(IndexFixture, TheFetcherFunnelReportsItsCauseToTheJournal) {
	/**
	 * Возвращаем уровень записей, гашение снимая.
	 *
	 * @warning Поверке этой журнал НУЖЕН, и она обязана заявить нужду свою САМА:
	 *          сторож гашения своего набора глушит журнал у всякой его проверки,
	 *          а полагаться на состояние, оставленное соседями, значило бы городить
	 *          поверку, чей исход зависит от порядка запуска
	 */
	awh::log::level(awh::log::level_t::ALL);
	// Донесения, снятые с журнала подпискою
	vector <string> journal;
	// Разрешаем отложенный вывод: подписка кормится именно им
	awh::log::mode({awh::log::mode_t::DEFERRED});
	// Выполняем подписку на журнал ради разбора донесений об отказах
	awh::log::subscribe([&journal](const awh::log::flag_t, const string_view text) noexcept -> void {
		// Выполняем накопление очередного донесения журнала
		journal.emplace_back(text);
	});
	// Выборщик записей контейнера
	abc::fetcher_t fetcher;
	/**
	 * Выполняем открытие БЕЗ работы чтения октетов: работа обязана быть отвечена отказом
	 */
	ASSERT_FALSE(fetcher.open(nullptr)) << "открытие без источника удалось, проверка негодна";
	// Выполняем получение объявленной причины отказа
	const abc::error_t cause = fetcher.error();
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
/**
 * @brief Проверка того, что отказ снятия заголовка опознания выборщиком ПЕРЕНОСИТСЯ
 *
 * @details Открытие снимает заголовок опознания и, встретив отказ, обязано вынести наружу
 *          причину ЗАГОЛОВКА, а не завести свою. Без заслона снятие продолжилось бы по
 *          заголовку недоснятому: поле оглавления осталось бы нулём, и открытие отказало
 *          бы соседним заслоном с причиною «оглавления нет». Отказ при том остался бы
 *          отказом, и проверка, спрашивающая одну лишь ложь возврата, была бы зелена
 *
 * @note Оттого утверждается ИМЕННО причина, и утверждается ДВОЯКО: она равна той, какую
 *       даёт снятие заголовка теми же октетами напрямую, и она НЕ РАВНА причине соседа
 *
 */
TEST_F(IndexFixture, TheHeaderRefusalIsCarriedOutwardByTheFetcher) {
	// Октеты собранного контейнера
	vector <uint8_t> data;
	{
		// Сборщик контейнера
		abc::assembler_t assembler;
		// Выполняем сборку вносимой записи
		const vector <uint8_t> item = record("запись");
		// Выполняем внесение записи в собираемый контейнер
		ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(assembler.error());
		// Выполняем завершение сборки контейнера
		ASSERT_TRUE(assembler.complete(data)) << "код отказа: " << abc::message(assembler.error());
	}
	/**
	 * Выполняем порчу опознавательной метки контейнера: метка стоит первыми октетами
	 * записи, и снятие заголовка обязано отвергнуть такие октеты
	 */
	data.at(0) = static_cast <uint8_t> (data.at(0) ^ 0xFF);
	// Код отказа снятия заголовка опознания напрямую
	abc::error_t straight = abc::error_t::NONE;
	{
		// Снимаемый заголовок опознания контейнера
		abc::header_t header;
		// Выполняем проверку того, что снятие заголовка испорченных октетов отвергнуто
		ASSERT_FALSE(header.unpack(data.data(), data.size(), straight))
			<< "снятие заголовка испорченных октетов удалось, проверка негодна";
	}
	// Выполняем проверку того, что снятие заголовка причину объявило
	ASSERT_NE(straight, abc::error_t::NONE) << "снятие заголовка причины не объявило";
	// Источник октетов испорченного контейнера
	Source source;
	// Выполняем передачу октетов испорченного контейнера источнику
	source.data = data;
	// Выборщик записей испорченного контейнера
	abc::fetcher_t fetcher;
	// Выполняем проверку того, что открытие испорченного контейнера отвечено отказом
	ASSERT_FALSE(fetcher.open([&source](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем чтение затребованных октетов контейнера
		return source.read(offset, size, result);
	})) << "открытие испорченного контейнера удалось, проверка негодна";
	/**
	 * Выполняем проверку того, что наружу вышла причина ЗАГОЛОВКА
	 */
	ASSERT_EQ(fetcher.error(), straight)
		<< "вышла причина «" << abc::message(fetcher.error())
		<< "» вместо причины заголовка «" << abc::message(straight) << "»";
	/**
	 * Выполняем проверку того, что причина эта НЕ принадлежит соседнему заслону: без
	 * заслона снятие пошло бы дальше по недоснятому заголовку, и наружу вышло бы
	 * объявление об отсутствии оглавления
	 */
	ASSERT_NE(fetcher.error(), abc::error_t::MISSING_INDEX)
		<< "вышла причина соседнего заслона, а не причина заголовка";
}
/**
 * @brief Проверка того, что отказ снятия оглавления оставляет выборщика ЗАКРЫТЫМ
 *
 * @details Снятие оглавления - последняя работа открытия, и признак открытости ставится
 *          строкою ниже. Без заслона открытие объявило бы себя удавшимся с оглавлением
 *          ПУСТЫМ: контейнер о четырёх записях предстал бы зовущему пустым, и молчание
 *          это было бы худшим родом отказа - выборка отвечала бы «записи нет» вместо
 *          «оглавление повреждено»
 *
 * @note Утверждается ДЕЙСТВИЕ - состояние выборщика после отказа, - а не причина: причину
 *       заводит снятие оглавления, и её стерегут заслоны самого снятия. Здесь же спрос об
 *       ином: пережил ли отказ признак открытости
 *
 */
TEST_F(IndexFixture, TheIndexRefusalLeavesTheFetcherClosed) {
	// Октеты собранного контейнера
	vector <uint8_t> data;
	{
		// Сборщик контейнера
		abc::assembler_t assembler;
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
	// Снятый заголовок опознания контейнера
	abc::header_t header;
	// Код отказа снятия заголовка
	abc::error_t error = abc::error_t::NONE;
	// Выполняем снятие заголовка опознания контейнера
	ASSERT_TRUE(header.unpack(data.data(), data.size(), error)) << abc::message(error);
	// Выполняем проверку того, что оглавление контейнера объявлено
	ASSERT_GT(header.index, 0u);
	// Выполняем проверку того, что оглавление легло в кадр открыто
	ASSERT_EQ(data.at(static_cast <size_t> (header.index)), 0x00) << "оглавление уложено сжатым";
	// Выполняем получение смещения первой строки оглавления в записи контейнера
	const size_t entry = static_cast <size_t> (header.index) + abc::CHUNK_HEADER;
	// Выполняем проверку того, что строка оглавления в записи контейнера умещается
	ASSERT_LE(entry + abc::ENTRY_LENGTH, data.size());
	/**
	 * Выполняем занятие хвостовых октетов строки оглавления: четыре октета эти оставлены
	 * впрок и укладываются нулями, а снятие оглавления требует нулей и отвергает иное
	 */
	for(size_t i = 20; i < 24; i++)
		// Выполняем укладку очередного октета неведомого хвоста строки
		data.at(entry + i) = 0xFF;
	/**
	 * Выполняем обновление контрольной суммы кадра оглавления: иначе отказ придёт по
	 * сумме, и до снятия оглавления дело не дойдёт вовсе
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
	// Источник октетов поддельного контейнера
	Source source;
	// Выполняем передачу октетов поддельного контейнера источнику
	source.data = data;
	// Выборщик записей поддельного контейнера
	abc::fetcher_t fetcher;
	// Выполняем проверку того, что открытие поддельного контейнера отвечено отказом
	ASSERT_FALSE(fetcher.open([&source](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Выполняем чтение затребованных октетов контейнера
		return source.read(offset, size, result);
	})) << "открытие поддельного контейнера удалось, проверка негодна";
	// Буфер выбираемой записи контейнера
	vector <uint8_t> item;
	/**
	 * Выполняем проверку того, что выборка записи отвечена отказом: выборщик обязан
	 * остаться ЗАКРЫТЫМ, а не открытым с пустым оглавлением
	 */
	ASSERT_FALSE(fetcher.record(0, item)) << "выборка после отказа открытия удалась";
	/**
	 * Выполняем проверку того, что отказ выборки объявлен ВНУТРЕННИМ: причина эта
	 * принадлежит закрытому выборщику, тогда как выборщик, открывшийся с пустым
	 * оглавлением, отвечал бы «строка оглавления повреждена»
	 */
	ASSERT_EQ(fetcher.error(), abc::error_t::INTERNAL)
		<< "выборщик остался открытым с пустым оглавлением, причина: " << abc::message(fetcher.error());
}
