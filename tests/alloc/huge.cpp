/**
 * @file huge.cpp
 * @date 2026-08-23
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
 * @brief Набор проверок крупных выдач и укрытой памяти
 *
 * @details Крупная выдача идёт мимо разрядов и мимо кэшей потоков: она берётся у
 *          системы напрямую и учитывается своим перечнем. Перечень этот разбирает
 *          адреса, растёт перехэшированием и возвращает область системе немедля -
 *          ничего из этого не проверялось вовсе, покрытие слоя составляло 8 %
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл главного модуля тестов
 */
#include "../main.hpp"

/**
 * Подключаем заголовочный файл набора
 */
#include "suite.hpp"

/**
 * Подключаем заголовочный файл слоя крупных выдач
 *
 * Нужен он ради начальной длины перечня: проверка роста обязана выдать заведомо
 * БОЛЬШЕ неё, а вписанное числом разошлось бы с кодом при первой же правке
 */
#include <alloc/huge.hpp>

/**
 * Стандартные модули
 */
#include <vector>
#include <cstdlib>
#include <cstring>

/**
 * Под санитайзерами проверять здесь нечего: захват не состоится, и слоёв наших
 * в процессе нет. Отсутствие это утверждает `AllocCaptureTest`
 */
#if !AWH_ALLOC_SANITIZED

/**
 * Порог крупной выдачи
 *
 * Имя `HUGE` здесь занято: у `math.h` так зовётся наибольшее дробное число, и наш
 * порог обратился бы в него молча
 *
 * Крупной считается выдача свыше четырёх мегабайт - предела куска, каким распределитель
 * берёт память у системы. Берём с запасом, а не впритык: выдача РОВНО в предел осталась
 * бы обычной, и проверка мерила бы не тот слой
 */
static constexpr size_t LARGE = (5u * 1024u * 1024u);

/**
 * @brief Тест опознания крупной выдачи
 *
 * @note Крупный блок обязан быть узнан разбором адреса своим и живым: перечень крупных
 *       выдач - единственное место, где он числится, и промах перечня обратил бы его в
 *       чужую память
 *
 */
TEST_F(AllocFixture, HugeBlockIsRecognized){
	// Выдаём крупный блок
	void * block = ::malloc(LARGE);
	// Крупная выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Разбираем адрес выданного блока
	const awh::alloc::region_t region = awh::alloc::Allocator::resolve(block);
	// Блок обязан быть узнан живым
	EXPECT_EQ(region.origin, awh::alloc::origin_t::LIVE);
	// Начало блока обязано совпасть с выданным адресом
	EXPECT_EQ(region.begin, block);
	// Размер блока обязан покрывать запрошенное
	EXPECT_GE(region.size, LARGE);
	// Смещение от начала обязано быть нулевым
	EXPECT_EQ(region.offset, static_cast <ptrdiff_t> (0));
	// Освобождаем крупный блок
	::free(block);
}
/**
 * @brief Тест разбора адреса внутри крупной выдачи
 *
 * @note Разбор обязан узнать блок и по адресу ВНУТРИ него, доложив смещение: ровно это
 *       превращает разбор в средство розыска, а не в опрос «наш ли адрес»
 *
 */
TEST_F(AllocFixture, HugeBlockIsFoundByInnerAddress){
	// Выдаём крупный блок
	char * block = reinterpret_cast <char *> (::malloc(LARGE));
	// Крупная выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Смещение, по какому спрашиваем
	const ptrdiff_t shift = (LARGE / 3);
	// Разбираем адрес внутри блока
	const awh::alloc::region_t region = awh::alloc::Allocator::resolve(block + shift);
	// Блок обязан быть узнан живым
	EXPECT_EQ(region.origin, awh::alloc::origin_t::LIVE);
	// Начало блока обязано указывать на его начало, а не на спрошенный адрес
	EXPECT_EQ(region.begin, block);
	// Смещение обязано совпасть со спрошенным
	EXPECT_EQ(region.offset, shift);
	// Освобождаем крупный блок
	::free(block);
}
/**
 * @brief Тест размера крупной выдачи
 *
 * @note Размер, доложенный распределителем, обязан покрывать запрошенное: код,
 *       положившийся на него, пишет по всей доложенной длине
 *
 */
TEST_F(AllocFixture, HugeBlockSizeCoversRequest){
	// Выдаём крупный блок
	void * block = ::malloc(LARGE + 7u);
	// Крупная выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Разбираем адрес выданного блока
	const awh::alloc::region_t region = awh::alloc::Allocator::resolve(block);
	// Размер обязан покрывать запрошенное
	ASSERT_GE(region.size, (LARGE + 7u));
	/**
	 * Пишем по всей доложенной длине
	 *
	 * Доложенный размер - обещание, а не справка: запись по нему обязана оставаться в
	 * своей области. Заведомо порченый размер повалил бы проверку здесь
	 */
	::memset(block, 0x5A, region.size);
	// Читаем последний байт доложенной длины
	EXPECT_EQ(reinterpret_cast <unsigned char *> (block)[region.size - 1], 0x5Au);
	// Освобождаем крупный блок
	::free(block);
}
/**
 * @brief Тест обнуления крупной выдачи
 *
 * @note Обнуление у крупной выдачи достаётся даром - система отдаёт страницы чистыми,
 *       - и оттого проверять его надо тем более: пропуск обнуления здесь ничего не
 *       ломает ровно до первой страницы, взятой повторно
 *
 */
TEST_F(AllocFixture, HugeBlockIsZeroed){
	// Выдаём крупный блок с обнулением
	unsigned char * block = reinterpret_cast <unsigned char *> (::calloc(1, LARGE));
	// Крупная выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Число ненулевых байтов
	size_t dirty = 0;
	/**
	 * Перебираем содержимое блока с шагом в страницу
	 *
	 * Перебор побайтно ничего не добавил бы: страницы система отдаёт целиком, и грязь в
	 * них появляется страницей же
	 */
	for(size_t i = 0; i < LARGE; i += 4096u){
		// Если байт оказался ненулевым
		if(block[i] != 0)
			// Считаем ненулевой байт
			dirty++;
	}
	// Ненулевых байтов быть не должно
	EXPECT_EQ(dirty, static_cast <size_t> (0));
	// Освобождаем крупный блок
	::free(block);
}
/**
 * @brief Тест перевыдачи крупного блока
 *
 * @note Перевыдача обязана сохранить содержимое и в обе стороны - и при росте, и при
 *       усечении. Крупный блок перевыдаётся не так, как разрядный: копирование идёт по
 *       МЕНЬШЕМУ из размеров, и путаница их портит содержимое молча
 *
 */
TEST_F(AllocFixture, HugeBlockKeepsContentOnRealloc){
	// Выдаём крупный блок
	unsigned char * block = reinterpret_cast <unsigned char *> (::malloc(LARGE));
	// Крупная выдача обязана состояться
	ASSERT_NE(block, nullptr);
	/**
	 * Засеваем содержимое узнаваемым следом
	 */
	for(size_t i = 0; i < LARGE; i += 4096u)
		// Кладём в начало каждой страницы её номер
		block[i] = static_cast <unsigned char> ((i / 4096u) & 0xFFu);
	// Перевыдаём блок с ростом
	block = reinterpret_cast <unsigned char *> (::realloc(block, (LARGE * 2u)));
	// Перевыдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Число расхождений содержимого
	size_t broken = 0;
	/**
	 * Сличаем содержимое с засеянным
	 */
	for(size_t i = 0; i < LARGE; i += 4096u){
		// Если содержимое разошлось с засеянным
		if(block[i] != static_cast <unsigned char> ((i / 4096u) & 0xFFu))
			// Считаем расхождение
			broken++;
	}
	// Расхождений быть не должно
	EXPECT_EQ(broken, static_cast <size_t> (0));
	// Перевыдаём блок с усечением до половины исходного
	block = reinterpret_cast <unsigned char *> (::realloc(block, (LARGE / 2u)));
	// Перевыдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Сбрасываем счётчик расхождений
	broken = 0;
	/**
	 * Сличаем уцелевшую половину содержимого
	 */
	for(size_t i = 0; i < (LARGE / 2u); i += 4096u){
		// Если содержимое разошлось с засеянным
		if(block[i] != static_cast <unsigned char> ((i / 4096u) & 0xFFu))
			// Считаем расхождение
			broken++;
	}
	// Расхождений быть не должно
	EXPECT_EQ(broken, static_cast <size_t> (0));
	// Освобождаем блок
	::free(block);
}
/**
 * @brief Тест роста перечня крупных выдач
 *
 * @note Перечень заведён на шестьдесят четыре записи и растёт перехэшированием. Рост
 *       этот - место, где записи переносятся в новый перечень, и потеря хотя бы одной
 *       обратила бы живой блок в чужую память. Оттого выдаём заведомо БОЛЬШЕ предела
 *
 */
TEST_F(AllocFixture, HugeTableSurvivesGrowth){
	// Число крупных выдач: вдвое больше начального предела перечня
	const size_t count = 128;
	// Перечень выданных блоков
	std::vector <void *> blocks;
	// Резервируем место под перечень
	blocks.reserve(count);
	/**
	 * Выдаём крупные блоки
	 *
	 * Размер берём наименьший из крупных: перечень проверяется числом записей, а не
	 * объёмом, и лишние гигабайты закрыли бы проверку на слабой машине
	 */
	for(size_t i = 0; i < count; i++){
		// Выдаём очередной крупный блок
		void * block = ::malloc(LARGE);
		// Если выдать блок не вышло
		if(block == nullptr)
			// Прекращаем выдачу
			break;
		// Запоминаем выданный блок
		blocks.push_back(block);
	}
	// Выдач должно состояться вдоволь для роста перечня
	ASSERT_GT(blocks.size(), awh::alloc::Huge::TABLE);
	// Число блоков, не узнанных разбором
	size_t lost = 0;
	/**
	 * Перебираем выданные блоки
	 */
	for(void * block : blocks){
		// Разбираем адрес блока
		const awh::alloc::region_t region = awh::alloc::Allocator::resolve(block);
		// Если блок не узнан живым
		if((region.origin != awh::alloc::origin_t::LIVE) || (region.begin != block))
			// Считаем потерянный блок
			lost++;
	}
	// Потерянных блоков быть не должно
	EXPECT_EQ(lost, static_cast <size_t> (0));
	/**
	 * Перебираем выданные блоки
	 */
	for(void * block : blocks)
		// Освобождаем блок
		::free(block);
}
/**
 * @brief Тест придержки освобождённой крупной области
 *
 * @note Освобождённая область не отдаётся системе тут же, а придерживается под
 *       потолком `hugeCache`: отданная область при следующей выдаче приходит ЧИСТОЙ, и
 *       за каждую её страницу платится отказом страницы. Проверяется это тем, что
 *       повторная выдача того же размера не берёт у системы НИЧЕГО
 *
 */
TEST_F(AllocFixture, HugeRegionIsRetainedForReuse){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём потолок придержанного с запасом под область
	options.hugeCache = (LARGE * 2u);
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Выдаём крупный блок
	void * first = ::malloc(LARGE);
	// Крупная выдача обязана состояться
	ASSERT_NE(first, nullptr);
	// Освобождаем крупный блок
	::free(first);
	// Запоминаем взятое у системы при придержанной области
	const size_t before = awh::alloc::Allocator::property(awh::alloc::property_t::HEAP);
	// Выдаём крупный блок того же размера
	void * second = ::malloc(LARGE);
	// Крупная выдача обязана состояться
	ASSERT_NE(second, nullptr);
	// Область обязана достаться придержанная, то есть ТА ЖЕ САМАЯ
	EXPECT_EQ(second, first);
	// Взятое у системы обязано остаться прежним: у системы не взято ничего
	EXPECT_EQ(awh::alloc::Allocator::property(awh::alloc::property_t::HEAP), before);
	// Освобождаем крупный блок
	::free(second);
}
/**
 * @brief Тест отдачи придержанного по просьбе
 *
 * @note Придержка бережёт страницы, но просьба об отдаче старше этой бережливости:
 *       просят её у затишья, когда приложение само знает, что крупных выдач впереди нет
 *
 */
TEST_F(AllocFixture, RetainedRegionIsGivenBackOnRequest){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём потолок придержанного с запасом под область
	options.hugeCache = (LARGE * 2u);
	// Задаём отдачу памяти системе по просьбе
	options.purgeMode = awh::alloc::purge_t::MANUAL;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Выдаём крупный блок
	void * block = ::malloc(LARGE);
	// Крупная выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Освобождаем крупный блок
	::free(block);
	// Запоминаем взятое у системы при придержанной области
	const size_t before = awh::alloc::Allocator::property(awh::alloc::property_t::HEAP);
	// Придержанная область обязана числиться взятой у системы: она у неё и взята
	ASSERT_GE(before, LARGE);
	// Просим отдать память системе
	awh::alloc::Allocator::purge();
	// Взятое у системы обязано убыть на размер придержанной области
	EXPECT_LE(awh::alloc::Allocator::property(awh::alloc::property_t::HEAP), (before - LARGE));
}
/**
 * @brief Тест отдачи крупной выдачи сверх потолка придержанного
 *
 * @note Потолок этот - плата за скорость, и плата ограниченная: область, в него не
 *       вошедшую, слой отдаёт системе тут же, как отдавал всё до появления придержки
 *
 */
TEST_F(AllocFixture, HugeBlockAboveCeilingIsGivenBackAtOnce){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём потолок придержанного заведомо меньше выдачи
	options.hugeCache = (LARGE / 2u);
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Запоминаем взятое у системы до выдачи
	const size_t before = awh::alloc::Allocator::property(awh::alloc::property_t::HEAP);
	// Выдаём крупный блок
	void * block = ::malloc(LARGE);
	// Крупная выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Запоминаем взятое у системы при живом блоке
	const size_t during = awh::alloc::Allocator::property(awh::alloc::property_t::HEAP);
	// Взятое у системы обязано вырасти на размер выдачи
	EXPECT_GE((during - before), LARGE);
	// Освобождаем крупный блок
	::free(block);
	// Взятое у системы обязано вернуться к прежнему БЕЗ просьбы об отдаче
	EXPECT_LT(awh::alloc::Allocator::property(awh::alloc::property_t::HEAP), during);
}
/**
 * @brief Тест роста области сверх разрядов НА МЕСТЕ
 *
 * @note Перенос содержимого при росте стоит копирования всего блока, и на цепочке
 *       удвоений цена его выходит квадратичной. Область сверх разрядов растёт за счёт
 *       соседней свободной, и адрес её при этом не меняется - ровно это и проверяется
 *
 * @warning Удаётся рост на месте НЕ ВСЕГДА: сосед вправе быть занят. Проверка ставит
 *          себя в условия, где он свободен заведомо - блок выдан последним и растёт
 *          тут же, - а утверждает не «всегда», а «в этих условиях»
 *
 */
TEST_F(AllocFixture, PagedBlockGrowsInPlace){
	// Размер, заведомо выходящий за разряды, но не достающий до крупных выдач
	const size_t start = (64u * 1024u);
	// Выдаём блок сверх разрядов
	void * block = ::malloc(start);
	// Выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Заполняем блок узнаваемым следом
	::memset(block, 0x3C, start);
	// Растим блок вдвое
	void * grown = ::realloc(block, (start * 2u));
	// Перевыдача обязана состояться
	ASSERT_NE(grown, nullptr);
	// Блок обязан остаться на месте: соседняя область свободна
	EXPECT_EQ(grown, block);
	// Размер обязан вырасти
	EXPECT_GE(awh::alloc::Allocator::resolve(grown).size, (start * 2u));
	// Содержимое обязано уцелеть
	EXPECT_EQ(reinterpret_cast <unsigned char *> (grown)[start - 1], 0x3Cu);
	// Освобождаем блок
	::free(grown);
}
/**
 * @brief Тест учёта расхода при росте на месте
 *
 * @note Рост на месте не проходит через выдачу, и учесть прибавку обязан он сам: иначе
 *       занятое прикладным кодом отставало бы на всю выросшую часть, а освобождение
 *       вычло бы её целиком и увело счёт в минус
 *
 */
TEST_F(AllocFixture, InPlaceGrowthIsAccounted){
	/**
	 * Проверка эта спрашивает учёт расхода, а тот снимается сборкой
	 *
	 * Признак AWH_ALLOC_NO_ACCOUNTING обращает ALLOCATED и PEAK в нуль намеренно, и
	 * утверждать по ним что-либо в такой сборке нельзя
	 */
	#if defined(AWH_ALLOC_NO_ACCOUNTING)
		// Пропускаем проверку: учёт расхода снят сборкой
		GTEST_SKIP() << "учёт расхода снят сборкой: AWH_ALLOC_NO_ACCOUNTING";
	#endif
	// Размер, заведомо выходящий за разряды
	const size_t start = (64u * 1024u);
	// Выдаём блок сверх разрядов
	void * block = ::malloc(start);
	// Выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Запоминаем занятое прикладным кодом до роста
	const size_t before = awh::alloc::Allocator::property(awh::alloc::property_t::ALLOCATED);
	// Растим блок вчетверо
	void * grown = ::realloc(block, (start * 4u));
	// Перевыдача обязана состояться
	ASSERT_NE(grown, nullptr);
	// Запоминаем занятое прикладным кодом после роста
	const size_t after = awh::alloc::Allocator::property(awh::alloc::property_t::ALLOCATED);
	// Занятое обязано вырасти не меньше, чем на прибавку
	EXPECT_GE((after - before), (start * 3u));
	// Освобождаем блок
	::free(grown);
	// Занятое обязано вернуться к тому, что было до выдачи
	EXPECT_LE(awh::alloc::Allocator::property(awh::alloc::property_t::ALLOCATED), before);
}
/**
 * @brief Тест правки размера у учёта мест выдачи при росте на месте
 *
 * @note Рост на месте через выдачу не проходит, и запись учёта осталась бы с прежним
 *       размером: поиск утечек шёл бы по числам МЕНЬШЕ настоящих как раз у тех блоков,
 *       что растут, - то есть у самых подозрительных
 *
 */
TEST_F(AllocFixture, InPlaceGrowthAmendsHoldings){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Берём под учёт всякую выдачу
	options.profileRate = 1;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Размер, заведомо выходящий за разряды
	const size_t start = (64u * 1024u);
	// Выдаём блок сверх разрядов
	void * block = ::malloc(start);
	// Выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Растим блок вчетверо
	void * grown = ::realloc(block, (start * 4u));
	// Перевыдача обязана состояться
	ASSERT_NE(grown, nullptr);
	// Блок обязан остаться на месте: иначе проверять нечего
	ASSERT_EQ(grown, block);
	// Размер блока, доложенный учётом
	size_t reported = 0;
	// Перебираем удерживаемое, разыскивая наш блок
	awh::alloc::Allocator::holdings([grown, &reported](const awh::alloc::holding_t & holding) noexcept -> bool {
		// Если запись говорит о нашем блоке
		if(holding.block == grown)
			// Запоминаем доложенный размер
			reported = holding.size;
		// Перебираем дальше
		return true;
	});
	// Учёт обязан доложить ВЫРОСШИЙ размер, а не прежний
	EXPECT_GE(reported, (start * 4u));
	// Освобождаем блок
	::free(grown);
}
/**
 * @brief Тест выдачи укрытой памяти
 *
 * @note Укрытая выдача идёт тем же слоем, что и крупная, но страницами и с защитой.
 *       Обещания у систем разные, оттого проверяется не сама защита, а честность
 *       доклада о ней: молчаливое понижение защиты хуже честного отказа
 *
 */
TEST_F(AllocFixture, SecureBlockIsAllocated){
	// Сведения о состоявшейся защите
	awh::alloc::shelter_t shelter;
	// Выдаём укрытую память
	void * secret = awh::alloc::Allocator::secure(1024u, &shelter);
	// Укрытая выдача обязана состояться
	ASSERT_NE(secret, nullptr);
	// Заполняем выданное содержимым
	::memset(secret, 0xC3, 1024u);
	// Разбираем адрес выданного блока
	const awh::alloc::region_t region = awh::alloc::Allocator::resolve(secret);
	// Укрытая выдача обязана быть узнана живой
	EXPECT_EQ(region.origin, awh::alloc::origin_t::LIVE);
	// Размер обязан покрывать запрошенное
	EXPECT_GE(region.size, static_cast <size_t> (1024u));
	/**
	 * Возвращаем укрытую память через `release`, а НЕ через `free`
	 *
	 * Совпадают они лишь там, где захват выдачи состоялся: у macOS и MS Windows без
	 * захвата `free` принадлежит системе и нашу область не узнаёт
	 */
	awh::alloc::Allocator::release(secret);
}
/**
 * @brief Тест страничной выдачи укрытой памяти
 *
 * @note Укрытие система выдаёт не мельче страницы, оттого всякая выдача здесь стоит не
 *       меньше страницы. Свойство это - цена, а не пробел, и закрепляется оно затем,
 *       чтобы будущая «бережливая» правка не свела укрытую выдачу к разрядной
 *
 */
TEST_F(AllocFixture, SecureBlockIsPageSized){
	// Выдаём укрытую память под мелочь
	void * secret = awh::alloc::Allocator::secure(16u, nullptr);
	// Укрытая выдача обязана состояться
	ASSERT_NE(secret, nullptr);
	// Разбираем адрес выданного блока
	const awh::alloc::region_t region = awh::alloc::Allocator::resolve(secret);
	// Начало блока обязано лежать на границе страницы
	EXPECT_EQ((reinterpret_cast <uintptr_t> (region.begin) % 4096u), static_cast <uintptr_t> (0));
	// Возвращаем укрытую память распределителю
	awh::alloc::Allocator::release(secret);
}
/**
 * @brief Тест независимости укрытых выдач
 *
 * @note Укрытые выдачи не должны делить между собою страницу: сосед по странице получил
 *       бы чужой ключ вместе со своим. Проверяется это расстоянием между выдачами
 *
 */
TEST_F(AllocFixture, SecureBlocksDoNotSharePages){
	// Число укрытых выдач
	const size_t count = 8;
	// Перечень выданных блоков
	void * blocks[count];
	/**
	 * Выдаём укрытую память россыпью
	 */
	for(size_t i = 0; i < count; i++)
		// Выдаём очередной укрытый блок
		blocks[i] = awh::alloc::Allocator::secure(64u, nullptr);
	// Число выдач, делящих страницу с соседом
	size_t shared = 0;
	/**
	 * Перебираем выданные блоки
	 */
	for(size_t i = 0; i < count; i++){
		// Если блок выдать не вышло
		if(blocks[i] == nullptr)
			// Пропускаем невыданный блок
			continue;
		/**
		 * Перебираем остальные блоки
		 */
		for(size_t j = (i + 1); j < count; j++){
			// Если блок выдать не вышло
			if(blocks[j] == nullptr)
				// Пропускаем невыданный блок
				continue;
			// Получаем адреса блоков числом
			const uintptr_t one = reinterpret_cast <uintptr_t> (blocks[i]);
			const uintptr_t two = reinterpret_cast <uintptr_t> (blocks[j]);
			// Если блоки легли на одну страницу
			if((one / 4096u) == (two / 4096u))
				// Считаем делёж страницы
				shared++;
		}
	}
	// Дележа страниц быть не должно
	EXPECT_EQ(shared, static_cast <size_t> (0));
	/**
	 * Перебираем выданные блоки
	 */
	for(size_t i = 0; i < count; i++){
		// Если блок выдан
		if(blocks[i] != nullptr)
			// Возвращаем блок распределителю
			awh::alloc::Allocator::release(blocks[i]);
	}
}

#endif // !AWH_ALLOC_SANITIZED
/**
 * @brief Тест придержки освобождённой области сверх разрядов
 *
 * @note Придержка эта ВЫКЛЮЧЕНА по умолчанию и включается настройкой `spanCache`.
 *       Довод умолчания записан у самой настройки: придержка враждебна росту области на
 *       месте, и обмен выбирает приложение
 *
 * @warning Сличать АДРЕСА выданных областей здесь нельзя: куча и без придержки отдаёт
 *          на тот же запрос ту же самую область, и совпадение адресов не доказывает
 *          ничего. Придержку отличает иное - освобождённая область куче НЕ
 *          возвращается, и свободное у кучи не растёт
 *
 */
TEST_F(AllocFixture, SpanReserveWorksOnlyWhenAsked){
	/**
	 * Под санитайзером проверять нечего: выдача принадлежит ЕМУ
	 *
	 * Захват там не состоится, память идёт мимо наших слоёв, и ни свободное у кучи, ни
	 * адрес области ничего о придержке не говорят. Проверка эта стоит ВНЕ блока
	 * `#if !AWH_ALLOC_SANITIZED`, каким укрыты соседи по файлу, оттого пропуск здесь свой
	 */
	#if AWH_ALLOC_SANITIZED
		// Пропускаем проверку: захват под санитайзером не состоится
		GTEST_SKIP() << "захват под санитайзером не состоится";
	#else
	// Размер области сверх разрядов: выше границы разрядов, ниже размера куска
	constexpr size_t SPAN = (64u * 1024u);
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	/**
	 * Сперва спрашиваем поведение по умолчанию: область обязана уйти куче
	 */
	{
		// Выключаем придержку областей сверх разрядов
		options.spanCache = 0;
		// Применяем настройки
		awh::alloc::Allocator::options(options);
		// Выдаём область сверх разрядов
		void * span = ::malloc(SPAN);
		// Выдача обязана состояться
		ASSERT_NE(span, nullptr);
		// Запоминаем свободное у кучи до освобождения
		const size_t before = awh::alloc::Allocator::property(awh::alloc::property_t::PAGEFREE);
		// Освобождаем область
		::free(span);
		// Свободное у кучи обязано вырасти: область ей возвращена
		EXPECT_GE(awh::alloc::Allocator::property(awh::alloc::property_t::PAGEFREE), (before + SPAN));
	}
	/**
	 * Теперь включаем придержку и повторяем круг
	 */
	{
		// Задаём потолок придержки с запасом под область
		options.spanCache = (SPAN * 4u);
		// Применяем настройки
		awh::alloc::Allocator::options(options);
		// Выдаём область сверх разрядов
		void * span = ::malloc(SPAN);
		// Выдача обязана состояться
		ASSERT_NE(span, nullptr);
		// Запоминаем свободное у кучи до освобождения
		const size_t before = awh::alloc::Allocator::property(awh::alloc::property_t::PAGEFREE);
		// Освобождаем область
		::free(span);
		// Свободное у кучи обязано остаться прежним: область придержана, а не отдана
		EXPECT_EQ(awh::alloc::Allocator::property(awh::alloc::property_t::PAGEFREE), before);
		// Выдаём область того же размера
		void * again = ::malloc(SPAN);
		// Выдача обязана состояться
		ASSERT_NE(again, nullptr);
		// Область обязана достаться придержанная, то есть та же самая
		EXPECT_EQ(again, span);
		// Освобождаем область
		::free(again);
	}
	// Выключаем придержку: придержанное обязано уйти куче немедленно
	options.spanCache = 0;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Свободное у кучи обязано вобрать отданную придержку
	EXPECT_GE(awh::alloc::Allocator::property(awh::alloc::property_t::PAGEFREE), SPAN);
	#endif
}
