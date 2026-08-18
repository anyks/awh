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
