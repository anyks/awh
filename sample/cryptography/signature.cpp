/**
 * @file signature.cpp
 * @date 2026-08-18
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
 * @brief Пример работы с электронной подписью — виды подписи, связка ключей, отпечаток
 *        открытого ключа, длина подписи наперёд и поточная работа
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные модули
 */
#include <iomanip>
#include <iostream>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/log.hpp>
#include <cryptography/crypto.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Функция вывода двоичного буфера шестнадцатеричной записью
 *
 * @param title название выводимого буфера
 * @param data  выводимый буфер
 *
 */
static void print(const string & title, const vector <uint8_t> & data) noexcept {
	// Выводим название буфера
	cout << title << " (" << data.size() << " октетов): ";
	/**
	 * Выполняем перебор октетов буфера
	 */
	for(size_t i = 0; i < data.size(); i++){
		// Выводим первые двенадцать октетов буфера
		if(i < 12)
			// Выводим очередной октет шестнадцатеричной записью
			cout << hex << setw(2) << setfill('0') << static_cast <uint32_t> (data.at(i));
		// Обрываем вывод длинного буфера многоточием
		else {
			// Выводим признак обрыва вывода
			cout << "...";
			// Выходим из цикла вывода
			break;
		}
	}
	// Возвращаем запись вывода к десятичной
	cout << dec << endl;
}
/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Снимаем предупреждения о неиспользуемых параметрах
	(void) argc;
	(void) argv;
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект для работы с логами
	log_t log(&fmk);
	// Создаём объект для работы с криптографией
	crypto_t crypto(&fmk, &log);
	// Подписываемые данные
	const string data = "ANYKS Framework, электронная подпись";
	/**
	 * Ключей на объекте держится несколько, и всякий зовётся своим именем: один
	 * документ подписывают владелец и заверитель, а проверяющая сторона сличает с
	 * несколькими открытыми ключами подряд
	 */
	// Печатаем заголовок вывода видов подписи
	cout << " ======== ВИДЫ ПОДПИСИ ======== " << endl;
	/**
	 * Выполняем перебор всех заведённых видов подписи
	 */
	for(auto & kind : {crypto_t::signature_t::ED25519, crypto_t::signature_t::ECDSA, crypto_t::signature_t::RSA}){
		// Название вида подписи
		const string name = ((kind == crypto_t::signature_t::ED25519) ? "ed25519" : ((kind == crypto_t::signature_t::ECDSA) ? "ecdsa" : "rsa"));
		// Выполняем выработку ключа подписи
		if(!crypto.generateKey(name, kind)){
			// Выводим сообщение об отказе выработки ключа
			cout << "Ключ " << name << " выработать не удалось" << endl;
			// Переходим к следующему виду подписи
			continue;
		}
		/**
		 * Тип хэш-суммы обязателен схемам, подписывающим хэш-сумму, и неуместен схеме
		 * Ed25519: она подписывает сообщение сама, и поданная ей хэш-сумма будет
		 * отвергнута - договор различие это выражает прямо, а не сглаживает
		 */
		// Тип хэш-суммы, схеме подписи отвечающий
		const crypto_t::hash_t hash = ((kind == crypto_t::signature_t::ED25519) ? crypto_t::hash_t::NONE : crypto_t::hash_t::SHA256);
		// Буфер подписи
		vector <uint8_t> signature;
		// Выполняем подписание данных
		if(crypto.sign(name, reinterpret_cast <const uint8_t *> (data.data()), data.size(), hash, signature)){
			// Выводим название вида подписи
			cout << "Вид подписи: " << name << endl;
			/**
			 * Длина подписи спрашивается наперёд: работам, правящим запись на месте,
			 * место под подпись приходится резервировать заранее. Точной длины схема
			 * ECDSA не имеет - запись её несёт два числа переменной длины, - и такая
			 * схема отвечает нулём, а место резервируется по верхнему пределу
			 */
			// Выводим длину подписи, объявленную наперёд
			cout << "  длина наперёд: " << crypto.length(name) << ", верхний предел: " << crypto.limit(name) << endl;
			// Выводим выработанную подпись
			print("  подпись", signature);
			/**
			 * Отпечаток открытого ключа отвечает на вопрос «чей это документ» прежде
			 * всякой проверки подписи. Считается он от открытой части и выдаётся тогда,
			 * когда закрытого ключа нет вовсе
			 */
			// Выводим отпечаток открытого ключа
			print("  отпечаток", crypto.fingerprint <vector <uint8_t>> (name));
			// Выводим результат проверки подписи
			cout << "  проверка: " << (crypto.verify(name, reinterpret_cast <const uint8_t *> (data.data()), data.size(), signature, hash) ? "принята" : "отвергнута") << endl;
			// Выполняем порчу одного разряда подписи
			signature[signature.size() / 2] ^= 0x01;
			// Выводим результат проверки испорченной подписи
			cout << "  проверка испорченной: " << (crypto.verify(name, reinterpret_cast <const uint8_t *> (data.data()), data.size(), signature, hash) ? "принята" : "отвергнута") << endl;
			// Выводим поточность вида подписи
			cout << "  поточная работа: " << (crypto.streamable(kind) ? "есть" : "нет") << endl;
		}
	}
	// Возвращаем пустую строку
	cout << endl;
	/**
	 * Проверяющая сторона закрытого ключа не имеет: ей довольно открытого, и подписать
	 * им она не сможет
	 */
	// Печатаем заголовок вывода проверки одним открытым ключом
	cout << " ======== ПРОВЕРКА ОДНИМ ОТКРЫТЫМ КЛЮЧОМ ======== " << endl;
	// Буфер подписи владельца
	vector <uint8_t> signature;
	// Выполняем подписание данных ключом владельца
	if(crypto.sign("ed25519", reinterpret_cast <const uint8_t *> (data.data()), data.size(), crypto_t::hash_t::NONE, signature)){
		// Получаем запись открытого ключа владельца
		const string & key = crypto.getKey("ed25519", crypto_t::key_type_t::PUBLIC);
		// Создаём объект проверяющей стороны
		crypto_t verifier(&fmk, &log);
		// Выполняем ввод одного лишь открытого ключа
		if(verifier.setKey("owner", key, crypto_t::key_type_t::PUBLIC)){
			// Выводим отпечаток введённого открытого ключа
			print("Отпечаток владельца", verifier.fingerprint <vector <uint8_t>> ("owner"));
			// Выводим результат проверки подписи одним открытым ключом
			cout << "Проверка открытым ключом: " << (verifier.verify("owner", reinterpret_cast <const uint8_t *> (data.data()), data.size(), signature, crypto_t::hash_t::NONE) ? "принята" : "отвергнута") << endl;
			// Буфер подписи проверяющей стороны
			vector <uint8_t> denied;
			// Выводим результат попытки подписать одним открытым ключом
			cout << "Подпись открытым ключом: " << (verifier.sign("owner", reinterpret_cast <const uint8_t *> (data.data()), data.size(), crypto_t::hash_t::NONE, denied) ? "выработана" : "отвергнута") << endl;
		}
	}
	// Возвращаем пустую строку
	cout << endl;
	/**
	 * Поточная работа нужна записи, в память не поднимающейся. Схема Ed25519 поточной
	 * работы не имеет вовсе - спросить об этом можно наперёд через streamable, - а
	 * схемы RSA и ECDSA подписывают хэш-сумму, и та набирается порциями
	 */
	// Печатаем заголовок вывода поточной работы
	cout << " ======== ПОТОЧНАЯ ПОДПИСЬ И ПРОВЕРКА ======== " << endl;
	// Буфер подписи потока
	vector <uint8_t> streamed;
	// Выполняем заведение потока подписи
	if(crypto.signInitialize("ecdsa", crypto_t::hash_t::SHA256)){
		/**
		 * Выполняем подачу данных потоку порциями по восемь октетов
		 */
		for(size_t offset = 0; offset < data.size(); offset += 8)
			// Выполняем подачу очередной порции потоку подписи
			crypto.signUpdate(reinterpret_cast <const uint8_t *> (data.data() + offset), ((data.size() - offset) < 8 ? (data.size() - offset) : 8));
		// Выполняем завершение потока подписи
		if(crypto.signFinalize(streamed))
			// Выводим выработанную поточно подпись
			print("Подпись потоком", streamed);
	}
	// Выполняем заведение потока проверки подписи
	if(crypto.verifyInitialize("ecdsa", crypto_t::hash_t::SHA256)){
		/**
		 * Выполняем подачу данных потоку проверки порциями по восемь октетов
		 */
		for(size_t offset = 0; offset < data.size(); offset += 8)
			// Выполняем подачу очередной порции потоку проверки
			crypto.verifyUpdate(reinterpret_cast <const uint8_t *> (data.data() + offset), ((data.size() - offset) < 8 ? (data.size() - offset) : 8));
		// Выводим результат поточной проверки подписи
		cout << "Проверка потоком: " << (crypto.verifyFinalize(streamed) ? "принята" : "отвергнута") << endl;
	}
	/**
	 * Поток на объекте один, а работы у него две: подача не в свой поток отвергается, а
	 * заведение поверх незавершённого потока прежний сбрасывает и оглашает это
	 */
	// Выполняем заведение потока подписи
	if(crypto.signInitialize("ecdsa", crypto_t::hash_t::SHA256))
		// Выводим результат подачи в поток проверки, заведённый выработкой
		cout << "Подача в чужой поток: " << (crypto.verifyUpdate(reinterpret_cast <const uint8_t *> (data.data()), data.size()) ? "принята" : "отвергнута") << endl;
	// Выводим результат заведения потока подписи схемой, поточной работы не имеющей
	cout << "Поток у Ed25519: " << (crypto.signInitialize("ed25519", crypto_t::hash_t::SHA256) ? "заведён" : "отвергнут") << endl;
	// Возвращаем пустую строку
	cout << endl;
	/**
	 * Ключи хранятся записью PEM: закрытый выдаётся защищённым паролем, если пароль
	 * защиты установлен, а файл его заводится доступным одному лишь владельцу
	 */
	// Печатаем заголовок вывода хранения ключей
	cout << " ======== ХРАНЕНИЕ КЛЮЧЕЙ ======== " << endl;
	// Устанавливаем пароль защиты закрытого ключа
	crypto.passwordRSA("password");
	// Выполняем запись закрытого ключа в файл
	if(crypto.saveKey("ed25519", "signature_private.pem", crypto_t::key_type_t::PRIVATE)){
		// Создаём объект, читающий ключ из файла
		crypto_t reader(&fmk, &log);
		// Устанавливаем пароль защиты закрытого ключа
		reader.passwordRSA("password");
		// Выполняем чтение закрытого ключа из файла
		if(reader.loadKey("owner", "signature_private.pem", crypto_t::key_type_t::PRIVATE))
			// Выводим отпечаток прочитанного ключа
			print("Отпечаток прочитанного", reader.fingerprint <vector <uint8_t>> ("owner"));
		// Выполняем снятие файла закрытого ключа
		::remove("signature_private.pem");
	}
	// Выполняем снятие ключа из связки
	cout << "Снятие ключа из связки: " << (crypto.removeKey("rsa") ? "выполнено" : "отказано") << endl;
	// Выводим вид подписи снятого ключа
	cout << "Вид снятого ключа: " << static_cast <uint16_t> (crypto.signature("rsa")) << " (0 значит, что ключа нет)" << endl;
	// Возвращаем пустую строку
	cout << endl;
	// Возвращаем результат
	return EXIT_SUCCESS;
}
