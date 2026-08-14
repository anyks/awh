/**
 * @file hmac.cpp
 * @date 2026-07-14
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Реализация схемы HMAC-авторизации HTTP-сообщений —
 *        сборка канонической базы подписи из покрываемых компонентов запроса,
 *        расчёт и проверка подписи и формирование заголовков Signature и Signature-Input
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Если максимальное число HMAC-nonce не указано
 */
#ifndef AWH_AUTH_HMAC_NONCE_MAX
	/**
	 * Устанавливаем максимальное число принятых HMAC-nonce в 4096
	 */
	#define AWH_AUTH_HMAC_NONCE_MAX 0x1000
#endif

/**
 * Стандартный заголовочный файл
 */
#include <cstdlib>

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/auth/hmac.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статические параметры в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Метод сохранения принятого HMAC-nonce с LRU-вытеснением
	 *
	 * @param sign  параметры подписи
	 * @param nonce одноразовое значение
	 *
	 */
	void rememberNonce(http::auth_t::sign_t & sign, const string & nonce) noexcept {
		// Если nonce уже сохранён — повторно не добавляем
		if(sign.usedNonces.find(nonce) != sign.usedNonces.end())
			// Завершаем сохранение
			return;
		// Если достигнут лимит — удаляем самую старую запись
		if((sign.usedNonces.size() >= AWH_AUTH_HMAC_NONCE_MAX) && !sign.usedNoncesOrder.empty()){
			// Удаляем старейший nonce из множества
			sign.usedNonces.erase(sign.usedNoncesOrder.front());
			// Удаляем старейший nonce из LRU-очереди
			sign.usedNoncesOrder.pop_front();
		}
		// Добавляем nonce в конец LRU-очереди
		const auto pos = sign.usedNoncesOrder.insert(sign.usedNoncesOrder.end(), nonce);
		// Сохраняем принятый nonce (значение + позиция в LRU-очереди)
		sign.usedNonces.emplace(nonce, pos);
	}
	/**
	 * @brief Функция приведения ключей параметров Signature-Input к нижнему регистру
	 *
	 * @param params значение параметров подписи (@signature-params)
	 * @param fmk    объект фреймворка
	 *
	 */
	void normalizeSignatureParamKeys(string & params, const fmk_t * fmk) noexcept {
		// Выполняем поиск конца списка покрываемых компонентов
		const size_t rp = params.find(')');
		// Если список компонентов не найден
		if(rp == string::npos)
			// Завершаем нормализацию
			return;
		// Сохраняем список покрываемых компонентов
		string result = params.substr(0, rp + 1);
		// Извлекаем хвост с параметрами подписи
		const string tail = params.substr(rp + 1);
		// Список параметров подписи
		vector <string> parts;
		// Выполняем разделение параметров подписи
		fmk->split(tail, ";", parts);
		/**
		 * Выполняем перебор всех параметров подписи
		 */
		for(auto & part : parts){
			// Удаляем крайние пробелы у параметра
			fmk->transform(part, fmk_t::transform_t::TRIM);
			// Если параметр пустой - пропускаем его
			if(part.empty())
				// Переходим к следующему параметру
				continue;
			// Выполняем поиск разделителя «ключ=значение»
			const size_t sep = part.find('=');
			// Если разделитель не найден - пропускаем параметр
			if(sep == string::npos)
				// Переходим к следующему параметру
				continue;
			// Извлекаем ключ параметра
			string key = ::move(part.substr(0, sep));
			// Удаляем крайние пробелы у ключа
			fmk->transform(key, fmk_t::transform_t::TRIM);
			// Приводим ключ параметра к нижнему регистру
			fmk->transform(key, fmk_t::transform_t::LOWER_CASE);
			// Добавляем разделитель «;» и ключ параметра в нижнем регистре
			result.append(1, ';');
			// Добавляем ключ параметра в нижнем регистре
			result.append(key);
			// Добавляем разделитель «=» и значение параметра
			result.append(part.substr(sep));
		}
		// Сохраняем нормализованное значение параметров подписи
		params = ::move(result);
	}
};

/**
 * @brief Метод получения текстового имени алгоритма подписи
 *
 * @return название алгоритма подписи (hmac-sha256 и т.д.)
 *
 */
string awh::http::Hmac::algName() const noexcept {
	/**
	 * Определяем алгоритм хэширования
	 */
	switch(static_cast <uint8_t> (this->_params.hash)){
		// Если алгоритм хэширования MD5
		case static_cast <uint8_t> (crypto_t::hash_t::MD5):
			// Выводим имя алгоритма подписи
			return "hmac-md5";
		// Если алгоритм хэширования SHA1
		case static_cast <uint8_t> (crypto_t::hash_t::SHA1):
			// Выводим имя алгоритма подписи
			return "hmac-sha1";
		// Если алгоритм хэширования SHA224
		case static_cast <uint8_t> (crypto_t::hash_t::SHA224):
			// Выводим имя алгоритма подписи
			return "hmac-sha224";
		// Если алгоритм хэширования SHA256
		case static_cast <uint8_t> (crypto_t::hash_t::SHA256):
			// Выводим имя алгоритма подписи
			return "hmac-sha256";
		// Если алгоритм хэширования SHA384
		case static_cast <uint8_t> (crypto_t::hash_t::SHA384):
			// Выводим имя алгоритма подписи
			return "hmac-sha384";
		// Если алгоритм хэширования SHA512
		case static_cast <uint8_t> (crypto_t::hash_t::SHA512):
			// Выводим имя алгоритма подписи
			return "hmac-sha512";
	}
	// По умолчанию используем HMAC-SHA256
	return "hmac-sha256";
}
/**
 * @brief Метод получения значения покрываемого компонента по имени
 *
 * @param name имя компонента
 * @return     значение компонента (пустая строка, если не найден)
 *
 */
string awh::http::Hmac::value(string_view name) const noexcept {
	// Формируем ключ компонента в нижнем регистре
	string search(name);
	// Приводим ключ компонента к нижнему регистру
	this->_fmk->transform(search, fmk_t::transform_t::LOWER_CASE);
	// Если компонент найден в индексе
	if(const auto it = this->_params.sign.componentIndex.find(search); it != this->_params.sign.componentIndex.end())
		// Выводим значение компонента
		return this->_params.sign.components.at(it->second).second;
	// Компонент с указанным именем не найден
	return "";
}
/**
 * @brief Метод формирования значения параметров подписи (@signature-params)
 *
 * @return значение параметров подписи
 *
 */
string awh::http::Hmac::params() noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Получаем ссылку на параметры подписи
		auth_t::sign_t & sign = this->_params.sign;
		// Если штамп времени создания подписи не установлен
		if(sign.date.created == 0)
			// Устанавливаем текущий штамп времени в секундах
			sign.date.created = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::SECONDS);
		// Формируем список покрываемых компонентов из порядка их добавления
		sign.covered.clear();
		// Строка списка покрываемых компонентов
		string list = "(";
		/**
		 * Выполняем перебор всех добавленных компонентов
		 */
		for(auto & component : sign.components){
			// Если список уже содержит компоненты - добавляем разделитель
			if(sign.covered.size() > 0)
				// Добавляем разделитель компонентов
				list.append(1, ' ');
			// Сохраняем имя компонента в порядке покрытия
			sign.covered.push_back(component.first);
			// Формируем копию имени компонента в нижнем регистре
			string cname(component.first);
			// Приводим имя компонента к нижнему регистру
			this->_fmk->transform(cname, fmk_t::transform_t::LOWER_CASE);
			// Добавляем имя компонента в список (в нижнем регистре, в кавычках)
			list.append(this->_fmk->format("\"%s\"", cname.c_str()));
		}
		// Закрываем список покрываемых компонентов
		list.append(1, ')');
		// Формируем параметры подписи: обязательные created и alg
		result = this->_fmk->format("%s;created=%s;alg=\"%s\"", list.c_str(), std::to_string(sign.date.created).c_str(), this->algName().c_str());
		// Если идентификатор ключа установлен
		if(!sign.keyId.empty())
			// Добавляем идентификатор ключа
			result.append(this->_fmk->format(";keyid=\"%s\"", sign.keyId.c_str()));
		// Если штамп времени истечения подписи установлен
		if(sign.date.expires > 0)
			// Добавляем штамп времени истечения подписи
			result.append(this->_fmk->format(";expires=%s", std::to_string(sign.date.expires).c_str()));
		// Если одноразовое значение подписи установлено
		if(!sign.nonce.empty())
			// Добавляем одноразовое значение подписи
			result.append(this->_fmk->format(";nonce=\"%s\"", sign.nonce.c_str()));
		// Если тег приложения установлен
		if(!sign.tag.empty())
			// Добавляем тег приложения
			result.append(this->_fmk->format(";tag=\"%s\"", sign.tag.c_str()));
		// Сохраняем сырое значение параметров подписи
		sign.params = result;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим в лог сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод формирования канонической базы подписи
 *
 * @param params значение параметров подписи (@signature-params)
 * @return       каноническая база подписи
 *
 */
string awh::http::Hmac::base(const string & params) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Выполняем перебор всех покрываемых подписью компонентов
		 */
		for(auto & name : this->_params.sign.covered){
			// Формируем копию имени компонента в нижнем регистре
			string component(name);
			// Приводим имя компонента к нижнему регистру
			this->_fmk->transform(component, fmk_t::transform_t::LOWER_CASE);
			// Формируем строку компонента канонической базы
			result.append(this->_fmk->format("\"%s\": %s\n", component.c_str(), this->value(component).c_str()));
		}
		// Добавляем завершающую строку с параметрами подписи (без переноса строки)
		result.append(this->_fmk->format("\"@signature-params\": %s", params.c_str()));
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(params), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим в лог сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод расчёта BASE64-подписи по канонической базе
 *
 * @param base каноническая база подписи
 * @param key  секретный ключ подписи
 * @return     подпись в формате BASE64
 *
 */
string awh::http::Hmac::sign(const string & base, const string & key) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные для подписи переданы
	if(!base.empty() && !key.empty() && (this->_crypto != nullptr)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Имитовставка запрашивается двоичным видом: по умолчанию модуль
			 * криптографии выдаёт шестнадцатеричную запись, и кодирование её
			 * давало подпись «BASE64 от записи имитовставки» вместо «BASE64 от
			 * самой имитовставки», предписанной RFC 9421. Чужие работы такую
			 * подпись не принимают, а наши не принимали бы чужой
			 */
			// Рассчитываем HMAC над канонической базой (в виде сырых байт)
			const vector <uint8_t> & digest = this->_crypto->hmac <vector <uint8_t>> (key, base, this->_params.hash, crypto_t::format_t::RAW);
			// Если подпись рассчитана
			if(!digest.empty()){
				// Формируем строковый буфер из сырых байт подписи
				const string buffer(digest.begin(), digest.end());
				// Выполняем кодирование подписи в BASE64
				result = this->_crypto->encrypt <string> (buffer, crypto_t::hash_t::NONE, crypto_t::cipher_t::BASE64);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(base, key), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим в лог сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод формирования заголовков подписи (клиент)
 *
 * @param input     ссылка для записи значения заголовка Signature-Input
 * @param signature ссылка для записи значения заголовка Signature
 * @return          результат формирования
 *
 */
bool awh::http::Hmac::build(string & input, string & signature) noexcept {
	// Получаем ссылку на параметры подписи
	auth_t::sign_t & sign = this->_params.sign;
	// Если секретный ключ или покрываемые компоненты не заданы
	if(sign.key.empty() || sign.components.empty())
		// Сообщаем о неудачном формировании
		return false;
	// Формируем значение параметров подписи
	const string & params = this->params();
	// Формируем каноническую базу подписи
	const string & base = this->base(params);
	// Рассчитываем подпись
	const string & sig = this->sign(base, sign.key);
	// Если подпись не рассчитана
	if(sig.empty())
		// Сообщаем о неудачном формировании
		return false;
	// Формируем значение заголовка Signature-Input
	input = this->_fmk->format("%s=%s", sign.label.c_str(), params.c_str());
	// Формируем значение заголовка Signature
	signature = this->_fmk->format("%s=:%s:", sign.label.c_str(), sig.c_str());
	// Сообщаем об успешном формировании
	return true;
}
/**
 * @brief Метод разбора входящего заголовка авторизации
 *
 * @param header значение заголовка (Signature-Input либо Signature)
 * @return       результат разбора
 *
 */
bool awh::http::Hmac::parse(const string_view header) noexcept {
	// Если заголовок содержит список покрываемых компонентов - это Signature-Input
	if(header.find('(') != string::npos)
		// Выполняем разбор как заголовка Signature-Input
		return this->parse("signature-input", header);
	// В противном случае выполняем разбор как заголовка Signature
	return this->parse("signature", header);
}
/**
 * @brief Метод разбора входящего заголовка авторизации с указанием имени
 *
 * @param name   имя входящего заголовка (Signature-Input либо Signature)
 * @param header значение входящего заголовка
 * @return       результат разбора
 *
 */
bool awh::http::Hmac::parse(const string_view name, const string_view header) noexcept {
	// Результат работы функции
	bool result = false;
	// Если имя и значение заголовка переданы
	if(!name.empty() && !header.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Приводим имя заголовка к нижнему регистру
			string field(name);
			// Выполняем перевод имени заголовка в нижний регистр
			this->_fmk->transform(field, fmk_t::transform_t::LOWER_CASE);
			// Получаем ссылку на параметры подписи
			auth_t::sign_t & sign = this->_params.sign;
			// Если разбирается заголовок Signature-Input
			if(field.compare("signature-input") == 0){
				// Получаем значение заголовка
				string value(header);
				// Выполняем поиск разделителя метки подписи
				const size_t eq = value.find('=');
				// Если разделитель не найден - завершаем разбор
				if(eq == string::npos)
					// Сообщаем о неудачном разборе
					return result;
				// Извлекаем и очищаем метку подписи
				string label = ::move(value.substr(0, eq));
				// Удаляем крайние пробелы у метки подписи
				this->_fmk->transform(label, fmk_t::transform_t::TRIM);
				// Устанавливаем метку подписи
				sign.label = ::move(label);
				// Запоминаем метку из Signature-Input для сверки с Signature
				sign.inputLabel = sign.label;
				// Извлекаем сырое значение параметров подписи
				string rest = ::move(value.substr(eq + 1));
				// Удаляем крайние пробелы у параметров подписи
				this->_fmk->transform(rest, fmk_t::transform_t::TRIM);
				// Выполняем поиск границ списка покрываемых компонентов
				const size_t lp = rest.find('('), rp = rest.find(')');
				// Если список покрываемых компонентов найден
				if((result = ((lp != string::npos) && (rp != string::npos) && (rp > lp)))){
					// Сохраняем сырое значение параметров подписи
					sign.params = rest;
					// Извлекаем содержимое списка покрываемых компонентов
					const string inner = ::move(rest.substr(lp + 1, rp - lp - 1));
					// Список покрываемых компонентов
					vector <string> items;
					// Выполняем разделение списка покрываемых компонентов
					this->_fmk->split(inner, " ", items);
					// Очищаем текущий список покрываемых компонентов
					sign.covered.clear();
					/**
					 * Выполняем перебор всех компонентов списка
					 */
					for(auto & item : items){
						// Удаляем крайние пробелы у компонента
						this->_fmk->transform(item, fmk_t::transform_t::TRIM);
						// Если компонент обёрнут в кавычки - удаляем их
						if((item.length() > 1) && (item.front() == '"') && (item.back() == '"'))
							// Снимаем обрамляющие кавычки
							item = ::move(item.substr(1, item.length() - 2));
						// Если компонент получен - добавляем его в список покрываемых
						if(!item.empty())
							// Сохраняем имя покрываемого компонента
							sign.covered.push_back(::move(item));
					}
					// Извлекаем параметры подписи после списка компонентов
					const string tail = ::move(rest.substr(rp + 1));
					// Список параметров подписи
					vector <string> parts;
					// Выполняем разделение параметров подписи
					this->_fmk->split(tail, ";", parts);
					/**
					 * Выполняем перебор всех параметров подписи
					 */
					for(auto & part : parts){
						// Удаляем крайние пробелы у параметра
						this->_fmk->transform(part, fmk_t::transform_t::TRIM);
						// Если параметр пустой - пропускаем его
						if(part.empty())
							// Переходим к следующему параметру
							continue;
						// Выполняем поиск разделителя «ключ=значение»
						const size_t sep = part.find('=');
						// Если разделитель не найден - пропускаем параметр
						if(sep == string::npos)
							// Переходим к следующему параметру
							continue;
						// Извлекаем ключ параметра
						string key = ::move(part.substr(0, sep));
						// Извлекаем значение параметра
						string value = ::move(part.substr(sep + 1));
						// Удаляем крайние пробелы у ключа
						this->_fmk->transform(key, fmk_t::transform_t::TRIM);
						// Удаляем крайние пробелы у значения
						this->_fmk->transform(value, fmk_t::transform_t::TRIM);
						// Приводим ключ параметра к нижнему регистру
						this->_fmk->transform(key, fmk_t::transform_t::LOWER_CASE);
						// Если значение обёрнуто в кавычки - удаляем их
						if((value.length() > 1) && (value.front() == '"') && (value.back() == '"'))
							// Снимаем обрамляющие кавычки
							value = ::move(value.substr(1, value.length() - 2));
						// Если параметр является идентификатором ключа
						if(key.compare("keyid") == 0)
							// Устанавливаем идентификатор ключа
							sign.keyId = ::move(value);
						// Если параметр является алгоритмом подписи
						else if(key.compare("alg") == 0) {
							// Приводим название алгоритма к нижнему регистру
							this->_fmk->transform(value, fmk_t::transform_t::LOWER_CASE);
							// Если алгоритм является HMAC-MD5
							if(value.compare("hmac-md5") == 0)
								// Устанавливаем алгоритм MD5
								this->_params.hash = crypto_t::hash_t::MD5;
							// Если алгоритм является HMAC-SHA1
							else if(value.compare("hmac-sha1") == 0)
								// Устанавливаем алгоритм SHA1
								this->_params.hash = crypto_t::hash_t::SHA1;
							// Если алгоритм является HMAC-SHA224
							else if(value.compare("hmac-sha224") == 0)
								// Устанавливаем алгоритм SHA224
								this->_params.hash = crypto_t::hash_t::SHA224;
							// Если алгоритм является HMAC-SHA256
							else if(value.compare("hmac-sha256") == 0)
								// Устанавливаем алгоритм SHA256
								this->_params.hash = crypto_t::hash_t::SHA256;
							// Если алгоритм является HMAC-SHA384
							else if(value.compare("hmac-sha384") == 0)
								// Устанавливаем алгоритм SHA384
								this->_params.hash = crypto_t::hash_t::SHA384;
							// Если алгоритм является HMAC-SHA512
							else if(value.compare("hmac-sha512") == 0)
								// Устанавливаем алгоритм SHA512
								this->_params.hash = crypto_t::hash_t::SHA512;
						// Если параметр является штампом времени создания
						} else if(key.compare("created") == 0)
							// Устанавливаем штамп времени создания подписи
							sign.date.created = static_cast <uint64_t> (::strtoull(value.c_str(), nullptr, 10));
						// Если параметр является штампом времени истечения
						else if(key.compare("expires") == 0)
							// Устанавливаем штамп времени истечения подписи
							sign.date.expires = static_cast <uint64_t> (::strtoull(value.c_str(), nullptr, 10));
						// Если параметр является одноразовым значением
						else if(key.compare("nonce") == 0)
							// Устанавливаем одноразовое значение подписи
							sign.nonce = ::move(value);
						// Если параметр является тегом приложения
						else if(key.compare("tag") == 0)
							// Устанавливаем тег приложения
							sign.tag = ::move(value);
					}
					/**
					 * В простом режиме приводим ключи параметров к нижнему регистру для лояльной сверки;
					 * в строгом режиме сверка байт-точная — параметры подписи не модифицируются (RFC 9421)
					 */
					if(this->_params.mode.validation != auth_t::mode_t::STRICT)
						// Приводим ключи параметров к нижнему регистру для канонической сверки
						::normalizeSignatureParamKeys(sign.params, this->_fmk);
				}
			// Если разбирается заголовок Signature
			} else if(field.compare("signature") == 0) {
				// Получаем значение заголовка
				string value(header);
				// Выполняем поиск разделителя метки подписи
				const size_t eq = value.find('=');
				// Если разделитель не найден - завершаем разбор
				if(eq == string::npos)
					// Сообщаем о неудачном разборе
					return result;
				// Извлекаем и очищаем метку подписи
				string label = ::move(value.substr(0, eq));
				// Удаляем крайние пробелы у метки подписи
				this->_fmk->transform(label, fmk_t::transform_t::TRIM);
				// Если метка не совпадает с Signature-Input — отклоняем разбор
				if(!sign.inputLabel.empty() && !secureCompare(label, sign.inputLabel))
					// Сообщаем о неудачном разборе
					return result;
				// Устанавливаем метку подписи
				sign.label = ::move(label);
				// Извлекаем значение подписи
				string rest = ::move(value.substr(eq + 1));
				// Удаляем крайние пробелы у значения подписи
				this->_fmk->transform(rest, fmk_t::transform_t::TRIM);
				// Выполняем поиск границ значения подписи
				const size_t c1 = rest.find(':'), c2 = rest.rfind(':');
				// Если границы значения подписи найдены
				if((result = ((c1 != string::npos) && (c2 != string::npos) && (c2 > c1))))
					// Извлекаем подпись в формате BASE64
					sign.signature = ::move(rest.substr(c1 + 1, c2 - c1 - 1));
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(name, header), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим в лог сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки учётных данных (только для сервера)
 *
 * @return результат проверки
 *
 */
bool awh::http::Hmac::check() noexcept {
	// На стороне клиента проверка не требуется
	if(this->_owner == auth_t::owner_t::CLIENT)
		// Подтверждаем корректность
		return true;
	// Получаем ссылку на параметры подписи
	const auth_t::sign_t & sign = this->_params.sign;
	// Если данные, необходимые для проверки, отсутствуют
	if(sign.covered.empty() || sign.params.empty() || sign.signature.empty())
		// Сообщаем о неудачной проверке
		return false;
	// Штамп created обязателен для проверки подписи на сервере
	if(sign.date.created == 0)
		// Сообщаем о неудачной проверке
		return false;
	// В строгом режиме алгоритм подписи должен совпадать с настроенным (защита от подмены алгоритма)
	if((this->_params.mode.validation == auth_t::mode_t::STRICT) && (this->_params.hash != this->_params.scheme))
		// Сообщаем о неудачной проверке
		return false;
	// Метки Signature-Input и Signature должны совпадать
	if(!sign.inputLabel.empty() && !secureCompare(sign.label, sign.inputLabel))
		// Сообщаем о неудачной проверке
		return false;
	// Получаем текущий штамп времени в секундах
	const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::SECONDS);
	// Допустимое расхождение локальных часов
	const uint64_t skew = this->_params.mode.clockSkew;
	// Если подпись создана слишком далеко в будущем
	if((sign.date.created > 0) && (now + skew < sign.date.created))
		// Сообщаем о неудачной проверке
		return false;
	// Если срок действия подписи истёк
	if((sign.date.expires > 0) && (now > (sign.date.expires + skew)))
		// Сообщаем о неудачной проверке
		return false;
	// Определяем эффективный лимит возраста подписи без expires
	uint64_t maxAge = this->_params.mode.signMaxAge;
	// В строгом режиме при отсутствии явного лимита применяем значение по умолчанию (ограниченный срок жизни подписи)
	if((maxAge == 0) && (this->_params.mode.validation == auth_t::mode_t::STRICT))
		// Устанавливаем максимальный возраст подписи по умолчанию для строгого режима
		maxAge = this->_params.mode.signStrictMaxAge;
	// Если expires не задан — проверяем максимальный возраст подписи
	if((sign.date.expires == 0) && (maxAge > 0) && (now > (sign.date.created + maxAge + skew)))
		// Сообщаем о неудачной проверке
		return false;
	// Если задан одноразовый nonce — проверяем повторное использование
	if(!sign.nonce.empty()){
		// Если nonce уже был принят ранее
		if(sign.usedNonces.find(sign.nonce) != sign.usedNonces.end())
			// Сообщаем о неудачной проверке
			return false;
	}
	// Извлекаем секретный ключ подписи (по идентификатору либо из параметров)
	const string key = (this->_params.callback.extractKey != nullptr ? this->_params.callback.extractKey(sign.keyId) : sign.key);
	// Если секретный ключ не получен
	if(key.empty())
		// Сообщаем о неудачной проверке
		return false;
	// Формируем каноническую базу подписи по сохранённым параметрам
	const string & base = this->base(sign.params);
	// Рассчитываем ожидаемую подпись
	const string & signature = this->sign(base, key);
	// Если подпись не совпадает
	if(signature.empty() || !secureCompare(signature, sign.signature))
		// Сообщаем о неудачной проверке
		return false;
	// Если задан одноразовый nonce — фиксируем его как использованный
	if(!sign.nonce.empty())
		// Сохраняем nonce с LRU-вытеснением
		::rememberNonce(this->_params.sign, sign.nonce);
	// Подтверждаем успешную проверку подписи
	return true;
}
/**
 * @brief Метод формирования исходящего заголовка авторизации
 *
 * @details При full = false возвращается значение заголовка Signature,
 *          при full = true возвращаются оба заголовка (Signature-Input и Signature)
 *
 * @param full режим вывода вместе с именами заголовков
 * @return     значение заголовка (заголовков) авторизации
 *
 */
string awh::http::Hmac::header(const bool full) noexcept {
	// Результат работы функции
	string result = "";
	// Подпись формируется только на стороне клиента
	if(this->_owner == auth_t::owner_t::CLIENT){
		// Значения заголовков Signature-Input и Signature
		string input = "", signature = "";
		// Если заголовки подписи сформированы
		if(this->build(input, signature)){
			// Если требуется вывести заголовки вместе с их именами
			if(full)
				// Формируем оба заголовка подписи
				result = this->_fmk->format("Signature-Input: %s\r\nSignature: %s\r\n", input.c_str(), signature.c_str());
			// Иначе возвращаем только значение заголовка Signature
			else result = signature;
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод формирования набора исходящих заголовков авторизации
 *
 * @param result контейнер для набора заголовков (Signature-Input и Signature)
 *
 */
void awh::http::Hmac::headers(vector <pair <string, string>> & result) noexcept {
	// Подпись формируется только на стороне клиента
	if(this->_owner == auth_t::owner_t::CLIENT){
		// Значения заголовков Signature-Input и Signature
		string input = "", signature = "";
		// Если заголовки подписи сформированы
		if(this->build(input, signature)){
			// Добавляем заголовок Signature-Input
			result.emplace_back("Signature-Input", input);
			// Добавляем заголовок Signature
			result.emplace_back("Signature", signature);
		}
	}
}
/**
 * @brief Конструктор
 *
 * @param owner  сторона работы (клиент/сервер)
 * @param params общие параметры авторизации
 * @param crypto объект криптографии
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 *
 */
awh::http::Hmac::Hmac(const auth_t::owner_t owner, auth_t::params_t & params, const crypto_t * crypto, const fmk_t * fmk, const log_t * log) noexcept :
 auth_t::scheme_t(owner, params, crypto, fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::http::Hmac::~Hmac() noexcept {}
