/**
 * @file: device.cpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация кодека описания устройства UPnP — разбор описания, обход вложенных
 *        устройств, сбор перечня служб и сборка полных адресов управления
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <proto/portmap/device.hpp>

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
	 * Пространство имён библиотеки
	 */
	using namespace awh;

	/**
	 * @brief Наибольшая глубина обхода вложенных устройств
	 *
	 * @details Устройства вкладываются друг в друга, и договор глубину не ограничивает.
	 * Предел обязателен: описание берётся у устройства, о котором заранее ничего не
	 * известно, и глубоко вложенным деревом оно способно занять всю память
	 *
	 */
	constexpr uint32_t MAX_DEPTH = 0x10;

	/**
	 * @brief Метод поиска вложенного узла по местному имени
	 *
	 * @details Сличение ведётся лишь по местному имени, без учёта пространства имён.
	 * Так и задумано: описание получено у одного определённого устройства по адресу
	 * из обнаружения, чужой разметке взяться в нём неоткуда, а устройства пространство
	 * имён описания то опускают, то пишут с опечаткой
	 *
	 * @param node  узел, среди вложенных которого ведётся поиск
	 * @param local местное имя искомого узла
	 * @return      найденный узел дерева разметки
	 *
	 */
	codec::xml::node_t child(const codec::xml::node_t & node, const string_view local) noexcept {
		/**
		 * Выполняем перебор всех непосредственно вложенных узлов
		 */
		for(codec::xml::node_t item = node.first(); item.valid(); item = item.next()){
			/**
			 * Если вложенный узел узлом разметки не является
			 */
			if(item.kind() != codec::xml::kind_t::ELEMENT)
				// Выполняем переход к следующему вложенному узлу
				continue;
			/**
			 * Если местное имя вложенного узла совпадает с искомым
			 */
			if(item.name().local.compare(local) == 0)
				// Выводим обнаруженный узел дерева разметки
				return item;
		}
		// Выводим непригодный узел дерева разметки
		return codec::xml::node_t();
	}
	/**
	 * @brief Метод получения содержимого вложенного узла по местному имени
	 *
	 * @param node  узел, среди вложенных которого ведётся поиск
	 * @param local местное имя искомого узла
	 * @return      содержимое найденного узла либо пустая строка
	 *
	 */
	string text(const codec::xml::node_t & node, const string_view local) noexcept {
		// Выводим содержимое обнаруженного узла дерева разметки
		return ::child(node, local).text();
	}
};

/**
 * @brief Метод разбора описания устройства
 *
 * @param text        разбираемое описание устройства
 * @param description ссылка на разобранное описание устройства
 * @param error       ссылка на код причины отказа
 * @return            признак успешного разбора
 *
 */
bool awh::proto::portmap::Device::parse(const string_view text, description_t & description, error_t & error) const noexcept {
	// Выполняем сброс кода причины отказа
	error = error_t::NONE;
	// Выполняем сброс разобранного описания устройства
	description = description_t();
	/**
	 * Если разбираемое описание пусто
	 */
	if(text.empty()){
		// Запоминаем код причины отказа
		error = error_t::EMPTY;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text.length()), log_t::flag_t::WARNING, message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	/**
	 * Если разбираемое описание длиннее допустимого
	 */
	if(text.length() > MAX_DESCRIPTION_SIZE){
		// Запоминаем код причины отказа
		error = error_t::TOO_LARGE;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(text.length()), log_t::flag_t::WARNING, message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	// Объект дерева разметки описания устройства
	codec::xml::document_t document;
	/**
	 * Если разбор описания устройства выполнить не удалось
	 */
	if(!document.parse(text)){
		// Запоминаем код причины отказа
		error = error_t::MALFORMED;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug(
				"%s", __PRETTY_FUNCTION__,
				make_tuple(document.errorLocation().line, document.errorLocation().column),
				log_t::flag_t::WARNING, codec::xml::message(document.error())
			);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, codec::xml::message(document.error()));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	// Получаем корневой узел описания устройства
	const codec::xml::node_t root = document.element();
	/**
	 * Если корневым узлом описание устройства не является
	 */
	if(!root.valid() || (root.name().local.compare("root") != 0)){
		// Запоминаем код причины отказа
		error = error_t::MISSING_ROOT;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(root.name().local), log_t::flag_t::WARNING, message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	// Запоминаем объявленное основание для сборки относительных адресов
	description.base = ::text(root, "URLBase");
	// Получаем узел самого устройства
	const codec::xml::node_t device = ::child(root, "device");
	/**
	 * Если узел самого устройства в описании отсутствует
	 */
	if(!device.valid()){
		// Запоминаем код причины отказа
		error = error_t::MISSING_SPEC;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(description.base), log_t::flag_t::WARNING, message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	// Запоминаем обозначение вида устройства
	description.type = ::text(device, "deviceType");
	// Запоминаем понятное человеку название устройства
	description.name = ::text(device, "friendlyName");
	// Запоминаем изготовителя устройства
	description.manufacturer = ::text(device, "manufacturer");
	// Запоминаем обозначение изделия
	description.model = ::text(device, "modelName");
	// Запоминаем обозначение самого устройства
	description.udn = ::text(device, "UDN");
	/**
	 * Если обозначения у устройства нет
	 *
	 * @note Обозначение у устройства единственное и неизменное: им устройство
	 *       и опознаётся между обнаружениями. Без него отличить устройство от
	 *       другого такого же в сети нечем
	 */
	if(description.udn.empty()){
		// Запоминаем код причины отказа
		error = error_t::MISSING_UDN;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(description.name, description.type), log_t::flag_t::WARNING, message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	/**
	 * @brief Метод обхода устройства и всех вложенных в него устройств
	 *
	 * @param node  узел обходимого устройства
	 * @param depth текущая глубина вложенности устройства
	 * @param self  ссылка на саму себя для рекурсивного обхода
	 *
	 */
	const auto collect = [&description](const codec::xml::node_t & node, const uint32_t depth, const auto & self) noexcept -> void {
		/**
		 * Если глубина вложенности превысила допустимую
		 *
		 * @note Предел обязателен: описание берётся у устройства, о котором заранее
		 *       ничего не известно, и глубоко вложенным деревом оно способно занять
		 *       всю память
		 */
		if(depth > MAX_DEPTH)
			// Выходим из обхода устройства
			return;
		// Получаем перечень служб обходимого устройства
		const codec::xml::node_t services = ::child(node, "serviceList");
		/**
		 * Если перечень служб у обходимого устройства заведён
		 */
		if(services.valid()){
			/**
			 * Выполняем перебор всех служб обходимого устройства
			 */
			for(codec::xml::node_t item = services.first(); item.valid(); item = item.next()){
				/**
				 * Если вложенный узел службой не является
				 */
				if((item.kind() != codec::xml::kind_t::ELEMENT) || (item.name().local.compare("service") != 0))
					// Выполняем переход к следующему вложенному узлу
					continue;
				// Собираемая служба устройства
				service_t service;
				// Запоминаем обозначение вида службы
				service.type = ::text(item, "serviceType");
				// Запоминаем обозначение самой службы
				service.id = ::text(item, "serviceId");
				// Запоминаем адрес управления службой
				service.control = ::text(item, "controlURL");
				// Запоминаем адрес подписки на события службы
				service.event = ::text(item, "eventSubURL");
				// Запоминаем адрес описания действий службы
				service.spec = ::text(item, "SCPDURL");
				/**
				 * Если у службы нет обозначения вида либо адреса управления
				 *
				 * @note Служба без адреса управления бесполезна: обратиться к ней
				 *       попросту некуда
				 */
				if(service.type.empty() || service.control.empty())
					// Выполняем переход к следующей службе устройства
					continue;
				// Выполняем добавление собранной службы к описанию устройства
				description.services.push_back(::std::move(service));
			}
		}
		// Получаем перечень вложенных устройств
		const codec::xml::node_t devices = ::child(node, "deviceList");
		/**
		 * Если перечня вложенных устройств нет
		 */
		if(!devices.valid())
			// Выходим из обхода устройства
			return;
		/**
		 * Выполняем перебор всех вложенных устройств
		 *
		 * @note Служба перенаправления лежит не в корне, а двумя уровнями ниже:
		 *       внутри устройства доступа в сеть, а в нём - внутри устройства
		 *       соединения. Обходить дерево целиком поэтому обязательно
		 */
		for(codec::xml::node_t item = devices.first(); item.valid(); item = item.next()){
			/**
			 * Если вложенный узел устройством не является
			 */
			if((item.kind() != codec::xml::kind_t::ELEMENT) || (item.name().local.compare("device") != 0))
				// Выполняем переход к следующему вложенному узлу
				continue;
			// Выполняем обход вложенного устройства
			self(item, (depth + 1), self);
		}
	};
	// Выполняем обход устройства и всех вложенных в него устройств
	collect(device, 0, collect);
	/**
	 * Если ни одной пригодной службы устройство не имеет
	 */
	if(description.services.empty()){
		// Запоминаем код причины отказа
		error = error_t::EMPTY_SERVICE;
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(description.name, description.udn), log_t::flag_t::WARNING, message(error));
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, message(error));
		#endif
		// Выводим признак неудачного разбора
		return false;
	}
	// Выводим признак успешного разбора
	return true;
}
/**
 * @brief Метод поиска службы устройства по обозначению вида
 *
 * @param description разобранное описание устройства
 * @param type        обозначение искомого вида службы
 * @return            найденная служба устройства либо пустой указатель
 *
 */
const awh::proto::portmap::Device::service_t * awh::proto::portmap::Device::service(const description_t & description, const string_view type) const noexcept {
	/**
	 * Выполняем перебор всех служб устройства
	 */
	for(const service_t & service : description.services){
		/**
		 * Если обозначение вида службы совпадает с искомым
		 *
		 * @note Сличение ведётся без учёта регистра: обозначения служб устройства
		 *       записывают вольно, а отвергать пригодную службу из-за разницы в
		 *       написании незачем
		 */
		if(this->_fmk->compare(service.type, type))
			// Выводим обнаруженную службу устройства
			return &service;
	}
	// Выводим пустой указатель на службу устройства
	return nullptr;
}
/**
 * @brief Метод сборки полного адреса управления службой
 *
 * @param description разобранное описание устройства
 * @param location    адрес, по которому получено описание устройства
 * @param address     собираемый адрес управления службой
 * @return            собранный полный адрес управления службой
 *
 */
string awh::proto::portmap::Device::address(const description_t & description, const string_view location, const string_view address) const noexcept {
	/**
	 * Если собираемый адрес управления не передан
	 */
	if(address.empty())
		// Выводим пустой адрес управления службой
		return string();
	/**
	 * Выполняем очистку объекта работы с адресами ресурсов
	 *
	 * @note Очистка обязательна: разбор ведётся относительно уже разобранного, и
	 *       без неё очередной адрес разрешился бы относительно предыдущего
	 */
	this->_uri.clear();
	/**
	 * Получаем основание для сборки относительного адреса
	 *
	 * @note Основание устройства объявляют не всегда, и полагаться на него нельзя:
	 *       при отсутствии основанием служит сам адрес, по которому получено описание
	 */
	const string_view base = (description.base.empty() ? location : string_view(description.base));
	/**
	 * Если разобрать основание для сборки адреса не удалось
	 */
	if(this->_uri.parse(base) == uri_t::type_t::NONE){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(string(base), string(address)), log_t::flag_t::WARNING, "unable to parse the device description address");
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, "unable to parse the device description address");
		#endif
		// Выводим пустой адрес управления службой
		return string();
	}
	/**
	 * Выполняем разрешение адреса управления относительно основания
	 *
	 * @note Устройства записывают адрес управления и путём, и полным адресом:
	 *       разрешение относительно основания годится для обоих видов
	 */
	if(!this->_uri.resolve(address)){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(string(base), string(address)), log_t::flag_t::WARNING, "unable to resolve the service control address");
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::WARNING, "unable to resolve the service control address");
		#endif
		// Выводим пустой адрес управления службой
		return string();
	}
	// Выводим собранный полный адрес управления службой
	return this->_uri.print();
}
/**
 * @brief Метод получения описания кода причины отказа кодека описания устройства
 *
 * @param error код причины отказа кодека
 * @return      описание кода причины отказа на английском языке
 *
 */
const char * awh::proto::portmap::message(const device_t::error_t error) noexcept {
	/**
	 * Определяем код причины отказа кодека
	 */
	switch(static_cast <uint8_t> (error)){
		// Если ошибок не обнаружено
		case static_cast <uint8_t> (device_t::error_t::NONE):
			// Выводим описание кода причины отказа
			return "no error";
		// Если разбираемое описание пусто
		case static_cast <uint8_t> (device_t::error_t::EMPTY):
			// Выводим описание кода причины отказа
			return "device description is empty";
		// Если описание длиннее допустимого
		case static_cast <uint8_t> (device_t::error_t::TOO_LARGE):
			// Выводим описание кода причины отказа
			return "device description is too large";
		// Если описание построено ошибочно
		case static_cast <uint8_t> (device_t::error_t::MALFORMED):
			// Выводим описание кода причины отказа
			return "malformed device description";
		// Если в описании нет корневого узла устройства
		case static_cast <uint8_t> (device_t::error_t::MISSING_ROOT):
			// Выводим описание кода причины отказа
			return "device description has no root element";
		// Если в описании нет самого устройства
		case static_cast <uint8_t> (device_t::error_t::MISSING_SPEC):
			// Выводим описание кода причины отказа
			return "device description has no device element";
		// Если у устройства нет обозначения
		case static_cast <uint8_t> (device_t::error_t::MISSING_UDN):
			// Выводим описание кода причины отказа
			return "device has no unique device name";
		// Если ни одной пригодной службы устройство не имеет
		case static_cast <uint8_t> (device_t::error_t::EMPTY_SERVICE):
			// Выводим описание кода причины отказа
			return "device provides no usable services";
	}
	// Выводим описание неизвестного кода причины отказа
	return "unknown error";
}
