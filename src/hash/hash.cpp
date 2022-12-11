/**
 * @file: hash.cpp
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
#include <hash/hash.hpp>

/**
 * init Метод инициализации AES шифрования
 * @return результат инициализации
 */
bool awh::Hash::init() const {
	// Размеры массивов IV и KEY
	uint8_t keySize = 0, ivSize = 0;
	// Создаем тип шифрования
	const EVP_CIPHER * cipher = EVP_enc_null();
	// Определяем длину шифрования
	switch((u_short) this->_cipher){
		// Устанавливаем шифрование в 128
		case (u_short) cipher_t::AES128: {
			// Устанавливаем размер массива IV
			ivSize = 8;
			// Устанавливаем размер массива KEY
			keySize = 16;
			// Устанавливаем функцию шифрования
			cipher = EVP_aes_128_ecb();
		} break;
		// Устанавливаем шифрование в 192
		case (u_short) cipher_t::AES192: {
			// Устанавливаем размер массива IV
			ivSize = 12;
			// Устанавливаем размер массива KEY
			keySize = 24;
			// Устанавливаем функцию шифрования
			cipher = EVP_aes_192_ecb();
		} break;
		// Устанавливаем шифрование в 256
		case (u_short) cipher_t::AES256: {
			// Устанавливаем размер массива IV
			ivSize = 16;
			// Устанавливаем размер массива KEY
			keySize = 32;
			// Устанавливаем функцию шифрования
			cipher = EVP_aes_256_ecb();
		} break;
		// Если ничего не выбрано, сбрасываем
		default: return false;
	}
	// Создаем контекст
	EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();
	// Привязываем контекст к типу шифрования
	EVP_EncryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr);
	// Выделяем нужное количество памяти
	vector <u_char> iv(ivSize, 0);
	vector <u_char> key(keySize, 0);
	/*
	// Выделяем нужное количество памяти
	vector <u_char> iv(EVP_CIPHER_CTX_iv_length(ctx), 0);
	vector <u_char> key(EVP_CIPHER_CTX_key_length(ctx), 0);
	*/
	// Выполняем инициализацию ключа
	const int ok = EVP_BytesToKey(cipher, EVP_sha256(), (this->_salt.empty() ? nullptr : (u_char *) this->_salt.data()), (u_char *) this->_pass.data(), this->_pass.length(), this->_rounds, key.data(), iv.data());
	// Очищаем контекст
	EVP_CIPHER_CTX_free(ctx);
	// Если инициализация не произошла
	if(ok == 0) return false;
	// Устанавливаем ключ шифрования
	const int res = AES_set_encrypt_key(key.data(), key.size() * 8, &this->_key);
	// Если установка ключа не произошло
	if(res != 0) return false;
	// Обнуляем номер
	this->_state.num = 0;
	// Заполняем половину структуры нулями
	memset(this->_state.ivec, 0, sizeof(this->_state.ivec));
	// Копируем данные шифрования
	memcpy(this->_state.ivec, iv.data(), iv.size());
	// Выполняем шифрование
	// AES_encrypt(this->_state.ivec, this->_state.count, &this->_key);
	// Сообщаем что всё удачно
	return true;
}
/**
 * cipher Метод получения размера шифрования
 * @return размер шифрования
 */
awh::Hash::cipher_t awh::Hash::cipher() const {
	// Выводим размер шифрования
	return this->_cipher;
}
/**
 * cipher Метод установки размера шифрования
 * @param cipher размер шифрования (128, 192, 256)
 */
void awh::Hash::cipher(const cipher_t cipher) noexcept {
	// Устанавливаем размер шифрования
	this->_cipher = cipher;
}
/**
 * rmTail Метод удаления хвостовых данных
 * @param buffer буфер для удаления хвоста
 */
void awh::Hash::rmTail(vector <char> & buffer) const noexcept {
	// Если сообщение является финальным
	if(buffer.size() > sizeof(this->_btype)){
		// Выполняем поиск хвостового списка байт для удаления
		auto it = search(buffer.begin(), buffer.end(), this->_btype, this->_btype + sizeof(this->_btype));
		// Удаляем хвостовой список байт из буфера данных
		buffer.erase(it, buffer.end());
	}
}
/**
 * setTail Метод добавления хвостовых данных
 * @param buffer буфер для добавления хвоста
 */
void awh::Hash::setTail(vector <char> & buffer) const noexcept {
	// Добавляем хвостовой буфер в полезную нагрузку
	buffer.insert(buffer.end(), this->_btype, this->_btype + sizeof(this->_btype));
}
/**
 * compressBrotli Метод компрессии данных в Brotli
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @return       результат компрессии
 */
vector <char> awh::Hash::compressBrotli(const char * buffer, const size_t size) noexcept {
	// Результирующая строка с данными
	vector <char> result;
	// Если буфер для сжатия передан
	if((buffer != nullptr) && (size > 0)){
		// Получаем размер бинарного буфера входящих данных
		size_t sizeInput = size;
		// Создаём временный буфер данных
		vector <uint8_t> data(CHUNK_BUFFER_SIZE);
		// Получаем бинарный буфер входящих данных
		const uint8_t * nextInput = reinterpret_cast <const uint8_t *> (buffer);
		// Инициализируем стейт энкодера Brotli
		BrotliEncoderState * encoder = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
		// Выполняем сжатие данных
		while(true){
			// Если енкодер закончил работу, выходим
			if(BrotliEncoderIsFinished(encoder)) break;
			// Получаем размер буфера закодированных бинарных данных
			size_t sizeOutput = data.size();
			// Получаем буфер закодированных бинарных данных
			uint8_t * nextOutput = data.data();
			// Если сжатие данных закончено, то завершаем работу
			if(!BrotliEncoderCompressStream(encoder, BROTLI_OPERATION_FINISH, &sizeInput, &nextInput, &sizeOutput, &nextOutput, nullptr)){
				// Очищаем результат собранных данных
				result.clear();
				// Освобождаем память энкодера
				BrotliEncoderDestroyInstance(encoder);
				// Выходим из функции
				return result;
			}
			// Получаем размер полученных данных
			size_t size = (data.size() - sizeOutput);
			// Если данные получены, формируем результирующий буфер
			if(size > 0){
				// Получаем буфер данных
				const char * buffer = reinterpret_cast <const char *> (data.data());
				// Формируем результирующий буфер бинарных данных
				result.insert(result.end(), buffer, buffer + size);
			}
		}
		// Освобождаем память энкодера
		BrotliEncoderDestroyInstance(encoder);
	}
	// Выводим результат
	return result;
}
/**
 * decompressBrotli Метод декомпрессии данных в Brotli
 * @param buffer буфер данных для декомпрессии
 * @param size   размер данных для декомпрессии
 * @return       результат декомпрессии
 */
vector <char> awh::Hash::decompressBrotli(const char * buffer, const size_t size) noexcept {
	// Результирующая строка с данными
	vector <char> result;
	// Если буфер для сжатия передан
	if((buffer != nullptr) && (size > 0)){
		// Полный размер обработанных данных
		size_t total = 0;
		// Получаем размер бинарного буфера входящих данных
		size_t sizeInput = size;
		// Создаём временный буфер данных
		vector <uint8_t> data(CHUNK_BUFFER_SIZE);
		// Получаем бинарный буфер входящих данных
		const uint8_t * nextInput = reinterpret_cast <const uint8_t *> (buffer);
		// Активируем работу декодера
		BrotliDecoderResult rbr = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
		// Инициализируем стейт декодера Brotli
		BrotliDecoderState * decoder = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
		// Если декодеру есть с чем работать
		while(rbr == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT){
			// Получаем размер буфера декодированных бинарных данных
			size_t sizeOutput = data.size();
			// Получаем буфер декодированных бинарных данных
			char * nextOutput = reinterpret_cast <char *> (data.data());
			// Выполняем декодирование бинарных данных
			rbr = BrotliDecoderDecompressStream(decoder, &sizeInput, &nextInput, &sizeOutput, reinterpret_cast <uint8_t **> (&nextOutput), &total);
			// Если декодирование данных не выполнено
			if(rbr == BROTLI_DECODER_RESULT_ERROR){
				// Выполняем очистку результата
				result.clear();
				// Освобождаем память декодера
				BrotliDecoderDestroyInstance(decoder);
				// Выводим результат
				return result;
			}
			// Получаем размер полученных данных
			size_t size = (data.size() - sizeOutput);
			// Если данные получены, формируем результирующий буфер
			if(size > 0){
				// Получаем буфер данных
				const char * buffer = reinterpret_cast <const char *> (data.data());
				// Формируем результирующий буфер бинарных данных
				result.insert(result.end(), buffer, buffer + size);
			}
		}
		// Если декомпрессия данных выполнена не удачно
		if((rbr != BROTLI_DECODER_RESULT_SUCCESS) && (rbr != BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT))
			// Выполняем очистку результата
			result.clear();
		// Освобождаем память декодера
		BrotliDecoderDestroyInstance(decoder);
	}
	// Выводим результат
	return result;
}
/**
 * compressGzip Метод компрессии данных в GZIP
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @return       результат компрессии
 */
vector <char> awh::Hash::compressGzip(const char * buffer, const size_t size) const noexcept {
	// Результирующая строка с данными
	vector <char> result;
	// Если буфер для сжатия передан
	if((buffer != nullptr) && (size > 0)){
		// Результирующий размер данных
		int ret = 0;
		// Создаем поток zip
		z_stream zs;
		// Заполняем его нулями
		memset(&zs, 0, sizeof(zs));
		// Если поток инициализировать не удалось, выходим
		if(deflateInit2(&zs, this->levelGzip, Z_DEFLATED, MOD_GZIP_ZLIB_WINDOWSIZE + 16, MOD_GZIP_ZLIB_CFACTOR, Z_DEFAULT_STRATEGY) == Z_OK){
			// Указываем размер входного буфера
			zs.avail_in = size;
			// Заполняем входные данные буфера
			zs.next_in = (u_char *) const_cast <char *> (buffer);
			// Создаем буфер с сжатыми данными
			vector <u_char> output(zs.avail_in, 0);
			// Выполняем сжатие данных
			do {
				// Устанавливаем максимальный размер буфера
				zs.avail_out = zs.avail_in;
				// Устанавливаем буфер для получения результата
				zs.next_out = output.data();
				// Выполняем сжатие
				ret = deflate(&zs, Z_FINISH);
				// Если данные добавлены не полностью
				if(result.size() < zs.total_out)
					// Добавляем оставшиеся данные
					result.insert(result.end(), output.data(), output.data() + (zs.total_out - result.size()));
			} while(ret == Z_OK);
		}
		// Завершаем сжатие
		deflateEnd(&zs);
		// Если сжатие не удалось то очищаем выходные данные
		if(ret != Z_STREAM_END) result.clear();
	}
	// Выводим результат
	return result;
}
/**
 * decompressGzip Метод декомпрессии данных в GZIP
 * @param buffer буфер данных для декомпрессии
 * @param size   размер данных для декомпрессии
 * @return       результат декомпрессии
 */
vector <char> awh::Hash::decompressGzip(const char * buffer, const size_t size) const noexcept {
	// Результирующая строка с данными
	vector <char> result;
	// Если буфер для сжатия передан
	if((buffer != nullptr) && (size > 0)){
		// Результирующий размер данных
		int ret = 0;
		// Создаем поток zip
		z_stream zs;
		// Заполняем его нулями
		memset(&zs, 0, sizeof(zs));
		// Если поток инициализировать не удалось, выходим
		if(inflateInit2(&zs, MOD_GZIP_ZLIB_WINDOWSIZE + 16) == Z_OK){
			// Указываем размер входного буфера
			zs.avail_in = size;
			// Заполняем входные данные буфера
			zs.next_in = (u_char *) const_cast <char *> (buffer);
			// Получаем размер выходных данных
			const size_t length = (zs.avail_in * 10);
			// Создаем буфер с сжатыми данными
			vector <u_char> output(length, 0);
			// Выполняем расжатие данных
			do {
				// Устанавливаем максимальный размер буфера
				zs.avail_out = length;
				// Устанавливаем буфер для получения результата
				zs.next_out = output.data();
				// Выполняем расжатие
				ret = inflate(&zs, 0);
				// Если данные добавлены не полностью
				if(result.size() < zs.total_out)
					// Добавляем оставшиеся данные
					result.insert(result.end(), output.data(), output.data() + (zs.total_out - result.size()));
			} while(ret == Z_OK);
		}
		// Завершаем расжатие
		inflateEnd(&zs);
		// Если сжатие не удалось то очищаем выходные данные
		if(ret != Z_STREAM_END) result.clear();
	}
	// Выводим результат
	return result;
}
/**
 * compressDeflate Метод компрессии данных в DEFLATE
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @return       результат компрессии
 */
vector <char> awh::Hash::compressDeflate(const char * buffer, const size_t size) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если буфер передан
	if((buffer != nullptr) && (size > 0)){
		// Создаем поток zip
		z_stream zs = {0};
		// Обнуляем структуру
		zs.zfree  = Z_NULL;
		zs.zalloc = Z_NULL;
		zs.opaque = Z_NULL;
		// Буфер выходных данных		
		vector <u_char> output(size, 0);
		// Если поток инициализировать не удалось, выходим
		if(this->_takeOverCompress || (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -1 * this->_wbit, DEFAULT_MEM_LEVEL, Z_HUFFMAN_ONLY) == Z_OK)){
			// Результат проверки декомпрессии
			int ret = Z_OK;
			// Если поток декомпрессора не создан ранее
			if(!this->_takeOverCompress){
				// Устанавливаем количество доступных данных
				zs.avail_in = size;
				// Устанавливаем буфер с данными для шифрования
				zs.next_in = (Bytef *) buffer;
			// Если нужно переиспользовать поток декомпрессора
			} else {
				// Устанавливаем количество доступных данных
				this->_zdef.avail_in = size;
				// Устанавливаем буфер с данными для шифрования
				this->_zdef.next_in = (Bytef *) buffer;
			}
			/**
			 * Выполняем компрессию всех данных
			 */
			do {
				// Если поток декомпрессора не создан ранее
				if(!this->_takeOverCompress){
					// Устанавливаем буфер для записи шифрованных данных
					zs.next_out = output.data();
					// Устанавливаем количество доступных данных для записи
					zs.avail_out = output.size();
					// Выполняем сжатие данных
					ret = deflate(&zs, Z_SYNC_FLUSH);
				// Если нужно переиспользовать поток декомпрессора
				} else {
					// Устанавливаем буфер для записи шифрованных данных
					this->_zdef.next_out = output.data();
					// Устанавливаем количество доступных данных для записи
					this->_zdef.avail_out = output.size();
					// Выполняем сжатие данных
					ret = deflate(&this->_zdef, Z_FULL_FLUSH);
				}
				// Если данные обработаны удачно
				if((ret == Z_OK) || (ret == Z_STREAM_END)){
					// Получаем размер полученных данных
					size_t size = (output.size() - (!this->_takeOverDecompress ? zs.avail_out : this->_zdef.avail_out));
					// Добавляем оставшиеся данные в список
					result.insert(result.end(), output.begin(), output.begin() + size);
				// Если данные не могут быть обработанны, то выходим
				} else break;
			// Если все данные уже сжаты
			} while(ret != Z_STREAM_END);
			// Закрываем поток
			if(!this->_takeOverCompress) deflateEnd(&zs);
		}
	}
	// Выводим результат
	return result;
}
/**
 * decompressDeflate Метод декомпрессии данных в DEFLATE
 * @param buffer буфер данных для декомпрессии
 * @param size   размер данных для декомпрессии
 * @return       результат декомпрессии
 */
vector <char> awh::Hash::decompressDeflate(const char * buffer, const size_t size) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если буфер передан
	if((buffer != nullptr) && (size > 0)){
		// Создаем поток zip
		z_stream zs = {0};
		// Обнуляем структуру
		zs.zfree  = Z_NULL;
		zs.zalloc = Z_NULL;
		zs.opaque = Z_NULL;
		// Буфер выходных данных		
		vector <u_char> output(size, 0);
		// Если поток инициализировать не удалось, выходим
		if(this->_takeOverDecompress || (inflateInit2(&zs, -1 * this->_wbit) == Z_OK)){
			// Результат проверки декомпрессии
			int ret = Z_OK;
			// Если поток декомпрессора не создан ранее
			if(!this->_takeOverDecompress){
				// Устанавливаем количество доступных данных
				zs.avail_in = size;
				// Копируем входящий буфер для дешифровки
				zs.next_in = (Bytef *) buffer;
			// Если нужно переиспользовать поток декомпрессора
			} else {
				// Устанавливаем количество доступных данных
				this->_zinf.avail_in = size;
				// Копируем входящий буфер для дешифровки
				this->_zinf.next_in = (Bytef *) buffer;
			}
			/**
			 * Выполняем декомпрессию всех данных
			 */
			do {
				// Если поток декомпрессора не создан ранее
				if(!this->_takeOverDecompress){
					// Устанавливаем буфер для записи дешифрованных данных
					zs.next_out = output.data();
					// Устанавливаем количество доступных данных для записи
					zs.avail_out = output.size();
					// Выполняем декомпрессию данных
					ret = inflate(&zs, Z_NO_FLUSH);
				// Если нужно переиспользовать поток декомпрессора
				} else {
					// Устанавливаем буфер для записи дешифрованных данных
					this->_zinf.next_out = output.data();
					// Устанавливаем количество доступных данных для записи
					this->_zinf.avail_out = output.size();
					// Выполняем декомпрессию данных
					ret = inflate(&this->_zinf, Z_SYNC_FLUSH);
				}
				// Если данные обработаны удачно
				if((ret == Z_OK) || (ret == Z_STREAM_END)){
					// Получаем размер полученных данных
					size_t size = (output.size() - (!this->_takeOverDecompress ? zs.avail_out : this->_zinf.avail_out));
					// Добавляем оставшиеся данные в список
					result.insert(result.end(), output.begin(), output.begin() + size);
				// Если данные не могут быть обработанны, то выходим
				} else break;
			// Если все данные уже дешифрованы
			} while(ret != Z_STREAM_END);
			// Очищаем выделенную память для декомпрессора
			if(!this->_takeOverDecompress) inflateEnd(&zs);
		}
	}
	// Выводим результат
	return result;
}
/**
 * encrypt Метод шифрования текста
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @return       результат шифрования
 */
vector <char> awh::Hash::encrypt(const char * buffer, const size_t size) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Если пароль установлен
		if(!this->_pass.empty()){
			// Выполняем инициализацию AES
			if(this->init()){
				// Максимальный размер считываемых данных
				int chunk = 0;
				// Размер буфера полученных данных
				size_t count = 0;
				// Определяем размер данных для считывания
				size_t len = size;
				// Выделяем память для буфера данных
				vector <u_char> output(len, 0);
				// Входные данные
				const u_char * input = (u_char *) buffer;
				// Выполняем извлечение оставшихся данных
				do {
					// Максимальный размер считываемых данных
					chunk = (len > CHUNK_BUFFER_SIZE ? CHUNK_BUFFER_SIZE : len);
					// Выполняем сжатие данных
					AES_cfb128_encrypt(input + count, output.data() + count, chunk, &this->_key, this->_state.ivec, &this->_state.num, AES_ENCRYPT);
					// Увеличиваем смещение
					count += chunk;
					// Вычитаем считанные данные
					len -= chunk;
				} while(len > 0);
				// Запоминаем полученные данные
				result.insert(result.end(), output.data(), output.data() + count);
			}
		// Выводим тот же самый буфер как он был передан
		} else result.insert(result.end(), buffer, buffer + size);
	}
	// Выводим результат
	return result;
}
/**
 * decrypt Метод дешифрования текста
 * @param buffer буфер данных для дешифрования
 * @param size   размер данных для дешифрования
 * @return       результат дешифрования
 */
vector <char> awh::Hash::decrypt(const char * buffer, const size_t size) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Если пароль установлен
		if(!this->_pass.empty()){
			// Выполняем инициализацию AES
			if(this->init()){
				// Максимальный размер считываемых данных
				int chunk = 0;
				// Размер буфера полученных данных
				size_t count = 0;
				// Определяем размер данных для считывания
				size_t len = size;
				// Выделяем память для буфера данных
				vector <u_char> output(len, 0);
				// Входные данные
				const u_char * input = (u_char *) buffer;
				// Выполняем извлечение оставшихся данных
				do {
					// Максимальный размер считываемых данных
					chunk = (len > CHUNK_BUFFER_SIZE ? CHUNK_BUFFER_SIZE : len);
					// Выполняем сжатие данных
					AES_cfb128_encrypt(input + count, output.data() + count, chunk, &this->_key, this->_state.ivec, &this->_state.num, AES_DECRYPT);
					// Увеличиваем смещение
					count += chunk;
					// Вычитаем считанные данные
					len -= chunk;
				} while(len > 0);
				// Запоминаем полученные данные
				result.insert(result.end(), output.data(), output.data() + count);
			}
		// Выводим тот же самый буфер как он был передан
		} else result.insert(result.end(), buffer, buffer + size);
	}
	// Выводим результат
	return result;
}
/**
 * compress Метод компрессии данных
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @param method метод компрессии
 * @return       результат компрессии
 */
vector <char> awh::Hash::compress(const char * buffer, const size_t size, const method_t method) noexcept {
	// Определяем метод компрессии данных
	switch((uint8_t) method){
		// Если метод компрессии установлен GZIP
		case (uint8_t) method_t::GZIP:
			// Выполняем компрессию данных
			return this->compressGzip(buffer, size);
		// Если метод компрессии установлен BROTLI
		case (uint8_t) method_t::BROTLI:
			// Выполняем компрессию данных
			return this->compressBrotli(buffer, size);
		// Если метод компрессии установлен DEFLATE
		case (uint8_t) method_t::DEFLATE:
			// Выполняем компрессию данных
			return this->compressDeflate(buffer, size);
	}
	// Выходим из функции
	return vector <char> ();
}
/**
 * decompress Метод декомпрессии данных
 * @param buffer буфер данных для декомпрессии
 * @param size   размер данных для декомпрессии
 * @param method метод компрессии
 * @return       результат декомпрессии
 */
vector <char> awh::Hash::decompress(const char * buffer, const size_t size, const method_t method) noexcept {
	// Определяем метод декомпрессию данных
	switch((uint8_t) method){
		// Если метод декомпрессии установлен GZIP
		case (uint8_t) method_t::GZIP:
			// Выполняем компрессию данных
			return this->decompressGzip(buffer, size);
		// Если метод декомпрессии установлен BROTLI
		case (uint8_t) method_t::BROTLI:
			// Выполняем компрессию данных
			return this->decompressBrotli(buffer, size);
		// Если метод декомпрессии установлен DEFLATE
		case (uint8_t) method_t::DEFLATE:
			// Выполняем компрессию данных
			return this->decompressDeflate(buffer, size);
	}
	// Выходим из функции
	return vector <char> ();
}
/**
 * wbit Метод установки размера скользящего окна
 * @param wbit размер скользящего окна
 */
void awh::Hash::wbit(const short wbit) noexcept {
	// Устанавливаем размер скользящего окна
	this->_wbit = wbit;
	// Выполняем пересборку контекстов LZ77 для компрессии
	this->takeoverCompress(this->_takeOverCompress);
	// Выполняем пересборку контекстов LZ77 для декомпрессии
	this->takeoverDecompress(this->_takeOverDecompress);
}
/**
 * round Метод установки количества раундов шифрования
 * @param round количество раундов шифрования
 */
void awh::Hash::round(const int round) noexcept {
	// Устанавливаем количество раундов шифрования
	this->_rounds = round;
}
/**
 * salt Метод установки соли шифрования
 * @param salt соль для шифрования
 */
void awh::Hash::salt(const string & salt) noexcept {
	// Если соль передана
	this->_salt = salt;
}
/**
 * pass Метод установки пароля шифрования
 * @param pass пароль шифрования
 */
void awh::Hash::pass(const string & pass) noexcept {
	// Если пароль передан
	this->_pass = pass;
}
/**
 * takeoverCompress Метод установки флага переиспользования контекста компрессии
 * @param flag флаг переиспользования контекста компрессии
 */
void awh::Hash::takeoverCompress(const bool flag) noexcept {
	// Если флаг установлен
	if(this->_takeOverCompress)
		// Очищаем выделенную память для компрессора
		deflateEnd(&this->_zdef);
	// Устанавливаем переданный флаг
	this->_takeOverCompress = flag;
	// Если флаг установлен
	if(this->_takeOverCompress){
		// Заполняем его нулями потока для компрессора
		memset(&this->_zdef, 0, sizeof(this->_zdef));
		// Обнуляем структуру потока для компрессора
		this->_zdef.zalloc = Z_NULL;
		this->_zdef.zfree  = Z_NULL;
		this->_zdef.opaque = Z_NULL;
		// Если поток инициализировать не удалось, выходим
		if(deflateInit2(&this->_zdef, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -1 * this->_wbit, DEFAULT_MEM_LEVEL, Z_HUFFMAN_ONLY) != Z_OK){
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, "deflate stream is not create");
			/**
			 * Если операционной системой является Nix-подобная
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Выходим из приложения
				raise(SIGINT);
			/**
			 * Если операционной системой является MS Windows
			 */
			#else
				// Выходим из приложения
				exit(EXIT_FAILURE);
			#endif
		}
	}
}
/**
 * takeoverDecompress Метод установки флага переиспользования контекста декомпрессии
 * @param flag флаг переиспользования контекста декомпрессии
 */
void awh::Hash::takeoverDecompress(const bool flag) noexcept {
	// Если флаг установлен
	if(this->_takeOverDecompress)
		// Очищаем выделенную память для декомпрессора
		inflateEnd(&this->_zinf);
	// Устанавливаем переданный флаг
	this->_takeOverDecompress = flag;
	// Если флаг установлен
	if(this->_takeOverDecompress){
		// Заполняем его нулями потока для декомпрессора
		memset(&this->_zinf, 0, sizeof(this->_zinf));
		// Обнуляем структуру потока для декомпрессора
		this->_zinf.avail_in = 0;
		this->_zinf.zalloc   = Z_NULL;
		this->_zinf.zfree    = Z_NULL;
		this->_zinf.opaque   = Z_NULL;
		this->_zinf.next_in  = Z_NULL;
		// Если поток инициализировать не удалось, выходим
		if(inflateInit2(&this->_zinf, -1 * this->_wbit) != Z_OK){
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, "inflate stream is not create");
			/**
			 * Если операционной системой является Nix-подобная
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Выходим из приложения
				raise(SIGINT);
			/**
			 * Если операционной системой является MS Windows
			 */
			#else
				// Выходим из приложения
				exit(EXIT_FAILURE);
			#endif
		}
	}
}
/**
 * ~Hash Деструктор
 */
awh::Hash::~Hash() noexcept {
	// Очищаем выделенную память для компрессора
	if(this->_takeOverCompress) deflateEnd(&this->_zdef);
	// Очищаем выделенную память для декомпрессора
	if(this->_takeOverDecompress) inflateEnd(&this->_zinf);
}
