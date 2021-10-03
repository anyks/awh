/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <server/socks5.hpp>

/**
 * resCmd Метод получения бинарного буфера ответа
 */
void awh::Socks5Server::resCmd() const noexcept {

}
/**
 * resAuth Метод получения бинарного буфера ответа на авторизацию клиента
 */
void awh::Socks5Server::resAuth() const noexcept {

}
/**
 * resMethod Метод получения бинарного буфера выбора метода подключения
 * @param methods методы авторизаций выбранныйе пользователем
 */
void awh::Socks5Server::resMethod(const vector <method_t> & methods) const noexcept {
	// Создаём объект ответа
	resMet_t response;
	// Устанавливаем версию прокси-протокола
	response.ver = VER;
	// Устанавливаем запрещённый метод авторизации
	response.method = (uint8_t) method_t::NOMETHOD;
	// Если пользователь выбрал список методов
	if(!methods.empty()){
		// Переходим по всем методам авторизаций
		for(auto & method : methods){
			// Если метод авторизации выбран логин/пароль пользователя
			if(method == method_t::PASSWD){
				// Если пользователи установлены
				if((this->users != nullptr) && !this->users->empty()){
					// Устанавливаем метод прокси-сервера
					response.method = (uint8_t) method;
					// Выходим из цикла
					break;
				}
			// Если пользователь выбрал метод без авторизации
			} else if(method == method_t::NOAUTH) {
				// Если пользователи не установлены
				if((this->users == nullptr) || this->users->empty()){
					// Устанавливаем метод прокси-сервера
					response.method = (uint8_t) method;
					// Выходим из цикла
					break;
				}
			}
		}
	}
	// Очищаем бинарный буфер данных
	this->buffer.clear();
	// Увеличиваем память на 4 октета
	this->buffer.resize(sizeof(uint8_t) * 2, 0x0);
	// Копируем в буфер нашу структуру ответа
	memcpy(&response, this->buffer.data(), sizeof(response));
}
/**
 * parse Метод парсинга входящих данных
 * @param buffer бинарный буфер входящих данных
 * @param size   размер бинарного буфера входящих данных
 */
void awh::Socks5Server::parse(const char * buffer, const size_t size) noexcept {

}
/**
 * reset Метод сброса собранных данных
 */
void awh::Socks5Server::reset() noexcept {
	// Выполняем сброс статуса ошибки
	this->code = 0x00;
	// Выполняем очистку буфера данных
	this->buffer.clear();
	// Выполняем сброс стейта
	this->state = state_t::METHOD;
}
/**
 * setUsers Метод добавления списка пользователей
 * @param users список пользователей для добавления
 */
void awh::Socks5Server::setUsers(const unordered_map <string, string> * users) noexcept {
	// Устанавливаем список пользователей
	this->users = users;
}
