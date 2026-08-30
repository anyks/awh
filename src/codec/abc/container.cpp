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
 * @brief Метод объявления отказа сборки контейнера
 *
 * @param error объявляемый код отказа
 * @return      признак успешности, всегда ложь
 *
 */
bool awh::codec::abc::Assembler::fail(const error_t error) noexcept {
	// Выполняем установку кода отказа
	this->_error = error;
	/**
	 * Если объект логирования отдан, доносим об отказе
	 */
	/**
	 * @warning Сброс кода отказа сюда НЕ идёт: воронка эта объявляет отказ, а сброс
	 *          лишь снимает прежний, и донесение о нём наполняло бы журнал записями
	 *          «no error» на всякий успешный вызов. Проверено на себе
	 */
	if((error != error_t::NONE) && (this->_log != nullptr)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("ABC: %s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (error)),
			 log_t::flag_t::WARNING, abc::message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("ABC: %s", log_t::flag_t::WARNING, abc::message(error));
		#endif
	}
	// Сообщаем, что работа отвечена отказом
	return false;
}
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
	writer_t writer(this->_log);
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
		this->fail(error_t::EMPTY_RECORD);
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
		this->fail(error_t::INVALID_LENGTH);
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
	 *
	 * @note Внесение это стоит ПОСЛЕ правки оглавления, и отката при отказе его нет.
	 *       Разобрано 31.08.2026 и оставлено намеренно - порядок этот верен по трём
	 *       доводам разом, а не по недосмотру:
	 *
	 *       1. Повторная попытка укладки строк НЕ удваивает: `_marks` очищен, и обход
	 *          выше вносить более нечего;
	 *       2. Смещение кадра `entry.chunk` берётся от `_body.size()`, а тело растёт
	 *          лишь на пути успеха - НИЖЕ этого отказа. Строки, внесённые попыткою
	 *          неудавшейся, несут ровно то смещение, какое кадр и получит удавшейся;
	 *       3. Отказ этот через открытый API недостижим вовсе: подписыватель и модуль
	 *          шифрования дерева задаются ОДНИМ вызовом `signer()`, и разойтись им
	 *          нечем, а иных причин отказа у внесения нет.
	 *
	 *       Довод второй и есть тот, на каком порядок держится; переставь кто внесение
	 *       в тело выше этого места - и строки неудавшейся попытки станут указывать на
	 *       кадр чужой
	 */
	if(this->_signer != nullptr){
		/**
		 * Если внести кадр свёрткой в дерево не вышло
		 */
		if(!this->_merkle.add(chunk.data(), chunk.size())){
			// Выполняем установку кода отказа выработки свёртки
			this->fail(error_t::SIGNING_FAILED);
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
	if(this->_settings.indexed){
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
		/**
		 * Выполняем объявление длины содержимого кадра оглавления: заголовок несёт
		 * контрольную сумму, и длина, им объявленная, поверяет прочитанную из кадра
		 */
		this->_header.extent = static_cast <uint32_t> (tail.size() - CHUNK_HEADER);
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
		/**
		 * Если снять свёртку по кадрам контейнера не вышло
		 *
		 * @note Кадр оглавления подаётся свёртке отдельно, а его может и не быть вовсе -
		 *       оглавление отключаемо, да и пустому контейнеру его не из чего собрать.
		 *       Подача пустого кадра отвечала отказом, и подпись выходила недостижимой у
		 *       всякого контейнера без оглавления
		 */
		if(!(tail.empty() ? this->_merkle.root(sign.root)
		 : this->_merkle.root(sign.root, tail.data(), tail.size()))){
			// Выполняем установку кода отказа выработки свёртки
			this->fail(error_t::SIGNING_FAILED);
			// Выводим признак неудачной сборки контейнера
			return false;
		}
		/**
		 * Если выработать подпись корня дерева не вышло
		 */
		if(!this->_signer->sign(this->_name, sign.root.data(), sign.root.size(), sign.hash, sign.signature) ||
		 sign.signature.empty() || (sign.signature.size() > 0xFFFF) || (sign.root.size() > 0xFFFF)){
			// Выполняем установку кода отказа выработки подписи
			this->fail(error_t::SIGNING_FAILED);
			// Выводим признак неудачной сборки контейнера
			return false;
		}
		// Выполняем укладку записи подписи владельца контейнера
		abc::pack(sign, signature);
		/**
		 * Выполняем обёртку записи подписи кадром: тело контейнера обходится кадрами
		 * подряд, и запись подписи, оказавшись внутри обхода при следующей фиксации,
		 * обязана быть кадром, а не голой записью
		 */
		if(!abc::envelope(signature, this->_number, static_cast <uint32_t> (this->_header.generation))){
			// Выполняем установку кода отказа подписи
			this->fail(error_t::SIGNING_FAILED);
			// Сообщаем, что завершение сборки отвечено отказом
			return false;
		}
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
	/**
	 * Выполняем очистку поданного буфера
	 *
	 * @note Укладка заголовка ДОПИСЫВАЕТ к буферу намеренно - правке она кладёт головной
	 *       и хвостовой заголовки в один буфер, - и без очистки здесь контейнер начался бы
	 *       не с метки, а с того, что потребитель оставил в буфере прежде. Очистка запаса
	 *       буфера не отбирает, потому и стоит перед отведением
	 */
	result.clear();
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
 * @param log объект для работы с логами
 *
 */
awh::codec::abc::Assembler::Assembler(const log_t * log) noexcept :
 _packer(log), _error(error_t::NONE), _kind(payload_t::MIXED), _index(log), _records(0), _number(0),
 _compressed(false), _merkle(log), _signer(nullptr), _hash(crypto_t::hash_t::SHA256), _log(log) {}

/**
 * @brief Метод объявления отказа снятия контейнера
 *
 * @param error объявляемый код отказа
 * @return      признак успешности, всегда ложь
 *
 */
bool awh::codec::abc::Loader::fail(const error_t error) noexcept {
	// Выполняем установку кода отказа
	this->_error = error;
	/**
	 * Если объект логирования отдан, доносим об отказе
	 */
	/**
	 * @warning Сброс кода отказа сюда НЕ идёт: воронка эта объявляет отказ, а сброс
	 *          лишь снимает прежний, и донесение о нём наполняло бы журнал записями
	 *          «no error» на всякий успешный вызов. Проверено на себе
	 */
	if((error != error_t::NONE) && (this->_log != nullptr)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("ABC: %s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (error)),
			 log_t::flag_t::WARNING, abc::message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("ABC: %s", log_t::flag_t::WARNING, abc::message(error));
		#endif
	}
	// Сообщаем, что работа отвечена отказом
	return false;
}
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
		this->fail(error_t::INTERNAL);
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
			this->fail(error_t::TRUNCATED_HEADER);
			// Выводим признак того, что кадр не выдан
			return false;
		}
		/**
		 * Если снять заголовок опознания контейнера не вышло
		 */
		{
			// Код отказа снятия заголовка опознания
			error_t error = error_t::NONE;
			/**
			 * Если снять заголовок опознания контейнера не вышло
			 */
			if(!this->_header.unpack(this->_buffer.data() + this->_offset, HEADER_LENGTH, error))
				// Выводим признак того, что кадр не выдан
				return this->fail(error);
		}
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
			this->fail(error_t::INVALID_CHUNK);
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
 * @brief Метод проверки исчерпанности тела контейнера
 *
 * @return признак того, что тело контейнера подано целиком
 *
 */
bool awh::codec::abc::Loader::complete() const noexcept {
	/**
	 * Если заголовок опознания контейнера ещё не снят
	 */
	if(!this->_ready)
		// Выводим признак неисчерпанного тела контейнера
		return false;
	// Выводим признак того, что подано октетов не менее объявленной длины тела
	return ((this->_origin + static_cast <uint64_t> (this->_offset)) >=
	 (HEADER_LENGTH + this->_header.length));
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
 * @param log объект для работы с логами
 *
 */
awh::codec::abc::Loader::Loader(const log_t * log) noexcept :
 _packer(log), _error(error_t::NONE), _ready(false), _offset(0), _origin(0), _log(log) {}

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
/**
 * @brief Пространство имён работ, доступных лишь этому файлу
 *
 */
namespace {
	/**
	 * @brief Функция объявления отказа поверки подписи
	 *
	 * @details Поверка есть свободная функция, и своего кода отказа у неё нет: код уходит
	 *          доводом наружу. Воронка эта сводит выдачу кода и донесение о нём в одно
	 *          место, как то сделано у работающих классов кодека
	 *
	 * @param error объявляемый код отказа
	 * @param out   довод, куда следует уложить код отказа
	 * @param log   объект работы с логами
	 * @return      признак успешности, всегда ложь
	 *
	 */
	bool refuse(const awh::codec::abc::error_t error, awh::codec::abc::error_t & out, const awh::log_t * log) noexcept {
		// Выполняем установку кода отказа поверки
		out = error;
		/**
		 * Если объект логирования отдан, доносим об отказе поверки
		 */
		if(log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log->debug("ABC: %s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (error)),
				 awh::log_t::flag_t::WARNING, awh::codec::abc::message(error));
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log->print("ABC: %s", awh::log_t::flag_t::WARNING, awh::codec::abc::message(error));
			#endif
		}
		// Сообщаем, что поверка отвечена отказом
		return false;
	}
	
};

bool awh::codec::abc::verify(const crypto_t & crypto, const string & name,
 const void * buffer, const size_t size, error_t & error, const log_t * log) noexcept {
	// Выполняем сброс кода отказа поверки подписи
	error = error_t::NONE;
	/**
	 * Если октеты контейнера нам не переданы
	 */
	if((buffer == nullptr) || (size == 0)){
		// Выводим признак несошедшейся подписи
		return ::refuse(error_t::INTERNAL, error, log);
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
		return ::refuse(error, error, log);
	/**
	 * Если подпись владельца контейнером не объявлена
	 */
	if(!header.is(flag_t::SIGNED) || (header.signature == 0)){
		// Выводим признак несошедшейся подписи
		return ::refuse(error_t::UNSIGNED_CONTAINER, error, log);
	}
	/**
	 * Если объявленное смещение подписи лежит за поданными октетами
	 */
	if(header.signature >= static_cast <uint64_t> (size)){
		// Выводим признак несошедшейся подписи
		return ::refuse(error_t::TRUNCATED_SIGNATURE, error, log);
	}
	// Дерево свёрток по кадрам поверяемого контейнера
	merkle_t merkle(log);
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
			// Выводим признак несошедшейся подписи
			return ::refuse(error_t::TRUNCATED_CHUNK, error, log);
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
			// Выводим признак несошедшейся подписи
			return ::refuse(error_t::TRUNCATED_CHUNK, error, log);
		}
		/**
		 * Если внести кадр свёрткой в дерево не вышло
		 */
		if(!merkle.add(octets + offset, static_cast <size_t> (CHUNK_HEADER + length))){
			// Выводим признак несошедшейся подписи
			return ::refuse(error_t::SIGNING_FAILED, error, log);
		}
		// Выполняем сдвиг смещения разбора на длину кадра
		offset += (CHUNK_HEADER + length);
	}
	// Снятая подпись владельца контейнера
	sign_t sign;
	/**
	 * Если снять запись подписи владельца контейнера не вышло
	 */
	/**
	 * Если поданных октетов недостаёт на заголовок кадра записи подписи
	 */
	if((header.signature + CHUNK_HEADER) > static_cast <uint64_t> (size)){
		// Выводим признак несошедшейся подписи
		return ::refuse(error_t::TRUNCATED_SIGNATURE, error, log);
	}
	/**
	 * Выполняем снятие записи подписи из кадра-обёртки: смещение подписи указывает на
	 * заголовок кадра, а сама запись лежит за ним
	 */
	if(!abc::unpack(octets + header.signature + CHUNK_HEADER,
	 size - static_cast <size_t> (header.signature + CHUNK_HEADER), sign, error))
		// Выводим признак несошедшейся подписи
		return ::refuse(error, error, log);
	// Корень дерева свёрток поверяемого контейнера
	vector <uint8_t> root;
	/**
	 * Если свести дерево свёрток к корню не вышло
	 */
	if(!merkle.root(root)){
		// Выводим признак несошедшейся подписи
		return ::refuse(error_t::SIGNING_FAILED, error, log);
	}
	/**
	 * Если корень дерева разошёлся с подписанным, содержимое контейнера правлено
	 */
	if(root != sign.root){
		// Выводим признак несошедшейся подписи
		return ::refuse(error_t::REFUSED_SIGNATURE, error, log);
	}
	/**
	 * Если подпись корня дерева не сошлась, корень подписан не тем ключом
	 */
	if(!crypto.verify(name, sign.root.data(), sign.root.size(), sign.signature, sign.hash)){
		// Выводим признак несошедшейся подписи
		return ::refuse(error_t::REFUSED_SIGNATURE, error, log);
	}
	// Выводим признак сошедшейся подписи владельца контейнера
	return true;
}
