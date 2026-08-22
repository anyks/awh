/**
 * @file static.cpp
 * @date 2026-08-05
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
 * @brief Статические тесты защиты от макросов MS Windows — проверка того, что открытые заголовки
 *        собираются при действующих макросах системы и что макросы эти возвращаются потребителю
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Для операционной системы MS Windows
 *
 * @details Заголовки MS Windows подключаются здесь **первыми**, до всех заголовков AWH,
 *          и намеренно без выключателей WIN32_LEAN_AND_MEAN и NOGDI. Порядок этот
 *          воспроизводит худший случай со стороны потребителя библиотеки: макросы
 *          DELETE, ERROR, STRICT и прочие уже действуют, когда разбираются перечисления
 *          AWH, и защита push_macro/pop_macro в заголовках обязана с этим справиться
 *
 * @note Проверка здесь по существу на этапе сборки: развались защита — до запуска
 *       тестов дело не дойдёт вовсе, сборка отвечает отказом. Тела же тестов ниже
 *       закрепляют вторую половину — что макросы возвращены потребителю
 *
 */
#if _WIN32 || _WIN64
	#include <windows.h>
	#include <winsock2.h>
#endif

/**
 * Подключаем заголовочный файлы проекта
 */
#include "macro.hpp"

/**
 * Подключаем открытые заголовочные файлы, объявляющие или применяющие имена,
 * занятые макросами MS Windows
 */
#include "../../../include/net/net.hpp"
#include "../../../include/net/uri.hpp"
#include "../../../include/net/event.hpp"
#include "../../../include/unit/cluster.hpp"
#include "../../../include/unit/server.hpp"
#include "../../../include/unit/client.hpp"
#include "../../../include/proto/quic/quic.hpp"
#include "../../../include/proto/portmap/upnp.hpp"
#include "../../../include/proto/http/auth/auth.hpp"
#include "../../../include/proto/http/parser/parser.hpp"
#include "../../../include/proto/http/parser/http2/h2.hpp"
#include "../../../include/proto/http/parser/http3/h3.hpp"
#include "../../../include/codec/xml/reader.hpp"
#include "../../../include/codec/xml/document.hpp"
#include "../../../include/encoding/charset/charset.hpp"
#include "../../../include/regex/common.hpp"
#include "../../../include/regex/grok/common.hpp"
#include "../../../include/encoding/idna/types.hpp"

/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * Закрепляем возврат макросов потребителю библиотеки
	 *
	 * @details Заголовки AWH снимают эти имена лишь на время своих объявлений и обязаны
	 *          вернуть их в прежнем виде. Пропади возврат — потребитель лишился бы имён,
	 *          какими вправе пользоваться, и обнаружилось бы это у него, а не у нас
	 *
	 * @note Проверка ведётся препроцессором, а не телом теста: к моменту работы тела
	 *       макросы уже раскрыты, и отсутствие их телом не выявить
	 *
	 */
	#ifndef DELETE
		#error "Макрос DELETE не возвращён потребителю заголовками AWH"
	#endif
	#ifndef ERROR
		#error "Макрос ERROR не возвращён потребителю заголовками AWH"
	#endif
	#ifndef STRICT
		#error "Макрос STRICT не возвращён потребителю заголовками AWH"
	#endif
	#ifndef NO_ERROR
		#error "Макрос NO_ERROR не возвращён потребителю заголовками AWH"
	#endif
	#ifndef ALTERNATE
		#error "Макрос ALTERNATE не возвращён потребителю заголовками AWH"
	#endif
	#ifndef TRANSPARENT
		#error "Макрос TRANSPARENT не возвращён потребителю заголовками AWH"
	#endif
	#ifndef INVALID_SOCKET
		#error "Макрос INVALID_SOCKET не возвращён потребителю заголовками AWH"
	#endif
	#ifndef TEXT
		#error "Макрос TEXT не возвращён потребителю заголовками AWH"
	#endif
	#ifndef FAILED
		#error "Макрос FAILED не возвращён потребителю заголовками AWH"
	#endif
#endif

/**
 * @brief Метод проверки сборки открытых заголовков при действующих макросах MS Windows
 *
 */
TEST_F(MacroFixture, HeadersCompileUnderWindowsMacrosTest){
	/**
	 * Сам факт сборки этой единицы трансляции и означает успех: заголовки MS Windows
	 * подключены выше первыми, и защита push_macro/pop_macro выдержала
	 */
	SUCCEED();
}

/**
 * Снимаем макросы на время теста, именующего члены перечислений
 *
 * @details Показательная часть набора. Заголовки AWH возвращают макросы потребителю, и
 *          потому запись вида `action_t::DELETE` в коде, где заголовки MS Windows
 *          действуют, упирается в макрос — тот заменяет имя ещё до разбора. Обойти это
 *          иначе как временным снятием нельзя, разве что переименованием членов, на
 *          какое библиотека не идёт
 *
 *          Потребителю библиотеки поступать следует ровно так же, и набор этот
 *          заодно служит тому образцом
 *
 */
#include "../../../include/sys/macro_push.hpp"

/**
 * @brief Метод проверки доступности членов перечислений, чьи имена заняты макросами
 *
 */
TEST_F(MacroFixture, EnumMembersRemainAddressableTest){
	// Проверяем член перечисления, сталкивающийся с макросом DELETE
	ASSERT_EQ(static_cast <uint8_t> (awh::event::action_t::DELETE), 0x05);
	// Проверяем член перечисления, сталкивающийся с макросом INVALID_SOCKET
	ASSERT_EQ(static_cast <uint8_t> (awh::event::error_t::INVALID_SOCKET), 0x08);
	// Проверяем член перечисления, сталкивающийся с макросом STRICT
	ASSERT_EQ(static_cast <uint8_t> (awh::event::mtu_discover_t::STRICT), 0x06);
}

/**
 * Возвращаем макросы, снятые перед тестом выше
 */
#include "../../../include/sys/macro_pop.hpp"

/**
 * @brief Метод проверки совпадения типа сокета AWH с системным
 *
 */
TEST_F(MacroFixture, SocketTypeMatchesSystemTest){
	/**
	 * Для операционной системы MS Windows
	 *
	 * @note Тип awh::net::socket_t выписан в открытом заголовке своими словами, чтобы тот
	 *       не тянул за собой заголовки MS Windows. Проверка следит, чтобы написание
	 *       не разошлось с системным SOCKET
	 *
	 */
	#if _WIN32 || _WIN64
		// Проверяем совпадение размера типа сокета с системным
		ASSERT_EQ(sizeof(awh::net::socket_t), sizeof(SOCKET));
		// Проверяем совпадение самого типа сокета с системным
		ASSERT_TRUE((std::is_same <awh::net::socket_t, SOCKET>::value));
		// Проверяем совпадение значения негодного сокета с системным
		ASSERT_EQ(awh::net::invalid_socket_t, static_cast <awh::net::socket_t> (static_cast <SOCKET> (INVALID_SOCKET)));
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		// Проверяем значение негодного сокета
		ASSERT_EQ(awh::net::invalid_socket_t, -1);
	#endif
}
