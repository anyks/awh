/**
 * @file vault.cpp
 * @date 2026-08-22
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
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл
 */
#include <cryptography/vault.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Подключаем заголовочные файлы библиотеки криптографии
 */
#include <openssl/rand.h>
#include <openssl/crypto.h>

/**
 * @brief Пространство имён вспомогательных средств
 *
 */
namespace {
	/**
	 * Длина случайного ключа склада
	 *
	 * Ключ этот наружу не выдаётся и человеком не читается, оттого длина его выбрана
	 * не удобством записи, а запасом стойкости
	 */
	static constexpr size_t KEYSIZE = 32;
	/**
	 * @brief Метод получения случайных октетов
	 *
	 * @param buffer буфер под случайные октеты
	 * @param size   размер буфера
	 * @return       признак удавшегося набора
	 *
	 */
	static bool entropy(uint8_t * buffer, const size_t size) noexcept {
		// Выводим признак удавшегося набора случайных октетов
		return (::RAND_bytes(buffer, static_cast <int32_t> (size)) == 1);
	}
	/**
	 * @brief Метод записи октетов шестнадцатеричным текстом
	 *
	 * @note Пароль средству шифрования передаётся текстом, а случайные октеты текстом
	 *       не являются: нулевой октет оборвал бы его посередине
	 *
	 * @param buffer случайные октеты
	 * @param size   размер буфера
	 * @return       текст шестнадцатеричной записи
	 *
	 */
	static std::string hexify(const uint8_t * buffer, const size_t size) noexcept {
		// Знаки шестнадцатеричной записи
		static const char DIGITS[] = "0123456789abcdef";
		// Текст шестнадцатеричной записи
		std::string result;
		// Отводим место под запись
		result.reserve(size * 2);
		/**
		 * Перебираем октеты буфера
		 */
		for(size_t i = 0; i < size; i++){
			// Записываем старший полуоктет
			result.push_back(DIGITS[(buffer[i] >> 4) & 0x0F]);
			// Записываем младший полуоктет
			result.push_back(DIGITS[buffer[i] & 0x0F]);
		}
		// Выводим текст шестнадцатеричной записи
		return result;
	}
};

/**
 * @brief Метод получения содержимого тайны
 *
 * @return содержимое тайны
 *
 */
const char * awh::Vault::Handle::data() const noexcept {
	// Выводим содержимое тайны
	return (this->_plain.empty() ? nullptr : this->_plain.data());
}
/**
 * @brief Метод получения размера тайны
 *
 * @return размер тайны в байтах
 *
 */
size_t awh::Vault::Handle::size() const noexcept {
	// Выводим размер тайны
	return this->_plain.size();
}
/**
 * @brief Метод получения признака удавшегося взятия
 *
 * @return признак удавшегося взятия
 *
 */
bool awh::Vault::Handle::valid() const noexcept {
	// Выводим признак удавшегося взятия
	return this->_valid;
}
/**
 * @brief Оператор переноса
 *
 * @param handle переносимая рукоять
 * @return       текущая рукоять
 *
 */
awh::Vault::Handle & awh::Vault::Handle::operator = (Handle && handle) noexcept {
	// Если переносится не сама рукоять
	if(this != &handle){
		// Затираем открытый текст, лежавший в рукояти прежде
		if(!this->_plain.empty())
			// Затираем содержимое рукояти
			::OPENSSL_cleanse(this->_plain.data(), this->_plain.size());
		// Переносим открытый текст тайны
		this->_plain = ::std::move(handle._plain);
		// Переносим признак удавшегося взятия
		this->_valid = handle._valid;
		// Отмечаем отданную рукоять негодной
		handle._valid = false;
		// Очищаем отданную рукоять
		handle._plain.clear();
	}
	// Выводим текущую рукоять
	return (* this);
}
/**
 * @brief Конструктор
 *
 */
awh::Vault::Handle::Handle() noexcept : _plain(), _valid(false) {}
/**
 * @brief Конструктор переноса
 *
 * @param handle переносимая рукоять
 *
 */
awh::Vault::Handle::Handle(Handle && handle) noexcept : _plain(::std::move(handle._plain)), _valid(handle._valid) {
	// Отмечаем отданную рукоять негодной
	handle._valid = false;
	// Очищаем отданную рукоять
	handle._plain.clear();
}
/**
 * @brief Конструктор
 *
 * @param plain открытый текст тайны
 *
 */
awh::Vault::Handle::Handle(buffer_t && plain) noexcept : _plain(::std::move(plain)), _valid(true) {}
/**
 * @brief Деструктор
 *
 */
awh::Vault::Handle::~Handle() noexcept {
	/**
	 * Затираем открытый текст немедля
	 *
	 * Затирание обещано и самой укрытой памятью - при возврате её распределителю, - но
	 * буфер вправе пережить рукоять переносом, а обещание рукояти гласит: открытый
	 * текст живёт лишь пока рукоять цела
	 */
	if(!this->_plain.empty())
		// Затираем содержимое рукояти
		::OPENSSL_cleanse(this->_plain.data(), this->_plain.size());
}
/**
 * @brief Метод получения признака заведённого склада
 *
 * @return признак заведённого склада
 *
 */
bool awh::Vault::ready() const noexcept {
	// Выводим признак заведённого склада
	return this->_ready;
}
/**
 * @brief Метод укладки тайны на склад
 *
 * @param name название тайны
 * @param data содержимое тайны
 * @param size размер содержимого
 * @return     признак удавшейся укладки
 *
 */
bool awh::Vault::store(const std::string & name, const void * data, const size_t size) noexcept {
	// Если склад заведён не был либо название тайны не задано
	if(!this->ready() || name.empty())
		// Отвечаем отказом
		return false;
	// Если содержимое не передано, а размер его заявлен
	if((data == nullptr) && (size > 0))
		// Отвечаем отказом
		return false;
	// Шифротекст тайны
	std::vector <char> cipher;
	// Шифруем содержимое тайны
	if(!this->_crypto.encrypt(data, size, cipher, awh::Crypto::hash_t::SHA256, awh::Crypto::cipher_t::AES256)){
		// Затираем то, что успело лечь в буфер
		if(!cipher.empty())
			// Затираем буфер шифротекста
			::OPENSSL_cleanse(cipher.data(), cipher.size());
		// Отвечаем отказом
		return false;
	}
	// Укладываем шифротекст тайны на склад
	this->_secrets[name] = ::std::move(cipher);
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод взятия тайны со склада
 *
 * @param name название тайны
 * @return     рукоять тайны
 *
 */
awh::Vault::Handle awh::Vault::borrow(const std::string & name) noexcept {
	// Если склад заведён не был
	if(!this->ready())
		// Выводим негодную рукоять
		return Handle();
	// Выполняем поиск тайны на складе
	auto i = this->_secrets.find(name);
	// Если тайна на складе не найдена
	if(i == this->_secrets.end())
		// Выводим негодную рукоять
		return Handle();
	/**
	 * Открытый текст ложится в УКРЫТУЮ память
	 *
	 * Обычное хранилище языка отдало бы его в общую кучу - оттуда он ушёл бы в
	 * подкачку и попал бы в снимок памяти при падении, то есть ровно туда, ради чего
	 * склад и заведён
	 */
	buffer_t plain;
	// Расшифровываем содержимое тайны
	if(!this->_crypto.decrypt(i->second.data(), i->second.size(), plain, awh::Crypto::hash_t::SHA256, awh::Crypto::cipher_t::AES256)){
		// Затираем то, что успело лечь в буфер
		if(!plain.empty())
			// Затираем буфер открытого текста
			::OPENSSL_cleanse(plain.data(), plain.size());
		// Выводим негодную рукоять
		return Handle();
	}
	// Выводим рукоять тайны
	return Handle(::std::move(plain));
}
/**
 * @brief Метод снятия тайны со склада
 *
 * @param name название тайны
 * @return     признак снятой тайны
 *
 */
bool awh::Vault::erase(const std::string & name) noexcept {
	// Выполняем поиск тайны на складе
	auto i = this->_secrets.find(name);
	// Если тайна на складе не найдена
	if(i == this->_secrets.end())
		// Отвечаем отказом
		return false;
	// Затираем шифротекст тайны прежде снятия
	if(!i->second.empty())
		// Затираем буфер шифротекста
		::OPENSSL_cleanse(i->second.data(), i->second.size());
	// Снимаем тайну со склада
	this->_secrets.erase(i);
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод снятия шифротекста тайны
 *
 * @param name   название тайны
 * @param cipher буфер, куда ложится шифротекст
 * @return       признак снятого шифротекста
 *
 */
bool awh::Vault::sealed(const std::string & name, std::vector <char> & cipher) const noexcept {
	// Выполняем поиск тайны на складе
	auto i = this->_secrets.find(name);
	// Если тайна на складе не найдена
	if(i == this->_secrets.end())
		// Отвечаем отказом
		return false;
	// Записываем шифротекст тайны
	cipher = i->second;
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод проверки наличия тайны на складе
 *
 * @param name название тайны
 * @return     признак наличия тайны
 *
 */
bool awh::Vault::has(const std::string & name) const noexcept {
	// Выводим признак наличия тайны на складе
	return (this->_secrets.find(name) != this->_secrets.end());
}
/**
 * @brief Метод получения числа тайн на складе
 *
 * @return число тайн на складе
 *
 */
size_t awh::Vault::count() const noexcept {
	// Выводим число тайн на складе
	return this->_secrets.size();
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::Vault::Vault(const fmk_t * fmk, const log_t * log) noexcept :
 _secrets(), _crypto(fmk, log), _ready(false), _fmk(fmk), _log(log) {
	// Случайный ключ склада
	uint8_t key[KEYSIZE];
	// Случайная соль склада
	uint8_t salt[KEYSIZE];
	/**
	 * Ключ берётся случайным при заведении склада
	 *
	 * Пароля, заданного человеком, здесь нет намеренно: склад переживает лишь работу
	 * программы, и хранить его между запусками нечем
	 */
	if(::entropy(key, sizeof(key)) && ::entropy(salt, sizeof(salt))){
		// Записываем ключ шестнадцатеричным текстом
		std::string password = ::hexify(key, sizeof(key));
		// Записываем соль шестнадцатеричным текстом
		std::string flavour = ::hexify(salt, sizeof(salt));
		// Устанавливаем соль шифрования
		this->_crypto.salt(flavour);
		// Устанавливаем пароль шифрования
		this->_crypto.password(password);
		// Затираем текст пароля: средство шифрования вывело из него ключ и в тексте больше не нуждается
		::OPENSSL_cleanse(&password[0], password.size());
		// Затираем текст соли
		::OPENSSL_cleanse(&flavour[0], flavour.size());
		// Отмечаем склад заведённым
		this->_ready = true;
	/**
	 * Если случайного ключа взять неоткуда
	 */
	} else if(this->_log != nullptr)
		// Выводим сообщение об ошибке
		this->_log->print("Vault is not prepared: the source of randomness is not available", log_t::flag_t::CRITICAL);
	// Затираем случайные октеты
	::OPENSSL_cleanse(key, sizeof(key));
	// Затираем случайные октеты соли
	::OPENSSL_cleanse(salt, sizeof(salt));
}
/**
 * @brief Деструктор
 *
 */
awh::Vault::~Vault() noexcept {
	/**
	 * Перебираем тайны склада
	 */
	for(auto & item : this->_secrets){
		// Затираем шифротекст тайны
		if(!item.second.empty())
			// Затираем буфер шифротекста
			::OPENSSL_cleanse(item.second.data(), item.second.size());
	}
}
