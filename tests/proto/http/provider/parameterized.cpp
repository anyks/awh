/**
 * @file: parameterized.cpp
 * @date: 2026-07-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Параметризованные тесты провайдеров HTTP-сообщений — прогон подготовленных наборов входных данных через
 *        методы модуля с проверкой формирования и сериализации структуры HTTP-запроса клиента и HTTP-ответа сервера
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <memory>
#include <utility>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "provider.hpp"

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Структура параметров теста объекта запроса клиента
 *
 */
struct RequestTestParameter {
	// Параметры URI-запроса
	std::string uri;
	// Метод запроса клиента
	method_t method;
	// Версия протокола запроса
	version_t version;
};

/**
 * @brief Класс фикстуры теста объекта запроса клиента
 *
 */
class RequestParameterizedFixture : public ProviderFixture, public ::testing::WithParamInterface <RequestTestParameter> {
	public:
		// Параметры теста объекта запроса клиента
		RequestTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования конструирования, копирования, клонирования и перемещения запроса
 *
 */
TEST_P(RequestParameterizedFixture, RequestLifecycleTest){
	// Создаём объект запроса клиента с параметрами теста
	request_t request(this->_parameter.version, this->_parameter.method, this->_parameter.uri);
	// Проверяем что направление трафика соответствует запросу
	ASSERT_EQ(request.direct, direct_t::REQUEST);
	// Проверяем что версия протокола установлена
	ASSERT_EQ(request.version, this->_parameter.version);
	// Проверяем что метод запроса установлен
	ASSERT_EQ(request.method, this->_parameter.method);
	// Проверяем что URI-адрес установлен
	ASSERT_EQ(request.uri, this->_parameter.uri);
	// Проверяем копирование через конструктор копирования
	request_t copy(request);
	// Проверяем что копия равна исходному объекту
	ASSERT_TRUE(copy == request);
	// Клонируем объект запроса через базовый интерфейс провайдера
	std::unique_ptr <provider_t> cloned = request.clone();
	// Проверяем что клон создан
	ASSERT_TRUE(cloned != nullptr);
	// Проверяем что направление трафика клона соответствует запросу
	ASSERT_EQ(cloned->direct, direct_t::REQUEST);
	// Безопасно приводим клон к типу запроса (тип подтверждён флагом direct)
	const request_t * clonedPtr = static_cast <const request_t *> (cloned.get());
	// Проверяем что клон равен исходному объекту без срезки производной части
	ASSERT_TRUE((* clonedPtr) == request);
	// Перемещаем копию через конструктор перемещения
	request_t moved(std::move(copy));
	// Проверяем что перемещённый объект содержит корректные данные
	ASSERT_EQ(moved.uri, this->_parameter.uri);
	// Проверяем что метод перемещённого объекта корректен
	ASSERT_EQ(moved.method, this->_parameter.method);
	// Проверяем что исходный объект сброшен на значения по умолчанию
	ASSERT_EQ(copy.method, method_t::NONE);
}

/**
 * @brief Инициализация параметров теста объекта запроса клиента
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, RequestParameterizedFixture,
	::testing::Values(
		RequestTestParameter({"*", method_t::OPTIONS, version_t::HTTP1_1}),
		RequestTestParameter({"/upload", method_t::PUT, version_t::HTTP1_0}),
		RequestTestParameter({"/submit", method_t::POST, version_t::HTTP1_1}),
		RequestTestParameter({"/item/42", method_t::DELETE, version_t::HTTP2}),
		RequestTestParameter({"/index.html", method_t::GET, version_t::HTTP1_1}),
		RequestTestParameter({"example.com:443", method_t::CONNECT, version_t::HTTP2})
	)
);

/**
 * @brief Структура параметров теста объекта ответа сервера
 *
 */
struct ResponseTestParameter {
	// Код ответа сервера
	uint16_t code;
	// Версия протокола ответа
	version_t version;
	// Сообщение сервера
	std::string message;
};

/**
 * @brief Класс фикстуры теста объекта ответа сервера
 *
 */
class ResponseParameterizedFixture : public ProviderFixture, public ::testing::WithParamInterface <ResponseTestParameter> {
	public:
		// Параметры теста объекта ответа сервера
		ResponseTestParameter _parameter = GetParam();
};

/**
 * @brief Метод тестирования конструирования, копирования, клонирования и перемещения ответа
 *
 */
TEST_P(ResponseParameterizedFixture, ResponseLifecycleTest){
	// Создаём объект ответа сервера с параметрами теста
	response_t response(this->_parameter.version, this->_parameter.code, this->_parameter.message);
	// Проверяем что направление трафика соответствует ответу
	ASSERT_EQ(response.direct, direct_t::RESPONSE);
	// Проверяем что версия протокола установлена
	ASSERT_EQ(response.version, this->_parameter.version);
	// Проверяем что код ответа установлен
	ASSERT_EQ(response.code, this->_parameter.code);
	// Проверяем что сообщение сервера установлено
	ASSERT_EQ(response.message, this->_parameter.message);
	// Проверяем копирование через конструктор копирования
	response_t copy(response);
	// Проверяем что копия равна исходному объекту
	ASSERT_TRUE(copy == response);
	// Клонируем объект ответа через базовый интерфейс провайдера
	std::unique_ptr <provider_t> cloned = response.clone();
	// Проверяем что клон создан
	ASSERT_TRUE(cloned != nullptr);
	// Проверяем что направление трафика клона соответствует ответу
	ASSERT_EQ(cloned->direct, direct_t::RESPONSE);
	// Безопасно приводим клон к типу ответа (тип подтверждён флагом direct)
	const response_t * clonedPtr = static_cast <const response_t *> (cloned.get());
	// Проверяем что клон равен исходному объекту без срезки производной части
	ASSERT_TRUE((* clonedPtr) == response);
	// Перемещаем копию через конструктор перемещения
	response_t moved(std::move(copy));
	// Проверяем что перемещённый объект содержит корректные данные
	ASSERT_EQ(moved.code, this->_parameter.code);
	// Проверяем что сообщение перемещённого объекта корректно
	ASSERT_EQ(moved.message, this->_parameter.message);
	// Проверяем что код исходного объекта сброшен на значение по умолчанию
	ASSERT_EQ(copy.code, 0u);
}

/**
 * @brief Инициализация параметров теста объекта ответа сервера
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, ResponseParameterizedFixture,
	::testing::Values(
		ResponseTestParameter({200, version_t::HTTP1_1, "OK"}),
		ResponseTestParameter({404, version_t::HTTP1_1, "Not Found"}),
		ResponseTestParameter({301, version_t::HTTP1_0, "Moved Permanently"}),
		ResponseTestParameter({500, version_t::HTTP2, "Internal Server Error"}),
		ResponseTestParameter({204, version_t::HTTP1_1, "No Content"})
	)
);
