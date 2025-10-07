/**
 * @file: ws.cpp
 * @date: 2025-10-07
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Стандартные модули
 */
#include <random>
#include <algorithm>

/**
 * Подключаем заголовочный файл
 */
#include <ws/ws.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод инициализации
 *
 * @param flag флаг направления передачи данных
 */
void awh::Websocket::init(const process_t flag) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем флаг выполняемого процесса
		 */
		switch(static_cast <uint8_t> (flag)){
			// Если нужно сформировать данные запроса
			case static_cast <uint8_t> (process_t::REQUEST): {
				// Выполняем извлечение заголовков HTTP-протокола
				headers_t & headers = this->_web.headers();
				// Удаляем заголовок Accept
				headers.erase("Accept");
				// Удаляем заголовок отключения кеширования
				headers.erase("Pragma");
				// Удаляем заголовок отключения кеширования
				headers.erase("Cache-Control");
				// Удаляем заголовок типа запроса
				headers.erase("Sec-Fetch-Mode");
				// Удаляем заголовок места назначения запроса
				headers.erase("Sec-Fetch-Dest");
				// Удаляем заголовок требования сжимать содержимое ответов
				headers.erase("Accept-Encoding");
				// Удаляем заголовок поддерживаемых языков
				headers.erase("Accept-Language");
				// Удаляем заголовок версии Websocket
				headers.erase("Sec-Websocket-Version");
				// Если список поддерживаемых сабпротоколов установлен
				if(!headers.has("Sec-Websocket-Protocol") && !this->_protocols.supported.empty()){
					// Если количество поддерживаемых сабпротоколов больше 5-ти
					if(this->_protocols.supported.size() > 5){
						// Список поддерживаемых сабпротоколов
						string subprotocols = "";
						// Переходим по всему списку поддерживаемых сабпротоколов
						for(auto & subprotocol : this->_protocols.supported){
							// Если сабпротокол уже не пустой
							if(!subprotocols.empty())
								// Добавляем разделитель
								subprotocols.append(", ");
							// Добавляем в список поддерживаемых сабпротоколов
							subprotocols.append(subprotocol);
						}
						// Добавляем заголовок поддерживаемых сабпротоколов
						headers.emplace("Sec-Websocket-Protocol", ::move(subprotocols));
					// Если сабпротоколов слишком много
					} else {
						// Переходим по всему списку поддерживаемых сабпротоколов
						for(auto & subprotocol : this->_protocols.supported)
							// Добавляем полученный заголовок
							headers.emplace("Sec-Websocket-Protocol", ::move(subprotocol));
					}
				}
				// Выполняем применение расширений
				this->extensions(flag);
				// Добавляем заголовок Accept
				headers.emplace("Accept", "*/*");
				// Добавляем заголовок отключения кеширования
				headers.emplace("Pragma", "No-Cache");
				// Добавляем заголовок отключения кеширования
				headers.emplace("Cache-Control", "No-Cache");
				// Добавляем заголовок типа запроса
				headers.emplace("Sec-Fetch-Mode", "websocket");
				// Добавляем заголовок места назначения запроса
				headers.emplace("Sec-Fetch-Dest", "websocket");
				// Добавляем заголовок поддерживаемых языков
				headers.emplace("Accept-Language", HTTP_HEADER_ACCEPTLANGUAGE);
				// Добавляем заголовок версии Websocket
				headers.emplace("Sec-Websocket-Version", ws_t::VERSION);
				// Если компрессор уже выбран
				if(http_t::_compressors.selected != compressor_t::NONE){
					/**
					 * Определяем метод сжатия который поддерживает клиент
					 */
					switch(static_cast <uint8_t> (http_t::_compressors.selected)){
						// Если клиент поддерживает методот сжатия LZ4
						case static_cast <uint8_t> (compressor_t::LZ4):
							// Добавляем заголовок требования сжимать содержимое ответов
							headers.emplace("Accept-Encoding", "lz4");
						break;
						// Если клиент поддерживает методот сжатия Zstandard
						case static_cast <uint8_t> (compressor_t::ZSTD):
							// Добавляем заголовок требования сжимать содержимое ответов
							headers.emplace("Accept-Encoding", "zstd");
						break;
						// Если клиент поддерживает методот сжатия LZma
						case static_cast <uint8_t> (compressor_t::LZMA):
							// Добавляем заголовок требования сжимать содержимое ответов
							headers.emplace("Accept-Encoding", "xz");
						break;
						// Если клиент поддерживает методот сжатия Brotli
						case static_cast <uint8_t> (compressor_t::BROTLI):
							// Добавляем заголовок требования сжимать содержимое ответов
							headers.emplace("Accept-Encoding", "br");
						break;
						// Если клиент поддерживает методот сжатия BZip2
						case static_cast <uint8_t> (compressor_t::BZIP2):
							// Добавляем заголовок требования сжимать содержимое ответов
							headers.emplace("Accept-Encoding", "bzip2");
						break;
						// Если клиент поддерживает методот сжатия GZip
						case static_cast <uint8_t> (compressor_t::GZIP):
							// Добавляем заголовок требования сжимать содержимое ответов
							headers.emplace("Accept-Encoding", "gzip");
						break;
						// Если клиент поддерживает методот сжатия Deflate
						case static_cast <uint8_t> (compressor_t::DEFLATE):
							// Добавляем заголовок требования сжимать содержимое ответов
							headers.emplace("Accept-Encoding", "deflate");
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
						/**
						 * Определяем метод сжатия который поддерживает клиент
						 */
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
						headers.emplace("Accept-Encoding", ::move(compressors));
				}
			} break;
			// Если нужно сформировать данные ответа
			case static_cast <uint8_t> (process_t::RESPONSE): {
				// Добавляем в чёрный список заголовок Content-Type
				this->addToBlacklist("Content-Type");
				// Выполняем извлечение заголовков HTTP-протокола
				headers_t & headers = this->_web.headers();
				// Получаем объект ответа клиенту
				const web_t::res_t & res = this->_web.response();
				// Если ответ сервера положительный
				if((res.version >= 2.) ? (res.code == 200) : (res.code == 101)){
					// Выполняем применение расширений
					this->extensions(flag);
					// Добавляем в чёрный список заголовок Content-Encoding
					this->addToBlacklist("Content-Encoding");
					// Добавляем в чёрный список заголовок X-AWH-Encryption
					this->addToBlacklist("X-AWH-Encryption");
				}
				// Если список выбранных сабпротоколов установлен
				if(!headers.has("Sec-Websocket-Protocol") && !this->_protocols.selected.empty()){
					// Если количество выбранных сабпротоколов больше 5-ти
					if(this->_protocols.selected.size() > 5){
						// Список выбранных сабпротоколов
						string subprotocols = "";
						// Переходим по всему списку выбранных сабпротоколов
						for(auto & subprotocol : this->_protocols.selected){
							// Если сабпротокол уже не пустой
							if(!subprotocols.empty())
								// Добавляем разделитель
								subprotocols.append(", ");
							// Добавляем в список выбранных сабпротоколов
							subprotocols.append(subprotocol);
						}
						// Добавляем заголовок выбранных сабпротоколов
						headers.emplace("Sec-Websocket-Protocol", ::move(subprotocols));
					// Если сабпротоколов слишком много
					} else {
						// Переходим по всему списку выбранных сабпротоколов
						for(auto & subprotocol : this->_protocols.selected)
							// Добавляем полученный заголовок
							headers.emplace("Sec-Websocket-Protocol", ::move(subprotocol));
					}
				}
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки выбранных расширений
 *
 * @param flag флаг направления передачи данных
 */
void awh::Websocket::extensions(const process_t flag) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем извлечение заголовков HTTP-протокола
		headers_t & headers = this->_web.headers();
		// Если заголовки расширений не установлены
		if(!headers.has("Sec-Websocket-Extensions")){
			// Список поддверживаемых расширений
			vector <vector <string>> extensions;
			/**
			 * Определяем тип активной компрессии
			 */
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
					if(!this->_permessage.server.takeover)
						// Добавляем флаг запрещения использования контекста компрессии для сервера
						extensions.push_back({"server_no_context_takeover"});
					// Если запрещено переиспользовать контекст компрессии для клиента
					if(!this->_permessage.client.takeover)
						// Добавляем флаг запрещения использования контекста компрессии для клиента
						extensions.push_back({"client_no_context_takeover"});
					/**
					 * Определяем флаг выполняемого процесса
					 */
					switch(static_cast <uint8_t> (flag)){
						// Если нужно сформировать данные запроса
						case static_cast <uint8_t> (process_t::REQUEST): {
							// Если размер скользящего окна клиента установлен как максимальный
							if(this->_permessage.client.wbits == static_cast <int16_t> (GZIP_MAX_WBITS))
								// Добавляем максимальный размер скользящего окна
								extensions.push_back({"client_max_window_bits"});
							// Выполняем установку указанного размера скользящего окна
							else extensions.push_back({this->_fmk->format("client_max_window_bits=%u", this->_permessage.client.wbits)});
							// Если размер скользящего окна сервера установлен как максимальный
							if(this->_permessage.server.wbits == static_cast <int16_t> (GZIP_MAX_WBITS))
								// Добавляем максимальный размер скользящего окна
								extensions.push_back({"server_max_window_bits"});
							// Выполняем установку указанного размера скользящего окна сервера
							else extensions.push_back({this->_fmk->format("server_max_window_bits=%u", this->_permessage.server.wbits)});
						} break;
						// Если нужно сформировать данные ответа
						case static_cast <uint8_t> (process_t::RESPONSE): {
							// Выполняем установку указанного размера скользящего окна
							extensions.push_back({this->_fmk->format("client_max_window_bits=%u", this->_permessage.client.wbits)});
							// Выполняем установку указанного размера скользящего окна сервера
							extensions.push_back({this->_fmk->format("server_max_window_bits=%u", this->_permessage.server.wbits)});
						} break;
					}
				} break;
			}
			// Если данные должны быть зашифрованны
			if(this->_encryption)
				// Выполняем установку указанного метода шифрования
				extensions.push_back({this->_fmk->format("permessage-encrypt=%u", static_cast <uint16_t> (this->_encrypt.cipher))});
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
							headers.emplace("Sec-Websocket-Extensions", ::move(records));
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
				headers.emplace("Sec-Websocket-Extensions", ::move(records));
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод генерации ключа
 *
 * @return сгенерированный ключ
 */
string awh::Websocket::key() const noexcept {
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
		for(uint8_t i = 0; i < 16; i++)
			// Выполняем формирование ключа
			nonce += static_cast <char> (dist(randev));
		// Выполняем создание ключа
		this->_hash.encode(nonce.data(), nonce.size(), hash_t::cipher_t::BASE64, result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		// Выполняем повторно генерацию ключа
		result = this->key();
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод генерации хэша SHA1 ключа
 *
 * @return сгенерированный хэш ключа клиента
 */
string awh::Websocket::sha1() const noexcept {
	// Результат работы функции
	string result = "";
	// Если ключ клиента передан
	if(!this->_key.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаем контекст
			SHA_CTX ctx;
			// Выполняем инициализацию контекста
			SHA1_Init(&ctx);
			// Массив полученных значений
			uint8_t digest[20];
			// Формируем магический ключ
			const string text = this->_fmk->format("%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", this->_key.c_str());
			// Выполняем расчет суммы
			SHA1_Update(&ctx, text.c_str(), text.length());
			// Копируем полученные данные
			SHA1_Final(digest, &ctx);
			// Получаем значение ключа
			const string key(reinterpret_cast <const char *> (digest), sizeof(digest));
			// Формируем ключ для клиента
			this->_hash.encode(key.data(), key.size(), hash_t::cipher_t::BASE64, result);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод извлечения системного расширения из заголовка
 *
 * @param extension запись из которой нужно извлечь расширение
 * @return          результат извлечения
 */
bool awh::Websocket::extract(const string & extension) noexcept {
	bool result = false;
	// Если заголовок передан
	if(!extension.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если нужно производить шифрование данных
			if((result = this->_encryption = this->_fmk->exists("permessage-encrypt=", extension))){
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					/**
					 * Определяем размер шифрования
					 */
					switch(static_cast <uint16_t> (::stoi(extension.substr(19)))){
						// Если шифрование произведено 128 битным ключём
						case 128: this->_encrypt.cipher = hash_t::cipher_t::AES128; break;
						// Если шифрование произведено 192 битным ключём
						case 192: this->_encrypt.cipher = hash_t::cipher_t::AES192; break;
						// Если шифрование произведено 256 битным ключём
						case 256: this->_encrypt.cipher = hash_t::cipher_t::AES256; break;
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception &) {
					// Если шифрование произведено 128 битным ключём
					this->_encrypt.cipher = hash_t::cipher_t::AES128;
				}
			// Если клиент просит отключить перехват контекста сжатия для сервера
			} else if((result = this->_fmk->compare(extension, "server_no_context_takeover"))) {
				/**
				 * Определяем флаг типа текущего модуля
				 */
				switch(static_cast <uint8_t> (this->_web.hid())){
					// Если флаг текущего модуля соответствует клиенту
					case static_cast <uint8_t> (web_t::hid_t::CLIENT):
						// Выполняем отключение перехвата контекста
						this->_permessage.server.takeover = false;
					break;
					// Если флаг текущего модуля соответствует серверу
					case static_cast <uint8_t> (web_t::hid_t::SERVER): {
						// Выполняем отключение перехвата контекста
						this->_permessage.server.takeover = false;
						// Выполняем отключение перехвата контекста
						this->_permessage.client.takeover = false;
					} break;
				}
			// Если клиент просит отключить перехват контекста сжатия для клиента
			} else if((result = this->_fmk->compare(extension, "client_no_context_takeover")))
				// Выполняем отключение перехвата контекста
				this->_permessage.client.takeover = false;
			// Если получены заголовки требующие сжимать передаваемые фреймы методом Deflate
			else if((result = this->_fmk->compare(extension, "permessage-deflate") || this->_fmk->compare(extension, "perframe-deflate"))) {
				/**
				 * Определяем флаг типа текущего модуля
				 */
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
				/**
				 * Определяем флаг типа текущего модуля
				 */
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
				/**
				 * Определяем флаг типа текущего модуля
				 */
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
				/**
				 * Определяем флаг типа текущего модуля
				 */
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
				/**
				 * Определяем флаг типа текущего модуля
				 */
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
				/**
				 * Определяем флаг типа текущего модуля
				 */
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
				/**
				 * Определяем флаг типа текущего модуля
				 */
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
				/**
				 * Определяем флаг типа текущего модуля
				 */
				switch(static_cast <uint8_t> (this->_web.hid())){
					// Если флаг текущего модуля соответствует клиенту
					case static_cast <uint8_t> (web_t::hid_t::CLIENT):
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Устанавливаем размер скользящего окна
							this->_permessage.client.wbits = static_cast <int16_t> (::stoi(extension.substr(23)));
						/**
						 * Если возникает ошибка
						 */
						} catch(const exception &) {
							// Устанавливаем размер скользящего окна
							this->_permessage.client.wbits = GZIP_MAX_WBITS;
						}
						// Если размер скользящего окна установлен неправильно
						if((this->_permessage.client.wbits < GZIP_MIN_WBITS) || (this->_permessage.client.wbits > GZIP_MAX_WBITS)){
							// Выводим сообщение об ошибке
							this->_log->print("Deflate max_window_bits for the client is set incorrectly", log_t::flag_t::WARNING);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Deflate max_window_bits for the client is set incorrectly");
						}
					break;
					// Если флаг текущего модуля соответствует серверу
					case static_cast <uint8_t> (web_t::hid_t::SERVER): {
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Устанавливаем размер скользящего окна
							this->_permessage.client.wbits = static_cast <int16_t> (::stoi(extension.substr(23)));
						/**
						 * Если возникает ошибка
						 */
						} catch(const exception &) {
							// Устанавливаем размер скользящего окна
							this->_permessage.client.wbits = GZIP_MAX_WBITS;
						}
						// Если размер скользящего окна установлен слишком маленький
						if(this->_permessage.client.wbits < GZIP_MIN_WBITS)
							// Выполняем корректировку размера скользящего окна
							this->_permessage.client.wbits = GZIP_MIN_WBITS;
						// Если размер скользящего окна установлен слишком высоким
						else if(this->_permessage.client.wbits > GZIP_MAX_WBITS)
							// Выполняем корректировку размера скользящего окна
							this->_permessage.client.wbits = GZIP_MAX_WBITS;
					} break;
				}
			// Если разрешено использовать максимальный размер скользящего окна для клиента
			} else if((result = this->_fmk->compare(extension, "client_max_window_bits"))) {
				/**
				 * Определяем флаг типа текущего модуля
				 */
				switch(static_cast <uint8_t> (this->_web.hid())){
					// Если флаг текущего модуля соответствует клиенту
					case static_cast <uint8_t> (web_t::hid_t::CLIENT):
						// Устанавливаем максимальный размер скользящего окна
						this->_permessage.client.wbits = GZIP_MAX_WBITS;
					break;
					// Если флаг текущего модуля соответствует серверу
					case static_cast <uint8_t> (web_t::hid_t::SERVER):
						// Устанавливаем максимальный размер скользящего окна
						this->_permessage.client.wbits = GZIP_MAX_WBITS;
					break;
				}
			// Если размер скользящего окна для сервера получен
			} else if((result = this->_fmk->exists("server_max_window_bits=", extension))) {
				/**
				 * Определяем флаг типа текущего модуля
				 */
				switch(static_cast <uint8_t> (this->_web.hid())){
					// Если флаг текущего модуля соответствует клиенту
					case static_cast <uint8_t> (web_t::hid_t::CLIENT):
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Устанавливаем размер скользящего окна
							this->_permessage.server.wbits = static_cast <int16_t> (::stoi(extension.substr(23)));
						/**
						 * Если возникает ошибка
						 */
						} catch(const exception &) {
							// Устанавливаем размер скользящего окна
							this->_permessage.server.wbits = GZIP_MAX_WBITS;
						}
						// Если размер скользящего окна установлен неправильно
						if((this->_permessage.server.wbits < GZIP_MIN_WBITS) || (this->_permessage.server.wbits > GZIP_MAX_WBITS)){
							// Выводим сообщение об ошибке
							this->_log->print("Deflate max_window_bits for the server is set incorrectly", log_t::flag_t::WARNING);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::WARNING, http::error_t::PROTOCOL, "Deflate max_window_bits for the server is set incorrectly");
						}
					break;
					// Если флаг текущего модуля соответствует серверу
					case static_cast <uint8_t> (web_t::hid_t::SERVER): {
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Устанавливаем размер скользящего окна
							this->_permessage.server.wbits = static_cast <int16_t> (::stoi(extension.substr(23)));
						/**
						 * Если возникает ошибка
						 */
						} catch(const exception &) {
							// Устанавливаем размер скользящего окна
							this->_permessage.server.wbits = GZIP_MAX_WBITS;
						}
						// Если размер скользящего окна установлен слишком маленький
						if(this->_permessage.server.wbits < GZIP_MIN_WBITS)
							// Выполняем корректировку размера скользящего окна
							this->_permessage.server.wbits = GZIP_MIN_WBITS;
						// Если размер скользящего окна установлен слишком высоким
						else if(this->_permessage.server.wbits > GZIP_MAX_WBITS)
							// Выполняем корректировку размера скользящего окна
							this->_permessage.server.wbits = GZIP_MAX_WBITS;
					} break;
				}
			// Если разрешено использовать максимальный размер скользящего окна для сервера
			} else if((result = this->_fmk->compare(extension, "server_max_window_bits"))) {
				/**
				 * Определяем флаг типа текущего модуля
				 */
				switch(static_cast <uint8_t> (this->_web.hid())){
					// Если флаг текущего модуля соответствует клиенту
					case static_cast <uint8_t> (web_t::hid_t::CLIENT):
						// Устанавливаем максимальный размер скользящего окна
						this->_permessage.server.wbits = GZIP_MAX_WBITS;
					break;
					// Если флаг текущего модуля соответствует серверу
					case static_cast <uint8_t> (web_t::hid_t::SERVER):
						// Устанавливаем максимальный размер скользящего окна
						this->_permessage.server.wbits = GZIP_MAX_WBITS;
					break;
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(extension), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод очистки собранных данных
 *
 */
void awh::Websocket::clean() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем очистку родительских данных
		http_t::clear();
		// Выполняем сброс ключа клиента
		this->_key.clear();
		// Выполняем сброс списка выбранных сабпротоколов
		this->_protocols.selected.clear();
		// Выполняем сброс списка поддерживаемых сабпротоколов
		this->_protocols.supported.clear();
		// Выполняем сброс размера скользящего окна для клиента
		this->_permessage.client.wbits = GZIP_MAX_WBITS;
		// Выполняем сброс размера скользящего окна для сервера
		this->_permessage.server.wbits = GZIP_MAX_WBITS;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод получения бинарного дампа
 *
 * @return бинарный дамп данных
 */
awh::buffer_t awh::Websocket::dump() const noexcept {
	// Результат работы функции
	buffer_t result(this->_fmk, this->_log);
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем параметры компрессии сообщений
		result.push(&this->_permessage, sizeof(this->_permessage));
		// Устанавливаем метод компрессии отправляемых данных
		result.push(&this->_compressors.selected, sizeof(this->_compressors.selected));
		// Устанавливаем количество поддерживаемых компрессоров
		result.push(this->_compressors.supports.size());
		// Если список поддерживаемых компрессоров не пустой
		if(!this->_compressors.supports.empty()){
			// Выполняем перебор всех поддерживаемых компрессоров
			for(auto & compressor : this->_compressors.supports){
				// Выполняем установку веска компрессора
				result.push(&compressor.first, sizeof(compressor.first));
				// Выполняем установку идентификатора компрессора
				result.push(&compressor.second, sizeof(compressor.second));
			}
		}
		// Устанавливаем количество расширений
		result.push(this->_extensions.size());
		// Если расширения установлены
		if(!this->_extensions.empty()){
			// Выполняем перебор всего списка расширений
			for(auto & extensions : this->_extensions){
				// Устанавливаем количество расширений
				result.push(extensions.size());
				// Выполняем перебор всего количества расширений
				for(auto & extension : extensions){
					// Устанавливаем количество расширений
					result.push(extension.length());
					// Устанавливаем значение полученного расширения
					result.push(extension);
				}
			}
		}
		// Устанавливаем размер ключа клиента
		result.push(this->_key.length());
		// Добавляем ключ клиента
		result.push(this->_key);
		// Устанавливаем количество выбранных сабпротоколов
		result.push(this->_protocols.selected.size());
		// Выполняем перебор всех выбранных сабпротоколов
		for(auto & subprotocol : this->_protocols.selected){
			// Устанавливаем размер выбранному сабпротокола
			result.push(subprotocol.length());
			// Устанавливаем данные выбранному сабпротокола
			result.push(subprotocol);
		}
		// Устанавливаем количество поддерживаемых сабпротоколов
		result.push(this->_protocols.supported.size());
		// Выполняем перебор всех поддерживаемых сабпротоколов
		for(auto & subprotocol : this->_protocols.supported){
			// Устанавливаем размер поддерживаемого сабпротокола
			result.push(subprotocol.length());
			// Устанавливаем данные поддерживаемого сабпротокола
			result.push(subprotocol);
		}
		// Выполняем получение дампа основного класса
		buffer_t dump = http_t::dump();
		// Устанавливаем размер дампа бинарных данных модуля
		result.push(dump.size());
		// Добавляем дамп бинарных данных модуля
		result.push(dump);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки бинарного дампа
 *
 * @param data бинарный дамп данных
 */
void awh::Websocket::dump(const buffer_t & data) noexcept {
	// Если данные бинарного дампа переданы
	if(!data.empty())
		// Выполняем установку дампа данных
		this->dump(static_cast <const char *> (data), static_cast <size_t> (data));
}
/**
 * @brief Метод установки бинарного дампа
 *
 * @param buffer буфер бинарных данных
 * @param size   размер бинарных данных
 */
void awh::Websocket::dump(const char * buffer, const size_t size) noexcept {
	// Если данные бинарного дампа переданы
	if((buffer != nullptr) && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Длина строки, количество элементов и смещение в буфере
			size_t length = 0, count = 0, offset = 0;
			// Выполняем получение параметров компрессии сообщений
			::memcpy(&this->_permessage, buffer + offset, sizeof(this->_permessage));
			// Выполняем смещение в буфере
			offset += sizeof(this->_permessage);
			// Выполняем получение методов компрессии отправляемых данных
			::memcpy(&this->_compressors.selected, buffer + offset, sizeof(this->_compressors.selected));
			// Выполняем смещение в буфере
			offset += sizeof(this->_compressors.selected);
			// Выполняем получение количества поддерживаемых компрессоров
			::memcpy(&count, buffer + offset, sizeof(count));
			// Выполняем смещение в буфере
			offset += sizeof(count);
			// Выполняем очистку списку поддерживаемых компрессоров
			this->_compressors.supports.clear();
			// Если количество компрессоров полученно
			if(count > 0){
				// Вес компрессора
				float weight = .0f;
				// Идентификатор компрессора
				compressor_t compressor = compressor_t::NONE;
				// Выполняем последовательную установку всех компрессоров
				for(size_t i = 0; i < count; i++){
					// Выполняем получение веса компрессора
					::memcpy(&weight, buffer + offset, sizeof(weight));
					// Выполняем смещение в буфере
					offset += sizeof(weight);
					// Выполняем получение идентификатора компрессора
					::memcpy(&compressor, buffer + offset, sizeof(compressor));
					// Выполняем смещение в буфере
					offset += sizeof(compressor);
					// Выполняем установку метода компрессора
					this->_compressors.supports.emplace(weight, compressor);
				}
			}
			// Выполняем получение количества расширений
			::memcpy(&count, buffer + offset, sizeof(count));
			// Выполняем смещение в буфере
			offset += sizeof(count);
			// Если количество расширений получено
			if(count > 0){
				// Выполняем инициализацию списка расширений
				this->_extensions.resize(count);
				// Выполняем перебор всех групп расширений
				for(size_t i = 0; i < this->_extensions.size(); i++){
					// Выполняем получение количества групп расширений
					::memcpy(&count, buffer + offset, sizeof(count));
					// Выполняем смещение в буфере
					offset += sizeof(count);
					// Выполняем инициализацию списка групп расширений
					this->_extensions.at(i).resize(count);
					// Выполняем перебор всех расширений
					for(size_t j = 0; j < this->_extensions.at(i).size(); j++){
						// Выполняем получение размера расширения
						::memcpy(&length, buffer + offset, sizeof(length));
						// Выполняем смещение в буфере
						offset += sizeof(length);
						// Если размер получен
						if(length > 0){
							// Выполняем инициализацию расширения
							this->_extensions.at(i).at(j).resize(length);
							// Выполняем копирование полученного расширения
							::memcpy(this->_extensions.at(i).at(j).data(), this->_extensions.at(i).at(j).data() + offset, length);
							// Выполняем смещение в буфере
							offset += length;
						}
					}
				}
			}
			// Выполняем получение размера ключа клиента
			::memcpy(&length, buffer + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Если размер получен
			if(length > 0){
				// Выполняем выделение памяти для ключа клиента
				this->_key.resize(length, 0);
				// Выполняем получение ключа клиента
				::memcpy(this->_key.data(), buffer + offset, length);
				// Выполняем смещение в буфере
				offset += length;
			}
			// Выполняем получение количества выбранных сабпротоколов
			::memcpy(&count, buffer + offset, sizeof(count));
			// Выполняем смещение в буфере
			offset += sizeof(count);
			// Выполняем сброс списка выбранных сабпротоколов
			this->_protocols.selected.clear();
			// Если количество сабпротоколов получено
			if(count > 0){
				// Извлекаемый сабпротокол
				string subprotocol = "";
				// Выполняем последовательную загрузку всех выбранных сабпротоколов
				for(size_t i = 0; i < count; i++){
					// Выполняем получение размера поддерживаемого сабпротокола
					::memcpy(&length, buffer + offset, sizeof(length));
					// Выполняем смещение в буфере
					offset += sizeof(length);
					// Если размер получен
					if(length > 0){
						// Выделяем память для поддерживаемого сабпротокола
						subprotocol.resize(length, 0);
						// Выполняем получение поддерживаемого сабпротокола
						::memcpy(subprotocol.data(), buffer + offset, length);
						// Выполняем смещение в буфере
						offset += length;
						// Если сабпротокол получен, добавляем его в список
						if(!subprotocol.empty())
							// Выполняем установку списка выбранных сабпротоколов
							this->_protocols.selected.emplace(::move(subprotocol));
					}
				}
			}
			// Выполняем получение количества поддерживаемых сабпротоколов
			::memcpy(&count, buffer + offset, sizeof(count));
			// Выполняем смещение в буфере
			offset += sizeof(count);
			// Выполняем сброс списка поддерживаемых сабпротоколов
			this->_protocols.supported.clear();
			// Если количество сабпротоколов получено
			if(count > 0){
				// Извлекаемый сабпротокол
				string subprotocol = "";
				// Выполняем последовательную загрузку всех поддерживаемых сабпротоколов
				for(size_t i = 0; i < count; i++){
					// Выполняем получение размера поддерживаемого сабпротокола
					::memcpy(&length, buffer + offset, sizeof(length));
					// Выполняем смещение в буфере
					offset += sizeof(length);
					// Если размер получен
					if(length > 0){
						// Выделяем память для поддерживаемого сабпротокола
						subprotocol.resize(length, 0);
						// Выполняем получение поддерживаемого сабпротокола
						::memcpy(subprotocol.data(), buffer + offset, length);
						// Выполняем смещение в буфере
						offset += length;
						// Если сабпротокол получен, добавляем его в список
						if(!subprotocol.empty())
							// Выполняем установку списка поддерживаемых сабпротоколов
							this->_protocols.supported.emplace(::move(subprotocol));
					}
				}
			}
			// Выполняем получение размера дампа бинарных данных модуля
			::memcpy(&length, buffer + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Если размер получен
			if(length > 0){
				// Выполняем установку бинарного буфера данных
				http_t::dump(buffer + offset, length);
				// Выполняем смещение в буфере
				offset += length;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод проверки шагов рукопожатия
 *
 * @param step флаг выполнения проверки
 * @return     результат проверки соответствия
 */
bool awh::Websocket::step(const step_t step) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем флаг выполнения проверки
		 */
		switch(static_cast <uint8_t> (step)){
			// Если требуется выполнить проверки на переключение контекста
			case static_cast <uint8_t> (step_t::UPGRADE): {
				// Получаем значение заголовка Upgrade
				const string & upgrade = this->_web.header("upgrade");
				// Получаем значение заголовка Connection
				const string connection = this->_web.header("connection");
				// Если заголовки расширений найдены
				if(!upgrade.empty() && !connection.empty()){
					// Переводим значение заголовка Connection в нижний регистр
					this->_fmk->transform(connection, fmk_t::transform_t::LOWER);
					// Если заголовки соответствуют
					return (this->_fmk->compare(upgrade, "websocket") && this->_fmk->exists("upgrade", connection));
				}
			} break;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (step)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return false;
}
/**
 * @brief Метод выполнения проверки рукопожатия
 *
 * @param flag флаг выполняемого процесса
 * @return     результат выполнения проверки рукопожатия
 */
bool awh::Websocket::handshake(const process_t flag) noexcept {
	// Результат работы функции
	bool result = (this->_session.state == state_t::HANDSHAKE);
	// Если рукопожатие не выполнено
	if(!result){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Версия протокола
			double version = 1.1;
			/**
			 * Определяем флаг выполняемого процесса
			 */
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
			result = (this->_session.handshake == handshake_t::GOOD);
			// Если результат удачный
			if(result)
				// Выполняем проверку версии протокола
				result = this->step(step_t::VERSION);
			// Если подключение не выполнено
			else return result;
			// Если результат удачный
			if(result){
				// Если версия протокола ниже 2.0
				if(version < 2.)
					// Проверяем произошло ли переключение протокола
					result = this->step(step_t::UPGRADE);
			// Если версия протокола не соответствует
			} else {
				// Выводим сообщение об ошибке
				this->_log->print("Protocol version not supported", log_t::flag_t::WARNING);
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "Protocol version not supported");
				// Выходим из функции
				return result;
			}
			// Если версия протокола ниже 2.0
			if(version < 2.){
				// Если результат удачный
				if(result)
					// Проверяем ключ клиента
					result = this->step(step_t::KEY);
				// Если протокол не был переключён
				else {
					// Выводим сообщение об ошибке
					this->_log->print("Protocol not upgraded", log_t::flag_t::WARNING);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "Protocol not upgraded");
					// Выходим из функции
					return result;
				}
			}
			// Если рукопожатие выполнено
			if(result)
				// Устанавливаем стейт рукопожатия
				this->_session.state = state_t::HANDSHAKE;
			// Если ключ клиента и сервера не согласованы
			else {
				// Выводим сообщение об ошибке
				this->_log->print("Client and server keys are inconsistent", log_t::flag_t::CRITICAL);
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "Client and server keys are inconsistent");
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения размер скользящего окна
 *
 * @param hid тип текущего модуля
 * @return    размер скользящего окна
 */
int16_t awh::Websocket::wbits(const web_t::hid_t hid) const noexcept {
	/**
	 * Определяем флаг типа текущего модуля
	 */
	switch(static_cast <uint8_t> (hid)){
		// Если флаг текущего модуля соответствует клиенту
		case static_cast <uint8_t> (web_t::hid_t::CLIENT):
			// Выводим размер скользящего окна
			return this->_permessage.client.wbits;
		// Если флаг текущего модуля соответствует серверу
		case static_cast <uint8_t> (web_t::hid_t::SERVER):
			// Выводим размер скользящего окна
			return this->_permessage.server.wbits;
	}
	// Выводим результат
	return GZIP_MAX_WBITS;
}
/**
 * @brief Метод получения флага переиспользования контекста компрессии
 *
 * @param hid тип текущего модуля
 * @return    флаг запрета переиспользования контекста компрессии
 */
bool awh::Websocket::takeover(const web_t::hid_t hid) const noexcept {
	/**
	 * Определяем флаг типа текущего модуля
	 */
	switch(static_cast <uint8_t> (hid)){
		// Если флаг текущего модуля соответствует клиенту
		case static_cast <uint8_t> (web_t::hid_t::CLIENT):
			// Выводим флаг переиспользования компрессии
			return this->_permessage.client.takeover;
		// Если флаг текущего модуля соответствует серверу
		case static_cast <uint8_t> (web_t::hid_t::SERVER):
			// Выводим флаг переиспользования компрессии
			return this->_permessage.server.takeover;
	}
	// Выводим результат
	return false;
}
/**
 * @brief Метод установки флага переиспользования контекста компрессии
 *
 * @param hid  тип текущего модуля
 * @param mode режим запрета переиспользования контекста компрессии
 */
void awh::Websocket::takeover(const web_t::hid_t hid, const bool mode) noexcept {
	/**
	 * Определяем флаг типа текущего модуля
	 */
	switch(static_cast <uint8_t> (hid)){
		// Если флаг текущего модуля соответствует клиенту
		case static_cast <uint8_t> (web_t::hid_t::CLIENT):
			// Устанавливаем флаг переиспользования компрессии
			this->_permessage.client.takeover = mode;
		break;
		// Если флаг текущего модуля соответствует серверу
		case static_cast <uint8_t> (web_t::hid_t::SERVER):
			// Устанавливаем флаг переиспользования компрессии
			this->_permessage.server.takeover = mode;
		break;
	}
}
/**
 * @brief Метод извлечения выбранного метода компрессии
 *
 * @return метод компрессии
 */
awh::http_t::compressor_t awh::Websocket::compression() const noexcept {
	// Выполняем извлечение выбранного метода компрессии
	return this->_compressors.selected;
}
/**
 * @brief Метод установки выбранного метода компрессии
 *
 * @param compressor метод компрессии
 */
void awh::Websocket::compression(const compressor_t compressor) noexcept {
	// Выполняем установку выбранного метода компрессии
	this->_compressors.selected = compressor;
}
/**
 * @brief Метод установки списка поддерживаемых компрессоров
 *
 * @param compressors методы компрессии данных полезной нагрузки
 */
void awh::Websocket::compressors(const vector <compressor_t> & compressors) noexcept {
	// Если список архиваторов передан
	if(!compressors.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Вес запрашиваемого компрессора
			float weight = 1.f;
			// Выполняем перебор списка запрашиваемых компрессоров
			for(auto & compressor : compressors){
				// Выполняем установку полученного компрессера
				this->_compressors.supports.emplace(weight, compressor);
				// Выполняем уменьшение веса компрессора
				weight -= .1f;
			}
			// Устанавливаем флаг метода компрессии
			this->_compressors.selected = this->_compressors.supports.rbegin()->second;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
}
/**
 * @brief Метод извлечения списка расширений
 *
 * @return список поддерживаемых расширений
 */
const vector <vector <string>> & awh::Websocket::extensions() const noexcept {
	// Выводим результат
	return this->_extensions;
}
/**
 * @brief Метод установки списка расширений
 *
 * @param extensions список поддерживаемых расширений
 */
void awh::Websocket::extensions(const vector <vector <string>> & extensions) noexcept {
	// Если список поддерживаемых расширений переданы
	if(!extensions.empty())
		// Выполняем установку списка поддерживаемых расширений
		this->_extensions.assign(extensions.begin(), extensions.end());
	// Выполняем список поддерживаемых расширений
	else this->_extensions.clear();
}
/**
 * @brief Метод установки поддерживаемого сабпротокола
 *
 * @param subprotocol сабпротокол для установки
 */
void awh::Websocket::subprotocol(const string & subprotocol) noexcept {
	// Если сабпротокол передан
	if(!subprotocol.empty())
		// Выполняем установку поддерживаемого сабпротокола
		this->_protocols.supported.emplace(subprotocol);
}
/**
 * @brief Метод получения списка выбранных сабпротоколов
 *
 * @return список выбранных сабпротоколов
 */
const std::unordered_set <string> & awh::Websocket::subprotocols() const noexcept {
	// Выводим список выбранных сабпротоколов
	return this->_protocols.selected;
}
/**
 * @brief Метод установки списка поддерживаемых сабпротоколов
 *
 * @param subprotocols сабпротоколы для установки
 */
void awh::Websocket::subprotocols(const std::unordered_set <string> & subprotocols) noexcept {
	// Если список сабпротоколов получен
	if(!subprotocols.empty())
		// Выполняем установку списка поддерживаемых сабпротоколов
		this->_protocols.supported = subprotocols;
}
/**
 * @brief Метод создания отрицательного ответа
 *
 * @param res объект параметров HTTP-ответа
 * @return    буфер данных ответа в бинарном виде
 */
awh::buffer_t awh::Websocket::reject(const web_t::res_t & res) noexcept {
	// Выполняем очистку выбранного сабпротокола
	this->_protocols.selected.clear();
	// Выполняем генерацию сообщения ответа
	return ::move(http_t::reject(res));
}
/**
 * @brief Метод создания отрицательного ответа (для протокола HTTP/2)
 *
 * @param res объект параметров HTTP-ответа
 * @return    буфер данных ответа в бинарном виде
 */
vector <std::pair <string, string>> awh::Websocket::reject2(const web_t::res_t & res) noexcept {
	// Выполняем очистку выбранного сабпротокола
	this->_protocols.selected.clear();
	// Выполняем генерацию сообщения ответа
	return http_t::reject2(res);
}
/**
 * @brief Метод создания выполняемого процесса в бинарном виде
 *
 * @param flag флаг выполняемого процесса
 * @param prov параметры провайдера обмена сообщениями
 * @return     буфер данных в бинарном виде
 */
awh::buffer_t awh::Websocket::process(const process_t flag, const web_t::provider_t & prov) noexcept {
	// Результат работы функции
	buffer_t result(this->_fmk, this->_log);
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем флаг выполняемого процесса
		 */
		switch(static_cast <uint8_t> (flag)){
			// Если нужно сформировать данные запроса
			case static_cast <uint8_t> (process_t::REQUEST): {
				// Получаем объект ответа клиенту
				const web_t::req_t & req = static_cast <const web_t::req_t &> (prov);
				// Если параметры запроса получены
				if(!req.url.empty() && (req.version >= 2. ? (req.method == web_t::method_t::CONNECT) : (req.method == web_t::method_t::GET))){
					// Генерируем ключ клиента
					this->_key = this->key();
					// Извлекаем список заголовков
					headers_t & headers = this->_web.headers();
					// Удаляем заголовок апгрейд
					headers.erase("Upgrade");
					// Удаляем заголовок протокола подключения
					headers.erase(":protocol");
					// Удаляем заголовок подключения
					headers.erase("Connection");
					// Удаляем заголовок ключ клиента
					headers.erase("Sec-Websocket-Key");
					// Добавляем заголовок апгрейд
					headers.emplace("Upgrade", "websocket");
					// Добавляем заголовок подключения
					headers.emplace("Connection", "Keep-Alive, Upgrade");
					// Добавляем заголовок ключ клиента
					headers.emplace("Sec-Websocket-Key", this->_key);
					// Устанавливаем парарметры запроса
					this->_web.request(req);
				// Если данные переданы неверные
				} else {
					// Выводим сообщение, что данные переданы неверные
					this->_log->print("Address or request method for Websocket-client is incorrect", log_t::flag_t::CRITICAL);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "Address or request method for Websocket-client is incorrect");
					// Выходим из функции
					return result;
				}
			} break;
			// Если нужно сформировать данные ответа
			case static_cast <uint8_t> (process_t::RESPONSE): {
				// Получаем объект ответа клиенту
				const web_t::res_t & res = static_cast <const web_t::res_t &> (prov);
				// Если параметры запроса получены
				if(res.version >= 2. ? (res.code != 101) : (res.code != 200)){
					// Извлекаем список заголовков
					headers_t & headers = this->_web.headers();
					// Удаляем статус ответа
					headers.erase(":status");
					// Удаляем заголовок апгрейд
					headers.erase("Upgrade");
					// Удаляем заголовок подключения
					headers.erase("Connection");
					// Удаляем заголовок хеша ключа
					headers.erase("Sec-Websocket-Accept");
					// Если ответ сервера положительный
					if(res.version >= 2. ? res.code == 200 : res.code == 101){
						// Добавляем заголовок подключения
						headers.emplace("Connection", "Upgrade");
						// Добавляем заголовок апгрейд
						headers.emplace("Upgrade", "websocket");
					}
					// Если версия протокола ниже 2.0
					if((res.version < 2.) && (res.code == 101)){
						// Выполняем генерацию хеша ключа
						const string & sha1 = this->sha1();
						// Если SHA1-ключ не сгенерирован
						if(sha1.empty()){
							// Если ключ клиента и сервера не согласованы, выводим сообщение об ошибке
							this->_log->print("SHA1 key could not be generated, no further work possiblet", log_t::flag_t::CRITICAL);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "SHA1 key could not be generated, no further work possiblet");
							// Выходим из функции
							return result;
						}
						// Добавляем заголовок хеша ключа
						headers.emplace("Sec-Websocket-Accept", sha1.c_str());
					}
					// Устанавливаем параметры ответа
					this->_web.response(res);
				// Если данные переданы неверные
				} else {
					// Выводим сообщение, что данные переданы неверные
					this->_log->print("Websocket-server response code set incorrectly", log_t::flag_t::CRITICAL);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "Websocket-server response code set incorrectly");
					// Выходим из функции
					return result;
				}
			} break;
		}
		// Выполняем инициализацию
		this->init(flag);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return ::move(http_t::process(flag, prov));
}
/**
 * @brief Метод создания выполняемого процесса в бинарном виде (для протокола HTTP/2)
 *
 * @param flag флаг выполняемого процесса
 * @param prov параметры провайдера обмена сообщениями
 * @return     буфер данных в бинарном виде
 */
vector <std::pair <string, string>> awh::Websocket::process2(const process_t flag, const web_t::provider_t & prov) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем флаг выполняемого процесса
		 */
		switch(static_cast <uint8_t> (flag)){
			// Если нужно сформировать данные запроса
			case static_cast <uint8_t> (process_t::REQUEST): {
				// Получаем объект ответа клиенту
				const web_t::req_t & req = static_cast <const web_t::req_t &> (prov);
				// Если параметры запроса получены
				if(!req.url.empty() && (req.method == web_t::method_t::CONNECT)){
					// Извлекаем список заголовков
					headers_t & headers = this->_web.headers();
					// Удаляем заголовок апгрейд
					headers.erase("Upgrade");
					// Удаляем заголовок протокола подключения
					headers.erase(":protocol");
					// Удаляем заголовок подключения
					headers.erase("Connection");
					// Удаляем заголовок ключ клиента
					headers.erase("Sec-Websocket-Key");
					// Добавляем заголовок протокола подключения
					headers.emplace(":protocol", "websocket");
					// Устанавливаем парарметры запроса
					this->_web.request(req);
				// Если данные переданы неверные
				} else {
					// Выводим сообщение, что данные переданы неверные
					this->_log->print("Address or request method for Websocket-client is incorrect", log_t::flag_t::CRITICAL);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "Address or request method for Websocket-client is incorrect");
					// Выходим из функции
					return vector <std::pair <string, string>> ();
				}
			} break;
			// Если нужно сформировать данные ответа
			case static_cast <uint8_t> (process_t::RESPONSE): {
				// Получаем объект ответа клиенту
				const web_t::res_t & res = static_cast <const web_t::res_t &> (prov);
				// Если параметры запроса получены
				if(res.code != 101){
					// Извлекаем список заголовков
					headers_t & headers = this->_web.headers();
					// Удаляем заголовок апгрейд
					headers.erase("Upgrade");
					// Удаляем статус ответа
					headers.erase(":status");
					// Удаляем заголовок подключения
					headers.erase("Connection");
					// Удаляем заголовок хеша ключа
					headers.erase("Sec-Websocket-Accept");
					// Устанавливаем параметры ответа
					this->_web.response(res);
				// Если данные переданы неверные
				} else {
					// Выводим сообщение, что данные переданы неверные
					this->_log->print("Websocket-server response code set incorrectly", log_t::flag_t::CRITICAL);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, "Websocket-server response code set incorrectly");
					// Выходим из функции
					return vector <std::pair <string, string>> ();
				}
			} break;
		}
		// Выполняем инициализацию
		this->init(flag);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callback.is("error"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const uint32_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_web.id(), log_t::flag_t::CRITICAL, http::error_t::PROTOCOL, error.what());
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (flag)), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return http_t::process2(flag, prov);
}
/**
 * @brief Метод проверки на зашифрованные данные
 *
 * @return флаг проверки на зашифрованные данные
 */
bool awh::Websocket::crypted() const noexcept {
	// Выводим результат проверки
	return this->_encryption;
}
/**
 * @brief Метод активации шифрования
 *
 * @param mode флаг активации шифрования
 */
void awh::Websocket::encryption(const bool mode) noexcept {
	// Устанавливаем флаг шифрования
	this->_encryption = mode;
	// Устанавливаем флаг шифрования у родительского модуля
	http_t::encryption(mode);
}
/**
 * @brief Метод установки параметров шифрования
 *
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::Websocket::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования у родительского модуля
	http_t::encryption(pass, salt, cipher);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Websocket::Websocket(const fmk_t * fmk, const log_t * log) noexcept :
 http_t(identity_t::WS, fmk, log), _encryption(false), _key{""} {}
/**
 * @brief Деструктор
 *
 */
awh::Websocket::~Websocket() noexcept {}
