/**
 * @file index.cpp
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
 * @brief Файл реализации оглавления бинарного контейнера ABC
 *
 * \~english
 * @brief Implementation file of the index of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <codec/abc/index.hpp>
#include <codec/abc/encoding.hpp>

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
 * @brief Метод проверки объявленного свойства строки оглавления
 *
 * @param mark проверяемое свойство строки
 * @return     признак объявленности свойства
 *
 */
bool awh::codec::abc::Entry::is(const mark_t mark) const noexcept {
	// Выводим признак объявленности свойства строки оглавления
	return ((this->marks & static_cast <uint32_t> (mark)) != 0);
}
/**
 * @brief Метод объявления свойства строки оглавления
 *
 * @param mark  объявляемое свойство строки
 * @param value устанавливаемое значение свойства
 *
 */
void awh::codec::abc::Entry::set(const mark_t mark, const bool value) noexcept {
	// Если свойство строки следует объявить
	if(value)
		// Выполняем объявление свойства строки оглавления
		this->marks |= static_cast <uint32_t> (mark);
	// Иначе снимаем объявленное свойство строки оглавления
	else this->marks &= ~static_cast <uint32_t> (mark);
}
/**
 * @brief Метод внесения строки оглавления
 *
 * @param entry вносимая строка оглавления
 *
 */
void awh::codec::abc::Index::add(const entry_t & entry) noexcept {
	// Выполняем внесение строки оглавления
	this->_entries.push_back(entry);
}
/**
 * @brief Метод извлечения строк оглавления
 *
 * @return строки оглавления контейнера
 *
 */
const vector <awh::codec::abc::entry_t> & awh::codec::abc::Index::entries() const noexcept {
	// Выводим строки оглавления контейнера
	return this->_entries;
}
/**
 * @brief Метод правки строки оглавления
 *
 * @param number номер правимой строки оглавления
 * @param entry  устанавливаемая строка оглавления
 * @return       признак успешной правки
 *
 */
bool awh::codec::abc::Index::replace(const uint64_t number, const entry_t & entry) noexcept {
	/**
	 * Если правимой строки в оглавлении нет
	 */
	if(number >= static_cast <uint64_t> (this->_entries.size()))
		// Выводим признак неудачной правки строки оглавления
		return false;
	// Выполняем правку строки оглавления
	this->_entries.at(static_cast <size_t> (number)) = entry;
	// Выводим признак успешной правки строки оглавления
	return true;
}
/**
 * @brief Метод извлечения количества строк оглавления
 *
 * @return количество строк оглавления
 *
 */
size_t awh::codec::abc::Index::size() const noexcept {
	// Выводим количество строк оглавления
	return this->_entries.size();
}
/**
 * @brief Метод очистки оглавления контейнера
 *
 */
void awh::codec::abc::Index::clear() noexcept {
	// Выполняем очистку строк оглавления контейнера
	this->_entries.clear();
}
/**
 * @brief Метод укладки оглавления в октеты
 *
 * @param result буфер, куда следует уложить оглавление
 *
 */
void awh::codec::abc::Index::pack(vector <uint8_t> & result) const noexcept {
	// Выполняем получение смещения начала укладываемого оглавления
	const size_t start = result.size();
	// Выполняем заведение места под все строки оглавления
	result.resize(start + (this->_entries.size() * ENTRY_LENGTH), 0);
	/**
	 * Выполняем перебор всех строк оглавления
	 */
	for(size_t i = 0; i < this->_entries.size(); i++){
		// Выполняем получение указателя на укладываемую строку оглавления
		uint8_t * record = result.data() + (start + (i * ENTRY_LENGTH));
		// Выполняем укладку смещения кадра от начала тела контейнера
		abc::fixed(record, this->_entries.at(i).chunk, 8);
		// Выполняем укладку смещения записи в содержимом кадра
		abc::fixed(record + 8, static_cast <uint64_t> (this->_entries.at(i).offset), 4);
		// Выполняем укладку длины записи
		abc::fixed(record + 12, static_cast <uint64_t> (this->_entries.at(i).length), 4);
		// Выполняем укладку разрядов свойств строки оглавления
		abc::fixed(record + 16, static_cast <uint64_t> (this->_entries.at(i).marks), 4);
	}
}
/**
 * @brief Метод снятия оглавления с октетов
 *
 * @param buffer буфер поданных октетов
 * @param size   размер поданных октетов
 * @param error  код отказа, если снять оглавление не удалось
 * @return       признак успешно снятого оглавления
 *
 */
bool awh::codec::abc::Index::unpack(const void * buffer, const size_t size, error_t & error) noexcept {
	// Выполняем сброс кода отказа снятия оглавления
	error = error_t::NONE;
	// Выполняем очистку прежнего оглавления контейнера
	this->_entries.clear();
	/**
	 * Если октеты нам переданы неверно
	 */
	if((buffer == nullptr) && (size > 0)){
		// Выполняем установку кода отказа снятия оглавления
		error = error_t::INTERNAL;
		// Выводим признак неудачного снятия оглавления
		return false;
	}
	/**
	 * Если длина поданных октетов не кратна длине строки оглавления
	 */
	if((size % ENTRY_LENGTH) != 0){
		// Выполняем установку кода отказа снятия оглавления
		error = error_t::INVALID_INDEX;
		// Выводим признак неудачного снятия оглавления
		return false;
	}
	// Выполняем получение указателя на поданные октеты
	const uint8_t * octets = reinterpret_cast <const uint8_t *> (buffer);
	// Выполняем заведение места под все строки оглавления
	this->_entries.reserve(size / ENTRY_LENGTH);
	/**
	 * Выполняем перебор всех строк поданного оглавления
	 */
	for(size_t i = 0; i < (size / ENTRY_LENGTH); i++){
		// Снимаемая строка оглавления
		entry_t entry;
		// Выполняем получение указателя на снимаемую строку оглавления
		const uint8_t * record = octets + (i * ENTRY_LENGTH);
		// Выполняем снятие смещения кадра от начала тела контейнера
		entry.chunk = abc::gather(record, 8);
		// Выполняем снятие смещения записи в содержимом кадра
		entry.offset = static_cast <uint32_t> (abc::gather(record + 8, 4));
		// Выполняем снятие длины записи
		entry.length = static_cast <uint32_t> (abc::gather(record + 12, 4));
		// Выполняем снятие разрядов свойств строки оглавления
		entry.marks = static_cast <uint32_t> (abc::gather(record + 16, 4));
		/**
		 * Если разряды свойств строки несут неведомое.
		 *
		 * Сличение это равняется на сличение разрядов кадра: оба поля приходят с
		 * провода, и оглавление целостностью не защищено вовсе - кадр контрольной
		 * суммы не несёт, а подпись необязательна. Порча старших разрядов без этой
		 * проверки уходила бы обратно в запись при перекладке контейнера.
		 *
		 * Ценою тому - невозможность завести новое свойство строки, подняв одну лишь
		 * младшую версию вида записи: разбор прежней сборки отвергнет такое оглавление
		 * целиком. Свойства строки заводятся подъёмом старшей версии
		 */
		if((entry.marks & ~static_cast <uint32_t> (mark_t::ERASED)) != 0){
			// Выполняем очистку снятого оглавления контейнера
			this->_entries.clear();
			// Выполняем установку кода отказа снятия оглавления
			error = error_t::INVALID_INDEX;
			// Выводим признак неудачного снятия оглавления
			return false;
		}
		/**
		 * Если длина записи нулевая, оглавление повреждено: пустых записей
		 * контейнер не несёт вовсе
		 */
		if((entry.length == 0) && !entry.is(mark_t::ERASED)){
			// Выполняем очистку снятого оглавления контейнера
			this->_entries.clear();
			// Выполняем установку кода отказа снятия оглавления
			error = error_t::INVALID_INDEX;
			// Выводим признак неудачного снятия оглавления
			return false;
		}
		// Выполняем внесение снятой строки оглавления
		this->_entries.push_back(entry);
	}
	// Выводим признак успешно снятого оглавления
	return true;
}

/**
 * @brief Метод объявления отказа выборки записи
 *
 * @param error объявляемый код отказа
 * @return      признак успешности, всегда ложь
 *
 */
bool awh::codec::abc::Fetcher::fail(const error_t error) noexcept {
	// Выполняем установку кода отказа
	this->_error = error;
	/**
	 * Если объект логирования отдан, доносим об отказе.
	 *
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
void awh::codec::abc::Fetcher::compressor(const compressor::block_t * value) noexcept {
	// Выполняем установку модуля сжатия снимателю кадра
	this->_packer.compressor(value);
}
/**
 * @brief Метод установки модуля шифрования
 *
 * @param value устанавливаемый модуль шифрования, ноль - снятие модуля
 *
 */
void awh::codec::abc::Fetcher::crypto(const crypto_t * value) noexcept {
	// Выполняем установку модуля шифрования снимателю кадра
	this->_packer.crypto(value);
}
/**
 * @brief Метод открытия контейнера отданной работой чтения
 *
 * @param source устанавливаемая работа чтения октетов контейнера
 * @return       признак успешно открытого контейнера
 *
 */
bool awh::codec::abc::Fetcher::open(source_t source) noexcept {
	// Выполняем сброс состояния выборки записей
	this->reset();
	/**
	 * Если работа чтения октетов контейнера нам не передана
	 */
	if(source == nullptr){
		// Выполняем установку кода отказа открытия контейнера
		this->fail(error_t::INTERNAL);
		// Выводим признак неудачного открытия контейнера
		return false;
	}
	// Выполняем установку работы чтения октетов контейнера
	this->_source = ::std::move(source);
	// Буфер вычитанных октетов заголовка опознания контейнера
	vector <uint8_t> buffer;
	/**
	 * Если вычитать заголовок опознания контейнера не вышло
	 */
	if(!this->_source(0, HEADER_LENGTH, buffer) || (buffer.size() < HEADER_LENGTH)){
		// Выполняем установку кода отказа чтения октетов контейнера
		this->fail(error_t::UNREADABLE_SOURCE);
		// Выводим признак неудачного открытия контейнера
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
		if(!this->_header.unpack(buffer.data(), buffer.size(), error))
			// Выводим признак неудачного открытия контейнера
			return this->fail(error);
	}
	/**
	 * Если оглавление контейнера заголовком не объявлено, выборка по номеру
	 * невозможна: без оглавления до записи добираются лишь снятием всех кадров
	 */
	if(this->_header.index == 0){
		// Выполняем установку кода отказа отсутствия оглавления
		this->fail(error_t::MISSING_INDEX);
		// Выводим признак неудачного открытия контейнера
		return false;
	}
	// Выполняем очистку буфера вычитанных октетов
	buffer.clear();
	/**
	 * Если вычитать заголовок кадра оглавления не вышло
	 */
	if(!this->_source(this->_header.index, CHUNK_HEADER, buffer) || (buffer.size() < CHUNK_HEADER)){
		// Выполняем установку кода отказа чтения октетов контейнера
		this->fail(error_t::UNREADABLE_SOURCE);
		// Выводим признак неудачного открытия контейнера
		return false;
	}
	/**
	 * Выполняем получение длины уложенного содержимого кадра оглавления:
	 * длина эта объявлена заголовком кадра, и вычитывать оглавление целиком
	 * наугад не приходится
	 */
	const uint64_t length = abc::gather(buffer.data() + 4, 4);
	// Выполняем очистку буфера вычитанных октетов
	buffer.clear();
	/**
	 * Если вычитать кадр оглавления целиком не вышло
	 */
	if(!this->_source(this->_header.index, static_cast <size_t> (CHUNK_HEADER + length), buffer) ||
	 (buffer.size() < static_cast <size_t> (CHUNK_HEADER + length))){
		// Выполняем установку кода отказа чтения октетов контейнера
		this->fail(error_t::UNREADABLE_SOURCE);
		// Выводим признак неудачного открытия контейнера
		return false;
	}
	// Смещение снятия кадра оглавления
	size_t offset = 0;
	// Содержимое снятого кадра оглавления
	vector <uint8_t> payload;
	// Сведения о снятом кадре оглавления
	chunk_t chunk;
	/**
	 * Если снять кадр оглавления не вышло
	 */
	if(!this->_packer.unpack(buffer.data(), buffer.size(), offset, payload, chunk)){
		// Выполняем установку кода отказа снятия кадра оглавления
		this->_error = this->_packer.error();
		// Выводим признак неудачного открытия контейнера
		return false;
	}
	/**
	 * Если снять оглавление контейнера не вышло
	 */
	{
		// Код отказа снятия оглавления контейнера
		error_t error = error_t::NONE;
		/**
		 * Если снять оглавление контейнера не вышло
		 */
		if(!this->_index.unpack(payload.data(), payload.size(), error))
			// Выводим признак неудачного открытия контейнера
			return this->fail(error);
	}
	// Выполняем установку признака открытого контейнера
	this->_opened = true;
	// Выводим признак успешно открытого контейнера
	return true;
}
/**
 * @brief Метод выборки записи контейнера по номеру
 *
 * @param number порядковый номер выбираемой записи
 * @param result буфер, куда следует положить выбранную запись
 * @return       признак успешно выбранной записи
 *
 */
bool awh::codec::abc::Fetcher::record(const uint64_t number, vector <uint8_t> & result) noexcept {
	// Выполняем сброс кода отказа выборки записи
	this->_error = error_t::NONE;
	// Выполняем очистку буфера выбираемой записи
	result.clear();
	/**
	 * Если контейнер ещё не открыт
	 */
	if(!this->_opened){
		// Выполняем установку кода отказа выборки записи
		this->fail(error_t::INTERNAL);
		// Выводим признак неудачной выборки записи
		return false;
	}
	/**
	 * Если затребованной записи в оглавлении контейнера нет
	 */
	if(number >= static_cast <uint64_t> (this->_index.size())){
		// Выполняем установку кода отказа выборки записи
		this->fail(error_t::INVALID_INDEX);
		// Выводим признак неудачной выборки записи
		return false;
	}
	// Выполняем получение строки оглавления затребованной записи
	const entry_t & entry = this->_index.entries().at(static_cast <size_t> (number));
	/**
	 * Если затребованная запись снесена правкой контейнера
	 */
	if(entry.is(mark_t::ERASED)){
		// Выполняем установку кода отказа выборки снесённой записи
		this->fail(error_t::MISSING_RECORD);
		// Выводим признак неудачной выборки записи
		return false;
	}
	/**
	 * Если кадр затребованной записи не удерживается, выполняем снятие его
	 */
	if(!this->_cached || (this->_origin != entry.chunk)){
		// Выполняем сброс признака удержания снятого кадра
		this->_cached = false;
		// Буфер вычитанных октетов кадра
		vector <uint8_t> buffer;
		/**
		 * Если вычитать заголовок кадра не вышло
		 */
		if(!this->_source(HEADER_LENGTH + entry.chunk, CHUNK_HEADER, buffer) || (buffer.size() < CHUNK_HEADER)){
			// Выполняем установку кода отказа чтения октетов контейнера
			this->fail(error_t::UNREADABLE_SOURCE);
			// Выводим признак неудачной выборки записи
			return false;
		}
		// Выполняем получение длины уложенного содержимого кадра
		const uint64_t length = abc::gather(buffer.data() + 4, 4);
		// Выполняем очистку буфера вычитанных октетов
		buffer.clear();
		/**
		 * Если вычитать кадр целиком не вышло
		 */
		if(!this->_source(HEADER_LENGTH + entry.chunk, static_cast <size_t> (CHUNK_HEADER + length), buffer) ||
		 (buffer.size() < static_cast <size_t> (CHUNK_HEADER + length))){
			// Выполняем установку кода отказа чтения октетов контейнера
			this->fail(error_t::UNREADABLE_SOURCE);
			// Выводим признак неудачной выборки записи
			return false;
		}
		// Смещение снятия кадра
		size_t offset = 0;
		// Сведения о снятом кадре
		chunk_t chunk;
		/**
		 * Если снять кадр не вышло
		 */
		if(!this->_packer.unpack(buffer.data(), buffer.size(), offset, this->_chunk, chunk)){
			// Выполняем установку кода отказа снятия кадра
			this->_error = this->_packer.error();
			// Выводим признак неудачной выборки записи
			return false;
		}
		// Выполняем установку смещения удерживаемого кадра
		this->_origin = entry.chunk;
		// Выполняем установку признака удержания снятого кадра
		this->_cached = true;
	}
	/**
	 * Если строка оглавления указывает за содержимое снятого кадра
	 */
	if((static_cast <uint64_t> (entry.offset) + entry.length) > static_cast <uint64_t> (this->_chunk.size())){
		// Выполняем установку кода отказа выборки записи
		this->fail(error_t::INVALID_INDEX);
		// Выводим признак неудачной выборки записи
		return false;
	}
	// Выполняем выборку записи из содержимого снятого кадра
	result.assign(this->_chunk.begin() + static_cast <ptrdiff_t> (entry.offset),
	 this->_chunk.begin() + static_cast <ptrdiff_t> (entry.offset + entry.length));
	// Выводим признак успешно выбранной записи
	return true;
}
/**
 * @brief Метод сброса состояния выборки записей
 *
 */
void awh::codec::abc::Fetcher::reset() noexcept {
	// Выполняем сброс кода отказа выборки записи
	this->_error = error_t::NONE;
	// Выполняем сброс признака открытого контейнера
	this->_opened = false;
	// Выполняем сброс признака удержания снятого кадра
	this->_cached = false;
	// Выполняем сброс смещения удерживаемого кадра
	this->_origin = 0;
	// Выполняем сброс содержимого удерживаемого кадра
	this->_chunk.clear();
	// Выполняем сброс оглавления контейнера
	this->_index.clear();
	// Выполняем сброс работы чтения октетов контейнера
	this->_source = nullptr;
	// Выполняем сброс заголовка опознания контейнера
	this->_header = header_t();
}
/**
 * @brief Метод извлечения количества записей контейнера
 *
 * @return количество записей контейнера
 *
 */
uint64_t awh::codec::abc::Fetcher::records() const noexcept {
	// Выводим количество записей контейнера
	return static_cast <uint64_t> (this->_index.size());
}
/**
 * @brief Метод извлечения снятого заголовка опознания контейнера
 *
 * @return снятый заголовок опознания
 *
 */
const awh::codec::abc::header_t & awh::codec::abc::Fetcher::header() const noexcept {
	// Выводим снятый заголовок опознания контейнера
	return this->_header;
}
/**
 * @brief Метод извлечения снятого оглавления контейнера
 *
 * @return снятое оглавление контейнера
 *
 */
const awh::codec::abc::index_t & awh::codec::abc::Fetcher::index() const noexcept {
	// Выводим снятое оглавление контейнера
	return this->_index;
}
/**
 * @brief Метод извлечения кода отказа выборки записи
 *
 * @return код отказа
 *
 */
awh::codec::abc::error_t awh::codec::abc::Fetcher::error() const noexcept {
	// Выводим код отказа выборки записи
	return this->_error;
}
/**
 * @brief Метод извлечения модуля снятия кадра
 *
 * @return модуль снятия кадра
 *
 */
awh::codec::abc::packer_t & awh::codec::abc::Fetcher::packer() noexcept {
	// Выводим модуль снятия кадра
	return this->_packer;
}
/**
 * @brief Конструктор
 *
 * @param log объект для работы с логами
 *
 */
awh::codec::abc::Fetcher::Fetcher(const log_t * log) noexcept :
 _index(log), _packer(log), _error(error_t::NONE), _opened(false), _cached(false), _origin(0), _source(nullptr), _log(log) {}
