/**
 * @file: static.cpp
 * @date: 2026-07-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
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
 * @brief Метод проверки всех конструкторов объекта запроса клиента
 *
 */
TEST_F(ProviderFixture, RequestConstructorsTest){
	// Проверяем конструктор по умолчанию
	request_t request1;
	// Проверяем направление трафика по умолчанию
	ASSERT_EQ(request1.direct, direct_t::REQUEST);
	// Проверяем метод запроса по умолчанию
	ASSERT_EQ(request1.method, method_t::NONE);
	// Проверяем версию протокола по умолчанию
	ASSERT_EQ(request1.version, version_t::HTTP1_1);
	// Проверяем URI-адрес по умолчанию
	ASSERT_TRUE(request1.uri.empty());
	// Проверяем конструктор с указанием метода запроса
	request_t request2(method_t::POST);
	// Проверяем что метод запроса установлен
	ASSERT_EQ(request2.method, method_t::POST);
	// Проверяем конструктор с указанием версии протокола
	request_t request3(version_t::HTTP2);
	// Проверяем что версия протокола установлена
	ASSERT_EQ(request3.version, version_t::HTTP2);
	// Проверяем конструктор с указанием URI-адреса
	request_t request4(std::string("/index.html"));
	// Проверяем что URI-адрес установлен
	ASSERT_EQ(request4.uri, "/index.html");
	// Проверяем конструктор с указанием метода запроса и URI-адреса
	request_t request5(method_t::GET, "/search");
	// Проверяем что метод запроса установлен
	ASSERT_EQ(request5.method, method_t::GET);
	// Проверяем что URI-адрес установлен
	ASSERT_EQ(request5.uri, "/search");
	// Проверяем конструктор с указанием версии протокола и URI-адреса
	request_t request6(version_t::HTTP1_1, "/only-uri");
	// Проверяем что URI-адрес установлен
	ASSERT_EQ(request6.uri, "/only-uri");
	// Проверяем конструктор с указанием версии протокола и метода запроса
	request_t request7(version_t::HTTP2, method_t::HEAD);
	// Проверяем что метод запроса установлен
	ASSERT_EQ(request7.method, method_t::HEAD);
	// Проверяем что версия протокола установлена
	ASSERT_EQ(request7.version, version_t::HTTP2);
	// Проверяем конструктор с указанием версии протокола, метода запроса и URI-адреса
	request_t request8(version_t::HTTP1_1, method_t::PUT, "/upload");
	// Проверяем что версия протокола установлена
	ASSERT_EQ(request8.version, version_t::HTTP1_1);
	// Проверяем что метод запроса установлен
	ASSERT_EQ(request8.method, method_t::PUT);
	// Проверяем что URI-адрес установлен
	ASSERT_EQ(request8.uri, "/upload");
}

/**
 * @brief Метод проверки всех конструкторов объекта ответа сервера
 *
 */
TEST_F(ProviderFixture, ResponseConstructorsTest){
	// Проверяем конструктор по умолчанию
	response_t response1;
	// Проверяем направление трафика по умолчанию
	ASSERT_EQ(response1.direct, direct_t::RESPONSE);
	// Проверяем код ответа по умолчанию
	ASSERT_EQ(response1.code, 0u);
	// Проверяем версию протокола по умолчанию
	ASSERT_EQ(response1.version, version_t::HTTP1_1);
	// Проверяем что сообщение сервера по умолчанию пустое
	ASSERT_TRUE(response1.message.empty());
	// Проверяем конструктор с указанием кода ответа
	response_t response2(static_cast <uint16_t> (200));
	// Проверяем что код ответа установлен
	ASSERT_EQ(response2.code, 200u);
	// Проверяем конструктор с указанием версии протокола
	response_t response3(version_t::HTTP2);
	// Проверяем что версия протокола установлена
	ASSERT_EQ(response3.version, version_t::HTTP2);
	// Проверяем конструктор с указанием сообщения сервера
	response_t response4(std::string("Custom"));
	// Проверяем что сообщение сервера установлено
	ASSERT_EQ(response4.message, "Custom");
	// Проверяем конструктор с указанием версии протокола и кода ответа
	response_t response5(version_t::HTTP2, static_cast <uint16_t> (204));
	// Проверяем что версия протокола установлена
	ASSERT_EQ(response5.version, version_t::HTTP2);
	// Проверяем что код ответа установлен
	ASSERT_EQ(response5.code, 204u);
	// Проверяем конструктор с указанием кода ответа и сообщения сервера
	response_t response6(static_cast <uint16_t> (404), "Not Found");
	// Проверяем что код ответа установлен
	ASSERT_EQ(response6.code, 404u);
	// Проверяем что сообщение сервера установлено
	ASSERT_EQ(response6.message, "Not Found");
	// Проверяем конструктор с указанием версии протокола и сообщения сервера
	response_t response7(version_t::HTTP1_1, std::string("Only Message"));
	// Проверяем что сообщение сервера установлено
	ASSERT_EQ(response7.message, "Only Message");
	// Проверяем конструктор с указанием версии протокола, кода ответа и сообщения сервера
	response_t response8(version_t::HTTP1_1, static_cast <uint16_t> (500), "Internal Server Error");
	// Проверяем что версия протокола установлена
	ASSERT_EQ(response8.version, version_t::HTTP1_1);
	// Проверяем что код ответа установлен
	ASSERT_EQ(response8.code, 500u);
	// Проверяем что сообщение сервера установлено
	ASSERT_EQ(response8.message, "Internal Server Error");
}

/**
 * @brief Метод проверки копирования и перемещения объекта запроса клиента
 *
 */
TEST_F(ProviderFixture, RequestCopyMoveTest){
	// Создаём исходный объект запроса клиента
	request_t source(version_t::HTTP1_1, method_t::POST, "/submit");
	// Проверяем конструктор копирования
	request_t copy(source);
	// Проверяем что скопированный объект равен исходному
	ASSERT_TRUE(copy == source);
	// Проверяем оператор копирования
	request_t assigned;
	// Выполняем присваивание копированием
	assigned = source;
	// Проверяем что присвоенный объект равен исходному
	ASSERT_TRUE(assigned == source);
	// Проверяем конструктор перемещения
	request_t moved(std::move(copy));
	// Проверяем что перемещённый объект содержит корректные данные
	ASSERT_EQ(moved.method, method_t::POST);
	// Проверяем что URI-адрес перемещён корректно
	ASSERT_EQ(moved.uri, "/submit");
	// Проверяем что исходный объект сброшен на значения по умолчанию
	ASSERT_EQ(copy.method, method_t::NONE);
	// Проверяем оператор перемещения
	request_t movedAssigned;
	// Выполняем присваивание перемещением
	movedAssigned = std::move(assigned);
	// Проверяем что перемещённый объект содержит корректные данные
	ASSERT_EQ(movedAssigned.uri, "/submit");
	// Проверяем что исходный объект сброшен на значения по умолчанию
	ASSERT_EQ(assigned.method, method_t::NONE);
}

/**
 * @brief Метод проверки копирования и перемещения объекта ответа сервера
 *
 */
TEST_F(ProviderFixture, ResponseCopyMoveTest){
	// Создаём исходный объект ответа сервера
	response_t source(version_t::HTTP1_1, static_cast <uint16_t> (201), "Created");
	// Проверяем конструктор копирования
	response_t copy(source);
	// Проверяем что скопированный объект равен исходному
	ASSERT_TRUE(copy == source);
	// Проверяем оператор копирования
	response_t assigned;
	// Выполняем присваивание копированием
	assigned = source;
	// Проверяем что присвоенный объект равен исходному
	ASSERT_TRUE(assigned == source);
	// Проверяем конструктор перемещения
	response_t moved(std::move(copy));
	// Проверяем что перемещённый объект содержит корректные данные
	ASSERT_EQ(moved.code, 201u);
	// Проверяем что сообщение сервера перемещено корректно
	ASSERT_EQ(moved.message, "Created");
	// Проверяем что код исходного объекта сброшен на значение по умолчанию
	ASSERT_EQ(copy.code, 0u);
	// Проверяем оператор перемещения
	response_t movedAssigned;
	// Выполняем присваивание перемещением
	movedAssigned = std::move(assigned);
	// Проверяем что перемещённый объект содержит корректные данные
	ASSERT_EQ(movedAssigned.code, 201u);
	// Проверяем что код исходного объекта сброшен на значение по умолчанию
	ASSERT_EQ(assigned.code, 0u);
}

/**
 * @brief Метод проверки операторов сравнения объектов запроса и ответа
 *
 */
TEST_F(ProviderFixture, ProviderEqualityTest){
	// Создаём два одинаковых объекта запроса
	request_t request1(version_t::HTTP1_1, method_t::GET, "/a"), request2(version_t::HTTP1_1, method_t::GET, "/a");
	// Проверяем что одинаковые запросы равны
	ASSERT_TRUE(request1 == request2);
	// Проверяем что оператор неравенства работает корректно
	ASSERT_FALSE(request1 != request2);
	// Изменяем метод второго запроса
	request2.method = method_t::POST;
	// Проверяем что запросы с разными методами не равны
	ASSERT_TRUE(request1 != request2);
	// Создаём два одинаковых объекта ответа
	response_t response1(version_t::HTTP1_1, static_cast <uint16_t> (200), "OK"), response2(version_t::HTTP1_1, static_cast <uint16_t> (200), "OK");
	// Проверяем что одинаковые ответы равны
	ASSERT_TRUE(response1 == response2);
	// Изменяем код второго ответа
	response2.code = 500;
	// Проверяем что ответы с разными кодами не равны
	ASSERT_TRUE(response1 != response2);
}

/**
 * @brief Метод проверки клонирования провайдеров без срезки производного класса
 *
 */
TEST_F(ProviderFixture, ProviderCloneTest){
	// Создаём исходный объект запроса клиента
	request_t request(version_t::HTTP2, method_t::PATCH, "/resource");
	// Клонируем объект запроса через базовый интерфейс провайдера
	std::unique_ptr <provider_t> clonedRequest = request.clone();
	// Проверяем что клон создан
	ASSERT_TRUE(clonedRequest != nullptr);
	// Проверяем что направление трафика клона соответствует запросу
	ASSERT_EQ(clonedRequest->direct, direct_t::REQUEST);
	// Безопасно приводим клон к типу запроса (тип подтверждён флагом direct)
	const request_t * clonedRequestPtr = static_cast <const request_t *> (clonedRequest.get());
	// Проверяем что производная часть склонирована без срезки
	ASSERT_EQ(clonedRequestPtr->method, method_t::PATCH);
	// Проверяем что URI-адрес склонирован корректно
	ASSERT_EQ(clonedRequestPtr->uri, "/resource");
	// Создаём исходный объект ответа сервера
	response_t response(version_t::HTTP1_1, static_cast <uint16_t> (403), "Forbidden");
	// Клонируем объект ответа через базовый интерфейс провайдера
	std::unique_ptr <provider_t> clonedResponse = response.clone();
	// Проверяем что клон создан
	ASSERT_TRUE(clonedResponse != nullptr);
	// Проверяем что направление трафика клона соответствует ответу
	ASSERT_EQ(clonedResponse->direct, direct_t::RESPONSE);
	// Безопасно приводим клон к типу ответа (тип подтверждён флагом direct)
	const response_t * clonedResponsePtr = static_cast <const response_t *> (clonedResponse.get());
	// Проверяем что производная часть склонирована без срезки
	ASSERT_EQ(clonedResponsePtr->code, 403u);
	// Проверяем что сообщение сервера склонировано корректно
	ASSERT_EQ(clonedResponsePtr->message, "Forbidden");
}
