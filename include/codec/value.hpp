/**
 * @file value.hpp
 * @date 2026-10-10
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
 * \~russian
 * @brief Заголовочный файл общего для всех текстовых кодеков рамки вида значения —
 *        перечень видов, под который текстовые кодеки обязаны подводить свои записи,
 *        и через который потребитель спрашивает вид значения у любого из девяти
 *
 * \~english
 * @brief Header file of the kind of a value common for all the text codecs of the framework —
 *        enumeration of the kinds under which the text codecs must bring their records,
 *        and through which the consumer asks a codec for the kind of the value held at a path
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Стандартный заголовочный файл
 */
#include <cstdint>

/**
 * \~russian
 * @brief Основное пространство имён
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Подключаем пространство имён стандартной библиотеки
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён кодеков
	 *
	 * \~english
	 * @brief Namespace of the codecs
	 *
	 * \~
	 */
	namespace codec {
		/**
		 * \~russian
		 * @brief Виды значения, общие у всех текстовых кодеков рамки
		 *
		 * @details Перечень видов заведён решением от 10.10.2026: текстовые кодеки
		 *          несут СВОИ системы видов (по одной в каждом из девяти — `toml::type_t`,
		 *          `cef::type_t`, `json::kind_t` и так далее), но ПОВЕРХ них потребителю
		 *          нужен общий, и прежде эту общность нес `awh::codec::abc::kind_t` — вид
		 *          значения бинарного контейнера ABC. Подмена эта была неверна: ABC лишь
		 *          одно из девяти текстовых представлений наряду с прочими, и тянуть
		 *          общий enum оттуда в toml, cef и другие — подмена зависимости на
		 *          зависимость, которой нет.
		 *
		 * @note Два перечня — `awh::codec::kind_t` (текстовое дерево) и
		 *       `awh::codec::abc::kind_t` (бинарный контейнер ABC) — намеренно
		 *       сосуществуют с одинаковыми значениями: контейнер ABC держит
		 *       свою систему видов под себя, текстовое дерево держит свою, и
		 *       сводить их в один тип — отдельная работа, которая здесь не идёт
		 *       и без слова владельца не пойдёт
		 *
		 * \~english
		 * @brief Kinds of a value, common for all the text codecs of the framework
		 *
		 * @details The set of kinds is introduced by the decision of 10.10.2026: the text codecs
		 *          carry their OWN systems of kinds (one in each of the nine — `toml::type_t`,
		 *          `cef::type_t`, `json::kind_t` and so on), but ON TOP of them the consumer
		 *          needs a common one, and before that this commonality was carried by `awh::codec::abc::kind_t` — the
		 *          kind of a value of the binary container ABC. That substitution was wrong: ABC is only
		 *          one of the nine text representations alongside the others, and pulling
		 *          the common enum from there into toml, cef and others is a substitution of a dependency by
		 *          a dependency that does not exist.
		 *
		 * @note The two enums — `awh::codec::kind_t` (text tree) and
		 *       `awh::codec::abc::kind_t` (binary container ABC) — intentionally
		 *       coexist with the same values: the container ABC holds
		 *       its own system of kinds for itself, the text tree holds its own, and
		 *       merging them into one type is a separate work that is not going here
		 *       and will not go without the word of the owner
		 *
		 * \~
		 */
		enum class kind_t : uint8_t {
			NONE   = 0x00, // Значение не определено либо путь недействителен
			NUL    = 0x01, // Пустое значение
			BOOL   = 0x02, // Логическое значение
			NUMBER = 0x03, // Число любого вида
			STRING = 0x04, // Последовательность знаков
			BLOB   = 0x05, // Двоичные данные
			TIME   = 0x06, // Отметка времени
			UUID   = 0x07, // Опознаватель
			ARRAY  = 0x08, // Массив
			MAP    = 0x09, // Отображение
			CUSTOM = 0x0A  // Открытое расширение, заведённое потребителем
		};
	};
};
