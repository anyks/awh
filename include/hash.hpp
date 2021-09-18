/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_HASH__
#define __AWH_HASH__

/**
 * Стандартная библиотека
 */
#include <ctime>
#include <string>
#include <vector>
#include <cstring>
#include <zlib.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * Подключаем OpenSSL
 */
#include <openssl/md5.h>
#include <openssl/aes.h>
#include <openssl/evp.h>

/**
 * Наши модули
 */
#include <fmk.hpp>

// Параметры Zlib
#define MOD_GZIP_ZLIB_CFACTOR 9
#define MOD_GZIP_ZLIB_BSIZE 8096
#define MOD_GZIP_ZLIB_WINDOWSIZE 15

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Hash Класс хеширования данных
	 */
	typedef class Hash {
		private:
			/**
			 * State Стрейт шифрования
			 */
			mutable struct StateAES {
				// Количество обработанных байт
				int num;
				// Буфер данных для шифрования
				u_char ivec[AES_BLOCK_SIZE];
				/**
				 * StateAES Конструктор
				 */
				StateAES() : num(0) {}
			} stateAES;
		public:
			/**
			 * Набор размеров шифрования
			 */
			enum class aes_t : u_short {AES128 = 128, AES192 = 192, AES256 = 256};
		private:
			// Размер окна байтов для компрессии
			short wbit;
			// Устанавливаем количество раундов
			int roundsAES;
			// Соль и пароль для шифрования
			string salt, password;
		private:
			// Хвостовой буфер для удаления из финального сообщения
			const char btype[4] = {
				static_cast <char> (0x00),
				static_cast <char> (0x00),
				static_cast <char> (0xFF),
				static_cast <char> (0xFF)
			};
		private:
			// Определяем длину шифрования по умолчанию
			aes_t aesSize;
			// Ключ шифрования
			mutable AES_KEY aesKey;
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Адрес файла для сохранения логов
			const char * logfile = nullptr;
		private:
			// Размер чанка в байтах
			static constexpr u_int CHUNK_SIZE = 0x4000;
			// Устанавливаем уровень сжатия
			static constexpr u_short DEFAULT_MEM_LEVEL = 4;
			// Устанавливаем размер чанка
			static constexpr u_int ZLIB_CHUNK_SIZE = 0x4000;
		private:
			/**
			 * initAES Метод инициализации AES шифрования
			 * @return результат инициализации
			 */
			const bool initAES() const;
		public:
			/**
			 * rmTail Метод удаления хвостовых данных
			 * @param buffer буфер для удаления хвоста
			 */
			void rmTail(vector <char> & buffer) const noexcept;
			/**
			 * setTail Метод добавления хвостовых данных
			 * @param buffer буфер для добавления хвоста
			 */
			void setTail(vector <char> & buffer) const noexcept;
		public:
			/**
			 * encrypt Метод шифрования текста
			 * @param buffer буфер данных для шифрования
			 * @param size   размер данных для шифрования
			 * @return       результат шифрования
			 */
			const vector <char> encrypt(const char * buffer, const size_t size) const noexcept;
			/**
			 * decrypt Метод дешифрования текста
			 * @param buffer буфер данных для дешифрования
			 * @param size   размер данных для дешифрования
			 * @return       результат дешифрования
			 */
			const vector <char> decrypt(const char * buffer, const size_t size) const noexcept;
		public:
			/**
			 * compress Метод компрессии данных
			 * @param buffer буфер данных для компрессии
			 * @param size   размер данных для компрессии
			 * @return       результат компрессии
			 */
			const vector <char> compress(const char * buffer, const size_t size) const noexcept;
			/**
			 * decompress Метод декомпрессии данных
			 * @param buffer буфер данных для декомпрессии
			 * @param size   размер данных для декомпрессии
			 * @return       результат декомпрессии
			 */
			const vector <char> decompress(const char * buffer, const size_t size) const noexcept;
		public:
			/**
			 * setAES Метод установки размера шифрования
			 * @param size размер шифрования (128, 192, 256)
			 */
			void setAES(const aes_t size) noexcept;
			/**
			 * setWbit Метод установки размера окна байтов для компрессии
			 * @param wbit размер окна байтов для компрессии
			 */
			void setWbit(const short wbit) noexcept;
			/**
			 * setRoundAES Метод установки количества раундов шифрования
			 * @param round количество раундов шифрования
			 */
			void setRoundAES(const int round) noexcept;
			/**
			 * setSalt Метод установки соли шифрования
			 * @param salt соль для шифрования
			 */
			void setSalt(const string & salt) noexcept;
			/**
			 * setPassword Метод установки пароля шифрования
			 * @param password пароль шифрования
			 */
			void setPassword(const string & password) noexcept;
		public:
			/**
			 * Hash Конструктор
			 * @param fmk     объект фреймворка
			 * @param logfile адрес файла для сохранения логов
			 */
			Hash(const fmk_t * fmk, const char * logfile = nullptr) : fmk(fmk), logfile(logfile), wbit(MAX_WBITS), roundsAES(5), aesSize(aes_t::AES128), salt(""), password("") {}
	} hash_t;
};

#endif // __AWH_HASH__
