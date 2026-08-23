/**
 * @file container.cpp
 * @date 2026-08-19
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
 * \~russian
 * @brief Файл реализации сборки бинарного контейнера ABC целиком
 *
 * \~english
 * @brief Implementation file of the assembling of the whole ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <codec/abc/container.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>
#include <limits>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Конструктор
 *
 */
awh::codec::abc::Assembler::Settings::Settings() noexcept :
 block(0x10000), canonical(false), stream(true), indexed(true), content(0) {}

/**
 * @brief Метод установки модуля сжатия
 *
 * @param value устанавливаемый модуль сжатия, ноль - снятие модуля
 *
 */
void awh::codec::abc::Assembler::compressor(const compressor::block_t * value) noexcept {
	// Выполняем установку модуля сжатия укладчику кадра
	this->_packer.compressor(value);
}
/**
 * @brief Метод установки модуля шифрования
 *
 * @param value устанавливаемый модуль шифрования, ноль - снятие модуля
 *
 */
void awh::codec::abc::Assembler::crypto(const crypto_t * value) noexcept {
	// Выполняем установку модуля шифрования укладчику кадра
	this->_packer.crypto(value);
}
/**
 * @brief Метод объявления подписи собираемого контейнера
 *
 * @param crypto модуль шифрования, ноль - снятие подписи
 * @param name   имя ключа владельца контейнера
 * @param hash   желаемый вид хэш-суммы подписи
 *
 */
void awh::codec::abc::Assembler::sign(const crypto_t * crypto, const string & name,
 const crypto_t::hash_t hash) noexcept {
	// Выполняем установку модуля шифрования для подписи контейнера
	this->_signer = crypto;
	// Выполняем установку имени ключа владельца контейнера
	this->_name = name;
	// Выполняем установку желаемого вида хэш-суммы подписи
	this->_hash = hash;
	// Выполняем установку модуля шифрования дереву свёрток
	this->_merkle.crypto(crypto);
}
/**
 * @brief Метод установки признака владельца контейнера
 *
 * @param buffer буфер устанавливаемого признака владельца
 * @param size   размер устанавливаемого признака владельца
 *
 */
void awh::codec::abc::Assembler::owner(const void * buffer, const size_t size) noexcept {
	// Выполняем очистку прежнего признака владельца контейнера
	::memset(this->_header.owner, 0, OWNER_LENGTH);
	/**
	 * Если признак владельца контейнера нам передан
	 */
	if((buffer != nullptr) && (size > 0))
		/**
		 * Выполняем установку признака владельца, обрезая лишнее: длина признака
		 * постоянна, и втиснуть в неё больше нельзя
		 */
		::memcpy(this->_header.owner, buffer, (size < OWNER_LENGTH ? size : OWNER_LENGTH));
}
/**
 * @brief Метод установки отпечатка открытого ключа владельца
 *
 * @param buffer буфер устанавливаемого отпечатка ключа
 * @param size   размер устанавливаемого отпечатка ключа
 *
 */
void awh::codec::abc::Assembler::fingerprint(const void * buffer, const size_t size) noexcept {
	// Выполняем очистку прежнего отпечатка открытого ключа владельца
	::memset(this->_header.fingerprint, 0, FINGERPRINT_LENGTH);
	/**
	 * Если отпечаток открытого ключа владельца нам передан
	 */
	if((buffer != nullptr) && (size > 0))
		// Выполняем установку отпечатка открытого ключа, обрезая лишнее
		::memcpy(this->_header.fingerprint, buffer, (size < FINGERPRINT_LENGTH ? size : FINGERPRINT_LENGTH));
}
/**
 * @brief Метод внесения значения записью контейнера
 *
 * @param value вносимое значение
 * @param kind  вид содержимого вносимой записи
 * @return      признак успешности внесения
 *
 */
bool awh::codec::abc::Assembler::append(const value_t & value, const payload_t kind) noexcept {
	// Выполняем сброс кода отказа сборки контейнера
	this->_error = error_t::NONE;
	// Создаём сборщик бинарной записи
	writer_t writer;
	// Получаем настройки сборки бинарной записи
	writer_t::settings_t settings = writer.settings();
	// Выполняем установку строгого вида записи
	settings.canonical = this->_settings.canonical;
	// Выполняем установку настроек сборки бинарной записи
	writer.settings(settings);
	/**
	 * Если собрать запись из поданного значения не вышло
	 */
	if(!value.compose(writer)){
		// Выполняем установку кода отказа сборки записи
		this->_error = writer.error();
		// Выводим признак неудачного внесения записи
		return false;
	}
	// Выполняем внесение собранной записи
	return this->append(writer.record().data(), writer.record().size(), kind);
}
/**
 * @brief Метод внесения готовой записи контейнера
 *
 * @param buffer буфер вносимой записи
 * @param size   размер вносимой записи
 * @param kind   вид содержимого вносимой записи
 * @return       признак успешности внесения
 *
 */
bool awh::codec::abc::Assembler::append(const void * buffer, const size_t size, const payload_t kind) noexcept {
	// Выполняем сброс кода отказа сборки контейнера
	this->_error = error_t::NONE;
	/**
	 * Если запись нам не передана
	 */
	if((buffer == nullptr) || (size == 0)){
		// Выполняем установку кода отказа внесения записи
		this->_error = error_t::EMPTY_RECORD;
		// Выводим признак неудачного внесения записи
		return false;
	}
	/**
	 * Если вносимая запись не вмещается в поля строки оглавления.
	 *
	 * Смещение и длина записи ложатся в оглавление разрядностью 32, и приведение к
	 * ней усекает молча: строка указала бы не туда, а собранный контейнер вышел бы
	 * с виду годным. Отказ укладчика по тому же пределу приходит позже - строки
	 * оглавления к тому времени уже заведены усечёнными.
	 *
	 * Сличение стоит ДО накопления поданного: отказ обязан не трогать поданного
	 * буфера вовсе, иначе доказать его можно лишь буфером, взаправду занявшим память
	 */
	if(size > static_cast <size_t> (numeric_limits <uint32_t>::max())){
		// Выполняем установку кода отказа длины записи
		this->_error = error_t::INVALID_LENGTH;
		// Выводим признак неудачного внесения записи
		return false;
	}
	/**
	 * Если вид содержимого сменился, выполняем укладку накопленного кадром:
	 * кадр вперемешку отнял бы у подбора метода сжатия всякий смысл
	 */
	if(!this->_pending.empty() && (kind != this->_kind)){
		/**
		 * Если уложить накопленное кадром не вышло
		 */
		if(!this->flush())
			// Выводим признак неудачного внесения записи
			return false;
	}
	/**
	 * Если накопленное вместе с вносимой записью выходит за поле смещения строки
	 * оглавления, выполняем укладку накопленного кадром: порог накопления есть
	 * пожелание, а разрядность поля - предел, и уступает пожелание
	 */
	if(!this->_pending.empty() &&
	 ((this->_pending.size() + size) > static_cast <size_t> (numeric_limits <uint32_t>::max()))){
		/**
		 * Если уложить накопленное кадром не вышло
		 */
		if(!this->flush())
			// Выводим признак неудачного внесения записи
			return false;
	}
	// Выполняем установку вида содержимого накопленных записей
	this->_kind = kind;
	/**
	 * Если оглавление ведётся, выполняем заведение строки его на вносимую запись
	 */
	if(this->_settings.indexed){
		// Заводимая строка оглавления
		entry_t entry;
		/**
		 * Выполняем установку смещения записи в содержимом кадра: смещение кадра в
		 * теле контейнера станет известно лишь по укладке его
		 */
		entry.offset = static_cast <uint32_t> (this->_pending.size());
		// Выполняем установку длины вносимой записи
		entry.length = static_cast <uint32_t> (size);
		// Выполняем внесение заведённой строки оглавления
		this->_marks.push_back(entry);
	}
	// Выполняем накопление поданной записи
	this->_pending.insert(this->_pending.end(),
	 reinterpret_cast <const uint8_t *> (buffer), reinterpret_cast <const uint8_t *> (buffer) + size);
	// Выполняем увеличение количества уложенных записей
	this->_records++;
	/**
	 * Если накоплено достаточно записей, выполняем укладку их кадром
	 */
	if(this->_pending.size() >= this->_settings.block)
		// Выводим результат укладки накопленного кадром
		return this->flush();
	// Выводим признак успешного внесения записи
	return true;
}
/**
 * @brief Метод укладки накопленных записей кадром
 *
 * @return признак успешности укладки
 *
 */
bool awh::codec::abc::Assembler::flush() noexcept {
	// Выполняем сброс кода отказа сборки контейнера
	this->_error = error_t::NONE;
	/**
	 * Если накопленных записей нет, укладывать нечего
	 */
	if(this->_pending.empty())
		// Выводим признак успешной укладки
		return true;
	// Буфер уложенного кадра
	vector <uint8_t> chunk;
	/**
	 * Если уложить накопленные записи кадром не вышло
	 */
	if(!this->_packer.pack(this->_pending.data(), this->_pending.size(), this->_kind, this->_number, 0, chunk)){
		// Выполняем установку кода отказа укладки кадра
		this->_error = this->_packer.error();
		/**
		 * Накопленное отказом не сбрасывается: причина отказа может быть
		 * устранена, и следующая попытка пройдёт по тем же данным
		 */
		return false;
	}
	/**
	 * Если кадр вышел сжатым, выполняем объявление сжатости тела контейнера
	 */
	if(chunk[0] != static_cast <uint8_t> (compressor::method_t::NONE))
		// Выполняем установку признака сжатого кадра
		this->_compressed = true;
	/**
	 * Выполняем перебор строк оглавления накопленных записей
	 */
	for(entry_t & entry : this->_marks){
		// Выполняем установку смещения уложенного кадра от начала тела контейнера
		entry.chunk = static_cast <uint64_t> (this->_body.size());
		// Выполняем внесение строки в оглавление собираемого контейнера
		this->_index.add(entry);
	}
	// Выполняем очистку строк оглавления накопленных записей
	this->_marks.clear();
	/**
	 * Если контейнер подписывается, выполняем внесение кадра в дерево свёрток
	 */
	if(this->_signer != nullptr){
		/**
		 * Если внести кадр свёрткой в дерево не вышло
		 */
		if(!this->_merkle.add(chunk.data(), chunk.size())){
			// Выполняем установку кода отказа выработки свёртки
			this->_error = error_t::SIGNING_FAILED;
			// Выводим признак неудачной укладки
			return false;
		}
	}
	// Выполняем внесение уложенного кадра в тело контейнера
	this->_body.insert(this->_body.end(), chunk.begin(), chunk.end());
	// Выполняем увеличение порядкового номера следующего кадра
	this->_number++;
	// Выполняем очистку накопленных записей
	this->_pending.clear();
	// Выводим признак успешной укладки
	return true;
}
/**
 * @brief Метод завершения сборки контейнера
 *
 * @param result буфер, куда следует уложить собранный контейнер
 * @return       признак успешности сборки
 *
 */
bool awh::codec::abc::Assembler::complete(vector <uint8_t> & result) noexcept {
	/**
	 * Если уложить накопленные записи кадром не вышло
	 */
	if(!this->flush())
		// Выводим признак неудачной сборки контейнера
		return false;
	// Буфер уложенного кадра оглавления собранного контейнера
	vector <uint8_t> tail;
	/**
	 * Если оглавление ведётся и записи в нём есть, выполняем укладку его за телом
	 */
	if(this->_settings.indexed && (this->_index.size() > 0)){
		// Буфер уложенного оглавления собранного контейнера
		vector <uint8_t> entries;
		// Выполняем укладку оглавления собранного контейнера в октеты
		this->_index.pack(entries);
		/**
		 * Выполняем укладку оглавления кадром видом однородных чисел: оглавление
		 * есть череда чисел, и метод сжатия ему подобает свой, а не тела
		 */
		if(!this->_packer.pack(entries.data(), entries.size(), payload_t::NUMERIC, this->_number, 0, tail)){
			// Выполняем установку кода отказа укладки кадра оглавления
			this->_error = this->_packer.error();
			// Выводим признак неудачной сборки контейнера
			return false;
		}
		/**
		 * Выполняем установку смещения оглавления от начала контейнера: оглавление
		 * лежит за телом, и длиною тела оно не считается
		 */
		this->_header.index = static_cast <uint64_t> (HEADER_LENGTH + this->_body.size());
	}
	// Выполняем установку длины тела собранного контейнера
	this->_header.length = static_cast <uint64_t> (this->_body.size());
	// Выполняем установку количества записей тела собранного контейнера
	this->_header.records = this->_records;
	// Выполняем установку вида содержимого собранного контейнера
	this->_header.content = this->_settings.content;
	// Выполняем объявление строгого вида записи собранного контейнера
	this->_header.set(flag_t::CANONICAL, this->_settings.canonical);
	// Выполняем объявление череды документов в теле собранного контейнера
	this->_header.set(flag_t::STREAM, this->_settings.stream);
	// Выполняем объявление сжатости тела собранного контейнера
	this->_header.set(flag_t::COMPRESSED, this->_compressed);
	// Выполняем объявление зашифрованности тела собранного контейнера
	this->_header.set(flag_t::ENCRYPTED, this->_packer.settings().encrypt);
	// Буфер уложенной записи подписи владельца контейнера
	vector <uint8_t> signature;
	/**
	 * Если контейнер подписывается, выполняем выработку подписи владельца
	 */
	if(this->_signer != nullptr){
		// Собираемая подпись владельца контейнера
		sign_t sign;
		// Выполняем получение вида подписи по ключу владельца
		sign.kind = this->_signer->signature(this->_name);
		/**
		 * Выполняем подбор вида хэш-суммы под вид подписи: поданный служит лишь
		 * пожеланием, а решает вид ключа
		 */
		sign.hash = digest(sign.kind, this->_hash);
		/**
		 * Если свести дерево свёрток к корню не вышло
		 */
		/**
		 * Кадр оглавления придаётся сведению дерева, а не оседает в нём: оглавление
		 * подписывается наравне с телом, но при правке контейнера ложится наново
		 */
		if(!this->_merkle.root(sign.root, tail.data(), tail.size())){
			// Выполняем установку кода отказа выработки свёртки
			this->_error = error_t::SIGNING_FAILED;
			// Выводим признак неудачной сборки контейнера
			return false;
		}
		/**
		 * Если выработать подпись корня дерева не вышло
		 */
		if(!this->_signer->sign(this->_name, sign.root.data(), sign.root.size(), sign.hash, sign.signature) ||
		 sign.signature.empty()){
			// Выполняем установку кода отказа выработки подписи
			this->_error = error_t::SIGNING_FAILED;
			// Выводим признак неудачной сборки контейнера
			return false;
		}
		// Выполняем укладку записи подписи владельца контейнера
		abc::pack(sign, signature);
		/**
		 * Выполняем установку смещения подписи от начала контейнера: подпись лежит
		 * за оглавлением, ибо оглавление ею же и подписано
		 */
		this->_header.signature = static_cast <uint64_t> (HEADER_LENGTH + this->_body.size() + tail.size());
		// Выполняем объявление подписанности контейнера
		this->_header.set(flag_t::SIGNED, true);
		// Буфер усечённого отпечатка открытого ключа владельца
		vector <uint8_t> print;
		/**
		 * Если отпечаток открытого ключа владельца выработан
		 */
		if(abc::fingerprint(* this->_signer, this->_name, print))
			// Выполняем установку отпечатка открытого ключа владельца
			this->fingerprint(print.data(), print.size());
	}
	/**
	 * Выполняем отведение памяти под собранный контейнер целиком
	 *
	 * @details Размер контейнера здесь известен точно: заголовок, тело, кадр оглавления
	 *          и запись подписи уже собраны. Без отведения наперёд внесение тела растит
	 *          буфер удвоением и переносит мегабайты по нескольку раз — у Windows, где
	 *          крупный блок памяти возвращается системе, это вдвое роняло полосу сборки
	 *
	 * @note Отведение стоит ДО укладки заголовка: очистка буфера внутри неё отведённую
	 *       память не отбирает, а вот отведение после укладки затёрло бы уложенное
	 */
	result.reserve(HEADER_LENGTH + this->_body.size() + tail.size() + signature.size());
	// Выполняем укладку заголовка опознания собранного контейнера
	this->_header.pack(result);
	// Выполняем внесение тела собранного контейнера
	result.insert(result.end(), this->_body.begin(), this->_body.end());
	// Выполняем внесение кадра оглавления собранного контейнера
	result.insert(result.end(), tail.begin(), tail.end());
	// Выполняем внесение записи подписи владельца собранного контейнера
	result.insert(result.end(), signature.begin(), signature.end());
	// Выводим признак успешной сборки контейнера
	return true;
}
/**
 * @brief Метод сброса состояния сборки контейнера
 *
 */
void awh::codec::abc::Assembler::reset() noexcept {
	// Выполняем сброс кода отказа сборки контейнера
	this->_error = error_t::NONE;
	// Выполняем сброс вида содержимого накопленных записей
	this->_kind = payload_t::MIXED;
	// Выполняем сброс накопленных записей
	this->_pending.clear();
	// Выполняем сброс тела собираемого контейнера
	this->_body.clear();
	// Выполняем сброс оглавления собираемого контейнера
	this->_index.clear();
	// Выполняем сброс строк оглавления накопленных записей
	this->_marks.clear();
	// Выполняем сброс количества уложенных записей
	this->_records = 0;
	// Выполняем сброс порядкового номера следующего кадра
	this->_number = 0;
	// Выполняем сброс признака сжатости тела контейнера
	this->_compressed = false;
	// Выполняем сброс дерева свёрток по кадрам контейнера
	this->_merkle.clear();
	/**
	 * Заголовок опознания сбрасывается целиком вместе с признаком владельца:
	 * сброс есть заведение нового контейнера, а не очистка тела прежнего
	 */
	this->_header = header_t();
}
/**
 * @brief Метод извлечения количества уложенных записей
 *
 * @return количество уложенных записей
 *
 */
uint64_t awh::codec::abc::Assembler::records() const noexcept {
	// Выводим количество уложенных записей
	return this->_records;
}
/**
 * @brief Метод извлечения размера накопленных записей
 *
 * @return размер накопленных записей в октетах
 *
 */
size_t awh::codec::abc::Assembler::pending() const noexcept {
	// Выводим размер накопленных записей
	return this->_pending.size();
}
/**
 * @brief Метод извлечения оглавления собираемого контейнера
 *
 * @return оглавление собираемого контейнера
 *
 */
const awh::codec::abc::index_t & awh::codec::abc::Assembler::index() const noexcept {
	// Выводим оглавление собираемого контейнера
	return this->_index;
}
/**
 * @brief Метод извлечения кода отказа сборки контейнера
 *
 * @return код отказа
 *
 */
awh::codec::abc::error_t awh::codec::abc::Assembler::error() const noexcept {
	// Выводим код отказа сборки контейнера
	return this->_error;
}
/**
 * @brief Метод извлечения модуля укладки кадра
 *
 * @return модуль укладки кадра
 *
 */
awh::codec::abc::packer_t & awh::codec::abc::Assembler::packer() noexcept {
	// Выводим модуль укладки кадра
	return this->_packer;
}
/**
 * @brief Метод извлечения настроек сборки контейнера
 *
 * @return настройки сборки контейнера
 *
 */
const awh::codec::abc::Assembler::settings_t & awh::codec::abc::Assembler::settings() const noexcept {
	// Выводим настройки сборки контейнера
	return this->_settings;
}
/**
 * @brief Метод установки настроек сборки контейнера
 *
 * @param settings устанавливаемые настройки сборки контейнера
 *
 */
void awh::codec::abc::Assembler::settings(const settings_t & settings) noexcept {
	// Выполняем установку настроек сборки контейнера
	this->_settings = settings;
	/**
	 * Если порог накопления записей не установлен, ставим его наименьшим:
	 * нулевой порог уложил бы всякую запись своим кадром
	 */
	if(this->_settings.block == 0)
		// Выполняем установку наименьшего порога накопления записей
		this->_settings.block = 1;
}
/**
 * @brief Конструктор
 *
 */
awh::codec::abc::Assembler::Assembler() noexcept :
 _error(error_t::NONE), _kind(payload_t::MIXED), _records(0), _number(0), _compressed(false),
 _signer(nullptr), _hash(crypto_t::hash_t::SHA256) {}

/**
 * @brief Метод установки модуля сжатия
 *
 * @param value устанавливаемый модуль сжатия, ноль - снятие модуля
 *
 */
void awh::codec::abc::Loader::compressor(const compressor::block_t * value) noexcept {
	// Выполняем установку модуля сжатия снимателю кадра
	this->_packer.compressor(value);
}
/**
 * @brief Метод установки модуля шифрования
 *
 * @param value устанавливаемый модуль шифрования, ноль - снятие модуля
 *
 */
void awh::codec::abc::Loader::crypto(const crypto_t * value) noexcept {
	// Выполняем установку модуля шифрования снимателю кадра
	this->_packer.crypto(value);
}
/**
 * @brief Метод подачи октетов контейнера
 *
 * @param buffer буфер подаваемых октетов
 * @param size   размер подаваемых октетов
 * @return       признак успешности подачи
 *
 */
bool awh::codec::abc::Loader::feed(const void * buffer, const size_t size) noexcept {
	/**
	 * Если октеты нам переданы неверно
	 */
	if((buffer == nullptr) && (size > 0)){
		// Выполняем установку кода отказа подачи октетов
		this->_error = error_t::INTERNAL;
		// Выводим признак неудачной подачи октетов
		return false;
	}
	/**
	 * Если октеты нам переданы
	 */
	if(size > 0)
		// Выполняем накопление поданных октетов
		this->_buffer.insert(this->_buffer.end(),
		 reinterpret_cast <const uint8_t *> (buffer), reinterpret_cast <const uint8_t *> (buffer) + size);
	// Выводим признак успешной подачи октетов
	return true;
}
/**
 * @brief Метод выдачи содержимого очередного кадра
 *
 * @param result буфер, куда следует положить содержимое кадра
 * @param chunk  снятые сведения о кадре
 * @return       признак выданного кадра
 *
 */
bool awh::codec::abc::Loader::next(vector <uint8_t> & result, chunk_t & chunk) noexcept {
	// Выполняем сброс кода отказа снятия контейнера
	this->_error = error_t::NONE;
	/**
	 * Если заголовок опознания контейнера ещё не снят
	 */
	if(!this->_ready){
		/**
		 * Если октетов заголовка опознания ещё не набралось
		 */
		if((this->_buffer.size() - this->_offset) < HEADER_LENGTH){
			// Выполняем установку кода отказа снятия заголовка
			this->_error = error_t::TRUNCATED_HEADER;
			// Выводим признак того, что кадр не выдан
			return false;
		}
		/**
		 * Если снять заголовок опознания контейнера не вышло
		 */
		if(!this->_header.unpack(this->_buffer.data() + this->_offset, HEADER_LENGTH, this->_error))
			// Выводим признак того, что кадр не выдан
			return false;
		// Выполняем сдвиг смещения разбора на длину заголовка опознания
		this->_offset += HEADER_LENGTH;
		// Выполняем установку признака снятого заголовка опознания
		this->_ready = true;
	}
	/**
	 * Выполняем снятие кадров, пропуская обращённые правкой в мусор
	 */
	while(true){
		/**
		 * Если тело контейнера вычитано целиком, кадров больше нет
		 */
		if(static_cast <uint64_t> ((this->_origin + this->_offset) - HEADER_LENGTH) >= this->_header.length)
			// Выводим признак того, что кадр не выдан
			return false;
		// Смещение снятия очередного кадра
		size_t offset = this->_offset;
		/**
		 * Если снять очередной кадр не вышло
		 */
		if(!this->_packer.unpack(this->_buffer.data(), this->_buffer.size(), offset, result, chunk)){
			// Выполняем установку кода отказа снятия кадра
			this->_error = this->_packer.error();
			// Выводим признак того, что кадр не выдан
			return false;
		}
		/**
		 * Если снятый кадр вышел за тело контейнера.
		 *
		 * Сниматель кадра сличает объявленную длину лишь с поданными октетами, а тело
		 * контейнера кончается ранее их: за телом лежат оглавление и подпись. Кадр,
		 * объявивший длину сверх тела, без этой проверки вобрал бы оглавление
		 * содержимым записей и выдал бы его потребителю за содержимое контейнера
		 */
		if((this->_origin + static_cast <uint64_t> (offset)) > (HEADER_LENGTH + this->_header.length)){
			// Выполняем установку кода отказа опознания кадра
			this->_error = error_t::INVALID_CHUNK;
			// Выполняем очистку содержимого снятого кадра
			result.clear();
			// Выводим признак того, что кадр не выдан
			return false;
		}
		// Выполняем сдвиг смещения разбора на длину снятого кадра
		this->_offset = offset;
		/**
		 * Выполняем ужатие буфера подачи: содержимое кадра выдано копией, и
		 * вычитанные октеты держать больше незачем
		 */
		if(this->_offset > 0){
			// Выполняем отбрасывание вычитанных октетов из буфера подачи
			this->_buffer.erase(this->_buffer.begin(), this->_buffer.begin() + static_cast <ptrdiff_t> (this->_offset));
			// Выполняем увеличение количества отброшенных октетов
			this->_origin += this->_offset;
			// Выполняем сброс смещения разбора
			this->_offset = 0;
		}
		/**
		 * Если снятый кадр обращён правкой в мусор, выполняем пропуск его: записей
		 * он более не несёт, а лежит на носителе до уборки
		 */
		if(chunk.waste){
			// Выполняем очистку содержимого пропущенного кадра
			result.clear();
			// Выполняем переход к следующему кадру
			continue;
		}
		// Выводим признак выданного кадра
		return true;
	}
}
/**
 * @brief Метод сброса состояния снятия контейнера
 *
 */
void awh::codec::abc::Loader::reset() noexcept {
	// Выполняем сброс кода отказа снятия контейнера
	this->_error = error_t::NONE;
	// Выполняем сброс признака снятого заголовка опознания
	this->_ready = false;
	// Выполняем сброс смещения разбора
	this->_offset = 0;
	// Выполняем сброс количества отброшенных октетов
	this->_origin = 0;
	// Выполняем сброс буфера поданных октетов
	this->_buffer.clear();
	// Выполняем сброс снятого заголовка опознания контейнера
	this->_header = header_t();
}
/**
 * @brief Метод проверки снятости заголовка опознания контейнера
 *
 * @return признак снятого заголовка
 *
 */
bool awh::codec::abc::Loader::ready() const noexcept {
	// Выводим признак снятого заголовка опознания контейнера
	return this->_ready;
}
/**
 * @brief Метод извлечения снятого заголовка опознания контейнера
 *
 * @return снятый заголовок опознания
 *
 */
const awh::codec::abc::header_t & awh::codec::abc::Loader::header() const noexcept {
	// Выводим снятый заголовок опознания контейнера
	return this->_header;
}
/**
 * @brief Метод извлечения кода отказа снятия контейнера
 *
 * @return код отказа
 *
 */
awh::codec::abc::error_t awh::codec::abc::Loader::error() const noexcept {
	// Выводим код отказа снятия контейнера
	return this->_error;
}
/**
 * @brief Метод извлечения модуля снятия кадра
 *
 * @return модуль снятия кадра
 *
 */
awh::codec::abc::packer_t & awh::codec::abc::Loader::packer() noexcept {
	// Выводим модуль снятия кадра
	return this->_packer;
}
/**
 * @brief Конструктор
 *
 */
awh::codec::abc::Loader::Loader() noexcept :
 _error(error_t::NONE), _ready(false), _offset(0), _origin(0) {}

/**
 * @brief Функция поверки подписи владельца контейнера
 *
 * @param crypto модуль шифрования, отданный потребителем
 * @param name   имя ключа владельца контейнера
 * @param buffer буфер поданных октетов контейнера
 * @param size   размер поданных октетов контейнера
 * @param error  код отказа, если поверка не удалась
 * @return       признак сошедшейся подписи владельца
 *
 */
bool awh::codec::abc::verify(const crypto_t & crypto, const string & name,
 const void * buffer, const size_t size, error_t & error) noexcept {
	// Выполняем сброс кода отказа поверки подписи
	error = error_t::NONE;
	/**
	 * Если октеты контейнера нам не переданы
	 */
	if((buffer == nullptr) || (size == 0)){
		// Выполняем установку кода отказа поверки подписи
		error = error_t::INTERNAL;
		// Выводим признак несошедшейся подписи
		return false;
	}
	// Выполняем получение указателя на поданные октеты контейнера
	const uint8_t * octets = reinterpret_cast <const uint8_t *> (buffer);
	// Заголовок опознания поверяемого контейнера
	header_t header;
	/**
	 * Если снять заголовок опознания контейнера не вышло
	 */
	if(!header.unpack(octets, size, error))
		// Выводим признак несошедшейся подписи
		return false;
	/**
	 * Если подпись владельца контейнером не объявлена
	 */
	if(!header.is(flag_t::SIGNED) || (header.signature == 0)){
		// Выполняем установку кода отказа отсутствия подписи
		error = error_t::UNSIGNED_CONTAINER;
		// Выводим признак несошедшейся подписи
		return false;
	}
	/**
	 * Если объявленное смещение подписи лежит за поданными октетами
	 */
	if(header.signature >= static_cast <uint64_t> (size)){
		// Выполняем установку кода отказа обрыва подписи
		error = error_t::TRUNCATED_SIGNATURE;
		// Выводим признак несошедшейся подписи
		return false;
	}
	// Дерево свёрток по кадрам поверяемого контейнера
	merkle_t merkle;
	// Выполняем установку модуля шифрования дереву свёрток
	merkle.crypto(& crypto);
	/**
	 * Выполняем перебор всех кадров контейнера, от начала тела до подписи.
	 *
	 * Кадры сводятся в дерево по октетам, как они лежат: подписан шифротекст, и
	 * ключа расшифровки поверка не требует вовсе
	 */
	for(uint64_t offset = HEADER_LENGTH; offset < header.signature;){
		/**
		 * Если поданных октетов недостаёт на заголовок кадра
		 */
		if((offset + CHUNK_HEADER) > header.signature){
			// Выполняем установку кода отказа обрыва кадра
			error = error_t::TRUNCATED_CHUNK;
			// Выводим признак несошедшейся подписи
			return false;
		}
		// Собираемая длина уложенного содержимого кадра
		uint64_t length = 0;
		/**
		 * Выполняем сборку длины уложенного содержимого кадра
		 */
		for(uint8_t i = 0; i < 4; i++)
			// Выполняем сборку значения из очередного октета записи
			length |= (static_cast <uint64_t> (octets[offset + 4 + i]) << (i * 8));
		/**
		 * Если объявленная длина кадра выходит за подпись
		 */
		if((offset + CHUNK_HEADER + length) > header.signature){
			// Выполняем установку кода отказа обрыва кадра
			error = error_t::TRUNCATED_CHUNK;
			// Выводим признак несошедшейся подписи
			return false;
		}
		/**
		 * Если внести кадр свёрткой в дерево не вышло
		 */
		if(!merkle.add(octets + offset, static_cast <size_t> (CHUNK_HEADER + length))){
			// Выполняем установку кода отказа выработки свёртки
			error = error_t::SIGNING_FAILED;
			// Выводим признак несошедшейся подписи
			return false;
		}
		// Выполняем сдвиг смещения разбора на длину кадра
		offset += (CHUNK_HEADER + length);
	}
	// Снятая подпись владельца контейнера
	sign_t sign;
	/**
	 * Если снять запись подписи владельца контейнера не вышло
	 */
	if(!abc::unpack(octets + header.signature, size - static_cast <size_t> (header.signature), sign, error))
		// Выводим признак несошедшейся подписи
		return false;
	// Корень дерева свёрток поверяемого контейнера
	vector <uint8_t> root;
	/**
	 * Если свести дерево свёрток к корню не вышло
	 */
	if(!merkle.root(root)){
		// Выполняем установку кода отказа выработки свёртки
		error = error_t::SIGNING_FAILED;
		// Выводим признак несошедшейся подписи
		return false;
	}
	/**
	 * Если корень дерева разошёлся с подписанным, содержимое контейнера правлено
	 */
	if(root != sign.root){
		// Выполняем установку кода отказа несошедшейся подписи
		error = error_t::REFUSED_SIGNATURE;
		// Выводим признак несошедшейся подписи
		return false;
	}
	/**
	 * Если подпись корня дерева не сошлась, корень подписан не тем ключом
	 */
	if(!crypto.verify(name, sign.root.data(), sign.root.size(), sign.signature, sign.hash)){
		// Выполняем установку кода отказа несошедшейся подписи
		error = error_t::REFUSED_SIGNATURE;
		// Выводим признак несошедшейся подписи
		return false;
	}
	// Выводим признак сошедшейся подписи владельца контейнера
	return true;
}
