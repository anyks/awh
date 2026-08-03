/**
 * @file: idna.cpp
 * @date: 2026-08-03
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация модуля приведения доменных имён — преобразование символов
 *        по таблице приложения по обработке доменных имён, приведение к нормальному
 *        представлению, проверка допустимости меток и запись их кодировкой Punycode
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <idna/idna.hpp>
#include <sys/ascii.hpp>
#include <unicode/utf8.hpp>
#include <unicode/unicode.hpp>
#include <unicode/normalize.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Пространство имён вспомогательных функций приведения доменных имён
 *
 */
namespace {
	/**
	 * @brief Кодовое значение разделителя меток доменного имени
	 *
	 */
	constexpr uint32_t SEPARATOR = 0x002E;

	/**
	 * @brief Кодовое значение знака переноса
	 *
	 */
	constexpr uint32_t HYPHEN = 0x002D;

	/**
	 * @brief Признак установленного режима приведения доменного имени
	 *
	 * @param mode набор режимов приведения доменного имени
	 * @param flag проверяемый режим приведения доменного имени
	 * @return     результат проверки установки режима приведения
	 *
	 */
	inline bool enabled(const uint16_t mode, const awh::idna::option_t flag) noexcept {
		// Выводим результат проверки установки режима приведения
		return ((mode & static_cast <uint16_t> (flag)) != 0);
	}
	/**
	 * @brief Функция проверки допустимости метки доменного имени
	 *
	 * @param label   набор кодовых значений проверяемой метки
	 * @param mode    набор режимов приведения доменного имени
	 * @param inverse признак записи доменного имени двунаправленным письмом
	 * @return        код ошибки проверки допустимости метки
	 *
	 */
	awh::idna::error_t validate(const vector <uint32_t> & label, const uint16_t mode, const bool inverse) noexcept {
		/**
		 * Если проверять оказалось нечего
		 */
		if(label.empty())
			// Выводим отсутствие ошибки проверки допустимости метки
			return awh::idna::error_t::NONE;
		// Набор кодовых значений метки в нормальном представлении
		vector <uint32_t> normal;
		// Выполняем приведение метки к нормальному представлению
		awh::unicode::normalize(label, awh::unicode::form_t::NFC, normal);
		/**
		 * Если метка к нормальному представлению не приведена
		 */
		if(normal != label)
			// Выводим ошибку приведения метки к нормальному представлению
			return awh::idna::error_t::NOT_NORMAL;
		/**
		 * Если требуется проверка размещения знаков переноса
		 */
		if(enabled(mode, awh::idna::option_t::HYPHENS)) {
			/**
			 * Если метка начинается либо завершается знаком переноса
			 */
			if((label.front() == HYPHEN) || (label.back() == HYPHEN))
				// Выводим ошибку размещения знака переноса
				return awh::idna::error_t::HYPHEN;
			/**
			 * Если третий и четвёртый символы метки являются знаками переноса
			 *
			 * @details Такое размещение отведено обозначению записи метки кодировкой,
			 *          и метка, записанная иначе, его занимать не вправе.
			 *
			 */
			if((label.size() > 3) && (label.at(2) == HYPHEN) && (label.at(3) == HYPHEN))
				// Выводим ошибку размещения знака переноса
				return awh::idna::error_t::HYPHEN;
		}
		/**
		 * Если метка начинается сочетающимся знаком
		 */
		if(awh::unicode::holds(label.front(), static_cast <uint16_t> (awh::unicode::property_id_t::M)))
			// Выводим ошибку размещения сочетающегося знака
			return awh::idna::error_t::LEADING_MARK;
		// Набор кодовых значений преобразования очередного символа
		vector <uint32_t> mapped;
		/**
		 * Выполняем обход символов проверяемой метки
		 */
		for(auto & code : label) {
			/**
			 * Если очередной символ является разделителем меток
			 */
			if(code == SEPARATOR)
				// Выводим ошибку размещения недопустимого символа
				return awh::idna::error_t::DISALLOWED;
			// Получаем состояние очередного символа в таблице преобразований
			const awh::idna::status_t value = awh::idna::status(code, mapped);
			/**
			 * Определяем состояние очередного символа метки
			 */
			switch(static_cast <uint8_t> (value)) {
				// Символ метке допустим
				case static_cast <uint8_t> (awh::idna::status_t::VALID): break;
				/**
				 * Символ метке допустим, если правила записи имён узлов не применяются
				 */
				case static_cast <uint8_t> (awh::idna::status_t::DISALLOWED_STD3_VALID): {
					/**
					 * Если применяются правила записи имён узлов
					 */
					if(enabled(mode, awh::idna::option_t::STD3))
						// Выводим ошибку размещения недопустимого символа
						return awh::idna::error_t::DISALLOWED;
				} break;
				/**
				 * Символ метке допустим, если переходный режим не применяется
				 */
				case static_cast <uint8_t> (awh::idna::status_t::DEVIATION): {
					/**
					 * Если применяется переходный режим преобразования символов
					 */
					if(enabled(mode, awh::idna::option_t::TRANSITIONAL))
						// Выводим ошибку размещения недопустимого символа
						return awh::idna::error_t::DISALLOWED;
				} break;
				// В остальных случаях символ метке недопустим
				default: return awh::idna::error_t::DISALLOWED;
			}
		}
		/**
		 * Если требуется проверка правил сочетания соединителей
		 */
		if(enabled(mode, awh::idna::option_t::JOINERS) && !awh::idna::joiners(label))
			// Выводим ошибку нарушения правил сочетания соединителей
			return awh::idna::error_t::CONTEXT;
		/**
		 * Если требуется проверка правила двунаправленного письма
		 */
		if(inverse && enabled(mode, awh::idna::option_t::BIDI) && !awh::idna::bidirectional(label))
			// Выводим ошибку нарушения правила двунаправленного письма
			return awh::idna::error_t::BIDI;
		// Выводим отсутствие ошибки проверки допустимости метки
		return awh::idna::error_t::NONE;
	}
	/**
	 * @brief Функция разбора доменного имени на метки с преобразованием символов
	 *
	 * @param domain  разбираемое доменное имя, записанное в кодировке UTF-8
	 * @param mode    набор режимов приведения доменного имени
	 * @param labels  набор получившихся меток доменного имени
	 * @param error   код ошибки разбора доменного имени
	 * @return        результат выполнения разбора доменного имени
	 *
	 */
	bool separate(string_view domain, const uint16_t mode, vector <vector <uint32_t>> & labels, awh::idna::error_t & error) noexcept {
		// Выполняем очистку набора меток доменного имени
		labels.clear();
		// Набор кодовых значений доменного имени с преобразованными символами
		vector <uint32_t> text;
		// Набор кодовых значений преобразования очередного символа
		vector <uint32_t> mapped;
		// Кодовое значение очередного разобранного символа
		uint32_t code = 0;
		/**
		 * Выполняем обход записей символов доменного имени
		 */
		for(size_t pos = 0; pos < domain.size();) {
			// Выполняем разбор записи очередного символа доменного имени
			const size_t length = awh::utf8::decode(domain, pos, code);
			/**
			 * Если запись очередного символа разобрать не вышло
			 */
			if(length == 0) {
				// Выводим ошибку записи доменного имени
				error = awh::idna::error_t::ENCODING;
				// Выводим результат выполнения разбора доменного имени
				return false;
			}
			// Выполняем переход к записи следующего символа
			pos += length;
			// Получаем состояние очередного символа в таблице преобразований
			const awh::idna::status_t value = awh::idna::status(code, mapped);
			/**
			 * Определяем состояние очередного символа доменного имени
			 */
			switch(static_cast <uint8_t> (value)) {
				// Символ опускается
				case static_cast <uint8_t> (awh::idna::status_t::IGNORED): continue;
				/**
				 * Символ преобразуется набором символов
				 */
				case static_cast <uint8_t> (awh::idna::status_t::MAPPED): {
					/**
					 * Выполняем добавление символов преобразования
					 */
					for(auto & value : mapped)
						// Выполняем добавление символа преобразования
						text.push_back(value);
				} continue;
				/**
				 * Символ преобразуется лишь переходным режимом
				 */
				case static_cast <uint8_t> (awh::idna::status_t::DEVIATION): {
					/**
					 * Если применяется переходный режим преобразования символов
					 */
					if(enabled(mode, awh::idna::option_t::TRANSITIONAL)) {
						// Выполняем добавление символов преобразования
						for(auto & value : mapped)
							// Выполняем добавление символа преобразования
							text.push_back(value);
						// Переходим к следующему символу доменного имени
						continue;
					}
				} break;
				/**
				 * Символ преобразуется вне правил записи имён узлов
				 */
				case static_cast <uint8_t> (awh::idna::status_t::DISALLOWED_STD3_MAPPED): {
					/**
					 * Если правила записи имён узлов не применяются
					 */
					if(!enabled(mode, awh::idna::option_t::STD3)) {
						// Выполняем добавление символов преобразования
						for(auto & value : mapped)
							// Выполняем добавление символа преобразования
							text.push_back(value);
						// Переходим к следующему символу доменного имени
						continue;
					}
					// Выводим ошибку размещения недопустимого символа
					error = awh::idna::error_t::DISALLOWED;
					// Выводим результат выполнения разбора доменного имени
					return false;
				}
				/**
				 * Символ недопустим
				 */
				case static_cast <uint8_t> (awh::idna::status_t::DISALLOWED): {
					// Выводим ошибку размещения недопустимого символа
					error = awh::idna::error_t::DISALLOWED;
					// Выводим результат выполнения разбора доменного имени
					return false;
				}
				/**
				 * Символ допустим вне правил записи имён узлов
				 */
				case static_cast <uint8_t> (awh::idna::status_t::DISALLOWED_STD3_VALID): {
					/**
					 * Если применяются правила записи имён узлов
					 */
					if(enabled(mode, awh::idna::option_t::STD3)) {
						// Выводим ошибку размещения недопустимого символа
						error = awh::idna::error_t::DISALLOWED;
						// Выводим результат выполнения разбора доменного имени
						return false;
					}
				} break;
			}
			// Выполняем добавление символа доменного имени
			text.push_back(code);
		}
		/**
		 * Выполняем приведение доменного имени к нормальному представлению
		 *
		 * @details Приведение выполняется вторым шагом обработки, до разбиения имени
		 *          на метки: приложение по обработке доменных имён предписывает
		 *          приводить имя целиком, а не каждую метку по отдельности.
		 *
		 */
		vector <uint32_t> normal;
		// Выполняем приведение доменного имени к нормальному представлению
		awh::unicode::normalize(text, awh::unicode::form_t::NFC, normal);
		// Выполняем добавление первой метки доменного имени
		labels.emplace_back();
		/**
		 * Выполняем разбиение доменного имени на метки
		 */
		for(auto & value : normal) {
			/**
			 * Если очередной символ является разделителем меток
			 */
			if(value == SEPARATOR)
				// Выполняем добавление следующей метки доменного имени
				labels.emplace_back();
			// Выполняем добавление символа в метку доменного имени
			else labels.back().push_back(value);
		}
		// Выводим результат выполнения разбора доменного имени
		return true;
	}
	/**
	 * @brief Функция приведения метки доменного имени к записи Юникода
	 *
	 * @details Метка, записанная кодировкой Punycode, разбирается, а её содержимое
	 *          проверяется на допустимость. Метка, разобрать которую не вышло,
	 *          выводится без изменений.
	 *
	 * @param label   набор кодовых значений приводимой метки
	 * @param mode    набор режимов приведения доменного имени
	 * @param inverse признак записи доменного имени двунаправленным письмом
	 * @param result  набор кодовых значений получившейся метки
	 * @return        код ошибки приведения метки
	 *
	 */
	awh::idna::error_t restore(const vector <uint32_t> & label, const uint16_t mode, const bool inverse, vector <uint32_t> & result) noexcept {
		// Выполняем запись метки без изменений
		result = label;
		// Получаем длину приставки записи метки кодировкой Punycode
		const size_t length = ::strlen(awh::idna::PREFIX);
		/**
		 * Если метка приставкой записи кодировкой Punycode не обозначена
		 */
		if(label.size() < length)
			// Выводим результат проверки допустимости метки
			return validate(label, mode, inverse);
		/**
		 * Выполняем сличение приставки записи метки кодировкой Punycode
		 */
		for(size_t i = 0; i < length; i++) {
			/**
			 * Если очередной символ метки приставке не отвечает
			 *
			 * @details Приставка сличается без учёта регистра, что предписано
			 *          приложением по обработке доменных имён.
			 *
			 */
			if((label.at(i) > 0x7F) || (ascii::toLower(static_cast <char> (label.at(i))) != awh::idna::PREFIX[i]))
				// Выводим результат проверки допустимости метки
				return validate(label, mode, inverse);
		}
		// Запись метки, разбираемая кодировкой Punycode
		string encoded = "";
		/**
		 * Выполняем сборку записи метки из символов набора ASCII
		 */
		for(size_t i = length; i < label.size(); i++) {
			/**
			 * Если очередной символ метки набору ASCII не принадлежит
			 */
			if(label.at(i) > 0x7F)
				// Выводим ошибку разбора записи метки
				return awh::idna::error_t::PUNYCODE;
			// Выполняем добавление символа записи метки
			encoded.append(1, static_cast <char> (label.at(i)));
		}
		// Набор кодовых значений разобранной метки
		vector <uint32_t> decoded;
		/**
		 * Если разобрать запись метки кодировкой Punycode не вышло
		 */
		if(!awh::punycode::decode(encoded, decoded))
			// Выводим ошибку разбора записи метки
			return awh::idna::error_t::PUNYCODE;
		/**
		 * Если разобранная метка пуста
		 *
		 * @details Пустая метка, обозначенная приставкой, записи не имеет.
		 *
		 */
		if(decoded.empty())
			// Выводим ошибку разбора записи метки
			return awh::idna::error_t::PUNYCODE;
		// Выполняем запись разобранной метки
		result = decoded;
		/**
		 * Выводим результат проверки допустимости разобранной метки
		 *
		 * @details Разобранная метка проверяется без применения переходного режима,
		 *          что предписано приложением по обработке доменных имён.
		 *
		 */
		return validate(decoded, (mode & ~static_cast <uint16_t> (awh::idna::option_t::TRANSITIONAL)), inverse);
	}
};

/**
 * @brief Функция извлечения состояния символа в таблице преобразований
 *
 * @param code   кодовое значение символа
 * @param result набор кодовых значений преобразования символа
 * @return       состояние символа в таблице преобразований
 *
 */
awh::idna::status_t awh::idna::status(const uint32_t code, vector <uint32_t> & result) noexcept {
	// Выполняем очистку набора кодовых значений преобразования символа
	result.clear();
	// Получаем наименьший индекс просматриваемого участка таблицы
	size_t lower = 0;
	// Получаем наибольший индекс просматриваемого участка таблицы
	size_t upper = MAPPINGS_COUNT;
	/**
	 * Выполняем двоичный поиск диапазона в таблице преобразований
	 */
	while(lower < upper) {
		// Получаем индекс середины просматриваемого участка
		const size_t middle = (lower + ((upper - lower) / 2));
		/**
		 * Если кодовое значение находится левее диапазона
		 */
		if(code < MAPPINGS[middle].begin)
			// Продолжаем поиск в левой половине участка
			upper = middle;
		/**
		 * Если кодовое значение находится правее диапазона
		 */
		else if(code > MAPPINGS[middle].end)
			// Продолжаем поиск в правой половине участка
			lower = (middle + 1);
		/**
		 * Выводим состояние обнаруженного символа
		 */
		else {
			/**
			 * Выполняем извлечение набора кодовых значений преобразования
			 */
			for(size_t i = 0; i < MAPPINGS[middle].length; i++)
				// Выполняем добавление очередного кодового значения преобразования
				result.push_back(MAPPING_SETS[MAPPINGS[middle].offset + i]);
			// Выводим состояние символа в таблице преобразований
			return MAPPINGS[middle].status;
		}
	}
	// Выводим недопустимость символа, таблицей преобразований не заданного
	return status_t::DISALLOWED;
}
/**
 * @brief Функция извлечения описания ошибки приведения доменного имени
 *
 * @param error код ошибки приведения доменного имени
 * @return      описание ошибки приведения доменного имени
 *
 */
string_view awh::idna::message(const error_t error) noexcept {
	/**
	 * Определяем код ошибки приведения доменного имени
	 */
	switch(static_cast <uint8_t> (error)) {
		case static_cast <uint8_t> (error_t::NONE): return "no error";
		case static_cast <uint8_t> (error_t::ENCODING): return "domain name is not a valid UTF-8 sequence";
		case static_cast <uint8_t> (error_t::DISALLOWED): return "domain name contains a disallowed character";
		case static_cast <uint8_t> (error_t::PUNYCODE): return "unable to decode a punycode label";
		case static_cast <uint8_t> (error_t::HYPHEN): return "hyphen is placed in a disallowed position";
		case static_cast <uint8_t> (error_t::LEADING_MARK): return "label begins with a combining mark";
		case static_cast <uint8_t> (error_t::NOT_NORMAL): return "label is not in the normalization form NFC";
		case static_cast <uint8_t> (error_t::BIDI): return "label violates the bidirectional rule";
		case static_cast <uint8_t> (error_t::CONTEXT): return "label violates a joiner context rule";
		case static_cast <uint8_t> (error_t::LABEL_LENGTH): return "label length is out of bounds";
		case static_cast <uint8_t> (error_t::DOMAIN_LENGTH): return "domain name length is out of bounds";
	}
	// Выводим описание неизвестной ошибки приведения доменного имени
	return "unknown error";
}
/**
 * @brief Функция приведения доменного имени к записи из символов набора ASCII
 *
 * @param domain приводимое доменное имя, записанное в кодировке UTF-8
 * @param result получившаяся запись доменного имени
 * @param error  код ошибки приведения доменного имени
 * @param mode   набор режимов приведения доменного имени
 * @return       результат выполнения приведения доменного имени
 *
 */
bool awh::idna::toAscii(string_view domain, string & result, error_t & error, const uint16_t mode) noexcept {
	// Выполняем очистку получившейся записи доменного имени
	result.clear();
	// Выполняем сброс кода ошибки приведения доменного имени
	error = error_t::NONE;
	// Набор меток приводимого доменного имени
	vector <vector <uint32_t>> labels;
	/**
	 * Если разобрать доменное имя на метки не вышло
	 */
	if(!separate(domain, mode, labels, error))
		// Выводим результат выполнения приведения доменного имени
		return false;
	// Набор меток доменного имени, приведённых к записи Юникода
	vector <vector <uint32_t>> restored(labels.size());
	/**
	 * Выполняем приведение меток доменного имени к записи Юникода
	 *
	 * @details Приведение выполняется до проверки правила двунаправленного письма:
	 *          признак записи имени этим письмом снимается с разобранных меток.
	 *
	 */
	for(size_t i = 0; i < labels.size(); i++) {
		// Получаем код ошибки приведения очередной метки
		const error_t status = restore(labels.at(i), (mode & ~static_cast <uint16_t> (option_t::BIDI)), false, restored.at(i));
		/**
		 * Если привести очередную метку не вышло
		 */
		if(status != error_t::NONE) {
			// Выводим код ошибки приведения доменного имени
			error = status;
			// Выводим результат выполнения приведения доменного имени
			return false;
		}
	}
	/**
	 * Если требуется проверка правила двунаправленного письма
	 */
	if(enabled(mode, option_t::BIDI) && directional(restored)) {
		/**
		 * Выполняем проверку правила двунаправленного письма для каждой метки
		 */
		for(auto & label : restored) {
			/**
			 * Если очередная метка правилу двунаправленного письма не отвечает
			 */
			if(!bidirectional(label)) {
				// Выводим ошибку нарушения правила двунаправленного письма
				error = error_t::BIDI;
				// Выводим результат выполнения приведения доменного имени
				return false;
			}
		}
	}
	// Буфер записи символа в кодировке UTF-8
	char buffer[awh::utf8::MAX_LENGTH];
	/**
	 * Выполняем сборку записи доменного имени из символов набора ASCII
	 */
	for(size_t i = 0; i < restored.size(); i++) {
		/**
		 * Если очередная метка не является первой
		 */
		if(i > 0)
			// Выполняем добавление разделителя меток доменного имени
			result.append(1, '.');
		// Запись очередной метки доменного имени
		string encoded = "";
		// Признак принадлежности всех символов метки набору ASCII
		bool plain = true;
		/**
		 * Выполняем проверку принадлежности символов метки набору ASCII
		 */
		for(auto & code : restored.at(i)) {
			/**
			 * Если очередной символ метки набору ASCII не принадлежит
			 */
			if(code > 0x7F) {
				// Запоминаем принадлежность метки Юникоду
				plain = false;
				// Прекращаем проверку символов метки
				break;
			}
		}
		/**
		 * Если все символы метки принадлежат набору ASCII
		 */
		if(plain) {
			/**
			 * Выполняем запись метки без изменений
			 */
			for(auto & code : restored.at(i))
				// Выполняем добавление символа метки
				encoded.append(1, static_cast <char> (code));
		/**
		 * Если метка содержит символы Юникода
		 */
		} else {
			// Запись метки кодировкой Punycode
			string punycoded = "";
			/**
			 * Если представить метку кодировкой Punycode не вышло
			 */
			if(!punycode::encode(restored.at(i), punycoded)) {
				// Выполняем очистку получившейся записи доменного имени
				result.clear();
				// Выводим ошибку разбора записи метки
				error = error_t::PUNYCODE;
				// Выводим результат выполнения приведения доменного имени
				return false;
			}
			// Выполняем добавление приставки записи метки кодировкой Punycode
			encoded.append(PREFIX);
			// Выполняем добавление записи метки кодировкой Punycode
			encoded.append(punycoded);
		}
		/**
		 * Если требуется проверка длины доменного имени и его меток
		 */
		if(enabled(mode, option_t::LENGTH) && (encoded.empty() || (encoded.size() > MAX_LABEL))) {
			/**
			 * Если пустой является последняя метка доменного имени
			 *
			 * @details Доменное имя, завершающееся разделителем меток, задаёт имя
			 *          от корня зоны, и его последняя метка пуста намеренно.
			 *
			 */
			if(!(encoded.empty() && ((i + 1) == restored.size()) && (i > 0))) {
				// Выполняем очистку получившейся записи доменного имени
				result.clear();
				// Выводим ошибку длины метки доменного имени
				error = error_t::LABEL_LENGTH;
				// Выводим результат выполнения приведения доменного имени
				return false;
			}
		}
		// Выполняем добавление записи метки к доменному имени
		result.append(encoded);
	}
	// Заглушаем предупреждение о неиспользуемом буфере записи символа
	(void) buffer;
	/**
	 * Если требуется проверка длины доменного имени и его меток
	 */
	if(enabled(mode, option_t::LENGTH)) {
		// Получаем длину получившейся записи доменного имени
		size_t length = result.size();
		/**
		 * Если доменное имя завершается разделителем меток
		 */
		if((length > 0) && (result.back() == '.'))
			// Выполняем исключение завершающего разделителя из длины
			length--;
		/**
		 * Если длина доменного имени недопустима
		 */
		if((length < 1) || (length > MAX_DOMAIN)) {
			// Выполняем очистку получившейся записи доменного имени
			result.clear();
			// Выводим ошибку длины доменного имени
			error = error_t::DOMAIN_LENGTH;
			// Выводим результат выполнения приведения доменного имени
			return false;
		}
	}
	// Выводим результат выполнения приведения доменного имени
	return true;
}
/**
 * @brief Функция приведения доменного имени к записи из символов набора ASCII
 *
 * @param domain приводимое доменное имя, записанное в кодировке UTF-8
 * @param mode   набор режимов приведения доменного имени
 * @return       получившаяся запись доменного имени
 *
 */
string awh::idna::toAscii(string_view domain, const uint16_t mode) noexcept {
	// Получившаяся запись доменного имени
	string result = "";
	// Код ошибки приведения доменного имени
	error_t error = error_t::NONE;
	// Выполняем приведение доменного имени к записи из символов набора ASCII
	toAscii(domain, result, error, mode);
	// Выводим получившуюся запись доменного имени
	return result;
}
/**
 * @brief Функция приведения доменного имени к записи Юникода
 *
 * @param domain приводимое доменное имя
 * @param result получившаяся запись доменного имени в кодировке UTF-8
 * @param error  код ошибки приведения доменного имени
 * @param mode   набор режимов приведения доменного имени
 * @return       результат выполнения приведения доменного имени
 *
 */
bool awh::idna::toUnicode(string_view domain, string & result, error_t & error, const uint16_t mode) noexcept {
	// Выполняем очистку получившейся записи доменного имени
	result.clear();
	// Выполняем сброс кода ошибки приведения доменного имени
	error = error_t::NONE;
	// Набор меток приводимого доменного имени
	vector <vector <uint32_t>> labels;
	/**
	 * Если разобрать доменное имя на метки не вышло
	 */
	if(!separate(domain, mode, labels, error))
		// Выводим результат выполнения приведения доменного имени
		return false;
	// Набор меток доменного имени, приведённых к записи Юникода
	vector <vector <uint32_t>> restored(labels.size());
	// Код ошибки приведения доменного имени, обнаруженной при разборе меток
	error_t detected = error_t::NONE;
	/**
	 * Выполняем приведение меток доменного имени к записи Юникода
	 */
	for(size_t i = 0; i < labels.size(); i++) {
		// Получаем код ошибки приведения очередной метки
		const error_t status = restore(labels.at(i), (mode & ~static_cast <uint16_t> (option_t::BIDI)), false, restored.at(i));
		/**
		 * Если привести очередную метку не вышло
		 *
		 * @details Метка, привести которую не вышло, выводится без изменений:
		 *          обратное приведение служит показу имени человеку, и показ имени
		 *          с неразобранной меткой полезнее отказа.
		 *
		 */
		if(status != error_t::NONE) {
			// Запоминаем код обнаруженной ошибки приведения
			detected = status;
			// Выполняем запись метки без изменений
			restored.at(i) = labels.at(i);
		}
	}
	/**
	 * Если требуется проверка правила двунаправленного письма
	 */
	if(enabled(mode, option_t::BIDI) && directional(restored)) {
		/**
		 * Выполняем проверку правила двунаправленного письма для каждой метки
		 */
		for(auto & label : restored) {
			// Если очередная метка правилу двунаправленного письма не отвечает
			if(!bidirectional(label))
				// Запоминаем ошибку нарушения правила двунаправленного письма
				detected = error_t::BIDI;
		}
	}
	/**
	 * Если требуется проверка длины доменного имени и его меток
	 */
	if(enabled(mode, option_t::LENGTH)) {
		/**
		 * Выполняем проверку длины меток доменного имени
		 */
		for(size_t i = 0; i < restored.size(); i++) {
			/**
			 * Если очередная метка не пуста
			 */
			if(!restored.at(i).empty())
				// Переходим к следующей метке доменного имени
				continue;
			/**
			 * Если пустой является последняя метка доменного имени
			 *
			 * @details Доменное имя, завершающееся разделителем меток, задаёт имя
			 *          от корня зоны, и его последняя метка пуста намеренно.
			 *
			 */
			if(((i + 1) == restored.size()) && (i > 0))
				// Переходим к следующей метке доменного имени
				continue;
			// Запоминаем ошибку длины метки доменного имени
			detected = error_t::LABEL_LENGTH;
		}
	}
	// Буфер записи символа в кодировке UTF-8
	char buffer[awh::utf8::MAX_LENGTH];
	/**
	 * Выполняем сборку записи доменного имени в кодировке UTF-8
	 */
	for(size_t i = 0; i < restored.size(); i++) {
		/**
		 * Если очередная метка не является первой
		 */
		if(i > 0)
			// Выполняем добавление разделителя меток доменного имени
			result.append(1, '.');
		/**
		 * Выполняем запись символов очередной метки
		 */
		for(auto & code : restored.at(i)) {
			// Выполняем представление кодового значения записью UTF-8
			const size_t length = awh::utf8::encode(code, buffer);
			/**
			 * Если представить кодовое значение записью не вышло
			 */
			if(length == 0) {
				// Выполняем очистку получившейся записи доменного имени
				result.clear();
				// Выводим ошибку записи доменного имени
				error = error_t::ENCODING;
				// Выводим результат выполнения приведения доменного имени
				return false;
			}
			// Выполняем добавление записи символа метки
			result.append(buffer, length);
		}
	}
	// Выводим код обнаруженной ошибки приведения доменного имени
	error = detected;
	// Выводим результат выполнения приведения доменного имени
	return (detected == error_t::NONE);
}
/**
 * @brief Функция приведения доменного имени к записи Юникода
 *
 * @param domain приводимое доменное имя
 * @param mode   набор режимов приведения доменного имени
 * @return       получившаяся запись доменного имени в кодировке UTF-8
 *
 */
string awh::idna::toUnicode(string_view domain, const uint16_t mode) noexcept {
	// Получившаяся запись доменного имени
	string result = "";
	// Код ошибки приведения доменного имени
	error_t error = error_t::NONE;
	// Выполняем приведение доменного имени к записи Юникода
	toUnicode(domain, result, error, mode);
	// Выводим получившуюся запись доменного имени
	return result;
}
