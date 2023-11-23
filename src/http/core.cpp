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
 * @param web    объект HTTP-парсера
 */
void awh::Http::chunkingCallback(const uint64_t id, const vector <char> & buffer, const web_t * web) noexcept {
	// Если функция обратного вызова на вывод полученного чанка установлена
	if(this->_callback.is("chunking"))
		// Выполняем функцию обратного вызова
		this->_callback.call <const uint64_t, const vector <char> &, const http_t *> ("chunking", id, buffer, this);
}
/**
 * encrypt Метод выполнения шифрования полезной нагрузки
 */
void awh::Http::encrypt() noexcept {
	// Если полезная нагрузка не зашифрована
	if(!this->_crypted && this->_encryption){
		// Получаем данные тела
		const auto & body = this->_web.body();
		// Если тело сообщения получено
		if(!body.empty()){
			// Выполняем шифрование полезной нагрузки
			const auto & result = this->_hash.encrypt(body.data(), body.size());
			// Если шифрование выполнено
			if((this->_crypted = !result.empty())){
				// Выполняем очистку данных тела
				this->_web.clearBody();
				// Формируем новое тело сообщения
				this->_web.body(result);
			// Если шифрование не выполнено
			} else {
				// Выводим сообщение об ошибке
				this->_log->print("Encryption module has failed", log_t::flag_t::WARNING);
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Encryption module has failed");
			}
		}
	}
}
/**
 * decrypt Метод выполнения дешифровани полезной нагрузки
 */
void awh::Http::decrypt() noexcept {
	// Если полезная нагрузка зашифрованна
	if(this->_crypted){
		// Получаем данные тела
		const auto & body = this->_web.body();
		// Если тело сообщения получено
		if(!body.empty()){
			// Выполняем дешифрование полезной нагрузки
			const auto & result = this->_hash.decrypt(body.data(), body.size());
			// Если дешифрование выполнено
			if(!(this->_crypted = result.empty())){
				// Выполняем очистку данных тела
				this->_web.clearBody();
				// Формируем новое тело сообщения
				this->_web.body(result);
			// Если дешифрование не выполнено
			} else {
				// Выводим сообщение об ошибке
				this->_log->print("Decryption module has failed", log_t::flag_t::WARNING);
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Decryption module has failed");
			}
		}
	}
}
/**
 * compress Метод выполнения компрессии полезной нагрузки
 */
void awh::Http::compress() noexcept {
	// Если полезную нагрузку необходимо сжать
	if(this->_compressor.current == compress_t::NONE){
		// Получаем данные тела
		const auto & body = this->_web.body();
		// Если тело сообщения получено
		if(!body.empty()){
			// Определяем метод компрессии полезной нагрузки
			switch(static_cast <uint8_t> (this->_compressor.selected)){
				// Если полезную нагрузку необходимо сжать методом BROTLI
				case static_cast <uint8_t> (compress_t::BROTLI): {
					// Выполняем компрессию полезной нагрузки
					const auto & result = this->_hash.compress(body.data(), body.size(), hash_t::method_t::BROTLI);
					// Если компрессия выполнена
					if(!result.empty()){
						// Выполняем очистку данных тела
						this->_web.clearBody();
						// Формируем новое тело сообщения
						this->_web.body(result);
						// Устанавливаем флаг компрессии
						this->_compressor.current = compress_t::BROTLI;
					// Если компрессия не выполнена
					} else {
						// Выводим сообщение об ошибке
						this->_log->print("BROTLI compression module has failed", log_t::flag_t::WARNING);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "BROTLI compression module has failed");
					}
				} break;
				// Если полезную нагрузку необходимо сжать методом GZIP
				case static_cast <uint8_t> (compress_t::GZIP): {
					// Выполняем компрессию полезной нагрузки
					const auto & result = this->_hash.compress(body.data(), body.size(), hash_t::method_t::GZIP);
					// Если компрессия выполнена
					if(!result.empty()){
						// Выполняем очистку данных тела
						this->_web.clearBody();
						// Формируем новое тело сообщения
						this->_web.body(result);
						// Устанавливаем флаг компрессии
						this->_compressor.current = compress_t::GZIP;
					// Если компрессия не выполнена
					} else {
						// Выводим сообщение об ошибке
						this->_log->print("GZIP compression module has failed", log_t::flag_t::WARNING);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "GZIP compression module has failed");
					}
				} break;
				// Если полезную нагрузку необходимо сжать методом DEFLATE
				case static_cast <uint8_t> (compress_t::DEFLATE): {
					// Выполняем компрессию полезной нагрузки
					auto result = this->_hash.compress(body.data(), body.size(), hash_t::method_t::DEFLATE);
					// Если компрессия выполнена
					if(!result.empty()){
						// Выполняем очистку данных тела
						this->_web.clearBody();
						// Удаляем хвост в полученных данных
						this->_hash.rmTail(result);
						// Формируем новое тело сообщения
						this->_web.body(result);
						// Устанавливаем флаг компрессии
						this->_compressor.current = compress_t::DEFLATE;
					// Если компрессия не выполнена
					} else {
						// Выводим сообщение об ошибке
						this->_log->print("DEFLATE compression module has failed", log_t::flag_t::WARNING);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "DEFLATE compression module has failed");
					}
				} break;
			}
		}
	}
}
/**
 * decompress Метод выполнения декомпрессии полезной нагрузки
 */
void awh::Http::decompress() noexcept {
	// Если полезную нагрузку необходимо извлечь
	if(this->_compressor.current != compress_t::NONE){
		// Получаем данные тела
		const auto & body = this->_web.body();
		// Если тело сообщения получено
		if(!body.empty()){
			// Определяем метод компрессии полезной нагрузки
			switch(static_cast <uint8_t> (this->_compressor.current)){
				// Если полезную нагрузку нужно извлечь методом BROTLI
				case static_cast <uint8_t> (compress_t::BROTLI): {
					// Выполняем декомпрессию данных
					const auto & result = this->_hash.decompress(body.data(), body.size(), hash_t::method_t::BROTLI);
					// Если декомпрессия выполнена
					if(!result.empty()){
						// Выполняем очистку данных тела
						this->_web.clearBody();
						// Формируем новое тело сообщения
						this->_web.body(result);
						// Снимаем флаг компрессии
						this->_compressor.current = compress_t::NONE;
					}
				} break;
				// Если полезную нагрузку нужно извлечь методом GZIP
				case static_cast <uint8_t> (compress_t::GZIP): {
					// Выполняем декомпрессию полезной нагрузки
					const auto & result = this->_hash.decompress(body.data(), body.size(), hash_t::method_t::GZIP);
					// Если декомпрессия выполнена
					if(!result.empty()){
						// Выполняем очистку данных тела
						this->_web.clearBody();
						// Формируем новое тело сообщения
						this->_web.body(result);
						// Снимаем флаг компрессии
						this->_compressor.current = compress_t::NONE;
					}
				} break;
				// Если полезную нагрузку нужно извлечь методом DEFLATE
				case static_cast <uint8_t> (compress_t::DEFLATE): {
					// Получаем данные тела в бинарном виде
					vector <char> buffer(body.begin(), body.end());
					// Добавляем хвост в полученные данные
					this->_hash.setTail(buffer);
					// Выполняем декомпрессию полезной нагрузки
					const auto & result = this->_hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::DEFLATE);
					// Если декомпрессия выполнена
					if(!result.empty()){
						// Выполняем очистку данных тела
						this->_web.clearBody();
						// Формируем новое тело сообщения
						this->_web.body(result);
						// Снимаем флаг компрессии
						this->_compressor.current = compress_t::NONE;
					}
				} break;
			}
			// Определяем метод компрессии полезной нагрузки
			switch(static_cast <uint8_t> (this->_compressor.current)){
				// Если метод компрессии не изменился и остался BROTLI
				case static_cast <uint8_t> (compress_t::BROTLI): {
					// Сообщаем, что переданное тело содержит ошибки
					this->_log->print("BROTLI decompression module has failed", log_t::flag_t::WARNING);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "BROTLI decompression module has failed");
				} break;
				// Если метод компрессии не изменился и остался GZIP
				case static_cast <uint8_t> (compress_t::GZIP): {
					// Сообщаем, что переданное тело содержит ошибки
					this->_log->print("GZIP decompression module has failed", log_t::flag_t::WARNING);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "GZIP decompression module has failed");
				} break;
				// Если метод компрессии не изменился и остался DEFLATE
				case static_cast <uint8_t> (compress_t::DEFLATE): {
					// Сообщаем, что переданное тело содержит ошибки
					this->_log->print("DEFLATE decompression module has failed", log_t::flag_t::WARNING);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "DEFLATE decompression module has failed");
				} break;
			}
		}
	}
}
/**
 * commit Метод применения полученных результатов
 */
void awh::Http::commit() noexcept {
	// Если данные ещё не зафиксированы
	if(this->_status == status_t::NONE){
		// Выполняем проверку авторизации
		this->_status = this->status();
		// Если ключ соответствует
		if(this->_status == status_t::GOOD)
			// Устанавливаем стейт рукопожатия
			this->_state = state_t::GOOD;
		// Поменяем данные как бракованные
		else this->_state = state_t::BROKEN;
		// Получаем заголовок шифрования
		const string & encrypt = this->_web.header("x-awh-encryption");
		// Если заголовок найден
		if((this->_crypted = !encrypt.empty())){
			// Определяем размер шифрования
			switch(static_cast <uint16_t> (::stoi(encrypt))){
				// Если шифрование произведено 128 битным ключём
				case 128: this->_hash.cipher(hash_t::cipher_t::AES128); break;
				// Если шифрование произведено 192 битным ключём
				case 192: this->_hash.cipher(hash_t::cipher_t::AES192); break;
				// Если шифрование произведено 256 битным ключём
				case 256: this->_hash.cipher(hash_t::cipher_t::AES256); break;
			}
		}
		// Отключаем сжатие тела сообщения
		this->_compressor.current = compress_t::NONE;
		// Если заголовок с параметрами контента получен
		if(this->_web.isHeader("content-encoding")){
			// Получаем список доступных заголовков
			const auto & headers = this->_web.headers();
			// Если список заголовков получен
			if(!headers.empty()){
				/**
				 * extractFn Функция выбора типа компрессора
				 * @param compressor название компрессора в текстовом виде
				 */
				auto extractFn = [this](const string & compressor) noexcept -> void {
					// Отключаем сжатие тела сообщения
					this->_compressor.current = compress_t::NONE;
					// Если данные пришли сжатые методом Brotli
					if(this->_fmk->compare(compressor, "br"))
						// Устанавливаем тип компрессии полезной нагрузки
						this->_compressor.current = compress_t::BROTLI;
					// Если данные пришли сжатые методом GZip
					else if(this->_fmk->compare(compressor, "gzip"))
						// Устанавливаем тип компрессии полезной нагрузки
						this->_compressor.current = compress_t::GZIP;
					// Если данные пришли сжатые методом Deflate
					else if(this->_fmk->compare(compressor, "deflate"))
						// Устанавливаем тип компрессии полезной нагрузки
						this->_compressor.current = compress_t::DEFLATE;
				};
				// Список компрессоров которым выполненно сжатие
				vector <string> compressors;
				// Выполняем извлечение списка нужных заголовков
				const auto & range = headers.equal_range("content-encoding");
				// Выполняем перебор всего списка указанных заголовков
				for(auto it = range.first; it != range.second; ++it){
					// Создаём временный список компрессоров полученных в из заголовка
					vector <string> tmp;
					// Выполняем извлечение списка компрессоров
					this->_fmk->split(it->second, ",", tmp);
					// Если список компрессоров получен
					if(!tmp.empty())
						// Добавляем в общий список компрессоров
						compressors.insert(compressors.end(), tmp.begin(), tmp.end());
				}
				// Если список компрессоров получен
				if(!compressors.empty()){
					// Если компрессоров в списке больше 1-го
					if(compressors.size() > 1){
						// Выполняем перебор всех компрессоров
						for(size_t i = (compressors.size() - 1); i > 0; i--){
							// Выполняем определение типа компрессора
							extractFn(compressors.at(i));
							// Выполняем декомпрессию
							this->decompress();
						}
					}
					// Выполняем определение типа компрессора
					extractFn(compressors.front());
				}
			}
		}
		// Определяем к какому сервису относится модуль
		switch(static_cast <uint8_t> (this->_web.hid())){
			// Если модуль соответствует клиенту
			case static_cast <uint8_t> (web_t::hid_t::CLIENT): {
				// Если заголовок с параметрами передачи контента получен и контент не зашифрован
				if(this->_web.isHeader("transfer-encoding") && (this->_compressor.current == compress_t::NONE)){
					// Получаем список доступных заголовков
					const auto & headers = this->_web.headers();
					// Если список заголовков получен
					if(!headers.empty()){
						/**
						 * extractFn Функция выбора типа компрессора
						 * @param compressor название компрессора в текстовом виде
						 */
						auto extractFn = [this](const string & compressor) noexcept -> bool {
							// Результат работы функции
							bool result = false;
							// Если данные пришли сжатые методом Brotli
							if(this->_fmk->compare(compressor, "br"))
								// Устанавливаем тип компрессии полезной нагрузки
								result = static_cast <bool> (this->_compressor.current = compress_t::BROTLI);
							// Если данные пришли сжатые методом GZip
							else if(this->_fmk->compare(compressor, "gzip"))
								// Устанавливаем тип компрессии полезной нагрузки
								result = static_cast <bool> (this->_compressor.current = compress_t::GZIP);
							// Если данные пришли сжатые методом Deflate
							else if(this->_fmk->compare(compressor, "deflate"))
								// Устанавливаем тип компрессии полезной нагрузки
								result = static_cast <bool> (this->_compressor.current = compress_t::DEFLATE);
							// Если мы получили параметр передачи данных чанками
							else if(this->_fmk->compare(compressor, "chunked"))
								// Выполняем активацию передачу данных чанками
								this->_te.chunking = true;
							// Выводим результат
							return result;
						};
						// Список полученных параметров
						vector <string> params;
						// Выполняем извлечение списка нужных заголовков
						const auto & range = headers.equal_range("transfer-encoding");
						// Выполняем перебор всего списка указанных заголовков
						for(auto it = range.first; it != range.second; ++it){
							// Создаём временный список параметров полученных в из заголовка
							vector <string> tmp;
							// Выполняем извлечение списка параметров
							this->_fmk->split(it->second, ",", tmp);
							// Если список параметров получен
							if(!tmp.empty())
								// Добавляем в общий список параметров
								params.insert(params.end(), tmp.begin(), tmp.end());
						}
						// Если список параметров получен
						if(!params.empty()){
							// Если параметров в списке больше 1-го
							if(params.size() > 1){
								// Выполняем перебор всех параметров
								for(size_t i = (params.size() - 1); i > 0; i--){
									// Выполняем определение типа параметров
									if(extractFn(params.at(i)))
										// Выполняем декомпрессию
										this->decompress();
								}
							}
							// Выполняем определение типа параметров
							extractFn(params.front());
						}
						// Выполняем удаление заголовка
						this->_web.delHeader("transfer-encoding");
						// Если активирован режим чанкинга
						if(this->_te.chunking)
							// Выполняем корректировку значения заголовка
							this->_web.header("Transfer-Encoding", "chunked");
					}
				}
				// Если тело полезной нагрузки получено в сжатом виде
				if(this->_compressor.current != compress_t::NONE)
					// Устанавливаем флаг метода компрессии
					this->_compressor.selected = this->_compressor.current;
			} break;
			// Если модуль соответствует серверу
			case static_cast <uint8_t> (web_t::hid_t::SERVER): {
				// Отключаем сжатие тела сообщения
				this->_compressor.selected = compress_t::NONE;
				// Если список поддерживаемых протоколов установлен
				if(!this->_compressor.supports.empty()){
					// Если заголовок с запрашиваемым контентом существует
					if(this->_web.isHeader("accept-encoding")){
						// Получаем список доступных заголовков
						const auto & headers = this->_web.headers();
						// Если список заголовков получен
						if(!headers.empty()){
							// Список запрашиваемых компрессоров клиентом
							multimap <float, compress_t> requested;
							// Выполняем извлечение списка нужных заголовков
							const auto & range = headers.equal_range("accept-encoding");
							// Выполняем перебор всего списка указанных заголовков
							for(auto it = range.first; it != range.second; ++it){
								// Если конкретный метод сжатия запрашивается любой
								if(this->_fmk->compare(it->second, "*")){
									// Выполняем перебор всего списка доступных компрессоров
									for(auto & compressor : this->_compressor.supports)
										// Добавляем в список запрашиваемых компрессоров
										requested.emplace(compressor.first, compressor.second);
								// Если указан конкретный метод компрессии
								} else {
									// Список компрессоров которым выполненно сжатие
									vector <string> compressors;
									// Выполняем извлечение списка компрессоров
									this->_fmk->split(it->second, ",", compressors);
									// Если список компрессоров получен
									if(!compressors.empty()){
										// Вес запрашиваемого компрессора
										float weight = 1.0f;
										// Выполняем перебор списка запрашиваемых компрессоров
										for(auto & compressor : compressors){
											// Если найден вес компрессора
											if(this->_fmk->exists(";q=", compressor)){
												// Выполняем поиск разделителя
												const size_t pos = compressor.rfind(';');
												// Если разделитель найден
												if((pos != string::npos) && (compressor.size() >= (pos + 4))){
													// Получаем вес компрессора
													const string & second = compressor.substr(pos + 3);
													// Если вес указан верный
													if(!second.empty() && (this->_fmk->is(second, fmk_t::check_t::DECIMAL) || this->_fmk->is(second, fmk_t::check_t::NUMBER))){
														// Изавлекаем название компрессора
														const string & first = compressor.substr(0, pos);
														// Если данные пришли сжатые методом Brotli
														if(this->_fmk->compare(first, "br"))
															// Добавляем в список полученный компрессор
															requested.emplace(::stof(second), compress_t::BROTLI);
														// Если данные пришли сжатые методом GZip
														else if(this->_fmk->compare(first, "gzip"))
															// Добавляем в список полученный компрессор
															requested.emplace(::stof(second), compress_t::GZIP);
														// Если данные пришли сжатые методом Deflate
														else if(this->_fmk->compare(first, "deflate"))
															// Добавляем в список полученный компрессор
															requested.emplace(::stof(second), compress_t::DEFLATE);
													// Если вес компрессора указан не является числом
													} else {
														// Выводим сообщение об ошибке
														this->_log->print("Weight of the requested %s compressor is not a number [%s]", log_t::flag_t::WARNING, compressor.substr(0, pos).c_str(), second.c_str());
														// Если функция обратного вызова на на вывод ошибок установлена
														if(this->_callback.is("error"))
															// Выполняем функцию обратного вызова
															this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_fmk->format("Weight of the requested %s compressor is not a number [%s]", compressor.substr(0, pos).c_str(), second.c_str()));
													}
												// Если мы получили данные в неверном формате
												} else {
													// Выводим сообщение об ошибке
													this->_log->print("We received data in the wrong format [%s]", log_t::flag_t::WARNING, compressor.c_str());
													// Если функция обратного вызова на на вывод ошибок установлена
													if(this->_callback.is("error"))
														// Выполняем функцию обратного вызова
														this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_fmk->format("We received data in the wrong format [%s]", compressor.c_str()));
												}
											// Если вес компрессора не установлен
											} else {
												// Если данные пришли сжатые методом Brotli
												if(this->_fmk->compare(compressor, "br"))
													// Добавляем в список полученный компрессор
													requested.emplace(weight, compress_t::BROTLI);
												// Если данные пришли сжатые методом GZip
												else if(this->_fmk->compare(compressor, "gzip"))
													// Добавляем в список полученный компрессор
													requested.emplace(weight, compress_t::GZIP);
												// Если данные пришли сжатые методом Deflate
												else if(this->_fmk->compare(compressor, "deflate"))
													// Добавляем в список полученный компрессор
													requested.emplace(weight, compress_t::DEFLATE);
												// Выполняем уменьшение веса выбранного компрессора
												weight -= .1f;
											}
										}
									}
								}
							}
							// Если список запрашиваемых компрессоров получен
							if(!requested.empty()){
								// Выполняем перебор списка запрашиваемых компрессоров
								for(auto it = requested.rbegin(); it != requested.rend(); ++it){
									// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
									if(this->_fmk->findInMap(it->second, this->_compressor.supports) != this->_compressor.supports.end()){
										// Устанавливаем флаг метода компрессии
										this->_compressor.selected = it->second;
										// Выходим из цикла
										break;
									}
								}
							}
						}
					}
				}
				// Если заголовок с параметрами передачи данных Transfer-Encoding существует
				if((this->_te.enabled = this->_web.isHeader("te"))){
					// Если версия протокола подключения выше чем HTTP/1.1
					if(this->_web.request().version > 1.1f){
						// Выполняем активации получение трейлеров
						this->_te.trailers = true;
						// Выполняем активацию передачу данных чанками
						this->_te.chunking = true;
					// Если версия протокола подключения не выше HTTP/1.1
					} else {
						// Получаем список доступных заголовков
						const auto & headers = this->_web.headers();
						// Если список заголовков получен
						if(!headers.empty()){
							// Список запрашиваемых компрессоров клиентом
							multimap <float, compress_t> requested;
							// Выполняем извлечение списка нужных заголовков
							const auto & range = headers.equal_range("te");
							// Выполняем перебор всего списка указанных заголовков
							for(auto it = range.first; it != range.second; ++it){
								// Список параметров запроса для Transfer-Encoding
								vector <string> params;
								// Выполняем извлечение списка параметров
								this->_fmk->split(it->second, ",", params);
								// Если список параметров получен
								if(!params.empty()){
									// Вес запрашиваемого компрессора
									float weight = 1.0f;
									// Выполняем перебор списка параметров
									for(auto & param : params){
										// Если найден вес компрессора
										if(this->_fmk->exists(";q=", param)){
											// Выполняем поиск разделителя
											const size_t pos = param.rfind(';');
											// Если разделитель найден
											if((pos != string::npos) && (param.size() >= (pos + 4))){
												// Получаем вес компрессора
												const string & second = param.substr(pos + 3);
												// Если вес указан верный
												if(!second.empty() && (this->_fmk->is(second, fmk_t::check_t::DECIMAL) || this->_fmk->is(second, fmk_t::check_t::NUMBER))){
													// Изавлекаем название компрессора
													const string & first = param.substr(0, pos);
													// Если данные пришли сжатые методом Brotli
													if(this->_fmk->compare(first, "br"))
														// Добавляем в список полученный компрессор
														requested.emplace(::stof(second), compress_t::BROTLI);
													// Если данные пришли сжатые методом GZip
													else if(this->_fmk->compare(first, "gzip"))
														// Добавляем в список полученный компрессор
														requested.emplace(::stof(second), compress_t::GZIP);
													// Если данные пришли сжатые методом Deflate
													else if(this->_fmk->compare(first, "deflate"))
														// Добавляем в список полученный компрессор
														requested.emplace(::stof(second), compress_t::DEFLATE);
													// Если получен параметр разрешающий использовать трейлеры
													else if(this->_fmk->compare(first, "trailers"))
														// Выполняем активации получение трейлеров
														this->_te.trailers = true;
													// Если получен параметр разрешающий обмениваться чанками
													else if(this->_fmk->compare(first, "chunked"))
														// Выполняем активацию передачу данных чанками
														this->_te.chunking = true;
												// Если вес компрессора указан не является числом
												} else {
													// Выводим сообщение об ошибке
													this->_log->print("Weight of the requested %s compressor is not a number [%s]", log_t::flag_t::WARNING, param.substr(0, pos).c_str(), second.c_str());
													// Если функция обратного вызова на на вывод ошибок установлена
													if(this->_callback.is("error"))
														// Выполняем функцию обратного вызова
														this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_fmk->format("Weight of the requested %s compressor is not a number [%s]", param.substr(0, pos).c_str(), second.c_str()));
												}
											// Если мы получили данные в неверном формате
											} else {
												// Выводим сообщение об ошибке
												this->_log->print("We received data in the wrong format [%s]", log_t::flag_t::WARNING, param.c_str());
												// Если функция обратного вызова на на вывод ошибок установлена
												if(this->_callback.is("error"))
													// Выполняем функцию обратного вызова
													this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_fmk->format("We received data in the wrong format [%s]", param.c_str()));
											}
										// Если вес компрессора не установлен
										} else {
											// Если данные пришли сжатые методом Brotli
											if(this->_fmk->compare(param, "br"))
												// Добавляем в список полученный компрессор
												requested.emplace(weight, compress_t::BROTLI);
											// Если данные пришли сжатые методом GZip
											else if(this->_fmk->compare(param, "gzip"))
												// Добавляем в список полученный компрессор
												requested.emplace(weight, compress_t::GZIP);
											// Если данные пришли сжатые методом Deflate
											else if(this->_fmk->compare(param, "deflate"))
												// Добавляем в список полученный компрессор
												requested.emplace(weight, compress_t::DEFLATE);
											// Если получен параметр разрешающий использовать трейлеры
											else if(this->_fmk->compare(param, "trailers"))
												// Выполняем активации получение трейлеров
												this->_te.trailers = true;
											// Если получен параметр разрешающий обмениваться чанками
											else if(this->_fmk->compare(param, "chunked"))
												// Выполняем активацию передачу данных чанками
												this->_te.chunking = true;
											// Выполняем уменьшение веса выбранного компрессора
											weight -= .1f;
										}
									}
								}
							}
							// Если список запрашиваемых компрессоров получен
							if(!requested.empty() && !this->_compressor.supports.empty()){
								// Выполняем перебор списка запрашиваемых компрессоров
								for(auto it = requested.rbegin(); it != requested.rend(); ++it){
									// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
									if(this->_fmk->findInMap(it->second, this->_compressor.supports) != this->_compressor.supports.end()){
										// Устанавливаем флаг метода компрессии
										this->_compressor.selected = it->second;
										// Выходим из цикла
										break;
									}
								}
							}
							// Выполняем удаление заголовка
							this->_web.delHeader("te");
							// Выполняем извлечение данных заголовка подключения
							string connection = this->_web.header("connection");
							// Если заголовок подключения получен
							if(!connection.empty()){
								// Переводим значение в нижний регистр
								this->_fmk->transform(connection, fmk_t::transform_t::LOWER);
								// Выполняем поиск заголовка Transfer-Encoding
								const size_t pos = connection.find("te");
								// Если заголовок найден
								if(pos != string::npos){
									// Выполняем удаление значение TE из заголовка
									connection.erase(pos, 2);
									// Если первый символ является запятой, удаляем
									if(connection.front() == ',')
										// Удаляем запятую
										connection.erase(0, 1);
									// Выполняем удаление лишних пробелов
									this->_fmk->transform(connection, fmk_t::transform_t::TRIM);
									// Выполняем удаление заголовка
									this->_web.delHeader("connection");
									// Если значение подключение получено
									if(!connection.empty())
										// Устанавливаем отредактированное подключение
										this->_web.header("Connection", connection);
									// Иначе заменяем значение подключение по умолчанию
									else this->_web.header("Connection", "Keep-Alive");
								}
							}
						}
					}
				}
			} break;
		}
		// Выполняем установку стейта завершения получения данных
		this->_web.state(web_t::state_t::END);
	}
}
/**
 * precise Метод установки флага точной установки хоста
 * @param mode флаг для установки
 */
void awh::Http::precise(const bool mode) noexcept {
	// Выполняем установку флага точной установки хоста
	this->_precise = mode;
}
/**
 * clear Метод очистки собранных данных
 */
void awh::Http::clear() noexcept {
	// Выполняем очистку данных парсера
	this->_web.clear();
	// Выполняем сброс чёрного списка HTTP заголовков
	this->_black.clear();
	// Очищаем список установленных трейлеров
	this->_trailers.clear();
	// Снимаем флаг зашифрованной полезной нагрузки
	this->_crypted = false;
	// Выполняем сброс флага формирования чанков
	this->_te.chunking = false;
	// Снимаем флаг сжатой полезной нагрузки
	this->_compressor.current = compress_t::NONE;
}
/**
 * reset Метод сброса параметров запроса
 */
void awh::Http::reset() noexcept {
	// Выполняем сброс данных парсера
	this->_web.reset();
	// Выполняем сброс стейта текущего запроса
	this->_state = state_t::NONE;
	// Выполняем сброс стейта авторизации
	this->_status = status_t::NONE;
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
 * proto Метод извлечения список протоколов к которому принадлежит заголовок
 * @param key ключ заголовка
 * @return    список протоколов
 */
set <awh::web_t::proto_t> awh::Http::proto(const string & key) const noexcept {
	// Выполняем извлечение списка протоколов к которому принадлежит заголовок
	return this->_web.proto(key);
}
/**
 * payload Метод чтения чанка полезной нагрузки
 * @return текущий чанк полезной нагрузки
 */
const vector <char> awh::Http::payload() const noexcept {
	// Результат работы функции
	vector <char> result;
	// Выполняем компрессию полезной нагрузки
	const_cast <http_t *> (this)->compress();
	// Выполняем шифрование полезной нагрузки
	const_cast <http_t *> (this)->encrypt();
	// Получаем собранные данные тела
	vector <char> * body = const_cast <vector <char> *> (&this->_web.body());
	// Если данные тела ещё существуют
	if(!body->empty()){
		// Версия протокола HTTP
		float version = 1.1f;
		// Определяем тип HTTP-модуля
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
		if((version > 1.0f) && this->_te.chunking){
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
					// Определяем тип HTTP-модуля
					switch(static_cast <uint8_t> (this->_web.hid())){
						// Если мы работаем с клиентом
						case static_cast <uint8_t> (web_t::hid_t::CLIENT):
							// Добавляем конец запроса
							chunk.append("\r\n0\r\n\r\n");
						break;
						// Если мы работаем с сервером
						case static_cast <uint8_t> (web_t::hid_t::SERVER): {
							// Если нужно отправить трейлеры
							if(!this->_trailers.empty())
								// Добавляем конец запроса
								chunk.append("\r\n0\r\n");
							// Добавляем конец запроса
							else chunk.append("\r\n0\r\n\r\n");
						} break;
					}
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
				// Освобождаем память
				vector <char> ().swap(* body);
			}
		}
	}
	// Если тело передаваемых данных уже пустое
	if(body->empty()){
		// Снимаем флаг зашифрованной полезной нагрузки
		const_cast <http_t *> (this)->_crypted = false;
		// Снимаем флаг сжатой полезной нагрузки
		const_cast <http_t *> (this)->_compressor.current = compress_t::NONE;
	}
	// Выводим результат
	return result;
}
/**
 * payload Метод установки чанка полезной нагрузки
 * @param payload буфер чанка полезной нагрузки
 */
void awh::Http::payload(const vector <char> & payload) noexcept {
	// Устанавливаем данные телал сообщения
	this->_web.body(payload);
}
/**
 * clear Метод очистки данных HTTP-протокола
 * @param suite тип набора к которому соответствует заголовок
 */
void awh::Http::clear(const suite_t suite) noexcept {
	// Определяем запрашиваемый набор к которому принадлежит заголовок
	switch(static_cast <uint8_t> (suite)){
		// Если набор соответствует телу сообщения
		case static_cast <uint8_t> (suite_t::BODY): {
			// Выполняем очистку данных тела
			this->_web.clearBody();
			// Снимаем флаг зашифрованной полезной нагрузки
			const_cast <http_t *> (this)->_crypted = false;
			// Снимаем флаг сжатой полезной нагрузки
			const_cast <http_t *> (this)->_compressor.current = compress_t::NONE;	
		} break;
		// Если набор соответствует заголовку сообщения
		case static_cast <uint8_t> (suite_t::HEADER): {
			// Выполняем сброс флага формирования чанков
			this->_te.chunking = false;
			// Выполняем очистку списка заголовков
			this->_web.clearHeaders();
		} break;
	}
}
/**
 * black Метод добавления заголовка в чёрный список
 * @param key ключ заголовка
 */
void awh::Http::black(const string & key) noexcept {
	// Если ключ заголовка передан, добавляем в список
	if(!key.empty())
		// Выполняем добавление заголовка в чёрный список
		this->_black.emplace(this->_fmk->transform(key, fmk_t::transform_t::LOWER));
}
/**
 * body Метод получения данных тела запроса
 * @return буфер данных тела запроса
 */
const vector <char> & awh::Http::body() const noexcept {
	// Выполняем дешифрование полезной нагрузки
	const_cast <http_t *> (this)->decrypt();
	// Выполняем декомпрессию полезной нагрузки
	const_cast <http_t *> (this)->decompress();
	// Выводим данные тела
	return this->_web.body();
}
/**
 * body Метод установки данных тела
 * @param body буфер тела для установки
 */
void awh::Http::body(const vector <char> & body) noexcept {
	// Выполняем дешифрование полезной нагрузки
	this->decrypt();
	// Выполняем декомпрессию полезной нагрузки
	this->decompress();
	// Устанавливаем данные телал сообщения
	this->_web.body(body);
}
/**
 * trailers Метод получения списка установленных трейлеров
 * @return количество установленных трейлеров
 */
size_t awh::Http::trailers() const noexcept {
	// Выводим список установленных трейлеров
	return this->_trailers.size();
}
/**
 * trailer Метод установки трейлера
 * @param key ключ заголовка
 * @param val значение заголовка
 */
void awh::Http::trailer(const string & key, const string & val) noexcept {
	// Если ключ и значение заголовка переданы
	if(!key.empty() && !val.empty()){
		// Определяем тип HTTP-модуля
		switch(static_cast <uint8_t> (this->_web.hid())){
			// Если мы работаем с клиентом
			case static_cast <uint8_t> (web_t::hid_t::CLIENT): {
				// Выводим сообщение, что клиент не может отправлять трейлеры
				this->_log->print("Add trailer [%s=%s] failed because the client cannot send trailers", log_t::flag_t::WARNING, key.c_str(), val.c_str());
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_fmk->format("Add trailer [%s=%s] failed because the client cannot send trailers", key.c_str(), val.c_str()));
			} break;
			// Если мы работаем с сервером
			case static_cast <uint8_t> (web_t::hid_t::SERVER): {
				// Если разрешено добавление трейлеров
				if(this->_te.trailers){
					// Добавляем заголовок названия трейлера
					this->_web.header("Trailer", key);
					// Выполняем добавление заголовка в список трейлеров
					this->_trailers.emplace(this->_fmk->transform(key, fmk_t::transform_t::LOWER), val);
				// Если добавление трейлеров не запрашивалось клиентом
				} else {
					// Выводим сообщение о невозможности установки трейлера
					this->_log->print("It is impossible to add a [%s=%s] trailer because the client did not request the transfer of trailers", log_t::flag_t::WARNING, key.c_str(), val.c_str());
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, this->_fmk->format("It is impossible to add a [%s=%s] trailer because the client did not request the transfer of trailers", key.c_str(), val.c_str()));
				}
			} break;
		}	
	}
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
	// Если мы работаем с клиентом
	if(this->_web.hid() == web_t::hid_t::SERVER){
		// Если список трейлеров установлен
		if(this->_te.trailers && !this->_trailers.empty()){
			// Название трейлера для добавления
			string name = "";
			// Выполняем перебор всех трейлеров
			for(auto & trailer : this->_trailers){
				// Выполняем получение названия трейлера
				name = trailer.first;
				// Добавляем заголовок названия трейлера
				this->_web.header("Trailer", this->_fmk->transform(name, fmk_t::transform_t::SMART));
			}
		}
	}
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
	// Если ключ заголовка соответствует протоколу подключения
	} else if(this->_fmk->compare(key, ":protocol")) {
		// Если протокол соответствует WebSocket-у
		if(this->_fmk->compare(val, "websocket"))
			// Выполняем установку идентичность протоколу WebSocket
			this->_identity = identity_t::WS;
		/* Просто пропускаем, потому, что протокол мы не используем */
	// Если ключ заголовка соответствует схеме протокола
	} else if(this->_fmk->compare(key, ":scheme")) {
		/* Просто пропускаем, потому, что схему мы не используем */
	// Если ключ соответствует доменному имени
	} else if(this->_fmk->compare(key, ":authority")) {
		// Создаём объект работы с IP-адресами
		net_t net;
		// Устанавливаем хост
		this->header("Host", val);
		// Получаем объект параметров запроса
		web_t::req_t request = this->_web.request();
		// Получаем хост запрашиваемого сервера
		request.url.host = val;
		// Выполняем установку порта по умолчанию
		request.url.port = 80;
		// Выполняем установку схемы запроса
		request.url.schema = "http";
		// Выполняем поиск разделителя
		const size_t pos = request.url.host.rfind(':');
		// Если разделитель найден
		if(pos != string::npos){
			// Получаем порт сервера
			const string & port = request.url.host.substr(pos + 1);
			// Если данные порта являются числом
			if(this->_fmk->is(port, fmk_t::check_t::NUMBER)){
				// Выполняем установку порта сервера
				request.url.port = static_cast <u_int> (::stoi(port));
				// Выполняем получение хоста сервера
				request.url.host = request.url.host.substr(0, pos);
				// Если порт установлен как 443
				if(request.url.port == 443)
					// Выполняем установку защищённую схему запроса
					request.url.schema = "https";
			}
		}
		// Определяем тип домена
		switch(static_cast <uint8_t> (net.host(request.url.host))){
			// Если - это IP-адрес сети IPv4
			case static_cast <uint8_t> (net_t::type_t::IPV4): {
				// Выполняем установку семейства IP-адресов
				request.url.family = AF_INET;
				// Выполняем установку IPv4 адреса
				request.url.ip = request.url.host;
			} break;
			// Если - это IP-адрес сети IPv6
			case static_cast <uint8_t> (net_t::type_t::IPV6): {
				// Выполняем установку семейства IP-адресов
				request.url.family = AF_INET6;
				// Выполняем установку IPv6 адреса
				request.url.ip = net = request.url.host;
			} break;
			// Если - это доменное имя
			case static_cast <uint8_t> (net_t::type_t::DOMN):
				// Выполняем установку IPv6 адреса
				request.url.domain = this->_fmk->transform(request.url.host, fmk_t::transform_t::LOWER);
			break;
		}
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
 * auth Метод проверки статуса авторизации
 * @return результат проверки
 */
awh::Http::status_t awh::Http::auth() const noexcept {
	// Выводим результат проверки
	return this->_status;
}
/**
 * auth Метод извлечения строки авторизации
 * @param flag флаг выполняемого процесса
 * @param prov параметры провайдера обмена сообщениями
 * @return     строка авторизации на удалённом сервере
 */
string awh::Http::auth(const process_t flag, const web_t::provider_t & prov) const noexcept {
	// Определяем флаг выполняемого процесса
	switch(static_cast <uint8_t> (flag)){
		// Если нужно сформировать данные запроса
		case static_cast <uint8_t> (process_t::REQUEST): {
			// Получаем объект ответа клиенту
			const web_t::req_t & req = static_cast <const web_t::req_t &> (prov);
			// Если параметры REST-запроса переданы
			if(!req.url.empty() && (req.method != web_t::method_t::NONE)){
				// Устанавливаем параметры REST-запроса
				this->_auth.client.uri(this->_uri.url(req.url));
				// Определяем метод запроса
				switch(static_cast <uint8_t> (req.method)){
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
 * url Метод извлечения параметров запроса
 * @return установленные параметры запроса
 */
const awh::uri_t::url_t & awh::Http::url() const noexcept {
	// Выводим параметры запроса
	return this->_web.request().url;
}
/**
 * compression Метод извлечения выбранного метода компрессии
 * @return метод компрессии
 */
awh::Http::compress_t awh::Http::compression() const noexcept {
	// Выполняем извлечение выбранного метода компрессии
	return this->_compressor.selected;
}
/**
 * compression Метод установки выбранного метода компрессии
 * @param compress метод компрессии
 */
void awh::Http::compression(const compress_t compress) noexcept {
	// Выполняем установку выбранного метода компрессии
	this->_compressor.selected = compress;
}
/**
 * compressors Метод установки списка поддерживаемых компрессоров
 * @param compress методы компрессии данных полезной нагрузки
 */
void awh::Http::compressors(const vector <compress_t> & compressors) noexcept {
	// Если список архиваторов передан
	if(!compressors.empty()){
		// Вес запрашиваемого компрессора
		float weight = 1.0f;
		// Выполняем очистку списка доступных компрессоров
		this->_compressor.supports.clear();
		// Выполняем перебор списка запрашиваемых компрессоров
		for(auto & compressor : compressors){
			// Выполняем установку полученного компрессера
			this->_compressor.supports.emplace(weight, compressor);
			// Выполняем уменьшение веса компрессора
			weight -= .1f;
		}
	}
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
		// Устанавливаем флаг разрешающий параметры Transfer-Encoding
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_te), reinterpret_cast <const char *> (&this->_te) + sizeof(this->_te));
		// Устанавливаем размер одного чанка
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_chunk), reinterpret_cast <const char *> (&this->_chunk) + sizeof(this->_chunk));
		// Устанавливаем стейт текущего запроса
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_state), reinterpret_cast <const char *> (&this->_state) + sizeof(this->_state));
		// Устанавливаем стейт проверки авторизации
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_status), reinterpret_cast <const char *> (&this->_status) + sizeof(this->_status));
		// Устанавливаем флаг зашифрованных данных полузной нагрузки
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_crypted), reinterpret_cast <const char *> (&this->_crypted) + sizeof(this->_crypted));
		// Устанавливаем флаг требования шифрования данных полезной нагрузки
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_encryption), reinterpret_cast <const char *> (&this->_encryption) + sizeof(this->_encryption));
		// Устанавливаем метод компрессии хранимых данных
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_compressor.current), reinterpret_cast <const char *> (&this->_compressor.current) + sizeof(this->_compressor.current));
		// Устанавливаем метод компрессии отправляемых данных
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_compressor.selected), reinterpret_cast <const char *> (&this->_compressor.selected) + sizeof(this->_compressor.selected));
		// Получаем количество поддерживаемых компрессоров
		count = this->_compressor.supports.size();
		// Устанавливаем количество поддерживаемых компрессоров
		result.insert(result.end(), reinterpret_cast <const char *> (&count), reinterpret_cast <const char *> (&count) + sizeof(count));
		// Если список поддерживаемых компрессоров не пустой
		if(!this->_compressor.supports.empty()){
			// Выполняем перебор всех поддерживаемых компрессоров
			for(auto & compressor : this->_compressor.supports){
				// Выполняем установку веска компрессора
				result.insert(result.end(), reinterpret_cast <const char *> (&compressor.first), reinterpret_cast <const char *> (&compressor.first) + sizeof(compressor.first));
				// Выполняем установку идентификатора компрессора
				result.insert(result.end(), reinterpret_cast <const char *> (&compressor.second), reinterpret_cast <const char *> (&compressor.second) + sizeof(compressor.second));
			}
		}
		// Получаем размер идентификатора сервиса
		length = this->_ident.id.size();
		// Устанавливаем размер идентификатора сервиса
		result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
		// Устанавливаем данные идентификатора сервиса
		result.insert(result.end(), this->_ident.id.begin(), this->_ident.id.end());
		// Получаем размер версии модуля приложения
		length = this->_ident.version.size();
		// Устанавливаем размер версии модуля приложения
		result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
		// Устанавливаем данные версии модуля приложения
		result.insert(result.end(), this->_ident.version.begin(), this->_ident.version.end());
		// Получаем размер названия сервиса
		length = this->_ident.name.size();
		// Устанавливаем размер названия сервиса
		result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
		// Устанавливаем данные названия сервиса
		result.insert(result.end(), this->_ident.name.begin(), this->_ident.name.end());
		// Получаем размер User-Agent для HTTP-запроса
		length = this->_userAgent.size();
		// Устанавливаем размер User-Agent для HTTP-запроса
		result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
		// Устанавливаем данные User-Agent для HTTP-запроса
		result.insert(result.end(), this->_userAgent.begin(), this->_userAgent.end());
		// Получаем количество записей чёрного списка
		count = this->_black.size();
		// Устанавливаем количество записей чёрного списка
		result.insert(result.end(), reinterpret_cast <const char *> (&count), reinterpret_cast <const char *> (&count) + sizeof(count));
		// Выполняем переход по всему чёрному списку
		for(auto & header : this->_black){
			// Получаем размер заголовка из чёрного списка
			length = header.size();
			// Устанавливаем размер заголовка из чёрного списка
			result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
			// Устанавливаем данные заголовка из чёрного списка
			result.insert(result.end(), header.begin(), header.end());
		}
		// Получаем дамп WEB данных
		const auto & web = this->_web.dump();
		// Получаем размер буфера WEB данных
		length = web.size();
		// Устанавливаем размер буфера WEB данных
		result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
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
		// Выполняем получение флага разрешающий параметры Transfer-Encoding
		::memcpy(reinterpret_cast <void *> (&this->_te), data.data() + offset, sizeof(this->_te));
		// Выполняем смещение в буфере
		offset += sizeof(this->_te);
		// Выполняем получение размера одного чанка
		::memcpy(reinterpret_cast <void *> (&this->_chunk), data.data() + offset, sizeof(this->_chunk));
		// Выполняем смещение в буфере
		offset += sizeof(this->_chunk);
		// Выполняем получение стейта текущего запроса
		::memcpy(reinterpret_cast <void *> (&this->_state), data.data() + offset, sizeof(this->_state));
		// Выполняем смещение в буфере
		offset += sizeof(this->_state);
		// Выполняем получение стейта проверки авторизации
		::memcpy(reinterpret_cast <void *> (&this->_status), data.data() + offset, sizeof(this->_status));
		// Выполняем смещение в буфере
		offset += sizeof(this->_status);
		// Выполняем получение флага зашифрованных хранимых данных полезной нагрузки
		::memcpy(reinterpret_cast <void *> (&this->_crypted), data.data() + offset, sizeof(this->_crypted));
		// Выполняем смещение в буфере
		offset += sizeof(this->_crypted);
		// Выполняем получение флага требования шифрования данных полезной нагрузки
		::memcpy(reinterpret_cast <void *> (&this->_encryption), data.data() + offset, sizeof(this->_encryption));
		// Выполняем смещение в буфере
		offset += sizeof(this->_encryption);
		// Выполняем получение метода компрессии хранимых данных
		::memcpy(reinterpret_cast <void *> (&this->_compressor.current), data.data() + offset, sizeof(this->_compressor.current));
		// Выполняем смещение в буфере
		offset += sizeof(this->_compressor.current);
		// Выполняем получение метода компрессии отправляемых данных
		::memcpy(reinterpret_cast <void *> (&this->_compressor.selected), data.data() + offset, sizeof(this->_compressor.selected));
		// Выполняем смещение в буфере
		offset += sizeof(this->_compressor.selected);
		// Выполняем получение количества поддерживаемых компрессоров
		::memcpy(reinterpret_cast <void *> (&count), data.data() + offset, sizeof(count));
		// Выполняем смещение в буфере
		offset += sizeof(count);
		// Выполняем очистку списку поддерживаемых компрессоров
		this->_compressor.supports.clear();
		// Если количество компрессоров больше нуля
		if(count > 0){
			// Выполняем последовательную установку всех компрессоров
			for(size_t i = 0; i < count; i++){
				// Вес компрессора
				float weight = .0f;
				// Идентификатор компрессора
				compress_t compressor = compress_t::NONE;
				// Выполняем получение веса компрессора
				::memcpy(reinterpret_cast <void *> (&weight), data.data() + offset, sizeof(weight));
				// Выполняем смещение в буфере
				offset += sizeof(weight);
				// Выполняем получение идентификатора компрессора
				::memcpy(reinterpret_cast <void *> (&compressor), data.data() + offset, sizeof(compressor));
				// Выполняем смещение в буфере
				offset += sizeof(compressor);
				// Выполняем установку метода компрессора
				this->_compressor.supports.emplace(weight, compressor);
			}
		}
		// Выполняем получение размера идентификатора сервиса
		::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Если размер получен
		if(length > 0){
			// Выделяем память для данных идентификатора сервиса
			this->_ident.id.resize(length, 0);
			// Выполняем получение данных идентификатора сервиса
			::memcpy(reinterpret_cast <void *> (this->_ident.id.data()), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
		}
		// Выполняем получение размера версии модуля приложения
		::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Если размер получен
		if(length > 0){
			// Выделяем память для данных версии модуля приложения
			this->_ident.version.resize(length, 0);
			// Выполняем получение данных версии модуля приложения
			::memcpy(reinterpret_cast <void *> (this->_ident.version.data()), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
		}
		// Выполняем получение размера названия сервиса
		::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Если размер получен
		if(length > 0){
			// Выделяем память для данных названия сервиса
			this->_ident.name.resize(length, 0);
			// Выполняем получение данных названия сервиса
			::memcpy(reinterpret_cast <void *> (this->_ident.name.data()), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
		}
		// Выполняем получение размера User-Agent для HTTP-запроса
		::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Если размер получен
		if(length > 0){
			// Выделяем память для данных User-Agent для HTTP-запроса
			this->_userAgent.resize(length, 0);
			// Выполняем получение данных User-Agent для HTTP-запроса
			::memcpy(reinterpret_cast <void *> (this->_userAgent.data()), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
		}
		// Выполняем получение количества записей чёрного списка
		::memcpy(reinterpret_cast <void *> (&count), data.data() + offset, sizeof(count));
		// Выполняем смещение в буфере
		offset += sizeof(count);
		// Выполняем сброс заголовков чёрного списка
		this->_black.clear();
		// Если количество элементов получено
		if(count > 0){
			// Выполняем последовательную загрузку всех заголовков
			for(size_t i = 0; i < count; i++){
				// Выполняем получение размера заголовка из чёрного списка
				::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
				// Выполняем смещение в буфере
				offset += sizeof(length);
				// Если размер получен
				if(length > 0){
					// Выделяем память для заголовка чёрного списка
					string header(length, 0);
					// Выполняем получение заголовка чёрного списка
					::memcpy(reinterpret_cast <void *> (header.data()), data.data() + offset, length);
					// Выполняем смещение в буфере
					offset += length;
					// Если заголовок чёрного списка получен
					if(!header.empty())
						// Выполняем добавление заголовка чёрного списка
						this->_black.emplace(std::move(header));
				}
			}
		}
		// Выполняем получение размера дампа WEB данных
		::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Если размер получен
		if(length > 0){
			// Выделяем память для дампа WEB данных
			vector <char> buffer(length, 0);
			// Выполняем получение дампа WEB данных
			::memcpy(reinterpret_cast <void *> (buffer.data()), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
			// Если дамп Web-данных получен, устанавливаем его
			if(!buffer.empty())
				// Выполняем установку буфера модуля Web
				this->_web.dump(std::move(buffer));
		}
	}
}
/**
 * is Метод проверки активного состояния
 * @param state состояние которое необходимо проверить
 */
bool awh::Http::is(const state_t state) const noexcept {
	// Определяем запрашиваемое состояние
	switch(static_cast <uint8_t> (state)){
		// Если проверяется режим завершения сбора данных
		case static_cast <uint8_t> (state_t::END):
			// Выводрим результат проверки
			return (
				(this->_state == state_t::GOOD) ||
				(this->_state == state_t::BROKEN) ||
				(this->_state == state_t::HANDSHAKE)
			);
		// Если проверяется режим удачного выполнения запроса
		case static_cast <uint8_t> (state_t::GOOD):
			// Выводрим результат проверки
			return (this->_state == state_t::GOOD);
		// Если проверяется режим уставновки постоянного подключения
		case static_cast <uint8_t> (state_t::ALIVE): {
			// Определяем идентичность сервера
			switch(static_cast <uint8_t> (this->_identity)){
				// Если сервер соответствует WebSocket-серверу
				case static_cast <uint8_t> (identity_t::WS):
				// Если сервер соответствует HTTP-серверу
				case static_cast <uint8_t> (identity_t::HTTP): {
					// Запрашиваем заголовок подключения
					const string & header = this->_web.header("connection");
					// Если заголовок подключения найден
					if(!header.empty())
						// Выполняем проверку является ли соединение закрытым
						return !this->_fmk->exists("close", header);
					// Если заголовок подключения не найден
					else {
						// Переходим по всему списку заголовков
						for(auto & header : this->_web.headers()){
							// Если заголовок найден
							if(this->_fmk->compare(header.first, "connection"))
								// Выполняем проверку является ли соединение закрытым
								return !this->_fmk->exists("close", header.second);
						}
					}
				} break;
				// Если сервер соответствует PROXY-серверу
				case static_cast <uint8_t> (identity_t::PROXY): {
					// Запрашиваем заголовок подключения
					const string & header = this->_web.header("proxy-connection");
					// Если заголовок подключения найден
					if(!header.empty())
						// Выполняем проверку является ли соединение закрытым
						return !this->_fmk->exists("close", header);
					// Если заголовок подключения не найден
					else {
						// Переходим по всему списку заголовков
						for(auto & header : this->_web.headers()){
							// Если заголовок найден
							if(this->_fmk->compare(header.first, "proxy-connection"))
								// Выполняем проверку является ли соединение закрытым
								return !this->_fmk->exists("close", header.second);
						}
					}
				} break;
			}
			// Сообщаем, что подключение постоянное
			return true;
		}
		// Если проверяется режим бракованных данных
		case static_cast <uint8_t> (state_t::BROKEN):
			// Выводрим результат проверки
			return (this->_state == state_t::BROKEN);
		// Если проверяется режим выполненного рукопожатия
		case static_cast <uint8_t> (state_t::HANDSHAKE):
			// Выполняем проверку на удачное рукопожатие
			return (this->_state == state_t::HANDSHAKE);
		// Если проверяется режим флага запраса трейлеров
		case static_cast <uint8_t> (state_t::TRAILERS):
			// Выводим проверку на установку флага запроса передачи трейлеров
			return this->_te.trailers;
	}
	// Выводим результат
	return false;
}
/**
 * is Метод проверки существования заголовка
 * @param suite тип набора к которому соответствует заголовок
 * @param key   ключ заголовка для проверки
 * @return      результат проверки
 */
bool awh::Http::is(const suite_t suite, const string & key) const noexcept {
	// Если ключ заголовка передан
	if(!key.empty()){
		// Определяем запрашиваемый набор к которому принадлежит заголовок
		switch(static_cast <uint8_t> (suite)){
			// Если набор соответствует заголовку чёрного списка
			case static_cast <uint8_t> (suite_t::BLACK):
				// Выполняем проверку наличия заголовка в чёрном списке
				return (this->_black.find(this->_fmk->transform(key, fmk_t::transform_t::LOWER)) != this->_black.end());
			// Если набор соответствует заголовку сообщения
			case static_cast <uint8_t> (suite_t::HEADER):
				// Выводим результат проверки
				return this->_web.isHeader(key);
			// Если набор соответствует стандартному заголовку
			case static_cast <uint8_t> (suite_t::STANDARD):
				// Выводим результат проверки
				return this->_web.isStandard(key);
		}
	}
	// Выводим результат
	return false;
}
/**
 * rm Метод удаления установленных заголовков
 * @param suite тип набора к которому соответствует заголовок
 * @param key   ключ заголовка для удаления
 */
void awh::Http::rm(const suite_t suite, const string & key) const noexcept {
	// Если ключ заголовка передан
	if(!key.empty()){
		// Определяем запрашиваемый набор к которому принадлежит заголовок
		switch(static_cast <uint8_t> (suite)){
			// Если набор соответствует заголовку чёрного списка
			case static_cast <uint8_t> (suite_t::BLACK):
				// Выполняем удаление заголовка из чёрного списка
				this->_black.erase(this->_fmk->transform(key, fmk_t::transform_t::LOWER));
			break;
			// Если набор соответствует заголовку сообщения
			case static_cast <uint8_t> (suite_t::HEADER):
				// Выполняем удаление заголовка
				this->_web.delHeader(key);
			break;
		}
	}
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
 * date Метод получения текущей даты для HTTP-запроса
 * @param stamp штамп времени в числовом виде
 * @return      штамп времени в текстовом виде
 */
const string awh::Http::date(const time_t stamp) const noexcept {
	// Создаём буфер данных
	char buffer[1000];
	// Получаем текущее время
	time_t date = (stamp > 0 ? stamp : ::time(nullptr));
	// Извлекаем текущее время
	struct tm tm = (* ::gmtime(&date));
	// Зануляем буфер
	::memset(buffer, 0, sizeof(buffer));
	// Получаем формат времени
	::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", &tm);
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
 * mapping Метод маппинга полученных данных
 * @param flag флаг выполняемого процесса
 * @param http объект для маппинга
 */
void awh::Http::mapping(const process_t flag, Http & http) noexcept {
	// Выполняем очистку списка заголовков
	http.clear(suite_t::HEADER);
	// Устанавливаем идентификатор объекта
	http.id(this->_web.id());
	// Устанавливаем размер одного чанка
	http.chunk(this->_chunk);
	// Устанавливаем параметры Transfer-Encoding
	http._te = this->_te;
	// Устанавливаем активный стейт объекта
	http._state = this->_state;
	// Устанавливаем тип статуса авторизации
	http._status = this->_status;
	// Устанавливаем флаг зашифрованной полезной нагрузки
	http._crypted = this->_crypted;
	// Устанавливаем флаг шифрования объекта
	http._encryption = this->_encryption;
	// Устанавливаем флаг компрессии полезной нагрузки
	http._compressor.current = this->_compressor.current;
	// Устанавливаем выбранный метод компрессии
	http._compressor.selected = this->_compressor.selected;
	// Выполняем установку списка поддерживаемых компрессоров
	http._compressor.supports = this->_compressor.supports;
	// Извлекаем список заголовков
	const auto & headers = this->_web.headers();
	// Если заголовки получены, выполняем установку
	if(!headers.empty())
		// Выполняем установку заголовков
		http.headers(headers);
	// Устанавливаем параметры идентификации сервиса
	http.ident(this->_ident.id, this->_ident.name, this->_ident.version);
	// Если нужно сформировать данные запроса
	if(flag == process_t::REQUEST){
		// Устанавливаем User-Agent, если он установлен
		http.userAgent(this->_userAgent);
		// Получаем ответ с удалённого сервера
		const web_t::res_t & response = this->_web.response();
		// Если авторизация установлена как Digest
		if(this->_auth.client.type() == awh::auth_t::type_t::DIGEST){
			// Проверяем код ответа
			switch(response.code){
				// Если требуется авторизация для сервера
				case 401: {
					// Получаем параметры авторизации
					const string & auth = this->_web.header("www-authenticate");
					// Если параметры авторизации найдены
					if(!auth.empty())
						// Устанавливаем заголовок HTTP в параметры авторизации
						http._auth.client.header(auth);
				} break;
				// Если требуется авторизация для прокси-сервера
				case 407: {
					// Получаем параметры авторизации
					const string & auth = this->_web.header("proxy-authenticate");
					// Если параметры авторизации найдены
					if(!auth.empty())
						// Устанавливаем заголовок HTTP в параметры авторизации
						http._auth.client.header(auth);
				} break;
			}
		}
		// Проверяем код ответа
		switch(response.code){
			// Если нужно произвести редирект
			case 201:
			case 301:
			case 302:
			case 303:
			case 307:
			case 308: {
				// Получаем параметры переадресации
				const string & location = this->_web.header("location");
				// Если адрес перенаправления найден
				if(!location.empty()){
					// Получаем объект параметров запроса
					web_t::req_t request = this->_web.request();
					// Выполняем парсинг полученного URL-адреса
					request.url = this->_uri.parse(location);
					// Выполняем установку параметров запроса
					http._web.request(std::move(request));
				}
			} break;
		}
	}
}
/**
 * trailer Метод получения буфера отправляемого трейлера
 * @return буфер данных ответа в бинарном виде
 */
vector <char> awh::Http::trailer() const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если разрешено добавление трейлеров
	if(this->_te.trailers){
		// Если список трейлеров получен
		if(!this->_trailers.empty()){
			// Получаем первый трейлер из списка
			auto it = this->_trailers.begin();
			// Получаем название заголовка
			const string name = it->first;
			// Переводим заголовок в нормальный режим
			this->_fmk->transform(name, fmk_t::transform_t::SMART);
			// Сформированный отправляемый ответ
			string response = this->_fmk->format("%s: %s\r\n", name.c_str(), it->second.c_str());
			// Выполняем удаление отправляемого трейлера из списка
			const_cast <http_t *> (this)->_trailers.erase(it);
			// Если трейлеров в списке больше нет
			if(this->_trailers.empty())
				// Выполняем добавление конца запроса
				response.append("\r\n");
			// Устанавливаем результат
			result.assign(response.begin(), response.end());
		}
	}
	// Выводим результат
	return result;
}
/**
 * trailers2 Метод получения буфера отправляемых трейлеров (для протокола HTTP/2)
 * @return буфер данных ответа в бинарном виде
 */
vector <pair <string, string>> awh::Http::trailers2() const noexcept {
	// Результат работы функции
	vector <pair <string, string>> result;
	// Если разрешено добавление трейлеров
	if(this->_te.trailers){
		// Если список трейлеров получен
		if(!this->_trailers.empty()){
			// Переходим по всему списку доступных трейлеров
			for(auto it = this->_trailers.begin(); it != this->_trailers.end();){
				// Устанавливаем трейлер в список для отправки
				result.push_back(make_pair(it->first, it->second));
				// Выполняем удаление отправляемого трейлера из списка
				it = const_cast <http_t *> (this)->_trailers.erase(it);
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
		const_cast <http_t *> (this)->black("Accept");
		// Добавляем в чёрный список заголовок Accept-Language
		const_cast <http_t *> (this)->black("Accept-Language");
		// Добавляем в чёрный список заголовок Accept-Encoding
		const_cast <http_t *> (this)->black("Accept-Encoding");
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
		return this->process(process_t::REQUEST, dynamic_cast <const web_t::provider_t &> (req));
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
		const_cast <http_t *> (this)->black("Accept");
		// Добавляем в чёрный список заголовок Accept-Language
		const_cast <http_t *> (this)->black("Accept-Language");
		// Добавляем в чёрный список заголовок Accept-Encoding
		const_cast <http_t *> (this)->black("Accept-Encoding");
		// Добавляем заголовок протокола подключения
		const_cast <http_t *> (this)->header(":protocol", "proxy");
		// Устанавливаем параметры REST-запроса
		this->_auth.client.uri(this->_uri.url(req.url));
		// Устанавливаем парарметр запроса
		this->_web.request(req);
		// Выполняем создание запроса
		return this->process2(process_t::REQUEST, dynamic_cast <const web_t::provider_t &> (req));
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
		// Выполняем очистку списка установленных заголовков
		this->_web.clearHeaders();
		// Определяем код ответа авторизационных данных
		switch(res.code){
			// Если код ответа соответствует авторизации на HTTP-сервере
			case 401:
				// Добавляем заголовок постоянного подключения
				this->_web.header("Connection", "Keep-Alive");
			break;
			// Если код ответа соответствует авторизации на PROXY-сервере
			case 407: {
				// Добавляем заголовок постоянного подключения на HTTP-сервере
				this->_web.header("Connection", "Keep-Alive");
				// Добавляем заголовок постоянного подключения на PROXY-сервере
				this->_web.header("Proxy-Connection", "Keep-Alive");
			} break;
			// Для всех остальных кодов ответа
			default: {
				// Определяем идентичность сервера
				switch(static_cast <uint8_t> (this->_identity)){
					// Если сервер соответствует WebSocket-серверу
					case static_cast <uint8_t> (identity_t::WS):
					// Если сервер соответствует HTTP-серверу
					case static_cast <uint8_t> (identity_t::HTTP):
						// Добавляем заголовок закрытия подключения
						this->_web.header("Connection", "Close");
					break;
					// Если сервер соответствует PROXY-серверу
					case static_cast <uint8_t> (identity_t::PROXY): {
						// Добавляем заголовок закрытия подключения на HTTP-сервере
						this->_web.header("Connection", "Close");
						// Добавляем заголовок закрытия подключения на PROXY-сервере
						this->_web.header("Proxy-Connection", "Close");
					} break;
				}
			}
		}
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
				const_cast <http_t *> (this)->clear(suite_t::BODY);
				// Добавляем тело сообщения
				const_cast <http_t *> (this)->body(vector <char> (body.begin(), body.end()));
			}
			// Добавляем заголовок тела сообщения
			this->_web.header("Content-Length", ::to_string(body.size()));
		}
		// Устанавливаем парарметр ответа
		this->_web.response(res);
		// Выводим результат
		return this->process(process_t::RESPONSE, dynamic_cast <const web_t::provider_t &> (res));
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
		// Выполняем очистку списка установленных заголовков
		this->_web.clearHeaders();
		// Определяем код ответа авторизационных данных
		switch(res.code){
			// Если код ответа соответствует авторизации на HTTP-сервере
			case 401:
				// Добавляем заголовок постоянного подключения
				this->_web.header("connection", "keep-alive");
			break;
			// Если код ответа соответствует авторизации на PROXY-сервере
			case 407: {
				// Добавляем заголовок постоянного подключения на HTTP-сервере
				this->_web.header("connection", "keep-alive");
				// Добавляем заголовок постоянного подключения на PROXY-сервере
				this->_web.header("proxy-connection", "keep-alive");
			} break;
			// Для всех остальных кодов ответа
			default: {
				// Определяем идентичность сервера
				switch(static_cast <uint8_t> (this->_identity)){
					// Если сервер соответствует WebSocket-серверу
					case static_cast <uint8_t> (identity_t::WS):
					// Если сервер соответствует HTTP-серверу
					case static_cast <uint8_t> (identity_t::HTTP):
						// Добавляем заголовок закрытия подключения
						this->_web.header("connection", "close");
					break;
					// Если сервер соответствует PROXY-серверу
					case static_cast <uint8_t> (identity_t::PROXY): {
						// Добавляем заголовок закрытия подключения на HTTP-сервере
						this->_web.header("connection", "close");
						// Добавляем заголовок закрытия подключения на PROXY-сервере
						this->_web.header("proxy-connection", "close");
					} break;
				}
			}
		}
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
				const_cast <http_t *> (this)->clear(suite_t::BODY);
				// Добавляем тело сообщения
				const_cast <http_t *> (this)->body(vector <char> (body.begin(), body.end()));
			}
		}
		// Устанавливаем парарметр ответа
		this->_web.response(res);
		// Выводим результат
		return this->process2(process_t::RESPONSE, dynamic_cast <const web_t::provider_t &> (res));
	}
	// Выводим результат
	return vector <pair <string, string>> ();
}
/**
 * process Метод создания выполняемого процесса в бинарном виде
 * @param flag флаг выполняемого процесса
 * @param prov параметры провайдера обмена сообщениями
 * @return     буфер данных в бинарном виде
 */
vector <char> awh::Http::process(const process_t flag, const web_t::provider_t & prov) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Определяем флаг выполняемого процесса
	switch(static_cast <uint8_t> (flag)){
		// Если нужно сформировать данные запроса
		case static_cast <uint8_t> (process_t::REQUEST): {
			// Получаем объект ответа клиенту
			const web_t::req_t & req = static_cast <const web_t::req_t &> (prov);
			// Если параметры REST-запроса переданы
			if(!req.url.empty() && (req.method != web_t::method_t::NONE)){
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
				// Определяем тип HTTP-модуля
				switch(static_cast <uint8_t> (this->_web.hid())){
					// Если мы работаем с клиентом
					case static_cast <uint8_t> (web_t::hid_t::CLIENT): {
						/**
						 * Типы основных заголовков
						 */
						bool available[14] = {
							false, // te
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
						// Устанавливаем парарметры запроса
						this->_web.request(req);
						// Устанавливаем параметры REST-запроса
						this->_auth.client.uri(this->_uri.url(req.url));
						// Переходим по всему списку заголовков
						for(auto & header : this->_web.headers()){
							// Флаг разрешающий вывода заголовка
							bool allow = !this->is(suite_t::BLACK, header.first);
							// Выполняем перебор всех обязательных заголовков
							for(uint8_t i = 0; i < 14; i++){
								// Если заголовок уже найден пропускаем его
								if(available[i])
									// Продолжаем поиск дальше
									continue;
								// Выполняем првоерку заголовка
								switch(i){
									case 0:  available[i] = this->_fmk->compare(header.first, "te");                    break;
									case 1:  available[i] = this->_fmk->compare(header.first, "host");                  break;
									case 2:  available[i] = this->_fmk->compare(header.first, "accept");                break;
									case 3:  available[i] = this->_fmk->compare(header.first, "origin");                break;
									case 4:  available[i] = this->_fmk->compare(header.first, "user-agent");            break;
									case 5:  available[i] = this->_fmk->compare(header.first, "connection");            break;
									case 6:  available[i] = this->_fmk->compare(header.first, "proxy-connection");      break;
									case 8:  available[i] = this->_fmk->compare(header.first, "accept-language");       break;
									case 9:  available[i] = this->_fmk->compare(header.first, "accept-encoding");       break;
									case 10: available[i] = this->_fmk->compare(header.first, "content-encoding");      break;
									case 11: available[i] = this->_fmk->compare(header.first, "transfer-encoding");     break;
									case 12: available[i] = this->_fmk->compare(header.first, "x-awh-encryption");      break;
									case 13: available[i] = (this->_fmk->compare(header.first, "authorization") ||
															 this->_fmk->compare(header.first, "proxy-authorization")); break;
									case 7: {
										// Запоминаем, что мы нашли заголовок размера тела
										available[i] = this->_fmk->compare(header.first, "content-length");
										// Устанавливаем размер тела сообщения
										if(available[i])
											// Устанавливаем длину передаваемого текста
											length = static_cast <size_t> (::stoull(header.second));
									} break;
								}
								// Если заголовок разрешён для вывода
								if(allow){
									// Выполняем првоерку заголовка
									switch(i){
										case 0:
										case 5:
										case 6:
										case 7:
										case 10:
										case 11:
										case 12: allow = !available[i]; break;
									}
								}
							}
							// Если заголовок не является запрещённым, добавляем заголовок в запрос
							if(allow){
								// Получаем название заголовка
								string name = header.first;
								// Переводим заголовок в нормальный режим
								this->_fmk->transform(name, fmk_t::transform_t::SMART);
								// Формируем строку запроса
								request.append(this->_fmk->format("%s: %s\r\n", name.c_str(), header.second.c_str()));
							}
						}
						// Устанавливаем Host если не передан и метод подключения не является CONNECT
						if(!available[1] && !this->is(suite_t::BLACK, "Host") && (req.method != web_t::method_t::CONNECT)){
							// Если флаг точной установки хоста не установлен
							if(!this->_precise)
								// Добавляем заголовок в запрос
								request.append(this->_fmk->format("Host: %s\r\n", req.url.host.c_str()));
							// Добавляем заголовок в запрос
							else request.append(this->_fmk->format("Host: %s:%u\r\n", req.url.host.c_str(), req.url.port));
						}
						// Устанавливаем Accept если не передан
						if(!available[2] && (req.method != web_t::method_t::CONNECT) && !this->is(suite_t::BLACK, "Accept"))
							// Добавляем заголовок в запрос
							request.append(this->_fmk->format("Accept: %s\r\n", HTTP_HEADER_ACCEPT));
						// Устанавливаем Connection если не передан
						if(!available[5] && !this->is(suite_t::BLACK, "Connection")){
							// Если нужно вставить заголовок TE и он не находится в чёрном списке
							if(available[0] && !this->is(suite_t::BLACK, "TE"))
								// Добавляем заголовок в запрос
								request.append(this->_fmk->format("Connection: TE, %s\r\n", HTTP_HEADER_CONNECTION));
							// Добавляем заголовок в запрос
							else request.append(this->_fmk->format("Connection: %s\r\n", HTTP_HEADER_CONNECTION));
						// Если заголовок Connection уже передан и не находится в чёрном списке
						} else if(!this->is(suite_t::BLACK, "Connection")) {
							// Поулчаем заголовок Connection
							const string & header = this->_web.header("Connection");
							// Если нужно вставить заголовок TE и он не находится в чёрном списке
							if(available[0] && !this->is(suite_t::BLACK, "TE") && !this->_fmk->exists("TE", header))
								// Добавляем заголовок в запрос
								request.append(this->_fmk->format("Connection: TE, %s\r\n", header.c_str()));
							// Добавляем заголовок в запрос
							else request.append(this->_fmk->format("Connection: %s\r\n", header.c_str()));
						}
						// Устанавливаем Proxy-Connection если не передан
						if(!available[6] && !this->is(suite_t::BLACK, "Proxy-Connection")){
							// Если сервер соответствует PROXY-серверу
							if(this->_identity == identity_t::PROXY)
								// Добавляем заголовок в запрос
								request.append(this->_fmk->format("Proxy-Connection: %s\r\n", HTTP_HEADER_CONNECTION));
						// Если заголовок Proxy-Connection уже передан и не находится в чёрном списке
						} else if(!this->is(suite_t::BLACK, "Proxy-Connection")) {
							// Если сервер соответствует PROXY-серверу
							if(this->_identity == identity_t::PROXY){
								// Поулчаем заголовок Proxy-Connection
								const string & header = this->_web.header("Proxy-Connection");
								// Добавляем заголовок в запрос
								request.append(this->_fmk->format("Proxy-Connection: %s\r\n", header.c_str()));
							}
						}
						// Устанавливаем Accept-Language если не передан
						if(!available[8] && (req.method != web_t::method_t::CONNECT) && !this->is(suite_t::BLACK, "Accept-Language"))
							// Добавляем заголовок в запрос
							request.append(this->_fmk->format("Accept-Language: %s\r\n", HTTP_HEADER_ACCEPTLANGUAGE));
						// Если нужно запросить компрессию в удобном нам виде
						if(!available[9] && (req.method != web_t::method_t::CONNECT) && (!this->_compressor.supports.empty() || (this->_compressor.selected != compress_t::NONE)) && !this->is(suite_t::BLACK, "Accept-Encoding")){
							// Если компрессор уже выбран
							if(this->_compressor.selected != compress_t::NONE){
								// Определяем метод сжатия который поддерживает клиент
								switch(static_cast <uint8_t> (this->_compressor.selected)){
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
								}
							// Если список компрессоров установлен
							} else if(!this->_compressor.supports.empty()) {
								// Строка со списком компрессоров
								string compressors = "";
								// Выполняем перебор всего списка компрессоров
								for(auto it = this->_compressor.supports.rbegin(); it != this->_compressor.supports.rend(); ++it){
									// Если список компрессоров уже не пустой
									if(!compressors.empty())
										// Выполняем добавление разделителя
										compressors.append(", ");
									// Определяем метод сжатия который поддерживает клиент
									switch(static_cast <uint8_t> (it->second)){
										// Если клиент поддерживает методот сжатия BROTLI
										case static_cast <uint8_t> (compress_t::BROTLI):
											// Добавляем компрессор в список
											compressors.append("br");
										break;
										// Если клиент поддерживает методот сжатия GZIP
										case static_cast <uint8_t> (compress_t::GZIP):
											// Добавляем компрессор в список
											compressors.append("gzip");
										break;
										// Если клиент поддерживает методот сжатия DEFLATE
										case static_cast <uint8_t> (compress_t::DEFLATE):
											// Добавляем компрессор в список
											compressors.append("deflate");
										break;
									}
								}
								// Если список компрессоров получен
								if(!compressors.empty())
									// Добавляем заголовок в запрос
									request.append(this->_fmk->format("Accept-Encoding: %s\r\n", compressors.c_str()));
							}
						}
						// Устанавливаем User-Agent если не передан
						if(!available[4] && !this->is(suite_t::BLACK, "User-Agent")){
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
								// Выполняем генерацию Юзер-агента клиента выполняющего HTTP-запрос
								this->_userAgent = this->_fmk->format("%s (%s; %s/%s)", this->_ident.name.c_str(), os, this->_ident.id.c_str(), this->_ident.version.c_str());
							}
							// Добавляем заголовок в запрос
							request.append(this->_fmk->format("User-Agent: %s\r\n", this->_userAgent.c_str()));
						}
						// Если заголовок авторизации не передан
						if(!available[13]){
							// Метод HTTP-запроса
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
									if(!this->is(suite_t::BLACK, "Proxy-Authorization")){
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
									if(!this->is(suite_t::BLACK, "Authorization")){
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
						// Если нужно вставить заголовок TE и он не находится в чёрном списке
						if(available[0] && !this->is(suite_t::BLACK, "TE")){
							// Получаем список доступных заголовков
							const auto & headers = this->_web.headers();
							// Если список заголовков получен
							if(!headers.empty()){
								// Строка отправляемого заголовка
								string header = "";
								// Выполняем извлечение списка нужных заголовков
								const auto & range = headers.equal_range("te");
								// Выполняем перебор всего списка указанных заголовков
								for(auto it = range.first; it != range.second; ++it){
									// Если заголовок уже собран
									if(!header.empty())
										// Добавляем разделитель
										header.append(", ");
									// Добавляем заголовок в список
									header.append(this->_fmk->transform(it->second, fmk_t::transform_t::LOWER));
								}
								// Если заголовок собран
								if(!header.empty())
									// Добавляем заголовок параметров Transfer-Encoding в запрос
									request.append(this->_fmk->format("TE: %s\r\n", header.c_str()));
							}
						}
						// Если запрос является PUT, POST, PATCH
						if((req.method == web_t::method_t::PUT) || (req.method == web_t::method_t::POST) || (req.method == web_t::method_t::PATCH)){
							// Если заголовок не запрещён
							if(!this->is(suite_t::BLACK, "Date"))
								// Добавляем заголовок даты в запрос
								request.append(this->_fmk->format("Date: %s\r\n", this->date().c_str()));
							// Если тело запроса существует
							if(!this->_web.body().empty()){
								// Выполняем компрессию полезной нагрузки
								const_cast <http_t *> (this)->compress();
								// Выполняем шифрование полезной нагрузки
								const_cast <http_t *> (this)->encrypt();
								// Проверяем нужно ли передать тело разбив на чанки
								this->_te.chunking = (this->_crypted || (this->_compressor.current != compress_t::NONE));
								// Заменяем размер тела данных
								if(!this->_te.chunking)
									// Устанавливаем размер тела сообщения
									length = this->_web.body().size();
								// Если данные зашифрованы, устанавливаем соответствующие заголовки
								if(this->_crypted)
									// Устанавливаем X-AWH-Encryption
									request.append(this->_fmk->format("X-AWH-Encryption: %u\r\n", static_cast <u_short> (this->_hash.cipher())));
								// Определяем метод компрессии полезной нагрузки
								switch(static_cast <uint8_t> (this->_compressor.current)){
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
								// Если данные необходимо разбивать на чанки
								if(this->_te.chunking && !this->is(suite_t::BLACK, "Transfer-Encoding"))
									// Устанавливаем заголовок Transfer-Encoding
									request.append(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
								// Если заголовок размера передаваемого тела, не запрещён
								else if(!this->is(suite_t::BLACK, "Content-Length") && ((length > 0) || this->_web.isHeader("Content-Length")))
									// Устанавливаем размер передаваемого тела Content-Length
									request.append(this->_fmk->format("Content-Length: %zu\r\n", length));
							// Если тело запроса не существует
							} else {
								// Проверяем нужно ли передать тело разбив на чанки
								this->_te.chunking = (this->_encryption || (this->_compressor.selected != compress_t::NONE));
								// Если данные зашифрованы, устанавливаем соответствующие заголовки
								if(this->_encryption && !this->is(suite_t::BLACK, "X-AWH-Encryption"))
									// Устанавливаем X-AWH-Encryption
									request.append(this->_fmk->format("X-AWH-Encryption: %u\r\n", static_cast <u_short> (this->_hash.cipher())));
								// Устанавливаем Content-Encoding если не передан
								if(!this->is(suite_t::BLACK, "Content-Encoding")){
									// Определяем метод компрессии полезной нагрузки
									switch(static_cast <uint8_t> (this->_compressor.selected)){
										// Если полезная нагрузка сжата методом BROTLI
										case static_cast <uint8_t> (compress_t::BROTLI):
											// Устанавливаем Content-Encoding если не передан
											request.append(this->_fmk->format("Content-Encoding: %s\r\n", "br"));
										break;
										// Если полезная нагрузка сжата методом GZIP
										case static_cast <uint8_t> (compress_t::GZIP):
											// Устанавливаем Content-Encoding если не передан
											request.append(this->_fmk->format("Content-Encoding: %s\r\n", "gzip"));
										break;
										// Если полезная нагрузка сжата методом DEFLATE
										case static_cast <uint8_t> (compress_t::DEFLATE):
											// Устанавливаем Content-Encoding если не передан
											request.append(this->_fmk->format("Content-Encoding: %s\r\n", "deflate"));
										break;
									}
								}
								// Если данные необходимо разбивать на чанки
								if(this->_te.chunking && !this->is(suite_t::BLACK, "Transfer-Encoding") && this->_web.isHeader("Transfer-Encoding"))
									// Устанавливаем заголовок Transfer-Encoding
									request.append(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
								// Если заголовок размера передаваемого тела, не запрещён
								else if(!this->is(suite_t::BLACK, "Content-Length") && ((length > 0) || this->_web.isHeader("Content-Length")))
									// Устанавливаем размер передаваемого тела Content-Length
									request.append(this->_fmk->format("Content-Length: %zu\r\n", length));
							}
						// Если запрос не содержит тела запроса
						} else {
							// Если данные зашифрованы, устанавливаем соответствующие заголовки
							if((this->_te.chunking = (this->_encryption && !this->is(suite_t::BLACK, "X-AWH-Encryption"))))
								// Устанавливаем X-AWH-Encryption
								request.append(this->_fmk->format("X-AWH-Encryption: %u\r\n", static_cast <u_short> (this->_hash.cipher())));
							// Устанавливаем Content-Encoding если заголовок есть в запросе
							if(available[10] && !this->is(suite_t::BLACK, "Content-Encoding")){
								// Определяем метод компрессии полезной нагрузки
								switch(static_cast <uint8_t> (this->_compressor.selected)){
									// Если полезная нагрузка сжата методом BROTLI
									case static_cast <uint8_t> (compress_t::BROTLI):
										// Устанавливаем Content-Encoding если не передан
										request.append(this->_fmk->format("Content-Encoding: %s\r\n", "br"));
									break;
									// Если полезная нагрузка сжата методом GZIP
									case static_cast <uint8_t> (compress_t::GZIP):
										// Устанавливаем Content-Encoding если не передан
										request.append(this->_fmk->format("Content-Encoding: %s\r\n", "gzip"));
									break;
									// Если полезная нагрузка сжата методом DEFLATE
									case static_cast <uint8_t> (compress_t::DEFLATE):
										// Устанавливаем Content-Encoding если не передан
										request.append(this->_fmk->format("Content-Encoding: %s\r\n", "deflate"));
									break;
								}
								// Проверяем нужно ли передать тело разбив на чанки
								this->_te.chunking = (this->_compressor.selected != compress_t::NONE);
							}
							// Очищаем тела сообщения
							const_cast <http_t *> (this)->clear(suite_t::BODY);
						}
					} break;
					// Если мы работаем с сервером
					case static_cast <uint8_t> (web_t::hid_t::SERVER): {
						// Название заголовка
						string name = "";
						// Переходим по всему списку заголовков
						for(auto & header : this->_web.headers()){
							// Устанавливаем название заголовка
							name = header.first;
							// Переводим заголовок в нормальный режим
							this->_fmk->transform(name, fmk_t::transform_t::SMART);
							// Формируем строку запроса
							request.append(this->_fmk->format("%s: %s\r\n", name.c_str(), header.second.c_str()));
						}
					} break;
				}
				// Устанавливаем завершающий разделитель
				request.append("\r\n");
				// Формируем результат запроса
				result.assign(request.begin(), request.end());
			}
		} break;
		// Если нужно сформировать данные ответа
		case static_cast <uint8_t> (process_t::RESPONSE): {
			// Получаем объект ответа клиенту
			const web_t::res_t & res = static_cast <const web_t::res_t &> (prov);
			// Если текст сообщения не установлен
			if(res.message.empty())
				// Выполняем установку сообщения
				const_cast <web_t::res_t &> (res).message = this->message(res.code);
			// Если сообщение получено
			if(!res.message.empty()){
				// Данные REST-ответа
				string response = this->_fmk->format("HTTP/%.1f %u %s\r\n", res.version, res.code, res.message.c_str());
				// Определяем тип HTTP-модуля
				switch(static_cast <uint8_t> (this->_web.hid())){
					// Если мы работаем с клиентом
					case static_cast <uint8_t> (web_t::hid_t::CLIENT): {
						// Название заголовка
						string name = "";
						// Переходим по всему списку заголовков
						for(auto & header : this->_web.headers()){
							// Устанавливаем название заголовка
							name = header.first;
							// Переводим заголовок в нормальный режим
							this->_fmk->transform(name, fmk_t::transform_t::SMART);
							// Формируем строку ответа
							response.append(this->_fmk->format("%s: %s\r\n", name.c_str(), header.second.c_str()));
						}
					} break;
					// Если мы работаем с сервером
					case static_cast <uint8_t> (web_t::hid_t::SERVER): {
						/**
						 * Типы основных заголовков
						 */
						bool available[12] = {
							false, // Date
							false, // Server
							false, // Connection
							false, // Proxy-Connection
							false, // X-Powered-By
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
						// Устанавливаем парарметры ответа
						this->_web.response(res);
						// Переходим по всему списку заголовков
						for(auto & header : this->_web.headers()){
							// Флаг разрешающий вывода заголовка
							bool allow = !this->is(suite_t::BLACK, header.first);
							// Выполняем перебор всех обязательных заголовков
							for(uint8_t i = 0; i < 12; i++){
								// Если заголовок уже найден пропускаем его
								if(available[i])
									// Продолжаем поиск дальше
									continue;
								// Выполняем првоерку заголовка
								switch(i){
									case 0:  available[i] = this->_fmk->compare(header.first, "date");               break;
									case 1:  available[i] = this->_fmk->compare(header.first, "server");             break;
									case 2:  available[i] = this->_fmk->compare(header.first, "connection");         break;
									case 3:  available[i] = this->_fmk->compare(header.first, "proxy-connection");   break;
									case 4:  available[i] = this->_fmk->compare(header.first, "x-powered-by");       break;
									case 5:  available[i] = this->_fmk->compare(header.first, "content-type");       break;
									case 7:  available[i] = this->_fmk->compare(header.first, "content-encoding");   break;
									case 8:  available[i] = this->_fmk->compare(header.first, "transfer-encoding");  break;
									case 9:  available[i] = this->_fmk->compare(header.first, "x-awh-encryption");   break;
									case 10: available[i] = this->_fmk->compare(header.first, "www-authenticate");   break;
									case 11: available[i] = this->_fmk->compare(header.first, "proxy-authenticate"); break;
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
										case 7:
										case 8:
										case 9: allow = !available[i]; break;
									}
									// Если ответ является информационным
									if((((res.code >= 100) && (res.code < 200)) || (res.code == 204)) && available[i]){
										// Запрещяем указанным заголовкам формирование
										switch(i){
											case 0:
											case 5:
											case 6:
											case 7:
											case 8:
											case 9:
											case 10:
											case 11: allow = false; break;
										}
									}
								}
							}
							// Если заголовок не является запрещённым, добавляем заголовок в ответ
							if(allow){
								// Получаем название заголовка
								string name = header.first;
								// Переводим заголовок в нормальный режим
								this->_fmk->transform(name, fmk_t::transform_t::SMART);
								// Формируем строку ответа
								response.append(this->_fmk->format("%s: %s\r\n", name.c_str(), header.second.c_str()));
							}
						}
						// Если заголовок не запрещён
						if(!available[1] && !this->is(suite_t::BLACK, "Server"))
							// Добавляем название сервера в ответ
							response.append(this->_fmk->format("Server: %s\r\n", this->_ident.name.c_str()));
						// Устанавливаем Connection если не передан
						if(!available[2] && !this->is(suite_t::BLACK, "Connection"))
							// Добавляем заголовок в ответ
							response.append(this->_fmk->format("Connection: %s\r\n", HTTP_HEADER_CONNECTION));
						// Устанавливаем Proxy-Connection если не передан
						if(!available[3] && !this->is(suite_t::BLACK, "Proxy-Connection")){
							// Если клиент соответствует PROXY-клиенту
							if(this->_identity == identity_t::PROXY)
								// Добавляем заголовок в ответ
								response.append(this->_fmk->format("Proxy-Connection: %s\r\n", HTTP_HEADER_CONNECTION));
						}
						/*
						// Устанавливаем Content-Type если не передан
						if(!available[5] && (res.code >= 200) && !this->is(suite_t::BLACK, "Content-Type"))
							// Добавляем заголовок в ответ
							response.append(this->_fmk->format("Content-Type: %s\r\n", HTTP_HEADER_CONTENTTYPE));
						*/
						// Если заголовок не запрещён
						if(!available[4] && !this->is(suite_t::BLACK, "X-Powered-By"))
							// Добавляем название рабочей системы в ответ
							response.append(this->_fmk->format("X-Powered-By: %s/%s\r\n", this->_ident.id.c_str(), this->_ident.version.c_str()));
						// Если заголовок авторизации не передан
						if(((res.code == 401) && !available[10]) || ((res.code == 407) && !available[11])){
							// Получаем параметры авторизации
							const string & auth = this->_auth.server;
							// Если параметры авторизации получены
							if(!auth.empty()){
								// Определяем код авторизации
								switch(res.code){
									// Если авторизация производится для Web-Сервера
									case 401: {
										// Если заголовок не запрещён
										if(!this->is(suite_t::BLACK, "WWW-Authenticate"))
											// Добавляем параметры авторизации
											response.append(this->_fmk->format("WWW-Authenticate: %s\r\n", auth.c_str()));
									} break;
									// Если авторизация производится для Прокси-Сервера
									case 407: {
										// Если заголовок не запрещён
										if(!this->is(suite_t::BLACK, "Proxy-Authenticate"))
											// Добавляем параметры авторизации
											response.append(this->_fmk->format("Proxy-Authenticate: %s\r\n", auth.c_str()));
									} break;
								}
							}
						}
						// Если запрос должен содержать тело и тело ответа существует
						if((res.code >= 200) && (res.code != 204) && (res.code != 304) && (res.code != 308)){
							// Если тело запроса существует
							if(!this->_web.body().empty()){
								// Выполняем компрессию полезной нагрузки
								const_cast <http_t *> (this)->compress();
								// Выполняем шифрование полезной нагрузки
								const_cast <http_t *> (this)->encrypt();
								// Проверяем нужно ли передать тело разбив на чанки
								this->_te.chunking = (this->_crypted || this->_te.trailers || (this->_compressor.current != compress_t::NONE));
								// Если заголовок не запрещён
								if(!available[0] && !this->is(suite_t::BLACK, "Date"))
									// Добавляем заголовок даты в ответ
									response.append(this->_fmk->format("Date: %s\r\n", this->date().c_str()));
								// Заменяем размер тела данных
								if(!this->_te.chunking)
									// Устанавливаем размер тела сообщения
									length = this->_web.body().size();
								// Если данные зашифрованы, устанавливаем соответствующие заголовки
								if(this->_crypted)
									// Устанавливаем X-AWH-Encryption
									response.append(this->_fmk->format("X-AWH-Encryption: %u\r\n", static_cast <u_short> (this->_hash.cipher())));
								{
									// Название компрессора
									string compressor = "";
									// Определяем метод компрессии полезной нагрузки
									switch(static_cast <uint8_t> (this->_compressor.current)){
										// Если полезная нагрузка сжата методом BROTLI то устанавливаем название компрессора BR
										case static_cast <uint8_t> (compress_t::BROTLI):  compressor = "br";      break;
										// Если полезная нагрузка сжата методом GZIP то устанавливаем название компрессора GZIP
										case static_cast <uint8_t> (compress_t::GZIP):    compressor = "gzip";    break;
										// Если полезная нагрузка сжата методом DEFLATE то устанавливаем название компрессора DEFLATE
										case static_cast <uint8_t> (compress_t::DEFLATE): compressor = "deflate"; break;
									}
									// Если компрессор получен
									if(!compressor.empty()){
										// Если активирован режим отправки через Transfer-Encoding
										if(this->_te.enabled && !this->is(suite_t::BLACK, "Transfer-Encoding")){
											// Если активирован режим передачи чанками
											if(this->_te.chunking)
												// Устанавливаем Transfer-Encoding если не передан
												response.append(this->_fmk->format("Transfer-Encoding: %s, chunked\r\n", compressor.c_str()));
											// Устанавливаем Transfer-Encoding если не передан
											else response.append(this->_fmk->format("Transfer-Encoding: %s\r\n", compressor.c_str()));
										// Устанавливаем Content-Encoding если не передан
										} else response.append(this->_fmk->format("Content-Encoding: %s\r\n", compressor.c_str()));
									// Если активирован режим передачи чанками
									} else if(this->_te.enabled && this->_te.chunking && !this->is(suite_t::BLACK, "Transfer-Encoding"))
										// Устанавливаем заголовок Transfer-Encoding
										response.append(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
								}
								// Если данные необходимо разбивать на чанки
								if(this->_te.chunking && !this->is(suite_t::BLACK, "Transfer-Encoding")){
									// Если режим отправки шифровани через Transfer-Encoding не активирован
									if(!this->_te.enabled)
										// Устанавливаем заголовок Transfer-Encoding
										response.append(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
								// Если заголовок размера передаваемого тела, не запрещён
								} else if(!this->is(suite_t::BLACK, "Content-Length") && ((length > 0) || this->_web.isHeader("Content-Length")))
									// Устанавливаем размер передаваемого тела Content-Length
									response.append(this->_fmk->format("Content-Length: %zu\r\n", length));
							// Если тело запроса не существует
							} else {
								// Проверяем нужно ли передать тело разбив на чанки
								this->_te.chunking = (this->_encryption || this->_te.trailers || (this->_compressor.selected != compress_t::NONE));
								// Если заголовок не запрещён
								if(!available[0] && !this->is(suite_t::BLACK, "Date"))
									// Добавляем заголовок даты в ответ
									response.append(this->_fmk->format("Date: %s\r\n", this->date().c_str()));
								// Если данные зашифрованы, устанавливаем соответствующие заголовки
								if(this->_encryption && !this->is(suite_t::BLACK, "X-AWH-Encryption"))
									// Устанавливаем X-AWH-Encryption
									response.append(this->_fmk->format("X-AWH-Encryption: %u\r\n", static_cast <u_short> (this->_hash.cipher())));
								{
									// Название компрессора
									string compressor = "";
									// Определяем метод компрессии полезной нагрузки
									switch(static_cast <uint8_t> (this->_compressor.selected)){
										// Если полезная нагрузка сжата методом BROTLI то устанавливаем название компрессора BR
										case static_cast <uint8_t> (compress_t::BROTLI):  compressor = "br";      break;
										// Если полезная нагрузка сжата методом GZIP то устанавливаем название компрессора GZIP
										case static_cast <uint8_t> (compress_t::GZIP):    compressor = "gzip";    break;
										// Если полезная нагрузка сжата методом DEFLATE то устанавливаем название компрессора DEFLATE
										case static_cast <uint8_t> (compress_t::DEFLATE): compressor = "deflate"; break;
									}
									// Если компрессор получен
									if(!compressor.empty()){
										// Если активирован режим отправки через Transfer-Encoding
										if(this->_te.enabled && !this->is(suite_t::BLACK, "Transfer-Encoding")){
											// Если активирован режим передачи чанками
											if(this->_te.chunking)
												// Устанавливаем Transfer-Encoding если не передан
												response.append(this->_fmk->format("Transfer-Encoding: %s, chunked\r\n", compressor.c_str()));
											// Устанавливаем Transfer-Encoding если не передан
											else response.append(this->_fmk->format("Transfer-Encoding: %s\r\n", compressor.c_str()));
										// Если Content-Encoding не запрещён в запросе
										} else if(!this->is(suite_t::BLACK, "Content-Encoding"))
											// Устанавливаем Content-Encoding если не передан
											response.append(this->_fmk->format("Content-Encoding: %s\r\n", compressor.c_str()));
									// Если активирован режим передачи чанками
									} else if(this->_te.enabled && this->_te.chunking && available[8] && !this->is(suite_t::BLACK, "Transfer-Encoding"))
										// Устанавливаем заголовок Transfer-Encoding
										response.append(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
								}
								// Если данные необходимо разбивать на чанки
								if(this->_te.chunking && available[8] && !this->is(suite_t::BLACK, "Transfer-Encoding")){
									// Если режим отправки шифровани через Transfer-Encoding не активирован
									if(!this->_te.enabled)
										// Устанавливаем заголовок Transfer-Encoding
										response.append(this->_fmk->format("Transfer-Encoding: %s\r\n", "chunked"));
								// Если заголовок размера передаваемого тела, не запрещён
								} else if(!this->is(suite_t::BLACK, "Content-Length") && ((length > 0) || this->_web.isHeader("Content-Length")))
									// Устанавливаем размер передаваемого тела Content-Length
									response.append(this->_fmk->format("Content-Length: %zu\r\n", length));
							}
						// Очищаем тела сообщения
						} else const_cast <http_t *> (this)->clear(suite_t::BODY);
					} break;
				}
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
 * @param flag флаг выполняемого процесса
 * @param prov параметры провайдера обмена сообщениями
 * @return     буфер данных в бинарном виде
 */
vector <pair <string, string>> awh::Http::process2(const process_t flag, const web_t::provider_t & prov) const noexcept {
	// Результат работы функции
	vector <pair<string, string>> result;
	// Определяем флаг выполняемого процесса
	switch(static_cast <uint8_t> (flag)){
		// Если нужно сформировать данные запроса
		case static_cast <uint8_t> (process_t::REQUEST): {
			// Получаем объект ответа клиенту
			const web_t::req_t & req = static_cast <const web_t::req_t &> (prov);
			// Если параметры запроса получены
			if(!req.url.empty() && (req.method != web_t::method_t::NONE)){
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
				if(this->_precise || (req.method == web_t::method_t::CONNECT))
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
				// Определяем тип HTTP-модуля
				switch(static_cast <uint8_t> (this->_web.hid())){
					// Если мы работаем с клиентом
					case static_cast <uint8_t> (web_t::hid_t::CLIENT): {
						/**
						 * Типы основных заголовков
						 */
						bool available[14] = {
							false, // te
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
						// Переходим по всему списку заголовков
						for(auto & header : this->_web.headers()){
							// Если заголовок не является системным
							if(header.first.front() != ':'){
								// Флаг разрешающий вывода заголовка
								bool allow = !this->is(suite_t::BLACK, header.first);
								// Выполняем перебор всех обязательных заголовков
								for(uint8_t i = 0; i < 14; i++){
									// Если заголовок уже найден пропускаем его
									if(available[i])
										// Продолжаем поиск дальше
										continue;
									// Выполняем првоерку заголовка
									switch(i){
										case 0:  available[i] = this->_fmk->compare(header.first, "te");                    break;
										case 1:  available[i] = this->_fmk->compare(header.first, "host");                  break;
										case 2:  available[i] = this->_fmk->compare(header.first, "accept");                break;
										case 3:  available[i] = this->_fmk->compare(header.first, "origin");                break;
										case 4:  available[i] = this->_fmk->compare(header.first, "user-agent");            break;
										case 5:  available[i] = this->_fmk->compare(header.first, "connection");            break;
										case 6:  available[i] = this->_fmk->compare(header.first, "proxy-connection");      break;
										case 7:  available[i] = this->_fmk->compare(header.first, "content-length");        break;
										case 8:  available[i] = this->_fmk->compare(header.first, "accept-language");       break;
										case 9:  available[i] = this->_fmk->compare(header.first, "accept-encoding");       break;
										case 10: available[i] = this->_fmk->compare(header.first, "content-encoding");      break;
										case 11: available[i] = this->_fmk->compare(header.first, "transfer-encoding");     break;
										case 12: available[i] = this->_fmk->compare(header.first, "x-awh-encryption");      break;
										case 13: available[i] = (this->_fmk->compare(header.first, "authorization") ||
																 this->_fmk->compare(header.first, "proxy-authorization")); break;
									}
									// Если заголовок разрешён для вывода
									if(allow){
										// Выполняем првоерку заголовка
										switch(i){
											case 0:
											case 1:
											case 5:
											case 6:
											case 7:
											case 10:
											case 11:
											case 12: allow = !available[i]; break;
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
						if(!available[2] && (req.method != web_t::method_t::CONNECT) && !this->is(suite_t::BLACK, "accept"))
							// Добавляем заголовок в запрос
							result.push_back(make_pair("accept", HTTP_HEADER_ACCEPT));
						// Устанавливаем Accept-Language если не передан
						if(!available[8] && (req.method != web_t::method_t::CONNECT) && !this->is(suite_t::BLACK, "accept-language"))
							// Добавляем заголовок в запрос
							result.push_back(make_pair("accept-language", HTTP_HEADER_ACCEPTLANGUAGE));
						// Если нужно запросить компрессию в удобном нам виде
						if(!available[9] && (req.method != web_t::method_t::CONNECT) && (!this->_compressor.supports.empty() || (this->_compressor.selected != compress_t::NONE)) && !this->is(suite_t::BLACK, "accept-encoding")){
							// Если компрессор уже выбран
							if(this->_compressor.selected != compress_t::NONE){
								// Определяем метод сжатия который поддерживает клиент
								switch(static_cast <uint8_t> (this->_compressor.selected)){
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
								}
							// Если список компрессоров установлен
							} else if(!this->_compressor.supports.empty()) {
								// Строка со списком компрессоров
								string compressors = "";
								// Выполняем перебор всего списка компрессоров
								for(auto it = this->_compressor.supports.rbegin(); it != this->_compressor.supports.rend(); ++it){
									// Если список компрессоров уже не пустой
									if(!compressors.empty())
										// Выполняем добавление разделителя
										compressors.append(", ");
									// Определяем метод сжатия который поддерживает клиент
									switch(static_cast <uint8_t> (it->second)){
										// Если клиент поддерживает методот сжатия BROTLI
										case static_cast <uint8_t> (compress_t::BROTLI):
											// Добавляем компрессор в список
											compressors.append("br");
										break;
										// Если клиент поддерживает методот сжатия GZIP
										case static_cast <uint8_t> (compress_t::GZIP):
											// Добавляем компрессор в список
											compressors.append("gzip");
										break;
										// Если клиент поддерживает методот сжатия DEFLATE
										case static_cast <uint8_t> (compress_t::DEFLATE):
											// Добавляем компрессор в список
											compressors.append("deflate");
										break;
									}
								}
								// Если список компрессоров получен
								if(!compressors.empty())
									// Добавляем заголовок в запрос
									result.push_back(make_pair("accept-encoding", compressors));
							}
						}
						// Устанавливаем User-Agent если не передан
						if(!available[4] && !this->is(suite_t::BLACK, "user-agent")){
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
								// Выполняем генерацию Юзер-агента клиента выполняющего HTTP-запрос
								this->_userAgent = this->_fmk->format("%s (%s; %s/%s)", this->_ident.name.c_str(), os, this->_ident.id.c_str(), this->_ident.version.c_str());
							}
							// Добавляем заголовок в запрос
							result.push_back(make_pair("user-agent", this->_userAgent));
						}
						// Если заголовок авторизации не передан
						if(!available[13]){
							// Метод HTTP-запроса
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
									if(!this->is(suite_t::BLACK, "proxy-authorization")){
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
									if(!this->is(suite_t::BLACK, "authorization")){
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
						// Если нужно вставить заголовок TE и он не находится в чёрном списке
						if(available[0] && !this->is(suite_t::BLACK, "te"))
							// Устанавливаем Transfer-Encoding в запрос
							result.push_back(make_pair("te", "trailers"));
						// Если запрос является PUT, POST, PATCH
						if((req.method == web_t::method_t::PUT) || (req.method == web_t::method_t::POST) || (req.method == web_t::method_t::PATCH)){
							// Если заголовок не запрещён
							if(!this->is(suite_t::BLACK, "date"))
								// Добавляем заголовок даты в запрос
								result.push_back(make_pair("date", this->date()));
							// Если тело запроса существует
							if(!this->_web.body().empty()){
								// Выполняем компрессию полезной нагрузки
								const_cast <http_t *> (this)->compress();
								// Выполняем шифрование полезной нагрузки
								const_cast <http_t *> (this)->encrypt();
								// Проверяем нужно ли передать тело разбив на чанки
								this->_te.chunking = (this->_crypted || (this->_compressor.current != compress_t::NONE));
								// Если данные зашифрованы, устанавливаем соответствующие заголовки
								if(this->_crypted)
									// Устанавливаем X-AWH-Encryption
									result.push_back(make_pair("x-awh-encryption", ::to_string(static_cast <u_short> (this->_hash.cipher()))));
								// Определяем метод компрессии полезной нагрузки
								switch(static_cast <uint8_t> (this->_compressor.current)){
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
							// Если тело запроса не существует
							} else {
								// Проверяем нужно ли передать тело разбив на чанки
								this->_te.chunking = (this->_encryption || (this->_compressor.selected != compress_t::NONE));
								// Если данные зашифрованы, устанавливаем соответствующие заголовки
								if(this->_encryption && !this->is(suite_t::BLACK, "x-awh-encryption"))
									// Устанавливаем X-AWH-Encryption
									result.push_back(make_pair("x-awh-encryption", ::to_string(static_cast <u_short> (this->_hash.cipher()))));
								// Устанавливаем Content-Encoding если не передан
								if(!this->is(suite_t::BLACK, "content-encoding")){
									// Определяем метод компрессии полезной нагрузки
									switch(static_cast <uint8_t> (this->_compressor.selected)){
										// Если полезная нагрузка сжата методом BROTLI
										case static_cast <uint8_t> (compress_t::BROTLI):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(make_pair("content-encoding", "br"));
										break;
										// Если полезная нагрузка сжата методом GZIP
										case static_cast <uint8_t> (compress_t::GZIP):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(make_pair("content-encoding", "gzip"));
										break;
										// Если полезная нагрузка сжата методом DEFLATE
										case static_cast <uint8_t> (compress_t::DEFLATE):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(make_pair("content-encoding", "deflate"));
										break;
									}
								}
							}
						// Если запрос не содержит тела запроса
						} else {
							// Если данные зашифрованы, устанавливаем соответствующие заголовки
							if((this->_te.chunking = (this->_encryption && !this->is(suite_t::BLACK, "x-awh-encryption"))))
								// Устанавливаем X-AWH-Encryption
								result.push_back(make_pair("x-awh-encryption", ::to_string(static_cast <u_short> (this->_hash.cipher()))));
							// Устанавливаем Content-Encoding если заголовок есть в запросе
							if(available[10] && !this->is(suite_t::BLACK, "content-encoding")){
								// Определяем метод компрессии полезной нагрузки
								switch(static_cast <uint8_t> (this->_compressor.selected)){
									// Если полезная нагрузка сжата методом BROTLI
									case static_cast <uint8_t> (compress_t::BROTLI):
										// Устанавливаем Content-Encoding если не передан
										result.push_back(make_pair("content-encoding", "br"));
									break;
									// Если полезная нагрузка сжата методом GZIP
									case static_cast <uint8_t> (compress_t::GZIP):
										// Устанавливаем Content-Encoding если не передан
										result.push_back(make_pair("content-encoding", "gzip"));
									break;
									// Если полезная нагрузка сжата методом DEFLATE
									case static_cast <uint8_t> (compress_t::DEFLATE):
										// Устанавливаем Content-Encoding если не передан
										result.push_back(make_pair("content-encoding", "deflate"));
									break;
								}
								// Проверяем нужно ли передать тело разбив на чанки
								this->_te.chunking = (this->_compressor.selected != compress_t::NONE);
							}
							// Очищаем тела сообщения
							const_cast <http_t *> (this)->clear(suite_t::BODY);
						}
					} break;
					// Если мы работаем с сервером
					case static_cast <uint8_t> (web_t::hid_t::SERVER): {
						// Переходим по всему списку заголовков
						for(auto & header : this->_web.headers()){
							// Если заголовок не является системным
							if(header.first.front() != ':')
								// Формируем строку запроса
								result.push_back(make_pair(this->_fmk->transform(header.first, fmk_t::transform_t::LOWER), header.second));
						}
					} break;
				}
			}
		} break;
		// Если нужно сформировать данные ответа
		case static_cast <uint8_t> (process_t::RESPONSE): {
			// Получаем объект ответа клиенту
			const web_t::res_t & res = static_cast <const web_t::res_t &> (prov);
			// Если текст сообщения не установлен
			if(res.message.empty())
				// Выполняем установку сообщения
				const_cast <web_t::res_t &> (res).message = this->message(res.code);
			// Если сообщение получено
			if(!res.message.empty()){
				// Данные REST ответа
				result.push_back(make_pair(":status", ::to_string(res.code)));
				// Определяем тип HTTP-модуля
				switch(static_cast <uint8_t> (this->_web.hid())){
					// Если мы работаем с клиентом
					case static_cast <uint8_t> (web_t::hid_t::CLIENT): {
						// Переходим по всему списку заголовков
						for(auto & header : this->_web.headers())
							// Формируем строку ответа
							result.push_back(make_pair(this->_fmk->transform(header.first, fmk_t::transform_t::LOWER), header.second));
					} break;
					// Если мы работаем с сервером
					case static_cast <uint8_t> (web_t::hid_t::SERVER): {
						/**
						 * Типы основных заголовков
						 */
						bool available[12] = {
							false, // Date
							false, // Server
							false, // Connection
							false, // Proxy-Connection
							false, // X-Powered-By
							false, // Content-Type
							false, // Content-Length
							false, // Content-Encoding
							false, // Transfer-Encoding
							false, // X-AWH-Encryption
							false, // WWW-Authenticate
							false  // Proxy-Authenticate
						};
						// Устанавливаем параметры ответа
						this->_web.response(res);
						// Переходим по всему списку заголовков
						for(auto & header : this->_web.headers()){
							// Флаг разрешающий вывода заголовка
							bool allow = !this->is(suite_t::BLACK, header.first);
							// Выполняем перебор всех обязательных заголовков
							for(uint8_t i = 0; i < 12; i++){
								// Если заголовок уже найден пропускаем его
								if(available[i])
									// Продолжаем поиск дальше
									continue;
								// Выполняем првоерку заголовка
								switch(i){
									case 0:  available[i] = this->_fmk->compare(header.first, "date");               break;
									case 1:  available[i] = this->_fmk->compare(header.first, "server");             break;
									case 2:  available[i] = this->_fmk->compare(header.first, "connection");         break;
									case 3:  available[i] = this->_fmk->compare(header.first, "proxy-connection");   break;
									case 4:  available[i] = this->_fmk->compare(header.first, "x-powered-by");       break;
									case 5:  available[i] = this->_fmk->compare(header.first, "content-type");       break;
									case 6:  available[i] = this->_fmk->compare(header.first, "content-length");     break;
									case 7:  available[i] = this->_fmk->compare(header.first, "content-encoding");   break;
									case 8:  available[i] = this->_fmk->compare(header.first, "transfer-encoding");  break;
									case 9:  available[i] = this->_fmk->compare(header.first, "x-awh-encryption");   break;
									case 10: available[i] = this->_fmk->compare(header.first, "www-authenticate");   break;
									case 11: available[i] = this->_fmk->compare(header.first, "proxy-authenticate"); break;
								}
								// Если заголовок разрешён для вывода
								if(allow){
									// Выполняем првоерку заголовка
									switch(i){
										case 2:
										case 3:
										case 6:
										case 7:
										case 8:
										case 9: allow = !available[i]; break;
									}
									// Если ответ является информационным
									if((((res.code >= 100) && (res.code < 200)) || (res.code == 204)) && available[i]){
										// Запрещяем указанным заголовкам формирование
										switch(i){
											case 0:
											case 5:
											case 6:
											case 7:
											case 8:
											case 9:
											case 10:
											case 11: allow = false; break;
										}
									}
								}
							}
							// Если заголовок не является запрещённым, добавляем заголовок в ответ
							if(allow)
								// Формируем строку ответа
								result.push_back(make_pair(this->_fmk->transform(header.first, fmk_t::transform_t::LOWER), header.second));
						}
						// Если заголовок не запрещён
						if(!available[1] && !this->is(suite_t::BLACK, "server"))
							// Добавляем название сервера в ответ
							result.push_back(make_pair("server", this->_ident.name));
						/*
						// Устанавливаем Content-Type если не передан
						if(!available[5] && (res.code >= 200) && !this->is(suite_t::BLACK, "content-type"))
							// Добавляем заголовок в ответ
							result.push_back(make_pair("content-type", HTTP_HEADER_CONTENTTYPE));
						*/
						// Если заголовок не запрещён
						if(!available[4] && !this->is(suite_t::BLACK, "x-powered-by"))
							// Добавляем название рабочей системы в ответ
							result.push_back(make_pair("x-powered-by", this->_fmk->format("%s/%s", this->_ident.id.c_str(), this->_ident.version.c_str())));
						// Если заголовок авторизации не передан
						if(((res.code == 401) && !available[10]) || ((res.code == 407) && !available[11])){
							// Получаем параметры авторизации
							const string & auth = this->_auth.server;
							// Если параметры авторизации получены
							if(!auth.empty()){
								// Определяем код авторизации
								switch(res.code){
									// Если авторизация производится для Web-Сервера
									case 401: {
										// Если заголовок не запрещён
										if(!this->is(suite_t::BLACK, "www-authenticate"))
											// Добавляем параметры авторизации
											result.push_back(make_pair("www-authenticate", auth));
									} break;
									// Если авторизация производится для Прокси-Сервера
									case 407: {
										// Если заголовок не запрещён
										if(!this->is(suite_t::BLACK, "proxy-authenticate"))
											// Добавляем параметры авторизации
											result.push_back(make_pair("proxy-authenticate", auth));
									} break;
								}
							}
						}
						// Если запрос должен содержать тело и тело ответа существует
						if((res.code >= 200) && (res.code != 204) && (res.code != 304) && (res.code != 308)){
							// Если тело запроса существует
							if(!this->_web.body().empty()){
								// Выполняем компрессию полезной нагрузки
								const_cast <http_t *> (this)->compress();
								// Выполняем шифрование полезной нагрузки
								const_cast <http_t *> (this)->encrypt();
								// Проверяем нужно ли передать тело разбив на чанки
								this->_te.chunking = (this->_crypted || (this->_compressor.current != compress_t::NONE));
								// Если заголовок не запрещён
								if(!available[0] && !this->is(suite_t::BLACK, "date"))
									// Добавляем заголовок даты в ответ
									result.push_back(make_pair("date", this->date()));
								// Если данные зашифрованы, устанавливаем соответствующие заголовки
								if(this->_crypted)
									// Устанавливаем X-AWH-Encryption
									result.push_back(make_pair("x-awh-encryption", ::to_string(static_cast <u_short> (this->_hash.cipher()))));
								// Определяем метод компрессии полезной нагрузки
								switch(static_cast <uint8_t> (this->_compressor.current)){
									// Если полезная нагрузка сжата методом BROTLI
									case static_cast <uint8_t> (compress_t::BROTLI):
										// Устанавливаем Content-Encoding если не передан
										result.push_back(make_pair("content-encoding", "br"));
									break;
									// Если полезная нагрузка сжата методом GZIP
									case static_cast <uint8_t> (compress_t::GZIP):
										// Устанавливаем Content-Encoding если не передан
										result.push_back(make_pair("content-encoding", "gzip"));
									break;
									// Если полезная нагрузка сжата методом DEFLATE
									case static_cast <uint8_t> (compress_t::DEFLATE):
										// Устанавливаем Content-Encoding если не передан
										result.push_back(make_pair("content-encoding", "deflate"));
									break;
								}
							// Если тело запроса не существует
							} else {
								// Проверяем нужно ли передать тело разбив на чанки
								this->_te.chunking = (this->_encryption || (this->_compressor.selected != compress_t::NONE));
								// Если заголовок не запрещён
								if(!available[0] && !this->is(suite_t::BLACK, "date"))
									// Добавляем заголовок даты в ответ
									result.push_back(make_pair("date", this->date()));
								// Если данные зашифрованы, устанавливаем соответствующие заголовки
								if(this->_encryption && !this->is(suite_t::BLACK, "x-awh-encryption"))
									// Устанавливаем X-AWH-Encryption
									result.push_back(make_pair("x-awh-encryption", ::to_string(static_cast <u_short> (this->_hash.cipher()))));
								// Устанавливаем Content-Encoding если не передан
								if(!this->is(suite_t::BLACK, "content-encoding")){
									// Определяем метод компрессии полезной нагрузки
									switch(static_cast <uint8_t> (this->_compressor.selected)){
										// Если полезная нагрузка сжата методом BROTLI
										case static_cast <uint8_t> (compress_t::BROTLI):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(make_pair("content-encoding", "br"));
										break;
										// Если полезная нагрузка сжата методом GZIP
										case static_cast <uint8_t> (compress_t::GZIP):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(make_pair("content-encoding", "gzip"));
										break;
										// Если полезная нагрузка сжата методом DEFLATE
										case static_cast <uint8_t> (compress_t::DEFLATE):
											// Устанавливаем Content-Encoding если не передан
											result.push_back(make_pair("content-encoding", "deflate"));
										break;
									}
								}
							}
						// Очищаем тела сообщения
						} else const_cast <http_t *> (this)->clear(suite_t::BODY);
					} break;
				}
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
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::Http::on(function <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для перехвата ошибок
	this->_web.on(callback);
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", callback);
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
 * userAgent Метод установки User-Agent для HTTP-запроса
 * @param userAgent агент пользователя для HTTP-запроса
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
 * crypted Метод проверки на зашифрованные данные
 * @return флаг проверки на зашифрованные данные
 */
bool awh::Http::crypted() const noexcept {
	// Выводим результат проверки
	return this->_crypted;
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::Http::encryption(const bool mode) noexcept {
	// Устанавливаем флаг шифрования
	this->_encryption = mode;
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::Http::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Если пароль шифрования передан
	if(!pass.empty()){
		// Устанавливаем соль шифрования
		this->_hash.salt(salt);
		// Устанавливаем пароль шифрования
		this->_hash.pass(pass);
		// Устанавливаем размер шифрования
		this->_hash.cipher(cipher);
	}
}
/**
 * Http Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Http::Http(const fmk_t * fmk, const log_t * log) noexcept :
 _uri(fmk), _callback(log), _web(fmk, log), _auth(fmk, log), _hash(log),
 _crypted(false), _encryption(false), _precise(false), _chunk(BUFFER_CHUNK),
 _state(state_t::NONE), _status(status_t::NONE), _identity(identity_t::NONE),
 _userAgent(HTTP_HEADER_AGENT), _fmk(fmk), _log(log) {
	// Выполняем установку идентификатора объекта
	this->_web.id(this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS));
	// Устанавливаем функцию обратного вызова для получения чанков
	this->_web.on(std::bind(&awh::Http::chunkingCallback, this, _1, _2, _3));
}
