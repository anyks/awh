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
	// Определяем флаг выполняемого процесса
	switch(static_cast <uint8_t> (flag)){
		// Если нужно сформировать данные запроса
		case static_cast <uint8_t> (process_t::REQUEST): {
			// Удаляем заголовок Accept
			this->rm(suite_t::HEADER, "Accept");
			// Удаляем заголовок отключения кеширования
			this->rm(suite_t::HEADER, "Pragma");
			// Удаляем заголовок отключения кеширования
			this->rm(suite_t::HEADER, "Cache-Control");
			// Удаляем заголовок типа запроса
			this->rm(suite_t::HEADER, "Sec-Fetch-Mode");
			// Удаляем заголовок места назначения запроса
			this->rm(suite_t::HEADER, "Sec-Fetch-Dest");
			// Удаляем заголовок требования сжимать содержимое ответов
			this->rm(suite_t::HEADER, "Accept-Encoding");
			// Удаляем заголовок поддерживаемых языков
			this->rm(suite_t::HEADER, "Accept-Language");
			// Удаляем заголовок версии WebSocket
			this->rm(suite_t::HEADER, "Sec-WebSocket-Version");
			// Если список поддерживаемых сабпротоколов установлен
			if(!this->is(suite_t::HEADER, "Sec-WebSocket-Protocol") && !this->_supportedProtocols.empty()){
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
						const_cast <std::unordered_multimap <string, string> *> (&headers)->insert({{"Sec-WebSocket-Protocol", subprotocol}});
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
			this->header("Sec-Fetch-Mode", "websocket");
			// Добавляем заголовок места назначения запроса
			this->header("Sec-Fetch-Dest", "websocket");
			// Добавляем заголовок поддерживаемых языков
			this->header("Accept-Language", HTTP_HEADER_ACCEPTLANGUAGE);
			// Добавляем заголовок версии WebSocket
			this->header("Sec-WebSocket-Version", std::to_string(WS_VERSION));
			// Если компрессор уже выбран
			if(http_t::_compressors.selected != compressor_t::NONE){
				// Определяем метод сжатия который поддерживает клиент
				switch(static_cast <uint8_t> (http_t::_compressors.selected)){
					// Если клиент поддерживает методот сжатия LZ4
					case static_cast <uint8_t> (compressor_t::LZ4):
						// Добавляем заголовок требования сжимать содержимое ответов
						this->header("Accept-Encoding", "lz4");
					break;
					// Если клиент поддерживает методот сжатия Zstandard
					case static_cast <uint8_t> (compressor_t::ZSTD):
						// Добавляем заголовок требования сжимать содержимое ответов
						this->header("Accept-Encoding", "zstd");
					break;
					// Если клиент поддерживает методот сжатия LZma
					case static_cast <uint8_t> (compressor_t::LZMA):
						// Добавляем заголовок требования сжимать содержимое ответов
						this->header("Accept-Encoding", "xz");
					break;
					// Если клиент поддерживает методот сжатия Brotli
					case static_cast <uint8_t> (compressor_t::BROTLI):
						// Добавляем заголовок требования сжимать содержимое ответов
						this->header("Accept-Encoding", "br");
					break;
					// Если клиент поддерживает методот сжатия BZip2
					case static_cast <uint8_t> (compressor_t::BZIP2):
						// Добавляем заголовок требования сжимать содержимое ответов
						this->header("Accept-Encoding", "bzip2");
					break;
					// Если клиент поддерживает методот сжатия GZip
					case static_cast <uint8_t> (compressor_t::GZIP):
						// Добавляем заголовок требования сжимать содержимое ответов
						this->header("Accept-Encoding", "gzip");
					break;
					// Если клиент поддерживает методот сжатия Deflate
					case static_cast <uint8_t> (compressor_t::DEFLATE):
						// Добавляем заголовок требования сжимать содержимое ответов
						this->header("Accept-Encoding", "deflate");
					break;
				}
			// Если список компрессоров установлен
			} else if(!http_t::_compressors.supports.empty()) {
				// Строка со списком компрессоров
				string compressors = "";
				// Выполняем перебор всего списка компрессоров
				for(auto i = http_t::_compressors.supports.rbegin(); i != http_t::_compressors.supports.rend(); ++i){
					// Если список компрессоров уже не пустой
					if(!compressors.empty())
						// Выполняем добавление разделителя
						compressors.append(", ");
					// Определяем метод сжатия который поддерживает клиент
					switch(static_cast <uint8_t> (i->second)){
						// Если клиент поддерживает методот сжатия LZ4
						case static_cast <uint8_t> (compressor_t::LZ4):
							// Добавляем компрессор в список
							compressors.append("lz4");
						break;
						// Если клиент поддерживает методот сжатия Zstandard
						case static_cast <uint8_t> (compressor_t::ZSTD):
							// Добавляем компрессор в список
							compressors.append("zstd");
						break;
						// Если клиент поддерживает методот сжатия LZma
						case static_cast <uint8_t> (compressor_t::LZMA):
							// Добавляем компрессор в список
							compressors.append("xz");
						break;
						// Если клиент поддерживает методот сжатия Brotli
						case static_cast <uint8_t> (compressor_t::BROTLI):
							// Добавляем компрессор в список
							compressors.append("br");
						break;
						// Если клиент поддерживает методот сжатия BZip2
						case static_cast <uint8_t> (compressor_t::BZIP2):
							// Добавляем компрессор в список
							compressors.append("bzip2");
						break;
						// Если клиент поддерживает методот сжатия GZip
						case static_cast <uint8_t> (compressor_t::GZIP):
							// Добавляем компрессор в список
							compressors.append("gzip");
						break;
						// Если клиент поддерживает методот сжатия Deflate
						case static_cast <uint8_t> (compressor_t::DEFLATE):
							// Добавляем компрессор в список
							compressors.append("deflate");
						break;
					}
				}
				// Если список компрессоров получен
				if(!compressors.empty())
					// Добавляем заголовок требования сжимать содержимое ответов
					this->header("Accept-Encoding", compressors);
			}
		} break;
		// Если нужно сформировать данные ответа
		case static_cast <uint8_t> (process_t::RESPONSE): {
			// Добавляем в чёрный список заголовок Content-Type
			this->black("Content-Type");
			// Получаем объект ответа клиенту
			const web_t::res_t & res = this->_web.response();
			// Если ответ сервера положительный
			if(res.version >= 2.0f ? res.code == 200 : res.code == 101){
				// Выполняем применение расширений
				this->applyExtensions(flag);
				// Добавляем в чёрный список заголовок Content-Encoding
				this->black("Content-Encoding");
				// Добавляем в чёрный список заголовок X-AWH-Encryption
				this->black("X-AWH-Encryption");
			}
			// Если список выбранных сабпротоколов установлен
			if(!this->is(suite_t::HEADER, "Sec-WebSocket-Protocol") && !this->_selectedProtocols.empty()){
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
						const_cast <std::unordered_multimap <string, string> *> (&headers)->insert({{"Sec-WebSocket-Protocol", subprotocol}});
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
	// Если заголовки расширений не установлены
	if(!this->is(suite_t::HEADER, "Sec-WebSocket-Extensions")){
		// Список поддверживаемых расширений
		vector <vector <string>> extensions;
		// Определяем тип активной компрессии
		switch(static_cast <uint8_t> (this->_compressors.selected)){
			// Если метод компрессии выбран LZ4
			case static_cast <uint8_t> (compressor_t::LZ4):
				// Добавляем метод сжатия LZ4
				extensions.push_back({"permessage-lz4"});
			break;
			// Если метод компрессии выбран Zstandard
			case static_cast <uint8_t> (compressor_t::ZSTD):
				// Добавляем метод сжатия Zstandard
				extensions.push_back({"permessage-zstd"});
			break;
			// Если метод компрессии выбран LZma
			case static_cast <uint8_t> (compressor_t::LZMA):
				// Добавляем метод сжатия LZma
				extensions.push_back({"permessage-xz"});
			break;
			// Если метод компрессии выбран Brotli
			case static_cast <uint8_t> (compressor_t::BROTLI):
				// Добавляем метод сжатия Brotli
				extensions.push_back({"permessage-br"});
			break;
			// Если метод компрессии выбран BZip2
			case static_cast <uint8_t> (compressor_t::BZIP2):
				// Добавляем метод сжатия BZip2
				extensions.push_back({"permessage-bzip2"});
			break;
			// Если метод компрессии выбран GZip
			case static_cast <uint8_t> (compressor_t::GZIP):
				// Добавляем метод сжатия GZip
				extensions.push_back({"permessage-gzip"});
			break;
			// Если метод компрессии выбран Deflate
			case static_cast <uint8_t> (compressor_t::DEFLATE): {
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
						if(this->_client.wbit == static_cast <int16_t> (GZIP_MAX_WBITS))
							// Добавляем максимальный размер скользящего окна
							extensions.push_back({"client_max_window_bits"});
						// Выполняем установку указанного размера скользящего окна
						else extensions.push_back({this->_fmk->format("client_max_window_bits=%u", this->_client.wbit)});
						// Если размер скользящего окна сервера установлен как максимальный
						if(this->_server.wbit == static_cast <int16_t> (GZIP_MAX_WBITS))
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
		if(this->_encryption)
			// Выполняем установку указанного метода шифрования
			extensions.push_back({this->_fmk->format("permessage-encrypt=%u", static_cast <uint16_t> (this->_hash.cipher()))});
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
		// Резервируем память
		nonce.reserve(16);
		// Адаптер для работы с случайным распределением
		std::random_device randev;
		// Формируем равномерное распределение целых чисел в выходном инклюзивно-эксклюзивном диапазоне
		std::uniform_int_distribution <uint16_t> dist(0, 255);
		// Формируем бинарный ключ из случайных значений
		for(size_t c = 0; c < 16; c++)
			// Выполняем формирование ключа
			nonce += static_cast <char> (dist(randev));
		// Выполняем создание ключа
		result = base64_t().encode(nonce);
	// Выполняем прехват ошибки
	} catch(const exception & error) {
		// Выводим сообщение об ошибке
		this->_log->print("WebSocket Key: %s", log_t::flag_t::WARNING, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		#endif
		// Выполняем повторно генерацию ключа
		result = this->key();
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
		result = base64_t().encode(string(reinterpret_cast <const char *> (digest), 20));
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
		if((result = this->_encryption = this->_fmk->exists("permessage-encrypt=", extension))){
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
					this->_compressors.selected = compressor_t::DEFLATE;
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
					if(this->_fmk->findInMap(compressor_t::DEFLATE, this->_compressors.supports) != this->_compressors.supports.end())
						// Устанавливаем флаг метода компрессии
						this->_compressors.selected = compressor_t::DEFLATE;
					// Выполняем сброс типа компрессии
					else this->_compressors.selected = compressor_t::NONE;
				} break;
			}
		// Если получены заголовки требующие сжимать передаваемые фреймы методом LZ4
		} else if((result = this->_fmk->compare(extension, "permessage-lz4") || this->_fmk->compare(extension, "perframe-lz4"))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Устанавливаем требование выполнять декомпрессию полезной нагрузки
					this->_compressors.selected = compressor_t::LZ4;
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
					if(this->_fmk->findInMap(compressor_t::LZ4, this->_compressors.supports) != this->_compressors.supports.end())
						// Устанавливаем флаг метода компрессии
						this->_compressors.selected = compressor_t::LZ4;
					// Выполняем сброс типа компрессии
					else this->_compressors.selected = compressor_t::NONE;
				} break;
			}
		// Если получены заголовки требующие сжимать передаваемые фреймы методом Zstandard
		} else if((result = this->_fmk->compare(extension, "permessage-zstd") || this->_fmk->compare(extension, "perframe-zstd"))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Устанавливаем требование выполнять декомпрессию полезной нагрузки
					this->_compressors.selected = compressor_t::ZSTD;
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
					if(this->_fmk->findInMap(compressor_t::ZSTD, this->_compressors.supports) != this->_compressors.supports.end())
						// Устанавливаем флаг метода компрессии
						this->_compressors.selected = compressor_t::ZSTD;
					// Выполняем сброс типа компрессии
					else this->_compressors.selected = compressor_t::NONE;
				} break;
			}
		// Если получены заголовки требующие сжимать передаваемые фреймы методом LZma
		} else if((result = this->_fmk->compare(extension, "permessage-xz") || this->_fmk->compare(extension, "perframe-xz"))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Устанавливаем требование выполнять декомпрессию полезной нагрузки
					this->_compressors.selected = compressor_t::LZMA;
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
					if(this->_fmk->findInMap(compressor_t::LZMA, this->_compressors.supports) != this->_compressors.supports.end())
						// Устанавливаем флаг метода компрессии
						this->_compressors.selected = compressor_t::LZMA;
					// Выполняем сброс типа компрессии
					else this->_compressors.selected = compressor_t::NONE;
				} break;
			}
		// Если получены заголовки требующие сжимать передаваемые фреймы методом Brotli
		} else if((result = this->_fmk->compare(extension, "permessage-br") || this->_fmk->compare(extension, "perframe-br"))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Устанавливаем требование выполнять декомпрессию полезной нагрузки
					this->_compressors.selected = compressor_t::BROTLI;
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
					if(this->_fmk->findInMap(compressor_t::BROTLI, this->_compressors.supports) != this->_compressors.supports.end())
						// Устанавливаем флаг метода компрессии
						this->_compressors.selected = compressor_t::BROTLI;
					// Выполняем сброс типа компрессии
					else this->_compressors.selected = compressor_t::NONE;
				} break;
			}
		// Если получены заголовки требующие сжимать передаваемые фреймы методом BZip2
		} else if((result = this->_fmk->compare(extension, "permessage-bzip2") || this->_fmk->compare(extension, "perframe-bzip2"))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Устанавливаем требование выполнять декомпрессию полезной нагрузки
					this->_compressors.selected = compressor_t::BZIP2;
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
					if(this->_fmk->findInMap(compressor_t::BZIP2, this->_compressors.supports) != this->_compressors.supports.end())
						// Устанавливаем флаг метода компрессии
						this->_compressors.selected = compressor_t::BZIP2;
					// Выполняем сброс типа компрессии
					else this->_compressors.selected = compressor_t::NONE;
				} break;
			}
		// Если получены заголовки требующие сжимать передаваемые фреймы методом GZip
		} else if((result = this->_fmk->compare(extension, "permessage-gzip") || this->_fmk->compare(extension, "perframe-gzip"))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Устанавливаем требование выполнять декомпрессию полезной нагрузки
					this->_compressors.selected = compressor_t::GZIP;
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Выполняем поиск в списке доступных компрессоров запрашиваемый компрессор
					if(this->_fmk->findInMap(compressor_t::GZIP, this->_compressors.supports) != this->_compressors.supports.end())
						// Устанавливаем флаг метода компрессии
						this->_compressors.selected = compressor_t::GZIP;
					// Выполняем сброс типа компрессии
					else this->_compressors.selected = compressor_t::NONE;
				} break;
			}
		// Если размер скользящего окна для клиента получен
		} else if((result = this->_fmk->exists("client_max_window_bits=", extension))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Устанавливаем размер скользящего окна
					this->_client.wbit = static_cast <int16_t> (::stoi(extension.substr(23)));
					// Если размер скользящего окна установлен неправильно
					if((this->_client.wbit < GZIP_MIN_WBITS) || (this->_client.wbit > GZIP_MAX_WBITS)){
						// Выводим сообщение об ошибке
						this->_log->print("Deflate max_window_bits for the client is set incorrectly", log_t::flag_t::WARNING);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callbacks.is("error"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Deflate max_window_bits for the client is set incorrectly");
					}
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Устанавливаем размер скользящего окна
					this->_client.wbit = static_cast <int16_t> (::stoi(extension.substr(23)));
					// Если размер скользящего окна установлен слишком маленький
					if(this->_client.wbit < GZIP_MIN_WBITS)
						// Выполняем корректировку размера скользящего окна
						this->_client.wbit = GZIP_MIN_WBITS;
					// Если размер скользящего окна установлен слишком высоким
					else if(this->_client.wbit > GZIP_MAX_WBITS)
						// Выполняем корректировку размера скользящего окна
						this->_client.wbit = GZIP_MAX_WBITS;
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
				case static_cast <uint8_t> (web_t::hid_t::SERVER):
					// Устанавливаем максимальный размер скользящего окна
					this->_client.wbit = GZIP_MAX_WBITS;
				break;
			}
		// Если размер скользящего окна для сервера получен
		} else if((result = this->_fmk->exists("server_max_window_bits=", extension))) {
			// Определяем флаг типа текущего модуля
			switch(static_cast <uint8_t> (this->_web.hid())){
				// Если флаг текущего модуля соответствует клиенту
				case static_cast <uint8_t> (web_t::hid_t::CLIENT):
					// Устанавливаем размер скользящего окна
					this->_server.wbit = static_cast <int16_t> (::stoi(extension.substr(23)));
					// Если размер скользящего окна установлен неправильно
					if((this->_server.wbit < GZIP_MIN_WBITS) || (this->_server.wbit > GZIP_MAX_WBITS)){
						// Выводим сообщение об ошибке
						this->_log->print("Deflate max_window_bits for the server is set incorrectly", log_t::flag_t::WARNING);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callbacks.is("error"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Deflate max_window_bits for the server is set incorrectly");
					}
				break;
				// Если флаг текущего модуля соответствует серверу
				case static_cast <uint8_t> (web_t::hid_t::SERVER): {
					// Устанавливаем размер скользящего окна
					this->_server.wbit = static_cast <int16_t> (::stoi(extension.substr(23)));
					// Если размер скользящего окна установлен слишком маленький
					if(this->_server.wbit < GZIP_MIN_WBITS)
						// Выполняем корректировку размера скользящего окна
						this->_server.wbit = GZIP_MIN_WBITS;
					// Если размер скользящего окна установлен слишком высоким
					else if(this->_server.wbit > GZIP_MAX_WBITS)
						// Выполняем корректировку размера скользящего окна
						this->_server.wbit = GZIP_MAX_WBITS;
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
				case static_cast <uint8_t> (web_t::hid_t::SERVER):
					// Устанавливаем максимальный размер скользящего окна
					this->_server.wbit = GZIP_MAX_WBITS;
				break;
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
		// Устанавливаем параметры партнёра клиента
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_client), reinterpret_cast <const char *> (&this->_client) + sizeof(this->_client));
		// Устанавливаем параметры партнёра сервера
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_server), reinterpret_cast <const char *> (&this->_server) + sizeof(this->_server));
		// Устанавливаем метод компрессии отправляемых данных
		result.insert(result.end(), reinterpret_cast <const char *> (&this->_compressors.selected), reinterpret_cast <const char *> (&this->_compressors.selected) + sizeof(this->_compressors.selected));
		// Получаем количество поддерживаемых компрессоров
		count = this->_compressors.supports.size();
		// Устанавливаем количество поддерживаемых компрессоров
		result.insert(result.end(), reinterpret_cast <const char *> (&count), reinterpret_cast <const char *> (&count) + sizeof(count));
		// Если список поддерживаемых компрессоров не пустой
		if(!this->_compressors.supports.empty()){
			// Выполняем перебор всех поддерживаемых компрессоров
			for(auto & compressor : this->_compressors.supports){
				// Выполняем установку веска компрессора
				result.insert(result.end(), reinterpret_cast <const char *> (&compressor.first), reinterpret_cast <const char *> (&compressor.first) + sizeof(compressor.first));
				// Выполняем установку идентификатора компрессора
				result.insert(result.end(), reinterpret_cast <const char *> (&compressor.second), reinterpret_cast <const char *> (&compressor.second) + sizeof(compressor.second));
			}
		}
		// Получаем количество расширений
		count = this->_extensions.size();
		// Устанавливаем количество расширений
		result.insert(result.end(), reinterpret_cast <const char *> (&count), reinterpret_cast <const char *> (&count) + sizeof(count));
		// Если расширения установлены
		if(!this->_extensions.empty()){
			// Выполняем перебор всего списка расширений
			for(auto & extensions : this->_extensions){
				// Получаем количество расширений
				count = extensions.size();
				// Устанавливаем количество расширений
				result.insert(result.end(), reinterpret_cast <const char *> (&count), reinterpret_cast <const char *> (&count) + sizeof(count));
				// Выполняем перебор всего количества расширений
				for(auto & extension : extensions){
					// Получаем размер текущего расширения
					length = extension.size();
					// Устанавливаем количество расширений
					result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
					// Устанавливаем значение полученного расширения
					result.insert(result.end(), extension.begin(), extension.end());
				}
			}
		}
		// Получаем размер ключа клиента
		length = this->_key.size();
		// Устанавливаем размер ключа клиента
		result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
		// Добавляем ключ клиента
		result.insert(result.end(), this->_key.begin(), this->_key.end());
		// Получаем количество выбранных сабпротоколов
		count = this->_selectedProtocols.size();
		// Устанавливаем количество выбранных сабпротоколов
		result.insert(result.end(), reinterpret_cast <const char *> (&count), reinterpret_cast <const char *> (&count) + sizeof(count));
		// Выполняем перебор всех выбранных сабпротоколов
		for(auto & subprotocol : this->_selectedProtocols){
			// Получаем размер выбранному сабпротокола
			length = subprotocol.size();
			// Устанавливаем размер выбранному сабпротокола
			result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
			// Устанавливаем данные выбранному сабпротокола
			result.insert(result.end(), subprotocol.begin(), subprotocol.end());
		}
		// Получаем количество поддерживаемых сабпротоколов
		count = this->_supportedProtocols.size();
		// Устанавливаем количество поддерживаемых сабпротоколов
		result.insert(result.end(), reinterpret_cast <const char *> (&count), reinterpret_cast <const char *> (&count) + sizeof(count));
		// Выполняем перебор всех поддерживаемых сабпротоколов
		for(auto & subprotocol : this->_supportedProtocols){
			// Получаем размер поддерживаемого сабпротокола
			length = subprotocol.size();
			// Устанавливаем размер поддерживаемого сабпротокола
			result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
			// Устанавливаем данные поддерживаемого сабпротокола
			result.insert(result.end(), subprotocol.begin(), subprotocol.end());
		}{
			// Выполняем получение дампа основного класса
			const auto & dump = http_t::dump();
			// Получаем размер дамп бинарных данных модуля
			length = dump.size();
			// Устанавливаем размер дампа бинарных данных модуля
			result.insert(result.end(), reinterpret_cast <const char *> (&length), reinterpret_cast <const char *> (&length) + sizeof(length));
			// Добавляем дамп бинарных данных модуля
			result.insert(result.end(), dump.begin(), dump.end());
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
		// Выполняем получение параметров партнёра клиента
		::memcpy(reinterpret_cast <void *> (&this->_client), data.data() + offset, sizeof(this->_client));
		// Выполняем смещение в буфере
		offset += sizeof(this->_client);
		// Выполняем получение параметров партнёра сервера
		::memcpy(reinterpret_cast <void *> (&this->_server), data.data() + offset, sizeof(this->_server));
		// Выполняем смещение в буфере
		offset += sizeof(this->_server);
		// Выполняем получение метода компрессии отправляемых данных
		::memcpy(reinterpret_cast <void *> (&this->_compressors.selected), data.data() + offset, sizeof(this->_compressors.selected));
		// Выполняем смещение в буфере
		offset += sizeof(this->_compressors.selected);
		// Выполняем получение количества поддерживаемых компрессоров
		::memcpy(reinterpret_cast <void *> (&count), data.data() + offset, sizeof(count));
		// Выполняем смещение в буфере
		offset += sizeof(count);
		// Выполняем очистку списку поддерживаемых компрессоров
		this->_compressors.supports.clear();
		// Если количество компрессоров полученно
		if(count > 0){
			// Выполняем последовательную установку всех компрессоров
			for(size_t i = 0; i < count; i++){
				// Вес компрессора
				float weight = .0f;
				// Идентификатор компрессора
				compressor_t compressor = compressor_t::NONE;
				// Выполняем получение веса компрессора
				::memcpy(reinterpret_cast <void *> (&weight), data.data() + offset, sizeof(weight));
				// Выполняем смещение в буфере
				offset += sizeof(weight);
				// Выполняем получение идентификатора компрессора
				::memcpy(reinterpret_cast <void *> (&compressor), data.data() + offset, sizeof(compressor));
				// Выполняем смещение в буфере
				offset += sizeof(compressor);
				// Выполняем установку метода компрессора
				this->_compressors.supports.emplace(weight, compressor);
			}
		}
		// Выполняем получение количества расширений
		::memcpy(reinterpret_cast <void *> (&count), data.data() + offset, sizeof(count));
		// Выполняем смещение в буфере
		offset += sizeof(count);
		// Если количество расширений получено
		if(count > 0){
			// Выполняем инициализацию списка расширений
			this->_extensions.resize(count);
			// Выполняем перебор всех групп расширений
			for(size_t i = 0; i < this->_extensions.size(); i++){
				// Выполняем получение количества групп расширений
				::memcpy(reinterpret_cast <void *> (&count), data.data() + offset, sizeof(count));
				// Выполняем смещение в буфере
				offset += sizeof(count);
				// Выполняем инициализацию списка групп расширений
				this->_extensions.at(i).resize(count);
				// Выполняем перебор всех расширений
				for(size_t j = 0; j < this->_extensions.at(i).size(); j++){
					// Выполняем получение размера расширения
					::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
					// Выполняем смещение в буфере
					offset += sizeof(length);
					// Если размер получен
					if(length > 0){
						// Выполняем инициализацию расширения
						this->_extensions.at(i).at(j).resize(length);
						// Выполняем копирование полученного расширения
						::memcpy(reinterpret_cast <void *> (this->_extensions.at(i).at(j).data()), this->_extensions.at(i).at(j).data() + offset, length);
						// Выполняем смещение в буфере
						offset += length;
					}
				}
			}
		}
		// Выполняем получение размера ключа клиента
		::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Если размер получен
		if(length > 0){
			// Выполняем выделение памяти для ключа клиента
			this->_key.resize(length, 0);
			// Выполняем получение ключа клиента
			::memcpy(reinterpret_cast <void *> (this->_key.data()), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
		}
		// Выполняем получение количества выбранных сабпротоколов
		::memcpy(reinterpret_cast <void *> (&count), data.data() + offset, sizeof(count));
		// Выполняем смещение в буфере
		offset += sizeof(count);
		// Выполняем сброс списка выбранных сабпротоколов
		this->_selectedProtocols.clear();
		// Если количество сабпротоколов получено
		if(count > 0){
			// Выполняем последовательную загрузку всех выбранных сабпротоколов
			for(size_t i = 0; i < count; i++){
				// Выполняем получение размера поддерживаемого сабпротокола
				::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
				// Выполняем смещение в буфере
				offset += sizeof(length);
				// Если размер получен
				if(length > 0){
					// Выделяем память для поддерживаемого сабпротокола
					string subprotocol(length, 0);
					// Выполняем получение поддерживаемого сабпротокола
					::memcpy(reinterpret_cast <void *> (subprotocol.data()), data.data() + offset, length);
					// Выполняем смещение в буфере
					offset += length;
					// Если сабпротокол получен, добавляем его в список
					if(!subprotocol.empty())
						// Выполняем установку списка выбранных сабпротоколов
						this->_selectedProtocols.emplace(std::move(subprotocol));
				}
			}
		}
		// Выполняем получение количества поддерживаемых сабпротоколов
		::memcpy(reinterpret_cast <void *> (&count), data.data() + offset, sizeof(count));
		// Выполняем смещение в буфере
		offset += sizeof(count);
		// Выполняем сброс списка поддерживаемых сабпротоколов
		this->_supportedProtocols.clear();
		// Если количество сабпротоколов получено
		if(count > 0){
			// Выполняем последовательную загрузку всех поддерживаемых сабпротоколов
			for(size_t i = 0; i < count; i++){
				// Выполняем получение размера поддерживаемого сабпротокола
				::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
				// Выполняем смещение в буфере
				offset += sizeof(length);
				// Если размер получен
				if(length > 0){
					// Выделяем память для поддерживаемого сабпротокола
					string subprotocol(length, 0);
					// Выполняем получение поддерживаемого сабпротокола
					::memcpy(reinterpret_cast <void *> (subprotocol.data()), data.data() + offset, length);
					// Выполняем смещение в буфере
					offset += length;
					// Если сабпротокол получен, добавляем его в список
					if(!subprotocol.empty())
						// Выполняем установку списка поддерживаемых сабпротоколов
						this->_supportedProtocols.emplace(std::move(subprotocol));
				}
			}
		}
		// Выполняем получение размера дампа бинарных данных модуля
		::memcpy(reinterpret_cast <void *> (&length), data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Если размер получен
		if(length > 0){
			// Создаём бинарный буфер дампа
			vector <char> dump(length, 0);
			// Выполняем получение бинарного буфера дампа
			::memcpy(reinterpret_cast <void *> (dump.data()), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
			// Выполняем установку бинарного буфера данных
			http_t::dump(dump);
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
 * crypted Метод проверки на зашифрованные данные
 * @return флаг проверки на зашифрованные данные
 */
bool awh::WCore::crypted() const noexcept {
	// Выводим флаг шифрования данных
	return this->_encryption;
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::WCore::encryption(const bool mode) noexcept {
	// Устанавливаем флаг шифрования
	this->_encryption = mode;
	// Устанавливаем флаг шифрования у родительского модуля
	http_t::encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::WCore::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования у родительского модуля
	http_t::encryption(pass, salt, cipher);
}
/**
 * compression Метод извлечения выбранного метода компрессии
 * @return метод компрессии
 */
awh::Http::compressor_t awh::WCore::compression() const noexcept {
	// Выполняем извлечение выбранного метода компрессии
	return this->_compressors.selected;
}
/**
 * compression Метод установки выбранного метода компрессии
 * @param compressor метод компрессии
 */
void awh::WCore::compression(const compressor_t compressor) noexcept {
	// Выполняем установку выбранного метода компрессии
	this->_compressors.selected = compressor;
}
/**
 * compressors Метод установки списка поддерживаемых компрессоров
 * @param compressors методы компрессии данных полезной нагрузки
 */
void awh::WCore::compressors(const vector <compressor_t> & compressors) noexcept {
	// Если список архиваторов передан
	if(!compressors.empty()){
		// Вес запрашиваемого компрессора
		float weight = 1.0f;
		// Выполняем перебор списка запрашиваемых компрессоров
		for(auto & compressor : compressors){
			// Выполняем установку полученного компрессера
			this->_compressors.supports.emplace(weight, compressor);
			// Выполняем уменьшение веса компрессора
			weight -= .1f;
		}
		// Устанавливаем флаг метода компрессии
		this->_compressors.selected = this->_compressors.supports.rbegin()->second;
	}
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
 * handshake Метод выполнения проверки рукопожатия
 * @param flag флаг выполняемого процесса
 * @return     результат выполнения проверки рукопожатия
 */
bool awh::WCore::handshake(const process_t flag) noexcept {
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
		result = (this->_status == status_t::GOOD);
		// Если результат удачный
		if(result)
			// Выполняем проверку версии протокола
			result = this->check(ws_core_t::flag_t::VERSION);
		// Если подключение не выполнено
		else return result;
		// Если результат удачный
		if(result){
			// Если версия протокола ниже 2.0
			if(version < 2.0f)
				// Проверяем произошло ли переключение протокола
				result = this->check(ws_core_t::flag_t::UPGRADE);
		// Если версия протокола не соответствует
		} else {
			// Выводим сообщение об ошибке
			this->_log->print("Protocol version not supported", log_t::flag_t::CRITICAL);
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "Protocol version not supported");
			// Выходим из функции
			return result;
		}
		// Если версия протокола ниже 2.0
		if(version < 2.0f){
			// Если результат удачный
			if(result)
				// Проверяем ключ клиента
				result = this->check(ws_core_t::flag_t::KEY);
			// Если протокол не был переключён
			else {
				// Выводим сообщение об ошибке
				this->_log->print("Protocol not upgraded", log_t::flag_t::CRITICAL);
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callbacks.is("error"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "Protocol not upgraded");
				// Выходим из функции
				return result;
			}
		}
		// Если рукопожатие выполнено
		if(result)
			// Устанавливаем стейт рукопожатия
			this->_state = state_t::HANDSHAKE;
		// Если ключ клиента и сервера не согласованы
		else {
			// Выводим сообщение об ошибке
			this->_log->print("Client and server keys are inconsistent", log_t::flag_t::CRITICAL);
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "Client and server keys are inconsistent");
		}
	}
	// Выводим результат
	return result;
}
/**
 * check Метод проверки шагов рукопожатия
 * @param flag флаг выполнения проверки
 * @return     результат проверки соответствия
 */
bool awh::WCore::check(const flag_t flag) noexcept {
	// Определяем флаг выполнения проверки
	switch(static_cast <uint8_t> (flag)){
		// Если требуется выполнить проверки на переключение контекста
		case static_cast <uint8_t> (flag_t::UPGRADE): {
			// Получаем значение заголовка Upgrade
			const string & upgrade = this->_web.header("upgrade");
			// Получаем значение заголовка Connection
			const string & connection = this->_web.header("connection");
			// Если заголовки расширений найдены
			if(!upgrade.empty() && !connection.empty()){
				// Переводим значение заголовка Connection в нижний регистр
				this->_fmk->transform(connection, fmk_t::transform_t::LOWER);
				// Если заголовки соответствуют
				return (this->_fmk->compare(upgrade, "websocket") && this->_fmk->exists("upgrade", connection));
			}
		} break;
	}
	// Выводим результат
	return false;
}
/**
 * wbit Метод получения размер скользящего окна
 * @param hid тип модуля
 * @return    размер скользящего окна
 */
int16_t awh::WCore::wbit(const web_t::hid_t hid) const noexcept {
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
vector <std::pair <string, string>> awh::WCore::reject2(const web_t::res_t & res) const noexcept {
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
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, "Upgrade");
				// Удаляем заголовок протокола подключения
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, ":protocol");
				// Удаляем заголовок подключения
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, "Connection");
				// Удаляем заголовок ключ клиента
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, "Sec-WebSocket-Key");
				// Добавляем заголовок апгрейд
				const_cast <ws_core_t *> (this)->header("Upgrade", "websocket");
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
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callbacks.is("error"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "Address or request method for WebSocket-client is incorrect");
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
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, ":status");
				// Удаляем заголовок апгрейд
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, "Upgrade");
				// Удаляем заголовок подключения
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, "Connection");
				// Удаляем заголовок хеша ключа
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, "Sec-WebSocket-Accept");
				// Если ответ сервера положительный
				if(res.version >= 2.0f ? res.code == 200 : res.code == 101){
					// Добавляем заголовок подключения
					const_cast <ws_core_t *> (this)->header("Connection", "Upgrade");
					// Добавляем заголовок апгрейд
					const_cast <ws_core_t *> (this)->header("Upgrade", "websocket");
				}
				// Если версия протокола ниже 2.0
				if((res.version < 2.0f) && (res.code == 101)){
					// Выполняем генерацию хеша ключа
					const string & sha1 = this->sha1();
					// Если SHA1-ключ не сгенерирован
					if(sha1.empty()){
						// Если ключ клиента и сервера не согласованы, выводим сообщение об ошибке
						this->_log->print("SHA1 key could not be generated, no further work possiblet", log_t::flag_t::CRITICAL);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callbacks.is("error"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "SHA1 key could not be generated, no further work possiblet");
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
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callbacks.is("error"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "WebSocket-server response code set incorrectly");
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
vector <std::pair <string, string>> awh::WCore::process2(const process_t flag, const web_t::provider_t & provider) const noexcept {
	// Определяем флаг выполняемого процесса
	switch(static_cast <uint8_t> (flag)){
		// Если нужно сформировать данные запроса
		case static_cast <uint8_t> (process_t::REQUEST): {
			// Получаем объект ответа клиенту
			const web_t::req_t & req = static_cast <const web_t::req_t &> (provider);
			// Если параметры запроса получены
			if(!req.url.empty() && (req.method == web_t::method_t::CONNECT)){
				// Удаляем заголовок апгрейд
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, "Upgrade");
				// Удаляем заголовок протокола подключения
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, ":protocol");
				// Удаляем заголовок подключения
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, "Connection");
				// Удаляем заголовок ключ клиента
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, "Sec-WebSocket-Key");
				// Добавляем заголовок протокола подключения
				const_cast <ws_core_t *> (this)->header(":protocol", "websocket");
				// Устанавливаем парарметры запроса
				this->_web.request(req);
			// Если данные переданы неверные
			} else {
				// Выводим сообщение, что данные переданы неверные
				this->_log->print("Address or request method for WebSocket-client is incorrect", log_t::flag_t::CRITICAL);
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callbacks.is("error"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "Address or request method for WebSocket-client is incorrect");
				// Выходим из функции
				return vector <std::pair <string, string>> ();
			}
		} break;
		// Если нужно сформировать данные ответа
		case static_cast <uint8_t> (process_t::RESPONSE): {
			// Получаем объект ответа клиенту
			const web_t::res_t & res = static_cast <const web_t::res_t &> (provider);
			// Если параметры запроса получены
			if(res.code != 101){
				// Удаляем заголовок апгрейд
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, "Upgrade");
				// Удаляем статус ответа
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, ":status");
				// Удаляем заголовок подключения
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, "Connection");
				// Удаляем заголовок хеша ключа
				const_cast <ws_core_t *> (this)->rm(suite_t::HEADER, "Sec-WebSocket-Accept");
				// Устанавливаем параметры ответа
				this->_web.response(res);
			// Если данные переданы неверные
			} else {
				// Выводим сообщение, что данные переданы неверные
				this->_log->print("WebSocket-server response code set incorrectly", log_t::flag_t::CRITICAL);
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callbacks.is("error"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "WebSocket-server response code set incorrectly");
				// Выходим из функции
				return vector <std::pair <string, string>> ();
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
const std::set <string> & awh::WCore::subprotocols() const noexcept {
	// Выводим список выбранных сабпротоколов
	return this->_selectedProtocols;
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::WCore::subprotocols(const std::set <string> & subprotocols) noexcept {
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
