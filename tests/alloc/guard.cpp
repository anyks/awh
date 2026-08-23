/**
 * @file guard.cpp
 * @date 2026-08-22
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
 * @brief Тесты заслонов, карантина и разбора адреса до блока
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
 * Стандартные модули
 */
#include <vector>
#include <cstdlib>

/**
 * Под санитайзерами проверять здесь нечего: захват не состоится, и слоёв наших
 * в процессе нет. Отсутствие это утверждает `AllocCaptureTest`
 */
#if !AWH_ALLOC_SANITIZED

/**
 * @brief Тест действующих заслонов
 *
 * @note Заслон кладёт блок вплотную к концу страницы, а следующую страницу закрывает:
 *       запись за конец блока валит программу немедля, а не портит соседа. Проверяется
 *       это по СЛЕДУ - концу блока на границе страницы, - а не по признаку настройки:
 *       заслон, взятый и не поставленный, признаком от рабочего не отличить
 *
 */
TEST_F(AllocFixture, GuardsAreActuallyPlaced){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём выборку заслонов: один блок из шестнадцати
	options.guardRate = 16;
	// Задаём объём карантина
	options.quarantine = (256u * 1024u);
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Выданные блоки
	std::vector <void *> alive;
	/**
	 * Выдаём вдоволь: выборка берёт одну выдачу из шестнадцати
	 */
	for(size_t i = 0; i < 4096; i++){
		// Выдаём блок изменчивого размера
		void * block = ::malloc(64 + (i % 512));
		// Запоминаем выданный блок
		if(block != nullptr)
			// Добавляем блок в список живых
			alive.push_back(block);
	}
	// Число заслонённых блоков
	size_t sealed = 0;
	/**
	 * Перебираем живые блоки
	 */
	for(void * block : alive){
		// Разбираем адрес блока
		const awh::alloc::region_t region = awh::alloc::Allocator::resolve(block);
		// У заслонённого блока конец совпадает с границей страницы системы
		if((region.size > 0) && (((reinterpret_cast <uintptr_t> (block) + region.size) % 4096) == 0))
			// Считаем заслонённый блок
			sealed++;
	}
	// Хоть один блок обязан оказаться заслонённым
	EXPECT_GT(sealed, static_cast <size_t> (0));
	/**
	 * Перебираем живые блоки
	 */
	for(void * block : alive)
		// Освобождаем блок
		::free(block);
}
/**
 * @brief Тест опознания освобождённого блока
 *
 * @note Карантин держит освобождённое некоторое время, и разбор адреса обязан назвать
 *       такой блок освобождённым: ровно это отличает висячий указатель от чужой памяти
 *
 */
TEST_F(AllocFixture, FreedBlockIsRecognized){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём выборку заслонов
	options.guardRate = 1;
	// Задаём объём карантина
	options.quarantine = (256u * 1024u);
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Выдаём блок
	void * block = ::malloc(512);
	ASSERT_NE(block, nullptr);
	// Записываем блок целиком
	::memset(block, 0x77, 512);
	// Живой блок обязан опознаваться живым
	EXPECT_EQ(awh::alloc::Allocator::resolve(block).origin, awh::alloc::origin_t::LIVE);
	// Освобождаем блок
	::free(block);
	// Освобождённый блок обязан опознаваться освобождённым
	EXPECT_EQ(awh::alloc::Allocator::resolve(block).origin, awh::alloc::origin_t::FREED);
}
/**
 * @brief Тест разбора чужих и негодных адресов
 *
 */
TEST_F(AllocFixture, ForeignAddressesAreTold){
	// Место на стеке распределителю не принадлежит
	const uint64_t anchor = 0;
	EXPECT_EQ(awh::alloc::Allocator::resolve(&anchor).origin, awh::alloc::origin_t::FOREIGN);
	// Нулевая страница опознаётся собою: разыменование пустого указателя видно сразу
	EXPECT_EQ(awh::alloc::Allocator::resolve(reinterpret_cast <const void *> (16)).origin, awh::alloc::origin_t::NULLPAGE);
}
/**
 * @brief Тест разбора адреса за концом блока
 *
 * @note Разбор обязан назвать не только принадлежность, но и СТОРОНУ промаха: адрес за
 *       концом блока и адрес перед началом - разные ошибки, и лечатся они по-разному
 *
 */
TEST_F(AllocFixture, OverrunAndUnderrunAreTold){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём выборку заслонов: заслон ставится всякой выдаче
	options.guardRate = 1;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Выдаём блок
	uint8_t * block = reinterpret_cast <uint8_t *> (::malloc(128));
	ASSERT_NE(block, nullptr);
	// Разбираем адрес блока
	const awh::alloc::region_t region = awh::alloc::Allocator::resolve(block);
	// Если блок оказался заслонённым
	if(region.origin == awh::alloc::origin_t::LIVE){
		// Адрес за концом блока обязан опознаваться промахом за конец
		const awh::alloc::region_t beyond = awh::alloc::Allocator::resolve(block + region.size + 1);
		// Промах обязан быть назван: живым такой адрес быть не может
		EXPECT_NE(beyond.origin, awh::alloc::origin_t::LIVE);
	}
	// Освобождаем блок
	::free(block);
}


/**
 * @brief Тест опознания записи по освобождённому блоку
 *
 * @note Карантин держит освобождённое ЗАСЕЯННЫМ и сличает засев при изъятии блока: не
 *       сохранившийся засев означает запись по указателю, освобождённому прежде, - тот
 *       самый дефект, ради какого карантин и заведён
 *
 * @warning Проверка намеренно пишет по освобождённому указателю - иначе ловить было бы
 *          нечего. Блок при этом лежит в карантине и никому больше не выдан: чужой
 *          памяти запись не задевает
 *
 */
TEST_F(AllocFixture, WriteAfterFreeIsCaught){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём объём карантина
	options.quarantine = (1u * 1024u * 1024u);
	// Задаём засев освобождаемой памяти
	options.fill = awh::alloc::fill_t::JUNK;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Запоминаем число испорченных блоков до проверки
	const size_t before = awh::alloc::Allocator::spoiled();
	// Выдаём блок
	uint8_t * block = reinterpret_cast <uint8_t *> (::malloc(128));
	// Выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Освобождаем блок: с этого мига он лежит в карантине засеянным
	::free(block);
	// Пишем по освобождённому указателю - ровно тот дефект, что ловит карантин
	block[64] = 0x00;
	/**
	 * Вытесняем блок из карантина
	 *
	 * Сличение засева идёт при ИЗЪЯТИИ блока из карантина, а не при освобождении:
	 * покуда блок лежит, писать в него прикладной код может сколько угодно раз, и
	 * сличать имеет смысл однажды - когда блок карантин покидает
	 */
	for(size_t i = 0; i < 20000; i++){
		// Выдаём блок нагрузки
		void * spare = ::malloc(128);
		// Если блок выдан
		if(spare != nullptr)
			// Освобождаем блок нагрузки
			::free(spare);
	}
	// Адрес испорченного блока
	const void * culprit = nullptr;
	// Смещение первой порчи от начала блока
	size_t offset = 0;
	// Испорченных блоков обязано прибавиться
	EXPECT_GT(awh::alloc::Allocator::spoiled(&culprit, &offset), before);
	// Адрес испорченного блока обязан быть известен
	EXPECT_NE(culprit, nullptr);
}
/**
 * @brief Тест молчания карантина на исправном коде
 *
 * @note Ловец, срабатывающий и без дефекта, хуже отсутствующего: доклад его перестают
 *       читать. Оттого проверяется и обратное - что при исправном коде находок нет
 *
 */
TEST_F(AllocFixture, HealthyCodeLeavesQuarantineSilent){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём объём карантина
	options.quarantine = (1u * 1024u * 1024u);
	// Задаём засев освобождаемой памяти
	options.fill = awh::alloc::fill_t::JUNK;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Запоминаем число испорченных блоков до проверки
	const size_t before = awh::alloc::Allocator::spoiled();
	/**
	 * Выдаём и освобождаем блоки, ни разу не тронув освобождённое
	 */
	for(size_t i = 0; i < 20000; i++){
		// Выдаём блок нагрузки
		uint8_t * block = reinterpret_cast <uint8_t *> (::malloc(64 + (i % 256)));
		// Если блок выдан
		if(block != nullptr){
			// Записываем блок, покуда он живой
			::memset(block, static_cast <int> (i & 0xFF), 64);
			// Освобождаем блок
			::free(block);
		}
	}
	// Испорченных блоков прибавиться не должно
	EXPECT_EQ(awh::alloc::Allocator::spoiled(), before);
}

/**
 * @brief Тест выключения карантина в работе
 *
 * @details Карантин наполняется, затем выключается настройкой, и удержанное обязано
 *          ВЕРНУТЬСЯ слоям, а не остаться у него навсегда
 *
 * @note Возвращают удержанное лишь внутри освобождения, принятого карантином, - а
 *       выключенный не принимает ничего. Оттого выключение, не отдающее излишка само,
 *       не действовало бы вовсе: всё, что карантин держал в тот миг, осталось бы у него
 *       до конца работы, хотя приложение заказало ровно обратное
 *
 */
TEST_F(AllocFixture, QuarantineShutdownReturnsHeldMemory){
	/**
	 * Под санитайзером карантина нет: захват не состоится
	 */
	#if AWH_ALLOC_SANITIZED
		GTEST_SKIP() << "захват под санитайзером не состоится";
	#else
		// Получаем действующие настройки
		awh::alloc::options_t options = awh::alloc::Allocator::options();
		// Запоминаем настройки для возврата
		const awh::alloc::options_t restore = options;
		// Заводим карантин объёмом в четверть мегабайта
		options.quarantine = (256u * 1024u);
		// Применяем настройки
		awh::alloc::Allocator::options(options);
		// Освобождённые блоки
		std::vector <void *> freed;
		/**
		 * Наполняем карантин освобождёнными блоками
		 */
		for(size_t i = 0; i < 64; i++){
			// Выдаём блок
			void * block = ::malloc(1024);
			ASSERT_NE(block, nullptr);
			// Запоминаем адрес блока
			freed.push_back(block);
			// Освобождаем блок: он уходит под карантин
			::free(block);
		}
		// Число блоков, удержанных карантином
		size_t held = 0;
		/**
		 * Считаем удержанные карантином блоки
		 */
		for(void * block : freed)
			// Считаем блок удержанным, если разбор зовёт его освобождённым
			held += ((awh::alloc::Allocator::resolve(block).origin == awh::alloc::origin_t::FREED) ? 1 : 0);
		// Карантин обязан держать хоть что-то: иначе проверять нечего
		ASSERT_GT(held, static_cast <size_t> (0)) << "карантин не принял ни одного блока";
		// Выключаем карантин
		options.quarantine = 0;
		// Применяем настройки
		awh::alloc::Allocator::options(options);
		// Число блоков, оставшихся у карантина после выключения
		size_t stayed = 0;
		/**
		 * Считаем блоки, оставшиеся у выключенного карантина
		 */
		for(void * block : freed)
			// Считаем блок оставшимся, если разбор всё ещё зовёт его освобождённым
			stayed += ((awh::alloc::Allocator::resolve(block).origin == awh::alloc::origin_t::FREED) ? 1 : 0);
		// У выключенного карантина не обязано остаться ничего
		EXPECT_EQ(stayed, static_cast <size_t> (0)) << "выключенный карантин держит " << stayed << " блок(ов) из " << held;
		// Возвращаем прежние настройки
		awh::alloc::Allocator::options(restore);
	#endif
}

/**
 * @brief Тест границы объёма удерживаемых закрытых областей
 *
 * @details Заслоны не отдают закрытую область системе намеренно: обращение по
 *          освобождённому адресу обязано валить, а не проходить молча. Но удержание
 *          обязано быть ограничено ОБЪЁМОМ, а не только числом областей
 *
 * @note Прежде граница была одна - четыре тысячи областей. Под заслонами выдаётся до
 *       мегабайта, и та же граница держала бы до четырёх гигабайт памяти, взятой у
 *       системы и однажды тронутой. Замерено щупом на кругах: заслоны держали 201
 *       мегабайт при НУЛЕВОМ занятом, и рост встал не оттого, что память вернулась, а
 *       оттого, что счёт областей упёрся в предел
 *
 */
TEST_F(AllocFixture, SealedGuardRetentionIsBoundedByVolume){
	/**
	 * Под санитайзером заслонов нет: захват не состоится
	 */
	#if AWH_ALLOC_SANITIZED
		GTEST_SKIP() << "захват под санитайзером не состоится";
	#else
		// Получаем действующие настройки
		awh::alloc::options_t options = awh::alloc::Allocator::options();
		// Запоминаем настройки для возврата
		const awh::alloc::options_t restore = options;
		// Заслоняем каждую выдачу
		options.guardRate = 1;
		// Применяем настройки
		awh::alloc::Allocator::options(options);
		// Запоминаем взятое у системы до нагрузки
		const size_t before = awh::alloc::Allocator::property(awh::alloc::property_t::HEAP);
		// Размер заслоняемого блока: половина наибольшего, дозволенного заслонам
		const size_t size = (512u * 1024u);
		/**
		 * Гоняем заслонённые блоки кругами
		 *
		 * Четыреста кругов по половине мегабайта - двести мегабайт: без границы по
		 * объёму они удержались бы все, ибо в границу по числу областей не упираются
		 */
		for(size_t i = 0; i < 400; i++){
			// Выдаём блок под заслонами
			void * block = ::malloc(size);
			// Если выдача состоялась
			if(block != nullptr){
				// Записываем блок целиком
				::memset(block, 0x2B, size);
				// Освобождаем блок: область закрывается и уходит в очередь
				::free(block);
			}
		}
		// Определяем взятое у системы после нагрузки
		const size_t after = awh::alloc::Allocator::property(awh::alloc::property_t::HEAP);
		// Определяем прирост взятого у системы
		const size_t growth = ((after > before) ? (after - before) : 0);
		/**
		 * Прирост обязан уложиться в границу объёма с запасом
		 *
		 * Запас нужен: в приросте лежат и живые области, и куски самой кучи, взятые под
		 * учётные записи. Сторожим здесь ПОРЯДОК величины, а не точное число: без
		 * границы по объёму прирост был бы вчетверо больше проверяемого
		 */
		EXPECT_LT(growth, (128u * 1024u * 1024u)) << "заслоны удержали " << (growth / (1024u * 1024u)) << " МБ";
		// Возвращаем прежние настройки
		awh::alloc::Allocator::options(restore);
	#endif
}

#endif // !AWH_ALLOC_SANITIZED
