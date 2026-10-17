/**
 * @file source-limit.cpp
 * @date 2026-10-17
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
 * @brief Реализация регрессионных проверок размеров и бюджета системного источника
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем системные заголовочные файлы до подмены вызовов
 */
#include <sys/mman.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <gtest/gtest.h>

/**
 * Подключаем слои с независимыми замками
 */
#include <alloc/huge.hpp>
#include <alloc/guard.hpp>

/**
 * @brief Инкапсулируем перехват системных вызовов
 *
 */
namespace {
	// Подмена первого отображения безопасным условным адресом
	std::atomic <bool> __awh_fake__(false);
	// Отказ следующего отображения
	std::atomic <bool> __awh_failMap__(false);
	// Отказ следующего освобождения
	std::atomic <bool> __awh_failUnmap__(false);
	// Остановка следующего отображения
	std::atomic <bool> __awh_pause__(false);
	// Признак входа в остановленное отображение
	std::atomic <bool> __awh_entered__(false);
	// Разрешение продолжить остановленное отображение
	std::atomic <bool> __awh_resume__(false);
	// Число обращений к отображению
	std::atomic <size_t> __awh_calls__(0);
	/**
	 * @brief Функция контролируемого отображения страниц
	 *
	 * @param addr       адрес отображения
	 * @param size       длина отображения
	 * @param protection права доступа
	 * @param flags      флаги отображения
	 * @param fd         дескриптор файла
	 * @param offset     смещение файла
	 * @return           адрес отображения либо признак отказа
	 *
	 */
	static void * auditMap(void * addr, const size_t size, const int protection, const int flags, const int fd, const off_t offset) noexcept {
		// Считаем обращения к отображению
		::__awh_calls__.fetch_add(1);
		// Подменяем опасный запрос без обращения к ядру
		if(::__awh_fake__.exchange(false))
			// Возвращаем условный адрес для воспроизведения старого дефекта
			return reinterpret_cast <void *> (0x100000);
		// Воспроизводим отказ ядра
		if(::__awh_failMap__.exchange(false))
			// Возвращаем признак отказа отображения
			return MAP_FAILED;
		// Останавливаем только первый запрос
		if(::__awh_pause__.exchange(false)){
			// Сообщаем о прохождении проверки бюджета
			::__awh_entered__.store(true);
			/**
			 * Дожидаемся проверки независимого слоя
			 */
			while(!::__awh_resume__.load())
				// Уступаем процессор проверяющему потоку
				std::this_thread::yield();
		}
		// Выполняем настоящее отображение
		return ::mmap(addr, size, protection, flags, fd, offset);
	}
	/**
	 * @brief Функция контролируемого освобождения страниц
	 *
	 * @param addr адрес освобождаемой области
	 * @param size длина освобождаемой области
	 * @return     результат операции
	 *
	 */
	static int auditUnmap(void * addr, const size_t size) noexcept {
		// Не передаём ядру опасные диапазоны старой версии
		if(size > (static_cast <size_t> (-1) / 4))
			// Имитируем завершение освобождения
			return 0;
		// Воспроизводим отказ освобождения
		if(::__awh_failUnmap__.exchange(false))
			// Оставляем область отображённой
			return -1;
		// Выполняем настоящее освобождение
		return ::munmap(addr, size);
	}
};

/**
 * Подменяем отображение только внутри проверяемой реализации
 */
#define mmap auditMap
/**
 * Подменяем освобождение только внутри проверяемой реализации
 */
#define munmap auditUnmap
/**
 * Подключаем текущую реализацию без перехвата malloc процесса
 */
#include "../../src/alloc/source.cpp"
/**
 * Завершаем подмену отображения
 */
#undef mmap
/**
 * Завершаем подмену освобождения
 */
#undef munmap

/**
 * @brief Тест отказа переполненного запроса через слой крупных выдач
 *
 */
TEST(SourceLimit, HugeAlignmentOverflow){
	// Создаём источник страниц
	awh::alloc::SystemSource source;
	// Создаём слой крупных выдач
	awh::alloc::Huge huge;
	// Заводим слой на проверяемом источнике
	ASSERT_TRUE(huge.init(&source));
	// Наибольшее выравнивание, представимое степенью двойки
	const size_t alignment = (static_cast <size_t> (1) << ((sizeof(size_t) * 8) - 1));
	// Начальное число системных вызовов
	const size_t calls = ::__awh_calls__.load();
	// Защищаем процесс от опасных запросов неисправной версии
	::__awh_fake__.store(true);
	// Передаём переполненную сумму через верхний слой
	void * block = huge.alloc((alignment + source.granularity()), alignment);
	// Выдача обязана завершиться отказом
	EXPECT_EQ(block, nullptr);
	// Отказ обязан предшествовать обращению к системе
	EXPECT_EQ(::__awh_calls__.load(), calls);
	// Снимаем подмену для последующих проверок
	::__awh_fake__.store(false);
	// Снимаем учётные записи, если неисправная версия их завела
	huge.reset();
	// Учёт обязан остаться пустым
	EXPECT_EQ(source.taken(), 0u);
}
/**
 * @brief Тест общего потолка при одновременной работе двух слоёв
 *
 */
TEST(SourceLimit, IndependentLayersShareBudget){
	// Создаём общий источник
	awh::alloc::SystemSource source;
	// Создаём слой крупных выдач
	awh::alloc::Huge huge;
	// Создаём слой заслонов
	awh::alloc::Guard guard;
	// Заводим первый слой
	ASSERT_TRUE(huge.init(&source));
	// Заводим второй слой
	ASSERT_TRUE(guard.init(&source));
	// Получаем зернистость источника
	const size_t grain = source.granularity();
	// Прогреваем учётные записи крупных выдач
	void * warm = huge.alloc((grain * 8), 0);
	// Проверяем контрольную выдачу
	ASSERT_NE(warm, nullptr);
	// Возвращаем контрольную область
	huge.free(warm);
	// Прогреваем учётные записи заслонов
	warm = guard.alloc(1);
	// Проверяем контрольный заслон
	ASSERT_NE(warm, nullptr);
	// Признак принадлежности заслона
	bool mine = false;
	// Возвращаем контрольный заслон
	guard.free(warm, &mine);
	// Фиксируем расход учётных записей
	const size_t base = source.taken();
	// Разрешаем ещё одну крупную выдачу
	source.ceiling(base + (grain * 8));
	// Обнуляем признак входа в системное отображение
	::__awh_entered__.store(false);
	// Закрываем продолжение первого отображения
	::__awh_resume__.store(false);
	// Останавливаем следующую выдачу после проверки бюджета
	::__awh_pause__.store(true);
	// Адрес первой выдачи
	void * first = nullptr;
	// Запускаем первый слой в отдельном потоке
	std::thread worker([&]() {
		// Занимаем весь оставшийся бюджет
		first = huge.alloc((grain * 8), 0);
	});
	// Ограничиваем ожидание входа в системный вызов
	const auto deadline = (std::chrono::steady_clock::now() + std::chrono::seconds(5));
	/**
	 * Дожидаемся остановки первого потока
	 */
	while(!::__awh_entered__.load() && (std::chrono::steady_clock::now() < deadline))
		// Уступаем процессор первому потоку
		std::this_thread::yield();
	// Запоминаем факт входа до разрешения продолжить
	const bool entered = ::__awh_entered__.load();
	// Второй слой пытается выделить память сверх зарезервированного
	void * second = (entered ? guard.alloc(1) : nullptr);
	// Разрешаем первому слою закончить отображение
	::__awh_resume__.store(true);
	// Дожидаемся завершения работника
	worker.join();
	// Проверяем достижение нужного чередования
	EXPECT_TRUE(entered);
	// Первый слой обязан получить разрешённую память
	EXPECT_NE(first, nullptr);
	// Второй слой обязан получить отказ
	EXPECT_EQ(second, nullptr);
	// Общий расход не должен превышать потолок
	EXPECT_LE(source.taken(), (base + (grain * 8)));
	// Возвращаем первую область
	huge.free(first);
	// Возвращаем вторую область, если дефект её пропустил
	guard.free(second, &mine);
	// Снимаем учёт крупных выдач
	huge.reset();
	// Снимаем учёт заслонов
	guard.reset();
	// Все области должны быть возвращены
	EXPECT_EQ(source.taken(), 0u);
}
/**
 * @brief Тест возврата резерва после отказа системного отображения
 *
 */
TEST(SourceLimit, MappingFailureRefundsBudget){
	// Создаём независимый источник
	awh::alloc::SystemSource source;
	// Получаем размер страницы
	const size_t grain = source.granularity();
	// Ограничиваем бюджет одной страницей
	source.ceiling(grain);
	// Действительный размер выдачи
	size_t actual = 99;
	// Задаём отказ ближайшего отображения
	::__awh_failMap__.store(true);
	// Проверяем отказ выдачи
	EXPECT_EQ(source.alloc(1, 0, actual), nullptr);
	// Неудачная выдача не должна сообщать объём
	EXPECT_EQ(actual, 0u);
	// Неудачная выдача не должна занимать бюджет
	EXPECT_EQ(source.taken(), 0u);
	// Повторяем выдачу после временного отказа
	void * block = source.alloc(1, 0, actual);
	// Повторная выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Возвращаем область
	EXPECT_TRUE(source.release(block, actual));
	// Бюджет обязан освободиться
	EXPECT_EQ(source.taken(), 0u);
}
/**
 * @brief Тест сохранения бюджета при отказе освобождения
 *
 */
TEST(SourceLimit, ReleaseFailureKeepsBudget){
	// Создаём источник
	awh::alloc::SystemSource source;
	// Получаем размер страницы
	const size_t grain = source.granularity();
	// Ограничиваем бюджет одной страницей
	source.ceiling(grain);
	// Размер выданной области
	size_t actual = 0;
	// Занимаем бюджет
	void * block = source.alloc(1, 0, actual);
	// Выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Задаём отказ освобождения
	::__awh_failUnmap__.store(true);
	// Проверяем сохранение отображения
	EXPECT_FALSE(source.release(block, actual));
	// Пока отображение живо, бюджет занят
	EXPECT_EQ(source.taken(), grain);
	// Повторяем освобождение
	EXPECT_TRUE(source.release(block, actual));
	// После успеха бюджет свободен
	EXPECT_EQ(source.taken(), 0u);
}
/**
 * @brief Тест округления, выравнивания и защищённой выдачи под потолком
 *
 */
TEST(SourceLimit, RoundedAlignedAndConcealed){
	// Создаём источник
	awh::alloc::SystemSource source;
	// Размер страницы
	const size_t grain = source.granularity();
	// Задаём бюджет в две страницы
	source.ceiling(grain * 2);
	// Размер первой области
	size_t actual = 0;
	// Просим байт сверх страницы с повышенным выравниванием
	void * block = source.alloc((grain + 1), (grain * 4), actual);
	// Выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Проверяем выравнивание
	EXPECT_EQ((reinterpret_cast <uintptr_t> (block) % (grain * 4)), 0u);
	// Проверяем округление учтённого объёма
	EXPECT_EQ(source.taken(), (grain * 2));
	// Признак состоявшегося укрытия
	bool hidden = false;
	// Размер второй области
	size_t concealed = 99;
	// Защищённая выдача не должна обходить потолок
	EXPECT_EQ(source.conceal(1, concealed, hidden), nullptr);
	// При отказе размер обнуляется
	EXPECT_EQ(concealed, 0u);
	// Возвращаем первую область
	EXPECT_TRUE(source.release(block, actual));
	// Повторяем защищённую выдачу с освободившимся бюджетом
	block = source.conceal(1, concealed, hidden);
	// Теперь защищённая выдача обязана состояться
	ASSERT_NE(block, nullptr);
	// Учитывается одна системная страница
	EXPECT_EQ(source.taken(), grain);
	// Возвращаем защищённую область
	EXPECT_TRUE(source.release(block, concealed));
	// Источник снова пуст
	EXPECT_EQ(source.taken(), 0u);
}
/**
 * @brief Тест отказа некорректных размеров до обращения к системе
 *
 */
TEST(SourceLimit, InvalidSizesNeverMap){
	// Создаём источник
	awh::alloc::SystemSource source;
	// Сохраняем число обращений к отображению
	const size_t calls = ::__awh_calls__.load();
	// Действительный размер выдачи
	size_t actual = 99;
	// Нулевой размер не создаёт отображения
	EXPECT_EQ(source.alloc(0, 0, actual), nullptr);
	// Размер при отказе обнуляется
	EXPECT_EQ(actual, 0u);
	// Некорректное выравнивание отвергается
	EXPECT_EQ(source.alloc(1, 3, actual), nullptr);
	// Переполнение округления отвергается
	EXPECT_EQ(source.alloc(static_cast <size_t> (-1), 0, actual), nullptr);
	// Ни один запрос не должен дойти до системы
	EXPECT_EQ(::__awh_calls__.load(), calls);
	// Ни один запрос не должен занять бюджет
	EXPECT_EQ(source.taken(), 0u);
}
/**
 * @brief Тест возврата бюджета после отказа выровненного отображения
 *
 */
TEST(SourceLimit, AlignedFailureRefundsBudget){
	// Создаём источник
	awh::alloc::SystemSource source;
	// Получаем размер страницы
	const size_t grain = source.granularity();
	// Ограничиваем бюджет одной страницей
	source.ceiling(grain);
	// Размер выдачи
	size_t actual = 0;
	// Задаём отказ отображения с запасом выравнивания
	::__awh_failMap__.store(true);
	// Выдача должна завершиться отказом
	EXPECT_EQ(source.alloc(1, (grain * 4), actual), nullptr);
	// Резерв не должен остаться занятым
	EXPECT_EQ(source.taken(), 0u);
	// Повторяем запрос после отказа
	void * block = source.alloc(1, (grain * 4), actual);
	// Выдача должна состояться
	ASSERT_NE(block, nullptr);
	// В бюджете остаётся окончательная область без запаса выравнивания
	EXPECT_EQ(source.taken(), grain);
	// Возвращаем область
	EXPECT_TRUE(source.release(block, actual));
	// Бюджет освобождён
	EXPECT_EQ(source.taken(), 0u);
}
