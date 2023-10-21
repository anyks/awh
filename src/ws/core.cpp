/**
 * @file: ws.cpp
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
#include <ws/core.hpp>

/**
 * init Метод инициализации
 * @param flag флаг направления передачи данных
 */
void awh::WCore::init(const process_t flag) noexcept {
	// Удаляем заголовок сабпратоколов
	this->rmHeader("Sec-WebSocket-Protocol");
	// Определяем флаг выполняемого процесса
	switch(static_cast <uint8_t> (flag)){
		// Если нужно сформировать данные запроса
		case static_cast <uint8_t> (process_t::REQUEST): {
			// Удаляем заголовок Accept
			this->rmHeader("Accept");
			// Удаляем заголовок отключения кеширования
			this->rmHeader("Pragma");
			// Удаляем заголовок отключения кеширования
			this->rmHeader("Cache-Control");
			// Удаляем заголовок типа запроса
			this->rmHeader("Sec-Fetch-Mode");
			// Удаляем заголовок места назначения запроса
			this->rmHeader("Sec-Fetch-Dest");
			// Удаляем заголовок требования сжимать содержимое ответов
			this->rmHeader("Accept-Encoding");
			// Удаляем заголовок поддерживаемых языков
			this->rmHeader("Accept-Language");
			// Удаляем заголовок версии WebSocket
			this->rmHeader("Sec-WebSocket-Version");
			// Если список поддерживаемых сабпротоколов установлен
			if(!this->_supportedProtocols.empty()){
				// Если количество поддерживаемых сабпротоколов больше 5-ти
				if(this->_supportedProtocols.size() > 5){
					// Список поддерживаемых сабпротоколов
					string subprotocols = "";
					// Переходим по всему списку поддерживаемых сабпротоколов
					for(auto & subprotocol : this->_supportedProtocols){
						// Если сабпротокол уже не пустой
						if(!subprotocols.empty())
							// Добавляем разделитель
							subprotocols.append(", ");
						// Добавляем в список поддерживаемых сабпротоколов
						subprotocols.append(subprotocol);
					}
					// Добавляем заголовок поддерживаемых сабпротоколов
					this->header("Sec-WebSocket-Protocol", subprotocols);
				// Если сабпротоколов слишком много
				} else {
					// Получаем список заголовков
					const auto & headers = this->_web.headers();
					// Переходим по всему списку поддерживаемых сабпротоколов
					for(auto & subprotocol : this->_supportedProtocols)
						// Добавляем полученный заголовок
						const_cast <unordered_multimap <string, string> *> (&headers)->insert({{"Sec-WebSocket-Protocol", subprotocol}});
				}
			}
			// Выполняем применение расширений
			this->applyExtensions(flag);
			// Добавляем заголовок Accept
			this->header("Accept", "*/*");
			// Добавляем заголовок отключения кеширования
			this->header("Pragma", "No-Cache");
			// Добавляем заголовок отключения кеширования
			this->header("Cache-Control", "No-Cache");
			// Добавляем заголовок типа запроса
			this->header("Sec-Fetch-Mode", "WebSocket");
			// Добавляем заголовок места назначения запроса
			this->header("Sec-Fetch-Dest", "WebSocket");
			// Добавляем заголовок требования сжимать содержимое ответов
			this->header("Accept-Encoding", "gzip, deflate, br");
			// Добавляем заголовок поддерживаемых языков
			this->header("Accept-Language", HTTP_HEADER_ACCEPTLANGUAGE);
			// Добавляем заголовок версии WebSocket
			this->header("Sec-WebSocket-Version", std::to_string(WS_VERSION));
		} break;
		// Если нужно сформировать данные ответа
		case static_cast <uint8_t> (process_t::RESPONSE): {
			// Добавляем в чёрный список заголовок Content-Type
			this->addBlack("Content-Type");
			// Получаем объект ответа клиенту
			const web_t::res_t & res = this->_web.response();
			// Если ответ сервера положительный
			if(res.version >= 2.0f ? res.code == 200 : res.code == 101){
				// Выполняем применение расширений
				this->applyExtensions(flag);
				// Добавляем в чёрный список заголовок Content-Encoding
				this->addBlack("Content-Encoding");
				// Добавляем в чёрный список заголовок X-AWH-Encryption
				this->addBlack("X-AWH-Encryption");
			}
			// Если список выбранных сабпротоколов установлен
			if(!this->_selectedProtocols.empty()){
				// Если количество выбранных сабпротоколов больше 5-ти
				if(this->_selectedProtocols.size() > 5){
					// Список выбранных сабпротоколов
					string subprotocols = "";
					// Переходим по всему списку выбранных сабпротоколов
					for(auto & subprotocol : this->_selectedProtocols){
						// Если сабпротокол уже не пустой
						if(!subprotocols.empty())
							// Добавляем разделитель
							subprotocols.append(", ");
						// Добавляем в список выбранных сабпротоколов
						subprotocols.append(subprotocol);
					}
					// Добавляем заголовок выбранных сабпротоколов
					this->header("Sec-WebSocket-Protocol", subprotocols);
				// Если сабпротоколов слишком много
				} else {
					// Получаем список заголовков
					const auto & headers = this->_web.headers();
					// Переходим по всему списку выбранных сабпротоколов
					for(auto & subprotocol : this->_selectedProtocols)
						// Добавляем полученный заголовок
						const_cast <unordered_multimap <string, string> *> (&headers)->insert({{"Sec-WebSocket-Protocol", subprotocol}});
				}
			}
		} break;
	}
}
/**
 * applyExtensions Метод установки выбранных расширений
 * @param flag флаг направления передачи данных
 */
void awh::WCore::applyExtensions(const process_t flag) noexcept {
	// Список поддверживаемых расширений
	vector <vector <string>> extensions;
	// Удаляем заголовок расширений
	this->rmHeader("Sec-WebSocket-Extensions");
	// Определяем тип активной компрессии
	switch(static_cast <uint8_t> (this->_compress)){
		// Если метод компрессии выбран GZip
		case static_cast <uint8_t> (compress_t::GZIP):
			// Добавляем метод сжатия GZip
			extensions.push_back({"permessage-gzip"});
		break;
		// Если метод компрессии выбран Brotli
		case static_cast <uint8_t> (compress_t::BROTLI):
			// Добавляем метод сжатия Brotli
			extensions.push_back({"permessage-br"});
		break;
		// Если метод компрессии выбран Deflate
		case static_cast <uint8_t> (compress_t::DEFLATE): {
			// Добавляем метод сжатия Deflate
			extensions.push_back({"permessage-deflate"});
			// Если запрещено переиспользовать контекст компрессии для сервера
			if(!this->_server.takeover)
				// Добавляем флаг запрещения использования контекста компрессии для сервера
				extensions.push_back({"server_no_context_takeover"});
			// Если запрещено переиспользовать контекст компрессии для клиента
			if(!this->_client.takeover)
				// Добавляем флаг запрещения использования контекста компрессии для клиента
				extensions.push_back({"client_no_context_takeover"});
			// Определяем флаг выполняемого процесса
			switch(static_cast <uint8_t> (flag)){
				// Если нужно сформировать данные запроса
				case static_cast <uint8_t> (process_t::REQUEST): {
					// Если размер скользящего окна клиента установлен как максимальный
					if(this->_client.wbit == static_cast <short> (GZIP_MAX_WBITS))
						// Добавляем максимальный размер скользящего окна
						extensions.push_back({"client_max_window_bits"});
					// Выполняем установку указанного размера скользящего окна
					else extensions.push_back({this->_fmk->format("client_max_window_bits=%u", this->_client.wbit)});
					// Если размер скользящего окна сервера установлен как максимальный
					if(this->_server.wbit == static_cast <short> (GZIP_MAX_WBITS))
						// Добавляем максимальный размер скользящего окна
						extensions.push_back({"server_max_window_bits"});
					// Выполняем установку указанного размера скользящего окна сервера
					else extensions.push_back({this->_fmk->format("server_max_window_bits=%u", this->_server.wbit)});
				} break;
				// Если нужно сформировать данные ответа
				case static_cast <uint8_t> (process_t::RESPONSE): {
					// Выполняем установку указанного размера скользящего окна
					extensions.push_back({this->_fmk->format("client_max_window_bits=%u", this->_client.wbit)});
					// Выполняем установку указанного размера скользящего окна сервера
					extensions.push_back({this->_fmk->format("server_max_window_bits=%u", this->_server.wbit)});
				} break;
			}
		} break;
	}
	// Если данные должны быть зашифрованны
	if(this->_crypted)
		// Выполняем установку указанного метода шифрования
		extensions.push_back({this->_fmk->format("permessage-encrypt=%u", static_cast <u_short> (this->_hash.cipher()))});
	// Если список расширений не пустой
	if(!this->_extensions.empty())
		// Добавляем к списку расширений пользовательские расширения
		extensions.insert(extensions.end(), this->_extensions.begin(), this->_extensions.end());
	// Если список расширений не пустой
	if(!extensions.empty()){
		// Список записей расширений
		string records = "";
		// Выполняем перебор установленных расширений
		for(auto & extension : extensions){
			// Если в списке расширений уже есть записи
			if(!records.empty())
				// Добавляем разделитель
				records.append("; ");
			// Если количество элементов в списке больше 3-х
			if(extension.size() > 3){
				// Выполняем установку первой записи
				records.append(extension.front());
				// Выполняем перебор оставшихся расширений
				for(size_t i = 1; i < extension.size(); i++){
					// Добавляем заголовок сабпротокола
					this->header("Sec-WebSocket-Extensions", records);
					// Выполняем очистку списка расширений
					records.clear();
					// Выполняем установку записи
					records.append(extension.at(i));
				}
			// Если количество элементов минимально
			} else {
				// Выполняем перебор всех доступных расширений
				for(size_t i = 0; i < extension.size(); i++){
					// Если запись уже не первая, добавляем разделитель
					if(i > 0)
						// Добавляем разделитель
						records.append(", ");
					// Выполняем установку записи
					records.append(extension.at(i));
				}
			}
		}
		// Добавляем заголовок сабпротокола
		this->header("Sec-WebSocket-Extensions", records);
	}
}
/**
 * key Метод генерации ключа
 * @return сгенерированный ключ
 */
const string awh::WCore::key() const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Создаём контейнер
		string nonce = "";
		// Адаптер для работы с случайным распределением
		random_device rd;
		// Резервируем память
		nonce.reserve(16);
		// Формируем равномерное распределение целых чисел в выходном инклюзивно-эксклюзивном диапазоне
		uniform_int_distribution <u_short> dist(0, 255);
		// Формируем бинарный ключ из случайных значений
		for(size_t c = 0; c < 16; c++)
			// Выполняем формирование ключа
			nonce += static_cast <char> (dist(rd));
		// Выполняем создание ключа
		result = base64_t().encode(nonce);
	// Выполняем прехват ошибки
	} catch(const exception & error) {
		// Выполняем повторно генерацию ключа
		result = this->key();
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * sha1 Метод генерации хэша SHA1 ключа
 * @return сгенерированный хэш ключа клиента
 */
const string awh::WCore::sha1() const noexcept {
	// Результат работы функции
	string result = "";
	// Если ключ клиента передан
	if(!this->_key.empty()){
		// Создаем контекст
		SHA_CTX ctx;
		// Выполняем инициализацию контекста
		SHA1_Init(&ctx);
		// Массив полученных значений
		u_char digest[20];
		// Формируем магический ключ
		const string text = this->_fmk->format("%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", this->_key.c_str());
		// Выполняем расчет суммы
		SHA1_Update(&ctx, text.c_str(), text.length());
		// Копируем полученные данные
		SHA1_Final(digest, &ctx);
		// Формируем ключ для клиента
		result = base64_t().encode(string((const char *) digest, 20));
	}
	// Выводим результат
	return result;
}
/**
 * extractExtension Метод извлечения системного расширения из заголовка
 * @param extension запись из которой нужно извлечь расширение
 * @return          результат извлечения
 */
bool awh::WCore::extractExtension(const string & extension) noexcept {
	// Результат работы функции
	bool result = false;
	// Если заголовок передан
	if(!extension.empty()){
		// Если нужно производить шифрование данных
		if((result = this->_crypted = this->_fmk->exists("permessage-encrypt=", extension))){
			
			cout << " !!!!!!!!!!!!!!!!!!!! " << this->_crypted << endl;
			
			// Определяем размер шифрования
			switch(static_cast <uint16_t> (::stoi(extension.substr(19)))){
				// Если шифрование произведено 128 битным ключём
				case 128: this->_hash.cipher(hash_t::cipher_t::AES128); break;
				// Если шифрование произведено 192 битным ключём
				case 192: this->_hash.cipher(hash_t::cipher_t::AES192); break;
				// Если шифрование произведено 256 битным ключём
				case 256: this->_hash.cipher(hash_t::cipher_t::AES256); break;
			}
		// Если клиент просит отключить перехват контекста сжатия для сервера
		} else if((result = this->_fmk->compare(extension, "server_no_context_takeover"))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Выполняем отключение перехвата контекста
					this->_server.takeover = false;
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Выполняем отключение перехвата контекста
					this->_server.takeover = false;
					// Выполняем отключение перехвата контекста
					this->_client.takeover = false;
				} break;
			}
		// Если клиент просит отключить перехват контекста сжатия для клиента
		} else if((result = this->_fmk->compare(extension, "client_no_context_takeover")))
			// Выполняем отключение перехвата контекста
			this->_client.takeover = false;
		// Если получены заголовки требующие сжимать передаваемые фреймы методом Deflate
		else if((result = this->_fmk->compare(extension, "permessage-deflate") || this->_fmk->compare(extension, "perframe-deflate"))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Устанавливаем требование выполнять декомпрессию полезной нагрузки
					this->_compress = compress_t::DEFLATE;
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Устанавливаем требование выполнять компрессию полезной нагрузки
					if((this->_compress != compress_t::DEFLATE) && (this->_compress != compress_t::ALL_COMPRESS))
						// Выполняем сброс типа компрессии
						this->_compress = compress_t::NONE;
					// Устанавливаем требование выполнять декомпрессию полезной нагрузки
					else this->_compress = compress_t::DEFLATE;
				} break;
			}
		// Если получены заголовки требующие сжимать передаваемые фреймы методом GZip
		} else if((result = this->_fmk->compare(extension, "permessage-gzip") || this->_fmk->compare(extension, "perframe-gzip"))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Устанавливаем требование выполнять декомпрессию полезной нагрузки
					this->_compress = compress_t::GZIP;
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Устанавливаем требование выполнять компрессию полезной нагрузки
					if((this->_compress != compress_t::GZIP) && (this->_compress != compress_t::ALL_COMPRESS))
						// Выполняем сброс типа компрессии
						this->_compress = compress_t::NONE;
					// Устанавливаем требование выполнять декомпрессию полезной нагрузки
					else this->_compress = compress_t::GZIP;
				} break;
			}
		// Если получены заголовки требующие сжимать передаваемые фреймы методом Brotli
		} else if((result = this->_fmk->compare(extension, "permessage-br") || this->_fmk->compare(extension, "perframe-br"))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Устанавливаем требование выполнять декомпрессию полезной нагрузки
					this->_compress = compress_t::BROTLI;
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Устанавливаем требование выполнять компрессию полезной нагрузки
					if((this->_compress != compress_t::BROTLI) && (this->_compress != compress_t::ALL_COMPRESS))
						// Выполняем сброс типа компрессии
						this->_compress = compress_t::NONE;
					// Устанавливаем требование выполнять декомпрессию полезной нагрузки
					else this->_compress = compress_t::BROTLI;
				} break;
			}
		// Если размер скользящего окна для клиента получен
		} else if((result = this->_fmk->exists("client_max_window_bits=", extension))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Устанавливаем размер скользящего окна
					this->_client.wbit = static_cast <short> (::stoi(extension.substr(23)));
					// Если размер скользящего окна установлен неправильно
					if((this->_client.wbit < GZIP_MIN_WBITS) || (this->_client.wbit > GZIP_MAX_WBITS))
						// Выводим сообщение об ошибке
						this->_log->print("Deflate max_window_bits for the client is set incorrectly", log_t::flag_t::WARNING);
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Устанавливаем размер скользящего окна
					if(this->_compress != compress_t::NONE){
						// Устанавливаем размер скользящего окна
						this->_client.wbit = static_cast <short> (::stoi(extension.substr(23)));
						// Если размер скользящего окна установлен слишком маленький
						if(this->_client.wbit < GZIP_MIN_WBITS)
							// Выполняем корректировку размера скользящего окна
							this->_client.wbit = GZIP_MIN_WBITS;
						// Если размер скользящего окна установлен слишком высоким
						else if(this->_client.wbit > GZIP_MAX_WBITS)
							// Выполняем корректировку размера скользящего окна
							this->_client.wbit = GZIP_MAX_WBITS;
					}
				} break;
			}
		// Если разрешено использовать максимальный размер скользящего окна для клиента
		} else if((result = this->_fmk->compare(extension, "client_max_window_bits"))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Устанавливаем максимальный размер скользящего окна
					this->_client.wbit = GZIP_MAX_WBITS;
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Устанавливаем максимальный размер скользящего окна
					if(this->_compress != compress_t::NONE)
						// Устанавливаем максимальный размер скользящего окна
						this->_client.wbit = GZIP_MAX_WBITS;
				} break;
			}
		// Если размер скользящего окна для сервера получен
		} else if((result = this->_fmk->exists("server_max_window_bits=", extension))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Устанавливаем размер скользящего окна
					this->_server.wbit = static_cast <short> (::stoi(extension.substr(23)));
					// Если размер скользящего окна установлен неправильно
					if((this->_server.wbit < GZIP_MIN_WBITS) || (this->_server.wbit > GZIP_MAX_WBITS))
						// Выводим сообщение об ошибке
						this->_log->print("Deflate max_window_bits for the server is set incorrectly", log_t::flag_t::WARNING);
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Устанавливаем размер скользящего окна
					if(this->_compress != compress_t::NONE){
						// Устанавливаем размер скользящего окна
						this->_server.wbit = static_cast <short> (::stoi(extension.substr(23)));
						// Если размер скользящего окна установлен слишком маленький
						if(this->_server.wbit < GZIP_MIN_WBITS)
							// Выполняем корректировку размера скользящего окна
							this->_server.wbit = GZIP_MIN_WBITS;
						// Если размер скользящего окна установлен слишком высоким
						else if(this->_server.wbit > GZIP_MAX_WBITS)
							// Выполняем корректировку размера скользящего окна
							this->_server.wbit = GZIP_MAX_WBITS;
					}
				} break;
			}
		// Если разрешено использовать максимальный размер скользящего окна для сервера
		} else if((result = this->_fmk->compare(extension, "server_max_window_bits"))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Устанавливаем максимальный размер скользящего окна
					this->_server.wbit = GZIP_MAX_WBITS;
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Устанавливаем максимальный размер скользящего окна
					if(this->_compress != compress_t::NONE)
						// Устанавливаем максимальный размер скользящего окна
						this->_server.wbit = GZIP_MAX_WBITS;
				} break;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * dump Метод получения бинарного дампа
 * @return бинарный дамп данных
 */
vector <char> awh::WCore::dump() const noexcept {
	// Результат работы функции
	vector <char> result;
	{
		// Длина строки, количество элементов
		size_t length = 0, count = 0;
		// Выполняем получение дампа основного класса
		const auto & dump = reinterpret_cast <http_t *> (const_cast <ws_core_t *> (this))->dump();
		// Получаем размер дамп бинарных данных модуля
		length = dump.size();
		// Устанавливаем размер дампа бинарных данных модуля
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Добавляем дамп бинарных данных модуля
		result.insert(result.end(), dump.begin(), dump.end());
		// Устанавливаем параметры партнёра клиента
		result.insert(result.end(), (const char *) &this->_client, (const char *) &this->_client + sizeof(this->_client));
		// Устанавливаем параметры партнёра сервера
		result.insert(result.end(), (const char *) &this->_server, (const char *) &this->_server + sizeof(this->_server));
		// Устанавливаем метод сжатия данных запроса/ответа
		result.insert(result.end(), (const char *) &this->_compress, (const char *) &this->_compress + sizeof(this->_compress));
		// Получаем количество расширений
		length = this->_extensions.size();
		// Устанавливаем количество расширений
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Если расширения установлены
		if(!this->_extensions.empty()){
			// Выполняем перебор всего списка расширений
			for(auto & extensions : this->_extensions){
				// Получаем количество расширений
				length = extensions.size();
				// Устанавливаем количество расширений
				result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
				// Выполняем перебор всего количества расширений
				for(auto & extension : extensions){
					// Получаем размер текущего расширения
					length = extension.size();
					// Устанавливаем количество расширений
					result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
					// Устанавливаем значение полученного расширения
					result.insert(result.end(), extension.begin(), extension.end());
				}
			}
		}
		// Получаем размер ключа клиента
		length = this->_key.size();
		// Устанавливаем размер ключа клиента
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Добавляем ключ клиента
		result.insert(result.end(), this->_key.begin(), this->_key.end());
		// Получаем количество выбранных сабпротоколов
		count = this->_selectedProtocols.size();
		// Устанавливаем количество выбранных сабпротоколов
		result.insert(result.end(), (const char *) &count, (const char *) &count + sizeof(count));
		// Выполняем перебор всех выбранных сабпротоколов
		for(auto & subprotocol : this->_selectedProtocols){
			// Получаем размер выбранному сабпротокола
			length = subprotocol.size();
			// Устанавливаем размер выбранному сабпротокола
			result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
			// Устанавливаем данные выбранному сабпротокола
			result.insert(result.end(), subprotocol.begin(), subprotocol.end());
		}
		// Получаем количество поддерживаемых сабпротоколов
		count = this->_supportedProtocols.size();
		// Устанавливаем количество поддерживаемых сабпротоколов
		result.insert(result.end(), (const char *) &count, (const char *) &count + sizeof(count));
		// Выполняем перебор всех поддерживаемых сабпротоколов
		for(auto & subprotocol : this->_supportedProtocols){
			// Получаем размер поддерживаемого сабпротокола
			length = subprotocol.size();
			// Устанавливаем размер поддерживаемого сабпротокола
			result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
			// Устанавливаем данные поддерживаемого сабпротокола
			result.insert(result.end(), subprotocol.begin(), subprotocol.end());
		}
	}
	// Выводим результат
	return result;
}
/**
 * dump Метод установки бинарного дампа
 * @param data бинарный дамп данных
 */
void awh::WCore::dump(const vector <char> & data) noexcept {
	// Если данные бинарного дампа переданы
	if(!data.empty()){
		// Длина строки, количество элементов и смещение в буфере
		size_t length = 0, count = 0, offset = 0;
		// Выполняем получение размера дампа бинарных данных модуля
		::memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Создаём бинарный буфер дампа
		vector <char> dump(length, 0);
		// Выполняем получение бинарного буфера дампа
		::memcpy((void *) dump.data(), data.data() + offset, length);
		// Выполняем смещение в буфере
		offset += length;
		// Выполняем установку бинарного буфера данных
		reinterpret_cast <http_t *> (const_cast <ws_core_t *> (this))->dump(dump);
		// Выполняем получение параметров партнёра клиента
		::memcpy((void *) &this->_client, data.data() + offset, sizeof(this->_client));
		// Выполняем смещение в буфере
		offset += sizeof(this->_client);
		// Выполняем получение параметров партнёра сервера
		::memcpy((void *) &this->_server, data.data() + offset, sizeof(this->_server));
		// Выполняем смещение в буфере
		offset += sizeof(this->_server);
		// Выполняем получение метода сжатия данных запроса/ответа
		::memcpy((void *) &this->_compress, data.data() + offset, sizeof(this->_compress));
		// Выполняем смещение в буфере
		offset += sizeof(this->_compress);
		// Выполняем получение количества расширений
		::memcpy((void *) &count, data.data() + offset, sizeof(count));
		// Выполняем смещение в буфере
		offset += sizeof(count);
		// Если количество расширений получено
		if(count > 0){
			// Выполняем инициализацию списка расширений
			this->_extensions.resize(count);
			// Выполняем перебор всех групп расширений
			for(size_t i = 0; i < this->_extensions.size(); i++){
				// Выполняем получение количества групп расширений
				::memcpy((void *) &count, data.data() + offset, sizeof(count));
				// Выполняем смещение в буфере
				offset += sizeof(count);
				// Выполняем инициализацию списка групп расширений
				this->_extensions.at(i).resize(count);
				// Выполняем перебор всех расширений
				for(size_t j = 0; j < this->_extensions.at(i).size(); j++){
					// Выполняем получение размера расширения
					::memcpy((void *) &length, data.data() + offset, sizeof(length));
					// Выполняем смещение в буфере
					offset += sizeof(length);
					// Выполняем инициализацию расширения
					this->_extensions.at(i).at(j).resize(length);
					// Выполняем копирование полученного расширения
					::memcpy((void *) this->_extensions.at(i).at(j).data(), this->_extensions.at(i).at(j).data() + offset, length);
					// Выполняем смещение в буфере
					offset += length;
				}
			}
		}
		// Выполняем получение размера ключа клиента
		::memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Выполняем выделение памяти для ключа клиента
		this->_key.resize(length, 0);
		// Выполняем получение ключа клиента
		::memcpy((void *) this->_key.data(), data.data() + offset, length);
		// Выполняем смещение в буфере
		offset += length;
		// Выполняем получение количества выбранных сабпротоколов
		::memcpy((void *) &count, data.data() + offset, sizeof(count));
		// Выполняем смещение в буфере
		offset += sizeof(count);
		// Выполняем сброс списка выбранных сабпротоколов
		this->_selectedProtocols.clear();
		// Выполняем последовательную загрузку всех выбранных сабпротоколов
		for(size_t i = 0; i < count; i++){
			// Выполняем получение размера поддерживаемого сабпротокола
			::memcpy((void *) &length, data.data() + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Выделяем память для поддерживаемого сабпротокола
			string subprotocol(length, 0);
			// Выполняем получение поддерживаемого сабпротокола
			::memcpy((void *) subprotocol.data(), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
			// Если сабпротокол получен, добавляем его в список
			if(!subprotocol.empty())
				// Выполняем установку списка выбранных сабпротоколов
				this->_selectedProtocols.emplace(std::move(subprotocol));
		}
		// Выполняем получение количества поддерживаемых сабпротоколов
		::memcpy((void *) &count, data.data() + offset, sizeof(count));
		// Выполняем смещение в буфере
		offset += sizeof(count);
		// Выполняем сброс списка поддерживаемых сабпротоколов
		this->_supportedProtocols.clear();
		// Выполняем последовательную загрузку всех поддерживаемых сабпротоколов
		for(size_t i = 0; i < count; i++){
			// Выполняем получение размера поддерживаемого сабпротокола
			::memcpy((void *) &length, data.data() + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Выделяем память для поддерживаемого сабпротокола
			string subprotocol(length, 0);
			// Выполняем получение поддерживаемого сабпротокола
			::memcpy((void *) subprotocol.data(), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
			// Если сабпротокол получен, добавляем его в список
			if(!subprotocol.empty())
				// Выполняем установку списка поддерживаемых сабпротоколов
				this->_supportedProtocols.emplace(std::move(subprotocol));
		}
	}
}
/**
 * clean Метод очистки собранных данных
 */
void awh::WCore::clean() noexcept {
	// Выполняем очистку родительских данных
	http_t::clear();
	// Выполняем сброс ключа клиента
	this->_key.clear();
	// Выполняем сброс списка выбранных сабпротоколов
	this->_selectedProtocols.clear();
	// Выполняем сброс списка поддерживаемых сабпротоколов
	this->_supportedProtocols.clear();
	// Выполняем сброс размера скользящего окна для клиента
	this->_client.wbit = GZIP_MAX_WBITS;
	// Выполняем сброс размера скользящего окна для сервера
	this->_server.wbit = GZIP_MAX_WBITS;
}
/**
 * compress Метод получения метода компрессии
 * @return метод компрессии сообщений
 */
awh::Http::compress_t awh::WCore::compress() const noexcept {
	// Выводим метод компрессии сообщений
	return this->_compress;
}
/**
 * compress Метод установки метода компрессии
 * @param compress метод компрессии сообщений
 */
void awh::WCore::compress(const compress_t compress) noexcept {
	// Устанавливаем метод компрессии сообщений
	this->_compress = compress;
}
/**
 * extensions Метод извлечения списка расширений
 * @return список поддерживаемых расширений
 */
const vector <vector <string>> & awh::WCore::extensions() const noexcept {
	// Выводим результат
	return this->_extensions;
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::WCore::extensions(const vector <vector <string>> & extensions) noexcept {
	// Если список поддерживаемых расширений переданы
	if(!extensions.empty())
		// Выполняем установку списка поддерживаемых расширений
		this->_extensions.assign(extensions.begin(), extensions.end());
	// Выполняем список поддерживаемых расширений
	else this->_extensions.clear();
}
/**
 * checkUpgrade Метод получения флага переключения протокола
 * @return флага переключения протокола
 */
bool awh::WCore::checkUpgrade() const noexcept {
	// Результат работы функции
	bool result = false;
	// Получаем значение заголовка Upgrade
	const string & upgrade = this->_web.header("upgrade");
	// Получаем значение заголовка Connection
	const string & connection = this->_web.header("connection");
	// Если заголовки расширений найдены
	if(!upgrade.empty() && !connection.empty()){
		// Переводим значение заголовка Connection в нижний регистр
		this->_fmk->transform(connection, fmk_t::transform_t::LOWER);
		// Если заголовки соответствуют
		result = (this->_fmk->compare(upgrade, "websocket") && this->_fmk->exists("upgrade", connection));
	}
	// Выводим результат
	return result;
}
/**
 * isHandshake Метод выполнения проверки рукопожатия
 * @param flag флаг выполняемого процесса
 * @return     результат выполнения проверки рукопожатия
 */
bool awh::WCore::isHandshake(const process_t flag) noexcept {
	// Результат работы функции
	bool result = (this->_state == state_t::HANDSHAKE);
	// Если рукопожатие не выполнено
	if(!result){
		// Версия протокола
		float version = 1.1f;
		// Определяем флаг выполняемого процесса
		switch(static_cast <uint8_t> (flag)){
			// Если нужно сформировать данные запроса
			case static_cast <uint8_t> (process_t::REQUEST):
				// Выполняем извлечение версии из параметров запроса
				version = this->request().version;
			break;
			// Если нужно сформировать данные ответа
			case static_cast <uint8_t> (process_t::RESPONSE):
				// Выполняем извлечение версии из параметров ответа
				version = this->response().version;
			break;
		}
		// Выполняем проверку на удачное завершение запроса
		result = (this->_stath == stath_t::GOOD);
		// Если результат удачный
		if(result)
			// Выполняем проверку версии протокола
			result = this->checkVer();
		// Если подключение не выполнено
		else return result;
		// Если результат удачный
		if(result){
			// Если версия протокола ниже 2.0
			if(version < 2.0f)
				// Проверяем произошло ли переключение протокола
				result = this->checkUpgrade();
		// Если версия протокола не соответствует
		} else {
			// Выводим сообщение об ошибке
			this->_log->print("Protocol version not supported", log_t::flag_t::CRITICAL);
			// Выходим из функции
			return result;
		}
		// Если версия протокола ниже 2.0
		if(version < 2.0f){
			// Если результат удачный
			if(result)
				// Проверяем ключ клиента
				result = this->checkKey();
			// Если протокол не был переключён
			else {
				// Выводим сообщение об ошибке
				this->_log->print("Protocol not upgraded", log_t::flag_t::CRITICAL);
				// Выходим из функции
				return result;
			}
		}
		// Если рукопожатие выполнено
		if(result)
			// Устанавливаем стейт рукопожатия
			this->_state = state_t::HANDSHAKE;
		// Если ключ клиента и сервера не согласованы, выводим сообщение об ошибке
		else this->_log->print("Client and server keys are inconsistent", log_t::flag_t::CRITICAL);
	}
	// Выводим результат
	return result;
}
/**
 * wbit Метод получения размер скользящего окна
 * @param hid тип модуля
 * @return    размер скользящего окна
 */
short awh::WCore::wbit(const web_t::hid_t hid) const noexcept {
	// Определяем флаг типа текущего модуля
	switch(static_cast <uint8_t> (hid)){
		// Если флаг текущего модуля соответствует клиенту
		case static_cast <uint8_t> (web_t::hid_t::CLIENT):
			// Выводим размер скользящего окна
			return this->_client.wbit;
		// Если флаг текущего модуля соответствует серверу
		case static_cast <uint8_t> (web_t::hid_t::SERVER):
			// Выводим размер скользящего окна
			return this->_server.wbit;
	}
	// Выводим результат
	return GZIP_MAX_WBITS;
}
/**
 * reject Метод создания отрицательного ответа
 * @param req объект параметров REST-ответа
 * @return    буфер данных ответа в бинарном виде
 */
vector <char> awh::WCore::reject(const web_t::res_t & res) const noexcept {
	// Выполняем очистку выбранного сабпротокола
	const_cast <ws_core_t *> (this)->_selectedProtocols.clear();
	// Выполняем генерацию сообщения ответа
	return http_t::reject(res);
}
/**
 * reject2 Метод создания отрицательного ответа (для протокола HTTP/2)
 * @param req объект параметров REST-ответа
 * @return    буфер данных ответа в бинарном виде
 */
vector <pair <string, string>> awh::WCore::reject2(const web_t::res_t & res) const noexcept {
	// Выполняем очистку выбранного сабпротокола
	const_cast <ws_core_t *> (this)->_selectedProtocols.clear();
	// Выполняем генерацию сообщения ответа
	return http_t::reject2(res);
}
/**
 * process Метод создания выполняемого процесса в бинарном виде
 * @param flag     флаг выполняемого процесса
 * @param provider параметры провайдера обмена сообщениями
 * @return         буфер данных в бинарном виде
 */
vector <char> awh::WCore::process(const process_t flag, const web_t::provider_t & provider) const noexcept {
	// Определяем флаг выполняемого процесса
	switch(static_cast <uint8_t> (flag)){
		// Если нужно сформировать данные запроса
		case static_cast <uint8_t> (process_t::REQUEST): {
			// Получаем объект ответа клиенту
			const web_t::req_t & req = static_cast <const web_t::req_t &> (provider);
			// Если параметры запроса получены
			if(!req.url.empty() && (req.version >= 2.0f ? req.method == web_t::method_t::CONNECT : req.method == web_t::method_t::GET)){
				// Генерируем ключ клиента
				const_cast <ws_core_t *> (this)->_key = this->key();
				// Удаляем заголовок апгрейд
				const_cast <ws_core_t *> (this)->rmHeader("Upgrade");
				// Удаляем заголовок протокола подключения
				const_cast <ws_core_t *> (this)->rmHeader(":protocol");
				// Удаляем заголовок подключения
				const_cast <ws_core_t *> (this)->rmHeader("Connection");
				// Удаляем заголовок ключ клиента
				const_cast <ws_core_t *> (this)->rmHeader("Sec-WebSocket-Key");
				// Добавляем заголовок апгрейд
				const_cast <ws_core_t *> (this)->header("Upgrade", "WebSocket");
				// Добавляем заголовок подключения
				const_cast <ws_core_t *> (this)->header("Connection", "Keep-Alive, Upgrade");
				// Добавляем заголовок ключ клиента
				const_cast <ws_core_t *> (this)->header("Sec-WebSocket-Key", this->_key);
				// Устанавливаем парарметры запроса
				this->_web.request(req);
			// Если данные переданы неверные
			} else {
				// Выводим сообщение, что данные переданы неверные
				this->_log->print("Address or request method for WebSocket-client is incorrect", log_t::flag_t::CRITICAL);
				// Выходим из функции
				return vector <char> ();
			}
		} break;
		// Если нужно сформировать данные ответа
		case static_cast <uint8_t> (process_t::RESPONSE): {
			// Получаем объект ответа клиенту
			const web_t::res_t & res = static_cast <const web_t::res_t &> (provider);
			// Если параметры запроса получены
			if(res.version >= 2.0f ? res.code != 101 : res.code != 200){
				// Удаляем статус ответа
				const_cast <ws_core_t *> (this)->rmHeader(":status");
				// Удаляем заголовок апгрейд
				const_cast <ws_core_t *> (this)->rmHeader("Upgrade");
				// Удаляем заголовок подключения
				const_cast <ws_core_t *> (this)->rmHeader("Connection");
				// Удаляем заголовок хеша ключа
				const_cast <ws_core_t *> (this)->rmHeader("Sec-WebSocket-Accept");
				// Если ответ сервера положительный
				if(res.version >= 2.0f ? res.code == 200 : res.code == 101){
					// Добавляем заголовок подключения
					const_cast <ws_core_t *> (this)->header("Connection", "Upgrade");
					// Добавляем заголовок апгрейд
					const_cast <ws_core_t *> (this)->header("Upgrade", "WebSocket");
				}
				// Если версия протокола ниже 2.0
				if((res.version < 2.0f) && (res.code == 101)){
					// Выполняем генерацию хеша ключа
					const string & sha1 = this->sha1();
					// Если SHA1-ключ не сгенерирован
					if(sha1.empty()){
						// Если ключ клиента и сервера не согласованы, выводим сообщение об ошибке
						this->_log->print("SHA1 key could not be generated, no further work possiblet", log_t::flag_t::CRITICAL);
						// Выходим из функции
						return vector <char> ();
					}
					// Добавляем заголовок хеша ключа
					const_cast <ws_core_t *> (this)->header("Sec-WebSocket-Accept", sha1.c_str());
				}
				// Устанавливаем параметры ответа
				this->_web.response(res);
			// Если данные переданы неверные
			} else {
				// Выводим сообщение, что данные переданы неверные
				this->_log->print("WebSocket-server response code set incorrectly", log_t::flag_t::CRITICAL);
				// Выходим из функции
				return vector <char> ();
			}
		} break;
	}
	// Выполняем инициализацию
	const_cast <ws_core_t *> (this)->init(flag);
	// Выводим результат
	return http_t::process(flag, provider);
}
/**
 * process2 Метод создания выполняемого процесса в бинарном виде (для протокола HTTP/2)
 * @param flag     флаг выполняемого процесса
 * @param provider параметры провайдера обмена сообщениями
 * @return         буфер данных в бинарном виде
 */
vector <pair <string, string>> awh::WCore::process2(const process_t flag, const web_t::provider_t & provider) const noexcept {
	// Определяем флаг выполняемого процесса
	switch(static_cast <uint8_t> (flag)){
		// Если нужно сформировать данные запроса
		case static_cast <uint8_t> (process_t::REQUEST): {
			// Получаем объект ответа клиенту
			const web_t::req_t & req = static_cast <const web_t::req_t &> (provider);
			// Если параметры запроса получены
			if(!req.url.empty() && (req.method == web_t::method_t::CONNECT)){
				// Удаляем заголовок апгрейд
				const_cast <ws_core_t *> (this)->rmHeader("Upgrade");
				// Удаляем заголовок протокола подключения
				const_cast <ws_core_t *> (this)->rmHeader(":protocol");
				// Удаляем заголовок подключения
				const_cast <ws_core_t *> (this)->rmHeader("Connection");
				// Удаляем заголовок ключ клиента
				const_cast <ws_core_t *> (this)->rmHeader("Sec-WebSocket-Key");
				// Добавляем заголовок протокола подключения
				const_cast <ws_core_t *> (this)->header(":protocol", "websocket");
				// Устанавливаем парарметры запроса
				this->_web.request(req);
			// Если данные переданы неверные
			} else {
				// Выводим сообщение, что данные переданы неверные
				this->_log->print("Address or request method for WebSocket-client is incorrect", log_t::flag_t::CRITICAL);
				// Выходим из функции
				return vector <pair <string, string>> ();
			}
		} break;
		// Если нужно сформировать данные ответа
		case static_cast <uint8_t> (process_t::RESPONSE): {
			// Получаем объект ответа клиенту
			const web_t::res_t & res = static_cast <const web_t::res_t &> (provider);
			// Если параметры запроса получены
			if(res.code != 101){
				// Удаляем заголовок апгрейд
				const_cast <ws_core_t *> (this)->rmHeader("Upgrade");
				// Удаляем статус ответа
				const_cast <ws_core_t *> (this)->rmHeader(":status");
				// Удаляем заголовок подключения
				const_cast <ws_core_t *> (this)->rmHeader("Connection");
				// Удаляем заголовок хеша ключа
				const_cast <ws_core_t *> (this)->rmHeader("Sec-WebSocket-Accept");
				// Устанавливаем параметры ответа
				this->_web.response(res);
			// Если данные переданы неверные
			} else {
				// Выводим сообщение, что данные переданы неверные
				this->_log->print("WebSocket-server response code set incorrectly", log_t::flag_t::CRITICAL);
				// Выходим из функции
				return vector <pair <string, string>> ();
			}
		} break;
	}
	// Выполняем инициализацию
	const_cast <ws_core_t *> (this)->init(flag);
	// Выводим результат
	return http_t::process2(flag, provider);
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::WCore::subprotocol(const string & subprotocol) noexcept {
	// Если сабпротокол передан
	if(!subprotocol.empty())
		// Выполняем установку поддерживаемого сабпротокола
		this->_supportedProtocols.emplace(subprotocol);
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @return список выбранных сабпротоколов
 */
const set <string> & awh::WCore::subprotocols() const noexcept {
	// Выводим список выбранных сабпротоколов
	return this->_selectedProtocols;
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::WCore::subprotocols(const set <string> & subprotocols) noexcept {
	// Если список сабпротоколов получен
	if(!subprotocols.empty())
		// Выполняем установку списка поддерживаемых сабпротоколов
		this->_supportedProtocols = subprotocols;
}
/**
 * takeover Метод получения флага переиспользования контекста компрессии
 * @param hid тип текущего модуля
 * @return    флаг запрета переиспользования контекста компрессии
 */
bool awh::WCore::takeover(const web_t::hid_t hid) const noexcept {
	// Определяем флаг типа текущего модуля
	switch(static_cast <uint8_t> (hid)){
		// Если флаг текущего модуля соответствует клиенту
		case static_cast <uint8_t> (web_t::hid_t::CLIENT):
			// Выводим флаг переиспользования компрессии
			return this->_client.takeover;
		// Если флаг текущего модуля соответствует серверу
		case static_cast <uint8_t> (web_t::hid_t::SERVER):
			// Выводим флаг переиспользования компрессии
			return this->_server.takeover;
	}
	// Выводим результат
	return false;
}
/**
 * takeover Метод установки флага переиспользования контекста компрессии
 * @param hid  тип текущего модуля
 * @param flag флаг запрета переиспользования контекста компрессии
 */
void awh::WCore::takeover(const web_t::hid_t hid, const bool flag) noexcept {
	// Определяем флаг типа текущего модуля
	switch(static_cast <uint8_t> (hid)){
		// Если флаг текущего модуля соответствует клиенту
		case static_cast <uint8_t> (web_t::hid_t::CLIENT):
			// Устанавливаем флаг переиспользования компрессии
			this->_client.takeover = flag;
		break;
		// Если флаг текущего модуля соответствует серверу
		case static_cast <uint8_t> (web_t::hid_t::SERVER):
			// Устанавливаем флаг переиспользования компрессии
			this->_server.takeover = flag;
		break;
	}
}
