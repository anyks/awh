/**
 * @file: natpmp.cpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Проверка кодека договора NAT-PMP — сборка запросов, разбор ответов маршрутизатора,
 *        отклонение испорченных сообщений и расчёт срока ожидания повторных попыток
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <proto/portmap/natpmp.hpp>

/**
 * Подключаем заголовочные файлы тестового окружения
 */
#include "../../main.hpp"

/**
 * Используем пространство имён договоров перенаправления портов
 */
using namespace awh::proto::portmap;

/**
 * @brief Объект фреймворка тестового окружения
 *
 */
static const awh::fmk_t __fmk;

/**
 * @brief Объект работы с логами тестового окружения
 *
 */
static const awh::log_t __log(&__fmk);

/**
 * @brief Кодек договора NAT-PMP тестового окружения
 *
 */
static const natpmp_t __natpmp(&__fmk, &__log);

/**
 * @brief Проверка сборки запроса внешнего адреса маршрутизатора
 *
 * @details Запрос внешнего адреса договором задан двумя октетами: изданием договора
 *          и кодом действия. Проверка сличает собранное сообщение с образцом из RFC 6886
 *
 */
TEST(ProtoPortmapNatPmp, RequestAddress) {
	// Место под собираемое сообщение
	uint8_t buffer[natpmp_t::MAX_MESSAGE_SIZE] = {0};
	// Код причины отказа кодека
	natpmp_t::error_t error = natpmp_t::error_t::NONE;
	// Выполняем сборку запроса внешнего адреса маршрутизатора
	ASSERT_EQ(__natpmp.address(buffer, sizeof(buffer), error), static_cast <size_t> (2));
	// Выполняем проверку кода причины отказа кодека
	ASSERT_EQ(error, natpmp_t::error_t::NONE);
	// Выполняем проверку издания договора
	ASSERT_EQ(buffer[0], 0);
	// Выполняем проверку кода действия запроса
	ASSERT_EQ(buffer[1], 0);
	/**
	 * Выполняем проверку отклонения нехватки отведённого места
	 */
	{
		// Место под собираемое сообщение
		uint8_t small[1] = {0};
		// Выполняем сборку запроса в недостаточное место
		ASSERT_EQ(__natpmp.address(small, sizeof(small), error), static_cast <size_t> (0));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, natpmp_t::error_t::BUFFER_TOO_SMALL);
	}
}
/**
 * @brief Проверка сборки просьбы о перенаправлении порта
 *
 */
TEST(ProtoPortmapNatPmp, RequestMapping) {
	// Место под собираемое сообщение
	uint8_t buffer[natpmp_t::MAX_MESSAGE_SIZE] = {0};
	// Код причины отказа кодека
	natpmp_t::error_t error = natpmp_t::error_t::NONE;
	// Параметры просьбы о перенаправлении порта
	natpmp_t::request_t request;
	// Устанавливаем договор перенаправления порта
	request.proto = natpmp_t::proto_t::TCP;
	// Устанавливаем внутренний порт перенаправления
	request.internalPort = 8080;
	// Устанавливаем желаемый внешний порт перенаправления
	request.externalPort = 8080;
	// Устанавливаем запрашиваемый срок жизни перенаправления
	request.lifeTime = 3600;
	// Выполняем сборку просьбы о перенаправлении порта
	ASSERT_EQ(__natpmp.mapping(buffer, sizeof(buffer), request, error), static_cast <size_t> (12));
	// Выполняем проверку кода причины отказа кодека
	ASSERT_EQ(error, natpmp_t::error_t::NONE);
	// Выполняем проверку издания договора
	ASSERT_EQ(buffer[0], 0);
	// Выполняем проверку кода действия, отвечающего договору TCP
	ASSERT_EQ(buffer[1], 2);
	// Выполняем проверку отведённого договором пустого поля
	ASSERT_EQ(buffer[2], 0);
	// Выполняем проверку отведённого договором пустого поля
	ASSERT_EQ(buffer[3], 0);
	// Выполняем проверку внутреннего порта перенаправления
	ASSERT_EQ(buffer[4], 0x1F);
	// Выполняем проверку внутреннего порта перенаправления
	ASSERT_EQ(buffer[5], 0x90);
	// Выполняем проверку желаемого внешнего порта перенаправления
	ASSERT_EQ(buffer[6], 0x1F);
	// Выполняем проверку желаемого внешнего порта перенаправления
	ASSERT_EQ(buffer[7], 0x90);
	// Выполняем проверку срока жизни перенаправления
	ASSERT_EQ(buffer[8], 0);
	// Выполняем проверку срока жизни перенаправления
	ASSERT_EQ(buffer[9], 0);
	// Выполняем проверку срока жизни перенаправления
	ASSERT_EQ(buffer[10], 0x0E);
	// Выполняем проверку срока жизни перенаправления
	ASSERT_EQ(buffer[11], 0x10);
	/**
	 * Выполняем проверку сборки просьбы об удалении перенаправления
	 *
	 * @note Отдельного действия для удаления договор не имеет: перенаправление
	 *       убирается той же просьбой с нулевым сроком жизни
	 */
	{
		// Устанавливаем договор перенаправления порта
		request.proto = natpmp_t::proto_t::UDP;
		// Обнуляем желаемый внешний порт перенаправления
		request.externalPort = 0;
		// Обнуляем запрашиваемый срок жизни перенаправления
		request.lifeTime = 0;
		// Выполняем сборку просьбы об удалении перенаправления
		ASSERT_EQ(__natpmp.mapping(buffer, sizeof(buffer), request, error), static_cast <size_t> (12));
		// Выполняем проверку кода действия, отвечающего договору UDP
		ASSERT_EQ(buffer[1], 1);
		// Выполняем проверку обнулённого внешнего порта перенаправления
		ASSERT_EQ(buffer[6], 0);
		// Выполняем проверку обнулённого внешнего порта перенаправления
		ASSERT_EQ(buffer[7], 0);
		// Выполняем проверку обнулённого срока жизни перенаправления
		ASSERT_EQ(buffer[11], 0);
	}
	/**
	 * Выполняем проверку отклонения неопределённого договора перенаправления
	 */
	{
		// Сбрасываем договор перенаправления порта
		request.proto = natpmp_t::proto_t::NONE;
		// Выполняем сборку просьбы с неопределённым договором
		ASSERT_EQ(__natpmp.mapping(buffer, sizeof(buffer), request, error), static_cast <size_t> (0));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, natpmp_t::error_t::INVALID_OPCODE);
	}
}
/**
 * @brief Проверка разбора ответа с внешним адресом маршрутизатора
 *
 */
TEST(ProtoPortmapNatPmp, ResponseAddress) {
	// Ответ маршрутизатора с внешним адресом 203.0.113.7
	const uint8_t data[12] = {
		0x00, 0x80, 0x00, 0x00,
		0x00, 0x00, 0x12, 0x34,
		0xCB, 0x00, 0x71, 0x07
	};
	// Разобранный ответ маршрутизатора
	natpmp_t::answer_t answer;
	// Код причины отказа кодека
	natpmp_t::error_t error = natpmp_t::error_t::NONE;
	// Выполняем разбор ответа маршрутизатора
	ASSERT_TRUE(__natpmp.parse(data, sizeof(data), answer, error)) << message(error);
	// Выполняем проверку вида полученного сообщения
	ASSERT_EQ(answer.kind, natpmp_t::kind_t::ADDRESS);
	// Выполняем проверку кода итога, выданного маршрутизатором
	ASSERT_EQ(answer.result, natpmp_t::result_t::SUCCESS);
	// Выполняем проверку времени работы маршрутизатора
	ASSERT_EQ(answer.epoch, static_cast <uint32_t> (0x1234));
	// Выполняем проверку внешнего адреса маршрутизатора
	ASSERT_EQ(answer.address, static_cast <uint32_t> (0xCB007107));
}
/**
 * @brief Проверка разбора ответа о перенаправлении порта
 *
 */
TEST(ProtoPortmapNatPmp, ResponseMapping) {
	// Ответ маршрутизатора о перенаправлении порта TCP
	const uint8_t data[16] = {
		0x00, 0x82, 0x00, 0x00,
		0x00, 0x00, 0x12, 0x34,
		0x1F, 0x90, 0x30, 0x39,
		0x00, 0x00, 0x0E, 0x10
	};
	// Разобранный ответ маршрутизатора
	natpmp_t::answer_t answer;
	// Код причины отказа кодека
	natpmp_t::error_t error = natpmp_t::error_t::NONE;
	// Выполняем разбор ответа маршрутизатора
	ASSERT_TRUE(__natpmp.parse(data, sizeof(data), answer, error)) << message(error);
	// Выполняем проверку вида полученного сообщения
	ASSERT_EQ(answer.kind, natpmp_t::kind_t::MAPPING);
	// Выполняем проверку договора перенаправления порта
	ASSERT_EQ(answer.proto, natpmp_t::proto_t::TCP);
	// Выполняем проверку кода итога, выданного маршрутизатором
	ASSERT_EQ(answer.result, natpmp_t::result_t::SUCCESS);
	// Выполняем проверку внутреннего порта перенаправления
	ASSERT_EQ(answer.internalPort, 8080);
	/**
	 * Выполняем проверку внешнего порта, назначенного маршрутизатором
	 *
	 * @note Назначенный порт с запрошенным не совпадает намеренно: запрошенный
	 *       порт для маршрутизатора лишь пожелание, и объявлять другим следует
	 *       именно назначенный
	 */
	ASSERT_EQ(answer.externalPort, 12345);
	// Выполняем проверку срока жизни перенаправления
	ASSERT_EQ(answer.lifeTime, static_cast <uint32_t> (3600));
}
/**
 * @brief Проверка разбора отказа маршрутизатора
 *
 * @details Отказ маршрутизатора сообщением построен верно, и разбор его обязан
 *          удаваться: отличать испорченное сообщение от осмысленного отказа
 *          необходимо, иначе отказ настройки не отличить от помехи в сети
 *
 */
TEST(ProtoPortmapNatPmp, ResponseRefused) {
	// Отказ маршрутизатора выполнить просьбу
	const uint8_t data[16] = {
		0x00, 0x82, 0x00, 0x02,
		0x00, 0x00, 0x12, 0x34,
		0x1F, 0x90, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	// Разобранный ответ маршрутизатора
	natpmp_t::answer_t answer;
	// Код причины отказа кодека
	natpmp_t::error_t error = natpmp_t::error_t::NONE;
	// Выполняем разбор ответа маршрутизатора
	ASSERT_TRUE(__natpmp.parse(data, sizeof(data), answer, error)) << message(error);
	// Выполняем проверку отсутствия отказа кодека
	ASSERT_EQ(error, natpmp_t::error_t::NONE);
	// Выполняем проверку кода итога, выданного маршрутизатором
	ASSERT_EQ(answer.result, natpmp_t::result_t::NOT_AUTHORIZED);
	/**
	 * Выполняем проверку сохранения кода итога, кодеку неизвестного
	 *
	 * @note Неизвестный код подменяться известным не должен: договор оставляет
	 *       место под новые коды, и подмена скрыла бы настоящую причину отказа
	 */
	{
		// Отказ маршрутизатора с кодом итога, кодеку неизвестным
		uint8_t unknown[16] = {0};
		// Выполняем копирование ответа маршрутизатора
		for(size_t i = 0; i < sizeof(unknown); i++)
			// Выполняем копирование очередного октета ответа
			unknown[i] = data[i];
		// Устанавливаем код итога, кодеку неизвестный
		unknown[3] = 0x63;
		// Выполняем разбор ответа маршрутизатора
		ASSERT_TRUE(__natpmp.parse(unknown, sizeof(unknown), answer, error)) << message(error);
		// Выполняем проверку сохранения кода итога, кодеку неизвестного
		ASSERT_EQ(static_cast <uint16_t> (answer.result), static_cast <uint16_t> (0x63));
	}
}
/**
 * @brief Проверка отклонения испорченных сообщений
 *
 */
TEST(ProtoPortmapNatPmp, Malformed) {
	// Разобранный ответ маршрутизатора
	natpmp_t::answer_t answer;
	// Код причины отказа кодека
	natpmp_t::error_t error = natpmp_t::error_t::NONE;
	/**
	 * Выполняем проверку отклонения сообщения короче заголовка
	 */
	{
		// Сообщение короче заголовка ответа
		const uint8_t data[3] = {0x00, 0x80, 0x00};
		// Выполняем разбор сообщения короче заголовка
		ASSERT_FALSE(__natpmp.parse(data, sizeof(data), answer, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, natpmp_t::error_t::TRUNCATED);
	}
	/**
	 * Выполняем проверку отклонения ответа с внешним адресом короче положенного
	 */
	{
		// Ответ с внешним адресом короче положенного
		const uint8_t data[8] = {0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34};
		// Выполняем разбор ответа короче положенного
		ASSERT_FALSE(__natpmp.parse(data, sizeof(data), answer, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, natpmp_t::error_t::TRUNCATED);
	}
	/**
	 * Выполняем проверку отклонения ответа о перенаправлении короче положенного
	 */
	{
		// Ответ о перенаправлении короче положенного
		const uint8_t data[12] = {0x00, 0x82, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x1F, 0x90, 0x30, 0x39};
		// Выполняем разбор ответа короче положенного
		ASSERT_FALSE(__natpmp.parse(data, sizeof(data), answer, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, natpmp_t::error_t::TRUNCATED);
	}
	/**
	 * Выполняем проверку отклонения неизвестного издания договора
	 */
	{
		// Ответ с неизвестным изданием договора
		const uint8_t data[12] = {0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0xCB, 0x00, 0x71, 0x07};
		// Выполняем разбор ответа с неизвестным изданием договора
		ASSERT_FALSE(__natpmp.parse(data, sizeof(data), answer, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, natpmp_t::error_t::INVALID_VERSION);
	}
	/**
	 * Выполняем проверку отклонения сообщения, ответом не являющегося
	 *
	 * @note На порт ответов приходят и чужие запросы, разосланные по сети:
	 *       принимать их за ответы недопустимо
	 */
	{
		// Запрос, ответом не являющийся
		const uint8_t data[12] = {0x00, 0x02, 0x00, 0x00, 0x1F, 0x90, 0x1F, 0x90, 0x00, 0x00, 0x0E, 0x10};
		// Выполняем разбор сообщения, ответом не являющегося
		ASSERT_FALSE(__natpmp.parse(data, sizeof(data), answer, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, natpmp_t::error_t::NOT_A_RESPONSE);
	}
	/**
	 * Выполняем проверку отклонения неизвестного кода действия
	 */
	{
		// Ответ с неизвестным кодом действия
		const uint8_t data[16] = {0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x1F, 0x90, 0x30, 0x39, 0x00, 0x00, 0x0E, 0x10};
		// Выполняем разбор ответа с неизвестным кодом действия
		ASSERT_FALSE(__natpmp.parse(data, sizeof(data), answer, error));
		// Выполняем проверку кода причины отказа кодека
		ASSERT_EQ(error, natpmp_t::error_t::INVALID_OPCODE);
	}
}
/**
 * @brief Проверка расчёта срока ожидания повторных попыток
 *
 * @details Договор велит удваивать срок ожидания с каждой попыткой, начиная с
 *          четверти секунды: девять попыток укладываются примерно в минуту
 *
 */
TEST(ProtoPortmapNatPmp, Timeout) {
	// Выполняем проверку срока ожидания первой попытки
	ASSERT_EQ(natpmp_t::timeout(0), static_cast <uint32_t> (250));
	// Выполняем проверку срока ожидания второй попытки
	ASSERT_EQ(natpmp_t::timeout(1), static_cast <uint32_t> (500));
	// Выполняем проверку срока ожидания третьей попытки
	ASSERT_EQ(natpmp_t::timeout(2), static_cast <uint32_t> (1000));
	// Выполняем проверку срока ожидания последней попытки
	ASSERT_EQ(natpmp_t::timeout(natpmp_t::MAX_ATTEMPTS - 1), static_cast <uint32_t> (64000));
	/**
	 * Выполняем проверку ограничения срока ожидания
	 *
	 * @note Без ограничения сдвиг вышел бы за разрядность числа, а срок
	 *       ожидания стал бы бессмысленным
	 */
	ASSERT_EQ(natpmp_t::timeout(200), natpmp_t::timeout(natpmp_t::MAX_ATTEMPTS - 1));
}
