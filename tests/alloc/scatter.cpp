/**
 * @file scatter.cpp
 * @date 2026-09-15
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
 * @brief Проверки рассеяния ключевого материала по образу
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл главного модуля тестов
 */
#include "../main.hpp"

/**
 * Подключаем наши модули
 */
#include <alloc/scatter.hpp>
#include <alloc/vessel.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Подключаем заголовочный файл набора
 */
#include "suite.hpp"

/**
 * @brief Пространство имён вспомогательных средств
 *
 */
namespace {
	/**
	 * @brief Метод заведения приёмника, годного на этой системе
	 *
	 * @note Строгий приёмник отвечает отказом там, где запереть страницы нечем: проверки
	 *       существа рассеяния от окружения зависеть не должны
	 *
	 * @return заведённый приёмник
	 *
	 */
	awh::alloc::vessel_t relaxed() noexcept {
		// Выводим приёмник, работающий и там, где запереть страницы нечем
		return awh::alloc::vessel_t(awh::alloc::secrecy_t::RELAXED);
	}
};

/**
 * @brief Проверка кругового хода: разложенный секрет собирается обратно
 *
 */
TEST(AllocScatterTest, RoundTripRestoresTheSecret){
	// Раскладываемый секрет
	const uint8_t secret[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
	// Зерно раскладки
	const uint64_t seed = 0x0123456789ABCDEFULL;
	// Раскладываем секрет по носителю с запасом
	const std::vector <uint8_t> carrier = awh::alloc::Scatter::lay(secret, sizeof(secret), seed, 256);
	// Носитель обязан быть заведён заданной ёмкости
	ASSERT_EQ(carrier.size(), static_cast <size_t> (256));
	// Заводим приёмник под собранный секрет
	awh::alloc::vessel_t vessel = ::relaxed();
	// Собираем секрет из носителя тем же зерном
	ASSERT_TRUE(awh::alloc::Scatter::gather(carrier.data(), carrier.size(), seed, sizeof(secret), vessel));
	// Занятое обязано совпасть с длиной секрета
	EXPECT_EQ(vessel.size(), sizeof(secret));
	// Признак совпадения собранного с исходным
	bool matched = false;
	// Обращаемся к собранному содержимому
	ASSERT_TRUE(vessel.apply([&](const uint8_t * data, const size_t size) noexcept {
		// Запоминаем признак совпадения собранного с исходным
		matched = ((size == sizeof(secret)) && (::memcmp(data, secret, sizeof(secret)) == 0));
	}));
	// Собранный секрет обязан совпасть с исходным
	EXPECT_TRUE(matched);
}
/**
 * @brief Проверка сбора чужим зерном: выходит мусор, а не секрет
 *
 * @note Раскладку задаёт зерно: перестановка мест и гамма выведены из него. Иное зерно
 *       даёт иную перестановку и иную гамму, и собранное с ним секретом не будет
 *
 */
TEST(AllocScatterTest, WrongSeedGathersGarbage){
	// Раскладываемый секрет
	const uint8_t secret[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
	// Раскладываем секрет своим зерном
	const std::vector <uint8_t> carrier = awh::alloc::Scatter::lay(secret, sizeof(secret), 0xAAAAAAAAAAAAAAAAULL, 128);
	// Носитель обязан быть заведён
	ASSERT_FALSE(carrier.empty());
	// Заводим приёмник под собранное
	awh::alloc::vessel_t vessel = ::relaxed();
	// Собираем чужим зерном
	ASSERT_TRUE(awh::alloc::Scatter::gather(carrier.data(), carrier.size(), 0xBBBBBBBBBBBBBBBBULL, sizeof(secret), vessel));
	// Признак совпадения собранного с исходным
	bool matched = true;
	// Обращаемся к собранному содержимому
	ASSERT_TRUE(vessel.apply([&](const uint8_t * data, const size_t size) noexcept {
		// Запоминаем признак совпадения собранного с исходным
		matched = ((size == sizeof(secret)) && (::memcmp(data, secret, sizeof(secret)) == 0));
	}));
	// Собранное чужим зерном секретом быть не должно
	EXPECT_FALSE(matched);
}
/**
 * @brief Проверка отсутствия открытого секрета в носителе
 *
 * @note Ляг секрет в носитель открытым, пусть и рассеянным, - его подстрока нашлась бы
 *       в носителе. Наложение гаммой и рассеяние вместе такого не оставляют
 *
 */
TEST(AllocScatterTest, CarrierHoldsNoPlaintextRun){
	// Узнаваемый секрет
	const char text[] = "ANYKS-PRIVATE-KEY";
	// Длина секрета без завершающего нуля
	const size_t length = sizeof(text) - 1;
	// Раскладываем секрет по носителю
	const std::vector <uint8_t> carrier = awh::alloc::Scatter::lay(reinterpret_cast <const uint8_t *> (text), length, 0x1122334455667788ULL, 512);
	// Носитель обязан быть заведён
	ASSERT_EQ(carrier.size(), static_cast <size_t> (512));
	// Признак присутствия подстроки секрета в носителе
	bool present = false;
	/**
	 * Ищем подстроку секрета во всех местах носителя
	 */
	for(size_t i = 0; (i + length) <= carrier.size(); i++){
		// Если подстрока секрета нашлась на этом месте
		if(::memcmp(carrier.data() + i, text, length) == 0){
			// Запоминаем присутствие подстроки
			present = true;
			// Прерываем поиск
			break;
		}
	}
	// Открытой подстроки секрета в носителе быть не должно
	EXPECT_FALSE(present);
}
/**
 * @brief Проверка локальности наложения: один байт секрета правит одно место носителя
 *
 * @note Каждый байт секрета ложится на СВОЁ место под наложением. Смени один байт
 *       секрета при том же зерне - и носитель обязан измениться ровно в одном месте:
 *       перестановка мест и гамма от зерна не зависят от самого секрета
 *
 */
TEST(AllocScatterTest, OneSecretByteChangesOneCarrierPlace){
	// Первый секрет
	uint8_t first[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
	// Второй секрет, отличный от первого одним байтом
	uint8_t second[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
	// Меняем один байт второго секрета
	second[3] = 0xFF;
	// Зерно раскладки
	const uint64_t seed = 0xCAFEBABEDEADBEEFULL;
	// Раскладываем оба секрета одним зерном по носителю одной ёмкости
	const std::vector <uint8_t> left = awh::alloc::Scatter::lay(first, sizeof(first), seed, 64);
	const std::vector <uint8_t> right = awh::alloc::Scatter::lay(second, sizeof(second), seed, 64);
	// Носители обязаны быть одной ёмкости
	ASSERT_EQ(left.size(), right.size());
	// Число разошедшихся мест носителя
	size_t diverged = 0;
	/**
	 * Считаем разошедшиеся места носителя
	 */
	for(size_t i = 0; i < left.size(); i++){
		// Если место разошлось
		if(left[i] != right[i])
			// Увеличиваем число разошедшихся мест
			diverged++;
	}
	// Смена одного байта секрета обязана править ровно одно место носителя
	EXPECT_EQ(diverged, static_cast <size_t> (1));
}
/**
 * @brief Проверка зависимости раскладки от зерна
 *
 * @note Одно зерно даёт одну раскладку, иное - иную: смена зерна обесценивает знание,
 *       добытое из прежней версии образа
 *
 */
TEST(AllocScatterTest, LayoutDependsOnSeed){
	// Раскладываемый секрет
	const uint8_t secret[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	// Раскладываем секрет двумя разными зёрнами по носителю одной ёмкости
	const std::vector <uint8_t> left = awh::alloc::Scatter::lay(secret, sizeof(secret), 0x1111111111111111ULL, 96);
	const std::vector <uint8_t> right = awh::alloc::Scatter::lay(secret, sizeof(secret), 0x2222222222222222ULL, 96);
	// Носители обязаны быть заведены одной ёмкости
	ASSERT_EQ(left.size(), right.size());
	// Носители под разными зёрнами обязаны разойтись
	EXPECT_NE(::memcmp(left.data(), right.data(), left.size()), 0);
}
/**
 * @brief Проверка отказа при носителе теснее секрета
 *
 * @note Носителю негде вместить секрет, если ёмкость его меньше длины: раскладчик
 *       отвечает пустым носителем, сборщик - отказом
 *
 */
TEST(AllocScatterTest, RefusesCarrierSmallerThanSecret){
	// Раскладываемый секрет
	const uint8_t secret[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
	// Раскладка в тесный носитель обязана вернуть пустой носитель
	EXPECT_TRUE(awh::alloc::Scatter::lay(secret, sizeof(secret), 0x99ULL, sizeof(secret) - 1).empty());
	// Носитель на границе годен: ёмкость равна длине секрета
	EXPECT_EQ(awh::alloc::Scatter::lay(secret, sizeof(secret), 0x99ULL, sizeof(secret)).size(), sizeof(secret));
	// Заводим приёмник под сбор
	awh::alloc::vessel_t vessel = ::relaxed();
	// Место под мнимый носитель
	const uint8_t stub[8] = {0};
	// Сбор из носителя теснее секрета обязан отвечать отказом
	EXPECT_FALSE(awh::alloc::Scatter::gather(stub, sizeof(stub), 0x99ULL, sizeof(stub) + 1, vessel));
}
/**
 * @brief Проверка порождения исходного текста с носителем
 *
 * @note Порождённый текст несёт носитель и длину секрета, но ни зерна, ни самого
 *       секрета: снять наложение по одному тексту нечем
 *
 */
TEST(AllocScatterTest, EmitProducesCarrierAndLength){
	// Раскладываемый секрет
	const uint8_t secret[] = {0xAB, 0xCD, 0xEF, 0x01};
	// Раскладываем секрет по носителю
	const std::vector <uint8_t> carrier = awh::alloc::Scatter::lay(secret, sizeof(secret), 0x5A5A5A5AULL, 32);
	// Носитель обязан быть заведён
	ASSERT_EQ(carrier.size(), static_cast <size_t> (32));
	// Порождаем исходный текст с носителем
	const std::string source = awh::alloc::Scatter::emit(carrier, sizeof(secret), "embedded_carrier");
	// Текст обязан быть порождён
	ASSERT_FALSE(source.empty());
	// Текст обязан нести имя массива носителя
	EXPECT_NE(source.find("embedded_carrier[32]"), std::string::npos);
	// Текст обязан нести длину секрета
	EXPECT_NE(source.find("embedded_carrier_length = 4"), std::string::npos);
	// Порождение с пустым носителем обязано вернуть пустой текст
	EXPECT_TRUE(awh::alloc::Scatter::emit(std::vector <uint8_t> (), sizeof(secret), "empty").empty());
}
/**
 * @brief Проверка наложения гаммы: повтор байтов секрета не повторяется в носителе
 *
 * @note Проверка ловит ИМЕННО наложение, а не рассеяние. Носитель заведён впритык, без
 *       запаса, - все его места заняты секретом, заполнения нет вовсе. Секрет из одних и
 *       тех же байтов лёг бы в носитель повтором того же байта, не будь наложения; с
 *       наложением каждое место несёт свой байт потока, и повтора нет
 *
 */
TEST(AllocScatterTest, GammaMasksRepeatedBytes){
	// Секрет из тридцати двух одинаковых байтов
	uint8_t secret[32];
	// Заполняем секрет одним и тем же байтом
	::memset(secret, 0xAA, sizeof(secret));
	// Раскладываем секрет по носителю впритык, без запаса
	const std::vector <uint8_t> carrier = awh::alloc::Scatter::lay(secret, sizeof(secret), 0x7788990011223344ULL, sizeof(secret));
	// Носитель обязан быть заведён впритык
	ASSERT_EQ(carrier.size(), sizeof(secret));
	// Число мест носителя, несущих исходный байт секрета открытым
	size_t bare = 0;
	/**
	 * Считаем места носителя, где лёг исходный байт секрета
	 */
	for(size_t i = 0; i < carrier.size(); i++){
		// Если место несёт исходный байт секрета
		if(carrier[i] == 0xAA)
			// Увеличиваем число открытых мест
			bare++;
	}
	// Открытым байтам секрета в носителе взяться неоткуда: наложение их скрыло
	EXPECT_LT(bare, carrier.size());
	// Признак носителя из одного и того же байта
	bool uniform = true;
	/**
	 * Ищем в носителе место, отличное от первого
	 */
	for(size_t i = 1; i < carrier.size(); i++){
		// Если место отлично от первого
		if(carrier[i] != carrier[0]){
			// Запоминаем разнородность носителя
			uniform = false;
			// Прерываем поиск
			break;
		}
	}
	// Носитель из повторяющегося секрета обязан быть разнородным
	EXPECT_FALSE(uniform);
}
/**
 * @brief Проверка привязки раскладки к контрольной сумме кода
 *
 * @note Якорь подмешан в зерно: собрать секрет удаётся лишь той же суммой, какой он
 *       разложен. Иная сумма - патч образа - даёт мусор, а не секрет
 *
 */
TEST(AllocScatterTest, AnchorBindsLayoutToChecksum){
	// Раскладываемый секрет
	const uint8_t secret[] = {0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58};
	// Зерно раскладки из закрытого контура
	const uint64_t seed = 0x0F1E2D3C4B5A6978ULL;
	// Контрольная сумма кода при сборке
	const uint64_t checksum = 0xABCDEF0123456789ULL;
	// Раскладываем секрет зерном, привязанным к контрольной сумме
	const std::vector <uint8_t> carrier = awh::alloc::Scatter::lay(secret, sizeof(secret), awh::alloc::Scatter::anchor(seed, checksum), 128);
	// Носитель обязан быть заведён
	ASSERT_FALSE(carrier.empty());
	// Заводим приёмник под сбор верной суммой
	awh::alloc::vessel_t right = ::relaxed();
	// Собираем той же контрольной суммой
	ASSERT_TRUE(awh::alloc::Scatter::gather(carrier.data(), carrier.size(), awh::alloc::Scatter::anchor(seed, checksum), sizeof(secret), right));
	// Признак совпадения собранного верной суммой с исходным
	bool matched = false;
	// Обращаемся к собранному верной суммой
	ASSERT_TRUE(right.apply([&](const uint8_t * data, const size_t size) noexcept {
		// Сверяем собранное с исходным секретом
		matched = ((size == sizeof(secret)) && (::memcmp(data, secret, sizeof(secret)) == 0));
	}));
	// Собранное верной суммой обязано совпасть с исходным
	EXPECT_TRUE(matched);
	// Заводим приёмник под сбор изменённой суммой
	awh::alloc::vessel_t wrong = ::relaxed();
	// Собираем изменённой суммой - имитируем патч кода на один разряд
	ASSERT_TRUE(awh::alloc::Scatter::gather(carrier.data(), carrier.size(), awh::alloc::Scatter::anchor(seed, checksum ^ 0x1ULL), sizeof(secret), wrong));
	// Признак совпадения собранного изменённой суммой с исходным
	bool patched = true;
	// Обращаемся к собранному изменённой суммой
	ASSERT_TRUE(wrong.apply([&](const uint8_t * data, const size_t size) noexcept {
		// Сверяем собранное с исходным секретом
		patched = ((size == sizeof(secret)) && (::memcmp(data, secret, sizeof(secret)) == 0));
	}));
	// Собранное изменённой суммой секретом быть не должно
	EXPECT_FALSE(patched);
}
/**
 * @brief Проверка развода сборок версией ключа
 *
 * @note Версия подмешана в зерно наравне с суммой: боевой контур и отладочный выводят
 *       ключи разных версий, и контейнер одной версии не открывается ключом другой не
 *       запретом, а тем, что ключ иной
 *
 */
TEST(AllocScatterTest, AnchorVersionSeparatesBuilds){
	// Раскладываемый секрет
	const uint8_t secret[] = {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68};
	// Зерно раскладки
	const uint64_t seed = 0x1234ABCD5678EF90ULL;
	// Контрольная сумма кода
	const uint64_t checksum = 0x00FF00FF00FF00FFULL;
	// Раскладываем секрет боевой версией ключа
	const std::vector <uint8_t> carrier = awh::alloc::Scatter::lay(secret, sizeof(secret), awh::alloc::Scatter::anchor(seed, checksum, 1), 128);
	// Носитель обязан быть заведён
	ASSERT_FALSE(carrier.empty());
	// Заводим приёмник под сбор отладочной версией
	awh::alloc::vessel_t vessel = ::relaxed();
	// Собираем отладочной версией того же ключа
	ASSERT_TRUE(awh::alloc::Scatter::gather(carrier.data(), carrier.size(), awh::alloc::Scatter::anchor(seed, checksum, 2), sizeof(secret), vessel));
	// Признак совпадения собранного иной версией с исходным
	bool crossed = true;
	// Обращаемся к собранному иной версией
	ASSERT_TRUE(vessel.apply([&](const uint8_t * data, const size_t size) noexcept {
		// Сверяем собранное с исходным секретом
		crossed = ((size == sizeof(secret)) && (::memcmp(data, secret, sizeof(secret)) == 0));
	}));
	// Собранное иной версией секретом быть не должно
	EXPECT_FALSE(crossed);
}
/**
 * @brief Проверка воспроизводимости якоря
 *
 * @note Один вход - один выход: обе стороны договора обязаны получить одно зерно из
 *       одного зерна, одной суммы и одной версии, иначе поток разойдётся
 *
 */
TEST(AllocScatterTest, AnchorIsDeterministic){
	// Одинаковый вход обязан давать одинаковое привязанное зерно
	EXPECT_EQ(awh::alloc::Scatter::anchor(0x11ULL, 0x22ULL, 3), awh::alloc::Scatter::anchor(0x11ULL, 0x22ULL, 3));
	// Смена суммы обязана менять привязанное зерно
	EXPECT_NE(awh::alloc::Scatter::anchor(0x11ULL, 0x22ULL, 3), awh::alloc::Scatter::anchor(0x11ULL, 0x23ULL, 3));
	// Смена версии обязана менять привязанное зерно
	EXPECT_NE(awh::alloc::Scatter::anchor(0x11ULL, 0x22ULL, 3), awh::alloc::Scatter::anchor(0x11ULL, 0x22ULL, 4));
}
/**
 * @brief Проверка вплетения секрета в рабочую таблицу без порчи прочих данных
 *
 * @note Вплетение правит лишь места секрета, прочие места таблицы - настоящие рабочие
 *       данные - оставляет нетронутыми. Изменённых мест ровно столько, какова длина
 *       секрета, а собрать секрет из вплетённой таблицы удаётся тем же зерном
 *
 */
TEST(AllocScatterTest, WeaveKeepsWorkingDataIntact){
	// Размер рабочей таблицы
	const size_t size = 200;
	// Длина вплетаемого секрета
	const uint8_t secret[] = {0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A};
	// Зерно раскладки
	const uint64_t seed = 0x2468ACE013579BDFULL;
	// Заводим рабочую таблицу с осмысленными данными
	std::vector <uint8_t> table(size);
	/**
	 * Заполняем таблицу рабочими данными
	 */
	for(size_t i = 0; i < size; i++)
		// Записываем очередное рабочее значение
		table[i] = static_cast <uint8_t> ((i * 7) + 3);
	// Снимаем копию таблицы до вплетения
	const std::vector <uint8_t> before = table;
	// Вплетаем секрет в рабочую таблицу
	ASSERT_TRUE(awh::alloc::Scatter::weave(table.data(), table.size(), secret, sizeof(secret), seed));
	// Число изменённых мест таблицы
	size_t changed = 0;
	/**
	 * Считаем изменённые места таблицы
	 */
	for(size_t i = 0; i < size; i++){
		// Если место изменилось
		if(table[i] != before[i])
			// Увеличиваем число изменённых мест
			changed++;
	}
	// Изменённых мест обязано быть не больше длины секрета
	EXPECT_LE(changed, sizeof(secret));
	// Заводим приёмник под сбор из вплетённой таблицы
	awh::alloc::vessel_t vessel = ::relaxed();
	// Собираем секрет из вплетённой таблицы тем же зерном
	ASSERT_TRUE(awh::alloc::Scatter::gather(table.data(), table.size(), seed, sizeof(secret), vessel));
	// Признак совпадения собранного с исходным
	bool matched = false;
	// Обращаемся к собранному секрету
	ASSERT_TRUE(vessel.apply([&](const uint8_t * data, const size_t size) noexcept {
		// Сверяем собранное с исходным секретом
		matched = ((size == sizeof(secret)) && (::memcmp(data, secret, sizeof(secret)) == 0));
	}));
	// Собранный из таблицы секрет обязан совпасть с исходным
	EXPECT_TRUE(matched);
}
/**
 * @brief Проверка воспроизводимости раскладки между системами
 *
 * @note Зашитый ключ раскладывается на сборочной машине, а собирается в образе на
 *       целевой - возможно иной архитектуры. Поток `splitmix64` есть арифметика над
 *       шестидесятичетырёхразрядным словом, `byte()` берёт младший разряд, перестановка
 *       ведётся остатком - всё это обязано давать один ряд на всякой системе. Эталон снят
 *       на macOS ARM64 15.09.2026; расхождение на ином стенде означает, что раскладка
 *       НЕ воспроизводима между системами, и зашитый ключ на целевой машине не соберётся
 *
 */
TEST(AllocScatterTest, CarrierMatchesGoldenVector){
	// Эталонный носитель для известного входа
	static const uint8_t golden[48] = {
		0x64, 0x8D, 0x1B, 0xB2, 0xBC, 0x4D, 0x65, 0x3E, 0xA9, 0x73, 0x77, 0x5A,
		0xD0, 0x1D, 0x13, 0x02, 0x70, 0xB7, 0xC0, 0xD5, 0x9E, 0x29, 0x62, 0xCF,
		0xDD, 0x0B, 0x05, 0xE4, 0x8C, 0x34, 0xD4, 0x7D, 0xFF, 0x93, 0xA9, 0xDB,
		0x1A, 0xDA, 0x48, 0x64, 0x54, 0xBB, 0xB2, 0x1C, 0x6F, 0x7A, 0x65, 0xCF
	};
	// Известный секрет эталона
	const char secret[] = "ANYKS-KEY";
	// Длина известного секрета
	const size_t length = sizeof(secret) - 1;
	// Зерно эталона
	const uint64_t seed = 0x0123456789ABCDEFULL;
	// Раскладываем известный секрет известным зерном
	const std::vector <uint8_t> carrier = awh::alloc::Scatter::lay(reinterpret_cast <const uint8_t *> (secret), length, seed, sizeof(golden));
	// Носитель обязан быть заведён размером эталона
	ASSERT_EQ(carrier.size(), sizeof(golden));
	// Носитель обязан совпасть с эталоном байт в байт
	EXPECT_EQ(::memcmp(carrier.data(), golden, sizeof(golden)), 0);
	// Якорь обязан давать одно значение на всякой системе
	EXPECT_EQ(awh::alloc::Scatter::anchor(seed, 0xABCDULL, 1), static_cast <uint64_t> (0x1C52017B87888DF7ULL));
}
