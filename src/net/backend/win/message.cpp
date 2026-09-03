/**
 * @file message.cpp
 * @date 2026-09-03
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
 * @brief Реализация строя сообщений POSIX для MS Windows — приём сообщения вместе со
 *        служебными метаданными
 *
 * @details Разбор устройства и доводы к выносу лежат при заголовке модуля. Здесь одно
 *          лишь тело приёма: остальное - типы, обход метаданных и выравнивание -
 *          встраивается на месте обращения и живёт в заголовке
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>

/**
 * Подключаем заголовочный файл модуля
 */
#include <net/backend/win/message.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция получения расширенного вызова приёма сообщения с метаданными
 *
 * @param sock сокет, у которого спрашивается вызов
 * @return     адрес расширенного вызова, либо пустое значение
 *
 */
LPFN_WSARECVMSG awh::win::message::extended(const net::socket_t sock) noexcept {
	// Запомненный адрес расширенного вызова
	static LPFN_WSARECVMSG result = nullptr;
	// Если адрес уже взят - отдаём запомненный
	if(result != nullptr)
		// Выводим адрес расширенного вызова
		return result;
	// Опознаватель расширенного вызова приёма сообщения
	GUID identifier = WSAID_WSARECVMSG;
	// Размер полученного ответа, обращению обязательный
	DWORD bytes = 0;
	// Выполняем запрос адреса расширенного вызова у сокета
	if(::WSAIoctl(static_cast <SOCKET> (sock), SIO_GET_EXTENSION_FUNCTION_POINTER, &identifier, sizeof(identifier), &result, sizeof(result), &bytes, nullptr, nullptr) != 0)
		// Сбрасываем адрес расширенного вызова
		result = nullptr;
	// Выводим адрес расширенного вызова
	return result;
}

/**
 * @brief Функция приёма сообщения со служебными метаданными
 *
 * @param sock  сокет, из которого ведётся приём
 * @param msg   описание принимаемого сообщения
 * @param flags признаки приёма, системой не употребляемые
 * @param error код отказа системы, если отказ случился
 * @return      число принятых октетов, либо -1 при отказе
 *
 */
int64_t awh::win::message::receive(const net::socket_t sock, struct msghdr * msg, [[maybe_unused]] const int32_t flags, int32_t * error) noexcept {
	// Получаем расширенный вызов приёма сообщения
	LPFN_WSARECVMSG method = ::awh::win::message::extended(sock);
	// Если вызов взять не удалось либо описание не передано
	if((method == nullptr) || (msg == nullptr)){
		// Если код отказа спрашивают
		if(error != nullptr)
			// Отмечаем приём сообщения недоступным
			(* error) = WSAEOPNOTSUPP;
		// Выводим признак отказа приёма
		return -1;
	}
	// Набор буферов приёма в строе MS Windows
	std::vector <WSABUF> buffers(msg->msg_iovlen);
	/**
	 * Переносим набор буферов приёма
	 */
	for(size_t i = 0; i < msg->msg_iovlen; i++){
		// Устанавливаем начало буфера приёма
		buffers[i].buf = reinterpret_cast <CHAR *> (msg->msg_iov[i].iov_base);
		// Устанавливаем размер буфера приёма
		buffers[i].len = static_cast <ULONG> (msg->msg_iov[i].iov_len);
	}
	// Описание принимаемого сообщения в строе MS Windows
	WSAMSG message{};
	// Устанавливаем адрес собеседника
	message.name = reinterpret_cast <LPSOCKADDR> (msg->msg_name);
	// Устанавливаем размер адреса собеседника
	message.namelen = static_cast <INT> (msg->msg_namelen);
	// Устанавливаем набор буферов приёма
	message.lpBuffers = buffers.data();
	// Устанавливаем число буферов приёма
	message.dwBufferCount = static_cast <ULONG> (buffers.size());
	// Устанавливаем начало буфера служебных метаданных
	message.Control.buf = reinterpret_cast <CHAR *> (msg->msg_control);
	// Устанавливаем размер буфера служебных метаданных
	message.Control.len = static_cast <ULONG> (msg->msg_controllen);
	// Число принятых октетов
	DWORD received = 0;
	// Если приём отказом завершился
	if(method(static_cast <SOCKET> (sock), &message, &received, nullptr, nullptr) != 0){
		// Если код отказа спрашивают
		if(error != nullptr)
			// Запоминаем код отказа приёма
			(* error) = ::WSAGetLastError();
		// Выводим признак отказа приёма
		return -1;
	}
	// Запоминаем размер принятого адреса собеседника
	msg->msg_namelen = static_cast <socklen_t> (message.namelen);
	// Запоминаем размер принятых служебных метаданных
	msg->msg_controllen = static_cast <size_t> (message.Control.len);
	// Запоминаем признаки принятого сообщения
	msg->msg_flags = static_cast <int32_t> (message.dwFlags);
	// Выводим число принятых октетов
	return static_cast <int64_t> (received);
}
