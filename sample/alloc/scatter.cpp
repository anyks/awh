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
 * @brief Пример зашивания ключевого материала в образ — раскладка секрета по носителю
 *        под наложением гаммы (`lay`), порождение исходного текста с носителем (`emit`)
 *        и сборка секрета на запуске прямо в приёмник тайн (`gather`)
 *
 * @details Полный цикл здесь показан ОДНОЙ программой ради наглядности. В бою стороны
 *          разнесены: `lay` и `emit` работают при сборке на закрытой машине, где лежит
 *          секрет и зерно, а их выход - носитель - уходит в образ; `gather` работает
 *          на запуске в приложении, у которого секрета нет, есть лишь носитель и то же
 *          зерно. Секрет наружу образа не уходит никогда.
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/macro/lib.hpp>
#include <alloc/alloc.hpp>
#include <alloc/scatter.hpp>
#include <alloc/vessel.hpp>
#include <sys/log.hpp>
#include <sys/fmk.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * @brief Метод поиска открытой подстроки секрета в носителе
 *
 * @param carrier носитель с рассеянным секретом
 * @param secret  адрес секрета
 * @param length  длина секрета в байтах
 * @return         признак присутствия открытой подстроки секрета
 *
 */
static bool exposed(const std::vector <uint8_t> & carrier, const uint8_t * secret, const size_t length) noexcept {
	// Если искать нечего
	if(carrier.size() < length)
		// Открытой подстроки нет
		return false;
	/**
	 * Ищем открытую подстроку секрета во всех местах носителя
	 */
	for(size_t i = 0; (i + length) <= carrier.size(); i++){
		// Если подстрока секрета нашлась на этом месте
		if(::memcmp(carrier.data() + i, secret, length) == 0)
			// Открытая подстрока секрета присутствует
			return true;
	}
	// Открытой подстроки секрета в носителе нет
	return false;
}

/**
 * @brief Функция запуска приложения
 *
 * @param argc количество передаваемых аргументов
 * @param argv список передаваемых аргументов
 * @return     код выхода из приложения
 *
 */
int main(int argc, char * argv[]) noexcept {
	// Обходим неиспользуемые аргументы
	static_cast <void> (argc);
	static_cast <void> (argv);
	// Заводим распределитель памяти
	alloc::allocator_t::capture(alloc::allocator_t::options());
	/**
	 * ШАГ ЗАКРЫТОГО КОНТУРА (время сборки)
	 *
	 * Здесь лежит секрет и зерно. Ни того, ни другого в образ не уходит: секрет
	 * растворяется в носителе под наложением гаммы, а зерно остаётся в закрытом контуре
	 * рядом с ключом. В бою этот шаг ведёт утилита сборки, а не приложение
	 */
	// Зашиваемый секрет — здесь приватный ключ либо пароль
	const char secret[] = "ANYKS-PRIVATE-KEY-0123456789ABCD";
	// Длина секрета без завершающего нуля
	const size_t length = sizeof(secret) - 1;
	// Зерно раскладки из закрытого контура
	const uint64_t seed = 0xA5C3E17B9D42F680ULL;
	// Ёмкость носителя: запас сверх длины рассеивает секрет пошире
	const size_t capacity = 1024;
	// Раскладываем секрет по носителю под наложением гаммы
	const std::vector <uint8_t> carrier = alloc::Scatter::lay(reinterpret_cast <const uint8_t *> (secret), length, seed, capacity);
	// Если раскладка не удалась
	if(carrier.empty()){
		// Печатаем отказ раскладки
		::printf("Раскладка секрета не удалась\n");
		// Выходим с ошибкой
		return EXIT_FAILURE;
	}
	// Печатаем свод раскладки
	::printf("=== Закрытый контур (сборка) ===\n");
	::printf("Длина секрета:   %zu байт\n", length);
	::printf("Ёмкость носителя: %zu байт\n", carrier.size());
	/**
	 * Проверяем, что открытого секрета в носителе нет
	 *
	 * Ляг секрет открытым, пусть и рассеянным, его подстрока нашлась бы поиском.
	 * Наложение гаммой и рассеяние вместе такого не оставляют
	 */
	::printf("Открытый секрет в носителе: %s\n",
	 ::exposed(carrier, reinterpret_cast <const uint8_t *> (secret), length) ? "НАЙДЕН (беда)" : "не найден");
	/**
	 * Порождаем исходный текст с носителем
	 *
	 * В бою этот текст перенаправляют в заголовок закрытого контура и включают в сборку
	 * приложения. Ни секрета, ни зерна текст не несёт — только носитель и длину
	 */
	const std::string source = alloc::Scatter::emit(carrier, length, "embedded_key");
	// Печатаем начало порождённого текста
	::printf("\n=== Порождённый заголовок (первые строки) ===\n");
	// Печатаем не более первых трёхсот знаков текста
	::printf("%.300s%s\n", source.c_str(), (source.size() > 300) ? "\n..." : "");
	/**
	 * ШАГ ПРИЛОЖЕНИЯ (запуск)
	 *
	 * Здесь секрета нет. Есть носитель (он в образе) и то же зерно (оно рядом с ключом
	 * в закрытом контуре, откуда попало в сборку). Приложение собирает секрет прямо в
	 * приёмник тайн, минуя кучу, и работает с ним лишь на время обращения
	 */
	::printf("\n=== Приложение (запуск) ===\n");
	// Заводим приёмник тайн послабленным: пример должен идти и там, где запереть страницы нечем
	alloc::vessel_t vessel(alloc::secrecy_t::RELAXED);
	// Собираем секрет из носителя тем же зерном
	if(!alloc::Scatter::gather(carrier.data(), carrier.size(), seed, length, vessel)){
		// Печатаем отказ сборки
		::printf("Сборка секрета не удалась\n");
		// Выходим с ошибкой
		return EXIT_FAILURE;
	}
	// Печатаем занятое в приёмнике
	::printf("Собрано в приёмник: %zu байт\n", vessel.size());
	/**
	 * Работаем с собранным секретом на время обращения
	 *
	 * За пределами обработчика приёмник закрыт: чтение по адресу отвечает отказом, а не
	 * байтами. Здесь секрет живёт лишь время вызова — ровно то, ради чего склад заведён
	 */
	bool matched = false;
	// Обращаемся к собранному секрету
	vessel.apply([&](const uint8_t * data, const size_t size) noexcept {
		// Сверяем собранное с исходным секретом
		matched = ((size == length) && (::memcmp(data, secret, length) == 0));
	});
	// Печатаем итог сверки
	::printf("Собранный секрет совпал с исходным: %s\n", matched ? "да" : "нет");
	/**
	 * Проверяем стойкость к чужому зерну
	 *
	 * Тот же носитель, но иное зерно — и собирается мусор, а не секрет: раскладку задаёт
	 * зерно, и без него носитель не читается
	 */
	alloc::vessel_t other(alloc::secrecy_t::RELAXED);
	// Собираем чужим зерном
	if(alloc::Scatter::gather(carrier.data(), carrier.size(), seed ^ 0x1ULL, length, other)){
		// Признак совпадения собранного чужим зерном
		bool wrong = true;
		// Обращаемся к собранному чужим зерном
		other.apply([&](const uint8_t * data, const size_t size) noexcept {
			// Сверяем собранное с исходным секретом
			wrong = ((size == length) && (::memcmp(data, secret, length) == 0));
		});
		// Печатаем итог сборки чужим зерном
		::printf("Сборка чужим зерном дала секрет: %s\n", wrong ? "да (беда)" : "нет");
	}
	/**
	 * ПРИВЯЗКА К ЯКОРЮ: контрольная сумма кода и версия ключа
	 *
	 * В бою зерно подают не голым, а привязанным к контрольной сумме собственного кода и
	 * версии ключа через `anchor`. Патч образа меняет сумму, а с ней зерно и весь поток:
	 * секрет собирается неверным, и контейнер просто не открывается. Саму сумму и участок
	 * образа подаёт закрытый контур; здесь для примера сумма задана числом
	 */
	::printf("\n=== Привязка к якорю (контрольная сумма + версия) ===\n");
	// Мнимая контрольная сумма собственного кода
	const uint64_t checksum = 0x1122334455667788ULL;
	// Версия ключа: боевая
	const uint64_t version = 1;
	// Раскладываем секрет зерном, привязанным к сумме и версии
	const std::vector <uint8_t> bound = alloc::Scatter::lay(reinterpret_cast <const uint8_t *> (secret), length, alloc::Scatter::anchor(seed, checksum, version), capacity);
	// Собираем той же суммой и версией
	alloc::vessel_t anchored(alloc::secrecy_t::RELAXED);
	// Признак успешной сборки верным якорем
	bool byRight = false;
	// Если сборка верным якорем удалась
	if(alloc::Scatter::gather(bound.data(), bound.size(), alloc::Scatter::anchor(seed, checksum, version), length, anchored))
		// Обращаемся к собранному верным якорем
		anchored.apply([&](const uint8_t * data, const size_t size) noexcept {
			// Сверяем собранное с исходным секретом
			byRight = ((size == length) && (::memcmp(data, secret, length) == 0));
		});
	::printf("Верной суммой и версией секрет собран: %s\n", byRight ? "да" : "нет");
	// Собираем изменённой суммой — имитируем патч кода
	alloc::vessel_t patched(alloc::secrecy_t::RELAXED);
	// Признак сборки секрета изменённой суммой
	bool byPatched = true;
	// Если сборка изменённой суммой отработала
	if(alloc::Scatter::gather(bound.data(), bound.size(), alloc::Scatter::anchor(seed, checksum ^ 0x1ULL, version), length, patched))
		// Обращаемся к собранному изменённой суммой
		patched.apply([&](const uint8_t * data, const size_t size) noexcept {
			// Сверяем собранное с исходным секретом
			byPatched = ((size == length) && (::memcmp(data, secret, length) == 0));
		});
	::printf("Патч кода (иная сумма) дал секрет: %s\n", byPatched ? "да (беда)" : "нет");

	/**
	 * ВПЛЕТЕНИЕ В РАБОЧУЮ ТАБЛИЦУ: маскировка под данные, которые используются
	 *
	 * `lay` заводит свой носитель, весь высокой неопределённости, — сканер нашёл бы его
	 * целиком. `weave` вместо того вплетает секрет в ПРЕДОСТАВЛЕННУЮ рабочую таблицу,
	 * правя лишь места секрета, а прочие её данные оставляя нетронутыми. Так секрет
	 * сливается с осмысленными данными
	 */
	::printf("\n=== Вплетение в рабочую таблицу ===\n");
	// Заводим рабочую таблицу с осмысленными данными
	std::vector <uint8_t> table(512);
	// Заполняем таблицу рабочими данными
	for(size_t i = 0; i < table.size(); i++) table[i] = static_cast <uint8_t> ((i * 31) + 7);
	// Снимаем копию таблицы до вплетения
	const std::vector <uint8_t> before = table;
	// Вплетаем секрет в рабочую таблицу
	if(alloc::Scatter::weave(table.data(), table.size(), reinterpret_cast <const uint8_t *> (secret), length, seed)){
		// Число изменённых мест таблицы
		size_t changed = 0;
		// Считаем изменённые места таблицы
		for(size_t i = 0; i < table.size(); i++) if(table[i] != before[i]) changed++;
		::printf("Размер таблицы: %zu, изменено мест: %zu (длина секрета %zu)\n", table.size(), changed, length);
		// Собираем секрет из вплетённой таблицы
		alloc::vessel_t woven(alloc::secrecy_t::RELAXED);
		// Признак сборки секрета из таблицы
		bool fromTable = false;
		// Если сборка из таблицы удалась
		if(alloc::Scatter::gather(table.data(), table.size(), seed, length, woven))
			// Обращаемся к собранному из таблицы
			woven.apply([&](const uint8_t * data, const size_t size) noexcept {
				// Сверяем собранное с исходным секретом
				fromTable = ((size == length) && (::memcmp(data, secret, length) == 0));
			});
		::printf("Секрет собран из рабочей таблицы: %s\n", fromTable ? "да" : "нет");
	}

	// Выходим успешно
	return EXIT_SUCCESS;
}
