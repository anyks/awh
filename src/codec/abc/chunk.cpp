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
#include <codec/abc/encoding.hpp>

/**
 * Подключаем заголовочный файл выработки свёрток: кадр несёт контрольную сумму
 */
#include <cryptography/hash.hpp>

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
	 * @brief Зерно контрольной суммы кадра
	 *
	 * @note Зерно своё, не заголовка контейнера: сумма кадра и сумма заголовка кроют
	 *       разное, и общее зерно роднило бы их без всякой нужды
	 *
	 */
	constexpr uint64_t Seed = 0x41424348554E4B01ull;
	/**
	 * @brief Разряд объявления кадра мусором
	 *
	 * @details Кадр, обращённый правкой в мусор, при подрядном чтении пропускается.
	 *          Пометка эта есть правка одного октета, а не перезапись кадра: перезапись
	 *          обязала бы шифровать содержимое наново, ничего в нём не меняя
	 *
	 */
	constexpr uint8_t Waste = awh::codec::abc::CHUNK_WASTE;
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
 * @param log объект для работы с логами
 *
 */
awh::codec::abc::Packer::Packer(const log_t * log) noexcept :
 _error(error_t::NONE), _compressor(nullptr), _crypto(nullptr), _log(log) {}
/**
 * @brief Метод объявления отказа укладки либо снятия кадра
 *
 * @param error объявляемый код отказа
 * @return      признак успешности, всегда ложь
 *
 */
bool awh::codec::abc::Packer::fail(const error_t error) noexcept {
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
 * @brief Функция выработки контрольной суммы кадра
 *
 * @param buffer буфер кадра целиком, от заголовка его
 * @param size   размер кадра в октетах
 * @return       выработанная контрольная сумма кадра
 *
 */
uint64_t awh::codec::abc::digest(const void * buffer, const size_t size) noexcept {
	// Выполняем получение указателя на октеты кадра
	const uint8_t * octets = reinterpret_cast <const uint8_t *> (buffer);
	// Если кадр короче заголовка своего
	if((octets == nullptr) || (size < CHUNK_HEADER))
		// Выводим пустую контрольную сумму
		return 0;
	// Заголовок кадра, приведённый к виду, каким сумма и вырабатывается
	uint8_t head[CHUNK_HEADER];
	// Выполняем перенос заголовка кадра
	::memcpy(head, octets, CHUNK_HEADER);
	/**
	 * Выполняем снятие разряда мусора с признаков кадра: разряд учётный, метится
	 * правкой одного октета уже уложенного кадра, и сумма ему следовать не обязана
	 */
	head[CHUNK_FLAGS] = static_cast <uint8_t> (head[CHUNK_FLAGS] & ~CHUNK_WASTE);
	// Выполняем обнуление места самой контрольной суммы: в себя она не входит
	::memset(head + CHUNK_DIGEST, 0, 8);
	// Выработанная контрольная сумма заголовка кадра
	uint64_t result = awh::hashing::generate(head, CHUNK_HEADER, Seed);
	// Если у кадра есть содержимое
	if(size > CHUNK_HEADER)
		// Выполняем выработку суммы по содержимому кадра с зерном заголовка
		result = awh::hashing::generate(octets + CHUNK_HEADER, size - CHUNK_HEADER, result);
	// Выводим выработанную контрольную сумму кадра
	return result;
}
/**
 * @brief Функция обёртки уложенной записи кадром
 *
 * @param result     буфер уложенной записи, обёртываемой кадром
 * @param number     порядковый номер кадра
 * @param generation поколение записи кадра
 *
 */
bool awh::codec::abc::envelope(vector <uint8_t> & result, const uint64_t number, const uint32_t generation) noexcept {
	// Выполняем получение длины обёртываемой записи
	const uint64_t length = static_cast <uint64_t> (result.size());
	/**
	 * Если длина обёртываемой записи в запись кадра не вмещается
	 *
	 * @note Длина кадра объявлена четырьмя октетами, и запись длиннее легла бы в них
	 *       усечённой МОЛЧА: кадр вышел бы годным по виду, а содержимое его -
	 *       оборванным, и вскрылось бы это лишь у читающего
	 */
	if(length > static_cast <uint64_t> (numeric_limits <uint32_t>::max()))
		// Сообщаем, что обернуть запись кадром не удалось
		return false;
	// Выполняем отведение места под заголовок кадра впереди записи
	result.insert(result.begin(), CHUNK_HEADER, 0);
	// Выполняем получение указателя на заголовок кадра
	uint8_t * head = result.data();
	/**
	 * Выполняем объявление кадра мусором: содержимое его записью контейнера не
	 * является, и подрядное чтение обязано его пропустить
	 */
	head[CHUNK_FLAGS] = CHUNK_WASTE;
	// Выполняем укладку длины уложенного содержимого кадра
	abc::fixed(head + 4, length, 4);
	// Выполняем укладку длины исходного содержимого кадра
	abc::fixed(head + 8, length, 4);
	// Выполняем укладку порядкового номера кадра
	abc::fixed(head + 12, number, 8);
	// Выполняем укладку поколения записи кадра
	abc::fixed(head + 20, static_cast <uint64_t> (generation), 4);
	// Выполняем укладку контрольной суммы кадра-обёртки
	abc::fixed(result.data() + CHUNK_DIGEST, abc::digest(result.data(), result.size()), 8);
	// Сообщаем, что запись обёрнута кадром
	return true;
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
		this->fail(error_t::INTERNAL);
		// Сообщаем, что укладка отвечена отказом
		return false;
	}
	// Если длина укладываемого содержимого не вмещается в запись кадра
	if(size > static_cast <size_t> (numeric_limits <uint32_t>::max())){
		// Выполняем установку кода отказа длины
		this->fail(error_t::INVALID_LENGTH);
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
			this->fail(error_t::ENCRYPTION_FAILED);
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
			this->fail(error_t::ENCRYPTION_FAILED);
			// Сообщаем, что укладка отвечена отказом
			return false;
		}
		// Выполняем перенесение зашифрованного содержимого
		payload = std::move(secured);
		// Выполняем установку признака зашифрованности содержимого
		encrypted = true;
	}
	/**
	 * Если длина уложенного содержимого не вмещается в запись кадра
	 *
	 * @warning Заслон ПОСЛЕДНЕЙ РУКИ: длина кадра объявлена тридцатью двумя разрядами, и
	 *          сюда доходит лишь содержимое свыше четырёх гигабайт. Проверкою такое не
	 *          закрепить: буфер потребен настоящий, и всякий стенд набора на нём встанет.
	 *          Молчание щупов здесь означает НЕДОСТИЖИМОСТЬ УСЛОВИЯ ценою прогона, а не
	 *          мёртвый путь: щуп пути, принуждающий заслон срабатывать всегда, красит
	 *          восемьдесят проверок - путь этот хожен всякой укладкой (замерено 05.09.2026)
	 *
	 * @note Убирать заслон нельзя: без него длина усечётся молча, и кадр объявит длину,
	 *       содержимому не равную
	 */
	if(payload.size() > static_cast <size_t> (numeric_limits <uint32_t>::max())){
		// Выполняем установку кода отказа длины
		this->fail(error_t::INVALID_LENGTH);
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
	abc::fixed(head + 4, static_cast <uint64_t> (payload.size()), 4);
	// Выполняем укладку длины исходного содержимого кадра
	abc::fixed(head + 8, static_cast <uint64_t> (size), 4);
	// Выполняем укладку порядкового номера кадра
	abc::fixed(head + 12, number, 8);
	// Выполняем укладку поколения записи кадра
	abc::fixed(head + 20, static_cast <uint64_t> (generation), 4);
	// Если содержимое кадра не пусто
	if(!payload.empty())
		// Выполняем укладку содержимого кадра
		result.insert(result.end(), payload.begin(), payload.end());
	/**
	 * Выполняем укладку контрольной суммы кадра последней, по кадру готовому:
	 * сумма кроет заголовок вместе с содержимым, и вырабатывать её раньше нечем
	 */
	abc::fixed(result.data() + start + CHUNK_DIGEST,
	 abc::digest(result.data() + start, result.size() - start), 8);
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
		this->fail(error_t::INTERNAL);
		// Сообщаем, что кадр не снят
		return false;
	}
	// Если поданных октетов недостаёт на заголовок кадра
	if((size < offset) || ((size - offset) < CHUNK_HEADER)){
		// Выполняем установку кода отказа обрыва кадра
		this->fail(error_t::TRUNCATED_CHUNK);
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
		this->fail(error_t::INVALID_CHUNK);
		// Сообщаем, что кадр не снят
		return false;
	}
	/**
	 * Если октеты запаса заголовка кадра несут неведомое.
	 *
	 * Два октета эти оставлены впрок, укладка кладёт их нулями, и требование нулей
	 * равняется на сличение разрядов кадра, стоящее выше, и на то же требование у
	 * строки оглавления: поле приходит с провода и обязано быть опознано
	 */
	if(abc::gather(head + 2, 2) != 0){
		// Выполняем установку кода отказа опознания кадра
		this->fail(error_t::INVALID_CHUNK);
		// Сообщаем, что кадр не снят
		return false;
	}
	// Выполняем снятие длины уложенного содержимого кадра
	const uint32_t length = static_cast <uint32_t> (abc::gather(head + 4, 4));
	// Если содержимого кадра в поданных октетах недостаёт
	if((size - offset - CHUNK_HEADER) < static_cast <size_t> (length)){
		// Выполняем установку кода отказа обрыва кадра
		this->fail(error_t::TRUNCATED_CHUNK);
		// Сообщаем, что кадр не снят
		return false;
	}
	/**
	 * Если контрольная сумма кадра не сошлась
	 *
	 * @details Сличение стоит ПРЕЖДЕ всякого толкования кадра: за ним идут разжатие и
	 *          расшифровка, а подавать им испорченное - значит гадать по итогу их. Без
	 *          суммы порча октета внутри кадра проходила молча: развёртка 25.08.2026
	 *          дала 24 молчаливо неверных чтения из 530 при порче кадра оглавления
	 */
	if(abc::gather(head + CHUNK_DIGEST, 8) !=
	 abc::digest(head, static_cast <size_t> (CHUNK_HEADER + length))){
		// Выполняем установку кода отказа несошедшейся суммы
		this->fail(error_t::INVALID_CHECKSUM);
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
		this->fail(error_t::INVALID_CHUNK);
		// Сообщаем, что кадр не снят
		return false;
	}
	// Выполняем снятие метода сжатия содержимого кадра
	chunk.method = static_cast <compressor::method_t> (head[0]);
	// Выполняем установку длины уложенного содержимого кадра
	chunk.length = length;
	// Выполняем снятие длины исходного содержимого кадра
	chunk.origin = static_cast <uint32_t> (abc::gather(head + 8, 4));
	// Выполняем снятие порядкового номера кадра
	chunk.number = abc::gather(head + 12, 8);
	// Выполняем снятие поколения записи кадра
	chunk.generation = static_cast <uint32_t> (abc::gather(head + 20, 4));
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
			this->fail(error_t::ENCRYPTION_FAILED);
			// Сообщаем, что кадр не снят
			return false;
		}
		// Выполняем расшифровку содержимого кадра
		vector <uint8_t> opened = this->_crypto->decrypt <vector <uint8_t>> (content,
		 this->_settings.hash, this->_settings.cipher);
		// Если расшифровка содержимого отвечена отказом
		if(opened.empty() && (chunk.origin > 0)){
			// Выполняем установку кода отказа шифрования
			this->fail(error_t::ENCRYPTION_FAILED);
			// Сообщаем, что кадр не снят
			return false;
		}
		// Выполняем перенесение расшифрованного содержимого
		content = std::move(opened);
	}
	// Если содержимое кадра сжато
	if(chunk.method != compressor::method_t::NONE){
		/**
		 * Если модуль сжатия не отдан
		 *
		 * @details Заслон стережёт РАЗЛАД ОСНАСТКИ, а не отказ подсистемы сжатия: метод кадр
		 *          несёт в себе, а разжиматель отдаёт зовущий, и снимающий волен разойтись
		 *          с укладчиком. Снятие заслона уводит в разыменование пустого указателя и
		 *          валит весь набор - оттого зрячесть проверки доказана подменою ПРИЧИНЫ, а
		 *          не снятием заслона: сорванный прогон судить по именам нельзя
		 *
		 * @note Закреплено проверкою `ChunkFixture.UnpackingCompressedWithoutTheCompressorIsRefused`:
		 *       подмена причины красит одну лишь её (2012 прогнано, 2011 прошло), и до
		 *       05.09.2026 место было слепым
		 */
		if(this->_compressor == nullptr){
			// Выполняем установку кода отказа сжатия
			this->fail(error_t::COMPRESSION_FAILED);
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
			this->fail(error_t::COMPRESSION_FAILED);
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
		this->fail(error_t::INVALID_CHUNK);
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
