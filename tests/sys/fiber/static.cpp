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
 * Стандартные заголовочные файлы
 */
#include <algorithm>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "fiber.hpp"

/**
 * Для операционной системы, отличной от MS Windows
 *
 * @note Подключение заведено ради щупа сторожевой страницы: он подаёт адрес
 *       доводом системного вызова, а стоком служит открытый /dev/null
 */
#if !defined(_WIN32) && !defined(_WIN64)
	#include <fcntl.h>
	#include <unistd.h>
#endif

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
	});
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
	});
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
	});
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
	});
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
	});
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
	});
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
	});
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
	ASSERT_EQ(fiber::spawn(nullptr), nullptr);
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
	});
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
 * @brief Тест роспуска спящего волокна
 *
 * @details Уничтожить спящее волокно нельзя: кадры его не раскручены. Роспуск -
 *          единственный способ довести спящее волокно до уничтожимого состояния:
 *          взводится признак, волокно будится и выходит само
 *
 */
TEST_F(FiberFixture, FiberDismissSuspendedTest){
	// Счётчик оборотов тела волокна
	uint32_t rounds = 0;
	// Заводим волокно, спрашивающее признак роспуска на каждом обороте
	fiber::ctx_t * worker = fiber::spawn([&rounds]() noexcept -> void {
		/**
		 * Выполняем работу, пока волокно не распущено
		 */
		while(!fiber::dismissed()){
			// Считаем оборот
			rounds++;
			// Усыпляем волокно
			fiber::yield();
		}
	});
	// Проверяем что волокно заведено
	ASSERT_NE(worker, nullptr);
	// Прокручиваем тело волокна трижды
	for(uint32_t i = 0; i < 3; i++)
		// Пробуждаем волокно
		ASSERT_TRUE(fiber::resume(worker));
	// Проверяем что тело отработало ровно три оборота
	ASSERT_EQ(rounds, static_cast <uint32_t> (3));
	// Проверяем что волокно спит
	ASSERT_EQ(fiber::state(worker), fiber::state_t::SUSPENDED);
	// Проверяем что спящее волокно уничтожить нельзя
	ASSERT_FALSE(fiber::destroy(worker));
	// Проверяем что роспуск довёл волокно до уничтожимого состояния
	ASSERT_TRUE(fiber::dismiss(worker));
	// Проверяем что тело больше не отработало ни одного оборота
	ASSERT_EQ(rounds, static_cast <uint32_t> (3));
	// Проверяем что волокно доработало
	ASSERT_EQ(fiber::state(worker), fiber::state_t::FINISHED);
	// Проверяем что распущенное волокно уничтожается
	ASSERT_TRUE(fiber::destroy(worker));
}

/**
 * @brief Тест отказа роспуска волокна, не спрашивающего признак
 *
 * @details Роспуск не вправе выходить молчаливой неудачей: тело, признак не
 *          спрашивающее, уснёт снова и останется спящим - и уничтожать его
 *          по-прежнему нельзя. Потребитель обязан узнать об этом кодом возврата
 *
 */
TEST_F(FiberFixture, FiberDismissIgnoredRefusedTest){
	// Заводим волокно, признак роспуска не спрашивающее вовсе
	fiber::ctx_t * worker = fiber::spawn([]() noexcept -> void {
		/**
		 * Засыпаем без конца, признака роспуска не спрашивая
		 */
		while(true)
			// Усыпляем волокно
			fiber::yield();
	});
	// Проверяем что волокно заведено
	ASSERT_NE(worker, nullptr);
	// Пробуждаем волокно: оно дойдёт до сна
	ASSERT_TRUE(fiber::resume(worker));
	// Проверяем что роспуск отвечает отказом
	ASSERT_FALSE(fiber::dismiss(worker));
	// Проверяем что волокно так и осталось спящим
	ASSERT_EQ(fiber::state(worker), fiber::state_t::SUSPENDED);
	// Проверяем что уничтожить его по-прежнему нельзя
	ASSERT_FALSE(fiber::destroy(worker));
}

/**
 * @brief Тест признака роспуска вне волокна
 *
 */
TEST_F(FiberFixture, FiberDismissedOutsideTest){
	// Проверяем что вне волокна признак роспуска отвечает ложью
	ASSERT_FALSE(fiber::dismissed());
	// Проверяем что роспуск несуществующего волокна отвечает отказом
	ASSERT_FALSE(fiber::dismiss(nullptr));
}

/**
 * @brief Тест роспуска волокна изнутри его самого
 *
 * @details Будить выполняющееся волокно отсюда нельзя: переход шёл бы в стек, на
 *          котором лежит кадр этого самого вызова. Признак взводится, ответом идёт
 *          отказ, а выходит тело само - уже после возврата из роспуска
 *
 */
TEST_F(FiberFixture, FiberSelfDismissTest){
	// Признак того, что тело волокна увидело роспуск
	bool noticed = false;
	// Волокно, распускающее само себя
	fiber::ctx_t * worker = nullptr;
	// Заводим волокно
	worker = fiber::spawn([&worker, &noticed]() noexcept -> void {
		// Проверяем что роспуск изнутри отвечает отказом: волокно ещё выполняется
		EXPECT_FALSE(fiber::dismiss(worker));
		// Запоминаем, увидело ли тело свой роспуск
		noticed = fiber::dismissed();
	});
	// Проверяем что волокно заведено
	ASSERT_NE(worker, nullptr);
	// Пробуждаем волокно: оно распустит само себя и выйдет
	ASSERT_TRUE(fiber::resume(worker));
	// Проверяем что тело увидело свой роспуск
	ASSERT_TRUE(noticed);
	// Проверяем что волокно доработало
	ASSERT_EQ(fiber::state(worker), fiber::state_t::FINISHED);
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
	});
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
	#if defined(SIZE_MAX) && (SIZE_MAX > 0xFFFFFFFFULL)
		// Проверяем что волокно с непосильным стеком не заводится
		ASSERT_EQ(fiber::spawn([]() noexcept -> void {}, (static_cast <size_t> (1) << 62)), nullptr);
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
		});
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

/**
 * Для операционной системы, отличной от MS Windows
 *
 * @note Стек волокну у MS Windows отводит сама система, сторожевой страницы модуль
 *       там не ставит, и проверять нечего
 */
#if !defined(_WIN32) && !defined(_WIN64)

/**
 * @brief Тест работоспособности волокна на стеке в одну страницу
 *
 * @details Опорная проверка к FiberStackGuardPageTest: та берёт стек ровно в одну
 *          страницу, и без этой проверки её отказ был бы неотличим от тесноты
 *
 */
TEST_F(FiberFixture, FiberSinglePageStackRunsTest){
	// Признак того, что тело волокна доработало до конца
	bool executed = false;
	// Заводим волокно со стеком ровно в одну страницу
	fiber::ctx_t * worker = fiber::spawn([&executed]() noexcept -> void {
		// Отмечаем работу волокна выполненной
		executed = true;
	}, static_cast <size_t> (::sysconf(_SC_PAGESIZE)));
	// Проверяем что волокно заведено
	ASSERT_NE(worker, nullptr);
	// Пробуждаем волокно
	ASSERT_TRUE(fiber::resume(worker));
	// Проверяем что тело волокна доработало до конца
	ASSERT_TRUE(executed);
	// Проверяем что доработавшее волокно уничтожается
	ASSERT_TRUE(fiber::destroy(worker));
}

/**
 * @brief Тест сторожевой страницы под стеком волокна
 *
 * @details Под стеком волокна лежит страница, закрытая mprotect(PROT_NONE):
 *          переполнение стека обязано валить работу на месте, а не молча править
 *          чужую память. Проверка эту защиту закрепляет - без неё сторожевую
 *          страницу можно снять, и набор останется зелёным
 *
 *          Защита закрепляется ДВУМЯ признаками, и порознь ни один не доказателен:
 *
 *          1. **Запас под стеком.** Стеки волокон ложатся вплотную, и шаг между
 *             ними виден снаружи: со сторожевой страницей он равен двум страницам,
 *             без неё - одной. Признак этот отвечает на вопрос «отведена ли
 *             страница», но не на вопрос «закрыта ли она»
 *          2. **Недоступность страницы.** Адрес на страницу ниже местной
 *             переменной подаётся ядру доводом системного вызова: закрытая память
 *             отвечает отказом EFAULT, а не валит процесс. Признак этот отвечает
 *             на вопрос «закрыта ли», но сам по себе прошёл бы и над
 *             неотображённой памятью
 *
 * @note Размер стека взят РОВНО в одну страницу намеренно. Тогда местная
 *       переменная лежит в пределах [основание, основание + страница), и адрес
 *       «минус страница» попадает в сторожевую страницу ПО РАСЧЁТУ, а не по удаче
 *
 * @warning Проверка на гибель процесса тут НЕ ГОДИТСЯ, и первые две её редакции
 *          вышли вечнозелёными. Волокно, заведённое в одиночку, валится при записи
 *          ниже основания и БЕЗ сторожевой страницы: там просто нет отображения.
 *          Ряд волокон эту беду снимает, но развилка процесса у смертельной
 *          проверки заводит своё расположение отображений, и смежность соседей
 *          снова выходит делом удачи. Оба признака выше наблюдаемы БЕЗ гибели, и
 *          от расположения не зависят. Разница замерена снятием сторожевой
 *          страницы: шаг 1.00 против 2.00 страниц, 7 пар из 7 в обоих прогонах
 *
 */
TEST_F(FiberFixture, FiberStackGuardPageTest){
	// Получаем размер страницы памяти
	const size_t page = static_cast <size_t> (::sysconf(_SC_PAGESIZE));
	// Количество заводимых волокон
	constexpr size_t COUNT = 8;
	/**
	 * Заводим канал, служащий стоком щупу памяти
	 *
	 * @warning Сток обязан довод ЧИТАТЬ. Первая редакция подавала щуп в открытый
	 *          /dev/null - а он у macOS довода не читает вовсе и отвечает успехом
	 *          на ЛЮБОЙ адрес, включая закрытый. Проверка от этого краснела при
	 *          исправной защите
	 */
	int32_t sink[2] = {-1, -1};
	// Проверяем что канал заведён
	ASSERT_EQ(::pipe(sink), 0);
	// Набор заведённых волокон
	std::vector <fiber::ctx_t *> workers;
	// Набор адресов местных переменных, снятых телами волокон
	std::vector <char *> stacks(COUNT, nullptr);
	// Набор итогов щупа страницы под основанием стека
	std::vector <int32_t> probes(COUNT, 0);
	// Набор кодов отказа, полученных щупом
	std::vector <int32_t> errors(COUNT, 0);
	/**
	 * Заводим ряд волокон со стеком ровно в одну страницу
	 */
	for(size_t i = 0; i < COUNT; i++){
		// Заводим очередное волокно
		fiber::ctx_t * worker = fiber::spawn([&stacks, &probes, &errors, i, page, &sink]() noexcept -> void {
			// Заводим местную переменную, лежащую в пределах стека волокна
			volatile char probe = 0;
			// Запоминаем адрес местной переменной
			stacks[i] = const_cast <char *> (&probe);
			/**
			 * Щупаем страницу под основанием стека доводом системного вызова
			 *
			 * @note Ядро отвечает на закрытую память отказом EFAULT, а не валит
			 *       процесс - потому щуп и подаётся вызовом, а не обращением
			 */
			probes[i] = static_cast <int32_t> (::write(sink[1], (stacks[i] - static_cast <std::ptrdiff_t> (page)), 1));
			// Запоминаем код отказа
			errors[i] = errno;
		}, page);
		// Проверяем что волокно заведено
		ASSERT_NE(worker, nullptr) << "волокно: " << i;
		// Запоминаем заведённое волокно
		workers.push_back(worker);
	}
	/**
	 * Прокручиваем тела волокон
	 */
	for(size_t i = 0; i < COUNT; i++)
		// Пробуждаем очередное волокно
		ASSERT_TRUE(fiber::resume(workers.at(i))) << "волокно: " << i;
	/**
	 * Признак второй: страница под основанием стека закрыта
	 */
	for(size_t i = 0; i < COUNT; i++){
		// Проверяем что подача закрытой памяти ядру отвергнута
		ASSERT_EQ(probes.at(i), -1) << "волокно: " << i;
		// Проверяем что отказ получен именно на недоступную память
		ASSERT_EQ(errors.at(i), EFAULT) << "волокно: " << i;
	}
	// Выстраиваем снятые адреса по возрастанию
	std::sort(stacks.begin(), stacks.end());
	/**
	 * Признак первый: под каждым стеком отведён запас в страницу
	 */
	for(size_t i = 1; i < COUNT; i++)
		// Проверяем что соседние стеки отстоят не менее чем на две страницы
		ASSERT_GE(static_cast <size_t> (stacks.at(i) - stacks.at(i - 1)), (page * 2)) << "пара: " << i;
	/**
	 * Уничтожаем заведённые волокна
	 */
	for(size_t i = 0; i < COUNT; i++)
		// Уничтожаем очередное волокно
		ASSERT_TRUE(fiber::destroy(workers.at(i))) << "волокно: " << i;
	// Закрываем принимающий конец стока
	::close(sink[0]);
	// Закрываем подающий конец стока
	::close(sink[1]);
}

#endif
