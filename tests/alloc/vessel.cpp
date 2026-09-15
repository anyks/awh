/**
 * @file vessel.cpp
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
 * @brief Проверки приёмника тайн и смены доступности защищённых блоков
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
#include <alloc/vessel.hpp>
#include <alloc/prompt.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <atomic>
#include <thread>

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
	 * @note Строгий приёмник отвечает отказом там, где система не даёт запереть
	 *       страницы: право это ограничено пределом `RLIMIT_MEMLOCK`, а у illumos
	 *       требует прав. Проверки существа от окружения зависеть не должны, оттого
	 *       берут они приёмник послабленный, а строгость поверяется своей проверкой
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
 * @brief Проверка побайтного приёма содержимого
 *
 */
TEST(AllocVesselTest, ContentIsPouredByteByByte){
	// Принимаемое содержимое
	const uint8_t source[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
	// Заводим приёмник
	awh::alloc::vessel_t vessel = ::relaxed();
	// Заводим приёмник под приём содержимого
	ASSERT_TRUE(vessel.reserve(sizeof(source)));
	// Приёмник обязан считаться заведённым
	EXPECT_TRUE(vessel.exists());
	// Ёмкость обязана совпасть с затребованной
	EXPECT_EQ(vessel.capacity(), sizeof(source));
	// Занятого в приёмнике пока быть не должно
	EXPECT_EQ(vessel.size(), static_cast <size_t> (0));
	/**
	 * Выполняем перебор всех байтов принимаемого содержимого
	 */
	for(size_t i = 0; i < sizeof(source); i++)
		// Принимаем очередной байт содержимого
		ASSERT_TRUE(vessel.pour(source[i]));
	// Занятое обязано совпасть с принятым
	EXPECT_EQ(vessel.size(), sizeof(source));
	// Признак совпадения содержимого
	bool matched = false;
	// Обращаемся к содержимому приёмника
	ASSERT_TRUE(vessel.apply([&](const uint8_t * data, const size_t size) noexcept {
		// Запоминаем признак совпадения содержимого
		matched = ((size == sizeof(source)) && (::memcmp(data, source, sizeof(source)) == 0));
	}));
	// Содержимое обязано вернуться неизменным
	EXPECT_TRUE(matched);
}
/**
 * @brief Проверка отказа приёма сверх ёмкости
 *
 * @note Приёмник НЕ растёт намеренно: рост означал бы перевыделение, а перевыделение -
 *       копию прежнего содержимого в куче, какую никто не затирает
 *
 */
TEST(AllocVesselTest, VesselRefusesToGrowBeyondCapacity){
	// Заводим приёмник
	awh::alloc::vessel_t vessel = ::relaxed();
	// Заводим приёмник ёмкостью в четыре байта
	ASSERT_TRUE(vessel.reserve(4));
	/**
	 * Выполняем приём содержимого по ёмкости
	 */
	for(size_t i = 0; i < 4; i++)
		// Принимаем очередной байт содержимого
		ASSERT_TRUE(vessel.pour(static_cast <uint8_t> (i)));
	// Приём сверх ёмкости обязан отвечать отказом
	EXPECT_FALSE(vessel.pour(0xFF));
	// Занятое от отказа расти не должно
	EXPECT_EQ(vessel.size(), static_cast <size_t> (4));
	// Ёмкость от отказа меняться не должна
	EXPECT_EQ(vessel.capacity(), static_cast <size_t> (4));
}
/**
 * @brief Проверка запечатывания при выходе исключением
 *
 * @note Запечатывание ведётся деструктором ради ровно этого случая: уход из
 *       обработчика исключением не должен оставлять приёмник раскрытым
 *
 */
TEST(AllocVesselTest, VesselIsSealedBackWhenHandlerThrows){
	// Заводим приёмник
	awh::alloc::vessel_t vessel = ::relaxed();
	// Заводим приёмник под приём содержимого
	ASSERT_TRUE(vessel.reserve(4));
	// Принимаем содержимое в приёмник
	ASSERT_TRUE(vessel.pour(0x11));
	/**
	 * Выполняем обращение к содержимому, уходя из него исключением
	 */
	try {
		// Обращаемся к содержимому приёмника
		static_cast <void> (vessel.apply([](const uint8_t *, const size_t){
			// Уходим из обработчика исключением
			throw std::runtime_error("уход из обработчика");
		}));
		// Исключение обязано было уйти наружу
		FAIL() << "Исключение из обработчика не вышло наружу";
	/**
	 * Перехватываем исключение обработчика
	 */
	} catch(const std::runtime_error &) {
		// Признак совпадения содержимого
		bool matched = false;
		// Приёмник обязан работать по-прежнему
		ASSERT_TRUE(vessel.apply([&](const uint8_t * data, const size_t size) noexcept {
			// Запоминаем признак совпадения содержимого
			matched = ((size == 1) && (data[0] == 0x11));
		}));
		// Содержимое обязано вернуться неизменным
		EXPECT_TRUE(matched);
		// Приём после исключения обязан идти по-прежнему
		EXPECT_TRUE(vessel.pour(0x22));
	}
}
/**
 * @brief Проверка вложенного обращения к содержимому
 *
 * @note Раскрытия считаются: внутреннее обращение, закончившись, НЕ вправе закрыть
 *       область, покуда её держит внешнее. Иначе внешний обработчик продолжил бы
 *       работу по недоступной памяти - отказ доступа посреди своей же работы
 *
 * @note Проверка эта и `RevealCountSurvivesTwoThreadsAtOnce` ДУБЛИРУЮЩИМИ НЕ ЯВЛЯЮТСЯ:
 *       обе валятся от снятия счёта держателей, но эта ловит беду устройства одним
 *       потоком, а та - беду гонки, какой у одного потока нет вовсе. Довод записан
 *       затем, что снимают обычно ту, что выглядит дублирующей
 *
 */
TEST(AllocVesselTest, NestedAccessKeepsTheVesselOpen){
	// Заводим приёмник
	awh::alloc::vessel_t vessel = ::relaxed();
	// Заводим приёмник под приём содержимого
	ASSERT_TRUE(vessel.reserve(2));
	// Принимаем содержимое в приёмник
	ASSERT_TRUE(vessel.pour(0xA1));
	// Принимаем содержимое в приёмник
	ASSERT_TRUE(vessel.pour(0xA2));
	// Признак совпадения содержимого после вложенного обращения
	bool matched = false;
	// Обращаемся к содержимому приёмника
	ASSERT_TRUE(vessel.apply([&](const uint8_t * data, const size_t size) noexcept {
		// Выполняем вложенное обращение к содержимому приёмника
		static_cast <void> (vessel.apply([](const uint8_t *, const size_t) noexcept {}));
		/**
		 * Содержимое обязано остаться доступным
		 *
		 * Закройся область вложенным обращением - чтение это отвечало бы отказом
		 * доступа, а не байтами
		 */
		matched = ((size == 2) && (data[0] == 0xA1) && (data[1] == 0xA2));
	}));
	// Содержимое обязано пережить вложенное обращение
	EXPECT_TRUE(matched);
}
/**
 * @brief Проверка затирания приёмника
 *
 */
TEST(AllocVesselTest, WipedVesselHoldsNothing){
	// Заводим приёмник
	awh::alloc::vessel_t vessel = ::relaxed();
	// Заводим приёмник под приём содержимого
	ASSERT_TRUE(vessel.reserve(8));
	// Принимаем содержимое в приёмник
	ASSERT_TRUE(vessel.pour(0x5A));
	// Затираем и освобождаем приёмник
	vessel.wipe();
	// Приёмник обязан считаться незаведённым
	EXPECT_FALSE(vessel.exists());
	// Ёмкость обязана обнулиться
	EXPECT_EQ(vessel.capacity(), static_cast <size_t> (0));
	// Занятое обязано обнулиться
	EXPECT_EQ(vessel.size(), static_cast <size_t> (0));
	// Приём в затёртый приёмник обязан отвечать отказом
	EXPECT_FALSE(vessel.pour(0x00));
	// Обращение к затёртому приёмнику обязано отвечать отказом
	EXPECT_FALSE(vessel.apply([](const uint8_t *, const size_t) noexcept {}));
}
/**
 * @brief Проверка переноса приёмника
 *
 */
TEST(AllocVesselTest, MovedVesselKeepsContentAndEmptiesSource){
	// Заводим приёмник
	awh::alloc::vessel_t vessel = ::relaxed();
	// Заводим приёмник под приём содержимого
	ASSERT_TRUE(vessel.reserve(4));
	// Принимаем содержимое в приёмник
	ASSERT_TRUE(vessel.pour(0xC0));
	// Принимаем содержимое в приёмник
	ASSERT_TRUE(vessel.pour(0xC1));
	// Переносим приёмник в другой объект
	awh::alloc::vessel_t moved = std::move(vessel);
	// Прежний приёмник обязан остаться пустым
	EXPECT_FALSE(vessel.exists());
	// Перенесённый приёмник обязан считаться заведённым
	ASSERT_TRUE(moved.exists());
	// Занятое обязано пережить перенос
	EXPECT_EQ(moved.size(), static_cast <size_t> (2));
	// Признак совпадения содержимого
	bool matched = false;
	// Обращаемся к содержимому перенесённого приёмника
	ASSERT_TRUE(moved.apply([&](const uint8_t * data, const size_t size) noexcept {
		// Запоминаем признак совпадения содержимого
		matched = ((size == 2) && (data[0] == 0xC0) && (data[1] == 0xC1));
	}));
	// Содержимое обязано пережить перенос
	EXPECT_TRUE(matched);
}
/**
 * @brief Проверка честности отчёта о состоявшейся защите
 *
 * @note Отчёт обязан говорить о том, что состоялось НА ДЕЛЕ. Поверяется он
 *       согласованностью с поведением строгого приёмника: там, где запрет подкачки не
 *       состоялся, строгий приёмник обязан отвечать отказом, и наоборот
 *
 */
TEST(AllocVesselTest, ShelterReportAgreesWithStrictVessel){
	// Заводим послабленный приёмник
	awh::alloc::vessel_t relaxed = ::relaxed();
	// Заводим послабленный приёмник под приём содержимого
	ASSERT_TRUE(relaxed.reserve(16));
	// Запоминаем признак состоявшегося запрета подкачки
	const bool wired = relaxed.shelter().wired;
	// Заводим строгий приёмник
	awh::alloc::vessel_t strict(awh::alloc::secrecy_t::STRICT);
	// Заводим строгий приёмник под приём содержимого
	const bool reserved = strict.reserve(16);
	/**
	 * Строгий приёмник заводится ровно тогда, когда запрет подкачки состоялся
	 *
	 * Расхождение здесь значило бы, что отчёт о защите и поведение строгого приёмника
	 * берут сведения из разных мест
	 */
	EXPECT_EQ(reserved, wired);
	// Если строгий приёмник заведён
	if(reserved)
		// Запрет подкачки обязан быть отмечен и у него
		EXPECT_TRUE(strict.shelter().wired);
}
/**
 * @brief Проверка снятия последнего принятого байта
 *
 * @note Метод заведён ради правки опечатки при вводе пароля: без него человеку остаётся
 *       начинать набор заново
 *
 */
TEST(AllocVesselTest, DroppedByteLeavesNoTrace){
	// Заводим приёмник
	awh::alloc::vessel_t vessel = ::relaxed();
	// Заводим приёмник под приём содержимого
	ASSERT_TRUE(vessel.reserve(4));
	// Принимаем содержимое в приёмник
	ASSERT_TRUE(vessel.pour(0x41));
	// Принимаем содержимое в приёмник
	ASSERT_TRUE(vessel.pour(0x42));
	// Снимаем последний принятый байт
	ASSERT_TRUE(vessel.drop());
	// Занятое обязано уменьшиться
	EXPECT_EQ(vessel.size(), static_cast <size_t> (1));
	// Признак совпадения содержимого
	bool matched = false;
	// Обращаемся к содержимому приёмника
	ASSERT_TRUE(vessel.apply([&](const uint8_t * data, const size_t size) noexcept {
		// Запоминаем признак совпадения содержимого
		matched = ((size == 1) && (data[0] == 0x41));
	}));
	// Снятый байт в содержимое идти не должен
	EXPECT_TRUE(matched);
	// Приём после снятия обязан идти по-прежнему
	ASSERT_TRUE(vessel.pour(0x43));
	// Признак совпадения содержимого после повторного приёма
	bool refilled = false;
	// Обращаемся к содержимому приёмника
	ASSERT_TRUE(vessel.apply([&](const uint8_t * data, const size_t size) noexcept {
		// Запоминаем признак совпадения содержимого
		refilled = ((size == 2) && (data[0] == 0x41) && (data[1] == 0x43));
	}));
	// Принятое поверх снятого обязано лечь на его место
	EXPECT_TRUE(refilled);
	// Снимаем всё принятое
	ASSERT_TRUE(vessel.drop());
	// Снимаем всё принятое
	ASSERT_TRUE(vessel.drop());
	// Снятие из пустого приёмника обязано отвечать отказом
	EXPECT_FALSE(vessel.drop());
	// Занятого в приёмнике остаться не должно
	EXPECT_EQ(vessel.size(), static_cast <size_t> (0));
}
/**
 * @brief Проверка приёма пароля без терминала
 *
 * @note Приём с ТЕРМИНАЛА набором не проверяется: терминала у прогона нет, а подделать
 *       его нечем - проверка потребовала бы своего псевдотерминала и человека за ним.
 *       Проверяется здесь ровно то, что проверяемо: отказ там, где терминала нет, и
 *       согласованность его с ответом `available()`
 *
 * @note Отказ этот НАМЕРЕННЫЙ и есть решение устройства: читать поток ввода вместо
 *       терминала нельзя - его вправе подменить конвейер либо чужой процесс, и пароль
 *       пришёл бы не от человека, да ещё с включённым эхом
 *
 */
TEST(AllocVesselTest, PromptRefusesWithoutTerminal){
	// Заводим приёмник
	awh::alloc::vessel_t vessel = ::relaxed();
	// Заводим приёмник под приём пароля
	ASSERT_TRUE(vessel.reserve(128));
	// Спрашиваем наличие терминала
	const bool terminal = awh::alloc::prompt_t::available();
	// Если терминала у прогона нет
	if(!terminal){
		// Приём пароля обязан отвечать отсутствием терминала
		EXPECT_EQ(awh::alloc::prompt_t::read(vessel, nullptr),
		 awh::alloc::prompt_t::result_t::NOTERM);
		// Приёмник при отказе остаться заведённым обязан
		EXPECT_TRUE(vessel.exists());
		// Содержимого в приёмнике появиться не должно
		EXPECT_EQ(vessel.size(), static_cast <size_t> (0));
	/**
	 * Если терминал у прогона всё же есть
	 *
	 * Читать из него нельзя: прогон встал бы, ожидая набора, и набор ждал бы вечно.
	 * Утверждаем лишь согласованность ответов
	 */
	} else GTEST_SKIP() << "У прогона есть терминал: приём пароля ждал бы набора вечно";
}
/**
 * @brief Проверка отказа приёма в незаведённый приёмник
 *
 */
TEST(AllocVesselTest, PromptRefusesUnpreparedVessel){
	// Заводим приёмник, не заводя области
	awh::alloc::vessel_t vessel = ::relaxed();
	/**
	 * Приём в незаведённый приёмник обязан отвечать отказом окружения
	 *
	 * Ёмкость приёмника и есть верхняя граница набора, и объявляет её зовущий: приём,
	 * заводящий область сам, растил бы её по ходу набора - то есть перевыделял
	 */
	EXPECT_EQ(awh::alloc::prompt_t::read(vessel, nullptr),
	 awh::alloc::prompt_t::result_t::FAILED);
}
/**
 * @brief Проверка счёта раскрытий из двух потоков разом
 *
 * @note Пуск потоков ОДНОВРЕМЕННЫЙ, и это не украшение: заведение потока стоит дороже
 *       сотни обращений, и пущенные по мере заведения они почти не пересекаются - тогда
 *       проверка стережёт согласованность там, где её никто и не нарушает. Числом
 *       повторов беда не лечится, лечится барьером. Правило принесено Григорием
 *       15.09.2026: он снял замок у склада и получил падение лишь в четырёх прогонах из
 *       пяти, покуда не поставил одновременный пуск
 *
 * @note Стережёт проверка ровно счёт держателей: закрой область поток, закончивший
 *       первым, - второй читал бы недоступную память и валил бы программу отказом
 *       доступа
 *
 * @note Проверка эта и `NestedAccessKeepsTheVesselOpen` ДУБЛИРУЮЩИМИ НЕ ЯВЛЯЮТСЯ, хотя
 *       и выглядят так: снятие счёта держателей валит обе, но по разным причинам.
 *       Вложенная показывает беду УСТРОЙСТВА - её ловит и один поток, оттого под
 *       мутацией она падает первой; эта показывает беду ГОНКИ, какой у одного потока
 *       нет вовсе. Сняв одну как лишнюю, теряют половину охвата, и потеря эта молчит
 *
 */
TEST(AllocVesselTest, RevealCountSurvivesTwoThreadsAtOnce){
	// Сведения о состоявшейся защите
	awh::alloc::shelter_t shelter;
	// Выдаём защищённый блок
	void * block = awh::alloc::Allocator::secure(4096, &shelter);
	// Блок обязан быть выдан
	ASSERT_NE(block, nullptr);
	// Закрываем защищённый блок
	if(!awh::alloc::Allocator::shield(block, false)){
		// Освобождаем защищённый блок
		awh::alloc::Allocator::release(block);
		// Смены доступности источник не умеет, стеречь нечего
		GTEST_SKIP() << "Источник страниц смены доступности не умеет";
	}
	// Число изготовившихся потоков
	std::atomic <int32_t> ready(0);
	// Признак общего слова к началу работы
	std::atomic <bool> started(false);
	// Число обращений, прошедших по доступной памяти
	std::atomic <int32_t> served(0);
	/**
	 * @brief Работа одного потока
	 *
	 */
	auto worker = [&]() noexcept {
		// Отмечаемся изготовившимися
		ready.fetch_add(1, std::memory_order_release);
		// Ждём общего слова к началу работы
		while(!started.load(std::memory_order_acquire))
			// Уступаем время прочим потокам
			std::this_thread::yield();
		/**
		 * Выполняем перебор обращений к блоку
		 */
		for(int32_t i = 0; i < 2048; i++){
			// Открываем защищённый блок
			if(!awh::alloc::Allocator::shield(block, true))
				// Переходим к следующему обращению
				continue;
			/**
			 * Пишем в блок, покуда держим его открытым
			 *
			 * Запись эта и есть утверждение: закрой область сосед, закончивший раньше,
			 * - она отвечала бы отказом доступа, а не ложилась бы на место
			 */
			reinterpret_cast <volatile uint8_t *> (block)[0] = static_cast <uint8_t> (i);
			// Считаем обращение, прошедшее по доступной памяти
			served.fetch_add(1, std::memory_order_relaxed);
			// Закрываем защищённый блок
			static_cast <void> (awh::alloc::Allocator::shield(block, false));
		}
	};
	// Заводим первый поток
	std::thread first(worker);
	// Заводим второй поток
	std::thread second(worker);
	/**
	 * Ждём, покуда оба потока изготовятся
	 */
	while(ready.load(std::memory_order_acquire) < 2)
		// Уступаем время прочим потокам
		std::this_thread::yield();
	// Даём общее слово к началу работы
	started.store(true, std::memory_order_release);
	// Дожидаемся первого потока
	first.join();
	// Дожидаемся второго потока
	second.join();
	// Все обращения обоих потоков обязаны были пройти
	EXPECT_EQ(served.load(std::memory_order_relaxed), 4096);
	// Открываем блок перед освобождением
	static_cast <void> (awh::alloc::Allocator::shield(block, true));
	// Освобождаем защищённый блок
	awh::alloc::Allocator::release(block);
}
/**
 * @brief Проверка смены доступности защищённого блока
 *
 */
TEST(AllocVesselTest, ProtectedBlockChangesItsAccessibility){
	// Сведения о состоявшейся защите
	awh::alloc::shelter_t shelter;
	// Выдаём защищённый блок
	void * block = awh::alloc::Allocator::secure(4096, &shelter);
	// Блок обязан быть выдан
	ASSERT_NE(block, nullptr);
	/**
	 * Смена доступности - умение НЕОБЯЗАТЕЛЬНОЕ
	 *
	 * Источник, страницами системы не владеющий, отвечает отказом, и отказ этот
	 * означает «не умею», а не «не вышло». Проверка судит лишь согласованность:
	 * умеющий обязан уметь и открыть, и закрыть
	 */
	// Закрываем защищённый блок
	const bool closed = awh::alloc::Allocator::shield(block, false);
	// Если блок закрыт
	if(closed){
		// Повторное закрытие закрытого блока обязано проходить без беды
		EXPECT_TRUE(awh::alloc::Allocator::shield(block, false));
		// Открываем защищённый блок обратно
		EXPECT_TRUE(awh::alloc::Allocator::shield(block, true));
		/**
		 * Содержимое открытого блока обязано быть доступно
		 *
		 * Запись эта и есть проверка: останься блок закрытым, она отвечала бы отказом
		 * доступа, а не ложилась бы на место
		 */
		reinterpret_cast <uint8_t *> (block)[0] = 0x7E;
		// Записанное обязано читаться обратно
		EXPECT_EQ(reinterpret_cast <uint8_t *> (block)[0], static_cast <uint8_t> (0x7E));
	}
	// Смена доступности чужого адреса обязана отвечать отказом
	EXPECT_FALSE(awh::alloc::Allocator::shield(nullptr, true));
	// Открываем блок перед освобождением
	static_cast <void> (awh::alloc::Allocator::shield(block, true));
	// Освобождаем защищённый блок
	awh::alloc::Allocator::release(block);
}
