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
 * initAES Метод инициализации AES шифрования
 * @return результат инициализации
 */
bool awh::Hash::initAES() const {
	// Размеры массивов IV и KEY
	uint8_t keySize = 0, ivSize = 0;
	// Создаем тип шифрования
	const EVP_CIPHER * cipher = EVP_enc_null();
	// Определяем длину шифрования
	switch((u_short) this->aesSize){
		// Устанавливаем шифрование в 128
		case (u_short) aes_t::AES128: {
			// Устанавливаем размер массива IV
			ivSize = 8;
			// Устанавливаем размер массива KEY
			keySize = 16;
			// Устанавливаем функцию шифрования
			cipher = EVP_aes_128_ecb();
		} break;
		// Устанавливаем шифрование в 192
		case (u_short) aes_t::AES192: {
			// Устанавливаем размер массива IV
			ivSize = 12;
			// Устанавливаем размер массива KEY
			keySize = 24;
			// Устанавливаем функцию шифрования
			cipher = EVP_aes_192_ecb();
		} break;
		// Устанавливаем шифрование в 256
		case (u_short) aes_t::AES256: {
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
	const int ok = EVP_BytesToKey(cipher, EVP_sha256(), (this->salt.empty() ? nullptr : (u_char *) this->salt.data()), (u_char *) this->password.data(), this->password.length(), this->roundsAES, key.data(), iv.data());
	// Очищаем контекст
	EVP_CIPHER_CTX_free(ctx);
	// Если инициализация не произошла
	if(ok == 0) return false;
	// Устанавливаем ключ шифрования
	const int res = AES_set_encrypt_key(key.data(), key.size() * 8, &this->aesKey);
	// Если установка ключа не произошло
	if(res != 0) return false;
	// Обнуляем номер
	this->stateAES.num = 0;
	// Заполняем половину структуры нулями
	memset(this->stateAES.ivec, 0, sizeof(this->stateAES.ivec));
	// Копируем данные шифрования
	memcpy(this->stateAES.ivec, iv.data(), iv.size());
	// Выполняем шифрование
	// AES_encrypt(this->stateAES.ivec, this->stateAES.count, &this->aesKey);
	// Сообщаем что всё удачно
	return true;
}
/**
 * getAES Метод получения размера шифрования
 * @return размер шифрования
 */
awh::Hash::aes_t awh::Hash::getAES() const {
	// Выводим размер шифрования
	return this->aesSize;
}
/**
 * rmTail Метод удаления хвостовых данных
 * @param buffer буфер для удаления хвоста
 */
void awh::Hash::rmTail(vector <char> & buffer) const noexcept {
	// Если сообщение является финальным
	if(buffer.size() > sizeof(this->btype)){
		// Выполняем поиск хвостового списка байт для удаления
		auto it = search(buffer.begin(), buffer.end(), this->btype, this->btype + sizeof(this->btype));
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
	buffer.insert(buffer.end(), this->btype, this->btype + sizeof(this->btype));
}
/**
 * encrypt Метод шифрования текста
 * @param buffer буфер данных для шифрования
 * @param size   размер данных для шифрования
 * @return       результат шифрования
 */
const vector <char> awh::Hash::encrypt(const char * buffer, const size_t size) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Если пароль установлен
		if(!this->password.empty()){
			// Выполняем инициализацию AES
			if(this->initAES()){
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
					AES_cfb128_encrypt(input + count, output.data() + count, chunk, &this->aesKey, this->stateAES.ivec, &this->stateAES.num, AES_ENCRYPT);
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
const vector <char> awh::Hash::decrypt(const char * buffer, const size_t size) const noexcept {
	// Результат работы функции
	vector <char> result;
	// Если буфер данных передан
	if((buffer != nullptr) && (size > 0)){
		// Если пароль установлен
		if(!this->password.empty()){
			// Выполняем инициализацию AES
			if(this->initAES()){
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
					AES_cfb128_encrypt(input + count, output.data() + count, chunk, &this->aesKey, this->stateAES.ivec, &this->stateAES.num, AES_DECRYPT);
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
 * @return       результат компрессии
 */
const vector <char> awh::Hash::compress(const char * buffer, const size_t size) const noexcept {
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
		if(this->takeOverCompress || (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -1 * this->wbit, DEFAULT_MEM_LEVEL, Z_HUFFMAN_ONLY) == Z_OK)){
			// Результат проверки декомпрессии
			int ret = Z_OK;
			// Если поток декомпрессора не создан ранее
			if(!this->takeOverCompress){
				// Устанавливаем количество доступных данных
				zs.avail_in = size;
				// Устанавливаем буфер с данными для шифрования
				zs.next_in = (Bytef *) buffer;
			// Если нужно переиспользовать поток декомпрессора
			} else {
				// Устанавливаем количество доступных данных
				this->zdef.avail_in = size;
				// Устанавливаем буфер с данными для шифрования
				this->zdef.next_in = (Bytef *) buffer;
			}
			/**
			 * Выполняем компрессию всех данных
			 */
			do {
				// Если поток декомпрессора не создан ранее
				if(!this->takeOverCompress){
					// Устанавливаем буфер для записи шифрованных данных
					zs.next_out = output.data();
					// Устанавливаем количество доступных данных для записи
					zs.avail_out = output.size();
					// Выполняем сжатие данных
					ret = deflate(&zs, Z_SYNC_FLUSH);
				// Если нужно переиспользовать поток декомпрессора
				} else {
					// Устанавливаем буфер для записи шифрованных данных
					this->zdef.next_out = output.data();
					// Устанавливаем количество доступных данных для записи
					this->zdef.avail_out = output.size();
					// Выполняем сжатие данных
					ret = deflate(&this->zdef, Z_FULL_FLUSH);
				}
				// Если данные обработаны удачно
				if((ret == Z_OK) || (ret == Z_STREAM_END)){
					// Получаем размер полученных данных
					size_t size = (output.size() - (!this->takeOverDecompress ? zs.avail_out : this->zdef.avail_out));
					// Добавляем оставшиеся данные в список
					result.insert(result.end(), output.begin(), output.begin() + size);
				// Если данные не могут быть обработанны, то выходим
				} else break;
			// Если все данные уже сжаты
			} while(ret != Z_STREAM_END);
			// Закрываем поток
			if(!this->takeOverCompress) deflateEnd(&zs);
		}
	}
	// Выводим результат
	return result;
}
/**
 * decompress Метод декомпрессии данных
 * @param buffer буфер данных для декомпрессии
 * @param size   размер данных для декомпрессии
 * @return       результат декомпрессии
 */
const vector <char> awh::Hash::decompress(const char * buffer, const size_t size) const noexcept {
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
		if(this->takeOverDecompress || (inflateInit2(&zs, -1 * this->wbit) == Z_OK)){
			// Результат проверки декомпрессии
			int ret = Z_OK;
			// Если поток декомпрессора не создан ранее
			if(!this->takeOverDecompress){
				// Устанавливаем количество доступных данных
				zs.avail_in = size;
				// Копируем входящий буфер для дешифровки
				zs.next_in = (Bytef *) buffer;
			// Если нужно переиспользовать поток декомпрессора
			} else {
				// Устанавливаем количество доступных данных
				this->zinf.avail_in = size;
				// Копируем входящий буфер для дешифровки
				this->zinf.next_in = (Bytef *) buffer;
			}
			/**
			 * Выполняем декомпрессию всех данных
			 */
			do {
				// Если поток декомпрессора не создан ранее
				if(!this->takeOverDecompress){
					// Устанавливаем буфер для записи дешифрованных данных
					zs.next_out = output.data();
					// Устанавливаем количество доступных данных для записи
					zs.avail_out = output.size();
					// Выполняем декомпрессию данных
					ret = inflate(&zs, Z_NO_FLUSH);
				// Если нужно переиспользовать поток декомпрессора
				} else {
					// Устанавливаем буфер для записи дешифрованных данных
					this->zinf.next_out = output.data();
					// Устанавливаем количество доступных данных для записи
					this->zinf.avail_out = output.size();
					// Выполняем декомпрессию данных
					ret = inflate(&this->zinf, Z_SYNC_FLUSH);
				}
				// Если данные обработаны удачно
				if((ret == Z_OK) || (ret == Z_STREAM_END)){
					// Получаем размер полученных данных
					size_t size = (output.size() - (!this->takeOverDecompress ? zs.avail_out : this->zinf.avail_out));
					// Добавляем оставшиеся данные в список
					result.insert(result.end(), output.begin(), output.begin() + size);
				// Если данные не могут быть обработанны, то выходим
				} else break;
			// Если все данные уже дешифрованы
			} while(ret != Z_STREAM_END);
			// Очищаем выделенную память для декомпрессора
			if(!this->takeOverDecompress) inflateEnd(&zs);
		}
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
const vector <char> awh::Hash::compressGzip(const char * buffer, const size_t size) const noexcept {
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
const vector <char> awh::Hash::decompressGzip(const char * buffer, const size_t size) const noexcept {
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
 * compressBrotli Метод компрессии данных в Brotli
 * @param buffer буфер данных для компрессии
 * @param size   размер данных для компрессии
 * @return       результат компрессии
 */
const vector <char> awh::Hash::compressBrotli(const char * buffer, const size_t size) noexcept {
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
const vector <char> awh::Hash::decompressBrotli(const char * buffer, const size_t size) noexcept {
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
 * setAES Метод установки размера шифрования
 * @param size размер шифрования (128, 192, 256)
 */
void awh::Hash::setAES(const aes_t size) noexcept {
	// Устанавливаем размер шифрования
	this->aesSize = size;
}
/**
 * setWbit Метод установки размера скользящего окна
 * @param wbit размер скользящего окна
 */
void awh::Hash::setWbit(const short wbit) noexcept {
	// Устанавливаем размер скользящего окна
	this->wbit = wbit;
	// Выполняем пересборку контекстов LZ77 для компрессии
	this->setTakeoverCompress(this->takeOverCompress);
	// Выполняем пересборку контекстов LZ77 для декомпрессии
	this->setTakeoverDecompress(this->takeOverDecompress);
}
/**
 * setRoundAES Метод установки количества раундов шифрования
 * @param round количество раундов шифрования
 */
void awh::Hash::setRoundAES(const int round) noexcept {
	// Устанавливаем количество раундов шифрования
	this->roundsAES = round;
}
/**
 * setSalt Метод установки соли шифрования
 * @param salt соль для шифрования
 */
void awh::Hash::setSalt(const string & salt) noexcept {
	// Если соль передана
	this->salt = salt;
}
/**
 * setPassword Метод установки пароля шифрования
 * @param password пароль шифрования
 */
void awh::Hash::setPassword(const string & password) noexcept {
	// Если пароль передан
	this->password = password;
}
/**
 * setTakeoverCompress Метод установки флага переиспользования контекста компрессии
 * @param flag флаг переиспользования контекста компрессии
 */
void awh::Hash::setTakeoverCompress(const bool flag) noexcept {
	// Если флаг установлен
	if(this->takeOverCompress)
		// Очищаем выделенную память для компрессора
		deflateEnd(&this->zdef);
	// Устанавливаем переданный флаг
	this->takeOverCompress = flag;
	// Если флаг установлен
	if(this->takeOverCompress){
		// Заполняем его нулями потока для компрессора
		memset(&this->zdef, 0, sizeof(this->zdef));
		// Обнуляем структуру потока для компрессора
		this->zdef.zalloc = Z_NULL;
		this->zdef.zfree  = Z_NULL;
		this->zdef.opaque = Z_NULL;
		// Если поток инициализировать не удалось, выходим
		if(deflateInit2(&this->zdef, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -1 * this->wbit, DEFAULT_MEM_LEVEL, Z_HUFFMAN_ONLY) != Z_OK){
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::CRITICAL, "deflate stream is not create");
			// Выходим из приложения
			exit(EXIT_FAILURE);
		}
	}
}
/**
 * setTakeoverDecompress Метод установки флага переиспользования контекста декомпрессии
 * @param flag флаг переиспользования контекста декомпрессии
 */
void awh::Hash::setTakeoverDecompress(const bool flag) noexcept {
	// Если флаг установлен
	if(this->takeOverDecompress)
		// Очищаем выделенную память для декомпрессора
		inflateEnd(&this->zinf);
	// Устанавливаем переданный флаг
	this->takeOverDecompress = flag;
	// Если флаг установлен
	if(this->takeOverDecompress){
		// Заполняем его нулями потока для декомпрессора
		memset(&this->zinf, 0, sizeof(this->zinf));
		// Обнуляем структуру потока для декомпрессора
		this->zinf.avail_in = 0;
		this->zinf.zalloc   = Z_NULL;
		this->zinf.zfree    = Z_NULL;
		this->zinf.opaque   = Z_NULL;
		this->zinf.next_in  = Z_NULL;
		// Если поток инициализировать не удалось, выходим
		if(inflateInit2(&this->zinf, -1 * this->wbit) != Z_OK){
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::CRITICAL, "inflate stream is not create");
			// Выходим из приложения
			exit(EXIT_FAILURE);
		}
	}
}
/**
 * ~Hash Деструктор
 */
awh::Hash::~Hash() noexcept {
	// Очищаем выделенную память для компрессора
	if(this->takeOverCompress) deflateEnd(&this->zdef);
	// Очищаем выделенную память для декомпрессора
	if(this->takeOverDecompress) inflateEnd(&this->zinf);
}
