/**
 * @file chunk.cpp
 * @date 2026-08-19
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки кадра бинарного контейнера ABC — круговой обход укладки, сжатие с
 *        подбором метода, шифрование поодиночке и стойкость снятия к порче
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
	 * @brief Класс опоры проверок кадра
	 *
	 * @details Модуль сжатия и модуль шифрования заводит потребитель, а не кодек: у них
	 *          свои зависимости, и кодек их за потребителя не заводит
	 *
	 */
	class ChunkFixture : public testing::Test {
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
	 * @brief Функция сборки рассеянного содержимого, сжатию не поддающегося
	 *
	 * @param size размер собираемого содержимого
	 * @return     собранное содержимое
	 *
	 */
	string scattered(const size_t size) noexcept {
		// Собираемое содержимое
		string result;
		// Выполняем заведение места под собираемое содержимое
		result.reserve(size);
		// Состояние выработки рассеянных октетов
		uint64_t state = 0x9E3779B97F4A7C15ull;
		/**
		 * Выполняем сборку рассеянного содержимого закреплённой выработкой:
		 * случайный источник дал бы проверку, воспроизводимую не всегда
		 */
		for(size_t i = 0; i < size; i++){
			// Выполняем шаг выработки рассеянных октетов
			state = ((state * 6364136223846793005ull) + 1442695040888963407ull);
			// Выполняем добавление очередного рассеянного октета
			result.push_back(static_cast <char> ((state >> 33) & 0xFF));
		}
		// Выводим собранное содержимое
		return result;
	}
	/**
	 * @brief Функция сборки хорошо сжимаемого содержимого
	 *
	 * @param count количество повторений
	 * @return      собранное содержимое
	 *
	 */
	string repeated(const size_t count) noexcept {
		// Собираемое содержимое
		string result;
		/**
		 * Выполняем сборку хорошо сжимаемого содержимого
		 */
		for(size_t i = 0; i < count; i++)
			// Выполняем добавление очередного повторения
			result.append("повторяющийся текст, какой сожмётся отменно; ");
		// Выводим собранное содержимое
		return result;
	}
};

/**
 * @brief Проверка кругового обхода кадра без сжатия и шифрования
 *
 */
TEST_F(ChunkFixture, PlainRoundtrip) {
	// Укладчик кадров
	abc::packer_t packer;
	// Укладываемое содержимое кадра
	const string payload = "содержимое кадра";
	// Буфер уложенного кадра
	vector <uint8_t> record;
	// Выполняем укладку кадра
	ASSERT_TRUE(packer.pack(payload.data(), payload.size(), abc::payload_t::TEXT, 7, 3, record))
		<< "код отказа: " << abc::message(packer.error());
	// Выполняем проверку длины уложенного кадра
	ASSERT_EQ(record.size(), abc::CHUNK_HEADER + payload.size());
	// Смещение снятия кадра
	size_t offset = 0;
	// Снятое содержимое кадра
	vector <uint8_t> content;
	// Снятые сведения о кадре
	abc::chunk_t chunk;
	// Выполняем снятие кадра
	ASSERT_TRUE(packer.unpack(record.data(), record.size(), offset, content, chunk))
		<< "код отказа: " << abc::message(packer.error());
	// Выполняем проверку смещения снятия кадра
	ASSERT_EQ(offset, record.size());
	// Выполняем проверку снятого содержимого кадра
	ASSERT_EQ(string(content.begin(), content.end()), payload);
	// Выполняем проверку порядкового номера кадра
	ASSERT_EQ(chunk.number, 7u);
	// Выполняем проверку поколения записи кадра
	ASSERT_EQ(chunk.generation, 3u);
	// Выполняем проверку того, что содержимое кадра не сжато
	ASSERT_EQ(chunk.method, compressor::method_t::NONE);
	// Выполняем проверку того, что содержимое кадра не зашифровано
	ASSERT_FALSE(chunk.encrypted);
	// Выполняем проверку длины исходного содержимого кадра
	ASSERT_EQ(chunk.origin, payload.size());
}
/**
 * @brief Проверка укладки пустого содержимого кадра
 *
 */
TEST_F(ChunkFixture, EmptyPayload) {
	// Укладчик кадров
	abc::packer_t packer;
	// Буфер уложенного кадра
	vector <uint8_t> record;
	// Выполняем укладку кадра с пустым содержимым
	ASSERT_TRUE(packer.pack(nullptr, 0, abc::payload_t::MIXED, 0, 0, record))
		<< "код отказа: " << abc::message(packer.error());
	// Выполняем проверку длины уложенного кадра
	ASSERT_EQ(record.size(), abc::CHUNK_HEADER);
	// Смещение снятия кадра
	size_t offset = 0;
	// Снятое содержимое кадра
	vector <uint8_t> content;
	// Снятые сведения о кадре
	abc::chunk_t chunk;
	// Выполняем снятие кадра
	ASSERT_TRUE(packer.unpack(record.data(), record.size(), offset, content, chunk))
		<< "код отказа: " << abc::message(packer.error());
	// Выполняем проверку пустоты снятого содержимого
	ASSERT_TRUE(content.empty());
	// Выполняем проверку длины исходного содержимого кадра
	ASSERT_EQ(chunk.origin, 0u);
}
/**
 * @brief Проверка сжатия содержимого кадра
 *
 * @details Сжатие, выигрыша не давшее, отбрасывается: заголовок метода и словарь съели
 * бы больше, чем сберегли
 *
 */
TEST_F(ChunkFixture, Compression) {
	// Укладчик кадров
	abc::packer_t packer;
	// Выполняем установку модуля сжатия
	packer.compressor(this->_compressor.get());
	// Укладываемое хорошо сжимаемое содержимое
	const string payload = repeated(200);
	// Буфер уложенного кадра
	vector <uint8_t> record;
	// Выполняем укладку кадра
	ASSERT_TRUE(packer.pack(payload.data(), payload.size(), abc::payload_t::TEXT, 1, 0, record))
		<< "код отказа: " << abc::message(packer.error());
	// Смещение снятия кадра
	size_t offset = 0;
	// Снятое содержимое кадра
	vector <uint8_t> content;
	// Снятые сведения о кадре
	abc::chunk_t chunk;
	// Выполняем снятие кадра
	ASSERT_TRUE(packer.unpack(record.data(), record.size(), offset, content, chunk))
		<< "код отказа: " << abc::message(packer.error());
	// Выполняем проверку снятого содержимого кадра
	ASSERT_EQ(string(content.begin(), content.end()), payload);
	// Выполняем проверку длины исходного содержимого кадра
	ASSERT_EQ(chunk.origin, payload.size());
	/**
	 * Выполняем проверку того, что содержимое действительно сжато.
	 *
	 * Проверка условная здесь негодна: при отключённом сжатии она прошла бы вхолостую
	 * и молча, а сжатие числилось бы проверенным. Метод подбирается настройками, и
	 * подобранный обязан быть в сборке
	 */
	ASSERT_NE(chunk.method, compressor::method_t::NONE)
		<< "сжатие не выполнено, метод подобран: " << static_cast <uint32_t> (packer.suggest(abc::payload_t::TEXT));
	// Выполняем проверку того, что уложенный кадр короче исходного содержимого
	ASSERT_LT(record.size(), payload.size()) << "метод сжатия: " << static_cast <uint32_t> (chunk.method);
	// Выполняем проверку того, что уложенная длина короче исходной
	ASSERT_LT(chunk.length, chunk.origin);
	// Выполняем проверку того, что выигрыш сжатия существен
	ASSERT_LT(chunk.length, (chunk.origin / 2));
	// Выполняем проверку того, что мелкое содержимое не сжимается вовсе
	{
		// Укладываемое мелкое содержимое
		const string small = "мелочь";
		// Буфер уложенного кадра
		vector <uint8_t> record;
		// Выполняем укладку кадра
		ASSERT_TRUE(packer.pack(small.data(), small.size(), abc::payload_t::TEXT, 2, 0, record));
		// Смещение снятия кадра
		size_t offset = 0;
		// Снятое содержимое кадра
		vector <uint8_t> content;
		// Снятые сведения о кадре
		abc::chunk_t chunk;
		// Выполняем снятие кадра
		ASSERT_TRUE(packer.unpack(record.data(), record.size(), offset, content, chunk));
		// Выполняем проверку того, что мелкое содержимое не сжато
		ASSERT_EQ(chunk.method, compressor::method_t::NONE);
		// Выполняем проверку снятого содержимого кадра
		ASSERT_EQ(string(content.begin(), content.end()), small);
	}
	/**
	 * Выполняем проверку того, что порог соблюдается и на сжимаемом содержимом.
	 *
	 * Мелкое содержимое не сжимается по двум разным причинам сразу: порог не даёт
	 * даже взяться за него, а сжатие без выигрыша отбрасывается. Разделить их можно
	 * лишь содержимым, какое ниже порога, но сжалось бы отменно
	 */
	{
		// Настройки укладки кадра с высоким порогом сжатия
		abc::packer_t::settings_t settings;
		// Выполняем установку порога, какого содержимое не достигает
		settings.threshold = 4096;
		// Выполняем установку настроек укладки кадра
		packer.settings(settings);
		// Укладываемое сжимаемое содержимое ниже порога
		const string below = repeated(20);
		// Выполняем проверку того, что содержимое действительно ниже порога
		ASSERT_LT(below.size(), settings.threshold);
		// Буфер уложенного кадра
		vector <uint8_t> record;
		// Выполняем укладку кадра
		ASSERT_TRUE(packer.pack(below.data(), below.size(), abc::payload_t::TEXT, 3, 0, record));
		// Смещение снятия кадра
		size_t offset = 0;
		// Снятое содержимое кадра
		vector <uint8_t> content;
		// Снятые сведения о кадре
		abc::chunk_t chunk;
		// Выполняем снятие кадра
		ASSERT_TRUE(packer.unpack(record.data(), record.size(), offset, content, chunk));
		// Выполняем проверку того, что содержимое ниже порога не сжато вовсе
		ASSERT_EQ(chunk.method, compressor::method_t::NONE)
			<< "порог сжатия не соблюдён на содержимом длиною " << below.size();
		// Выполняем проверку снятого содержимого кадра
		ASSERT_EQ(string(content.begin(), content.end()), below);
	}
	/**
	 * Выполняем проверку того, что сжатие без выигрыша отбрасывается.
	 *
	 * Рассеянное содержимое сжатию не поддаётся и от него лишь растёт. Взятое
	 * несмотря на это, оно удлинило бы кадр, а разбор о том не узнал бы вовсе
	 */
	{
		// Настройки укладки кадра
		abc::packer_t::settings_t settings;
		// Выполняем установку низкого порога сжатия
		settings.threshold = 16;
		// Выполняем установку настроек укладки кадра
		packer.settings(settings);
		// Укладываемое рассеянное содержимое
		const string noise = scattered(1024);
		// Буфер уложенного кадра
		vector <uint8_t> record;
		// Выполняем укладку кадра
		ASSERT_TRUE(packer.pack(noise.data(), noise.size(), abc::payload_t::BINARY, 4, 0, record));
		// Смещение снятия кадра
		size_t offset = 0;
		// Снятое содержимое кадра
		vector <uint8_t> content;
		// Снятые сведения о кадре
		abc::chunk_t chunk;
		// Выполняем снятие кадра
		ASSERT_TRUE(packer.unpack(record.data(), record.size(), offset, content, chunk));
		// Выполняем проверку того, что рассеянное содержимое не сжато
		ASSERT_EQ(chunk.method, compressor::method_t::NONE)
			<< "взято сжатие без выигрыша, уложено " << chunk.length << " при исходных " << chunk.origin;
		// Выполняем проверку того, что кадр не вырос сверх содержимого и заголовка
		ASSERT_EQ(record.size(), abc::CHUNK_HEADER + noise.size());
		// Выполняем проверку снятого содержимого кадра
		ASSERT_EQ(string(content.begin(), content.end()), noise);
	}
}
/**
 * @brief Проверка подбора метода сжатия под вид содержимого
 *
 */
TEST_F(ChunkFixture, Suggestion) {
	// Укладчик кадров
	abc::packer_t packer;
	// Настройки укладки кадра
	abc::packer_t::settings_t settings;
	// Выполняем установку метода сжатия знакового текста
	settings.text = compressor::method_t::BROTLI;
	// Выполняем установку метода сжатия сырых октетов
	settings.binary = compressor::method_t::LZ4;
	// Выполняем установку метода сжатия однородных чисел
	settings.numeric = compressor::method_t::ZSTD;
	// Выполняем установку метода сжатия содержимого вперемешку
	settings.mixed = compressor::method_t::GZIP;
	// Выполняем установку настроек укладки кадра
	packer.settings(settings);
	// Выполняем проверку подбора метода сжатия знакового текста
	ASSERT_EQ(packer.suggest(abc::payload_t::TEXT), compressor::method_t::BROTLI);
	// Выполняем проверку подбора метода сжатия сырых октетов
	ASSERT_EQ(packer.suggest(abc::payload_t::BINARY), compressor::method_t::LZ4);
	// Выполняем проверку подбора метода сжатия однородных чисел
	ASSERT_EQ(packer.suggest(abc::payload_t::NUMERIC), compressor::method_t::ZSTD);
	// Выполняем проверку подбора метода сжатия содержимого вперемешку
	ASSERT_EQ(packer.suggest(abc::payload_t::MIXED), compressor::method_t::GZIP);
}
/**
 * @brief Проверка шифрования содержимого кадра
 *
 * @details Всякий кадр шифруется своим вызовом, и вызов этот заводит свой вектор
 * инициализации. Оттого два кадра с одним и тем же содержимым обязаны дать разную
 * запись: совпадение их означало бы повтор вектора, а повтор его при том же ключе - это
 * взлом, и по работе программы он не виден вовсе
 *
 */
TEST_F(ChunkFixture, Encryption) {
	// Укладчик кадров
	abc::packer_t packer;
	// Настройки укладки кадра
	abc::packer_t::settings_t settings;
	// Выполняем объявление шифрования содержимого кадра
	settings.encrypt = true;
	// Выполняем установку настроек укладки кадра
	packer.settings(settings);
	// Выполняем установку модуля шифрования
	packer.crypto(this->_crypto.get());
	// Укладываемое содержимое кадра
	const string payload = "тайное содержимое кадра";
	// Буфер первого уложенного кадра
	vector <uint8_t> first;
	// Выполняем укладку первого кадра
	ASSERT_TRUE(packer.pack(payload.data(), payload.size(), abc::payload_t::TEXT, 1, 0, first))
		<< "код отказа: " << abc::message(packer.error());
	// Буфер второго уложенного кадра
	vector <uint8_t> second;
	// Выполняем укладку второго кадра с тем же содержимым
	ASSERT_TRUE(packer.pack(payload.data(), payload.size(), abc::payload_t::TEXT, 2, 0, second))
		<< "код отказа: " << abc::message(packer.error());
	// Выполняем проверку того, что записи кадров с одним содержимым разошлись
	ASSERT_NE(first, second) << "вектор инициализации повторился у двух кадров";
	// Смещение снятия кадра
	size_t offset = 0;
	// Снятое содержимое кадра
	vector <uint8_t> content;
	// Снятые сведения о кадре
	abc::chunk_t chunk;
	// Выполняем снятие первого кадра
	ASSERT_TRUE(packer.unpack(first.data(), first.size(), offset, content, chunk))
		<< "код отказа: " << abc::message(packer.error());
	// Выполняем проверку признака зашифрованности содержимого
	ASSERT_TRUE(chunk.encrypted);
	// Выполняем проверку снятого содержимого кадра
	ASSERT_EQ(string(content.begin(), content.end()), payload);
	// Выполняем проверку того, что содержимое кадра не лежит открыто
	{
		// Запись уложенного кадра строкою
		const string record(first.begin() + abc::CHUNK_HEADER, first.end());
		// Выполняем проверку того, что открытого содержимого в записи нет
		ASSERT_EQ(record.find(payload), string::npos);
	}
	// Выполняем проверку отказа снятия кадра без модуля шифрования
	{
		// Укладчик кадров без модуля шифрования
		abc::packer_t plain;
		// Смещение снятия кадра
		size_t offset = 0;
		// Снятое содержимое кадра
		vector <uint8_t> content;
		// Снятые сведения о кадре
		abc::chunk_t chunk;
		// Выполняем проверку отказа снятия зашифрованного кадра
		ASSERT_FALSE(plain.unpack(first.data(), first.size(), offset, content, chunk));
		// Выполняем проверку кода отказа
		ASSERT_EQ(plain.error(), abc::error_t::ENCRYPTION_FAILED);
	}
}
/**
 * @brief Проверка укладки череды кадров подряд
 *
 */
TEST_F(ChunkFixture, Sequence) {
	// Укладчик кадров
	abc::packer_t packer;
	// Буфер уложенных кадров
	vector <uint8_t> record;
	/**
	 * Выполняем укладку череды кадров подряд
	 */
	for(uint64_t i = 0; i < 5; i++){
		// Укладываемое содержимое очередного кадра
		const string payload = ("кадр номер " + to_string(i));
		// Выполняем укладку очередного кадра
		ASSERT_TRUE(packer.pack(payload.data(), payload.size(), abc::payload_t::TEXT, i, 0, record))
			<< "номер кадра: " << i;
	}
	// Смещение снятия кадров
	size_t offset = 0;
	// Количество снятых кадров
	uint64_t count = 0;
	/**
	 * Выполняем снятие всех уложенных кадров
	 */
	while(offset < record.size()){
		// Снятое содержимое кадра
		vector <uint8_t> content;
		// Снятые сведения о кадре
		abc::chunk_t chunk;
		// Выполняем снятие очередного кадра
		ASSERT_TRUE(packer.unpack(record.data(), record.size(), offset, content, chunk))
			<< "номер кадра: " << count << ", код отказа: " << abc::message(packer.error());
		// Выполняем проверку порядкового номера снятого кадра
		ASSERT_EQ(chunk.number, count);
		// Выполняем проверку содержимого снятого кадра
		ASSERT_EQ(string(content.begin(), content.end()), ("кадр номер " + to_string(count)));
		// Выполняем учёт снятого кадра
		count++;
	}
	// Выполняем проверку количества снятых кадров
	ASSERT_EQ(count, 5u);
}
/**
 * @brief Проверка отказов снятия кадра
 *
 */
TEST_F(ChunkFixture, Failures) {
	// Укладчик кадров
	abc::packer_t packer;
	// Укладываемое содержимое кадра
	const string payload = "содержимое кадра";
	// Буфер уложенного кадра
	vector <uint8_t> record;
	// Выполняем укладку кадра
	ASSERT_TRUE(packer.pack(payload.data(), payload.size(), abc::payload_t::TEXT, 1, 0, record));
	/**
	 * Выполняем перебор всех неполных подач кадра
	 */
	for(size_t size = 0; size < record.size(); size++){
		// Смещение снятия кадра
		size_t offset = 0;
		// Снятое содержимое кадра
		vector <uint8_t> content;
		// Снятые сведения о кадре
		abc::chunk_t chunk;
		// Выполняем проверку отказа снятия оборванного кадра
		ASSERT_FALSE(packer.unpack(record.data(), size, offset, content, chunk)) << "подано октетов: " << size;
		// Выполняем проверку кода отказа
		ASSERT_EQ(packer.error(), abc::error_t::TRUNCATED_CHUNK) << "подано октетов: " << size;
		// Выполняем проверку того, что смещение осталось нетронутым
		ASSERT_EQ(offset, 0u) << "подано октетов: " << size;
	}
	// Выполняем проверку отказа на неведомые разряды кадра
	{
		// Буфер повреждаемого кадра
		vector <uint8_t> damaged = record;
		// Выполняем установку неведомого разряда кадра
		damaged.at(1) = 0x80;
		// Смещение снятия кадра
		size_t offset = 0;
		// Снятое содержимое кадра
		vector <uint8_t> content;
		// Снятые сведения о кадре
		abc::chunk_t chunk;
		// Выполняем проверку отказа снятия кадра с неведомыми разрядами
		ASSERT_FALSE(packer.unpack(damaged.data(), damaged.size(), offset, content, chunk));
		// Выполняем проверку кода отказа
		ASSERT_EQ(packer.error(), abc::error_t::INVALID_CHUNK);
	}
	// Выполняем проверку отказа на расхождение объявленной длины с действительной
	{
		// Буфер повреждаемого кадра
		vector <uint8_t> damaged = record;
		// Выполняем порчу объявленной длины исходного содержимого
		damaged.at(8) = static_cast <uint8_t> (damaged.at(8) + 1);
		// Смещение снятия кадра
		size_t offset = 0;
		// Снятое содержимое кадра
		vector <uint8_t> content;
		// Снятые сведения о кадре
		abc::chunk_t chunk;
		// Выполняем проверку отказа снятия кадра с расходящейся длиной
		ASSERT_FALSE(packer.unpack(damaged.data(), damaged.size(), offset, content, chunk));
		// Выполняем проверку кода отказа
		ASSERT_EQ(packer.error(), abc::error_t::INVALID_CHUNK);
	}
	// Выполняем проверку отказа снятия отсутствующего кадра
	{
		// Смещение снятия кадра
		size_t offset = 0;
		// Снятое содержимое кадра
		vector <uint8_t> content;
		// Снятые сведения о кадре
		abc::chunk_t chunk;
		// Выполняем проверку отказа снятия отсутствующего кадра
		ASSERT_FALSE(packer.unpack(nullptr, abc::CHUNK_HEADER, offset, content, chunk));
		// Выполняем проверку кода отказа
		ASSERT_EQ(packer.error(), abc::error_t::INTERNAL);
	}
}
