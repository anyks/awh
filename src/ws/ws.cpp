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
	// Выполняем перехват ошибки
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
		result = this->key();
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
 * clear Метод очистки собранных данных
 */
void awh::WS::clear() noexcept {
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
	// Выполняем сброс метода сжатия данных
	this->compress = compress_t::DEFLATE;
}
/**
 * checkUpgrade Метод получения флага переключения протокола
 * @return флага переключения протокола
 */
bool awh::WS::checkUpgrade() const noexcept {
	// Результат работы функции
	bool result = false;
	// Если список заголовков получен
	if(!this->headers.empty()){
		// Выполняем поиск заголовка смены протокола
		auto it = this->headers.find("upgrade");
		// Выполняем поиск заголовка с параметрами подключения
		auto jt = this->headers.find("connection");
		// Если заголовки расширений найдены
		if((it != this->headers.end()) && (jt != this->headers.end())){
			// Получаем значение заголовка Upgrade
			const string & upgrade = this->fmk->toLower(it->second);
			// Получаем значение заголовка Connection
			const string & connection = this->fmk->toLower(jt->second);
			// Если заголовки соответствуют
			result = ((upgrade.compare("websocket") == 0) && (connection.compare("upgrade") == 0));
		}
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
vector <char> awh::WS::response() const noexcept {
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
					extensions = "permessage-deflate; server_no_context_takeover; client_max_window_bits";
					// Если требуется указать количество байт
					if(this->wbitServer > 0) extensions.append(this->fmk->format("; server_max_window_bits=%u", this->wbitServer));
				// Если метод компрессии выбран GZip
				} else if(this->compress == compress_t::GZIP)
					// Устанавливаем тип компрессии GZip
					extensions = "permessage-gzip; server_no_context_takeover";
				// Если данные должны быть зашифрованны
				if(this->crypt) extensions.append(this->fmk->format("; permessage-encrypt=%u", (u_short) this->aes));
			// Если метод компрессии не указан но указан режим шифрования
			} else if(this->crypt) extensions = this->fmk->format("permessage-encrypt=%u; server_no_context_takeover", (u_short) this->aes);
			// Добавляем полученный заголовок
			this->headers.emplace("Sec-WebSocket-Extensions", extensions);
		}
		// Если подпротокол выбран
		if(!this->sub.empty())
			// Добавляем заголовок сабпротокола
			this->headers.emplace("Sec-WebSocket-Protocol", this->sub.c_str());
		// Добавляем заголовок подключения
		this->headers.emplace("Connection", "upgrade");
		// Добавляем заголовок апгрейд
		this->headers.emplace("Upgrade", "websocket");
		// Добавляем заголовок хеша ключа
		this->headers.emplace("Sec-WebSocket-Accept", hash.c_str());
		// Выводим результат
		return http_t::response(101);
	}
	// Выводим результат
	return result;
}
/**
 * request Метод создания запроса
 * @param url объект параметров REST запроса
 * @return    буфер данных запроса в бинарном виде
 */
vector <char> awh::WS::request(const uri_t::url_t & url) const noexcept {
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
			this->headers.emplace("Sec-WebSocket-Protocol", subs);
		// Если подпротоколов слишком много
		} else {
			// Переходим по всему списку подпротоколов
			for(auto & sub : this->subs)
				// Добавляем полученный заголовок
				this->headers.insert({{"Sec-WebSocket-Protocol", sub}});
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
			// Если данные должны быть зашифрованны
			if(this->crypt) extensions.append(this->fmk->format("; permessage-encrypt=%u", (u_short) this->aes));
		// Если метод компрессии не указан но указан режим шифрования
		} else if(this->crypt) extensions = this->fmk->format("permessage-encrypt=%u", (u_short) this->aes));
		// Добавляем полученный заголовок
		this->headers.emplace("Sec-WebSocket-Extensions", extensions);
	}
	// Генерируем ключ клиента
	this->key = this->getKey();
	// Добавляем заголовок подключения
	this->headers.emplace("Connection", "Upgrade");
	// Добавляем заголовок апгрейд
	this->headers.emplace("Upgrade", "websocket");
	// Добавляем заголовок версии WebSocket
	this->headers.emplace("Sec-WebSocket-Version", WS_VERSION);
	// Добавляем заголовок ключ клиента
	this->headers.emplace("Sec-WebSocket-Key", this->key);
	// Выводим результат
	return http_t::request(url, method_t::GET);
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
