/**
 * @file editor.cpp
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
 * @brief Файл реализации правки бинарного контейнера ABC на месте
 *
 * \~english
 * @brief Implementation file of the editing of the ABC binary container in place
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <codec/abc/editor.hpp>

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
 * @brief Пространство имён работ, доступных лишь этому файлу
 *
 */
namespace {
	/**
	 * @brief Функция снятия целого числа установленной ширины с записи
	 *
	 * @param buffer буфер поданной записи
	 * @param width  ширина записи в октетах
	 * @return       снятое значение
	 *
	 */
	uint64_t restore(const uint8_t * buffer, const uint8_t width) noexcept {
		// Собираемое значение
		uint64_t result = 0;
		/**
		 * Выполняем перебор всех октетов записи, от младшего к старшему
		 */
		for(uint8_t i = 0; i < width; i++)
			// Выполняем сборку значения из очередного октета записи
			result |= (static_cast <uint64_t> (buffer[i]) << (i * 8));
		// Выводим собранное значение
		return result;
	}
};

/**
 * @brief Конструктор
 *
 */
awh::codec::abc::Editor::Settings::Settings() noexcept :
 mode(mode_t::MANUAL), block(0x10000), limit(0x100000), delay(0) {}

/**
 * @brief Метод снятия кадра контейнера с носителя
 *
 * @param origin смещение кадра от начала тела контейнера
 * @return       признак успешно снятого кадра
 *
 */
bool awh::codec::abc::Editor::fetch(const uint64_t origin) noexcept {
	/**
	 * Если затребованный кадр уже удерживается, снимать его наново незачем
	 */
	if(this->_cached && (this->_origin == origin))
		// Выводим признак успешно снятого кадра
		return true;
	// Выполняем сброс признака удержания снятого кадра
	this->_cached = false;
	// Буфер вычитанных октетов кадра
	vector <uint8_t> buffer;
	/**
	 * Если вычитать заголовок кадра не вышло
	 */
	if(!this->_source(HEADER_LENGTH + origin, CHUNK_HEADER, buffer) || (buffer.size() < CHUNK_HEADER)){
		// Выполняем установку кода отказа чтения октетов контейнера
		this->_error = error_t::UNREADABLE_SOURCE;
		// Выводим признак неудачного снятия кадра
		return false;
	}
	// Выполняем получение длины уложенного содержимого кадра
	const uint64_t length = restore(buffer.data() + 4, 4);
	// Выполняем очистку буфера вычитанных октетов
	buffer.clear();
	/**
	 * Если вычитать кадр целиком не вышло
	 */
	if(!this->_source(HEADER_LENGTH + origin, static_cast <size_t> (CHUNK_HEADER + length), buffer) ||
	 (buffer.size() < static_cast <size_t> (CHUNK_HEADER + length))){
		// Выполняем установку кода отказа чтения октетов контейнера
		this->_error = error_t::UNREADABLE_SOURCE;
		// Выводим признак неудачного снятия кадра
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
		// Выводим признак неудачного снятия кадра
		return false;
	}
	// Выполняем установку смещения удерживаемого кадра
	this->_origin = origin;
	// Выполняем установку признака удержания снятого кадра
	this->_cached = true;
	// Выводим признак успешно снятого кадра
	return true;
}
/**
 * @brief Метод укладки накопленных записей кадром в память
 *
 * @return признак успешности укладки
 *
 */
bool awh::codec::abc::Editor::pack() noexcept {
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
	if(!this->_packer.pack(this->_pending.data(), this->_pending.size(), this->_kind,
	 this->_number, static_cast <uint32_t> (this->_header.generation + 1), chunk)){
		// Выполняем установку кода отказа укладки кадра
		this->_error = this->_packer.error();
		/**
		 * Накопленное отказом не сбрасывается: причина отказа может быть устранена,
		 * и следующая попытка пройдёт по тем же данным
		 */
		return false;
	}
	/**
	 * Выполняем перебор накопленных правок оглавления
	 */
	for(edit_t & mark : this->_marks){
		/**
		 * Если правка ещё не привязана к уложенному кадру
		 */
		if(mark.batch == numeric_limits <size_t>::max())
			// Выполняем привязку правки к укладываемому кадру
			mark.batch = this->_batches.size();
	}
	// Выполняем внесение уложенного кадра в череду ожидающих записи
	this->_batches.push_back(::std::move(chunk));
	// Выполняем увеличение порядкового номера следующего кадра
	this->_number++;
	// Выполняем очистку накопленных записей
	this->_pending.clear();
	// Выводим признак успешной укладки
	return true;
}
/**
 * @brief Метод установки модуля сжатия
 *
 * @param value устанавливаемый модуль сжатия, ноль - снятие модуля
 *
 */
void awh::codec::abc::Editor::compressor(const compressor::block_t * value) noexcept {
	// Выполняем установку модуля сжатия укладчику кадра
	this->_packer.compressor(value);
}
/**
 * @brief Метод установки модуля шифрования
 *
 * @param value устанавливаемый модуль шифрования, ноль - снятие модуля
 *
 */
void awh::codec::abc::Editor::crypto(const crypto_t * value) noexcept {
	// Выполняем установку модуля шифрования укладчику кадра
	this->_packer.crypto(value);
}
/**
 * @brief Метод открытия контейнера отданными работами чтения и записи
 *
 * @param source устанавливаемая работа чтения октетов контейнера
 * @param sink   устанавливаемая работа записи октетов контейнера
 * @param length полная длина контейнера на носителе
 * @return       признак успешно открытого контейнера
 *
 */
bool awh::codec::abc::Editor::open(source_t source, sink_t sink, const uint64_t length) noexcept {
	// Выполняем захват замка состояния правки контейнера
	lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем сброс состояния правки контейнера
	this->reset();
	/**
	 * Если работы чтения либо записи октетов контейнера нам не переданы
	 */
	if((source == nullptr) || (sink == nullptr)){
		// Выполняем установку кода отказа открытия контейнера
		this->_error = error_t::INTERNAL;
		// Выводим признак неудачного открытия контейнера
		return false;
	}
	// Выполняем установку работы чтения октетов контейнера
	this->_source = ::std::move(source);
	// Выполняем установку работы записи октетов контейнера
	this->_sink = ::std::move(sink);
	// Выполняем установку полной длины контейнера на носителе
	this->_length = length;
	// Буфер вычитанных октетов заголовка опознания контейнера
	vector <uint8_t> buffer;
	/**
	 * Если вычитать головной заголовок опознания контейнера не вышло
	 */
	if(!this->_source(0, HEADER_LENGTH, buffer) || (buffer.size() < HEADER_LENGTH)){
		// Выполняем установку кода отказа чтения октетов контейнера
		this->_error = error_t::UNREADABLE_SOURCE;
		// Выводим признак неудачного открытия контейнера
		return false;
	}
	/**
	 * Если хвостовой заголовок опознания на носителе поместился, выполняем чтение его
	 */
	if(length >= static_cast <uint64_t> (HEADER_LENGTH * 2)){
		// Буфер вычитанных октетов хвостового заголовка опознания
		vector <uint8_t> tail;
		/**
		 * Если хвостовой заголовок вычитан и опознан
		 */
		if(this->_source(length - HEADER_LENGTH, HEADER_LENGTH, tail) && probe(tail.data(), tail.size())){
			// Выполняем установку признака наличия хвостового заголовка
			this->_tailed = true;
			/**
			 * Если головной заголовок опознан не был, берём хвостовой: негодный
			 * головной значит обрыв посреди правки, и контейнер откатывается к
			 * поколению, какое было до неё
			 */
			if(!probe(buffer.data(), buffer.size()))
				// Выполняем перенесение хвостового заголовка головному
				buffer = ::std::move(tail);
		}
	}
	/**
	 * Если снять заголовок опознания контейнера не вышло
	 */
	if(!this->_header.unpack(buffer.data(), buffer.size(), this->_error))
		// Выводим признак неудачного открытия контейнера
		return false;
	/**
	 * Если оглавление контейнера заголовком не объявлено, правка невозможна:
	 * без оглавления неведомо, где какая запись лежит
	 */
	if(this->_header.index == 0){
		// Выполняем установку кода отказа отсутствия оглавления
		this->_error = error_t::MISSING_INDEX;
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
		this->_error = error_t::UNREADABLE_SOURCE;
		// Выводим признак неудачного открытия контейнера
		return false;
	}
	// Выполняем получение длины уложенного содержимого кадра оглавления
	const uint64_t packed = restore(buffer.data() + 4, 4);
	// Выполняем очистку буфера вычитанных октетов
	buffer.clear();
	/**
	 * Если вычитать кадр оглавления целиком не вышло
	 */
	if(!this->_source(this->_header.index, static_cast <size_t> (CHUNK_HEADER + packed), buffer) ||
	 (buffer.size() < static_cast <size_t> (CHUNK_HEADER + packed))){
		// Выполняем установку кода отказа чтения октетов контейнера
		this->_error = error_t::UNREADABLE_SOURCE;
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
	if(!this->_index.unpack(payload.data(), payload.size(), this->_error))
		// Выводим признак неудачного открытия контейнера
		return false;
	/**
	 * Выполняем установку порядкового номера следующего кадра счётом записей:
	 * номер этот лишь бы рос, а сквозным счётом кадров он не ведётся
	 */
	this->_number = this->_header.records;
	// Выполняем установку признака открытого контейнера
	this->_opened = true;
	// Выводим признак успешно открытого контейнера
	return true;
}
/**
 * @brief Метод внесения записи в конец контейнера
 *
 * @param buffer буфер вносимой записи
 * @param size   размер вносимой записи
 * @param kind   вид содержимого вносимой записи
 * @return       признак успешности внесения
 *
 */
bool awh::codec::abc::Editor::add(const void * buffer, const size_t size,
 const payload_t kind, const bool added, const uint64_t number) noexcept {
	// Выполняем захват замка состояния правки контейнера
	lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем сброс кода отказа правки контейнера
	this->_error = error_t::NONE;
	/**
	 * Если контейнер ещё не открыт
	 */
	if(!this->_opened){
		// Выполняем установку кода отказа накопления записи
		this->_error = error_t::INTERNAL;
		// Выводим признак неудачного накопления записи
		return false;
	}
	/**
	 * Если запись нам не передана
	 */
	if((buffer == nullptr) || (size == 0)){
		// Выполняем установку кода отказа накопления записи
		this->_error = error_t::EMPTY_RECORD;
		// Выводим признак неудачного накопления записи
		return false;
	}
	/**
	 * Если вид содержимого сменился, выполняем укладку накопленного кадром
	 */
	if(!this->_pending.empty() && (kind != this->_kind)){
		/**
		 * Если уложить накопленное кадром не вышло
		 */
		if(!this->pack())
			// Выводим признак неудачного накопления записи
			return false;
	}
	// Выполняем установку вида содержимого накопленных записей
	this->_kind = kind;
	// Заводимая правка оглавления
	edit_t mark;
	// Выполняем установку смещения записи в содержимом кадра
	mark.entry.offset = static_cast <uint32_t> (this->_pending.size());
	// Выполняем установку длины накопляемой записи
	mark.entry.length = static_cast <uint32_t> (size);
	// Выполняем установку признака того, что запись вносится, а не правится
	mark.added = added;
	// Выполняем установку номера правимой строки оглавления
	mark.number = number;
	// Выполняем объявление того, что запись кадром ещё не уложена
	mark.batch = numeric_limits <size_t>::max();
	// Выполняем внесение заведённой правки оглавления
	this->_marks.push_back(mark);
	// Выполняем накопление поданной записи
	this->_pending.insert(this->_pending.end(),
	 reinterpret_cast <const uint8_t *> (buffer), reinterpret_cast <const uint8_t *> (buffer) + size);
	// Выполняем объявление наличия незакреплённых правок
	this->_dirty = true;
	/**
	 * Если накоплено записей больше порога, выполняем укладку их кадром
	 */
	if(this->_pending.size() >= this->_settings.block){
		/**
		 * Если уложить накопленное кадром не вышло
		 */
		if(!this->pack())
			// Выводим признак неудачного накопления записи
			return false;
	}
	/**
	 * Если способ фиксации велит закреплять правки по размеру их
	 */
	if((this->_settings.mode == mode_t::SIZE) && (this->pending() >= this->_settings.limit))
		// Выводим результат самочинной фиксации накопленных правок
		return this->commit();
	/**
	 * Если способ фиксации велит закреплять правки по количеству их
	 */
	if((this->_settings.mode == mode_t::RECORDS) && (this->_marks.size() >= this->_settings.limit))
		// Выводим результат самочинной фиксации накопленных правок
		return this->commit();
	/**
	 * Если способ фиксации велит поверять срок при обращении и срок наступил
	 */
	if((this->_settings.mode == mode_t::DEADLINE) && this->_schedule.touch())
		// Выводим результат самочинной фиксации накопленных правок
		return this->commit();
	// Выводим признак успешного накопления записи
	return true;
}
/**
 * @brief Метод внесения записи в конец контейнера
 *
 * @param buffer буфер вносимой записи
 * @param size   размер вносимой записи
 * @param kind   вид содержимого вносимой записи
 * @return       признак успешности внесения
 *
 */
bool awh::codec::abc::Editor::append(const void * buffer, const size_t size, const payload_t kind) noexcept {
	// Выполняем захват замка состояния правки контейнера
	lock_guard <recursive_mutex> lock(this->_mtx);
	// Выводим результат накопления вносимой записи
	return this->add(buffer, size, kind, true, 0);
}
/**
 * @brief Метод правки записи контейнера по номеру
 *
 * @param number порядковый номер правимой записи
 * @param buffer буфер новой записи
 * @param size   размер новой записи
 * @param kind   вид содержимого новой записи
 * @return       признак успешности правки
 *
 */
bool awh::codec::abc::Editor::replace(const uint64_t number, const void * buffer,
 const size_t size, const payload_t kind) noexcept {
	// Выполняем захват замка состояния правки контейнера
	lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем сброс кода отказа правки контейнера
	this->_error = error_t::NONE;
	/**
	 * Если правимой записи в контейнере нет
	 */
	if(!this->_opened || (number >= static_cast <uint64_t> (this->_index.size()))){
		// Выполняем установку кода отказа правки записи
		this->_error = (this->_opened ? error_t::INVALID_INDEX : error_t::INTERNAL);
		// Выводим признак неудачной правки записи
		return false;
	}
	/**
	 * Выводим результат накопления новой записи: правка ведётся дописыванием, а
	 * прежние октеты остаются на носителе мусором до уборки
	 */
	return this->add(buffer, size, kind, false, number);
}
/**
 * @brief Метод сноса записи контейнера по номеру
 *
 * @param number порядковый номер сносимой записи
 * @return       признак успешности сноса
 *
 */
bool awh::codec::abc::Editor::erase(const uint64_t number) noexcept {
	// Выполняем захват замка состояния правки контейнера
	lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем сброс кода отказа правки контейнера
	this->_error = error_t::NONE;
	/**
	 * Если сносимой записи в контейнере нет
	 */
	if(!this->_opened || (number >= static_cast <uint64_t> (this->_index.size()))){
		// Выполняем установку кода отказа сноса записи
		this->_error = (this->_opened ? error_t::INVALID_INDEX : error_t::INTERNAL);
		// Выводим признак неудачного сноса записи
		return false;
	}
	// Выполняем получение строки оглавления сносимой записи
	entry_t entry = this->_index.entries().at(static_cast <size_t> (number));
	/**
	 * Если запись снесена ранее, сносить её повторно незачем
	 */
	if(entry.is(mark_t::ERASED))
		// Выводим признак успешного сноса записи
		return true;
	// Выполняем объявление сноса записи правкой контейнера
	entry.set(mark_t::ERASED, true);
	// Выполняем увеличение количества октетов, обращённых в мусор
	this->_garbage += static_cast <uint64_t> (entry.length);
	// Выполняем правку строки оглавления контейнера
	this->_index.replace(number, entry);
	// Выполняем объявление наличия незакреплённых правок
	this->_dirty = true;
	// Выводим признак успешного сноса записи
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
bool awh::codec::abc::Editor::record(const uint64_t number, vector <uint8_t> & result) noexcept {
	// Выполняем захват замка состояния правки контейнера
	lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем сброс кода отказа правки контейнера
	this->_error = error_t::NONE;
	// Выполняем очистку буфера выбираемой записи
	result.clear();
	/**
	 * Если контейнер ещё не открыт
	 */
	if(!this->_opened){
		// Выполняем установку кода отказа выборки записи
		this->_error = error_t::INTERNAL;
		// Выводим признак неудачной выборки записи
		return false;
	}
	// Номер накопленной правки, отвечающей затребованной записи
	size_t found = numeric_limits <size_t>::max();
	/**
	 * Порядковый номер, какой получит следующая внесённая, но ещё не
	 * закреплённая запись: внесённые становятся в конец оглавления по порядку
	 */
	uint64_t added = static_cast <uint64_t> (this->_index.size());
	/**
	 * Выполняем перебор накопленных правок оглавления по порядку: последняя
	 * правка записи и есть нынешнее её содержимое, оттого перебор идёт до конца
	 */
	for(size_t i = 0; i < this->_marks.size(); i++){
		// Выполняем получение очередной накопленной правки оглавления
		const edit_t & mark = this->_marks.at(i);
		// Порядковый номер записи, какой ведает очередная правка
		uint64_t current = mark.number;
		/**
		 * Если правка эта вносит новую запись
		 */
		if(mark.added){
			// Выполняем получение порядкового номера вносимой записи
			current = added;
			// Выполняем увеличение номера следующей вносимой записи
			added++;
		}
		// Если правка эта ведает затребованной записью
		if(current == number)
			// Выполняем установку номера найденной правки
			found = i;
	}
	/**
	 * Если накопленная правка затребованной записи найдена
	 */
	if(found != numeric_limits <size_t>::max()){
		// Выполняем получение найденной накопленной правки оглавления
		const edit_t & mark = this->_marks.at(found);
		/**
		 * Если запись лежит в уложенном кадре, выдать её без снятия кадра нельзя
		 */
		if(mark.batch != numeric_limits <size_t>::max()){
			// Смещение снятия уложенного кадра
			size_t offset = 0;
			// Содержимое снятого кадра
			vector <uint8_t> payload;
			// Сведения о снятом кадре
			chunk_t chunk;
			// Выполняем получение уложенного кадра, несущего запись
			const vector <uint8_t> & storage = this->_batches.at(mark.batch);
			/**
			 * Если снять уложенный кадр не вышло
			 */
			if(!this->_packer.unpack(storage.data(), storage.size(), offset, payload, chunk)){
				// Выполняем установку кода отказа снятия кадра
				this->_error = this->_packer.error();
				// Выводим признак неудачной выборки записи
				return false;
			}
			/**
			 * Если строка правки указывает за содержимое снятого кадра
			 */
			if((static_cast <uint64_t> (mark.entry.offset) + mark.entry.length) > static_cast <uint64_t> (payload.size())){
				// Выполняем установку кода отказа выборки записи
				this->_error = error_t::INVALID_INDEX;
				// Выводим признак неудачной выборки записи
				return false;
			}
			// Выполняем выборку записи из содержимого снятого кадра
			result.assign(payload.begin() + static_cast <ptrdiff_t> (mark.entry.offset),
			 payload.begin() + static_cast <ptrdiff_t> (mark.entry.offset + mark.entry.length));
			// Выводим признак успешно выбранной записи
			return true;
		}
		/**
		 * Выполняем выборку записи из накопления, кадром ещё не уложенного:
		 * накопленное читается наравне с закреплённым, ибо оно уже в памяти
		 */
		result.assign(this->_pending.begin() + static_cast <ptrdiff_t> (mark.entry.offset),
		 this->_pending.begin() + static_cast <ptrdiff_t> (mark.entry.offset + mark.entry.length));
		// Выводим признак успешно выбранной записи
		return true;
	}
	/**
	 * Если затребованной записи в оглавлении контейнера нет
	 */
	if(number >= static_cast <uint64_t> (this->_index.size())){
		// Выполняем установку кода отказа выборки записи
		this->_error = error_t::INVALID_INDEX;
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
		this->_error = error_t::MISSING_RECORD;
		// Выводим признак неудачной выборки записи
		return false;
	}
	/**
	 * Если снять кадр затребованной записи не вышло
	 */
	if(!this->fetch(entry.chunk))
		// Выводим признак неудачной выборки записи
		return false;
	/**
	 * Если строка оглавления указывает за содержимое снятого кадра
	 */
	if((static_cast <uint64_t> (entry.offset) + entry.length) > static_cast <uint64_t> (this->_chunk.size())){
		// Выполняем установку кода отказа выборки записи
		this->_error = error_t::INVALID_INDEX;
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
 * @brief Метод фиксации накопленных правок на носителе
 *
 * @return признак успешности фиксации
 *
 */
bool awh::codec::abc::Editor::commit() noexcept {
	// Выполняем захват замка состояния правки контейнера
	lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем сброс кода отказа правки контейнера
	this->_error = error_t::NONE;
	/**
	 * Если контейнер ещё не открыт
	 */
	if(!this->_opened){
		// Выполняем установку кода отказа фиксации правок
		this->_error = error_t::INTERNAL;
		// Выводим признак неудачной фиксации правок
		return false;
	}
	/**
	 * Если незакреплённых правок нет, закреплять нечего
	 */
	if(!this->_dirty)
		// Выводим признак успешной фиксации правок
		return true;
	/**
	 * Если уложить накопленные записи кадром не вышло
	 */
	if(!this->pack())
		// Выводим признак неудачной фиксации правок
		return false;
	/**
	 * Выполняем получение смещения записи новых кадров: хвостовой заголовок
	 * прежнего поколения перекрывается, ибо пишется наново
	 */
	uint64_t offset = (this->_tailed ? (this->_length - HEADER_LENGTH) : this->_length);
	// Буфер вычитанных октетов заголовка кадра оглавления
	vector <uint8_t> buffer;
	/**
	 * Если вычитать заголовок кадра прежнего оглавления не вышло
	 */
	if(!this->_source(this->_header.index, CHUNK_HEADER, buffer) || (buffer.size() < CHUNK_HEADER)){
		// Выполняем установку кода отказа чтения октетов контейнера
		this->_error = error_t::UNREADABLE_SOURCE;
		// Выводим признак неудачной фиксации правок
		return false;
	}
	/**
	 * Выполняем пометку прежнего оглавления мусором правкой одного октета:
	 * перезапись кадра обязала бы шифровать содержимое наново, ничего в нём не
	 * меняя, а подрядное чтение обязано его пропустить
	 */
	const uint8_t marked = static_cast <uint8_t> (buffer.at(CHUNK_FLAGS) | CHUNK_WASTE);
	/**
	 * Если записать пометку прежнего оглавления не вышло
	 */
	if(!this->_sink(this->_header.index + CHUNK_FLAGS, &marked, 1)){
		// Выполняем установку кода отказа записи октетов контейнера
		this->_error = error_t::UNWRITABLE_SINK;
		// Выводим признак неудачной фиксации правок
		return false;
	}
	// Выполняем увеличение количества октетов, обращённых в мусор
	this->_garbage += (CHUNK_HEADER + restore(buffer.data() + 4, 4));
	/**
	 * Выполняем перебор всех уложенных кадров, ожидающих записи
	 */
	for(size_t i = 0; i < this->_batches.size(); i++){
		/**
		 * Если записать очередной кадр на носитель не вышло
		 */
		if(!this->_sink(offset, this->_batches.at(i).data(), this->_batches.at(i).size())){
			// Выполняем установку кода отказа записи октетов контейнера
			this->_error = error_t::UNWRITABLE_SINK;
			// Выводим признак неудачной фиксации правок
			return false;
		}
		/**
		 * Выполняем перебор накопленных правок оглавления
		 */
		for(edit_t & mark : this->_marks){
			/**
			 * Если правка эта не о записанном кадре
			 */
			if(mark.batch != i)
				// Переходим к следующей накопленной правке оглавления
				continue;
			// Выполняем установку смещения кадра от начала тела контейнера
			mark.entry.chunk = (offset - HEADER_LENGTH);
			/**
			 * Если правка эта вносит новую запись
			 */
			if(mark.added)
				// Выполняем внесение строки в оглавление контейнера
				this->_index.add(mark.entry);
			/**
			 * Иначе выполняем перенаправление строки оглавления на новую запись
			 */
			else {
				/**
				 * Выполняем увеличение количества октетов, обращённых в мусор:
				 * прежняя запись остаётся на носителе до уборки
				 */
				this->_garbage += static_cast <uint64_t> (
				 this->_index.entries().at(static_cast <size_t> (mark.number)).length);
				// Выполняем правку строки оглавления контейнера
				this->_index.replace(mark.number, mark.entry);
			}
		}
		// Выполняем сдвиг смещения записи на длину записанного кадра
		offset += static_cast <uint64_t> (this->_batches.at(i).size());
	}
	// Буфер уложенного оглавления контейнера
	vector <uint8_t> entries;
	// Выполняем укладку оглавления контейнера в октеты
	this->_index.pack(entries);
	// Буфер уложенного кадра оглавления контейнера
	vector <uint8_t> tail;
	/**
	 * Если уложить оглавление кадром не вышло
	 */
	if(!this->_packer.pack(entries.data(), entries.size(), payload_t::NUMERIC,
	 this->_number, static_cast <uint32_t> (this->_header.generation + 1), tail)){
		// Выполняем установку кода отказа укладки кадра оглавления
		this->_error = this->_packer.error();
		// Выводим признак неудачной фиксации правок
		return false;
	}
	/**
	 * Если записать кадр оглавления на носитель не вышло
	 */
	if(!this->_sink(offset, tail.data(), tail.size())){
		// Выполняем установку кода отказа записи октетов контейнера
		this->_error = error_t::UNWRITABLE_SINK;
		// Выводим признак неудачной фиксации правок
		return false;
	}
	// Выполняем установку длины тела контейнера
	this->_header.length = (offset - HEADER_LENGTH);
	// Выполняем установку смещения оглавления контейнера
	this->_header.index = offset;
	// Выполняем установку количества записей контейнера
	this->_header.records = static_cast <uint64_t> (this->_index.size());
	// Выполняем увеличение поколения записи контейнера
	this->_header.generation++;
	// Буфер уложенного заголовка опознания контейнера
	vector <uint8_t> head;
	// Выполняем укладку заголовка опознания контейнера
	this->_header.pack(head);
	/**
	 * Выполняем запись хвостового заголовка опознания прежде головного: обрыв
	 * посреди правки головного оставит контейнер прежнего поколения, а хвостовой
	 * заголовок нового поколения к тому времени уже цел
	 */
	if(!this->_sink(offset + static_cast <uint64_t> (tail.size()), head.data(), head.size())){
		// Выполняем установку кода отказа записи октетов контейнера
		this->_error = error_t::UNWRITABLE_SINK;
		// Выводим признак неудачной фиксации правок
		return false;
	}
	/**
	 * Если записать головной заголовок опознания не вышло
	 */
	if(!this->_sink(0, head.data(), head.size())){
		// Выполняем установку кода отказа записи октетов контейнера
		this->_error = error_t::UNWRITABLE_SINK;
		// Выводим признак неудачной фиксации правок
		return false;
	}
	// Выполняем установку полной длины контейнера на носителе
	this->_length = (offset + static_cast <uint64_t> (tail.size()) + HEADER_LENGTH);
	// Выполняем установку признака наличия хвостового заголовка
	this->_tailed = true;
	// Выполняем очистку уложенных кадров, ожидавших записи
	this->_batches.clear();
	// Выполняем очистку накопленных правок оглавления
	this->_marks.clear();
	// Выполняем снятие признака наличия незакреплённых правок
	this->_dirty = false;
	// Выполняем сброс признака удержания снятого кадра
	this->_cached = false;
	// Выводим признак успешной фиксации правок
	return true;
}
/**
 * @brief Метод уборки мусора перестройкой контейнера
 *
 * @param target работа записи октетов убираемого контейнера
 * @param kind   вид содержимого записей убираемого контейнера
 * @param length полная длина убранного контейнера
 * @return       признак успешности уборки
 *
 */
bool awh::codec::abc::Editor::compact(sink_t target, const payload_t kind, uint64_t & length) noexcept {
	// Выполняем захват замка состояния правки контейнера
	lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем сброс кода отказа правки контейнера
	this->_error = error_t::NONE;
	// Выполняем сброс полной длины убранного контейнера
	length = 0;
	/**
	 * Если контейнер ещё не открыт либо работа записи нам не передана
	 */
	if(!this->_opened || (target == nullptr)){
		// Выполняем установку кода отказа уборки контейнера
		this->_error = error_t::INTERNAL;
		// Выводим признак неудачной уборки контейнера
		return false;
	}
	/**
	 * Если закрепить накопленные правки не вышло, убирать нельзя: уборка по
	 * оглавлению, разошедшемуся с памятью, потеряла бы накопленное
	 */
	if(!this->commit())
		// Выводим признак неудачной уборки контейнера
		return false;
	// Смещение записи убранного контейнера
	uint64_t offset = HEADER_LENGTH;
	// Оглавление убранного контейнера
	index_t index;
	// Накопленные записи убранного контейнера
	vector <uint8_t> pending;
	/**
	 * Номера строк накопленных записей в оглавлении убранного контейнера.
	 *
	 * Строка заводится сразу же, а смещением кадра дополняется по укладке его:
	 * заводить её лишь при укладке значило бы ставить строки снесённых записей
	 * впереди живых и тем перемешать номера
	 */
	vector <uint64_t> marks;
	// Порядковый номер следующего кадра убранного контейнера
	uint64_t number = 0;
	// Буфер уложенного кадра убранного контейнера
	vector <uint8_t> chunk;
	/**
	 * Выполняем перебор всех строк оглавления правимого контейнера
	 */
	for(size_t i = 0; i <= this->_index.size(); i++){
		/**
		 * Если строки перебраны не все, выполняем перенесение очередной живой записи
		 */
		if(i < this->_index.size()){
			// Выполняем получение очередной строки оглавления
			const entry_t & entry = this->_index.entries().at(i);
			/**
			 * Если запись снесена, выполняем перенесение строки её пустою: изъятие
			 * строки сдвинуло бы номера соседей, а номера эти живут и вне контейнера
			 */
			if(entry.is(mark_t::ERASED)){
				// Заводимая строка снесённой записи
				entry_t erased;
				// Выполняем объявление сноса записи
				erased.set(mark_t::ERASED, true);
				// Выполняем внесение строки в оглавление убранного контейнера
				index.add(erased);
				// Переходим к следующей строке оглавления
				continue;
			}
			// Буфер перенесённой записи контейнера
			vector <uint8_t> item;
			/**
			 * Если выбрать очередную живую запись не вышло
			 */
			if(!this->record(static_cast <uint64_t> (i), item))
				// Выводим признак неудачной уборки контейнера
				return false;
			// Заводимая строка перенесённой записи
			entry_t moved;
			// Выполняем установку смещения записи в содержимом кадра
			moved.offset = static_cast <uint32_t> (pending.size());
			// Выполняем установку длины перенесённой записи
			moved.length = static_cast <uint32_t> (item.size());
			// Выполняем внесение номера строки перенесённой записи
			marks.push_back(static_cast <uint64_t> (index.size()));
			// Выполняем внесение строки в оглавление убранного контейнера
			index.add(moved);
			// Выполняем накопление перенесённой записи
			pending.insert(pending.end(), item.begin(), item.end());
		}
		/**
		 * Если накопленного на кадр не набралось, а строки перебраны не все
		 */
		if((pending.size() < this->_settings.block) && (i < this->_index.size()))
			// Переходим к следующей строке оглавления
			continue;
		/**
		 * Если накопленных записей нет, укладывать нечего
		 */
		if(pending.empty())
			// Переходим к следующей строке оглавления
			continue;
		// Выполняем очистку буфера уложенного кадра
		chunk.clear();
		/**
		 * Если уложить накопленные записи кадром не вышло
		 */
		if(!this->_packer.pack(pending.data(), pending.size(), kind, number, 0, chunk)){
			// Выполняем установку кода отказа укладки кадра
			this->_error = this->_packer.error();
			// Выводим признак неудачной уборки контейнера
			return false;
		}
		/**
		 * Если записать уложенный кадр на носитель не вышло
		 */
		if(!target(offset, chunk.data(), chunk.size())){
			// Выполняем установку кода отказа записи октетов контейнера
			this->_error = error_t::UNWRITABLE_SINK;
			// Выводим признак неудачной уборки контейнера
			return false;
		}
		/**
		 * Выполняем перебор номеров строк накопленных записей
		 */
		for(const uint64_t mark : marks){
			// Выполняем получение строки оглавления накопленной записи
			entry_t entry = index.entries().at(static_cast <size_t> (mark));
			// Выполняем установку смещения кадра от начала тела контейнера
			entry.chunk = (offset - HEADER_LENGTH);
			// Выполняем правку строки оглавления убранного контейнера
			index.replace(mark, entry);
		}
		// Выполняем очистку строк оглавления накопленных записей
		marks.clear();
		// Выполняем очистку накопленных записей
		pending.clear();
		// Выполняем сдвиг смещения записи на длину записанного кадра
		offset += static_cast <uint64_t> (chunk.size());
		// Выполняем увеличение порядкового номера следующего кадра
		number++;
	}
	// Буфер уложенного оглавления убранного контейнера
	vector <uint8_t> entries;
	// Выполняем укладку оглавления убранного контейнера в октеты
	index.pack(entries);
	// Буфер уложенного кадра оглавления
	vector <uint8_t> tail;
	/**
	 * Если уложить оглавление кадром не вышло
	 */
	if(!this->_packer.pack(entries.data(), entries.size(), payload_t::NUMERIC, number, 0, tail)){
		// Выполняем установку кода отказа укладки кадра оглавления
		this->_error = this->_packer.error();
		// Выводим признак неудачной уборки контейнера
		return false;
	}
	/**
	 * Если записать кадр оглавления на носитель не вышло
	 */
	if(!target(offset, tail.data(), tail.size())){
		// Выполняем установку кода отказа записи октетов контейнера
		this->_error = error_t::UNWRITABLE_SINK;
		// Выводим признак неудачной уборки контейнера
		return false;
	}
	// Заголовок опознания убранного контейнера
	header_t header = this->_header;
	// Выполняем установку длины тела убранного контейнера
	header.length = (offset - HEADER_LENGTH);
	// Выполняем установку смещения оглавления убранного контейнера
	header.index = offset;
	// Выполняем установку количества записей убранного контейнера
	header.records = static_cast <uint64_t> (index.size());
	// Выполняем увеличение поколения записи контейнера
	header.generation++;
	// Буфер уложенного заголовка опознания
	vector <uint8_t> head;
	// Выполняем укладку заголовка опознания убранного контейнера
	header.pack(head);
	/**
	 * Если записать хвостовой заголовок опознания не вышло
	 */
	if(!target(offset + static_cast <uint64_t> (tail.size()), head.data(), head.size())){
		// Выполняем установку кода отказа записи октетов контейнера
		this->_error = error_t::UNWRITABLE_SINK;
		// Выводим признак неудачной уборки контейнера
		return false;
	}
	/**
	 * Если записать головной заголовок опознания не вышло
	 */
	if(!target(0, head.data(), head.size())){
		// Выполняем установку кода отказа записи октетов контейнера
		this->_error = error_t::UNWRITABLE_SINK;
		// Выводим признак неудачной уборки контейнера
		return false;
	}
	// Выполняем установку полной длины убранного контейнера
	length = (offset + static_cast <uint64_t> (tail.size()) + HEADER_LENGTH);
	// Выводим признак успешной уборки контейнера
	return true;
}
/**
 * @brief Метод сброса состояния правки контейнера
 *
 */
void awh::codec::abc::Editor::reset() noexcept {
	// Выполняем захват замка состояния правки контейнера
	lock_guard <recursive_mutex> lock(this->_mtx);
	// Выполняем сброс кода отказа правки контейнера
	this->_error = error_t::NONE;
	// Выполняем сброс признака открытого контейнера
	this->_opened = false;
	// Выполняем сброс признака наличия хвостового заголовка
	this->_tailed = false;
	// Выполняем сброс признака наличия незакреплённых правок
	this->_dirty = false;
	// Выполняем сброс признака удержания снятого кадра
	this->_cached = false;
	// Выполняем сброс полной длины контейнера на носителе
	this->_length = 0;
	// Выполняем сброс количества октетов, обращённых в мусор
	this->_garbage = 0;
	// Выполняем сброс порядкового номера следующего кадра
	this->_number = 0;
	// Выполняем сброс смещения удерживаемого кадра
	this->_origin = 0;
	// Выполняем сброс вида содержимого накопленных записей
	this->_kind = payload_t::MIXED;
	// Выполняем сброс накопленных записей
	this->_pending.clear();
	// Выполняем сброс накопленных правок оглавления
	this->_marks.clear();
	// Выполняем сброс уложенных кадров, ожидающих записи
	this->_batches.clear();
	// Выполняем сброс содержимого удерживаемого кадра
	this->_chunk.clear();
	// Выполняем сброс оглавления контейнера
	this->_index.clear();
	// Выполняем сброс работы чтения октетов контейнера
	this->_source = nullptr;
	// Выполняем сброс работы записи октетов контейнера
	this->_sink = nullptr;
	// Выполняем сброс заголовка опознания контейнера
	this->_header = header_t();
}
/**
 * @brief Метод извлечения количества записей контейнера
 *
 * @return количество записей контейнера, снесённые в счёт входят
 *
 */
uint64_t awh::codec::abc::Editor::records() const noexcept {
	// Выполняем захват замка состояния правки контейнера
	lock_guard <recursive_mutex> lock(this->_mtx);
	// Количество внесённых, но ещё не закреплённых записей
	uint64_t added = 0;
	/**
	 * Выполняем перебор накопленных правок оглавления
	 */
	for(const edit_t & mark : this->_marks){
		// Если правка эта вносит новую запись
		if(mark.added)
			// Выполняем увеличение количества внесённых записей
			added++;
	}
	// Выводим количество записей контейнера
	return (static_cast <uint64_t> (this->_index.size()) + added);
}
/**
 * @brief Метод извлечения количества октетов, обращённых в мусор
 *
 * @return количество октетов мусора на носителе
 *
 */
uint64_t awh::codec::abc::Editor::garbage() const noexcept {
	// Выполняем захват замка состояния правки контейнера
	lock_guard <recursive_mutex> lock(this->_mtx);
	// Выводим количество октетов мусора на носителе
	return this->_garbage;
}
/**
 * @brief Метод извлечения размера накопленных правок
 *
 * @return размер накопленных записей в октетах
 *
 */
size_t awh::codec::abc::Editor::pending() const noexcept {
	// Выполняем захват замка состояния правки контейнера
	lock_guard <recursive_mutex> lock(this->_mtx);
	// Собираемый размер накопленных правок
	size_t result = this->_pending.size();
	/**
	 * Выполняем перебор всех уложенных кадров, ожидающих записи
	 */
	for(const vector <uint8_t> & batch : this->_batches)
		// Выполняем накопление размера уложенного кадра
		result += batch.size();
	// Выводим размер накопленных правок
	return result;
}
/**
 * @brief Метод извлечения полной длины контейнера на носителе
 *
 * @return полная длина контейнера в октетах
 *
 */
uint64_t awh::codec::abc::Editor::length() const noexcept {
	// Выполняем захват замка состояния правки контейнера
	lock_guard <recursive_mutex> lock(this->_mtx);
	// Выводим полную длину контейнера на носителе
	return this->_length;
}
/**
 * @brief Метод извлечения заголовка опознания правимого контейнера
 *
 * @return заголовок опознания контейнера
 *
 */
const awh::codec::abc::header_t & awh::codec::abc::Editor::header() const noexcept {
	// Выполняем захват замка состояния правки контейнера
	lock_guard <recursive_mutex> lock(this->_mtx);
	// Выводим заголовок опознания правимого контейнера
	return this->_header;
}
/**
 * @brief Метод извлечения оглавления правимого контейнера
 *
 * @return оглавление контейнера
 *
 */
const awh::codec::abc::index_t & awh::codec::abc::Editor::index() const noexcept {
	// Выполняем захват замка состояния правки контейнера
	lock_guard <recursive_mutex> lock(this->_mtx);
	// Выводим оглавление правимого контейнера
	return this->_index;
}
/**
 * @brief Метод извлечения кода отказа правки контейнера
 *
 * @return код отказа
 *
 */
awh::codec::abc::error_t awh::codec::abc::Editor::error() const noexcept {
	// Выводим код отказа правки контейнера
	return this->_error;
}
/**
 * @brief Метод извлечения модуля укладки и снятия кадра
 *
 * @return модуль укладки и снятия кадра
 *
 */
awh::codec::abc::packer_t & awh::codec::abc::Editor::packer() noexcept {
	// Выводим модуль укладки и снятия кадра
	return this->_packer;
}
/**
 * @brief Метод извлечения настроек правки контейнера
 *
 * @return настройки правки контейнера
 *
 */
const awh::codec::abc::Editor::settings_t & awh::codec::abc::Editor::settings() const noexcept {
	// Выводим настройки правки контейнера
	return this->_settings;
}
/**
 * @brief Метод установки настроек правки контейнера
 *
 * @param settings устанавливаемые настройки правки контейнера
 *
 */
void awh::codec::abc::Editor::settings(const settings_t & settings) noexcept {
	{
		// Выполняем захват замка состояния правки контейнера
		lock_guard <recursive_mutex> lock(this->_mtx);
		// Выполняем установку настроек правки контейнера
		this->_settings = settings;
		/**
		 * Если порог накопления записей не установлен, ставим его наименьшим
		 */
		if(this->_settings.block == 0)
			// Выполняем установку наименьшего порога накопления записей
			this->_settings.block = 1;
		/**
		 * Если порог самочинной фиксации не установлен, ставим его наименьшим
		 */
		if(this->_settings.limit == 0)
			// Выполняем установку наименьшего порога самочинной фиксации
			this->_settings.limit = 1;
	}
	/**
	 * Отбой срока правится вне замка намеренно: остановка его дожидается конца
	 * своего потока, а поток тот идёт в фиксацию за тем же замком - держать замок
	 * при остановке значило бы свести оба в затяжку
	 */
	switch(static_cast <uint8_t> (settings.mode)){
		// Если фиксация ведётся своим потоком
		case static_cast <uint8_t> (mode_t::THREAD): {
			// Выполняем установку работы, зовомой по наступлении срока
			this->_schedule.callback([this]() noexcept -> void {
				// Выполняем фиксацию накопленных правок на носителе
				this->commit();
			});
			// Выполняем запуск отбоя срока своим потоком
			this->_schedule.start(schedule_t::mode_t::THREAD, settings.delay);
		} break;
		// Если срок поверяется при обращении
		case static_cast <uint8_t> (mode_t::DEADLINE):
			// Выполняем запуск отбоя срока поверкой при обращении
			this->_schedule.start(schedule_t::mode_t::DEADLINE, settings.delay);
		break;
		// Иначе выполняем остановку отбоя срока
		default: this->_schedule.stop();
	}
}
/**
 * @brief Деструктор
 *
 */
awh::codec::abc::Editor::~Editor() noexcept {
	/**
	 * Выполняем остановку отбоя срока прежде разрушения состояния: поток отбоя
	 * ходит в фиксацию, и разрушать состояние под ним значило бы отдать ему
	 * разрушенное
	 */
	this->_schedule.stop();
}
/**
 * @brief Конструктор
 *
 */
awh::codec::abc::Editor::Editor() noexcept :
 _error(error_t::NONE), _opened(false), _length(0), _garbage(0), _number(0),
 _kind(payload_t::MIXED), _origin(0), _cached(false), _tailed(false), _dirty(false),
 _source(nullptr), _sink(nullptr) {}
