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
 */
void awh::WCore::init(const process_t flag) noexcept {
	// Удаляем заголовок апгрейд
	this->rmHeader("Upgrade");
	// Удаляем заголовок подключения
	this->rmHeader("Connection");
	// Удаляем заголовок сабпратоколов
	this->rmHeader("Sec-WebSocket-Protocol");
	// Удаляем заголовок расширений
	this->rmHeader("Sec-WebSocket-Extensions");
	// Определяем флаг выполняемого процесса
	switch(static_cast <uint8_t> (flag)){
		// Если нужно сформировать данные запроса
		case static_cast <uint8_t> (process_t::REQUEST): {
			// Удаляем заголовок Accept
			this->rmHeader("Accept");
			// Удаляем заголовок отключения кеширования
			this->rmHeader("Pragma");
			// Удаляем заголовок протокола подключения
			this->rmHeader(":protocol");
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
			// Удаляем заголовок ключ клиента
			this->rmHeader("Sec-WebSocket-Key");
			// Удаляем заголовок версии WebSocket
			this->rmHeader("Sec-WebSocket-Version");
			// Если подпротоколы существуют
			if(!this->_subs.empty()){
				// Если количество подпротоколов больше 5-ти
				if(this->_subs.size() > 5){
					// Список желаемых подпротоколов
					string subs = "";
					// Переходим по всему списку подпротоколов
					for(auto & sub : this->_subs){
						// Если подпротокол уже не пустой, добавляем разделитель
						if(!subs.empty()) subs.append(", ");
						// Добавляем в список желаемый подпротокол
						subs.append(sub);
					}
					// Добавляем заголовок сабпротокола
					this->header("Sec-WebSocket-Protocol", subs);
				// Если подпротоколов слишком много
				} else {
					// Получаем список заголовков
					const auto & headers = this->web.headers();
					// Переходим по всему списку подпротоколов
					for(auto & sub : this->_subs)
						// Добавляем полученный заголовок
						const_cast <unordered_multimap <string, string> *> (&headers)->insert({{"Sec-WebSocket-Protocol", sub}});
				}
			}
			// Если метод компрессии указан
			if(this->crypt || (this->_compress != compress_t::NONE)){
				// Список расширений протокола
				string extensions = "";
				// Если метод компрессии указан
				if(this->_compress != compress_t::NONE){
					// Если метод компрессии выбран Deflate
					if(this->_compress == compress_t::DEFLATE){
						// Устанавливаем тип компрессии Deflate
						extensions = "permessage-deflate";
						// Если запрещено переиспользовать контекст компрессии для сервера
						if(!this->_server.takeover && this->_client.takeover)
							// Устанавливаем запрет на переиспользование контекста компрессии для сервера
							extensions.append("; server_no_context_takeover");
						// Если запрещено переиспользовать контекст компрессии для клиента
						else if(this->_server.takeover && !this->_client.takeover)
							// Устанавливаем запрет на переиспользование контекста компрессии для клиента
							extensions.append("; client_no_context_takeover");
						// Если запрещено переиспользовать контекст компрессии для клиента и сервера
						else if(!this->_server.takeover && !this->_client.takeover)
							// Устанавливаем запрет на переиспользование контекста компрессии для клиента и сервера
							extensions.append("; server_no_context_takeover; client_no_context_takeover");
						// Устанавливаем максимальный размер скользящего окна
						extensions.append("; client_max_window_bits");
					// Если метод компрессии выбран GZip
					} else if(this->_compress == compress_t::GZIP)
						// Устанавливаем тип компрессии GZip
						extensions = "permessage-gzip";
					// Если метод компрессии выбран Brotli
					else if(this->_compress == compress_t::BROTLI)
						// Устанавливаем тип компрессии Brotli
						extensions = "permessage-br";
					// Если данные должны быть зашифрованны
					if(this->crypt) extensions.append(this->fmk->format("; permessage-encrypt=%u", static_cast <u_short> (this->hash.cipher())));
				// Если метод компрессии не указан но указан режим шифрования
				} else if(this->crypt) extensions = this->fmk->format("permessage-encrypt=%u", static_cast <u_short> (this->hash.cipher()));
				// Добавляем полученный заголовок
				this->header("Sec-WebSocket-Extensions", extensions);
			}
			// Если версия протокола меньше 2.0
			if(this->web.request().version < 2.0f){
				// Генерируем ключ клиента
				this->_key = this->key();
				// Добавляем заголовок апгрейд
				this->header("Upgrade", "WebSocket");
				// Добавляем заголовок подключения
				this->header("Connection", "Keep-Alive, Upgrade");
				// Добавляем заголовок ключ клиента
				this->header("Sec-WebSocket-Key", this->_key);
			// Если версия протокола выше или соответствует 2.0, добавляем заголовок протокола подключения
			} else this->header(":protocol", "websocket");
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
			// Удаляем статус ответа
			this->rmHeader(":status");
			// Удаляем заголовок хеша ключа
			this->rmHeader("Sec-WebSocket-Accept");
			// Если метод компрессии указан
			if(this->crypt || (this->_compress != compress_t::NONE)){
				// Список расширений протокола
				string extensions = "";
				// Если метод компрессии указан
				if(this->_compress != compress_t::NONE){
					// Если метод компрессии выбран Deflate
					if(this->_compress == compress_t::DEFLATE){
						// Устанавливаем тип компрессии Deflate
						extensions = "permessage-deflate";
						// Если запрещено переиспользовать контекст компрессии для клиента и сервера
						if(!this->_server.takeover && !this->_client.takeover)
							// Устанавливаем запрет на переиспользование контекста компрессии для клиента и сервера
							extensions.append("; server_no_context_takeover; client_no_context_takeover");
						// Если запрещено переиспользовать контекст компрессии для клиента
						else if(this->_server.takeover && !this->_client.takeover)
							// Устанавливаем запрет на переиспользование контекста компрессии для клиента
							extensions.append("; client_no_context_takeover");
						// Если запрещено переиспользовать контекст компрессии для сервера
						else if(!this->_server.takeover && this->_client.takeover)
							// Устанавливаем запрет на переиспользование контекста компрессии для сервера
							extensions.append("; server_no_context_takeover");
						// Если требуется указать количество байт
						if(this->_server.wbit > 0)
							// Выполняем установку размера скользящего окна
							extensions.append(this->fmk->format("; server_max_window_bits=%u", this->_server.wbit));
					// Если метод компрессии выбран GZip
					} else if(this->_compress == compress_t::GZIP)
						// Устанавливаем тип компрессии GZip
						extensions = "permessage-gzip";
					// Если метод компрессии выбран Brotli
					else if(this->_compress == compress_t::BROTLI)
						// Устанавливаем тип компрессии Brotli
						extensions = "permessage-br";
					// Если данные должны быть зашифрованны
					if(this->crypt) extensions.append(this->fmk->format("; permessage-encrypt=%u", static_cast <u_short> (this->hash.cipher())));
				// Если метод компрессии не указан но указан режим шифрования
				} else if(this->crypt) extensions = this->fmk->format("permessage-encrypt=%u", static_cast <u_short> (this->hash.cipher()));
				// Добавляем заголовок расширений
				this->header("Sec-WebSocket-Extensions", extensions);
			}
			// Добавляем в чёрный список заголовок Content-Type
			this->addBlack("Content-Type");
			// Если подпротокол выбран
			if(!this->_sub.empty())
				// Добавляем заголовок сабпротокола
				this->header("Sec-WebSocket-Protocol", this->_sub.c_str());
			// Если версия протокола меньше 2.0
			if(this->web.response().version < 2.0f){
				// Добавляем заголовок подключения
				this->header("Connection", "Upgrade");
				// Добавляем заголовок апгрейд
				this->header("Upgrade", "WebSocket");
				// Выполняем генерацию хеша ключа
				const string & sha1 = this->sha1();
				// Если SHA1-ключ не сгенерирован
				if(sha1.empty()){
					// Если ключ клиента и сервера не согласованы, выводим сообщение об ошибке
					this->log->print("SHA1 key could not be generated, no further work possiblet", log_t::flag_t::CRITICAL);
					// Выходим из приложения
					exit(EXIT_FAILURE);
				}
				// Добавляем заголовок хеша ключа
				this->header("Sec-WebSocket-Accept", sha1.c_str());
			}
		} break;
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
		for(size_t c = 0; c < 16; c++) nonce += static_cast <char> (dist(rd));
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
			this->log->print("%s", log_t::flag_t::CRITICAL, error.what());
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
		const string text = (this->_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
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
		// Получаем размер поддерживаемого сабпротокола
		length = this->_sub.size();
		// Устанавливаем размер поддерживаемого сабпротокола
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Добавляем поддерживаемый сабпротокол
		result.insert(result.end(), this->_sub.begin(), this->_sub.end());
		// Получаем размер ключа клиента
		length = this->_key.size();
		// Устанавливаем размер ключа клиента
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Добавляем ключ клиента
		result.insert(result.end(), this->_key.begin(), this->_key.end());
		// Получаем количество поддерживаемых сабпротоколов
		count = this->_subs.size();
		// Устанавливаем количество поддерживаемых сабпротоколов
		result.insert(result.end(), (const char *) &count, (const char *) &count + sizeof(count));
		// Выполняем перебор всех поддерживаемых сабпротоколов
		for(auto & sub : this->_subs){
			// Получаем размер поддерживаемого сабпротокола
			length = sub.size();
			// Устанавливаем размер поддерживаемого сабпротокола
			result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
			// Устанавливаем данные поддерживаемого сабпротокола
			result.insert(result.end(), sub.begin(), sub.end());
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
		// Выполняем получение размера поддерживаемого сабпротокола
		::memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Выполняем выделение памяти для поддерживаемого сабпротокола
		this->_sub.resize(length, 0);
		// Выполняем получение поддерживаемого сабпротокола
		::memcpy((void *) this->_sub.data(), data.data() + offset, length);
		// Выполняем смещение в буфере
		offset += length;
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
		// Выполняем получение количества поддерживаемых сабпротоколов
		::memcpy((void *) &count, data.data() + offset, sizeof(count));
		// Выполняем смещение в буфере
		offset += sizeof(count);
		// Выполняем сброс списка поддерживаемых сабпротоколов
		this->_subs.clear();
		// Выполняем последовательную загрузку всех поддерживаемых сабпротоколов
		for(size_t i = 0; i < count; i++){
			// Выполняем получение размера поддерживаемого сабпротокола
			::memcpy((void *) &length, data.data() + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Выделяем память для поддерживаемого сабпротокола
			string sub(length, 0);
			// Выполняем получение поддерживаемого сабпротокола
			::memcpy((void *) sub.data(), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
			// Если сабпротокол получен, добавляем его в список
			if(!sub.empty()) this->_subs.emplace(std::move(sub));
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
	// Выполняем сброс поддерживаемого сабпротокола
	this->_sub.clear();
	// Выполняем сброс поддерживаемых сабпротоколов
	this->_subs.clear();
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
 * isHandshake Метод получения флага рукопожатия
 * @return флаг получения рукопожатия
 */
bool awh::WCore::isHandshake() noexcept {
	// Результат работы функции
	bool result = (this->state == state_t::HANDSHAKE);
	// Если рукопожатие не выполнено
	if(!result){
		// Выполняем проверку на удачное завершение запроса
		result = (this->stath == stath_t::GOOD);
		// Если результат удачный
		if(result)
			// Выполняем проверку версии протокола
			result = this->checkVer();
		// Если подключение не выполнено
		else return result;
		// Если результат удачный, проверяем произошло ли переключение протокола
		if(result) result = this->checkUpgrade();
		// Если версия протокола не соответствует
		else {
			// Выводим сообщение об ошибке
			this->log->print("protocol version not supported", log_t::flag_t::CRITICAL);
			// Выходим из функции
			return result;
		}
		// Если результат удачный, проверяем ключ клиента
		if(result) result = this->checkKey();
		// Если протокол не был переключён
		else {
			// Выводим сообщение об ошибке
			this->log->print("protocol not upgraded", log_t::flag_t::CRITICAL);
			// Выходим из функции
			return result;
		}
		// Если рукопожатие выполнено, устанавливаем стейт рукопожатия
		if(result) this->state = state_t::HANDSHAKE;
		// Если ключ клиента и сервера не согласованы, выводим сообщение об ошибке
		else this->log->print("client and server keys are inconsistent", log_t::flag_t::CRITICAL);
	}
	// Выводим результат
	return result;
}
/**
 * checkUpgrade Метод получения флага переключения протокола
 * @return флага переключения протокола
 */
bool awh::WCore::checkUpgrade() const noexcept {
	// Результат работы функции
	bool result = false;
	// Получаем значение заголовка Upgrade
	const string & upgrade = this->web.header("upgrade");
	// Получаем значение заголовка Connection
	const string & connection = this->web.header("connection");
	// Если заголовки расширений найдены
	if(!upgrade.empty() && !connection.empty()){
		// Переводим значение заголовка Connection в нижний регистр
		this->fmk->transform(connection, fmk_t::transform_t::LOWER);
		// Если заголовки соответствуют
		result = (this->fmk->compare(upgrade, "websocket") && this->fmk->exists("upgrade", connection));
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
			if(!req.url.empty() && (req.method == web_t::method_t::GET))
				// Устанавливаем парарметры запроса
				this->web.request(req);
			// Если данные переданы неверные
			else {
				// Выводим сообщение, что данные переданы неверные
				this->log->print("Address or request method for WebSocket-client is incorrect", log_t::flag_t::CRITICAL);
				// Выходим из функции
				return vector <char> ();
			}
		} break;
		// Если нужно сформировать данные ответа
		case static_cast <uint8_t> (process_t::RESPONSE): {
			// Получаем объект ответа клиенту
			const web_t::res_t & res = static_cast <const web_t::res_t &> (provider);
			// Если параметры запроса получены
			if(res.code == 101)
				// Устанавливаем параметры ответа
				this->web.response(res);
			// Если данные переданы неверные
			else {
				// Выводим сообщение, что данные переданы неверные
				this->log->print("WebSocket-server response code set incorrectly", log_t::flag_t::CRITICAL);
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
			if(!req.url.empty() && (req.method == web_t::method_t::CONNECT))
				// Устанавливаем парарметры запроса
				this->web.request(req);
			// Если данные переданы неверные
			else {
				// Выводим сообщение, что данные переданы неверные
				this->log->print("Address or request method for WebSocket-client is incorrect", log_t::flag_t::CRITICAL);
				// Выходим из функции
				return vector <pair <string, string>> ();
			}
		} break;
		// Если нужно сформировать данные ответа
		case static_cast <uint8_t> (process_t::RESPONSE): {
			// Получаем объект ответа клиенту
			const web_t::res_t & res = static_cast <const web_t::res_t &> (provider);
			// Если параметры запроса получены
			if(res.code == 200)
				// Устанавливаем параметры ответа
				this->web.response(res);
			// Если данные переданы неверные
			else {
				// Выводим сообщение, что данные переданы неверные
				this->log->print("WebSocket-server response code set incorrectly", log_t::flag_t::CRITICAL);
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
 * sub Метод получения выбранного сабпротокола
 * @return выбранный сабпротокол
 */
const string & awh::WCore::sub() const noexcept {
	// Выводим выбранный сабпротокол
	return this->_sub;
}
/**
 * setSub Метод установки подпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::WCore::sub(const string & sub) noexcept {
	// Устанавливаем подпротокол
	if(!sub.empty()) this->_subs.emplace(sub);
}
/**
 * subs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::WCore::subs(const vector <string> & subs) noexcept {
	// Если список подпротоколов получен
	if(!subs.empty()){
		// Переходим по всем подпротоколам
		for(auto & sub : subs)
			// Устанавливаем подпротокол
			this->sub(sub);
	}
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
