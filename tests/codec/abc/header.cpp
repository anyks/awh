/**
 * @file header.cpp
 * @date 2026-08-19
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки заголовка опознания бинарного контейнера ABC — круговой обход укладки,
 *        опознание до загрузки, стойкость к порче и закрепление раскладки октетов
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

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
	 * @brief Функция извлечения объекта журнала проверок
	 *
	 * @details Журнал заводится единожды на весь набор и гасится: проверки отказов
	 *          выводили бы записью всякий свой отказ, а их тут большинство. Гашение
	 *          это - настройка журнала, а не молчание модуля: модуль доносит как
	 *          обычно, а показывать ли - решает журнал
	 *
	 * @return объект журнала проверок
	 *
	 */
	[[maybe_unused]] const log_t * logger() noexcept {
		// Объект фреймворка проверок
		static fmk_t fmk;
		// Объект журнала проверок
		static log_t log(& fmk);
		// Признак выполненной настройки журнала
		static const bool ready = [](){
			// Выполняем гашение вывода журнала проверок
			log.level(log_t::level_t::NONE);
			// Выводим признак выполненной настройки
			return true;
		}();
		// Снимаем неиспользуемый признак настройки
		(void) ready;
		// Выводим объект журнала проверок
		return & log;
	}
	/**
	 * @brief Функция сборки заголовка для проверок
	 *
	 * @return собранный заголовок опознания
	 *
	 */
	abc::header_t assemble() noexcept {
		// Собираемый заголовок опознания
		abc::header_t header;
		// Выполняем установку вида содержимого контейнера
		header.content = 0x11223344;
		// Выполняем установку длины тела контейнера
		header.length = 0x0102030405060708ull;
		// Выполняем установку количества записей в теле
		header.records = 4242;
		// Выполняем установку смещения оглавления
		header.index = 777;
		// Выполняем установку смещения подписи
		header.signature = 888;
		/**
		 * Выполняем установку признака владельца контейнера
		 */
		for(size_t i = 0; i < abc::OWNER_LENGTH; i++)
			// Выполняем установку очередного октета признака владельца
			header.owner[i] = static_cast <uint8_t> (0xA0 + i);
		/**
		 * Выполняем установку отпечатка открытого ключа
		 */
		for(size_t i = 0; i < abc::FINGERPRINT_LENGTH; i++)
			// Выполняем установку очередного октета отпечатка
			header.fingerprint[i] = static_cast <uint8_t> (0x50 + i);
		// Выполняем объявление строгого вида записи тела
		header.set(abc::flag_t::CANONICAL, true);
		// Выполняем объявление сжатия тела
		header.set(abc::flag_t::COMPRESSED, true);
		// Выводим собранный заголовок опознания
		return header;
	}
};

/**
 * @brief Проверка кругового обхода укладки заголовка
 *
 */
TEST(CodecAbcHeader, Roundtrip) {
	// Собранный заголовок опознания
	const abc::header_t header = assemble();
	// Буфер уложенного заголовка
	vector <uint8_t> record;
	// Выполняем укладку заголовка в октеты
	header.pack(record);
	// Выполняем проверку длины уложенного заголовка
	ASSERT_EQ(record.size(), abc::HEADER_LENGTH);
	// Снимаемый заголовок опознания
	abc::header_t restored;
	// Код отказа снятия заголовка
	abc::error_t error = abc::error_t::NONE;
	// Выполняем снятие заголовка с октетов
	ASSERT_TRUE(restored.unpack(record.data(), record.size(), error))
		<< "код отказа: " << abc::message(error);
	// Выполняем проверку старшей версии вида записи
	ASSERT_EQ(restored.version, abc::VERSION_MAJOR);
	// Выполняем проверку младшей версии вида записи
	ASSERT_EQ(restored.revision, abc::VERSION_MINOR);
	// Выполняем проверку разрядов свойств контейнера
	ASSERT_EQ(restored.flags, header.flags);
	// Выполняем проверку вида содержимого контейнера
	ASSERT_EQ(restored.content, header.content);
	// Выполняем проверку длины тела контейнера
	ASSERT_EQ(restored.length, header.length);
	// Выполняем проверку количества записей в теле
	ASSERT_EQ(restored.records, header.records);
	// Выполняем проверку смещения оглавления
	ASSERT_EQ(restored.index, header.index);
	// Выполняем проверку смещения подписи
	ASSERT_EQ(restored.signature, header.signature);
	// Выполняем проверку признака владельца контейнера
	ASSERT_EQ(::memcmp(restored.owner, header.owner, abc::OWNER_LENGTH), 0);
	// Выполняем проверку отпечатка открытого ключа
	ASSERT_EQ(::memcmp(restored.fingerprint, header.fingerprint, abc::FINGERPRINT_LENGTH), 0);
	// Выполняем проверку объявленного строгого вида записи
	ASSERT_TRUE(restored.is(abc::flag_t::CANONICAL));
	// Выполняем проверку объявленного сжатия тела
	ASSERT_TRUE(restored.is(abc::flag_t::COMPRESSED));
	// Выполняем проверку того, что шифрование не объявлено
	ASSERT_FALSE(restored.is(abc::flag_t::ENCRYPTED));
}
/**
 * @brief Проверка объявления и снятия свойств контейнера
 *
 */
TEST(CodecAbcHeader, Flags) {
	// Заголовок опознания
	abc::header_t header;
	// Выполняем проверку того, что свойств не объявлено
	ASSERT_EQ(header.flags, static_cast <uint16_t> (abc::flag_t::NONE));
	// Выполняем объявление шифрования тела
	header.set(abc::flag_t::ENCRYPTED, true);
	// Выполняем объявление подписи контейнера
	header.set(abc::flag_t::SIGNED, true);
	// Выполняем проверку объявленного шифрования тела
	ASSERT_TRUE(header.is(abc::flag_t::ENCRYPTED));
	// Выполняем проверку объявленной подписи контейнера
	ASSERT_TRUE(header.is(abc::flag_t::SIGNED));
	// Выполняем снятие объявленного шифрования тела
	header.set(abc::flag_t::ENCRYPTED, false);
	// Выполняем проверку снятого шифрования тела
	ASSERT_FALSE(header.is(abc::flag_t::ENCRYPTED));
	// Выполняем проверку того, что подпись осталась объявленной
	ASSERT_TRUE(header.is(abc::flag_t::SIGNED));
}
/**
 * @brief Проверка опознания контейнера до загрузки
 *
 */
TEST(CodecAbcHeader, Probe) {
	// Собранный заголовок опознания
	const abc::header_t header = assemble();
	// Буфер уложенного заголовка
	vector <uint8_t> record;
	// Выполняем укладку заголовка в октеты
	header.pack(record);
	// Выполняем добавление тела за заголовком
	record.insert(record.end(), {0x01, 0x02, 0x03});
	// Выполняем проверку опознания контейнера
	ASSERT_TRUE(abc::probe(record.data(), record.size()));
	// Выполняем проверку опознания по одному лишь заголовку
	ASSERT_TRUE(abc::probe(record.data(), abc::HEADER_LENGTH));
	// Выполняем проверку отказа опознания при недостаче октетов
	ASSERT_FALSE(abc::probe(record.data(), abc::HEADER_LENGTH - 1));
	// Выполняем проверку отказа опознания пустого буфера
	ASSERT_FALSE(abc::probe(nullptr, 0));
	// Буфер чужой записи
	const vector <uint8_t> foreign(abc::HEADER_LENGTH, 0x00);
	// Выполняем проверку отказа опознания чужой записи
	ASSERT_FALSE(abc::probe(foreign.data(), foreign.size()));
	// Буфер записи JSON
	const string json = "{\"кто\": \"не контейнер\", \"а\": \"текст в кодировке UTF-8\"}";
	// Выполняем проверку отказа опознания записи JSON
	ASSERT_FALSE(abc::probe(json.data(), json.size()));
}
/**
 * @brief Проверка стойкости заголовка к порче
 *
 * @details Порча любого октета заголовка обязана быть замечена: смещения из
 * повреждённого заголовка увели бы чтение в произвольное место записи
 *
 */
TEST(CodecAbcHeader, Corruption) {
	// Собранный заголовок опознания
	const abc::header_t header = assemble();
	// Буфер уложенного заголовка
	vector <uint8_t> record;
	// Выполняем укладку заголовка в октеты
	header.pack(record);
	/**
	 * Выполняем перебор всех октетов уложенного заголовка
	 */
	for(size_t i = 0; i < record.size(); i++){
		// Буфер повреждаемого заголовка
		vector <uint8_t> damaged = record;
		// Выполняем порчу очередного октета заголовка
		damaged.at(i) = static_cast <uint8_t> (damaged.at(i) ^ 0x01);
		// Снимаемый заголовок опознания
		abc::header_t restored;
		// Код отказа снятия заголовка
		abc::error_t error = abc::error_t::NONE;
		// Выполняем проверку отказа снятия повреждённого заголовка
		ASSERT_FALSE(restored.unpack(damaged.data(), damaged.size(), error)) << "испорчен октет: " << i;
		// Если испорчена опознавательная запись контейнера
		if(i < 4)
			// Выполняем проверку кода отказа опознания
			ASSERT_EQ(error, abc::error_t::INVALID_MAGIC) << "испорчен октет: " << i;
		// Выполняем проверку кода отказа контрольной суммы
		else ASSERT_EQ(error, abc::error_t::INVALID_CHECKSUM) << "испорчен октет: " << i;
		// Выполняем проверку отказа опознания повреждённого заголовка
		ASSERT_FALSE(abc::probe(damaged.data(), damaged.size())) << "испорчен октет: " << i;
	}
}
/**
 * @brief Проверка отказов снятия заголовка
 *
 */
TEST(CodecAbcHeader, Failures) {
	// Собранный заголовок опознания
	const abc::header_t header = assemble();
	// Буфер уложенного заголовка
	vector <uint8_t> record;
	// Выполняем укладку заголовка в октеты
	header.pack(record);
	// Снимаемый заголовок опознания
	abc::header_t restored;
	// Код отказа снятия заголовка
	abc::error_t error = abc::error_t::NONE;
	// Выполняем проверку отказа снятия оборванного заголовка
	ASSERT_FALSE(restored.unpack(record.data(), abc::HEADER_LENGTH - 1, error));
	// Выполняем проверку кода отказа
	ASSERT_EQ(error, abc::error_t::TRUNCATED_HEADER);
	// Выполняем проверку отказа снятия отсутствующего заголовка
	ASSERT_FALSE(restored.unpack(nullptr, abc::HEADER_LENGTH, error));
	// Выполняем проверку кода отказа
	ASSERT_EQ(error, abc::error_t::INTERNAL);
	/**
	 * Выполняем проверку отказа на неподдерживаемый вид записи
	 */
	{
		// Заголовок опознания иного вида записи
		abc::header_t future = assemble();
		// Выполняем установку старшей версии вида записи из будущего
		future.version = static_cast <uint8_t> (abc::VERSION_MAJOR + 1);
		// Буфер уложенного заголовка
		vector <uint8_t> record;
		// Выполняем укладку заголовка в октеты
		future.pack(record);
		// Выполняем проверку того, что контейнер опознаётся как наш
		ASSERT_TRUE(abc::probe(record.data(), record.size()));
		// Выполняем проверку отказа снятия заголовка иного вида записи
		ASSERT_FALSE(restored.unpack(record.data(), record.size(), error));
		// Выполняем проверку кода отказа
		ASSERT_EQ(error, abc::error_t::INVALID_VERSION);
	}
}
/**
 * @brief Проверка закрепления раскладки октетов заголовка
 *
 * @details Раскладка эта есть договор с носителем: смена её обратила бы прежние
 * контейнеры в нечитаемые, и оттого всякое её изменение обязано быть намеренным
 *
 */
TEST(CodecAbcHeader, Layout) {
	// Выполняем проверку длины заголовка опознания
	ASSERT_EQ(abc::HEADER_LENGTH, 96u);
	// Выполняем проверку длины признака владельца контейнера
	ASSERT_EQ(abc::OWNER_LENGTH, 16u);
	// Выполняем проверку длины отпечатка открытого ключа
	ASSERT_EQ(abc::FINGERPRINT_LENGTH, 16u);
	// Собранный заголовок опознания
	const abc::header_t header = assemble();
	// Буфер уложенного заголовка
	vector <uint8_t> record;
	// Выполняем укладку заголовка в октеты
	header.pack(record);
	// Выполняем проверку опознавательной записи контейнера
	ASSERT_EQ(record.at(0), 'A');
	// Выполняем проверку второго октета опознавательной записи
	ASSERT_EQ(record.at(1), 'B');
	// Выполняем проверку третьего октета опознавательной записи
	ASSERT_EQ(record.at(2), 'C');
	// Выполняем проверку четвёртого октета опознавательной записи
	ASSERT_EQ(record.at(3), 0x00);
	// Выполняем проверку места старшей версии вида записи
	ASSERT_EQ(record.at(4), abc::VERSION_MAJOR);
	// Выполняем проверку места младшей версии вида записи
	ASSERT_EQ(record.at(5), abc::VERSION_MINOR);
	// Выполняем проверку места разрядов свойств контейнера
	ASSERT_EQ(record.at(6), static_cast <uint8_t> (header.flags & 0xFF));
	// Выполняем проверку места признака владельца контейнера
	ASSERT_EQ(record.at(8), header.owner[0]);
	// Выполняем проверку места вида содержимого контейнера
	ASSERT_EQ(record.at(24), static_cast <uint8_t> (header.content & 0xFF));
	// Выполняем проверку места длины тела контейнера
	ASSERT_EQ(record.at(32), static_cast <uint8_t> (header.length & 0xFF));
	// Выполняем проверку места количества записей в теле
	ASSERT_EQ(record.at(40), static_cast <uint8_t> (header.records & 0xFF));
	// Выполняем проверку места смещения оглавления
	ASSERT_EQ(record.at(48), static_cast <uint8_t> (header.index & 0xFF));
	// Выполняем проверку места смещения подписи
	ASSERT_EQ(record.at(56), static_cast <uint8_t> (header.signature & 0xFF));
	// Выполняем проверку места отпечатка открытого ключа
	ASSERT_EQ(record.at(64), header.fingerprint[0]);
	/**
	 * Выполняем проверку того, что отведённые под будущее октеты обнулены: занять их
	 * однажды можно будет лишь тогда, когда прежние контейнеры несут в них нули
	 */
	for(size_t i = 80; i < 88; i++)
		// Выполняем проверку обнуления очередного отведённого октета
		ASSERT_EQ(record.at(i), 0x00) << "отведённый октет: " << i;
}
