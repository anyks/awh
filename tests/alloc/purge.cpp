/**
 * @file purge.cpp
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
 * @brief Тесты возврата памяти системе и потолков занятого
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
 * Подключаем наши модули
 */
#include <alloc/source.hpp>

/**
 * Стандартные модули
 */
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdlib>

/**
 * Под санитайзерами проверять здесь нечего: захват не состоится
 */
#if !AWH_ALLOC_SANITIZED

/**
 * @brief Пространство имён вспомогательных средств
 *
 */
namespace {
	/**
	 * @brief Метод набора и освобождения памяти
	 *
	 * @note Берётся ЗАВЕДОМО много и крупными кусками: отдавать системе распределитель
	 *       вправе лишь целыми страницами, и мелочь россыпью повода к отдаче не даёт
	 *
	 * @param count число блоков
	 * @param size  размер блока
	 *
	 */
	static void churn(const size_t count, const size_t size) noexcept {
		// Выданные блоки
		std::vector <void *> alive;
		// Отводим место под список блоков
		alive.reserve(count);
		/**
		 * Перебираем блоки
		 */
		for(size_t i = 0; i < count; i++){
			// Выдаём блок
			void * block = ::malloc(size);
			// Если блок выдан
			if(block != nullptr){
				// Записываем начало блока
				::memset(block, 0x33, 64);
				// Запоминаем выданный блок
				alive.push_back(block);
			}
		}
		/**
		 * Перебираем выданные блоки
		 */
		for(void * block : alive)
			// Освобождаем блок
			::free(block);
	}
};

/**
 * @brief Тест возврата памяти системе по просьбе
 *
 */
TEST_F(AllocFixture, ManualPurgeReturnsMemory){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём отдачу лишь по просьбе
	options.purgeMode = awh::alloc::purge_t::MANUAL;
	// Снимаем отсрочку: удерживать освобождённое незачем
	options.purgeDelay = 0;
	// Снимаем порог куска
	options.purgeBlock = 0;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Набираем и освобождаем память
	::churn(4096, 8192);
	// Свободного в страничной куче обязано накопиться
	EXPECT_GT(awh::alloc::Allocator::property(awh::alloc::property_t::PAGEFREE), static_cast <size_t> (0));
	// Запоминаем отданное системе до просьбы
	const size_t before = awh::alloc::Allocator::property(awh::alloc::property_t::UNMAPPED);
	// Просим отдать свободное системе
	const size_t purged = awh::alloc::Allocator::purge();
	// Отдача обязана состояться: либо по ответу, либо по счётчику отданного
	EXPECT_TRUE((purged > 0) || (awh::alloc::Allocator::property(awh::alloc::property_t::UNMAPPED) > before));
}
/**
 * @brief Тест полного запрета отдачи
 *
 * @note Запрет обязан быть СИЛЬНЕЕ просьбы: приложение, занявшее память навсегда,
 *       вправе рассчитывать, что случайный вызов `purge` её не отнимет
 *
 */
TEST_F(AllocFixture, NeverPurgeIsStrongerThanRequest){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём полный запрет отдачи
	options.purgeMode = awh::alloc::purge_t::NEVER;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Набираем и освобождаем память
	::churn(2048, 8192);
	// Запоминаем отданное системе до просьбы
	const size_t before = awh::alloc::Allocator::property(awh::alloc::property_t::UNMAPPED);
	// Просьба обязана остаться без последствий
	EXPECT_EQ(awh::alloc::Allocator::purge(), static_cast <size_t> (0));
	// Отданное системе обязано остаться прежним
	EXPECT_EQ(awh::alloc::Allocator::property(awh::alloc::property_t::UNMAPPED), before);
}
/**
 * @brief Тест потолка поток-локальных кэшей
 *
 */
TEST_F(AllocFixture, CacheCeilingHolds){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём потолок поток-локальных кэшей
	options.cacheLimit = (64u * 1024u);
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Набираем и освобождаем память мелкими блоками: их и держат кэши
	::churn(20000, 64);
	/**
	 * Потолок сверяется с запасом
	 *
	 * Потолок этот - на кэш ПОТОКА, а не на процесс: потоков в наборе больше одного,
	 * и требовать от суммы всех кэшей потолка одного было бы неверно
	 */
	EXPECT_LE(awh::alloc::Allocator::property(awh::alloc::property_t::CACHED), (options.cacheLimit * 8));
}
/**
 * @brief Тест опроса состояния занятого
 *
 */
TEST_F(AllocFixture, PropertiesAreConsistent){
	// Выдаём заметный блок
	void * block = ::malloc(1u << 20);
	ASSERT_NE(block, nullptr);
	// Записываем начало блока
	::memset(block, 0x11, 64);
	// Занятое прикладным кодом обязано быть не меньше выданного
	EXPECT_GE(awh::alloc::Allocator::property(awh::alloc::property_t::ALLOCATED), static_cast <size_t> (1u << 20));
	// Наибольшее занятое обязано быть не меньше занятого прямо сейчас
	EXPECT_GE(awh::alloc::Allocator::property(awh::alloc::property_t::PEAK), awh::alloc::Allocator::property(awh::alloc::property_t::ALLOCATED));
	// Взятое у системы обязано покрывать занятое прикладным кодом
	EXPECT_GE(awh::alloc::Allocator::property(awh::alloc::property_t::HEAP), awh::alloc::Allocator::property(awh::alloc::property_t::ALLOCATED));
	// Освобождаем блок
	::free(block);
}


/**
 * @brief Тест потолка кучи
 *
 * @note Потолок заведён для узлов, где распределителю отведена доля памяти машины:
 *       упершись в него, выдача обязана отвечать ОТКАЗОМ, а не брать у системы сверх
 *       заказанного. Отказ этот - обещание настройки, и молчаливое его нарушение
 *       сделало бы потолок украшением
 *
 */
TEST_F(AllocFixture, HeapCeilingRefusesBeyondLimit){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Запоминаем взятое у системы прямо сейчас
	const size_t taken = awh::alloc::Allocator::property(awh::alloc::property_t::HEAP);
	/**
	 * Задаём потолок с небольшим запасом над взятым
	 *
	 * Потолок НИЖЕ взятого задавать нельзя: уже выданное никуда не денется, а всякая
	 * последующая выдача отвечала бы отказом - в том числе выдача самих проверок
	 */
	options.heapLimit = (taken + (2u * 1024u * 1024u));
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Выданные блоки
	std::vector <void *> alive;
	// Признак состоявшегося отказа
	bool refused = false;
	/**
	 * Выдаём блоки сверх разрядов, покуда не упрёмся в потолок
	 */
	for(size_t i = 0; i < 64; i++){
		// Выдаём блок сверх разрядов
		void * block = ::malloc(256u * 1024u);
		// Если выдача ответила отказом
		if(block == nullptr){
			// Отмечаем состоявшийся отказ
			refused = true;
			// Упираться дальше незачем
			break;
		}
		// Запоминаем выданный блок
		alive.push_back(block);
	}
	// Отказ обязан состояться: запаса над потолком меньше, чем запрошено
	EXPECT_TRUE(refused);
	/**
	 * Сверяем ПРИРОСТ взятого, а не полное число
	 *
	 * Потолок сторожат страничная куча и слой крупных выдач; страницы же заслонов
	 * берутся у источника напрямую и в потолок не входят - а `HEAP` докладывает всё
	 * взятое, заслоны в том числе. Заслонённые блоки, оставшиеся от соседних проверок,
	 * дают здесь лишний десяток мегабайт, к этой проверке отношения не имеющий.
	 * Прирост же за время самой проверки заслонов не содержит и обязан уложиться в
	 * заказанный запас с точностью до куска кучи
	 */
	// Определяем взятое у системы после упора в потолок
	const size_t after = awh::alloc::Allocator::property(awh::alloc::property_t::HEAP);
	// Прирост обязан уложиться в заказанный запас с точностью до куска кучи
	EXPECT_LE((after - taken), ((2u * 1024u * 1024u) + (4u * 1024u * 1024u)));
	/**
	 * Снимаем потолок ПРЕЖДЕ освобождения
	 *
	 * Освобождение памяти при упёртой в потолок куче ничего не ломает, но соседние
	 * проверки идут в том же процессе: оставь мы потолок - отказом отвечала бы уже их
	 * выдача, и отказ этот выглядел бы их собственным
	 */
	options.heapLimit = 0;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	/**
	 * Со снятым потолком выдача обязана вновь состояться
	 *
	 * Упор в потолок это ОТКАЗ, а не поломка: куча, отказавшая в росте, обязана
	 * остаться пригодной к работе целиком. Отказ приходит посреди взятия нового куска,
	 * когда учётные записи уже выданы, а память у источника ещё нет либо уже взята, - и
	 * всякий незакрытый откат на этом пути оставил бы кучу в полусобранном виде: кусок в
	 * списках, но не в таблице поиска, или взятое у источника, о котором куча не знает.
	 * Наружу такая поломка вышла бы не отказом, а много позже - разбором чужого адреса
	 */
	void * revived = ::malloc(256u * 1024u);
	// Выдача со снятым потолком обязана состояться
	EXPECT_NE(revived, nullptr);
	// Если выдача состоялась
	if(revived != nullptr){
		// Блок обязан быть пригоден к записи целиком
		::memset(revived, 0x5A, (256u * 1024u));
		// Разбор адреса обязан признать блок нашим
		EXPECT_GE(awh::alloc::Allocator::resolve(revived).size, static_cast <size_t> (256u * 1024u));
		// Освобождаем выданное
		::free(revived);
	}
	/**
	 * Перебираем выданные блоки
	 */
	for(void * block : alive)
		// Освобождаем блок
		::free(block);
}


/**
 * @brief Тест доклада о крупной выдаче
 *
 * @note Доклад обещан настройкой `reportLarge` всякой выдаче сверх порога. Идёт он
 *       ПОТОКОМ доклада, а не изнутри выдачи: отклик прикладного кода сам обращается
 *       за памятью, и звать его из выдачи значило бы уйти в возвратность
 *
 * @warning Отклик снимается в конце ОБЯЗАТЕЛЬНО: он живёт в переменных процесса и
 *          переживает проверку, а замыкание его смотрит на местную переменную -
 *          оставленный, он ловил бы выдачи соседних проверок по снесённому адресу
 *
 */
TEST_F(AllocFixture, LargeAllocationIsReported){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Задаём порог доклада о крупной выдаче
	options.reportLarge = (512u * 1024u);
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Число докладов о крупной выдаче
	std::atomic <size_t> reported(0);
	// Наибольший доложенный размер
	std::atomic <size_t> largest(0);
	// Ставим отклик на крупную выдачу
	awh::alloc::Allocator::onLarge([&reported, &largest](const void * ptr, const size_t size) noexcept {
		// Адрес доложенного блока здесь не нужен
		static_cast <void> (ptr);
		// Считаем доклад
		reported.fetch_add(1, std::memory_order_relaxed);
		// Запоминаем наибольший доложенный размер
		if(size > largest.load(std::memory_order_relaxed))
			// Запоминаем наибольший доложенный размер
			largest.store(size, std::memory_order_relaxed);
	});
	// Выдаём блок сверх порога доклада
	void * block = ::malloc(1024u * 1024u);
	// Выдача обязана состояться
	ASSERT_NE(block, nullptr);
	/**
	 * Ждём доклада СРОКОМ, а не числом заходов
	 *
	 * Поток доклада ходит по кольцу не чаще, чем раз в полсотни миллисекунд, и
	 * проверять сразу после выдачи значило бы проверять до доклада
	 */
	for(size_t i = 0; (i < 200) && (reported.load(std::memory_order_relaxed) == 0); i++)
		// Ждём между заходами
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	// Доклад о крупной выдаче обязан состояться
	EXPECT_GT(reported.load(std::memory_order_relaxed), static_cast <size_t> (0));
	// Доложенный размер обязан покрывать запрошенное
	EXPECT_GE(largest.load(std::memory_order_relaxed), (1024u * 1024u));
	/**
	 * Снимаем отклик ПРЕЖДЕ освобождения
	 *
	 * Снятый позже, он успел бы поймать выдачу, случившуюся между освобождением и
	 * снятием, - и смотрел бы на переменные, уходящие вместе с проверкой
	 */
	awh::alloc::Allocator::onLarge(nullptr);
	// Освобождаем блок
	::free(block);
}

/**
 * @brief Тест просьбы о крупных страницах
 *
 * @note Настройка эта - именно ПРОСЬБА: крупные страницы у всех наших систем требуют
 *       либо заранее отведённого запаса, либо особого права, и отказ в них - обычный
 *       исход, а не дефект. Проверяется здесь не то, что они достались, а то, что
 *       выдача при этой просьбе работает всё так же: отказ обязан быть молчаливым
 *       ОТСТУПЛЕНИЕМ к обычным страницам, а не отказом выдачи
 *
 */
TEST_F(AllocFixture, SuperpagesRequestKeepsAllocationWorking){
	// Получаем действующие настройки
	awh::alloc::options_t options = awh::alloc::Allocator::options();
	// Просим у системы крупные страницы
	options.hugePages = true;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
	// Выданные блоки
	std::vector <void *> alive;
	/**
	 * Выдаём блоки всех трёх путей: разрядного, страничного и крупного
	 */
	for(size_t size : {size_t(64), size_t(4096), size_t(64u * 1024u), size_t(5u * 1024u * 1024u)}){
		// Выдаём блок требуемого размера
		void * block = ::malloc(size);
		// Выдача обязана состояться и при отказе в крупных страницах
		ASSERT_NE(block, nullptr) << "выдача отказала на размере " << size;
		// Записываем блок целиком: страницы обязаны быть настоящими
		::memset(block, 0x6B, size);
		// Блок обязан быть узнан нашим
		EXPECT_GE(awh::alloc::Allocator::resolve(block).size, size);
		// Запоминаем выданный блок
		alive.push_back(block);
	}
	/**
	 * Перебираем выданные блоки
	 */
	for(void * block : alive)
		// Освобождаем блок
		::free(block);
	// Снимаем просьбу о крупных страницах
	options.hugePages = false;
	// Применяем настройки
	awh::alloc::Allocator::options(options);
}


/**
 * @brief Тест схождения счёта занятого
 *
 * @note Счёт занятого прикладным кодом обязан ВЕРНУТЬСЯ к прежнему после того, как всё
 *       выданное освобождено. Расхождение здесь означает, что выдача и освобождение
 *       считают РАЗНОЕ: так и было - выдача сверх разрядов учитывала округлённый
 *       запрос, а освобождение списывало размер выданной области, какая вправе быть
 *       крупнее. Счёт уходил в минус, и опрос расхода отвечал нулём при занятых
 *       мегабайтах
 *
 * @warning Проверка идёт по ВСЕМ трём путям выдачи разом - разрядному, страничному и
 *          крупному: расхождение было лишь у одного из них, и путь, взятый в
 *          одиночку, его бы не показал
 *
 */
TEST_F(AllocFixture, AccountingBalancesAcrossPaths){
	// Запоминаем занятое прикладным кодом до выдачи
	const size_t before = awh::alloc::Allocator::property(awh::alloc::property_t::ALLOCATED);
	// Выданные блоки
	std::vector <void *> alive;
	/**
	 * Выдаём блоки всех трёх путей вперемешку
	 */
	for(size_t round = 0; round < 64; round++){
		// Перебираем размеры всех трёх путей
		for(size_t size : {size_t(64), size_t(1024), size_t(32768), size_t(96u * 1024u), size_t(5u * 1024u * 1024u)}){
			// Выдаём блок требуемого размера
			void * block = ::malloc(size + round);
			// Если блок выдан
			if(block != nullptr)
				// Запоминаем выданный блок
				alive.push_back(block);
		}
	}
	// Занятое обязано вырасти
	EXPECT_GT(awh::alloc::Allocator::property(awh::alloc::property_t::ALLOCATED), before);
	/**
	 * Перебираем выданные блоки
	 */
	for(void * block : alive)
		// Освобождаем блок
		::free(block);
	/**
	 * Счёт обязан сойтись с прежним
	 *
	 * Сходится он не до байта: соседние потоки живут своей жизнью, а карантин держит
	 * освобождённое. Запас берётся заведомо меньше выданного - без правки расхождение
	 * составляло мегабайты
	 */
	const size_t after = awh::alloc::Allocator::property(awh::alloc::property_t::ALLOCATED);
	// Разница обязана уложиться в запас
	EXPECT_LE(((after > before) ? (after - before) : (before - after)), static_cast <size_t> (256u * 1024u));
	/**
	 * Повторяем то же под ПОТОЛКОМ кучи
	 *
	 * Расхождение выдачи с освобождением вылезало именно там: под потолком куче не
	 * хватает учётных записей под остаток, и области выдаются ЦЕЛИКОМ, крупнее
	 * округлённого запроса. Без этого условия проверка проходит и на порченом коде -
	 * сверено отключением правки
	 */
	{
		// Получаем действующие настройки
		awh::alloc::options_t options = awh::alloc::Allocator::options();
		// Задаём потолок с небольшим запасом над взятым
		options.heapLimit = (awh::alloc::Allocator::property(awh::alloc::property_t::HEAP) + (2u * 1024u * 1024u));
		// Применяем настройки
		awh::alloc::Allocator::options(options);
		// Выданные под потолком блоки
		std::vector <void *> jammed;
		/**
		 * Выдаём блоки сверх разрядов, покуда не упрёмся в потолок
		 */
		for(size_t i = 0; i < 128; i++){
			// Выдаём блок сверх разрядов
			void * block = ::malloc(96u * 1024u);
			// Если выдача ответила отказом
			if(block == nullptr)
				// Упираться дальше незачем
				break;
			// Запоминаем выданный блок
			jammed.push_back(block);
		}
		/**
		 * Перебираем выданные под потолком блоки
		 */
		for(void * block : jammed)
			// Освобождаем блок
			::free(block);
		// Снимаем потолок
		options.heapLimit = 0;
		// Применяем настройки
		awh::alloc::Allocator::options(options);
		// Выдаём блок после всего
		void * last = ::malloc(64u * 1024u);
		// Выдача обязана состояться
		ASSERT_NE(last, nullptr);
		/**
		 * Занятое обязано быть НЕНУЛЕВЫМ
		 *
		 * Нуль здесь означает счёт, ушедший в минус: живой блок есть, а расход
		 * докладывается пустым
		 */
		EXPECT_GT(awh::alloc::Allocator::property(awh::alloc::property_t::ALLOCATED), static_cast <size_t> (0));
		// Освобождаем блок
		::free(last);
	}
}

/**
 * @brief Тест согласия доклада о взятом у системы со счётом источника
 *
 * @details Доклад обязан сходиться со счётом самого источника ТОЧНО, а не примерно
 *
 * @note Прежде доклад складывал слои - кучу, крупные выдачи и заслоны, - и оттого не
 *       считал того, что идёт у источника мимо слоёв: собственный учёт кучи, кольцо
 *       карантина, таблицы учёта мест выдачи. Замерено щупом: двадцать тысяч выдач без
 *       карантина и заслонов дали доклад в 50 331 648 байт против 65 830 912 у
 *       источника - пятая часть взятого у системы не докладывалась вовсе
 *
 * @note Важно это не само по себе: потолок взятого сторожит счёт ИСТОЧНИКА. Приложение,
 *       задающее потолок по числу из этого опроса, упиралось бы в него раньше, чем
 *       ждало, - и разница росла бы с раздробленностью кучи
 *
 */
TEST_F(AllocFixture, HeapReportMatchesSourceAccounting){
	/**
	 * Под санитайзером спрашивать нечего: захват не состоится
	 */
	#if AWH_ALLOC_SANITIZED
		GTEST_SKIP() << "захват под санитайзером не состоится";
	#else
		// Получаем действующий источник страниц
		awh::alloc::source_t * source = awh::alloc::Allocator::source();
		// Источник обязан быть заведён
		ASSERT_NE(source, nullptr);
		// Выданные блоки
		std::vector <void *> alive;
		/**
		 * Нагружаем кучу выдачами изменчивого размера
		 *
		 * Изменчивого намеренно: собственный учёт кучи растёт от ДРОБЛЕНИЯ областей, а
		 * не от числа выдач, и на блоках одного размера расхождение не набралось бы
		 */
		for(size_t i = 0; i < 4096; i++){
			// Выдаём блок изменчивого размера
			void * block = ::malloc(64 + ((i * 37) % 8192));
			// Если выдача состоялась
			if(block != nullptr)
				// Запоминаем выданный блок
				alive.push_back(block);
		}
		// Определяем взятое у системы по докладу
		const size_t reported = awh::alloc::Allocator::property(awh::alloc::property_t::HEAP);
		// Определяем взятое у системы по счёту источника
		const size_t taken = reinterpret_cast <awh::alloc::SystemSource *> (source)->taken();
		// Доклад обязан сходиться со счётом источника точно
		EXPECT_EQ(reported, taken) << "доклад расходится со счётом источника на "
		 << (static_cast <int64_t> (taken) - static_cast <int64_t> (reported)) << " байт";
		/**
		 * Перебираем выданные блоки
		 */
		for(void * block : alive)
			// Освобождаем блок
			::free(block);
	#endif
}

#endif // !AWH_ALLOC_SANITIZED
