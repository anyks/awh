/**
 * @file: frame.cpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация слоя кадров HTTP/3 (RFC 9114 §7) — разбор заголовка кадра и полезной нагрузки
 *        управляющих кадров, сборка кадров всех типов протокола
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/parser/http3/frame.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Конструктор
 *
 */
awh::http::h3::frame::Setting::Setting() noexcept : id(0), value(0) {}
/**
 * @brief Конструктор
 *
 */
awh::http::h3::frame::Header::Header() noexcept : type(0), length(0) {}
/**
 * @brief Конструктор
 *
 */
awh::http::h3::frame::Push_Promise::Push_Promise() noexcept : pushId(0), block{} {}
/**
 * @brief Конструктор
 *
 */
awh::http::h3::frame::Priority_Update::Priority_Update() noexcept : push(false), id(0), value{} {}

/**
 * @brief Функция разбора заголовка кадра
 *
 * @param data   входной буфер
 * @param size   доступно байт
 * @param output разобранный заголовок кадра
 * @return       количество прочитанных октетов либо 0, если данных недостаточно
 *
 */
size_t awh::http::h3::frame::parser::header(const uint8_t * data, const size_t size, header_t & output) noexcept {
	// Если входной буфер не передан
	if((data == nullptr) || (size == 0))
		// Выводим признак нехватки данных
		return 0;
	// Тип кадра
	uint64_t type = 0;
	// Выполняем чтение типа кадра
	const size_t consumed = quic::varint::read(data, size, type);
	// Если тип кадра прочитать не удалось
	if(consumed == 0)
		// Выводим признак нехватки данных
		return 0;
	// Длина полезной нагрузки кадра
	uint64_t length = 0;
	// Выполняем чтение длины полезной нагрузки кадра
	const size_t tail = quic::varint::read((data + consumed), (size - consumed), length);
	// Если длину полезной нагрузки прочитать не удалось
	if(tail == 0)
		// Выводим признак нехватки данных
		return 0;
	// Устанавливаем тип кадра
	output.type = type;
	// Устанавливаем длину полезной нагрузки кадра
	output.length = length;
	// Выводим размер прочитанного заголовка кадра
	return (consumed + tail);
}
/**
 * @brief Функция разбора нагрузки кадра из единственного целого переменной длины
 *
 * @param payload полезная нагрузка кадра
 * @param size    размер полезной нагрузки
 * @param value   разобранное число
 * @param error   код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 *
 */
awh::http::h3::status_t awh::http::h3::frame::parser::identifier(const uint8_t * payload, const size_t size, uint64_t & value, error_t & error) noexcept {
	// Если полезная нагрузка не передана
	if((payload == nullptr) || (size == 0)){
		// Устанавливаем код ошибки протокола
		error = error_t::H3_FRAME_ERROR;
		// Выводим результат разбора
		return status_t::ERROR;
	}
	// Выполняем чтение числа
	const size_t consumed = quic::varint::read(payload, size, value);
	/**
	 * Нагрузка обязана содержать ровно одно число: и её обрыв, и лишние октеты
	 * после числа - ошибка уровня соединения (RFC 9114 §7.1)
	 */
	if((consumed == 0) || (consumed != size)){
		// Устанавливаем код ошибки протокола
		error = error_t::H3_FRAME_ERROR;
		// Выводим результат разбора
		return status_t::ERROR;
	}
	// Выводим результат разбора
	return status_t::OK;
}
/**
 * @brief Функция разбора нагрузки кадра SETTINGS (RFC 9114 §7.2.4)
 *
 * @param payload полезная нагрузка кадра
 * @param size    размер полезной нагрузки
 * @param output  разобранный набор параметров
 * @param error   код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 *
 */
awh::http::h3::status_t awh::http::h3::frame::parser::settings(const uint8_t * payload, const size_t size, vector <setting_entry_t> & output, error_t & error) noexcept {
	// Выполняем очистку набора параметров
	output.clear();
	/**
	 * Обрыв нагрузки и повтор параметра - разные ошибки: обрыв нарушает требования
	 * к нагрузке любого кадра и даёт H3_FRAME_ERROR (RFC 9114 §7.1), а повтор
	 * идентификатора нарушает требования именно к SETTINGS и даёт H3_SETTINGS_ERROR
	 * (RFC 9114 §7.2.4.1). Эталонная реализация nghttp3 различает их так же
	 */
	// Если полезная нагрузка не передана
	if((payload == nullptr) && (size > 0)){
		// Устанавливаем код ошибки протокола
		error = error_t::H3_FRAME_ERROR;
		// Выводим результат разбора
		return status_t::ERROR;
	}
	// Смещение разбора в полезной нагрузке
	size_t offset = 0;
	/**
	 * Выполняем чтение всех параметров набора
	 */
	while(offset < size){
		// Разбираемый параметр
		setting_entry_t item;
		// Выполняем чтение идентификатора параметра
		size_t consumed = quic::varint::read((payload + offset), (size - offset), item.id);
		// Если идентификатор параметра прочитать не удалось
		if(consumed == 0){
			// Устанавливаем код ошибки протокола
			error = error_t::H3_FRAME_ERROR;
			// Выводим результат разбора
			return status_t::ERROR;
		}
		// Выполняем смещение разбора
		offset += consumed;
		// Выполняем чтение значения параметра
		consumed = quic::varint::read((payload + offset), (size - offset), item.value);
		/**
		 * Значение параметра обязано присутствовать: параметр без значения означает
		 * обрыв нагрузки посреди пары (RFC 9114 §7.1)
		 */
		if(consumed == 0){
			// Устанавливаем код ошибки протокола
			error = error_t::H3_FRAME_ERROR;
			// Выводим результат разбора
			return status_t::ERROR;
		}
		// Выполняем смещение разбора
		offset += consumed;
		/**
		 * Выполняем поиск повторного объявления параметра: повтор идентификатора -
		 * ошибка уровня соединения (RFC 9114 §7.2.4.1)
		 */
		for(auto & exists : output){
			// Если идентификатор параметра уже встречался
			if(exists.id == item.id){
				// Устанавливаем код ошибки протокола
				error = error_t::H3_SETTINGS_ERROR;
				// Выводим результат разбора
				return status_t::ERROR;
			}
		}
		// Дописываем разобранный параметр в набор
		output.push_back(item);
	}
	// Выводим результат разбора
	return status_t::OK;
}
/**
 * @brief Функция разбора нагрузки кадра PUSH_PROMISE (RFC 9114 §7.2.5)
 *
 * @param payload полезная нагрузка кадра
 * @param size    размер полезной нагрузки
 * @param output  разобранная полезная нагрузка
 * @param error   код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 *
 */
awh::http::h3::status_t awh::http::h3::frame::parser::pushPromise(const uint8_t * payload, const size_t size, push_promise_t & output, error_t & error) noexcept {
	// Если полезная нагрузка не передана
	if((payload == nullptr) || (size == 0)){
		// Устанавливаем код ошибки протокола
		error = error_t::H3_FRAME_ERROR;
		// Выводим результат разбора
		return status_t::ERROR;
	}
	// Выполняем чтение идентификатора обещанного push
	const size_t consumed = quic::varint::read(payload, size, output.pushId);
	// Если идентификатор обещанного push прочитать не удалось
	if(consumed == 0){
		// Устанавливаем код ошибки протокола
		error = error_t::H3_FRAME_ERROR;
		// Выводим результат разбора
		return status_t::ERROR;
	}
	/**
	 * Остаток нагрузки - секция полей запроса; пустая секция допустима синтаксически,
	 * а её содержательную проверку выполняет декодер QPACK и семантика HTTP
	 */
	output.block = string_view(reinterpret_cast <const char *> (payload + consumed), (size - consumed));
	// Выводим результат разбора
	return status_t::OK;
}
/**
 * @brief Функция разбора нагрузки кадра PRIORITY_UPDATE (RFC 9218 §7.2)
 *
 * @param type    тип кадра, различающий поток запроса и поток push
 * @param payload полезная нагрузка кадра
 * @param size    размер полезной нагрузки
 * @param output  разобранная полезная нагрузка
 * @param error   код ошибки протокола
 * @return        результат разбора (OK/ERROR)
 *
 */
awh::http::h3::status_t awh::http::h3::frame::parser::priorityUpdate(const uint64_t type, const uint8_t * payload, const size_t size, priority_update_t & output, error_t & error) noexcept {
	// Если полезная нагрузка не передана
	if((payload == nullptr) || (size == 0)){
		// Устанавливаем код ошибки протокола
		error = error_t::H3_FRAME_ERROR;
		// Выводим результат разбора
		return status_t::ERROR;
	}
	// Выполняем чтение идентификатора приоритезируемого элемента
	const size_t consumed = quic::varint::read(payload, size, output.id);
	// Если идентификатор приоритезируемого элемента прочитать не удалось
	if(consumed == 0){
		// Устанавливаем код ошибки протокола
		error = error_t::H3_FRAME_ERROR;
		// Выводим результат разбора
		return status_t::ERROR;
	}
	// Устанавливаем признак назначения приоритета потоку push
	output.push = (type == static_cast <uint64_t> (frame_t::PRIORITY_UPDATE_PUSH));
	// Устанавливаем значение поля приоритета
	output.value = string_view(reinterpret_cast <const char *> (payload + consumed), (size - consumed));
	// Выводим результат разбора
	return status_t::OK;
}
/**
 * @brief Функция записи заголовка кадра
 *
 * @param output выходной буфер
 * @param type   тип кадра
 * @param length длина полезной нагрузки
 *
 */
void awh::http::h3::frame::serialize::header(string & output, const uint64_t type, const uint64_t length) noexcept {
	// Выполняем запись типа кадра
	quic::varint::write(output, type);
	// Выполняем запись длины полезной нагрузки кадра
	quic::varint::write(output, length);
}
/**
 * @brief Функция записи типа однонаправленного потока (RFC 9114 §6.2)
 *
 * @param output выходной буфер
 * @param type   тип однонаправленного потока
 *
 */
void awh::http::h3::frame::serialize::unistream(string & output, const uint64_t type) noexcept {
	// Выполняем запись типа однонаправленного потока
	quic::varint::write(output, type);
}
/**
 * @brief Функция записи кадра DATA (RFC 9114 §7.2.1)
 *
 * @param output выходной буфер
 * @param data   данные тела
 *
 */
void awh::http::h3::frame::serialize::data(string & output, string_view data) noexcept {
	// Выполняем запись заголовка кадра
	serialize::header(output, static_cast <uint64_t> (frame_t::DATA), data.size());
	// Выполняем запись данных тела
	output.append(data);
}
/**
 * @brief Функция записи кадра HEADERS (RFC 9114 §7.2.2)
 *
 * @param output выходной буфер
 * @param block  секция полей, закодированная QPACK
 *
 */
void awh::http::h3::frame::serialize::headers(string & output, string_view block) noexcept {
	// Выполняем запись заголовка кадра
	serialize::header(output, static_cast <uint64_t> (frame_t::HEADERS), block.size());
	// Выполняем запись секции полей
	output.append(block);
}
/**
 * @brief Функция записи кадра GOAWAY (RFC 9114 §7.2.6)
 *
 * @param output выходной буфер
 * @param id     идентификатор потока запроса (от сервера) либо push (от клиента)
 *
 */
void awh::http::h3::frame::serialize::goaway(string & output, const uint64_t id) noexcept {
	// Выполняем запись заголовка кадра
	serialize::header(output, static_cast <uint64_t> (frame_t::GOAWAY), quic::varint::size(id));
	// Выполняем запись идентификатора
	quic::varint::write(output, id);
}
/**
 * @brief Функция записи кадра CANCEL_PUSH (RFC 9114 §7.2.3)
 *
 * @param output выходной буфер
 * @param pushId идентификатор отменяемого push
 *
 */
void awh::http::h3::frame::serialize::cancelPush(string & output, const uint64_t pushId) noexcept {
	// Выполняем запись заголовка кадра
	serialize::header(output, static_cast <uint64_t> (frame_t::CANCEL_PUSH), quic::varint::size(pushId));
	// Выполняем запись идентификатора отменяемого push
	quic::varint::write(output, pushId);
}
/**
 * @brief Функция записи кадра MAX_PUSH_ID (RFC 9114 §7.2.7)
 *
 * @param output выходной буфер
 * @param pushId наибольший допустимый идентификатор push
 *
 */
void awh::http::h3::frame::serialize::maxPushId(string & output, const uint64_t pushId) noexcept {
	// Выполняем запись заголовка кадра
	serialize::header(output, static_cast <uint64_t> (frame_t::MAX_PUSH_ID), quic::varint::size(pushId));
	// Выполняем запись наибольшего допустимого идентификатора push
	quic::varint::write(output, pushId);
}
/**
 * @brief Функция записи кадра SETTINGS (RFC 9114 §7.2.4)
 *
 * @param output выходной буфер
 * @param items  набор параметров
 * @param count  количество параметров
 *
 */
void awh::http::h3::frame::serialize::settings(string & output, const setting_entry_t * items, const size_t count) noexcept {
	// Длина полезной нагрузки кадра
	uint64_t length = 0;
	/**
	 * Выполняем подсчёт длины нагрузки: длина обязана быть записана до самих
	 * параметров, поэтому набор проходится дважды
	 */
	for(size_t i = 0; (items != nullptr) && (i < count); i++)
		// Дописываем размер очередной пары идентификатор-значение
		length += (quic::varint::size(items[i].id) + quic::varint::size(items[i].value));
	// Выполняем запись заголовка кадра
	serialize::header(output, static_cast <uint64_t> (frame_t::SETTINGS), length);
	/**
	 * Выполняем запись всех параметров набора
	 */
	for(size_t i = 0; (items != nullptr) && (i < count); i++){
		// Выполняем запись идентификатора параметра
		quic::varint::write(output, items[i].id);
		// Выполняем запись значения параметра
		quic::varint::write(output, items[i].value);
	}
}
/**
 * @brief Функция записи кадра PUSH_PROMISE (RFC 9114 §7.2.5)
 *
 * @param output выходной буфер
 * @param pushId идентификатор обещанного push
 * @param block  секция полей запроса, закодированная QPACK
 *
 */
void awh::http::h3::frame::serialize::pushPromise(string & output, const uint64_t pushId, string_view block) noexcept {
	// Выполняем запись заголовка кадра
	serialize::header(output, static_cast <uint64_t> (frame_t::PUSH_PROMISE), (quic::varint::size(pushId) + block.size()));
	// Выполняем запись идентификатора обещанного push
	quic::varint::write(output, pushId);
	// Выполняем запись секции полей запроса
	output.append(block);
}
/**
 * @brief Функция записи кадра PRIORITY_UPDATE (RFC 9218 §7.2)
 *
 * @param output выходной буфер
 * @param push   признак назначения приоритета потоку push, а не потоку запроса
 * @param id     идентификатор потока запроса либо идентификатор push
 * @param value  значение поля приоритета в синтаксисе структурированных полей
 *
 */
void awh::http::h3::frame::serialize::priorityUpdate(string & output, const bool push, const uint64_t id, string_view value) noexcept {
	// Определяем тип кадра по виду приоритезируемого элемента
	const uint64_t type = static_cast <uint64_t> (push ? frame_t::PRIORITY_UPDATE_PUSH : frame_t::PRIORITY_UPDATE_REQUEST);
	// Выполняем запись заголовка кадра
	serialize::header(output, type, (quic::varint::size(id) + value.size()));
	// Выполняем запись идентификатора приоритезируемого элемента
	quic::varint::write(output, id);
	// Выполняем запись значения поля приоритета
	output.append(value);
}
/**
 * @brief Функция записи зарезервированного кадра (RFC 9114 §7.2.8)
 *
 * @param output выходной буфер
 * @param seed   порядковый номер N в последовательности зарезервированных типов
 * @param data   произвольная нагрузка кадра
 *
 */
void awh::http::h3::frame::serialize::reserved(string & output, const uint64_t seed, string_view data) noexcept {
	/**
	 * Ограничиваем порядковый номер так, чтобы тип кадра остался в области значений
	 * целого переменной длины: за границей 2^62-1 записать его было бы нечем
	 */
	const uint64_t type = (proto::GREASE_BASE + (proto::GREASE_STEP * (seed % ((proto::MAX_VARINT - proto::GREASE_BASE) / proto::GREASE_STEP))));
	// Выполняем запись заголовка кадра
	serialize::header(output, type, data.size());
	// Выполняем запись произвольной нагрузки кадра
	output.append(data);
}
