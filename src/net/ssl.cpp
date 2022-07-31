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
 * socketIsBlocking Функция проверки сокета блокирующий режим
 * @param fd  файловый дескриптор (сокет)
 * @param log объект для работы с логами
 * @return    результат работы функции
 */
static int socketIsBlocking(const int fd, const awh::log_t * log = nullptr) noexcept {
	// Если файловый дескриптор передан
	if(fd > -1){
		/**
		 * Методы только для OS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			{
				// Все удачно
				return -1;
			}
		/**
		 * Для всех остальных операционных систем
		 */
		#else
			{
				// Флаги файлового дескриптора
				int flags = 0;
				// Получаем флаги файлового дескриптора
				if((flags = fcntl(fd, F_GETFL, nullptr)) < 0){
					// Выводим в лог информацию
					if(log != nullptr) log->print("cannot set BLOCK option on socket %d", awh::log_t::flag_t::CRITICAL, fd);
					// Выходим
					return -1;
				}
				// Если флаг неблокирующего режима работы установлен
				if(flags & O_NONBLOCK)
					// Сообщаем, что сокет находится в неблокирующем режиме
					return 0;
				// Сообщаем, что сокет находится в блокирующем режиме
				else return 1;
			}
		#endif
	}
	// Все удачно
	return -1;
}
/**
 * error Метод вывода информации об ошибке
 * @param status статус ошибки
 */
void awh::ASSL::Context::error(const int status) const noexcept {
	// Получаем данные описание ошибки
	const int error = SSL_get_error(this->ssl, status);
	// Определяем тип ошибки
	switch(error){
		// Если был возвращён ноль
		case SSL_ERROR_ZERO_RETURN: {
			// Если удалённая сторона произвела закрытие подключения
			if(SSL_get_shutdown(this->ssl) & SSL_RECEIVED_SHUTDOWN)
				// Выводим в лог сообщение
				this->log->print("the remote side closed the connection", log_t::flag_t::INFO);
		} break;
		// Если произошла ошибка вызова
		case SSL_ERROR_SYSCALL: {
			// Получаем данные описание ошибки
			u_long error = ERR_get_error();
			// Если ошибка получена
			if(error != 0){
				// Выводим в лог сообщение
				this->log->print("%s", log_t::flag_t::CRITICAL, ERR_error_string(error, nullptr));
				/**
				 * Выполняем извлечение остальных ошибок
				 */
				do {
					// Выводим в лог сообщение
					this->log->print("%s", log_t::flag_t::CRITICAL, ERR_error_string(error, nullptr));
				// Если ещё есть ошибки
				} while((error = ERR_get_error()));
			// Если данные записаны неверно
			} else if(status == -1)
				// Выводим в лог сообщение
				this->log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
		} break;
		// Для всех остальных ошибок
		default: {
			// Получаем данные описание ошибки
			u_long error = 0;
			// Выполняем чтение ошибок OpenSSL
			while((error = ERR_get_error()))
				// Выводим в лог сообщение
				this->log->print("%s", log_t::flag_t::CRITICAL, ERR_error_string(error, nullptr));
		}
	};
}
/**
 * get Метод получения файлового дескриптора
 * @return файловый дескриптор
 */
int awh::ASSL::Context::get() const noexcept {
	// Выводим установленный файловый дескриптор
	return this->fd;
}
/**
 * wrapped Метод првоерки на активацию контекста
 * @return результат проверки
 */
bool awh::ASSL::Context::wrapped() const noexcept {
	// Выводим результат проверки
	return (this->fd > -1);
}
/**
 * read Метод чтения данных из сокета
 * @param buffer буфер данных для чтения
 * @param size   размер буфера данных
 * @return       количество считанных байт
 */
int64_t awh::ASSL::Context::read(char * buffer, const size_t size) const noexcept {
	// Результат работы функции
	int64_t result = 0;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0) && (this->type != type_t::NONE) && (this->fd > -1)){
		// Выполняем зануление буфера
		memset(buffer, 0, size);
		// Если защищённый режим работы разрешён
		if(this->mode){
			// Выполняем очистку ошибок OpenSSL
			ERR_clear_error();
			// Если подключение выполнено
			if((result = ((this->type == type_t::SERVER) ? SSL_accept(this->ssl) : SSL_connect(this->ssl))))
				// Выполняем чтение из защищённого сокета
				result = SSL_read(this->ssl, buffer, size);
		// Выполняем чтение из буфера данных стандартным образом
		} else result = recv(this->fd, buffer, size, 0);
		// Если данные прочитать не удалось
		if(result <= 0){
			// Получаем статус сокета
			const int status = socketIsBlocking(this->fd, this->log);
			// Если сокет находится в блокирующем режиме
			if((result < 0) && (status != 0))
				// Выполняем обработку ошибок
				this->error(result);
			// Если произошла ошибка
			else if((result < 0) && (status == 0)) {
				// Если произошёл системный сигнал попробовать ещё раз
				if(errno == EINTR) return -2;
				// Если защищённый режим работы разрешён
				if(this->mode){
					// Получаем данные описание ошибки
					if(SSL_get_error(this->ssl, result) == SSL_ERROR_WANT_READ)
						// Выполняем пропуск попытки
						return -1;
					// Иначе выводим сообщение об ошибке
					else this->error(result);
				// Если защищённый режим работы запрещён
				} else if(errno == EAGAIN) return -1;
				// Иначе просто закрываем подключение
				result = 0;
			}
			// Если подключение разорвано или сокет находится в блокирующем режиме
			if((result == 0) || (status != 0))
				// Выполняем отключение от сервера
				result = 0;
		}
	}
	// Выводим результат
	return result;
}
/**
 * write Метод записи данных в сокет
 * @param buffer буфер данных для записи
 * @param size   размер буфера данных
 * @return       количество записанных байт
 */
int64_t awh::ASSL::Context::write(const char * buffer, const size_t size) const noexcept {
	// Результат работы функции
	int64_t result = 0;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0) && (this->type != type_t::NONE) && (this->fd > -1)){
		// Если защищённый режим работы разрешён
		if(this->mode){
			// Выполняем очистку ошибок OpenSSL
			ERR_clear_error();
			// Если подключение выполнено
			if((result = ((this->type == type_t::SERVER) ? SSL_accept(this->ssl) : SSL_connect(this->ssl))))
				// Выполняем отправку сообщения через защищённый канал
				result = SSL_write(this->ssl, buffer, size);
		// Выполняем отправку сообщения в сокет
		} else result = send(this->fd, buffer, size, 0);
		// Если данные записать не удалось
		if(result <= 0){
			// Получаем статус сокета
			const int status = socketIsBlocking(this->fd, this->log);
			// Если сокет находится в блокирующем режиме
			if((result < 0) && (status != 0))
				// Выполняем обработку ошибок
				this->error(result);
			// Если произошла ошибка
			else if((result < 0) && (status == 0)) {
				// Если произошёл системный сигнал попробовать ещё раз
				if(errno == EINTR) return -2;
				// Если защищённый режим работы разрешён
				if(this->mode){
					// Получаем данные описание ошибки
					if(SSL_get_error(this->ssl, result) == SSL_ERROR_WANT_WRITE)
						// Выполняем пропуск попытки
						return -1;
					// Иначе выводим сообщение об ошибке
					else this->error(result);
				// Если защищённый режим работы запрещён
				} else if(errno == EAGAIN) return -1;
				// Иначе просто закрываем подключение
				result = 0;
			}
			// Если подключение разорвано или сокет находится в блокирующем режиме
			if((result == 0) || (status != 0))
				// Выполняем отключение от сервера
				result = 0;
		}
	}
	// Выводим результат
	return result;
}
/**
 * block Метод установки блокирующего сокета
 * @return результат работы функции
 */
int awh::ASSL::Context::block() noexcept {
	// Результат работы функции
	int result = 0;
	// Если защищённый режим работы разрешён
	if(this->mode && (this->fd > -1)){
		// Устанавливаем блокирующий режим ввода/вывода для сокета
		BIO_set_nbio(this->bio, 0);
		// Флаг необходимо установить только для неблокирующего сокета
		SSL_clear_mode(this->ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
	}
	// Выводим результат
	return result;
}
/**
 * noblock Метод установки неблокирующего сокета
 * @return результат работы функции
 */
int awh::ASSL::Context::noblock() noexcept {
	// Результат работы функции
	int result = 0;
	// Если файловый дескриптор активен
	if(this->mode && (this->fd > -1)){
		// Устанавливаем неблокирующий режим ввода/вывода для сокета
		BIO_set_nbio(this->bio, 1);
		// Флаг необходимо установить только для неблокирующего сокета
		SSL_set_mode(this->ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
	}
	// Выводим результат
	return result;
}
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
 * verifyHost Функция обратного вызова для проверки валидности сертификата
 * @param x509 данные сертификата
 * @param ctx  передаваемый контекст
 * @return     результат проверки
 */
int awh::ASSL::verifyHost(X509_STORE_CTX * x509, void * ctx) noexcept {
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
			/**
			 * Если включён режим отладки
			 */
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
 * initTrustedStore Метод инициализации магазина доверенных сертификатов
 * @param ctx объект контекста SSL
 * @return    результат инициализации
 */
bool awh::ASSL::initTrustedStore(SSL_CTX * ctx) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если контекст SSL передан
	if(ctx != nullptr){
		// Если доверенный сертификат (CA-файл) найден и адрес файла указан
		if(!this->trusted.empty()){
			// Определяем путь где хранятся сертификаты
			const char * path = (!this->path.empty() ? this->path.c_str() : nullptr);
			// Выполняем проверку
			if(SSL_CTX_load_verify_locations(ctx, this->trusted.c_str(), path) != 1){
				// Выводим в лог сообщение
				this->log->print("%s", log_t::flag_t::CRITICAL, "ssl verify locations is not allow");
				// Выходим
				return result;
			}
			// Если каталог получен
			if(path != nullptr){
				// Получаем полный адрес
				const string & trustdir = fs_t::realPath(this->path);
				// Если адрес существует
				if(fs_t::isdir(trustdir) && !fs_t::isfile(this->trusted)){
					/**
					 * Если операционной системой является MS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Выполняем сплит адреса
						const auto & params = this->uri->split(trustdir);
						// Если диск получен
						if(!params.front().empty()){
							// Выполняем сплит адреса
							auto path = this->uri->splitPath(params.back(), FS_SEPARATOR);
							// Добавляем адрес файла в список
							path.push_back(this->trusted);
							// Формируем полный адарес файла
							string filename = this->fmk->format("%s:%s", params.front().c_str(), this->uri->joinPath(path, FS_SEPARATOR).c_str());
							// Выполняем проверку доверенного сертификата
							if(!filename.empty()){
								// Выполняем декодирование адреса файла
								filename = this->uri->urlDecode(filename);
								// Если адрес файла существует
								if((result = fs_t::isfile(filename))){
									// Выполняем проверку доверенного сертификата
									SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(filename.c_str()));
									// Переходим к следующей итерации
									goto Next;
								}
							}
						}
						// Выполняем очистку адреса доверенного сертификата
						this->trusted.clear();
					/**
					 * Если операционной системой является Nix-подобная
					 */
					#else
						// Выполняем сплит адреса
						auto path = this->uri->splitPath(trustdir, FS_SEPARATOR);
						// Добавляем адрес файла в список
						path.push_back(this->trusted);
						// Формируем полный адарес файла
						string filename = this->uri->joinPath(path, FS_SEPARATOR);
						// Выполняем проверку доверенного сертификата
						if(!filename.empty()){
							// Выполняем декодирование адреса файла
							filename = this->uri->urlDecode(filename);
							// Если адрес файла существует
							if((result = fs_t::isfile(filename))){
								// Выполняем проверку CA файла
								SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(filename.c_str()));
								// Переходим к следующей итерации
								goto Next;
							}
						}
						// Выполняем очистку адреса доверенного сертификата
						this->trusted.clear();
					#endif
				// Если адрес файла существует
				} else if((result = fs_t::isfile(this->trusted)))
					// Выполняем проверку доверенного сертификата
					SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(this->trusted.c_str()));
				// Выполняем очистку адреса доверенного сертификата
				else this->trusted.clear();
			// Если адрес файла существует
			} else if((result = fs_t::isfile(this->trusted)))
				// Выполняем проверку доверенного сертификата
				SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(this->trusted.c_str()));
		}
		// Метка следующей итерации
		Next:
		// Если доверенный сертификат не указан
		if(this->trusted.empty()){
			// Получаем данные стора
			X509_STORE * store = SSL_CTX_get_cert_store(ctx);
			/**
			 * Если операционной системой является MS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				/**
				 * addCertToStoreFn Функция проверки параметров сертификата
				 * @param store стор с сертификатами для работы
				 * @param name  название параметра сертификата
				 * @return      результат проверки
				 */
				auto addCertToStoreFn = [this](X509_STORE * store = nullptr, const char * name = nullptr) noexcept -> int {
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
				// Выводим в лог сообщение
				this->log->print("%s", log_t::flag_t::CRITICAL, "set default paths for x509 store is not allow");
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * clear Метод очистки контекста
 * @param ctx контекст для очистки
 */
void awh::ASSL::clear(ctx_t & ctx) const noexcept {
	// Если сокет активен
	if(ctx.fd > -1){
		/**
		 * Если операционной системой является Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Запрещаем работу с сокетом
			shutdown(ctx.fd, SD_BOTH);
			// Выполняем закрытие сокета
			closesocket(ctx.fd);
		/**
		 * Если операционной системой является Nix-подобная
		 */
		#else
			// Запрещаем работу с сокетом
			shutdown(ctx.fd, SHUT_RDWR);
			// Выполняем закрытие сокета
			::close(ctx.fd);
		#endif
		// Выполняем сброс сокета
		ctx.fd = -1;
	}
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
	/*
	// Если BIO создано
	if(ctx.bio != nullptr){
		// Выполняем очистку BIO
		BIO_free(ctx.bio);
		// Зануляем контекст BIO
		ctx.bio = nullptr;
	}
	*/
	// Сбрасываем флаг инициализации
	ctx.mode = false;
}
/**
 * wrap Метод обертывания файлового дескриптора для сервера
 * @param ctx контекст для очистки
 * @return    объект SSL контекста
 */
awh::ASSL::ctx_t awh::ASSL::wrap(ctx_t & ctx) noexcept {
	// Если объект ещё не обёрнут в SSL контекст
	if(!ctx.mode)
		// Выполняем обёртывание уже активного SSL контекста
		return this->wrap(ctx.fd);
	// Если объект уже обёрнут, выводим его как есть
	else return ctx;
}
/**
 * wrap Метод обертывания файлового дескриптора для клиента
 * @param ctx контекст для очистки
 * @param url Параметры URL адреса для инициализации
 * @return    объект SSL контекста
 */
awh::ASSL::ctx_t awh::ASSL::wrap(ctx_t & ctx, const uri_t::url_t & url) noexcept {
	// Если объект ещё не обёрнут в SSL контекст
	if(!ctx.mode)
		// Выполняем обёртывание уже активного SSL контекста
		return this->wrap(ctx.fd, url);
	// Если объект уже обёрнут, выводим его как есть
	else return ctx;
}
/**
 * wrap Метод обертывания файлового дескриптора для сервера
 * @param fd   файловый дескриптор (сокет)
 * @param mode флаг выполнения обертывания файлового дескриптора
 * @return     объект SSL контекста
 */
awh::ASSL::ctx_t awh::ASSL::wrap(const int fd, const bool mode) noexcept {
	// Результат работы функции
	ctx_t result(this->log);
	// Устанавливаем файловый дескриптор
	result.fd = fd;
	// Устанавливаем тип приложения
	result.type = type_t::SERVER;
	// Отключаем алгоритм Нейгла
	BIO_set_tcp_ndelay(result.fd, 1);
	// Если обёртывание выполнять не нужно, выходим
	if(!mode) return result;
	// Если объект фреймворка существует
	if((fd >= 0) && !this->privkey.empty() && !this->fullchain.empty()){
		// Активируем рандомный генератор
		if(RAND_poll() == 0){
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "rand poll is not allow");
			// Выходим
			return result;
		}
		// Получаем контекст OpenSSL
		result.ctx = SSL_CTX_new(TLSv1_2_server_method());
		// Если контекст не создан
		if(result.ctx == nullptr){
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "context ssl is not initialization");
			// Выходим
			return result;
		}
		// Устанавливаем опции запроса
		SSL_CTX_set_options(result.ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
		// Устанавливаем минимально-возможную версию TLS
		SSL_CTX_set_min_proto_version(result.ctx, 0);
		// Устанавливаем максимально-возможную версию TLS
		SSL_CTX_set_max_proto_version(result.ctx, TLS1_3_VERSION);
		// Если нужно установить основные алгоритмы шифрования
		if(!this->cipher.empty()){
			// Устанавливаем все основные алгоритмы шифрования
			if(!SSL_CTX_set_cipher_list(result.ctx, this->cipher.c_str())){
				// Очищаем созданный контекст
				this->clear(result);
				// Выводим в лог сообщение
				this->log->print("%s", log_t::flag_t::CRITICAL, "set ssl ciphers");
				// Выходим
				return result;
			}
			// Заставляем серверные алгоритмы шифрования использовать в приоритете
			SSL_CTX_set_options(result.ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
		}
		// Устанавливаем поддерживаемые кривые
		if(!SSL_CTX_set_ecdh_auto(result.ctx, 1)){
			// Очищаем созданный контекст
			this->clear(result);
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "set ssl ecdh");
			// Выходим
			return result;
		}
		// Выполняем инициализацию доверенного сертификата
		if(!this->initTrustedStore(result.ctx)){
			// Очищаем созданный контекст
			this->clear(result);
			// Выходим
			return result;
		}
		// Устанавливаем флаг quiet shutdown
		SSL_CTX_set_quiet_shutdown(result.ctx, 1);
		// Запускаем кэширование
		SSL_CTX_set_session_cache_mode(result.ctx, SSL_SESS_CACHE_SERVER | SSL_SESS_CACHE_NO_INTERNAL);
		// Если цепочка сертификатов установлена
		if(!this->fullchain.empty()){
			// Если цепочка сертификатов не установлена
			if(SSL_CTX_use_certificate_chain_file(result.ctx, this->fullchain.c_str()) < 1){
				// Выводим в лог сообщение
				this->log->print("%s", log_t::flag_t::CRITICAL, "certificate fullchain cannot be set");
				// Очищаем созданный контекст
				this->clear(result);
				// Выходим
				return result;
			}
		}
		// Если приватный ключ установлен
		if(!this->privkey.empty()){
			// Если приватный ключ не может быть установлен
			if(SSL_CTX_use_PrivateKey_file(result.ctx, this->privkey.c_str(), SSL_FILETYPE_PEM) < 1){
				// Выводим в лог сообщение
				this->log->print("%s", log_t::flag_t::CRITICAL, "private key cannot be set");
				// Очищаем созданный контекст
				this->clear(result);
				// Выходим
				return result;
			}
		}	
		// Если приватный ключ недействителен
		if(SSL_CTX_check_private_key(result.ctx) < 1){
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "private key is not valid");
			// Очищаем созданный контекст
			this->clear(result);
			// Выходим
			return result;
		}
		// Если доверенный сертификат недействителен
		if(SSL_CTX_set_default_verify_file(result.ctx) < 1){
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "trusted certificate is invalid");
			// Очищаем созданный контекст
			this->clear(result);
			// Выходим
			return result;
		}
   		// Запрещаем выполнять првоерку сертификата пользователя
		SSL_CTX_set_verify(result.ctx, SSL_VERIFY_NONE, nullptr);
		// Создаем ssl объект
		result.ssl = SSL_new(result.ctx);
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
				// Выходим
				return result;
			}
		}
		// Выполняем обёртывание сокета в BIO SSL
		result.bio = BIO_new_socket(result.fd, BIO_NOCLOSE);
		// Если BIO SSL создано
		if(result.bio != nullptr){
			// Устанавливаем неблокирующий режим ввода/вывода для сокета
			result.noblock();
			// Выполняем установку BIO SSL
			SSL_set_bio(result.ssl, result.bio, result.bio);
			// Выполняем активацию сервера SSL
			SSL_set_accept_state(result.ssl);
		// Если BIO SSL не создано
		} else {
			// Очищаем созданный контекст
			this->clear(result);
			// Выводим сообщение об ошибке
			this->log->print("BIO new socket is failed", log_t::flag_t::CRITICAL);
			// Выходим из функции
			return result;
		}
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
 * wrap Метод обертывания файлового дескриптора для клиента
 * @param fd  файловый дескриптор (сокет)
 * @param url Параметры URL адреса для инициализации
 * @return    объект SSL контекста
 */
awh::ASSL::ctx_t awh::ASSL::wrap(const int fd, const uri_t::url_t & url) noexcept {
	// Результат работы функции
	ctx_t result(this->log);
	// Устанавливаем файловый дескриптор
	result.fd = fd;
	// Устанавливаем тип приложения
	result.type = type_t::CLIENT;
	// Отключаем алгоритм Нейгла
	BIO_set_tcp_ndelay(result.fd, 1);
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
		result.ctx = SSL_CTX_new(TLSv1_2_client_method());
		// Если контекст не создан
		if(result.ctx == nullptr){
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, "context ssl is not initialization");
			// Выходим
			return result;
		}
		// Устанавливаем опции запроса
		SSL_CTX_set_options(result.ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
		// Выполняем инициализацию доверенного сертификата
		if(!this->initTrustedStore(result.ctx)){
			// Очищаем созданный контекст
			this->clear(result);
			// Выходим
			return result;
		}
		// Если нужно произвести проверку
		if(this->verify && !url.domain.empty()){
			// Создаём объект проверки домена
			result.verify = new verify_t(url.domain, this);
			// Выполняем проверку сертификата
			SSL_CTX_set_verify(result.ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, nullptr);
			// Выполняем проверку всех дочерних сертификатов
			SSL_CTX_set_cert_verify_callback(result.ctx, &verifyHost, result.verify);
		// Запрещаем выполнять првоерку сертификата пользователя
		} else SSL_CTX_set_verify(result.ctx, SSL_VERIFY_NONE, nullptr);
		// Создаем ssl объект
		result.ssl = SSL_new(result.ctx);
		/**
		 * Если нужно установить TLS расширение
		 */
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
		// Выполняем обёртывание сокета в BIO SSL
		result.bio = BIO_new_socket(result.fd, BIO_NOCLOSE);
		// Если BIO SSL создано
		if(result.bio != nullptr){
			// Устанавливаем блокирующий режим ввода/вывода для сокета
			result.block();
			// Выполняем установку BIO SSL
			SSL_set_bio(result.ssl, result.bio, result.bio);
			// Выполняем активацию клиента SSL
			SSL_set_connect_state(result.ssl);
		// Если BIO SSL не создано
		} else {
			// Очищаем созданный контекст
			this->clear(result);
			// Выводим сообщение об ошибке
			this->log->print("BIO new socket is failed", log_t::flag_t::CRITICAL);
			// Выходим из функции
			return result;
		}
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
 * setVerify Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::ASSL::setVerify(const bool mode) noexcept {
	// Устанавливаем флаг проверки
	this->verify = mode;
}
/**
 * setCipher Метод установки алгоритмов шифрования
 * @param cipher список алгоритмов шифрования для установки
 */
void awh::ASSL::setCipher(const vector <string> & cipher) noexcept {
	// Если список алгоритмов шифрования передан
	if(!cipher.empty()){
		// Очищаем установленный список алгоритмов шифрования
		this->cipher.clear();
		// Переходим по всему списку алгоритмов шифрования
		for(auto & cip : cipher){
			// Если список алгоритмов шифрования уже не пустой, вставляем разделитель
			if(!this->cipher.empty())
				// Устанавливаем разделитель
				this->cipher.append(":");
			// Устанавливаем алгоритм шифрования
			this->cipher.append(cip);
		}
	}
}
/**
 * setTrusted Метод установки доверенного сертификата (CA-файла)
 * @param trusted адрес доверенного сертификата (CA-файла)
 * @param path    адрес каталога где находится сертификат (CA-файл)
 */
void awh::ASSL::setTrusted(const string & trusted, const string & path) noexcept {
	// Если адрес CA-файла передан
	if(!trusted.empty()){
		// Устанавливаем адрес доверенного сертификата (CA-файла)
		this->trusted = fs_t::realPath(trusted);
		// Если адрес каталога с доверенным сертификатом (CA-файлом) передан, устанавливаем и его
		if(!path.empty() && fs_t::isdir(path))
			// Устанавливаем адрес каталога с доверенным сертификатом (CA-файлом)
			this->path = fs_t::realPath(path);
	}
}
/**
 * setCert Метод установки файлов сертификата
 * @param chain файл цепочки сертификатов
 * @param key   приватный ключ сертификата (если требуется)
 */
void awh::ASSL::setCertificate(const string & chain, const string & key) noexcept {
	// Устанавливаем приватный ключ сертификата
	this->privkey = key;
	// Устанавливаем файл полной цепочки сертификатов
	this->fullchain = chain;
}
/**
 * ASSL Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 */
awh::ASSL::ASSL(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : fmk(fmk), uri(uri), log(log) {
	// Выполняем модификацию доверенного сертификата (CA-файла)
	this->trusted = fs_t::realPath(this->trusted);
	// Выполняем установку алгоритмов шифрования
	this->setCipher({
		"ECDHE-RSA-AES128-GCM-SHA256",
		"ECDHE-ECDSA-AES128-GCM-SHA256",
		"ECDHE-RSA-AES256-GCM-SHA384",
		"ECDHE-ECDSA-AES256-GCM-SHA384",
		"DHE-RSA-AES128-GCM-SHA256",
		"DHE-DSS-AES128-GCM-SHA256",
		"kEDH+AESGCM",
		"ECDHE-RSA-AES128-SHA256",
		"ECDHE-ECDSA-AES128-SHA256",
		"ECDHE-RSA-AES128-SHA",
		"ECDHE-ECDSA-AES128-SHA",
		"ECDHE-RSA-AES256-SHA384",
		"ECDHE-ECDSA-AES256-SHA384",
		"ECDHE-RSA-AES256-SHA",
		"ECDHE-ECDSA-AES256-SHA",
		"DHE-RSA-AES128-SHA256",
		"DHE-RSA-AES128-SHA",
		"DHE-DSS-AES128-SHA256",
		"DHE-RSA-AES256-SHA256",
		"DHE-DSS-AES256-SHA",
		"DHE-RSA-AES256-SHA",
		"AES128-GCM-SHA256",
		"AES256-GCM-SHA384",
		"AES128-SHA256",
		"AES256-SHA256",
		"AES128-SHA",
		"AES256-SHA",
		"AES",
		"CAMELLIA",
		"DES-CBC3-SHA",
		"!aNULL",
		"!eNULL",
		"!EXPORT",
		"!DES",
		"!RC4",
		"!MD5",
		"!PSK",
		"!aECDH",
		"!EDH-DSS-DES-CBC3-SHA",
		"!EDH-RSA-DES-CBC3-SHA",
		"!KRB5-DES-CBC3-SHA"
	});
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
	/**
	 * Если версия OpenSSL старая
	 */
	#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000L)
		// Выполняем освобождение памяти
		EVP_cleanup();
		ERR_free_strings();
		/**
		 * Если версия OpenSSL старая
		 */
		#if OPENSSL_VERSION_NUMBER < 0x10000000L
			// Освобождаем стейт
			ERR_remove_state(0);
		/**
		 * Если версия OpenSSL более новая
		 */
		#else
			// Освобождаем стейт для потока
			ERR_remove_thread_state(nullptr);
		#endif
		// Освобождаем оставшиеся данные
		CRYPTO_cleanup_all_ex_data();
		sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
	#endif
}
