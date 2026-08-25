/**
 * @file zone.cpp
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
 * @brief Набор проверок зоны выдачи памяти macOS
 *
 * @details Захват у macOS идёт зоной: наша зона встаёт основной, и система обращается к
 *          ней своими откликами - измерением, выдачей, освобождением, перечислением
 *          блоков, сбором сведений. Отклики эти зовёт СИСТЕМА, а не прикладной код:
 *          вызов `::malloc` из нашей же программы идёт прямо к нашему имени и зоны не
 *          касается вовсе. Оттого покрытие слоя составляло 42 % - и ровно в нём жил
 *          дефект возвратности `size`, валивший потомка после ветвления
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
 * Зона выдачи памяти есть лишь у macOS
 */
#if (defined(__APPLE__) || defined(__MACH__)) && !AWH_ALLOC_SANITIZED

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>
#include <dlfcn.h>
#include <mach/mach.h>
#include <malloc/malloc.h>

/**
 * Подключаем заголовочный файл захвата зоной
 *
 * Имя зоны берём ОТТУДА, а не вписываем строкой: вписанное разошлось бы с кодом при
 * первой же правке, и проверка молча перестала бы находить нашу зону
 */
#include <alloc/zone.hpp>

/**
 * @brief Пространство имён вспомогательных средств
 *
 */
namespace {
	/**
	 * @brief Метод получения нашей зоны
	 *
	 * @note Ищем зону ПО ИМЕНИ, а не берём основную: основной вправе оказаться и чужая
	 *       зона, заведённая кем-то после нас, и проверка мерила бы тогда чужой слой
	 *
	 * @return наша зона, либо nullptr если её нет
	 *
	 */
	static ::malloc_zone_t * owned() noexcept {
		// Число зон процесса
		unsigned int count = 0;
		// Перечень зон процесса
		::vm_address_t * zones = nullptr;
		// Получаем перечень зон процесса
		if(::malloc_get_all_zones(mach_task_self(), nullptr, &zones, &count) != KERN_SUCCESS)
			// Перечня зон нет
			return nullptr;
		/**
		 * Перебираем зоны процесса
		 */
		for(unsigned int i = 0; i < count; i++){
			// Получаем очередную зону
			::malloc_zone_t * zone = reinterpret_cast <::malloc_zone_t *> (zones[i]);
			// Если зона безымянна
			if((zone == nullptr) || (zone->zone_name == nullptr))
				// Пропускаем безымянную зону
				continue;
			// Если зона наша
			if(::strcmp(zone->zone_name, awh::alloc::ZoneCapture::TITLE) == 0)
				// Выводим нашу зону
				return zone;
		}
		// Нашей зоны в процессе нет
		return nullptr;
	}
};

/**
 * @brief Тест наличия нашей зоны в процессе
 *
 * @note Захват, состоявшийся лишь на словах, ничего не значит: зона обязана лежать в
 *       перечне зон процесса и носить наше имя - по имени её и находят средства
 *       разбора памяти вроде `leaks` и `heap`
 *
 */
TEST(AllocZoneTest, ZoneIsRegistered){
	// Получаем нашу зону
	::malloc_zone_t * zone = ::owned();
	// Наша зона обязана лежать в перечне зон процесса
	ASSERT_NE(zone, nullptr);
	// Зона обязана нести отклики выдачи и освобождения
	EXPECT_NE(zone->malloc, nullptr);
	EXPECT_NE(zone->free, nullptr);
	EXPECT_NE(zone->size, nullptr);
	/**
	 * Выравнивающая выдача обязана быть заполнена
	 *
	 * Библиотека времени исполнения зовёт её БЕЗ проверки на пустоту, и незаполненное
	 * поле валит процесс при первом же `posix_memalign`
	 */
	EXPECT_NE(zone->memalign, nullptr);
	// Зона обязана нести средства разбора
	ASSERT_NE(zone->introspect, nullptr);
}
/**
 * @brief Тест опознания выданного блока обходом зон
 *
 * @note Принадлежность эта - то, чем система решает, кому отдать освобождение: она
 *       обходит зоны и спрашивает каждую
 *
 * @warning Имя зоны здесь НЕ сличается, и это не послабление: `malloc_zone_from_ptr`
 *          отвечает не зоной перечня, а неизменной обёрткой из общего образа системы -
 *          той самой, о какой сказано у `defaultZone`. Обёртка эта пересылает вопрос
 *          дальше, оттого судить надо по ОТВЕТУ, а не по имени отвечавшего: проверено
 *          щупом, где обёртка лежала по адресу вне перечня зон вовсе
 *
 */
TEST(AllocZoneTest, BlockIsClaimedWithRightSize){
	// Выдаём память
	void * block = ::malloc(1024);
	// Выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Получаем зону, какой принадлежит блок
	::malloc_zone_t * zone = ::malloc_zone_from_ptr(block);
	// Блок обязан быть кем-то опознан
	ASSERT_NE(zone, nullptr);
	// Опознавшая зона обязана нести отклик размера
	ASSERT_NE(zone->size, nullptr);
	// Размер, доложенный опознавшей зоной, обязан покрывать запрошенное
	EXPECT_GE((* zone->size)(zone, block), static_cast <size_t> (1024));
	// Тот же размер обязан доложить и обход зон
	EXPECT_EQ(::malloc_size(block), (* zone->size)(zone, block));
	// Блок обязан оставаться нашим по разбору адреса
	EXPECT_GE(awh::alloc::Allocator::resolve(block).size, static_cast <size_t> (1024));
	// Освобождаем выданное
	::free(block);
}
/**
 * @brief Тест выдачи и освобождения через саму зону
 *
 * @note Путь этот - не то же, что `::malloc`: прикладной код идёт прямо к нашему имени,
 *       а система зовёт зону откликом. Ходят они в один слой, но разными дверями, и
 *       проверять надо обе
 *
 */
TEST(AllocZoneTest, ZoneServesAllocation){
	// Получаем нашу зону
	::malloc_zone_t * zone = ::owned();
	// Наша зона обязана быть в процессе
	ASSERT_NE(zone, nullptr);
	// Выдаём память самой зоной
	void * block = ::malloc_zone_malloc(zone, 300);
	// Выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Выданное обязано принадлежать нашему распределителю
	EXPECT_GE(awh::alloc::Allocator::resolve(block).size, static_cast <size_t> (300));
	// Заполняем выданное содержимым
	::memset(block, 0x7E, 300);
	// Перевыдаём блок самой зоной
	block = ::malloc_zone_realloc(zone, block, 900);
	// Перевыдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Содержимое обязано уцелеть
	EXPECT_EQ(reinterpret_cast <unsigned char *> (block)[299], 0x7Eu);
	// Освобождаем блок самой зоной
	::malloc_zone_free(zone, block);
	// Выдаём обнулённую память самой зоной
	unsigned char * zeroed = reinterpret_cast <unsigned char *> (::malloc_zone_calloc(zone, 64, 8));
	// Выдача обязана состояться
	ASSERT_NE(zeroed, nullptr);
	// Число ненулевых байтов
	size_t dirty = 0;
	/**
	 * Перебираем содержимое выданного
	 */
	for(size_t i = 0; i < (64u * 8u); i++){
		// Если байт оказался ненулевым
		if(zeroed[i] != 0)
			// Считаем ненулевой байт
			dirty++;
	}
	// Ненулевых байтов быть не должно
	EXPECT_EQ(dirty, static_cast <size_t> (0));
	// Освобождаем блок самой зоной
	::malloc_zone_free(zone, zeroed);
}
/**
 * @brief Тест страничной выдачи зоной
 *
 * @note Отклик `valloc` обязан отдавать память с началом на границе страницы: система
 *       зовёт его там, где выравнивание требуется самим устройством обращения
 *
 */
TEST(AllocZoneTest, ZoneServesPageAlignedAllocation){
	// Получаем нашу зону
	::malloc_zone_t * zone = ::owned();
	// Наша зона обязана быть в процессе
	ASSERT_NE(zone, nullptr);
	// Выдаём страничную память самой зоной
	void * block = ::malloc_zone_valloc(zone, 8192);
	// Выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Начало блока обязано лежать на границе страницы
	EXPECT_EQ((reinterpret_cast <uintptr_t> (block) % 4096u), static_cast <uintptr_t> (0));
	// Освобождаем блок самой зоной
	::malloc_zone_free(zone, block);
}
/**
 * @brief Тест хорошего размера блока
 *
 * @note Спрашивают его затем, чтобы просить сразу столько, сколько всё равно отведётся:
 *       ответ обязан покрывать запрошенное, иначе спросивший недосчитается места
 *
 */
TEST(AllocZoneTest, ZoneReportsGoodSize){
	// Хороший размер обязан покрывать запрошенное
	EXPECT_GE(::malloc_good_size(100), static_cast <size_t> (100));
	EXPECT_GE(::malloc_good_size(1), static_cast <size_t> (1));
	EXPECT_GE(::malloc_good_size(65536), static_cast <size_t> (65536));
}
/**
 * @brief Тест сбора сведений о зоне
 *
 * @note Сведения эти читают средства разбора памяти системы. Отвечающая нулями зона
 *       выглядит для них пустой, и утечка в ней остаётся невидимой
 *
 */
TEST(AllocZoneTest, ZoneReportsStatistics){
	// Получаем нашу зону
	::malloc_zone_t * zone = ::owned();
	// Наша зона обязана быть в процессе
	ASSERT_NE(zone, nullptr);
	// Держим блок живым, чтобы зоне было о чём отчитаться
	void * block = ::malloc(4096);
	// Выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Сведения о зоне
	::malloc_statistics_t stats;
	// Обнуляем сведения о зоне
	::memset(&stats, 0, sizeof(stats));
	// Собираем сведения о зоне
	::malloc_zone_statistics(zone, &stats);
	// Занятое зоной обязано быть ненулевым
	EXPECT_GT(stats.size_in_use, static_cast <size_t> (0));
	// Взятое зоной у системы обязано покрывать занятое
	EXPECT_GE(stats.size_allocated, stats.size_in_use);
	// Освобождаем выданное
	::free(block);
}
/**
 * @brief Тест проверки целостности зоны
 *
 * @note Отклик этот зовут средства разбора памяти. Отвечать им отказом на здоровой куче
 *       значило бы поднимать ложную тревогу
 *
 */
TEST(AllocZoneTest, ZonePassesCheck){
	// Получаем нашу зону
	::malloc_zone_t * zone = ::owned();
	// Наша зона обязана быть в процессе
	ASSERT_NE(zone, nullptr);
	// Целостность зоны обязана подтвердиться
	EXPECT_TRUE(::malloc_zone_check(zone));
}
/**
 * @brief Тест выдачи и освобождения пачкой
 *
 * @note Пачкой система выдаёт мелочь россыпью. Отклик вправе выдать МЕНЬШЕ
 *       запрошенного - но всё выданное обязано быть настоящим, и всё обязано
 *       освободиться
 *
 */
TEST(AllocZoneTest, ZoneServesBatch){
	// Получаем нашу зону
	::malloc_zone_t * zone = ::owned();
	// Наша зона обязана быть в процессе
	ASSERT_NE(zone, nullptr);
	// Перечень выданных блоков
	void * blocks[16];
	// Обнуляем перечень выданных блоков
	::memset(blocks, 0, sizeof(blocks));
	// Выдаём память пачкой
	const unsigned int served = ::malloc_zone_batch_malloc(zone, 128, blocks, 16);
	// Выдача обязана состояться хотя бы отчасти
	ASSERT_GT(served, static_cast <unsigned int> (0));
	/**
	 * Перебираем выданные блоки
	 */
	for(unsigned int i = 0; i < served; i++){
		// Блок обязан быть выдан
		ASSERT_NE(blocks[i], nullptr);
		// Блок обязан принадлежать нашему распределителю
		EXPECT_GE(awh::alloc::Allocator::resolve(blocks[i]).size, static_cast <size_t> (128));
	}
	// Освобождаем память пачкой
	::malloc_zone_batch_free(zone, blocks, served);
}
/**
 * @brief Тест отдачи памяти по требованию системы
 *
 * @note Требование это приходит, когда системе не хватает памяти. Отклик обязан
 *       отработать без падения: отказавший здесь распределитель валит процесс в самую
 *       тяжёлую для системы пору
 *
 */
TEST(AllocZoneTest, ZoneRelievesPressure){
	// Получаем нашу зону
	::malloc_zone_t * zone = ::owned();
	// Наша зона обязана быть в процессе
	ASSERT_NE(zone, nullptr);
	// Перечень выданных блоков
	void * blocks[64];
	/**
	 * Занимаем и освобождаем память, чтобы зоне было что отдавать
	 */
	for(size_t i = 0; i < 64; i++)
		// Выдаём очередной блок
		blocks[i] = ::malloc(8192);
	/**
	 * Перебираем выданные блоки
	 */
	for(size_t i = 0; i < 64; i++){
		// Если блок выдан
		if(blocks[i] != nullptr)
			// Освобождаем блок
			::free(blocks[i]);
	}
	// Требуем отдать память системе
	const size_t given = ::malloc_zone_pressure_relief(zone, 0);
	// Отдача обязана отработать: сколько именно отдано - дело настроек
	EXPECT_GE(given, static_cast <size_t> (0));
	// Куча обязана уцелеть после отдачи
	void * block = ::malloc(1024);
	// Выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Освобождаем выданное
	::free(block);
}
/**
 * @brief Тест опознания принадлежности адреса
 *
 * @note Отклик этот отличается от `size` тем, что обязан отвечать БЕЗ обращения к
 *       содержимому блока: система зовёт его при разборе упавшего процесса, где
 *       содержимое читать опасно
 *
 */
TEST(AllocZoneTest, ZoneClaimsOwnAddress){
	// Получаем нашу зону
	::malloc_zone_t * zone = ::owned();
	// Наша зона обязана быть в процессе
	ASSERT_NE(zone, nullptr);
	// Если отклик опознания зоной не заполнен
	if(zone->claimed_address == nullptr)
		// Проверять нечего
		GTEST_SKIP() << "зона не несёт отклика опознания адреса";
	// Выдаём память
	void * block = ::malloc(512);
	// Выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Наш блок обязан быть опознан своим
	EXPECT_TRUE((* zone->claimed_address)(zone, block));
	// Адрес на стеке заведомо чужой
	volatile int local = 0;
	// Чужой адрес обязан быть опознан чужим
	EXPECT_FALSE((* zone->claimed_address)(zone, const_cast <int *> (&local)));
	// Освобождаем выданное
	::free(block);
}

/**
 * @brief Тест подмены имён - второй половины захвата у macOS
 *
 * @details Захват у macOS двойной: зона ловит ВСЯКИЙ указатель процесса, а подмена имён
 *          уводит к нам обращения своего образа коротким путём, минуя раздачу libsystem
 *          по зонам. Раздача та замерена в 2,53 нс на действие - треть всей цены модуля
 *          на macOS, - и снимает её именно подмена
 *
 * @note Проверяется не быстродействие, а само устройство: имя `malloc`, каким его видит
 *       наш образ, обязано указывать на код НАШЕЙ библиотеки, а не на libsystem. Проверка
 *       по времени была бы негодной - разница в наносекунды тонет в шуме машины
 *
 * @warning Обе половины обязаны стоять РАЗОМ. Останься одна зона - вернётся дань раздаче;
 *          останься одна подмена - мимо нас уйдёт всё, что libsystem выдаёт себе самой, и
 *          распределитель разделится надвое: выдача одним, освобождение другим
 *
 */
TEST(AllocZoneTest, NamesAreSubstitutedBesideZone){
	// Сведения об образе, которому принадлежит имя
	::Dl_info about;
	// Обнуляем сведения об образе
	::memset(&about, 0, sizeof(about));
	/**
	 * Спрашиваем имя ТАК ЖЕ, как его спрашивает наш образ
	 *
	 * Берём адрес самой функции, а не ищем имя перебором: адрес этот связан теми же
	 * правилами, по каким пойдёт всякое обращение к `malloc` из нашего кода
	 */
	void * (* entry)(size_t) = &::malloc;
	// Разбираем, какому образу принадлежит вход выдачи
	ASSERT_NE(::dladdr(reinterpret_cast <void *> (entry), &about), 0);
	// Имя образа обязано быть известно
	ASSERT_NE(about.dli_fname, nullptr);
	/**
	 * Вход выдачи обязан лежать ВНЕ libsystem
	 *
	 * Сличаем по отсутствию имени библиотеки времени исполнения, а не по совпадению с
	 * нашим: имя нашего образа зависит от способа сборки - статическая библиотека входит
	 * в саму программу, а разделяемая остаётся отдельным образом
	 */
	EXPECT_EQ(::strstr(about.dli_fname, "libsystem_malloc"), nullptr) << "выдача уходит в libsystem: подмена имён не состоялась, образ - " << about.dli_fname;
	/**
	 * Память, выданную ВНУТРИ libsystem, обязана держать наша зона
	 *
	 * Обращение к выдаче изнутри библиотеки времени исполнения помечено её же именем и
	 * нашим определением не заслоняется - ловит его только зона. Проверка эта и стережёт
	 * полноту захвата: пропади зона, блок ниже окажется чужим
	 */
	char * copy = ::strdup("проверка полноты захвата");
	// Удвоение строки обязано состояться
	ASSERT_NE(copy, nullptr);
	// Получаем нашу зону
	::malloc_zone_t * zone = ::owned();
	// Наша зона обязана быть в процессе
	ASSERT_NE(zone, nullptr);
	/**
	 * Спрашиваем принадлежность У САМОЙ ЗОНЫ, а не через `malloc_zone_from_ptr`
	 *
	 * Разбор тот на macOS 26 отвечает не зоной-владельцем, а заглушкой из общего кэша
	 * образов: проверено щупом - НАШ блок он приписал `DefaultMallocZone` по адресу
	 * 0x1f193c000, какого нет и в перечне зон процесса. Прямой же вопрос зоне отвечает
	 * верно, и он же и есть договор: зона знает свои блоки и отвечает нулём о чужих
	 */
	// Размер блока, выданного внутри libsystem, по мнению нашей зоны
	const size_t owned = (* zone->size)(zone, copy);
	// Блок, выданный внутри libsystem, обязан принадлежать нашей зоне
	EXPECT_GT(owned, 0u) << "выдача изнутри libsystem прошла мимо нашей зоны";
	// Освобождаем удвоенную строку
	::free(copy);
}

#endif // (__APPLE__ || __MACH__) && !AWH_ALLOC_SANITIZED
