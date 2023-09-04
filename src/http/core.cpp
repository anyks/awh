/**
 * @file: core.cpp
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
#include <http/core.hpp>

/**
 * chunkingCallback Функция вывода полученных чанков полезной нагрузки
 * @param buffer буфер данных чанка полезной нагрузки
 * @param web    объект HTTP парсера
 */
void awh::Http::chunkingCallback(const vector <char> & buffer, const web_t * web) noexcept {
	// Если функция обратного вызова передана
	if(this->_fn != nullptr)
		// Выполняем сборку бинарных чанков
		this->_fn(buffer, this);
}
/**
 * commit Метод применения полученных результатов
 */
void awh::Http::commit() noexcept {
	// Выполняем проверку авторизации
	this->stath = this->checkAuth();
	// Если ключ соответствует
	if(this->stath == stath_t::GOOD)
		// Устанавливаем стейт рукопожатия
		this->state = state_t::GOOD;
	// Поменяем данные как бракованные
	else this->state = state_t::BROKEN;
	// Получаем данные тела
	const auto & body = this->web.body();
	// Если тело сообщения получено
	if(!body.empty()){
		// Получаем заголовок шифрования
		const string & encrypt = this->web.header("x-awh-encryption");
		// Если заголовок найден
		if((this->crypt = !encrypt.empty())){
			// Определяем размер шифрования
			switch(stoi(encrypt)){
				// Если шифрование произведено 128 битным ключём
				case 128: this->hash.cipher(hash_t::cipher_t::AES128); break;
				// Если шифрование произведено 192 битным ключём
				case 192: this->hash.cipher(hash_t::cipher_t::AES192); break;
				// Если шифрование произведено 256 битным ключём
				case 256: this->hash.cipher(hash_t::cipher_t::AES256); break;
			}
			// Выполняем дешифрование полученных данных
			const auto & result = this->hash.decrypt(body.data(), body.size());
			// Заменяем полученное тело
			if(!result.empty()){
				// Очищаем тело сообщения
				this->clearBody();
				// Формируем новое тело сообщения
				this->web.body(result);
			}
		}
		// Проверяем пришли ли сжатые данные
		const string & encoding = this->web.header("content-encoding");
		// Если данные пришли сжатые
		if(!encoding.empty()){
			// Если данные пришли сжатые методом Brotli
			if(this->fmk->compare(encoding, "br")){
				// Устанавливаем требование выполнять декомпрессию тела сообщения
				this->_compress = compress_t::BROTLI;
				// Выполняем декомпрессию данных
				const auto & result = this->hash.decompress(body.data(), body.size(), hash_t::method_t::BROTLI);
				// Заменяем полученное тело
				if(!result.empty()){
					// Очищаем тело сообщения
					this->clearBody();
					// Формируем новое тело сообщения
					this->web.body(result);
				}
			// Если данные пришли сжатые методом GZip
			} else if(this->fmk->compare(encoding, "gzip")) {
				// Устанавливаем требование выполнять декомпрессию тела сообщения
				this->_compress = compress_t::GZIP;
				// Выполняем декомпрессию данных
				const auto & result = this->hash.decompress(body.data(), body.size(), hash_t::method_t::GZIP);
				// Заменяем полученное тело
				if(!result.empty()){
					// Очищаем тело сообщения
					this->clearBody();
					// Формируем новое тело сообщения
					this->web.body(result);
				}
			// Если данные пришли сжатые методом Deflate
			} else if(this->fmk->compare(encoding, "deflate")) {
				// Устанавливаем требование выполнять декомпрессию тела сообщения
				this->_compress = compress_t::DEFLATE;
				// Получаем данные тела в бинарном виде
				vector <char> buffer(body.begin(), body.end());
				// Добавляем хвост в полученные данные
				this->hash.setTail(buffer);
				// Выполняем декомпрессию данных
				const auto & result = this->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::DEFLATE);
				// Заменяем полученное тело
				if(!result.empty()){
					// Очищаем тело сообщения
					this->clearBody();
					// Формируем новое тело сообщения
					this->web.body(result);
				}
			// Отключаем сжатие тела сообщения
			} else this->_compress = compress_t::NONE;
		// Отключаем сжатие тела сообщения
		} else this->_compress = compress_t::NONE;
	}
	// Если мы работаем с HTTP сервером и метод компрессии установлен
	if((this->httpType == web_t::hid_t::SERVER) && (this->_compress != compress_t::NONE)){
		// Список запрашиваемых методов
		set <compress_t> compress;
		// Если заголовок с запрашиваемой кодировкой существует
		if(this->web.isHeader("accept-encoding")){
			// Переходим по всему списку заголовков
			for(auto & header : this->web.headers()){
				// Если заголовок найден
				if(this->fmk->compare(header.first, "accept-encoding")){
					// Если конкретный метод сжатия не запрашивается
					if(this->fmk->compare(header.second, "*")) break;
					// Если запрашиваются конкретные методы сжатия
					else {
						// Если найден запрашиваемый метод компрессии BROTLI
						if(header.second.find("br") != string::npos){
							// Запоминаем запрашиваемый метод компрессии BROTLI
							compress.emplace(compress_t::BROTLI);
							// Если уже в списке существует метод компрессии GZIP
							if(compress.count(compress_t::GZIP) > 0){
								// Устанавливаем метод компрессии GZIP, BROTLI
								compress.emplace(compress_t::GZIP_BROTLI);
								// Если уже в списке существует метод компрессии DEFLATE
								if(compress.count(compress_t::DEFLATE) > 0)
									// Устанавливаем метод компрессии GZIP, DEFLATE, BROTLI
									compress.emplace(compress_t::ALL_COMPRESS);
							// Если уже в списке существует метод компрессии DEFLATE
							} else if(compress.count(compress_t::DEFLATE) > 0)
								// Устанавливаем метод компрессии DEFLATE, BROTLI
								compress.emplace(compress_t::DEFLATE_BROTLI);
						}
						// Если найден запрашиваемый метод компрессии GZip
						if(header.second.find("gzip") != string::npos){
							// Запоминаем запрашиваемый метод компрессии GZip
							compress.emplace(compress_t::GZIP);
							// Если уже в списке существует метод компрессии BROTLI
							if(compress.count(compress_t::BROTLI) > 0){
								// Устанавливаем метод компрессии GZIP, BROTLI
								compress.emplace(compress_t::GZIP_BROTLI);
								// Если уже в списке существует метод компрессии DEFLATE
								if(compress.count(compress_t::DEFLATE) > 0)
									// Устанавливаем метод компрессии GZIP, DEFLATE, BROTLI
									compress.emplace(compress_t::ALL_COMPRESS);
							// Если уже в списке существует метод компрессии DEFLATE
							} else if(compress.count(compress_t::DEFLATE) > 0)
								// Устанавливаем метод компрессии GZIP, DEFLATE
								compress.emplace(compress_t::GZIP_DEFLATE);
						}
						// Если найден запрашиваемый метод компрессии Deflate
						if(header.second.find("deflate") != string::npos){
							// Запоминаем запрашиваемый метод компрессии Deflate
							compress.emplace(compress_t::DEFLATE);
							// Если уже в списке существует метод компрессии BROTLI
							if(compress.count(compress_t::BROTLI) > 0){
								// Устанавливаем метод компрессии DEFLATE, BROTLI
								compress.emplace(compress_t::DEFLATE_BROTLI);
								// Если уже в списке существует метод компрессии GZIP
								if(compress.count(compress_t::GZIP) > 0)
									// Устанавливаем метод компрессии GZIP, DEFLATE, BROTLI
									compress.emplace(compress_t::ALL_COMPRESS);
							// Если уже в списке существует метод компрессии GZIP
							} else if(compress.count(compress_t::GZIP) > 0)
								// Устанавливаем метод компрессии GZIP, DEFLATE
								compress.emplace(compress_t::GZIP_DEFLATE);
						}
					}
				}
			}
		}
		// Если метод компрессии сервера совпадает с выбором клиента
		if(!compress.empty()){
			// Определяем метод сжатия который поддерживает клиент
			switch(static_cast <uint8_t> (this->_compress)){
				// Если клиент поддерживает методот сжатия GZIP, BROTLI
				case static_cast <uint8_t> (compress_t::GZIP_BROTLI): {
					// Если клиент поддерживает метод компрессии BROTLI
					if(compress.count(compress_t::BROTLI) > 0)
						// Переключаем метод компрессии на BROTLI
						this->_compress = compress_t::BROTLI;
					// Если клиент поддерживает метод компрессии GZIP
					else if(compress.count(compress_t::GZIP) > 0)
						// Переключаем метод компрессии на GZIP
						this->_compress = compress_t::GZIP;
					// Отключаем поддержку сжатия на сервере
					else this->_compress = compress_t::NONE;
				} break;
				// Если клиент поддерживает методот сжатия GZIP, DEFLATE
				case static_cast <uint8_t> (compress_t::GZIP_DEFLATE): {
					// Если клиент поддерживает метод компрессии GZIP
					if(compress.count(compress_t::GZIP) > 0)
						// Переключаем метод компрессии на GZIP
						this->_compress = compress_t::GZIP;
					// Если клиент поддерживает метод компрессии DEFLATE
					else if(compress.count(compress_t::DEFLATE) > 0)
						// Переключаем метод компрессии на DEFLATE
						this->_compress = compress_t::DEFLATE;
					// Отключаем поддержку сжатия на сервере
					else this->_compress = compress_t::NONE;
				} break;
				// Если клиент поддерживает методот сжатия DEFLATE, BROTLI
				case static_cast <uint8_t> (compress_t::DEFLATE_BROTLI): {
					// Если клиент поддерживает метод компрессии BROTLI
					if(compress.count(compress_t::BROTLI) > 0)
						// Переключаем метод компрессии на BROTLI
						this->_compress = compress_t::BROTLI;
					// Если клиент поддерживает метод компрессии DEFLATE
					else if(compress.count(compress_t::DEFLATE) > 0)
						// Переключаем метод компрессии на DEFLATE
						this->_compress = compress_t::DEFLATE;
					// Отключаем поддержку сжатия на сервере
					else this->_compress = compress_t::NONE;
				} break;
				// Если клиент поддерживает все методы сжатия
				case static_cast <uint8_t> (compress_t::ALL_COMPRESS): {
					// Если клиент поддерживает метод компрессии BROTLI
					if(compress.count(compress_t::BROTLI) > 0)
						// Переключаем метод компрессии на BROTLI
						this->_compress = compress_t::BROTLI;
					// Если клиент поддерживает метод компрессии GZIP
					else if(compress.count(compress_t::GZIP) > 0)
						// Переключаем метод компрессии на GZIP
						this->_compress = compress_t::GZIP;
					// Если клиент поддерживает метод компрессии DEFLATE
					else if(compress.count(compress_t::DEFLATE) > 0)
						// Переключаем метод компрессии на DEFLATE
						this->_compress = compress_t::DEFLATE;
					// Отключаем поддержку сжатия на сервере
					else this->_compress = compress_t::NONE;
				} break;
				// Для всех остальных методов компрессии
				default: {
					// Если метод компрессии сервера не совпадает с выбранным методом компрессии клиентом
					if(compress.count(this->_compress) < 1)
						// Отключаем поддержку сжатия на сервере
						this->_compress = compress_t::NONE;
				}
			}
		// Отключаем поддержку сжатия на сервере
		} else this->_compress = compress_t::NONE;
	}
	// Выполняем установку стейта завершения получения данных
	this->web.state(web_t::state_t::END);
}
/**
 * clear Метод очистки собранных данных
 */
void awh::Http::clear() noexcept {
	// Выполняем очистку данных парсера
	this->web.clear();
	// Выполняем сброс чёрного списка HTTP заголовков
	this->black.clear();
}
/**
 * reset Метод сброса параметров запроса
 */
void awh::Http::reset() noexcept {
	// Выполняем сброс данных парсера
	this->web.reset();
	// Выполняем сброс параметров запроса
	this->url.clear();
	// Выполняем сброс стейта авторизации
	this->stath = stath_t::NONE;
	// Выполняем сброс стейта текущего запроса
	this->state = state_t::NONE;
}
/**
 * rmBlack Метод удаления заголовка из чёрного списка
 * @param key ключ заголовка
 */
void awh::Http::rmBlack(const string & key) noexcept {
	// Если ключ заголовка передан, удаляем его
	if(!key.empty())
		// Выполняем удаление заголовка из чёрного списка
		this->black.erase(this->fmk->transform(key, fmk_t::transform_t::LOWER));
}
/**
 * addBlack Метод добавления заголовка в чёрный список
 * @param key ключ заголовка
 */
void awh::Http::addBlack(const string & key) noexcept {
	// Если ключ заголовка передан, добавляем в список
	if(!key.empty())
		// Выполняем добавление заголовка в чёрный список
		this->black.emplace(this->fmk->transform(key, fmk_t::transform_t::LOWER));
}
/**
 * parse Метод парсинга сырых данных
 * @param buffer буфер данных для обработки
 * @param size   размер буфера данных
 * @return       размер обработанных данных
 */
size_t awh::Http::parse(const char * buffer, const size_t size) noexcept {
	// Результат работы функции
	size_t result = 0;
	// Если рукопожатие не выполнено
	if((this->state != state_t::GOOD) && (this->state != state_t::BROKEN) && (this->state != state_t::BROKEN)){
		// Выполняем парсинг сырых данных
		result = this->web.parse(buffer, size);
		// Если парсинг выполнен
		if(this->web.isEnd())
			// Выполняем коммит полученного результата
			this->commit();
	}
	// Выводим реузльтат
	return result;
}
/**
 * payload Метод чтения чанка тела запроса
 * @return текущий чанк запроса
 */
const vector <char> awh::Http::payload() const noexcept {
	// Результат работы функции
	vector <char> result;
	// Получаем собранные данные тела
	vector <char> * body = const_cast <vector <char> *> (&this->web.body());
	// Если данные тела ещё существуют
	if(!body->empty()){
		// Получаем объект параметров запроса
		const web_t::query_t & query = this->web.query();
		// Если нужно тело выводить в виде чанков
		if((query.ver > 1.0f) && this->_chunking){
			// Если версия протокола интернета выше 1.1
			if(query.ver > 1.1f){
				// Если тело сообщения больше размера чанка
				if(body->size() >= this->_chunk){
					// Формируем результат
					result.assign(body->begin(), body->begin() + this->_chunk);
					// Удаляем полученные данные в теле сообщения
					body->erase(body->begin(), body->begin() + this->_chunk);
				// Если тело сообщения полностью убирается в размер чанка
				} else {
					// Формируем результат
					result.assign(body->begin(), body->end());
					// Очищаем объект тела запроса
					body->clear();
					// Освобождаем память
					vector <char> ().swap(* body);
				}
			// Выполняем сборку чанков для протокола HTTP/1.1
			} else {
				// Тело чанка запроса
				string chunk = "";
				// Если тело сообщения больше размера чанка
				if(body->size() >= this->_chunk){
					// Получаем размер чанка
					chunk = this->fmk->itoa(this->_chunk, 16);
					// Добавляем разделитель
					chunk.append("\r\n");
					// Формируем тело чанка
					chunk.insert(chunk.end(), body->begin(), body->begin() + this->_chunk);
					// Добавляем конец запроса
					chunk.append("\r\n");
					// Удаляем полученные данные в теле сообщения
					body->erase(body->begin(), body->begin() + this->_chunk);
				// Если тело сообщения полностью убирается в размер чанка
				} else {
					// Получаем размер чанка
					chunk = this->fmk->itoa(body->size(), 16);
					// Добавляем разделитель
					chunk.append("\r\n");
					// Формируем тело чанка
					chunk.insert(chunk.end(), body->begin(), body->end());
					// Добавляем конец запроса
					chunk.append("\r\n0\r\n\r\n");
					// Очищаем данные тела
					body->clear();
					// Освобождаем память
					vector <char> ().swap(* body);
				}
				// Формируем результат
				result.assign(chunk.begin(), chunk.end());
				// Освобождаем память
				string().swap(chunk);
			}
		// Выводим данные тела как есть
		} else {
			// Если тело сообщения больше размера чанка
			if(body->size() >= this->_chunk){
				// Получаем нужный нам размер данных
				result.assign(body->begin(), body->begin() + this->_chunk);
				// Удаляем полученные данные в теле сообщения
				body->erase(body->begin(), body->begin() + this->_chunk);
			// Если тело сообщения полностью убирается в размер чанка
			} else {
				// Получаем нужный нам размер данных
				result.assign(body->begin(), body->end());
				// Очищаем данные тела
				body->clear();
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * clearBody Метод очистки данных тела
 */
void awh::Http::clearBody() const noexcept {
	// Выполняем очистку данных тела
	this->web.clearBody();
}
/**
 * clearHeaders Метод очистки списка заголовков
 */
void awh::Http::clearHeaders() const noexcept {
	// Выполняем очистку списка заголовков
	this->web.clearHeaders();
}
/**
 * body Метод получения данных тела запроса
 * @return буфер данных тела запроса
 */
const vector <char> & awh::Http::body() const noexcept {
	// Выводим данные тела
	return this->web.body();
}
/**
 * body Метод установки данных тела
 * @param body буфер тела для установки
 */
void awh::Http::body(const vector <char> & body) noexcept {
	// Устанавливаем данные телал сообщения
	this->web.body(body);
}
/**
 * rmHeader Метод удаления заголовка
 * @param key ключ заголовка
 */
void awh::Http::rmHeader(const string & key) noexcept {
	// Выполняем удаление заголовка
	this->web.rmHeader(key);
}
/**
 * header Метод получения данных заголовка
 * @param key ключ заголовка
 * @return    значение заголовка
 */
const string & awh::Http::header(const string & key) const noexcept {
	// Выводим запрашиваемый заголовок
	return this->web.header(key);
}
/**
 * header Метод добавления заголовка
 * @param key ключ заголовка
 * @param val значение заголовка
 */
void awh::Http::header(const string & key, const string & val) noexcept {
	// Если даныне заголовка переданы
	if(!key.empty() && !val.empty())
		// Выполняем добавление передаваемого заголовка
		this->web.header(key, val);
}
/**
 * headers Метод получения списка заголовков
 * @return список существующих заголовков
 */
const unordered_multimap <string, string> & awh::Http::headers() const noexcept {
	// Выводим список доступных заголовков
	return this->web.headers();
}
/**
 * headers Метод установки списка заголовков
 * @param headers список заголовков для установки
 */
void awh::Http::headers(const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем заголовки сообщения
	this->web.headers(headers);
}
/**
 * header2 Метод добавления заголовка в формате HTTP/2
 * @param key ключ заголовка
 * @param val значение заголовка
 */
void awh::Http::header2(const string & key, const string & val) noexcept {
	// Определяем соответствует ли ключ методу запроса
	if(this->fmk->compare(key, ":method")){
		// Получаем объект параметров запроса
		web_t::query_t query = this->web.query();
		// Устанавливаем версию протокола
		query.ver = 2.0f;
		// Если метод является GET запросом
		if(this->fmk->compare(val, "GET"))
			// Выполняем установку метода запроса GET
			query.method = web_t::method_t::GET;
		// Если метод является PUT запросом
		else if(this->fmk->compare(val, "PUT"))
			// Выполняем установку метода запроса PUT
			query.method = web_t::method_t::PUT;
		// Если метод является POST запросом
		else if(this->fmk->compare(val, "POST"))
			// Выполняем установку метода запроса POST
			query.method = web_t::method_t::POST;
		// Если метод является HEAD запросом
		else if(this->fmk->compare(val, "HEAD"))
			// Выполняем установку метода запроса HEAD
			query.method = web_t::method_t::HEAD;
		// Если метод является PATCH запросом
		else if(this->fmk->compare(val, "PATCH"))
			// Выполняем установку метода запроса PATCH
			query.method = web_t::method_t::PATCH;
		// Если метод является TRACE запросом
		else if(this->fmk->compare(val, "TRACE"))
			// Выполняем установку метода запроса TRACE
			query.method = web_t::method_t::TRACE;
		// Если метод является DELETE запросом
		else if(this->fmk->compare(val, "DELETE"))
			// Выполняем установку метода запроса DELETE
			query.method = web_t::method_t::DEL;
		// Если метод является OPTIONS запросом
		else if(this->fmk->compare(val, "OPTIONS"))
			// Выполняем установку метода запроса OPTIONS
			query.method = web_t::method_t::OPTIONS;
		// Если метод является CONNECT запросом
		else if(this->fmk->compare(val, "CONNECT"))
			// Выполняем установку метода запроса CONNECT
			query.method = web_t::method_t::CONNECT;
		// Выполняем сохранение параметров запроса
		this->web.query(std::move(query));
	// Если ключ запроса соответствует пути запроса
	} else if(this->fmk->compare(key, ":path")) {
		// Получаем объект параметров запроса
		web_t::query_t query = this->web.query();
		// Выполняем установку пути запроса
		query.uri = val;
		// Выполняем сохранение параметров запроса
		this->web.query(std::move(query));
	// Если ключ заголовка соответствует схеме протокола
	} else if(this->fmk->compare(key, ":scheme")) {
		/* Просто пропускаем, потому, что схему мы не используем */
	// Если ключ соответствует доменному имени
	} else if(this->fmk->compare(key, ":authority"))
		// Устанавливаем хост
		this->header("Host", val);
	// Если ключ соответствует статусу ответа
	else if(this->fmk->compare(key, ":status")) {
		// Получаем объект параметров запроса
		web_t::query_t query = this->web.query();
		// Устанавливаем версию протокола
		query.ver = 2.0f;
		// Выполняем установку статуса ответа
		query.code = static_cast <u_int> (::stoi(val));
		// Выполняем формирование текста ответа
		query.message = this->message(query.code);
		// Выполняем сохранение параметров запроса
		this->web.query(std::move(query));
	// Если ключ соответствует обычным заголовкам
	} else this->header(key, val);
}
/**
 * headers2 Метод установки списка заголовков в формате HTTP/2
 * @param headers список заголовков для установки
 */
void awh::Http::headers2(const vector <pair<string, string>> & headers) noexcept {
	// Если список заголовков не пустой
	if(!headers.empty()){
		// Переходим по всему списку заголовков
		for(auto & header : headers)
			// Выполняем установку заголовка
			this->header2(header.first, header.second);
	}
}
/**
 * getAuth Метод проверки статуса авторизации
 * @return результат проверки
 */
awh::Http::stath_t awh::Http::getAuth() const noexcept {
	// Выводим результат проверки
	return this->stath;
}
/**
 * compression Метод извлечения метода компрессии
 * @return метод компрессии
 */
awh::Http::compress_t awh::Http::compression() const noexcept {
	// Результат работы функции
	compress_t result = compress_t::NONE;
	// Проверяем пришли ли сжатые данные
	const string & encoding = this->web.header("content-encoding");
	// Если данные пришли сжатые
	if(!encoding.empty()){
		// Если данные пришли сжатые методом Brotli
		if(this->fmk->compare(encoding, "br"))
			// Устанавливаем требование выполнять декомпрессию тела сообщения
			result = compress_t::BROTLI;
		// Если данные пришли сжатые методом GZip
		else if(this->fmk->compare(encoding, "gzip"))
			// Устанавливаем требование выполнять декомпрессию тела сообщения
			result = compress_t::GZIP;
		// Если данные пришли сжатые методом Deflate
		else if(this->fmk->compare(encoding, "deflate"))
			// Устанавливаем требование выполнять декомпрессию тела сообщения
			result = compress_t::DEFLATE;
	}
	// Если мы работаем с HTTP сервером и метод компрессии установлен
	if(this->httpType == web_t::hid_t::SERVER){
		// Если заголовок с запрашиваемой кодировкой существует
		if(this->web.isHeader("accept-encoding")){
			// Переходим по всему списку заголовков
			for(auto & header : this->web.headers()){
				// Если заголовок найден
				if(this->fmk->compare(header.first, "accept-encoding")){
					// Если конкретный метод сжатия не запрашивается
					if(this->fmk->compare(header.second, "*"))
						// Устанавливаем требование выполнять декомпрессию тела сообщения
						return compress_t::BROTLI;
					// Если запрашиваются конкретные методы сжатия
					else {
						// Если найден запрашиваемый метод компрессии BROTLI
						if(header.second.find("br") != string::npos)
							// Устанавливаем требование выполнять декомпрессию тела сообщения
							return compress_t::BROTLI;
						// Если найден запрашиваемый метод компрессии GZip
						else if(header.second.find("gzip") != string::npos)
							// Устанавливаем требование выполнять декомпрессию тела сообщения
							result = compress_t::GZIP;
						// Если найден запрашиваемый метод компрессии Deflate
						else if(header.second.find("deflate") != string::npos){
							// Если не указаны другие методы компрессии
							if(result == compress_t::NONE)
								// Устанавливаем требование выполнять декомпрессию тела сообщения
								result = compress_t::DEFLATE;
						}
					}
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * compress Метод получения метода компрессии
 * @return метод компрессии сообщений
 */
awh::Http::compress_t awh::Http::compress() const noexcept {
	// Выводим метод компрессии сообщений
	return this->_compress;
}
/**
 * compress Метод установки метода компрессии
 * @param compress метод компрессии сообщений
 */
void awh::Http::compress(const compress_t compress) noexcept {
	// Устанавливаем метод компрессии сообщений
	this->_compress = compress;
}
/**
 * getUrl Метод извлечения параметров запроса
 * @return установленные параметры запроса
 */
const awh::uri_t::url_t & awh::Http::getUrl() const noexcept {
	// Выводим параметры запроса
	return this->url;
}
/**
 * dump Метод получения бинарного дампа
 * @return бинарный дамп данных
 */
vector <char> awh::Http::dump() const noexcept {
	// Результат работы функции
	vector <char> result;
	{
		// Длина строки, количество элементов
		size_t length = 0, count = 0;
		// Устанавливаем тип HTTP модуля
		result.insert(result.end(), (const char *) &this->httpType, (const char *) &this->httpType + sizeof(this->httpType));
		// Устанавливаем метод компрессии отправляемых данных
		result.insert(result.end(), (const char *) &this->_compress, (const char *) &this->_compress + sizeof(this->_compress));
		// Устанавливаем стейт проверки авторизации
		result.insert(result.end(), (const char *) &this->stath, (const char *) &this->stath + sizeof(this->stath));
		// Устанавливаем стейт текущего запроса
		result.insert(result.end(), (const char *) &this->state, (const char *) &this->state + sizeof(this->state));
		// Устанавливаем флаг зашифрованных данных
		result.insert(result.end(), (const char *) &this->crypt, (const char *) &this->crypt + sizeof(this->crypt));
		// Устанавливаем флаг разрешающий передавать тело запроса чанками
		result.insert(result.end(), (const char *) &this->_chunking, (const char *) &this->_chunking + sizeof(this->_chunking));
		// Устанавливаем размер одного чанка
		result.insert(result.end(), (const char *) &this->_chunk, (const char *) &this->_chunk + sizeof(this->_chunk));
		// Получаем размер идентификатора сервиса
		length = this->_servId.size();
		// Устанавливаем размер идентификатора сервиса
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Устанавливаем данные идентификатора сервиса
		result.insert(result.end(), this->_servId.begin(), this->_servId.end());
		// Получаем размер версии модуля приложения
		length = this->_servVer.size();
		// Устанавливаем размер версии модуля приложения
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Устанавливаем данные версии модуля приложения
		result.insert(result.end(), this->_servVer.begin(), this->_servVer.end());
		// Получаем размер названия сервиса
		length = this->_servName.size();
		// Устанавливаем размер названия сервиса
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Устанавливаем данные названия сервиса
		result.insert(result.end(), this->_servName.begin(), this->_servName.end());
		// Получаем размер User-Agent для HTTP запроса
		length = this->_userAgent.size();
		// Устанавливаем размер User-Agent для HTTP запроса
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Устанавливаем данные User-Agent для HTTP запроса
		result.insert(result.end(), this->_userAgent.begin(), this->_userAgent.end());
		// Получаем URL адрес HTTP запроса
		const string & url = this->uri->url(this->url);
		// Получаем размер URL адреса HTTP запроса
		length = url.size();
		// Устанавливаем размер URL адреса HTTP запроса
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Устанавливаем данные URL адреса HTTP запроса
		result.insert(result.end(), url.begin(), url.end());
		// Получаем количество записей чёрного списка
		count = this->black.size();
		// Устанавливаем количество записей чёрного списка
		result.insert(result.end(), (const char *) &count, (const char *) &count + sizeof(count));
		// Выполняем переход по всему чёрному списку
		for(auto & header : this->black){
			// Получаем размер заголовка из чёрного списка
			length = header.size();
			// Устанавливаем размер заголовка из чёрного списка
			result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
			// Устанавливаем данные заголовка из чёрного списка
			result.insert(result.end(), header.begin(), header.end());
		}
		// Получаем дамп WEB данных
		const auto & web = this->web.dump();
		// Получаем размер буфера WEB данных
		length = web.size();
		// Устанавливаем размер буфера WEB данных
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Устанавливаем данные буфера WEB данных
		result.insert(result.end(), web.begin(), web.end());			
	}
	// Выводим результат
	return result;
}
/**
 * dump Метод установки бинарного дампа
 * @param data бинарный дамп данных
 */
void awh::Http::dump(const vector <char> & data) noexcept {
	// Если данные бинарного дампа переданы
	if(!data.empty()){
		// Длина строки, количество элементов и смещение в буфере
		size_t length = 0, count = 0, offset = 0;
		// Выполняем получение типа HTTP модуля
		memcpy((void *) &this->httpType, data.data() + offset, sizeof(this->httpType));
		// Выполняем смещение в буфере
		offset += sizeof(this->httpType);
		// Выполняем получение метода компрессии отправляемых данных
		memcpy((void *) &this->_compress, data.data() + offset, sizeof(this->_compress));
		// Выполняем смещение в буфере
		offset += sizeof(this->_compress);
		// Выполняем получение стейта проверки авторизации
		memcpy((void *) &this->stath, data.data() + offset, sizeof(this->stath));
		// Выполняем смещение в буфере
		offset += sizeof(this->stath);
		// Выполняем получение стейта текущего запроса
		memcpy((void *) &this->state, data.data() + offset, sizeof(this->state));
		// Выполняем смещение в буфере
		offset += sizeof(this->state);
		// Выполняем получение флага зашифрованных данных
		memcpy((void *) &this->crypt, data.data() + offset, sizeof(this->crypt));
		// Выполняем смещение в буфере
		offset += sizeof(this->crypt);
		// Выполняем получение флага разрешающий передавать тело запроса чанками
		memcpy((void *) &this->_chunking, data.data() + offset, sizeof(this->_chunking));
		// Выполняем смещение в буфере
		offset += sizeof(this->_chunking);
		// Выполняем получение размера одного чанка
		memcpy((void *) &this->_chunk, data.data() + offset, sizeof(this->_chunk));
		// Выполняем смещение в буфере
		offset += sizeof(this->_chunk);
		// Выполняем получение размера идентификатора сервиса
		memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Выделяем память для данных идентификатора сервиса
		this->_servId.resize(length, 0);
		// Выполняем получение данных идентификатора сервиса
		memcpy((void *) this->_servId.data(), data.data() + offset, length);
		// Выполняем смещение в буфере
		offset += length;
		// Выполняем получение размера версии модуля приложения
		memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Выделяем память для данных версии модуля приложения
		this->_servVer.resize(length, 0);
		// Выполняем получение данных версии модуля приложения
		memcpy((void *) this->_servVer.data(), data.data() + offset, length);
		// Выполняем смещение в буфере
		offset += length;
		// Выполняем получение размера названия сервиса
		memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Выделяем память для данных названия сервиса
		this->_servName.resize(length, 0);
		// Выполняем получение данных названия сервиса
		memcpy((void *) this->_servName.data(), data.data() + offset, length);
		// Выполняем смещение в буфере
		offset += length;
		// Выполняем получение размера User-Agent для HTTP запроса
		memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Выделяем память для данных User-Agent для HTTP запроса
		this->_userAgent.resize(length, 0);
		// Выполняем получение данных User-Agent для HTTP запроса
		memcpy((void *) this->_userAgent.data(), data.data() + offset, length);
		// Выполняем смещение в буфере
		offset += length;
		// Выполняем получение размера URL адреса HTTP запроса
		memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Выделяем память для данных URL адрес HTTP запроса
		string url(length, 0);
		// Выполняем получение данных URL адрес HTTP запроса
		memcpy((void *) url.data(), data.data() + offset, length);
		// Выполняем смещение в буфере
		offset += length;
		// Устанавливаем полученный URL адрес
		this->url = this->uri->parse(url);
		// Выполняем получение количества записей чёрного списка
		memcpy((void *) &count, data.data() + offset, sizeof(count));
		// Выполняем смещение в буфере
		offset += sizeof(count);
		// Выполняем сброс заголовков чёрного списка
		this->black.clear();
		// Выполняем последовательную загрузку всех заголовков
		for(size_t i = 0; i < count; i++){
			// Выполняем получение размера заголовка из чёрного списка
			memcpy((void *) &length, data.data() + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Выделяем память для заголовка чёрного списка
			string header(length, 0);
			// Выполняем получение заголовка чёрного списка
			memcpy((void *) header.data(), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
			// Если заголовок чёрного списка получен
			if(!header.empty())
				// Выполняем добавление заголовка чёрного списка
				this->black.emplace(std::move(header));
		}
		// Выполняем получение размера дампа WEB данных
		memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Выделяем память для дампа WEB данных
		vector <char> buffer(length, 0);
		// Выполняем получение дампа WEB данных
		memcpy((void *) buffer.data(), data.data() + offset, length);
		// Выполняем смещение в буфере
		offset += length;
		// Если дамп WEB данных получен, устанавливаем его
		if(!buffer.empty()) this->web.dump(std::move(buffer));
	}
}
/**
 * isEnd Метод проверки завершения обработки
 * @return результат проверки
 */
bool awh::Http::isEnd() const noexcept {
	// Выводрим результат проверки
	return (
		(this->state == state_t::GOOD) ||
		(this->state == state_t::BROKEN) ||
		(this->state == state_t::HANDSHAKE)
	);
}
/**
 * isCrypt Метод проверки на зашифрованные данные
 * @return флаг проверки на зашифрованные данные
 */
bool awh::Http::isCrypt() const noexcept {
	// Выводим результат проверки
	return this->crypt;
}
/**
 * isAlive Метод проверки на постоянное подключение
 * @return результат проверки
 */
bool awh::Http::isAlive() const noexcept {
	// Результат работы функции
	bool result = true;
	// Запрашиваем заголовок подключения
	const string & header = this->web.header("connection");
	// Если заголовок подключения найден
	if(!header.empty())
		// Выполняем проверку является ли соединение закрытым
		result = !this->fmk->compare(header, "close");
	// Если заголовок подключения не найден
	else {
		// Переходим по всему списку заголовков
		for(auto & header : this->web.headers()){
			// Если заголовок найден
			if(this->fmk->compare(header.first, "connection")){
				// Выполняем проверку является ли соединение закрытым
				result = !this->fmk->compare(header.second, "close");
				// Выходим из цикла
				break;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * isHandshake Метод проверки рукопожатия
 * @return проверка рукопожатия
 */
bool awh::Http::isHandshake() noexcept {
	// Выполняем проверку на удачное рукопожатие
	return (this->state == state_t::HANDSHAKE);
}
/**
 * isBlack Метод проверки существования заголовка в чёрный списоке
 * @param key ключ заголовка для проверки
 * @return    результат проверки
 */
bool awh::Http::isBlack(const string & key) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если ключ заголовка передан
	if(!key.empty())
		// Выполняем проверку наличия заголовка в чёрном списке
		result = (this->black.find(this->fmk->transform(key, fmk_t::transform_t::LOWER)) != this->black.end());
	// Выводим результат
	return result;
}
/**
 * isHeader Метод проверки существования заголовка
 * @param key ключ заголовка для проверки
 * @return    результат проверки
 */
bool awh::Http::isHeader(const string & key) const noexcept {
	// Выводим результат проверки
	return this->web.isHeader(key);
}
/**
 * query Метод получения объекта запроса сервера
 * @return объект запроса сервера
 */
const awh::web_t::query_t & awh::Http::query() const noexcept {
	// Выводим объект запроса сервера
	return this->web.query();
}
/**
 * query Метод добавления объекта запроса клиента
 * @param query объект запроса клиента
 */
void awh::Http::query(const web_t::query_t & query) noexcept {
	// Устанавливаем объект запроса клиента
	this->web.query(query);
}
/**
 * date Метод получения текущей даты для HTTP запроса
 * @param stamp штамп времени в числовом виде
 * @return      штамп времени в текстовом виде
 */
const string awh::Http::date(const time_t stamp) const noexcept {
	// Создаём буфер данных
	char buffer[1000];
	// Получаем текущее время
	time_t date = (stamp > 0 ? stamp : time(nullptr));
	// Извлекаем текущее время
	struct tm tm = (* gmtime(&date));
	// Зануляем буфер
	memset(buffer, 0, sizeof(buffer));
	// Получаем формат времени
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", &tm);
	// Выводим результат
	return buffer;
}
/**
 * message Метод получения HTTP сообщения
 * @param code код сообщения для получение
 * @return     соответствующее коду HTTP сообщение
 */
const string & awh::Http::message(const u_int code) const noexcept {
	/**
	 * Подробнее: https://developer.mozilla.org/ru/docs/Web/HTTP/Status
	 */
	// Результат работы функции
	static string result = "";
	// Выполняем поиск кода сообщения
	auto it = this->messages.find(code);
	// Если код сообщения найден
	if(it != this->messages.end()) return it->second;
	// Выводим результат
	return result;
}
/**
 * decode Метод декодирования полученных чанков
 * @param buffer буфер данных для декодирования
 * @return       декодированный буфер данных
 */
awh::Http::crypto_t awh::Http::decode(const vector <char> & buffer) const noexcept {
	// Результат работы функции
	crypto_t result;
	// Если буфер для декодирования данных передан
	if(!buffer.empty()){
		// Устанавливаем данные буфера
		result.data.assign(buffer.begin(), buffer.end());
		// Получаем заголовок шифрования
		const string & encrypt = this->web.header("x-awh-encryption");
		// Если заголовок найден
		if(!encrypt.empty()){
			// Определяем размер шифрования
			switch(stoi(encrypt)){
				// Если шифрование произведено 128 битным ключём
				case 128: this->dhash.cipher(hash_t::cipher_t::AES128); break;
				// Если шифрование произведено 192 битным ключём
				case 192: this->dhash.cipher(hash_t::cipher_t::AES192); break;
				// Если шифрование произведено 256 битным ключём
				case 256: this->dhash.cipher(hash_t::cipher_t::AES256); break;
			}
			// Выполняем дешифрование полученных данных
			const auto & res = this->dhash.decrypt(result.data.data(), result.data.size());
			// Если данные расшифрованны, заменяем тело данных
			if((result.encrypt = !res.empty())) result.data.assign(res.begin(), res.end());
		}
		// Проверяем пришли ли сжатые данные
		const string & encoding = this->web.header("content-encoding");
		// Если данные пришли сжатые
		if(!encoding.empty()){
			// Если данные пришли сжатые методом Brotli
			if(this->fmk->compare(encoding, "br")){
				// Выполняем декомпрессию данных
				const auto & res = this->dhash.decompress(result.data.data(), result.data.size(), hash_t::method_t::BROTLI);
				// Заменяем полученное тело
				if((result.compress = !res.empty())) result.data.assign(res.begin(), res.end());
			// Если данные пришли сжатые методом GZip
			} else if(this->fmk->compare(encoding, "gzip")) {
				// Выполняем декомпрессию данных
				const auto & res = this->dhash.decompress(result.data.data(), result.data.size(), hash_t::method_t::GZIP);
				// Заменяем полученное тело
				if((result.compress = !res.empty())) result.data.assign(res.begin(), res.end());
			// Если данные пришли сжатые методом Deflate
			} else if(this->fmk->compare(encoding, "deflate")) {
				// Получаем данные тела в бинарном виде
				vector <char> buffer(result.data.begin(), result.data.end());
				// Добавляем хвост в полученные данные
				this->dhash.setTail(buffer);
				// Выполняем декомпрессию данных
				const auto & res = this->dhash.decompress(buffer.data(), buffer.size(), hash_t::method_t::DEFLATE);
				// Заменяем полученное тело
				if((result.compress = !res.empty())) result.data.assign(res.begin(), res.end());
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * encode Метод кодирования полученных чанков
 * @param buffer буфер данных для кодирования
 * @return       кодированный буфер данных
 */
awh::Http::crypto_t awh::Http::encode(const vector <char> & buffer) const noexcept {
	// Результат работы функции
	crypto_t result;
	// Если буфер для декодирования данных передан
	if(!buffer.empty()){
		// Устанавливаем данные буфера
		result.data.assign(buffer.begin(), buffer.end());
		// Если нужно производить шифрование
		if(this->crypt && !this->isBlack("X-AWH-Encryption")){
			// Выполняем шифрование переданных данных
			const auto & res = this->hash.encrypt(result.data.data(), result.data.size());
			// Если данные зашифрованы, заменяем тело данных
			if((result.encrypt = !res.empty())) result.data.assign(res.begin(), res.end());
		}
		// Если заголовок не запрещён
		if(!this->isBlack("Content-Encoding")){
			// Определяем метод компрессии тела сообщения
			switch(static_cast <uint8_t> (this->_compress)){
				// Если нужно сжать тело методом BROTLI
				case static_cast <uint8_t> (compress_t::BROTLI): {
					// Выполняем сжатие тела сообщения
					const auto & brotli = this->hash.compress(result.data.data(), result.data.size(), hash_t::method_t::BROTLI);
					// Если данные сжаты, заменяем тело данных
					if((result.compress = !brotli.empty())) result.data.assign(brotli.begin(), brotli.end());
				} break;
				// Если нужно сжать тело методом GZIP
				case static_cast <uint8_t> (compress_t::GZIP): {
					// Выполняем сжатие тела сообщения
					const auto & gzip = this->hash.compress(result.data.data(), result.data.size(), hash_t::method_t::GZIP);
					// Если данные сжаты, заменяем тело данных
					if((result.compress = !gzip.empty())) result.data.assign(gzip.begin(), gzip.end());
				} break;
				// Если нужно сжать тело методом DEFLATE
				case static_cast <uint8_t> (compress_t::DEFLATE): {
					// Выполняем сжатие тела сообщения
					auto deflate = this->hash.compress(result.data.data(), result.data.size(), hash_t::method_t::DEFLATE);
					// Удаляем хвост в полученных данных
					this->hash.rmTail(deflate);
					// Если данные сжаты, заменяем тело данных
					if((result.compress = !deflate.empty())) result.data.assign(deflate.begin(), deflate.end());
				} break;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * request Метод создания запроса как он есть
 * @param nobody флаг запрета подготовки тела
 * @return       буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::request(const bool nobody) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если заголовки получены
	if(!this->web.headers().empty()){
		// Получаем объект параметров запроса
		const web_t::query_t & query = this->web.query();
		// Если параметры запроса получены
		if(!query.uri.empty() && (query.method != web_t::method_t::NONE)){
			/**
			 * Типы основных заголовков
			 */
			bool available[4] = {
				false, // Content-Length
				false, // Content-Encoding
				false, // Transfer-Encoding
				false  // X-AWH-Encryption
			};
			// Размер тела сообщения
			size_t length = 0;
			// Данные REST запроса
			string request = "";
			// Определяем метод запроса
			switch(static_cast <uint8_t> (query.method)){
				// Если метод запроса указан как GET
				case static_cast <uint8_t> (web_t::method_t::GET):
					// Формируем GET запрос
					request = this->fmk->format("GET %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как PUT
				case static_cast <uint8_t> (web_t::method_t::PUT):
					// Формируем PUT запрос
					request = this->fmk->format("PUT %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как POST
				case static_cast <uint8_t> (web_t::method_t::POST):
					// Формируем POST запрос
					request = this->fmk->format("POST %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как HEAD
				case static_cast <uint8_t> (web_t::method_t::HEAD):
					// Формируем HEAD запрос
					request = this->fmk->format("HEAD %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как PATCH
				case static_cast <uint8_t> (web_t::method_t::PATCH):
					// Формируем PATCH запрос
					request = this->fmk->format("PATCH %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как TRACE
				case static_cast <uint8_t> (web_t::method_t::TRACE):
					// Формируем TRACE запрос
					request = this->fmk->format("TRACE %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как DELETE
				case static_cast <uint8_t> (web_t::method_t::DEL):
					// Формируем DELETE запрос
					request = this->fmk->format("DELETE %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как OPTIONS
				case static_cast <uint8_t> (web_t::method_t::OPTIONS):
					// Формируем OPTIONS запрос
					request = this->fmk->format("OPTIONS %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как CONNECT
				case static_cast <uint8_t> (web_t::method_t::CONNECT):
					// Формируем CONNECT запрос
					request = this->fmk->format("CONNECT %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
			}
			// Переходим по всему списку заголовков
			for(auto & header : this->web.headers()){
				// Флаг разрешающий вывода заголовка
				bool allow = !this->isBlack(header.first);
				// Выполняем перебор всех обязательных заголовков
				for(uint8_t i = 0; i < 4; i++){
					// Если заголовок уже найден пропускаем его
					if(available[i]) continue;
					// Выполняем првоерку заголовка
					switch(i){
						case 1: available[i] = this->fmk->compare(header.first, "content-encoding");  break;
						case 2: available[i] = this->fmk->compare(header.first, "transfer-encoding"); break;
						case 3: available[i] = this->fmk->compare(header.first, "x-awh-encryption");  break;
						case 0: {
							// Запоминаем, что мы нашли заголовок размера тела
							available[i] = this->fmk->compare(header.first, "content-length");
							// Устанавливаем размер тела сообщения
							if(available[i]) length = static_cast <size_t> (::stoull(header.second));
						} break;
					}
					// Если заголовок разрешён для вывода
					if(allow){
						// Выполняем првоерку заголовка
						switch(i){
							case 0:
							case 1:
							case 2:
							case 3: allow = !available[i]; break;
						}
					}
				}
				// Если заголовок не является запрещённым, добавляем заголовок в запрос
				if(allow)
					// Формируем строку запроса
					request.append(
						this->fmk->format(
							"%s: %s\r\n",
							this->fmk->transform(header.first, fmk_t::transform_t::SMART).c_str(),
							header.second.c_str()
						)
					);
			}
			// Получаем данные тела
			const auto & body = this->web.body();
			// Если запрос не является GET, HEAD или TRACE, а тело запроса существует
			if((query.method != web_t::method_t::GET) && (query.method != web_t::method_t::HEAD) && (query.method != web_t::method_t::TRACE) && !body.empty()){
				// Выполняем кодирование данных
				const auto & crypto = this->encode(body);
				// Проверяем нужно ли передать тело разбив на чанки
				this->_chunking = (!available[0] || ((length > 0) && (length != body.size())));
				// Если данные были сжаты, либо зашифрованы
				if(crypto.encrypt || crypto.compress){
					// Если флаг запрета подготовки тела полезной нагрузки не установлен
					if(!nobody){
						// Выполняем очистку тела сообщения
						this->clearBody();
						// Заменяем тело данных
						this->web.body(crypto.data);
						// Заменяем размер тела данных
						if(!this->_chunking) length = body.size();
					// Заменяем размер тела данных
					} else if(!this->_chunking) length = crypto.data.size();
				}
				// Если данные зашифрованы, устанавливаем соответствующие заголовки
				if(crypto.encrypt)
					// Устанавливаем X-AWH-Encryption
					request.append(this->fmk->format("X-AWH-Encryption: %u\r\n", static_cast <u_short> (this->hash.cipher())));
				// Если данные сжаты, устанавливаем соответствующие заголовки
				if(crypto.compress){
					// Определяем метод компрессии тела сообщения
					switch(static_cast <uint8_t> (this->_compress)){
						// Если нужно сжать тело методом BROTLI
						case static_cast <uint8_t> (compress_t::BROTLI):
							// Устанавливаем Content-Encoding если не передан
							request.append(this->fmk->format("Content-Encoding: %s\r\n", "br"));
						break;
						// Если нужно сжать тело методом GZIP
						case static_cast <uint8_t> (compress_t::GZIP):
							// Устанавливаем Content-Encoding если не передан
							request.append(this->fmk->format("Content-Encoding: %s\r\n", "gzip"));
						break;
						// Если нужно сжать тело методом DEFLATE
						case static_cast <uint8_t> (compress_t::DEFLATE):
							// Устанавливаем Content-Encoding если не передан
							request.append(this->fmk->format("Content-Encoding: %s\r\n", "deflate"));
						break;
					}
				}
				// Если данные необходимо разбивать на чанки
				if(this->_chunking && !this->isBlack("Transfer-Encoding"))
					// Устанавливаем заголовок Transfer-Encoding
					request.append(this->fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
				// Если заголовок размера передаваемого тела, не запрещён
				else if(!this->isBlack("Content-Length"))
					// Устанавливаем размер передаваемого тела Content-Length
					request.append(this->fmk->format("Content-Length: %zu\r\n", length));
			// Очищаем тела сообщения
			} else this->clearBody();
			// Устанавливаем завершающий разделитель
			request.append("\r\n");
			// Формируем результат запроса
			result.assign(request.begin(), request.end());
		}
	}
	// Выводим результат
	return result;
}
/**
 * response Метод создания ответа как он есть
 * @param nobody флаг запрета подготовки тела
 * @return       буфер данных ответа в бинарном виде
 */
vector <char> awh::Http::response(const bool nobody) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если заголовки получены
	if(!this->web.headers().empty()){
		// Получаем объект параметров запроса
		const web_t::query_t & query = this->web.query();
		// Если параметры запроса получены
		if(query.code > 0){
			// Если ответ не установлен, формируем по умолчанию
			if(query.message.empty())
				// Устанавливаем сообщение по умолчанию
				const_cast <web_t::query_t *> (&query)->message = this->message(query.code);
			/**
			 * Типы основных заголовков
			 */
			bool available[4] = {
				false, // Content-Length
				false, // Content-Encoding
				false, // Transfer-Encoding
				false  // X-AWH-Encryption
			};
			// Размер тела сообщения
			size_t length = 0;
			// Данные REST ответа
			string response = this->fmk->format("HTTP/%.1f %u %s\r\n", query.ver, query.code, query.message.c_str());
			// Переходим по всему списку заголовков
			for(auto & header : this->web.headers()){
				// Флаг разрешающий вывода заголовка
				bool allow = !this->isBlack(header.first);
				// Выполняем перебор всех обязательных заголовков
				for(uint8_t i = 0; i < 4; i++){
					// Если заголовок уже найден пропускаем его
					if(available[i]) continue;
					// Выполняем првоерку заголовка
					switch(i){
						case 1: available[i] = this->fmk->compare(header.first, "content-encoding");  break;
						case 2: available[i] = this->fmk->compare(header.first, "transfer-encoding"); break;
						case 3: available[i] = this->fmk->compare(header.first, "x-awh-encryption");  break;
						case 0: {
							// Запоминаем, что мы нашли заголовок размера тела
							available[i] = this->fmk->compare(header.first, "content-length");
							// Устанавливаем размер тела сообщения
							if(available[i]) length = static_cast <size_t> (::stoull(header.second));
						} break;
					}
					// Если заголовок разрешён для вывода
					if(allow){
						// Выполняем првоерку заголовка
						switch(i){
							case 0:
							case 1:
							case 2:
							case 3: allow = !available[i]; break;
						}
					}
				}
				// Если заголовок не является запрещённым, добавляем заголовок в ответ
				if(allow)
					// Формируем строку ответа
					response.append(
						this->fmk->format(
							"%s: %s\r\n",
							this->fmk->transform(header.first, fmk_t::transform_t::SMART).c_str(),
							header.second.c_str()
						)
					);
			}
			// Получаем данные тела
			const auto & body = this->web.body();
			// Если запрос должен содержать тело и тело ответа существует
			if((query.code >= 200) && (query.code != 204) && (query.code != 304) && (query.code != 308) && !body.empty()){
				// Выполняем кодирование данных
				const auto & crypto = this->encode(body);
				// Проверяем нужно ли передать тело разбив на чанки
				this->_chunking = (!available[0] || ((length > 0) && (length != body.size())));
				// Если данные были сжаты, либо зашифрованы
				if(crypto.encrypt || crypto.compress){
					// Если флаг запрета подготовки тела полезной нагрузки не установлен
					if(!nobody){
						// Выполняем очистку тела сообщения
						this->clearBody();
						// Заменяем тело данных
						this->web.body(crypto.data);
						// Заменяем размер тела данных
						if(!this->_chunking) length = body.size();
					// Заменяем размер тела данных
					} else if(!this->_chunking) length = crypto.data.size();
				}
				// Если данные зашифрованы, устанавливаем соответствующие заголовки
				if(crypto.encrypt)
					// Устанавливаем X-AWH-Encryption
					response.append(this->fmk->format("X-AWH-Encryption: %u\r\n", static_cast <u_short> (this->hash.cipher())));
				// Если данные сжаты, устанавливаем соответствующие заголовки
				if(crypto.compress){
					// Определяем метод компрессии тела сообщения
					switch(static_cast <uint8_t> (this->_compress)){
						// Если нужно сжать тело методом BROTLI
						case static_cast <uint8_t> (compress_t::BROTLI):
							// Устанавливаем Content-Encoding если не передан
							response.append(this->fmk->format("Content-Encoding: %s\r\n", "br"));
						break;
						// Если нужно сжать тело методом GZIP
						case static_cast <uint8_t> (compress_t::GZIP):
							// Устанавливаем Content-Encoding если не передан
							response.append(this->fmk->format("Content-Encoding: %s\r\n", "gzip"));
						break;
						// Если нужно сжать тело методом DEFLATE
						case static_cast <uint8_t> (compress_t::DEFLATE):
							// Устанавливаем Content-Encoding если не передан
							response.append(this->fmk->format("Content-Encoding: %s\r\n", "deflate"));
						break;
					}
				}
				// Если данные необходимо разбивать на чанки
				if(this->_chunking && !this->isBlack("Transfer-Encoding"))
					// Устанавливаем заголовок Transfer-Encoding
					response.append(this->fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
				// Если заголовок размера передаваемого тела, не запрещён
				else if(!this->isBlack("Content-Length"))
					// Устанавливаем размер передаваемого тела Content-Length
					response.append(this->fmk->format("Content-Length: %zu\r\n", length));
			// Очищаем тела сообщения
			} else this->clearBody();
			// Устанавливаем завершающий разделитель
			response.append("\r\n");
			// Формируем результат ответа
			result.assign(response.begin(), response.end());
		}
	}
	// Выводим результат
	return result;
}
/**
 * proxy Метод создания запроса для авторизации на прокси-сервере
 * @param url объект параметров REST запроса
 * @return    буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::proxy(const uri_t::url_t & url) noexcept {
	// Результат работы функции
	vector <char> result;
	// Если параметры REST запроса переданы
	if(!url.empty()){
		// Получаем хост сервера
		const string & host = (!url.domain.empty() ? url.domain : url.ip);
		// Если хост сервера получен
		if(!host.empty() && (url.port > 0)){
			// Объект параметров запроса
			web_t::query_t query;
			// Добавляем в чёрный список заголовок Accept
			this->addBlack("Accept");
			// Добавляем в чёрный список заголовок Accept-Language
			this->addBlack("Accept-Language");
			// Добавляем в чёрный список заголовок Accept-Encoding
			this->addBlack("Accept-Encoding");
			// Если заголовок подключения ещё не существует
			if(!this->web.isHeader("connection"))
				// Добавляем поддержку постоянного подключения
				this->header("Connection", "Keep-Alive");
			// Если заголовок подключения прокси ещё не существует
			if(!this->web.isHeader("proxy-connection"))
				// Добавляем поддержку постоянного подключения для прокси-сервера
				this->header("Proxy-Connection", "Keep-Alive");
			// Устанавливаем параметры REST запроса
			this->auth.client.uri(this->uri->url(url));
			// Получаем параметры авторизации
			const string & auth = this->auth.client.header("connect", true);
			// Если данные авторизации получены
			if(!auth.empty()) this->header("Proxy-Authorization", auth);
			// Формируем URI запроса
			query.uri = this->fmk->format("%s:%u", host.c_str(), url.port);
			// Устанавливаем парарметр запроса
			this->web.query(query);
			// Выполняем создание запроса
			return this->request(url, web_t::method_t::CONNECT);
		}
	}
	// Выводим результат
	return result;
}
/**
 * reject Метод создания отрицательного ответа
 * @param code код ответа
 * @param mess сообщение ответа
 * @return     буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::reject(const u_int code, const string & mess) const noexcept {
	// Объект параметров запроса
	web_t::query_t query;
	// Получаем текст сообщения
	query.message = (!mess.empty() ? mess : this->message(code));
	// Если сообщение получено
	if(!query.message.empty()){
		// Если заголовок подключения ещё не существует
		if(!this->web.isHeader("connection")){
			// Если требуется ввод авторизационных данных
			if((code == 401) || (code == 407))
				// Добавляем заголовок закрытия подключения
				this->web.header("Connection", "Keep-Alive");
			// Добавляем заголовок закрытия подключения
			else this->web.header("Connection", "Close");
		}
		// Если заголовок типа контента не существует
		if(!this->web.isHeader("content-type"))
			// Добавляем заголовок тип контента
			this->web.header("Content-type", "text/html; charset=utf-8");
		// Если запрос должен содержать тело сообщения
		if((code >= 200) && (code != 204) && (code != 304) && (code != 308)){
			// Получаем данные тела
			const auto & body = this->web.body();
			// Если тело ответа не установлено, устанавливаем своё
			if(body.empty()){
				// Формируем тело ответа
				const string & body = this->fmk->format(
					"<html>\n<head>\n<title>%u %s</title>\n</head>\n<body>\n<h2>%u %s</h2>\n</body>\n</html>\n",
					code, query.message.c_str(), code, query.message.c_str()
				);
				// Выполняем очистку тела сообщения
				this->clearBody();
				// Добавляем тело сообщения
				this->web.body(vector <char> (body.begin(), body.end()));
			}
			// Удаляем размер передаваемого контента
			this->web.rmHeader("content-length");
			// Добавляем заголовок тела сообщения
			this->web.header("Content-Length", ::to_string(body.size()));
		}
		// Устанавливаем парарметр запроса
		this->web.query(query);
		// Выводим результат
		return this->response(code, mess);
	}
	// Выводим результат
	return vector <char> ();
}
/**
 * response Метод создания ответа
 * @param code код ответа
 * @param mess сообщение ответа
 * @return     буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::response(const u_int code, const string & mess) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Получаем объект параметров запроса
	web_t::query_t query = this->web.query();
	// Получаем текст сообщения
	query.message = (!mess.empty() ? mess : this->message(code));
	// Если сообщение получено
	if(!query.message.empty()){
		/**
		 * Типы основных заголовков
		 */
		bool available[8] = {
			false, // Connection
			false, // Content-Type
			false, // Content-Length
			false, // Content-Encoding
			false, // Transfer-Encoding
			false, // X-AWH-Encryption
			false, // WWW-Authenticate
			false  // Proxy-Authenticate
		};
		// Размер тела сообщения
		size_t length = 0;
		// Устанавливаем код ответа
		query.code = code;
		// Данные REST ответа
		string response = this->fmk->format("HTTP/%.1f %u %s\r\n", query.ver, code, query.message.c_str());
		// Если заголовок не запрещён
		if(!this->isBlack("Date"))
			// Добавляем заголовок даты в ответ
			response.append(this->fmk->format("Date: %s\r\n", this->date().c_str()));
		// Переходим по всему списку заголовков
		for(auto & header : this->web.headers()){
			// Флаг разрешающий вывода заголовка
			bool allow = !this->isBlack(header.first);
			// Выполняем перебор всех обязательных заголовков
			for(uint8_t i = 0; i < 8; i++){
				// Если заголовок уже найден пропускаем его
				if(available[i]) continue;
				// Выполняем првоерку заголовка
				switch(i){
					case 0: available[i] = this->fmk->compare(header.first, "connection");         break;
					case 1: available[i] = this->fmk->compare(header.first, "content-type");       break;
					case 3: available[i] = this->fmk->compare(header.first, "content-encoding");   break;
					case 4: available[i] = this->fmk->compare(header.first, "transfer-encoding");  break;
					case 5: available[i] = this->fmk->compare(header.first, "x-awh-encryption");   break;
					case 6: available[i] = this->fmk->compare(header.first, "www-authenticate");   break;
					case 7: available[i] = this->fmk->compare(header.first, "proxy-authenticate"); break;
					case 2: {
						// Запоминаем, что мы нашли заголовок размера тела
						available[i] = this->fmk->compare(header.first, "content-length");
						// Устанавливаем размер тела сообщения
						if(available[i]) length = static_cast <size_t> (::stoull(header.second));
					} break;
				}
				// Если заголовок разрешён для вывода
				if(allow){
					// Выполняем првоерку заголовка
					switch(i){
						case 2:
						case 3:
						case 4:
						case 5: allow = !available[i]; break;
					}
				}
			}
			// Если заголовок не является запрещённым, добавляем заголовок в ответ
			if(allow)
				// Формируем строку ответа
				response.append(
					this->fmk->format(
						"%s: %s\r\n",
						this->fmk->transform(header.first, fmk_t::transform_t::SMART).c_str(),
						header.second.c_str()
					)
				);
		}
		// Устанавливаем Connection если не передан
		if(!available[0] && !this->isBlack("Connection"))
			// Добавляем заголовок в ответ
			response.append(this->fmk->format("Connection: %s\r\n", HTTP_HEADER_CONNECTION));
		// Устанавливаем Content-Type если не передан
		if(!available[1] && !this->isBlack("Content-Type"))
			// Добавляем заголовок в ответ
			response.append(this->fmk->format("Content-Type: %s\r\n", HTTP_HEADER_CONTENTTYPE));
		// Если заголовок не запрещён
		if(!this->isBlack("Server"))
			// Добавляем название сервера в ответ
			response.append(this->fmk->format("Server: %s\r\n", this->_servName.c_str()));
		// Если заголовок не запрещён
		if(!this->isBlack("X-Powered-By"))
			// Добавляем название рабочей системы в ответ
			response.append(this->fmk->format("X-Powered-By: %s/%s\r\n", this->_servId.c_str(), this->_servVer.c_str()));
		// Если заголовок авторизации не передан
		if(((code == 401) && !available[6]) || ((code == 407) && !available[7])){
			// Получаем параметры авторизации
			const string & auth = this->auth.server.header(true);
			// Если параметры авторизации получены
			if(!auth.empty()){
				// Определяем код авторизации
				switch(code){
					// Если авторизация производится для Web-Сервера
					case 401: {
						// Если заголовок не запрещён
						if(!this->isBlack("WWW-Authenticate"))
							// Добавляем параметры авторизации
							response.append(this->fmk->format("WWW-Authenticate: %s\r\n", auth.c_str()));
					} break;
					// Если авторизация производится для Прокси-Сервера
					case 407: {
						// Если заголовок не запрещён
						if(!this->isBlack("Proxy-Authenticate"))
							// Добавляем параметры авторизации
							response.append(this->fmk->format("Proxy-Authenticate: %s\r\n", auth.c_str()));
					} break;
				}
			}
		}
		// Получаем данные тела
		const auto & body = this->web.body();
		// Если запрос должен содержать тело и тело ответа существует
		if((code >= 200) && (code != 204) && (code != 304) && (code != 308) && !body.empty()){
			// Выполняем кодирование данных
			const auto & crypto = this->encode(body);
			// Проверяем нужно ли передать тело разбив на чанки
			this->_chunking = (!available[2] || ((length > 0) && (length != body.size())));
			// Если данные были сжаты, либо зашифрованы
			if(crypto.encrypt || crypto.compress){
				// Выполняем очистку тела сообщения
				this->clearBody();
				// Заменяем тело данных
				this->web.body(crypto.data);
				// Заменяем размер тела данных
				if(!this->_chunking) length = body.size();
			}
			// Если данные зашифрованы, устанавливаем соответствующие заголовки
			if(crypto.encrypt)
				// Устанавливаем X-AWH-Encryption
				response.append(this->fmk->format("X-AWH-Encryption: %u\r\n", static_cast <u_short> (this->hash.cipher())));
			// Если данные сжаты, устанавливаем соответствующие заголовки
			if(crypto.compress){
				// Определяем метод компрессии тела сообщения
				switch(static_cast <uint8_t> (this->_compress)){
					// Если нужно сжать тело методом BROTLI
					case static_cast <uint8_t> (compress_t::BROTLI):
						// Устанавливаем Content-Encoding если не передан
						response.append(this->fmk->format("Content-Encoding: %s\r\n", "br"));
					break;
					// Если нужно сжать тело методом GZIP
					case static_cast <uint8_t> (compress_t::GZIP):
						// Устанавливаем Content-Encoding если не передан
						response.append(this->fmk->format("Content-Encoding: %s\r\n", "gzip"));
					break;
					// Если нужно сжать тело методом DEFLATE
					case static_cast <uint8_t> (compress_t::DEFLATE):
						// Устанавливаем Content-Encoding если не передан
						response.append(this->fmk->format("Content-Encoding: %s\r\n", "deflate"));
					break;
				}
			}
			// Если данные необходимо разбивать на чанки
			if(this->_chunking && !this->isBlack("Transfer-Encoding"))
				// Устанавливаем заголовок Transfer-Encoding
				response.append(this->fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
			// Если заголовок размера передаваемого тела, не запрещён
			else if(!this->isBlack("Content-Length"))
				// Устанавливаем размер передаваемого тела Content-Length
				response.append(this->fmk->format("Content-Length: %zu\r\n", length));
		// Очищаем тела сообщения
		} else this->web.clearBody();
		// Устанавливаем завершающий разделитель
		response.append("\r\n");
		// Формируем результат ответа
		result.assign(response.begin(), response.end());
	}
	// Выводим результат
	return result;
}
/**
 * request Метод создания запроса
 * @param url    объект параметров REST запроса
 * @param method метод REST запроса
 * @return       буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::request(const uri_t::url_t & url, const web_t::method_t method) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если параметры REST запроса переданы
	if(!url.empty()){
		// Получаем хост запроса
		const string & host = (!url.domain.empty() ? url.domain : url.ip);
		// Если хост получен
		if(!host.empty()){
			/**
			 * Типы основных заголовков
			 */
			bool available[12] = {
				false, // Host
				false, // Accept
				false, // Origin
				false, // User-Agent
				false, // Connection
				false, // Content-Length
				false, // Accept-Language
				false, // Accept-Encoding
				false, // Content-Encoding
				false, // Transfer-Encoding
				false, // X-AWH-Encryption
				false  // Proxy-Authorization
			};
			// Запоминаем параметры запроса
			this->url = url;
			// Размер тела сообщения
			size_t length = 0;
			// Данные REST запроса
			string request = "";
			// Получаем объект параметров запроса
			web_t::query_t query = this->web.query();
			// Устанавливаем параметры REST запроса
			this->auth.client.uri(this->uri->url(url));
			// Если метод не CONNECT или URI не установлен
			if((method != web_t::method_t::CONNECT) || query.uri.empty())
				// Формируем HTTP запрос
				query.uri = this->uri->query(url);
			// Определяем метод запроса
			switch(static_cast <uint8_t> (method)){
				// Если метод запроса указан как GET
				case static_cast <uint8_t> (web_t::method_t::GET):
					// Формируем GET запрос
					request = this->fmk->format("GET %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как PUT
				case static_cast <uint8_t> (web_t::method_t::PUT):
					// Формируем PUT запрос
					request = this->fmk->format("PUT %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как POST
				case static_cast <uint8_t> (web_t::method_t::POST):
					// Формируем POST запрос
					request = this->fmk->format("POST %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как HEAD
				case static_cast <uint8_t> (web_t::method_t::HEAD):
					// Формируем HEAD запрос
					request = this->fmk->format("HEAD %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как PATCH
				case static_cast <uint8_t> (web_t::method_t::PATCH):
					// Формируем PATCH запрос
					request = this->fmk->format("PATCH %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как TRACE
				case static_cast <uint8_t> (web_t::method_t::TRACE):
					// Формируем TRACE запрос
					request = this->fmk->format("TRACE %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как DELETE
				case static_cast <uint8_t> (web_t::method_t::DEL):
					// Формируем DELETE запрос
					request = this->fmk->format("DELETE %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как OPTIONS
				case static_cast <uint8_t> (web_t::method_t::OPTIONS):
					// Формируем OPTIONS запрос
					request = this->fmk->format("OPTIONS %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
				// Если метод запроса указан как CONNECT
				case static_cast <uint8_t> (web_t::method_t::CONNECT):
					// Формируем CONNECT запрос
					request = this->fmk->format("CONNECT %s HTTP/%.1f\r\n", query.uri.c_str(), query.ver);
				break;
			}
			// Запоминаем метод запроса
			query.method = method;
			// Переходим по всему списку заголовков
			for(auto & header : this->web.headers()){
				// Флаг разрешающий вывода заголовка
				bool allow = !this->isBlack(header.first);
				// Выполняем перебор всех обязательных заголовков
				for(uint8_t i = 0; i < 12; i++){
					// Если заголовок уже найден пропускаем его
					if(available[i]) continue;
					// Выполняем првоерку заголовка
					switch(i){
						case 0:  available[i] = this->fmk->compare(header.first, "host");                  break;
						case 1:  available[i] = this->fmk->compare(header.first, "accept");                break;
						case 2:  available[i] = this->fmk->compare(header.first, "origin");                break;
						case 3:  available[i] = this->fmk->compare(header.first, "user-agent");            break;
						case 4:  available[i] = this->fmk->compare(header.first, "connection");            break;
						case 6:  available[i] = this->fmk->compare(header.first, "accept-language");       break;
						case 7:  available[i] = this->fmk->compare(header.first, "accept-encoding");       break;
						case 8:  available[i] = this->fmk->compare(header.first, "content-encoding");      break;
						case 9:  available[i] = this->fmk->compare(header.first, "transfer-encoding");     break;
						case 10: available[i] = this->fmk->compare(header.first, "x-awh-encryption");      break;
						case 11: available[i] = (this->fmk->compare(header.first, "authorization") ||
							                     this->fmk->compare(header.first, "proxy-authorization")); break;
						case 5: {
							// Запоминаем, что мы нашли заголовок размера тела
							available[i] = this->fmk->compare(header.first, "content-length");
							// Устанавливаем размер тела сообщения
							if(available[i]) length = static_cast <size_t> (::stoull(header.second));
						} break;
					}
					// Если заголовок разрешён для вывода
					if(allow){
						// Выполняем првоерку заголовка
						switch(i){
							case 5:
							case 7:
							case 8:
							case 9:
							case 10: allow = !available[i]; break;
						}
					}
				}
				// Если заголовок не является запрещённым, добавляем заголовок в запрос
				if(allow)
					// Формируем строку запроса
					request.append(
						this->fmk->format(
							"%s: %s\r\n",
							this->fmk->transform(header.first, fmk_t::transform_t::SMART).c_str(),
							header.second.c_str()
						)
					);
			}
			// Устанавливаем Host если не передан
			if(!available[0] && !this->isBlack("Host"))
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Host: %s\r\n", host.c_str()));
			// Устанавливаем Accept если не передан
			if(!available[1] && (method != web_t::method_t::CONNECT) && !this->isBlack("Accept"))
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Accept: %s\r\n", HTTP_HEADER_ACCEPT));
			// Устанавливаем Origin если не передан
			if(!available[2] && !this->isBlack("Origin"))
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Origin: %s\r\n", this->uri->origin(url).c_str()));
			// Устанавливаем Connection если не передан
			if(!available[4] && !this->isBlack("Connection"))
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Connection: %s\r\n", HTTP_HEADER_CONNECTION));
			// Устанавливаем Accept-Language если не передан
			if(!available[6] && (method != web_t::method_t::CONNECT) && !this->isBlack("Accept-Language"))
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("Accept-Language: %s\r\n", HTTP_HEADER_ACCEPTLANGUAGE));
			// Если нужно запросить компрессию в удобном нам виде
			if((this->_compress != compress_t::NONE) && (method != web_t::method_t::CONNECT) && !this->isBlack("Accept-Encoding")){
				// Определяем метод сжатия который поддерживает клиент
				switch(static_cast <uint8_t> (this->_compress)){
					// Если клиент поддерживает методот сжатия BROTLI
					case static_cast <uint8_t> (compress_t::BROTLI):
						// Добавляем заголовок в запрос
						request.append(this->fmk->format("Accept-Encoding: %s\r\n", "br"));
					break;
					// Если клиент поддерживает методот сжатия GZIP
					case static_cast <uint8_t> (compress_t::GZIP):
						// Добавляем заголовок в запрос
						request.append(this->fmk->format("Accept-Encoding: %s\r\n", "gzip"));
					break;
					// Если клиент поддерживает методот сжатия DEFLATE
					case static_cast <uint8_t> (compress_t::DEFLATE):
						// Добавляем заголовок в запрос
						request.append(this->fmk->format("Accept-Encoding: %s\r\n", "deflate"));
					break;
					// Если клиент поддерживает методот сжатия GZIP, BROTLI
					case static_cast <uint8_t> (compress_t::GZIP_BROTLI):
						// Добавляем заголовок в запрос
						request.append(this->fmk->format("Accept-Encoding: %s\r\n", "gzip, br"));
					break;
					// Если клиент поддерживает методот сжатия GZIP, DEFLATE
					case static_cast <uint8_t> (compress_t::GZIP_DEFLATE):
						// Добавляем заголовок в запрос
						request.append(this->fmk->format("Accept-Encoding: %s\r\n", "gzip, deflate"));
					break;
					// Если клиент поддерживает методот сжатия DEFLATE, BROTLI
					case static_cast <uint8_t> (compress_t::DEFLATE_BROTLI):
						// Добавляем заголовок в запрос
						request.append(this->fmk->format("Accept-Encoding: %s\r\n", "deflate, br"));
					break;
					// Если клиент поддерживает все методы сжатия
					case static_cast <uint8_t> (compress_t::ALL_COMPRESS):
						// Добавляем заголовок в запрос
						request.append(this->fmk->format("Accept-Encoding: %s\r\n", "gzip, deflate, br"));
					break;
				}
			}
			// Устанавливаем User-Agent если не передан
			if(!available[3] && !this->isBlack("User-Agent")){
				// Если User-Agent установлен стандартный
				if(this->fmk->compare(this->_userAgent, HTTP_HEADER_AGENT)){
					// Название операционной системы
					const char * os = nullptr;
					// Определяем название операционной системы
					switch(static_cast <uint8_t> (this->fmk->os())){
						// Если операционной системой является Unix
						case static_cast <uint8_t> (fmk_t::os_t::UNIX): os = "Unix"; break;
						// Если операционной системой является Linux
						case static_cast <uint8_t> (fmk_t::os_t::LINUX): os = "Linux"; break;
						// Если операционной системой является неизвестной
						case static_cast <uint8_t> (fmk_t::os_t::NONE): os = "Unknown"; break;
						// Если операционной системой является Windows
						case static_cast <uint8_t> (fmk_t::os_t::WIND32):
						case static_cast <uint8_t> (fmk_t::os_t::WIND64): os = "Windows"; break;
						// Если операционной системой является MacOS X
						case static_cast <uint8_t> (fmk_t::os_t::MACOSX): os = "MacOS X"; break;
						// Если операционной системой является FreeBSD
						case static_cast <uint8_t> (fmk_t::os_t::FREEBSD): os = "FreeBSD"; break;
					}
					// Выполняем генерацию Юзер-агента клиента выполняющего HTTP запрос
					this->_userAgent = this->fmk->format("%s (%s; %s/%s)", this->_servName.c_str(), os, this->_servId.c_str(), this->_servVer.c_str());
				}
				// Добавляем заголовок в запрос
				request.append(this->fmk->format("User-Agent: %s\r\n", this->_userAgent.c_str()));
			}
			// Если заголовок авторизации не передан
			if(!available[11] && !this->isBlack("Authorization") && !this->isBlack("Proxy-Authorization")){
				// Метод HTTP запроса
				string httpMethod = "";
				// Определяем метод запроса
				switch(static_cast <uint8_t> (method)){
					// Если метод запроса указан как GET
					case static_cast <uint8_t> (web_t::method_t::GET): httpMethod = "get"; break;
					// Если метод запроса указан как PUT
					case static_cast <uint8_t> (web_t::method_t::PUT): httpMethod = "put"; break;
					// Если метод запроса указан как POST
					case static_cast <uint8_t> (web_t::method_t::POST): httpMethod = "post"; break;
					// Если метод запроса указан как HEAD
					case static_cast <uint8_t> (web_t::method_t::HEAD): httpMethod = "head"; break;
					// Если метод запроса указан как PATCH
					case static_cast <uint8_t> (web_t::method_t::PATCH): httpMethod = "patch"; break;
					// Если метод запроса указан как TRACE
					case static_cast <uint8_t> (web_t::method_t::TRACE): httpMethod = "trace"; break;
					// Если метод запроса указан как DELETE
					case static_cast <uint8_t> (web_t::method_t::DEL): httpMethod = "delete"; break;
					// Если метод запроса указан как OPTIONS
					case static_cast <uint8_t> (web_t::method_t::OPTIONS): httpMethod = "options"; break;
					// Если метод запроса указан как CONNECT
					case static_cast <uint8_t> (web_t::method_t::CONNECT): httpMethod = "connect"; break;
				}
				// Получаем параметры авторизации
				const string & auth = this->auth.client.header(httpMethod, false);
				// Если данные авторизации получены
				if(!auth.empty()) request.append(auth);
			}
			// Получаем данные тела
			const auto & body = this->web.body();
			// Если запрос не является GET, HEAD или TRACE, а тело запроса существует
			if((method != web_t::method_t::GET) && (method != web_t::method_t::HEAD) && (method != web_t::method_t::TRACE) && !body.empty()){
				// Выполняем кодирование данных
				const auto & crypto = this->encode(body);
				// Если заголовок не запрещён
				if(!this->isBlack("Date"))
					// Добавляем заголовок даты в запрос
					request.append(this->fmk->format("Date: %s\r\n", this->date().c_str()));
				// Проверяем нужно ли передать тело разбив на чанки
				this->_chunking = (!available[5] || ((length > 0) && (length != body.size())));
				// Если данные были сжаты, либо зашифрованы
				if(crypto.encrypt || crypto.compress){
					// Выполняем очистку тела сообщения
					this->clearBody();
					// Заменяем тело данных
					this->web.body(crypto.data);
					// Заменяем размер тела данных
					if(!this->_chunking) length = body.size();
				}
				// Если данные зашифрованы, устанавливаем соответствующие заголовки
				if(crypto.encrypt)
					// Устанавливаем X-AWH-Encryption
					request.append(this->fmk->format("X-AWH-Encryption: %u\r\n", static_cast <u_short> (this->hash.cipher())));
				// Если данные сжаты, устанавливаем соответствующие заголовки
				if(crypto.compress){
					// Определяем метод компрессии тела сообщения
					switch(static_cast <uint8_t> (this->_compress)){
						// Если нужно сжать тело методом BROTLI
						case static_cast <uint8_t> (compress_t::BROTLI):
							// Устанавливаем Content-Encoding если не передан
							request.append(this->fmk->format("Content-Encoding: %s\r\n", "br"));
						break;
						// Если нужно сжать тело методом GZIP
						case static_cast <uint8_t> (compress_t::GZIP):
							// Устанавливаем Content-Encoding если не передан
							request.append(this->fmk->format("Content-Encoding: %s\r\n", "gzip"));
						break;
						// Если нужно сжать тело методом DEFLATE
						case static_cast <uint8_t> (compress_t::DEFLATE):
							// Устанавливаем Content-Encoding если не передан
							request.append(this->fmk->format("Content-Encoding: %s\r\n", "deflate"));
						break;
					}
				}
				// Если данные необходимо разбивать на чанки
				if(this->_chunking && !this->isBlack("Transfer-Encoding"))
					// Устанавливаем заголовок Transfer-Encoding
					request.append(this->fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
				// Если заголовок размера передаваемого тела, не запрещён
				else if(!this->isBlack("Content-Length"))
					// Устанавливаем размер передаваемого тела Content-Length
					request.append(this->fmk->format("Content-Length: %zu\r\n", length));
			// Очищаем тела сообщения
			} else this->web.clearBody();
			// Устанавливаем завершающий разделитель
			request.append("\r\n");
			// Формируем результат запроса
			result.assign(request.begin(), request.end());
		}
	}
	// Выводим результат
	return result;
}
/**
 * proxy2 Метод создания запроса для авторизации на прокси-сервере (протокол HTTP/2)
 * @param url объект параметров REST запроса
 * @return    буфер данных запроса в бинарном виде
 */
vector <pair<string, string>> awh::Http::proxy2(const uri_t::url_t & url) noexcept {
	// Если параметры REST запроса переданы
	if(!url.empty()){
		// Получаем хост сервера
		const string & host = (!url.domain.empty() ? url.domain : url.ip);
		// Если хост сервера получен
		if(!host.empty() && (url.port > 0)){
			// Объект параметров запроса
			web_t::query_t query;
			// Добавляем в чёрный список заголовок Accept
			this->addBlack("Accept");
			// Добавляем в чёрный список заголовок Accept-Language
			this->addBlack("Accept-Language");
			// Добавляем в чёрный список заголовок Accept-Encoding
			this->addBlack("Accept-Encoding");
			// Если заголовок подключения ещё не существует
			if(!this->web.isHeader("connection"))
				// Добавляем поддержку постоянного подключения
				this->header("Connection", "Keep-Alive");
			// Если заголовок подключения прокси ещё не существует
			if(!this->web.isHeader("proxy-connection"))
				// Добавляем поддержку постоянного подключения для прокси-сервера
				this->header("Proxy-Connection", "Keep-Alive");
			// Устанавливаем параметры REST запроса
			this->auth.client.uri(this->uri->url(url));
			// Получаем параметры авторизации
			const string & auth = this->auth.client.header("connect", true);
			// Если данные авторизации получены
			if(!auth.empty()) this->header("Proxy-Authorization", auth);
			// Формируем URI запроса
			query.uri = this->fmk->format("%s:%u", host.c_str(), url.port);
			// Устанавливаем парарметр запроса
			this->web.query(query);
			// Выполняем создание запроса
			return this->request2(url, web_t::method_t::CONNECT);
		}
	}
	// Выводим результат
	return vector <pair<string, string>> ();
}
/**
 * reject2 Метод создания отрицательного ответа (протокол HTTP/2)
 * @param code код ответа
 * @param mess сообщение ответа
 * @return     буфер данных запроса в бинарном виде
 */
vector <pair<string, string>> awh::Http::reject2(const u_int code, const string & mess) const noexcept {
	// Объект параметров запроса
	web_t::query_t query;
	// Получаем текст сообщения
	query.message = (!mess.empty() ? mess : this->message(code));
	// Если сообщение получено
	if(!query.message.empty()){
		// Если заголовок подключения ещё не существует
		if(!this->web.isHeader("connection")){
			// Если требуется ввод авторизационных данных
			if((code == 401) || (code == 407))
				// Добавляем заголовок закрытия подключения
				this->web.header("Connection", "Keep-Alive");
			// Добавляем заголовок закрытия подключения
			else this->web.header("Connection", "Close");
		}
		// Если заголовок типа контента не существует
		if(!this->web.isHeader("content-type"))
			// Добавляем заголовок тип контента
			this->web.header("Content-type", "text/html; charset=utf-8");
		// Если запрос должен содержать тело сообщения
		if((code >= 200) && (code != 204) && (code != 304) && (code != 308)){
			// Получаем данные тела
			const auto & body = this->web.body();
			// Если тело ответа не установлено, устанавливаем своё
			if(body.empty()){
				// Формируем тело ответа
				const string & body = this->fmk->format(
					"<html>\n<head>\n<title>%u %s</title>\n</head>\n<body>\n<h2>%u %s</h2>\n</body>\n</html>\n",
					code, query.message.c_str(), code, query.message.c_str()
				);
				// Выполняем очистку тела сообщения
				this->clearBody();
				// Добавляем тело сообщения
				this->web.body(vector <char> (body.begin(), body.end()));
			}
			// Удаляем размер передаваемого контента
			this->web.rmHeader("content-length");
			// Добавляем заголовок тела сообщения
			this->web.header("Content-Length", ::to_string(body.size()));
		}
		// Устанавливаем парарметр запроса
		this->web.query(query);
		// Выводим результат
		return this->response2(code);
	}
	// Выводим результат
	return vector <pair<string, string>> ();
}
/**
 * response2 Метод создания ответа (протокол HTTP/2)
 * @param code код ответа
 * @return     буфер данных запроса в бинарном виде
 */
vector <pair<string, string>> awh::Http::response2(const u_int code) const noexcept {
	// Результат работы функции
	vector <pair<string, string>> result;
	/**
	 * Типы основных заголовков
	 */
	bool available[8] = {
		false, // Connection
		false, // Content-Type
		false, // Content-Length
		false, // Content-Encoding
		false, // Transfer-Encoding
		false, // X-AWH-Encryption
		false, // WWW-Authenticate
		false  // Proxy-Authenticate
	};
	// Размер тела сообщения
	size_t length = 0;
	// Данные REST ответа
	result.push_back(make_pair(":status", ::to_string(code)));
	// Если заголовок не запрещён
	if(!this->isBlack("Date"))
		// Добавляем заголовок даты в ответ
		result.push_back(make_pair("Date", this->date()));
	// Переходим по всему списку заголовков
	for(auto & header : this->web.headers()){
		// Флаг разрешающий вывода заголовка
		bool allow = !this->isBlack(header.first);
		// Выполняем перебор всех обязательных заголовков
		for(uint8_t i = 0; i < 8; i++){
			// Если заголовок уже найден пропускаем его
			if(available[i]) continue;
			// Выполняем првоерку заголовка
			switch(i){
				case 0: available[i] = this->fmk->compare(header.first, "connection");         break;
				case 1: available[i] = this->fmk->compare(header.first, "content-type");       break;
				case 3: available[i] = this->fmk->compare(header.first, "content-encoding");   break;
				case 4: available[i] = this->fmk->compare(header.first, "transfer-encoding");  break;
				case 5: available[i] = this->fmk->compare(header.first, "x-awh-encryption");   break;
				case 6: available[i] = this->fmk->compare(header.first, "www-authenticate");   break;
				case 7: available[i] = this->fmk->compare(header.first, "proxy-authenticate"); break;
				case 2: {
					// Запоминаем, что мы нашли заголовок размера тела
					available[i] = this->fmk->compare(header.first, "content-length");
					// Устанавливаем размер тела сообщения
					if(available[i]) length = static_cast <size_t> (::stoull(header.second));
				} break;
			}
			// Если заголовок разрешён для вывода
			if(allow){
				// Выполняем првоерку заголовка
				switch(i){
					case 0:
					case 2:
					case 3:
					case 4:
					case 5: allow = !available[i]; break;
				}
			}
		}
		// Если заголовок не является запрещённым, добавляем заголовок в ответ
		if(allow)
			// Формируем строку ответа
			result.push_back(make_pair(this->fmk->transform(header.first, fmk_t::transform_t::SMART), header.second));
	}
	// Устанавливаем Content-Type если не передан
	if(!available[1] && !this->isBlack("Content-Type"))
		// Добавляем заголовок в ответ
		result.push_back(make_pair("Content-Type", HTTP_HEADER_CONTENTTYPE));
	// Если заголовок не запрещён
	if(!this->isBlack("Server"))
		// Добавляем название сервера в ответ
		result.push_back(make_pair("Server", this->_servName));
	// Если заголовок не запрещён
	if(!this->isBlack("X-Powered-By"))
		// Добавляем название рабочей системы в ответ
		result.push_back(make_pair("X-Powered-By", this->fmk->format("%s/%s", this->_servId.c_str(), this->_servVer.c_str())));
	// Если заголовок авторизации не передан
	if(((code == 401) && !available[6]) || ((code == 407) && !available[7])){
		// Получаем параметры авторизации
		const string & auth = this->auth.server.header(true);
		// Если параметры авторизации получены
		if(!auth.empty()){
			// Определяем код авторизации
			switch(code){
				// Если авторизация производится для Web-Сервера
				case 401: {
					// Если заголовок не запрещён
					if(!this->isBlack("WWW-Authenticate"))
						// Добавляем параметры авторизации
						result.push_back(make_pair("WWW-Authenticate", auth));
				} break;
				// Если авторизация производится для Прокси-Сервера
				case 407: {
					// Если заголовок не запрещён
					if(!this->isBlack("Proxy-Authenticate"))
						// Добавляем параметры авторизации
						result.push_back(make_pair("Proxy-Authenticate", auth));
				} break;
			}
		}
	}
	// Получаем данные тела
	const auto & body = this->web.body();
	// Если запрос должен содержать тело и тело ответа существует
	if((code >= 200) && (code != 204) && (code != 304) && (code != 308) && !body.empty()){
		// Выполняем кодирование данных
		const auto & crypto = this->encode(body);
		// Проверяем нужно ли передать тело разбив на чанки
		this->_chunking = (!available[2] || ((length > 0) && (length != body.size())));
		// Если данные были сжаты, либо зашифрованы
		if(crypto.encrypt || crypto.compress){
			// Выполняем очистку тела сообщения
			this->clearBody();
			// Заменяем тело данных
			this->web.body(crypto.data);
			// Заменяем размер тела данных
			if(!this->_chunking) length = body.size();
		}
		// Если данные зашифрованы, устанавливаем соответствующие заголовки
		if(crypto.encrypt)
			// Устанавливаем X-AWH-Encryption
			result.push_back(make_pair("X-AWH-Encryption", ::to_string(static_cast <u_short> (this->hash.cipher()))));
		// Если данные сжаты, устанавливаем соответствующие заголовки
		if(crypto.compress){
			// Определяем метод компрессии тела сообщения
			switch(static_cast <uint8_t> (this->_compress)){
				// Если нужно сжать тело методом BROTLI
				case static_cast <uint8_t> (compress_t::BROTLI):
					// Устанавливаем Content-Encoding если не передан
					result.push_back(make_pair("Content-Encoding", "br"));
				break;
				// Если нужно сжать тело методом GZIP
				case static_cast <uint8_t> (compress_t::GZIP):
					// Устанавливаем Content-Encoding если не передан
					result.push_back(make_pair("Content-Encoding", "gzip"));
				break;
				// Если нужно сжать тело методом DEFLATE
				case static_cast <uint8_t> (compress_t::DEFLATE):
					// Устанавливаем Content-Encoding если не передан
					result.push_back(make_pair("Content-Encoding", "deflate"));
				break;
			}
		}
		// Если данные необходимо разбивать на чанки
		if(this->_chunking && !this->isBlack("Transfer-Encoding"))
			// Устанавливаем заголовок Transfer-Encoding
			result.push_back(make_pair("Transfer-Encoding", "chunked"));
		// Если заголовок размера передаваемого тела, не запрещён
		else if(!this->isBlack("Content-Length"))
			// Устанавливаем размер передаваемого тела Content-Length
			result.push_back(make_pair("Content-Length", ::to_string(length)));
	// Очищаем тела сообщения
	} else this->web.clearBody();
	// Выводим результат
	return result;
}
/**
 * request2 Метод создания запроса (протокол HTTP/2)
 * @param url    объект параметров REST запроса
 * @param method метод REST запроса
 * @return       буфер данных запроса в бинарном виде
 */
vector <pair<string, string>> awh::Http::request2(const uri_t::url_t & url, const web_t::method_t method) const noexcept {
	// Результат работы функции
	vector <pair<string, string>> result;
	// Если параметры REST запроса переданы
	if(!url.empty()){
		// Получаем хост запроса
		const string & host = (!url.domain.empty() ? url.domain : url.ip);
		// Если хост получен
		if(!host.empty()){
			/**
			 * Типы основных заголовков
			 */
			bool available[12] = {
				false, // Host
				false, // Accept
				false, // Origin
				false, // User-Agent
				false, // Connection
				false, // Content-Length
				false, // Accept-Language
				false, // Accept-Encoding
				false, // Content-Encoding
				false, // Transfer-Encoding
				false, // X-AWH-Encryption
				false  // Proxy-Authorization
			};
			// Запоминаем параметры запроса
			this->url = url;
			// Размер тела сообщения
			size_t length = 0;
			// Получаем объект параметров запроса
			web_t::query_t query = this->web.query();
			// Устанавливаем параметры REST запроса
			this->auth.client.uri(this->uri->url(url));
			// Если метод не CONNECT или URI не установлен
			if((method != web_t::method_t::CONNECT) || query.uri.empty())
				// Формируем HTTP запрос
				query.uri = this->uri->query(url);
			// Определяем метод запроса
			switch(static_cast <uint8_t> (method)){
				// Если метод запроса указан как GET
				case static_cast <uint8_t> (web_t::method_t::GET):
					// Формируем GET запрос
					result.push_back(make_pair(":method", "GET"));
				break;
				// Если метод запроса указан как PUT
				case static_cast <uint8_t> (web_t::method_t::PUT):
					// Формируем PUT запрос
					result.push_back(make_pair(":method", "PUT"));
				break;
				// Если метод запроса указан как POST
				case static_cast <uint8_t> (web_t::method_t::POST):
					// Формируем POST запрос
					result.push_back(make_pair(":method", "POST"));
				break;
				// Если метод запроса указан как HEAD
				case static_cast <uint8_t> (web_t::method_t::HEAD):
					// Формируем HEAD запрос
					result.push_back(make_pair(":method", "HEAD"));
				break;
				// Если метод запроса указан как PATCH
				case static_cast <uint8_t> (web_t::method_t::PATCH):
					// Формируем PATCH запрос
					result.push_back(make_pair(":method", "PATCH"));
				break;
				// Если метод запроса указан как TRACE
				case static_cast <uint8_t> (web_t::method_t::TRACE):
					// Формируем TRACE запрос
					result.push_back(make_pair(":method", "TRACE"));
				break;
				// Если метод запроса указан как DELETE
				case static_cast <uint8_t> (web_t::method_t::DEL):
					// Формируем DELETE запрос
					result.push_back(make_pair(":method", "DELETE"));
				break;
				// Если метод запроса указан как OPTIONS
				case static_cast <uint8_t> (web_t::method_t::OPTIONS):
					// Формируем OPTIONS запрос
					result.push_back(make_pair(":method", "OPTIONS"));
				break;
				// Если метод запроса указан как CONNECT
				case static_cast <uint8_t> (web_t::method_t::CONNECT):
					// Формируем CONNECT запрос
					result.push_back(make_pair(":method", "CONNECT"));
				break;
			}
			// Запоминаем метод запроса
			query.method = method;
			// Выполняем установку пути запроса
			result.push_back(make_pair(":path", query.uri));
			// Выполняем установку схемы протокола
			result.push_back(make_pair(":scheme", "https"));
			// Выполняем установку хоста сервера
			result.push_back(make_pair(":authority", host));
			// Переходим по всему списку заголовков
			for(auto & header : this->web.headers()){
				// Флаг разрешающий вывода заголовка
				bool allow = !this->isBlack(header.first);
				// Выполняем перебор всех обязательных заголовков
				for(uint8_t i = 0; i < 12; i++){
					// Если заголовок уже найден пропускаем его
					if(available[i]) continue;
					// Выполняем првоерку заголовка
					switch(i){
						case 0:  available[i] = this->fmk->compare(header.first, "host");                  break;
						case 1:  available[i] = this->fmk->compare(header.first, "accept");                break;
						case 2:  available[i] = this->fmk->compare(header.first, "origin");                break;
						case 3:  available[i] = this->fmk->compare(header.first, "user-agent");            break;
						case 4:  available[i] = this->fmk->compare(header.first, "connection");            break;
						case 6:  available[i] = this->fmk->compare(header.first, "accept-language");       break;
						case 7:  available[i] = this->fmk->compare(header.first, "accept-encoding");       break;
						case 8:  available[i] = this->fmk->compare(header.first, "content-encoding");      break;
						case 9:  available[i] = this->fmk->compare(header.first, "transfer-encoding");     break;
						case 10: available[i] = this->fmk->compare(header.first, "x-awh-encryption");      break;
						case 11: available[i] = (this->fmk->compare(header.first, "authorization") ||
							                     this->fmk->compare(header.first, "proxy-authorization")); break;
						case 5: {
							// Запоминаем, что мы нашли заголовок размера тела
							available[i] = this->fmk->compare(header.first, "content-length");
							// Устанавливаем размер тела сообщения
							if(available[i]) length = static_cast <size_t> (::stoull(header.second));
						} break;
					}
					// Если заголовок разрешён для вывода
					if(allow){
						// Выполняем првоерку заголовка
						switch(i){
							case 0:
							case 4:
							case 5:
							case 7:
							case 8:
							case 9:
							case 10: allow = !available[i]; break;
						}
					}
				}
				// Если заголовок не является запрещённым, добавляем заголовок в запрос
				if(allow)
					// Формируем строку запроса
					result.push_back(make_pair(this->fmk->transform(header.first, fmk_t::transform_t::SMART), header.second));
			}
			// Устанавливаем Accept если не передан
			if(!available[1] && (method != web_t::method_t::CONNECT) && !this->isBlack("Accept"))
				// Добавляем заголовок в запрос
				result.push_back(make_pair("Accept", HTTP_HEADER_ACCEPT));
			// Устанавливаем Origin если не передан
			if(!available[2] && !this->isBlack("Origin"))
				// Добавляем заголовок в запрос
				result.push_back(make_pair("Origin", this->uri->origin(url)));
			// Устанавливаем Accept-Language если не передан
			if(!available[6] && (method != web_t::method_t::CONNECT) && !this->isBlack("Accept-Language"))
				// Добавляем заголовок в запрос
				result.push_back(make_pair("Accept-Language", HTTP_HEADER_ACCEPTLANGUAGE));
			// Если нужно запросить компрессию в удобном нам виде
			if((this->_compress != compress_t::NONE) && (method != web_t::method_t::CONNECT) && !this->isBlack("Accept-Encoding")){
				// Определяем метод сжатия который поддерживает клиент
				switch(static_cast <uint8_t> (this->_compress)){
					// Если клиент поддерживает методот сжатия BROTLI
					case static_cast <uint8_t> (compress_t::BROTLI):
						// Добавляем заголовок в запрос
						result.push_back(make_pair("Accept-Encoding", "br"));
					break;
					// Если клиент поддерживает методот сжатия GZIP
					case static_cast <uint8_t> (compress_t::GZIP):
						// Добавляем заголовок в запрос
						result.push_back(make_pair("Accept-Encoding", "gzip"));
					break;
					// Если клиент поддерживает методот сжатия DEFLATE
					case static_cast <uint8_t> (compress_t::DEFLATE):
						// Добавляем заголовок в запрос
						result.push_back(make_pair("Accept-Encoding", "deflate"));
					break;
					// Если клиент поддерживает методот сжатия GZIP, BROTLI
					case static_cast <uint8_t> (compress_t::GZIP_BROTLI):
						// Добавляем заголовок в запрос
						result.push_back(make_pair("Accept-Encoding", "gzip, br"));
					break;
					// Если клиент поддерживает методот сжатия GZIP, DEFLATE
					case static_cast <uint8_t> (compress_t::GZIP_DEFLATE):
						// Добавляем заголовок в запрос
						result.push_back(make_pair("Accept-Encoding", "gzip, deflate"));
					break;
					// Если клиент поддерживает методот сжатия DEFLATE, BROTLI
					case static_cast <uint8_t> (compress_t::DEFLATE_BROTLI):
						// Добавляем заголовок в запрос
						result.push_back(make_pair("Accept-Encoding", "deflate, br"));
					break;
					// Если клиент поддерживает все методы сжатия
					case static_cast <uint8_t> (compress_t::ALL_COMPRESS):
						// Добавляем заголовок в запрос
						result.push_back(make_pair("Accept-Encoding", "gzip, deflate, br"));
					break;
				}
			}
			// Устанавливаем User-Agent если не передан
			if(!available[3] && !this->isBlack("User-Agent")){
				// Если User-Agent установлен стандартный
				if(this->fmk->compare(this->_userAgent, HTTP_HEADER_AGENT)){
					// Название операционной системы
					const char * os = nullptr;
					// Определяем название операционной системы
					switch(static_cast <uint8_t> (this->fmk->os())){
						// Если операционной системой является Unix
						case static_cast <uint8_t> (fmk_t::os_t::UNIX): os = "Unix"; break;
						// Если операционной системой является Linux
						case static_cast <uint8_t> (fmk_t::os_t::LINUX): os = "Linux"; break;
						// Если операционной системой является неизвестной
						case static_cast <uint8_t> (fmk_t::os_t::NONE): os = "Unknown"; break;
						// Если операционной системой является Windows
						case static_cast <uint8_t> (fmk_t::os_t::WIND32):
						case static_cast <uint8_t> (fmk_t::os_t::WIND64): os = "Windows"; break;
						// Если операционной системой является MacOS X
						case static_cast <uint8_t> (fmk_t::os_t::MACOSX): os = "MacOS X"; break;
						// Если операционной системой является FreeBSD
						case static_cast <uint8_t> (fmk_t::os_t::FREEBSD): os = "FreeBSD"; break;
					}
					// Выполняем генерацию Юзер-агента клиента выполняющего HTTP запрос
					this->_userAgent = this->fmk->format("%s (%s; %s/%s)", this->_servName.c_str(), os, this->_servId.c_str(), this->_servVer.c_str());
				}
				// Добавляем заголовок в запрос
				result.push_back(make_pair("User-Agent", this->_userAgent));
			}
			// Если заголовок авторизации не передан
			if(!available[11] && !this->isBlack("Authorization") && !this->isBlack("Proxy-Authorization")){
				// Метод HTTP запроса
				string httpMethod = "";
				// Определяем метод запроса
				switch(static_cast <uint8_t> (method)){
					// Если метод запроса указан как GET
					case static_cast <uint8_t> (web_t::method_t::GET): httpMethod = "get"; break;
					// Если метод запроса указан как PUT
					case static_cast <uint8_t> (web_t::method_t::PUT): httpMethod = "put"; break;
					// Если метод запроса указан как POST
					case static_cast <uint8_t> (web_t::method_t::POST): httpMethod = "post"; break;
					// Если метод запроса указан как HEAD
					case static_cast <uint8_t> (web_t::method_t::HEAD): httpMethod = "head"; break;
					// Если метод запроса указан как PATCH
					case static_cast <uint8_t> (web_t::method_t::PATCH): httpMethod = "patch"; break;
					// Если метод запроса указан как TRACE
					case static_cast <uint8_t> (web_t::method_t::TRACE): httpMethod = "trace"; break;
					// Если метод запроса указан как DELETE
					case static_cast <uint8_t> (web_t::method_t::DEL): httpMethod = "delete"; break;
					// Если метод запроса указан как OPTIONS
					case static_cast <uint8_t> (web_t::method_t::OPTIONS): httpMethod = "options"; break;
					// Если метод запроса указан как CONNECT
					case static_cast <uint8_t> (web_t::method_t::CONNECT): httpMethod = "connect"; break;
				}
				// Получаем параметры авторизации
				const string & auth = this->auth.client.header(httpMethod, false);
				// Если данные авторизации получены
				if(!auth.empty()){
					// Выполняем поиск разделителя
					const size_t pos = auth.find(':');
					// Если разделитель заголовка получен
					if(pos != string::npos)
						// Выполняем установку заголовка
						result.push_back(make_pair(auth.substr(0, pos), this->fmk->transform(auth.substr(pos + 1), fmk_t::transform_t::TRIM)));
				}
			}
			// Получаем данные тела
			const auto & body = this->web.body();
			// Если запрос не является GET, HEAD или TRACE, а тело запроса существует
			if((method != web_t::method_t::GET) && (method != web_t::method_t::HEAD) && (method != web_t::method_t::TRACE) && !body.empty()){
				// Выполняем кодирование данных
				const auto & crypto = this->encode(body);
				// Если заголовок не запрещён
				if(!this->isBlack("Date"))
					// Добавляем заголовок даты в запрос
					result.push_back(make_pair("Date", this->date()));
				// Проверяем нужно ли передать тело разбив на чанки
				this->_chunking = (!available[5] || ((length > 0) && (length != body.size())));
				// Если данные были сжаты, либо зашифрованы
				if(crypto.encrypt || crypto.compress){
					// Выполняем очистку тела сообщения
					this->clearBody();
					// Заменяем тело данных
					this->web.body(crypto.data);
					// Заменяем размер тела данных
					if(!this->_chunking) length = body.size();
				}
				// Если данные зашифрованы, устанавливаем соответствующие заголовки
				if(crypto.encrypt)
					// Устанавливаем X-AWH-Encryption
					result.push_back(make_pair("X-AWH-Encryption", ::to_string(static_cast <u_short> (this->hash.cipher()))));
				// Если данные сжаты, устанавливаем соответствующие заголовки
				if(crypto.compress){
					// Определяем метод компрессии тела сообщения
					switch(static_cast <uint8_t> (this->_compress)){
						// Если нужно сжать тело методом BROTLI
						case static_cast <uint8_t> (compress_t::BROTLI):
							// Устанавливаем Content-Encoding если не передан
							result.push_back(make_pair("Content-Encoding", "br"));
						break;
						// Если нужно сжать тело методом GZIP
						case static_cast <uint8_t> (compress_t::GZIP):
							// Устанавливаем Content-Encoding если не передан
							result.push_back(make_pair("Content-Encoding", "gzip"));
						break;
						// Если нужно сжать тело методом DEFLATE
						case static_cast <uint8_t> (compress_t::DEFLATE):
							// Устанавливаем Content-Encoding если не передан
							result.push_back(make_pair("Content-Encoding", "deflate"));
						break;
					}
				}
				// Если данные необходимо разбивать на чанки
				if(this->_chunking && !this->isBlack("Transfer-Encoding"))
					// Устанавливаем заголовок Transfer-Encoding
					result.push_back(make_pair("Transfer-Encoding", "chunked"));
				// Если заголовок размера передаваемого тела, не запрещён
				else if(!this->isBlack("Content-Length"))
					// Устанавливаем размер передаваемого тела Content-Length
					result.push_back(make_pair("Content-Length", ::to_string(length)));
			// Очищаем тела сообщения
			} else this->web.clearBody();
		}
	}
	// Выводим результат
	return result;
}
/**
 * chunking Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
void awh::Http::chunking(function <void (const vector <char> &, const Http *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_fn = callback;
	// Устанавливаем функцию обратного вызова для получения чанков
	this->web.chunking(std::bind(&awh::Http::chunkingCallback, this, _1, _2));
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::Http::chunk(const size_t size) noexcept {
	// Устанавливаем размер чанка
	if(size >= 100) this->_chunk = size;
}
/**
 * userAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::Http::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()) this->_userAgent = userAgent;
}
/**
 * serv Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::Http::serv(const string & id, const string & name, const string & ver) noexcept {
	// Если идентификатор сервиса передан, устанавливаем
	if(!id.empty()) this->_servId = id;
	// Если версия сервиса передана
	if(!ver.empty()) this->_servVer = ver;
	// Если название сервиса передано, устанавливаем
	if(!name.empty()) this->_servName = name;
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::Http::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем флаг шифрования
	this->crypt = !pass.empty();
	{
		// Устанавливаем соль шифрования
		this->hash.salt(salt);
		// Устанавливаем пароль шифрования
		this->hash.pass(pass);
		// Устанавливаем размер шифрования
		this->hash.cipher(cipher);
	}{
		// Устанавливаем соль шифрования
		this->dhash.salt(salt);
		// Устанавливаем пароль шифрования
		this->dhash.pass(pass);
		// Устанавливаем размер шифрования
		this->dhash.cipher(cipher);
	}
}
/**
 * Http Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 */
awh::Http::Http(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept :
 web(fmk, log), auth(fmk, log), hash(log), dhash(log), crypt(false), _chunking(false),
 _chunk(BUFFER_CHUNK), _servId(AWH_SHORT_NAME), _servVer(AWH_VERSION), _servName(AWH_NAME),
 _userAgent(HTTP_HEADER_AGENT), stath(stath_t::NONE), state(state_t::NONE),
 _compress(compress_t::NONE), httpType(web_t::hid_t::NONE), _fn(nullptr), fmk(fmk), log(log), uri(uri) {
	// Устанавливаем функцию обратного вызова для получения чанков
	this->web.chunking(std::bind(&awh::Http::chunkingCallback, this, _1, _2));
}
