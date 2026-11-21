/**
 * @file http2.cpp
 * @date 2023-09-27
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
 * @brief Реализация класса работы с фреймами HTTP/2 — адаптер над парсером HTTP/2 AWH v5,
 *        сохраняющий прежний интерфейс модуля
 *
 * @copyright Copyright © 2025
 *
 */


/**
 * Подключаем заголовочный файл
 */
#include <http/http2.hpp>
// Снимаем системные макросы Windows (ERROR, NO_ERROR), занявшие имена значений перечислений
#include "../../include/sys/macro/suppress.hpp"

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция преобразования кода ошибки модуля в код протокола HTTP/2
 *
 * @param error код ошибки модуля
 * @return      код ошибки протокола
 *
 */
static awh::http::h2::error_t __awhH2Error(const awh::Http2::error_t error) noexcept {
	/**
	 * Определяем тип переданной ошибки
	 */
	switch(static_cast <uint8_t> (error)){
		// Требование выполнения отмены запроса
		case static_cast <uint8_t> (awh::Http2::error_t::CANCEL):
			return awh::http::h2::error_t::CANCEL;
		// Ошибка TCP-соединения для метода CONNECT
		case static_cast <uint8_t> (awh::Http2::error_t::CONNECT_ERROR):
			return awh::http::h2::error_t::CONNECT_ERROR;
		// Получен кадр для завершения потока
		case static_cast <uint8_t> (awh::Http2::error_t::STREAM_CLOSED):
			return awh::http::h2::error_t::STREAM_CLOSED;
		// Поток не обработан
		case static_cast <uint8_t> (awh::Http2::error_t::REFUSED_STREAM):
			return awh::http::h2::error_t::REFUSED_STREAM;
		// Ошибка протокола HTTP/2
		case static_cast <uint8_t> (awh::Http2::error_t::PROTOCOL_ERROR):
			return awh::http::h2::error_t::PROTOCOL_ERROR;
		// Получена ошибка реализации
		case static_cast <uint8_t> (awh::Http2::error_t::INTERNAL_ERROR):
			return awh::http::h2::error_t::INTERNAL_ERROR;
		// Размер кадра некорректен
		case static_cast <uint8_t> (awh::Http2::error_t::FRAME_SIZE_ERROR):
			return awh::http::h2::error_t::FRAME_SIZE_ERROR;
		// Установка параметров завершилась по таймауту
		case static_cast <uint8_t> (awh::Http2::error_t::SETTINGS_TIMEOUT):
			return awh::http::h2::error_t::SETTINGS_TIMEOUT;
		// Состояние компрессии не обновлено
		case static_cast <uint8_t> (awh::Http2::error_t::COMPRESSION_ERROR):
			return awh::http::h2::error_t::COMPRESSION_ERROR;
		// Превышена емкость для обработки
		case static_cast <uint8_t> (awh::Http2::error_t::ENHANCE_YOUR_CALM):
			return awh::http::h2::error_t::ENHANCE_YOUR_CALM;
		// Для запроса требуется протокол HTTP/1.1
		case static_cast <uint8_t> (awh::Http2::error_t::HTTP_1_1_REQUIRED):
			return awh::http::h2::error_t::HTTP_1_1_REQUIRED;
		// Ошибка превышения предела управления потоком
		case static_cast <uint8_t> (awh::Http2::error_t::FLOW_CONTROL_ERROR):
			return awh::http::h2::error_t::FLOW_CONTROL_ERROR;
		// Согласованные параметры SSL не приемлемы
		case static_cast <uint8_t> (awh::Http2::error_t::INADEQUATE_SECURITY):
			return awh::http::h2::error_t::INADEQUATE_SECURITY;
	}
	// Выводим значение по умолчанию
	return awh::http::h2::error_t::NO_ERROR;
}
/**
 * @brief Метод преобразования кода ошибки протокола HTTP/2
 *
 * @param code код ошибки протокола
 * @return     код ошибки модуля
 *
 */
awh::Http2::error_t awh::Http2::error(const uint32_t code) noexcept {
	/**
	 * Определяем код ошибки протокола
	 */
	switch(static_cast <http::h2::error_t> (code)){
		// Штатное завершение потока
		case http::h2::error_t::NO_ERROR:
			return error_t::NONE;
		// Требование выполнения отмены запроса
		case http::h2::error_t::CANCEL:
			return error_t::CANCEL;
		// Ошибка TCP-соединения для метода CONNECT
		case http::h2::error_t::CONNECT_ERROR:
			return error_t::CONNECT_ERROR;
		// Получен кадр для завершения потока
		case http::h2::error_t::STREAM_CLOSED:
			return error_t::STREAM_CLOSED;
		// Поток не обработан
		case http::h2::error_t::REFUSED_STREAM:
			return error_t::REFUSED_STREAM;
		// Ошибка протокола HTTP/2
		case http::h2::error_t::PROTOCOL_ERROR:
			return error_t::PROTOCOL_ERROR;
		// Получена ошибка реализации
		case http::h2::error_t::INTERNAL_ERROR:
			return error_t::INTERNAL_ERROR;
		// Размер кадра некорректен
		case http::h2::error_t::FRAME_SIZE_ERROR:
			return error_t::FRAME_SIZE_ERROR;
		// Установка параметров завершилась по таймауту
		case http::h2::error_t::SETTINGS_TIMEOUT:
			return error_t::SETTINGS_TIMEOUT;
		// Состояние компрессии не обновлено
		case http::h2::error_t::COMPRESSION_ERROR:
			return error_t::COMPRESSION_ERROR;
		// Превышена емкость для обработки
		case http::h2::error_t::ENHANCE_YOUR_CALM:
			return error_t::ENHANCE_YOUR_CALM;
		// Для запроса требуется протокол HTTP/1.1
		case http::h2::error_t::HTTP_1_1_REQUIRED:
			return error_t::HTTP_1_1_REQUIRED;
		// Ошибка превышения предела управления потоком
		case http::h2::error_t::FLOW_CONTROL_ERROR:
			return error_t::FLOW_CONTROL_ERROR;
		// Согласованные параметры SSL не приемлемы
		case http::h2::error_t::INADEQUATE_SECURITY:
			return error_t::INADEQUATE_SECURITY;
	}
	// Выводим значение по умолчанию
	return error_t::NONE;
}
/**
 * @brief Метод преобразования списка заголовков в поля парсера
 *
 * @param headers список заголовков
 * @param result  список полей парсера
 *
 */
void awh::Http2::fields(const vector <std::pair <string, string>> & headers, vector <http::h2::hpack::field_t> & result) noexcept {
	// Резервируем место под все передаваемые заголовки
	result.reserve(headers.size());
	/**
	 * Перебираем все заголовки
	 */
	for(auto & header : headers)
		// Добавляем заголовок в список полей
		result.emplace_back(header.first, header.second);
}
/**
 * @brief Метод начала выполнения операции
 *
 * @param event событие выполняемой операции
 *
 */
void awh::Http2::activate(const event_t event) noexcept {
	// Если это самая внешняя операция, запоминаем её событие
	if(this->_depth++ == 0)
		// Запоминаем активное событие операции
		this->_event = event;
}
/**
 * @brief Метод завершения выполнения операции
 *
 * @param event событие выполненной операции
 *
 */
void awh::Http2::completed([[maybe_unused]] const event_t event) noexcept {
	// Если глубина вложенности операций не нулевая
	if(this->_depth > 0)
		// Уменьшаем глубину вложенности операций
		this->_depth--;
	/**
	 * Завершаем только самую внешнюю операцию: пока выполняется вложенная операция,
	 * мы находимся внутри функций обратного вызова парсера и удалять его нельзя
	 */
	if(this->_depth == 0){
		// Снимаем активное событие
		this->_event = event_t::NONE;
		// Если есть закрытые потоки
		if(!this->_closed.empty()){
			/**
			 * Перебираем все закрытые потоки
			 */
			for(auto sid : this->_closed){
				// Удаляем записи потока
				this->_records.erase(sid);
				// Удаляем буфер полезной нагрузки потока
				this->_payloads.erase(sid);
			}
			// Очищаем список закрытых потоков
			this->_closed.clear();
		}
		// Если функция обратного вызова на тригер установлена
		if(this->_callback.is(1)){
			// Вызываем функцию триггера
			this->_callback.call <void (void)> (1);
			// Снимаем триггерную функцию
			this->_callback.erase(1);
		}
		// Если есть требование закрыть подключение
		if(this->_close)
			// Закрываем подключение
			this->close();
	}
}
/**
 * @brief Метод пометки буферов потока на удаление
 *
 * @param sid идентификатор потока
 *
 */
void awh::Http2::release(const int32_t sid) noexcept {
	// Флаг найденных буферов потока
	bool found = false;
	// Ищем записи потока
	auto records = this->_records.find(sid);
	// Если записи для потока найдены
	if((found = (records != this->_records.end())))
		// Очищаем записи потока
		std::queue <std::pair <size_t, flag_t>> ().swap(records->second);
	// Ищем буфер данных потока
	auto payload = this->_payloads.find(sid);
	// Если буфер данных для потока найден
	if(payload != this->_payloads.end()){
		// Очищаем буфер данных потока
		payload->second->clear();
		// Запоминаем, что буферы потока найдены
		found = true;
	}
	/**
	 * Если буферы потока найдены, помечаем их на удаление. Элементы словарей удаляются
	 * только после выхода из внешней операции, так как по ним может идти перебор
	 */
	if(found)
		// Добавляем поток в список закрытых
		this->_closed.emplace(sid);
}
/**
 * @brief Метод отправки накопленной полезной нагрузки потока
 *
 * @param sid идентификатор потока
 *
 */
void awh::Http2::pump(const int32_t sid) noexcept {
	/**
	 * Прокачка не реентерабельна: обратный вызов writable приходит из движка,
	 * пока внешняя прокачка ещё держит очередь и буфер потока
	 */
	if(this->_pumping)
		// Выходим из функции
		return;
	// Помечаем, что прокачка уже выполняется
	this->_pumping = true;
	// Снимаем признак при любом выходе из функции
	struct PumpGuard {
		// Ссылка на признак выполняемой прокачки
		bool & flag;
		// Снимаем признак при разрушении стража
		~PumpGuard() noexcept { this->flag = false; }
	} guard{this->_pumping};
	// Если парсер не инициализирован
	if(this->_engine == nullptr){
		// Выходим из функции
		return;
	}
	// Если поток уже закрыт, освобождаем его буферы
	if(!this->_engine->streamExists(static_cast <uint32_t> (sid))){
		// Пометка буферов на удаление выполняется только для существующих записей
		if(this->_records.find(sid) != this->_records.end())
			// Помечаем буферы потока на удаление
			this->release(sid);
		// Выходим из функции
		return;
	}
	// Ищем записи потока
	auto records = this->_records.find(sid);
	// Ищем буфер полезной нагрузки потока
	auto payload = this->_payloads.find(sid);
	// Если записи и буфер полезной нагрузки найдены
	if((records != this->_records.end()) && (payload != this->_payloads.end())){
		/**
		 * Перебираем весь список записей, которые ещё не отправлены
		 */
		while(!records->second.empty() && !this->_wouldblock){
			// Текущая запись потока: копия, потому что после вызова движка очередь может быть освобождена
			const std::pair <size_t, flag_t> record = records->second.front();
			// Если буфера полезной нагрузки не хватает - пометить и выйти
			if(payload->second->size() < record.first){
				// Помечаем буферы потока на удаление
				this->release(sid);
				// Выходим из функции
				return;
			}
			// Отдаём часть тела парсеру
			const size_t result = this->_engine->sendData(static_cast <uint32_t> (sid), payload->second->data(), record.first, (record.second == flag_t::END_STREAM));
			// Если парсер не принял данных, выходим
			if(result == 0)
				// Выходим из цикла
				break;
			/**
			 * Отправка END_STREAM закрывает поток синхронно: обратный вызов закрытия
			 * очищает очередь и буфер, и прежние итераторы становятся недействительными.
			 * Ищем их заново и не обращаемся к опустевшим записям
			 */
			records = this->_records.find(sid);
			// Заново ищем буфер полезной нагрузки потока
			payload = this->_payloads.find(sid);
			// Если поток закрыт и буферы освобождены - отправлять больше нечего
			if((records == this->_records.end()) || (payload == this->_payloads.end()) || records->second.empty() || payload->second->empty())
				// Выходим из функции
				return;
			// Уменьшаем размер отправленных данных записи
			records->second.front().first -= result;
			// Удаляем из буфера отправленные данные
			payload->second->erase(result);
			// Если все данные записи отправленны
			if(records->second.front().first == 0)
				// Снимаем запись с очереди
				records->second.pop();
			// Если данные буфера закончились, а записи остались - рассинхронизация
			if(payload->second->empty() && !records->second.empty()){
				// Помечаем буферы потока на удаление
				this->release(sid);
				// Выходим из функции
				return;
			}
		}
	}
}
/**
 * @brief Метод отправки исходящих байтов в сеть
 *
 * @return результат отправки
 *
 */
bool awh::Http2::flush() noexcept {
	// Если парсер не инициализирован
	if(this->_engine == nullptr){
		// Выходим из функции
		return false;
	}
	/**
	 * Отправляем исходящие байты, пока их принимает очередь отправки
	 */
	while(!this->_wouldblock){
		// Получаем ещё не отправленные исходящие байты парсера
		const string_view output = this->_engine->pending();
		// Если исходящих байтов нет, выходим
		if(output.empty())
			// Выходим из цикла
			break;
		// Определяем размер отправляемой части
		const size_t size = ::min(output.size(), static_cast <size_t> (MAX_FRAME_SIZE_MIN));
		/**
		 * Если функция обратного вызова проверки места в очереди отправки установлена
		 * и очередь заполнена, останавливаем отправку до вызова resume()
		 */
		if(this->_callback.is("writable") && !this->_callback.call <bool (const size_t)> ("writable", size)){
			// Помечаем остановку отправки
			this->_wouldblock = true;
			// Выходим из функции
			return false;
		}
		// Если функция обратного вызова отправки установлена
		if(this->_callback.is("send"))
			// Вызываем функцию обратного вызова
			this->_callback.call <void (const uint8_t *, const size_t)> ("send", reinterpret_cast <const uint8_t *> (output.data()), size);
		// Освобождаем отправленные байты из исходящего буфера парсера
		this->_engine->consumePending(size);
	}
	// Выводим результат
	return true;
}
/**
 * @brief Метод вызова функции обратного вызова фрейма
 *
 * @param sid    идентификатор потока
 * @param direct направление передачи фрейма
 * @param type   тип фрейма
 * @param flags  флаги фрейма
 * @return       статус обработки фрейма
 *
 */
int32_t awh::Http2::fire(const int32_t sid, const direct_t direct, const frame_t type, const std::set <flag_t> & flags) noexcept {
	// Результат работы функции
	int32_t result = 0;
	// Если функция обратного вызова установлена
	if(this->_callback.is("frame")){
		// Вызываем функцию обратного вызова
		result = this->_callback.call <int32_t (const int32_t, const direct_t, const frame_t, const std::set <flag_t> &)> ("frame", sid, direct, type, flags);
		// Если функция обратного вызова потребовала прервать разбор
		if(result != 0)
			// Помечаем требование прервать разбор соединения
			this->_failed = true;
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод выполнения пинга
 *
 * @return результат работы пинга
 *
 */
bool awh::Http2::ping() noexcept {
	// Начинаем операцию
	this->activate(event_t::SEND_PING);
	// Результат работы функции
	bool result = false;
	// Если парсер инициализирован
	if(this->_engine != nullptr){
		/**
		 * Пинг проверки живости отправляем запросом, а не подтверждением:
		 * несанкционированный PING с флагом ACK пир обязан считать ошибкой
		 * соединения (RFC 9113 §6.7), и nghttp2 рвёт сессию с PROTOCOL_ERROR
		 */
		this->_engine->sendPing(false);
		/**
		 * Отправляем исходящие байты в сеть. Заполненная очередь отправки -
		 * не отказ: кадр уже в исходящем буфере движка и уйдёт по resume()
		 */
		this->flush();
		// Пинг поставлен в очередь отправки
		result = true;
	}
	// Вызываем метод выполненного события
	this->completed(event_t::SEND_PING);
	// Выводим результат
	return result;
}
/**
 * @brief Метод запрещения получения данных с клиента
 *
 * @return результат выполнения операции
 *
 */
bool awh::Http2::shutdown() noexcept {
	// Начинаем операцию
	this->activate(event_t::SEND_SHUTDOWN);
	// Результат работы функции
	bool result = false;
	// Если парсер инициализирован
	if(this->_engine != nullptr){
		// Отправляем предупреждение о завершении работы
		this->_engine->sendShutdown();
		/**
		 * Отправляем исходящие байты в сеть. Заполненная очередь отправки -
		 * не отказ: GOAWAY уже в исходящем буфере движка и уйдёт по resume()
		 */
		this->flush();
		// Предупреждение поставлено в очередь отправки
		result = true;
	}
	// Вызываем метод выполненного события
	this->completed(event_t::SEND_SHUTDOWN);
	// Выводим результат
	return result;
}
/**
 * @brief Метод чтения данных фрейма из бинарного буфера
 *
 * @param buffer буфер бинарных данных для чтения фрейма
 * @param size   размер буфера бинарных данных
 * @return       результат чтения данных фрейма
 *
 */
bool awh::Http2::frame(const uint8_t * buffer, const size_t size) noexcept {
	// Начинаем операцию
	this->activate(event_t::RECV_FRAME);
	// Снимаем требование прервать разбор соединения
	this->_failed = false;
	// Результат работы функции
	bool result = false;
	// Если данные для чтения переданы и парсер инициализирован
	if((buffer != nullptr) && (size > 0) && (this->_engine != nullptr)){
		// Разбираем полученные байты соединения
		const size_t bytes = this->_engine->parse(buffer, size);
		// Отправляем подготовленные ответные фреймы
		this->flush();
		// Если разбор завершился ошибкой
		if(!(result = ((bytes == size) && (this->_engine->status() != http::parser_t::status_t::ERROR) && !this->_failed))){
			// Получаем текст ошибки разбора
			const string message = this->_fmk->format("HTTP/2 parse error: %s", string(this->_engine->errorName()).c_str());
			// Выводим сообщение об полученной ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, message.c_str());
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Вызываем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_RECV, message);
		}
	}
	// Вызываем метод выполненного события
	this->completed(event_t::RECV_FRAME);
	// Выводим результат
	return result;
}
/**
 * @brief Метод продолжения отправки фреймов, отложенных из-за заполненной очереди
 *
 * @note Если функция обратного вызова «writable» сообщает, что очередь отправки
 *       заполнена, байты не отдаются, а накапливаются до этого вызова
 *
 * @return результат отправки
 *
 */
bool awh::Http2::resume() noexcept {
	// Если парсер не инициализирован
	if(this->_engine == nullptr)
		// Выводим результат
		return false;
	// Активируем событие отправки данных
	this->activate(event_t::SEND_DATA);
	// Снимаем остановку отправки
	this->_wouldblock = false;
	// Отправляем отложенные фреймы
	bool result = this->flush();
	/**
	 * Перебираем потоки, ждавшие снятия остановки
	 */
	for(auto & record : this->_records)
		// Отдаём данные полезной нагрузки потока парсеру
		this->pump(record.first);
	// Досылаем байты, поставленные в очередь прокачкой потоков
	result = (this->flush() && result);
	// Завершаем событие отправки данных
	this->completed(event_t::SEND_DATA);
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения размера данных потока, ещё не отданных в сеть
 *
 * @param id идентификатор потока
 * @return   размер данных в байтах
 *
 */
size_t awh::Http2::pending(const int32_t id) const noexcept {
	// Ищем буфер полезной нагрузки потока
	auto payload = this->_payloads.find(id);
	// Выводим размер данных в буфере
	return ((payload != this->_payloads.end()) ? payload->second->size() : 0);
}
/**
 * @brief Метод выполнения сброса подключения
 *
 * @param sid   идентификатор потока
 * @param error код отправляемой ошибки
 * @return      результат отправки сообщения
 *
 */
bool awh::Http2::reject(const int32_t sid, const error_t error) noexcept {
	// Начинаем операцию
	this->activate(event_t::SEND_REJECT);
	// Результат работы функции
	bool result = false;
	/**
	 * Определяем идентификатор сервиса
	 */
	switch(static_cast <uint8_t> (this->_mode)){
		// Если сервис идентифицирован как клиент
		case static_cast <uint8_t> (mode_t::CLIENT): {
			// Выводим сообщение об ошибке
			this->_log->print("Client is not allowed to call the \"%s\" method", log_t::flag_t::WARNING, "REJECT");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Вызываем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_CANCEL, "Client is not allowed to call the \"REJECT\" method");
		} break;
		// Если сервис идентифицирован как сервер
		case static_cast <uint8_t> (mode_t::SERVER): {
			// Если парсер инициализирован
			if(this->_engine != nullptr){
				// Сбрасываем поток клиента
				this->_engine->sendRstStream(static_cast <uint32_t> (sid), __awhH2Error(error));
				/**
				 * Отправляем исходящие байты в сеть. Заполненная очередь отправки -
				 * не отказ: RST_STREAM уже в исходящем буфере движка и уйдёт по resume()
				 */
				this->flush();
				// Сброс потока поставлен в очередь отправки
				result = true;
			}
		} break;
	}
	// Вызываем метод выполненного события
	this->completed(event_t::SEND_REJECT);
	// Выводим результат
	return result;
}
/**
 * @brief Метод отправки списка разрешённых источников
 *
 */
void awh::Http2::sendOrigin() noexcept {
	// Если список источников пуст, отправлять нечего и операцию не начинаем
	if(this->_origins.empty() || (this->_engine == nullptr))
		// Выходим из функции
		return;
	// Начинаем операцию
	this->activate(event_t::SEND_ORIGIN);
	/**
	 * Определяем идентификатор сервиса
	 */
	switch(static_cast <uint8_t> (this->_mode)){
		// Если сервис идентифицирован как клиент
		case static_cast <uint8_t> (mode_t::CLIENT): {
			// Выводим сообщение об ошибке
			this->_log->print("Client is not allowed to call the \"%s\" method", log_t::flag_t::WARNING, "ORIGIN");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Вызываем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_CANCEL, "Client is not allowed to call the \"ORIGIN\" method");
		} break;
		// Если сервис идентифицирован как сервер
		case static_cast <uint8_t> (mode_t::SERVER):
			// Ставим фрейм разрешённых источников в очередь отправки
			this->_engine->sendOrigin(this->_origins);
		break;
	}
	// Вызываем метод выполненного события
	this->completed(event_t::SEND_ORIGIN);
}
/**
 * @brief Метод отправки расширения альтернативного сервиса RFC7383
 *
 * @param sid идентификатор потока
 *
 */
void awh::Http2::sendAltSvc(const int32_t sid) noexcept {
	// Если список альтернативных сервисов пуст, отправлять нечего и операцию не начинаем
	if(this->_altsvc.empty() || (this->_engine == nullptr))
		// Выходим из функции
		return;
	// Начинаем операцию
	this->activate(event_t::SEND_ALTSVC);
	/**
	 * Определяем идентификатор сервиса
	 */
	switch(static_cast <uint8_t> (this->_mode)){
		// Если сервис идентифицирован как клиент
		case static_cast <uint8_t> (mode_t::CLIENT): {
			// Выводим сообщение об ошибке
			this->_log->print("Client is not allowed to call the \"%s\" method", log_t::flag_t::WARNING, "ALTSVC");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Вызываем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_CANCEL, "Client is not allowed to call the \"ALTSVC\" method");
		} break;
		// Если сервис идентифицирован как сервер
		case static_cast <uint8_t> (mode_t::SERVER): {
			/**
			 * Определяем идентификатор потока
			 */
			switch(sid){
				// Если фрейм является системным
				case 0: {
					/**
					 * Перебираем все альтернативные сервисы
					 */
					for(auto & item : this->_altsvc)
						// Ставим фрейм альтернативного сервиса в очередь отправки
						this->_engine->sendAltSvc(0, item.first, item.second);
				} break;
				// Если фрейм является пользовательским запросом
				default:
					// Ставим фрейм альтернативного сервиса в очередь отправки
					this->_engine->sendAltSvc(static_cast <uint32_t> (sid), "", "");
				break;
			}
		} break;
	}
	// Вызываем метод выполненного события
	this->completed(event_t::SEND_ALTSVC);
}
/**
 * @brief Метод отправки трейлеров
 *
 * @param id      идентификатор потока
 * @param headers заголовки отправляемые
 * @return        результат отправки данных фрейма
 *
 */
bool awh::Http2::sendTrailers(const int32_t id, const vector <std::pair <string, string>> & headers) noexcept {
	// Начинаем операцию
	this->activate(event_t::SEND_TRAILERS);
	// Результат работы функции
	bool result = false;
	// Если заголовки для отправки переданы и парсер инициализирован
	if(!headers.empty() && (this->_engine != nullptr)){
		/**
		 * Определяем идентификатор сервиса
		 */
		switch(static_cast <uint8_t> (this->_mode)){
			// Если сервис идентифицирован как клиент
			case static_cast <uint8_t> (mode_t::CLIENT): {
				// Выводим сообщение об ошибке
				this->_log->print("Client is not allowed to call the \"%s\" method", log_t::flag_t::WARNING, "TRAILERS");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Вызываем функцию обратного вызова
					this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_CANCEL, "Client is not allowed to call the \"TRAILERS\" method");
			} break;
			// Если сервис идентифицирован как сервер
			case static_cast <uint8_t> (mode_t::SERVER): {
				// Список полей заголовков трейлера
				vector <http::h2::hpack::field_t> fields;
				// Преобразуем список заголовков в поля парсера
				http2_t::fields(headers, fields);
				// Отправляем трейлеры с завершением потока
				this->_engine->sendHeaders(static_cast <uint32_t> (id), fields, true);
				/**
				 * Отправляем исходящие байты в сеть. Заполненная очередь отправки -
				 * не отказ: трейлеры уже в исходящем буфере движка и уйдут по resume()
				 */
				this->flush();
				// Трейлеры поставлены в очередь отправки
				result = true;
				// Флаги отправленного фрейма
				std::set <flag_t> flags;
				// Ставим флаг завершения заголовков
				flags.emplace(flag_t::END_HEADERS);
				// Ставим флаг завершения потока
				flags.emplace(flag_t::END_STREAM);
				// Вызываем функцию обратного вызова фрейма
				this->fire(id, direct_t::SEND, frame_t::HEADERS, flags);
			} break;
		}
	}
	// Вызываем метод выполненного события
	this->completed(event_t::SEND_TRAILERS);
	// Выводим результат
	return result;
}
/**
 * @brief Метод отправки бинарных данных
 *
 * @param id     идентификатор потока
 * @param buffer буфер бинарных данных передаваемых
 * @param size   размер передаваемых данных в байтах
 * @param flag   флаг передаваемого потока по сети
 * @return       результат отправки данных фрейма
 *
 */
bool awh::Http2::sendData(const int32_t id, const uint8_t * buffer, const size_t size, const flag_t flag) noexcept {
	// Начинаем операцию
	this->activate(event_t::SEND_DATA);
	// Результат работы функции
	bool result = false;
	// Если данные полезной нагрузки для отправки в сеть переданы
	if((buffer != nullptr) && (size > 0) && (this->_engine != nullptr)){
		/**
		 * Если на сервере поток, открытый клиентом (нечётный идентификатор), уже закрыт (например, сброшен клиентом),
		 * данные доставить некому: не накапливаем их в буфере, иначе они останутся в нём до закрытия подключения.
		 * Это не ошибка подключения, поэтому сообщаем об успехе, чтобы подключение не закрывалось
		 */
		if((this->_mode == mode_t::SERVER) && ((id % 2) == 1) && !this->_engine->streamExists(static_cast <uint32_t> (id))){
			// Вызываем метод выполненного события
			this->completed(event_t::SEND_DATA);
			// Выводим результат
			return true;
		}
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			{
				// Ищем буфер полезной нагрузки
				auto payload = this->_payloads.find(id);
				// Если буфер полезной нагрузки существует
				if(payload != this->_payloads.end())
					// Дописываем данные в буфер полезной нагрузки
					payload->second->push(buffer, size);
				// Если буфер полезной нагрузки не существует
				else {
					// Создаём буфер полезной нагрузки
					auto ret = this->_payloads.emplace(id, std::make_unique <buffer_t> (this->_fmk, this->_log));
					// Дописываем данные в буфер полезной нагрузки
					ret.first->second->push(buffer, size);
				}
			}{
				// Получаем список записей потока
				auto records = this->_records.find(id);
				// Если список записей для потока получен
				if(records != this->_records.end())
					// Добавляем новую запись в очередь
					records->second.push(std::make_pair(size, flag));
				// Создаём новый список записей потока
				else {
					// Создаём новый список записей потока
					auto ret = this->_records.emplace(id, std::queue <std::pair <size_t, flag_t>> ());
					// Добавляем новую запись в очередь
					ret.first->second.push(std::make_pair(size, flag));
				}
			}
			// Отдаём данные полезной нагрузки потока парсеру
			this->pump(id);
			/**
			 * Досылаем исходящие байты в сеть. Заполненная очередь отправки - не отказ:
			 * данные уже приняты в буфер потока, остаток уйдёт по resume() после записи
			 */
			this->flush();
			// Данные приняты и поставлены в очередь отправки
			result = true;
			// Флаги отправленного фрейма
			std::set <flag_t> flags;
			// Если требуется завершить поток после передачи фрейма
			if(flag == flag_t::END_STREAM)
				// Ставим флаг завершения потока
				flags.emplace(flag_t::END_STREAM);
			// Вызываем функцию обратного вызова фрейма
			this->fire(id, direct_t::SEND, frame_t::DATA, flags);
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, buffer, size, static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(id, buffer, size, static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Вызываем метод выполненного события
	this->completed(event_t::SEND_DATA);
	// Выводим результат
	return result;
}
/**
 * @brief Метод отправки push-уведомлений
 *
 * @param id      идентификатор потока
 * @param headers заголовки отправляемые
 * @param flag    флаг передаваемого потока по сети
 * @return        флаг завершения потока передачи данных
 *
 */
int32_t awh::Http2::sendPush(const int32_t id, const vector <std::pair <string, string>> & headers, const flag_t flag) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Начинаем операцию
	this->activate(event_t::SEND_PUSH);
	// Если заголовки для отправки переданы и парсер инициализирован
	if(!headers.empty() && (this->_engine != nullptr)){
		// Список полей заголовков обещанного запроса
		vector <http::h2::hpack::field_t> fields;
		// Преобразуем список заголовков в поля парсера
		http2_t::fields(headers, fields);
		// Отправляем push-уведомление клиенту
		const uint32_t sid = this->_engine->sendPushPromise(static_cast <uint32_t> (id), fields);
		// Если запрос не получилось отправить
		if(sid == 0){
			// Выводим в лог сообщение
			this->_log->print("%s", log_t::flag_t::WARNING, "Could not submit PUSH_PROMISE");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Вызываем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_SUBMIT, "Could not submit PUSH_PROMISE");
		// Если запрос отправить удалось
		} else {
			// Отправляем исходящие байты в сеть
			this->flush();
			// Флаги отправленного фрейма
			std::set <flag_t> flags;
			// Ставим флаг завершения заголовков
			flags.emplace(flag_t::END_HEADERS);
			// Вызываем функцию обратного вызова фрейма
			this->fire(id, direct_t::SEND, frame_t::PUSH_PROMISE, flags);
			// Выводим идентификатор зарезервированного потока
			result = static_cast <int32_t> (sid);
		}
	}
	// Вызываем метод выполненного события
	this->completed(event_t::SEND_PUSH);
	// Выводим результат
	return result;
}
/**
 * @brief Метод отправки заголовков
 *
 * @param id      идентификатор потока
 * @param headers заголовки отправляемые
 * @param flag    флаг передаваемого потока по сети
 * @return        флаг завершения потока передачи данных
 *
 */
int32_t awh::Http2::sendHeaders(const int32_t id, const vector <std::pair <string, string>> & headers, const flag_t flag) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Начинаем операцию
	this->activate(event_t::SEND_HEADERS);
	// Если заголовки для отправки переданы и парсер инициализирован
	if(!headers.empty() && (this->_engine != nullptr)){
		// Определяем идентификатор потока
		int32_t sid = id;
		// Если идентификатор потока не передан, выделяем новый
		if(sid < 0)
			// Выделяем идентификатор нового потока
			sid = static_cast <int32_t> (this->_engine->nextStreamId());
		// Если идентификатор потока получен
		if(sid > 0){
			// Список полей заголовков запроса
			vector <http::h2::hpack::field_t> fields;
			// Преобразуем список заголовков в поля парсера
			http2_t::fields(headers, fields);
			// Определяем требование завершения потока
			const bool end = (flag == flag_t::END_STREAM);
			// Отправляем заголовки удалённому узлу
			this->_engine->sendHeaders(static_cast <uint32_t> (sid), fields, end);
			// Отправляем исходящие байты в сеть
			this->flush();
			// Флаги отправленного фрейма
			std::set <flag_t> flags;
			// Ставим флаг завершения заголовков
			flags.emplace(flag_t::END_HEADERS);
			// Если требуется завершить поток после передачи фрейма
			if(end)
				// Ставим флаг завершения потока
				flags.emplace(flag_t::END_STREAM);
			// Вызываем функцию обратного вызова фрейма
			this->fire(sid, direct_t::SEND, frame_t::HEADERS, flags);
			// Если идентификатор потока выделялся автоматически, возвращаем его
			result = ((id < 0) ? sid : 0);
		// Если идентификатор потока не получен
		} else {
			// Выводим в лог сообщение
			this->_log->print("%s", log_t::flag_t::WARNING, "Could not allocate HTTP/2 stream");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Вызываем функцию обратного вызова
				this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_SUBMIT, "Could not allocate HTTP/2 stream");
		}
	}
	// Вызываем метод выполненного события
	this->completed(event_t::SEND_HEADERS);
	// Выводим результат
	return result;
}
/**
 * @brief Метод отправки сообщения закрытия всех потоков
 *
 * @param last   идентификатор последнего потока
 * @param error  код отправляемой ошибки
 * @param buffer буфер отправляемых данных если требуется
 * @param size   размер отправляемого буфера данных
 * @return       результат отправки данных фрейма
 *
 */
bool awh::Http2::goaway(const int32_t last, const error_t error, const uint8_t * buffer, const size_t size) noexcept {
	// Начинаем операцию
	this->activate(event_t::SEND_GOAWAY);
	// Результат работы функции
	bool result = false;
	// Если размер окна фрейма передан
	if(last > 0){
		/**
		 * Определяем идентификатор сервиса
		 */
		switch(static_cast <uint8_t> (this->_mode)){
			// Если сервис идентифицирован как клиент
			case static_cast <uint8_t> (mode_t::CLIENT): {
				// Выводим сообщение об ошибке
				this->_log->print("Client is not allowed to call the \"%s\" method", log_t::flag_t::WARNING, "GOAWAY");
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Вызываем функцию обратного вызова
					this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::HTTP2_CANCEL, "Client is not allowed to call the \"GOAWAY\" method");
			} break;
			// Если сервис идентифицирован как сервер
			case static_cast <uint8_t> (mode_t::SERVER): {
				// Если парсер инициализирован
				if(this->_engine != nullptr){
					// Отправляем сообщение закрытия всех потоков
					this->_engine->sendGoaway(__awhH2Error(error), ((buffer != nullptr) && (size > 0) ? string_view(reinterpret_cast <const char *> (buffer), size) : string_view()));
					/**
					 * Отправляем исходящие байты в сеть. Заполненная очередь отправки -
					 * не отказ: GOAWAY уже в исходящем буфере движка и уйдёт по resume()
					 */
					this->flush();
					// GOAWAY поставлен в очередь отправки
					result = true;
					// Флаги отправленного фрейма
					std::set <flag_t> flags;
					// Вызываем функцию обратного вызова фрейма
					this->fire(last, direct_t::SEND, frame_t::GOAWAY, flags);
				}
			} break;
		}
	}
	// Вызываем метод выполненного события
	this->completed(event_t::SEND_GOAWAY);
	// Выводим результат
	return result;
}
/**
 * @brief Метод обновления размера окна фрейма
 *
 * @param sid  идентификатор потока
 * @param size размер нового окна
 * @return     результат установки размера офна фрейма
 *
 */
bool awh::Http2::windowUpdate(const int32_t sid, const int32_t size) noexcept {
	// Если размер окна фрейма передан и парсер инициализирован
	if((size > 0) && (this->_engine != nullptr)){
		// Ставим новый размер окна фрейма
		this->_engine->sendWindowUpdate(static_cast <uint32_t> (sid), static_cast <uint32_t> (size));
		// Отправляем исходящие байты в сеть
		return this->flush();
	}
	// Выводим результат
	return false;
}
/**
 * @brief Метод очистки активной сессии
 *
 */
void awh::Http2::free() noexcept {
	// Если парсер создан удачно
	if(this->_engine != nullptr)
		// Освобождаем парсер соединения
		this->_engine.reset();
	// Если очереди полезной нагрузки созданы
	if(!this->_payloads.empty())
		// Очищаем очереди полезной нагрузки
		this->_payloads.clear();
	// Если список записей существует
	if(!this->_records.empty())
		// Очищаем весь список записей
		this->_records.clear();
	// Очищаем список закрытых потоков
	this->_closed.clear();
	// Обнуляем максимальный размер списка заголовков
	this->_maxHeaderListSize = 0;
}
/**
 * @brief Метод закрытия подключения
 *
 */
void awh::Http2::close() noexcept {
	/**
	 * Если выполняется хотя бы одна операция, мы можем находиться внутри функций обратного
	 * вызова парсера: удаление парсера там приводит к обращению к освобождённой памяти,
	 * поэтому закрытие откладывается до завершения самой внешней операции
	 */
	if(!(this->_close = (this->_depth > 0))){
		// Если парсер создан удачно
		if(this->_engine != nullptr){
			// Освобождаем созданный ранее парсер
			this->free();
		}
	}
}
/**
 * @brief Метод проверки инициализации модуля
 *
 * @return результат проверки инициализации
 *
 */
bool awh::Http2::initialized() const noexcept {
	// Выводим результат проверки
	return (this->_engine != nullptr);
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 *
 */
void awh::Http2::callback(const callback_t & callback) noexcept {
	// Если установлена триггерная функция
	if(callback.is(1)){
		// Если активное событие не установлено
		if(this->_event == event_t::NONE)
			// Вызываем функцию обратного вызова
			callback.call <void (void)> (1);
		// Устанавливаем функцию обратного вызова
		else this->_callback.set(1, callback);
	// Если триггерная функция не установлена
	} else {
		// Устанавливаем функцию обратного вызова при отправки сообщения
		this->_callback.set("send", callback);
		// Устанавливаем функцию обратного вызова проверки места в очереди отправки
		this->_callback.set("writable", callback);
		// Устанавливаем функцию обратного вызова при закрытии потока
		this->_callback.set("close", callback);
		// Устанавливаем функцию обратного вызова начала открытии потока
		this->_callback.set("begin", callback);
		// Устанавливаем функцию обратного вызова при получении чанка
		this->_callback.set("chunk", callback);
		// Устанавливаем функцию обратного вызова превышения лимита тела потока
		this->_callback.set("limit", callback);
		// Устанавливаем функцию обратного вызова на событие получения ошибки
		this->_callback.set("error", callback);
		// Устанавливаем функцию обратного вызова при обмене фреймами
		this->_callback.set("frame", callback);
		// Устанавливаем функцию обратного вызова при создании фрейма
		this->_callback.set("create", callback);
		// Устанавливаем функцию обратного вызова при получении данных заголовка
		this->_callback.set("header", callback);
		// Устанавливаем функцию обратного вызова при получении источника подключения
		this->_callback.set("origin", callback);
		// Устанавливаем функцию обратного вызова при получении альтернативных сервисов
		this->_callback.set("altsvc", callback);
	}
}
/**
 * @brief Метод установки списка разрешённых источников
 *
 * @param origins список разрешённых источников
 *
 */
void awh::Http2::origin(const vector <string> & origins) noexcept {
	// Сохраняем список разрешённых источников
	this->_origins.assign(origins.begin(), origins.end());
}
/**
 * @brief Метод установки списка альтернативных сервисов
 *
 * @param origins список альтернативных сервисов
 *
 */
void awh::Http2::altsvc(const std::unordered_multimap <string, string> & origins) noexcept {
	// Сохраняем список альтернативных сервисов
	this->_altsvc = origins;
}
/**
 * @brief Метод установки предельного размера тела потока
 *
 * @param size предельный размер тела в байтах
 *
 */
void awh::Http2::maxBodySize(const uint64_t size) noexcept {
	// Если размер передан и парсер инициализирован
	if((size > 0) && (this->_engine != nullptr)){
		// Получаем текущие лимиты парсера
		http::parser_http2_t::limits_t limits = this->_engine->limits();
		// Устанавливаем предельный размер тела потока
		limits.maxBodySize = size;
		// Применяем лимиты парсера
		this->_engine->limits(limits);
	}
}
/**
 * @brief Метод инициализации
 *
 * @param mode     идентификатор сервиса
 * @param settings параметры настроек сессии
 * @return         результат выполнения инициализации
 *
 */
bool awh::Http2::init(const mode_t mode, const std::map <settings_t, uint32_t> & settings) noexcept {
	// Результат работы функции
	bool result = false;
	// Если параметры настроек переданы
	if(!settings.empty() && (mode != mode_t::NONE)){
		// Очищаем предыдущую сессию
		this->free();
		// Запоминаем идентификатор сервиса
		this->_mode = mode;
		/**
		 * Создаём парсер HTTP/2: сервер разбирает запросы клиента,
		 * клиент - ответы сервера
		 */
		try {
			// Создаём парсер HTTP/2
			this->_engine = std::make_unique <http::parser_http2_t> ((mode == mode_t::SERVER ? http::direct_t::REQUEST : http::direct_t::RESPONSE));
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (mode), settings.size()), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
			// Выводим результат
			return result;
		}
		// Параметры SETTINGS нашего узла
		http::parser_http2_t::settings_t local;
		// Лимиты безопасности парсера
		http::parser_http2_t::limits_t limits;
		// Запрещаем приоритизацию RFC 7540
		local.noRfc7540Priorities = 1;
		/**
		 * Устанавливаем максимальный размер списка заголовков по умолчанию. Клиенту допускаем больший размер,
		 * так как заголовки ответов серверов бывают крупнее (браузеры принимают до 256 КиБ)
		 */
		this->_maxHeaderListSize = (mode == mode_t::CLIENT ? (MAX_HEADER_LIST_SIZE * 4) : MAX_HEADER_LIST_SIZE);
		/**
		 * Перебираем полученные настройки
		 */
		for(auto & item : settings){
			/**
			 * Определяем код параметра настроек
			 */
			switch(static_cast <uint8_t> (item.first)){
				// Если мы получили разрешение присылать push-уведомления
				case static_cast <uint8_t> (settings_t::ENABLE_PUSH):
					// Устанавливаем разрешение присылать push-уведомления
					local.enablePush = item.second;
				break;
				// Если мы получили максимальный размер фрейма
				case static_cast <uint8_t> (settings_t::FRAME_SIZE):
					// Устанавливаем максимальный размер кадра в октетах, который собеседнику разрешено отправлять
					local.maxFrameSize = item.second;
				break;
				// Если мы получили максимальный размер таблицы заголовков
				case static_cast <uint8_t> (settings_t::HEADER_TABLE_SIZE):
					// Устанавливаем максимальный размер таблицы заголовков
					local.headerTableSize = item.second;
				break;
				// Если мы получили максимальный размер окна полезной нагрузки
				case static_cast <uint8_t> (settings_t::WINDOW_SIZE):
					// Устанавливаем элемент управления потоком (flow control)
					local.windowSize = static_cast <int32_t> (item.second);
				break;
				// Если настройки соответствуют количествам доступных потоков
				case static_cast <uint8_t> (settings_t::STREAMS):
					// Устанавливаем максимальное количество потоков, которое собеседнику разрешается использовать одновременно
					local.maxConcurrentStreams = item.second;
				break;
				// Если мы получили разрешение использования метода CONNECT
				case static_cast <uint8_t> (settings_t::CONNECT):
					// Устанавливаем разрешение применения метода CONNECT
					local.enableConnectProtocol = item.second;
				break;
				// Если мы получили максимальный размер списка заголовков
				case static_cast <uint8_t> (settings_t::HEADER_LIST_SIZE):
					// Ставим максимальный размер списка заголовков (0 - без ограничения)
					this->_maxHeaderListSize = item.second;
				break;
			}
		}
		// Устанавливаем максимальный размер списка заголовков в анонсируемые параметры
		local.maxHeaderListSize = this->_maxHeaderListSize;
		// Если максимальный размер списка заголовков ограничен
		if(this->_maxHeaderListSize > 0)
			// Устанавливаем ограничение распакованного списка заголовков
			limits.maxHeadersTotal = this->_maxHeaderListSize;
		// Передаём парсеру лимиты безопасности
		this->_engine->limits(limits);
		// Передаём парсеру параметры SETTINGS
		this->_engine->settings(local);
		// Ищем максимальный размер буфера полезной нагрузки
		auto payloadSize = settings.find(settings_t::PAYLOAD_SIZE);
		// Если параметр настроек максимального размера буфера полезной нагрузки установлен
		if((payloadSize != settings.end()) && (payloadSize->second > 0))
			// Ставим размер приёмного окна соединения
			this->_engine->connectionReceiveWindow(static_cast <int32_t> (payloadSize->second));
		/**
		 * Устанавливаем функции обратного вызова парсера
		 */
		{
			// Функция обратного вызова фазы приёма сообщения потока
			this->_engine->on(http::parser_http2_t::phase_callback_t([this](const uint32_t sid, const http::parser_t::phase_t phase, const http::parser_t::part_t part) noexcept -> bool {
				// Если сервис идентифицирован как сервер и начинается приём сообщения
				if((this->_mode == mode_t::SERVER) && (phase == http::parser_t::phase_t::BEGIN) && (part == http::parser_t::part_t::NONE)){
					// Отправляем список разрешённых источников
					this->sendOrigin();
					// Отправляем список альтернативных сервисов
					this->sendAltSvc(static_cast <int32_t> (sid));
				}
				// Если начинается приём сообщения потока
				if((phase == http::parser_t::phase_t::BEGIN) && (part == http::parser_t::part_t::NONE) && this->_callback.is("begin")){
					// Если функция обратного вызова потребовала прервать поток
					if(this->_callback.call <int32_t (const int32_t)> ("begin", static_cast <int32_t> (sid)) != 0)
						// Выводим результат
						return false;
				}
				// Выводим результат
				return true;
			}));
			// Функция обратного вызова полученного заголовка
			this->_engine->on(http::parser_http2_t::header_callback_t([this]([[maybe_unused]] const uint32_t sid, const string_view name, const string_view value, [[maybe_unused]] const http::parser_t::part_t part) noexcept -> bool {
				// Если функция обратного вызова установлена
				if(this->_callback.is("header") && (this->_callback.call <int32_t (const int32_t, const string &, const string &)> ("header", static_cast <int32_t> (sid), string(name), string(value)) != 0))
					// Выводим результат
					return false;
				// Выводим результат
				return true;
			}));
			// Функция обратного вызова завершения блока заголовков
			this->_engine->on(http::parser_http2_t::provider_callback_t([this](const uint32_t sid, [[maybe_unused]] const http::provider_t * provider, const bool endStream) noexcept -> bool {
				// Флаги полученного фрейма
				std::set <flag_t> flags;
				// Ставим флаг завершения заголовков
				flags.emplace(flag_t::END_HEADERS);
				// Если поток завершён
				if(endStream)
					// Ставим флаг завершения потока
					flags.emplace(flag_t::END_STREAM);
				// Вызываем функцию обратного вызова фрейма заголовков
				return (this->fire(static_cast <int32_t> (sid), direct_t::RECV, frame_t::HEADERS, flags) == 0);
			}));
			// Функция обратного вызова полученных данных тела
			this->_engine->on(http::parser_http2_t::data_callback_t([this](const uint32_t sid, const void * buffer, const size_t size, const bool endStream) noexcept -> bool {
				// Если функция обратного вызова установлена
				if(this->_callback.is("chunk") && (this->_callback.call <int32_t (const int32_t, const uint8_t *, const size_t)> ("chunk", static_cast <int32_t> (sid), reinterpret_cast <const uint8_t *> (buffer), size) != 0))
					// Выводим результат
					return false;
				// Флаги полученного фрейма
				std::set <flag_t> flags;
				// Если поток завершён
				if(endStream)
					// Ставим флаг завершения потока
					flags.emplace(flag_t::END_STREAM);
				// Вызываем функцию обратного вызова фрейма данных
				return (this->fire(static_cast <int32_t> (sid), direct_t::RECV, frame_t::DATA, flags) == 0);
			}));
			// Функция обратного вызова превышения лимита тела потока
			this->_engine->on(http::parser_http2_t::limit_callback_t([this](const uint32_t sid, const uint64_t received, const uint64_t limit) noexcept {
				// Если функция обратного вызова установлена
				if(this->_callback.is("limit"))
					// Вызываем функцию обратного вызова
					this->_callback.call <void (const int32_t, const uint64_t, const uint64_t)> ("limit", static_cast <int32_t> (sid), received, limit);
			}));
			// Функция обратного вызова закрытия потока
			this->_engine->on(http::parser_http2_t::close_callback_t([this](const uint32_t sid, const http::h2::error_t code) noexcept {
				// Помечаем буферы потока на удаление
				this->release(static_cast <int32_t> (sid));
				// Если поток сброшен ошибкой
				if(code != http::h2::error_t::NO_ERROR){
					// Флаги полученного фрейма
					std::set <flag_t> flags;
					// Вызываем функцию обратного вызова фрейма сброса потока
					this->fire(static_cast <int32_t> (sid), direct_t::RECV, frame_t::RST_STREAM, flags);
				}
				// Если функция обратного вызова установлена
				if(this->_callback.is("close"))
					// Вызываем функцию обратного вызова
					this->_callback.call <int32_t (const int32_t, const error_t)> ("close", static_cast <int32_t> (sid), http2_t::error(static_cast <uint32_t> (code)));
			}));
			// Функция обратного вызова ошибки уровня соединения
			this->_engine->on(http::parser_http2_t::error_callback_t([this]([[maybe_unused]] const http::h2::error_t code, const string_view message) noexcept {
				// Получаем текст ошибки
				const string error(message);
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.c_str());
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Вызываем функцию обратного вызова
					this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error);
			}));
			// Функция обратного вызова применённых параметров SETTINGS пира
			this->_engine->on(http::parser_http2_t::settings_callback_t([this]() noexcept {
				// Флаги полученного фрейма
				std::set <flag_t> flags;
				// Вызываем функцию обратного вызова фрейма настроек
				this->fire(0, direct_t::RECV, frame_t::SETTINGS, flags);
				// Если сервис идентифицирован как сервер
				if(this->_mode == mode_t::SERVER)
					// Отправляем список альтернативных сервисов
					this->sendAltSvc(0);
			}));
			// Функция обратного вызова полученного GOAWAY
			this->_engine->on(http::parser_http2_t::goaway_callback_t([this](const uint32_t sid, [[maybe_unused]] const http::h2::error_t code, [[maybe_unused]] const string_view debug) noexcept {
				// Флаги полученного фрейма
				std::set <flag_t> flags;
				// Вызываем функцию обратного вызова фрейма закрытия всех потоков
				this->fire(static_cast <int32_t> (sid), direct_t::RECV, frame_t::GOAWAY, flags);
			}));
			// Функция обратного вызова анонса server push
			this->_engine->on(http::parser_http2_t::push_callback_t([this](const uint32_t sid, [[maybe_unused]] const uint32_t promised) noexcept -> bool {
				// Если функция обратного вызова установлена
				if(this->_callback.is("begin") && (this->_callback.call <int32_t (const int32_t)> ("begin", static_cast <int32_t> (sid)) != 0))
					// Выводим результат
					return false;
				// Флаги полученного фрейма
				std::set <flag_t> flags;
				// Ставим флаг завершения заголовков
				flags.emplace(flag_t::END_HEADERS);
				// Вызываем функцию обратного вызова фрейма push-уведомления
				return (this->fire(static_cast <int32_t> (sid), direct_t::RECV, frame_t::PUSH_PROMISE, flags) == 0);
			}));
			// Функция обратного вызова полученного ALTSVC
			this->_engine->on(http::parser_http2_t::altsvc_callback_t([this](const uint32_t sid, const string_view origin, const string_view value) noexcept {
				// Если функция обратного вызова установлена
				if(this->_callback.is("altsvc"))
					// Вызываем функцию обратного вызова
					this->_callback.call <void (const string &, const string &)> ("altsvc", string(origin), string(value));
				// Флаги полученного фрейма
				std::set <flag_t> flags;
				// Вызываем функцию обратного вызова фрейма альтернативного сервиса
				this->fire(static_cast <int32_t> (sid), direct_t::RECV, frame_t::ALTSVC, flags);
			}));
			// Функция обратного вызова полученного ORIGIN
			this->_engine->on(http::parser_http2_t::origin_callback_t([this](const string_view origin) noexcept {
				// Если функция обратного вызова установлена
				if(this->_callback.is("origin")){
					// Создаём список полученных ресурсов
					vector <string> origins;
					// Добавляем полученный ресурс в список
					origins.emplace_back(origin);
					// Вызываем функцию обратного вызова
					this->_callback.call <void (const vector <string> &)> ("origin", origins);
				}
				// Флаги полученного фрейма
				std::set <flag_t> flags;
				// Вызываем функцию обратного вызова фрейма доверенных источников
				this->fire(0, direct_t::RECV, frame_t::ORIGIN, flags);
			}));
			// Функция обратного вызова готовности потока принимать данные
			this->_engine->on(http::parser_http2_t::writable_callback_t([this](const uint32_t sid) noexcept {
				// Отправляем накопленную полезную нагрузку потока
				this->pump(static_cast <int32_t> (sid));
				// Отправляем исходящие байты в сеть
				this->flush();
			}));
		}
		/**
		 * Отправляем preface соединения: клиент - magic-строку вместе с SETTINGS,
		 * сервер - только SETTINGS
		 */
		this->_engine->sendPreface();
		// Отправляем исходящие байты в сеть
		this->flush();
		// Выводим результат
		result = true;
	}
	// Выводим результат
	return result;
}
/**
 * @brief Оператор [=] зануления фрейма Http2
 *
 * @return сформированный объект Http2
 *
 */
awh::Http2 & awh::Http2::operator = (nullptr_t) noexcept {
	// Очищаем активную сессию
	this->free();
	// Выводим текущий объект в качестве результата
	return (* this);
}
/**
 * @brief Оператор [=] копирования объекта фрейма Http2
 *
 * @param ctx объект фрейма Http2
 * @return    сформированный объект Http2
 *
 */
awh::Http2 & awh::Http2::operator = ([[maybe_unused]] const http2_t & ctx) noexcept {
	/**
	 * Состояние соединения HTTP/2 не копируется: парсер владеет таблицами HPACK,
	 * потоками и окнами, и второй владелец того же состояния недопустим
	 */
	// Выводим текущий объект в качестве результата
	return (* this);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::Http2::Http2(const fmk_t * fmk, const log_t * log) noexcept :
 _close(false), _wouldblock(false), _pumping(false), _failed(false), _mode(mode_t::NONE), _event(event_t::NONE), _depth(0),
 _maxHeaderListSize(0), _socket(fmk, log), _callback(log), _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::Http2::~Http2() noexcept {
	// Освобождаем парсер соединения
	this->free();
}
