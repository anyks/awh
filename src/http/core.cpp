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
 * @param id     идентификатор объекта
 * @param buffer буфер данных чанка полезной нагрузки
 * @param web    объект HTTP парсера
 */
void awh::Http::chunkingCallback(const uint64_t id, const vector <char> & buffer, const web_t * web) noexcept {
	// Если функция обратного вызова на вывод полученного чанка установлена
	if(this->_callback.is("chunking"))
		// Выводим функцию обратного вызова
		this->_callback.call <const uint64_t, const vector <char> &, const http_t *> ("chunking", id, buffer, this);
}
/**
 * commit Метод применения полученных результатов
 */
void awh::Http::commit() noexcept {
	// Выполняем проверку авторизации
	this->_stath = this->checkAuth();
	// Если ключ соответствует
	if(this->_stath == stath_t::GOOD)
		// Устанавливаем стейт рукопожатия
		this->_state = state_t::GOOD;
	// Поменяем данные как бракованные
	else this->_state = state_t::BROKEN;
	// Получаем данные тела
	const auto & body = this->_web.body();
	// Если тело сообщения получено
	if(!body.empty()){
		// Получаем заголовок шифрования
		const string & encrypt = this->_web.header("x-awh-encryption");
		// Если заголовок найден
		if((this->_crypt = !encrypt.empty())){
			// Определяем размер шифрования
			switch(stoi(encrypt)){
				// Если шифрование произведено 128 битным ключём
				case 128: this->_hash.cipher(hash_t::cipher_t::AES128); break;
				// Если шифрование произведено 192 битным ключём
				case 192: this->_hash.cipher(hash_t::cipher_t::AES192); break;
				// Если шифрование произведено 256 битным ключём
				case 256: this->_hash.cipher(hash_t::cipher_t::AES256); break;
			}
			// Выполняем дешифрование полученных данных
			const auto & result = this->_hash.decrypt(body.data(), body.size());
			// Заменяем полученное тело
			if(!result.empty()){
				// Очищаем тело сообщения
				this->clearBody();
				// Формируем новое тело сообщения
				this->_web.body(result);
			}
		}
		// Проверяем пришли ли сжатые данные
		const string & encoding = this->_web.header("content-encoding");
		// Если данные пришли сжатые
		if(!encoding.empty()){
			// Если данные пришли сжатые методом Brotli
			if(this->_fmk->compare(encoding, "br")){
				// Устанавливаем требование выполнять декомпрессию тела сообщения
				this->_compress = compress_t::BROTLI;
				// Выполняем декомпрессию данных
				const auto & result = this->_hash.decompress(body.data(), body.size(), hash_t::method_t::BROTLI);
				// Заменяем полученное тело
				if(!result.empty()){
					// Очищаем тело сообщения
					this->clearBody();
					// Формируем новое тело сообщения
					this->_web.body(result);
				}
			// Если данные пришли сжатые методом GZip
			} else if(this->_fmk->compare(encoding, "gzip")) {
				// Устанавливаем требование выполнять декомпрессию тела сообщения
				this->_compress = compress_t::GZIP;
				// Выполняем декомпрессию данных
				const auto & result = this->_hash.decompress(body.data(), body.size(), hash_t::method_t::GZIP);
				// Заменяем полученное тело
				if(!result.empty()){
					// Очищаем тело сообщения
					this->clearBody();
					// Формируем новое тело сообщения
					this->_web.body(result);
				}
			// Если данные пришли сжатые методом Deflate
			} else if(this->_fmk->compare(encoding, "deflate")) {
				// Устанавливаем требование выполнять декомпрессию тела сообщения
				this->_compress = compress_t::DEFLATE;
				// Получаем данные тела в бинарном виде
				vector <char> buffer(body.begin(), body.end());
				// Добавляем хвост в полученные данные
				this->_hash.setTail(buffer);
				// Выполняем декомпрессию данных
				const auto & result = this->_hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::DEFLATE);
				// Заменяем полученное тело
				if(!result.empty()){
					// Очищаем тело сообщения
					this->clearBody();
					// Формируем новое тело сообщения
					this->_web.body(result);
				}
			// Отключаем сжатие тела сообщения
			} else this->_compress = compress_t::NONE;
		// Отключаем сжатие тела сообщения
		} else this->_compress = compress_t::NONE;
	}
	// Если мы работаем с HTTP сервером и метод компрессии установлен
	if((this->_web.hid() == web_t::hid_t::SERVER) && (this->_compress != compress_t::NONE)){
		// Список запрашиваемых методов
		set <compress_t> compress;
		// Если заголовок с запрашиваемой кодировкой существует
		if(this->_web.isHeader("accept-encoding")){
			// Переходим по всему списку заголовков
			for(auto & header : this->_web.headers()){
				// Если заголовок найден
				if(this->_fmk->compare(header.first, "accept-encoding")){
					// Если конкретный метод сжатия не запрашивается
					if(this->_fmk->compare(header.second, "*")) break;
					// Если запрашиваются конкретные методы сжатия
					else {
						// Если найден запрашиваемый метод компрессии BROTLI
						if(this->_fmk->exists("br", header.second)){
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
						if(this->_fmk->exists("gzip", header.second)){
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
						if(this->_fmk->exists("deflate", header.second)){
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
	this->_web.state(web_t::state_t::END);
}
/**
 * clear Метод очистки собранных данных
 */
void awh::Http::clear() noexcept {
	// Выполняем очистку данных парсера
	this->_web.clear();
	// Выполняем сброс чёрного списка HTTP заголовков
	this->_black.clear();
}
/**
 * reset Метод сброса параметров запроса
 */
void awh::Http::reset() noexcept {
	// Выполняем сброс данных парсера
	this->_web.reset();
	// Выполняем сброс стейта авторизации
	this->_stath = stath_t::NONE;
	// Выполняем сброс стейта текущего запроса
	this->_state = state_t::NONE;
}
/**
 * rmBlack Метод удаления заголовка из чёрного списка
 * @param key ключ заголовка
 */
void awh::Http::rmBlack(const string & key) noexcept {
	// Если ключ заголовка передан, удаляем его
	if(!key.empty())
		// Выполняем удаление заголовка из чёрного списка
		this->_black.erase(this->_fmk->transform(key, fmk_t::transform_t::LOWER));
}
/**
 * addBlack Метод добавления заголовка в чёрный список
 * @param key ключ заголовка
 */
void awh::Http::addBlack(const string & key) noexcept {
	// Если ключ заголовка передан, добавляем в список
	if(!key.empty())
		// Выполняем добавление заголовка в чёрный список
		this->_black.emplace(this->_fmk->transform(key, fmk_t::transform_t::LOWER));
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
	if((this->_state != state_t::GOOD) && (this->_state != state_t::BROKEN) && (this->_state != state_t::BROKEN)){
		// Выполняем парсинг сырых данных
		result = this->_web.parse(buffer, size);
		// Если парсинг выполнен
		if(this->_web.isEnd())
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
	vector <char> * body = const_cast <vector <char> *> (&this->_web.body());
	// Если данные тела ещё существуют
	if(!body->empty()){
		// Версия протокола HTTP
		float version = 1.1f;
		// Определяем тип HTTP модуля
		switch(static_cast <uint8_t> (this->_web.hid())){
			// Если мы работаем с клиентом
			case static_cast <uint8_t> (web_t::hid_t::CLIENT):
				// Выполняем получение версии HTTP-протокола
				version = this->_web.request().version;
			break;
			// Если мы работаем с сервером
			case static_cast <uint8_t> (web_t::hid_t::SERVER):
				// Выполняем получение версии HTTP-протокола
				version = this->_web.response().version;
			break;
		}
		// Если нужно тело выводить в виде чанков
		if((version > 1.0f) && this->_chunking){
			// Если версия протокола интернета выше 1.1
			if(version > 1.1f){
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
					chunk = this->_fmk->itoa(this->_chunk, 16);
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
					chunk = this->_fmk->itoa(body->size(), 16);
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
	this->_web.clearBody();
}
/**
 * clearHeaders Метод очистки списка заголовков
 */
void awh::Http::clearHeaders() const noexcept {
	// Выполняем очистку списка заголовков
	this->_web.clearHeaders();
}
/**
 * body Метод получения данных тела запроса
 * @return буфер данных тела запроса
 */
const vector <char> & awh::Http::body() const noexcept {
	// Выводим данные тела
	return this->_web.body();
}
/**
 * body Метод установки данных тела
 * @param body буфер тела для установки
 */
void awh::Http::body(const vector <char> & body) noexcept {
	// Устанавливаем данные телал сообщения
	this->_web.body(body);
}
/**
 * rmHeader Метод удаления заголовка
 * @param key ключ заголовка
 */
void awh::Http::rmHeader(const string & key) noexcept {
	// Выполняем удаление заголовка
	this->_web.rmHeader(key);
}
/**
 * header Метод получения данных заголовка
 * @param key ключ заголовка
 * @return    значение заголовка
 */
const string awh::Http::header(const string & key) const noexcept {
	// Выводим запрашиваемый заголовок
	return this->_web.header(key);
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
		this->_web.header(key, val);
}
/**
 * headers Метод получения списка заголовков
 * @return список существующих заголовков
 */
const unordered_multimap <string, string> & awh::Http::headers() const noexcept {
	// Выводим список доступных заголовков
	return this->_web.headers();
}
/**
 * headers Метод установки списка заголовков
 * @param headers список заголовков для установки
 */
void awh::Http::headers(const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем заголовки сообщения
	this->_web.headers(headers);
}
/**
 * header2 Метод добавления заголовка в формате HTTP/2
 * @param key ключ заголовка
 * @param val значение заголовка
 */
void awh::Http::header2(const string & key, const string & val) noexcept {
	// Определяем соответствует ли ключ методу запроса
	if(this->_fmk->compare(key, ":method")){
		// Получаем объект параметров запроса
		web_t::req_t request = this->_web.request();
		// Устанавливаем версию протокола
		request.version = 2.0f;
		// Если метод является GET запросом
		if(this->_fmk->compare(val, "GET"))
			// Выполняем установку метода запроса GET
			request.method = web_t::method_t::GET;
		// Если метод является PUT запросом
		else if(this->_fmk->compare(val, "PUT"))
			// Выполняем установку метода запроса PUT
			request.method = web_t::method_t::PUT;
		// Если метод является POST запросом
		else if(this->_fmk->compare(val, "POST"))
			// Выполняем установку метода запроса POST
			request.method = web_t::method_t::POST;
		// Если метод является HEAD запросом
		else if(this->_fmk->compare(val, "HEAD"))
			// Выполняем установку метода запроса HEAD
			request.method = web_t::method_t::HEAD;
		// Если метод является PATCH запросом
		else if(this->_fmk->compare(val, "PATCH"))
			// Выполняем установку метода запроса PATCH
			request.method = web_t::method_t::PATCH;
		// Если метод является TRACE запросом
		else if(this->_fmk->compare(val, "TRACE"))
			// Выполняем установку метода запроса TRACE
			request.method = web_t::method_t::TRACE;
		// Если метод является DELETE запросом
		else if(this->_fmk->compare(val, "DELETE"))
			// Выполняем установку метода запроса DELETE
			request.method = web_t::method_t::DEL;
		// Если метод является OPTIONS запросом
		else if(this->_fmk->compare(val, "OPTIONS"))
			// Выполняем установку метода запроса OPTIONS
			request.method = web_t::method_t::OPTIONS;
		// Если метод является CONNECT запросом
		else if(this->_fmk->compare(val, "CONNECT"))
			// Выполняем установку метода запроса CONNECT
			request.method = web_t::method_t::CONNECT;
		// Выполняем сохранение параметров запроса
		this->_web.request(std::move(request));
	// Если ключ запроса соответствует пути запроса
	} else if(this->_fmk->compare(key, ":path")) {
		// Получаем объект параметров запроса
		web_t::req_t request = this->_web.request();
		// Выполняем установку пути запроса
		this->_uri.create(request.url, this->_uri.parse(val));
		// Выполняем сохранение параметров запроса
		this->_web.request(std::move(request));
	// Если ключ заголовка соответствует схеме протокола
	} else if(this->_fmk->compare(key, ":scheme")) {
		/* Просто пропускаем, потому, что схему мы не используем */
	// Если ключ соответствует доменному имени
	} else if(this->_fmk->compare(key, ":authority")) {
		// Устанавливаем хост
		this->header("Host", val);
		// Получаем объект параметров запроса
		web_t::req_t request = this->_web.request();
		// Выполняем установку хоста запроса
		this->_uri.create(request.url, this->_uri.parse(val));
		// Выполняем сохранение параметров запроса
		this->_web.request(std::move(request));
	// Если ключ соответствует статусу ответа
	} else if(this->_fmk->compare(key, ":status")) {
		// Получаем объект параметров ответа
		web_t::res_t response = this->_web.response();
		// Устанавливаем версию протокола
		response.version = 2.0f;
		// Выполняем установку статуса ответа
		response.code = static_cast <u_int> (::stoi(val));
		// Выполняем формирование текста ответа
		response.message = this->message(response.code);
		// Выполняем сохранение параметров ответа
		this->_web.response(std::move(response));
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
	return this->_stath;
}
/**
 * getAuth Метод извлечения строки авторизации
 * @param flag   флаг выполняемого процесса
 * @param method метод выполняемого запроса
 * @return       строка авторизации на удалённом сервере
 */
string awh::Http::getAuth(const process_t flag, const web_t::method_t method) const noexcept {
	// Определяем флаг выполняемого процесса
	switch(static_cast <uint8_t> (flag)){
		// Если нужно сформировать данные запроса
		case static_cast <uint8_t> (process_t::REQUEST): {
			// Определяем метод запроса
			switch(static_cast <uint8_t> (method)){
				// Если метод запроса указан как GET
				case static_cast <uint8_t> (web_t::method_t::GET):
					// Получаем параметры авторизации
					return this->_auth.client.auth("get");
				// Если метод запроса указан как PUT
				case static_cast <uint8_t> (web_t::method_t::PUT):
					// Получаем параметры авторизации
					return this->_auth.client.auth("put");
				// Если метод запроса указан как POST
				case static_cast <uint8_t> (web_t::method_t::POST):
					// Получаем параметры авторизации
					return this->_auth.client.auth("post");
				// Если метод запроса указан как HEAD
				case static_cast <uint8_t> (web_t::method_t::HEAD):
					// Получаем параметры авторизации
					return this->_auth.client.auth("head");
				// Если метод запроса указан как DELETE
				case static_cast <uint8_t> (web_t::method_t::DEL):
					// Получаем параметры авторизации
					return this->_auth.client.auth("delete");
				// Если метод запроса указан как PATCH
				case static_cast <uint8_t> (web_t::method_t::PATCH):
					// Получаем параметры авторизации
					return this->_auth.client.auth("patch");
				// Если метод запроса указан как TRACE
				case static_cast <uint8_t> (web_t::method_t::TRACE):
					// Получаем параметры авторизации
					return this->_auth.client.auth("trace");
				// Если метод запроса указан как OPTIONS
				case static_cast <uint8_t> (web_t::method_t::OPTIONS):
					// Получаем параметры авторизации
					return this->_auth.client.auth("options");
				// Если метод запроса указан как CONNECT
				case static_cast <uint8_t> (web_t::method_t::CONNECT):
					// Получаем параметры авторизации
					return this->_auth.client.auth("connect");
			}
		} break;
		// Если нужно сформировать данные ответа
		case static_cast <uint8_t> (process_t::RESPONSE):
			// Получаем параметры авторизации
			return this->_auth.server;
	}
	// Выводим результат
	return "";
}
/**
 * getUrl Метод извлечения параметров запроса
 * @return установленные параметры запроса
 */
const awh::uri_t::url_t & awh::Http::getUrl() const noexcept {
	// Выводим параметры запроса
	return this->_web.request().url;
}
/**
 * compression Метод извлечения метода компрессии
 * @return метод компрессии
 */
awh::Http::compress_t awh::Http::compression() const noexcept {
	// Результат работы функции
	compress_t result = compress_t::NONE;
	// Проверяем пришли ли сжатые данные
	const string & encoding = this->_web.header("content-encoding");
	// Если данные пришли сжатые
	if(!encoding.empty()){
		// Если данные пришли сжатые методом Brotli
		if(this->_fmk->compare(encoding, "br"))
			// Устанавливаем требование выполнять декомпрессию тела сообщения
			result = compress_t::BROTLI;
		// Если данные пришли сжатые методом GZip
		else if(this->_fmk->compare(encoding, "gzip"))
			// Устанавливаем требование выполнять декомпрессию тела сообщения
			result = compress_t::GZIP;
		// Если данные пришли сжатые методом Deflate
		else if(this->_fmk->compare(encoding, "deflate"))
			// Устанавливаем требование выполнять декомпрессию тела сообщения
			result = compress_t::DEFLATE;
	}
	// Если мы работаем с HTTP сервером и метод компрессии установлен
	if(this->_web.hid() == web_t::hid_t::SERVER){
		// Если заголовок с запрашиваемой кодировкой существует
		if(this->_web.isHeader("accept-encoding")){
			// Переходим по всему списку заголовков
			for(auto & header : this->_web.headers()){
				// Если заголовок найден
				if(this->_fmk->compare(header.first, "accept-encoding")){
					// Если конкретный метод сжатия не запрашивается
					if(this->_fmk->compare(header.second, "*"))
						// Устанавливаем требование выполнять декомпрессию тела сообщения
						return compress_t::BROTLI;
					// Если запрашиваются конкретные методы сжатия
					else {
						// Если найден запрашиваемый метод компрессии BROTLI
						if(this->_fmk->exists("br", header.second))
							// Устанавливаем требование выполнять декомпрессию тела сообщения
							return compress_t::BROTLI;
						// Если найден запрашиваемый метод компрессии GZip
						else if(this->_fmk->exists("gzip", header.second))
							// Устанавливаем требование выполнять декомпрессию тела сообщения
							result = compress_t::GZIP;
						// Если найден запрашиваемый метод компрессии Deflate
						else if(this->_fmk->exists("deflate", header.second)){
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
 * dump Метод получения бинарного дампа
 * @return бинарный дамп данных
 */
vector <char> awh::Http::dump() const noexcept {
	// Результат работы функции
	vector <char> result;
	{
		// Длина строки, количество элементов
		size_t length = 0, count = 0;
		// Устанавливаем метод компрессии отправляемых данных
		result.insert(result.end(), (const char *) &this->_compress, (const char *) &this->_compress + sizeof(this->_compress));
		// Устанавливаем стейт проверки авторизации
		result.insert(result.end(), (const char *) &this->_stath, (const char *) &this->_stath + sizeof(this->_stath));
		// Устанавливаем стейт текущего запроса
		result.insert(result.end(), (const char *) &this->_state, (const char *) &this->_state + sizeof(this->_state));
		// Устанавливаем флаг зашифрованных данных
		result.insert(result.end(), (const char *) &this->_crypt, (const char *) &this->_crypt + sizeof(this->_crypt));
		// Устанавливаем флаг разрешающий передавать тело запроса чанками
		result.insert(result.end(), (const char *) &this->_chunking, (const char *) &this->_chunking + sizeof(this->_chunking));
		// Устанавливаем размер одного чанка
		result.insert(result.end(), (const char *) &this->_chunk, (const char *) &this->_chunk + sizeof(this->_chunk));
		// Получаем размер идентификатора сервиса
		length = this->_ident.id.size();
		// Устанавливаем размер идентификатора сервиса
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Устанавливаем данные идентификатора сервиса
		result.insert(result.end(), this->_ident.id.begin(), this->_ident.id.end());
		// Получаем размер версии модуля приложения
		length = this->_ident.version.size();
		// Устанавливаем размер версии модуля приложения
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Устанавливаем данные версии модуля приложения
		result.insert(result.end(), this->_ident.version.begin(), this->_ident.version.end());
		// Получаем размер названия сервиса
		length = this->_ident.name.size();
		// Устанавливаем размер названия сервиса
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Устанавливаем данные названия сервиса
		result.insert(result.end(), this->_ident.name.begin(), this->_ident.name.end());
		// Получаем размер User-Agent для HTTP запроса
		length = this->_userAgent.size();
		// Устанавливаем размер User-Agent для HTTP запроса
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Устанавливаем данные User-Agent для HTTP запроса
		result.insert(result.end(), this->_userAgent.begin(), this->_userAgent.end());
		// Получаем количество записей чёрного списка
		count = this->_black.size();
		// Устанавливаем количество записей чёрного списка
		result.insert(result.end(), (const char *) &count, (const char *) &count + sizeof(count));
		// Выполняем переход по всему чёрному списку
		for(auto & header : this->_black){
			// Получаем размер заголовка из чёрного списка
			length = header.size();
			// Устанавливаем размер заголовка из чёрного списка
			result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
			// Устанавливаем данные заголовка из чёрного списка
			result.insert(result.end(), header.begin(), header.end());
		}
		// Получаем дамп WEB данных
		const auto & web = this->_web.dump();
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
		// Выполняем получение метода компрессии отправляемых данных
		memcpy((void *) &this->_compress, data.data() + offset, sizeof(this->_compress));
		// Выполняем смещение в буфере
		offset += sizeof(this->_compress);
		// Выполняем получение стейта проверки авторизации
		memcpy((void *) &this->_stath, data.data() + offset, sizeof(this->_stath));
		// Выполняем смещение в буфере
		offset += sizeof(this->_stath);
		// Выполняем получение стейта текущего запроса
		memcpy((void *) &this->_state, data.data() + offset, sizeof(this->_state));
		// Выполняем смещение в буфере
		offset += sizeof(this->_state);
		// Выполняем получение флага зашифрованных данных
		memcpy((void *) &this->_crypt, data.data() + offset, sizeof(this->_crypt));
		// Выполняем смещение в буфере
		offset += sizeof(this->_crypt);
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
		this->_ident.id.resize(length, 0);
		// Выполняем получение данных идентификатора сервиса
		memcpy((void *) this->_ident.id.data(), data.data() + offset, length);
		// Выполняем смещение в буфере
		offset += length;
		// Выполняем получение размера версии модуля приложения
		memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Выделяем память для данных версии модуля приложения
		this->_ident.version.resize(length, 0);
		// Выполняем получение данных версии модуля приложения
		memcpy((void *) this->_ident.version.data(), data.data() + offset, length);
		// Выполняем смещение в буфере
		offset += length;
		// Выполняем получение размера названия сервиса
		memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Выделяем память для данных названия сервиса
		this->_ident.name.resize(length, 0);
		// Выполняем получение данных названия сервиса
		memcpy((void *) this->_ident.name.data(), data.data() + offset, length);
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
		// Выполняем получение количества записей чёрного списка
		memcpy((void *) &count, data.data() + offset, sizeof(count));
		// Выполняем смещение в буфере
		offset += sizeof(count);
		// Выполняем сброс заголовков чёрного списка
		this->_black.clear();
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
				this->_black.emplace(std::move(header));
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
		if(!buffer.empty()) this->_web.dump(std::move(buffer));
	}
}
/**
 * isEnd Метод проверки завершения обработки
 * @return результат проверки
 */
bool awh::Http::isEnd() const noexcept {
	// Выводрим результат проверки
	return (
		(this->_state == state_t::GOOD) ||
		(this->_state == state_t::BROKEN) ||
		(this->_state == state_t::HANDSHAKE)
	);
}
/**
 * isCrypt Метод проверки на зашифрованные данные
 * @return флаг проверки на зашифрованные данные
 */
bool awh::Http::isCrypt() const noexcept {
	// Выводим результат проверки
	return this->_crypt;
}
/**
 * isAlive Метод проверки на постоянное подключение
 * @return результат проверки
 */
bool awh::Http::isAlive() const noexcept {
	// Результат работы функции
	bool result = true;
	// Запрашиваем заголовок подключения
	const string & header = this->_web.header("connection");
	// Если заголовок подключения найден
	if(!header.empty())
		// Выполняем проверку является ли соединение закрытым
		result = !this->_fmk->compare(header, "close");
	// Если заголовок подключения не найден
	else {
		// Переходим по всему списку заголовков
		for(auto & header : this->_web.headers()){
			// Если заголовок найден
			if(this->_fmk->compare(header.first, "connection")){
				// Выполняем проверку является ли соединение закрытым
				result = !this->_fmk->compare(header.second, "close");
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
	return (this->_state == state_t::HANDSHAKE);
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
		result = (this->_black.find(this->_fmk->transform(key, fmk_t::transform_t::LOWER)) != this->_black.end());
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
	return this->_web.isHeader(key);
}
/**
 * request Метод получения объекта запроса на сервер
 * @return объект запроса на сервер
 */
const awh::web_t::req_t & awh::Http::request() const noexcept {
	// Выводим объект запроса на сервер
	return this->_web.request();
}
/**
 * request Метод добавления объекта запроса на сервер
 * @param req объект запроса на сервер
 */
void awh::Http::request(const web_t::req_t & req) noexcept {
	// Устанавливаем объект запроса на сервер
	this->_web.request(req);
}
/**
 * response Метод получения объекта ответа сервера
 * @return объект ответа сервера
 */
const awh::web_t::res_t & awh::Http::response() const noexcept {
	// Выводим объект ответа сервера
	return this->_web.response();
}
/**
 * response Метод добавления объекта ответа сервера
 * @param res объект ответа сервера
 */
void awh::Http::response(const web_t::res_t & res) noexcept {
	// Устанавливаем объект ответа сервера
	this->_web.response(res);
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
	static const string result = "";
	// Выполняем поиск кода сообщения
	auto it = this->messages.find(code);
	// Если код сообщения найден
	if(it != this->messages.end())
		// Выводим сообщение на код ответа
		return it->second;
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
		const string & encrypt = this->_web.header("x-awh-encryption");
		// Если заголовок найден
		if(!encrypt.empty()){
			// Определяем размер шифрования
			switch(stoi(encrypt)){
				// Если шифрование произведено 128 битным ключём
				case 128: this->_dhash.cipher(hash_t::cipher_t::AES128); break;
				// Если шифрование произведено 192 битным ключём
				case 192: this->_dhash.cipher(hash_t::cipher_t::AES192); break;
				// Если шифрование произведено 256 битным ключём
				case 256: this->_dhash.cipher(hash_t::cipher_t::AES256); break;
			}
			// Выполняем дешифрование полученных данных
			const auto & res = this->_dhash.decrypt(result.data.data(), result.data.size());
			// Если данные расшифрованны, заменяем тело данных
			if((result.encrypt = !res.empty())) result.data.assign(res.begin(), res.end());
		}
		// Проверяем пришли ли сжатые данные
		const string & encoding = this->_web.header("content-encoding");
		// Если данные пришли сжатые
		if(!encoding.empty()){
			// Если данные пришли сжатые методом Brotli
			if(this->_fmk->compare(encoding, "br")){
				// Выполняем декомпрессию данных
				const auto & res = this->_dhash.decompress(result.data.data(), result.data.size(), hash_t::method_t::BROTLI);
				// Заменяем полученное тело
				if((result.compress = !res.empty())) result.data.assign(res.begin(), res.end());
			// Если данные пришли сжатые методом GZip
			} else if(this->_fmk->compare(encoding, "gzip")) {
				// Выполняем декомпрессию данных
				const auto & res = this->_dhash.decompress(result.data.data(), result.data.size(), hash_t::method_t::GZIP);
				// Заменяем полученное тело
				if((result.compress = !res.empty())) result.data.assign(res.begin(), res.end());
			// Если данные пришли сжатые методом Deflate
			} else if(this->_fmk->compare(encoding, "deflate")) {
				// Получаем данные тела в бинарном виде
				vector <char> buffer(result.data.begin(), result.data.end());
				// Добавляем хвост в полученные данные
				this->_dhash.setTail(buffer);
				// Выполняем декомпрессию данных
				const auto & res = this->_dhash.decompress(buffer.data(), buffer.size(), hash_t::method_t::DEFLATE);
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
		if(this->_crypt && !this->isBlack("X-AWH-Encryption")){
			// Выполняем шифрование переданных данных
			const auto & res = this->_hash.encrypt(result.data.data(), result.data.size());
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
					const auto & brotli = this->_hash.compress(result.data.data(), result.data.size(), hash_t::method_t::BROTLI);
					// Если данные сжаты, заменяем тело данных
					if((result.compress = !brotli.empty())) result.data.assign(brotli.begin(), brotli.end());
				} break;
				// Если нужно сжать тело методом GZIP
				case static_cast <uint8_t> (compress_t::GZIP): {
					// Выполняем сжатие тела сообщения
					const auto & gzip = this->_hash.compress(result.data.data(), result.data.size(), hash_t::method_t::GZIP);
					// Если данные сжаты, заменяем тело данных
					if((result.compress = !gzip.empty())) result.data.assign(gzip.begin(), gzip.end());
				} break;
				// Если нужно сжать тело методом DEFLATE
				case static_cast <uint8_t> (compress_t::DEFLATE): {
					// Выполняем сжатие тела сообщения
					auto deflate = this->_hash.compress(result.data.data(), result.data.size(), hash_t::method_t::DEFLATE);
					// Удаляем хвост в полученных данных
					this->_hash.rmTail(deflate);
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
 * proxy Метод создания запроса для авторизации на прокси-сервере
 * @param req объект параметров REST-запроса
 * @return    буфер данных запроса в бинарном виде
 */
vector <char> awh::Http::proxy(const web_t::req_t & req) const noexcept {
	// Если хост сервера получен
	if(!req.url.host.empty() && (req.url.port > 0) && (req.method == web_t::method_t::CONNECT)){
		// Добавляем в чёрный список заголовок Accept
		const_cast <http_t *> (this)->addBlack("Accept");
		// Добавляем в чёрный список заголовок Accept-Language
		const_cast <http_t *> (this)->addBlack("Accept-Language");
		// Добавляем в чёрный список заголовок Accept-Encoding
		const_cast <http_t *> (this)->addBlack("Accept-Encoding");
		// Если заголовок подключения ещё не существует
		if(!this->_web.isHeader("connection"))
			// Добавляем поддержку постоянного подключения
			const_cast <http_t *> (this)->header("Connection", "Keep-Alive");
		// Если заголовок подключения прокси ещё не существует
		if(!this->_web.isHeader("proxy-connection"))
			// Добавляем поддержку постоянного подключения для прокси-сервера
			const_cast <http_t *> (this)->header("Proxy-Connection", "Keep-Alive");
		// Устанавливаем параметры REST-запроса
		this->_auth.client.uri(this->_uri.url(req.url));
		// Устанавливаем парарметр запроса
		this->_web.request(req);
		// Выполняем создание запроса
		return this->process(process_t::REQUEST, req);
	}
	// Выводим результат
	return vector <char> ();
}
/**
 * proxy2 Метод создания запроса для авторизации на прокси-сервере (для протокола HTTP/2)
 * @param req объект параметров REST-запроса
 * @return    буфер данных запроса в бинарном виде
 */
vector <pair <string, string>> awh::Http::proxy2(const web_t::req_t & req) const noexcept {
	// Если хост сервера получен
	if(!req.url.host.empty() && (req.url.port > 0) && (req.method == web_t::method_t::CONNECT)){
		// Добавляем в чёрный список заголовок Accept
		const_cast <http_t *> (this)->addBlack("Accept");
		// Добавляем в чёрный список заголовок Accept-Language
		const_cast <http_t *> (this)->addBlack("Accept-Language");
		// Добавляем в чёрный список заголовок Accept-Encoding
		const_cast <http_t *> (this)->addBlack("Accept-Encoding");
		// Устанавливаем параметры REST-запроса
		this->_auth.client.uri(this->_uri.url(req.url));
		// Устанавливаем парарметр запроса
		this->_web.request(req);
		// Выполняем создание запроса
		return this->process2(process_t::REQUEST, req);
	}
	// Выводим результат
	return vector <pair <string, string>> ();
}
/**
 * reject Метод создания отрицательного ответа
 * @param req объект параметров REST-ответа
 * @return    буфер данных ответа в бинарном виде
 */
vector <char> awh::Http::reject(const web_t::res_t & res) const noexcept {
	// Если текст сообщения не установлен
	if(res.message.empty())
		// Выполняем установку сообщения
		const_cast <web_t::res_t &> (res).message = this->message(res.code);
	// Если сообщение получено
	if(!res.message.empty()){
		// Если заголовок подключения ещё не существует
		if(!this->_web.isHeader("connection")){
			// Если требуется ввод авторизационных данных
			if((res.code == 401) || (res.code == 407))
				// Добавляем заголовок закрытия подключения
				this->_web.header("Connection", "Keep-Alive");
			// Добавляем заголовок закрытия подключения
			else this->_web.header("Connection", "Close");
		}
		// Если заголовок типа контента не существует
		if(!this->_web.isHeader("content-type"))
			// Добавляем заголовок тип контента
			this->_web.header("Content-type", "text/html; charset=utf-8");
		// Если запрос должен содержать тело сообщения
		if((res.code >= 200) && (res.code != 204) && (res.code != 304) && (res.code != 308)){
			// Получаем данные тела
			const auto & body = this->_web.body();
			// Если тело ответа не установлено, устанавливаем своё
			if(body.empty()){
				// Формируем тело ответа
				const string & body = this->_fmk->format(
					"<html>\n<head>\n<title>%u %s</title>\n</head>\n<body>\n<h2>%u %s</h2>\n</body>\n</html>\n",
					res.code, res.message.c_str(), res.code, res.message.c_str()
				);
				// Выполняем очистку тела сообщения
				this->clearBody();
				// Добавляем тело сообщения
				this->_web.body(vector <char> (body.begin(), body.end()));
			}
			// Удаляем размер передаваемого контента
			this->_web.rmHeader("content-length");
			// Добавляем заголовок тела сообщения
			this->_web.header("Content-Length", ::to_string(body.size()));
		}
		// Устанавливаем парарметр ответа
		this->_web.response(res);
		// Выводим результат
		return this->process(process_t::RESPONSE, res);
	}
	// Выводим результат
	return vector <char> ();
}
/**
 * reject2 Метод создания отрицательного ответа (для протокола HTTP/2)
 * @param req объект параметров REST-ответа
 * @return    буфер данных ответа в бинарном виде
 */
vector <pair <string, string>> awh::Http::reject2(const web_t::res_t & res) const noexcept {
	// Если текст сообщения не установлен
	if(res.message.empty())
		// Выполняем установку сообщения
		const_cast <web_t::res_t &> (res).message = this->message(res.code);
	// Если сообщение получено
	if(!res.message.empty()){
		// Если заголовок подключения ещё не существует
		if(!this->_web.isHeader("connection")){
			// Если требуется ввод авторизационных данных
			if((res.code == 401) || (res.code == 407))
				// Добавляем заголовок закрытия подключения
				this->_web.header("Connection", "Keep-Alive");
			// Добавляем заголовок закрытия подключения
			else this->_web.header("Connection", "Close");
		}
		// Если заголовок типа контента не существует
		if(!this->_web.isHeader("content-type"))
			// Добавляем заголовок тип контента
			this->_web.header("Content-type", "text/html; charset=utf-8");
		// Если запрос должен содержать тело сообщения
		if((res.code >= 200) && (res.code != 204) && (res.code != 304) && (res.code != 308)){
			// Получаем данные тела
			const auto & body = this->_web.body();
			// Если тело ответа не установлено, устанавливаем своё
			if(body.empty()){
				// Формируем тело ответа
				const string & body = this->_fmk->format(
					"<html>\n<head>\n<title>%u %s</title>\n</head>\n<body>\n<h2>%u %s</h2>\n</body>\n</html>\n",
					res.code, res.message.c_str(), res.code, res.message.c_str()
				);
				// Выполняем очистку тела сообщения
				this->clearBody();
				// Добавляем тело сообщения
				this->_web.body(vector <char> (body.begin(), body.end()));
			}
			// Удаляем размер передаваемого контента
			this->_web.rmHeader("content-length");
			// Добавляем заголовок тела сообщения
			this->_web.header("Content-Length", ::to_string(body.size()));
		}
		// Устанавливаем парарметр ответа
		this->_web.response(res);
		// Выводим результат
		return this->process2(process_t::RESPONSE, res);
	}
	// Выводим результат
	return vector <pair <string, string>> ();
}
/**
 * process Метод создания выполняемого процесса в бинарном виде
 * @param flag   флаг выполняемого процесса
 * @param nobody флаг запрета подготовки тела
 * @return       буфер данных в бинарном виде
 */
vector <char> awh::Http::process(const process_t flag, const bool nobody) const noexcept {
	// Результат работы функции
	vector <char> result;
	/**
	 * Типы основных заголовков
	 */
	bool available[4] = {
		false, // Content-Length
		false, // Content-Encoding
		false, // Transfer-Encoding
		false  // X-AWH-Encryption
	};
	// Определяем флаг выполняемого процесса
	switch(static_cast <uint8_t> (flag)){
		// Если нужно сформировать данные запроса
		case static_cast <uint8_t> (process_t::REQUEST): {
			// Получаем объект параметров запроса
			const web_t::req_t & req = this->_web.request();
			// Если параметры запроса получены
			if(!req.url.empty() && (req.method != web_t::method_t::NONE)){
				// Размер тела сообщения
				size_t length = 0;
				// Данные REST-запроса
				string request = "";
				// Определяем метод запроса
				switch(static_cast <uint8_t> (req.method)){
					// Если метод запроса указан как GET
					case static_cast <uint8_t> (web_t::method_t::GET):
						// Формируем GET запрос
						request = this->_fmk->format("GET %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как PUT
					case static_cast <uint8_t> (web_t::method_t::PUT):
						// Формируем PUT запрос
						request = this->_fmk->format("PUT %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как POST
					case static_cast <uint8_t> (web_t::method_t::POST):
						// Формируем POST запрос
						request = this->_fmk->format("POST %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как HEAD
					case static_cast <uint8_t> (web_t::method_t::HEAD):
						// Формируем HEAD запрос
						request = this->_fmk->format("HEAD %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как PATCH
					case static_cast <uint8_t> (web_t::method_t::PATCH):
						// Формируем PATCH запрос
						request = this->_fmk->format("PATCH %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как TRACE
					case static_cast <uint8_t> (web_t::method_t::TRACE):
						// Формируем TRACE запрос
						request = this->_fmk->format("TRACE %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как DELETE
					case static_cast <uint8_t> (web_t::method_t::DEL):
						// Формируем DELETE запрос
						request = this->_fmk->format("DELETE %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как OPTIONS
					case static_cast <uint8_t> (web_t::method_t::OPTIONS):
						// Формируем OPTIONS запрос
						request = this->_fmk->format("OPTIONS %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как CONNECT
					case static_cast <uint8_t> (web_t::method_t::CONNECT):
						// Формируем CONNECT запрос
						request = this->_fmk->format("CONNECT %s HTTP/%.1f\r\n", this->_fmk->format("%s:%u", req.url.host.c_str(), req.url.port).c_str(), req.version);
					break;
				}
				// Переходим по всему списку заголовков
				for(auto & header : this->_web.headers()){
					// Флаг разрешающий вывода заголовка
					bool allow = !this->isBlack(header.first);
					// Выполняем перебор всех обязательных заголовков
					for(uint8_t i = 0; i < 4; i++){
						// Если заголовок уже найден пропускаем его
						if(available[i]) continue;
						// Выполняем првоерку заголовка
						switch(i){
							case 1: available[i] = this->_fmk->compare(header.first, "content-encoding");  break;
							case 2: available[i] = this->_fmk->compare(header.first, "transfer-encoding"); break;
							case 3: available[i] = this->_fmk->compare(header.first, "x-awh-encryption");  break;
							case 0: {
								// Запоминаем, что мы нашли заголовок размера тела
								available[i] = this->_fmk->compare(header.first, "content-length");
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
							this->_fmk->format(
								"%s: %s\r\n",
								this->_fmk->transform(header.first, fmk_t::transform_t::SMART).c_str(),
								header.second.c_str()
							)
						);
				}
				// Получаем данные тела
				const auto & body = this->_web.body();
				// Если запрос не является GET, HEAD или TRACE, а тело запроса существует
				if((req.method != web_t::method_t::GET) && (req.method != web_t::method_t::HEAD) && (req.method != web_t::method_t::TRACE) && !body.empty()){
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
							this->_web.body(crypto.data);
							// Заменяем размер тела данных
							if(!this->_chunking)
								// Устанавливаем размер тела сообщения
								length = body.size();
						// Заменяем размер тела данных
						} else if(!this->_chunking)
							// Устанавливаем размер тела сообщения
							length = crypto.data.size();
					}
					// Если данные зашифрованы, устанавливаем соответствующие заголовки
					if(crypto.encrypt)
						// Устанавливаем X-AWH-Encryption
						request.append(this->_fmk->format("X-AWH-Encryption: %u\r\n", static_cast <u_short> (this->_hash.cipher())));
					// Если данные сжаты, устанавливаем соответствующие заголовки
					if(crypto.compress){
						// Определяем метод компрессии тела сообщения
						switch(static_cast <uint8_t> (this->_compress)){
							// Если нужно сжать тело методом BROTLI
							case static_cast <uint8_t> (compress_t::BROTLI):
								// Устанавливаем Content-Encoding если не передан
								request.append(this->_fmk->format("Content-Encoding: %s\r\n", "br"));
							break;
							// Если нужно сжать тело методом GZIP
							case static_cast <uint8_t> (compress_t::GZIP):
								// Устанавливаем Content-Encoding если не передан
								request.append(this->_fmk->format("Content-Encoding: %s\r\n", "gzip"));
							break;
							// Если нужно сжать тело методом DEFLATE
							case static_cast <uint8_t> (compress_t::DEFLATE):
								// Устанавливаем Content-Encoding если не передан
								request.append(this->_fmk->format("Content-Encoding: %s\r\n", "deflate"));
							break;
						}
					}
					// Если данные необходимо разбивать на чанки
					if(this->_chunking && !this->isBlack("Transfer-Encoding"))
						// Устанавливаем заголовок Transfer-Encoding
						request.append(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
					// Если заголовок размера передаваемого тела, не запрещён
					else if(!this->isBlack("Content-Length"))
						// Устанавливаем размер передаваемого тела Content-Length
						request.append(this->_fmk->format("Content-Length: %zu\r\n", length));
				// Очищаем тела сообщения
				} else this->clearBody();
				// Устанавливаем завершающий разделитель
				request.append("\r\n");
				// Формируем результат запроса
				result.assign(request.begin(), request.end());
			}
		} break;
		// Если нужно сформировать данные ответа
		case static_cast <uint8_t> (process_t::RESPONSE): {
			// Получаем объект параметров ответа
			const web_t::res_t & res = this->_web.response();
			// Если параметры запроса получены
			if(res.code > 0){
				// Размер тела сообщения
				size_t length = 0;
				// Если ответ не установлен, формируем по умолчанию
				if(res.message.empty())
					// Устанавливаем сообщение по умолчанию
					const_cast <web_t::res_t &> (res).message = this->message(res.code);
				// Данные REST ответа
				string response = this->_fmk->format("HTTP/%.1f %u %s\r\n", res.version, res.code, res.message.c_str());
				// Переходим по всему списку заголовков
				for(auto & header : this->_web.headers()){
					// Флаг разрешающий вывода заголовка
					bool allow = !this->isBlack(header.first);
					// Выполняем перебор всех обязательных заголовков
					for(uint8_t i = 0; i < 4; i++){
						// Если заголовок уже найден пропускаем его
						if(available[i]) continue;
						// Выполняем првоерку заголовка
						switch(i){
							case 1: available[i] = this->_fmk->compare(header.first, "content-encoding");  break;
							case 2: available[i] = this->_fmk->compare(header.first, "transfer-encoding"); break;
							case 3: available[i] = this->_fmk->compare(header.first, "x-awh-encryption");  break;
							case 0: {
								// Запоминаем, что мы нашли заголовок размера тела
								available[i] = this->_fmk->compare(header.first, "content-length");
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
							this->_fmk->format(
								"%s: %s\r\n",
								this->_fmk->transform(header.first, fmk_t::transform_t::SMART).c_str(),
								header.second.c_str()
							)
						);
				}
				// Получаем данные тела
				const auto & body = this->_web.body();
				// Если запрос должен содержать тело и тело ответа существует
				if((res.code >= 200) && (res.code != 204) && (res.code != 304) && (res.code != 308) && !body.empty()){
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
							this->_web.body(crypto.data);
							// Заменяем размер тела данных
							if(!this->_chunking)
								// Устанавливаем размер тела
								length = body.size();
						// Заменяем размер тела данных
						} else if(!this->_chunking)
							// Устанавливаем размер тела
							length = crypto.data.size();
					}
					// Если данные зашифрованы, устанавливаем соответствующие заголовки
					if(crypto.encrypt)
						// Устанавливаем X-AWH-Encryption
						response.append(this->_fmk->format("X-AWH-Encryption: %u\r\n", static_cast <u_short> (this->_hash.cipher())));
					// Если данные сжаты, устанавливаем соответствующие заголовки
					if(crypto.compress){
						// Определяем метод компрессии тела сообщения
						switch(static_cast <uint8_t> (this->_compress)){
							// Если нужно сжать тело методом BROTLI
							case static_cast <uint8_t> (compress_t::BROTLI):
								// Устанавливаем Content-Encoding если не передан
								response.append(this->_fmk->format("Content-Encoding: %s\r\n", "br"));
							break;
							// Если нужно сжать тело методом GZIP
							case static_cast <uint8_t> (compress_t::GZIP):
								// Устанавливаем Content-Encoding если не передан
								response.append(this->_fmk->format("Content-Encoding: %s\r\n", "gzip"));
							break;
							// Если нужно сжать тело методом DEFLATE
							case static_cast <uint8_t> (compress_t::DEFLATE):
								// Устанавливаем Content-Encoding если не передан
								response.append(this->_fmk->format("Content-Encoding: %s\r\n", "deflate"));
							break;
						}
					}
					// Если данные необходимо разбивать на чанки
					if(this->_chunking && !this->isBlack("Transfer-Encoding"))
						// Устанавливаем заголовок Transfer-Encoding
						response.append(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
					// Если заголовок размера передаваемого тела, не запрещён
					else if(!this->isBlack("Content-Length"))
						// Устанавливаем размер передаваемого тела Content-Length
						response.append(this->_fmk->format("Content-Length: %zu\r\n", length));
				// Очищаем тела сообщения
				} else this->clearBody();
				// Устанавливаем завершающий разделитель
				response.append("\r\n");
				// Формируем результат ответа
				result.assign(response.begin(), response.end());
			}
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * process2 Метод создания выполняемого процесса в бинарном виде (для протокола HTTP/2)
 * @param flag   флаг выполняемого процесса
 * @param nobody флаг запрета подготовки тела
 * @return       буфер данных в бинарном виде
 */
vector <pair <string, string>> awh::Http::process2(const process_t flag, const bool nobody) const noexcept {
	// Результат работы функции
	vector <pair <string, string>> result;
	/**
	 * Типы основных заголовков
	 */
	bool available[7] = {
		false, // Host
		false, // Connection
		false, // Proxy-Connection
		false, // Content-Length
		false, // Content-Encoding
		false, // Transfer-Encoding
		false  // X-AWH-Encryption
	};
	// Определяем флаг выполняемого процесса
	switch(static_cast <uint8_t> (flag)){
		// Если нужно сформировать данные запроса
		case static_cast <uint8_t> (process_t::REQUEST): {
			// Получаем объект параметров запроса
			const web_t::req_t & req = this->_web.request();
			// Если параметры запроса получены
			if(!req.url.empty() && (req.method != web_t::method_t::NONE)){
				// Размер тела сообщения
				size_t length = 0;
				// Определяем метод запроса
				switch(static_cast <uint8_t> (req.method)){
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
				// Выполняем установку схемы протокола
				result.push_back(make_pair(":scheme", "https"));
				// Если метод подключения установлен как CONNECT
				if(req.method == web_t::method_t::CONNECT)
					// Формируем URI запроса
					result.push_back(make_pair(":authority", this->_fmk->format("%s:%u", req.url.host.c_str(), req.url.port)));
				// Если метод подключения не является методом CONNECT
				else {
					// Выполняем установку хоста сервера
					result.push_back(make_pair(":authority", req.url.host));
					// Выполняем установку пути запроса
					result.push_back(make_pair(":path", this->_uri.query(req.url)));
				}
				// Переходим по всему списку заголовков
				for(auto & header : this->_web.headers()){
					// Если заголовок является системным
					if(header.first.front() == ':')
						// Формируем строку запроса
						result.push_back(make_pair(this->_fmk->transform(header.first, fmk_t::transform_t::LOWER), header.second));
				}
				// Переходим по всему списку заголовков
				for(auto & header : this->_web.headers()){
					// Если заголовок не является системным
					if(header.first.front() != ':'){
						// Флаг разрешающий вывода заголовка
						bool allow = !this->isBlack(header.first);
						// Выполняем перебор всех обязательных заголовков
						for(uint8_t i = 0; i < 7; i++){
							// Если заголовок уже найден пропускаем его
							if(available[i]) continue;
							// Выполняем првоерку заголовка
							switch(i){
								case 0: available[i] = this->_fmk->compare(header.first, "host");              break;
								case 1: available[i] = this->_fmk->compare(header.first, "connection");        break;
								case 2: available[i] = this->_fmk->compare(header.first, "proxy-connection");  break;
								case 4: available[i] = this->_fmk->compare(header.first, "content-encoding");  break;
								case 5: available[i] = this->_fmk->compare(header.first, "transfer-encoding"); break;
								case 6: available[i] = this->_fmk->compare(header.first, "x-awh-encryption");  break;
								case 3: {
									// Запоминаем, что мы нашли заголовок размера тела
									available[i] = this->_fmk->compare(header.first, "content-length");
									// Устанавливаем размер тела сообщения
									if(available[i]) length = static_cast <size_t> (::stoull(header.second));
								} break;
							}
							// Если заголовок разрешён для вывода
							if(allow){
								// Выполняем првоерку заголовка
								switch(i){
									case 0:
									case 3:
									case 4:
									case 5:
									case 6: allow = !available[i]; break;
									case 1:
									case 2: {
										// Если заголовок Connection определён
										if(available[i])
											// Выполняем определение разрешено ли выводить заголовок
											allow = !this->_fmk->compare(header.second, "keep-alive");
									} break;
								}
							}
						}
						// Если заголовок не является запрещённым, добавляем заголовок в запрос
						if(allow)
							// Формируем строку запроса
							result.push_back(make_pair(this->_fmk->transform(header.first, fmk_t::transform_t::LOWER), header.second));
					}
				}
				// Получаем данные тела
				const auto & body = this->_web.body();
				// Если запрос не является GET, HEAD или TRACE, а тело запроса существует
				if((req.method != web_t::method_t::GET) && (req.method != web_t::method_t::HEAD) && (req.method != web_t::method_t::TRACE) && !body.empty()){
					// Выполняем кодирование данных
					const auto & crypto = this->encode(body);
					// Проверяем нужно ли передать тело разбив на чанки
					this->_chunking = (!available[3] || ((length > 0) && (length != body.size())));
					// Если данные были сжаты, либо зашифрованы
					if(crypto.encrypt || crypto.compress){
						// Если флаг запрета подготовки тела полезной нагрузки не установлен
						if(!nobody){
							// Выполняем очистку тела сообщения
							this->clearBody();
							// Заменяем тело данных
							this->_web.body(crypto.data);
							// Заменяем размер тела данных
							if(!this->_chunking)
								// Устанавливаем размер тела сообщения
								length = body.size();
						// Заменяем размер тела данных
						} else if(!this->_chunking)
							// Устанавливаем размер тела сообщения
							length = crypto.data.size();
					}
					// Если данные зашифрованы, устанавливаем соответствующие заголовки
					if(crypto.encrypt)
						// Устанавливаем X-AWH-Encryption
						result.push_back(make_pair("x-awh-encryption", ::to_string(static_cast <u_short> (this->_hash.cipher()))));
					// Если данные сжаты, устанавливаем соответствующие заголовки
					if(crypto.compress){
						// Определяем метод компрессии тела сообщения
						switch(static_cast <uint8_t> (this->_compress)){
							// Если нужно сжать тело методом BROTLI
							case static_cast <uint8_t> (compress_t::BROTLI):
								// Устанавливаем Content-Encoding если не передан
								result.push_back(make_pair("content-encoding", "br"));
							break;
							// Если нужно сжать тело методом GZIP
							case static_cast <uint8_t> (compress_t::GZIP):
								// Устанавливаем Content-Encoding если не передан
								result.push_back(make_pair("content-encoding", "gzip"));
							break;
							// Если нужно сжать тело методом DEFLATE
							case static_cast <uint8_t> (compress_t::DEFLATE):
								// Устанавливаем Content-Encoding если не передан
								result.push_back(make_pair("content-encoding", "deflate"));
							break;
						}
					}
					// Если данные необходимо разбивать на чанки
					if(this->_chunking && !this->isBlack("Transfer-Encoding"))
						// Устанавливаем заголовок Transfer-Encoding
						result.push_back(make_pair("transfer-encoding", "chunked"));
					// Если заголовок размера передаваемого тела, не запрещён
					else if(!this->isBlack("Content-Length"))
						// Устанавливаем размер передаваемого тела Content-Length
						result.push_back(make_pair("content-length", ::to_string(length)));
				// Очищаем тела сообщения
				} else this->clearBody();
			}
		} break;
		// Если нужно сформировать данные ответа
		case static_cast <uint8_t> (process_t::RESPONSE): {
			// Получаем объект параметров ответа
			const web_t::res_t & res = this->_web.response();
			// Если параметры запроса получены
			if(res.code > 0){
				// Размер тела сообщения
				size_t length = 0;
				// Если ответ не установлен, формируем по умолчанию
				if(res.message.empty())
					// Устанавливаем сообщение по умолчанию
					const_cast <web_t::res_t &> (res).message = this->message(res.code);
				// Данные REST ответа
				result.push_back(make_pair(":status", ::to_string(res.code)));
				// Переходим по всему списку заголовков
				for(auto & header : this->_web.headers()){
					// Флаг разрешающий вывода заголовка
					bool allow = !this->isBlack(header.first);
					// Выполняем перебор всех обязательных заголовков
					for(uint8_t i = 0; i < 7; i++){
						// Если заголовок уже найден пропускаем его
						if(available[i]) continue;
						// Выполняем првоерку заголовка
						switch(i){
							case 1: available[i] = this->_fmk->compare(header.first, "connection");        break;
							case 2: available[i] = this->_fmk->compare(header.first, "proxy-connection");  break;
							case 4: available[i] = this->_fmk->compare(header.first, "content-encoding");  break;
							case 5: available[i] = this->_fmk->compare(header.first, "transfer-encoding"); break;
							case 6: available[i] = this->_fmk->compare(header.first, "x-awh-encryption");  break;
							case 3: {
								// Запоминаем, что мы нашли заголовок размера тела
								available[i] = this->_fmk->compare(header.first, "content-length");
								// Устанавливаем размер тела сообщения
								if(available[i]) length = static_cast <size_t> (::stoull(header.second));
							} break;
						}
						// Если заголовок разрешён для вывода
						if(allow){
							// Выполняем првоерку заголовка
							switch(i){
								case 3:
								case 4:
								case 5:
								case 6: allow = !available[i]; break;
								case 1:
								case 2: {
									// Если заголовок Connection определён
									if(available[i])
										// Выполняем определение разрешено ли выводить заголовок
										allow = !this->_fmk->compare(header.second, "keep-alive");
								} break;
							}
						}
					}
					// Если заголовок не является запрещённым, добавляем заголовок в ответ
					if(allow)
						// Формируем строку запроса
						result.push_back(make_pair(this->_fmk->transform(header.first, fmk_t::transform_t::LOWER), header.second));
				}
				// Получаем данные тела
				const auto & body = this->_web.body();
				// Если запрос должен содержать тело и тело ответа существует
				if((res.code >= 200) && (res.code != 204) && (res.code != 304) && (res.code != 308) && !body.empty()){
					// Выполняем кодирование данных
					const auto & crypto = this->encode(body);
					// Проверяем нужно ли передать тело разбив на чанки
					this->_chunking = (!available[3] || ((length > 0) && (length != body.size())));
					// Если данные были сжаты, либо зашифрованы
					if(crypto.encrypt || crypto.compress){
						// Если флаг запрета подготовки тела полезной нагрузки не установлен
						if(!nobody){
							// Выполняем очистку тела сообщения
							this->clearBody();
							// Заменяем тело данных
							this->_web.body(crypto.data);
							// Заменяем размер тела данных
							if(!this->_chunking)
								// Устанавливаем размер тела
								length = body.size();
						// Заменяем размер тела данных
						} else if(!this->_chunking)
							// Устанавливаем размер тела
							length = crypto.data.size();
					}
					// Если данные зашифрованы, устанавливаем соответствующие заголовки
					if(crypto.encrypt)
						// Устанавливаем X-AWH-Encryption
						result.push_back(make_pair("x-awh-encryption", ::to_string(static_cast <u_short> (this->_hash.cipher()))));
					// Если данные сжаты, устанавливаем соответствующие заголовки
					if(crypto.compress){
						// Определяем метод компрессии тела сообщения
						switch(static_cast <uint8_t> (this->_compress)){
							// Если нужно сжать тело методом BROTLI
							case static_cast <uint8_t> (compress_t::BROTLI):
								// Устанавливаем Content-Encoding если не передан
								result.push_back(make_pair("content-encoding", "br"));
							break;
							// Если нужно сжать тело методом GZIP
							case static_cast <uint8_t> (compress_t::GZIP):
								// Устанавливаем Content-Encoding если не передан
								result.push_back(make_pair("content-encoding", "gzip"));
							break;
							// Если нужно сжать тело методом DEFLATE
							case static_cast <uint8_t> (compress_t::DEFLATE):
								// Устанавливаем Content-Encoding если не передан
								result.push_back(make_pair("content-encoding", "deflate"));
							break;
						}
					}
					// Если данные необходимо разбивать на чанки
					if(this->_chunking && !this->isBlack("Transfer-Encoding"))
						// Устанавливаем заголовок Transfer-Encoding
						result.push_back(make_pair("transfer-encoding", "chunked"));
					// Если заголовок размера передаваемого тела, не запрещён
					else if(!this->isBlack("Content-Length"))
						// Устанавливаем размер передаваемого тела Content-Length
						result.push_back(make_pair("content-length", ::to_string(length)));
				// Очищаем тела сообщения
				} else this->clearBody();
			}
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * process Метод создания выполняемого процесса в бинарном виде
 * @param flag     флаг выполняемого процесса
 * @param provider параметры провайдера обмена сообщениями
 * @return         буфер данных в бинарном виде
 */
vector <char> awh::Http::process(const process_t flag, const web_t::provider_t & provider) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Определяем флаг выполняемого процесса
	switch(static_cast <uint8_t> (flag)){
		// Если нужно сформировать данные запроса
		case static_cast <uint8_t> (process_t::REQUEST): {
			// Получаем объект ответа клиенту
			const web_t::req_t & req = static_cast <const web_t::req_t &> (provider);
			// Если параметры REST-запроса переданы
			if(!req.url.empty() && (req.method != web_t::method_t::NONE)){
				/**
				 * Типы основных заголовков
				 */
				bool available[13] = {
					false, // Host
					false, // Accept
					false, // Origin
					false, // User-Agent
					false, // Connection
					false, // Proxy-Connection
					false, // Content-Length
					false, // Accept-Language
					false, // Accept-Encoding
					false, // Content-Encoding
					false, // Transfer-Encoding
					false, // X-AWH-Encryption
					false  // Proxy-Authorization
				};
				// Размер тела сообщения
				size_t length = 0;
				// Данные REST-запроса
				string request = "";
				// Устанавливаем парарметры запроса
				this->_web.request(req);
				// Устанавливаем параметры REST-запроса
				this->_auth.client.uri(this->_uri.url(req.url));
				// Определяем метод запроса
				switch(static_cast <uint8_t> (req.method)){
					// Если метод запроса указан как GET
					case static_cast <uint8_t> (web_t::method_t::GET):
						// Формируем GET запрос
						request = this->_fmk->format("GET %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как PUT
					case static_cast <uint8_t> (web_t::method_t::PUT):
						// Формируем PUT запрос
						request = this->_fmk->format("PUT %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как POST
					case static_cast <uint8_t> (web_t::method_t::POST):
						// Формируем POST запрос
						request = this->_fmk->format("POST %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как HEAD
					case static_cast <uint8_t> (web_t::method_t::HEAD):
						// Формируем HEAD запрос
						request = this->_fmk->format("HEAD %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как PATCH
					case static_cast <uint8_t> (web_t::method_t::PATCH):
						// Формируем PATCH запрос
						request = this->_fmk->format("PATCH %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как TRACE
					case static_cast <uint8_t> (web_t::method_t::TRACE):
						// Формируем TRACE запрос
						request = this->_fmk->format("TRACE %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как DELETE
					case static_cast <uint8_t> (web_t::method_t::DEL):
						// Формируем DELETE запрос
						request = this->_fmk->format("DELETE %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как OPTIONS
					case static_cast <uint8_t> (web_t::method_t::OPTIONS):
						// Формируем OPTIONS запрос
						request = this->_fmk->format("OPTIONS %s HTTP/%.1f\r\n", this->_uri.query(req.url).c_str(), req.version);
					break;
					// Если метод запроса указан как CONNECT
					case static_cast <uint8_t> (web_t::method_t::CONNECT):
						// Формируем CONNECT запрос
						request = this->_fmk->format("CONNECT %s HTTP/%.1f\r\n", this->_fmk->format("%s:%u", req.url.host.c_str(), req.url.port).c_str(), req.version);
					break;
				}
				// Переходим по всему списку заголовков
				for(auto & header : this->_web.headers()){
					// Флаг разрешающий вывода заголовка
					bool allow = !this->isBlack(header.first);
					// Выполняем перебор всех обязательных заголовков
					for(uint8_t i = 0; i < 12; i++){
						// Если заголовок уже найден пропускаем его
						if(available[i]) continue;
						// Выполняем првоерку заголовка
						switch(i){
							case 0:  available[i] = this->_fmk->compare(header.first, "host");                  break;
							case 1:  available[i] = this->_fmk->compare(header.first, "accept");                break;
							case 2:  available[i] = this->_fmk->compare(header.first, "origin");                break;
							case 3:  available[i] = this->_fmk->compare(header.first, "user-agent");            break;
							case 4:  available[i] = this->_fmk->compare(header.first, "connection");            break;
							case 5:  available[i] = this->_fmk->compare(header.first, "proxy-connection");      break;
							case 7:  available[i] = this->_fmk->compare(header.first, "accept-language");       break;
							case 8:  available[i] = this->_fmk->compare(header.first, "accept-encoding");       break;
							case 9:  available[i] = this->_fmk->compare(header.first, "content-encoding");      break;
							case 10: available[i] = this->_fmk->compare(header.first, "transfer-encoding");     break;
							case 11: available[i] = this->_fmk->compare(header.first, "x-awh-encryption");      break;
							case 12: available[i] = (this->_fmk->compare(header.first, "authorization") ||
													 this->_fmk->compare(header.first, "proxy-authorization")); break;
							case 6: {
								// Запоминаем, что мы нашли заголовок размера тела
								available[i] = this->_fmk->compare(header.first, "content-length");
								// Устанавливаем размер тела сообщения
								if(available[i]) length = static_cast <size_t> (::stoull(header.second));
							} break;
						}
						// Если заголовок разрешён для вывода
						if(allow){
							// Выполняем првоерку заголовка
							switch(i){
								case 6:
								case 9:
								case 10:
								case 11: allow = !available[i]; break;
							}
						}
					}
					// Если заголовок не является запрещённым, добавляем заголовок в запрос
					if(allow)
						// Формируем строку запроса
						request.append(
							this->_fmk->format(
								"%s: %s\r\n",
								this->_fmk->transform(header.first, fmk_t::transform_t::SMART).c_str(),
								header.second.c_str()
							)
						);
				}
				// Устанавливаем Host если не передан
				if(!available[0] && !this->isBlack("Host"))
					// Добавляем заголовок в запрос
					request.append(this->_fmk->format("Host: %s\r\n", req.url.host.c_str()));
				// Устанавливаем Accept если не передан
				if(!available[1] && (req.method != web_t::method_t::CONNECT) && !this->isBlack("Accept"))
					// Добавляем заголовок в запрос
					request.append(this->_fmk->format("Accept: %s\r\n", HTTP_HEADER_ACCEPT));
				// Устанавливаем Connection если не передан
				if(!available[4] && !this->isBlack("Connection"))
					// Добавляем заголовок в запрос
					request.append(this->_fmk->format("Connection: %s\r\n", HTTP_HEADER_CONNECTION));
				// Устанавливаем Accept-Language если не передан
				if(!available[7] && (req.method != web_t::method_t::CONNECT) && !this->isBlack("Accept-Language"))
					// Добавляем заголовок в запрос
					request.append(this->_fmk->format("Accept-Language: %s\r\n", HTTP_HEADER_ACCEPTLANGUAGE));
				// Если нужно запросить компрессию в удобном нам виде
				if(!available[8] && (this->_compress != compress_t::NONE) && (req.method != web_t::method_t::CONNECT) && !this->isBlack("Accept-Encoding")){
					// Определяем метод сжатия который поддерживает клиент
					switch(static_cast <uint8_t> (this->_compress)){
						// Если клиент поддерживает методот сжатия BROTLI
						case static_cast <uint8_t> (compress_t::BROTLI):
							// Добавляем заголовок в запрос
							request.append(this->_fmk->format("Accept-Encoding: %s\r\n", "br"));
						break;
						// Если клиент поддерживает методот сжатия GZIP
						case static_cast <uint8_t> (compress_t::GZIP):
							// Добавляем заголовок в запрос
							request.append(this->_fmk->format("Accept-Encoding: %s\r\n", "gzip"));
						break;
						// Если клиент поддерживает методот сжатия DEFLATE
						case static_cast <uint8_t> (compress_t::DEFLATE):
							// Добавляем заголовок в запрос
							request.append(this->_fmk->format("Accept-Encoding: %s\r\n", "deflate"));
						break;
						// Если клиент поддерживает методот сжатия GZIP, BROTLI
						case static_cast <uint8_t> (compress_t::GZIP_BROTLI):
							// Добавляем заголовок в запрос
							request.append(this->_fmk->format("Accept-Encoding: %s\r\n", "gzip, br"));
						break;
						// Если клиент поддерживает методот сжатия GZIP, DEFLATE
						case static_cast <uint8_t> (compress_t::GZIP_DEFLATE):
							// Добавляем заголовок в запрос
							request.append(this->_fmk->format("Accept-Encoding: %s\r\n", "gzip, deflate"));
						break;
						// Если клиент поддерживает методот сжатия DEFLATE, BROTLI
						case static_cast <uint8_t> (compress_t::DEFLATE_BROTLI):
							// Добавляем заголовок в запрос
							request.append(this->_fmk->format("Accept-Encoding: %s\r\n", "deflate, br"));
						break;
						// Если клиент поддерживает все методы сжатия
						case static_cast <uint8_t> (compress_t::ALL_COMPRESS):
							// Добавляем заголовок в запрос
							request.append(this->_fmk->format("Accept-Encoding: %s\r\n", "gzip, deflate, br"));
						break;
					}
				}
				// Устанавливаем User-Agent если не передан
				if(!available[3] && !this->isBlack("User-Agent")){
					// Если User-Agent установлен стандартный
					if(this->_fmk->compare(this->_userAgent, HTTP_HEADER_AGENT)){
						// Название операционной системы
						const char * os = nullptr;
						// Определяем название операционной системы
						switch(static_cast <uint8_t> (this->_fmk->os())){
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
						this->_userAgent = this->_fmk->format("%s (%s; %s/%s)", this->_ident.name.c_str(), os, this->_ident.id.c_str(), this->_ident.version.c_str());
					}
					// Добавляем заголовок в запрос
					request.append(this->_fmk->format("User-Agent: %s\r\n", this->_userAgent.c_str()));
				}
				// Если заголовок авторизации не передан
				if(!available[12]){
					// Метод HTTP запроса
					string method = "";
					// Определяем метод запроса
					switch(static_cast <uint8_t> (req.method)){
						// Если метод запроса указан как GET
						case static_cast <uint8_t> (web_t::method_t::GET): method = "get"; break;
						// Если метод запроса указан как PUT
						case static_cast <uint8_t> (web_t::method_t::PUT): method = "put"; break;
						// Если метод запроса указан как POST
						case static_cast <uint8_t> (web_t::method_t::POST): method = "post"; break;
						// Если метод запроса указан как HEAD
						case static_cast <uint8_t> (web_t::method_t::HEAD): method = "head"; break;
						// Если метод запроса указан как DELETE
						case static_cast <uint8_t> (web_t::method_t::DEL): method = "delete"; break;
						// Если метод запроса указан как PATCH
						case static_cast <uint8_t> (web_t::method_t::PATCH): method = "patch"; break;
						// Если метод запроса указан как TRACE
						case static_cast <uint8_t> (web_t::method_t::TRACE): method = "trace"; break;
						// Если метод запроса указан как OPTIONS
						case static_cast <uint8_t> (web_t::method_t::OPTIONS): method = "options"; break;
						// Если метод запроса указан как CONNECT
						case static_cast <uint8_t> (web_t::method_t::CONNECT): method = "connect"; break;
					}
					// Выполняем определения протокола модуля
					switch(static_cast <uint8_t> (this->_identity)){
						// Если протокол принадлежит прокси-серверу
						case static_cast <uint8_t> (identity_t::PROXY): {
							// Если заголовок авторизации на прокси-сервере не запрещён
							if(!this->isBlack("Proxy-Authorization")){
								// Получаем параметры авторизации
								const string & auth = this->_auth.client.auth(method);
								// Если данные авторизации получены
								if(!auth.empty())
									// Выполняем установку параметров авторизации
									request.append(this->_fmk->format("Proxy-Authorization: %s\r\n", auth.c_str()));
							}
						} break;
						// Если протокол принадлежит к остальным сервисам
						default: {
							// Если заголовок авторизации на прокси-сервере не запрещён
							if(!this->isBlack("Authorization")){
								// Получаем параметры авторизации
								const string & auth = this->_auth.client.auth(method);
								// Если данные авторизации получены
								if(!auth.empty())
									// Выполняем установку параметров авторизации
									request.append(this->_fmk->format("Authorization: %s\r\n", auth.c_str()));
							}
						}
					}
				}
				// Получаем данные тела
				const auto & body = this->_web.body();
				// Если запрос не является GET, HEAD или TRACE, а тело запроса существует
				if((req.method != web_t::method_t::GET) && (req.method != web_t::method_t::HEAD) && (req.method != web_t::method_t::TRACE) && !body.empty()){
					// Выполняем кодирование данных
					const auto & crypto = this->encode(body);
					// Если заголовок не запрещён
					if(!this->isBlack("Date"))
						// Добавляем заголовок даты в запрос
						request.append(this->_fmk->format("Date: %s\r\n", this->date().c_str()));
					// Проверяем нужно ли передать тело разбив на чанки
					this->_chunking = (!available[6] || ((length > 0) && (length != body.size())));
					// Если данные были сжаты, либо зашифрованы
					if(crypto.encrypt || crypto.compress){
						// Выполняем очистку тела сообщения
						this->clearBody();
						// Заменяем тело данных
						this->_web.body(crypto.data);
						// Заменяем размер тела данных
						if(!this->_chunking)
							// Устанавливаем размер тела сообщения
							length = body.size();
					}
					// Если данные зашифрованы, устанавливаем соответствующие заголовки
					if(crypto.encrypt)
						// Устанавливаем X-AWH-Encryption
						request.append(this->_fmk->format("X-AWH-Encryption: %u\r\n", static_cast <u_short> (this->_hash.cipher())));
					// Если данные сжаты, устанавливаем соответствующие заголовки
					if(crypto.compress){
						// Определяем метод компрессии тела сообщения
						switch(static_cast <uint8_t> (this->_compress)){
							// Если нужно сжать тело методом BROTLI
							case static_cast <uint8_t> (compress_t::BROTLI):
								// Устанавливаем Content-Encoding если не передан
								request.append(this->_fmk->format("Content-Encoding: %s\r\n", "br"));
							break;
							// Если нужно сжать тело методом GZIP
							case static_cast <uint8_t> (compress_t::GZIP):
								// Устанавливаем Content-Encoding если не передан
								request.append(this->_fmk->format("Content-Encoding: %s\r\n", "gzip"));
							break;
							// Если нужно сжать тело методом DEFLATE
							case static_cast <uint8_t> (compress_t::DEFLATE):
								// Устанавливаем Content-Encoding если не передан
								request.append(this->_fmk->format("Content-Encoding: %s\r\n", "deflate"));
							break;
						}
					}
					// Если данные необходимо разбивать на чанки
					if(this->_chunking && !this->isBlack("Transfer-Encoding"))
						// Устанавливаем заголовок Transfer-Encoding
						request.append(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
					// Если заголовок размера передаваемого тела, не запрещён
					else if(!this->isBlack("Content-Length"))
						// Устанавливаем размер передаваемого тела Content-Length
						request.append(this->_fmk->format("Content-Length: %zu\r\n", length));
				// Очищаем тела сообщения
				} else this->_web.clearBody();
				// Устанавливаем завершающий разделитель
				request.append("\r\n");
				// Формируем результат запроса
				result.assign(request.begin(), request.end());
			}
		} break;
		// Если нужно сформировать данные ответа
		case static_cast <uint8_t> (process_t::RESPONSE): {
			// Получаем объект ответа клиенту
			const web_t::res_t & res = static_cast <const web_t::res_t &> (provider);
			// Устанавливаем парарметры ответа
			this->_web.response(res);
			// Если текст сообщения не установлен
			if(res.message.empty())
				// Выполняем установку сообщения
				const_cast <web_t::res_t &> (res).message = this->message(res.code);
			// Если сообщение получено
			if(!res.message.empty()){
				/**
				 * Типы основных заголовков
				 */
				bool available[9] = {
					false, // Connection
					false, // Proxy-Connection
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
				string response = this->_fmk->format("HTTP/%.1f %u %s\r\n", res.version, res.code, res.message.c_str());
				// Если заголовок не запрещён
				if(!this->isBlack("Date"))
					// Добавляем заголовок даты в ответ
					response.append(this->_fmk->format("Date: %s\r\n", this->date().c_str()));
				// Переходим по всему списку заголовков
				for(auto & header : this->_web.headers()){
					// Флаг разрешающий вывода заголовка
					bool allow = !this->isBlack(header.first);
					// Выполняем перебор всех обязательных заголовков
					for(uint8_t i = 0; i < 8; i++){
						// Если заголовок уже найден пропускаем его
						if(available[i]) continue;
						// Выполняем првоерку заголовка
						switch(i){
							case 0: available[i] = this->_fmk->compare(header.first, "connection");         break;
							case 1: available[i] = this->_fmk->compare(header.first, "proxy-connection");   break;
							case 2: available[i] = this->_fmk->compare(header.first, "content-type");       break;
							case 4: available[i] = this->_fmk->compare(header.first, "content-encoding");   break;
							case 5: available[i] = this->_fmk->compare(header.first, "transfer-encoding");  break;
							case 6: available[i] = this->_fmk->compare(header.first, "x-awh-encryption");   break;
							case 7: available[i] = this->_fmk->compare(header.first, "www-authenticate");   break;
							case 8: available[i] = this->_fmk->compare(header.first, "proxy-authenticate"); break;
							case 3: {
								// Запоминаем, что мы нашли заголовок размера тела
								available[i] = this->_fmk->compare(header.first, "content-length");
								// Устанавливаем размер тела сообщения
								if(available[i]) length = static_cast <size_t> (::stoull(header.second));
							} break;
						}
						// Если заголовок разрешён для вывода
						if(allow){
							// Выполняем првоерку заголовка
							switch(i){
								case 3:
								case 4:
								case 5:
								case 6: allow = !available[i]; break;
							}
						}
					}
					// Если заголовок не является запрещённым, добавляем заголовок в ответ
					if(allow)
						// Формируем строку ответа
						response.append(
							this->_fmk->format(
								"%s: %s\r\n",
								this->_fmk->transform(header.first, fmk_t::transform_t::SMART).c_str(),
								header.second.c_str()
							)
						);
				}
				// Устанавливаем Connection если не передан
				if(!available[0] && !this->isBlack("Connection"))
					// Добавляем заголовок в ответ
					response.append(this->_fmk->format("Connection: %s\r\n", HTTP_HEADER_CONNECTION));
				// Устанавливаем Content-Type если не передан
				if(!available[2] && !this->isBlack("Content-Type"))
					// Добавляем заголовок в ответ
					response.append(this->_fmk->format("Content-Type: %s\r\n", HTTP_HEADER_CONTENTTYPE));
				// Если заголовок не запрещён
				if(!this->isBlack("Server"))
					// Добавляем название сервера в ответ
					response.append(this->_fmk->format("Server: %s\r\n", this->_ident.name.c_str()));
				// Если заголовок не запрещён
				if(!this->isBlack("X-Powered-By"))
					// Добавляем название рабочей системы в ответ
					response.append(this->_fmk->format("X-Powered-By: %s/%s\r\n", this->_ident.id.c_str(), this->_ident.version.c_str()));
				// Если заголовок авторизации не передан
				if(((res.code == 401) && !available[7]) || ((res.code == 407) && !available[8])){
					// Получаем параметры авторизации
					const string & auth = this->_auth.server;
					// Если параметры авторизации получены
					if(!auth.empty()){
						// Определяем код авторизации
						switch(res.code){
							// Если авторизация производится для Web-Сервера
							case 401: {
								// Если заголовок не запрещён
								if(!this->isBlack("WWW-Authenticate"))
									// Добавляем параметры авторизации
									response.append(this->_fmk->format("WWW-Authenticate: %s\r\n", auth.c_str()));
							} break;
							// Если авторизация производится для Прокси-Сервера
							case 407: {
								// Если заголовок не запрещён
								if(!this->isBlack("Proxy-Authenticate"))
									// Добавляем параметры авторизации
									response.append(this->_fmk->format("Proxy-Authenticate: %s\r\n", auth.c_str()));
							} break;
						}
					}
				}
				// Получаем данные тела
				const auto & body = this->_web.body();
				// Если запрос должен содержать тело и тело ответа существует
				if((res.code >= 200) && (res.code != 204) && (res.code != 304) && (res.code != 308) && !body.empty()){
					// Выполняем кодирование данных
					const auto & crypto = this->encode(body);
					// Проверяем нужно ли передать тело разбив на чанки
					this->_chunking = (!available[3] || ((length > 0) && (length != body.size())));
					// Если данные были сжаты, либо зашифрованы
					if(crypto.encrypt || crypto.compress){
						// Выполняем очистку тела сообщения
						this->clearBody();
						// Заменяем тело данных
						this->_web.body(crypto.data);
						// Заменяем размер тела данных
						if(!this->_chunking)
							// Устанавливаем размер тела сообщения
							length = body.size();
					}
					// Если данные зашифрованы, устанавливаем соответствующие заголовки
					if(crypto.encrypt)
						// Устанавливаем X-AWH-Encryption
						response.append(this->_fmk->format("X-AWH-Encryption: %u\r\n", static_cast <u_short> (this->_hash.cipher())));
					// Если данные сжаты, устанавливаем соответствующие заголовки
					if(crypto.compress){
						// Определяем метод компрессии тела сообщения
						switch(static_cast <uint8_t> (this->_compress)){
							// Если нужно сжать тело методом BROTLI
							case static_cast <uint8_t> (compress_t::BROTLI):
								// Устанавливаем Content-Encoding если не передан
								response.append(this->_fmk->format("Content-Encoding: %s\r\n", "br"));
							break;
							// Если нужно сжать тело методом GZIP
							case static_cast <uint8_t> (compress_t::GZIP):
								// Устанавливаем Content-Encoding если не передан
								response.append(this->_fmk->format("Content-Encoding: %s\r\n", "gzip"));
							break;
							// Если нужно сжать тело методом DEFLATE
							case static_cast <uint8_t> (compress_t::DEFLATE):
								// Устанавливаем Content-Encoding если не передан
								response.append(this->_fmk->format("Content-Encoding: %s\r\n", "deflate"));
							break;
						}
					}
					// Если данные необходимо разбивать на чанки
					if(this->_chunking && !this->isBlack("Transfer-Encoding"))
						// Устанавливаем заголовок Transfer-Encoding
						response.append(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
					// Если заголовок размера передаваемого тела, не запрещён
					else if(!this->isBlack("Content-Length"))
						// Устанавливаем размер передаваемого тела Content-Length
						response.append(this->_fmk->format("Content-Length: %zu\r\n", length));
				// Очищаем тела сообщения
				} else this->_web.clearBody();
				// Устанавливаем завершающий разделитель
				response.append("\r\n");
				// Формируем результат ответа
				result.assign(response.begin(), response.end());
			}
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * process2 Метод создания выполняемого процесса в бинарном виде (для протокола HTTP/2)
 * @param flag     флаг выполняемого процесса
 * @param provider параметры провайдера обмена сообщениями
 * @return         буфер данных в бинарном виде
 */
vector <pair <string, string>> awh::Http::process2(const process_t flag, const web_t::provider_t & provider) const noexcept {
	// Результат работы функции
	vector <pair<string, string>> result;
	// Определяем флаг выполняемого процесса
	switch(static_cast <uint8_t> (flag)){
		// Если нужно сформировать данные запроса
		case static_cast <uint8_t> (process_t::REQUEST): {
			// Получаем объект ответа клиенту
			const web_t::req_t & req = static_cast <const web_t::req_t &> (provider);
			// Если параметры запроса получены
			if(!req.url.empty() && (req.method != web_t::method_t::NONE)){
				// Размер тела сообщения
				size_t length = 0;
				/**
				 * Типы основных заголовков
				 */
				bool available[13] = {
					false, // Host
					false, // Accept
					false, // Origin
					false, // User-Agent
					false, // Connection
					false, // Proxy-Connection
					false, // Content-Length
					false, // Accept-Language
					false, // Accept-Encoding
					false, // Content-Encoding
					false, // Transfer-Encoding
					false, // X-AWH-Encryption
					false  // Proxy-Authorization
				};
				// Устанавливаем парарметры запроса
				this->_web.request(req);
				// Устанавливаем параметры REST-запроса
				this->_auth.client.uri(this->_uri.url(req.url));
				// Определяем метод запроса
				switch(static_cast <uint8_t> (req.method)){
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
				// Выполняем установку схемы протокола
				result.push_back(make_pair(":scheme", "https"));
				// Если метод подключения установлен как CONNECT
				if(req.method == web_t::method_t::CONNECT)
					// Формируем URI запроса
					result.push_back(make_pair(":authority", this->_fmk->format("%s:%u", req.url.host.c_str(), req.url.port)));
				// Если метод подключения не является методом CONNECT, выполняем установку хоста сервера
				else result.push_back(make_pair(":authority", req.url.host));
				// Выполняем установку пути запроса
				result.push_back(make_pair(":path", this->_uri.query(req.url)));
				// Переходим по всему списку заголовков
				for(auto & header : this->_web.headers()){
					// Если заголовок является системным
					if(header.first.front() == ':')
						// Формируем строку запроса
						result.push_back(make_pair(this->_fmk->transform(header.first, fmk_t::transform_t::LOWER), header.second));
				}
				// Переходим по всему списку заголовков
				for(auto & header : this->_web.headers()){
					// Если заголовок не является системным
					if(header.first.front() != ':'){
						// Флаг разрешающий вывода заголовка
						bool allow = !this->isBlack(header.first);
						// Выполняем перебор всех обязательных заголовков
						for(uint8_t i = 0; i < 12; i++){
							// Если заголовок уже найден пропускаем его
							if(available[i]) continue;
							// Выполняем првоерку заголовка
							switch(i){
								case 0:  available[i] = this->_fmk->compare(header.first, "host");                  break;
								case 1:  available[i] = this->_fmk->compare(header.first, "accept");                break;
								case 2:  available[i] = this->_fmk->compare(header.first, "origin");                break;
								case 3:  available[i] = this->_fmk->compare(header.first, "user-agent");            break;
								case 4:  available[i] = this->_fmk->compare(header.first, "connection");            break;
								case 5:  available[i] = this->_fmk->compare(header.first, "proxy-connection");      break;
								case 7:  available[i] = this->_fmk->compare(header.first, "accept-language");       break;
								case 8:  available[i] = this->_fmk->compare(header.first, "accept-encoding");       break;
								case 9:  available[i] = this->_fmk->compare(header.first, "content-encoding");      break;
								case 10: available[i] = this->_fmk->compare(header.first, "transfer-encoding");     break;
								case 11: available[i] = this->_fmk->compare(header.first, "x-awh-encryption");      break;
								case 12: available[i] = (this->_fmk->compare(header.first, "authorization") ||
														 this->_fmk->compare(header.first, "proxy-authorization")); break;
								case 6: {
									// Запоминаем, что мы нашли заголовок размера тела
									available[i] = this->_fmk->compare(header.first, "content-length");
									// Устанавливаем размер тела сообщения
									if(available[i]) length = static_cast <size_t> (::stoull(header.second));
								} break;
							}
							// Если заголовок разрешён для вывода
							if(allow){
								// Выполняем првоерку заголовка
								switch(i){
									case 0:
									case 6:
									case 9:
									case 10:
									case 11: allow = !available[i]; break;
									case 4:
									case 5: {
										// Если заголовок Connection определён
										if(available[i])
											// Выполняем определение разрешено ли выводить заголовок
											allow = !this->_fmk->compare(header.second, "keep-alive");
									} break;
								}
							}
						}
						// Если заголовок не является запрещённым, добавляем заголовок в запрос
						if(allow)
							// Формируем строку запроса
							result.push_back(make_pair(this->_fmk->transform(header.first, fmk_t::transform_t::LOWER), header.second));
					}
				}
				// Устанавливаем Accept если не передан
				if(!available[1] && (req.method != web_t::method_t::CONNECT) && !this->isBlack("accept"))
					// Добавляем заголовок в запрос
					result.push_back(make_pair("accept", HTTP_HEADER_ACCEPT));
				// Устанавливаем Accept-Language если не передан
				if(!available[7] && (req.method != web_t::method_t::CONNECT) && !this->isBlack("accept-language"))
					// Добавляем заголовок в запрос
					result.push_back(make_pair("accept-language", HTTP_HEADER_ACCEPTLANGUAGE));
				// Если нужно запросить компрессию в удобном нам виде
				if(!available[8] && (this->_compress != compress_t::NONE) && (req.method != web_t::method_t::CONNECT) && !this->isBlack("accept-encoding")){
					// Определяем метод сжатия который поддерживает клиент
					switch(static_cast <uint8_t> (this->_compress)){
						// Если клиент поддерживает методот сжатия BROTLI
						case static_cast <uint8_t> (compress_t::BROTLI):
							// Добавляем заголовок в запрос
							result.push_back(make_pair("accept-encoding", "br"));
						break;
						// Если клиент поддерживает методот сжатия GZIP
						case static_cast <uint8_t> (compress_t::GZIP):
							// Добавляем заголовок в запрос
							result.push_back(make_pair("accept-encoding", "gzip"));
						break;
						// Если клиент поддерживает методот сжатия DEFLATE
						case static_cast <uint8_t> (compress_t::DEFLATE):
							// Добавляем заголовок в запрос
							result.push_back(make_pair("accept-encoding", "deflate"));
						break;
						// Если клиент поддерживает методот сжатия GZIP, BROTLI
						case static_cast <uint8_t> (compress_t::GZIP_BROTLI):
							// Добавляем заголовок в запрос
							result.push_back(make_pair("accept-encoding", "gzip, br"));
						break;
						// Если клиент поддерживает методот сжатия GZIP, DEFLATE
						case static_cast <uint8_t> (compress_t::GZIP_DEFLATE):
							// Добавляем заголовок в запрос
							result.push_back(make_pair("accept-encoding", "gzip, deflate"));
						break;
						// Если клиент поддерживает методот сжатия DEFLATE, BROTLI
						case static_cast <uint8_t> (compress_t::DEFLATE_BROTLI):
							// Добавляем заголовок в запрос
							result.push_back(make_pair("accept-encoding", "deflate, br"));
						break;
						// Если клиент поддерживает все методы сжатия
						case static_cast <uint8_t> (compress_t::ALL_COMPRESS):
							// Добавляем заголовок в запрос
							result.push_back(make_pair("accept-encoding", "gzip, deflate, br"));
						break;
					}
				}
				// Устанавливаем User-Agent если не передан
				if(!available[3] && !this->isBlack("user-agent")){
					// Если User-Agent установлен стандартный
					if(this->_fmk->compare(this->_userAgent, HTTP_HEADER_AGENT)){
						// Название операционной системы
						const char * os = nullptr;
						// Определяем название операционной системы
						switch(static_cast <uint8_t> (this->_fmk->os())){
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
						this->_userAgent = this->_fmk->format("%s (%s; %s/%s)", this->_ident.name.c_str(), os, this->_ident.id.c_str(), this->_ident.version.c_str());
					}
					// Добавляем заголовок в запрос
					result.push_back(make_pair("user-agent", this->_userAgent));
				}
				// Если заголовок авторизации не передан
				if(!available[12]){
					// Метод HTTP запроса
					string method = "";
					// Определяем метод запроса
					switch(static_cast <uint8_t> (req.method)){
						// Если метод запроса указан как GET
						case static_cast <uint8_t> (web_t::method_t::GET): method = "get"; break;
						// Если метод запроса указан как PUT
						case static_cast <uint8_t> (web_t::method_t::PUT): method = "put"; break;
						// Если метод запроса указан как POST
						case static_cast <uint8_t> (web_t::method_t::POST): method = "post"; break;
						// Если метод запроса указан как HEAD
						case static_cast <uint8_t> (web_t::method_t::HEAD): method = "head"; break;
						// Если метод запроса указан как DELETE
						case static_cast <uint8_t> (web_t::method_t::DEL): method = "delete"; break;
						// Если метод запроса указан как PATCH
						case static_cast <uint8_t> (web_t::method_t::PATCH): method = "patch"; break;
						// Если метод запроса указан как TRACE
						case static_cast <uint8_t> (web_t::method_t::TRACE): method = "trace"; break;
						// Если метод запроса указан как OPTIONS
						case static_cast <uint8_t> (web_t::method_t::OPTIONS): method = "options"; break;
						// Если метод запроса указан как CONNECT
						case static_cast <uint8_t> (web_t::method_t::CONNECT): method = "connect"; break;
					}
					// Выполняем определения протокола модуля
					switch(static_cast <uint8_t> (this->_identity)){
						// Если протокол принадлежит прокси-серверу
						case static_cast <uint8_t> (identity_t::PROXY): {
							// Если заголовок авторизации на прокси-сервере не запрещён
							if(!this->isBlack("proxy-authorization")){
								// Получаем параметры авторизации
								const string & auth = this->_auth.client.auth(method);
								// Если данные авторизации получены
								if(!auth.empty())
									// Выполняем установку заголовка
									result.push_back(make_pair("proxy-authorization", auth));
							}
						} break;
						// Если протокол принадлежит к остальным сервисам
						default: {
							// Если заголовок авторизации на прокси-сервере не запрещён
							if(!this->isBlack("authorization")){
								// Получаем параметры авторизации
								const string & auth = this->_auth.client.auth(method);
								// Если данные авторизации получены
								if(!auth.empty())
									// Выполняем установку заголовка
									result.push_back(make_pair("authorization", auth));
							}
						}
					}
				}
				// Получаем данные тела
				const auto & body = this->_web.body();
				// Если запрос не является GET, HEAD или TRACE, а тело запроса существует
				if((req.method != web_t::method_t::GET) && (req.method != web_t::method_t::HEAD) && (req.method != web_t::method_t::TRACE) && !body.empty()){
					// Выполняем кодирование данных
					const auto & crypto = this->encode(body);
					// Если заголовок не запрещён
					if(!this->isBlack("date"))
						// Добавляем заголовок даты в запрос
						result.push_back(make_pair("date", this->date()));
					// Проверяем нужно ли передать тело разбив на чанки
					this->_chunking = (!available[6] || ((length > 0) && (length != body.size())));
					// Если данные были сжаты, либо зашифрованы
					if(crypto.encrypt || crypto.compress){
						// Выполняем очистку тела сообщения
						this->clearBody();
						// Заменяем тело данных
						this->_web.body(crypto.data);
						// Заменяем размер тела данных
						if(!this->_chunking)
							// Устанавливаем размер тела сообщения
							length = body.size();
					}
					// Если данные зашифрованы, устанавливаем соответствующие заголовки
					if(crypto.encrypt)
						// Устанавливаем X-AWH-Encryption
						result.push_back(make_pair("x-awh-encryption", ::to_string(static_cast <u_short> (this->_hash.cipher()))));
					// Если данные сжаты, устанавливаем соответствующие заголовки
					if(crypto.compress){
						// Определяем метод компрессии тела сообщения
						switch(static_cast <uint8_t> (this->_compress)){
							// Если нужно сжать тело методом BROTLI
							case static_cast <uint8_t> (compress_t::BROTLI):
								// Устанавливаем Content-Encoding если не передан
								result.push_back(make_pair("content-encoding", "br"));
							break;
							// Если нужно сжать тело методом GZIP
							case static_cast <uint8_t> (compress_t::GZIP):
								// Устанавливаем Content-Encoding если не передан
								result.push_back(make_pair("content-encoding", "gzip"));
							break;
							// Если нужно сжать тело методом DEFLATE
							case static_cast <uint8_t> (compress_t::DEFLATE):
								// Устанавливаем Content-Encoding если не передан
								result.push_back(make_pair("content-encoding", "deflate"));
							break;
						}
					}
					// Если данные необходимо разбивать на чанки
					if(this->_chunking && !this->isBlack("transfer-encoding"))
						// Устанавливаем заголовок Transfer-Encoding
						result.push_back(make_pair("transfer-encoding", "chunked"));
					// Если заголовок размера передаваемого тела, не запрещён
					else if(!this->isBlack("content-length"))
						// Устанавливаем размер передаваемого тела Content-Length
						result.push_back(make_pair("content-length", ::to_string(length)));
				// Очищаем тела сообщения
				} else this->_web.clearBody();
			}
		} break;
		// Если нужно сформировать данные ответа
		case static_cast <uint8_t> (process_t::RESPONSE): {
			// Размер тела сообщения
			size_t length = 0;
			/**
			 * Типы основных заголовков
			 */
			bool available[9] = {
				false, // Connection
				false, // Proxy-Connection
				false, // Content-Type
				false, // Content-Length
				false, // Content-Encoding
				false, // Transfer-Encoding
				false, // X-AWH-Encryption
				false, // WWW-Authenticate
				false  // Proxy-Authenticate
			};
			// Получаем объект ответа клиенту
			const web_t::res_t & res = static_cast <const web_t::res_t &> (provider);
			// Устанавливаем параметры ответа
			this->_web.response(res);
			// Если текст сообщения не установлен
			if(res.message.empty())
				// Выполняем установку сообщения
				const_cast <web_t::res_t &> (res).message = this->message(res.code);
			// Если сообщение получено
			if(!res.message.empty()){
				// Данные REST ответа
				result.push_back(make_pair(":status", ::to_string(res.code)));
				// Если заголовок не запрещён
				if(!this->isBlack("date"))
					// Добавляем заголовок даты в ответ
					result.push_back(make_pair("date", this->date()));
				// Переходим по всему списку заголовков
				for(auto & header : this->_web.headers()){
					// Флаг разрешающий вывода заголовка
					bool allow = !this->isBlack(header.first);
					// Выполняем перебор всех обязательных заголовков
					for(uint8_t i = 0; i < 8; i++){
						// Если заголовок уже найден пропускаем его
						if(available[i]) continue;
						// Выполняем првоерку заголовка
						switch(i){
							case 0: available[i] = this->_fmk->compare(header.first, "connection");         break;
							case 1: available[i] = this->_fmk->compare(header.first, "proxy-connection");   break;
							case 2: available[i] = this->_fmk->compare(header.first, "content-type");       break;
							case 4: available[i] = this->_fmk->compare(header.first, "content-encoding");   break;
							case 5: available[i] = this->_fmk->compare(header.first, "transfer-encoding");  break;
							case 6: available[i] = this->_fmk->compare(header.first, "x-awh-encryption");   break;
							case 7: available[i] = this->_fmk->compare(header.first, "www-authenticate");   break;
							case 8: available[i] = this->_fmk->compare(header.first, "proxy-authenticate"); break;
							case 3: {
								// Запоминаем, что мы нашли заголовок размера тела
								available[i] = this->_fmk->compare(header.first, "content-length");
								// Устанавливаем размер тела сообщения
								if(available[i]) length = static_cast <size_t> (::stoull(header.second));
							} break;
						}
						// Если заголовок разрешён для вывода
						if(allow){
							// Выполняем првоерку заголовка
							switch(i){
								case 3:
								case 4:
								case 5:
								case 6: allow = !available[i]; break;
								case 0:
								case 1: {
									// Если заголовок Connection определён
									if(available[i])
										// Выполняем определение разрешено ли выводить заголовок
										allow = !this->_fmk->compare(header.second, "keep-alive");
								} break;
							}
						}
					}
					// Если заголовок не является запрещённым, добавляем заголовок в ответ
					if(allow)
						// Формируем строку ответа
						result.push_back(make_pair(this->_fmk->transform(header.first, fmk_t::transform_t::LOWER), header.second));
				}
				// Устанавливаем Content-Type если не передан
				if(!available[2] && !this->isBlack("content-type"))
					// Добавляем заголовок в ответ
					result.push_back(make_pair("content-type", HTTP_HEADER_CONTENTTYPE));
				// Если заголовок не запрещён
				if(!this->isBlack("server"))
					// Добавляем название сервера в ответ
					result.push_back(make_pair("server", this->_ident.name));
				// Если заголовок не запрещён
				if(!this->isBlack("x-powered-by"))
					// Добавляем название рабочей системы в ответ
					result.push_back(make_pair("x-powered-by", this->_fmk->format("%s/%s", this->_ident.id.c_str(), this->_ident.version.c_str())));
				// Если заголовок авторизации не передан
				if(((res.code == 401) && !available[7]) || ((res.code == 407) && !available[8])){
					// Получаем параметры авторизации
					const string & auth = this->_auth.server;
					// Если параметры авторизации получены
					if(!auth.empty()){
						// Определяем код авторизации
						switch(res.code){
							// Если авторизация производится для Web-Сервера
							case 401: {
								// Если заголовок не запрещён
								if(!this->isBlack("www-authenticate"))
									// Добавляем параметры авторизации
									result.push_back(make_pair("www-authenticate", auth));
							} break;
							// Если авторизация производится для Прокси-Сервера
							case 407: {
								// Если заголовок не запрещён
								if(!this->isBlack("proxy-authenticate"))
									// Добавляем параметры авторизации
									result.push_back(make_pair("proxy-authenticate", auth));
							} break;
						}
					}
				}
				// Получаем данные тела
				const auto & body = this->_web.body();
				// Если запрос должен содержать тело и тело ответа существует
				if((res.code >= 200) && (res.code != 204) && (res.code != 304) && (res.code != 308) && !body.empty()){
					// Выполняем кодирование данных
					const auto & crypto = this->encode(body);
					// Проверяем нужно ли передать тело разбив на чанки
					this->_chunking = (!available[3] || ((length > 0) && (length != body.size())));
					// Если данные были сжаты, либо зашифрованы
					if(crypto.encrypt || crypto.compress){
						// Выполняем очистку тела сообщения
						this->clearBody();
						// Заменяем тело данных
						this->_web.body(crypto.data);
						// Заменяем размер тела данных
						if(!this->_chunking)
							// Устанавливаем размера передаваемых данных
							length = body.size();
					}
					// Если данные зашифрованы, устанавливаем соответствующие заголовки
					if(crypto.encrypt)
						// Устанавливаем X-AWH-Encryption
						result.push_back(make_pair("x-awh-encryption", ::to_string(static_cast <u_short> (this->_hash.cipher()))));
					// Если данные сжаты, устанавливаем соответствующие заголовки
					if(crypto.compress){
						// Определяем метод компрессии тела сообщения
						switch(static_cast <uint8_t> (this->_compress)){
							// Если нужно сжать тело методом BROTLI
							case static_cast <uint8_t> (compress_t::BROTLI):
								// Устанавливаем Content-Encoding если не передан
								result.push_back(make_pair("content-encoding", "br"));
							break;
							// Если нужно сжать тело методом GZIP
							case static_cast <uint8_t> (compress_t::GZIP):
								// Устанавливаем Content-Encoding если не передан
								result.push_back(make_pair("content-encoding", "gzip"));
							break;
							// Если нужно сжать тело методом DEFLATE
							case static_cast <uint8_t> (compress_t::DEFLATE):
								// Устанавливаем Content-Encoding если не передан
								result.push_back(make_pair("content-encoding", "deflate"));
							break;
						}
					}
					// Если данные необходимо разбивать на чанки
					if(this->_chunking && !this->isBlack("transfer-encoding"))
						// Устанавливаем заголовок Transfer-Encoding
						result.push_back(make_pair("transfer-encoding", "chunked"));
					// Если заголовок размера передаваемого тела, не запрещён
					else if(!this->isBlack("content-length"))
						// Устанавливаем размер передаваемого тела Content-Length
						result.push_back(make_pair("content-length", ::to_string(length)));
				// Очищаем тела сообщения
				} else this->_web.clearBody();
			}
		} break;
	}
	// Выводим результат
	return result;
}
/** 
 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
 * @param callback функция обратного вызова
 */
void awh::Http::on(function <void (const uint64_t, const u_int, const string &)> callback) noexcept {
	// Устанавливаем функции обратного вызова
	this->_web.on(callback);
}
/** 
 * on Метод установки функции вывода запроса клиента на выполненный запрос к серверу
 * @param callback функция обратного вызова
 */
void awh::Http::on(function <void (const uint64_t, const web_t::method_t, const uri_t::url_t &)> callback) noexcept {
	// Устанавливаем функции обратного вызова
	this->_web.on(callback);
}
/** 
 * on Метод установки функции вывода полученного заголовка с сервера
 * @param callback функция обратного вызова
 */
void awh::Http::on(function <void (const uint64_t, const string &, const string &)> callback) noexcept {
	// Устанавливаем функции обратного вызова
	this->_web.on(callback);
}
/**
 * on Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
void awh::Http::on(function <void (const uint64_t, const vector <char> &, const http_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения чанков
	this->_web.on(std::bind(&awh::Http::chunkingCallback, this, _1, _2, _3));
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const vector <char> &, const http_t *)> ("chunking", callback);
}
/** 
 * on Метод установки функции вывода полученного тела данных с сервера
 * @param callback функция обратного вызова
 */
void awh::Http::on(function <void (const uint64_t, const u_int, const string &, const vector <char> &)> callback) noexcept {
	// Устанавливаем функции обратного вызова
	this->_web.on(callback);
}
/** 
 * on Метод установки функции вывода полученного тела данных с сервера
 * @param callback функция обратного вызова
 */
void awh::Http::on(function <void (const uint64_t, const web_t::method_t, const uri_t::url_t &, const vector <char> &)> callback) noexcept {
	// Устанавливаем функции обратного вызова
	this->_web.on(callback);
}
/** 
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::Http::on(function <void (const uint64_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Устанавливаем функции обратного вызова
	this->_web.on(callback);
}
/** 
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::Http::on(function <void (const uint64_t, const web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Устанавливаем функции обратного вызова
	this->_web.on(callback);
}
/**
 * id Метод получения идентификатора объекта
 * @return идентификатор объекта
 */
uint64_t awh::Http::id() const noexcept {
	// Выводим идентификатор объекта
	return this->_web.id();
}
/**
 * id Метод установки идентификатора объекта
 * @param id идентификатор объекта
 */
void awh::Http::id(const uint64_t id) noexcept {
	// Выполняем установку идентификатора объекта
	this->_web.id(id);
}
/**
 * identity Метод извлечения идентичности протокола модуля
 * @return флаг идентичности протокола модуля
 */
awh::Http::identity_t awh::Http::identity() const noexcept {
	// Выводим флаг идентичности протокола
	return this->_identity;
}
/**
 * identity Метод установки идентичности протокола модуля
 * @param identity идентичность протокола модуля
 */
void awh::Http::identity(const identity_t identity) noexcept {
	// Выполняем установку флага идентичности протокола модуля
	this->_identity = identity;
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::Http::chunk(const size_t size) noexcept {
	// Устанавливаем размер чанка
	if(size >= 100)
		// Выполняем установку размера чанка
		this->_chunk = size;
}
/**
 * userAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::Http::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty())
		// Выполняем установку User-Agent
		this->_userAgent = userAgent;
}
/**
 * ident Метод установки идентификации сервера
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::Http::ident(const string & id, const string & name, const string & ver) noexcept {
	// Если идентификатор сервиса передан
	if(!id.empty())
		// Устанавливаем идентификатор сервиса
		this->_ident.id = id;
	// Если название сервиса передано
	if(!name.empty())
		// Устанавливаем название сервиса
		this->_ident.name = name;
	// Если версия сервиса передана
	if(!ver.empty())
		// Устанавливаем версию сервиса
		this->_ident.version = ver;
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::Http::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем флаг шифрования
	this->_crypt = !pass.empty();
	{
		// Устанавливаем соль шифрования
		this->_hash.salt(salt);
		// Устанавливаем пароль шифрования
		this->_hash.pass(pass);
		// Устанавливаем размер шифрования
		this->_hash.cipher(cipher);
	}{
		// Устанавливаем соль шифрования
		this->_dhash.salt(salt);
		// Устанавливаем пароль шифрования
		this->_dhash.pass(pass);
		// Устанавливаем размер шифрования
		this->_dhash.cipher(cipher);
	}
}
/**
 * Http Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Http::Http(const fmk_t * fmk, const log_t * log) noexcept :
 _uri(fmk), _stath(stath_t::NONE), _state(state_t::NONE), _identity(identity_t::NONE),
 _callback(log), _web(fmk, log), _auth(fmk, log), _hash(log), _dhash(log),
 _crypt(false), _chunking(false), _chunk(BUFFER_CHUNK), _compress(compress_t::NONE),
 _userAgent(HTTP_HEADER_AGENT), _fmk(fmk), _log(log) {
	// Выполняем установку идентификатора объекта
	this->_web.id(this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS));
	// Устанавливаем функцию обратного вызова для получения чанков
	this->_web.on(std::bind(&awh::Http::chunkingCallback, this, _1, _2, _3));
}
