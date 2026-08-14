/**
 * @file smallvec.cpp
 * @date 2026-07-29
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
 * @brief Тесты вектора с малым инлайн-хранилищем (awh::quic::small_vector) —
 *        покрытие инлайн- и кучевого путей, перемещения, обмена и очистки с
 *        детектором утечек и двойных освобождений через счётчик живых экземпляров
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <utility>
#include <vector>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/proto/quic/connection.hpp"

/**
 * @brief Внутренние средства теста
 *
 */
namespace {
	/**
	 * @brief Перемещаемый элемент со счётчиком живых экземпляров
	 *
	 * @details Копирование запрещено (как у элемента учётной записи пакета), а
	 *          счётчик живых экземпляров ловит утечку (остался > 0) и двойное
	 *          освобождение (ушёл < 0) на любом пути small_vector
	 *
	 */
	struct Tracker {
		// Число живых экземпляров
		static int live;
		// Идентификатор экземпляра (-1 у перемещённого)
		int id;
		// Полезная нагрузка (зеркалит строковое поле chunk_t для реалистичности перемещения)
		std::string payload;
		/**
		 * @brief Конструктор по умолчанию
		 */
		Tracker() noexcept : id(0) { Tracker::live++; }
		/**
		 * @brief Конструктор со значением
		 * @param i идентификатор экземпляра
		 */
		explicit Tracker(const int i) noexcept : id(i), payload(std::to_string(i)) { Tracker::live++; }
		/**
		 * @brief Конструктор перемещения
		 * @param o источник
		 */
		Tracker(Tracker && o) noexcept : id(o.id), payload(std::move(o.payload)) { Tracker::live++; o.id = -1; }
		/**
		 * @brief Оператор перемещения
		 * @param o источник
		 * @return  ссылка на текущий объект
		 */
		Tracker & operator = (Tracker && o) noexcept { this->id = o.id; this->payload = std::move(o.payload); o.id = -1; return * this; }
		/**
		 * Копирование запрещено - small_vector не должен копировать элементы
		 */
		Tracker(const Tracker &) = delete;
		Tracker & operator = (const Tracker &) = delete;
		/**
		 * @brief Деструктор
		 */
		~Tracker() noexcept { Tracker::live--; }
	};
	// Инициализация счётчика живых экземпляров
	int Tracker::live = 0;
	/**
	 * @brief Функция сбора идентификаторов элементов вектора в порядке хранения
	 *
	 * @param vec вектор для обхода
	 * @return    вектор идентификаторов
	 *
	 */
	template <size_t N>
	std::vector <int> ids(awh::quic::small_vector <Tracker, N> & vec) noexcept {
		// Результирующий вектор идентификаторов
		std::vector <int> result;
		// Обходим элементы вектора диапазонным циклом (как в движке)
		for(auto & item : vec)
			// Дописываем идентификатор элемента
			result.push_back(item.id);
		// Выводим собранные идентификаторы
		return result;
	}
};

/**
 * @brief Тест инлайн-пути: добавление в пределах инлайн-хранилища
 *
 */
TEST(QuicSmallVector, InlineBasics){
	// Сбрасываем счётчик живых экземпляров
	Tracker::live = 0;
	{
		// Создаём вектор с инлайн-хранилищем на 2 элемента
		awh::quic::small_vector <Tracker, 2> vec;
		// Проверяем исходную пустоту
		EXPECT_TRUE(vec.empty());
		EXPECT_EQ(vec.size(), 0u);
		// Добавляем два элемента в пределах инлайн-хранилища
		vec.push_back(Tracker(10));
		vec.push_back(Tracker(20));
		// Проверяем размер и содержимое
		EXPECT_FALSE(vec.empty());
		EXPECT_EQ(vec.size(), 2u);
		EXPECT_EQ(ids(vec), (std::vector <int> {10, 20}));
	}
	// После разрушения вектора живых экземпляров быть не должно
	EXPECT_EQ(Tracker::live, 0);
}

/**
 * @brief Тест кучевого пути: рост сверх инлайн-хранилища с сохранением порядка
 *
 */
TEST(QuicSmallVector, GrowToHeapPreservesOrder){
	// Сбрасываем счётчик живых экземпляров
	Tracker::live = 0;
	{
		// Создаём вектор с инлайн-хранилищем на 2 элемента
		awh::quic::small_vector <Tracker, 2> vec;
		// Добавляем пять элементов - переход инлайн -> куча на третьем
		for(int i = 1; i <= 5; i++)
			vec.push_back(Tracker(i * 100));
		// Проверяем размер и сохранение порядка после переносов роста
		EXPECT_EQ(vec.size(), 5u);
		EXPECT_EQ(ids(vec), (std::vector <int> {100, 200, 300, 400, 500}));
		// Все элементы живы (5 в векторе)
		EXPECT_EQ(Tracker::live, 5);
	}
	// После разрушения вектора живых экземпляров быть не должно
	EXPECT_EQ(Tracker::live, 0);
}

/**
 * @brief Тест очистки с сохранением ёмкости и повторного использования
 *
 */
TEST(QuicSmallVector, ClearAndReuse){
	// Сбрасываем счётчик живых экземпляров
	Tracker::live = 0;
	{
		// Создаём вектор с инлайн-хранилищем на 2 элемента
		awh::quic::small_vector <Tracker, 2> vec;
		// Заполняем до кучевого хранилища
		for(int i = 1; i <= 4; i++)
			vec.push_back(Tracker(i));
		// Все четыре элемента живы
		EXPECT_EQ(Tracker::live, 4);
		// Очищаем вектор
		vec.clear();
		// После очистки элементов нет, но объект жив
		EXPECT_TRUE(vec.empty());
		EXPECT_EQ(Tracker::live, 0);
		// Повторно используем вектор
		vec.push_back(Tracker(7));
		vec.push_back(Tracker(8));
		vec.push_back(Tracker(9));
		EXPECT_EQ(ids(vec), (std::vector <int> {7, 8, 9}));
	}
	// После разрушения вектора живых экземпляров быть не должно
	EXPECT_EQ(Tracker::live, 0);
}

/**
 * @brief Тест конструктора перемещения из инлайн- и кучевого источника
 *
 */
TEST(QuicSmallVector, MoveConstruct){
	// Сбрасываем счётчик живых экземпляров
	Tracker::live = 0;
	{
		// Источник в инлайн-хранилище
		awh::quic::small_vector <Tracker, 2> src;
		src.push_back(Tracker(1));
		src.push_back(Tracker(2));
		// Перемещаем в новый вектор
		awh::quic::small_vector <Tracker, 2> dst(std::move(src));
		// Источник опустошён, приёмник получил элементы
		EXPECT_TRUE(src.empty());
		EXPECT_EQ(ids(dst), (std::vector <int> {1, 2}));
	}
	EXPECT_EQ(Tracker::live, 0);
	{
		// Источник в кучевом хранилище
		awh::quic::small_vector <Tracker, 2> src;
		for(int i = 1; i <= 5; i++)
			src.push_back(Tracker(i));
		// Перемещаем в новый вектор (кража кучевого буфера)
		awh::quic::small_vector <Tracker, 2> dst(std::move(src));
		// Источник опустошён, приёмник получил все элементы в порядке
		EXPECT_TRUE(src.empty());
		EXPECT_EQ(ids(dst), (std::vector <int> {1, 2, 3, 4, 5}));
	}
	EXPECT_EQ(Tracker::live, 0);
}

/**
 * @brief Тест оператора перемещения по всем сочетаниям инлайн/куча источника и приёмника
 *
 */
TEST(QuicSmallVector, MoveAssign){
	// Сбрасываем счётчик живых экземпляров
	Tracker::live = 0;
	{
		// Приёмник в куче, источник в куче
		awh::quic::small_vector <Tracker, 2> dst, src;
		for(int i = 1; i <= 5; i++) dst.push_back(Tracker(i));
		for(int i = 1; i <= 3; i++) src.push_back(Tracker(i * 10));
		dst = std::move(src);
		EXPECT_TRUE(src.empty());
		EXPECT_EQ(ids(dst), (std::vector <int> {10, 20, 30}));
	}
	EXPECT_EQ(Tracker::live, 0);
	{
		// Приёмник в куче, источник в инлайне (приёмник должен освободить кучу и вернуться в инлайн)
		awh::quic::small_vector <Tracker, 2> dst, src;
		for(int i = 1; i <= 5; i++) dst.push_back(Tracker(i));
		src.push_back(Tracker(99));
		dst = std::move(src);
		EXPECT_TRUE(src.empty());
		EXPECT_EQ(ids(dst), (std::vector <int> {99}));
		// Повторное заполнение после возврата в инлайн работает
		dst.push_back(Tracker(98));
		EXPECT_EQ(ids(dst), (std::vector <int> {99, 98}));
	}
	EXPECT_EQ(Tracker::live, 0);
}

/**
 * @brief Тест самоперемещения (защита от самоприсваивания)
 *
 */
TEST(QuicSmallVector, SelfMoveAssignIsSafe){
	// Сбрасываем счётчик живых экземпляров
	Tracker::live = 0;
	{
		// Создаём и заполняем вектор
		awh::quic::small_vector <Tracker, 2> vec;
		for(int i = 1; i <= 3; i++) vec.push_back(Tracker(i));
		// Самоперемещение через ссылку не должно разрушать содержимое
		awh::quic::small_vector <Tracker, 2> & ref = vec;
		vec = std::move(ref);
		// Содержимое сохранено
		EXPECT_EQ(ids(vec), (std::vector <int> {1, 2, 3}));
		EXPECT_EQ(Tracker::live, 3);
	}
	EXPECT_EQ(Tracker::live, 0);
}

/**
 * @brief Тест обмена содержимым по всем сочетаниям инлайн/куча
 *
 */
TEST(QuicSmallVector, Swap){
	// Сбрасываем счётчик живых экземпляров
	Tracker::live = 0;
	{
		// Инлайн <-> инлайн
		awh::quic::small_vector <Tracker, 2> a, b;
		a.push_back(Tracker(1));
		b.push_back(Tracker(2));
		b.push_back(Tracker(3));
		a.swap(b);
		EXPECT_EQ(ids(a), (std::vector <int> {2, 3}));
		EXPECT_EQ(ids(b), (std::vector <int> {1}));
	}
	EXPECT_EQ(Tracker::live, 0);
	{
		// Инлайн <-> куча
		awh::quic::small_vector <Tracker, 2> a, b;
		a.push_back(Tracker(1));
		for(int i = 1; i <= 5; i++) b.push_back(Tracker(i * 10));
		a.swap(b);
		EXPECT_EQ(ids(a), (std::vector <int> {10, 20, 30, 40, 50}));
		EXPECT_EQ(ids(b), (std::vector <int> {1}));
	}
	EXPECT_EQ(Tracker::live, 0);
	{
		// Куча <-> куча
		awh::quic::small_vector <Tracker, 2> a, b;
		for(int i = 1; i <= 4; i++) a.push_back(Tracker(i));
		for(int i = 1; i <= 6; i++) b.push_back(Tracker(i + 100));
		a.swap(b);
		EXPECT_EQ(ids(a), (std::vector <int> {101, 102, 103, 104, 105, 106}));
		EXPECT_EQ(ids(b), (std::vector <int> {1, 2, 3, 4}));
	}
	EXPECT_EQ(Tracker::live, 0);
}

/**
 * @brief Тест переноса через контейнер std::vector (реаллокация должна перемещать, не копировать)
 *
 */
TEST(QuicSmallVector, VectorOfSmallVectorsRealloc){
	// Сбрасываем счётчик живых экземпляров
	Tracker::live = 0;
	{
		// Контейнер векторов с малым инлайн-хранилищем
		std::vector <awh::quic::small_vector <Tracker, 2>> box;
		// Множественная вставка вызывает реаллокации контейнера (перемещение элементов)
		for(int n = 0; n < 32; n++){
			// Создаём вектор с кучевым и инлайн-содержимым попеременно
			awh::quic::small_vector <Tracker, 2> vec;
			const int count = ((n % 2 == 0) ? 5 : 1);
			for(int i = 0; i < count; i++) vec.push_back(Tracker(n * 1000 + i));
			// Перемещаем в контейнер
			box.push_back(std::move(vec));
		}
		// Проверяем сохранность содержимого после всех реаллокаций
		EXPECT_EQ(box.size(), 32u);
		EXPECT_EQ(ids(box[0]), (std::vector <int> {0, 1, 2, 3, 4}));
		EXPECT_EQ(ids(box[31]), (std::vector <int> {31000}));
	}
	EXPECT_EQ(Tracker::live, 0);
}
