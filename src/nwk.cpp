/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <nwk.hpp>

/**
 * str Метод вывода результата в виде строки
 * @return строка ip адреса
 */
const string awh::Network::IPdata::str() const noexcept {
	// Строка результата
	string result = "";
	// Переходим по всему массиву
	for(u_int i = 0; i < this->size(); i++){
		// Если строка уже не пустая, добавляем разделитель
		if(!result.empty()) result.append(".");
		// Добавляем в строку значение ip адреса
		result.append(to_string(this->ptr[i]));
	}
	// Выводим результат
	return result;
}
/**
 * length Метод определения размера
 * @return размер массива
 */
const u_int awh::Network::IPdata::size() const noexcept {
	// Выводим размер массива
	return (sizeof(this->ptr) / sizeof(u_int));
}
/**
 * get Метод получения данных масива
 * @return указатель на масив ip адреса
 */
const u_int * awh::Network::IPdata::get() const noexcept {
	// Выводим результат
	return this->ptr;
}
/**
 * set Метод установки данных ip адреса
 * @param ptr массив значений ip адреса
 */
void awh::Network::IPdata::set(const u_int ptr1, const u_int ptr2, const u_int ptr3, const u_int ptr4) noexcept {
	// Если проверки пройдены тогда добавляем данные массива
	if(ptr1 <= 255) this->ptr[0] = ptr1;
	if(ptr2 <= 255) this->ptr[1] = ptr2;
	if(ptr3 <= 255) this->ptr[2] = ptr3;
	if(ptr4 <= 255) this->ptr[3] = ptr4;
}
/**
 * setZerroToStrIp Метод дописывает указанное количестов нулей к строке
 * @param str строка к которой нужно дописать нули
 */
void awh::Network::setZerroToStrIp(string & str) const noexcept {
	// Корректируем количество цифр
	if(str.length() < 3){
		// Дописываем количество нулей
		for(u_int i = 0; i < (4 - str.length()); i++)
			// Прописываем нули в начало строки
			str = this->fmk->format("0%s", str.c_str());
	}
}
/**
 * rmZerroToStrIp Метод удаляем указанное количестов нулей из строки
 * @param str строка из которой нужно удалить нули
 */
void awh::Network::rmZerroToStrIp(string & str) const noexcept {
	// Корректируем количество цифр
	if(!str.empty()){
		// Формируем регулярное выражение
		regex e("^0{1,2}([0-9]{1,2})$");
		// Выполняем удаление лишних нулей
		str = regex_replace(str, e, "$1");
	}
}
/**
 * checkMask Метод проверки на соответствии маски
 * @param ip   блок с данными ip адреса
 * @param mask блок с данными маски сети
 * @return     результат проверки
 */
const bool awh::Network::checkMask(const ipdata_t & ip, const ipdata_t & mask) const noexcept {
	// Переходим по всему блоку данных ip
	for(u_int i = 0; i < ip.size(); i++){
		// Определяем значение маски сети
		u_int msk = mask.get()[i];
		// Проверяем соответствует ли ip адрес маске
		if(msk && (ip.get()[i] > msk)) return false;
	}
	// Сообщаем что все удачно
	return true;
}
/**
 * getLow1Ip6 Метод упрощения IPv6 адреса первого порядка
 * @param ip адрес интернет протокола версии 6
 * @return   упрощенный вид ip адреса первого порядка
 */
const string awh::Network::getLow1Ip6(const string & ip) const noexcept {
	// Результат работы функции
	string result = "";
	// Если IP адрес передан
	if(!ip.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e(
			"^\\[?([a-f\\d]{4})\\:([a-f\\d]{4})"
			"\\:([a-f\\d]{4})\\:([a-f\\d]{4})"
			"\\:([a-f\\d]{4})\\:([a-f\\d]{4})"
			"\\:([a-f\\d]{4})\\:([a-f\\d]{4})\\]?$",
			regex::ECMAScript | regex::icase
		);
		// Выполняем поиск ip адреса
		regex_search(ip, match, e);
		// Если данные найдены
		if(!match.empty()){
			// Регулярное выражение для поиска старших нулей
			regex e("^(0+)([A-Fa-f\\d]+)$");
			// Переходим по всему массиву и заменяем 0000 на 0 и убираем старшие нули
			for(size_t i = 1; i < match.size(); i++){
				// Если ipv6 уже не пустой, добавляем разделитель
				if(!result.empty()) result.append(":");
				// Получаем группу байт адреса для обработки
				const string & str = match[i].str();
				// Если - это вся группа нулей
				if(str.compare("0000") == 0) result.append("0");
				// Заменяем старшие нули
				else result.append(regex_replace(str, e, "$2"));
			}
		// Иначе устанавливаем значение по умолчанию
		} else result = ip;
	}
	// Выводим результат
	return result;
}
/**
 * getLow2Ip6 Метод упрощения IPv6 адреса второго порядка
 * @param ip адрес интернет протокола версии 6
 * @return   упрощенный вид ip адреса второго порядка
 */
const string awh::Network::getLow2Ip6(const string & ip) const noexcept {
	// Результат работы функции
	string result = "";
	// Если IP адрес передан
	if(!ip.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e(
			"^\\[?([a-f\\d]{1,4})\\:([a-f\\d]{1,4})"
			"\\:([a-f\\d]{1,4})\\:([a-f\\d]{1,4})"
			"\\:([a-f\\d]{1,4})\\:([a-f\\d]{1,4})"
			"\\:([a-f\\d]{1,4})\\:([a-f\\d]{1,4})\\]?$",
			regex::ECMAScript | regex::icase
		);
		// Выполняем поиск ip адреса
		regex_search(ip, match, e);
		// Если данные найдены
		if(!match.empty()){
			// Копируем ip адрес
			string str = ip;
			// Массив найденных элементов
			vector <string> arr;
			// Регулярное выражение для поиска старших нулей
			regex e("(?:^|\\:)([0\\:]+)(?:\\:|$)");
			// Выполняем удаление из ip адреса нулей до тех пор пока все не удалим
			while(true){
				// Выполняем поиск ip адреса
				regex_search(str, match, e);
				// Если данные найдены
				if(!match.empty()){
					// Копируем найденные данные
					const string & delim = match[0].str();
					// Ищем строку еще раз
					size_t pos = str.find(delim);
					// Если позиция найдена
					if(pos != string::npos)
						// Удаляем из строки найденный символ
						str.replace(pos, delim.length(), "");
					// Добавляем в массив найденные данные
					arr.push_back(delim);
				// Иначе выходим
				} else break;
			}
			// Если массив существует
			if(!arr.empty()){
				// Индекс вектора с максимальным значением
				size_t max = 0, index = 0, len = 0;
				// Ищем максимальное значение в массиве
				for(size_t i = 0; i < arr.size(); i++){
					// Получаем длину строки
					len = arr[i].length();
					// Если размер строки еще больше то запоминаем его
					if(len > max){
						// Запоминаем текущий размер строки
						max = len;
						// Запоминаем индекс в массиве
						index = i;
					}
				}
				// Создаем регулярное выражение для поиска
				regex e(arr[index]);
				// Заменяем найденный элемент на ::
				result = regex_replace(ip, e, "::");
			// Запоминаем адрес так как он есть
			} else result = move(str);
		// Иначе устанавливаем значение по умолчанию
		} else result = ip;
	}
	// Выводим результат
	return result;
}
/**
 * setLow1Ip6 Метод восстановления IPv6 адреса первого порядка
 * @param ip адрес интернет протокола версии 6
 * @return   восстановленный вид ip адреса первого порядка
 */
const string awh::Network::setLow1Ip6(const string & ip) const noexcept {
	// Результат работы функции
	string result = "";
	// Если IP адрес передан
	if(!ip.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e(
			"^\\[?([a-f\\d]{1,4})\\:([a-f\\d]{1,4})"
			"\\:([a-f\\d]{1,4})\\:([a-f\\d]{1,4})"
			"\\:([a-f\\d]{1,4})\\:([a-f\\d]{1,4})"
			"\\:([a-f\\d]{1,4})\\:([a-f\\d]{1,4})\\]?$",
			regex::ECMAScript | regex::icase
		);
		// Выполняем поиск ip адреса
		regex_search(ip, match, e);
		// Если данные найдены
		if(!match.empty()){
			// Результат работы регулярного выражения
			smatch match;
			// Копируем ip адрес
			string str = ip;
			// Массив найденных элементов
			vector <string> arr;
			// Устанавливаем правило регулярного выражения
			regex e("([a-f\\d]{1,4})", regex::ECMAScript | regex::icase);
			// Выполняем удаление из ip адреса хексетов
			while(true){
				// Выполняем поиск ip адреса
				regex_search(str, match, e);
				// Если данные найдены
				if(!match.empty()){
					// Копируем полученные данные
					const string & delim = match[1].str();
					// Ищем строку еще раз
					size_t pos = str.find(delim);
					// Если позиция найдена
					if(pos != string::npos)
						// Удаляем из строки найденный символ
						str.replace(pos, delim.length(), "");
					// Добавляем в массив найденные данные
					arr.push_back(delim);
				// Если хексет не найден то выходим
				} else break;
			}
			// Переходим по всему массиву
			for(auto & value : arr){
				// Если ipv6 уже не пустой, добавляем разделитель
				if(!result.empty()) result.append(":");
				// Определяем длину хексета
				const size_t size = value.length();
				// Если размер хексета меньше 4 то дописываем нули
				if(size < 4){
					// Дописываем столько нулей, сколько необходимо
					for(size_t j = 0; j < (4 - size); j++)
						// Добавляем недостающие нули
						value = this->fmk->format("0%s", value.c_str());
				}
				// Формируем результат
				result.append(value);
			}
		// Иначе устанавливаем значение по умолчанию
		} else result = ip;
	}
	// Выводим результат
	return result;
}
/**
 * setLow2Ip6 Метод восстановления IPv6 адреса второго порядка
 * @param ip адрес интернет протокола версии 6
 * @return   восстановленный вид ip адреса второго порядка
 */
const string awh::Network::setLow2Ip6(const string & ip) const noexcept {
	// Результат работы функции
	string result = "";
	// Если IP адрес передан
	if(!ip.empty()){
		// Копируем ip адрес
		string str = ip;
		// Ищем строку еще раз
		const size_t pos = str.find("::");
		// Если позиция найдена
		if(pos != string::npos){
			// Результат работы регулярного выражения
			smatch match;
			// Массив найденных элементов
			vector <string> arr;
			// Устанавливаем правило регулярного выражения
			regex e("([a-f\\d]{1,4})", regex::ECMAScript | regex::icase);
			// Выполняем удаление из ip адреса хексетов
			while(true){
				// Выполняем поиск ip адреса
				regex_search(str, match, e);
				// Если данные найдены
				if(!match.empty()){
					// Копируем полученные данные
					const string & delim = match[1].str();
					// Ищем строку еще раз
					const size_t pos = str.find(delim);
					// Если позиция найдена
					if(pos != string::npos)
						// Удаляем из строки найденный символ
						str.replace(pos, delim.length(), "");
					// Добавляем в массив найденные данные
					arr.push_back(delim);
				// Если хексет не найден то выходим
				} else break;
			}
			// Определяем количество хексетов
			const u_int size = arr.size();
			// Если количество хексетов меньше 8 то определяем сколько не хватает
			if(size < 8){
				// Копируем полученные данные
				result = ip;
				// Маска IP адреса
				string mask = ":";
				// Составляем маску
				for(u_int i = 0; i < (8 - size); i++) mask.append("0:");
				// Удаляем из строки найденный символ
				result.replace(pos, 2, mask);
				// Устанавливаем правило регулярного выражения
				regex e("(?:^\\[?\\:|\\:\\]?$)");
				// Заменяем найденные не верные элементы
				result = regex_replace(result, e, "");
			}
		// Иначе устанавливаем значение по умолчанию
		} else result = ip;
	}
	// Выводим результат
	return result;
}
/**
 * getNetwork Метод извлечения данных сети
 * @param str адрес подключения (127.0.0.1/255.000.000.000 или 127.0.0.1/8 или 127.0.0.1)
 * @return    параметры подключения
 */
const awh::Network::nkdata_t awh::Network::getNetwork(const string & str) const noexcept {
	// Результат получения данных сети
	nkdata_t result;
	// Если строка передана
	if(!str.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e("^(\\d{1,3}(?:\\.\\d{1,3}){3})(?:\\/(\\d+|\\d{1,3}(?:\\.\\d{1,3}){3}))?$", regex::ECMAScript);
		// Выполняем поиск ip адреса и маски сети
		regex_search(str, match, e);
		// Если данные найдены
		if(!match.empty()){
			// Если маска найдена
			string mask = match[2].str();
			// Если ip адрес найден
			const string & ip = match[1].str();
			// Устанавливаем маску по умолчанию
			if(mask.empty()) mask = "0";
			// Ищем формат маски
			if(mask.find(".") == string::npos)
				// Запоминаем данные маски
				mask = this->getMaskByNumber(stoi(mask)).mask;
			// Оцифровываем данные ip
			const ipdata_t & ipData = this->getDataIp(ip);
			// Оцифровываем данные маски
			const ipdata_t & maskData = this->getDataIp(mask);
			// Оцифровываем данные сети
			const ipdata_t & nwkData = this->imposeMask(ipData, maskData);
			// Формируем результат
			result = {ipData, maskData, nwkData};
		}
	}
	// Выводим результат
	return result;
}
/**
 * getMaskByNumber Метод получения маски из цифровых обозначений
 * @param value цифровое обозначение маски
 * @return      объект с данными маски
 */
const awh::Network::ntdata_t awh::Network::getMaskByNumber(const u_int value) const noexcept {
	// Результат работы функции
	ntdata_t result;
	// Если маска найдена то запоминаем ее
	if(value < this->masks.size()){
		// Запоминаем данные маски
		result = this->masks.at(value);
		// Запоминаем что структура заполнена
		result.notEmpty = true;
	}
	// Выводим результат
	return result;
}
/**
 * getMaskByString Метод получения маски из строки обозначения маски
 * @param value строка с обозначением маски
 * @return      объект с данными маски
 */
const awh::Network::ntdata_t awh::Network::getMaskByString(const string & value) const noexcept {
	// Результат работы функции
	ntdata_t result;
	// Если строка с маской передана
	if(!value.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e("^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})$", regex::ECMAScript);
		// Выполняем поиск ip адреса
		regex_search(value, match, e);
		// Если данные найдены
		if(!match.empty()){
			// Получаем символы
			string oct1 = match[1].str();
			string oct2 = match[2].str();
			string oct3 = match[3].str();
			string oct4 = match[4].str();
			// Корректируем количество цифр
			this->setZerroToStrIp(oct1);
			this->setZerroToStrIp(oct2);
			this->setZerroToStrIp(oct3);
			this->setZerroToStrIp(oct4);
			// Формируем ip адрес
			const string & ip = this->fmk->format("%s.%s.%s.%s", oct1.c_str(), oct2.c_str(), oct3.c_str(), oct4.c_str());
			// Переходим по всему массиву масок и ищем нашу
			for(auto & value : this->masks){
				// Если нашли нашу маску то выводим результат
				if(ip.compare(value.mask) == 0){
					// Запоминаем данные маски
					result = move(value);
					// Запоминаем что структура заполнена
					result.notEmpty = true;
					// Выходим из цикла
					break;
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * getDataIp Метод получает цифровые данные ip адреса
 * @param ip данные ip адреса в виде строки
 * @return   оцифрованные данные ip адреса
 */
const awh::Network::ipdata_t awh::Network::getDataIp(const string & ip) const noexcept {
	// Результат работы функции
	ipdata_t result;
	// Если IP адрес передан
	if(!ip.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e("^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})$", regex::ECMAScript);
		// Выполняем поиск ip адреса
		regex_search(ip, match, e);
		// Если данные найдены
		if(!match.empty()){
			// Получаем данные ip адреса
			result.set(
				(u_int) stoi(match[1].str()),
				(u_int) stoi(match[2].str()),
				(u_int) stoi(match[3].str()),
				(u_int) stoi(match[4].str())
			);
		}
	}
	// Выводим данные ip адреса
	return result;
}
/**
 * imposeMask Метод наложения маски
 * @param ip   блок с данными ip адреса
 * @param mask блок с данными маски сети
 * @return     блок с данными сети
 */
const awh::Network::ipdata_t awh::Network::imposeMask(const awh::Network::ipdata_t & ip, const awh::Network::ipdata_t & mask) const noexcept {
	// Параметры ip адреса
	u_int ptr[4];
	// Объект сети
	ipdata_t network;
	// Переходим по всему блоку данных ip
	for(u_int i = 0; i < ip.size(); i++){
		// Получаемд анные ip адреса
		ptr[i] = ip.get()[i];
		// Накладываем маску
		if(ptr[i] > mask.get()[i]) ptr[i] = 0;
	}
	// Добавляем данные в объект
	network.set(ptr[0], ptr[1], ptr[2], ptr[3]);
	// Выводим результат
	return network;
}
/**
 * isPort Метод проверки на качество порта
 * @param port входная строка якобы содержащая порт
 * @return     результат проверки
 */
const bool awh::Network::isPort(const string & str) const noexcept {
	// Если строка существует
	if(!str.empty()){
		// Преобразуем строку в цифры
		if(this->fmk->isNumber(str)){
			// Получаем порт
			u_int port = stoi(str);
			// Проверяем диапазон портов
			if((port > 0) && (port < 65536)) return true;
		}
	}
	// Сообщаем что ничего не нашли
	return false;
}
/**
 * isV4ToV6 Метод проверки на отображение адреса IPv4 в IPv6
 * @param ip адрес для проверки
 * @return   результат проверки
 */
const bool awh::Network::isV4ToV6(const string & ip) const noexcept {
	// Результат проверки
	bool result = false;
	// Если ip адрес передан
	if(!ip.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e("^\\:\\:ffff\\:\\d{1,3}(?:\\.\\d{1,3}){3}$", regex::ECMAScript | regex::icase);
		// Выполняем проверку является ли адрес отображением сети ipv4 в ipv6
		regex_search(ip, match, e);
		// Запоминаем результат
		result = !match.empty();
	}
	// Выводим результат
	return result;
}
/**
 * isAddr Метод проверки на то является ли строка адресом
 * @param address строка адреса для проверки
 * @return        результат проверки
 */
const bool awh::Network::isAddr(const string & address) const noexcept {
	// Результат работы регулярного выражения
	smatch match;
	// Устанавливаем правило регулярного выражения
	regex e(
		// Определение домена
		"(?:[\\w\\-\\.]+\\.[a-z]+|"
		// Определение мак адреса
		"[a-f\\d]{2}(?:\\:[a-f\\d]{2}){5}|"
		// Определение ip4 адреса
		"\\d{1,3}(?:\\.\\d{1,3}){3}|"
		// Определение ip6 адреса
		"(?:\\[?(?:\\:\\:ffff\\:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))\\]?))",
		regex::ECMAScript | regex::icase
	);
	// Выполняем проверку
	regex_search(address, match, e);
	// Выводим результат
	return !match.empty();
}
/**
 * compareIP6 Метод проверки на совпадение ip адресов
 * @param ip1 адрес подключения IPv6
 * @param ip2 адрес подключения IPv6
 * @return    результат проверки
 */
const bool awh::Network::compareIP6(const string & ip1, const string & ip2) const noexcept {
	// Результат сравнения
	bool result = false;
	// Если ip адреса переданы
	if(!ip1.empty() && !ip2.empty())
		// Если ip адреса совпадают то запоминаем это
		result = (this->fmk->toLower(this->setLowIp6(ip1)).compare(this->fmk->toLower(this->setLowIp6(ip2))) == 0);
	// Выводим результат сравнения
	return result;
}
/**
 * checkMaskByNumber Метод проверки на соответствие маски по номеру маски
 * @param ip   данные ip адреса
 * @param mask номер маски
 * @return      результат проверки
 */
const bool awh::Network::checkMaskByNumber(const string & ip, const u_int mask) const noexcept {
	// Оцифровываем данные ip
	const ipdata_t & ipData = this->getDataIp(ip);
	// Оцифровываем данные маски
	const ipdata_t & maskData = this->getDataIp(this->getMaskByNumber(mask).mask);
	// Выводим результат проверки
	return this->checkMask(ipData, maskData);
}
/**
 * checkIPByNetwork Метод проверки, принадлежит ли ip адрес указанной сети
 * @param ip  данные ip адреса интернет протокола версии 4
 * @param nwk адрес сети (192.168.0.0/16)
 * @return    результат проверки
 */
const bool awh::Network::checkIPByNetwork(const string & ip, const string & nwk) const noexcept {
	// Получаем данные ip адреса
	const ipdata_t & ipData = this->getDataIp(ip);
	// Получаем данные сети
	const nkdata_t & nwkData1 = this->getNetwork(nwk);
	// Накладываем маску на ip адрес
	const ipdata_t & nwkData2 = this->imposeMask(ipData, nwkData1.mask);
	// Накладываем маску на ip адрес из списка
	if(nwkData2.str().compare(nwkData1.network.str()) == 0) return true;
	// Сообщачем что ничего не найдено
	return false;
}
/**
 * checkIPByNetwork6 Метод проверки, принадлежит ли ip адрес указанной сети
 * @param ip  данные ip адреса интернет протокола версии 6
 * @param nwk адрес сети (2001:db8::/32)
 * @return    результат проверки
 */
const bool awh::Network::checkIPByNetwork6(const string & ip, const string & nwk) const noexcept {
	// Результат сравнения
	bool result = false;
	// Если данные переданы
	if(!ip.empty() && !nwk.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e("^([\\s\\S]+)\\/(\\d+)$", regex::ECMAScript | regex::icase);
		// Выполняем поиск ip адреса и префикса сети
		regex_search(nwk, match, e);
		// Если данные найдены
		if(!match.empty()){
			// Хексеты IP адреса
			vector <wstring> mip;
			// Формируем вектор данных ip адреса
			this->fmk->split(this->fmk->toLower(this->setLowIp6(this->imposePrefix6(ip, stoi(match[2].str())))), ":", mip);
			// Если данные получены
			if(!mip.empty()){
				// Если первый хекстет нулевой значит это локальный адрес
				if(mip.front().compare(L"0000") == 0) result = true;
				// Выполняем сравнение по хекстетам
				else {
					// Хексеты сети IPv6
					vector <wstring> nwk;
					// Формируем вектор данных сети
					this->fmk->split(this->fmk->toLower(this->setLowIp6(match[1].str())), ":", nwk);
					// Если хексеты сети получены
					if(!nwk.empty() && (mip.size() == nwk.size())){
						// Начинаем проверять совпадения
						for(u_int j = 0; j < mip.size(); j++){
							// Если значение в маске совпадает тогда продолжаем проверку
							result = ((mip.at(j).compare(nwk.at(j)) == 0) || (nwk.at(j).compare(L"0000") == 0));
							// Выходим, если хексеты не совпали
							if(!result) break;
						}
					}
				}
			}
		}
	}
	// Выводим результат сравнения
	return result;
}
/**
 * checkMaskByString Метод проверки на соответствие маски по строке маски
 * @param ip   данные ip адреса
 * @param mask номер маски
 * @return     результат проверки
 */
const bool awh::Network::checkMaskByString(const string & ip, const string & mask) const noexcept {
	// Результат сравнения
	bool result = false;
	// Если данные переданы
	if(!ip.empty() && !mask.empty()){
		// Оцифровываем данные ip
		const ipdata_t & ipData = this->getDataIp(ip);
		// Оцифровываем данные маски
		const ipdata_t & maskData = this->getDataIp(mask);
		// Выводим результат проверки
		result = this->checkMask(ipData, maskData);
	}
	// Выводим результат сравнения
	return result;
}
/**
 * checkDomain Метод определения определения соответствия домена маски
 * @param domain название домена
 * @param mask   маска домена для проверки
 * @return       результат проверки
 */
const bool awh::Network::checkDomainByMask(const string & domain, const string & mask) const noexcept {
	// Результат проверки домена
	bool result = false;
	// Если и домен и маска переданы
	if(!domain.empty() && !mask.empty()){
		// Итератор
		wstring itm = L"";
		// Составляющие адреса
		vector <wstring> msk, dom;
		// Выполняем разбиение маски
		this->fmk->split(this->fmk->toLower(mask), ".", msk);
		// Выполняем разбиение домена на составляющие
		this->fmk->split(this->fmk->toLower(domain), ".", dom);
		// Выполняем реверс данных
		reverse(begin(dom), end(dom));
		reverse(begin(msk), end(msk));
		// Запоминаем размеры массивов
		const size_t dl = dom.size(), ml = msk.size();
		// Запоминаем максимальный размер массива
		const size_t max = (dl > ml ? dl : ml);
		// Запоминаем минимальный размер массива
		const size_t min = (dl < ml ? dl : ml);
		// Переходим по максимальному массиву
		for(u_int i = 0; i < max; i++){
			// Если мы не вышли за границу массива
			if(i < min){
				// Запоминаем значение маски
				itm = msk.at(i);
				// Сравниваем поэлементно
				if((itm.compare(L"*") == 0)
				|| (itm.compare(dom.at(i)) == 0)) result = true;
				// Если сравнение не сработало тогда выходим
				else result = false;
			// Если сравнение получилось
			} else if(itm.compare(L"*") == 0) result = true;
			// Если сравнение не сработало тогда выходим
			else result = false;
			// Если сравнение не удачно то выходим
			if(!result) break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * checkRange Метод проверки входит ли ip адрес в указанный диапазон
 * @param ip  ip данные ip адреса интернет протокола версии 6
 * @param bip начальный диапазон ip адресов
 * @param eip конечный диапазон ip адресов
 * @return    результат проверки
 */
const bool awh::Network::checkRange6(const string & ip, const string & bip, const string & eip) const noexcept {
	// Результат проверки
	bool result = false;
	// Если все данные переданы
	if(!ip.empty() && !bip.empty() && !eip.empty()){
		// Переводим в целочисленный вид
		const __uint64_t nip = this->strIp6ToHex64(this->setLowIp6(ip));
		const __uint64_t nbip = this->strIp6ToHex64(this->setLowIp6(bip));
		const __uint64_t neip = this->strIp6ToHex64(this->setLowIp6(eip));
		// Выполняем сравнение
		result = ((nip >= nbip) && (nip <= neip));
	}
	// Сообщаем что проверка не удалась
	return result;
}
/**
 * setLowIp Метод восстановления IPv4 адреса
 * @param ip адрес интернет протокола версии 4
 * @return   восстановленный вид ip адреса
 */
const string awh::Network::setLowIp(const string & ip) const noexcept {
	// Результат проверки
	string result = "";
	// Если IP адрес передан
	if(!ip.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e("^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})$", regex::ECMAScript);
		// Выполняем поиск ip адреса
		regex_search(ip, match, e);
		// Если данные найдены
		if(!match.empty()){
			// Получаем символы
			string oct1 = match[1].str();
			string oct2 = match[2].str();
			string oct3 = match[3].str();
			string oct4 = match[4].str();
			// Корректируем количество цифр
			this->setZerroToStrIp(oct1);
			this->setZerroToStrIp(oct2);
			this->setZerroToStrIp(oct3);
			this->setZerroToStrIp(oct4);
			// Формируем ip адрес
			result = this->fmk->format("%s.%s.%s.%s", oct1.c_str(), oct2.c_str(), oct3.c_str(), oct4.c_str());
		}
	}
	// Выводим результат
	return result;
}
/**
 * getLowIp Метод упрощения IPv4 адреса
 * @param ip адрес интернет протокола версии 4
 * @return   упрощенный вид ip адреса
 */
const string awh::Network::getLowIp(const string & ip) const noexcept {
	// Результат проверки
	string result = "";
	// Если IP адрес передан
	if(!ip.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e("^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})$", regex::ECMAScript);
		// Выполняем поиск ip адреса
		regex_search(ip, match, e);
		// Если данные найдены
		if(!match.empty()){
			// Получаем символы
			string oct1 = match[1].str();
			string oct2 = match[2].str();
			string oct3 = match[3].str();
			string oct4 = match[4].str();
			// Корректируем количество цифр
			this->rmZerroToStrIp(oct1);
			this->rmZerroToStrIp(oct2);
			this->rmZerroToStrIp(oct3);
			this->rmZerroToStrIp(oct4);
			// Формируем ip адрес
			result = this->fmk->format("%s.%s.%s.%s", oct1.c_str(), oct2.c_str(), oct3.c_str(), oct4.c_str());
		}
	}
	// Выводим результат
	return result;
}
/**
 * getLowIp6 Метод упрощения IPv6 адреса
 * @param ip адрес интернет протокола версии 6
 * @return   упрощенный вид ip адреса
 */
const string awh::Network::getLowIp6(const string & ip) const noexcept {
	// Выполняем преобразование первого порядка
	string str = this->getLow1Ip6(ip);
	// Если строка не существует то присваиваем исходную
	if(str.empty()) str = ip;
	// Выполняем преобразование второго порядка
	return this->getLow2Ip6(str);
}
/**
 * setLowIp6 Метод восстановления IPv6 адреса
 * @param ip адрес интернет протокола версии 6
 * @return   восстановленный вид ip адреса
 */
const string awh::Network::setLowIp6(const string & ip) const noexcept {
	// Выполняем преобразование первого порядка
	string str = this->setLow2Ip6(ip);
	// Если строка не существует то присваиваем исходную
	if(str.empty()) str = ip;
	// Выполняем преобразование второго порядка
	return this->setLow1Ip6(str);
}
/**
 * pathByString Метод извлечения правильной записи адреса
 * @param path первоначальная строка адреса
 * @return     правильная строка адреса
 */
const string awh::Network::pathByString(const string & path) const noexcept {
	// Результат проверки домена
	string result = path;
	// Если адрес передан
	if(!path.empty()){
		// Регулярное выражение для поиска завершающего символа
		regex e("(^\\/[\\w\\/]+\\w+)(?:\\/$|\\/?\\?.+)", regex::ECMAScript | regex::icase);
		// Вырезаем последний символ из адреса (/)
		result = regex_replace(path, e, "$1");
		// Если полученный результат не является параметрами
		if(!result.empty() && (result.substr(0, 1).compare("/") != 0)) result = "";
	}
	// Выводим результат
	return result;
}
/**
 * queryByString Метод извлечения параметров запроса из строки адреса
 * @param path первоначальная строка адреса
 * @return     параметры запроса
 */
const string awh::Network::queryByString(const string & path) const noexcept {
	// Результат проверки домена
	string result = path;
	// Если адрес передан
	if(!path.empty()){
		// Регулярное выражение для поиска параметров запроса
		regex e("(?:^\\/[\\w\\/]+\\w+)(?:\\/$|\\/?(\\?.+))", regex::ECMAScript | regex::icase);
		// Вырезаем параметры запроса
		result = regex_replace(path, e, "$1");
		// Если полученный результат не является параметрами
		if(!result.empty() && (result.substr(0, 1).compare("?") != 0)) result = "";
	}
	// Выводим результат
	return result;
}
/**
 * getIPByNetwork Метод извлечения данных ip адреса из сети
 * @param nwk строка содержащая адрес сети
 * @return    восстановленный вид ip адреса
 */
const string awh::Network::getIPByNetwork(const string & nwk) const noexcept {
	// Результат работы функции
	string result = "";
	// Если сеть передана
	if(!nwk.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e(
			// Если это ip4 адрес
			"((?:\\d{1,3}(?:\\.\\d{1,3}){3})|"
			// Если это ip6 адрес
			"(?:(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:)))",
			regex::ECMAScript | regex::icase
		);
		// Выполняем проверку
		regex_search(nwk, match, e);
		// Если результат найден
		if(!match.empty()) result = match[1].str();
	}
	// Выводим результат
	return result;
}
/**
 * ipv4IntToString Метод преобразования int 2 bytes в строку IP адреса
 * @param ip адрес интернет подключения в цифровом виде
 * @return   ip адрес в виде строки
 */
const string awh::Network::ipv4IntToString(const uint32_t ip) const noexcept {
	// Создаем буфер для хранения ip адреса
	char buffer[INET_ADDRSTRLEN];
	// Заполняем структуру нулями
	memset(buffer, 0, sizeof(buffer));
	// Если - это Unix
	#if !defined(_WIN32) && !defined(_WIN64)
		// Извлекаем данные ip адреса
		inet_ntop(AF_INET, &ip, buffer, INET_ADDRSTRLEN);
	#endif
	// Выводим результат
	return buffer;
}
/**
 * ipv6IntToString Метод преобразования int 8 bytes в строку IP адреса
 * @param ip адрес интернет подключения в цифровом виде
 * @return   ip адрес в виде строки
 */
const string awh::Network::ipv6IntToString(const u_char ip[16]) const noexcept {
	// Создаем буфер для хранения ip адреса
	char buffer[INET6_ADDRSTRLEN];
	// Заполняем структуру нулями
	memset(buffer, 0, sizeof(buffer));
	// Если - это Unix
	#if !defined(_WIN32) && !defined(_WIN64)
		// Извлекаем данные ip адреса
		inet_ntop(AF_INET6, ip, buffer, INET6_ADDRSTRLEN);
	#endif
	// Выводим результат
	return buffer;
}
/**
 * addToPath Метод формирования адреса из пути и названия файла
 * @param path путь где хранится файл
 * @param file название файла
 * @return     сформированный путь
 */
const string awh::Network::addToPath(const string & path, const string & file) const noexcept {
	// Результирующий адрес
	string result;
	// Если параметры переданы
	if(!path.empty() && !file.empty()){
		// Формируем регулярное выражение
		regex pe("\\/+$"), fe("^[\\/\\.\\~]+");
		// Формируем результирующий адрес
		result = this->fmk->format("%s/%s", regex_replace(path, pe, "").c_str(), regex_replace(file, fe, "").c_str());
	}
	// Выводим результат
	return result;
}
/**
 * imposePrefix6 Метод наложения префикса
 * @param ip6    адрес интернет протокола версии 6
 * @param prefix префикс сети
 * @return        результат наложения префикса
 */
const string awh::Network::imposePrefix6(const string & ip6, const u_int prefix) const noexcept {
	// Получаем строку с ip адресом
	string result = "";
	// Если префикс передан
	if(!ip6.empty() && (prefix > 0)){
		// Преобразуем ip адрес
		result = this->setLowIp6(ip6);
		// Если строка существует то продолжаем
		if(!result.empty()){
			// Если префикс меньше 128
			if(prefix < 128){
				// Результат работы регулярного выражения
				smatch match;
				// Копируем ip адрес
				string ip = result;
				// Массив найденных элементов
				vector <string> arr;
				// Устанавливаем правило регулярного выражения
				regex e("([a-f\\d]{4})", regex::ECMAScript | regex::icase);
				// Выполняем удаление из ip адреса хексетов
				while(true){
					// Выполняем поиск ip адреса
					regex_search(ip, match, e);
					// Если данные найдены
					if(!match.empty()){
						// Копируем полученные данные
						const string & delim = match[1].str();
						// Ищем строку еще раз
						const size_t pos = ip.find(delim);
						// Если позиция найдена
						if(pos != string::npos)
							// Удаляем из строки найденный символ
							ip.replace(pos, delim.length(), "");
						// Добавляем в массив найденные данные
						arr.push_back(delim);
					// Если хексет не найден то выходим
					} else break;
				}
				// Искомое число префикса
				u_int fprefix = prefix;
				// Ищем ближайшее число префикса
				while(fmod(fprefix, 4)) fprefix++;
				// Получаем длину адреса
				int len = (fprefix / 16);
				// Компенсируем диапазон
				if(!len) len = 1;
				// Очищаем строку
				result.clear();
				// Переходим по всему полученному массиву
				for(u_int i = 0; i < len; i++){
					// Формируем ip адрес
					result.append(arr.at(i));
					// Добавляем разделитель
					result.append(":");
				}
				// Добавляем оставшиеся нули
				for(u_int i = len; i < 8; i++){
					// Формируем ip адрес
					result.append("0000");
					// Добавляем разделитель
					if(i < 7) result.append(":");
				}
			}
		}
		// Выполняем упрощение ip адреса
		result = this->getLowIp6(result);
	}
	// Выводим полученную строку
	return result;
}
/**
 * pathByDomain Метод создания пути из доменного имени
 * @param domain название домена
 * @param delim  разделитель
 * @return       путь к файлу кэша
 */
const string awh::Network::pathByDomain(const string & domain, const string & delim) const noexcept {
	// Результирующий адрес
	string result;
	// Если параметры переданы
	if(!domain.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e("[\\w\\-\\.]+\\.[\\w\\-]+", regex::ECMAScript | regex::icase);
		// Выполняем проверку
		regex_search(domain, match, e);
		// Если домен существует
		if(!match.empty()){
			// Формируем регулярное выражение
			regex e("\\.");
			// Запоминаем результат
			result = match[0].str();
			// Формируем результирующий адрес
			result = regex_replace(result, e, delim);
			// Выполняем реверс строки
			reverse(result.begin(), result.end());
		}
	}
	// Выводим результат
	return result;
}
/**
 * ipv6StringToInt Метод преобразования строки IP адреса в int 16 байт
 * @param ip адрес интернет подключения
 * @return   ip адрес в цифровом виде
 */
const array <u_char, 16> awh::Network::ipv6StringToInt(const string & ip) const noexcept {
	// Буфер данных ip адреса
	u_char buffer[16];
	// Результат работы функции
	array <u_char, 16> result;
	// Заполняем структуру нулями
	memset(buffer, 0, sizeof(buffer));
	// Если - это Unix
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем преобразование ip адреса
		inet_pton(AF_INET6, ip.c_str(), buffer);
		// Копируем данные буфера в массив
		copy(begin(buffer), end(buffer), result.begin());
	#endif
	// Выводим результат
	return result;
}
/**
 * strIp6ToHex64 Метод преобразования строки ip адреса в 16-й вид
 * @param ip данные ip адреса интернет протокола версии 6
 * @return   результат в 16-м виде
 */
const __uint64_t awh::Network::strIp6ToHex64(const string & ip) const noexcept {
	// Результат работы функции
	__uint64_t result = 0;
	// Создаем поток
	stringstream strm;
	// Устанавливаем правило регулярного выражения
	regex e("[^a-f\\d]", regex::ECMAScript | regex::icase);
	// Убираем лишние символы из 16-го выражения
	string str = regex_replace(ip, e, "");
	// Если число слишком длинное
	if(str.length() > 16) str = str.erase(15, str.length());
	// Записываем полученную строку в поток
	strm << str;
	// Выполняем преобразование в 16-й вид
	strm >> std::hex >> result;
	// Выводим результат
	return result;
}
/**
 * ipv4StringToInt Метод преобразования строки IP адреса в int 2 байта
 * @param ip адрес интернет подключения
 * @return   ip адрес в цифровом виде
 */
const uint32_t awh::Network::ipv4StringToInt(const string & ip) const noexcept {
	// Результат работы функции
	uint32_t result = 0;
	// Если - это Unix
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем преобразование ip адреса
		inet_pton(AF_INET, ip.c_str(), &result);
	#endif
	// Выводим результат
	return result;
}
/**
 * checkNetworkByIp Метод определения типа сети по ip адресу
 * @param ip данные ip адреса
 * @return   тип сети в 10-м виде
 */
const u_short awh::Network::checkNetworkByIp(const string & ip) const noexcept {
	// Результат работы функции
	u_short result = 0;
	// Если IP адрес передан
	if(!ip.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e("^\\d{1,3}(?:\\.\\d{1,3}){3}$", regex::ECMAScript | regex::icase);
		// Выполняем поиск ip адреса
		regex_search(ip, match, e);
		// Если данные найдены
		if(!match.empty()) result = 4;
		// Если это не IPv4
		else {
			// Устанавливаем правило регулярного выражения
			regex e("^\\[?[a-f\\d]{1,4}(?:\\:[a-f\\d]{1,4}){7}\\]?$", regex::ECMAScript | regex::icase);
			// Приводим ip адрес к новой форме
			const string & ip2 = this->setLow2Ip6(ip);
			// Выполняем поиск ip адреса
			regex_search(ip2, match, e);
			// Если данные найдены
			if(!match.empty()) result = 6;
		}
	}
	// Выводим результат
	return result;
}
/**
 * getPrefixByNetwork Метод извлечения данных префикса из строки адреса сети
 * @param nwk строка содержащая адрес сети
 * @return    восстановленный вид префикса сети
 */
const u_int awh::Network::getPrefixByNetwork(const string & nwk) const noexcept {
	// Результат работы функции
	u_int result = 0;
	// Если сеть передана
	if(!nwk.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e(
			// Если это ip4 адрес
			"(?:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))\\/(\\d{1,3}(?:\\.\\d{1,3}){3}|\\d+)",
			regex::ECMAScript | regex::icase
		);
		// Выполняем проверку
		regex_search(nwk, match, e);
		// Если результат найден
		if(!match.empty()){
			// Получаем значение маски
			const string & mask = match[1].str();
			// Если это число
			if(this->fmk->isNumber(mask)) result = stoi(mask);
			// Если это не число то преобразуем маску
			else {
				// Получаем данные маски
				const ntdata_t & data = this->getMaskByString(mask);
				// Запоминаем результат
				result = data.number;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * isLocal Метод проверки на то является ли ip адрес локальным
 * @param ip адрес подключения ip
 * @return   результат проверки (-1 - запрещенный, 0 - локальный, 1 - глобальный)
 */
const int awh::Network::isLocal(const string & ip) const noexcept {
	// Результат работы функции
	int result = -1;
	// Если IP адрес передан
	if(!ip.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e("^\\d{1,3}(?:\\.\\d{1,3}){3}$", regex::ECMAScript | regex::icase);
		// Выполняем поиск ip адреса
		regex_search(ip, match, e);
		// Если данные найдены
		if(!match.empty()){
			// Получаем данные ip адреса
			const ipdata_t & ipData = this->getDataIp(ip);
			// Переходим по всему массиву адресов
			for(auto & local : this->locals){
				// Если ip адрес соответствует маске
				if(this->checkMaskByNumber(ip, local.mask)){
					// Получаем маску каждой сети
					const string & mask = this->getMaskByNumber(local.mask).mask;
					// Оцифровываем данные маски
					const ipdata_t & maskData = this->getDataIp(mask);
					// Накладываем маску на ip адрес
					const ipdata_t & nwkData = this->imposeMask(ipData, maskData);
					// Накладываем маску на ip адрес из списка
					if(nwkData.str().compare(local.network) == 0){
						// Проверяем является ли адрес запрещенным
						if(local.allow) return 0;
						// Сообщаем что адрес запрещен
						else return -1;
					}
				}
			}
			// Сообщаем что адрес глобальный
			result = 1;
		}
	}
	// Выводим результат
	return result;
}
/**
 * isLocal6 Метод проверки на то является ли ip адрес локальным
 * @param ip адрес подключения IPv6
 * @return   результат проверки (-1 - запрещенный, 0 - локальный, 1 - глобальный)
 */
const int awh::Network::isLocal6(const string & ip) const noexcept {
	// Искомый результат
	int result = -1;
	// Если IP адрес передан
	if(!ip.empty()){
		// Запоминаем, что адрес глобальный
		result = 1;
		// Результат сравнения
		bool compare = false;
		// Переходим по всему массиву адресов
		for(auto & local6 : this->locals6){
			// Если сравнение пройдено и есть еще конечная сеть
			if(!local6.eip.empty())
				// Выполняем дополнительную проверку на диапазон сетей
				compare = this->checkRange6(ip, local6.ip, local6.eip);
			else {
				// Хексеты IP адреса
				vector <wstring> mip;
				// Формируем вектор данных ip адреса
				this->fmk->split(this->fmk->toLower(this->setLowIp6(this->imposePrefix6(ip, local6.prefix))), ":", mip);
				// Если данные получены
				if(!mip.empty()){
					// Если первый хекстет нулевой значит это локальный адрес
					if(mip.front().compare(L"0000") == 0) compare = true;
					// Выполняем сравнение по хекстетам
					else {
						// Хексеты сети IPv6
						vector <wstring> nwk;
						// Формируем вектор данных сети
						this->fmk->split(this->fmk->toLower(this->setLowIp6(local6.ip)), ":", nwk);
						// Если хексеты сети получены
						if(!nwk.empty() && (mip.size() == nwk.size())){
							// Начинаем проверять совпадения
							for(u_int j = 0; j < mip.size(); j++){
								// Если значение в маске совпадает тогда продолжаем проверку
								compare = ((mip.at(j).compare(nwk.at(j)) == 0) || (nwk.at(j).compare(L"0000") == 0));
								// Если адреса не совпадают, выходим
								if(!compare) break;
							}
						}
					}
				}
			}
			// Формируем результат
			if(compare) result = (!local6.allow ? -1 : 0);
		}
	}
	// Если локальный адрес найден
	return result;
}
/**
 * parseUri Метод парсинга uri адреса
 * @param uri адрес запроса
 * @return    параметры запроса
 */
const awh::Network::url_t awh::Network::parseUri(const string & uri) const noexcept {
	// Результат работы функции
	url_t result;
	// Если текст передан
	if(!uri.empty()){
		// Позиция поиска
		size_t pos = 0;
		// Состояние извлечения данных
		u_short state = 0;
		// Переходим по всему буквам запроса
		for(size_t i = 0; i < uri.length(); i++){
			// Определяем является ли это последним символом
			bool last = (i >= (uri.length() - 1));
			// Если это 0 стейт то ищем протокол
			if((state == 0) && (uri[i] == ':')){
				// Получаем протокол
				result.ssl = (uri.substr(0, i).compare("https") == 0);
				// Устанавливаем следующее состояние поиска
				state++;
				// Выполняем смещение позиции
				i += 2;
				// Запоминаем текущую позицию
				pos = i;
			// Если это домен
			} else if((state == 1) && ((uri[i] == ':') || (uri[i] == '/') || (uri[i] == '?') || last)) {
				// Получаем протокол
				result.host = (last ? move(uri.substr(pos + 1)) : move(uri.substr(pos + 1, i - (pos + 1))));
				// Запоминаем текущую позицию
				pos = i;
				// Если это порт
				if(uri[i] == ':') state++;
				// Если это параметры uri
				else if((uri[i] == '/') || (uri[i] == '?')) {
					// Получаем uri
					result.query = move(uri.substr(i));
					// Выходим
					break;
				}
			// Если это найден порт
			} else if((state == 2) && ((uri[i] == '/') || (uri[i] == '?') || last)) {
				// Получаем протокол
				result.port = stoi(uri.substr(pos + 1, i - (pos + 1)));
				// Получаем uri
				if((uri[i] == '/') || (uri[i] == '?')) result.query = move(uri.substr(i));
				// Выходим
				break;
			}
		}
		// Если порт не получен
		if(result.port == 0) result.port = (result.ssl ? 443 : 80);
		// Если параметры запроса не указаны
		if(result.query.empty()) result.query = "/";
	}
	// Выводим результат
	return result;
}
/**
 * parseHost Метод определения типа данных из строки URL адреса
 * @param str строка с данными
 * @return    определенный тип данных
 */
const awh::Network::type_t awh::Network::parseHost(const string & str) const noexcept {
	// Результат полученных данных
	type_t result = type_t::NONE;
	// Если строка передана
	if(!str.empty()){
		// Результат работы регулярного выражения
		smatch match;
		// Устанавливаем правило регулярного выражения
		regex e(
			// Определение домена
			"^(?:((?:\\*\\.)?[\\w\\-\\.\\*]+\\.[a-z]+)|"
			// Определение мак адреса
			"([a-f\\d]{2}(?:\\:[a-f\\d]{2}){5})|"
			// Определение ip4 адреса
			"(\\d{1,3}(?:\\.\\d{1,3}){3})|"
			// Определение ip6 адреса
			"(?:\\[?(\\:\\:ffff\\:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))\\]?)|"
			// Если это сеть
			"((?:\\d{1,3}(?:\\.\\d{1,3}){3}|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4})|\\:){1,6}\\:[a-f\\d]{1,4})|(?:[a-f\\d]{1,4}(?:(?:\\:[a-f\\d]{1,4}){7}|(?:\\:[a-f\\d]{1,4}){1,6}\\:\\:|\\:\\:)|\\:\\:))\\/(?:\\d{1,3}(?:\\.\\d{1,3}){3}|\\d+))|"
			// Определение http адреса
			"(https?\\:\\/\\/[\\w\\-\\.]+\\.[a-z]+\\/[^\\r\\n\\t\\s]+)|"
			// Определение адреса
			"(\\.{0,2}\\/\\w+(?:\\/[\\w\\.\\-]+)*)|"
			// Если это метод
			"(OPTIONS|GET|HEAD|POST|PUT|PATCH|DELETE|TRACE|CONNECT))$",
			regex::ECMAScript | regex::icase
		);
		// Выполняем проверку
		regex_search(str, match, e);
		// Если результат найден
		if(!match.empty()){
			// Извлекаем полученные данные
			const string & domain = match[1].str();
			const string & mac = match[2].str();
			const string & ip4 = match[3].str();
			const string & ip6 = match[4].str();
			const string & network = match[5].str();
			const string & httpaddr = match[6].str();
			const string & address = match[7].str();
			const string & method = match[8].str();
			// Определяем тип данных
			if(!domain.empty())        result = type_t::DOMNAME;
			else if(!mac.empty())      result = type_t::MAC;
			else if(!ip4.empty())      result = type_t::IPV4;
			else if(!ip6.empty())      result = type_t::IPV6;
			else if(!network.empty())  result = type_t::NETWORK;
			else if(!address.empty())  result = type_t::ADDRESS;
			else if(!httpaddr.empty()) result = type_t::HTTPADDRESS;
			else if(!method.empty())   result = type_t::HTTPMETHOD;
		}
	}
	// Выводим результат
	return result;
}
