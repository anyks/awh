/**
 * @file chunk.cpp
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
 * @brief Файл реализации кадра бинарного контейнера ABC
 *
 * \~english
 * @brief Implementation file of a chunk of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <codec/abc/chunk.hpp>

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
	 * @brief Разряд объявления зашифрованного содержимого кадра
	 *
	 */
	constexpr uint8_t Encrypted = 0x01;
	/**
	 * @brief Разряд объявления кадра мусором
	 *
	 * @details Кадр, обращённый правкой в мусор, при подрядном чтении пропускается.
	 *          Пометка эта есть правка одного октета, а не перезапись кадра: перезапись
	 *          обязала бы шифровать содержимое наново, ничего в нём не меняя
	 *
	 */
	constexpr uint8_t Waste = awh::codec::abc::CHUNK_WASTE;
	/**
	 * @brief Функция укладки целого числа установленной ширины
	 *
	 * @param buffer буфер, куда следует уложить запись
	 * @param value  укладываемое значение
	 * @param width  ширина записи в октетах
	 *
	 */
	void lay(uint8_t * buffer, const uint64_t value, const uint8_t width) noexcept {
		/**
		 * Выполняем перебор всех октетов записи, от младшего к старшему
		 */
		for(uint8_t i = 0; i < width; i++)
			// Выполняем укладку очередного октета записи
			buffer[i] = static_cast <uint8_t> ((value >> (i * 8)) & 0xFF);
	}
	/**
	 * @brief Функция снятия целого числа установленной ширины
	 *
	 * @param buffer буфер поданной записи
	 * @param width  ширина записи в октетах
	 * @return       снятое значение
	 *
	 */
	uint64_t take(const uint8_t * buffer, const uint8_t width) noexcept {
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
 * @brief Конструктор настроек укладки кадра
 *
 */
awh::codec::abc::Packer::Settings::Settings() noexcept :
 mixed(compressor::method_t::ZSTD), text(compressor::method_t::ZSTD),
 binary(compressor::method_t::LZ4), numeric(compressor::method_t::ZSTD),
 threshold(64), hash(crypto_t::hash_t::SHA256), cipher(crypto_t::cipher_t::AES256), encrypt(false) {}
/**
 * @brief Конструктор
 *
 */
awh::codec::abc::Packer::Packer() noexcept :
 _error(error_t::NONE), _compressor(nullptr), _crypto(nullptr) {}
/**
 * @brief Метод подбора метода сжатия под вид содержимого
 *
 * @param kind вид содержимого кадра
 * @return     метод сжатия содержимого
 *
 */
awh::compressor::method_t awh::codec::abc::Packer::suggest(const payload_t kind) const noexcept {
	/**
	 * Определяем вид содержимого кадра
	 */
	switch(static_cast <uint8_t> (kind)){
		// Если содержимым является знаковый текст
		case static_cast <uint8_t> (payload_t::TEXT): return this->_settings.text;
		// Если содержимым являются сырые октеты
		case static_cast <uint8_t> (payload_t::BINARY): return this->_settings.binary;
		// Если содержимым являются однородные числа
		case static_cast <uint8_t> (payload_t::NUMERIC): return this->_settings.numeric;
	}
	// Выводим метод сжатия содержимого вперемешку
	return this->_settings.mixed;
}
/**
 * @brief Метод установки модуля сжатия
 *
 * @param value устанавливаемый модуль сжатия, ноль - снятие модуля
 *
 */
void awh::codec::abc::Packer::compressor(const compressor::block_t * value) noexcept {
	// Выполняем установку модуля сжатия
	this->_compressor = value;
}
/**
 * @brief Метод установки модуля шифрования
 *
 * @param value устанавливаемый модуль шифрования, ноль - снятие модуля
 *
 */
void awh::codec::abc::Packer::crypto(const crypto_t * value) noexcept {
	// Выполняем установку модуля шифрования
	this->_crypto = value;
}
/**
 * @brief Метод укладки кадра
 *
 * @param buffer     буфер укладываемого содержимого
 * @param size       размер укладываемого содержимого в октетах
 * @param kind       вид укладываемого содержимого
 * @param number     порядковый номер кадра
 * @param generation поколение записи кадра
 * @param result     буфер, куда следует уложить кадр
 * @return           признак успешности укладки
 *
 */
bool awh::codec::abc::Packer::pack(const void * buffer, const size_t size, const payload_t kind,
 const uint64_t number, const uint32_t generation, vector <uint8_t> & result) noexcept {
	// Выполняем сброс кода отказа укладки
	this->_error = error_t::NONE;
	// Если буфер укладываемого содержимого не существует, а октеты объявлены
	if((buffer == nullptr) && (size > 0)){
		// Выполняем установку кода внутреннего отказа
		this->_error = error_t::INTERNAL;
		// Сообщаем, что укладка отвечена отказом
		return false;
	}
	// Если длина укладываемого содержимого не вмещается в запись кадра
	if(size > static_cast <size_t> (numeric_limits <uint32_t>::max())){
		// Выполняем установку кода отказа длины
		this->_error = error_t::INVALID_LENGTH;
		// Сообщаем, что укладка отвечена отказом
		return false;
	}
	// Выполняем получение указателя на укладываемое содержимое
	const uint8_t * octets = reinterpret_cast <const uint8_t *> (buffer);
	// Укладываемое содержимое кадра
	vector <uint8_t> payload(octets, octets + size);
	// Метод сжатия содержимого кадра
	compressor::method_t method = compressor::method_t::NONE;
	// Если содержимое кадра следует сжать
	if((this->_compressor != nullptr) && (size >= this->_settings.threshold)){
		// Выполняем подбор метода сжатия под вид содержимого
		const compressor::method_t suggested = this->suggest(kind);
		// Если метод сжатия подобран и содержимое ему по размеру
		if((suggested != compressor::method_t::NONE) && compressor::fits(size, suggested)){
			// Сжатое содержимое кадра
			vector <uint8_t> compressed;
			// Выполняем сжатие содержимого кадра
			this->_compressor->compress(payload.data(), payload.size(), suggested, compressed);
			/**
			 * Если сжатие дало выигрыш, берём сжатое содержимое.
			 *
			 * Сжатие, выигрыша не давшее, отбрасывается: заголовок метода и словарь
			 * съели бы больше, чем сберегли, а разбор о том не узнал бы вовсе
			 */
			if(!compressed.empty() && (compressed.size() < payload.size())){
				// Выполняем перенесение сжатого содержимого
				payload = std::move(compressed);
				// Выполняем установку метода сжатия содержимого
				method = suggested;
			}
		}
	}
	// Признак того, что содержимое кадра зашифровано
	bool encrypted = false;
	// Если содержимое кадра следует зашифровать
	if(this->_settings.encrypt){
		// Если модуль шифрования не отдан
		if(this->_crypto == nullptr){
			// Выполняем установку кода отказа шифрования
			this->_error = error_t::ENCRYPTION_FAILED;
			// Сообщаем, что укладка отвечена отказом
			return false;
		}
		/**
		 * Выполняем шифрование содержимого кадра своим вызовом.
		 *
		 * Вызов этот заводит свой вектор инициализации: общий поток на все кадры
		 * повторил бы его при перезаписи кадра, а повтор вектора при том же ключе -
		 * это взлом, и по работе программы он не виден вовсе
		 */
		vector <uint8_t> secured = this->_crypto->encrypt <vector <uint8_t>> (payload,
		 this->_settings.hash, this->_settings.cipher);
		// Если шифрование содержимого отвечено отказом
		if(secured.empty() && !payload.empty()){
			// Выполняем установку кода отказа шифрования
			this->_error = error_t::ENCRYPTION_FAILED;
			// Сообщаем, что укладка отвечена отказом
			return false;
		}
		// Выполняем перенесение зашифрованного содержимого
		payload = std::move(secured);
		// Выполняем установку признака зашифрованности содержимого
		encrypted = true;
	}
	// Если длина уложенного содержимого не вмещается в запись кадра
	if(payload.size() > static_cast <size_t> (numeric_limits <uint32_t>::max())){
		// Выполняем установку кода отказа длины
		this->_error = error_t::INVALID_LENGTH;
		// Сообщаем, что укладка отвечена отказом
		return false;
	}
	// Выполняем получение смещения начала укладываемого кадра
	const size_t start = result.size();
	// Выполняем заведение места под заголовок кадра
	/**
	 * Выполняем отведение места под заголовок укладываемого кадра
	 *
	 * @warning Точное отведение памяти под кадр целиком здесь заводить НЕ следует, и
	 *          оно пробовано: буфер этот накопительный - кадры ложатся в него один за
	 *          другим, - и отведение под очередной кадр отменяет удвоение, заставляя
	 *          перекладывать буфер на всяком кадре. Замер 21.08.2026 на OpenBSD, по три
	 *          прогона: 1002-1006 МБ/с сборки контейнера без точного отведения против
	 *          993-999 с ним. Разница мала, но знак её отвечает доводу, а выигрыша нет
	 *          ни здесь, ни на рабочей машине
	 */
	result.resize(start + CHUNK_HEADER, 0);
	// Выполняем получение указателя на заголовок укладываемого кадра
	uint8_t * head = (result.data() + start);
	// Выполняем укладку метода сжатия содержимого кадра
	head[0] = static_cast <uint8_t> (method);
	// Выполняем укладку разрядов кадра
	head[1] = static_cast <uint8_t> (encrypted ? Encrypted : 0x00);
	// Выполняем укладку длины уложенного содержимого кадра
	lay(head + 4, static_cast <uint64_t> (payload.size()), 4);
	// Выполняем укладку длины исходного содержимого кадра
	lay(head + 8, static_cast <uint64_t> (size), 4);
	// Выполняем укладку порядкового номера кадра
	lay(head + 12, number, 8);
	// Выполняем укладку поколения записи кадра
	lay(head + 20, static_cast <uint64_t> (generation), 4);
	// Если содержимое кадра не пусто
	if(!payload.empty())
		// Выполняем укладку содержимого кадра
		result.insert(result.end(), payload.begin(), payload.end());
	// Сообщаем, что укладка успешна
	return true;
}
/**
 * @brief Метод снятия кадра
 *
 * @param buffer буфер поданных октетов
 * @param size   размер поданных октетов
 * @param offset смещение, с какого следует снимать кадр
 * @param result буфер, куда следует положить содержимое кадра
 * @param chunk  снятые сведения о кадре
 * @return       признак успешно снятого кадра
 *
 */
bool awh::codec::abc::Packer::unpack(const void * buffer, const size_t size, size_t & offset,
 vector <uint8_t> & result, chunk_t & chunk) noexcept {
	// Выполняем сброс кода отказа снятия
	this->_error = error_t::NONE;
	// Выполняем очистку буфера содержимого кадра
	result.clear();
	// Выполняем сброс снятых сведений о кадре
	chunk = chunk_t();
	// Если буфер поданных октетов не существует
	if(buffer == nullptr){
		// Выполняем установку кода внутреннего отказа
		this->_error = error_t::INTERNAL;
		// Сообщаем, что кадр не снят
		return false;
	}
	// Если поданных октетов недостаёт на заголовок кадра
	if((size < offset) || ((size - offset) < CHUNK_HEADER)){
		// Выполняем установку кода отказа обрыва кадра
		this->_error = error_t::TRUNCATED_CHUNK;
		// Сообщаем, что кадр не снят
		return false;
	}
	// Выполняем получение указателя на заголовок снимаемого кадра
	const uint8_t * head = (reinterpret_cast <const uint8_t *> (buffer) + offset);
	// Выполняем снятие разрядов кадра
	const uint8_t flags = head[1];
	// Если разряды кадра несут неведомое
	if((flags & static_cast <uint8_t> (~(Encrypted | Waste))) != 0){
		// Выполняем установку кода отказа опознания кадра
		this->_error = error_t::INVALID_CHUNK;
		// Сообщаем, что кадр не снят
		return false;
	}
	// Выполняем снятие длины уложенного содержимого кадра
	const uint32_t length = static_cast <uint32_t> (take(head + 4, 4));
	// Если содержимого кадра в поданных октетах недостаёт
	if((size - offset - CHUNK_HEADER) < static_cast <size_t> (length)){
		// Выполняем установку кода отказа обрыва кадра
		this->_error = error_t::TRUNCATED_CHUNK;
		// Сообщаем, что кадр не снят
		return false;
	}
	/**
	 * Если октет метода сжатия несёт неведомое.
	 *
	 * Сличение это стоит рядом со сличением разрядов кадра: оба октета приходят с
	 * провода и оба обязаны быть опознаны. Неопознанный метод без этой проверки
	 * уходил бы разжатию как настоящий, и порча заголовка отвечалась бы отказом
	 * сжатия, а на пустом содержимом принималась бы вовсе - кадр с любым мусором
	 * в этом октете числился бы годным
	 */
	if(head[0] > static_cast <uint8_t> (compressor::method_t::DENSITY)){
		// Выполняем установку кода отказа опознания кадра
		this->_error = error_t::INVALID_CHUNK;
		// Сообщаем, что кадр не снят
		return false;
	}
	// Выполняем снятие метода сжатия содержимого кадра
	chunk.method = static_cast <compressor::method_t> (head[0]);
	// Выполняем установку длины уложенного содержимого кадра
	chunk.length = length;
	// Выполняем снятие длины исходного содержимого кадра
	chunk.origin = static_cast <uint32_t> (take(head + 8, 4));
	// Выполняем снятие порядкового номера кадра
	chunk.number = take(head + 12, 8);
	// Выполняем снятие поколения записи кадра
	chunk.generation = static_cast <uint32_t> (take(head + 20, 4));
	// Выполняем установку признака зашифрованности содержимого
	chunk.encrypted = ((flags & Encrypted) != 0);
	// Выполняем установку признака того, что кадр обращён в мусор
	chunk.waste = ((flags & Waste) != 0);
	// Выполняем получение указателя на содержимое снимаемого кадра
	const uint8_t * payload = (head + CHUNK_HEADER);
	// Снимаемое содержимое кадра
	vector <uint8_t> content(payload, payload + length);
	// Если содержимое кадра зашифровано
	if(chunk.encrypted){
		// Если модуль шифрования не отдан
		if(this->_crypto == nullptr){
			// Выполняем установку кода отказа шифрования
			this->_error = error_t::ENCRYPTION_FAILED;
			// Сообщаем, что кадр не снят
			return false;
		}
		// Выполняем расшифровку содержимого кадра
		vector <uint8_t> opened = this->_crypto->decrypt <vector <uint8_t>> (content,
		 this->_settings.hash, this->_settings.cipher);
		// Если расшифровка содержимого отвечена отказом
		if(opened.empty() && (chunk.origin > 0)){
			// Выполняем установку кода отказа шифрования
			this->_error = error_t::ENCRYPTION_FAILED;
			// Сообщаем, что кадр не снят
			return false;
		}
		// Выполняем перенесение расшифрованного содержимого
		content = std::move(opened);
	}
	// Если содержимое кадра сжато
	if(chunk.method != compressor::method_t::NONE){
		// Если модуль сжатия не отдан
		if(this->_compressor == nullptr){
			// Выполняем установку кода отказа сжатия
			this->_error = error_t::COMPRESSION_FAILED;
			// Сообщаем, что кадр не снят
			return false;
		}
		// Разжатое содержимое кадра
		vector <uint8_t> opened;
		// Выполняем разжатие содержимого кадра
		this->_compressor->decompress(content.data(), content.size(), chunk.method, opened);
		// Если разжатие содержимого отвечено отказом
		if(opened.empty() && (chunk.origin > 0)){
			// Выполняем установку кода отказа сжатия
			this->_error = error_t::COMPRESSION_FAILED;
			// Сообщаем, что кадр не снят
			return false;
		}
		// Выполняем перенесение разжатого содержимого
		content = std::move(opened);
	}
	/**
	 * Если длина снятого содержимого разошлась с объявленной.
	 *
	 * Сличение это стережёт порчу, какую ни сжатие, ни шифрование не заметили:
	 * содержимое иной длины означает, что кадр собран не тем, чем разбирается
	 */
	if(content.size() != static_cast <size_t> (chunk.origin)){
		// Выполняем установку кода отказа опознания кадра
		this->_error = error_t::INVALID_CHUNK;
		// Сообщаем, что кадр не снят
		return false;
	}
	// Выполняем перенесение снятого содержимого кадра
	result = std::move(content);
	// Выполняем сдвиг смещения на снятый кадр
	offset += (CHUNK_HEADER + static_cast <size_t> (length));
	// Сообщаем, что кадр снят
	return true;
}
/**
 * @brief Метод извлечения кода отказа укладки либо снятия кадра
 *
 * @return код отказа
 *
 */
awh::codec::abc::error_t awh::codec::abc::Packer::error() const noexcept {
	// Выводим код отказа укладки либо снятия кадра
	return this->_error;
}
/**
 * @brief Метод извлечения настроек укладки кадра
 *
 * @return настройки укладки кадра
 *
 */
const awh::codec::abc::Packer::settings_t & awh::codec::abc::Packer::settings() const noexcept {
	// Выводим настройки укладки кадра
	return this->_settings;
}
/**
 * @brief Метод установки настроек укладки кадра
 *
 * @param settings устанавливаемые настройки укладки кадра
 *
 */
void awh::codec::abc::Packer::settings(const settings_t & settings) noexcept {
	// Выполняем установку настроек укладки кадра
	this->_settings = settings;
}
