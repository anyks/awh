/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <ws/frame.hpp>

/**
 * Устанавливаем шаблон функции
 */
template <typename T>
/**
 * frame Функция создания бинарного фрейма
 * @param payload бинарный буфер фрейма
 * @param buffer  данные сообщения
 * @param mask    флаг выполнения маскировки сообщения
 */
void frame(vector <char> & payload, T buffer, bool mask) noexcept {
	// Если данные переданы
	if(!payload.empty() && !buffer.empty()){
		// Размер смещения в буфере и размер передаваемых данных
		size_t offset = 0, size = buffer.size();
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
			const u_short length = htons((u_short) size);
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
			const size_t length = htonl(size);
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
			generate(begin(mask), end(mask), generatorFn);
			// Выполняем перебор всех байт передаваемых данных
			for(size_t i = 0; i < buffer.size(); i++){
				// Выполняем шифрование данных
				buffer.at(i) ^= mask[i % 4];
			}
			// Увеличиваем память ещё на четыре байта
			payload.resize(offset + 4, 0x0);
			// Устанавливаем сгенерированную маску
			memcpy(payload.data() + offset, mask.data(), mask.size());
		}
		// Выполняем копирования оставшихся данных в буфер
		payload.insert(payload.end(), buffer.begin(), buffer.end());
	}
}
/**
 * head Метод извлечения заголовка фрейма
 * @param buffer буфер с данными заголовка
 * @return       данные заголовка фрейма
 */
awh::Frame::head_t awh::Frame::head(const vector <char> & buffer) const noexcept {
	// Результат работы функции
	head_t result;
	// Если размер буфера составляет два байта
	if(buffer.size() == 2){
		// Получаем малый размер передаваемых данных
		result.size = (buffer.back() & 0x7F);
		// Получаем наличие маски
		result.mask = (buffer.back() & 0x80);
		// Определяем является ли сообщение последним
		result.fin = (buffer.front() & 0x80);
		// Определяем байты расширенного протокола
		result.rsv[0] = (buffer.front() & 0x40);
		result.rsv[1] = (buffer.front() & 0x20);
		result.rsv[2] = (buffer.front() & 0x10);
		// Получаем опкод
		result.optcode = opcode_t(buffer.front() & 0x0F);
		// Если размер пересылаемых данных, имеет более высокий размер
		if(result.size == 0x7E)
			// Устанавливаем размер смещения
			result.offset = 2;
		// Если размер пересылаемых данных, имеет очень большой размер
		else if(result.size == 0x7F)
			// Устанавливаем размер смещения
			result.offset = 8;
	}
	// Выводим результат
	return result;
}
/**
 * size Метод получения размера фрейма
 * @param head   заголовки фрейма
 * @param buffer буфер с данными фрейма
 * @return       размер полезной нагрузки фрейма
 */
uint8_t awh::Frame::size(const head_t & head, const vector <char> & buffer) const noexcept {
	// Результат работы функции
	uint8_t result = 0;
	// Если размер фрейма передан
	if(head.size > 0){
		// Если смещение в буфере нулевое
		if(head.offset == 0)
			// Запоминаем размер данных
			result = head.size;
		// Если заголовок фрейма и буфер данных переданы
		else if(buffer.size() == head.offset){
			// Если размер полезной нагрузки умещается в 2 байта
			if(head.offset == 2){
				// Размер полезной нагрузки
				u_short size = 0;
				// Получаем полезной нагрузки
				memcpy(&size, buffer.data(), sizeof(size));
				// Преобразуем сетевой порядок расположения байтов
				result = ntohs(size);
			// Если размер полезной нагрузки умещается в 8 байт
			} else if(head.offset == 8) {
				// Получаем размер данных
				memcpy(&result, buffer.data(), sizeof(result));
				// Преобразуем сетевой порядок расположения байтов
				result = ntohl(result);
			}
		}
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
	vector <char> result = {(u_char) 0x80 | (0x0F & (u_char) opcode_t::PING), 0x0};
	// Если сообщение передано
	if(!mess.empty()) frame(result, mess, mask);
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
	vector <char> result = {(u_char) 0x80 | (0x0F & (u_char) opcode_t::PONG), 0x0};
	// Если сообщение передано
	if(!mess.empty()) frame(result, mess, mask);
	// Выводим результат
	return result;
}
/**
 * get Метод извлечения данных фрейма
 * @param head   заголовки фрейма
 * @param buffer бинарные данные фрейма для извлечения (Без учёта заголовков и байтов размера)
 * @return       бинарные данные полезной нагрузки
 */
vector <char> awh::Frame::get(const head_t & head, const vector <char> & buffer) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если данные переданы
	if((head.size > 0) && !buffer.empty()){
		// Проверяем являются ли данные Пингом
		const bool isPing = (head.optcode == opcode_t::PING);
		// Проверяем являются ли данные Понгом
		const bool isPong = (head.optcode == opcode_t::PONG);
		// Проверяем являются ли данные текстовыми
		const bool isText = (head.optcode == opcode_t::TEXT);
		// Проверяем являются ли данные бинарными
		const bool isBin = (head.optcode == opcode_t::BINARY);
		// Если входящие данные не являются мусоромы
		if(isText || isBin || isPing || isPong){
			// Смещение в буфере
			size_t offset = 0;
			// Бинарные данные маски
			vector <u_char> mask(4);
			// Если маска требуется, маскируем данные
			if(head.mask){
				// Считываем ключ маски
				memcpy(mask.data(), buffer.data(), 4);
				// Устанавливаем размер смещения
				offset = 4;
			}
			// Получаем оставшиеся данные полезной нагрузки
			result.assign(buffer.begin() + offset, buffer.end());
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
 * @return       бинарные данные фрейма
 */
vector <char> awh::Frame::set(const head_t & head, const vector <char> & buffer) const noexcept {
	/**
	 * rsv[0] должен быть установлен в TRUE для первого сообщения в GZIP,
	 * и установлен в FALSE для всех остальных сообщений, в рамках одной сессии
	 */
	// Результат работы функции
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
	if(!buffer.empty()) frame(result, buffer, head.mask);
	// Выводим результат
	return result;
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
	if((mess.code > 0) && !mess.text.empty()){
		// Размер смещения в буфере и размер передаваемых данных
		size_t offset = 0, size = mess.text.size();
		// Увеличиваем память на 4 байта
		result.resize(4, 0x0);
		// Устанавливаем первый байт
		result.front() = ((char) 0x80 | (0x0F & (u_char) opcode_t::CLOSE));
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
			const u_short length = htons((u_short) (size + 2));
			// Устанавливаем размер строки в следующие 2 байта
			memcpy(result.data() + 2, &length, sizeof(length));
		}
		// Получаем код сообщения
		const u_short code = htons(mess.code);
		// Устанавливаем код сообщения
		memcpy(result.data() + offset, &code, sizeof(code));
		// Выполняем копирования оставшихся данных в буфер
		result.insert(result.end(), mess.text.begin(), mess.text.end());
	}
	// Выводим результат
	return result;
}
/**
 * message Метод извлечения сообщения из фрейма
 * @param head   заголовки фрейма
 * @param buffer бинарные данные фрейма для извлечения (Без учёта заголовков и байтов размера)
 * @return       сообщение в текстовом виде
 */
awh::Frame::mess_t awh::Frame::message(const head_t & head, const vector <char> & buffer) const noexcept {
	// Результат работы функции
	mess_t result;
	// Если данные переданы
	if((buffer.size() >= 2) && (head.optcode == opcode_t::CLOSE)){
		/**
		 * Коды ошибок: https://github.com/Luka967/websocket-close-codes
		 */
		// Считываем код ошибки
		memcpy(&result.code, buffer.data(), sizeof(result.code));
		// Преобразуем сетевой порядок расположения байтов
		result.code = ntohs(result.code);
		// Если коды ошибок соответствуют
		if((result.code > 0) && (result.code <= 4999)){
			// Выполняем перехват ошибки
			try {
				// Извлекаем текст сообщения
				if(buffer.size() > 2) result.text.assign(buffer.begin() + sizeof(result.code), buffer.end());
			// Выполняем прехват ошибки
			} catch(const exception & error) {
				// Выводим в лог сообщение
				this->fmk->log("%s", fmk_t::log_t::CRITICAL, this->logfile, error.what());
				// Устанавливаем текст ошибки
				this->mess.text = this->fmk->format("%s", error.what());
			}
		// Запоминаем размер смещения
		} else {
			// Устанавливаем код сообщения
			this->mess.code = 1006;
			// Устанавливаем текст ошибки
			this->mess.text = this->fmk->format("%s", "no close code frame has been receieved");
		}
	}
	// Выводим результат
	return result;
}
