/**
 * @file document.cpp
 * @date 2026-08-01
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
 * @brief Реализация дерева разметки XML — сборка дерева из событий потокового чтения,
 *        размещение узлов в арене и обход дерева узлами-держателями
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <cstring>
#include <algorithm>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <codec/xml/document.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние служебные объекты
 *
 */
namespace {
	/**
	 * Пространство имён контейнера XML
	 */
	using namespace awh::codec::xml;

	/**
	 * @brief Наибольший допустимый объём хранилища знаков дерева разметки
	 *
	 * @details Узлы дерева указывают на содержимое положением в общем хранилище, а
	 *          положение хранится четырьмя байтами: текст свыше этого объёма деревом
	 *          не разбирается, и для него предназначено потоковое чтение
	 *
	 */
	constexpr size_t STORAGE_LIMIT = 0xFFFFFFFF;
};

/**
 * @brief Конструктор
 *
 */
awh::codec::xml::Document::Record::Record() noexcept :
 kind(kind_t::NONE), attribute(0), attributes(0), parent(INVALID_NODE),
 first(INVALID_NODE), last(INVALID_NODE), next(INVALID_NODE), prev(INVALID_NODE) {}
/**
 * @brief Метод получения последовательности знаков по отрезку хранилища
 *
 * @param span отрезок общего хранилища знаков
 * @return     последовательность знаков указанного отрезка
 *
 */
string_view awh::codec::xml::Document::get(const span_t & span) const noexcept {
	/**
	 * Если отрезок выходит за пределы хранилища знаков
	 */
	if((static_cast <size_t> (span.offset) + span.length) > this->_storage.size())
		// Выводим пустую последовательность знаков
		return string_view();
	// Выводим последовательность знаков указанного отрезка
	return string_view(this->_storage.data() + span.offset, span.length);
}
/**
 * @brief Метод сборки имени по записи арены
 *
 * @param title запись имени в арене дерева разметки
 * @return      имя с учётом пространства имён
 *
 */
awh::codec::xml::name_t awh::codec::xml::Document::get(const title_t & title) const noexcept {
	// Собираемое имя с учётом пространства имён
	name_t result;
	// Запоминаем префикс имени
	result.prefix = this->get(title.prefix);
	// Запоминаем местное имя
	result.local = this->get(title.local);
	// Запоминаем обозначение пространства имён
	result.uri = this->get(title.uri);
	// Выводим собранное имя
	return result;
}
/**
 * @brief Метод разбора текста разметки
 *
 * @param text     исходный текст разметки целиком
 * @param settings настройки разбора текста разметки
 * @return         результат выполнения операции
 *
 */
bool awh::codec::xml::Document::parse(const string_view text, const reader_t::settings_t & settings) noexcept {
	// Выполняем очистку дерева разметки
	this->clear();
	// Объект потокового чтения текста разметки
	reader_t reader(settings);
	// Количество начал разметки в исходном тексте
	size_t tags = 0;
	{
		// Количество разделителей имени и значения атрибута в исходном тексте
		size_t pairs = 0;
		// Получаем конец исходного текста разметки
		const char * const finish = (text.data() + text.length());
		/**
		 * Выполняем перебор всех начал разметки исходного текста
		 */
		for(const char * offset = text.data(); offset < finish; offset++){
			// Выполняем поиск очередного начала разметки
			offset = reinterpret_cast <const char *> (::memchr(offset, '<', static_cast <size_t> (finish - offset)));
			// Если начал разметки больше нет, перебор прекращается
			if(offset == nullptr)
				// Выходим из перебора начал разметки
				break;
			// Выполняем подсчёт начал разметки исходного текста
			tags++;
		}
		/**
		 * Выполняем перебор всех разделителей имени и значения атрибута
		 *
		 * @note Разделитель приходится ровно на один атрибут, а встреченный в содержимом
		 *       узла оценку лишь завышает: занижена она быть не может
		 */
		for(const char * offset = text.data(); offset < finish; offset++){
			// Выполняем поиск очередного разделителя имени и значения
			offset = reinterpret_cast <const char *> (::memchr(offset, '=', static_cast <size_t> (finish - offset)));
			// Если разделителей больше нет, перебор прекращается
			if(offset == nullptr)
				// Выходим из перебора разделителей
				break;
			// Выполняем подсчёт разделителей имени и значения атрибута
			pairs++;
		}
		/**
		 * Приводим оценку к тому, что способен дать правильно построенный текст
		 *
		 * @details Оценка снимается с исходного текста до его разбора, и подставной
		 *          текст завышает её без предела: в мегабайте одних лишь начал разметки
		 *          их миллион, а узлом из них не станет ни один. Отвод места по такой
		 *          оценке занял бы гигабайты под дерево, которое останется пустым
		 *
		 * @note Всякое начало разметки открывает в правильно построенном тексте узел
		 *       длиной не менее трёх байтов, а всякий атрибут занимает не менее четырёх:
		 *       выше этого оценка достоверной быть не может
		 *
		 * @warning Отводу положен и предел сверху. Отвод места - предположение, а не
		 *          обязательство, и подкреплять предположение отказом в памяти нельзя:
		 *          разбор объявлен не выбрасывающим исключений, и отказ в отводе прекратил
		 *          бы работу приложения целиком. За пределом рост идёт удвоением, теряя
		 *          лишь последние несколько перекладываний
		 */
		constexpr size_t LIMIT = 0x100000;
		// Приводим оценку количества узлов к достоверной
		tags = ::min(tags, (text.length() / 3));
		// Приводим оценку количества атрибутов к достоверной
		pairs = ::min(pairs, (text.length() / 4));
		// Выполняем отвод места под узлы дерева разметки
		this->_nodes.reserve(::min(tags, LIMIT) + 1);
		// Выполняем отвод места под атрибуты узлов дерева разметки
		this->_attributes.reserve(::min(pairs, LIMIT));
		// Выполняем отвод места под общее хранилище знаков
		this->_storage.reserve(text.length());
	}
	/**
	 * Признак выхода дерева за отведённый ему предел
	 *
	 * @details Узлы дерева указывают на содержимое и друг на друга положением в четыре
	 *          байта: текст, дающий дерево крупнее, деревом не разбирается, и для него
	 *          предназначено потоковое чтение. Проверка ведётся при самом размещении, а
	 *          не по его итогу: выход за предел усёк бы положение молча, и дерево вышло
	 *          бы не с отказом, а с отрезками, указывающими не туда
	 */
	bool overflow = false;
	/**
	 * @brief Метод размещения последовательности знаков в хранилище дерева
	 *
	 * @param value размещаемая последовательность знаков
	 * @return      отрезок общего хранилища знаков
	 *
	 */
	auto store = [this, &overflow](const string_view value) noexcept -> span_t {
		/**
		 * Если размещение выведет хранилище знаков за предел
		 *
		 * @note Отрезок хранилища задан положением в четыре байта, и выход за предел
		 *       усёк бы положение молча, оставив дерево с отрезками, указывающими не туда
		 *
		 * @warning Ограда эта набором НЕ СЛИЧАЕТСЯ и сличена быть не может дёшево:
		 *          добраться до неё требует четырёх гигабайтов исходного текста. Молчание
		 *          карты покрытия здесь означает НЕПРОВЕРЕННОСТЬ, а не исправность, и
		 *          всякая правка размещения в хранилище обязана перечитывать эту ветвь
		 *          глазами. Предел этот разрядностью отрезка задан, а не выбором, и
		 *          настройкою не открывается: открыть его значило бы дозволить усечение
		 */
		if((this->_storage.size() + value.length()) > STORAGE_LIMIT){
			// Запоминаем, что дерево вышло за отведённый ему предел
			overflow = true;
			// Выводим пустой отрезок общего хранилища знаков
			return span_t();
		}
		// Собираемый отрезок общего хранилища знаков
		const span_t result(static_cast <uint32_t> (this->_storage.size()), static_cast <uint32_t> (value.length()));
		// Выполняем размещение последовательности знаков в хранилище
		this->_storage.append(value);
		// Выводим собранный отрезок общего хранилища знаков
		return result;
	};
	/**
	 * Признак сведения повторяющихся имён к единственному размещению
	 *
	 * @details Таблица повторов окупается лишь на дереве, где повторам есть откуда
	 *          взяться: у документа в десяток узлов сводить нечего, а заведение самой
	 *          таблицы обошлось бы дороже размещённых ею имён. Порог взят по числу
	 *          начал разметки - оно оценивается тем же проходом, что и отвод места
	 */
	const bool merge = (tags >= 128);
	// Буфер ключа отыскания в таблице размещённых имён
	string key;
	/**
	 * @brief Метод размещения части имени в хранилище дерева со сведением повторов
	 *
	 * @param value размещаемая часть имени
	 * @return      отрезок общего хранилища знаков
	 *
	 */
	auto intern = [this, &store, &key, merge](const string_view value) noexcept -> span_t {
		/**
		 * Если повторы не сводятся либо размещаемая часть имени пуста
		 *
		 * @note Пустое имя места не занимает, и заводить ему запись в таблице незачем
		 */
		if(!merge || value.empty())
			// Выводим отрезок размещённой части имени
			return store(value);
		// Выполняем сборку ключа отыскания размещённой части имени
		key.assign(value);
		// Выполняем отыскание размещённой части имени в таблице
		auto i = this->_interned.find(key);
		/**
		 * Если часть имени в хранилище уже размещена
		 */
		if(i != this->_interned.end())
			// Выводим отрезок размещённой части имени
			return i->second;
		// Выполняем размещение части имени в хранилище знаков
		const span_t result = store(value);
		// Выполняем добавление размещённой части имени к таблице
		this->_interned.emplace(key, result);
		// Выводим собранный отрезок общего хранилища знаков
		return result;
	};
	/**
	 * @brief Метод размещения имени в хранилище дерева
	 *
	 * @param name размещаемое имя с учётом пространства имён
	 * @return     запись имени в арене дерева разметки
	 *
	 */
	auto title = [&store, &intern](const name_t & name) noexcept -> title_t {
		// Собираемая запись имени
		title_t result;
		/**
		 * Выполняем размещение префикса и местного имени
		 *
		 * @details Местное имя и префикс - собственная запись узла, и повторяются они
		 *          лишь тогда, когда узлы одинаково названы. Отыскание их в таблице
		 *          повторов обходится дороже размещения: замер на ответе по договору
		 *          SOAP дал шестую часть скорости разбора за двадцатую часть памяти
		 */
		result.prefix = store(name.prefix);
		// Выполняем размещение местного имени
		result.local = store(name.local);
		/**
		 * Выполняем размещение обозначения пространства имён со сведением повторов
		 *
		 * @details Обозначение пространства имён собственной записью узла не является:
		 *          оно берётся из связывания префикса и по устройству разметки одно и
		 *          то же у всех узлов области видимости. Размещать его при каждом узле
		 *          значит переписывать одну и ту же последовательность знаков тысячи
		 *          раз, а длиной она превосходит и имя узла, и содержимое его
		 */
		result.uri = intern(name.uri);
		// Выводим собранную запись имени
		return result;
	};
	/**
	 * @brief Метод добавления узла к дереву разметки
	 *
	 * @param parent индекс родительского узла
	 * @param kind   вид добавляемого узла
	 * @return       индекс добавленного узла
	 *
	 */
	auto append = [this, &overflow](const node_id_t parent, const kind_t kind) noexcept -> node_id_t {
		/**
		 * Если добавление выведет количество узлов за предел
		 *
		 * @note Узлы указывают друг на друга индексом в четыре байта, и обозначение
		 *       отсутствующего узла из этого счёта изъято: дерево обязано остановиться,
		 *       не дойдя до него, иначе связи узлов указали бы в пустоту
		 *
		 * @warning Ограда эта набором НЕ СЛИЧАЕТСЯ, как и ограда хранилища выше: добраться
		 *          до неё требует четырёх миллиардов узлов настоящим текстом. Молчание
		 *          карты покрытия здесь означает НЕПРОВЕРЕННОСТЬ, а не исправность
		 */
		if(this->_nodes.size() >= INVALID_NODE){
			// Запоминаем, что дерево вышло за отведённый ему предел
			overflow = true;
			// Выводим обозначение непригодного узла дерева
			return INVALID_NODE;
		}
		// Получаем индекс добавляемого узла
		const node_id_t result = static_cast <node_id_t> (this->_nodes.size());
		// Собираемая запись узла
		record_t record;
		// Запоминаем вид добавляемого узла
		record.kind = kind;
		// Запоминаем индекс родительского узла
		record.parent = parent;
		// Выполняем добавление записи узла к арене
		this->_nodes.push_back(record);
		/**
		 * Если родительский узел указан
		 */
		if(parent != INVALID_NODE){
			// Получаем родительский узел дерева
			record_t & owner = this->_nodes[parent];
			/**
			 * Если родительский узел уже содержит вложенные
			 */
			if(owner.last != INVALID_NODE){
				// Запоминаем предыдущий узел того же уровня
				this->_nodes[result].prev = owner.last;
				// Запоминаем следующий узел того же уровня
				this->_nodes[owner.last].next = result;
			// Запоминаем индекс первого вложенного узла
			} else owner.first = result;
			// Запоминаем индекс последнего вложенного узла
			owner.last = result;
		}
		// Выводим индекс добавленного узла
		return result;
	};
	// Выполняем добавление корня дерева разметки
	append(INVALID_NODE, kind_t::DOCUMENT);
	// Индекс текущего родительского узла
	node_id_t parent = 0;
	/**
	 * Если передачу исходного текста выполнить не удалось
	 */
	/**
	 * Если подачу текста разметки чтению выполнить не удалось
	 *
	 * @warning Прикрытие это набором НЕ ПРОВЕРЯЕТСЯ и является НЕДОСТИЖИМЫМ: подача
	 *          отказывает лишь у чтения, уже отказавшего, завершённого либо
	 *          споткнувшегося на кодировке, а чтение здесь заводится ЗАНОВО на всякий
	 *          разбор и подаётся ему единожды. Отказы же кодировки приходят не сюда,
	 *          а событием разбора - проверены отдельно
	 * @note Молчание карты покрытия здесь означает недостижимость, а не
	 *       непроверенность; снимать прикрытие нельзя - оно оберегает от правки,
	 *       заводящей чтение полем либо подающей текст дважды
	 */
	if(!reader.feed(text)){
		// Запоминаем код ошибки разбора
		const error_t error = reader.error();
		// Запоминаем положение обнаруженной ошибки
		const location_t location = reader.errorLocation();
		/**
		 * Выполняем очистку дерева разметки
		 *
		 * @note Очистка сбрасывает и сведения об ошибке, поэтому они снимаются до неё
		 */
		this->clear();
		// Запоминаем код ошибки разбора
		this->_error = error;
		// Запоминаем положение обнаруженной ошибки
		this->_errorLocation = location;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Выполняем перебор всех событий разбора
	 */
	while(reader.next()){
		/**
		 * Если дерево вышло за отведённый ему предел
		 */
		if(overflow){
			// Запоминаем положение обнаруженной ошибки
			const location_t location = reader.location();
			/**
			 * Выполняем очистку дерева разметки
			 *
			 * @note Очистка сбрасывает и сведения об ошибке, поэтому они снимаются до неё
			 */
			this->clear();
			// Запоминаем код ошибки разбора
			this->_error = error_t::OVERFLOW_LIMIT;
			// Запоминаем положение обнаруженной ошибки
			this->_errorLocation = location;
			// Выводим отрицательный результат выполнения операции
			return false;
		}
		/**
		 * Определяем вид полученного события разбора
		 */
		switch(static_cast <uint8_t> (reader.event())){
			/**
			 * Если получено начало узла разметки
			 */
			case static_cast <uint8_t> (event_t::ELEMENT_OPEN): {
				// Выполняем добавление узла разметки к дереву
				const node_id_t id = append(parent, kind_t::ELEMENT);
				// Если добавить узел не удалось, разбор прекращается пределом
				if(id == INVALID_NODE) break;
				// Запоминаем имя узла разметки
				this->_nodes[id].name = title(reader.name());
				// Запоминаем положение узла в исходном тексте
				this->_nodes[id].location = reader.location();
				// Запоминаем индекс первого атрибута узла
				this->_nodes[id].attribute = static_cast <uint32_t> (this->_attributes.size());
				// Запоминаем количество атрибутов узла
				this->_nodes[id].attributes = static_cast <uint32_t> (reader.attributes().size());
				/**
				 * Если узел объявил пространства имён
				 *
				 * @note Отрезок хранилища связываний заводится лишь объявившим узлам: объявляют
				 *       их считанные из всего дерева, а место в записи узла заняли бы все
				 */
				if(!reader.bindings().empty()){
					// Собираемый отрезок хранилища связываний узла
					const span_t range(static_cast <uint32_t> (this->_scopes.size()), static_cast <uint32_t> (reader.bindings().size()));
					/**
					 * Выполняем перебор всех объявлений пространств имён узла
					 */
					for(const binding_t & binding : reader.bindings()){
						// Собираемая запись связывания префикса
						scope_t scope;
						// Выполняем размещение префикса объявления
						scope.prefix = store(binding.prefix);
						// Выполняем размещение обозначения объявляемого пространства имён
						scope.uri = intern(binding.uri);
						// Выполняем добавление записи связывания к хранилищу
						this->_scopes.push_back(scope);
					}
					// Выполняем привязку отрезка хранилища связываний к узлу
					this->_scoped.emplace(id, range);
				}
				/**
				 * Выполняем перебор всех атрибутов узла
				 */
				for(const attribute_t & attribute : reader.attributes()){
					/**
					 * Если добавление выведет количество атрибутов за предел
					 */
					if(this->_attributes.size() >= STORAGE_LIMIT){
						// Запоминаем, что дерево вышло за отведённый ему предел
						overflow = true;
						// Выходим из перебора атрибутов узла
						break;
					}
					// Собираемая запись атрибута
					property_t property;
					// Выполняем размещение имени атрибута
					property.name = title(attribute.name);
					// Выполняем размещение значения атрибута
					property.value = store(attribute.value);
					// Запоминаем положение атрибута в исходном тексте
					property.location = attribute.location;
					// Запоминаем признак значения, взятого из объявления по умолчанию
					property.defaulted = attribute.defaulted;
					// Выполняем добавление записи атрибута к хранилищу
					this->_attributes.push_back(property);
				}
				// Выполняем переход к вложенным узлам
				parent = id;
			} break;
			/**
			 * Если получен конец узла разметки
			 */
			case static_cast <uint8_t> (event_t::ELEMENT_CLOSE): {
				/**
				 * Если родительский узел указан
				 */
				if(parent != INVALID_NODE)
					// Выполняем возврат к родительскому узлу
					parent = this->_nodes[parent].parent;
			} break;
			/**
			 * Если получено содержимое узла
			 */
			case static_cast <uint8_t> (event_t::TEXT):
			case static_cast <uint8_t> (event_t::SPACE):
			case static_cast <uint8_t> (event_t::CDATA):
			case static_cast <uint8_t> (event_t::COMMENT):
			case static_cast <uint8_t> (event_t::DOCTYPE):
			case static_cast <uint8_t> (event_t::PROCESSING): {
				// Вид добавляемого узла дерева
				kind_t kind = kind_t::TEXT;
				/**
				 * Определяем вид полученного события разбора
				 */
				switch(static_cast <uint8_t> (reader.event())){
					// Если получен раздел дословного текста
					case static_cast <uint8_t> (event_t::CDATA): kind = kind_t::CDATA; break;
					// Если получено незначимое пробельное содержимое
					case static_cast <uint8_t> (event_t::SPACE): kind = kind_t::SPACE; break;
					// Если получено примечание
					case static_cast <uint8_t> (event_t::COMMENT): kind = kind_t::COMMENT; break;
					// Если получено описание типа документа
					case static_cast <uint8_t> (event_t::DOCTYPE): kind = kind_t::DOCTYPE; break;
					// Если получено указание обработчику
					case static_cast <uint8_t> (event_t::PROCESSING): kind = kind_t::PROCESSING; break;
				}
				// Выполняем добавление узла к дереву
				const node_id_t id = append(parent, kind);
				// Если добавить узел не удалось, разбор прекращается пределом
				if(id == INVALID_NODE) break;
				// Запоминаем содержимое узла
				this->_nodes[id].value = store(reader.text());
				// Запоминаем положение узла в исходном тексте
				this->_nodes[id].location = reader.location();
				/**
				 * Если добавлено указание обработчику
				 */
				if(kind == kind_t::PROCESSING)
					// Запоминаем цель указания обработчику
					this->_nodes[id].name = title(reader.name());
			} break;
		}
	}
	/**
	 * Если разбор текста разметки прекращён ошибкой либо дерево вышло за предел
	 *
	 * @note Признак проверяется и здесь: событие, выведшее дерево за предел, бывает и
	 *       последним, и тогда проверка в начале следующего обхода не выполнится вовсе
	 */
	if(overflow || (reader.state() != state_t::FINISHED)){
		// Запоминаем код ошибки разбора
		const error_t error = (overflow ? error_t::OVERFLOW_LIMIT : reader.error());
		// Запоминаем положение обнаруженной ошибки
		const location_t location = (overflow ? reader.location() : reader.errorLocation());
		/**
		 * Выполняем очистку дерева разметки
		 *
		 * @note Очистка сбрасывает и сведения об ошибке, поэтому они снимаются до неё
		 */
		this->clear();
		// Запоминаем код ошибки разбора
		this->_error = error;
		// Запоминаем положение обнаруженной ошибки
		this->_errorLocation = location;
		// Выводим отрицательный результат выполнения операции
		return false;
	}
	/**
	 * Выполняем очистку таблицы размещённых имён
	 *
	 * @note Таблица нужна лишь на время сборки дерева: она удерживает вторую копию
	 *       каждого сведённого обозначения, а собранному дереву довольно отрезков
	 *       хранилища знаков
	 */
	this->_interned.clear();
	// Выводим положительный результат выполнения операции
	return true;
}
/**
 * @brief Метод получения кода ошибки разбора
 *
 * @return код ошибки последней операции разбора
 *
 */
awh::codec::xml::error_t awh::codec::xml::Document::error() const noexcept {
	// Выводим код ошибки последней операции разбора
	return this->_error;
}
/**
 * @brief Метод получения места обнаружения ошибки
 *
 * @return положение обнаруженной ошибки в исходном тексте
 *
 */
const awh::codec::xml::location_t & awh::codec::xml::Document::errorLocation() const noexcept {
	// Выводим положение обнаруженной ошибки в исходном тексте
	return this->_errorLocation;
}
/**
 * @brief Метод получения корня дерева
 *
 * @return корневой узел дерева разметки
 *
 */
awh::codec::xml::node_t awh::codec::xml::Document::root() const noexcept {
	/**
	 * Если дерево разметки пусто
	 */
	if(this->_nodes.empty())
		// Выводим непригодный узел дерева разметки
		return node_t();
	// Выводим корень дерева разметки
	return node_t(this, 0);
}
/**
 * @brief Метод получения корневого узла разметки
 *
 * @return единственный узел разметки верхнего уровня
 *
 */
awh::codec::xml::node_t awh::codec::xml::Document::element() const noexcept {
	/**
	 * Выполняем перебор всех узлов верхнего уровня
	 */
	for(node_t node = this->root().first(); node.valid(); node = node.next()){
		/**
		 * Если обнаружен узел разметки
		 */
		if(node.kind() == kind_t::ELEMENT)
			// Выводим обнаруженный узел разметки
			return node;
	}
	// Выводим непригодный узел дерева разметки
	return node_t();
}
/**
 * @brief Метод получения количества узлов дерева
 *
 * @return количество узлов в арене дерева разметки
 *
 */
size_t awh::codec::xml::Document::size() const noexcept {
	// Выводим количество узлов в арене дерева разметки
	return this->_nodes.size();
}
/**
 * @brief Метод проверки дерева на пустоту
 *
 * @return результат проверки
 *
 */
bool awh::codec::xml::Document::empty() const noexcept {
	// Выводим результат проверки дерева на пустоту
	return (this->_nodes.size() <= 1);
}
/**
 * @brief Метод очистки дерева разметки
 *
 */
void awh::codec::xml::Document::clear() noexcept {
	// Выполняем сброс кода ошибки разбора
	this->_error = error_t::NONE;
	// Выполняем сброс положения обнаруженной ошибки
	this->_errorLocation = location_t();
	// Выполняем очистку арены узлов дерева разметки
	this->_nodes.clear();
	// Выполняем очистку хранилища атрибутов
	this->_attributes.clear();
	// Выполняем очистку хранилища связываний префиксов
	this->_scopes.clear();
	// Выполняем очистку отрезков хранилища связываний
	this->_scoped.clear();
	// Выполняем очистку общего хранилища знаков
	this->_storage.clear();
	// Выполняем очистку таблицы размещённых имён
	this->_interned.clear();
}
/**
 * @brief Конструктор
 *
 */
awh::codec::xml::Document::Document() noexcept : _error(error_t::NONE) {}
/**
 * @brief Деструктор
 *
 */
awh::codec::xml::Document::~Document() noexcept {
	// Выполняем очистку дерева разметки
	this->clear();
}
/**
 * @brief Метод проверки узла на пригодность
 *
 * @return результат проверки
 *
 */
bool awh::codec::xml::Node::valid() const noexcept {
	// Выводим результат проверки узла на пригодность
	return ((this->_document != nullptr) && (this->_id != INVALID_NODE) && (this->_id < this->_document->_nodes.size()));
}
/**
 * @brief Оператор приведения к логическому типу
 *
 * @return результат проверки узла на пригодность
 *
 */
awh::codec::xml::Node::operator bool () const noexcept {
	// Выводим результат проверки узла на пригодность
	return this->valid();
}
/**
 * @brief Оператор сравнения
 *
 * @param node узел для сравнения
 * @return     результат сравнения
 *
 */
bool awh::codec::xml::Node::operator == (const Node & node) const noexcept {
	// Выводим результат сравнения узлов дерева
	return ((this->_document == node._document) && (this->_id == node._id));
}
/**
 * @brief Оператор сравнения
 *
 * @param node узел для сравнения
 * @return     результат сравнения
 *
 */
bool awh::codec::xml::Node::operator != (const Node & node) const noexcept {
	// Выводим обратный результат сравнения узлов дерева
	return !((this->_document == node._document) && (this->_id == node._id));
}
/**
 * @brief Метод получения вида узла
 *
 * @return вид узла дерева разметки
 *
 */
awh::codec::xml::kind_t awh::codec::xml::Node::kind() const noexcept {
	/**
	 * Если узел непригоден
	 */
	if(!this->valid())
		// Выводим неопределённый вид узла
		return kind_t::NONE;
	// Выводим вид узла дерева разметки
	return this->_document->_nodes[this->_id].kind;
}
/**
 * @brief Метод получения имени узла
 *
 * @return имя узла с учётом пространства имён
 *
 */
awh::codec::xml::name_t awh::codec::xml::Node::name() const noexcept {
	/**
	 * Если узел непригоден
	 */
	if(!this->valid())
		// Выводим пустое имя узла
		return name_t();
	// Выводим имя узла с учётом пространства имён
	return this->_document->get(this->_document->_nodes[this->_id].name);
}
/**
 * @brief Метод получения содержимого узла
 *
 * @return содержимое узла
 *
 */
string awh::codec::xml::Node::text() const noexcept {
	// Собираемое содержимое узла
	string result;
	/**
	 * Если узел непригоден
	 */
	if(!this->valid())
		// Выводим пустое содержимое узла
		return result;
	// Получаем запись текущего узла
	const Document::record_t & record = this->_document->_nodes[this->_id];
	/**
	 * Если узел разметкой не является
	 */
	if(record.kind != kind_t::ELEMENT){
		// Выполняем добавление содержимого узла
		result.append(this->_document->get(record.value));
		// Выводим собранное содержимое узла
		return result;
	}
	/**
	 * Выполняем обход всего поддерева узла
	 *
	 * @note Содержимым узла разметки считается содержимое всех вложенных в него
	 *       текстовых узлов подряд, на любой глубине: разметка внутри содержимого
	 *       текст не разрывает
	 */
	for(node_t node = this->first(); node.valid();){
		// Получаем вид очередного узла поддерева
		const kind_t kind = node.kind();
		/**
		 * Если обнаружено текстовое содержимое
		 *
		 * @note Пробельное содержимое собирается наравне с текстовым: отделено оно
		 *       или нет, задаётся настройкой разбора, а содержимое узла от настроек
		 *       разбора зависеть не вправе. Отбор ведётся видом узла
		 */
		if((kind == kind_t::TEXT) || (kind == kind_t::CDATA) || (kind == kind_t::SPACE))
			// Выполняем добавление содержимого к собираемому
			result.append(this->_document->get(this->_document->_nodes[node._id].value));
		/**
		 * Если у очередного узла есть вложенные
		 */
		if(node.first().valid()){
			// Выполняем переход к вложенному узлу
			node = node.first();
			// Выполняем переход к следующему узлу обхода
			continue;
		}
		/**
		 * Выполняем подъём по дереву до ближайшего узла того же уровня
		 */
		while(node.valid() && !node.next().valid() && (node != *this))
			// Выполняем переход к родительскому узлу
			node = node.parent();
		/**
		 * Если обход поддерева завершён
		 */
		if(!node.valid() || (node == *this))
			// Выходим из обхода поддерева
			break;
		// Выполняем переход к следующему узлу того же уровня
		node = node.next();
	}
	// Выводим собранное содержимое узла
	return result;
}
/**
 * @brief Метод получения места узла в исходном тексте
 *
 * @return положение начала узла в исходном тексте
 *
 */
awh::codec::xml::location_t awh::codec::xml::Node::location() const noexcept {
	/**
	 * Если узел непригоден
	 */
	if(!this->valid())
		// Выводим неопределённое положение
		return location_t();
	// Выводим положение начала узла в исходном тексте
	return this->_document->_nodes[this->_id].location;
}
/**
 * @brief Метод получения родительского узла
 *
 * @return родительский узел дерева разметки
 *
 */
awh::codec::xml::node_t awh::codec::xml::Node::parent() const noexcept {
	/**
	 * Если узел непригоден
	 */
	if(!this->valid())
		// Выводим непригодный узел дерева разметки
		return node_t();
	// Выводим родительский узел дерева разметки
	return node_t(this->_document, this->_document->_nodes[this->_id].parent);
}
/**
 * @brief Метод получения первого вложенного узла
 *
 * @return первый вложенный узел дерева разметки
 *
 */
awh::codec::xml::node_t awh::codec::xml::Node::first() const noexcept {
	/**
	 * Если узел непригоден
	 */
	if(!this->valid())
		// Выводим непригодный узел дерева разметки
		return node_t();
	// Выводим первый вложенный узел дерева разметки
	return node_t(this->_document, this->_document->_nodes[this->_id].first);
}
/**
 * @brief Метод получения последнего вложенного узла
 *
 * @return последний вложенный узел дерева разметки
 *
 */
awh::codec::xml::node_t awh::codec::xml::Node::last() const noexcept {
	/**
	 * Если узел непригоден
	 */
	if(!this->valid())
		// Выводим непригодный узел дерева разметки
		return node_t();
	// Выводим последний вложенный узел дерева разметки
	return node_t(this->_document, this->_document->_nodes[this->_id].last);
}
/**
 * @brief Метод получения следующего узла того же уровня
 *
 * @return следующий узел того же уровня вложенности
 *
 */
awh::codec::xml::node_t awh::codec::xml::Node::next() const noexcept {
	/**
	 * Если узел непригоден
	 */
	if(!this->valid())
		// Выводим непригодный узел дерева разметки
		return node_t();
	// Выводим следующий узел того же уровня вложенности
	return node_t(this->_document, this->_document->_nodes[this->_id].next);
}
/**
 * @brief Метод получения предыдущего узла того же уровня
 *
 * @return предыдущий узел того же уровня вложенности
 *
 */
awh::codec::xml::node_t awh::codec::xml::Node::prev() const noexcept {
	/**
	 * Если узел непригоден
	 */
	if(!this->valid())
		// Выводим непригодный узел дерева разметки
		return node_t();
	// Выводим предыдущий узел того же уровня вложенности
	return node_t(this->_document, this->_document->_nodes[this->_id].prev);
}
/**
 * @brief Метод поиска вложенного узла разметки по имени
 *
 * @param local местное имя искомого узла
 * @param uri   обозначение пространства имён искомого узла
 * @return      найденный узел дерева разметки
 *
 */
awh::codec::xml::node_t awh::codec::xml::Node::child(const string_view local, const string_view uri) const noexcept {
	/**
	 * Выполняем перебор всех непосредственно вложенных узлов
	 */
	for(node_t node = this->first(); node.valid(); node = node.next()){
		/**
		 * Если обнаружен узел разметки с искомым именем
		 */
		if((node.kind() == kind_t::ELEMENT) && node.name().is(uri, local))
			// Выводим обнаруженный узел разметки
			return node;
	}
	// Выводим непригодный узел дерева разметки
	return node_t();
}
/**
 * @brief Метод получения перечня вложенных узлов разметки по имени
 *
 * @param local местное имя искомых узлов
 * @param uri   обозначение пространства имён искомых узлов
 * @return      перечень найденных узлов дерева разметки
 *
 */
vector <awh::codec::xml::node_t> awh::codec::xml::Node::children(const string_view local, const string_view uri) const noexcept {
	// Собираемый перечень найденных узлов
	vector <node_t> result;
	/**
	 * Выполняем перебор всех непосредственно вложенных узлов
	 */
	for(node_t node = this->first(); node.valid(); node = node.next()){
		/**
		 * Если обнаружен узел разметки с искомым именем
		 */
		if((node.kind() == kind_t::ELEMENT) && node.name().is(uri, local))
			// Выполняем добавление обнаруженного узла к перечню
			result.push_back(node);
	}
	// Выводим собранный перечень найденных узлов
	return result;
}
/**
 * @brief Метод поиска узла разметки вглубь дерева по имени
 *
 * @param local местное имя искомого узла
 * @param uri   обозначение пространства имён искомого узла
 * @return      найденный узел дерева разметки
 *
 */
awh::codec::xml::node_t awh::codec::xml::Node::find(const string_view local, const string_view uri) const noexcept {
	/**
	 * Если узел непригоден
	 */
	if(!this->valid())
		// Выводим непригодный узел дерева разметки
		return node_t();
	/**
	 * Выполняем обход всего поддерева узла в порядке следования в исходном тексте
	 */
	for(node_t node = this->first(); node.valid();){
		/**
		 * Если обнаружен узел разметки с искомым именем
		 */
		if((node.kind() == kind_t::ELEMENT) && node.name().is(uri, local))
			// Выводим обнаруженный узел разметки
			return node;
		/**
		 * Если у очередного узла есть вложенные
		 */
		if(node.first().valid()){
			// Выполняем переход к вложенному узлу
			node = node.first();
			// Выполняем переход к следующему узлу обхода
			continue;
		}
		/**
		 * Выполняем подъём по дереву до ближайшего узла того же уровня
		 */
		while(node.valid() && !node.next().valid() && (node != *this))
			// Выполняем переход к родительскому узлу
			node = node.parent();
		/**
		 * Если обход поддерева завершён
		 */
		if(!node.valid() || (node == *this))
			// Выходим из обхода поддерева
			break;
		// Выполняем переход к следующему узлу того же уровня
		node = node.next();
	}
	// Выводим непригодный узел дерева разметки
	return node_t();
}
/**
 * @brief Метод получения перечня атрибутов узла
 *
 * @return перечень атрибутов узла разметки
 *
 */
vector <awh::codec::xml::attribute_t> awh::codec::xml::Node::attributes() const noexcept {
	// Собираемый перечень атрибутов узла
	vector <attribute_t> result;
	/**
	 * Если узел непригоден
	 */
	if(!this->valid())
		// Выводим пустой перечень атрибутов
		return result;
	// Получаем запись текущего узла
	const Document::record_t & record = this->_document->_nodes[this->_id];
	/**
	 * Выполняем перебор всех атрибутов узла
	 */
	for(uint32_t i = 0; i < record.attributes; i++){
		// Получаем запись очередного атрибута
		const Document::property_t & property = this->_document->_attributes[record.attribute + i];
		// Собираемый атрибут узла
		attribute_t attribute;
		// Запоминаем имя атрибута
		attribute.name = this->_document->get(property.name);
		// Запоминаем значение атрибута
		attribute.value = this->_document->get(property.value);
		// Запоминаем положение атрибута в исходном тексте
		attribute.location = property.location;
		// Запоминаем признак значения, взятого из объявления по умолчанию
		attribute.defaulted = property.defaulted;
		// Выполняем добавление атрибута к перечню
		result.push_back(attribute);
	}
	// Выводим собранный перечень атрибутов узла
	return result;
}
/**
 * @brief Метод получения перечня объявлений пространств имён узла
 *
 * @return перечень связываний префиксов, объявленных узлом
 *
 */
vector <awh::codec::xml::binding_t> awh::codec::xml::Node::bindings() const noexcept {
	// Собираемый перечень объявлений пространств имён узла
	vector <binding_t> result;
	/**
	 * Если узел непригоден
	 */
	if(!this->valid())
		// Выводим пустой перечень объявлений
		return result;
	// Выполняем отыскание отрезка хранилища связываний узла
	auto i = this->_document->_scoped.find(this->_id);
	/**
	 * Если узел пространств имён не объявлял
	 */
	if(i == this->_document->_scoped.end())
		// Выводим пустой перечень объявлений
		return result;
	// Выполняем отвод места под перечень объявлений узла
	result.reserve(i->second.length);
	/**
	 * Выполняем перебор всех связываний, объявленных узлом
	 */
	for(uint32_t index = 0; index < i->second.length; index++){
		// Получаем запись очередного связывания префикса
		const Document::scope_t & scope = this->_document->_scopes[i->second.offset + index];
		// Собираемое объявление пространства имён
		binding_t binding;
		// Запоминаем префикс объявления
		binding.prefix = this->_document->get(scope.prefix);
		// Запоминаем обозначение объявляемого пространства имён
		binding.uri = this->_document->get(scope.uri);
		// Выполняем добавление объявления к перечню узла
		result.push_back(binding);
	}
	// Выводим собранный перечень объявлений пространств имён узла
	return result;
}
/**
 * @brief Метод получения значения атрибута узла
 *
 * @param local местное имя искомого атрибута
 * @param uri   обозначение пространства имён искомого атрибута
 * @return      значение найденного атрибута либо пустая последовательность
 *
 */
string_view awh::codec::xml::Node::attribute(const string_view local, const string_view uri) const noexcept {
	/**
	 * Если узел непригоден
	 */
	if(!this->valid())
		// Выводим пустую последовательность знаков
		return string_view();
	// Получаем запись текущего узла
	const Document::record_t & record = this->_document->_nodes[this->_id];
	/**
	 * Выполняем перебор всех атрибутов узла
	 */
	for(uint32_t i = 0; i < record.attributes; i++){
		// Получаем запись очередного атрибута
		const Document::property_t & property = this->_document->_attributes[record.attribute + i];
		/**
		 * Если имя атрибута совпадает с искомым
		 */
		if(this->_document->get(property.name).is(uri, local))
			// Выводим значение найденного атрибута
			return this->_document->get(property.value);
	}
	// Выводим пустую последовательность знаков
	return string_view();
}
/**
 * @brief Метод проверки наличия атрибута у узла
 *
 * @param local местное имя искомого атрибута
 * @param uri   обозначение пространства имён искомого атрибута
 * @return      результат проверки
 *
 */
bool awh::codec::xml::Node::has(const string_view local, const string_view uri) const noexcept {
	/**
	 * Если узел непригоден
	 */
	if(!this->valid())
		// Выводим отрицательный результат проверки
		return false;
	// Получаем запись текущего узла
	const Document::record_t & record = this->_document->_nodes[this->_id];
	/**
	 * Выполняем перебор всех атрибутов узла
	 */
	for(uint32_t i = 0; i < record.attributes; i++){
		/**
		 * Если имя атрибута совпадает с искомым
		 */
		if(this->_document->get(this->_document->_attributes[record.attribute + i].name).is(uri, local))
			// Выводим положительный результат проверки
			return true;
	}
	// Выводим отрицательный результат проверки
	return false;
}
