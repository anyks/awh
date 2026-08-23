/**
 * @file container.cpp
 * @date 2026-08-19
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки сборки бинарного контейнера ABC целиком — заголовок с чередой кадров,
 *        порог накопления, смена вида содержимого, поточная подача и сохранность
 *        накопленного при отказе укладки
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
 * Подключаем системные заголовочные файлы
 */
#include <sys/mman.h>

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
	 * @brief Класс опоры проверок сборки контейнера
	 *
	 * @details Модуль сжатия и модуль шифрования заводит потребитель, а не кодек: у них
	 *          свои зависимости, и кодек их за потребителя не заводит
	 *
	 */
	class ContainerFixture : public testing::Test {
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
	/**
	 * @brief Функция вычитывания содержимого всех кадров контейнера
	 *
	 * @param loader сниматель контейнера
	 * @param count  количество снятых кадров
	 * @return       собранное содержимое всех кадров
	 *
	 */
	vector <uint8_t> drain(abc::loader_t & loader, size_t & count) noexcept {
		// Собираемое содержимое всех кадров
		vector <uint8_t> result;
		// Содержимое очередного снятого кадра
		vector <uint8_t> payload;
		// Сведения об очередном снятом кадре
		abc::chunk_t chunk;
		// Выполняем сброс количества снятых кадров
		count = 0;
		/**
		 * Выполняем вычитывание всех кадров контейнера
		 */
		while(loader.next(payload, chunk)){
			// Выполняем увеличение количества снятых кадров
			count++;
			// Выполняем внесение содержимого снятого кадра
			result.insert(result.end(), payload.begin(), payload.end());
		}
		// Выводим собранное содержимое всех кадров
		return result;
	}
};

/**
 * @brief Проверка кругового обхода контейнера без сжатия и шифрования
 *
 */
TEST_F(ContainerFixture, PlainRoundtrip) {
	// Сборщик контейнера
	abc::assembler_t assembler;
	// Ожидаемое содержимое тела контейнера
	vector <uint8_t> expected;
	/**
	 * Выполняем внесение трёх записей в собираемый контейнер
	 */
	for(const string & text : {string{"первая запись"}, string{"вторая запись"}, string{"третья запись"}}){
		// Выполняем сборку очередной записи
		const vector <uint8_t> item = record(text);
		// Выполняем внесение очередной записи в собираемый контейнер
		ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(assembler.error());
		// Выполняем накопление ожидаемого содержимого тела контейнера
		expected.insert(expected.end(), item.begin(), item.end());
	}
	// Буфер собранного контейнера
	vector <uint8_t> buffer;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(buffer)) << "код отказа: " << abc::message(assembler.error());
	// Выполняем проверку опознания собранного контейнера до загрузки тела
	ASSERT_TRUE(abc::probe(buffer.data(), buffer.size()));
	// Сниматель контейнера
	abc::loader_t loader;
	// Выполняем подачу собранного контейнера снимателю
	ASSERT_TRUE(loader.feed(buffer.data(), buffer.size()));
	// Количество снятых кадров
	size_t count = 0;
	// Выполняем вычитывание содержимого всех кадров контейнера
	const vector <uint8_t> payload = drain(loader, count);
	// Выполняем проверку снятости заголовка опознания контейнера
	ASSERT_TRUE(loader.ready());
	// Выполняем проверку количества записей, объявленных заголовком
	ASSERT_EQ(loader.header().records, 3ull);
	/**
	 * Выполняем проверку того, что оглавление лежит сразу за телом контейнера:
	 * длиною тела оно не считается, оттого и смещение его равно концу тела
	 */
	ASSERT_EQ(loader.header().index, static_cast <uint64_t> (abc::HEADER_LENGTH) + loader.header().length);
	// Выполняем проверку того, что тело контейнера короче собранного контейнера
	ASSERT_LT(loader.header().length, static_cast <uint64_t> (buffer.size() - abc::HEADER_LENGTH));
	// Выполняем проверку количества снятых кадров
	ASSERT_EQ(count, 1ul);
	// Выполняем проверку содержимого снятых кадров
	ASSERT_EQ(payload, expected);
}
/**
 * @brief Проверка сохранности объявленных заголовком свойств контейнера
 *
 */
TEST_F(ContainerFixture, HeaderProperties) {
	// Сборщик контейнера
	abc::assembler_t assembler;
	// Получаем настройки сборки контейнера
	abc::assembler_t::settings_t settings = assembler.settings();
	// Выполняем установку строгого вида записи
	settings.canonical = true;
	// Выполняем установку признака одного документа в теле контейнера
	settings.stream = false;
	// Выполняем установку вида содержимого контейнера
	settings.content = 0xA1B2C3D4;
	// Выполняем установку настроек сборки контейнера
	assembler.settings(settings);
	// Признак владельца собираемого контейнера
	const string owner = "ANYKS";
	// Выполняем установку признака владельца контейнера
	assembler.owner(owner.data(), owner.size());
	// Выполняем внесение записи в собираемый контейнер
	ASSERT_TRUE(assembler.append(abc::value_t(string{"значение"}), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(assembler.error());
	// Буфер собранного контейнера
	vector <uint8_t> buffer;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(buffer)) << "код отказа: " << abc::message(assembler.error());
	// Сниматель контейнера
	abc::loader_t loader;
	// Выполняем подачу собранного контейнера снимателю
	ASSERT_TRUE(loader.feed(buffer.data(), buffer.size()));
	// Количество снятых кадров
	size_t count = 0;
	// Выполняем вычитывание содержимого всех кадров контейнера
	const vector <uint8_t> payload = drain(loader, count);
	// Выполняем проверку вида содержимого контейнера
	ASSERT_EQ(loader.header().content, 0xA1B2C3D4u);
	// Выполняем проверку признака владельца контейнера
	ASSERT_EQ(::memcmp(loader.header().owner, owner.data(), owner.size()), 0);
	// Выполняем проверку объявления строгого вида записи
	ASSERT_TRUE(loader.header().is(abc::flag_t::CANONICAL));
	// Выполняем проверку того, что череда документов не объявлена
	ASSERT_FALSE(loader.header().is(abc::flag_t::STREAM));
	// Выполняем проверку того, что шифрование не объявлено
	ASSERT_FALSE(loader.header().is(abc::flag_t::ENCRYPTED));
	// Выполняем проверку снятого содержимого
	ASSERT_EQ(payload, abc::value_t(string{"значение"}).dump());
}
/**
 * @brief Проверка укладки кадров по порогу накопления записей
 *
 */
TEST_F(ContainerFixture, BlockThreshold) {
	// Сборщик контейнера
	abc::assembler_t assembler;
	// Получаем настройки сборки контейнера
	abc::assembler_t::settings_t settings = assembler.settings();
	// Выполняем установку порога накопления записей в один октет
	settings.block = 1;
	// Выполняем установку настроек сборки контейнера
	assembler.settings(settings);
	/**
	 * Выполняем внесение трёх записей в собираемый контейнер
	 */
	for(const string & text : {string{"раз"}, string{"два"}, string{"три"}}){
		// Выполняем внесение очередной записи в собираемый контейнер
		ASSERT_TRUE(assembler.append(abc::value_t(text), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(assembler.error());
		// Выполняем проверку того, что накопленное уложено кадром сразу
		ASSERT_EQ(assembler.pending(), 0ul);
	}
	// Буфер собранного контейнера
	vector <uint8_t> buffer;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(buffer)) << "код отказа: " << abc::message(assembler.error());
	// Сниматель контейнера
	abc::loader_t loader;
	// Выполняем подачу собранного контейнера снимателю
	ASSERT_TRUE(loader.feed(buffer.data(), buffer.size()));
	// Количество снятых кадров
	size_t count = 0;
	// Выполняем вычитывание содержимого всех кадров контейнера
	drain(loader, count);
	// Выполняем проверку того, что всякая запись легла своим кадром
	ASSERT_EQ(count, 3ul);
	// Выполняем проверку количества записей, объявленных заголовком
	ASSERT_EQ(loader.header().records, 3ull);
}
/**
 * @brief Проверка укладки накопленного при смене вида содержимого
 *
 * @details Кадр вперемешку отнял бы у подбора метода сжатия всякий смысл, оттого смена
 *          вида содержимого обязана укладывать накопленное, не дожидаясь порога
 *
 */
TEST_F(ContainerFixture, PayloadKindSwitch) {
	// Сборщик контейнера
	abc::assembler_t assembler;
	// Выполняем сборку записи знакового текста
	const vector <uint8_t> text = record("знаковый текст");
	// Выполняем внесение записи знакового текста
	ASSERT_TRUE(assembler.append(text.data(), text.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(assembler.error());
	// Выполняем проверку того, что запись накоплена, а не уложена
	ASSERT_EQ(assembler.pending(), text.size());
	// Выполняем внесение записи сырых октетов
	ASSERT_TRUE(assembler.append(text.data(), text.size(), abc::payload_t::BINARY))
		<< "код отказа: " << abc::message(assembler.error());
	/**
	 * Выполняем проверку того, что смена вида содержимого уложила накопленное,
	 * оставив в накоплении лишь запись нового вида
	 */
	ASSERT_EQ(assembler.pending(), text.size());
	// Буфер собранного контейнера
	vector <uint8_t> buffer;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(buffer)) << "код отказа: " << abc::message(assembler.error());
	// Сниматель контейнера
	abc::loader_t loader;
	// Выполняем подачу собранного контейнера снимателю
	ASSERT_TRUE(loader.feed(buffer.data(), buffer.size()));
	// Количество снятых кадров
	size_t count = 0;
	// Выполняем вычитывание содержимого всех кадров контейнера
	drain(loader, count);
	// Выполняем проверку того, что вышло два кадра, а не один
	ASSERT_EQ(count, 2ul);
}
/**
 * @brief Проверка поточной подачи контейнера по одному октету
 *
 * @details Подача по октету ловит всякое место, где разбор решил бы, что октетов ему
 *          довольно: кадр, поданный не целиком, обязан дожидаться недостающих октетов,
 *          не сдвигая смещения разбора
 *
 */
TEST_F(ContainerFixture, StreamingFeed) {
	// Сборщик контейнера
	abc::assembler_t assembler;
	// Получаем настройки сборки контейнера
	abc::assembler_t::settings_t settings = assembler.settings();
	// Выполняем установку порога накопления записей в один октет
	settings.block = 1;
	// Выполняем установку настроек сборки контейнера
	assembler.settings(settings);
	// Ожидаемое содержимое тела контейнера
	vector <uint8_t> expected;
	/**
	 * Выполняем внесение четырёх записей в собираемый контейнер
	 */
	for(const string & text : {string{"один"}, string{"два"}, string{"три"}, string{"четыре"}}){
		// Выполняем сборку очередной записи
		const vector <uint8_t> item = record(text);
		// Выполняем внесение очередной записи в собираемый контейнер
		ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(assembler.error());
		// Выполняем накопление ожидаемого содержимого тела контейнера
		expected.insert(expected.end(), item.begin(), item.end());
	}
	// Буфер собранного контейнера
	vector <uint8_t> buffer;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(buffer)) << "код отказа: " << abc::message(assembler.error());
	// Сниматель контейнера
	abc::loader_t loader;
	// Собираемое содержимое всех кадров
	vector <uint8_t> payload;
	// Количество снятых кадров
	size_t count = 0;
	/**
	 * Выполняем подачу собранного контейнера по одному октету
	 */
	for(size_t i = 0; i < buffer.size(); i++){
		// Выполняем подачу очередного октета контейнера
		ASSERT_TRUE(loader.feed(buffer.data() + i, 1));
		// Содержимое очередного снятого кадра
		vector <uint8_t> chunked;
		// Сведения об очередном снятом кадре
		abc::chunk_t chunk;
		/**
		 * Выполняем вычитывание всех кадров, какие набрались подачей
		 */
		while(loader.next(chunked, chunk)){
			// Выполняем увеличение количества снятых кадров
			count++;
			// Выполняем внесение содержимого снятого кадра
			payload.insert(payload.end(), chunked.begin(), chunked.end());
		}
		/**
		 * Выполняем проверку того, что недостача октетов отвечена именно недостачей,
		 * а не отказом разбора: иначе поточная подача сорвалась бы на первом же кадре
		 */
		ASSERT_TRUE((loader.error() == abc::error_t::NONE) ||
		 (loader.error() == abc::error_t::TRUNCATED_HEADER) ||
		 (loader.error() == abc::error_t::TRUNCATED_CHUNK))
			<< "код отказа: " << abc::message(loader.error()) << " на октете " << i;
	}
	// Выполняем проверку количества снятых кадров
	ASSERT_EQ(count, 4ul);
	// Выполняем проверку содержимого снятых кадров
	ASSERT_EQ(payload, expected);
}
/**
 * @brief Проверка кругового обхода контейнера со сжатием и шифрованием
 *
 */
TEST_F(ContainerFixture, SecuredRoundtrip) {
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
	// Укладываемое содержимое записи
	string text;
	/**
	 * Выполняем сборку хорошо сжимаемого содержимого записи
	 */
	for(size_t i = 0; i < 64; i++)
		// Выполняем добавление очередного повторения
		text.append("повторяющийся текст, какой сожмётся отменно; ");
	// Выполняем сборку записи из собранного содержимого
	const vector <uint8_t> item = record(text);
	// Выполняем внесение записи в собираемый контейнер
	ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(assembler.error());
	// Буфер собранного контейнера
	vector <uint8_t> buffer;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(buffer)) << "код отказа: " << abc::message(assembler.error());
	/**
	 * Выполняем проверку того, что собранный контейнер вышел меньше записи:
	 * без сжатия он был бы больше её на заголовки
	 */
	ASSERT_LT(buffer.size(), item.size());
	// Сниматель контейнера
	abc::loader_t loader;
	// Выполняем установку модуля сжатия снимателю контейнера
	loader.compressor(this->_compressor.get());
	// Выполняем установку модуля шифрования снимателю контейнера
	loader.crypto(this->_crypto.get());
	// Выполняем подачу собранного контейнера снимателю
	ASSERT_TRUE(loader.feed(buffer.data(), buffer.size()));
	// Количество снятых кадров
	size_t count = 0;
	// Выполняем вычитывание содержимого всех кадров контейнера
	const vector <uint8_t> payload = drain(loader, count);
	// Выполняем проверку объявления сжатости тела контейнера
	ASSERT_TRUE(loader.header().is(abc::flag_t::COMPRESSED));
	// Выполняем проверку объявления зашифрованности тела контейнера
	ASSERT_TRUE(loader.header().is(abc::flag_t::ENCRYPTED));
	// Выполняем проверку количества снятых кадров
	ASSERT_EQ(count, 1ul);
	// Выполняем проверку снятого содержимого
	ASSERT_EQ(payload, item);
}
/**
 * @brief Проверка сохранности накопленного при отказе укладки
 *
 * @details Решение владельца: отказ укладки накопленного не сбрасывает. Причина отказа
 *          может быть устранена, и следующая попытка обязана пройти по тем же данным
 *
 */
TEST_F(ContainerFixture, FailureKeepsPending) {
	// Сборщик контейнера
	abc::assembler_t assembler;
	// Получаем настройки укладки кадра
	abc::packer_t::settings_t packing = assembler.packer().settings();
	// Выполняем установку признака шифрования содержимого кадра
	packing.encrypt = true;
	/**
	 * Выполняем установку настроек укладки кадра, не отдав модуля шифрования:
	 * шифровать нечем, и укладка кадра обязана ответить отказом
	 */
	assembler.packer().settings(packing);
	// Выполняем сборку записи
	const vector <uint8_t> item = record("накопленная запись");
	// Выполняем внесение записи в собираемый контейнер
	ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(assembler.error());
	// Выполняем проверку отказа укладки накопленного кадром
	ASSERT_FALSE(assembler.flush());
	// Выполняем проверку кода отказа укладки кадра
	ASSERT_EQ(assembler.error(), abc::error_t::ENCRYPTION_FAILED);
	// Выполняем проверку того, что накопленное отказом не сброшено
	ASSERT_EQ(assembler.pending(), item.size());
	// Выполняем установку модуля шифрования сборщику контейнера
	assembler.crypto(this->_crypto.get());
	// Буфер собранного контейнера
	vector <uint8_t> buffer;
	/**
	 * Выполняем завершение сборки контейнера: причина отказа устранена, и
	 * накопленное обязано лечь кадром без потерь
	 */
	ASSERT_TRUE(assembler.complete(buffer)) << "код отказа: " << abc::message(assembler.error());
	// Сниматель контейнера
	abc::loader_t loader;
	// Выполняем установку модуля шифрования снимателю контейнера
	loader.crypto(this->_crypto.get());
	// Выполняем подачу собранного контейнера снимателю
	ASSERT_TRUE(loader.feed(buffer.data(), buffer.size()));
	// Количество снятых кадров
	size_t count = 0;
	// Выполняем вычитывание содержимого всех кадров контейнера
	const vector <uint8_t> payload = drain(loader, count);
	// Выполняем проверку количества снятых кадров
	ASSERT_EQ(count, 1ul);
	// Выполняем проверку снятого содержимого
	ASSERT_EQ(payload, item);
}
/**
 * @brief Проверка остановки снятия по объявленной заголовком длине тела
 *
 * @details За телом лягут оглавление и подпись, и снятие обязано остановиться по
 *          объявленной длине тела, а не по концу поданных октетов: иначе оглавление
 *          пошло бы в разбор кадром
 *
 */
TEST_F(ContainerFixture, BodyLengthStopsTaking) {
	// Сборщик контейнера
	abc::assembler_t assembler;
	// Выполняем сборку записи
	const vector <uint8_t> item = record("запись тела");
	// Выполняем внесение записи в собираемый контейнер
	ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(assembler.error());
	// Буфер собранного контейнера
	vector <uint8_t> buffer;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(buffer)) << "код отказа: " << abc::message(assembler.error());
	// Октеты, положенные за телом контейнера видом оглавления
	const vector <uint8_t> trailer(64, 0xA5);
	// Выполняем добавление октетов за телом собранного контейнера
	buffer.insert(buffer.end(), trailer.begin(), trailer.end());
	// Сниматель контейнера
	abc::loader_t loader;
	// Выполняем подачу собранного контейнера снимателю
	ASSERT_TRUE(loader.feed(buffer.data(), buffer.size()));
	// Количество снятых кадров
	size_t count = 0;
	// Выполняем вычитывание содержимого всех кадров контейнера
	const vector <uint8_t> payload = drain(loader, count);
	// Выполняем проверку того, что снят один кадр, а октеты за телом в разбор не пошли
	ASSERT_EQ(count, 1ul);
	// Выполняем проверку того, что снятие остановлено без отказа разбора
	ASSERT_EQ(loader.error(), abc::error_t::NONE);
	// Выполняем проверку снятого содержимого
	ASSERT_EQ(payload, item);
}
/**
 * @brief Проверка отказа снятия чужих октетов
 *
 */
TEST_F(ContainerFixture, ForeignOctets) {
	// Сниматель контейнера
	abc::loader_t loader;
	// Чужие октеты, поданные снимателю контейнера
	const string foreign = "это вовсе не контейнер, а обыкновенный текст, длиною поболее заголовка опознания";
	// Выполняем подачу чужих октетов снимателю
	ASSERT_TRUE(loader.feed(foreign.data(), foreign.size()));
	// Содержимое очередного снятого кадра
	vector <uint8_t> payload;
	// Сведения об очередном снятом кадре
	abc::chunk_t chunk;
	// Выполняем проверку отказа снятия чужих октетов
	ASSERT_FALSE(loader.next(payload, chunk));
	// Выполняем проверку кода отказа снятия чужих октетов
	ASSERT_EQ(loader.error(), abc::error_t::INVALID_MAGIC);
	// Выполняем проверку того, что заголовок опознания снятым не объявлен
	ASSERT_FALSE(loader.ready());
}
/**
 * @brief Проверка работы контейнера без сжатия и шифрования вовсе
 *
 * @details Решение владельца: сжатие и шифрование суть настройки, а не обязательная
 *          часть работы. Контейнер, собранный без обоих модулей, обязан и собираться, и
 *          читаться, и выбираться по номеру - модулей ему не отдано вовсе
 *
 */
TEST_F(ContainerFixture, ModulesAreOptional) {
	// Сборщик контейнера, какому не отдано ни сжатия, ни шифрования
	abc::assembler_t assembler;
	// Получаем настройки сборки контейнера
	abc::assembler_t::settings_t settings = assembler.settings();
	// Выполняем установку порога накопления, дающего несколько кадров
	settings.block = 32;
	// Выполняем установку настроек сборки контейнера
	assembler.settings(settings);
	// Внесённые в контейнер записи
	vector <vector <uint8_t>> records;
	/**
	 * Выполняем внесение записей в собираемый контейнер
	 */
	for(size_t i = 0; i < 6; i++){
		// Выполняем сборку очередной записи
		const vector <uint8_t> item = record(string{"запись без сжатия и шифрования "} + to_string(i));
		// Выполняем внесение очередной записи в собираемый контейнер
		ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
			<< "код отказа: " << abc::message(assembler.error());
		// Выполняем накопление внесённой записи
		records.push_back(item);
	}
	// Буфер собранного контейнера
	vector <uint8_t> buffer;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(buffer)) << "код отказа: " << abc::message(assembler.error());
	// Сниматель контейнера, какому не отдано ни сжатия, ни шифрования
	abc::loader_t loader;
	// Выполняем подачу собранного контейнера снимателю
	ASSERT_TRUE(loader.feed(buffer.data(), buffer.size()));
	// Количество снятых кадров
	size_t count = 0;
	// Выполняем вычитывание содержимого всех кадров контейнера
	const vector <uint8_t> payload = drain(loader, count);
	// Выполняем проверку того, что сжатие контейнером не объявлено
	ASSERT_FALSE(loader.header().is(abc::flag_t::COMPRESSED));
	// Выполняем проверку того, что шифрование контейнером не объявлено
	ASSERT_FALSE(loader.header().is(abc::flag_t::ENCRYPTED));
	// Собираемое ожидаемое содержимое кадров контейнера
	vector <uint8_t> expected;
	// Выполняем перебор всех внесённых записей
	for(const vector <uint8_t> & item : records)
		// Выполняем накопление ожидаемого содержимого кадров
		expected.insert(expected.end(), item.begin(), item.end());
	// Выполняем проверку содержимого снятых кадров
	ASSERT_EQ(payload, expected);
	// Выборщик записей контейнера, какому не отдано ни сжатия, ни шифрования
	abc::fetcher_t fetcher;
	// Выполняем открытие контейнера отданной работой чтения
	ASSERT_TRUE(fetcher.open([&buffer](const uint64_t offset, const size_t size, vector <uint8_t> & result) noexcept -> bool {
		// Если затребованные октеты за концом контейнера
		if((offset + size) > static_cast <uint64_t> (buffer.size()))
			// Выводим признак неудачного чтения октетов
			return false;
		// Выполняем выдачу затребованных октетов
		result.assign(buffer.begin() + static_cast <ptrdiff_t> (offset),
		 buffer.begin() + static_cast <ptrdiff_t> (offset + size));
		// Выводим признак успешного чтения октетов
		return true;
	})) << "код отказа: " << abc::message(fetcher.error());
	// Буфер выбранной записи контейнера
	vector <uint8_t> picked;
	// Выполняем выборку последней записи контейнера
	ASSERT_TRUE(fetcher.record(records.size() - 1, picked)) << "код отказа: " << abc::message(fetcher.error());
	// Выполняем проверку выбранной записи контейнера
	ASSERT_EQ(picked, records.back());
}
/**
 * @brief Проверка отказа снятия кадра, объявившего длину за телом контейнера
 *
 * @details Длина кадра сличается снимателем кадра лишь с поданными октетами, а тело
 *          контейнера кончается ранее их: за телом лежат оглавление и подпись. Кадр,
 *          объявивший длину сверх тела, вобрал бы оглавление содержимым записей, и
 *          потребитель принял бы его за содержимое контейнера
 *
 */
TEST_F(ContainerFixture, ChunkBeyondBody) {
	// Сборщик контейнера
	abc::assembler_t assembler;
	// Выполняем сборку записи
	const vector <uint8_t> item = record("запись тела контейнера");
	// Выполняем внесение записи в собираемый контейнер
	ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(assembler.error());
	// Буфер собранного контейнера
	vector <uint8_t> pristine;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(pristine)) << "код отказа: " << abc::message(assembler.error());
	// Снятый заголовок опознания контейнера
	abc::header_t header;
	// Код отказа снятия заголовка
	abc::error_t error = abc::error_t::NONE;
	// Выполняем снятие заголовка опознания контейнера
	ASSERT_TRUE(header.unpack(pristine.data(), pristine.size(), error))
		<< "код отказа: " << abc::message(error);
	/**
	 * Выполняем проверку того, что за телом контейнера лежат октеты оглавления:
	 * без них кадру нечего было бы вобрать, и проверка прошла бы вхолостую
	 */
	ASSERT_GT(pristine.size(), abc::HEADER_LENGTH + static_cast <size_t> (header.length));
	// Выполняем получение количества октетов, лежащих за телом контейнера
	const size_t trailer = pristine.size() - (abc::HEADER_LENGTH + static_cast <size_t> (header.length));
	// Выполняем проверку того, что тело содержит единый кадр
	ASSERT_EQ(static_cast <size_t> (header.length), abc::CHUNK_HEADER + (item.size()));
	// Октеты правимой записи контейнера
	vector <uint8_t> damaged = pristine;
	/**
	 * Выполняем удлинение объявленной длины единственного кадра тела на октеты,
	 * лежащие за телом контейнера
	 */
	{
		// Выполняем получение объявленной длины содержимого кадра
		const uint32_t length = static_cast <uint32_t> (item.size()) + static_cast <uint32_t> (trailer);
		/**
		 * Выполняем перебор всех октетов поля длины содержимого кадра
		 */
		for(size_t i = 0; i < 4; i++)
			// Выполняем укладку очередного октета поля
			damaged.at(abc::HEADER_LENGTH + 4 + i) = static_cast <uint8_t> ((length >> (i * 8)) & 0xFF);
		/**
		 * Выполняем удлинение объявленной длины исходного содержимого кадра: без
		 * этого кадр отвечался бы отказом опознания по расхождению длин
		 */
		for(size_t i = 0; i < 4; i++)
			// Выполняем укладку очередного октета поля
			damaged.at(abc::HEADER_LENGTH + 8 + i) = static_cast <uint8_t> ((length >> (i * 8)) & 0xFF);
	}
	// Сниматель контейнера
	abc::loader_t loader;
	// Выполняем подачу правленого контейнера снимателю
	ASSERT_TRUE(loader.feed(damaged.data(), damaged.size()));
	// Содержимое снятого кадра
	vector <uint8_t> payload;
	// Сведения о снятом кадре
	abc::chunk_t chunk;
	// Выполняем проверку отказа снятия кадра, объявившего длину за телом
	ASSERT_FALSE(loader.next(payload, chunk))
		<< "кадр вобрал октеты за телом контейнера, снято " << payload.size() << " при теле " << header.length;
	// Выполняем проверку кода отказа снятия кадра
	ASSERT_EQ(loader.error(), abc::error_t::INVALID_CHUNK);
	/**
	 * Выполняем проверку того, что нетронутый контейнер снимается по-прежнему
	 */
	{
		// Сниматель контейнера
		abc::loader_t loader;
		// Выполняем подачу нетронутого контейнера снимателю
		ASSERT_TRUE(loader.feed(pristine.data(), pristine.size()));
		// Количество снятых кадров
		size_t count = 0;
		// Выполняем вычитывание содержимого всех кадров контейнера
		const vector <uint8_t> payload = drain(loader, count);
		// Выполняем проверку количества снятых кадров
		ASSERT_EQ(count, 1ul);
		// Выполняем проверку снятого содержимого
		ASSERT_EQ(payload, item);
	}
}
/**
 * @brief Проверка отказа внесения записи, не вмещающейся в строку оглавления
 *
 * @details Смещение и длина записи ложатся в оглавление разрядностью 32, и приведение
 *          к ней усекает молча: строка указала бы не туда, а контейнер вышел бы с виду
 *          годным. Сторож стоит до накопления поданного, потому и доказывается он
 *          нетронутым отображением памяти: ни единой страницы оно не занимает
 *
 * @note Проверка эта запрашивает у системы 4 ГБ памяти отображением, но не касается
 *       его ни разу. Занятой памяти отсюда не прибывает: страницы заводятся при
 *       первом обращении, а отказ обязан прийти прежде всякого обращения
 *
 */
TEST_F(ContainerFixture, RecordBeyondEntryField) {
	/**
	 * Если разрядность размера системы полю строки оглавления не шире, усечению
	 * взяться неоткуда: сторож на такой сборке недостижим по устройству
	 */
	if(numeric_limits <size_t>::max() <= static_cast <size_t> (numeric_limits <uint32_t>::max())){
		// Выводим сообщение о недостижимости сторожа на этой сборке
		GTEST_SKIP() << "разрядность размера системы не шире поля строки оглавления";
		// Выходим из проверки
		return;
	}
	// Размер запрашиваемого отображения памяти
	const size_t size = (static_cast <size_t> (numeric_limits <uint32_t>::max()) + 1);
	// Выполняем запрос отображения памяти у системы
	void * buffer = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	/**
	 * Если отображения памяти система не дала, проверить сторожа нечем
	 */
	if(buffer == MAP_FAILED){
		// Выводим сообщение об отказе системы в отображении памяти
		GTEST_SKIP() << "система не дала отображения памяти размером " << size << " октетов";
		// Выходим из проверки
		return;
	}
	// Сборщик контейнера
	abc::assembler_t assembler;
	// Выполняем проверку отказа внесения записи, не вмещающейся в строку оглавления
	const bool appended = assembler.append(buffer, size, abc::payload_t::BINARY);
	// Код отказа внесения записи
	const abc::error_t error = assembler.error();
	// Количество уложенных записей
	const uint64_t records = assembler.records();
	// Размер накопленных записей
	const size_t pending = assembler.pending();
	// Выполняем возврат отображения памяти системе
	::munmap(buffer, size);
	// Выполняем проверку отказа внесения записи
	ASSERT_FALSE(appended) << "запись длиною " << size << " внесена усечённою";
	// Выполняем проверку кода отказа внесения записи
	ASSERT_EQ(error, abc::error_t::INVALID_LENGTH);
	// Выполняем проверку того, что отказ ничего не накопил
	ASSERT_EQ(pending, 0ul);
	// Выполняем проверку того, что отказ записи не засчитал
	ASSERT_EQ(records, 0u);
}
