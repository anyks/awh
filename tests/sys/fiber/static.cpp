/**
 * @file static.cpp
 * @date 2026-08-26
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
 * @brief Статические тесты модуля волокон — заведение и уничтожение, усыпление и
 *        пробуждение, сохранность кадра вызова через сон, вложенность волокон,
 *        отказы на неверных доводах и на исчерпании памяти
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "fiber.hpp"

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * @brief Тест заведения и уничтожения волокна
 *
 */
TEST_F(FiberFixture, FiberSpawnAndDestroyTest){
	// Признак того, что работа волокна выполнялась
	bool executed = false;
	// Заводим волокно
	fiber::ctx_t * worker = fiber::spawn([&executed]() noexcept -> void {
		// Отмечаем работу волокна выполненной
		executed = true;
	}, this->_log.get());
	// Проверяем что волокно заведено
	ASSERT_NE(worker, nullptr);
	// Проверяем что заведённое волокно спит, а не работает
	ASSERT_EQ(fiber::state(worker), fiber::state_t::SUSPENDED);
	// Проверяем что работа волокна до пробуждения НЕ выполнялась
	ASSERT_FALSE(executed);
	// Пробуждаем волокно
	ASSERT_TRUE(fiber::resume(worker));
	// Проверяем что работа волокна выполнена
	ASSERT_TRUE(executed);
	// Проверяем что волокно доработало
	ASSERT_EQ(fiber::state(worker), fiber::state_t::FINISHED);
	// Проверяем что доработавшее волокно уничтожается
	ASSERT_TRUE(fiber::destroy(worker));
}

/**
 * @brief Тест сохранности кадра вызова через сон волокна
 *
 * @details Это и есть суть модуля: переменные кадра обязаны пережить сон, потому
 *          что кадр лежит на стеке волокна, а не на стеке разбудившей стороны
 *
 */
TEST_F(FiberFixture, FiberKeepsFrameAcrossYieldTest){
	// Итог, собираемый волокном по обе стороны сна
	std::string result = "";
	// Заводим волокно
	fiber::ctx_t * worker = fiber::spawn([&result]() noexcept -> void {
		// Заводим переменную кадра ДО сна
		const std::string before = "до";
		// Усыпляем волокно
		fiber::yield();
		// Собираем итог, обращаясь к переменной кадра ПОСЛЕ сна
		result = (before + "-после");
	}, this->_log.get());
	// Проверяем что волокно заведено
	ASSERT_NE(worker, nullptr);
	// Пробуждаем волокно: оно дойдёт до сна и вернёт управление
	ASSERT_TRUE(fiber::resume(worker));
	// Проверяем что волокно спит, а не доработало
	ASSERT_EQ(fiber::state(worker), fiber::state_t::SUSPENDED);
	// Проверяем что итог ещё не собран
	ASSERT_TRUE(result.empty());
	// Пробуждаем волокно вновь
	ASSERT_TRUE(fiber::resume(worker));
	// Проверяем что переменная кадра пережила сон
	ASSERT_STREQ(result.c_str(), "до-после");
	// Проверяем что волокно доработало
	ASSERT_EQ(fiber::state(worker), fiber::state_t::FINISHED);
	// Уничтожаем волокно
	ASSERT_TRUE(fiber::destroy(worker));
}

/**
 * @brief Тест многократного усыпления и пробуждения волокна
 *
 */
TEST_F(FiberFixture, FiberManyYieldsTest){
	// Количество кругов сна
	constexpr uint16_t ROUNDS = 1000;
	// Счётчик пройденных кругов
	uint16_t counter = 0;
	// Заводим волокно
	fiber::ctx_t * worker = fiber::spawn([&counter]() noexcept -> void {
		/**
		 * Проходим заданное количество кругов сна
		 */
		for(uint16_t i = 0; i < ROUNDS; i++){
			// Считаем очередной круг
			counter++;
			// Усыпляем волокно
			fiber::yield();
		}
	}, this->_log.get());
	// Проверяем что волокно заведено
	ASSERT_NE(worker, nullptr);
	/**
	 * Гоняем волокно по кругам, как это делал бы цикл событий
	 */
	for(uint16_t i = 0; i < ROUNDS; i++){
		// Пробуждаем волокно
		ASSERT_TRUE(fiber::resume(worker)) << "круг: " << i;
		// Проверяем что счётчик подвинулся ровно на один круг
		ASSERT_EQ(counter, (i + 1)) << "круг: " << i;
	}
	// Добиваем волокно последним пробуждением
	ASSERT_TRUE(fiber::resume(worker));
	// Проверяем что волокно доработало
	ASSERT_EQ(fiber::state(worker), fiber::state_t::FINISHED);
	// Уничтожаем волокно
	ASSERT_TRUE(fiber::destroy(worker));
}

/**
 * @brief Тест определения текущего волокна
 *
 */
TEST_F(FiberFixture, FiberCurrentTest){
	// Волокно, увиденное изнутри работы
	fiber::ctx_t * inside = nullptr;
	// Проверяем что вне волокна текущего волокна нет
	ASSERT_EQ(fiber::current(), nullptr);
	// Заводим волокно
	fiber::ctx_t * worker = fiber::spawn([&inside]() noexcept -> void {
		// Запоминаем волокно, увиденное изнутри
		inside = fiber::current();
	}, this->_log.get());
	// Проверяем что волокно заведено
	ASSERT_NE(worker, nullptr);
	// Пробуждаем волокно
	ASSERT_TRUE(fiber::resume(worker));
	// Проверяем что изнутри волокно увидело само себя
	ASSERT_EQ(inside, worker);
	// Проверяем что по возвращении текущего волокна снова нет
	ASSERT_EQ(fiber::current(), nullptr);
	// Уничтожаем волокно
	ASSERT_TRUE(fiber::destroy(worker));
}

/**
 * @brief Тест вложенности волокон
 *
 * @details Волокно вправе разбудить другое волокно и дождаться его: у каждого свой
 *          стек, и переключение одного не задевает другого
 *
 */
/**
 * @brief Проверка отказа уничтожить волокно изнутри его самого
 *
 * @details Уничтожение снимает отображение стека волокна, а при вызове ИЗНУТРИ на
 *          этом самом стеке лежит кадр вызывающего: возврат пошёл бы по снятому
 *          отображению. Достижимо это стало вместе с выводом наружу `current()` -
 *          пока указателя на выполняющееся волокно взять было неоткуда, состояние
 *          `RUNNING` в уничтожение не попадало вовсе
 *
 * @note Проверка утверждает не только отказ, но и то, что волокно ДОРАБОТАЛО после
 *       него: отказ, после которого управление не вернулось, был бы неотличим от
 *       крушения, ради предотвращения которого он и заведён
 *
 */
TEST_F(FiberFixture, FiberSelfDestroyRefusedTest){
	// Исход попытки уничтожения, снятый изнутри волокна
	bool refused = false;
	// Признак того, что волокно доработало до конца после отказа
	bool survived = false;
	// Заводим волокно, которое пробует уничтожить само себя
	fiber::ctx_t * worker = fiber::spawn([&refused, &survived]() noexcept -> void {
		// Пробуем уничтожить волокно, в котором сами же и выполняемся
		refused = !fiber::destroy(fiber::current());
		// Отмечаем, что управление вернулось и работа продолжилась
		survived = true;
	}, this->_log.get());
	// Проверяем что волокно заведено
	ASSERT_NE(worker, nullptr);
	// Пробуждаем волокно
	ASSERT_TRUE(fiber::resume(worker));
	// Проверяем что уничтожение изнутри отвергнуто
	EXPECT_TRUE(refused) << "уничтожение выполняющегося волокна не отвергнуто";
	// Проверяем что после отказа волокно доработало
	EXPECT_TRUE(survived) << "управление после отказа не вернулось";
	// Проверяем что волокно доработало и подлежит уничтожению
	EXPECT_EQ(fiber::state(worker), fiber::state_t::FINISHED);
	// Уничтожаем волокно снаружи, где это и дозволено
	EXPECT_TRUE(fiber::destroy(worker));
}

TEST_F(FiberFixture, FiberNestedTest){
	// Роспись порядка, в котором шло выполнение
	std::string sign = "";
	// Внутреннее волокно
	fiber::ctx_t * inner = fiber::spawn([&sign]() noexcept -> void {
		// Отмечаем вход во внутреннее волокно
		sign.append("[внутреннее:вход]");
		// Усыпляем внутреннее волокно
		fiber::yield();
		// Отмечаем возврат во внутреннее волокно
		sign.append("[внутреннее:возврат]");
	}, this->_log.get());
	// Проверяем что внутреннее волокно заведено
	ASSERT_NE(inner, nullptr);
	// Внешнее волокно
	fiber::ctx_t * outer = fiber::spawn([&sign, inner]() noexcept -> void {
		// Отмечаем вход во внешнее волокно
		sign.append("[внешнее:вход]");
		// Пробуждаем внутреннее волокно
		fiber::resume(inner);
		// Проверяем что после возврата текущим снова стало внешнее волокно
		sign.append((fiber::current() != nullptr) ? "[текущее:внешнее]" : "[текущее:НЕТ]");
		// Усыпляем внешнее волокно
		fiber::yield();
		// Отмечаем возврат во внешнее волокно
		sign.append("[внешнее:возврат]");
	}, this->_log.get());
	// Проверяем что внешнее волокно заведено
	ASSERT_NE(outer, nullptr);
	// Пробуждаем внешнее волокно
	ASSERT_TRUE(fiber::resume(outer));
	// Проверяем что оба волокна спят
	ASSERT_EQ(fiber::state(outer), fiber::state_t::SUSPENDED);
	ASSERT_EQ(fiber::state(inner), fiber::state_t::SUSPENDED);
	// Добиваем внутреннее волокно
	ASSERT_TRUE(fiber::resume(inner));
	// Добиваем внешнее волокно
	ASSERT_TRUE(fiber::resume(outer));
	// Проверяем порядок выполнения
	ASSERT_STREQ(sign.c_str(), "[внешнее:вход][внутреннее:вход][текущее:внешнее][внутреннее:возврат][внешнее:возврат]");
	// Уничтожаем волокна
	ASSERT_TRUE(fiber::destroy(inner));
	ASSERT_TRUE(fiber::destroy(outer));
}

/**
 * @brief Тест отказа заведения волокна без работы
 *
 */
TEST_F(FiberFixture, FiberSpawnWithoutTaskTest){
	// Проверяем что волокно без работы не заводится
	ASSERT_EQ(fiber::spawn(nullptr, this->_log.get()), nullptr);
}

/**
 * @brief Тест отказа уничтожения спящего волокна
 *
 * @details Кадры спящего волокна не раскручены, и всё, что они держат, осталось бы
 *          неосвобождённым - от объектов на его стеке до захваченных им ресурсов
 *
 */
TEST_F(FiberFixture, FiberDestroySuspendedRefusedTest){
	// Заводим волокно
	fiber::ctx_t * worker = fiber::spawn([]() noexcept -> void {
		// Усыпляем волокно
		fiber::yield();
	}, this->_log.get());
	// Проверяем что волокно заведено
	ASSERT_NE(worker, nullptr);
	// Проверяем что заведённое, но ни разу не разбуженное волокно уничтожить нельзя
	ASSERT_FALSE(fiber::destroy(worker));
	// Пробуждаем волокно: оно дойдёт до сна
	ASSERT_TRUE(fiber::resume(worker));
	// Проверяем что спящее волокно уничтожить нельзя
	ASSERT_FALSE(fiber::destroy(worker));
	// Добиваем волокно
	ASSERT_TRUE(fiber::resume(worker));
	// Проверяем что доработавшее волокно уничтожается
	ASSERT_TRUE(fiber::destroy(worker));
}

/**
 * @brief Тест отказов на неверных доводах
 *
 */
TEST_F(FiberFixture, FiberRefusalsTest){
	// Проверяем что несуществующее волокно считается доработавшим
	ASSERT_EQ(fiber::state(nullptr), fiber::state_t::FINISHED);
	// Проверяем что несуществующее волокно не пробуждается
	ASSERT_FALSE(fiber::resume(nullptr));
	// Проверяем что несуществующее волокно не уничтожается
	ASSERT_FALSE(fiber::destroy(nullptr));
	// Проверяем что усыпление вне волокна не валит проверку
	fiber::yield();
}

/**
 * @brief Тест отказа повторного пробуждения доработавшего волокна
 *
 */
TEST_F(FiberFixture, FiberResumeFinishedRefusedTest){
	// Счётчик выполнений работы волокна
	uint8_t counter = 0;
	// Заводим волокно
	fiber::ctx_t * worker = fiber::spawn([&counter]() noexcept -> void {
		// Считаем выполнение работы волокна
		counter++;
	}, this->_log.get());
	// Проверяем что волокно заведено
	ASSERT_NE(worker, nullptr);
	// Пробуждаем волокно
	ASSERT_TRUE(fiber::resume(worker));
	// Проверяем что работа выполнена единожды
	ASSERT_EQ(counter, 1);
	// Проверяем что доработавшее волокно повторно не пробуждается
	ASSERT_FALSE(fiber::resume(worker));
	// Проверяем что работа повторно не выполнялась
	ASSERT_EQ(counter, 1);
	// Уничтожаем волокно
	ASSERT_TRUE(fiber::destroy(worker));
}

/**
 * @brief Тест отказа заведения волокна с непосильным стеком
 *
 * @note Размер берётся заведомо неотводимым, чтобы отказ пришёл от системы, а не
 *       от исчерпания памяти машины
 *
 */
TEST_F(FiberFixture, FiberHugeStackRefusedTest){
	/**
	 * Если разрядность машины 64-битная
	 */
	#if (SIZE_MAX > 0xFFFFFFFFULL)
		// Проверяем что волокно с непосильным стеком не заводится
		ASSERT_EQ(fiber::spawn([]() noexcept -> void {}, (static_cast <size_t> (1) << 62), this->_log.get()), nullptr);
	/**
	 * Если разрядность машины 32-битная
	 */
	#else
		// Проверку пропускаем: непосильного размера в пределах разрядности не назвать
		GTEST_SKIP() << "разрядность машины не позволяет назвать заведомо непосильный размер стека";
	#endif
}

/**
 * @brief Тест множества одновременно живущих волокон
 *
 */
TEST_F(FiberFixture, FiberManyAliveTest){
	// Количество одновременно живущих волокон
	constexpr size_t COUNT = 64;
	// Набор заведённых волокон
	std::vector <fiber::ctx_t *> workers;
	// Набор итогов, собранных волокнами
	std::vector <size_t> results(COUNT, 0);
	/**
	 * Заводим набор волокон
	 */
	for(size_t i = 0; i < COUNT; i++){
		// Заводим очередное волокно
		fiber::ctx_t * worker = fiber::spawn([&results, i]() noexcept -> void {
			// Усыпляем волокно
			fiber::yield();
			// Собираем итог: каждое волокно пишет СВОЁ число
			results[i] = (i + 1);
		}, this->_log.get());
		// Проверяем что волокно заведено
		ASSERT_NE(worker, nullptr) << "волокно: " << i;
		// Запоминаем заведённое волокно
		workers.push_back(worker);
	}
	/**
	 * Доводим все волокна до сна
	 */
	for(size_t i = 0; i < COUNT; i++)
		// Пробуждаем очередное волокно
		ASSERT_TRUE(fiber::resume(workers[i])) << "волокно: " << i;
	/**
	 * Добиваем волокна в ОБРАТНОМ порядке: порядок пробуждения ни на что не влияет
	 */
	for(size_t i = COUNT; i > 0; i--)
		// Пробуждаем очередное волокно
		ASSERT_TRUE(fiber::resume(workers[i - 1])) << "волокно: " << (i - 1);
	/**
	 * Проверяем итоги всех волокон
	 */
	for(size_t i = 0; i < COUNT; i++){
		// Проверяем что волокно собрало СВОЙ итог
		ASSERT_EQ(results[i], (i + 1)) << "волокно: " << i;
		// Уничтожаем волокно
		ASSERT_TRUE(fiber::destroy(workers[i])) << "волокно: " << i;
	}
}
