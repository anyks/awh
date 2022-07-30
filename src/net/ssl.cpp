/**
 * @file: ssl.cpp
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
#include <net/ssl.hpp>

/**
 * rawEqual Метод проверки на эквивалентность доменных имен
 * @param first  первое доменное имя
 * @param second второе доменное имя
 * @return       результат проверки
 */
const bool awh::ASSL::rawEqual(const string & first, const string & second) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные переданы
	if(!first.empty() && !second.empty())
		// Проверяем совпадение строки
		result = (first.compare(second) == 0);
	// Выводим результат
	return result;
}
/**
 * rawNequal Метод проверки на не эквивалентность доменных имен
 * @param first  первое доменное имя
 * @param second второе доменное имя
 * @param max    количество начальных символов для проверки
 * @return       результат проверки
 */
const bool awh::ASSL::rawNequal(const string & first, const string & second, const size_t max) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные переданы
	if(!first.empty() && !second.empty()){
		// Получаем новые значения
		string first1 = first;
		string second1 = second;
		// Получаем новые значения переменных
		const string & first2 = first1.replace(max, first1.length() - max, "");
		const string & second2 = second1.replace(max, second1.length() - max, "");
		// Проверяем совпадение строки
		result = (first2.compare(second2) == 0);
	}
	// Выводим результат
	return result;
}
/**
 * hostmatch Метод проверки эквивалентности доменного имени с учетом шаблона
 * @param host доменное имя
 * @param patt шаблон домена
 * @return     результат проверки
 */
const bool awh::ASSL::hostmatch(const string & host, const string & patt) const noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные переданы
	if(!host.empty() && !patt.empty()){
		// Запоминаем шаблон и хоста
		string hostLabel = host;
		string pattLabel = patt;
		// Позиция звездочки в шаблоне
		const size_t pattWildcard = patt.find('*');
		// Ищем звездочку в шаблоне не найдена
		if(pattWildcard == string::npos)
			// Выполняем проверку эквивалентности доменных имён
			return this->rawEqual(patt, host);
		// Определяем конец шаблона
		const size_t pattLabelEnd = patt.find('.');
		// Если это конец тогда запрещаем активацию шаблона
		if((pattLabelEnd == string::npos) || (pattWildcard > pattLabelEnd) || this->rawNequal(patt, "xn--", 4))
			// Выполняем проверку эквивалентности доменных имён
			return this->rawEqual(patt, host);
		// Выполняем поиск точки в название хоста
		const size_t hostLabelEnd = host.find('.');
		// Если хост не найден
		if((pattLabelEnd != string::npos) && (hostLabelEnd != string::npos)){
			// Обрезаем строку шаблона
			const string & p = pattLabel.replace(0, pattLabelEnd, "");
			// Обрезаем строку хоста
			const string & h = hostLabel.replace(0, hostLabelEnd, "");
			// Выполняем сравнение
			if(!this->rawEqual(p, h)) return false;
		// Выходим
		} else return false;
		// Если диапазоны точки в шаблоне и хосте отличаются тогда выходим
		if(hostLabelEnd < pattLabelEnd) return false;
		// Получаем размер префикса и суфикса
		const size_t prefixlen = pattWildcard;
		const size_t suffixlen = (pattLabelEnd - (pattWildcard + 1));
		// Обрезаем строку шаблона
		const string & p = pattLabel.replace(0, pattWildcard + 1, "");
		// Обрезаем строку хоста
		const string & h = hostLabel.replace(0, hostLabelEnd - suffixlen, "");
		// Проверяем эквивалент результата
		return (this->rawNequal(patt, host, prefixlen) && this->rawNequal(p, h, suffixlen));
	}
	// Выводим результат
	return result;
}
/**
 * certHostcheck Метод проверки доменного имени по шаблону
 * @param host доменное имя
 * @param patt шаблон домена
 * @return     результат проверки
 */
const bool awh::ASSL::certHostcheck(const string & host, const string & patt) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные переданы
	if(!host.empty() && !patt.empty())
		// Проверяем эквивалентны ли домен и шаблон
		result = (this->rawEqual(host, patt) || this->hostmatch(host, patt));
	// Выводим результат
	return result;
}
/**
 * matchesCommonName Метод проверки доменного имени по данным из сертификата
 * @param host доменное имя
 * @param cert сертификат
 * @return     результат проверки
 */
const awh::ASSL::validate_t awh::ASSL::matchesCommonName(const string & host, const X509 * cert) const noexcept {
	// Результат работы функции
	validate_t result = validate_t::MatchNotFound;
	// Если данные переданы
	if(!host.empty() && (cert != nullptr)){
		// Получаем индекс имени по NID
		const int cnl = X509_NAME_get_index_by_NID(X509_get_subject_name((X509 *) cert), NID_commonName, -1);
		// Если индекс не получен тогда выходим
		if(cnl < 0) return validate_t::Error;
		// Извлекаем поле CN
		X509_NAME_ENTRY * cne = X509_NAME_get_entry(X509_get_subject_name((X509 *) cert), cnl);
		// Если поле не получено тогда выходим
		if(cne == nullptr) return validate_t::Error;
		// Конвертируем CN поле в C строку
		ASN1_STRING * cna = X509_NAME_ENTRY_get_data(cne);
		// Если строка не сконвертирована тогда выходим
		if(cna == nullptr) return validate_t::Error;
		// Извлекаем название в виде строки
		const string cn((char *) ASN1_STRING_get0_data(cna), ASN1_STRING_length(cna));
		// Сравниваем размеры полученных строк
		if(size_t(ASN1_STRING_length(cna)) != cn.length()) return validate_t::MalformedCertificate;
		// Выполняем рукопожатие
		if(this->certHostcheck(host, cn)) return validate_t::MatchFound;
	}
	// Выводим результат
	return result;
}
/**
 * matchSubjectName Метод проверки доменного имени по списку доменных имен из сертификата
 * @param host доменное имя
 * @param cert сертификат
 * @return     результат проверки
 */
const awh::ASSL::validate_t awh::ASSL::matchSubjectName(const string & host, const X509 * cert) const noexcept {
	// Результат работы функции
	validate_t result = validate_t::MatchNotFound;
	// Если данные переданы
	if(!host.empty() && (cert != nullptr)){
		// Получаем имена
		STACK_OF(GENERAL_NAME) * sn = reinterpret_cast <STACK_OF(GENERAL_NAME) *> (X509_get_ext_d2i((X509 *) cert, NID_subject_alt_name, nullptr, nullptr));
		// Если имена не получены тогда выходим
		if(sn == nullptr) return validate_t::NoSANPresent;
		// Получаем количество имен
		const int sanNamesNb = sk_GENERAL_NAME_num(sn);
		// Переходим по всему списку
		for(int i = 0; i < sanNamesNb; i++){
			// Получаем имя из списка
			const GENERAL_NAME * cn = sk_GENERAL_NAME_value(sn, i);
			// Проверяем тип имени
			if(cn->type == GEN_DNS){
				// Получаем dns имя
				const string dns((char *) ASN1_STRING_get0_data(cn->d.dNSName), ASN1_STRING_length(cn->d.dNSName));
				// Если размер имени не совпадает
				if(size_t(ASN1_STRING_length(cn->d.dNSName)) != dns.length()){
					// Запоминаем результат
					result = validate_t::MalformedCertificate;
					// Выходим из цикла
					break;
				// Если размер имени и dns имя совпадает
				} else if(this->certHostcheck(host, dns)){
					// Запоминаем результат что домен найден
					result = validate_t::MatchFound;
					// Выходим из цикла
					break;
				}
			}
		}
		// Очищаем список имен
		sk_GENERAL_NAME_pop_free(sn, GENERAL_NAME_free);
	}
	// Выводим результат
	return result;
}
/**
 * validateHostname Метод проверки доменного имени
 * @param host доменное имя
 * @param cert сертификат
 * @return     результат проверки
 */
const awh::ASSL::validate_t awh::ASSL::validateHostname(const string & host, const X509 * cert) const noexcept {
	// Результат работы функции
	validate_t result = validate_t::Error;
	// Если данные переданы
	if(!host.empty() && (cert != nullptr)){
		// Выполняем проверку имени хоста по списку доменов у сертификата
		result = this->matchSubjectName(host, cert);
		// Если у сертификата только один домен
		if(result == validate_t::NoSANPresent) result = this->matchesCommonName(host, cert);
	}
	// Выводим результат
	return result;
}
/**
 * clear Метод очистки контекста
 * @param ctx контекст для очистки
 */
void awh::ASSL::clear(ctx_t & ctx) const noexcept {
	// Если объект верификации домена создан
	if(ctx.verify != nullptr){
		// Удаляем объект верификации
		delete ctx.verify;
		// Зануляем объект верификации
		ctx.verify = nullptr;
	}
	// Если контекст SSL создан
	if(ctx.ssl != nullptr){
		// Выключаем получение данных SSL
		SSL_shutdown(ctx.ssl);
		// Очищаем объект SSL
		// SSL_clear(ctx.ssl);
		// Освобождаем выделенную память
		SSL_free(ctx.ssl);
		// Зануляем контекст сервера
		ctx.ssl = nullptr;
	}
	// Если контекст SSL сервер был поднят
	if(ctx.ctx != nullptr){
		// Очищаем контекст сервера
		SSL_CTX_free(ctx.ctx);
		// Зануляем контекст сервера
		ctx.ctx = nullptr;
	}
	// Сбрасываем флаг инициализации
	ctx.mode = false;
}
/**
 * init Метод инициализации контекста для сервера
 * @return объект SSL контекста
 */
awh::ASSL::ctx_t awh::ASSL::init() noexcept {
	// Результат работы функции
	ctx_t result;
	// Если объект фреймворка существует
	if((this->fmk != nullptr) && !this->key.empty() && !this->cert.empty()){
		
		/*
		// Активируем рандомный генератор
		if(RAND_poll() == 0){
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "rand poll is not allow");
			// Выходим
			return result;
		}
		*/



		// Получаем контекст OpenSSL
		// result.ctx = SSL_CTX_new(SSLv23_server_method()); // SSLv3_method()

		result.ctx = SSL_CTX_new(TLS_server_method());

		// Если контекст не создан
		if(result.ctx == nullptr){
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "context ssl is not initialization");
			// Выходим
			return result;
		}
		// Устанавливаем опции запроса
		SSL_CTX_set_options(result.ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3); // TLSv1	TLSv1.1	TLSv1.2

		SSL_CTX_set_min_proto_version(result.ctx, 0);
   		SSL_CTX_set_max_proto_version(result.ctx, TLS1_3_VERSION);

		// cout << " ^^^^^^^^^^^^^^^^^^^^^^ CIPHERS1 " << OSSL_default_ciphersuites() << endl;
		// cout << " ^^^^^^^^^^^^^^^^^^^^^^ CIPHERS2 " << OSSL_default_cipher_list() << endl;
		
		
		// Устанавливаем типы шифрования
		// if(!SSL_CTX_set_cipher_list(result.ctx, "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-RSA-AES256-SHA384")){		
		if(!SSL_CTX_set_cipher_list(result.ctx, "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA")){
		// SSL_CTX_set_ciphersuites(result.ctx, "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256");
		// if(!SSL_CTX_set_cipher_list(result.ctx, "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-SHA256:AES128-SHA:AES256-SHA:DES-CBC3-SHA:!DSS")){
		
		// if(!SSL_CTX_set_cipher_list(result.ctx, "ALL:!ADH:!RC4:+HIGH:+MEDIUM:-LOW:-SSLv2:-SSLv3:-EXP")){
			// Очищаем созданный контекст
			this->clear(result);
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "set ssl ciphers");
			// Выходим
			return result;
		}
		/*
		// Устанавливаем поддерживаемые кривые
		if(!SSL_CTX_set_ecdh_auto(result.ctx, 1)){
			// Очищаем созданный контекст
			this->clear(result);
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "set ssl ecdh");
			// Выходим
			return result;
		}
		*/

		SSL_CTX_set_options(result.ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
		
		
		/*
		// Если CA-файл не найден или адрес файла не указан
		if(this->cafile.empty()){
			// Получаем данные стора
			X509_STORE * store = SSL_CTX_get_cert_store(result.ctx);
			// Если - это Windows
			#if defined(_WIN32) || defined(_WIN64)
		*/
				/**
				 * addCertToStoreFn Функция проверки параметров сертификата
				 * @param store стор с сертификатами для работы
				 * @param name  название параметра сертификата
				 * @return      результат проверки
				 */
		/*
				auto addCertToStoreFn = [this](X509_STORE * store = nullptr, const char * name = nullptr) -> int {
					// Результат работы функции
					int result = 0;
					// Если объекты переданы верно
					if((store != nullptr) && (name != nullptr)){
						// Контекст сертификата
						PCCERT_CONTEXT ctx = nullptr;
						// Получаем данные системного стора
						HCERTSTORE sys = CertOpenSystemStore(0, name);
						// Если системный стор не получен
						if(!sys){
							// Выводим в лог сообщение
							this->log->print("%s", log_t::flag_t::CRITICAL, "failed to open system certificate store");
							// Выходим
							return -1;
						}
						// Перебираем все сертификаты в системном сторе
						while((ctx = CertEnumCertificatesInStore(sys, ctx))){
							// Получаем сертификат
							X509 * cert = d2i_X509(nullptr, (const u_char **) &ctx->pbCertEncoded, ctx->cbCertEncoded);
							// Если сертификат получен
							if(cert != nullptr){
								// Добавляем сертификат в стор
								X509_STORE_add_cert(store, cert);
								// Очищаем выделенную память
								X509_free(cert);
							// Если сертификат не получен
							} else {
								// Формируем результат ответа
								result = -1;
								// Выводим в лог сообщение
								this->log->print("%s failed", log_t::flag_t::CRITICAL, "d2i_X509");
								// Выходим из цикла
								break;
							}
						}
						// Закрываем системный стор
						CertCloseStore(sys, 0);
					}
					// Выходим
					return result;
				};
				// Проверяем существует ли путь
				if((addCertToStoreFn(store, "CA") < 0) || (addCertToStoreFn(store, "AuthRoot") < 0) || (addCertToStoreFn(store, "ROOT") < 0)) return result;
			#endif
			// Если стор не устанавливается, тогда выводим ошибку
			if(X509_STORE_set_default_paths(store) != 1){
				// Очищаем созданный контекст
				this->clear(result);
				// Выводим в лог сообщение
				this->log->print("%s", log_t::flag_t::CRITICAL, "set default paths for x509 store is not allow");
				// Выходим
				return result;
			}
		// Если CA файл найден
		} else {
			// Определяем путь где хранятся сертификаты
			const char * capath = (!this->capath.empty() ? this->capath.c_str() : nullptr);
			// Выполняем проверку
			if(SSL_CTX_load_verify_locations(result.ctx, this->cafile.c_str(), capath) != 1){
				// Очищаем созданный контекст
				this->clear(result);
				// Выводим в лог сообщение
				this->log->print("%s", log_t::flag_t::CRITICAL, "ssl verify locations is not allow");
				// Выходим
				return result;
			}
			// Если каталог получен
			if(capath != nullptr){
				// Получаем полный адрес
				const string & fullPath = fs_t::realPath(this->capath);
				// Если адрес существует
				if(fs_t::isdir(fullPath) && !fs_t::isfile(this->cafile)){
					// Если - это Windows
					#if defined(_WIN32) || defined(_WIN64)
						// Выполняем сплит адреса
						const auto & params = this->uri->split(fullPath);
						// Если диск получен
						if(!params.front().empty()){
							// Выполняем сплит адреса
							auto path = this->uri->splitPath(params.back(), FS_SEPARATOR);
							// Добавляем адрес файла в список
							path.push_back(this->cafile);
							// Формируем полный адарес файла
							string filename = this->fmk->format("%s:%s", params.front().c_str(), this->uri->joinPath(path, FS_SEPARATOR).c_str());
							// Выполняем проверку CA-файла
							if(!filename.empty()){
								// Выполняем декодирование адреса файла
								filename = this->uri->urlDecode(filename);
								// Если адрес файла существует
								if(fs_t::isfile(filename)){
									// Выполняем проверку CA-файла
									SSL_CTX_set_client_CA_list(result.ctx, SSL_load_client_CA_file(filename.c_str()));
									// Переходим к следующей итерации
									goto Next;
								}
							}
						}
						// Выполняем очистку CA-файла
						this->cafile.clear();
					// Если - это Unix
					#else
						// Выполняем сплит адреса
						auto path = this->uri->splitPath(fullPath, FS_SEPARATOR);
						// Добавляем адрес файла в список
						path.push_back(this->cafile);
						// Формируем полный адарес файла
						string filename = this->uri->joinPath(path, FS_SEPARATOR);
						// Выполняем проверку CA-файла
						if(!filename.empty()){
							// Выполняем декодирование адреса файла
							filename = this->uri->urlDecode(filename);
							// Если адрес файла существует
							if(fs_t::isfile(filename)){
								// Выполняем проверку CA файла
								SSL_CTX_set_client_CA_list(result.ctx, SSL_load_client_CA_file(filename.c_str()));
								// Переходим к следующей итерации
								goto Next;
							}
						}
						// Выполняем очистку CA-файла
						this->cafile.clear();
					#endif
				// Если адрес файла существует
				} else if(fs_t::isfile(this->cafile))
					// Выполняем проверку CA-файла
					SSL_CTX_set_client_CA_list(result.ctx, SSL_load_client_CA_file(this->cafile.c_str()));
				// Выполняем очистку CA-файла
				else this->cafile.clear();
			// Если адрес файла существует
			} else if(fs_t::isfile(this->cafile))
				// Выполняем проверку CA-файла
				SSL_CTX_set_client_CA_list(result.ctx, SSL_load_client_CA_file(this->cafile.c_str()));
		}
		*/
		// Метка следующей итерации
		Next:

		/*
		// Устанавливаем флаг quiet shutdown
		SSL_CTX_set_quiet_shutdown(result.ctx, 1);
		// Запускаем кэширование
		SSL_CTX_set_session_cache_mode(result.ctx, SSL_SESS_CACHE_SERVER | SSL_SESS_CACHE_NO_INTERNAL);
		*/

		/*
		// Запрашиваем данные цепочки доверия
		const int chain = (!this->chain.empty() ? SSL_CTX_use_certificate_chain_file(result.ctx, this->chain.c_str()) : 1);
		// Запрашиваем данные приватного ключа сертификата
		const int prv = (!this->key.empty() ? SSL_CTX_use_PrivateKey_file(result.ctx, this->key.c_str(), SSL_FILETYPE_PEM) : 0);
		// Запрашиваем данные сертификата
		const int cert = (!this->cert.empty() ? SSL_CTX_use_certificate_file(result.ctx, this->cert.c_str(), SSL_FILETYPE_PEM) : 0);
		*/

		
		cout << " -----------------------1 " << result.ctx << " === " << this->cert << " === " << SSL_CTX_use_certificate_file(result.ctx, this->chain.c_str(), SSL_FILETYPE_PEM) << endl;
		cout << " -----------------------3 " << result.ctx << " === " << this->chain << " === " << SSL_CTX_use_certificate_chain_file(result.ctx, this->cert.c_str()) << endl;
		cout << " -----------------------2 " << result.ctx << " === " << this->key << " === " << SSL_CTX_use_PrivateKey_file(result.ctx, this->key.c_str(), SSL_FILETYPE_PEM) << endl;
		cout << " -----------------------4 " << result.ctx << " === " << SSL_CTX_check_private_key(result.ctx) << endl;
		
		SSL_CTX_set_verify(result.ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    	SSL_CTX_set_verify_depth(result.ctx, 4);


		/*
		#ifdef SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER
			SSL_CTX_set_mode(result.ctx, SSL_CTX_get_mode(result.ctx) | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
		#endif
			SSL_CTX_set_verify(result.ctx, SSL_VERIFY_NONE, nullptr);
		*/
		
		/*
		int SSL_use_certificate_file(SSL *ssl, const char *file, int type);
		int SSL_use_certificate_chain_file(SSL *ssl, const char *file);

		int SSL_CTX_use_RSAPrivateKey_file(SSL_CTX *ctx, const char *file, int type);
		int SSL_use_PrivateKey_file(SSL *ssl, const char *file, int type);
		int SSL_use_RSAPrivateKey_file(SSL *ssl, const char *file, int type);
		*/

		/*
		// Если какой-то из файлов не получен то выходим
		if(!(result.mode = ((chain > 0) && (cert > 0) && (prv > 0)))){
			// Очищаем созданный контекст
			this->clear(result);
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "ssl certificates is not load");
		}
		*/

		// Создаем ssl объект
		result.ssl = SSL_new(result.ctx);

		/*
		cout << " =======================1 " << result.ssl << " === " << this->cert << " === " << SSL_use_certificate_file(result.ssl, this->cert.c_str(), SSL_FILETYPE_PEM) << endl;
		cout << " =======================2 " << result.ssl << " === " << this->key << " === " << SSL_use_PrivateKey_file(result.ssl, this->key.c_str(), SSL_FILETYPE_PEM) << endl;
		cout << " =======================3 " << result.ssl << " === " << this->chain << " === " << SSL_use_certificate_chain_file(result.ssl, this->chain.c_str()) << endl;
		
		cout << " =======================4 " << result.ssl << " === " << SSL_check_private_key(result.ssl) << endl;
		*/
		
		

		// int accept_result = SSL_accept(result.ssl);
		
		// cout << " =======================5 " << accept_result << endl;

		/*
		// Проверяем рукопожатие
		if(SSL_do_handshake(result.ssl) <= 0){
			// Выполняем проверку рукопожатия
			const long verify = SSL_get_verify_result(result.ssl);
			// Если рукопожатие не выполнено
			if(verify != X509_V_OK){
				// Очищаем созданный контекст
				this->clear(result);
				// Выводим в лог сообщение
				this->log->print("certificate chain validation failed: %s", log_t::flag_t::CRITICAL, X509_verify_cert_error_string(verify));
			}
		}
		*/
		
		// Если объект не создан
		if(!(result.mode = (result.ssl != nullptr))){
			// Очищаем созданный контекст
			this->clear(result);
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "ssl initialization is not allow");
			// Выходим
			return result;
		}

	}
	// Выводим результат
	return result;
}
/**
 * init Метод инициализации контекста для сервера
 * @param url Параметры URL адреса для инициализации
 * @return    объект SSL контекста
 */
awh::ASSL::ctx_t awh::ASSL::init(const uri_t::url_t & url) noexcept {
	// Результат работы функции
	ctx_t result;
	// Если объект фреймворка существует
	if((this->fmk != nullptr) && (!url.domain.empty() || !url.ip.empty()) && ((url.schema.compare("https") == 0) || (url.schema.compare("wss") == 0))){
		// Активируем рандомный генератор
		if(RAND_poll() == 0){
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "rand poll is not allow");
			// Выходим
			return result;
		}
		// Получаем контекст OpenSSL
		result.ctx = SSL_CTX_new(SSLv23_client_method());
		// Если контекст не создан
		if(result.ctx == nullptr){
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "context ssl is not initialization");
			// Выходим
			return result;
		}
		// Устанавливаем опции запроса
		SSL_CTX_set_options(result.ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
		// Если CA-файл найден и адрес файла указан
		if(!this->cafile.empty()){
			// Определяем путь где хранятся сертификаты
			const char * capath = (!this->capath.empty() ? this->capath.c_str() : nullptr);
			// Выполняем проверку
			if(SSL_CTX_load_verify_locations(result.ctx, this->cafile.c_str(), capath) != 1){
				// Очищаем созданный контекст
				this->clear(result);
				// Выводим в лог сообщение
				this->log->print("%s", log_t::flag_t::CRITICAL, "ssl verify locations is not allow");
				// Выходим
				return result;
			}
			// Если каталог получен
			if(capath != nullptr){
				// Получаем полный адрес
				const string & fullPath = fs_t::realPath(this->capath);
				// Если адрес существует
				if(fs_t::isdir(fullPath) && !fs_t::isfile(this->cafile)){
					// Если - это Windows
					#if defined(_WIN32) || defined(_WIN64)
						// Выполняем сплит адреса
						const auto & params = this->uri->split(fullPath);
						// Если диск получен
						if(!params.front().empty()){
							// Выполняем сплит адреса
							auto path = this->uri->splitPath(params.back(), FS_SEPARATOR);
							// Добавляем адрес файла в список
							path.push_back(this->cafile);
							// Формируем полный адарес файла
							string filename = this->fmk->format("%s:%s", params.front().c_str(), this->uri->joinPath(path, FS_SEPARATOR).c_str());
							// Выполняем проверку CA-файла
							if(!filename.empty()){
								// Выполняем декодирование адреса файла
								filename = this->uri->urlDecode(filename);
								// Если адрес файла существует
								if(fs_t::isfile(filename)){
									// Выполняем проверку CA-файла
									SSL_CTX_set_client_CA_list(result.ctx, SSL_load_client_CA_file(filename.c_str()));
									// Переходим к следующей итерации
									goto Next;
								}
							}
						}
						// Выполняем очистку CA-файла
						this->cafile.clear();
					// Если - это Unix
					#else
						// Выполняем сплит адреса
						auto path = this->uri->splitPath(fullPath, FS_SEPARATOR);
						// Добавляем адрес файла в список
						path.push_back(this->cafile);
						// Формируем полный адарес файла
						string filename = this->uri->joinPath(path, FS_SEPARATOR);
						// Выполняем проверку CA-файла
						if(!filename.empty()){
							// Выполняем декодирование адреса файла
							filename = this->uri->urlDecode(filename);
							// Если адрес файла существует
							if(fs_t::isfile(filename)){
								// Выполняем проверку CA файла
								SSL_CTX_set_client_CA_list(result.ctx, SSL_load_client_CA_file(filename.c_str()));
								// Переходим к следующей итерации
								goto Next;
							}
						}
						// Выполняем очистку CA-файла
						this->cafile.clear();
					#endif
				// Если адрес файла существует
				} else if(fs_t::isfile(this->cafile))
					// Выполняем проверку CA-файла
					SSL_CTX_set_client_CA_list(result.ctx, SSL_load_client_CA_file(this->cafile.c_str()));
				// Выполняем очистку CA-файла
				else this->cafile.clear();
			// Если адрес файла существует
			} else if(fs_t::isfile(this->cafile))
				// Выполняем проверку CA-файла
				SSL_CTX_set_client_CA_list(result.ctx, SSL_load_client_CA_file(this->cafile.c_str()));
		}
		// Метка следующей итерации
		Next:
		// Если CA-файл не указан
		if(this->cafile.empty()){
			// Получаем данные стора
			X509_STORE * store = SSL_CTX_get_cert_store(result.ctx);
			// Если - это Windows
			#if defined(_WIN32) || defined(_WIN64)
				/**
				 * addCertToStoreFn Функция проверки параметров сертификата
				 * @param store стор с сертификатами для работы
				 * @param name  название параметра сертификата
				 * @return      результат проверки
				 */
				auto addCertToStoreFn = [this](X509_STORE * store = nullptr, const char * name = nullptr) -> int {
					// Результат работы функции
					int result = 0;
					// Если объекты переданы верно
					if((store != nullptr) && (name != nullptr)){
						// Контекст сертификата
						PCCERT_CONTEXT ctx = nullptr;
						// Получаем данные системного стора
						HCERTSTORE sys = CertOpenSystemStore(0, name);
						// Если системный стор не получен
						if(!sys){
							// Выводим в лог сообщение
							this->log->print("%s", log_t::flag_t::CRITICAL, "failed to open system certificate store");
							// Выходим
							return -1;
						}
						// Перебираем все сертификаты в системном сторе
						while((ctx = CertEnumCertificatesInStore(sys, ctx))){
							// Получаем сертификат
							X509 * cert = d2i_X509(nullptr, (const u_char **) &ctx->pbCertEncoded, ctx->cbCertEncoded);
							// Если сертификат получен
							if(cert != nullptr){
								// Добавляем сертификат в стор
								X509_STORE_add_cert(store, cert);
								// Очищаем выделенную память
								X509_free(cert);
							// Если сертификат не получен
							} else {
								// Формируем результат ответа
								result = -1;
								// Выводим в лог сообщение
								this->log->print("%s failed", log_t::flag_t::CRITICAL, "d2i_X509");
								// Выходим из цикла
								break;
							}
						}
						// Закрываем системный стор
						CertCloseStore(sys, 0);
					}
					// Выходим
					return result;
				};
				// Проверяем существует ли путь
				if((addCertToStoreFn(store, "CA") < 0) || (addCertToStoreFn(store, "AuthRoot") < 0) || (addCertToStoreFn(store, "ROOT") < 0)) return result;
			#endif
			// Если стор не устанавливается, тогда выводим ошибку
			if(X509_STORE_set_default_paths(store) != 1){
				// Очищаем созданный контекст
				this->clear(result);
				// Выводим в лог сообщение
				this->log->print("%s", log_t::flag_t::CRITICAL, "set default paths for x509 store is not allow");
				// Выходим
				return result;
			}
		}
		// Если нужно произвести проверку
		if(this->verify && !url.domain.empty()){
			/**
			 * verifyFn Функция обратного вызова для проверки валидности сертификата
			 * @param x509 данные сертификата
			 * @param ctx  передаваемый контекст
			 * @return     результат проверки
			 */
			auto verifyFn = [](X509_STORE_CTX * x509 = nullptr, void * ctx = nullptr) -> int {
				// Если объекты переданы верно
				if((x509 != nullptr) && (ctx != nullptr)){
					// Буфер данных сертификатов из хранилища
					char buffer[256];
					// Заполняем структуру нулями
					memset(buffer, 0, sizeof(buffer));
					// Ошибка проверки сертификата
					string status = "X509VerifyCertFailed";
					// Результат проверки домена
					ssl_t::validate_t validate = ssl_t::validate_t::Error;
					// Получаем объект подключения
					const verify_t * obj = reinterpret_cast <const verify_t *> (ctx);
					// Выполняем проверку сертификата
					const int ok = X509_verify_cert(x509);
					// Запрашиваем данные сертификата
					X509 * cert = X509_STORE_CTX_get_current_cert(x509);
					// Если проверка сертификата прошла удачно
					if(ok){
						// Выполняем проверку на соответствие хоста с данными хостов у сертификата
						validate = obj->ssl->validateHostname(obj->host.c_str(), cert);
						// Определяем полученную ошибку
						switch((uint8_t) validate){
							case (uint8_t) ssl_t::validate_t::MatchFound:           status = "MatchFound";           break;
							case (uint8_t) ssl_t::validate_t::MatchNotFound:        status = "MatchNotFound";        break;
							case (uint8_t) ssl_t::validate_t::NoSANPresent:         status = "NoSANPresent";         break;
							case (uint8_t) ssl_t::validate_t::MalformedCertificate: status = "MalformedCertificate"; break;
							case (uint8_t) ssl_t::validate_t::Error:                status = "Error";                break;
							default:                                                status = "WTF!";
						}
					}
					// Запрашиваем имя домена
					X509_NAME_oneline(X509_get_subject_name(cert), buffer, sizeof(buffer));
					// Очищаем выделенную память
					X509_free(cert);
					// Если домен найден в записях сертификата (т.е. сертификат соответствует данному домену)
					if(validate == ssl_t::validate_t::MatchFound){
						// Если включён режим отладки
						#if defined(DEBUG_MODE)
							// Выводим в лог сообщение
							obj->ssl->log->print("https server [%s] has this certificate, which looks good to me: %s", log_t::flag_t::INFO, obj->host.c_str(), buffer);
						#endif
						// Выводим сообщение, что проверка пройдена
						return 1;
					// Если ресурс не найден тогда выводим сообщение об ошибке
					} else obj->ssl->log->print("%s for hostname '%s' [%s]", log_t::flag_t::CRITICAL, status.c_str(), obj->host.c_str(), buffer);
				}
				// Выводим сообщение, что проверка не пройдена
				return 0;
			};
			// Создаём объект проверки домена
			result.verify = new verify_t(url.domain, this);
			// Выполняем проверку сертификата
			SSL_CTX_set_verify(result.ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, nullptr);
			// Выполняем проверку всех дочерних сертификатов
			SSL_CTX_set_cert_verify_callback(result.ctx, verifyFn, result.verify);
		}
		// Создаем ssl объект
		result.ssl = SSL_new(result.ctx);
		// Если объект не создан
		if(!(result.mode = (result.ssl != nullptr))){
			// Очищаем созданный контекст
			this->clear(result);
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "ssl initialization is not allow");
			// Выходим
			return result;
		}
		// Если нужно установить TLS расширение
		#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
			// Устанавливаем имя хоста для SNI расширения
			SSL_set_tlsext_host_name(result.ssl, (!url.domain.empty() ? url.domain : url.ip).c_str());
		#endif
		// Активируем верификацию доменного имени
		if(!X509_VERIFY_PARAM_set1_host(SSL_get0_param(result.ssl), (!url.domain.empty() ? url.domain : url.ip).c_str(), 0)){
			// Очищаем созданный контекст
			this->clear(result);
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "domain ssl verification failed");
			// Выходим
			return result;
		}
		// Проверяем рукопожатие
		if(SSL_do_handshake(result.ssl) <= 0){
			// Выполняем проверку рукопожатия
			const long verify = SSL_get_verify_result(result.ssl);
			// Если рукопожатие не выполнено
			if(verify != X509_V_OK){
				// Очищаем созданный контекст
				this->clear(result);
				// Выводим в лог сообщение
				this->log->print("certificate chain validation failed: %s", log_t::flag_t::CRITICAL, X509_verify_cert_error_string(verify));
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * setVerify Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::ASSL::setVerify(const bool mode) noexcept {
	// Устанавливаем флаг проверки
	this->verify = mode;
}
/**
 * setCA Метод установки CA-файла корневого SSL сертификата
 * @param cafile адрес CA-файла
 * @param capath адрес каталога где находится CA-файл
 */
void awh::ASSL::setCA(const string & cafile, const string & capath) noexcept {
	// Если адрес CA-файла передан
	if(!cafile.empty()){
		// Устанавливаем адрес CA-файла
		this->cafile = fs_t::realPath(cafile);
		// Если адрес каталога с CA-файлом передан, устанавливаем и его
		if(!capath.empty() && fs_t::isdir(capath))
			// Устанавливаем адрес каталога с CA-файлами
			this->capath = fs_t::realPath(capath);
	}
}
/**
 * setCert Метод установки файлов сертификата
 * @param cert  корневой сертификат
 * @param key   приватный ключ сертификата
 * @param chain файл цепочки сертификатов
 */
void awh::ASSL::setCert(const string & cert, const string & key, const string & chain) noexcept {
	// Устанавливаем приватный ключ сертификата
	this->key = key;
	// Устанавливаем файл сертификата
	this->cert = cert;
	// Устанавливаем файл цепочки сертификатов
	this->chain = chain;
}
/**
 * ASSL Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 */
awh::ASSL::ASSL(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : fmk(fmk), uri(uri), log(log) {
	// Выполняем модификацию CA-файла
	this->cafile = fs_t::realPath(this->cafile);
	/**
	 * Если версия OPENSSL старая
	 */
	#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000L)
		// Выполняем инициализацию OpenSSL
		SSL_library_init();
		ERR_load_crypto_strings();
		SSL_load_error_strings();
		OpenSSL_add_all_algorithms();
	/**
	 * Для более свежей версии
	 */
	#else
		// Выполняем инициализацию OpenSSL
        OPENSSL_init_ssl(OPENSSL_INIT_SSL_DEFAULT, nullptr);
	#endif
}
/**
 * ~ASSL Деструктор
 */
awh::ASSL::~ASSL() noexcept {
	// Если версия OPENSSL старая
	#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000L)
		// Выполняем освобождение памяти
		EVP_cleanup();
		ERR_free_strings();
		// Освобождаем объект состояния
		#if OPENSSL_VERSION_NUMBER < 0x10000000L
			ERR_remove_state(0);
		#else
			ERR_remove_thread_state(nullptr);
		#endif
		// Освобождаем оставшиеся данные
		CRYPTO_cleanup_all_ex_data();
		sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
	#endif
}
