/**
 * @file: frame.cpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

// Подключаем заголовочный файл
#include <ws/frame.hpp>

/**
 * head Метод извлечения заголовка фрейма
 * @param head   объект для извлечения заголовка
 * @param buffer буфер с данными заголовка
 * @param size   размер передаваемого буфера
 */
void awh::Frame::head(head_t & head, const char * buffer, const size_t size) const noexcept {
	// Если данные переданы
	if((buffer != nullptr) && (size >= 2)){
		// Получаем наличие маски
		head.mask = (buffer[1] & 0x80);
		// Получаем малый размер полезной нагрузки
		head.payload = (buffer[1] & 0x7F);
		// Определяем является ли сообщение последним
		head.fin = (buffer[0] & 0x80);
		// Определяем байты расширенного протокола
		head.rsv[0] = (buffer[0] & 0x40);
		head.rsv[1] = (buffer[0] & 0x20);
		head.rsv[2] = (buffer[0] & 0x10);
		// Получаем опкод
		head.optcode = opcode_t(buffer[0] & 0x0F);
		// Если размер пересылаемых данных, имеет малый размер
		if(head.payload < 0x7E)
			// Получаем размер блока заголовков
			head.size = 2;
		// Если размер пересылаемых данных, имеет более высокий размер
		else if((head.payload == 0x7E) && (size >= 4)) {
			// Получаем размер блока заголовков
			head.size = 4;
			// Размер полезной нагрузки
			uint16_t size = 0;
			// Получаем размер данных
			memcpy(&size, buffer + 2, sizeof(size));
			// Преобразуем сетевой порядок расположения байтов
			head.payload = ntohs(size);
		// Если размер пересылаемых данных, имеет очень большой размер
		} else if((head.payload == 0x7F) && (size >= 10)) {
			// Получаем размер блока заголовков
			head.size = 10;
			// Получаем размер данных
			memcpy(&head.payload, buffer + 2, sizeof(head.payload));
			// Преобразуем сетевой порядок расположения байтов
			head.payload = ntohl(head.payload);
		}
	}
}
/**
 * frame Функция создания бинарного фрейма
 * @param payload бинарный буфер фрейма
 * @param buffer  бинарные данные полезной нагрузки
 * @param size    размер передаваемого буфера
 * @param mask    флаг выполнения маскировки сообщения
 */
void awh::Frame::frame(vector <char> & payload, const char * buffer, const size_t size, const bool mask) const noexcept {
	// Если данные переданы
	if(!payload.empty() && (buffer != nullptr) && (size > 0)){
		// Размер смещения в буфере и размер передаваемых данных
		uint8_t offset = 0;
		// Если размер строки меньше 126 байт, значит строка умещается во второй байт
		if(size < 0x7E){
			// Устанавливаем смещение в буфере
			offset = 2;
			// Устанавливаем размер строки
			payload.back() = ((char) (mask ? 0x80 : 0x0) | (0x7F & size));
		// Если строка не помещается во второй байт
		} else if(size < 0x10000) {
			// Устанавливаем смещение в буфере
			offset = 4;
			// Заполняем второй байт максимальным значением
			payload.back() = ((char) (mask ? 0x80 : 0x0) | (0x7F & 0x7E));
			// Увеличиваем память ещё на два байта
			payload.resize(offset, 0x0);
			// Выполняем перерасчёт размера передаваемых данных
			const uint16_t length = htons((uint16_t) size);
			// Устанавливаем размер строки в следующие 2 байта
			memcpy(payload.data() + 2, &length, sizeof(length));
		// Если сообщение очень большого размера
		} else {
			// Устанавливаем смещение в буфере
			offset = 10;
			// Заполняем второй байт максимальным значением
			payload.back() = ((char) (mask ? 0x80 : 0x0) | 0x7F);
			// Увеличиваем память ещё на восемь байт
			payload.resize(offset, 0x0);
			// Выполняем перерасчёт размера передаваемых данных
			const uint64_t length = htonl(size);
			// Устанавливаем размер строки в следующие 8 байт
			memcpy(payload.data() + 2, &length, sizeof(length));
		}
		// Если нужно выполнить маскировку сообщения
		if(mask){
			// Получаем генератор случайных чисел
			random_device device;
			// Бинарные данные маски
			vector <u_char> mask(4);
			// Подключаем генератор к двигателю
			mt19937 engine {device()};
			// Устанавливаем диапазон генератора случайных чисел
			uniform_int_distribution <u_char> dist {0, 255};
			/**
			 * generatorFn Функция генерации случайных чисел
			 */
			auto generatorFn = [&dist, &engine]{
				// Выполняем генерирование случайного числа
				return dist(engine);
			};
			// Выполняем заполнение маски случайными числами
			generate(mask.begin(), mask.end(), generatorFn);
			// Выполняем перебор всех байт передаваемых данных
			for(size_t i = 0; i < size; i++){
				// Выполняем шифрование данных
				const_cast <char *> (buffer)[i] ^= mask[i % 4];
			}
			// Увеличиваем память ещё на четыре байта
			payload.resize(offset + 4, 0x0);
			// Устанавливаем сгенерированную маску
			memcpy(payload.data() + offset, mask.data(), mask.size());
		}
		// Выполняем копирования оставшихся данных в буфер
		payload.insert(payload.end(), buffer, buffer + size);
	}
}
/**
 * message Метод создание фрейма сообщения
 * @param mess данные сообщения
 * @return     бинарные данные фрейма
 */
vector <char> awh::Frame::message(const mess_t & mess) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если сообщение передано
	if(mess.code > 0){
		// Увеличиваем память на 4 байта
		result.resize(4, 0x0);
		// Устанавливаем первый байт
		result.front() = ((char) 0x80 | (0x0F & (u_char) opcode_t::CLOSE));
		// Размер смещения в буфере
		uint16_t offset = 0;
		// Размер передаваемых данных
		uint64_t size = mess.text.size();
		// Если размер строки меньше 126 байт, значит строка умещается во второй байт
		if(size < 0x7E){
			// Устанавливаем смещение в буфере
			offset = 2;
			// Устанавливаем размер строки
			result.at(1) = ((char) (0x7F & (size + 2)));
		// Если строка не помещается во второй байт
		} else if(size < 0x10000) {
			// Устанавливаем смещение в буфере
			offset = 4;
			// Увеличиваем память ещё на два байта
			result.resize(offset + 2, 0x0);
			// Заполняем второй байт максимальным значением
			result.at(1) = ((char) (0x7F & 0x7E));
			// Выполняем перерасчёт размера передаваемых данных
			const uint16_t length = htons((uint16_t) (size + 2));
			// Устанавливаем размер строки в следующие 2 байта
			memcpy(result.data() + 2, &length, sizeof(length));
		}
		// Получаем код сообщения
		const uint16_t code = htons(mess.code);
		// Устанавливаем код сообщения
		memcpy(result.data() + offset, &code, sizeof(code));
		// Выполняем копирования оставшихся данных в буфер
		if(!mess.text.empty()) result.insert(result.end(), mess.text.begin(), mess.text.end());
	}
	// Выводим результат
	return result;
}
/**
 * message Метод извлечения сообщения из фрейма
 * @param buffer бинарные данные сообщения
 * @return       сообщение в текстовом виде
 */
awh::mess_t awh::Frame::message(const vector <char> & buffer) const noexcept {
	// Результат работы функции
	mess_t result;
	// Если данные переданы
	if(buffer.size() >= sizeof(result.code)){
		/**
		 * Подробнее: https://github.com/Luka967/websocket-close-codes
		 */
		// Считываем код ошибки
		memcpy(&result.code, buffer.data(), sizeof(result.code));
		// Преобразуем сетевой порядок расположения байтов
		result = ntohs(result.code);
		// Если коды ошибок соответствуют
		if((result.code > 0) && (result.code <= 4999)){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Если текст сообщения существует
				if(buffer.size() > sizeof(result.code))
					// Извлекаем текст сообщения
					result.text.assign(buffer.begin() + sizeof(result.code), buffer.end());
			// Выполняем прехват ошибки
			} catch(const exception & error) {
				// Выводим в лог сообщение
				this->log->print("%s", log_t::flag_t::CRITICAL, error.what());
				// Устанавливаем текст ошибки
				result = this->fmk->format("%s", error.what());
			}
		// Запоминаем размер смещения
		} else result = 1007;
	}
	// Выводим результат
	return result;
}
/**
 * ping Метод создания фрейма пинга
 * @param mess данные сообщения
 * @param mask флаг выполнения маскировки сообщения
 * @return     бинарные данные фрейма
 */
vector <char> awh::Frame::ping(const string & mess, const bool mask) const noexcept {
	// Создаём тело запроса и устанавливаем первый байт PING с пустой полезной нагрузкой
	vector <char> result = {(char) 0x80 | (0x0F & (char) opcode_t::PING), 0x0};
	// Если сообщение передано
	if(!mess.empty())
		// Выполняем формирование фрейма
		this->frame(result, mess.data(), mess.size(), mask);
	// Выводим результат
	return result;
}
/**
 * pong Метод создания фрейма понга
 * @param mess данные сообщения
 * @param mask флаг выполнения маскировки сообщения
 * @return     бинарные данные фрейма
 */
vector <char> awh::Frame::pong(const string & mess, const bool mask) const noexcept {
	// Создаём тело запроса и устанавливаем первый байт PONG с пустой полезной нагрузкой
	vector <char> result = {(char) 0x80 | (0x0F & (char) opcode_t::PONG), 0x0};
	// Если сообщение передано
	if(!mess.empty())
		// Выполняем формирование фрейма
		this->frame(result, mess.data(), mess.size(), mask);
	// Выводим результат
	return result;
}
/**
 * get Метод извлечения данных фрейма
 * @param head   заголовки фрейма
 * @param buffer бинарные данные фрейма для извлечения
 * @param size   размер передаваемого буфера
 * @return       бинарные данные полезной нагрузки
 */
vector <char> awh::Frame::get(head_t & head, const char * buffer, const size_t size) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Выполняем чтение заголовков
	this->head(head, buffer, size);
	// Если данные переданы в достаточном объёме
	if((buffer != nullptr) && ((size_t) (head.payload + head.size) <= size)){
		// Получаем размер смещения
		uint8_t offset = head.size;
		// Проверяем являются ли данные Пингом
		const bool isPing = (head.optcode == opcode_t::PING);
		// Проверяем являются ли данные Понгом
		const bool isPong = (head.optcode == opcode_t::PONG);
		// Проверяем являются ли данные текстовыми
		const bool isText = (head.optcode == opcode_t::TEXT);
		// Проверяем являются ли данные бинарными
		const bool isBin = (head.optcode == opcode_t::BINARY);
		// Проверяем является ли сообщение закрытием
		const bool isMess = (head.optcode == opcode_t::CLOSE);
		// Проверяем является ли сообщение продолжением фрагментированного текста
		const bool isContinue = (head.optcode == opcode_t::CONTINUATION);
		// Если входящие данные не являются мусоромы
		if(isText || isBin || isPing || isPong || isMess || isContinue){
			// Бинарные данные маски
			vector <u_char> mask(4);
			// Если маска требуется, маскируем данные
			if(head.mask){
				// Считываем ключ маски
				memcpy(mask.data(), buffer + offset, 4);
				// Увеличиваем размер смещения
				offset += 4;
			}
			// Получаем оставшиеся данные полезной нагрузки
			result.assign(buffer + offset, buffer + (head.payload + offset));
			// Если маска требуется, размаскируем данные
			if(head.mask){
				// Выполняем перебор всех байт передаваемых данных
				for(size_t i = 0; i < result.size(); i++){
					// Выполняем шифрование данных
					result.at(i) ^= mask[i % 4];
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * set Метод создания данных фрейма
 * @param head   заголовки фрейма
 * @param buffer бинарные данные полезной нагрузки
 * @param size   размер передаваемого буфера
 * @return       бинарные данные фрейма
 */
vector <char> awh::Frame::set(const head_t & head, const char * buffer, const size_t size) const noexcept {
	/**
	 * rsv[0] должен быть установлен в TRUE для первого сообщения в GZIP,
	 * и установлен в FALSE для всех остальных сообщений, в рамках одной сессии
	 */
	vector <char> result = {
		(
			char(
				(head.fin ? 0x80 : 0x0) |
				(head.rsv[0] ? 0x40 : 0x0) |
				(head.rsv[1] ? 0x20 : 0x0) |
				(head.rsv[2] ? 0x10 : 0x0) |
				(0x0F & (int) head.optcode)
			)
		), 0x0
	};
	// Если данные переданы
	if((buffer != nullptr) && (size > 0))
		// Выполняем формирование фрейма
		this->frame(result, buffer, size, head.mask);
	// Выводим результат
	return result;
}
