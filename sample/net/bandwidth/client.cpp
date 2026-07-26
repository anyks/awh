/**
 * @file: client.cpp
 * @date: 2026-02-13
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Пример клиента для измерения пропускной способности канала — демонстрация непрерывной отправки данных на
 *        сервер средствами движка ввода-вывода и подсчёта достигнутой скорости передачи
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <net/io.hpp>

/**
 * Используем пространство имён AWH
 */
using namespace awh;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Главная функция приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]){
	// Создаём объект фреймворка
	fmk_t fmk;
	// Создаём объект логирования
	log_t log(&fmk);
	// Устанавливаем логгер
	fmk.setLogger(&log);
	// Устанавливаем уровень логирования
	log.level(log_t::level_t::NONE);
	// Создаём объект асинхронного движка ввода-вывода
	engine::io_t io(&fmk, &log);
	// Добавляем новое событие клиента TCP
	event::id_t eid = io.event(event::node_t::CLIENT, event::family_t::IPV4, event::type_t::STREAM, event::protocol_t::TCP);
	// Устанавливаем порт события
	io.setTargetPort(eid, 2222);
	// Инициализируем асинхронный движок ввода-вывода
	if(io.initialize()){
		// Устананавливаем опции события
		if(io.setOptions(eid, event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC | event::options::TCP_NO_DELAY | event::options::KEEPALIVE))
			// Записываем в лог сообщение об успешной установке опций события
			cout << " Успешно установлены опции события!" << endl;
		// Записываем ошибку в лог установки опций события
		else cout << " Ошибка установки опций события!" << endl;
		// Устанавливаем IP-адрес события
		if(io.setAddress(eid, event::address_t::IPV4, "0.0.0.0") && io.bandwidth(eid, event::limiting_t::INGRESS, "100 Mbps") && io.bandwidth(eid, event::limiting_t::EGRESS, "100 Mbps")){
		// if(io.setAddress(eid, event::address_t::IPV4, "0.0.0.0")){
			// Устанавливаем адрес сервера назначения
			if(io.setTarget(eid, "127.0.0.1")){
				// Счётчики отправленных и полученных байт
				size_t bytesSent = 0, bytesReceived = 0;
				// Дата и время последнего отправленного сообщения
				uint64_t dateSent = fmk.timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
				// Дата и время последнего полученного сообщения
				uint64_t dateReceived = fmk.timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
				/**
				 * @brief Функция бесконечной отправки данных клиенту
				 *
				 * @param id идентификатор события
				 *
				 */
				auto sendMessage = [&io, &log](const event::id_t id) noexcept -> void {
					// Сообщение для отправки
					const string message = R"(TEXT(AWH LICENSE
                        Version 1.0, July 2026

              SPDX short identifier: LicenseRef-AWH-1.0

  Copyright (c) 2025-2026 Yuriy Lobarev (ANYKS). All rights reserved.
  Project repository: https://github.com/anyks/awh

  ANYKS Web Hub (AWH) is free to use, including in commercial and
  closed-source products, provided that the author is credited and that
  the Library is used as a whole and never taken apart.


0. DEFINITIONS

   "Licensor" means Yuriy Lobarev (ANYKS), the author and copyright
   holder of the Library.

   "Library" means the software project ANYKS Web Hub (AWH), taken as a
   whole, including its source code, header files, build system and
   documentation, as published by the Licensor at
   https://github.com/anyks/awh
   The Library does NOT include Third-Party Software as defined below,
   even where such software is bundled with, vendored into, referenced
   as a submodule of, or statically linked into the Library or a Library
   Build.

   "Third-Party Software" means any software included in, vendored into,
   referenced by or built together with the Library that was not
   authored by the Licensor and that carries the copyright notice or
   license of another party. This includes, by way of example and
   without limitation, the git submodules of the project, any code
   vendored into the project tree from an external origin, any build
   artifact of the foregoing installed into the project tree, and any
   external dependency the Library is linked against. Authorship, not
   location in the project tree, determines whether something is
   Third-Party Software.

   "Library Build" means a complete binary artifact produced from the
   Library as a whole, such as libawh.a, libawh.so, libawh.dylib,
   libawh.dll or awh.lib, together with the corresponding public
   header files.

   "Component" means any part of the Library taken in isolation: an
   individual source or header file, a module, a class, a function, an
   algorithm implementation, a data structure, or any other subset of
   the Library that is less than the Library as a whole.

   "You" means the individual or legal entity exercising the rights
   granted by this License.

   "Your Product" means any software, service, device or other product
   developed by You that links to, embeds or otherwise makes use of the
   Library as a whole.


1. GRANT OF RIGHTS

   Subject to Sections 2 and 3, the Licensor hereby grants You a
   worldwide, royalty-free, non-exclusive, perpetual and irrevocable
   (except as provided in Section 6) license to:

   (a) use, build and run the Library on any operating system, hardware
       platform and architecture, without any limitation on the number
       of installations, users, servers, cores or instances;

   (b) link the Library into Your Product as a whole, either statically
       (libawh.a, awh.lib) or dynamically (libawh.so, libawh.dylib,
       libawh.dll);

   (c) reproduce and distribute the Library and Library Builds, in
       whole, whether standalone, bundled with Your Product, or as part
       of an installer, container image or operating system image;

   (d) use the Library for any purpose whatsoever, including commercial,
       proprietary, internal, governmental and closed-source purposes;

   (e) modify the Library for the needs of Your Product, provided the
       result remains the Library as a whole and the restrictions of
       Section 3 are observed.

   No fee, registration, notification or separate permission from the
   Licensor is required to exercise these rights.


2. ATTRIBUTION (MANDATORY CONDITION)

   The rights granted in Section 1 are conditional upon compliance with
   this Section.

   2.1 You must retain, in unmodified form, all copyright notices,
       authorship notices and license notices contained in the Library,
       including the file headers of its source and header files. A copy
       of this License must accompany any distribution of the Library or
       of a Library Build.

   2.2 Your Product must credit the Library in a place that is
       reasonably accessible to the users of Your Product - such as the
       user documentation, an "About" or "Credits" screen, a legal
       notices section, a "--version" or "--help" output, a README file,
       or a third-party licenses page - by reproducing the following
       notice:

           This product uses AWH (ANYKS Web Hub) by Yuriy Lobarev
           (ANYKS).
           https://github.com/anyks/awh

       The repository address must be reproduced verbatim and, in any
       medium that supports hyperlinks, must be presented as a working
       link.

   2.3 The attribution required by Section 2.2 must not be removed,
       hidden, obfuscated, disclaimed, made conditional on payment, or
       presented in a manner materially less prominent than credits
       given to other third-party software used in Your Product.

   2.4 You must not state or imply that the Licensor endorses, sponsors
       or supports Your Product.


3. RESTRICTIONS

   3.1 Integrity of the Library. The Library may be used and distributed
       only as a whole. You must NOT extract, copy, split off, port,
       translate, transcribe, adapt or otherwise reuse any Component
       separately from the Library. In particular, and without
       limitation, You must not:

       (a) copy source files or portions of source code of the Library
           into another project, library, framework or code base;

       (b) publish, distribute or make available any Component as a
           standalone library, package, module, plugin, snippet or
           example, whether in source or binary form;

       (c) build, distribute or link against a partial build of the
           Library that provides only a subset of its functionality for
           the purpose of reusing that subset elsewhere;

       (d) mechanically or manually derive a separate work from a
           Component, including by machine translation to another
           programming language or by automated code generation.

       Build-time configuration of the Library by means of its own
       official build system (for example, disabling optional features
       or optional third-party dependencies) is expressly permitted and
       does not constitute a violation of this Section, provided that
       the resulting artifact is still a build of the Library itself and
       is used as such.

       This Section applies exclusively to material authored by the
       Licensor. It places no restriction whatsoever on Third-Party
       Software: You remain free to obtain, use, modify, extract and
       redistribute any Third-Party Software separately, on the terms of
       its own license, whether or not You use the Library.

   3.2 No repackaging or re-licensing. You must not distribute the
       Library, a Library Build or any Component under a different name,
       present it as Your own work, or sublicense or re-license it under
       any other terms. This License must remain the sole license under
       which the Library is made available to third parties.

   3.3 Modified versions. If You distribute a modified version of the
       Library, that version must remain complete, must be conspicuously
       marked as modified together with the date and nature of the
       modification, must retain all notices required by Section 2, must
       be distributed under this License, and remains fully subject to
       Section 3.1.

   3.4 Trademarks. This License grants no rights to use the names
       "ANYKS", "AWH", "ANYKS Web Hub", the Licensor's name, or any
       associated logos or trade dress, except as strictly required by
       the attribution notice in Section 2.2.


4. YOUR OWN SOURCE CODE

   Nothing in this License requires You to disclose, publish, share or
   license the source code of Your Product, or of any part of it. Linking
   the Library into Your Product, whether statically or dynamically,
   does not subject Your Product to any obligation to be made open
   source, and does not grant the Licensor or any third party any right
   in Your Product. Your Product remains entirely Yours.


5. THIRD-PARTY SOFTWARE

   The Library uses and may bundle Third-Party Software (including,
   without limitation, compression, cryptography and text-processing
   libraries), each of which is distributed under its own license terms.

   This License applies solely to the material authored by the Licensor.
   It does not modify, restrict or extend the terms applicable to
   Third-Party Software, and it grants the Licensor no rights in it.
   Nothing in this License - and in particular nothing in Section 3 -
   may be construed as limiting any right You have in Third-Party
   Software under its own license, including the right to use it
   separately from the Library.

   In the event of a conflict, the terms of the respective third-party
   license govern that third-party software, and You are responsible for
   complying with them.

   Where the license of a piece of Third-Party Software grants You
   rights that this License would otherwise restrict (for example, the
   right under the GNU LGPL to relink a Library Build against a modified
   version of that software), those rights prevail, and exercising them
   is not a breach of this License.


6. TERM AND TERMINATION

   6.1 This License is effective until terminated.

   6.2 Your rights under this License terminate automatically if You
       breach any of its terms, in particular Sections 2 and 3.

   6.3 If the breach is curable and You cure it within thirty (30) days
       of becoming aware of it, Your rights are reinstated automatically
       and retroactively, unless the Licensor has terminated Your rights
       expressly and in writing for a repeated breach.

   6.4 Upon termination, You must cease all use and distribution of the
       Library and of Library Builds. Sections 4, 5, 7, 8 and 9 survive
       termination.


7. DISCLAIMER OF WARRANTY

   THE LIBRARY IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
   NONINFRINGEMENT. THE ENTIRE RISK AS TO THE QUALITY, PERFORMANCE,
   SECURITY AND SUITABILITY OF THE LIBRARY IS WITH YOU. THE LICENSOR IS
   UNDER NO OBLIGATION TO PROVIDE SUPPORT, MAINTENANCE, UPDATES OR
   BUG FIXES.


8. LIMITATION OF LIABILITY

   IN NO EVENT AND UNDER NO LEGAL THEORY, WHETHER IN CONTRACT, TORT,
   NEGLIGENCE OR OTHERWISE, SHALL THE LICENSOR BE LIABLE FOR ANY DIRECT,
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES,
   INCLUDING BUT NOT LIMITED TO LOSS OF PROFITS, LOSS OF DATA, BUSINESS
   INTERRUPTION OR COMPUTER FAILURE, ARISING OUT OF THE USE OF OR
   INABILITY TO USE THE LIBRARY, EVEN IF ADVISED OF THE POSSIBILITY OF
   SUCH DAMAGES. TO THE EXTENT THAT APPLICABLE LAW DOES NOT ALLOW THE
   EXCLUSION OF CERTAIN LIABILITY, THE LICENSOR'S TOTAL AGGREGATE
   LIABILITY SHALL BE LIMITED TO THE AMOUNT ACTUALLY PAID BY YOU TO THE
   LICENSOR FOR THE LIBRARY, WHICH MAY BE ZERO.


9. MISCELLANEOUS

   9.1 If any provision of this License is held unenforceable, it shall
       be reformed only to the extent necessary to make it enforceable,
       and the remaining provisions shall remain in full force.

   9.2 Failure by the Licensor to enforce any provision is not a waiver
       of that provision.

   9.3 The Licensor may publish new versions of this License for future
       releases of the Library. Each release of the Library is governed
       by the version of the License distributed with it; You may also
       choose to use any later version published by the Licensor.

   9.4 This License is the entire agreement between You and the Licensor
       concerning the Library and supersedes any prior understanding.

   9.5 A separate written agreement with the Licensor may grant rights
       beyond those in this License, including the right to use
       Components separately. Requests: forman@anyks.com


                          END OF TERMS AND CONDITIONS)";
					/**
					 * Включаем бесконечную отправку данных
					 */
					for(;;){
						// Отправляем данные клиенту
						if(io.send(id, reinterpret_cast <const uint8_t *> (message.c_str()), message.size()))
							// Если данные успешно отправлены
							log.print("Отправлено: ID=%u, %zu байт, сообщение: %s", log_t::flag_t::INFO, id, message.size(), message.c_str());
						// Если данные не отправлены
						else {
							// Записываем ошибку в лог отправки данных
							log.print("Ошибка отправки: ID=%u", log_t::flag_t::CRITICAL, id);
							// Выходим из цикла отправки данных
							break;
						}
					}
				};
				// Устанавливаем функцию обратного вызова на событие очереди
				io.on(eid, [&sendMessage, &log](const event::id_t eid, const event::status_t status, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (status)){
						// Если статус переполнения очереди события
						case static_cast <uint8_t> (event::status_t::QUEUE_OVERFLOW):
							// Записываем в лог сообщение о переполнении очереди события
							cout << " Событие переполнения очереди: ID=" << eid << ", размер доступности: " << size << " байт!" << endl;
						break;
						// Если статус доступности очереди события
						case static_cast <uint8_t> (event::status_t::QUEUE_AVAILABLE): {
							// Записываем в лог сообщение о доступности очереди события
							cout << " Событие доступности очереди: ID=" << eid << ", размер доступности: " << size << " байт!" << endl;
							// Запускаем новый цикл отправки данных
							sendMessage(eid);
						} break;
					}
				});
				// Устанавливаем функцию обратного вызова на событие неудачной отправки данных
				io.on(eid, [&log](const event::id_t eid, const event::send_error_t error, const uint8_t * buffer, const size_t size) noexcept -> void {
					/**
					 * Обрабатываем статус ошибки отправки данных события
					 */
					switch(static_cast <uint8_t> (error)){
						// Если статус ошибки отправки данных события
						case static_cast <uint8_t> (event::send_error_t::IO_EVENT):
							// Записываем в лог сообщение о ошибке отправки данных события
							log.print("Событие ошибки отправки данных: ID=%u", log_t::flag_t::CRITICAL, eid);
						break;
						// Если статус ошибки отправки данных события из очереди
						case static_cast <uint8_t> (event::send_error_t::IO_QUEUE):
							// Записываем в лог сообщение о ошибке отправки данных события из очереди
							log.print("Событие ошибки отправки данных из очереди: ID=%u", log_t::flag_t::CRITICAL, eid);
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на событие клиента
				io.on(eid, [&log](const event::id_t eid, const event::status_t status) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (status)){
						// Если статус принятия
						case static_cast <uint8_t> (event::status_t::ACCEPTED):
							// Записываем в лог сообщение о принятии события
							log.print("Событие принято: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус уничтожения
						case static_cast <uint8_t> (event::status_t::DESTROYED):
							// Записываем в лог сообщение об уничтожении события
							log.print("Событие подлежит уничтожению: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус инициализации
						case static_cast <uint8_t> (event::status_t::INITIAL):
							// Записываем в лог сообщение об инициализации события
							log.print("Событие инициализировано: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус запуска события
						case static_cast <uint8_t> (event::status_t::LAUNCHED):
							// Записываем в лог сообщение о запуске события
							log.print("Событие запущено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус паузы события
						case static_cast <uint8_t> (event::status_t::PAUSED):
							// Записываем в лог сообщение о паузе события
							log.print("Событие на паузе: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус возобновления события
						case static_cast <uint8_t> (event::status_t::RESUMED):
							// Записываем в лог сообщение о возобновлении события
							log.print("Событие возобновлено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус успешного выполнения события
						case static_cast <uint8_t> (event::status_t::SUCCESS):
							// Записываем в лог сообщение о успешном выполнении события
							log.print("Событие успешно выполнено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус неудачного выполнения события
						case static_cast <uint8_t> (event::status_t::FAILURE):
							// Записываем в лог сообщение о неудачном выполнении события
							log.print("Событие выполнено с ошибкой: ID=%u", log_t::flag_t::CRITICAL, eid);
						break;
						// Если статус выполнения события в ожидании
						case static_cast <uint8_t> (event::status_t::PENDING):
							// Записываем в лог сообщение о выполнении события в ожидании
							log.print("Событие в ожидании: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус подключения события
						case static_cast <uint8_t> (event::status_t::CONNECTED):
							// Записываем в лог сообщение о подключении события
							log.print("Событие подключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус отмены события
						case static_cast <uint8_t> (event::status_t::CANCELLED):
							// Записываем в лог сообщение об отмене события
							log.print("Событие отменено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус переподключения события
						case static_cast <uint8_t> (event::status_t::RECONNECTED):
							// Записываем в лог сообщение о переподключении события
							log.print("Событие переподключено: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если статус прослушивания события
						case static_cast <uint8_t> (event::status_t::LISTENING):
							// Записываем в лог сообщение о прослушивании события
							log.print("Событие прослушивается: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на запись в событие
				io.on(eid, static_cast <engine::callback::write_t> ([&bytesSent, &dateSent, &fmk, &log](const event::id_t eid, const size_t size) noexcept -> void {
					// Получаем текущее время в миллисекундах
					const uint64_t currentTime = fmk.timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
					// Если с момента последнего отправленного сообщения прошло 60 секунд
					if((currentTime - dateSent) >= 60000){
						// Биты в секунду = (байты * 8 бит/байт * 1000 мс/сек) / миллисекунды
						const double speed = ((bytesSent * 8.0 * 1000.0) / (currentTime - dateSent));
						// Вычисляем скорость передачи данных в битах в секунду и выводим её на консоль
						cout << " Скорость передачи данных: ID=" << eid << ", " << (speed / 1000000.) << " Мбит/сек" << endl;
						// Сбрасываем счётчик отправленных байт
						bytesSent = 0;
						// Обновляем дату и время последнего отправленного сообщения
						dateSent = currentTime;
					}
					// Увеличиваем счётчик отправленных байт
					bytesSent += size;
					// Записываем в лог сообщение о отправке данных события
					cout << " Отправлено: ID=" << eid << ", " << size << " байт" << endl;
					// Записываем в лог сообщение о переподключении события
					// log.print("Записано: ID=%u, %zu байт", log_t::flag_t::INFO, eid, size);
				}));
				// Устанавливаем функцию обратного вызова на чтение из события
				io.on(eid, [&bytesReceived, &dateReceived, &fmk, &log](const event::id_t eid, const uint8_t * data, const size_t size) noexcept -> void {
					// Получаем текущее время в миллисекундах
					const uint64_t currentTime = fmk.timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
					// Если с момента последнего отправленного сообщения прошло 60 секунд
					if((currentTime - dateReceived) >= 60000){
						// Вычисляем скорость передачи данных в битах в секунду и выводим её на консоль
						const double speed = ((bytesReceived * 8.0 * 1000.0) / (currentTime - dateReceived));
						// Печатаем скорость передачи (бит/с)
						cout << " Скорость приёма данных: ID=" << eid << ", " << (speed / 1000000.) << " Мбит/сек" << endl;
						// Сбрасываем счётчик полученных байт
						bytesReceived = 0;
						// Обновляем дату и время последнего отправленного сообщения
						dateReceived = currentTime;
					}
					// Увеличиваем счётчик полученных байт
					bytesReceived += size;
					// Текст входящего сообщения
					const string message(reinterpret_cast <const char *> (data), size);
					// Записываем в лог сообщение о чтении данных
					cout << " Прочитано: ID=" << eid << ", " << size << " байт, сообщение: " << message << endl;
					// Записываем в лог сообщение о переподключении события
					// log.print("Прочитано: ID=%u, %zu байт, сообщение: %s", log_t::flag_t::INFO, eid, size, message.c_str());
				});
				// Устанавливаем функцию обратного вызова на ошибку события
				io.on(eid, [&log](const event::id_t eid, const event::error_t error, const string & description) noexcept -> void {
					/**
					 * Обрабатываем статус события
					 */
					switch(static_cast <uint8_t> (error)){
						// Если ошибка неизвестного события
						case static_cast <uint8_t> (event::error_t::UNKNOWN):
							// Записываем ошибку в лог неизвестного события
							log.print("Неизвестная ошибка события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недопустимой операции
						case static_cast <uint8_t> (event::error_t::INVALID):
							// Записываем ошибку в лог недопустимой операции
							log.print("Недопустимая операция события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа запрещёния
						case static_cast <uint8_t> (event::error_t::ACCESS_DENIED):
							// Записываем ошибку в лог доступа запрещёния
							log.print("Доступ к событию запрещён: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка уже существующего объекта
						case static_cast <uint8_t> (event::error_t::ALREADY_EXISTS):
							// Записываем ошибку в лог уже существующего объекта
							log.print("Объект события уже существует: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка доступа к сокету
						case static_cast <uint8_t> (event::error_t::INVALID_SOCKET):
							// Записываем ошибку в лог доступа к сокету
							log.print("Ошибка доступа к сокету события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка некорректного адреса
						case static_cast <uint8_t> (event::error_t::INVALID_ADDRESS):
							// Записываем ошибку в лог некорректного адреса
							log.print("Некорректный адрес события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка ошибки подключения
						case static_cast <uint8_t> (event::error_t::CONNECTION_FAIL):
							// Записываем ошибку в лог подключения
							log.print("Ошибка подключения события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка недостаточно ресурсов
						case static_cast <uint8_t> (event::error_t::INSUFFICIENT_RES):
							// Записываем ошибку в лог недостаточно ресурсов
							log.print("Недостаточно ресурсов для события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если ошибка события
						case static_cast <uint8_t> (event::error_t::EVENT_FAIL):
							// Записываем ошибку в лог события
							log.print("Ошибка события: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
						// Если объект не найден
						case static_cast <uint8_t> (event::error_t::NOT_FOUND):
							// Записываем ошибку в лог события
							log.print("Объект события не найден: ID=%u, Описание=%s", log_t::flag_t::CRITICAL, eid, description.c_str());
						break;
					}
				});
				// Устанавливаем функцию обратного вызова на удачное подключение к серверу
				io.on(eid, static_cast <engine::callback::connect_t> ([&sendMessage, &io, &log](const event::id_t eid, const bool ok) noexcept -> void {
					// Записываем в лог сообщение о принятии события
					// log.print("Событие подключения: ID=%u, результат: %s", log_t::flag_t::INFO, eid, ok ? "YES" : "NO");
					cout << " Событие подключения: ID=" << eid << ", результат: " << (ok ? "YES" : "NO") << endl;
					// Если подключение успешно
					if(ok)
						// Выполняем отправку данных клиенту
						sendMessage(eid);
				}));
				// Устанавливаем функцию обратного вызова на общее событие
				io.on(eid, [&log](const event::id_t eid, const event::action_t action) noexcept -> void {
					/**
					 * Обрабатываем действие события
					 */
					switch(static_cast <uint8_t> (action)){
						// Если действие является чтением
						case static_cast <uint8_t> (event::action_t::READ):
							// Записываем в лог сообщение о чтении события
							log.print("Событие на чтение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является записью
						case static_cast <uint8_t> (event::action_t::WRITE):
							// Записываем в лог сообщение о записи события
							log.print("Событие на запись: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является подключением
						case static_cast <uint8_t> (event::action_t::CONNECT):
							// Записываем в лог сообщение о подключении события
							log.print("Событие на подключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отключением
						case static_cast <uint8_t> (event::action_t::DISCONNECT):
							// Записываем в лог сообщение об отключении события
							log.print("Событие на отключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переподключением
						case static_cast <uint8_t> (event::action_t::RECONNECT):
							// Записываем в лог сообщение о переподключении события
							log.print("Событие на переподключение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является закрытием
						case static_cast <uint8_t> (event::action_t::CLOSE):
							// Записываем в лог сообщение о закрытии события
							log.print("Событие на закрытие подключения: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением
						case static_cast <uint8_t> (event::action_t::CHANGE):
							// Записываем в лог сообщение об изменении события
							log.print("Событие на изменение: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является удалением
						case static_cast <uint8_t> (event::action_t::DELETE):
							// Записываем в лог сообщение об удалении события
							log.print("Событие на удаление: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является переименованием
						case static_cast <uint8_t> (event::action_t::RENAME):
							// Записываем в лог сообщение о переименовании события
							log.print("Событие на переименование: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением атрибутов
						case static_cast <uint8_t> (event::action_t::ATTRIB):
							// Записываем в лог сообщение об изменении атрибутов события
							log.print("Событие на изменение атрибутов: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является отзывом доступа
						case static_cast <uint8_t> (event::action_t::REVOKE):
							// Записываем в лог сообщение об отзыве доступа события
							log.print("Событие на отзыв доступа: ID=%u", log_t::flag_t::INFO, eid);
						break;
						// Если действие является изменением счётчика жёстких ссылок
						case static_cast <uint8_t> (event::action_t::HDLINK):
							// Записываем в лог сообщение о изменении счётчика жёстких ссылок события
							log.print("Событие на изменение счётчика жёстких ссылок: ID=%u", log_t::flag_t::INFO, eid);
						break;
					}
				});
				// Устанавливаем таймаут события на чтение
				io.setTimeout(eid, event::action_t::READ, 5000);
				// Устанавливаем таймаут события на запись
				io.setTimeout(eid, event::action_t::WRITE, 5000);
				// Устанавливаем таймаут события на подключение
				io.setTimeout(eid, event::action_t::CONNECT, 5000);
				// Выполняем фиксацию настроек события сервера
				if(io.commit(eid)){
					// Если подключение к серверу прошло успешно
					if(io.connect(eid, true)){
						// Выполняем запуск события
						if(io.launch(eid)){
							// Записываем в лог сообщение об успешном запуске события
							cout << " Событие успешно запущено!" << endl;
							/**
							 * Запускаем опрос событий
							 */
							while(io.poll());
						// Записываем ошибку в лог запуска события
						} else cout << " Ошибка запуска события!" << endl;
					}
				}
			// Если адрес назначения не установлен
			} else cout << " Ошибка установки адреса сервера!" << endl;
		// Если адрес не установлен
		} else cout << " Ошибка установки адреса клиента!" << endl;
	}
	// Возвращаем результат
	return 0;
}
