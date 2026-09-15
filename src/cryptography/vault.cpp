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
#include <sys/log.hpp>

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
 * @brief Метод получения сведений о состоявшейся защите памяти склада
 *
 * @return сведения о состоявшейся защите
 *
 */
const awh::alloc::shelter_t & awh::Vault::shelter() const noexcept {
	// Выводим сведения о состоявшейся защите
	return this->_shelter;
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
	/**
	 * Замок ведётся лишь у склада, поделённого между потоками
	 *
	 * Склад, принадлежащий одному потоку, делить не с кем, и замок на горячем пути
	 * стоил бы дороже того, что стережёт. Стережёт он СОГЛАСОВАННОСТЬ СОСТАВА, а не
	 * страницы: доступность области считает сам распределитель
	 */
	// Охранник состава склада
	std::unique_lock <std::mutex> lock(this->_mutex, std::defer_lock);
	// Если склад делится между потоками
	if(this->_threading == threading_t::SHARED)
		// Берём замок состава склада
		lock.lock();
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
	/**
	 * Прежний шифротекст затирается ПРЕЖДЕ перезаписи
	 *
	 * Перенос поверх прежнего значения возвращает старый буфер куче незатёртым, и
	 * шифротекст, снятый со склада перезаписью, остался бы лежать в памяти - тогда
	 * как снятие через `erase` и роспуск склада его затирают. Перезапись была
	 * единственным путём, каким содержимое покидало склад нетронутым
	 */
	// Выполняем поиск прежней тайны того же названия
	auto i = this->_secrets.find(name);
	// Если тайна того же названия на складе уже лежит
	if((i != this->_secrets.end()) && !i->second.empty())
		// Затираем прежний шифротекст тайны
		::OPENSSL_cleanse(i->second.data(), i->second.size());
	// Укладываем шифротекст тайны на склад
	this->_secrets[name] = ::std::move(cipher);
	// Отвечаем успехом
	return true;
}
/**
 * @brief Метод укладки тайны на склад из приёмника тайн
 *
 * @param name   название тайны
 * @param vessel приёмник с содержимым тайны
 * @return       признак удавшейся укладки
 *
 */
bool awh::Vault::store(const std::string & name, awh::alloc::vessel_t & vessel) noexcept {
	// Признак удавшейся укладки
	bool result = false;
	/**
	 * Содержимое берётся из защищённой области приёмника
	 *
	 * Обращение через обработчик здесь не удобство, а суть дела: приёмник раскрывает
	 * область лишь на время вызова и запечатывает её обратно сам, и содержимое нигде
	 * не ложится вместилищем языка по дороге
	 */
	static_cast <void> (vessel.apply([this, &name, &result](const uint8_t * data, const size_t size) noexcept -> void {
		// Укладываем содержимое приёмника на склад
		result = this->store(name, static_cast <const void *> (data), size);
	}));
	// Выводим признак удавшейся укладки
	return result;
}
/**
 * @brief Метод взятия тайны со склада
 *
 * @param name название тайны
 * @return     рукоять тайны
 *
 */
awh::Vault::Handle awh::Vault::borrow(const std::string & name) noexcept {
	/**
	 * Замок ведётся лишь у склада, поделённого между потоками
	 *
	 * Склад, принадлежащий одному потоку, делить не с кем, и замок на горячем пути
	 * стоил бы дороже того, что стережёт. Стережёт он СОГЛАСОВАННОСТЬ СОСТАВА, а не
	 * страницы: доступность области считает сам распределитель
	 */
	// Охранник состава склада
	std::unique_lock <std::mutex> lock(this->_mutex, std::defer_lock);
	// Если склад делится между потоками
	if(this->_threading == threading_t::SHARED)
		// Берём замок состава склада
		lock.lock();
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
	/**
	 * Замок ведётся лишь у склада, поделённого между потоками
	 *
	 * Склад, принадлежащий одному потоку, делить не с кем, и замок на горячем пути
	 * стоил бы дороже того, что стережёт. Стережёт он СОГЛАСОВАННОСТЬ СОСТАВА, а не
	 * страницы: доступность области считает сам распределитель
	 */
	// Охранник состава склада
	std::unique_lock <std::mutex> lock(this->_mutex, std::defer_lock);
	// Если склад делится между потоками
	if(this->_threading == threading_t::SHARED)
		// Берём замок состава склада
		lock.lock();
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
	/**
	 * Замок ведётся лишь у склада, поделённого между потоками
	 *
	 * Склад, принадлежащий одному потоку, делить не с кем, и замок на горячем пути
	 * стоил бы дороже того, что стережёт. Стережёт он СОГЛАСОВАННОСТЬ СОСТАВА, а не
	 * страницы: доступность области считает сам распределитель
	 */
	// Охранник состава склада
	std::unique_lock <std::mutex> lock(this->_mutex, std::defer_lock);
	// Если склад делится между потоками
	if(this->_threading == threading_t::SHARED)
		// Берём замок состава склада
		lock.lock();
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
	/**
	 * Замок ведётся лишь у склада, поделённого между потоками
	 *
	 * Склад, принадлежащий одному потоку, делить не с кем, и замок на горячем пути
	 * стоил бы дороже того, что стережёт. Стережёт он СОГЛАСОВАННОСТЬ СОСТАВА, а не
	 * страницы: доступность области считает сам распределитель
	 */
	// Охранник состава склада
	std::unique_lock <std::mutex> lock(this->_mutex, std::defer_lock);
	// Если склад делится между потоками
	if(this->_threading == threading_t::SHARED)
		// Берём замок состава склада
		lock.lock();
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
	/**
	 * Замок ведётся лишь у склада, поделённого между потоками
	 *
	 * Склад, принадлежащий одному потоку, делить не с кем, и замок на горячем пути
	 * стоил бы дороже того, что стережёт. Стережёт он СОГЛАСОВАННОСТЬ СОСТАВА, а не
	 * страницы: доступность области считает сам распределитель
	 */
	// Охранник состава склада
	std::unique_lock <std::mutex> lock(this->_mutex, std::defer_lock);
	// Если склад делится между потоками
	if(this->_threading == threading_t::SHARED)
		// Берём замок состава склада
		lock.lock();
	// Выводим число тайн на складе
	return this->_secrets.size();
}
/**
 * @brief Метод получения склада, общего на процесс
 *
 * @return склад, общий на процесс
 *
 */
awh::Vault & awh::Vault::shared() noexcept {
	/**
	 * Заводится склад при первом обращении и живёт до конца работы программы
	 *
	 * Заведение это потокобезопасно само по себе: языком положено, что статический
	 * объект внутри хода заводится единожды, а прочие потоки ждут окончания заведения
	 */
	static Vault vault(awh::alloc::secrecy_t::STRICT, threading_t::SHARED);
	// Выводим склад, общий на процесс
	return vault;
}
/**
 * @brief Метод получения склада, своего у каждого потока
 *
 * @return склад, свой у каждого потока
 *
 */
awh::Vault & awh::Vault::local() noexcept {
	/**
	 * Свой склад у каждого потока замка не ведёт: делить его не с кем
	 *
	 * Память тем взята по числу потоков, обратившихся к нему, - это и есть плата за
	 * снятие замка с горячего пути, и назначена она намеренно
	 */
	static thread_local Vault vault(awh::alloc::secrecy_t::STRICT, threading_t::LOCAL);
	// Выводим склад, свой у текущего потока
	return vault;
}
/**
 * @brief Конструктор
 *
 * @param secrecy   строгость склада
 * @param threading вид многопоточности склада
 *
 */
awh::Vault::Vault(const awh::alloc::secrecy_t secrecy, const threading_t threading) noexcept :
 _secrets(), _crypto(), _ready(false), _threading(threading), _secrecy(secrecy), _mutex(), _shelter() {
	/**
	 * Спрашиваем у распределителя, какая защита состоялась НА ДЕЛЕ
	 *
	 * Спрашивается это единожды при заведении склада пробной выдачей: обещания у
	 * систем разные, а само хранилище языка спросить об этом неоткуда - оно передаёт
	 * распределителю пустоту вместо сведений. Не спроси склад здесь, о молчаливом
	 * понижении защиты не узнал бы никто
	 */
	{
		// Выполняем пробную укрытую выдачу
		void * probe = awh::alloc::Allocator::secure(KEYSIZE, &this->_shelter);
		// Если пробная выдача удалась
		if(probe != nullptr)
			// Возвращаем пробную выдачу распределителю
			awh::alloc::Allocator::release(probe);
	}
	/**
	 * Строгий склад отвечает отказом, не получив обещанной защиты
	 *
	 * Молчаливое понижение защиты хуже честного отказа: заводящий склад вправе знать,
	 * что тайны его уйдут в подкачку, ПРЕЖДЕ чем он туда что-то положит. Судится это
	 * запретом подкачки, а не укрытием от снимка: укрытие даётся не всюду, и требовать
	 * его значило бы закрыть склад на системах, где защита лишь ниже, а не отсутствует
	 */
	if(!this->_shelter.wired){
		// Если склад заведён строгим
		if(this->_secrecy == awh::alloc::secrecy_t::STRICT){
			// Записываем отказ заведения в журнал
			awh::log::print("Vault is not prepared: memory locking is unavailable, strict vault refuses to hold the secrets", awh::log::flag_t::CRITICAL);
			// Склад заведённым не отмечаем: работать он отказывается целиком
			return;
		}
		// Записываем понижение защиты в журнал
		awh::log::print("Vault: memory locking is unavailable, the secrets may reach the swap", awh::log::flag_t::WARNING);
	}
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
	} else
		// Выводим сообщение об ошибке
		awh::log::print("Vault is not prepared: the source of randomness is not available", awh::log::flag_t::CRITICAL);
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
