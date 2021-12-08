/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <ws/ws.hpp>

/**
 * getKey Метод генерации ключа
 * @return сгенерированный ключ
 */
const string awh::WS::getKey() const noexcept {
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
		// Выводим в лог сообщение
		this->log->print("%s", log_t::flag_t::CRITICAL, error.what());
		// Выполняем повторно генерацию ключа
		result = this->getKey();
	}
	// Выводим результат
	return result;
}
/**
 * getHash Метод генерации хэша ключа
 * @return сгенерированный хэш ключа клиента
 */
const string awh::WS::getHash() const noexcept {
	// Результат работы функции
	string result = "";
	// Если ключ клиента передан
	if(!this->key.empty()){
		// Создаем контекст
		SHA_CTX ctx;
		// Выполняем инициализацию контекста
		SHA1_Init(&ctx);
		// Массив полученных значений
		u_char digest[20];
		// Формируем магический ключ
		const string text = (this->key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
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
 * clean Метод очистки собранных данных
 */
void awh::WS::clean() noexcept {
	// Выполняем очистку родительских данных
	http_t::clear();
	// Выполняем сброс ключа клиента
	this->key.clear();
	// Выполняем сброс поддерживаемого сабпротокола
	this->sub.clear();
	// Выполняем сброс поддерживаемых сабпротоколов
	this->subs.clear();
	// Выполняем сброс размера скользящего окна для клиента
	this->wbitClient = GZIP_MAX_WBITS;
	// Выполняем сброс размера скользящего окна для сервера
	this->wbitServer = GZIP_MAX_WBITS;
}
/**
 * getCompress Метод получения метода сжатия
 * @return метод сжатия сообщений
 */
awh::Http::compress_t awh::WS::getCompress() const noexcept {
	// Выводим метод сжатия сообщений
	return this->compress;
}
/**
 * setCompress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::WS::setCompress(const compress_t compress) noexcept {
	// Устанавливаем метод сжатия сообщений
	this->compress = compress;
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
		// Если результат удачный, проверяем версию протокола
		if(result) result = this->checkVer();
		// Если результат удачный, проверяем произошло ли переключение протокола
		if(result) result = this->checkUpgrade();
		// Если результат удачный, проверяем ключ клиента
		if(result) result = this->checkKey();
		// Если рукопожатие выполнено, устанавливаем стейт рукопожатия
		if(result) this->state = state_t::HANDSHAKE;
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
	string upgrade = this->web.getHeader("upgrade");
	// Получаем значение заголовка Connection
	string connection = this->web.getHeader("connection");
	// Если заголовки расширений найдены
	if(!upgrade.empty() && !connection.empty()){
		// Переводим значение заголовка Upgrade в нижний регистр
		upgrade = this->fmk->toLower(upgrade);
		// Переводим значение заголовка Connection в нижний регистр
		connection = this->fmk->toLower(connection);
		// Если заголовки соответствуют
		result = ((upgrade.compare("websocket") == 0) && (connection.find("upgrade") != string::npos));
	}
	// Выводим результат
	return result;
}
/**
 * getWbitClient Метод получения размер скользящего окна для клиента
 * @return размер скользящего окна
 */
short awh::WS::getWbitClient() const noexcept {
	// Выводим размер скользящего окна
	return this->wbitClient;
}
/**
 * getWbitServer Метод получения размер скользящего окна для сервера
 * @return размер скользящего окна
 */
short awh::WS::getWbitServer() const noexcept {
	// Выводим размер скользящего окна
	return this->wbitServer;
}
/**
 * getSub Метод получения выбранного сабпротокола
 * @return выбранный сабпротокол
 */
const string & awh::WS::getSub() const noexcept {
	// Выводим выбранный сабпротокол
	return this->sub;
}
/**
 * response Метод создания ответа
 * @return буфер данных запроса в бинарном виде
 */
vector <char> awh::WS::response() noexcept {
	// Результат работы функции
	vector <char> result;
	// Выполняем генерацию хеша ключа
	const string & hash = this->getHash();
	// Если хэш ключа сгенерирован
	if(!hash.empty()){
		// Если метод компрессии указан
		if(this->crypt || (this->compress != compress_t::NONE)){
			// Список расширений протокола
			string extensions = "";
			// Если метод компрессии указан
			if(this->compress != compress_t::NONE){
				// Если метод компрессии выбран Deflate
				if(this->compress == compress_t::DEFLATE){
					// Устанавливаем тип компрессии Deflate
					extensions = "permessage-deflate; server_no_context_takeover";
					// Если требуется указать количество байт
					if(this->wbitServer > 0) extensions.append(this->fmk->format("; server_max_window_bits=%u", this->wbitServer));
				// Если метод компрессии выбран GZip
				} else if(this->compress == compress_t::GZIP)
					// Устанавливаем тип компрессии GZip
					extensions = "permessage-gzip; server_no_context_takeover";
				// Если метод компрессии выбран Brotli
				else if(this->compress == compress_t::BROTLI)
					// Устанавливаем тип компрессии Brotli
					extensions = "permessage-br; server_no_context_takeover";
				// Если данные должны быть зашифрованны
				if(this->crypt) extensions.append(this->fmk->format("; permessage-encrypt=%u", (u_short) this->hash.getAES()));
			// Если метод компрессии не указан но указан режим шифрования
			} else if(this->crypt) extensions = this->fmk->format("permessage-encrypt=%u; server_no_context_takeover", (u_short) this->hash.getAES());
			// Добавляем полученный заголовок
			this->addHeader("Sec-WebSocket-Extensions", extensions);
		}
		// Добавляем в чёрный список заголовок Content-Type
		this->addBlack("Content-Type");
		// Если подпротокол выбран
		if(!this->sub.empty())
			// Добавляем заголовок сабпротокола
			this->addHeader("Sec-WebSocket-Protocol", this->sub.c_str());
		// Добавляем заголовок подключения
		this->addHeader("Connection", "upgrade");
		// Добавляем заголовок апгрейд
		this->addHeader("Upgrade", "websocket");
		// Добавляем заголовок хеша ключа
		this->addHeader("Sec-WebSocket-Accept", hash.c_str());
		// Выводим результат
		return http_t::response((u_int) 101);
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
	if(!this->subs.empty()){
		// Если количество подпротоколов больше 5-ти
		if(this->subs.size() > 5){
			// Список желаемых подпротоколов
			string subs = "";
			// Переходим по всему списку подпротоколов
			for(auto & sub : this->subs){
				// Если подпротокол уже не пустой, добавляем разделитель
				if(!subs.empty()) subs.append(", ");
				// Добавляем в список желаемый подпротокол
				subs.append(sub);
			}
			// Добавляем полученный заголовок
			this->addHeader("Sec-WebSocket-Protocol", subs);
		// Если подпротоколов слишком много
		} else {
			// Получаем список заголовков
			const auto & headers = this->web.getHeaders();
			// Переходим по всему списку подпротоколов
			for(auto & sub : this->subs)
				// Добавляем полученный заголовок
				const_cast <unordered_multimap <string, string> *> (&headers)->insert({{"Sec-WebSocket-Protocol", sub}});
		}
	}
	// Если метод компрессии указан
	if(this->crypt || (this->compress != compress_t::NONE)){
		// Список расширений протокола
		string extensions = "";
		// Если метод компрессии указан
		if(this->compress != compress_t::NONE){
			// Если метод компрессии выбран Deflate
			if(this->compress == compress_t::DEFLATE)
				// Устанавливаем тип компрессии Deflate
				extensions = "permessage-deflate; client_max_window_bits";
			// Если метод компрессии выбран GZip
			else if(this->compress == compress_t::GZIP)
				// Устанавливаем тип компрессии GZip
				extensions = "permessage-gzip";
			// Если метод компрессии выбран Brotli
			else if(this->compress == compress_t::BROTLI)
				// Устанавливаем тип компрессии Brotli
				extensions = "permessage-br";
			// Если данные должны быть зашифрованны
			if(this->crypt) extensions.append(this->fmk->format("; permessage-encrypt=%u", (u_short) this->hash.getAES()));
		// Если метод компрессии не указан но указан режим шифрования
		} else if(this->crypt) extensions = this->fmk->format("permessage-encrypt=%u", (u_short) this->hash.getAES());
		// Добавляем полученный заголовок
		this->addHeader("Sec-WebSocket-Extensions", extensions);
	}
	// Генерируем ключ клиента
	this->key = this->getKey();
	// Добавляем в чёрный список заголовок Accept
	this->addBlack("Accept");
	// Добавляем заголовок подключения
	this->addHeader("Connection", "Upgrade");
	// Добавляем заголовок апгрейд
	this->addHeader("Upgrade", "websocket");
	// Добавляем заголовок версии WebSocket
	this->addHeader("Sec-WebSocket-Version", to_string(WS_VERSION));
	// Добавляем заголовок ключ клиента
	this->addHeader("Sec-WebSocket-Key", this->key);
	// Выводим результат
	return http_t::request(url, web_t::method_t::GET);
}
/**
 * setSub Метод установки подпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::WS::setSub(const string & sub) noexcept {
	// Устанавливаем подпротокол
	if(!sub.empty()) this->subs.emplace(sub);
}
/**
 * setSubs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::WS::setSubs(const vector <string> & subs) noexcept {
	// Если список подпротоколов получен
	if(!subs.empty()){
		// Переходим по всем подпротоколам
		for(auto & sub : subs)
			// Устанавливаем подпротокол
			this->setSub(sub);
	}
}
