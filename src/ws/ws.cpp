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
#include <ws/ws.hpp>

/**
 * key Метод генерации ключа
 * @return сгенерированный ключ
 */
const string awh::WS::key() const noexcept {
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
const string awh::WS::sha1() const noexcept {
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
vector <char> awh::WS::dump() const noexcept {
	// Результат работы функции
	vector <char> result;
	{
		// Длина строки, количество элементов
		size_t length = 0, count = 0;
		// Выполняем получение дампа основного класса
		const auto & dump = reinterpret_cast <http_t *> (const_cast <ws_t *> (this))->dump();
		// Получаем размер дамп бинарных данных модуля
		length = dump.size();
		// Устанавливаем размер дампа бинарных данных модуля
		result.insert(result.end(), (const char *) &length, (const char *) &length + sizeof(length));
		// Добавляем дамп бинарных данных модуля
		result.insert(result.end(), dump.begin(), dump.end());
		// Устанавливаем размер скользящего окна клиента
		result.insert(result.end(), (const char *) &this->_wbitClient, (const char *) &this->_wbitClient + sizeof(this->_wbitClient));
		// Устанавливаем размер скользящего окна сервера
		result.insert(result.end(), (const char *) &this->_wbitServer, (const char *) &this->_wbitServer + sizeof(this->_wbitServer));
		// Устанавливаем флаг запрета переиспользования контекста компрессии для клиента
		result.insert(result.end(), (const char *) &this->_noClientTakeover, (const char *) &this->_noClientTakeover + sizeof(this->_noClientTakeover));
		// Устанавливаем флаг запрета переиспользования контекста компрессии для сервера
		result.insert(result.end(), (const char *) &this->_noServerTakeover, (const char *) &this->_noServerTakeover + sizeof(this->_noServerTakeover));
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
void awh::WS::dump(const vector <char> & data) noexcept {
	// Если данные бинарного дампа переданы
	if(!data.empty()){
		// Длина строки, количество элементов и смещение в буфере
		size_t length = 0, count = 0, offset = 0;
		// Выполняем получение размера дампа бинарных данных модуля
		memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Создаём бинарный буфер дампа
		vector <char> dump(length, 0);
		// Выполняем получение бинарного буфера дампа
		memcpy((void *) dump.data(), data.data() + offset, length);
		// Выполняем смещение в буфере
		offset += length;
		// Выполняем установку бинарного буфера данных
		reinterpret_cast <http_t *> (const_cast <ws_t *> (this))->dump(dump);
		// Выполняем получение размера скользящего окна клиента
		memcpy((void *) &this->_wbitClient, data.data() + offset, sizeof(this->_wbitClient));
		// Выполняем смещение в буфере
		offset += sizeof(this->_wbitClient);
		// Выполняем получение размера скользящего окна сервера
		memcpy((void *) &this->_wbitServer, data.data() + offset, sizeof(this->_wbitServer));
		// Выполняем смещение в буфере
		offset += sizeof(this->_wbitServer);
		// Выполняем получение размера флага запрета переиспользования контекста компрессии для клиента
		memcpy((void *) &this->_noClientTakeover, data.data() + offset, sizeof(this->_noClientTakeover));
		// Выполняем смещение в буфере
		offset += sizeof(this->_noClientTakeover);
		// Выполняем получение размера флага запрета переиспользования контекста компрессии для сервера
		memcpy((void *) &this->_noServerTakeover, data.data() + offset, sizeof(this->_noServerTakeover));
		// Выполняем смещение в буфере
		offset += sizeof(this->_noServerTakeover);
		// Выполняем получение метода сжатия данных запроса/ответа
		memcpy((void *) &this->_compress, data.data() + offset, sizeof(this->_compress));
		// Выполняем смещение в буфере
		offset += sizeof(this->_compress);
		// Выполняем получение размера поддерживаемого сабпротокола
		memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Выполняем выделение памяти для поддерживаемого сабпротокола
		this->_sub.resize(length, 0);
		// Выполняем получение поддерживаемого сабпротокола
		memcpy((void *) this->_sub.data(), data.data() + offset, length);
		// Выполняем смещение в буфере
		offset += length;
		// Выполняем получение размера ключа клиента
		memcpy((void *) &length, data.data() + offset, sizeof(length));
		// Выполняем смещение в буфере
		offset += sizeof(length);
		// Выполняем выделение памяти для ключа клиента
		this->_key.resize(length, 0);
		// Выполняем получение ключа клиента
		memcpy((void *) this->_key.data(), data.data() + offset, length);
		// Выполняем смещение в буфере
		offset += length;
		// Выполняем получение количества поддерживаемых сабпротоколов
		memcpy((void *) &count, data.data() + offset, sizeof(count));
		// Выполняем смещение в буфере
		offset += sizeof(count);
		// Выполняем сброс списка поддерживаемых сабпротоколов
		this->_subs.clear();
		// Выполняем последовательную загрузку всех поддерживаемых сабпротоколов
		for(size_t i = 0; i < count; i++){
			// Выполняем получение размера поддерживаемого сабпротокола
			memcpy((void *) &length, data.data() + offset, sizeof(length));
			// Выполняем смещение в буфере
			offset += sizeof(length);
			// Выделяем память для поддерживаемого сабпротокола
			string sub(length, 0);
			// Выполняем получение поддерживаемого сабпротокола
			memcpy((void *) sub.data(), data.data() + offset, length);
			// Выполняем смещение в буфере
			offset += length;
			// Если сабпротокол получен, добавляем его в список
			if(!sub.empty()) this->_subs.emplace(move(sub));
		}
	}
}
/**
 * clean Метод очистки собранных данных
 */
void awh::WS::clean() noexcept {
	// Выполняем очистку родительских данных
	http_t::clear();
	// Выполняем сброс ключа клиента
	this->_key.clear();
	// Выполняем сброс поддерживаемого сабпротокола
	this->_sub.clear();
	// Выполняем сброс поддерживаемых сабпротоколов
	this->_subs.clear();
	// Выполняем сброс размера скользящего окна для клиента
	this->_wbitClient = GZIP_MAX_WBITS;
	// Выполняем сброс размера скользящего окна для сервера
	this->_wbitServer = GZIP_MAX_WBITS;
}
/**
 * compress Метод получения метода компрессии
 * @return метод компрессии сообщений
 */
awh::Http::compress_t awh::WS::compress() const noexcept {
	// Выводим метод компрессии сообщений
	return this->_compress;
}
/**
 * compress Метод установки метода компрессии
 * @param compress метод компрессии сообщений
 */
void awh::WS::compress(const compress_t compress) noexcept {
	// Устанавливаем метод компрессии сообщений
	this->_compress = compress;
}
/**
 * isHandshake Метод получения флага рукопожатия
 * @return флаг получения рукопожатия
 */
bool awh::WS::isHandshake() noexcept {
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
bool awh::WS::checkUpgrade() const noexcept {
	// Результат работы функции
	bool result = false;
	// Получаем значение заголовка Upgrade
	string upgrade = this->web.header("upgrade");
	// Получаем значение заголовка Connection
	string connection = this->web.header("connection");
	// Если заголовки расширений найдены
	if(!upgrade.empty() && !connection.empty()){
		// Переводим значение заголовка Upgrade в нижний регистр
		this->fmk->transform(upgrade, fmk_t::transform_t::LOWER);
		// Переводим значение заголовка Connection в нижний регистр
		this->fmk->transform(connection, fmk_t::transform_t::LOWER);
		// Если заголовки соответствуют
		result = ((upgrade.compare("websocket") == 0) && (connection.find("upgrade") != string::npos));
	}
	// Выводим результат
	return result;
}
/**
 * wbitClient Метод получения размер скользящего окна для клиента
 * @return размер скользящего окна
 */
short awh::WS::wbitClient() const noexcept {
	// Выводим размер скользящего окна
	return this->_wbitClient;
}
/**
 * wbitServer Метод получения размер скользящего окна для сервера
 * @return размер скользящего окна
 */
short awh::WS::wbitServer() const noexcept {
	// Выводим размер скользящего окна
	return this->_wbitServer;
}
/**
 * response Метод создания ответа
 * @return буфер данных запроса в бинарном виде
 */
vector <char> awh::WS::response() noexcept {
	// Результат работы функции
	vector <char> result;
	// Выполняем генерацию хеша ключа
	const string & sha1 = this->sha1();
	// Если хэш ключа сгенерирован
	if(!sha1.empty()){
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
					if(this->_noServerTakeover && this->_noClientTakeover)
						// Устанавливаем запрет на переиспользование контекста компрессии для клиента и сервера
						extensions.append("; server_no_context_takeover; client_no_context_takeover");
					// Если запрещено переиспользовать контекст компрессии для клиента
					else if(!this->_noServerTakeover && this->_noClientTakeover)
						// Устанавливаем запрет на переиспользование контекста компрессии для клиента
						extensions.append("; client_no_context_takeover");
					// Если запрещено переиспользовать контекст компрессии для сервера
					else if(this->_noServerTakeover && !this->_noClientTakeover)
						// Устанавливаем запрет на переиспользование контекста компрессии для сервера
						extensions.append("; server_no_context_takeover");
					// Если требуется указать количество байт
					if(this->_wbitServer > 0) extensions.append(this->fmk->format("; server_max_window_bits=%u", this->_wbitServer));
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
		// Добавляем в чёрный список заголовок Content-Type
		this->addBlack("Content-Type");
		// Если подпротокол выбран
		if(!this->_sub.empty())
			// Добавляем заголовок сабпротокола
			this->header("Sec-WebSocket-Protocol", this->_sub.c_str());
		// Добавляем заголовок подключения
		this->header("Connection", "Upgrade");
		// Добавляем заголовок апгрейд
		this->header("Upgrade", "WebSocket");
		// Добавляем заголовок хеша ключа
		this->header("Sec-WebSocket-Accept", sha1.c_str());
		// Выводим результат
		return http_t::response(static_cast <u_int> (101));
	}
	// Выводим результат
	return result;
}
/**
 * request Метод создания запроса
 * @param url объект параметров REST запроса
 * @return    буфер данных запроса в бинарном виде
 */
vector <char> awh::WS::request(const uri_t::url_t & url) noexcept {
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
			// Добавляем полученный заголовок
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
				if(this->_noServerTakeover && !this->_noClientTakeover)
					// Устанавливаем запрет на переиспользование контекста компрессии для сервера
					extensions.append("; server_no_context_takeover");
				// Если запрещено переиспользовать контекст компрессии для клиента
				else if(!this->_noServerTakeover && this->_noClientTakeover)
					// Устанавливаем запрет на переиспользование контекста компрессии для клиента
					extensions.append("; client_no_context_takeover");
				// Если запрещено переиспользовать контекст компрессии для клиента и сервера
				else if(this->_noServerTakeover && this->_noClientTakeover)
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
	// Генерируем ключ клиента
	this->_key = this->key();
	// Добавляем заголовок Accept
	this->header("Accept", "*/*");
	// Добавляем заголовок апгрейд
	this->header("Upgrade", "WebSocket");
	// Добавляем заголовок подключения
	this->header("Connection", "Keep-Alive, Upgrade");
	// Добавляем заголовок версии WebSocket
	this->header("Sec-WebSocket-Version", to_string(WS_VERSION));
	// Добавляем заголовок ключ клиента
	this->header("Sec-WebSocket-Key", this->_key);
	// Добавляем заголовок отключения кеширования
	this->header("Pragma", "No-Cache");
	// Добавляем заголовок отключения кеширования
	this->header("Cache-Control", "No-Cache");
	// Устанавливаем заголовок типа запроса
	this->header("Sec-Fetch-Mode", "WebSocket");
	// Устанавливаем заголовок места назначения запроса
	this->header("Sec-Fetch-Dest", "WebSocket");
	// Устанавливаем заголовок требования сжимать содержимое ответов
	this->header("Accept-Encoding", "gzip, deflate, br");
	// Устанавливаем заголовок поддерживаемых языков
	this->header("Accept-Language", HTTP_HEADER_ACCEPTLANGUAGE);
	// Выводим результат
	return http_t::request(url, web_t::method_t::GET);
}
/**
 * sub Метод получения выбранного сабпротокола
 * @return выбранный сабпротокол
 */
const string & awh::WS::sub() const noexcept {
	// Выводим выбранный сабпротокол
	return this->_sub;
}
/**
 * setSub Метод установки подпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::WS::sub(const string & sub) noexcept {
	// Устанавливаем подпротокол
	if(!sub.empty()) this->_subs.emplace(sub);
}
/**
 * subs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::WS::subs(const vector <string> & subs) noexcept {
	// Если список подпротоколов получен
	if(!subs.empty()){
		// Переходим по всем подпротоколам
		for(auto & sub : subs)
			// Устанавливаем подпротокол
			this->sub(sub);
	}
}
/**
 * clientTakeover Метод получения флага переиспользования контекста компрессии для клиента
 * @return флаг запрета переиспользования контекста компрессии для клиента
 */
bool awh::WS::clientTakeover() const noexcept {
	// Выводим результат проверки
	return !this->_noClientTakeover;
}
/**
 * clientTakeover Метод установки флага переиспользования контекста компрессии для клиента
 * @param flag флаг запрета переиспользования контекста компрессии для клиента
 */
void awh::WS::clientTakeover(const bool flag) noexcept {
	// Устанавливаем флаг запрета переиспользования контекста компрессии для клиента
	this->_noClientTakeover = !flag;
}
/**
 * serverTakeover Метод получения флага переиспользования контекста компрессии для сервера
 * @return флаг запрета переиспользования контекста компрессии для сервера
 */
bool awh::WS::serverTakeover() const noexcept {
	// Выводим результат проверки
	return !this->_noServerTakeover;
}
/**
 * serverTakeover Метод установки флага переиспользования контекста компрессии для сервера
 * @param flag флаг запрета переиспользования контекста компрессии для сервера
 */
void awh::WS::serverTakeover(const bool flag) noexcept {
	// Устанавливаем флаг запрета переиспользования контекста компрессии для сервера
	this->_noServerTakeover = !flag;
}
