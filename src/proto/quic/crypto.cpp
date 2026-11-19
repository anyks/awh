/**
 * @file crypto.cpp
 * @date 2026-07-21
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
 * @brief Реализация криптографического слоя QUIC (RFC 9001) — вывод начальных секретов,
 *        ключей пакетов и заголовочной защиты, шифрование и расшифровка пакетов на AEAD-примитивах A-NTS
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <new>
#include <memory>
#include <cstring>
#include <utility>

/**
 * Заголовочные файлы библиотеки A-NTS
 */
#include <cipher/aes.hpp>
#include <cipher/gcm.hpp>
#include <cipher/mode.hpp>
#include <cipher/chacha.hpp>
#include <cipher/chachapoly.hpp>
#include <digest/hash.hpp>
#include <digest/hkdf.hpp>
#include <sys/wipe.hpp>
#include <message/message.hpp>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <proto/quic/crypto.hpp>
#include <proto/quic/packet.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Пространство имён AWH
 *
 */
namespace awh {
	/**
	 * @brief Пространство имён транспортного протокола QUIC
	 *
	 */
	namespace quic {
		/**
		 * @brief Пространство имён слоя защиты пакетов QUIC
		 *
		 */
		namespace crypto {
			/**
			 * @brief Полное определение непрозрачного контекста AEAD-защиты пакетов
			 *
			 * @details Держит оба режима AEAD набора: GCM для AES-128/256-GCM и ChaCha20-Poly1305.
			 *          Активен один из них - по набору шифрования ключей направления. Режим запускается
			 *          один раз на время жизни ключей, нонс каждого пакета ставится перезапуском
			 *
			 */
			struct aead_t {
				// Набор шифрования (определяет используемый режим AEAD)
				suite_t suite;
				// Режим GCM (AES-128/256-GCM)
				nts::cipher::Gcm gcm;
				// Режим ChaCha20-Poly1305
				nts::cipher::Chachapoly chacha;
			};
			/**
			 * @brief Полное определение непрозрачного развёрнутого ключа защиты заголовка
			 *
			 * @details Держит блочный шифр AES с развёрнутым ключом защиты заголовка для наборов
			 *          на основе AES. У ChaCha20 развёртки ключа нет - контекст не создаётся
			 *
			 */
			struct mask_t {
				// Блочный шифр AES с развёрнутым ключом защиты заголовка
				nts::cipher::Aes aes;
			};
		};
	};
};

/**
 * @brief Внутренние вспомогательные функции и константы слоя защиты пакетов
 *
 */
namespace {
	/**
	 * Используем пространство имён протокола QUIC
	 */
	using namespace awh::quic;

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
	static nts::digest::hash_t __awh_suite_hash__(const crypto::suite_t suite) noexcept {
		// Набор AES_256_GCM_SHA384 использует SHA-384, остальные - SHA-256
		return ((suite == crypto::suite_t::AES_256_GCM_SHA384) ? nts::digest::hash_t::SHA384 : nts::digest::hash_t::SHA256);
	}
	/**
	 * @brief Функция получения длины ключа AEAD криптографического набора
	 *
	 * @param suite криптографический набор защиты пакетов
	 * @return      длина ключа AEAD в октетах
	 *
	 */
	static size_t __awh_key_size__(const crypto::suite_t suite) noexcept {
		// Набор AES_128_GCM использует 16-октетный ключ, остальные - 32-октетный
		return ((suite == crypto::suite_t::AES_128_GCM_SHA256) ? 16 : 32);
	}
	/**
	 * @brief Функция формирования нонса AEAD из вектора инициализации и номера пакета (RFC 9001 §5.3)
	 *
	 * @param iv    вектор инициализации направления (12 октетов)
	 * @param pn    полный номер пакета
	 * @param nonce сформированный нонс AEAD (12 октетов)
	 *
	 */
	static void __awh_make_nonce__(const string & iv, const uint64_t pn, uint8_t nonce[12]) noexcept {
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
	 * @brief Функция зашифрования нагрузки с тегом целостности по контексту AEAD
	 *
	 * @param aead       контекст AEAD-защиты пакетов
	 * @param nonce      нонс пакета (12 октетов)
	 * @param associated связанные данные, защищаемые лишь меткой
	 * @param source     зашифровываемая нагрузка
	 * @param result     место, в которое записывается зашифрованная нагрузка
	 * @param tag        место, в которое записывается тег целостности
	 * @return           результат зашифрования
	 *
	 */
	static bool __awh_aead_seal__(crypto::aead_t & aead, const uint8_t * nonce, const nts::view_t & associated, const nts::view_t & source, uint8_t * result, uint8_t * tag) noexcept {
		// Если набор использует ChaCha20-Poly1305
		if(aead.suite == crypto::suite_t::CHACHA20_POLY1305_SHA256){
			// Переводим режим на нонс пакета
			if(!aead.chacha.restart(nts::view_t(nonce, 12)))
				// Зашифрование невозможно
				return false;
			// Шифруем нагрузку с тегом целостности
			return aead.chacha.encrypt(associated, source, result, tag);
		}
		// Переводим режим на нонс пакета
		if(!aead.gcm.restart(nts::view_t(nonce, 12)))
			// Зашифрование невозможно
			return false;
		// Шифруем нагрузку с тегом целостности
		return aead.gcm.encrypt(associated, source, result, tag);
	}
	/**
	 * @brief Функция расшифрования нагрузки с проверкой тега целостности по контексту AEAD
	 *
	 * @param aead       контекст AEAD-защиты пакетов
	 * @param nonce      нонс пакета (12 октетов)
	 * @param associated связанные данные, защищаемые лишь меткой
	 * @param source     расшифровываемая нагрузка
	 * @param result     место, в которое записывается расшифрованная нагрузка
	 * @param tag        проверяемый тег целостности
	 * @return           результат расшифрования
	 *
	 */
	static bool __awh_aead_open__(crypto::aead_t & aead, const uint8_t * nonce, const nts::view_t & associated, const nts::view_t & source, uint8_t * result, const nts::view_t & tag) noexcept {
		// Если набор использует ChaCha20-Poly1305
		if(aead.suite == crypto::suite_t::CHACHA20_POLY1305_SHA256){
			// Переводим режим на нонс пакета
			if(!aead.chacha.restart(nts::view_t(nonce, 12)))
				// Расшифрование невозможно
				return false;
			// Расшифровываем нагрузку с проверкой тега целостности
			return aead.chacha.decrypt(associated, source, result, tag);
		}
		// Переводим режим на нонс пакета
		if(!aead.gcm.restart(nts::view_t(nonce, 12)))
			// Расшифрование невозможно
			return false;
		// Расшифровываем нагрузку с проверкой тега целостности
		return aead.gcm.decrypt(associated, source, result, tag);
	}
	/**
	 * @brief Функция вычисления тега целостности пакета Retry по псевдопакету (RFC 9001 §5.8)
	 *
	 * @param odcid  DCID первого пакета Initial клиента
	 * @param packet пакет Retry без тега целостности
	 * @param tag    вычисленный тег целостности (16 октетов)
	 * @return       результат вычисления
	 *
	 */
	static bool __awh_retry_tag__(const cid_t & odcid, string_view packet, uint8_t tag[proto::RETRY_TAG_SIZE]) noexcept {
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
		// Блочный шифр AES с фиксированным ключом Retry
		nts::cipher::Aes aes;
		// Режим GCM тега целостности
		nts::cipher::Gcm gcm;
		// Устанавливаем ключ Retry блочному шифру и запускаем режим
		if(!aes.key(nts::view_t(RETRY_KEY, sizeof(RETRY_KEY))) || !gcm.start(nts::cipher::block_t(aes), nts::view_t(RETRY_NONCE, sizeof(RETRY_NONCE)), proto::RETRY_TAG_SIZE))
			// Вычисление невозможно
			return false;
		// Вычисляем тег целостности по пустой нагрузке с псевдопакетом в качестве связанных данных
		return gcm.encrypt(nts::view_t(reinterpret_cast <const uint8_t *> (pseudo.data()), pseudo.size()), nts::view_t(nullptr, 0), nullptr, tag);
	}
	/**
	 * @brief Функция затирания ключевого материала набора в памяти
	 *
	 * @param keys затираемый набор ключей
	 *
	 */
	static void __awh_wipe_keys__(crypto::keys_t & keys) noexcept {
		// Затираем секрет направления
		nts::sys::wipe(keys.secret);
		// Затираем ключ AEAD-шифрования нагрузки
		nts::sys::wipe(keys.key);
		// Затираем вектор инициализации AEAD
		nts::sys::wipe(keys.iv);
		// Затираем ключ защиты заголовка
		nts::sys::wipe(keys.hp);
	}
};

/**
 * @brief Конструктор
 *
 */
awh::quic::crypto::Keys::Keys() noexcept : suite(suite_t::AES_128_GCM_SHA256) {}
/**
 * @brief Оператор копирующего присваивания
 *
 * @param keys копируемый набор ключей
 * @return     набор ключей текущего объекта
 *
 */
awh::quic::crypto::Keys & awh::quic::crypto::Keys::operator = (const Keys & keys) noexcept {
	// Если присваивание выполняется не самому себе
	if(this != &keys){
		// Затираем прежний ключевой материал до его перезаписи
		::__awh_wipe_keys__(* this);
		// Копируем криптографический набор
		this->suite = keys.suite;
		// Копируем секрет направления
		this->secret = keys.secret;
		// Копируем ключ AEAD-шифрования нагрузки
		this->key = keys.key;
		// Копируем вектор инициализации AEAD
		this->iv = keys.iv;
		// Копируем ключ защиты заголовка
		this->hp = keys.hp;
		// Копируем контекст AEAD-защиты пакетов
		this->aead = keys.aead;
		// Копируем развёрнутый ключ защиты заголовка
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
awh::quic::crypto::Keys & awh::quic::crypto::Keys::operator = (Keys && keys) noexcept {
	// Если присваивание выполняется не самому себе
	if(this != &keys){
		// Затираем прежний ключевой материал до его перезаписи
		::__awh_wipe_keys__(* this);
		// Копируем криптографический набор
		this->suite = keys.suite;
		// Перемещаем секрет направления
		this->secret = std::move(keys.secret);
		// Перемещаем ключ AEAD-шифрования нагрузки
		this->key = std::move(keys.key);
		// Перемещаем вектор инициализации AEAD
		this->iv = std::move(keys.iv);
		// Перемещаем ключ защиты заголовка
		this->hp = std::move(keys.hp);
		// Перемещаем контекст AEAD-защиты пакетов
		this->aead = std::move(keys.aead);
		// Перемещаем развёрнутый ключ защиты заголовка
		this->mask = std::move(keys.mask);
	}
	// Выводим текущий объект
	return * this;
}
/**
 * @brief Деструктор
 *
 */
awh::quic::crypto::Keys::~Keys() noexcept {
	// Затираем ключевой материал набора до освобождения его буферов
	::__awh_wipe_keys__(* this);
}

/**
 * @brief Функция вывода ключей HKDF-Expand-Label (RFC 8446 §7.1 / RFC 9001 §5.1)
 *
 * @param suite  криптографический набор (определяет хеш-функцию)
 * @param secret секрет-источник вывода
 * @param label  метка вывода (без префикса "tls13 ")
 * @param length требуемая длина выводимого материала
 * @param output выводимый ключевой материал
 * @return       результат вывода
 *
 */
bool awh::quic::crypto::hkdfExpandLabel(const suite_t suite, string_view secret, string_view label, const size_t length, string & output) noexcept {
	// Выделяем память под выводимый материал
	output.resize(length);
	// Выводим ключевой материал HKDF-Expand-Label
	return nts::digest::label(
		::__awh_suite_hash__(suite),
		nts::view_t(reinterpret_cast <const uint8_t *> (secret.data()), secret.size()),
		nts::view_t(reinterpret_cast <const uint8_t *> (label.data()), label.size()),
		nts::view_t(nullptr, 0),
		reinterpret_cast <uint8_t *> (&output[0]), length
	);
}
/**
 * @brief Функция вывода ключей защиты пакетов из секрета направления (RFC 9001 §5.1)
 *
 * @param keys ключи защиты пакетов (заполняются key/iv/hp)
 * @return     результат вывода
 *
 */
bool awh::quic::crypto::derive(keys_t & keys) noexcept {
	// Длина ключа AEAD криптографического набора
	const size_t length = ::__awh_key_size__(keys.suite);
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
	// Создаём разделяемый контекст AEAD один раз на время жизни ключей (RFC 9001 §5.3)
	auto aead = ::make_shared <aead_t> ();
	// Устанавливаем набор шифрования контекста
	aead->suite = keys.suite;
	// Нулевой нонс для первичного запуска режима (нонс пакета ставится перезапуском)
	const uint8_t zero[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	// Если набор использует ChaCha20-Poly1305
	if(keys.suite == suite_t::CHACHA20_POLY1305_SHA256){
		// Запускаем режим ChaCha20-Poly1305
		if(!aead->chacha.start(nts::view_t(reinterpret_cast <const uint8_t *> (keys.key.data()), keys.key.size()), nts::view_t(zero, 12)))
			// Вывод невозможен
			return false;
	// Если набор использует AES-GCM
	} else {
		// Блочный шифр AES с ключом AEAD
		nts::cipher::Aes aes;
		// Устанавливаем ключ AEAD блочному шифру
		if(!aes.key(nts::view_t(reinterpret_cast <const uint8_t *> (keys.key.data()), keys.key.size())))
			// Вывод невозможен
			return false;
		// Запускаем режим GCM (строит таблицы, нонс пакета ставится перезапуском)
		if(!aead->gcm.start(nts::cipher::block_t(aes), nts::view_t(zero, 12), AEAD_TAG_SIZE))
			// Вывод невозможен
			return false;
	}
	// Передаём AEAD-контекст во владение ключам направления
	keys.aead = ::move(aead);
	// Если набор использует ChaCha20 - развёртка ключа защиты заголовка не требуется (RFC 9001 §5.4.4)
	if(keys.suite == suite_t::CHACHA20_POLY1305_SHA256)
		// Вывод ключей выполнен успешно
		return true;
	// Создаём разделяемый развёрнутый ключ защиты заголовка
	auto mask = ::make_shared <mask_t> ();
	// Устанавливаем ключ защиты заголовка блочному шифру AES
	if(!mask->aes.key(nts::view_t(reinterpret_cast <const uint8_t *> (keys.hp.data()), keys.hp.size())))
		// Вывод невозможен
		return false;
	// Передаём развёрнутый ключ во владение ключам направления
	keys.mask = ::move(mask);
	// Вывод ключей выполнен успешно
	return true;
}
/**
 * @brief Функция вывода ключей Initial обоих направлений (RFC 9001 §5.2)
 *
 * @param dcid   идентификатор соединения получателя первого пакета Initial клиента
 * @param client ключи защиты пакетов клиента
 * @param server ключи защиты пакетов сервера
 * @return       результат вывода
 *
 */
bool awh::quic::crypto::initial(const cid_t & dcid, keys_t & client, keys_t & server) noexcept {
	// Общий секрет Initial (размер выхода SHA-256)
	uint8_t secret[32];
	// Извлекаем общий секрет HKDF-Extract с солью QUIC v1
	if(!nts::digest::extract(nts::digest::hash_t::SHA256, nts::view_t(INITIAL_SALT, sizeof(INITIAL_SALT)), nts::view_t(dcid.data, dcid.size), secret, sizeof(secret)))
		// Вывод невозможен
		return false;
	// Представление общего секрета для вывода ключей
	const string_view initialSecret(reinterpret_cast <const char *> (secret), sizeof(secret));
	// Пакеты Initial всегда защищаются набором AES_128_GCM_SHA256 (RFC 9001 §5.2)
	client.suite = suite_t::AES_128_GCM_SHA256;
	// Пакеты Initial всегда защищаются набором AES_128_GCM_SHA256 (RFC 9001 §5.2)
	server.suite = suite_t::AES_128_GCM_SHA256;
	// Результат вывода секретов и ключей обоих направлений
	const bool result = (
		hkdfExpandLabel(client.suite, initialSecret, "client in", 32, client.secret) &&
		hkdfExpandLabel(server.suite, initialSecret, "server in", 32, server.secret) &&
		derive(client) && derive(server)
	);
	// Затираем общий секрет Initial на стеке (RFC 9001 §6, defense-in-depth)
	nts::sys::wipe(secret, sizeof(secret));
	// Выводим результат вывода ключей
	return result;
}
/**
 * @brief Функция обновления ключей на новую фазу (RFC 9001 §6)
 *
 * @param current текущие ключи защиты пакетов
 * @param next    ключи защиты пакетов следующей фазы
 * @return        результат обновления
 *
 */
bool awh::quic::crypto::update(const keys_t & current, keys_t & next) noexcept {
	// Криптографический набор не меняется
	next.suite = current.suite;
	// Длина секрета соответствует размеру выхода хеш-функции набора
	const size_t length = nts::digest::length(::__awh_suite_hash__(current.suite));
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
 * @return       результат вычисления
 *
 */
bool awh::quic::crypto::hpMask(const keys_t & keys, const uint8_t sample[HP_SAMPLE_SIZE], uint8_t mask[HP_MASK_SIZE]) noexcept {
	// Если набор использует ChaCha20 (RFC 9001 §5.4.4)
	if(keys.suite == suite_t::CHACHA20_POLY1305_SHA256){
		// Счётчик ChaCha20 - первые четыре октета выборки в порядке little-endian
		const uint32_t counter = (static_cast <uint32_t> (sample[0]) | (static_cast <uint32_t> (sample[1]) << 8) |
		                          (static_cast <uint32_t> (sample[2]) << 16) | (static_cast <uint32_t> (sample[3]) << 24));
		// Нулевой блок для получения гаммы шифра
		const uint8_t zeros[HP_MASK_SIZE] = {0, 0, 0, 0, 0};
		// Поточный шифр ChaCha20
		nts::cipher::Chacha chacha;
		// Запускаем поток ключом защиты заголовка и нонсом из остатка выборки
		if(!chacha.start(nts::view_t(reinterpret_cast <const uint8_t *> (keys.hp.data()), keys.hp.size()), nts::view_t(sample + 4, 12), counter))
			// Вычисление невозможно
			return false;
		// Шифруем нулевой блок потоком
		return chacha.encrypt(nts::view_t(zeros, HP_MASK_SIZE), mask);
	}
	// Если развёрнутый ключ защиты заголовка не подготовлен выводом ключей
	if(!keys.mask)
		// Вычисление невозможно
		return false;
	// Блок результата шифрования AES-ECB
	uint8_t block[16];
	// Шифруем выборку одним блоком AES-ECB развёрнутым ключом (RFC 9001 §5.4.3)
	if(!keys.mask->aes.encrypt(sample, block))
		// Вычисление невозможно
		return false;
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
 * @return        результат защиты
 *
 */
bool awh::quic::crypto::seal(string & output, const keys_t & keys, const uint64_t pn, string_view header, string_view payload) noexcept {
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
	::__awh_make_nonce__(keys.iv, pn, nonce);
	// Смещение начала пакета в выходном буфере
	const size_t start = output.size();
	// Дописываем незащищённый заголовок пакета
	output.append(header);
	// Смещение зашифрованной нагрузки в выходном буфере
	const size_t sealed = output.size();
	// Выделяем память под зашифрованную нагрузку с тегом AEAD
	output.resize(sealed + payload.size() + AEAD_TAG_SIZE);
	// Шифруем нагрузку с заголовком в качестве связанных данных
	const bool result = ::__awh_aead_seal__(
		* keys.aead, nonce,
		nts::view_t(reinterpret_cast <const uint8_t *> (header.data()), header.size()),
		nts::view_t(reinterpret_cast <const uint8_t *> (payload.data()), payload.size()),
		reinterpret_cast <uint8_t *> (&output[sealed]),
		reinterpret_cast <uint8_t *> (&output[sealed + payload.size()])
	);
	// Если шифрование не выполнено
	if(!result){
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
awh::quic::status_t awh::quic::crypto::open(uint8_t * packet, const size_t size, const size_t pnOffset, const uint64_t largestPn, const keys_t & keys, uint64_t & pn, string & output, error_t & error) noexcept {
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
	::__awh_make_nonce__(keys.iv, pn, nonce);
	// Смещение зашифрованной нагрузки
	const size_t sealed = (pnOffset + pnSize);
	// Если зашифрованная нагрузка не содержит даже тега AEAD
	if(size < (sealed + AEAD_TAG_SIZE))
		// Снятие защиты невозможно
		return status_t::ERROR;
	// Длина расшифрованной нагрузки
	const size_t length = (size - sealed - AEAD_TAG_SIZE);
	// Выделяем память под расшифрованную нагрузку
	output.resize(length);
	// Расшифровываем нагрузку с расшифрованным заголовком в качестве связанных данных
	const bool result = ::__awh_aead_open__(
		* keys.aead, nonce,
		nts::view_t(packet, sealed),
		nts::view_t(packet + sealed, length),
		reinterpret_cast <uint8_t *> (&output[0]),
		nts::view_t(packet + sealed + length, AEAD_TAG_SIZE)
	);
	// Если расшифровка не выполнена (тег AEAD не сошёлся)
	if(!result)
		// Снятие защиты невозможно
		return status_t::ERROR;
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
 * @return       результат вычисления
 *
 */
bool awh::quic::crypto::retryTag(const cid_t & odcid, string_view packet, uint8_t tag[proto::RETRY_TAG_SIZE]) noexcept {
	// Вычисляем тег целостности по псевдопакету
	return ::__awh_retry_tag__(odcid, packet, tag);
}
/**
 * @brief Функция проверки тега целостности пакета Retry (RFC 9001 §5.8)
 *
 * @param odcid  DCID первого пакета Initial клиента
 * @param packet пакет Retry целиком (включая тег целостности)
 * @return       результат проверки (true - тег целостности корректен)
 *
 */
bool awh::quic::crypto::retryVerify(const cid_t & odcid, string_view packet) noexcept {
	// Если пакет не содержит даже тега целостности
	if(packet.size() <= proto::RETRY_TAG_SIZE)
		// Проверка невозможна
		return false;
	// Ожидаемый тег целостности
	uint8_t expected[proto::RETRY_TAG_SIZE];
	// Вычисляем тег целостности по пакету без тега
	if(!::__awh_retry_tag__(odcid, packet.substr(0, packet.size() - proto::RETRY_TAG_SIZE), expected))
		// Проверка невозможна
		return false;
	// Сравниваем теги целостности за постоянное время
	return nts::message::compare(
		nts::view_t(expected, proto::RETRY_TAG_SIZE),
		nts::view_t(reinterpret_cast <const uint8_t *> (packet.data() + (packet.size() - proto::RETRY_TAG_SIZE)), proto::RETRY_TAG_SIZE)
	);
}
