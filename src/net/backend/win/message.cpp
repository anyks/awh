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
#include <atomic>
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
	/**
	 * Запомненный адрес расширенного вызова
	 *
	 * @warning Прежде система писала ответ ПРЯМО В ТАЙНИК: адрес его отдавался
	 *          `WSAIoctl` местом под ответ. Движок же зовёт приём из нескольких
	 *          потоков набора, и покуда один поток тайник заполняет, другой вправе
	 *          прочесть его наполовину заполненным - и уйти вызывать по мусорному
	 *          адресу. Ответ берётся теперь в свой переменной, а в тайник кладётся
	 *          уже целым и разом
	 */
	static std::atomic <LPFN_WSARECVMSG> cache{nullptr};
	// Получаем запомненный адрес расширенного вызова
	LPFN_WSARECVMSG result = cache.load(std::memory_order_acquire);
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
		return nullptr;
	// Запоминаем взятый адрес расширенного вызова целиком
	cache.store(result, std::memory_order_release);
	// Выводим адрес расширенного вызова
	return result;
}

/**
 * @brief Функция приёма сообщения со служебными метаданными
 *
 * @param sock  сокет, из которого ведётся приём
 * @param msg   описание принимаемого сообщения
 * @param flags признаки приёма: поддерживается лишь нулевой набор
 * @param error код отказа системы, если отказ случился
 * @return      число принятых октетов, либо -1 при отказе
 *
 */
int64_t awh::win::message::receive(const net::socket_t sock, struct msghdr * msg, const int32_t flags, int32_t * error) noexcept {
	/**
	 * Признаки приёма расширенный вызов не принимает
	 *
	 * @warning Прежде довод этот числился неупотребляемым и отбрасывался МОЛЧА.
	 *          Приём тогда шёл обычным порядком, что бы ни просили, и опаснее всего
	 *          выходило с `MSG_PEEK`: просящий подсмотреть сообщение, не вынимая его,
	 *          получал сообщение ВЫНУТЫМ - дейтаграмма съедалась, а отказа не было
	 *
	 * @note Поле `dwFlags` записи `WSAMSG` у приёма служит лишь ответом: признаки
	 *       принятого сообщения система кладёт в него сама, а на вход не берёт ни
	 *       одного. `MSG_PEEK` расширенный вызов не поддерживает вовсе
	 *
	 * @note Нуль пропускается: движок зовёт приём с `MSG_NOSIGNAL`, а тот у MS Windows
	 *       объявлен нулём - сигнала, какой требовалось бы гасить, здесь нет
	 *
	 */
	if(flags != 0){
		// Если код отказа спрашивают
		if(error != nullptr)
			// Отмечаем признаки приёма неподдерживаемыми
			(* error) = WSAEOPNOTSUPP;
		// Выводим признак отказа приёма
		return -1;
	}
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
