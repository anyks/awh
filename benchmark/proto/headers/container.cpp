/**
 * @file container.cpp
 * @date 2026-07-31
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
 * @brief Бенчмарки горячих путей контейнера HTTP-заголовков
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <cstdio>

/**
 * Подключаем заголовочный файл бенчмарков
 */
#include "headers.hpp"

/**
 * Подписываемся на пространство имён бенчмарков контейнера HTTP-заголовков
 */
using namespace awh::benchmark::headers;

/**
 * @brief Внутренние параметры и сценарии бенчмарков контейнера заголовков
 *
 */
namespace {
	/**
	 * @brief Пороги количества операций в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке репозитория с двукратным
	 *          запасом: они ловят регрессию в разы, а не колебания планировщика
	 *          операционной системы. Отставание в десять-пятнадцать процентов порогу
	 *          по устройству незаметно - его ловит сверка долей с соперниками,
	 *          снятая стендами tools/benchmark/headers
	 *
	 */
	static constexpr double BUILD_THRESHOLD = 70000.0;
	static constexpr double LOOKUP_THRESHOLD = 2000000.0;
	static constexpr double SERIALIZE_THRESHOLD = 250000.0;
	static constexpr double REPLACE_THRESHOLD = 350000.0;
	/**
	 * @brief Пороги количества выделений памяти на одну операцию
	 *
	 * @details Показатель от машины и режима сборки не зависит, поэтому пороги заданы
	 *          вплотную к измеренным значениям.
	 *
	 *          Наполнению положено ровно одно выделение на поле: короткие названия
	 *          размещаются внутри самой строки и не выделяют ничего, длинные значения
	 *          выделяют по разу. Из десяти полей образца длиннее порога шесть,
	 *          а место под сам список отводится один раз при укладке первого поля:
	 *          шесть значений и один список - семь выделений. Восьмое означало бы
	 *          работу, которой контейнер не делал. Прежде их было десять - список
	 *          рос удвоением и проходил через четыре перевыделения вместо одного.
	 *
	 *          Поиску и замене не положено ни одного выделения. Поиск ничего наружу
	 *          не отдаёт и возвращает значение ссылкой, а замена короткого значения
	 *          на короткое размещается внутри самой строки и переиспользует ёмкость
	 *          набора. Единица здесь означала бы, что операция начала копировать.
	 *
	 *          Сборке сообщения положено два: стартовая строка и сам результат.
	 *          Прежде их было восемь - общий сборщик копировал набор целиком (одно
	 *          выделение под список и по одному под каждое значение длиннее порога
	 *          размещения внутри строки). Для протоколов вне семейства HTTP/2 сборщик
	 *          отдаёт набор без единого изменения, и копия не несла никакой работы,
	 *          поэтому печать идёт по самому набору. Порог опущен до измеренного
	 *          значения: возврат к восьми означал бы, что копия вернулась
	 *
	 * @warning Показатель этот воспроизводим до единиц в пределах одной стандартной
	 *          библиотеки, но не между ними: у libstdc++ короткая строка
	 *          размещается внутри объекта строки по иной границе, чем у libc++, и
	 *          значение, укладывающееся у одной внутрь объекта, у другой требует
	 *          выделения. Замер на стендах дал у NetBSD на единицу больше, чем у
	 *          macOS, FreeBSD и OpenBSD, совпавших между собой в точности
	 *
	 * @note Порог потому задан по стандартной библиотеке, а не поднят до
	 *       наибольшего из снятых: общий порог по наибольшему снял бы сторожа там,
	 *       где он работает, - ради ложной тревоги на одной системе перестал бы
	 *       ловиться откат на всех прочих
	 *
	 */
	#if defined(__GLIBCXX__)
		static constexpr double BUILD_ALLOCATIONS = 8.5;
	#else
		static constexpr double BUILD_ALLOCATIONS = 7.0;
	#endif
	static constexpr double LOOKUP_ALLOCATIONS = 0.0;
	static constexpr double SERIALIZE_ALLOCATIONS = 2.0;
	static constexpr double REPLACE_ALLOCATIONS = 0.0;

	/**
	 * @brief Названия полей, отыскиваемых сценарием поиска
	 *
	 * @details Названия чередуются по кругу, а не задано одно: с одним названием
	 *          оптимизатор вправе вычислить поиск однажды и умножить итог на число
	 *          повторений, и замер мерил бы пустой цикл. Регистр названий намеренно
	 *          не совпадает с хранимым - поиск обязан быть регистронезависимым
	 *
	 */
	static constexpr const char * LOOKUPS[2] = {"cache-control", "ACCEPT-LANGUAGE"};
	/**
	 * @brief Название поля, заменяемого сценарием замены
	 *
	 */
	static constexpr const char * REPLACE = "Content-Length";
	/**
	 * @brief Значения, подставляемые сценарием замены по кругу
	 *
	 * @details Значения разной длины: замена на строку той же длины измеряла бы
	 *          запись в готовый буфер, а не работу контейнера
	 *
	 */
	static constexpr const char * REPLACEMENTS[2] = {"4096", "131072"};

	/**
	 * @brief Структура итогов прогона сценария
	 *
	 */
	typedef struct Outcome {
		// Количество выполненных операций
		size_t operations;
		// Затраченное время в секундах
		double seconds;
		// Количество выполненных выделений памяти
		size_t allocations;
		// Суммарный объём выделенной памяти
		size_t bytes;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Outcome() noexcept : operations(0), seconds(0.0), allocations(0), bytes(0) {}
	} outcome_t;

	/**
	 * @brief Шаблон функции прогона одного сценария
	 *
	 * @tparam Scenario тип функции прогоняемого сценария
	 *
	 */
	template <typename Scenario>
	/**
	 * @brief Функция прогона одного сценария
	 *
	 * @param rounds   количество повторений замера
	 * @param counting признак учёта выделений памяти
	 * @param scenario прогоняемый сценарий
	 * @return         итоги прогона сценария
	 *
	 */
	static outcome_t measure(const size_t rounds, const bool counting, Scenario scenario) noexcept {
		// Итоги прогона сценария
		outcome_t result;
		/**
		 * Выполняем прогрев: первые повторения выходят на установившийся режим,
		 * и замер с холодного старта мерил бы разгон реализации, а не её скорость
		 */
		for(size_t i = 0; i < WARMUP; i++)
			// Выполняем очередное повторение прогрева
			checksum() += scenario(i);
		// Если измеряются выделения памяти - включаем учёт
		if(counting)
			// Включаем учёт выделений памяти
			awh::benchmark::counting(true);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем замер
		 */
		for(size_t i = 0; i < rounds; i++)
			// Выполняем очередное повторение замера
			checksum() += scenario(i);
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		// Если измеряются выделения памяти
		if(counting){
			// Отключаем учёт выделений памяти
			awh::benchmark::counting(false);
			// Получаем статистику выделений памяти
			awh::benchmark::allocations(result.allocations, result.bytes);
		}
		// Записываем количество выполненных операций
		result.operations = rounds;
		// Вычисляем затраченное время
		result.seconds = std::chrono::duration <double> (finish - start).count();
		// Выводим итоги прогона сценария
		return result;
	}
	/**
	 * @brief Функция формирования результата замера скорости
	 *
	 * @param outcome итоги прогона сценария
	 * @param subject название измеряемой сущности для сведений о прогоне
	 * @return        результат замера
	 *
	 */
	static awh::benchmark::result_t speed(const outcome_t & outcome, const char * subject) noexcept {
		// Результат замера
		awh::benchmark::result_t result;
		// Вычисляем количество операций в секунду
		result.value = ((outcome.seconds > 0.0) ? (static_cast <double> (outcome.operations) / outcome.seconds) : 0.0);
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details), "%s: %zu, время: %.3f с, на операцию: %.2f мкс",
			subject, outcome.operations, outcome.seconds,
			((outcome.operations > 0) ? ((outcome.seconds * 1e6) / static_cast <double> (outcome.operations)) : 0.0)
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат замера
		return result;
	}
	/**
	 * @brief Функция формирования результата замера выделений памяти
	 *
	 * @param outcome итоги прогона сценария
	 * @return        результат замера
	 *
	 */
	static awh::benchmark::result_t memory(const outcome_t & outcome) noexcept {
		// Результат замера
		awh::benchmark::result_t result;
		// Вычисляем количество выделений памяти на одну операцию
		result.value = ((outcome.operations > 0)
			? (static_cast <double> (outcome.allocations) / static_cast <double> (outcome.operations)) : 0.0);
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details), "операций: %zu, выделений: %zu, выделено: %.1f МБ",
			outcome.operations, outcome.allocations, (static_cast <double> (outcome.bytes) / 1048576.0)
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат замера
		return result;
	}

	/**
	 * @brief Функция прогона сценария наполнения контейнера полями запроса
	 *
	 * @details Наполнение выполняется на каждом исходящем сообщении и на каждом
	 *          принятом: именно этот путь проходит всякое поле, попадающее в обмен.
	 *          Контейнер заводится заново - так его и заводит отправляющая сторона
	 *          под очередное сообщение
	 *
	 * @param counting признак учёта выделений памяти
	 * @return         итоги прогона сценария
	 *
	 */
	static outcome_t building(const bool counting) noexcept {
		// Выполняем прогон измеряемой операции
		return measure(ROUNDS, counting, []([[maybe_unused]] const size_t index) noexcept -> uint64_t {
			// Создаём контейнер заголовков
			awh::http::headers_t object(awh::http::proto_t::HTTP1, framework(), logger());
			// Наполняем контейнер полями образца
			fill(object);
			// Выводим итог наполнения контейнера
			return static_cast <uint64_t> (object.size());
		});
	}
	/**
	 * @brief Функция прогона сценария поиска поля по названию
	 *
	 * @details Поиск выполняется на каждое обращение к полю принятого сообщения,
	 *          и обращений этих на сообщение приходится больше, чем самих полей:
	 *          длина тела, кодирование передачи, соединение, тип содержимого
	 *
	 * @param counting признак учёта выделений памяти
	 * @return         итоги прогона сценария
	 *
	 */
	static outcome_t lookup(const bool counting) noexcept {
		// Создаём контейнер заголовков сценария
		static awh::http::headers_t object(awh::http::proto_t::HTTP1, framework(), logger());
		// Наполняем контейнер полями образца, если он ещё не наполнен
		if(object.empty())
			// Наполняем контейнер полями образца
			fill(object);
		// Выполняем прогон измеряемой операции
		return measure(LIGHT_ROUNDS, counting, [](const size_t index) noexcept -> uint64_t {
			// Выводим длину найденного значения поля
			return static_cast <uint64_t> (object.at(LOOKUPS[index & 1]).size());
		});
	}
	/**
	 * @brief Функция прогона сценария сборки сообщения
	 *
	 * @details Сборка выполняется на каждом исходящем сообщении: именно её итог
	 *          уходит на провод у протокола HTTP/1
	 *
	 * @param counting признак учёта выделений памяти
	 * @return         итоги прогона сценария
	 *
	 */
	static outcome_t serializing(const bool counting) noexcept {
		// Создаём контейнер заголовков сценария
		static awh::http::headers_t object(awh::http::proto_t::HTTP1, framework(), logger());
		// Создаём объект запроса клиента
		static awh::http::request_t request(awh::http::version_t::HTTP1_1, awh::http::method_t::GET, std::string("/index.html"));
		// Наполняем контейнер полями образца, если он ещё не наполнен
		if(object.empty()){
			// Устанавливаем провайдер запроса
			object.provider(&request);
			// Наполняем контейнер полями образца
			fill(object);
		}
		// Выполняем прогон измеряемой операции
		return measure(ROUNDS, counting, []([[maybe_unused]] const size_t index) noexcept -> uint64_t {
			// Выводим длину собранного сообщения
			return static_cast <uint64_t> (object.print(awh::http::proto_t::HTTP1).size());
		});
	}
	/**
	 * @brief Функция прогона сценария замены значения существующего поля
	 *
	 * @details Замена выполняется отправляющей стороной на каждом сообщении: длина
	 *          тела, кодирование передачи и штамп времени проставляются поверх того,
	 *          что положило приложение. Работы в ней больше, чем в добавлении:
	 *          помещаемость проверяется до снятия прежних вхождений, чтобы
	 *          отвергнутая замена ничего не теряла
	 *
	 * @param counting признак учёта выделений памяти
	 * @return         итоги прогона сценария
	 *
	 */
	static outcome_t replacing(const bool counting) noexcept {
		// Создаём контейнер заголовков сценария
		static awh::http::headers_t object(awh::http::proto_t::HTTP1, framework(), logger());
		// Наполняем контейнер полями образца, если он ещё не наполнен
		if(object.empty())
			// Наполняем контейнер полями образца
			fill(object);
		// Выполняем прогон измеряемой операции
		return measure(LIGHT_ROUNDS, counting, [](const size_t index) noexcept -> uint64_t {
			// Выполняем замену значения поля очередным образцом
			return static_cast <uint64_t> (object.emplace(
				REPLACE, REPLACEMENTS[index & 1], awh::http::headers_t::mode_t::REPLACE
			));
		});
	}

	/**
	 * @brief Функция замера скорости наполнения контейнера
	 *
	 * @return результат замера
	 *
	 */
	static awh::benchmark::result_t speedBuild() noexcept {
		// Выводим результат замера скорости наполнения контейнера
		return speed(building(false), "наборов");
	}
	/**
	 * @brief Функция замера выделений памяти на наполнение контейнера
	 *
	 * @return результат замера
	 *
	 */
	static awh::benchmark::result_t memoryBuild() noexcept {
		// Выводим результат замера выделений памяти на наполнение контейнера
		return memory(building(true));
	}
	/**
	 * @brief Функция замера скорости поиска поля по названию
	 *
	 * @return результат замера
	 *
	 */
	static awh::benchmark::result_t speedLookup() noexcept {
		// Выводим результат замера скорости поиска поля
		return speed(lookup(false), "поисков");
	}
	/**
	 * @brief Функция замера выделений памяти на поиск поля
	 *
	 * @return результат замера
	 *
	 */
	static awh::benchmark::result_t memoryLookup() noexcept {
		// Выводим результат замера выделений памяти на поиск поля
		return memory(lookup(true));
	}
	/**
	 * @brief Функция замера скорости сборки сообщения
	 *
	 * @return результат замера
	 *
	 */
	static awh::benchmark::result_t speedSerialize() noexcept {
		// Выводим результат замера скорости сборки сообщения
		return speed(serializing(false), "сборок");
	}
	/**
	 * @brief Функция замера выделений памяти на сборку сообщения
	 *
	 * @return результат замера
	 *
	 */
	static awh::benchmark::result_t memorySerialize() noexcept {
		// Выводим результат замера выделений памяти на сборку сообщения
		return memory(serializing(true));
	}
	/**
	 * @brief Функция замера скорости замены значения поля
	 *
	 * @return результат замера
	 *
	 */
	static awh::benchmark::result_t speedReplace() noexcept {
		// Выводим результат замера скорости замены значения поля
		return speed(replacing(false), "замен");
	}
	/**
	 * @brief Функция замера выделений памяти на замену значения поля
	 *
	 * @return результат замера
	 *
	 */
	static awh::benchmark::result_t memoryReplace() noexcept {
		// Выводим результат замера выделений памяти на замену значения поля
		return memory(replacing(true));
	}

	// Регистрируем сценарий скорости наполнения контейнера
	static const bool gBuild = awh::benchmark::add(
		"proto/headers/build", "наборов/с", BUILD_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedBuild
	);
	// Регистрируем сценарий выделений памяти на наполнение контейнера
	static const bool gMemoryBuild = awh::benchmark::add(
		"proto/headers/build/allocations", "выделений", BUILD_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryBuild
	);
	// Регистрируем сценарий скорости поиска поля по названию
	static const bool gLookup = awh::benchmark::add(
		"proto/headers/lookup", "поисков/с", LOOKUP_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedLookup
	);
	// Регистрируем сценарий выделений памяти на поиск поля
	static const bool gMemoryLookup = awh::benchmark::add(
		"proto/headers/lookup/allocations", "выделений", LOOKUP_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryLookup
	);
	// Регистрируем сценарий скорости сборки сообщения
	static const bool gSerialize = awh::benchmark::add(
		"proto/headers/serialize", "сборок/с", SERIALIZE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedSerialize
	);
	// Регистрируем сценарий выделений памяти на сборку сообщения
	static const bool gMemorySerialize = awh::benchmark::add(
		"proto/headers/serialize/allocations", "выделений", SERIALIZE_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memorySerialize
	);
	// Регистрируем сценарий скорости замены значения поля
	static const bool gReplace = awh::benchmark::add(
		"proto/headers/replace", "замен/с", REPLACE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::speedReplace
	);
	// Регистрируем сценарий выделений памяти на замену значения поля
	static const bool gMemoryReplace = awh::benchmark::add(
		"proto/headers/replace/allocations", "выделений", REPLACE_ALLOCATIONS,
		awh::benchmark::bound_t::MAXIMUM, &::memoryReplace
	);
};
