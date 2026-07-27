/**
 * @file: crypto.cpp
 * @date: 2026-07-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация криптографического слоя QUIC (RFC 9001) — вывод начальных секретов,
 *        ключей пакетов и заголовочной защиты, шифрование и расшифровка пакетов на AEAD-примитивах BoringSSL
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <new>
#include <cstring>
#include <utility>

/**
 * Заголовочные файлы BoringSSL
 */
#include <openssl/aes.h>
#include <openssl/mem.h>
#include <openssl/aead.h>
#include <openssl/hkdf.h>
#include <openssl/chacha.h>
#include <openssl/digest.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <proto/quic2/crypto.hpp>
#include <proto/quic2/packet.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние вспомогательные функции и константы слоя защиты пакетов
 *
 */
namespace {
	/**
	 * Используем пространство имён протокола QUIC
	 */
	using namespace awh::quic2;

	/**
	 * @brief Соль вывода ключей Initial для QUIC v1 (RFC 9001 §5.2)
	 *
	 */
	static constexpr uint8_t INITIAL_SALT[20] = {
		0x38, 0x76, 0x2C, 0xF7, 0xF5, 0x59, 0x34, 0xB3, 0x4D, 0x17,
		0x9A, 0xE6, 0xA4, 0xC8, 0x0C, 0xAD, 0xCC, 0xBB, 0x7F, 0x0A
	};
	/**
	 * @brief Ключ AEAD тега целостности пакета Retry для QUIC v1 (RFC 9001 §5.8)
	 *
	 */
	static constexpr uint8_t RETRY_KEY[16] = {
		0xBE, 0x0C, 0x69, 0x0B, 0x9F, 0x66, 0x57, 0x5A,
		0x1D, 0x76, 0x6B, 0x54, 0xE3, 0x68, 0xC8, 0x4E
	};
	/**
	 * @brief Нонс AEAD тега целостности пакета Retry для QUIC v1 (RFC 9001 §5.8)
	 *
	 */
	static constexpr uint8_t RETRY_NONCE[12] = {
		0x46, 0x15, 0x99, 0xD3, 0x5D, 0x63, 0x2B, 0xF2,
		0x23, 0x98, 0x25, 0xBB
	};

	/**
	 * @brief Функция получения хеш-функции криптографического набора
	 *
	 * @param suite криптографический набор защиты пакетов
	 * @return      хеш-функция набора
	 *
	 */
	inline const EVP_MD * digest(const crypto::suite_t suite) noexcept {
		// Набор AES_256_GCM_SHA384 использует SHA-384, остальные - SHA-256
		return ((suite == crypto::suite_t::AES_256_GCM_SHA384) ? EVP_sha384() : EVP_sha256());
	}
	/**
	 * @brief Функция получения AEAD-алгоритма криптографического набора
	 *
	 * @param suite криптографический набор защиты пакетов
	 * @return      AEAD-алгоритм набора
	 *
	 */
	inline const EVP_AEAD * aead(const crypto::suite_t suite) noexcept {
		/**
		 * Определяем криптографический набор
		 */
		switch(static_cast <uint8_t> (suite)){
			// Набор AES-256-GCM
			case static_cast <uint8_t> (crypto::suite_t::AES_256_GCM_SHA384):
				// Выводим AEAD-алгоритм набора
				return EVP_aead_aes_256_gcm();
			// Набор ChaCha20-Poly1305
			case static_cast <uint8_t> (crypto::suite_t::CHACHA20_POLY1305_SHA256):
				// Выводим AEAD-алгоритм набора
				return EVP_aead_chacha20_poly1305();
		}
		// Набор AES-128-GCM
		return EVP_aead_aes_128_gcm();
	}
	/**
	 * @brief Функция получения длины ключа AEAD криптографического набора
	 *
	 * @param suite криптографический набор защиты пакетов
	 * @return      длина ключа AEAD в октетах
	 *
	 */
	inline size_t keySize(const crypto::suite_t suite) noexcept {
		// Набор AES_128_GCM использует 16-октетный ключ, остальные - 32-октетный
		return ((suite == crypto::suite_t::AES_128_GCM_SHA256) ? 16 : 32);
	}
	/**
	 * @brief Функция освобождения AEAD-контекста
	 *
	 * @param ctx освобождаемый AEAD-контекст
	 *
	 */
	inline void freeAead(EVP_AEAD_CTX * ctx) noexcept {
		// Если контекст создан
		if(ctx != nullptr)
			// Освобождаем AEAD-контекст
			EVP_AEAD_CTX_free(ctx);
	}
	/**
	 * @brief Функция освобождения развёрнутого ключа защиты заголовка
	 *
	 * @param key освобождаемый развёрнутый ключ AES
	 *
	 * @note Затирает расписание раундовых ключей до освобождения памяти: AES_KEY -
	 *       обычный POD, освобождаемый штатным delete без затирания, а из расписания
	 *       раундов тривиально восстанавливается исходный ключ защиты заголовка
	 *       (RFC 9001 §6, defense-in-depth)
	 *
	 */
	inline void freeMask(aes_key_st * key) noexcept {
		// Если развёрнутый ключ создан
		if(key != nullptr){
			// Затираем расписание раундовых ключей в памяти
			OPENSSL_cleanse(key, sizeof(AES_KEY));
			// Освобождаем развёрнутый ключ
			delete key;
		}
	}
	/**
	 * @brief Функция формирования нонса AEAD из вектора инициализации и номера пакета (RFC 9001 §5.3)
	 *
	 * @param iv    вектор инициализации направления (12 октетов)
	 * @param pn    полный номер пакета
	 * @param nonce сформированный нонс AEAD (12 октетов)
	 *
	 */
	inline void makeNonce(const string & iv, const uint64_t pn, uint8_t nonce[12]) noexcept {
		// Копируем вектор инициализации в нонс
		::memcpy(nonce, iv.data(), 12);
		/**
		 * Накладываем номер пакета на младшие октеты нонса операцией XOR
		 */
		for(size_t i = 0; i < 8; i++)
			// Накладываем очередной октет номера пакета
			nonce[11 - i] ^= static_cast <uint8_t> ((pn >> (i * 8)) & 0xFF);
	}
	/**
	 * @brief Функция вычисления тега целостности пакета Retry по псевдопакету (RFC 9001 §5.8)
	 *
	 * @param odcid  DCID первого пакета Initial клиента
	 * @param packet пакет Retry без тега целостности
	 * @param tag    вычисленный тег целостности (16 октетов)
	 * @return       результат вычисления (false - ошибка криптографической библиотеки)
	 *
	 */
	bool computeRetryTag(const cid_t & odcid, string_view packet, uint8_t tag[proto::RETRY_TAG_SIZE]) noexcept {
		// Формируем псевдопакет Retry: октет длины ODCID, ODCID и пакет без тега
		string pseudo;
		// Резервируем память под псевдопакет
		pseudo.reserve(1 + odcid.size + packet.size());
		// Дописываем октет длины ODCID
		pseudo.push_back(static_cast <char> (odcid.size));
		// Дописываем данные ODCID
		pseudo.append(reinterpret_cast <const char *> (odcid.data), odcid.size);
		// Дописываем пакет Retry без тега целостности
		pseudo.append(packet);
		// Создаём AEAD-контекст с фиксированным ключом Retry
		EVP_AEAD_CTX * ctx = EVP_AEAD_CTX_new(EVP_aead_aes_128_gcm(), RETRY_KEY, sizeof(RETRY_KEY), EVP_AEAD_DEFAULT_TAG_LENGTH);
		// Если контекст не создан
		if(ctx == nullptr)
			// Вычисление невозможно
			return false;
		// Длина результата шифрования
		size_t length = 0;
		// Шифруем пустую нагрузку с псевдопакетом в качестве дополнительных данных
		const bool result = (EVP_AEAD_CTX_seal(
			ctx, tag, &length, proto::RETRY_TAG_SIZE,
			RETRY_NONCE, sizeof(RETRY_NONCE), nullptr, 0,
			reinterpret_cast <const uint8_t *> (pseudo.data()), pseudo.size()
		) == 1);
		// Освобождаем AEAD-контекст
		EVP_AEAD_CTX_free(ctx);
		// Выводим результат вычисления с проверкой длины тега
		return (result && (length == proto::RETRY_TAG_SIZE));
	}
};

/**
 * @brief Конструктор
 *
 */
awh::quic2::crypto::Keys::Keys() noexcept : suite(suite_t::AES_128_GCM_SHA256) {}
/**
 * @brief Функция затирания ключевого материала набора в памяти
 *
 * @note Затирает секрет и выведенные из него ключ, вектор инициализации и ключ
 *       защиты заголовка до освобождения либо перезаписи их буферов: ключевой
 *       материал не должен оставаться в куче после разрыва соединения, смены фазы
 *       или сброса уровня (RFC 9001 §6, defense-in-depth). Ключ развёрнутого
 *       контекста AEAD затирается криптографической библиотекой при освобождении
 *       контекста, а расписание раундов ключа защиты заголовка - затирающим
 *       делетером freeMask при освобождении shared_ptr
 *
 * @param keys затираемый набор ключей
 *
 */
static void wipeKeys(awh::quic2::crypto::keys_t & keys) noexcept {
	if(!keys.secret.empty())
		// Затираем секрет направления
		OPENSSL_cleanse(&keys.secret[0], keys.secret.size());
	if(!keys.key.empty())
		// Затираем ключ AEAD-шифрования нагрузки
		OPENSSL_cleanse(&keys.key[0], keys.key.size());
	if(!keys.iv.empty())
		// Затираем вектор инициализации AEAD
		OPENSSL_cleanse(&keys.iv[0], keys.iv.size());
	if(!keys.hp.empty())
		// Затираем ключ защиты заголовка
		OPENSSL_cleanse(&keys.hp[0], keys.hp.size());
}
/**
 * @brief Оператор копирующего присваивания
 *
 * @param keys копируемый набор ключей
 * @return     набор ключей текущего объекта
 *
 */
awh::quic2::crypto::Keys & awh::quic2::crypto::Keys::operator = (const Keys & keys) noexcept {
	// Если присваивание выполняется не самому себе
	if(this != &keys){
		// Затираем прежний ключевой материал до его перезаписи
		wipeKeys(* this);
		// Копируем криптографический набор
		this->suite = keys.suite;
		// Копируем секрет и выведенные ключи
		this->secret = keys.secret;
		this->key = keys.key;
		this->iv = keys.iv;
		this->hp = keys.hp;
		// Копируем разделяемые контексты защиты пакетов
		this->aead = keys.aead;
		this->mask = keys.mask;
	}
	// Выводим текущий объект
	return * this;
}
/**
 * @brief Оператор перемещающего присваивания
 *
 * @param keys перемещаемый набор ключей
 * @return     набор ключей текущего объекта
 *
 */
awh::quic2::crypto::Keys & awh::quic2::crypto::Keys::operator = (Keys && keys) noexcept {
	// Если присваивание выполняется не самому себе
	if(this != &keys){
		// Затираем прежний ключевой материал до его перезаписи
		wipeKeys(* this);
		// Копируем криптографический набор
		this->suite = keys.suite;
		// Перемещаем секрет и выведенные ключи
		this->secret = std::move(keys.secret);
		this->key = std::move(keys.key);
		this->iv = std::move(keys.iv);
		this->hp = std::move(keys.hp);
		// Перемещаем разделяемые контексты защиты пакетов
		this->aead = std::move(keys.aead);
		this->mask = std::move(keys.mask);
	}
	// Выводим текущий объект
	return * this;
}
/**
 * @brief Деструктор
 *
 */
awh::quic2::crypto::Keys::~Keys() noexcept {
	// Затираем ключевой материал набора до освобождения его буферов
	wipeKeys(* this);
}

/**
 * @brief Функция вывода ключей HKDF-Expand-Label (RFC 8446 §7.1 / RFC 9001 §5.1)
 *
 * @param suite  криптографический набор (определяет хеш-функцию)
 * @param secret секрет-источник вывода
 * @param label  метка вывода (без префикса "tls13 ")
 * @param length требуемая длина выводимого материала
 * @param output выводимый ключевой материал
 * @return       результат вывода (false - ошибка криптографической библиотеки)
 *
 */
bool awh::quic2::crypto::hkdfExpandLabel(const suite_t suite, string_view secret, string_view label, const size_t length, string & output) noexcept {
	// Формируем структуру HkdfLabel (RFC 8446 §7.1)
	string info;
	// Дописываем старший октет требуемой длины
	info.push_back(static_cast <char> ((length >> 8) & 0xFF));
	// Дописываем младший октет требуемой длины
	info.push_back(static_cast <char> (length & 0xFF));
	// Дописываем длину метки с префиксом "tls13 "
	info.push_back(static_cast <char> (6 + label.size()));
	// Дописываем префикс метки
	info.append("tls13 ");
	// Дописываем метку вывода
	info.append(label);
	// Дописываем нулевую длину контекста
	info.push_back('\0');
	// Выделяем память под выводимый материал
	output.resize(length);
	// Выполняем вывод ключевого материала HKDF-Expand
	return (HKDF_expand(
		reinterpret_cast <uint8_t *> (&output[0]), length, digest(suite),
		reinterpret_cast <const uint8_t *> (secret.data()), secret.size(),
		reinterpret_cast <const uint8_t *> (info.data()), info.size()
	) == 1);
}
/**
 * @brief Функция вывода ключей защиты пакетов из секрета направления (RFC 9001 §5.1)
 *
 * @param keys ключи защиты пакетов (заполняются key/iv/hp)
 * @return     результат вывода (false - ошибка криптографической библиотеки)
 *
 */
bool awh::quic2::crypto::derive(keys_t & keys) noexcept {
	// Длина ключа AEAD криптографического набора
	const size_t length = keySize(keys.suite);
	// Сбрасываем кэш контекстов предыдущего ключевого материала
	keys.aead.reset();
	// Сбрасываем кэш развёрнутого ключа защиты заголовка
	keys.mask.reset();
	// Выводим ключ AEAD-шифрования нагрузки
	if(!hkdfExpandLabel(keys.suite, keys.secret, "quic key", length, keys.key))
		// Вывод невозможен
		return false;
	// Выводим вектор инициализации AEAD
	if(!hkdfExpandLabel(keys.suite, keys.secret, "quic iv", 12, keys.iv))
		// Вывод невозможен
		return false;
	// Выводим ключ защиты заголовка
	if(!hkdfExpandLabel(keys.suite, keys.secret, "quic hp", length, keys.hp))
		// Вывод невозможен
		return false;
	// Создаём AEAD-контекст один раз на время жизни ключей (RFC 9001 §5.3)
	EVP_AEAD_CTX * ctx = EVP_AEAD_CTX_new(aead(keys.suite), reinterpret_cast <const uint8_t *> (keys.key.data()), keys.key.size(), EVP_AEAD_DEFAULT_TAG_LENGTH);
	// Если AEAD-контекст не создан
	if(ctx == nullptr)
		// Вывод невозможен
		return false;
	// Передаём AEAD-контекст во владение ключам направления
	keys.aead = shared_ptr <evp_aead_ctx_st> (ctx, &freeAead);
	// Если набор использует ChaCha20 - развёртка ключа защиты заголовка не требуется (RFC 9001 §5.4.4)
	if(keys.suite == suite_t::CHACHA20_POLY1305_SHA256)
		// Вывод ключей выполнен успешно
		return true;
	// Создаём развёрнутый ключ защиты заголовка с затирающим делетером
	shared_ptr <aes_key_st> key(new (std::nothrow) AES_KEY(), &freeMask);
	// Если память под развёрнутый ключ не выделена
	if(!key)
		// Вывод невозможен
		return false;
	// Выполняем развёртку ключа защиты заголовка (RFC 9001 §5.4.3)
	if(AES_set_encrypt_key(reinterpret_cast <const uint8_t *> (keys.hp.data()), static_cast <uint32_t> (keys.hp.size() * 8), key.get()) != 0)
		// Вывод невозможен
		return false;
	// Передаём развёрнутый ключ во владение ключам направления
	keys.mask = ::move(key);
	// Вывод ключей выполнен успешно
	return true;
}
/**
 * @brief Функция вывода ключей Initial обоих направлений (RFC 9001 §5.2)
 *
 * @param dcid   идентификатор соединения получателя первого пакета Initial клиента
 * @param client ключи защиты пакетов клиента
 * @param server ключи защиты пакетов сервера
 * @return       результат вывода (false - ошибка криптографической библиотеки)
 *
 */
bool awh::quic2::crypto::initial(const cid_t & dcid, keys_t & client, keys_t & server) noexcept {
	// Общий секрет Initial (размер выхода SHA-256)
	uint8_t secret[32];
	// Длина общего секрета
	size_t length = sizeof(secret);
	// Выполняем извлечение общего секрета HKDF-Extract с солью QUIC v1
	if(HKDF_extract(secret, &length, EVP_sha256(), dcid.data, dcid.size, INITIAL_SALT, sizeof(INITIAL_SALT)) != 1)
		// Вывод невозможен
		return false;
	// Представление общего секрета для вывода ключей
	const string_view initialSecret(reinterpret_cast <const char *> (secret), length);
	// Пакеты Initial всегда защищаются набором AES_128_GCM_SHA256 (RFC 9001 §5.2)
	client.suite = suite_t::AES_128_GCM_SHA256;
	server.suite = suite_t::AES_128_GCM_SHA256;
	// Результат вывода секретов и ключей обоих направлений
	const bool result = (
		hkdfExpandLabel(client.suite, initialSecret, "client in", 32, client.secret) &&
		hkdfExpandLabel(server.suite, initialSecret, "server in", 32, server.secret) &&
		derive(client) && derive(server)
	);
	// Затираем общий секрет Initial на стеке (RFC 9001 §6, defense-in-depth)
	OPENSSL_cleanse(secret, sizeof(secret));
	// Выводим результат вывода ключей
	return result;
}
/**
 * @brief Функция обновления ключей на новую фазу (RFC 9001 §6)
 *
 * @param current текущие ключи защиты пакетов
 * @param next    ключи защиты пакетов следующей фазы
 * @return        результат обновления (false - ошибка криптографической библиотеки)
 *
 */
bool awh::quic2::crypto::update(const keys_t & current, keys_t & next) noexcept {
	// Криптографический набор не меняется
	next.suite = current.suite;
	// Длина секрета соответствует размеру выхода хеш-функции набора
	const size_t length = static_cast <size_t> (EVP_MD_size(digest(current.suite)));
	// Выводим секрет следующей фазы меткой "quic ku"
	if(!hkdfExpandLabel(current.suite, current.secret, "quic ku", length, next.secret))
		// Обновление невозможно
		return false;
	// Выводим ключи защиты пакетов из нового секрета
	if(!derive(next))
		// Обновление невозможно
		return false;
	// Ключ защиты заголовка при обновлении не меняется (RFC 9001 §6)
	next.hp = current.hp;
	// Переносим развёрнутый ключ защиты заголовка вместе с самим ключом
	next.mask = current.mask;
	// Обновление выполнено успешно
	return true;
}
/**
 * @brief Функция вычисления маски защиты заголовка (RFC 9001 §5.4)
 *
 * @param keys   ключи защиты пакетов
 * @param sample выборка защищённой нагрузки (16 октетов)
 * @param mask   вычисленная маска защиты заголовка (5 октетов)
 * @return       результат вычисления (false - ошибка криптографической библиотеки)
 *
 */
bool awh::quic2::crypto::hpMask(const keys_t & keys, const uint8_t sample[HP_SAMPLE_SIZE], uint8_t mask[HP_MASK_SIZE]) noexcept {
	// Если набор использует ChaCha20 (RFC 9001 §5.4.4)
	if(keys.suite == suite_t::CHACHA20_POLY1305_SHA256){
		// Счётчик ChaCha20 - первые четыре октета выборки в порядке little-endian
		const uint32_t counter = (static_cast <uint32_t> (sample[0]) | (static_cast <uint32_t> (sample[1]) << 8) |
		                          (static_cast <uint32_t> (sample[2]) << 16) | (static_cast <uint32_t> (sample[3]) << 24));
		// Нулевой блок для получения гаммы шифра
		const uint8_t zeros[HP_MASK_SIZE] = {0, 0, 0, 0, 0};
		// Шифруем нулевой блок потоком ChaCha20 (нонс - остальные октеты выборки)
		CRYPTO_chacha_20(mask, zeros, HP_MASK_SIZE, reinterpret_cast <const uint8_t *> (keys.hp.data()), sample + 4, counter);
		// Вычисление выполнено успешно
		return true;
	}
	// Если развёрнутый ключ защиты заголовка не подготовлен выводом ключей
	if(!keys.mask)
		// Вычисление невозможно
		return false;
	// Блок результата шифрования AES-ECB
	uint8_t block[16];
	// Шифруем выборку одним блоком AES-ECB развёрнутым ключом (RFC 9001 §5.4.3)
	AES_encrypt(sample, block, keys.mask.get());
	// Маска - первые пять октетов зашифрованного блока
	::memcpy(mask, block, HP_MASK_SIZE);
	// Вычисление выполнено успешно
	return true;
}
/**
 * @brief Функция полной защиты пакета: AEAD-шифрование нагрузки и защита заголовка (RFC 9001 §5.3/§5.4)
 *
 * @param output  выходной буфер (дописывается защищённый пакет целиком)
 * @param keys    ключи защиты пакетов отправителя
 * @param pn      полный номер пакета (для нонса AEAD)
 * @param header  собранный незащищённый заголовок пакета (включая Packet Number)
 * @param payload незашифрованная нагрузка пакета (фреймы)
 * @return        результат защиты (false - ошибка криптографической библиотеки)
 *
 */
bool awh::quic2::crypto::seal(string & output, const keys_t & keys, const uint64_t pn, string_view header, string_view payload) noexcept {
	// Если заголовок пуст либо AEAD-контекст не подготовлен выводом ключей
	if(header.empty() || !keys.aead || (keys.iv.size() < 12))
		// Защита невозможна
		return false;
	// Размер номера пакета из битов первого октета заголовка
	const size_t pnSize = (static_cast <size_t> (static_cast <uint8_t> (header[0]) & 0x03) + 1);
	// Если заголовок короче объявленного размера номера пакета
	if(header.size() < pnSize)
		// Защита невозможна
		return false;
	// Смещение поля Packet Number от начала заголовка
	const size_t pnOffset = (header.size() - pnSize);
	/**
	 * Если выборка защиты заголовка выходит за пределы собираемого пакета (RFC 9001 §5.4.2):
	 * нагрузка обязана содержать не менее 4 - pnSize октетов сверх тега AEAD
	 */
	if((pnOffset + proto::MAX_PKT_NUM_SIZE + HP_SAMPLE_SIZE) > (header.size() + payload.size() + AEAD_TAG_SIZE))
		// Защита невозможна
		return false;
	// Нонс AEAD-шифрования
	uint8_t nonce[12];
	// Формируем нонс из вектора инициализации и номера пакета
	makeNonce(keys.iv, pn, nonce);
	// Смещение начала пакета в выходном буфере
	const size_t start = output.size();
	// Дописываем незащищённый заголовок пакета
	output.append(header);
	// Смещение зашифрованной нагрузки в выходном буфере
	const size_t sealed = output.size();
	// Выделяем память под зашифрованную нагрузку с тегом AEAD
	output.resize(sealed + payload.size() + AEAD_TAG_SIZE);
	// Длина результата шифрования
	size_t length = 0;
	// Шифруем нагрузку с заголовком в качестве дополнительных данных
	const bool result = (EVP_AEAD_CTX_seal(
		keys.aead.get(), reinterpret_cast <uint8_t *> (&output[sealed]), &length, payload.size() + AEAD_TAG_SIZE,
		nonce, sizeof(nonce), reinterpret_cast <const uint8_t *> (payload.data()), payload.size(),
		reinterpret_cast <const uint8_t *> (header.data()), header.size()
	) == 1);
	// Если шифрование не выполнено или длина результата некорректна
	if(!result || (length != (payload.size() + AEAD_TAG_SIZE))){
		// Откатываем выходной буфер к состоянию до сборки пакета
		output.resize(start);
		// Защита невозможна
		return false;
	}
	// Выборка защищённой нагрузки для защиты заголовка (RFC 9001 §5.4.2)
	const uint8_t * sample = reinterpret_cast <const uint8_t *> (output.data() + start + pnOffset + proto::MAX_PKT_NUM_SIZE);
	// Маска защиты заголовка
	uint8_t mask[HP_MASK_SIZE];
	// Вычисляем маску защиты заголовка
	if(!hpMask(keys, sample, mask)){
		// Откатываем выходной буфер к состоянию до сборки пакета
		output.resize(start);
		// Защита невозможна
		return false;
	}
	// Если пакет с длинным заголовком - маскируем четыре младших бита первого октета
	if((static_cast <uint8_t> (output[start]) & 0x80) != 0)
		// Накладываем маску на биты номера пакета длинного заголовка
		output[start] = static_cast <char> (static_cast <uint8_t> (output[start]) ^ (mask[0] & 0x0F));
	// Если пакет с коротким заголовком - маскируем пять младших битов первого октета
	else output[start] = static_cast <char> (static_cast <uint8_t> (output[start]) ^ (mask[0] & 0x1F));
	/**
	 * Накладываем маску на октеты номера пакета
	 */
	for(size_t i = 0; i < pnSize; i++)
		// Накладываем очередной октет маски
		output[start + pnOffset + i] = static_cast <char> (static_cast <uint8_t> (output[start + pnOffset + i]) ^ mask[1 + i]);
	// Защита выполнена успешно
	return true;
}
/**
 * @brief Функция полного снятия защиты пакета: защита заголовка и AEAD-расшифровка (RFC 9001 §5.3/§5.4)
 *
 * @param packet    буфер пакета (модифицируется)
 * @param size      полный размер пакета
 * @param pnOffset  смещение поля Packet Number (граница защиты заголовка)
 * @param largestPn наибольший принятый номер пакета в пространстве номеров
 * @param keys      ключи защиты пакетов отправителя пакета
 * @param pn        восстановленный полный номер пакета
 * @param output    расшифрованная нагрузка пакета (фреймы)
 * @param error     код ошибки транспорта
 * @return          результат снятия защиты (OK/ERROR)
 *
 */
awh::quic2::status_t awh::quic2::crypto::open(uint8_t * packet, const size_t size, const size_t pnOffset, const uint64_t largestPn, const keys_t & keys, uint64_t & pn, string & output, error_t & error) noexcept {
	// Устанавливаем код ошибки транспорта на случай неудачи снятия защиты
	error = static_cast <error_t> (static_cast <uint64_t> (error_t::CRYPTO_ERROR));
	// Если AEAD-контекст не подготовлен выводом ключей
	if(!keys.aead || (keys.iv.size() < 12))
		// Снятие защиты невозможно
		return status_t::ERROR;
	// Если выборка защиты заголовка не помещается в пакет (сравнение без сложения - защита от переполнения pnOffset)
	if((pnOffset > size) || ((size - pnOffset) < (proto::MAX_PKT_NUM_SIZE + HP_SAMPLE_SIZE)))
		// Снятие защиты невозможно
		return status_t::ERROR;
	// Выборка защищённой нагрузки для защиты заголовка (RFC 9001 §5.4.2)
	const uint8_t * sample = (packet + pnOffset + proto::MAX_PKT_NUM_SIZE);
	// Маска защиты заголовка
	uint8_t mask[HP_MASK_SIZE];
	// Вычисляем маску защиты заголовка
	if(!hpMask(keys, sample, mask))
		// Снятие защиты невозможно
		return status_t::ERROR;
	// Если пакет с длинным заголовком - снимаем маску с четырёх младших битов первого октета
	if((packet[0] & 0x80) != 0)
		// Снимаем маску с битов номера пакета длинного заголовка
		packet[0] ^= (mask[0] & 0x0F);
	// Если пакет с коротким заголовком - снимаем маску с пяти младших битов первого октета
	else packet[0] ^= (mask[0] & 0x1F);
	// Размер номера пакета из расшифрованных битов первого октета
	const size_t pnSize = (static_cast <size_t> (packet[0] & 0x03) + 1);
	/**
	 * Снимаем маску с октетов номера пакета
	 */
	for(size_t i = 0; i < pnSize; i++)
		// Снимаем очередной октет маски
		packet[pnOffset + i] ^= mask[1 + i];
	// Усечённый номер пакета
	uint64_t truncated = 0;
	// Читаем усечённый номер пакета
	if(!packet::parser::packetNumber(packet + pnOffset, size - pnOffset, pnSize, truncated))
		// Снятие защиты невозможно
		return status_t::ERROR;
	// Восстанавливаем полный номер пакета
	pn = packet::decodePacketNumber(largestPn, truncated, pnSize);
	// Нонс AEAD-расшифровки
	uint8_t nonce[12];
	// Формируем нонс из вектора инициализации и номера пакета
	makeNonce(keys.iv, pn, nonce);
	// Смещение зашифрованной нагрузки
	const size_t sealed = (pnOffset + pnSize);
	// Если зашифрованная нагрузка не содержит даже тега AEAD
	if(size < (sealed + AEAD_TAG_SIZE))
		// Снятие защиты невозможно
		return status_t::ERROR;
	// Выделяем память под расшифрованную нагрузку
	output.resize(size - sealed);
	// Длина результата расшифровки
	size_t length = 0;
	// Расшифровываем нагрузку с расшифрованным заголовком в качестве дополнительных данных
	const bool result = (EVP_AEAD_CTX_open(
		keys.aead.get(), reinterpret_cast <uint8_t *> (&output[0]), &length, output.size(),
		nonce, sizeof(nonce), packet + sealed, size - sealed, packet, sealed
	) == 1);
	// Если расшифровка не выполнена (тег AEAD не сошёлся)
	if(!result)
		// Снятие защиты невозможно
		return status_t::ERROR;
	// Устанавливаем фактическую длину расшифрованной нагрузки до любых проверок
	output.resize(length);
	/**
	 * Проверяем зарезервированные биты первого октета: они проверяются только после
	 * снятия обеих защит, иначе неверные ключи давали бы ложное нарушение протокола
	 * (RFC 9000 §17.2/§17.3)
	 */
	const uint8_t reserved = ((packet[0] & 0x80) != 0 ? 0x0C : 0x18);
	// Если зарезервированные биты ненулевые
	if((packet[0] & reserved) != 0){
		// Устанавливаем код ошибки транспорта
		error = error_t::PROTOCOL_VIOLATION;
		// Снятие защиты невозможно
		return status_t::ERROR;
	}
	// Снятие защиты выполнено успешно
	return status_t::OK;
}
/**
 * @brief Функция вычисления тега целостности пакета Retry (RFC 9001 §5.8)
 *
 * @param odcid  DCID первого пакета Initial клиента
 * @param packet пакет Retry без тега целостности
 * @param tag    вычисленный тег целостности (16 октетов)
 * @return       результат вычисления (false - ошибка криптографической библиотеки)
 *
 */
bool awh::quic2::crypto::retryTag(const cid_t & odcid, string_view packet, uint8_t tag[proto::RETRY_TAG_SIZE]) noexcept {
	// Вычисляем тег целостности по псевдопакету
	return computeRetryTag(odcid, packet, tag);
}
/**
 * @brief Функция проверки тега целостности пакета Retry (RFC 9001 §5.8)
 *
 * @param odcid  DCID первого пакета Initial клиента
 * @param packet пакет Retry целиком (включая тег целостности)
 * @return       результат проверки (true - тег целостности корректен)
 *
 */
bool awh::quic2::crypto::retryVerify(const cid_t & odcid, string_view packet) noexcept {
	// Если пакет не содержит даже тега целостности
	if(packet.size() <= proto::RETRY_TAG_SIZE)
		// Проверка невозможна
		return false;
	// Ожидаемый тег целостности
	uint8_t expected[proto::RETRY_TAG_SIZE];
	// Вычисляем тег целостности по пакету без тега
	if(!computeRetryTag(odcid, packet.substr(0, packet.size() - proto::RETRY_TAG_SIZE), expected))
		// Проверка невозможна
		return false;
	// Сравниваем теги целостности за постоянное время
	return (CRYPTO_memcmp(expected, packet.data() + (packet.size() - proto::RETRY_TAG_SIZE), proto::RETRY_TAG_SIZE) == 0);
}
