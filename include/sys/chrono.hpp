/**
 * @file chrono.hpp
 * @date 2025-10-25
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
 * @brief Заголовочный файл модуля работы с датой и временем —
 *        класс Chrono для разбора и форматирования дат в различных форматах, конвертации единиц времени,
 *        работы с временными зонами, переходами на летнее время и штампами времени высокого разрешения
 *
 * @details Модуль вырос из задачи разбора записей системного журнала и потому
 *          устроен вокруг двух её особенностей. Первая - устаревший стандарт
 *          RFC 3164 года в штампе не записывает, отчего запись без года требует
 *          подстановки недостающих полей, а не отказа. Вторая - записи приходят в
 *          произвольных форматах разной полноты, и разбор ведётся по образцу, а не
 *          по жёсткой грамматике. Позже набор форматов расширился до общепринятых:
 *          RFC 5424 и RFC 3339 (штампы протоколов), RFC 5322 (заголовок Date
 *          электронной почты), ISO 8601, журнал веб-сервера в общем формате и
 *          переменные strftime стандарта POSIX.
 *
 *          Штамп времени модуля - количество миллисекунд, прошедших с 1 января
 *          1970 года. Поддерживаемый диапазон дат - с 1970 по 9999 год.
 *
 * @note Модуль не обращается к базе временных зон операционной системы. Известные
 *       обозначения зон разбираются по неизменяемой таблице внутри самого модуля,
 *       а метод addTimeZone заводит отдельный реестр своих обозначений, изначально
 *       пустой; очистка реестра методом clearTimeZones встроенных обозначений не
 *       затрагивает. Смещение зоны окружения берётся через tzset, поэтому переходы
 *       на летнее время сторонних зон модуль сам не отслеживает
 *
 * @section chrono_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Даты до 1970 года не поддерживаются.</b> Штамп времени беззнаковый, и
 *          нижний предел представимости - само начало эпохи. Так же поступает
 *          большинство сред исполнения. Отсчёт вглубь от 1970 года - отдельная
 *          задача, к календарю этого модуля отношения не имеющая.
 *
 *          <b>Нулевой штамп времени - законная дата.</b> Это 1 января 1970 года,
 *          а не признак отсутствия даты: разложение, сборка и календарная
 *          арифметика обязаны обрабатывать его наравне с прочими.
 *
 *          <b>Формирование записи по штампу времени ведётся во временной зоне
 *          окружения</b>, а не в нулевой. Журналы и отчётность живут по времени
 *          того сервера, на котором работают, а его зону вызывающая сторона заранее
 *          обычно не знает. Нулевая зона запрашивается явно - перегрузкой со
 *          смещением либо переменной формата \%i.
 *
 *          <b>Переменные \%U и \%W при разборе дают одинаковый результат.</b> Поле
 *          записи они вычитывают, но дату по нему не восстанавливают - так номер
 *          недели трактует strptime стандарта POSIX. При формировании записи они,
 *          как и положено, отсчитывают неделю от разных дней.
 *
 *          <b>Двузначный год раскрывается по выбранному правилу, а не по одному
 *          на все случаи.</b> Единого правила стандарты не дают: RFC 9110 задаёт
 *          скользящее окно от текущего года, POSIX для strptime - неподвижный рубеж
 *          между 68 и 69 годами. Оба действуют, оба применяются в живых записях, и
 *          выбор оставлен вызывающему методом century. По умолчанию действует
 *          скользящее окно RFC 9110 шириной в пятьдесят лет - им записан устаревший
 *          формат RFC 850, ради которого правило и заведено. Рубеж POSIX выражает
 *          годы с 1970 по 1999, до которых скользящее окно не достаёт ни при какой
 *          своей величине, но взамен теряет те же годы двадцать первого века.
 *
 *          <b>Единица unit_t::WEEKS округляется до ближайшего целого</b>, а не
 *          отбрасывает неполную неделю, и от номера недели в году (\%U, \%W)
 *          отличается. Это разные величины, а не разные способы счёта одной.
 *
 *          <b>Смещение методом offset на микросекунды и наносекунды меняет
 *          размерность ответа.</b> Довод принимается миллисекундным штампом, а тот
 *          доли миллисекунды не представляет: иначе такое смещение было бы просто
 *          бездействием.
 *
 *          <b>Штамп времени в наносекундах охватывает лишь часть промежутка.</b>
 *          Наибольший такой штамп, умещающийся в разрядность uint64_t, приходится
 *          на 21 июля 2554 года, тогда как календарь модуля доходит до 9999-го.
 *          Дата за этой границей выдаётся наибольшим представимым значением, а не
 *          обёрнутым: перевод в микросекунды подобной границы не имеет и покрывает
 *          весь промежуток.
 *
 *          <b>Метод strip не является константным.</b> С хранилищем storage_t::LOCAL
 *          разбор пишет в объект, ровно как это делает parse. Обход этого через
 *          const_cast был неопределённым поведением и снят намеренно.
 *
 *          <b>Метод digits внутри модуля не используется.</b> Он остаётся частью
 *          открытого набора инструментов: AWH - фреймворк, и состав его API
 *          определяется не числом обращений внутри самого репозитория.
 *
 *          <b>Разбор ищет поля записи, а не сверяет её посимвольно.</b> Символы
 *          формата, переменными не являющиеся, положение не задают: разделители не
 *          проверяются, а поле ищется в остатке записи. Это намеренно - модуль служит
 *          разбору журналов, где вокруг даты стоит посторонний текст, и запись вида
 *          "хост app: 06/Apr/2025" читается тем же форматом, что и одна голая дата.
 *          Следствие: validate подтверждает, что все поля формата в записи нашлись, а
 *          не что запись формату равна. Названия месяцев и дней недели при этом
 *          сверяются со словарём, поэтому посторонним словом поле не заполнится.
 *
 *          <b>Разбор числовые поля вне промежутка не отвергает, а приводит к
 *          календарю.</b> Тринадцатый месяц становится январём следующего года,
 *          тридцать второе число - первым числом следующего месяца, шестьдесят первая
 *          секунда - первой секундой следующей минуты. Судит о годности записи
 *          validate, и он такие поля отвергает: разделение обязанностей здесь то же,
 *          что и у разделителей записи. Установка единицы данных методом set устроена
 *          иначе - там поле вне промежутка отвергается на месте, поскольку задаётся
 *          оно по одному, а не всей записью сразу, и объект при отказе остаётся
 *          прежним: отвергается и значение, в разрядность поля не умещающееся, и
 *          смещение временной зоны, числом не являющееся. Единственное исключение -
 *          шестидесятая секунда: стандарт POSIX объявляет промежуток секунд от 0 до 60,
 *          и обе части договора переносят её на следующую минуту одинаково. Номер дня
 *          недели, переменными \%u и \%w заданный, разбор не принимает вовсе - поле
 *          выводное и считается от самой даты, - но проверка записи его промежуток
 *          смотрит наравне с прочими, и промежуток этот у каждой переменной свой:
 *          \%u ведёт счёт от одного до семи, \%w - от нуля до шести. Самому же
 *          календарю день недели не сверяется: запись "Mon, 06 Apr 2025" проверку
 *          проходит, хотя шестое апреля 2025 года - воскресенье. Так поступает и
 *          strptime стандарта POSIX: поле выводное, дату оно не задаёт, и разбор
 *          выводит его заново. Сверка названия с датой - это строгий разбор, а его
 *          модуль не ведёт: он служит журналам, где вокруг даты стоит посторонний текст.
 *
 *          <b>Смещение зоны, извлечённое из штампа времени, всегда нулевое.</b> Штамп
 *          отсчитывается от начала эпохи в нулевой зоне и обозначения зоны не несёт,
 *          поэтому unit_t::OFFSET по штампу даёт нуль, а не смещение окружения. Зону
 *          несёт объект: тот же запрос с хранилищем storage_t::LOCAL выдаёт смещение,
 *          выставленное объекту.
 *
 *          <b>Согласования доступа к полям объекта модуль не выполняет.</b> Ни
 *          мютекса, ни метода его включения у модуля нет. Замок во фреймворке
 *          заводится только там, где реализация держит общие данные в самом .cpp -
 *          статические, до которых пользующемуся не дотянуться. Таких данных здесь
 *          не осталось: справочные таблицы заполняются однажды при статической
 *          инициализации и дальше только читаются, таблица обозначений зон
 *          неизменяема, а смещение зоны окружения запоминается в памяти потока.
 *          Всё прочее - поля объекта, принадлежащие тому, кто объектом пользуется,
 *          и согласование доступа к ним отдано ему.
 *
 *          Довод не в одной лишь принадлежности. Замок на вызов не делает
 *          неделимой последовательность вызовов: проверка значения с последующей
 *          его установкой остаётся состязанием и под ним, поэтому делящему объект
 *          между потоками внешнее согласование нужно всё равно - а тогда
 *          внутреннее лишь вводит в заблуждение. Без изменяемых полей константные
 *          методы согласуются между собой не по обещанию, а по устройству: писать
 *          им попросту некуда.
 *
 *          <b>Летнее время считается по правилам США и Канады с 2007 года.</b>
 *          Переход весной назначен на второе воскресенье марта в 02:00 стандартного
 *          времени зоны, осенью - на первое воскресенье ноября в 02:00 летнего, то
 *          есть в 01:00 стандартного. Сводные зоны Северной Америки - zone_t::AT,
 *          zone_t::CT, zone_t::ET, zone_t::MT, zone_t::NT и zone_t::PT - выбирают
 *          стандартное либо летнее время по самому записываемому моменту, а не по
 *          текущему: смещение таких зон разрешается по описываемой дате всюду, где
 *          она задаётся либо меняется - разбором записи, установкой любой единицы
 *          данных, перекладкой в зону и формированием записи в указанную зону.
 *          Внутри самого часа перехода местное время либо
 *          повторяется, либо не существует вовсе, и однозначного разрешения такая
 *          запись не имеет: опорой разрешения служит стандартное время зоны, отчего
 *          ответ там хоть и условен, но от прежнего состояния объекта не зависит и
 *          всегда один и тот же. Признак летнего времени сводной зоны выводится из
 *          её разрешённого смещения, а не из полей объекта. Ньюфаундленд до 2012 года переходил в 00:01 местного времени, и
 *          на переходах 2007-2011 годов zone_t::NT расходится с базой временных зон
 *          системы намеренно. Правила прочих стран модуль не отслеживает, и поле dst
 *          объекта даты для зон вне Северной Америки - величина справочная, а не
 *          свойство зоны. Границы периода и календарное смещение местного хранилища
 *          отсчитываются по смещению, объекту выставленному, а в самих сутках перехода
 *          их двадцать три либо двадцать пять часов: постоянное смещение даёт там
 *          приближение, и у зон с постоянным смещением вроде zone_t::MSK этой разницы
 *          нет вовсе, а у сводных она равна часу и лишь в эти двое суток года. Признак летнего времени местного хранилища берётся у самого
 *          объекта и потому судит по его временной зоне, а признак, запрошенный по
 *          штампу времени, раскладывает его в нулевой зоне и остаётся справочным.
 *
 *          <b>Перегрузки getTimeZone и format, даты не принимающие, отвечают по
 *          текущему моменту.</b> Это их назначение, а не упущение: вопрос «какое
 *          смещение и какое обозначение у зоны сейчас» задаётся не реже, чем вопрос о
 *          смещении на заданную дату, и отвечать на него обязан кто-то один. Для
 *          второго вопроса заведены перегрузки, дату принимающие, и все внутренние
 *          пути разбора и формирования записей идут через них: обозначение сводной
 *          зоны разрешается по самой записи, а не по мгновению, в которое она
 *          разбирается. Разница видна лишь у сводных зон Северной Америки - смещение
 *          прочих от момента не зависит вовсе. Сводить эти перегрузки к одной значит
 *          лишать вызывающую сторону первого вопроса, и делаться этого не будет.
 *
 *          <b>Смещение временной зоны приводится к земным пределам.</b> Зоны лежат в
 *          промежутке от UTC-12 до UTC+14, и смещение за ним приводится к ближайшему
 *          из них - как при выставлении зоны числом, так и при установке единицей
 *          данных. Прежде принималось любое значение разрядности, тогда как проверка
 *          записи такие смещения отвергала.
 *
 *          <b>Нулевое смещение, заданное явно, обозначается UTC.</b> Обозначение
 *          здесь единственное, чем нулевая зона, заданная записью либо методом
 *          setTimeZone, отличается от незаданной, а от этого зависит, перекладывать
 *          ли запись в зону окружения при формировании с хранилищем storage_t::LOCAL.
 *          Незаданная зона обозначения не имеет и берётся из окружения.
 *
 *          <b>Установка единицы данных сохраняет момент времени либо задаёт дату.</b>
 *          Смена смещения временной зоны перекладывает запись в новую зону, сохраняя
 *          сам момент, ровно как это делает setTimeZone. Номер дня в году задаёт дату
 *          целиком, номер недели переносит её на указанную неделю, а день недели - на
 *          этот день той же недели; время суток при переносе сохраняется.
 *
 *          <b>Установка задающего поля согласует выводные.</b> Год, месяц и число
 *          месяца задают дату целиком, а номер дня в году, номер недели и день недели
 *          из них выводятся и пересчитываются при каждой установке: get и format над
 *          одним объектом отвечают одинаково. Метка времени суток и признак летнего
 *          времени выводятся так же и согласуются при установке любой единицы, а не
 *          одних лишь календарных: час, минуты и секунды разложения штампа не
 *          вызывают, и признаки оставались от прежней даты. Значения, выходящие за
 *          пределы, при этом приводятся к ним - час, минуты, секунды и доли секунды
 *          за своим промежутком отвергаются, а шестидесятая секунда принимается и
 *          переносится на следующую минуту наравне с разбором записи: стандарт POSIX
 *          объявляет промежуток секунд от 0 до 60, високосной секунды модуль не
 *          моделирует, и обе части договора обходятся с ней одинаково. Прочие
 *          значения приводятся к своим пределам - год к промежутку с 1970 по 9999,
 *          число месяца к его длине, номер дня в году к длине самого года.
 *
 *          <b>Конец периода равен началу следующего, а конец последнего периода
 *          календаря - последнему представимому мгновению.</b> Метод end выдаёт правую
 *          границу полуинтервала [begin, end), то есть само начало следующего периода:
 *          end суток нулевого дня эпохи равен 86400000, а не 86399999. Для 9999 года
 *          начала следующего периода не существует, а MAX_TIMESTAMP + 1 в разрядности
 *          uint64_t от самого календаря неотличим, поэтому end на краю насыщается
 *          последним представимым мгновением, и полуинтервал [begin, end) последнюю
 *          миллисекунду 9999 года уже не включает. Для края следует пользоваться
 *          замкнутым промежутком [begin, end].
 *
 *          <b>Год и месяц - единицы календарные.</b> Смещение по годам меняет
 *          обозначение года, а не отсчитывает 365 либо 366 суток: високосные годы
 *          внутри промежутка учитываются пересборкой штампа времени по календарю.
 *          Прибавление года равнозначно прибавлению двенадцати месяцев, включая
 *          29 февраля, которого в целевом году может не быть: число приводится к
 *          настоящей длине месяца, и 2024-02-29 плюс год даёт 2025-02-28.
 *
 *          <b>Установка месяца и года приводит число к длине месяца.</b> Смена месяца
 *          либо года оставляла в объекте число, в новом месяце отсутствующее, и
 *          сборка штампа времени переносила дату вперёд: 31 января со сменой месяца
 *          на второй давало 3 марта. Теперь даёт 28 февраля, а в високосном году -
 *          29-е. Разбор записи переносом даты по-прежнему пользуется намеренно:
 *          запись сообщает то, что в ней написано, а установка поля - правку
 *          состояния объекта.
 *
 *          <b>Шаблоны get и set приводят значение к запрошенному типу по правилам
 *          языка.</b> Единица данных хранится целым числом, и запрос её типом уже
 *          разрядности даёт усечение, а не отказ: get<uint8_t> для года 2025 вернёт
 *          233. Установка вещественным числом отбрасывает дробную часть. Прежде
 *          движок работал с октетами поля напрямую, отчего вещественные типы не
 *          работали вовсе, а узкие молча давали ноль.
 *
 *          <b>Единица DAYS отсчитывается от нуля, а переменная формата %j - от
 *          единицы.</b> DAYS - это количество суток, с начала года прошедших, и
 *          первого января оно равно нулю. Переменная %j задана стандартом как
 *          номер дня в году и первого января равна единице, поэтому %j всегда на
 *          единицу больше DAYS. Приводить их к общему отсчёту нельзя: %j обязан
 *          отвечать стандарту, а DAYS - оставаться количеством, пригодным для
 *          сложения с прочими единицами. Сходимость закреплена тестом.
 *
 *          <b>Доли миллисекунды живут в объекте, а не в штампе времени.</b> Штамп
 *          времени задан миллисекундами, и микросекунд с наносекундами в нём нет:
 *          извлечение этих единиц по штампу выдаёт ноль. Доли задают лишь
 *          установка штампа в микросекундах либо наносекундах и обновление
 *          текущего момента, и читаются они у объекта. Установка штампа в
 *          единицах крупнее микросекунды доли обнуляет, а установка отдельного
 *          поля - нет: set меняет только запрошенное поле, и миллисекунды долей
 *          миллисекунды не касаются наравне с тем, как секунды не касаются
 *          миллисекунд.
 *
 *          <b>Прошедшее и оставшееся в сумме дают на единицу меньше длины
 *          периода.</b> Неполная единица в счёт не идёт ни с одной стороны, и
 *          единица, на которую приходится сама дата, не прошла и не осталась:
 *          для суток года это 95 прошедших и 269 оставшихся при длине 365. Ровно
 *          на границе единицы неполной нет вовсе, и сумма сходится с длиной.
 *          Правило это общее для года, месяца, недели и суток, а для миллисекунды
 *          сумма сходится всегда, поскольку неполных миллисекунд не бывает.
 *          Месяцы длительности неравной, но считаются они тем же правилом: на
 *          начало января целыми остаются все двенадцать, на начало февраля один
 *          прошёл и одиннадцать осталось, а внутри месяца текущий не идёт в счёт
 *          ни туда, ни сюда.
 *
 *          <b>Номер дня в году принимается по длине года високосного.</b> Установка
 *          поля DAYS принимает значение до 365 включительно при любом годе, и в
 *          году обычном такой номер приводится раскладкой к последним его суткам
 *          наравне с прочими переполнениями полей. Проверка записи строже: она
 *          сверяет номер с длиной года разобранного и запись «2025 366» отвергает.
 *          Разделение это то же, что у разбора и проверки записи вообще.
 *
 *          <b>Месяц и число месяца имеют преимущество над номером дня в году.</b>
 *          Переменные формата %m и %d задают дату напрямую, а %j - через раскладку
 *          номера дня, и при совместном задании раскладка не выполняется. Сличения
 *          их между собой нет: запись, где номер дня и число месяца друг другу
 *          противоречат, считается пригодной, коль скоро пригодно каждое поле по
 *          отдельности. Так же поступает и strptime.
 *
 *          <b>Номер недели даётся тремя разными счётами, и они не совпадают.</b>
 *          Поле WEEKS - это количество недель, прошедших с начала года с
 *          округлением к ближайшей. Переменные формата %U и %W - номера недель по
 *          стандарту, отсчитываемые от первого воскресенья и первого понедельника
 *          соответственно. Счёт actual для недели в году даёт целые прошедшие
 *          недели. Четвёртого января 2025 года это 1, 00, 00 и 0. Сводить их к
 *          одному нельзя: %U и %W обязаны отвечать стандарту, actual - общему
 *          правилу целых единиц, а поле WEEKS отвечает округлённой доле года.
 *
 *          <b>Недельный счёт ISO 8601 ведётся отдельно от календарного.</b>
 *          Переменные формата %G, %g и %V отвечают году и номеру недели по ISO 8601:
 *          недели начинаются с понедельника, а первой считается та, на которую
 *          приходится первый четверг года. Оттого последние дни декабря могут
 *          принадлежать первой неделе следующего года - тридцатое декабря 2024 года
 *          записывается как 2025-W01. Разбор эти переменные читает и пропускает:
 *          календарную дату они задают лишь все вместе, а порознь ни одного поля не
 *          определяют, и так же поступает strptime. Дату разбор берёт у переменных
 *          календарных.
 *
 *          <b>Переменная %x записывает год двумя разрядами.</b> Стандарт POSIX задаёт
 *          для локали C запись %m/%d/%y, ей отвечают и %x, и %D. Отдельные среды
 *          исполнения - macOS в их числе - записывают %x годом полным, но это выбор
 *          их локали, а не стандарт.
 *
 *          <b>Штамп времени в годах и месяцах опирается на среднюю длительность.</b>
 *          Единицы YEAR и MONTH при чтении и установке штампа времени считаются
 *          постоянными - 365 суток и 30.44 суток соответственно, - а не календарём.
 *          Календарный сдвиг даёт offset, который меняет сам номер года либо месяца
 *          и приводит число к длине месяца.
 *
 *          <b>Образец формирования записи стандарта и образец её разбора совпадают
 *          не всегда.</b> Стандарты HTTP обозначают нулевую зону словом GMT, и в
 *          образце формирования оно записано литералом, поскольку переменная \%Z для
 *          нулевого смещения даёт UTC. Разбор литералы не читает вовсе, и такая
 *          запись оставалась без зоны - время по Гринвичу принималось за местное и
 *          уезжало на смещение зоны окружения. Разбору поэтому даётся отдельный
 *          образец с переменной зоны, которая слово GMT читает.
 *
 *          <b>Формы записи ISO 8601 не смешиваются.</b> Основная форма даты и времени
 *          разделителей не несёт, и смещение зоны при ней записывается четырьмя
 *          разрядами без двоеточия: расширенное «+03:00» рядом с «20250406T153701»
 *          стандарту не отвечает. Нулевое смещение обозначается буквой Z в обеих
 *          формах. Расширенную форму целиком даёт RFC 3339, её профиль.
 *
 *          <b>Смещение записи приводится к земным пределам при его применении, а не
 *          при разборе поля.</b> Разбор снисходителен и запись со смещением, которого
 *          на Земле нет, принимает, но применяет его приведённым к ближайшему пределу.
 *          Приведение в самом разборе поля отняло бы у проверки записи возможность
 *          такое смещение отвергнуть: она судит по разобранным полям, а не по итогу.
 *
 *          <b>Обозначение продолжительности занимает запись целиком.</b> Число
 *          обязано стоять в её начале, единица размерности - в её конце, и состоят
 *          они в единственном числе: составные записи вида «1h30m» не предусмотрены.
 *          Прежде разбор искал число с любой позиции записи, и составная читалась
 *          одним лишь своим хвостом - «1h» отбрасывалось молча, а проверка такую
 *          запись одобряла. Запись, прочитанная наполовину, хуже отвергнутой: она
 *          даёт величину, о подмене которой вызывающая сторона не узнаёт.
 *
 * \~english
 * @brief Header file of the date and time module —
 *        the Chrono class for parsing and formatting dates in various formats, converting time units,
 *        working with time zones, daylight saving time transitions and high resolution timestamps
 * @details The module grew out of the task of parsing system log records and is therefore
 *          built around two of its peculiarities. The first one — the obsolete RFC 3164
 *          standard does not write the year into the timestamp, and therefore a record without
 *          a year requires the missing fields to be substituted rather than the record to be rejected.
 *          The second one — the records come in arbitrary formats of different completeness, and the
 *          parsing is driven by a pattern rather than by a strict grammar. Later the set of formats
 *          was extended to the commonly accepted ones: RFC 5424 and RFC 3339 (protocol timestamps),
 *          RFC 5322 (the Date header of the electronic mail), ISO 8601, the web server log in the
 *          common format and the strftime variables of the POSIX standard.
 *          The timestamp of the module is the number of milliseconds elapsed since the 1st of January
 *          1970. The supported range of the dates is from the year 1970 to the year 9999.
 * @note The module does not address the time zone database of the operating system. The known
 *       designations of the zones are parsed by an immutable table inside the module itself,
 *       and the addTimeZone method starts a separate registry of one's own designations, initially
 *       empty; clearing that registry by the clearTimeZones method does not touch the built-in
 *       designations. The offset of the environment zone is taken through tzset, and therefore the
 *       module does not track the daylight saving time transitions of foreign zones by itself
 * @section chrono_decisions Deliberate decisions
 * @details What is listed below looks like an inconsistency, but it is chosen deliberately and
 *          is not subject to correction. The section is started so that reading the code would not
 *          begin every time with the very same conclusions.
 *          <b>Dates before the year 1970 are not supported.</b> The timestamp is unsigned, and
 *          the lower limit of the representability is the very beginning of the epoch. Most of the
 *          runtime environments act the same way. Counting backwards from the year 1970 is a separate
 *          task, having no relation to the calendar of this module.
 *          <b>A zero timestamp is a legal date.</b> It is the 1st of January 1970,
 *          and not a sign of the absence of a date: the decomposition, the assembly and the calendar
 *          arithmetic are obliged to handle it on a par with the rest.
 *          <b>Building a record by a timestamp is performed in the time zone
 *          of the environment</b>, and not in the zero one. Logs and reporting live by the time
 *          of the very server they work on, and the calling side usually does not know its zone
 *          in advance. The zero zone is requested explicitly — by the overload taking an
 *          offset or by the \%i format variable.
 *          <b>The \%U and \%W variables give the same result on parsing.</b> They do read
 *          the field of the record, but do not restore the date from it — that is how the number
 *          of the week is treated by strptime of the POSIX standard. On building a record they,
 *          as they should, count the week from different days.
 *          <b>A two-digit year is expanded by the chosen rule, and not by a single one
 *          for all the cases.</b> The standards give no single rule: RFC 9110 sets a
 *          sliding window from the current year, POSIX for strptime — an immovable boundary
 *          between the years 68 and 69. Both are in force, both are applied in living records, and
 *          the choice is left to the caller by the century method. By default the
 *          sliding window of RFC 9110 fifty years wide is in force — the obsolete format RFC 850
 *          is written by it, and the rule is started for its sake. The POSIX boundary expresses
 *          the years from 1970 to 1999, which the sliding window does not reach at any of its
 *          widths, but in exchange loses the very same years of the twenty first century.
 *          <b>The unit_t::WEEKS unit is rounded to the nearest whole</b>, and does not
 *          discard an incomplete week, and it differs from the number of the week in the year (\%U, \%W).
 *          These are different quantities, and not different ways of counting one and the same.
 *          <b>An offset by the offset method in microseconds and nanoseconds changes
 *          the dimension of the answer.</b> The argument is taken as a millisecond timestamp, and that one
 *          does not represent fractions of a millisecond: otherwise such an offset would simply be
 *          an inaction.
 *          <b>A timestamp in nanoseconds covers only a part of the range.</b>
 *          The largest such timestamp fitting into the width of uint64_t falls
 *          on the 21st of July 2554, while the calendar of the module reaches the year 9999.
 *          A date beyond that boundary is yielded as the largest representable value, and not as a
 *          wrapped one: the conversion into microseconds has no such boundary and covers
 *          the whole range.
 *          <b>The strip method is not a constant one.</b> With the storage_t::LOCAL storage
 *          the parsing writes into the object, exactly as parse does. Going around this through
 *          const_cast was an undefined behaviour and is removed deliberately.
 *          <b>The digits method is not used inside the module.</b> It remains a part
 *          of the public set of the tools: AWH is a framework, and the composition of its API
 *          is defined not by the number of the calls inside the repository itself.
 *          <b>The parsing searches for the fields of the record, and does not check it character
 *          by character.</b> The characters of the format that are not variables do not set the
 *          position: the separators are not checked, and a field is searched for in the remainder of the record.
 *          This is deliberate — the module serves the parsing of logs, where extraneous text stands
 *          around the date, and a record of the form "host app: 06/Apr/2025" is read by the very same
 *          format as a single bare date. The consequence: validate confirms that all the fields of the format
 *          have been found in the record, and not that the record is equal to the format. The names of the months
 *          and of the days of the week are at that checked against a dictionary, and therefore a field will not be
 *          filled by an extraneous word.
 *          <b>The parsing does not reject numeric fields outside their range, but brings them to the
 *          calendar.</b> The thirteenth month becomes January of the next year,
 *          the thirty second day — the first day of the next month, the sixty first
 *          second — the first second of the next minute. The fitness of the record is judged by
 *          validate, and it does reject such fields: the separation of the duties here is the same
 *          as with the separators of the record. Setting a data unit by the set method is arranged
 *          otherwise — there a field outside its range is rejected on the spot, since it is set
 *          one at a time, and not by the whole record at once, and on a refusal the object remains
 *          the previous one: both a value not fitting into the width of the field and an offset
 *          of the time zone that is not a number are rejected. The single exception is
 *          the sixtieth second: the POSIX standard declares the range of the seconds from 0 to 60,
 *          and both parts of the contract move it to the next minute in the same way. The number of the day
 *          of the week, set by the \%u and \%w variables, is not accepted by the parsing at all — the field
 *          is a derived one and is counted from the date itself, — but the check of the record looks at
 *          its range on a par with the rest, and that range is its own for each variable:
 *          \%u counts from one to seven, \%w — from zero to six. Against the calendar itself
 *          the day of the week is not checked: the record "Mon, 06 Apr 2025" passes the check,
 *          although the sixth of April 2025 is a Sunday. So does strptime of the POSIX standard: the field
 *          is a derived one, it does not set the date, and the parsing derives it anew.
 *          Checking the name against the date is strict parsing, and the module does not perform it:
 *          it serves the logs, where extraneous text stands around the date.
 *          <b>The offset of the zone, extracted from a timestamp, is always zero.</b> The timestamp
 *          is counted from the beginning of the epoch in the zero zone and carries no designation of a zone,
 *          and therefore unit_t::OFFSET by a timestamp gives zero, and not the offset of the environment. The zone
 *          is carried by the object: the same request with the storage_t::LOCAL storage yields the offset
 *          set to the object.
 *          <b>The module does not perform any synchronization of the access to the fields of the object.</b> Neither
 *          a mutex nor a method of switching it on is present in the module. A lock in the framework
 *          is started only there, where the implementation holds shared data in the .cpp itself —
 *          static data, which the user cannot reach. Such data has not remained here:
 *          the reference tables are filled once at the static
 *          initialization and further on are only read, the table of the designations of the zones
 *          is immutable, and the offset of the environment zone is remembered in the memory of the thread.
 *          Everything else consists of the fields of the object, belonging to the one who uses the object,
 *          and the synchronization of the access to them is given over to them.
 *          The argument is not in the ownership alone. A lock on a call does not make
 *          a sequence of calls atomic: a check of a value with the subsequent setting of it
 *          remains a race even under it, and therefore the one sharing the object
 *          between threads needs an external synchronization anyway — and then
 *          the internal one only misleads. Without mutable fields the constant
 *          methods agree with each other not by a promise, but by construction: they simply
 *          have nowhere to write.
 *          <b>The daylight saving time is counted by the rules of the USA and Canada since the year 2007.</b>
 *          The transition in the spring is appointed to the second Sunday of March at 02:00 of the standard
 *          time of the zone, in the autumn — to the first Sunday of November at 02:00 of the daylight saving one, that
 *          is at 01:00 of the standard one. The composite zones of North America — zone_t::AT,
 *          zone_t::CT, zone_t::ET, zone_t::MT, zone_t::NT and zone_t::PT — choose
 *          the standard or the daylight saving time by the very moment being written, and not by
 *          the current one: the offset of such zones is resolved by the date being described everywhere it
 *          is set or changed — by the parsing of a record, by the setting of any data
 *          unit, by the shift into a zone and by the building of a record in the specified zone.
 *          Inside the very hour of the transition the local time either
 *          repeats itself or does not exist at all, and such a record has no unambiguous resolution:
 *          the support of the resolution is the standard time of the zone, and therefore the
 *          answer there, though conditional, does not depend on the previous state of the object and
 *          is always one and the same. The sign of the daylight saving time of a composite zone is derived from
 *          its resolved offset, and not from the fields of the object. Newfoundland until the year 2012 switched at 00:01 of the local time, and
 *          on the transitions of the years 2007-2011 zone_t::NT diverges from the time zone database
 *          of the system deliberately. The rules of the other countries the module does not track, and the dst field
 *          of the date object for the zones outside North America is a reference quantity, and not
 *          a property of the zone. The boundaries of a period and the calendar offset of the local storage
 *          are counted by the offset set to the object, and in the very days of a transition
 *          there are twenty three or twenty five hours of them: a constant offset gives an approximation
 *          there, and the zones with a constant offset like zone_t::MSK have no such difference
 *          at all, while for the composite ones it equals an hour and only in these two days of the year. The sign of the daylight saving time of the local storage is taken from the very
 *          object and therefore judges by its time zone, and the sign requested by
 *          a timestamp decomposes it in the zero zone and remains a reference one.
 *          <b>The getTimeZone and format overloads that do not take a date answer by
 *          the current moment.</b> This is their purpose, and not an omission: the question "what
 *          offset and what designation does the zone have now" is asked no less often than the question about
 *          the offset on a given date, and someone is obliged to answer it. For
 *          the second question the overloads taking a date are started, and all the internal
 *          paths of the parsing and of the building of records go through them: the designation of a composite
 *          zone is resolved by the record itself, and not by the instant it is
 *          parsed at. The difference is seen only in the composite zones of North America — the offset
 *          of the rest does not depend on the moment at all. To reduce these overloads to one means
 *          depriving the calling side of the first question, and that will not be done.
 *          <b>The offset of a time zone is brought to the earthly limits.</b> The zones lie in
 *          the range from UTC-12 to UTC+14, and an offset beyond it is brought to the nearest
 *          of them — both when setting a zone by a number and when setting it by a data
 *          unit. Formerly any value of the width was accepted, while the check
 *          of a record rejected such offsets.
 *          <b>A zero offset, set explicitly, is designated as UTC.</b> The designation
 *          is here the only thing by which the zero zone, set by a record or by the
 *          setTimeZone method, differs from an unset one, and it depends on that whether the record
 *          should be shifted into the environment zone when building with the storage_t::LOCAL storage.
 *          An unset zone has no designation and is taken from the environment.
 *          <b>Setting a data unit either preserves the moment of time or sets the date.</b>
 *          Changing the offset of the time zone shifts the record into the new zone, preserving
 *          the moment itself, exactly as setTimeZone does. The number of the day in the year sets the date
 *          entirely, the number of the week moves it to the specified week, and the day of the week — to
 *          that day of the same week; the time of the day is preserved on the move.
 *          <b>Setting a defining field agrees the derived ones.</b> The year, the month and the day of the
 *          month set the date entirely, and the number of the day in the year, the number of the week and the day of the week
 *          are derived from them and are recomputed on every setting: get and format over
 *          one object answer identically. The mark of the time of the day and the sign of the daylight saving
 *          time are derived the same way and are agreed on the setting of any unit, and not
 *          of the calendar ones alone: the hour, the minutes and the seconds do not cause a decomposition of the timestamp,
 *          and the signs remained from the previous date. The values going beyond
 *          the limits are at that brought to them — the hour, the minutes, the seconds and the fractions of a second
 *          outside their range are rejected, and the sixtieth second is accepted and
 *          moved to the next minute on a par with the parsing of a record: the POSIX standard
 *          declares the range of the seconds from 0 to 60, the module does not model the leap second,
 *          and both parts of the contract deal with it identically. The other
 *          values are brought to their limits — the year to the range from 1970 to 9999,
 *          the day of the month to its length, the number of the day in the year to the length of the year itself.
 *          <b>The end of a period equals the beginning of the next one, and the end of the last period
 *          of the calendar — the last representable instant.</b> The end method yields the right
 *          boundary of the half-interval [begin, end), that is the very beginning of the next period:
 *          the end of the day of the zeroth day of the epoch equals 86400000, and not 86399999. For the year 9999
 *          the beginning of the next period does not exist, and MAX_TIMESTAMP + 1 in the width of
 *          uint64_t is indistinguishable from the calendar itself, and therefore end at the edge saturates
 *          at the last representable instant, and the half-interval [begin, end) no longer includes the last
 *          millisecond of the year 9999. For the edge one should use
 *          the closed range [begin, end].
 *          <b>The year and the month are calendar units.</b> An offset by years changes
 *          the designation of the year, and does not count 365 or 366 days: the leap years
 *          inside the range are taken into account by the reassembly of the timestamp by the calendar.
 *          Adding a year is equivalent to adding twelve months, including
 *          the 29th of February, which may be absent in the target year: the day is brought to
 *          the real length of the month, and 2024-02-29 plus a year gives 2025-02-28.
 *          <b>Setting the month and the year brings the day to the length of the month.</b> Changing the month
 *          or the year left in the object a day absent in the new month, and
 *          the assembly of the timestamp moved the date forward: the 31st of January with the change of the month
 *          to the second one gave the 3rd of March. Now it gives the 28th of February, and in a leap year —
 *          the 29th. The parsing of a record still uses the moving of the date deliberately:
 *          a record reports what is written in it, and the setting of a field is a correction
 *          of the state of the object.
 *          <b>The get and set templates bring the value to the requested type by the rules
 *          of the language.</b> A data unit is stored as an integer, and requesting it by a type narrower than
 *          its width gives a truncation, and not a refusal: get<uint8_t> for the year 2025 will return
 *          233. Setting by a real number discards the fractional part. Formerly the engine
 *          worked with the octets of the field directly, and therefore the real types did not
 *          work at all, and the narrow ones silently gave zero.
 *          <b>The DAYS unit is counted from zero, and the %j format variable — from
 *          one.</b> DAYS is the number of the days elapsed since the beginning of the year, and
 *          on the first of January it equals zero. The %j variable is set by the standard as
 *          the number of the day in the year and on the first of January equals one, and therefore %j is always
 *          greater than DAYS by one. Bringing them to a common count is impossible: %j is obliged
 *          to answer the standard, and DAYS — to remain a quantity suitable for
 *          addition with the other units. The consistency is fixed by a test.
 *          <b>The fractions of a millisecond live in the object, and not in the timestamp.</b> The timestamp
 *          is set in milliseconds, and there are no microseconds and nanoseconds in it:
 *          extracting these units by a timestamp yields zero. The fractions are set only by
 *          the setting of a timestamp in microseconds or nanoseconds and by the update of
 *          the current moment, and they are read from the object. Setting a timestamp in
 *          units coarser than a microsecond zeroes the fractions, and the setting of a separate
 *          field does not: set changes only the requested field, and the milliseconds do not touch the fractions
 *          of a millisecond on a par with the way the seconds do not touch
 *          the milliseconds.
 *          <b>The elapsed and the remaining in the sum give one less than the length
 *          of the period.</b> An incomplete unit does not count from either side, and
 *          the unit the date itself falls on has neither elapsed nor remained:
 *          for the days of a year that is 95 elapsed and 269 remaining at the length of 365. Exactly
 *          on the boundary of a unit there is no incomplete one at all, and the sum agrees with the length.
 *          This rule is common for the year, the month, the week and the day, and for the millisecond
 *          the sum agrees always, since incomplete milliseconds do not happen.
 *          The months are of unequal length, but they are counted by the same rule: at
 *          the beginning of January all twelve remain whole, at the beginning of February one
 *          has elapsed and eleven have remained, and inside a month the current one counts
 *          neither there nor here.
 *          <b>The number of the day in the year is accepted by the length of a leap year.</b> Setting
 *          the DAYS field accepts a value up to 365 inclusive at any year, and in
 *          an ordinary year such a number is brought by the decomposition to its last day
 *          on a par with the other overflows of the fields. The check of a record is stricter: it
 *          checks the number against the length of the parsed year and rejects the record "2025 366".
 *          This separation is the same as with the parsing and the check of a record in general.
 *          <b>The month and the day of the month have priority over the number of the day in the year.</b> The
 *          %m and %d format variables set the date directly, and %j — through the decomposition
 *          of the number of the day, and when they are set together the decomposition is not performed. There is no matching
 *          of them against each other: a record where the number of the day and the day of the month contradict
 *          each other is considered fit, as long as each field is fit on
 *          its own. So does strptime.
 *          <b>The number of the week is given by three different counts, and they do not coincide.</b>
 *          The WEEKS field is the number of the weeks elapsed since the beginning of the year with
 *          rounding to the nearest. The %U and %W format variables are the numbers of the weeks by
 *          the standard, counted from the first Sunday and the first Monday
 *          respectively. The actual count for the week in the year gives the whole elapsed
 *          weeks. On the fourth of January 2025 these are 1, 00, 00 and 0. To reduce them to
 *          one is impossible: %U and %W are obliged to answer the standard, actual — to the common
 *          rule of whole units, and the WEEKS field answers for a rounded fraction of the year.
 *          <b>The ISO 8601 week count is kept separately from the calendar one.</b>
 *          The %G, %g and %V format variables answer for the year and the number of the week by ISO 8601:
 *          the weeks begin on Monday, and the first one is the one the first Thursday of the year
 *          falls on. Because of that the last days of December may belong to the first
 *          week of the next year — the thirtieth of December 2024 is
 *          written as 2025-W01. The parsing reads these variables and skips them:
 *          they set a calendar date only all together, and separately they define no field,
 *          and so does strptime. The date is taken by the parsing from the calendar
 *          variables.
 *          <b>The %x variable writes the year with two digits.</b> The POSIX standard sets
 *          for the C locale the record %m/%d/%y, and both %x and %D answer to it. Some runtime
 *          environments — macOS among them — write %x with the full year, but this is the choice
 *          of their locale, and not the standard.
 *          <b>A timestamp in years and months relies on the average length.</b>
 *          The YEAR and MONTH units when reading and setting a timestamp are considered
 *          constant — 365 days and 30.44 days respectively, — and not by the calendar.
 *          A calendar shift is given by offset, which changes the very number of the year or of the month
 *          and brings the day to the length of the month.
 *          <b>The pattern of building a record of a standard and the pattern of parsing it coincide
 *          not always.</b> The HTTP standards designate the zero zone by the word GMT, and in
 *          the pattern of the building it is written as a literal, since the \%Z variable for
 *          a zero offset gives UTC. The parsing does not read literals at all, and such
 *          a record remained without a zone — the Greenwich time was taken for the local one and
 *          drifted away by the offset of the environment zone. The parsing is therefore given a separate
 *          pattern with a variable of the zone, which does read the word GMT.
 *          <b>The forms of the ISO 8601 record are not mixed.</b> The basic form of the date and the time
 *          carries no separators, and the offset of the zone with it is written with four
 *          digits without a colon: the extended "+03:00" next to "20250406T153701"
 *          does not answer the standard. A zero offset is designated by the letter Z in both
 *          forms. The extended form entirely is given by RFC 3339, its profile.
 *          <b>The offset of a record is brought to the earthly limits at its application, and not
 *          at the parsing of the field.</b> The parsing is lenient and accepts a record with an offset that
 *          does not exist on Earth, but applies it brought to the nearest limit.
 *          Bringing it in the parsing of the field itself would take away from the check of the record the possibility
 *          to reject such an offset: it judges by the parsed fields, and not by the result.
 *          <b>A designation of a duration occupies the record entirely.</b> The number
 *          is obliged to stand at its beginning, the unit of the dimension — at its end, and they come
 *          in the singular: compound records of the form "1h30m" are not provided for.
 *          Formerly the parsing searched for a number from any position of the record, and a compound one was read
 *          by its tail alone — "1h" was discarded silently, and the check approved such
 *          a record. A record read halfway is worse than a rejected one: it
 *          gives a quantity whose substitution the calling side will not learn about.
 *
 * \~
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CHRONO__
#define __AWH_CHRONO__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "fmk.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * \~russian
	 * @brief Прототип класса работы с логами
	 *
	 * \~english
	 * @brief Prototype of the class for working with logs
	 *
	 * \~
	 */
	class Logging;

	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Структура модуля Chrono
	 *
	 * @details Класс объединяет четыре независимые задачи: разбор записи даты по
	 *          образцу формата (parse), формирование записи по тому же образцу
	 *          (format), календарную арифметику над штампом времени (begin, end,
	 *          offset, actual, get) и перевод обозначений временных зон в смещение
	 *          (matchTimeZone, getTimeZone). Все они работают с одним и тем же
	 *          штампом времени - количеством миллисекунд с 1 января 1970 года.
	 *
	 *          <b>Переменные формата.</b> Значения приведены для момента
	 *          1743943021520, то есть 2025-04-06T12:37:01.520Z, воскресенья:
	 *
	 *          | Переменная    | Значение                  | Что означает                         |
	 *          |---------------|---------------------------|--------------------------------------|
	 *          | \%Y          | 2025                      | Год четырьмя разрядами               |
	 *          | \%y          | 25                        | Год двумя разрядами                  |
	 *          | \%G          | 2025                      | Год недельного счёта ISO 8601        |
	 *          | \%g          | 25                        | Год недельного счёта двумя разрядами |
	 *          | \%V          | 14                        | Номер недели по ISO 8601             |
	 *          | \%C           | 20                        | Век двумя разрядами                  |
	 *          | \%B           | April                     | Название месяца полностью            |
	 *          | \%b, \%h      | Apr                       | Название месяца сокращённо           |
	 *          | \%m           | 04                        | Номер месяца двумя разрядами         |
	 *          | \%d           | 06                        | Число месяца, дополняется нулём      |
	 *          | \%e           | " 6"                      | Число месяца, дополняется пробелом   |
	 *          | \%A           | Sunday                    | Название дня недели полностью        |
	 *          | \%a           | Sun                       | Название дня недели сокращённо       |
	 *          | \%u           | 7                         | День недели, от 1 (понедельник)      |
	 *          | \%w           | 0                         | День недели, от 0 (воскресенье)      |
	 *          | \%j           | 096                       | Номер дня в году тремя разрядами     |
	 *          | \%U, \%W      | 14                        | Номер недели в году                  |
	 *          | \%F           | 2025-04-06                | Дата целиком, запись ISO 8601        |
	 *          | \%D, \%x      | 04/06/25                  | Дата целиком, запись США             |
	 *          | \%H           | 12                        | Час от 00 до 23                      |
	 *          | \%I           | 12                        | Час от 01 до 12                      |
	 *          | \%M           | 37                        | Минуты                               |
	 *          | \%S           | 01                        | Секунды                              |
	 *          | \%s           | 520                       | Доля секунды, миллисекунды           |
	 *          | \%p           | PM                        | Половина суток                       |
	 *          | \%R           | 12:37                     | Часы и минуты                        |
	 *          | \%T, \%X      | 12:37:01                  | Время целиком                        |
	 *          | \%r           | 12:37:01 PM               | Время двенадцатичасовой записью      |
	 *          | \%c           | Sun Apr 6 12:37:01 2025   | Запись asctime                       |
	 *          | \%z           | +0000                     | Смещение зоны, запись ±hhmm          |
	 *          | \%o           | +00:00                    | Смещение зоны, запись ±hh:mm         |
	 *          | \%i           | Z                         | Поле time-offset RFC 3339            |
	 *          | \%Z           | UTC                       | Название зоны либо UTC±h:mm          |
	 *          | \%n           | перевод строки            | Управляющий символ \n                |
	 *          | \%t           | табуляция                 | Управляющий символ \t                |
	 *          | \%\%          | \%                        | Сам знак процента                    |
	 *
	 *          Символы, переменными не являющиеся, при формировании записываются как
	 *          есть, а при разборе служат разделителями полей.
	 *
	 *          Переменная \%C задаёт век и при разборе складывается с двузначным
	 *          обозначением года \%y, задавая его полностью: скользящее окно раскрытия
	 *          двузначного года в этом случае в дело не вступает. Порядок этих двух
	 *          переменных в записи произволен.
	 *
	 *          При формировании записи \%U отсчитывает неделю от воскресенья, а \%W -
	 *          от понедельника. При разборе обе переменные поле записи вычитывают, но
	 *          дату по нему не восстанавливают и потому дают одинаковый результат: так
	 *          номер недели трактует strptime стандарта POSIX. Дату задают год вместе
	 *          с месяцем и числом месяца либо с номером дня в году (\%j).
	 *
	 *          Переменные \%G, \%g и \%V ведут недельный счёт ISO 8601, отдельный от
	 *          календарного: первое января 2010 года относится к 53-й неделе 2009-го,
	 *          и \%G даёт там 2009 при \%Y равном 2010. Разбор их читает и пропускает
	 *          наравне с \%U и \%W.
	 *
	 *          <b>Готовые форматы стандартов:</b>
	 *          <b>Простейшее применение:</b>
	 * @note Согласования доступа к полям объекта модуль не выполняет. Константные
	 *       методы состояния не меняют и между собой согласуются, а смешение их с
	 *       изменяющими на одном объекте согласует тот, кто объектом пользуется.
	 *       Методы, работающие с переданным штампом времени, полей не читают вовсе
	 * @see parse
	 * @see format
	 *
	 * @code{.cpp}
	 * // RFC 3164, устаревший системный журнал: Apr  6 12:37:01
	 * "%b %e %H:%M:%S"
	 * // RFC 5424 и RFC 3339: 2025-04-06T12:37:01.520Z
	 * "%Y-%m-%dT%H:%M:%S.%s%i"
	 * // RFC 5322, заголовок Date: Sun, 06 Apr 2025 12:37:01 +0000
	 * "%a, %d %b %Y %H:%M:%S %z"
	 * // Журнал веб-сервера в общем формате: 06/Apr/2025:12:37:01 +0000
	 * "%d/%b/%Y:%H:%M:%S %z"
	 * @endcode
	 *
	 * @code{.cpp}
	 * awh::chrono_t chrono(&fmk, &log);
	 * // Разбираем запись журнала веб-сервера в штамп времени
	 * const uint64_t date = chrono.parse("06/Apr/2025:12:37:01 +0000", "%d/%b/%Y:%H:%M:%S %z");
	 * // Формируем ту же дату записью действующего стандарта журнала
	 * chrono.format(date, "%Y-%m-%dT%H:%M:%S.%s%i");          // 2025-04-06T12:37:01.000Z
	 * // Получаем начало суток и номер дня недели
	 * chrono.begin(date, awh::chrono_t::type_t::DAY);           // 1743897600000
	 * chrono.get <uint8_t> (date, awh::chrono_t::unit_t::DAY);  // 7
	 * @endcode
	 *
	 * \~english
	 * @brief Structure of the Chrono module
	 * @details The class unites four independent tasks: the parsing of a record of a date by
	 *          a pattern of a format (parse), the building of a record by the same pattern
	 *          (format), the calendar arithmetic over a timestamp (begin, end,
	 *          offset, actual, get) and the conversion of the designations of the time zones into an offset
	 *          (matchTimeZone, getTimeZone). All of them work with one and the same
	 *          timestamp — the number of milliseconds since the 1st of January 1970.
	 *          <b>The format variables.</b> The values are given for the moment
	 *          1743943021520, that is 2025-04-06T12:37:01.520Z, a Sunday:
	 *          | Variable      | Value                     | What it means                        |
	 *          |---------------|---------------------------|--------------------------------------|
	 *          | \%Y          | 2025                      | Year with four digits                |
	 *          | \%y          | 25                        | Year with two digits                 |
	 *          | \%G          | 2025                      | Year of the ISO 8601 week count      |
	 *          | \%g          | 25                        | Year of the week count, two digits   |
	 *          | \%V          | 14                        | Number of the week by ISO 8601       |
	 *          | \%C           | 20                        | Century with two digits              |
	 *          | \%B           | April                     | Name of the month in full            |
	 *          | \%b, \%h      | Apr                       | Name of the month abbreviated        |
	 *          | \%m           | 04                        | Number of the month with two digits  |
	 *          | \%d           | 06                        | Day of the month, padded with a zero |
	 *          | \%e           | " 6"                      | Day of the month, padded with a space|
	 *          | \%A           | Sunday                    | Name of the day of the week in full   |
	 *          | \%a           | Sun                       | Name of the day of the week abbreviated |
	 *          | \%u           | 7                         | Day of the week, from 1 (Monday)     |
	 *          | \%w           | 0                         | Day of the week, from 0 (Sunday)     |
	 *          | \%j           | 096                       | Number of the day in the year, three digits |
	 *          | \%U, \%W      | 14                        | Number of the week in the year       |
	 *          | \%F           | 2025-04-06                | Date entirely, the ISO 8601 record   |
	 *          | \%D, \%x      | 04/06/25                  | Date entirely, the USA record        |
	 *          | \%H           | 12                        | Hour from 00 to 23                   |
	 *          | \%I           | 12                        | Hour from 01 to 12                   |
	 *          | \%M           | 37                        | Minutes                              |
	 *          | \%S           | 01                        | Seconds                              |
	 *          | \%s           | 520                       | Fraction of a second, milliseconds   |
	 *          | \%p           | PM                        | Half of the day                      |
	 *          | \%R           | 12:37                     | Hours and minutes                    |
	 *          | \%T, \%X      | 12:37:01                  | Time entirely                        |
	 *          | \%r           | 12:37:01 PM               | Time in the twelve-hour record       |
	 *          | \%c           | Sun Apr 6 12:37:01 2025   | The asctime record                   |
	 *          | \%z           | +0000                     | Offset of the zone, the ±hhmm record |
	 *          | \%o           | +00:00                    | Offset of the zone, the ±hh:mm record|
	 *          | \%i           | Z                         | The time-offset field of RFC 3339    |
	 *          | \%Z           | UTC                       | Name of the zone or UTC±h:mm         |
	 *          | \%n           | line feed                 | The control character \n             |
	 *          | \%t           | tabulation                | The control character \t             |
	 *          | \%\%          | \%                        | The percent sign itself              |
	 *          The characters that are not variables are on the building written as
	 *          they are, and on the parsing serve as the separators of the fields.
	 *          The \%C variable sets the century and on the parsing is added to the two-digit
	 *          designation of the year \%y, setting it entirely: the sliding window of the expansion
	 *          of a two-digit year does not come into play in this case. The order of these two
	 *          variables in the record is arbitrary.
	 *          On the building of a record \%U counts the week from Sunday, and \%W —
	 *          from Monday. On the parsing both variables do read the field of the record, but
	 *          do not restore the date from it and therefore give the same result: that is how
	 *          the number of the week is treated by strptime of the POSIX standard. The date is set by the year together
	 *          with the month and the day of the month or with the number of the day in the year (\%j).
	 *          The \%G, \%g and \%V variables keep the ISO 8601 week count, separate from
	 *          the calendar one: the first of January 2010 belongs to the 53rd week of 2009,
	 *          and \%G gives 2009 there at \%Y equal to 2010. The parsing reads them and skips them
	 *          on a par with \%U and \%W.
	 *          <b>Ready formats of the standards:</b>
	 *          <b>The simplest application:</b>
	 * @note The module does not perform any synchronization of the access to the fields of the object. The constant
	 *       methods do not change the state and agree with each other, and the mixing of them with
	 *       the changing ones on one object is synchronized by the one who uses the object.
	 *       The methods working with a passed timestamp do not read the fields at all
	 * @see parse
	 * @see format
	 *
	 * @code{.cpp}
	 * // RFC 3164, the obsolete system log: Apr  6 12:37:01
	 * "%b %e %H:%M:%S"
	 * // RFC 5424 and RFC 3339: 2025-04-06T12:37:01.520Z
	 * "%Y-%m-%dT%H:%M:%S.%s%i"
	 * // RFC 5322, the Date header: Sun, 06 Apr 2025 12:37:01 +0000
	 * "%a, %d %b %Y %H:%M:%S %z"
	 * // The log of a web server in the common format: 06/Apr/2025:12:37:01 +0000
	 * "%d/%b/%Y:%H:%M:%S %z"
	 * @endcode
	 *
	 * @code{.cpp}
	 * awh::chrono_t chrono(&fmk, &log);
	 * // Parsing a record of the log of a web server into a timestamp
	 * const uint64_t date = chrono.parse("06/Apr/2025:12:37:01 +0000", "%d/%b/%Y:%H:%M:%S %z");
	 * // Building the same date in the record of the standard of the log in force
	 * chrono.format(date, "%Y-%m-%dT%H:%M:%S.%s%i");          // 2025-04-06T12:37:01.000Z
	 * // Getting the beginning of the day and the number of the day of the week
	 * chrono.begin(date, awh::chrono_t::type_t::DAY);           // 1743897600000
	 * chrono.get <uint8_t> (date, awh::chrono_t::unit_t::DAY);  // 7
	 * @endcode
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Chrono {
		private:
			/**
			 * \~russian
			 * @brief Формат парсинга даты
			 *
			 * @details Набор переменных покрывает оба стандарта системного журнала.
			 *          Устаревшему RFC 3164 соответствует формат "\%b \%e \%H:\%M:\%S":
			 *          стандарт задаёт запись длиной ровно пятнадцать разрядов, и
			 *          переменная \%e дополняет число месяца меньше десяти пробелом, а
			 *          не нулём, как того требует раздел 4.1.2. Года стандарт не
			 *          записывает, и разбор подставляет текущий, а если получившаяся дата
			 *          уходит вперёд дальше допуска - предыдущий. Действующему RFC 5424
			 *          соответствует формат "\%Y-\%m-\%dT\%H:\%M:\%S.\%s\%i", где \%s даёт долю
			 *          секунды, а \%i - поле time-offset стандарта RFC 3339. Доля секунды
			 *          читается десятичной дробью, а не числом миллисекунд: записи .5,
			 *          .50 и .500 означают одну и ту же половину секунды, а разряды за
			 *          третьим отбрасываются по пределу разрешающей способности штампа.
			 *
			 * @details Обозначение временной зоны записывается четырьмя переменными, и
			 *          каждая следует своему стандарту. Переменная \%z даёт основную
			 *          запись ±hhmm - её требуют одноимённая переменная стандарта
			 *          POSIX, поле зоны заголовка Date стандарта RFC 5322, журнал
			 *          веб-сервера в общем формате и основная запись ISO 8601.
			 *          Переменная \%o даёт расширенную запись ±hh:mm - её требуют
			 *          RFC 3339 и расширенная запись ISO 8601, ей же соответствует
			 *          переменная %:z библиотеки glibc. Обе записывают обе
			 *          составляющие смещения двумя разрядами, дополняя ведущим нулём.
			 *          Переменная \%i даёт поле time-offset стандарта RFC 3339 целиком:
			 *          нулевое смещение обозначается заглавной буквой Z, любое другое -
			 *          той же расширенной записью, что и у \%o. Переменная \%Z даёт
			 *          название зоны, а при его отсутствии - сокращённую запись
			 *          UTC±h[:mm], ни одному стандарту не соответствующую: она
			 *          предназначена для чтения человеком, а не для обмена, и для
			 *          нулевого смещения даёт UTC вместо требуемого стандартами Z.
			 *          Разбор принимает шире: смещение читается одной,
			 *          двумя, тремя и четырьмя цифрами, с двоеточием и без, отдельно
			 *          и следом за названием зоны.
			 *
			 * \~english
			 * @brief Format of the parsing of a date
			 * @details The set of the variables covers both standards of the system log.
			 *          The obsolete RFC 3164 corresponds to the format "\%b \%e \%H:\%M:\%S":
			 *          the standard sets a record exactly fifteen characters long, and
			 *          the \%e variable pads a day of the month less than ten with a space, and
			 *          not with a zero, as section 4.1.2 requires. The standard does not write the year,
			 *          and the parsing substitutes the current one, and if the resulting date
			 *          goes forward further than the tolerance — the previous one. The current RFC 5424
			 *          corresponds to the format "\%Y-\%m-\%dT\%H:\%M:\%S.\%s\%i", where \%s gives the fraction of
			 *          a second, and \%i — the time-offset field of the RFC 3339 standard. The fraction of a second
			 *          is read as a decimal fraction, and not as a number of milliseconds: the records .5,
			 *          .50 and .500 mean one and the same half of a second, and the digits beyond
			 *          the third are discarded by the limit of the resolution of the timestamp.
			 * @details The designation of a time zone is written by four variables, and
			 *          each one follows its own standard. The \%z variable gives the basic
			 *          record ±hhmm — it is required by the variable of the same name of the POSIX
			 *          standard, by the field of the zone of the Date header of the RFC 5322 standard, by the web server log
			 *          in the common format and by the basic ISO 8601 record.
			 *          The \%o variable gives the extended record ±hh:mm — it is required by
			 *          RFC 3339 and by the extended ISO 8601 record, and the %:z variable
			 *          of the glibc library corresponds to it as well. Both write both
			 *          parts of the offset with two digits, padding with a leading zero.
			 *          The \%i variable gives the time-offset field of the RFC 3339 standard entirely:
			 *          a zero offset is designated by the capital letter Z, any other one —
			 *          by the same extended record as \%o. The \%Z variable gives
			 *          the name of the zone, and in its absence — the abbreviated record
			 *          UTC±h[:mm], corresponding to no standard: it
			 *          is meant for reading by a human, and not for exchange, and for
			 *          a zero offset it gives UTC instead of the Z required by the standards.
			 *          The parsing accepts more broadly: an offset is read with one,
			 *          two, three and four digits, with a colon and without, separately
			 *          and following the name of the zone.
			 *
			 * \~
			 */
			enum class format_t : uint8_t {
				NONE = 0x00, // Формат не определён
				y    = 0x01, // Формат соответствует %y (YY)
				Y    = 0x02, // Формат соответствует %Y (YYYY)
				b    = 0x03, // Формат соответствует %b и %h (MMM)
				B    = 0x04, // Формат соответствует %B (MMMMM)
				m    = 0x05, // Формат соответствует %m (MM)
				d    = 0x06, // Формат соответствует %d (dd)
				a    = 0x07, // Формат соответствует %a (EEE)
				A    = 0x08, // Формат соответствует %A (EEEEE)
				j    = 0x09, // Формат соответствует %j
				u    = 0x0A, // Формат соответствует %u (u)
				w    = 0x0B, // Формат соответствует %w
				W    = 0x0C, // Формат соответствует %W
				D    = 0x0D, // Формат соответствует %D и %x (MM/dd/YYYY)
				F    = 0x0E, // Формат соответствует %F (YYYY-MM-dd)
				H    = 0x0F, // Формат соответствует %H (HH)
				I    = 0x10, // Формат соответствует %I (h)
				M    = 0x11, // Формат соответствует %M (mm)
				s    = 0x12, // Формат соответствует (SSS)
				S    = 0x13, // Формат соответствует %S (ss)
				p    = 0x14, // Формат соответствует %p (a)
				R    = 0x15, // Формат соответствует %R (HH:mm)
				T    = 0x16, // Формат соответствует %T и %X (HH:mm:ss)
				r    = 0x17, // Формат соответствует %r (h:mm:ss a)
				c    = 0x18, // Формат соответствует %c (EEE MMM dd HH:mm:ss YYYY)
				z    = 0x19, // Формат соответствует %z и %o (±hhmm и ±hh:mm)
				Z    = 0x1A, // Формат соответствует %Z и %i (название зоны, UTC±h:mm либо Z)
				C    = 0x1B, // Формат соответствует %C (CC)
				G    = 0x1C, // Формат соответствует %G и %g (год недельного счёта ISO 8601)
				V    = 0x1D  // Формат соответствует %V (номер недели по ISO 8601)
			};
		public:
			/**
			 * \~russian
			 * @brief 12-и часовой формат времени
			 *
			 * @details Половина суток, к которой относится момент времени. Полдень относится
			 *          к PM, полночь - к AM: часы от 0 до 11 дают AM, от 12 до 23 - PM.
			 *
			 * @see h12
			 *
			 * \~english
			 * @brief The 12-hour format of the time
			 * @details The half of the day the moment of time belongs to. The noon belongs
			 *          to PM, the midnight — to AM: the hours from 0 to 11 give AM, from 12 to 23 — PM.
			 * @see h12
			 *
			 * \~
			 */
			enum class h12_t : uint8_t {
				AM = 0x00, // До полудня
				PM = 0x01  // После полудня
			};
			/**
			 * \~russian
			 * @brief Параметры смещения
			 *
			 * @details Направление смещения даты методом offset: INCREMENT двигает дату
			 *          вперёд, DECREMENT - назад.
			 *
			 * @see offset
			 *
			 * \~english
			 * @brief Parameters of the offset
			 * @details The direction of the offset of a date by the offset method: INCREMENT moves the date
			 *          forward, DECREMENT — backward.
			 * @see offset
			 *
			 * \~
			 */
			enum class offset_t : uint8_t {
				INCREMENT = 0x00, // Инкремент
				DECREMENT = 0x01  // Декремент
			};
			/**
			 * \~russian
			 * @brief Параметры актуального состояния даты
			 *
			 * @details Направление отсчёта метода actual: LEFT считает от даты до конца
			 *          отрезка, PASSED - от начала отрезка до даты.
			 *
			 * @see actual
			 *
			 * \~english
			 * @brief Parameters of the actual state of a date
			 * @details The direction of the count of the actual method: LEFT counts from the date to the end
			 *          of the interval, PASSED — from the beginning of the interval to the date.
			 * @see actual
			 *
			 * \~
			 */
			enum class actual_t : uint8_t {
				LEFT   = 0x00, // Сколько осталось времени
				PASSED = 0x01  // Сколько прошло времени
			};
			/**
			 * \~russian
			 * @brief Правило раскрытия двузначного года
			 *
			 * @details Какому столетию отнести год, записанный двумя разрядами. Единого
			 *          правила на этот счёт нет: RFC 9110 задаёт скользящее окно от текущего
			 *          года, POSIX для strptime - неподвижный рубеж между 68 и 69 годами.
			 *          Оба действуют и оба нужны, поэтому выбор оставлен за вызывающим.
			 *
			 * @see century
			 *
			 * \~english
			 * @brief Rule of the expansion of a two-digit year
			 * @details Which century a year written with two digits should be attributed to. There is no single
			 *          rule on this account: RFC 9110 sets a sliding window from the current
			 *          year, POSIX for strptime — an immovable boundary between the years 68 and 69.
			 *          Both are in force and both are needed, and therefore the choice is left to the caller.
			 * @see century
			 *
			 * \~
			 */
			/**
			 * \~russian
			 * @brief Запись даты, заданная стандартом
			 *
			 * @details Готовые записи действующих стандартов. Формировать и разбирать их
			 *          строкой формата можно и вручную, но обозначение стандарта избавляет
			 *          от переписывания образца и от ошибок в его мелочах, а разбор
			 *          принимает все допустимые стандартом разновидности записи, а не одну.
			 *
			 * @see format
			 * @see parse
			 * @see validate
			 *
			 * \~english
			 * @brief Record of a date set by a standard
			 * @details Ready records of the current standards. Building and parsing them with
			 *          a format string is possible by hand as well, but the designation of a standard saves
			 *          from rewriting the pattern and from mistakes in its details, and the parsing
			 *          accepts all the varieties of the record allowed by the standard, and not one.
			 * @see format
			 * @see parse
			 * @see validate
			 *
			 * \~
			 */
			enum class standard_t : uint8_t {
				CLF     = 0x00, // Журнал веб-сервера: 06/Apr/2025:12:37:01 +0000
				RFC850  = 0x01, // Устаревшая запись HTTP: Sunday, 06-Apr-25 12:37:01 GMT
				RFC1123 = 0x02, // Заголовки HTTP: Sun, 06 Apr 2025 12:37:01 GMT
				RFC3164 = 0x03, // Устаревший системный журнал: Apr  6 12:37:01
				RFC3339 = 0x04, // Журналы и программные вводы: 2025-04-06T12:37:01.520Z
				RFC5322 = 0x05, // Заголовок Date почты: Sun, 06 Apr 2025 12:37:01 +0000
				ISO8601 = 0x06, // Основная форма ISO 8601: 20250406T123701Z
				ASCTIME = 0x07  // Запись asctime языка C: Sun Apr  6 12:37:01 2025
			};
			/**
			 * \~russian
			 * @brief Правило раскрытия двузначного года
			 *
			 * @details Какому столетию отнести год, записанный двумя разрядами. Единого
			 *          правила на этот счёт нет: RFC 9110 задаёт скользящее окно от текущего
			 *          года, POSIX для strptime - неподвижный рубеж между 68 и 69 годами.
			 *          Оба действуют и оба нужны, поэтому выбор оставлен за вызывающим.
			 *
			 * @see century
			 *
			 * \~english
			 * @brief Rule of the expansion of a two-digit year
			 * @details Which century a year written with two digits should be attributed to. There is no single
			 *          rule on this account: RFC 9110 sets a sliding window from the current
			 *          year, POSIX for strptime — an immovable boundary between the years 68 and 69.
			 *          Both are in force and both are needed, and therefore the choice is left to the caller.
			 * @see century
			 *
			 * \~
			 */
			enum class century_t : uint8_t {
				WINDOW = 0x00, // Скользящее окно от текущего года по RFC 9110 (§5.6.7)
				POSIX  = 0x01  // Неподвижный рубеж: 69-99 к двадцатому веку, 00-68 к двадцать первому
			};
			/**
			 * \~russian
			 * @brief Тип хранимой даты
			 *
			 * @details Откуда методы без довода даты берут момент времени. GLOBAL означает
			 *          системные часы: каждый вызов обращается к ним заново. LOCAL означает
			 *          внутренний объект даты класса - его заполняют методы set, timestamp и
			 *          разбор с этим же хранилищем, а clear возвращает к текущему моменту.
			 *          Хранилище LOCAL нужно, когда дату собирают по частям либо когда
			 *          несколько вызовов подряд обязаны относиться к одному моменту.
			 *
			 * @note Все обращения к хранилищу LOCAL читают поля объекта, а часть из них их
			 *       изменяет: согласование доступа при работе из нескольких потоков лежит
			 *       на том, кто объектом пользуется
			 *
			 * @code{.cpp}
			 * // Собираем дату по частям во внутреннем объекте
			 * chrono.timestamp(1743943021520, awh::chrono_t::type_t::MILLISECONDS);
			 * chrono.set <uint16_t> (2030, awh::chrono_t::unit_t::YEAR);
			 * chrono.format("%Y-%m-%d", awh::chrono_t::storage_t::LOCAL);  // 2030-04-06
			 * // Системные часы внутренним объектом не затронуты
			 * chrono.format("%Y-%m-%d", awh::chrono_t::storage_t::GLOBAL);
			 * @endcode
			 *
			 * \~english
			 * @brief Type of the stored date
			 * @details Where the methods without an argument of a date take the moment of time from. GLOBAL means
			 *          the system clock: every call addresses it anew. LOCAL means
			 *          the internal date object of the class — it is filled by the set and timestamp methods and
			 *          by the parsing with the same storage, and clear returns it to the current moment.
			 *          The LOCAL storage is needed when a date is assembled piece by piece or when
			 *          several calls in a row are obliged to belong to one moment.
			 * @note All the addresses to the LOCAL storage read the fields of the object, and a part of them changes
			 *       them: the synchronization of the access when working from several threads lies
			 *       on the one who uses the object
			 *
			 * @code{.cpp}
			 * // Building the date by parts in the internal object
			 * chrono.timestamp(1743943021520, awh::chrono_t::type_t::MILLISECONDS);
			 * chrono.set <uint16_t> (2030, awh::chrono_t::unit_t::YEAR);
			 * chrono.format("%Y-%m-%d", awh::chrono_t::storage_t::LOCAL);  // 2030-04-06
			 * // The system clock is not touched by the internal object
			 * chrono.format("%Y-%m-%d", awh::chrono_t::storage_t::GLOBAL);
			 * @endcode
			 *
			 */
			enum class storage_t : uint8_t {
				NONE   = 0x00, // Хранение даты не установлено
				LOCAL  = 0x01, // Дата в формате локальных настроек
				GLOBAL = 0x02  // Дата в формате глобального времени
			};
			/**
			 * \~russian
			 * @brief Тип штампа времени
			 *
			 * @details Единица измерения времени. Служит и размерностью штампа для методов
			 *          timestamp, и величиной календарного отрезка для методов begin, end,
			 *          actual и offset. Неделя отсчитывается с понедельника.
			 *
			 * @note Единица MONTH - единственная непостоянной длительности, поэтому
			 *       смещение на месяцы ограничивает число месяца последним днём конечного
			 *       месяца: 31 января со смещением на месяц вперёд даёт 28 либо 29 февраля
			 *
			 * @see begin
			 * @see end
			 * @see offset
			 * @see timestamp
			 *
			 * \~english
			 * @brief Type of the timestamp
			 * @details The unit of the measurement of time. Serves both as the dimension of the timestamp for the
			 *          timestamp methods, and as the length of the calendar interval for the begin, end,
			 *          actual and offset methods. The week is counted from Monday.
			 * @note The MONTH unit is the only one of a non-constant length, and therefore
			 *       an offset by months limits the day of the month by the last day of the final
			 *       month: the 31st of January with an offset by a month forward gives the 28th or the 29th of February
			 * @see begin
			 * @see end
			 * @see offset
			 * @see timestamp
			 *
			 * \~
			 */
			enum class type_t : uint8_t {
				NONE         = 0x00, // Не установлено
				YEAR         = 0x01, // Год
				MONTH        = 0x02, // Месяц
				WEEK         = 0x03, // Неделя
				DAY          = 0x04, // День
				HOUR         = 0x05, // Час
				MINUTES      = 0x06, // Минуты
				SECONDS      = 0x07, // Секунды
				MILLISECONDS = 0x08, // Миллисекунды
				MICROSECONDS = 0x09, // Микросекунды
				NANOSECONDS  = 0x0A  // Наносекунды
			};
			/**
			 * \~russian
			 * @brief Тип элементов даты
			 *
			 * @details Отдельная составляющая даты для методов get и set. В отличие от
			 *          type_t, задающего величину отрезка времени, unit_t задаёт поле
			 *          календарного разложения даты.
			 *
			 *          Составляющие DAY и MONTH извлекаются и числом, и названием - смотря
			 *          какой тип задан шаблонным доводом:
			 * @see get
			 * @see set
			 *
			 * @code{.cpp}
			 * chrono.get <uint8_t> (date, awh::chrono_t::unit_t::MONTH);      // 4
			 * chrono.get <std::string> (date, awh::chrono_t::unit_t::MONTH);  // April
			 * chrono.get <uint8_t> (date, awh::chrono_t::unit_t::DAY);        // 7
			 * chrono.get <std::string> (date, awh::chrono_t::unit_t::DAY);    // Sunday
			 * @endcode
			 *
			 * \~english
			 * @brief Type of the elements of a date
			 * @details A separate part of a date for the get and set methods. Unlike
			 *          type_t, setting the length of an interval of time, unit_t sets a field
			 *          of the calendar decomposition of a date.
			 *          The DAY and MONTH parts are extracted both as a number and as a name — depending on
			 *          which type is set by the template argument:
			 * @see get
			 * @see set
			 *
			 * @code{.cpp}
			 * chrono.get <uint8_t> (date, awh::chrono_t::unit_t::MONTH);      // 4
			 * chrono.get <std::string> (date, awh::chrono_t::unit_t::MONTH);  // April
			 * chrono.get <uint8_t> (date, awh::chrono_t::unit_t::DAY);        // 7
			 * chrono.get <std::string> (date, awh::chrono_t::unit_t::DAY);    // Sunday
			 * @endcode
			 *
			 */
			enum class unit_t : uint8_t {
				NONE         = 0x00, // Элемент даты не установлен
				DAY          = 0x01, // Номер текущего дня недели от 1 до 7
				DATE         = 0x02, // Число месяца от 1 до 31
				YEAR         = 0x03, // Полное обозначение года
				HOUR         = 0x04, // Количество часов от 0 до 23
				DAYS         = 0x05, // Количество прошедвших дней от 1 января
				MONTH        = 0x06, // Номер месяца от 1 до 12 (начиная с Января)
				WEEKS        = 0x07, // Количество недель с начала года, округлённое до ближайшего целого
				OFFSET       = 0x08, // Смещение временной зоны в секундах относительно UTC
				MINUTES      = 0x09, // Количество минут от 0 до 59
				SECONDS      = 0x0A, // Количество секунд от 0 до 59
				MILLISECONDS = 0x0B, // Количество миллисекунд
				MICROSECONDS = 0x0C, // Количество микросекунд
				NANOSECONDS  = 0x0D  // Количество наносекунд
			};
			/**
			 * \~russian
			 * @brief Временная зона
			 *
			 * @details Перечисление обозначений временных зон, принятых в записях дат.
			 *          Обозначение переводится в смещение методом getTimeZone, обратный
			 *          перевод обозначения в элемент перечисления делает matchTimeZone.
			 *
			 *          Имена элементов повторяют общепринятые сокращения зон. Там, где одно
			 *          сокращение занято несколькими зонами, к нему добавлены два разряда
			 *          страны: AMTAM - амазонское время, AMTAR - армянское, ISTID - индийское,
			 *          ISTIS - израильское, CSTNA - североамериканское центральное, CSTKT -
			 *          китайское.
			 *
			 * @note Перечисление задаёт лишь смещение зоны, но не правила перехода на
			 *       летнее время: стандартное и летнее время каждой зоны - это два разных
			 *       её элемента (EST и EDT, MSK и MSD). Выбрать нужный по текущей дате
			 *       позволяет перегрузка getTimeZone с двумя доводами
			 *
			 * @see getTimeZone
			 * @see matchTimeZone
			 *
			 * \~english
			 * @brief Time zone
			 * @details Enumeration of the designations of the time zones accepted in the records of dates.
			 *          A designation is converted into an offset by the getTimeZone method, the reverse
			 *          conversion of a designation into an element of the enumeration is done by matchTimeZone.
			 *          The names of the elements repeat the commonly accepted abbreviations of the zones. Where one
			 *          abbreviation is taken by several zones, two characters of the country are added to it:
			 *          AMTAM is the Amazon time, AMTAR — the Armenian one, ISTID — the Indian one,
			 *          ISTIS — the Israeli one, CSTNA — the North American central one, CSTKT —
			 *          the Chinese one.
			 * @note The enumeration sets only the offset of a zone, but not the rules of the transition to
			 *       the daylight saving time: the standard and the daylight saving time of every zone are two different
			 *       elements of it (EST and EDT, MSK and MSD). Choosing the needed one by the current date
			 *       is made possible by the getTimeZone overload with two arguments
			 * @see getTimeZone
			 * @see matchTimeZone
			 *
			 * \~
			 *
			 * @site https://24timezones.com/mirovoe_vremia3.php
			 *
			 *
			 */
			enum class zone_t : uint8_t {
				NONE   = 0x00, // Временная зона не установлена
				AT     = 0x01, // Атлантическое Время
				CT     = 0x02, // Северноамериканское Центральное Время
				ET     = 0x03, // Северноамериканское Восточное Время
				MT     = 0x04, // Северноамериканское Горное Время
				NT     = 0x05, // Время В Ньюфаундленде
				PT     = 0x06, // Северноамериканское Тихоокеанское Время
				MUT    = 0x07, // Стандартное Время На Острове Маврикий
				MVT    = 0x08, // Время На Мальдивах
				MYT    = 0x09, // Малайское Время
				NCT    = 0x0A, // Стандартное Время В Новой Каледонии
				NDT    = 0x0B, // Летнее Время В Ньюфаундленде
				NFT    = 0x0C, // Время На Острове Норфолк
				NPT    = 0x0D, // Непальськое Время
				NRT    = 0x0E, // Время На Острове Науру
				NST    = 0x0F, // Стандартное Время В Ньюфаундленде
				PWT    = 0x10, // Время На Острове Палау
				NUT    = 0x11, // Время На Острове Ниуэ
				FET    = 0x12, // Минское Время
				FJT    = 0x13, // Летнее Время На О. Фиджи
				PYT    = 0x14, // Парагвайское Стандартное Время
				RET    = 0x15, // Время На Острове Реюньон
				SBT    = 0x16, // Время На Соломоновых Островах
				SCT    = 0x17, // Время На Сейшелах
				SGT    = 0x18, // Сингапурское Время
				SRT    = 0x19, // Время В Суринаме
				SST    = 0x1A, // Стандартное Время На Острове Самоа
				TFT    = 0x1B, // Французское Южное И Антарктическое Время
				THA    = 0x1C, // Тайландское Время
				TJT    = 0x1D, // Время В Таджикистане
				TKT    = 0x1E, // Время На Островах Токелау
				TLT    = 0x1F, // Время В Восточном Тиморе
				TMT    = 0x20, // Стандартное Время В Туркмении
				TOT    = 0x21, // Время На Островах Тонга
				TRT    = 0x22, // Турецкое Время
				TVT    = 0x23, // Время На Островах Тувалу
				FKT    = 0x24, // Стандартное Время На Фолклендах
				FNT    = 0x25, // Стандартное Время На Фернанду-Ди-Норонья
				AFT    = 0x26, // Время В Афганистане
				ACT    = 0x27, // Амазонское Стандартное Время
				ADT    = 0x28, // Атлантическое Летнее Время
				AZT    = 0x29, // Азербайджанское Стандартное Время
				ART    = 0x2A, // Аргентинское Стандартное Время
				BDT    = 0x2B, // Время В Бруней-Даруссаламе
				BNT    = 0x2C, // Время В Бруней-Даруссаламе
				BOT    = 0x2D, // Боливийское Время
				BRT    = 0x2E, // Бразильское Стандартное Время
				BTT    = 0x2F, // Бутанское Время
				CAT    = 0x30, // Восточноафриканское Время
				CVT    = 0x31, // Стандартное Время На Островах Кабо-Верде
				CXT    = 0x33, // Время На Острове Рождества
				CCT    = 0x34, // Время На Кокосовые Островах
				CET    = 0x35, // Центральноевропейское Стандартное Время
				CIT    = 0x36, // Время В Центральной Индонезии
				CKT    = 0x37, // Стандартное Время На Островах Кука
				CLT    = 0x38, // Чилийское Стандартное Время
				COT    = 0x39, // Колумбийское Стандартное Время
				EAT    = 0x3A, // Восточноафриканский Час
				ECT    = 0x3B, // Эквадорское Время
				EDT    = 0x3C, // Северноамериканское Восточное Летнее Время
				EET    = 0x3D, // Восточноевропейское Стандартное Время
				EGT    = 0x3E, // Стандартное Время В Восточной Гренландии
				EIT    = 0x3F, // Время В Восточной Индонезии
				EST    = 0x40, // Северноамериканское Восточное Стандартное Время
				GET    = 0x41, // Грузинское Стандартное Время
				ICT    = 0x42, // Время В Индокитае
				IDT    = 0x43, // Израильское Летнее Время
				GFT    = 0x44, // Время В Французской Гвиане
				GIT    = 0x45, // Время На О. Гамбье
				GMT    = 0x46, // Среднее Время По Гринвичу
				GYT    = 0x47, // Время В Гайане
				HKT    = 0x48, // Гонконгское Стандартное Время
				JST    = 0x49, // Японское Стандартное Время
				KGT    = 0x4A, // Время В Киргизии
				KST    = 0x4B, // Корейское Стандартное Время
				MDT    = 0x4C, // Северноамериканское Горное Летнее Время
				MHT    = 0x4D, // Время На Маршалловых Островах
				MIT    = 0x4E, // Время На Маркизских Островах
				MMT    = 0x4F, // Время В Мьянме
				MSK    = 0x50, // Московское Время
				MSD    = 0x51, // Московское летнее время
				PST    = 0x52, // Северноамериканское Тихоокеанское Стандартное Время
				PDT    = 0x53, // Северноамериканское Тихоокеанское Летнее Время
				PET    = 0x54, // Стандартное Время В Перу
				PGT    = 0x55, // Время В Папуа-Новой Гвинее
				UTC    = 0x56, // Всемирное Координированное Время
				PHT    = 0x57, // Стандартное Время На Филлипинах
				PKT    = 0x58, // Пакистанское Стандартное Время
				UYT    = 0x59, // Стандартное Время В Уругвае
				UZT    = 0x5A, // Время В Узбекистане
				VET    = 0x5B, // Время В Венесуеле
				VUT    = 0x5C, // Стандартное Время На Островах Вануату
				WAT    = 0x5D, // Западноафриканское Стандартное Время
				WET    = 0x5E, // Западноевропейский Стандартний Час
				WFT    = 0x5F, // Время На Островах Уоллис И Футуна
				WIB    = 0x60, // Время В Западной Индонезии
				WIT    = 0x61, // Время В Восточной Индонезии
				ACDT   = 0x62, // Летнее Время В Центральной Австралии
				ACST   = 0x63, // Стандартное Время В Центральной Австралии
				AEDT   = 0x64, // Летнее Время В Восточной Австралии
				AEST   = 0x65, // Стандартное Время В Восточной Австралии
				AKDT   = 0x66, // Летнее Время На Аляске
				AKST   = 0x67, // Стандартное Время На Аляске
				AMST   = 0x68, // Амазонка, Стандартное Время
				AWST   = 0x69, // Стандартное Время В Западной Австралии
				AZOT   = 0x6A, // Стандартное Время На Азорских Островах
				BRST   = 0x6B, // Бразильское Летнее Время
				CLST   = 0x6C, // Чилийское Летнее Время
				CEST   = 0x6D, // Центральноевропейское Летнее Время
				CHOT   = 0x6E, // Стандартное Время В Чойлобалсане
				CHST   = 0x6F, // Час Чаморро
				CHUT   = 0x70, // Время На Островах Чуук
				COST   = 0x71, // Колумбийское Летнее Время
				DAVT   = 0x72, // Дейвис
				DDUT   = 0x73, // Дюмон-Д'юрвиль
				EGST   = 0x74, // Летнее Время В Восточной Гренландии
				EAST   = 0x75, // Стандартное Время На Острове Пасхи
				EEST   = 0x76, // Восточноевропейское Летнее Время
				FKST   = 0x77, // Летнее Время На Фолклендах
				GAMT   = 0x78, // Время На Острове Гамбье
				HOVT   = 0x79, // Стандартное Время В Ховде
				HADT   = 0x7A, // Гавайско-Алеутское Летнее Время
				HAST   = 0x7B, // Гавайско-Алеутское Стандартное Время
				IRDT   = 0x7C, // Иранское Летнее Время
				IRKT   = 0x7D, // Иркутское Стандартное Время
				IRST   = 0x7E, // Иранское Стандартное Время
				GILT   = 0x7F, // Время На Островах Гилберта
				GALT   = 0x80, // Время На Галапагосских Островах
				KOST   = 0x81, // Время На Острове Косраэ
				KRAT   = 0x82, // Красноярское Стандартное Время
				LHDT   = 0x83, // Летнее Время На Лорд-Хау
				LHST   = 0x84, // Стандартное Время На Лорд-Хау
				LINT   = 0x85, // Время На Острове Лайн
				MAGT   = 0x86, // Магаданское Стандартное Время
				MART   = 0x87, // Время На Маркизских Островах
				MIST   = 0x88, // Время На Станции Маккуори
				MAWT   = 0x89, // Время На Станции Моусон
				NZDT   = 0x8A, // Летнее Время В Новой Зеландии
				NZST   = 0x8B, // Стандартное Время В Новой Зеландии
				PYST   = 0x8C, // Парагвайское Летнее Время
				PETT   = 0x8D, // Камчатское Время
				PMDT   = 0x8E, // Летнее Время На Островах Сен-Пьер И Микелон
				PMST   = 0x8F, // Стандартное Время На Островах Сен-Пьер И Микелон
				PONT   = 0x90, // Время На Острове Понапе
				PHOT   = 0x91, // Время На Островах Феникс
				PhST   = 0x92, // Стандартное Время На Филлипинах
				ROTT   = 0x93, // Время На Станции Ротера
				SLST   = 0x94, // Стандартное Время В Шри-Ланке
				SAKT   = 0x95, // Сахалинское Стандартное Время
				SAMT   = 0x96, // Самарское Время
				SAST   = 0x97, // Южноафриканское Время
				SYOT   = 0x98, // Время На Станции Сёва
				TAHT   = 0x99, // Время На Острове Таити
				OMST   = 0x9A, // Омское Время
				ORAT   = 0x9B, // Время В Западном Казахстане
				VLAT   = 0x9C, // Владивостокское Время
				VOLT   = 0x9D, // Волгоградское Время
				VOST   = 0x9E, // Время На Станции Восток
				UYST   = 0x9F, // Летнее Время В Уругвае
				ULAT   = 0xA0, // Стандартное Время В Монголии
				USZ1   = 0xA1, // Калиниградское Время
				WAKT   = 0xA2, // Время На Острове Уэйк
				WAST   = 0xA3, // Западноафриканское Летнее Время
				WEST   = 0xA4, // Западноевропейское Летнее Время
				WGST   = 0xA5, // Стандартное Время В Западной Гренландии (обозначается WGT)
				YAKT   = 0xA6, // Якутское Время
				YEKT   = 0xA7, // Екатеринбургское Время
				ACWST  = 0xA8, // Центрально-Западная Австралия, Стандартное Время
				AZOST  = 0xA9, // Летнее Время На Азорских Островах
				CHADT  = 0xAA, // Летнее Время На Архипелаге Чатем
				CHAST  = 0xAB, // Стандартное Время На Архипелаге Чатем
				CHOST  = 0xAC, // Летнее Время В Чойлобалсане
				EASST  = 0xAD, // Летнее Время На Острове Пасхи
				HOVST  = 0xAE, // Летнее Время В Ховде
				ULAST  = 0xAF, // Летнее Время В Монголии
				AMTAM  = 0xB0, // Амазонское Стандартное Время
				AMTAR  = 0xB1, // Армянское Стандартное Время
				ASTAL  = 0xB2, // Атлантическое Стандартное Время
				ASTSA  = 0xB3, // Стандартное Время В Саудовской Аравии
				BSTBR  = 0xB4, // Британское Летнее Время
				BSTBL  = 0xB5, // Стандартное Время В Бангладеш
				CDTNA  = 0xB6, // Северноамериканское Центральное Летнее Время
				CDTCB  = 0xB7, // Кубинское Летнее Время
				CSTNA  = 0xB8, // Северноамериканское Центральное Стандартное Время
				CSTKT  = 0xB9, // Китайское Стандартное Время
				CSTCB  = 0xBA, // Кубинское Стандартное Время
				GSTPG  = 0xBB, // Время В Персидском Заливе
				GSTSG  = 0xBC, // Время В Южной Георгии
				ISTID  = 0xBD, // Индийское Стандартное Время
				ISTIR  = 0xBE, // Ирландия, Летнее Время
				ISTIS  = 0xBF, // Израильское Стандартное Время
				MSTNA  = 0xC0, // Северноамериканское Горное Стандартное Время
				MSTMS  = 0xC1, // Время В Малайзии
				WGSTST = 0xC2  // Летнее Время В Западной Гренландии (обозначается WGST)
			};
			/**
			 * \~russian
			 * @brief Структура параметров даты и времени
			 *
			 * @details Структура содержит все необходимые параметры для хранения даты и времени,
			 *          включая флаги летнего времени и високосного года, статус 12-и часового формата времени,
			 *          идентификатор временной зоны, а также различные компоненты даты и времени, такие как день недели,
			 *          число месяца, количество часов, минут, секунд, миллисекунд, микросекунд и наносекунд.
			 *          Также включены смещение временной зоны относительно UTC и количество прошедших дней с начала года.
			 *
			 * \~english
			 * @brief Structure of the parameters of the date and the time
			 * @details The structure contains all the parameters needed for storing a date and a time,
			 *          including the signs of the daylight saving time and of the leap year, the status of the 12-hour format of the time,
			 *          the identifier of the time zone, as well as the various parts of the date and the time, such as the day of the week,
			 *          the day of the month, the number of the hours, the minutes, the seconds, the milliseconds, the microseconds and the nanoseconds.
			 *          Also included are the offset of the time zone relative to UTC and the number of the days elapsed since the beginning of the year.
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ DateTime {
				bool dst;              // Флаг летнего времени
				bool leap;             // Флаг високосного года
				h12_t h12;             // Статус 12-и часового формата времени (по умолчанию AM)
				zone_t zone;           // Идентификатор временной зоны
				uint8_t day;           // Номер текущего дня недели от 1 до 7 (по умолчанию 2 - понедельник)
				uint8_t date;          // Число месяца от 1 до 31 (по умолчанию 1)
				uint8_t hour;          // Количество часов от 0 до 23
				uint8_t month;         // Номер месяца от 1 до 12 (начиная с Января, по умолчанию 1)
				uint8_t weeks;         // Количество недель прошедших с начала года
				uint8_t seconds;       // Количество секунд от 0 до 59
				uint8_t minutes;       // Количество минут от 0 до 59
				uint16_t year;         // Полное обозначение года (по умолчанию 1970)
				uint16_t days;         // Количество прошедвших дней от 1 января
				int32_t offset;        // Смещение временной зоны в секундах относительно UTC
				uint32_t milliseconds; // Количество миллисекунд в секунде, от 0 до 999
				uint64_t microseconds; // Количество микросекунд в миллисекунде, от 0 до 999
				uint64_t nanoseconds;  // Количество наносекунд в миллисекунде, от 0 до 999999, microseconds - его старшая часть
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit DateTime() noexcept;
			} __attribute__((packed)) dt_t;
		private:
			// Объект локального времени
			dt_t _dt;
		private:
			// Допуск отката года при разборе записи без года (в секундах)
			uint32_t _yearRollback;
			// Окно двузначного года при разборе записи с ним (в годах)
			uint8_t _yearWindow;
			// Правило раскрытия двузначного года при разборе записи с ним
			century_t _century;
			// Признак приёма секунды координации при проверке пригодности записи
			bool _leapSecond;
		private:
			// Список внутренних временных зон
			unordered_map <string, int32_t> _timeZones;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект логера
			const Logging * _log;
		public:
			/**
			 * \~russian
			 * @brief Метод очистки всех локальных данных
			 *
			 * @details Возвращает внутренний объект даты (хранилище storage_t::LOCAL) к
			 *          текущему моменту системных часов. Реестр временных зон не
			 *          затрагивается - его очищает отдельный метод clearTimeZones.
			 *
			 *          Поля даты раскладываются в нулевой временной зоне, и смещение зоны
			 *          сбрасывается вместе с ними: собственное смещение объект получает
			 *          разбором записи методом parse либо вызовом setTimeZone. Временная
			 *          зона окружения при этом никуда не девается - формирование записи
			 *          по штампу времени ведётся именно в ней.
			 *
			 * @see clearTimeZones
			 *
			 * \~english
			 * @brief Method of clearing all the local data
			 * @details Returns the internal date object (the storage_t::LOCAL storage) to the
			 *          current moment of the system clock. The registry of the time zones is not
			 *          touched — it is cleared by the separate clearTimeZones method.
			 *          The fields of the date are decomposed in the zero time zone, and the offset of the zone
			 *          is reset together with them: its own offset the object receives by
			 *          the parsing of a record by the parse method or by a call to setTimeZone. The time
			 *          zone of the environment at that goes nowhere — the building of a record
			 *          by a timestamp is performed in it exactly.
			 * @see clearTimeZones
			 *
			 * \~
			 */
			void clear() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения допуска отката года
			 *
			 * @details Выводит величину допуска, действующую при разборе записей, года
			 *          не содержащих
			 *
			 * @return допуск отката года в секундах, ноль если откат отключён
			 *
			 * @see yearRollback
			 *
			 * \~english
			 * @brief Method of getting the tolerance of the rollback of the year
			 * @details Yields the value of the tolerance in force at the parsing of the records that do not
			 *          contain a year
			 * @return tolerance of the rollback of the year in seconds, zero if the rollback is switched off
			 * @see yearRollback
			 *
			 * \~
			 */
			uint32_t yearRollback() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки допуска отката года
			 *
			 * @details Задаёт, насколько далеко вперёд может отстоять запись, года не
			 *          содержащая, прежде чем разбор отнесёт её к предыдущему году.
			 *          Устаревший стандарт системного журнала RFC 3164 года не
			 *          записывает, и разбор подставляет текущий; декабрьская запись,
			 *          прочитанная в январе, отстояла бы при этом на одиннадцать
			 *          месяцев вперёд, и допуск позволяет опознать такую запись.
			 *
			 *          По умолчанию допуск равен двадцати шести часам. Величина взята
			 *          не произвольно: тот же стандарт временной зоны в записи не
			 *          указывает, штамп содержит местное время отправителя, а читается
			 *          он в зоне получателя. Полный разброс временных зон - от UTC+14
			 *          до UTC-12 - составляет ровно двадцать шесть часов, и запись,
			 *          опережающая получателя меньше чем на эту величину, опережает его
			 *          законно. Меньший допуск отправляет на год назад каждую запись
			 *          хоста, стоящего восточнее получателя.
			 *
			 * @note Допуск заметно больший вреден: он начинает датировать будущим
			 *       записи, которым место в прошлом году. Запись «5 февраля»,
			 *       прочитанная 10 января, при допуске в три месяца окажется в будущем
			 *       на двадцать шесть суток вместо прошлогоднего февраля
			 * @note Нулевой допуск отключает откат целиком, а не задаёт нулевую
			 *       величину: запись без года всегда относится к текущему году
			 * @param seconds допуск отката года в секундах, ноль отключает откат
			 * @see parse
			 *
			 * @code{.cpp}
			 * // Один датацентр в одной временной зоне: разброса зон нет
			 * chrono.yearRollback(3600);
			 * // Хосты по всему свету, с запасом на разъехавшиеся часы
			 * chrono.yearRollback(30 * 3600);
			 * // Год приходит из другого источника, откат не нужен
			 * chrono.yearRollback(0);
			 * @endcode
			 *
			 * \~english
			 * @brief Method of setting the tolerance of the rollback of the year
			 * @details Sets how far forward a record that does not contain a year may stand
			 *          before the parsing attributes it to the previous year.
			 *          The obsolete standard of the system log RFC 3164 does not write
			 *          the year, and the parsing substitutes the current one; a December record,
			 *          read in January, would at that stand eleven
			 *          months forward, and the tolerance allows such a record to be recognized.
			 *          By default the tolerance equals twenty six hours. The value is taken
			 *          not arbitrarily: the same standard does not specify the time zone in the record,
			 *          the timestamp contains the local time of the sender, and it is read
			 *          in the zone of the receiver. The full spread of the time zones — from UTC+14
			 *          to UTC-12 — makes up exactly twenty six hours, and a record
			 *          leading the receiver by less than this value leads it
			 *          legally. A smaller tolerance sends a year back every record of a host
			 *          standing to the east of the receiver.
			 * @note A noticeably greater tolerance is harmful: it starts dating with the future
			 *       the records whose place is in the previous year. The record «5th of February»,
			 *       read on the 10th of January, at a tolerance of three months will turn out to be in the future
			 *       by twenty six days instead of the February of the previous year
			 * @note A zero tolerance switches off the rollback entirely, and does not set a zero
			 *       value: a record without a year always belongs to the current year
			 * @param seconds tolerance of the rollback of the year in seconds, zero switches off the rollback
			 * @see parse
			 *
			 * @code{.cpp}
			 * // One data centre in one time zone: there is no spread of the zones
			 * chrono.yearRollback(3600);
			 * // The hosts are all over the world, with a reserve for the clocks that have drifted apart
			 * chrono.yearRollback(30 * 3600);
			 * // The year comes from another source, the fallback is not needed
			 * chrono.yearRollback(0);
			 * @endcode
			 *
			 */
			void yearRollback(const uint32_t seconds) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения окна двузначного года
			 *
			 * @details Выводит величину окна, действующую при разборе записей, год в
			 *          которых записан двумя разрядами
			 *
			 * @return окно двузначного года в годах, ноль если правило отключено
			 *
			 * @see yearWindow
			 *
			 * \~english
			 * @brief Method of getting the window of a two-digit year
			 * @details Yields the value of the window in force at the parsing of the records the year in
			 *          which is written with two digits
			 * @return window of a two-digit year in years, zero if the rule is switched off
			 * @see yearWindow
			 *
			 * \~
			 */
			uint8_t yearWindow() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки окна двузначного года
			 *
			 * @details Задаёт, насколько далеко вперёд может отстоять запись, год в которой
			 *          записан двумя разрядами, прежде чем разбор отнесёт её к предыдущему
			 *          столетию. Правило это задано RFC 9110 (§5.6.7) для устаревшего
			 *          формата даты RFC 850, где год записывается двумя разрядами:
			 *          обозначение, попадающее дальше в будущее, чем допускает окно,
			 *          читается как ближайший прошедший год с теми же двумя разрядами.
			 *
			 *          По умолчанию окно равно пятидесяти годам, как того и требует
			 *          стандарт. При текущем 2026 годе оно покрывает промежуток с 1977 по
			 *          2076 год:
			 *
			 *          Стандарт POSIX для strptime задаёт вместо скользящего окна
			 *          неподвижный рубеж: разряды от 69 до 99 относятся к двадцатому веку,
			 *          от 00 до 68 - к двадцать первому. Правило это задаёт отдельный метод
			 *          century, и величина окна при нём не используется.
			 * @note Нулевое окно отключает правило целиком: двузначный год всегда
			 *       относится к двадцать первому веку, как это делалось до появления
			 *       правила
			 * @note Двузначным годом обозначаются переменные формата \%y и \%D. Полное
			 *       обозначение года переменной \%Y правило не затрагивает
			 * @note Запись двузначным годом неоднозначна по устройству: обратное чтение
			 *       записи, сформированной переменной \%y, даёт исходный год не всегда, а
			 *       только когда он лежит внутри окна
			 * @param years окно двузначного года в годах, ноль отключает правило
			 * @see parse
			 * @see yearRollback
			 *
			 * @code{.cpp}
			 * chrono.parse("06-Nov-94 08:49:37", "%d-%b-%y %H:%M:%S");  // 1994 год
			 * chrono.parse("06-Nov-70 08:49:37", "%d-%b-%y %H:%M:%S");  // 2070 год
			 * @endcode
			 *
			 * \~english
			 * @brief Method of setting the window of a two-digit year
			 * @details Sets how far forward a record the year in which is written with two
			 *          digits may stand before the parsing attributes it to the previous
			 *          century. This rule is set by RFC 9110 (§5.6.7) for the obsolete
			 *          date format RFC 850, where the year is written with two digits:
			 *          a designation falling further into the future than the window allows
			 *          is read as the nearest past year with the same two digits.
			 *          By default the window equals fifty years, exactly as the standard requires.
			 *          At the current year 2026 it covers the range from 1977 to
			 *          2076:
			 *          The POSIX standard for strptime sets an immovable boundary instead of a sliding window:
			 *          the digits from 69 to 99 belong to the twentieth century,
			 *          from 00 to 68 — to the twenty first one. That rule is set by the separate century
			 *          method, and the value of the window is not used with it.
			 * @note A zero window switches off the rule entirely: a two-digit year always
			 *       belongs to the twenty first century, as it was done before the appearance
			 *       of the rule
			 * @note A two-digit year is designated by the \%y and \%D format variables. The full
			 *       designation of the year by the \%Y variable the rule does not touch
			 * @note A record with a two-digit year is ambiguous by construction: the reverse reading of
			 *       a record built by the \%y variable gives the original year not always, but
			 *       only when it lies inside the window
			 * @param years window of a two-digit year in years, zero switches off the rule
			 * @see parse
			 * @see yearRollback
			 *
			 * @code{.cpp}
			 * chrono.parse("06-Nov-94 08:49:37", "%d-%b-%y %H:%M:%S");  // the year 1994
			 * chrono.parse("06-Nov-70 08:49:37", "%d-%b-%y %H:%M:%S");  // the year 2070
			 * @endcode
			 *
			 */
			void yearWindow(const uint8_t years) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения правила раскрытия двузначного года
			 *
			 * @return правило, действующее при разборе записей с двузначным годом
			 *
			 * @see century
			 *
			 * \~english
			 * @brief Method of getting the rule of the expansion of a two-digit year
			 * @return rule in force at the parsing of the records with a two-digit year
			 * @see century
			 *
			 * \~
			 */
			century_t century() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки правила раскрытия двузначного года
			 *
			 * @details Задаёт, какому столетию отнести год, записанный двумя разрядами.
			 *          Единого правила на этот счёт стандарты не дают, и оба действующих
			 *          доступны здесь.
			 *
			 *          Правило century_t::WINDOW отсчитывает скользящее окно от текущего
			 *          года, как того требует RFC 9110 (§5.6.7) для устаревшего формата
			 *          RFC 850: обозначение, попадающее дальше в будущее, чем допускает
			 *          окно yearWindow, читается ближайшим прошедшим годом с теми же
			 *          разрядами. Правило это действует по умолчанию и вместе с текущим
			 *          годом смещается.
			 *
			 *          Правило century_t::POSIX задаёт неподвижный рубеж, которым strptime
			 *          стандарта POSIX раскрывает переменную \%y: разряды от 69 до 99
			 *          относятся к двадцатому веку, от 00 до 68 - к двадцать первому.
			 *          Величина окна при этом правиле не используется.
			 *
			 * @note Правило POSIX относит разряды «69» к 1969 году, календарём модуля не
			 *       представимому: разбор приводит такую запись к началу эпохи, а проверка
			 *       записи её отвергает. Прочие разряды двадцатого века - от 70 до 99 -
			 *       представимы и читаются точно, чего скользящее окно не даёт ни при
			 *       какой его величине
			 * @note Правило затрагивает переменные формата \%y и \%D. Полное обозначение
			 *       года переменной \%Y им не задевается
			 * @param mode правило раскрытия двузначного года
			 * @see yearWindow
			 * @see parse
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // Скользящее окно при текущем 2026 годе относит «70» к 2070 году
			 * chrono.century(ch::century_t::WINDOW);
			 * chrono.parse("06-Nov-70 08:49:37", "%d-%b-%y %H:%M:%S");  // 2070 год
			 * // Неподвижный рубеж POSIX относит «70» к 1970 году
			 * chrono.century(ch::century_t::POSIX);
			 * chrono.parse("06-Nov-70 08:49:37", "%d-%b-%y %H:%M:%S");  // 1970 год
			 * @endcode
			 *
			 * \~english
			 * @brief Method of setting the rule of the expansion of a two-digit year
			 * @details Sets which century a year written with two digits should be attributed to.
			 *          The standards give no single rule on this account, and both the ones in force
			 *          are available here.
			 *          The century_t::WINDOW rule counts a sliding window from the current
			 *          year, exactly as RFC 9110 (§5.6.7) requires for the obsolete format
			 *          RFC 850: a designation falling further into the future than the yearWindow window
			 *          allows is read as the nearest past year with the same
			 *          digits. That rule is in force by default and moves together with the current
			 *          year.
			 *          The century_t::POSIX rule sets the immovable boundary by which strptime
			 *          of the POSIX standard expands the \%y variable: the digits from 69 to 99
			 *          belong to the twentieth century, from 00 to 68 — to the twenty first one.
			 *          The value of the window with this rule is not used.
			 * @note The POSIX rule attributes the digits «69» to the year 1969, not representable by the
			 *       calendar of the module: the parsing brings such a record to the beginning of the epoch, and the check
			 *       of the record rejects it. The other digits of the twentieth century — from 70 to 99 — are
			 *       representable and are read exactly, which the sliding window does not give at
			 *       any of its widths
			 * @note The rule touches the \%y and \%D format variables. The full designation
			 *       of the year by the \%Y variable is not affected by it
			 * @param mode rule of the expansion of a two-digit year
			 * @see yearWindow
			 * @see parse
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // The sliding window at the current year 2026 refers "70" to the year 2070
			 * chrono.century(ch::century_t::WINDOW);
			 * chrono.parse("06-Nov-70 08:49:37", "%d-%b-%y %H:%M:%S");  // the year 2070
			 * // The immovable boundary of POSIX refers "70" to the year 1970
			 * chrono.century(ch::century_t::POSIX);
			 * chrono.parse("06-Nov-70 08:49:37", "%d-%b-%y %H:%M:%S");  // the year 1970
			 * @endcode
			 *
			 */
			void century(const century_t mode) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения признака приёма секунды координации
			 *
			 * @return признак приёма секунды координации
			 *
			 * @see leapSecond
			 *
			 * \~english
			 * @brief Method of getting the sign of the acceptance of the leap second
			 * @return sign of the acceptance of the leap second
			 * @see leapSecond
			 *
			 * \~
			 */
			bool leapSecond() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки признака приёма секунды координации
			 *
			 * @details Задаёт, считать ли пригодной запись, где секунда равна шестидесяти.
			 *          Секунда эта - секунда координации, и её разрешают RFC 3339 вместе с
			 *          ISO 8601, а часть разборщиков отвергает наравне с ошибкой записи.
			 *          По умолчанию она принимается, как того и требуют стандарты.
			 *
			 * @note Признак затрагивает лишь проверку пригодности записи. Разбор принимает
			 *       секунду координации всегда и переносит её на первую секунду следующей
			 *       минуты, как это делают strptime и timegm
			 * @param mode признак приёма секунды координации
			 * @see validate
			 *
			 * @code{.cpp}
			 * chrono.validate("2016-12-31T23:59:60Z", "%Y-%m-%dT%H:%M:%S%i");  // true
			 * chrono.leapSecond(false);
			 * chrono.validate("2016-12-31T23:59:60Z", "%Y-%m-%dT%H:%M:%S%i");  // false
			 * @endcode
			 *
			 * \~english
			 * @brief Method of setting the sign of the acceptance of the leap second
			 * @details Sets whether a record where the second equals sixty should be considered fit.
			 *          That second is the leap second, and it is allowed by RFC 3339 together with
			 *          ISO 8601, and a part of the parsers rejects it on a par with an error of the record.
			 *          By default it is accepted, exactly as the standards require.
			 * @note The sign touches only the check of the fitness of a record. The parsing accepts
			 *       the leap second always and moves it to the first second of the next
			 *       minute, as strptime and timegm do
			 * @param mode sign of the acceptance of the leap second
			 * @see validate
			 *
			 * @code{.cpp}
			 * chrono.validate("2016-12-31T23:59:60Z", "%Y-%m-%dT%H:%M:%S%i");  // true
			 * chrono.leapSecond(false);
			 * chrono.validate("2016-12-31T23:59:60Z", "%Y-%m-%dT%H:%M:%S%i");  // false
			 * @endcode
			 *
			 */
			void leapSecond(const bool mode) noexcept;
		public:
		public:
			/**
			 * \~russian
			 * @brief Метод подсчёта количества десятичных разрядов числа
			 *
			 * @details Нулевое значение считается за один разряд
			 *
			 * @param value число для которого выполняется подсчёт разрядов
			 * @return      количество десятичных разрядов
			 *
			 * \~english
			 * @brief Method of counting the number of the decimal digits of a number
			 * @details A zero value counts as one digit
			 * @param value number the counting of the digits is performed for
			 * @return      number of the decimal digits
			 *
			 * \~
			 */
			uint8_t digits(const uint64_t value) const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод приведения числа месяца к его настоящей длине
			 *
			 * @details Длина месяца зависит и от самого месяца, и от високосности года,
			 *          поэтому число приводится к ней всякий раз, когда меняется месяц
			 *          либо год: иначе в объекте оседает несуществующее число вида
			 *          31 февраля, и сборка штампа времени переносит дату на март
			 *
			 * @param dt объект даты и времени для приведения
			 *
			 * \~english
			 * @brief Method of bringing the day of the month to its real length
			 * @details The length of a month depends both on the month itself and on the leapness of the year,
			 *          and therefore the day is brought to it every time the month or the year
			 *          changes: otherwise a non-existent day of the form
			 *          31st of February settles in the object, and the assembly of the timestamp moves the date to March
			 * @param dt object of the date and the time to bring
			 *
			 * \~
			 */
			void clampDay(dt_t & dt) const noexcept;
			/**
			 * \~russian
			 * @brief Метод переноса объекта даты на указанный момент времени
			 *
			 * @details Поля объекта лежат в его временной зоне, а штамп времени - в нулевой,
			 *          поэтому перенос идёт через неё: разложение в нулевой зоне и обратная
			 *          перекладка в зону объекта. Обозначение зоны при переносе сохраняется
			 *
			 * @param dt   объект даты и времени для переноса
			 * @param date штамп времени в миллисекундах, на который переносится объект
			 *
			 * \~english
			 * @brief Method of moving a date object to the specified moment of time
			 * @details The fields of the object lie in its time zone, and the timestamp — in the zero one,
			 *          and therefore the move goes through it: a decomposition in the zero zone and a reverse
			 *          shift into the zone of the object. The designation of the zone is preserved on the move
			 * @param dt   object of the date and the time to move
			 * @param date timestamp in milliseconds the object is moved to
			 *
			 * \~
			 */
			void moveDate(dt_t & dt, const uint64_t date) const noexcept;
			/**
			 * \~russian
			 * @brief Метод согласования полей объекта даты между собой
			 *
			 * @details Год, месяц и число месяца задают дату целиком, а номер дня в году,
			 *          номер недели, день недели и признак летнего времени из них выводятся.
			 *          Установка любого из задающих полей выводные оставляла прежними, и два
			 *          открытых способа прочитать один объект расходились: format их
			 *          вычислял заново, get читал поля как есть
			 *
			 * @param dt объект даты и времени для согласования
			 *
			 * \~english
			 * @brief Method of agreeing the fields of a date object with each other
			 * @details The year, the month and the day of the month set the date entirely, and the number of the day in the year,
			 *          the number of the week, the day of the week and the sign of the daylight saving time are derived from them.
			 *          Setting any of the defining fields left the derived ones the previous ones, and the two
			 *          public ways of reading one object diverged: format computed them
			 *          anew, get read the fields as they were
			 * @param dt object of the date and the time to agree
			 *
			 * \~
			 */
			void syncDate(dt_t & dt) const noexcept;
			/**
			 * \~russian
			 * @brief Метод разрешения смещения сводной временной зоны по самой записи
			 *
			 * @details Смещение сводных зон Северной Америки зависит от момента времени, и
			 *          потому пересчитывается всякий раз, когда меняется описываемая объектом
			 *          дата. Местные поля записи при этом остаются прежними: они и задают
			 *          местное время, а смещение из него выводится
			 *
			 * @param dt объект даты и времени для разрешения
			 *
			 * \~english
			 * @brief Method of resolving the offset of a composite time zone by the record itself
			 * @details The offset of the composite zones of North America depends on the moment of time, and
			 *          is therefore recomputed every time the date described by the object
			 *          changes. The local fields of the record at that remain the previous ones: it is they that set
			 *          the local time, and the offset is derived from it
			 * @param dt object of the date and the time to resolve
			 *
			 * \~
			 */
			void resolveZone(dt_t & dt) const noexcept;
			/**
			 * \~russian
			 * @brief Метод согласования выводных признаков объекта даты
			 *
			 * @details Метка времени суток и признак летнего времени задающими полями не
			 *          являются, а из них выводятся, и потому пересчитываются всякий раз,
			 *          когда дата объекта меняется. Установка часа, минут либо секунд
			 *          разложения штампа не вызывает вовсе, и признаки оставались от прежней
			 *          даты: час, выставленный пополудни поверх утреннего, метку времени
			 *          суток не менял, отчего переменная формата \%r печатала "03:00:00 AM"
			 *
			 * @param dt объект даты и времени для согласования
			 *
			 * \~english
			 * @brief Method of agreeing the derived signs of a date object
			 * @details The mark of the time of the day and the sign of the daylight saving time are not defining fields,
			 *          but are derived from them, and are therefore recomputed every time
			 *          the date of the object changes. Setting the hour, the minutes or the seconds
			 *          does not cause a decomposition of the timestamp at all, and the signs remained from the previous
			 *          date: an hour set in the afternoon over a morning one did not change the mark of the time
			 *          of the day, and therefore the \%r format variable printed "03:00:00 AM"
			 * @param dt object of the date and the time to agree
			 *
			 * \~
			 */
			void syncFlags(dt_t & dt) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения опорного момента времени для разрешения сводной зоны
			 *
			 * @details Опорой служит стандартное время зоны, а не текущее смещение объекта:
			 *          внутри часа перехода местное время либо повторяется, либо не существует
			 *          вовсе, и от текущего смещения ответ там зависел, отчего одна и та же
			 *          запись разрешалась по-разному в зависимости от того, что лежало в
			 *          объекте до вызова
			 *
			 * @param dt объект даты и времени
			 * @return   штамп времени в миллисекундах
			 *
			 * \~english
			 * @brief Method of getting the reference moment of time for the resolution of a composite zone
			 * @details The support is the standard time of the zone, and not the current offset of the object:
			 *          inside the hour of the transition the local time either repeats itself or does not exist
			 *          at all, and the answer there depended on the current offset, and therefore one and the same
			 *          record was resolved differently depending on what lay in the
			 *          object before the call
			 * @param dt object of the date and the time
			 * @return   timestamp in milliseconds
			 *
			 * \~
			 */
			uint64_t baseStamp(const dt_t & dt) const noexcept;
			/**
			 * \~russian
			 * @brief Метод определения летнего времени по местному времени зоны
			 *
			 * @details Правило перехода задано местным временем зоны, а не всемирным:
			 *          переход происходит в 02:00 по её стандартному времени. Прежде момент
			 *          раскладывался в нулевой зоне, и признак снимался с всемирных полей -
			 *          для восточного времени США это давало ошибку в пять часов, отчего
			 *          9 марта 2025 года зона отдавала летнее время уже с 23:00 предыдущих
			 *          суток по местному счёту
			 *
			 * @param date   штамп времени в миллисекундах
			 * @param offset смещение стандартного времени зоны в секундах
			 * @return       результат проверки
			 *
			 * \~english
			 * @brief Method of determining the daylight saving time by the local time of the zone
			 * @details The rule of the transition is set by the local time of the zone, and not by the universal one:
			 *          the transition happens at 02:00 by its standard time. Formerly the moment
			 *          was decomposed in the zero zone, and the sign was taken from the universal fields —
			 *          for the eastern time of the USA that gave an error of five hours, and therefore
			 *          on the 9th of March 2025 the zone gave back the daylight saving time already from 23:00 of the previous
			 *          day by the local count
			 * @param date   timestamp in milliseconds
			 * @param offset offset of the standard time of the zone in seconds
			 * @return       result of the check
			 *
			 * \~
			 */
			bool isDST(const uint64_t date, const int32_t offset) const noexcept;
			/**
			 * \~russian
			 * @brief Метод подсчёта количества високосных лет, прошедших с 1970 года
			 *
			 * @param years количество прошедших лет с 1970 года
			 * @return      количество високосных лет с учётом григорианского календаря
			 *
			 * \~english
			 * @brief Method of counting the number of the leap years elapsed since the year 1970
			 * @param years number of the years elapsed since the year 1970
			 * @return      number of the leap years with the Gregorian calendar taken into account
			 *
			 * \~
			 */
			uint16_t leapYears(const uint16_t years) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения штампа времени начала указанного года в миллисекундах
			 *
			 * @param year год для которого необходимо получить начало
			 * @return     штамп времени начала года в миллисекундах
			 *
			 * \~english
			 * @brief Method of getting the timestamp of the beginning of the specified year in milliseconds
			 * @param year year the beginning is needed to be obtained for
			 * @return     timestamp of the beginning of the year in milliseconds
			 *
			 * \~
			 */
			uint64_t beginOfYear(const uint16_t year) const noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения года из даты вместе с началом этого года
			 *
			 * @details Начало года извлечению года попутно: оно вычисляется по ходу дела и
			 *          прежде отбрасывалось, а вызывающая сторона получала его вторым
			 *          проходом того же расчёта. Обе величины выводятся здесь за один
			 *          проход, и перегрузка, начала года не принимающая, обращается сюда же
			 *
			 * @param date  штамп времени в миллисекундах
			 * @param begin штамп времени начала извлечённого года в миллисекундах
			 * @return      значение года, которому принадлежит дата
			 *
			 * @see beginOfYear
			 *
			 * \~english
			 * @brief Method of extracting the year from a date together with the beginning of that year
			 * @details The beginning of the year comes along with the extraction of the year: it is computed along the way and
			 *          formerly was discarded, and the calling side obtained it by a second
			 *          pass of the same computation. Both quantities are yielded here in one
			 *          pass, and the overload that does not take the beginning of the year addresses here as well
			 * @param date  timestamp in milliseconds
			 * @param begin timestamp of the beginning of the extracted year in milliseconds
			 * @return      value of the year the date belongs to
			 * @see beginOfYear
			 *
			 * \~
			 */
			uint16_t year(const uint64_t date, uint64_t & begin) const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод проверки действует ли летнее время (DST) по правилам США/Канады
			 *
			 * @param month номер месяца (1-12)
			 * @param date  число месяца (1-31)
			 * @param day   день недели (1 - понедельник, 7 - воскресенье)
			 * @param hour  количество часов (0-23)
			 * @return      результат проверки действия летнего времени
			 *
			 * \~english
			 * @brief Method of checking whether the daylight saving time (DST) is in force by the rules of the USA/Canada
			 * @param month number of the month (1-12)
			 * @param date  day of the month (1-31)
			 * @param day   day of the week (1 is Monday, 7 is Sunday)
			 * @param hour  number of the hours (0-23)
			 * @return      result of the check of the force of the daylight saving time
			 *
			 * \~
			 */
			bool isDST(const uint8_t month, const uint8_t date, const uint8_t day, const uint8_t hour) const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод получения штампа времени из объекта даты
			 *
			 * @param dt объект даты из которой необходимо получить штамп времени
			 * @return   штамп времени в миллисекундах
			 *
			 * \~english
			 * @brief Method of getting the timestamp from a date object
			 * @param dt date object the timestamp is needed to be obtained from
			 * @return   timestamp in milliseconds
			 *
			 * \~
			 */
			uint64_t makeDate(const dt_t & dt) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения штампа времени из объекта даты с учётом временной зоны
			 *
			 * @details Объект даты хранит поля в своей временной зоне, а смещение - таким,
			 *          каким его записывает сама запись даты: у московской зоны это +10800.
			 *          Сборка штампа времени смещение вычитает, приводя запись к UTC, и
			 *          принимает его потому с обратным знаком. Метод берёт на себя это
			 *          обращение знака: собирать штамп времени из объекта даты, минуя его,
			 *          значит сдвинуть время на величину зоны
			 *
			 * @param dt объект даты из которой необходимо получить штамп времени
			 * @return   штамп времени в миллисекундах
			 *
			 * \~english
			 * @brief Method of getting the timestamp from a date object with the time zone taken into account
			 * @details A date object holds the fields in its time zone, and the offset — such,
			 *          as the record of the date itself writes it: for the Moscow zone it is +10800.
			 *          The assembly of the timestamp subtracts the offset, bringing the record to UTC, and
			 *          therefore takes it with the opposite sign. The method takes this inversion of the sign
			 *          upon itself: to assemble a timestamp from a date object bypassing it
			 *          means to shift the time by the value of the zone
			 * @param dt date object the timestamp is needed to be obtained from
			 * @return   timestamp in milliseconds
			 *
			 * \~
			 */
			uint64_t makeStamp(const dt_t & dt) const noexcept;
			/**
			 * \~russian
			 * @brief Метод перекладки объекта даты в указанную временную зону
			 *
			 * @details Момент времени объект описывает один и тот же, меняется лишь зона,
			 *          в которой записаны его поля: объект даты 12:37 московской зоны
			 *          после перекладки в нулевую зону опишет то же мгновение как 09:37.
			 *          Прежде эта перекладка была расписана по десятку мест сборкой штампа
			 *          времени поверх самой себя, и объект, уже лежащий в своей зоне,
			 *          сдвигался ею повторно
			 *
			 * @param dt   объект даты который необходимо переложить
			 * @param zone смещение временной зоны в секундах
			 *
			 * \~english
			 * @brief Method of shifting a date object into the specified time zone
			 * @details The object describes one and the same moment of time, only the zone changes,
			 *          in which its fields are written: a date object of 12:37 of the Moscow zone
			 *          after the shift into the zero zone will describe the same instant as 09:37.
			 *          Formerly this shift was spelled out over a dozen places as an assembly of the timestamp
			 *          over itself, and an object already lying in its zone
			 *          was shifted by it once more
			 * @param dt   date object that needs to be shifted
			 * @param zone offset of the time zone in seconds
			 *
			 * \~
			 */
			void shiftDate(dt_t & dt, const int32_t zone) const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод раскрытия двузначного обозначения года в полное
			 *
			 * @details Правило раскрытия задаёт RFC 9110 (§5.6.7): обозначение, попадающее
			 *          дальше в будущее, чем допускает окно, относится к ближайшему
			 *          прошедшему году с теми же двумя разрядами
			 *
			 * @param value двузначное обозначение года
			 * @return      полное обозначение года
			 *
			 * \~english
			 * @brief Method of expanding a two-digit designation of a year into the full one
			 * @details The rule of the expansion is set by RFC 9110 (§5.6.7): a designation falling
			 *          further into the future than the window allows belongs to the nearest
			 *          past year with the same two digits
			 * @param value two-digit designation of a year
			 * @return      full designation of the year
			 *
			 * \~
			 */
			uint16_t makeFullYear(const uint16_t value) const noexcept;
			/**
			 * \~russian
			 * @brief Метод заполнения объекта даты из штампа времени
			 *
			 * @param date дата из которой необходимо заполнить объект
			 * @param dt   объект даты который необходимо заполнить
			 *
			 * \~english
			 * @brief Method of filling a date object from a timestamp
			 * @param date date the object is needed to be filled from
			 * @param dt   date object that needs to be filled
			 *
			 * \~
			 */
			void makeDate(const uint64_t date, dt_t & dt) const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод парсинга строки даты и времени в UnixTimestamp
			 *
			 * @details Общая реализация разбора для открытого метода parse и проверки
			 *          validate. Довод valid принимает признак пригодности записи, а при
			 *          нулевом указателе проверка пригодности не выполняется вовсе
			 *
			 * @param date    строка даты
			 * @param format  формат даты
			 * @param storage хранение значение времени
			 * @param valid   признак пригодности записи, ноль если проверка не нужна
			 * @return        дата в UnixTimestamp
			 *
			 * \~english
			 * @brief Method of parsing a string of a date and a time into a UnixTimestamp
			 * @details The common implementation of the parsing for the public parse method and for the validate
			 *          check. The valid argument takes the sign of the fitness of the record, and at
			 *          a null pointer the check of the fitness is not performed at all
			 * @param date    string of the date
			 * @param format  format of the date
			 * @param storage storage of the value of the time
			 * @param valid   sign of the fitness of the record, zero if the check is not needed
			 * @return        date as a UnixTimestamp
			 *
			 * \~
			 */
			uint64_t parse(string_view date, string_view format, const storage_t storage, bool * valid) noexcept;
			/**
			 * \~russian
			 * @brief Функция заполнения объекта даты и времени
			 *
			 * @param dt     объект даты и времени для заполнения
			 * @param text   текст в котором производится поиск
			 * @param format формат выполнения поиска
			 * @param pos    начальная позиция в тексте
			 * @return       конечная позиция обработанных данных в тексте
			 *
			 * \~english
			 * @brief Function of filling an object of a date and a time
			 * @param dt     object of the date and the time to fill
			 * @param text   text the search is performed in
			 * @param format format of the performance of the search
			 * @param pos    initial position in the text
			 * @return       final position of the processed data in the text
			 *
			 * \~
			 */
			ssize_t prepare(dt_t & dt, string_view text, const format_t format, const size_t pos = 0) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод перевода времени в аббревиатуру
			 *
			 * @details Подбирает наибольшую единицу измерения, в которой продолжительность
			 *          выражается числом не меньше единицы, и выводит её вместе со значением
			 *          в этой единице. Служит для показа продолжительностей человеку:
			 *          «2.14 месяца» читается лучше, чем «5184000000 миллисекунд».
			 *
			 * @note Довод - продолжительность в миллисекундах, а не момент времени: подбор
			 *       идёт от нуля, а не от эпохи
			 * @note Месяц здесь равен четырём неделям, а год - двенадцати таким месяцам, то
			 *       есть 336 суткам, тогда как перевод единиц времени в штамп считает месяц
			 *       средним по календарю, а год - тремястами шестьюдесятью пятью сутками.
			 *       Расхождение намеренное: аббревиатура служит показу продолжительности
			 *       человеку, и «два месяца» в ней означает восемь недель, а не отрезок
			 *       календаря. Для календарного счёта служат offset, begin, end и actual
			 * @param date дата в UnixTimestamp
			 * @return     сформированная аббревиатура даты
			 *
			 * @code{.cpp}
			 * chrono.abbreviation(500);          // MILLISECONDS, 500.0
			 * chrono.abbreviation(5000);         // SECONDS, 5.0
			 * chrono.abbreviation(7200000);      // HOUR, 2.0
			 * chrono.abbreviation(1209600000);   // WEEK, 2.0
			 * chrono.abbreviation(5184000000);   // MONTH, 2.142857
			 * @endcode
			 *
			 * \~english
			 * @brief Method of converting a time into an abbreviation
			 * @details Picks the largest unit of the measurement in which the duration
			 *          is expressed by a number not less than one, and yields it together with the value
			 *          in that unit. Serves for showing durations to a human:
			 *          «2.14 months» reads better than «5184000000 milliseconds».
			 * @note The argument is a duration in milliseconds, and not a moment of time: the picking
			 *       goes from zero, and not from the epoch
			 * @note A month here equals four weeks, and a year — twelve such months, that
			 *       is 336 days, while the conversion of the units of time into a timestamp counts a month
			 *       as an average one by the calendar, and a year — as three hundred and sixty five days.
			 *       The divergence is deliberate: the abbreviation serves for showing a duration
			 *       to a human, and «two months» in it means eight weeks, and not an interval
			 *       of the calendar. For the calendar count offset, begin, end and actual serve
			 * @param date date as a UnixTimestamp
			 * @return     the built abbreviation of the date
			 *
			 * @code{.cpp}
			 * chrono.abbreviation(500);          // MILLISECONDS, 500.0
			 * chrono.abbreviation(5000);         // SECONDS, 5.0
			 * chrono.abbreviation(7200000);      // HOUR, 2.0
			 * chrono.abbreviation(1209600000);   // WEEK, 2.0
			 * chrono.abbreviation(5184000000);   // MONTH, 2.142857
			 * @endcode
			 *
			 */
			std::pair <type_t, double> abbreviation(const uint64_t date) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения конца позиции указанной даты
			 *
			 * @details Выводит границу календарного отрезка, в который попадает дата.
			 *          Граница исключающая: она совпадает с началом следующего отрезка, а не
			 *          с последней его миллисекундой. Отрезок задаётся полуинтервалом
			 *          [begin, end), поэтому его длительность равна разности границ, а
			 *          проверка попадания записывается как (date >= begin) && (date < end).
			 *
			 * @note Неделя отсчитывается с понедельника, поэтому у воскресенья конец недели
			 *       совпадает с концом суток
			 * @param date дата для которой необходимо получить позицию
			 * @param type тип единиц измерений даты
			 * @return     конец указанной даты в формате UnixTimestamp
			 * @see begin
			 *
			 * @code{.cpp}
			 * // 2025-04-06T12:37:01.520Z, воскресенье
			 * const uint64_t date = 1743943021520;
			 * chrono.begin(date, awh::chrono_t::type_t::DAY);   // 2025-04-06T00:00:00
			 * chrono.end(date, awh::chrono_t::type_t::DAY);     // 2025-04-07T00:00:00
			 * chrono.end(date, awh::chrono_t::type_t::WEEK);    // 2025-04-07T00:00:00
			 * chrono.end(date, awh::chrono_t::type_t::MONTH);   // 2025-05-01T00:00:00
			 * chrono.end(date, awh::chrono_t::type_t::YEAR);    // 2026-01-01T00:00:00
			 * @endcode
			 *
			 * \~english
			 * @brief Method of getting the end of the position of the specified date
			 * @details Yields the boundary of the calendar interval the date falls into.
			 *          The boundary is an exclusive one: it coincides with the beginning of the next interval, and not
			 *          with its last millisecond. The interval is set by the half-interval
			 *          [begin, end), and therefore its length equals the difference of the boundaries, and
			 *          the check of the falling into it is written as (date >= begin) && (date < end).
			 * @note The week is counted from Monday, and therefore for a Sunday the end of the week
			 *       coincides with the end of the day
			 * @param date date the position is needed to be obtained for
			 * @param type type of the units of the measurements of the date
			 * @return     end of the specified date in the UnixTimestamp format
			 * @see begin
			 *
			 * @code{.cpp}
			 * // 2025-04-06T12:37:01.520Z, a Sunday
			 * const uint64_t date = 1743943021520;
			 * chrono.begin(date, awh::chrono_t::type_t::DAY);   // 2025-04-06T00:00:00
			 * chrono.end(date, awh::chrono_t::type_t::DAY);     // 2025-04-07T00:00:00
			 * chrono.end(date, awh::chrono_t::type_t::WEEK);    // 2025-04-07T00:00:00
			 * chrono.end(date, awh::chrono_t::type_t::MONTH);   // 2025-05-01T00:00:00
			 * chrono.end(date, awh::chrono_t::type_t::YEAR);    // 2026-01-01T00:00:00
			 * @endcode
			 *
			 */
			uint64_t end(const uint64_t date, const type_t type) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения конца позиции текущей даты
			 *
			 * @details Граница местного хранилища отсчитывается в зоне объекта: конец суток
			 *          записи, лежащей в зоне UTC+3, приходится на её местную полночь, а не на
			 *          полночь нулевой зоны. Выдаётся при этом штамп времени, а он зоны не
			 *          несёт и отсчитывается от начала эпохи в нулевой, как и всякий другой.
			 *          Граница общего хранилища отсчитывается в зоне нулевой.
			 *
			 * @param type    тип единиц измерений даты
			 * @param storage хранение значение времени
			 * @return        конец текущей даты в формате UnixTimestamp
			 *
			 * \~english
			 * @brief Method of getting the end of the position of the current date
			 * @details The boundary of the local storage is counted in the zone of the object: the end of the day of
			 *          a record lying in the UTC+3 zone falls on its local midnight, and not on
			 *          the midnight of the zero zone. What is yielded at that is a timestamp, and it carries no zone
			 *          and is counted from the beginning of the epoch in the zero one, as any other one.
			 *          The boundary of the global storage is counted in the zero zone.
			 * @param type    type of the units of the measurements of the date
			 * @param storage storage of the value of the time
			 * @return        end of the current date in the UnixTimestamp format
			 *
			 * \~
			 */
			uint64_t end(const type_t type, const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения начала позиции указанной даты
			 *
			 * @details Выводит начало календарного отрезка, в который попадает дата. Граница
			 *          включающая: сама дата начала отрезка попадает в отрезок. Вместе с end
			 *          задаёт полуинтервал [begin, end).
			 *
			 * @param date дата для которой необходимо получить позицию
			 * @param type тип единиц измерений даты
			 * @return     начало указанной даты в формате UnixTimestamp
			 * @see end
			 *
			 * @code{.cpp}
			 * // Начало суток нужной даты - частая основа группировки записей журнала
			 * const uint64_t day = chrono.begin(date, awh::chrono_t::type_t::DAY);
			 * @endcode
			 *
			 * \~english
			 * @brief Method of getting the beginning of the position of the specified date
			 * @details Yields the beginning of the calendar interval the date falls into. The boundary is
			 *          an inclusive one: the very date of the beginning of the interval falls into the interval. Together with end
			 *          it sets the half-interval [begin, end).
			 * @param date date the position is needed to be obtained for
			 * @param type type of the units of the measurements of the date
			 * @return     beginning of the specified date in the UnixTimestamp format
			 * @see end
			 *
			 * @code{.cpp}
			 * // The beginning of the day of the needed date is a frequent basis of the grouping of the records of a log
			 * const uint64_t day = chrono.begin(date, awh::chrono_t::type_t::DAY);
			 * @endcode
			 *
			 */
			uint64_t begin(const uint64_t date, const type_t type) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения начала позиции текущей даты
			 *
			 * @details Граница местного хранилища отсчитывается в зоне объекта: начало суток
			 *          записи, лежащей в зоне UTC+3, приходится на её местную полночь, а не на
			 *          полночь нулевой зоны. Выдаётся при этом штамп времени, а он зоны не
			 *          несёт и отсчитывается от начала эпохи в нулевой, как и всякий другой.
			 *          Граница общего хранилища отсчитывается в зоне нулевой.
			 *
			 * @param type    тип единиц измерений даты
			 * @param storage хранение значение времени
			 * @return        начало текущей даты в формате UnixTimestamp
			 *
			 * \~english
			 * @brief Method of getting the beginning of the position of the current date
			 * @details The boundary of the local storage is counted in the zone of the object: the beginning of the day of
			 *          a record lying in the UTC+3 zone falls on its local midnight, and not on
			 *          the midnight of the zero zone. What is yielded at that is a timestamp, and it carries no zone
			 *          and is counted from the beginning of the epoch in the zero one, as any other one.
			 *          The boundary of the global storage is counted in the zero zone.
			 * @param type    type of the units of the measurements of the date
			 * @param storage storage of the value of the time
			 * @return        beginning of the current date in the UnixTimestamp format
			 *
			 * \~
			 */
			uint64_t begin(const type_t type, const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод актуализации прошедшего и оставшегося времени
			 *
			 * @details Считает, сколько единиц value осталось до конца отрезка type либо
			 *          сколько их прошло от его начала. Довод value задаёт единицу счёта,
			 *          довод type - отрезок, внутри которого ведётся счёт.
			 *
			 * @note Считаются целые единицы: неполная единица в счёт не идёт. Сумма
			 *       прошедших и оставшихся единиц поэтому может быть на единицу меньше
			 *       полной их величины в отрезке
			 * @note Недели - исключение: они округляются до ближайшего целого, как и
			 *       единица unit_t::WEEKS, поэтому неполная неделя от половины и выше
			 *       засчитывается целой
			 * @param date   дата относительно которой производятся расчёты
			 * @param value  тип определяемых единиц измерений времени
			 * @param type   тип единиц измерений даты
			 * @param actual направление актуализации
			 * @return       результат вычисления
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // 2025-04-06, 96-й день года
			 * chrono.actual(date, ch::type_t::DAY, ch::type_t::YEAR, ch::actual_t::LEFT);    // 269
			 * chrono.actual(date, ch::type_t::DAY, ch::type_t::YEAR, ch::actual_t::PASSED);  // 95
			 * chrono.actual(date, ch::type_t::HOUR, ch::type_t::DAY, ch::actual_t::LEFT);    // 11
			 * chrono.actual(date, ch::type_t::MONTH, ch::type_t::YEAR, ch::actual_t::LEFT);  // 8
			 * @endcode
			 *
			 * \~english
			 * @brief Method of the actualization of the elapsed and of the remaining time
			 * @details Counts how many units of value have remained until the end of the interval of type or
			 *          how many of them have elapsed from its beginning. The value argument sets the unit of the count,
			 *          the type argument — the interval the count is kept inside of.
			 * @note Whole units are counted: an incomplete unit does not count. The sum of the
			 *       elapsed and of the remaining units may therefore be one less than
			 *       their full quantity in the interval
			 * @note The weeks are an exception: they are rounded to the nearest whole, as the
			 *       unit_t::WEEKS unit is, and therefore an incomplete week from a half and above
			 *       counts as a whole one
			 * @param date   date the computations are performed relative to
			 * @param value  type of the determined units of the measurements of the time
			 * @param type   type of the units of the measurements of the date
			 * @param actual direction of the actualization
			 * @return       result of the computation
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // 2025-04-06, the 96th day of the year
			 * chrono.actual(date, ch::type_t::DAY, ch::type_t::YEAR, ch::actual_t::LEFT);    // 269
			 * chrono.actual(date, ch::type_t::DAY, ch::type_t::YEAR, ch::actual_t::PASSED);  // 95
			 * chrono.actual(date, ch::type_t::HOUR, ch::type_t::DAY, ch::actual_t::LEFT);    // 11
			 * chrono.actual(date, ch::type_t::MONTH, ch::type_t::YEAR, ch::actual_t::LEFT);  // 8
			 * @endcode
			 *
			 */
			uint64_t actual(const uint64_t date, const type_t value, const type_t type, const actual_t actual) const noexcept;
			/**
			 * \~russian
			 * @brief Метод актуализации прошедшего и оставшегося времени
			 *
			 * @param value   тип определяемых единиц измерений времени
			 * @param type    тип единиц измерений даты
			 * @param actual  направление актуализации
			 * @param storage хранение значение времени
			 * @return        результат вычисления
			 *
			 * \~english
			 * @brief Method of the actualization of the elapsed and of the remaining time
			 * @param value   type of the determined units of the measurements of the time
			 * @param type    type of the units of the measurements of the date
			 * @param actual  direction of the actualization
			 * @param storage storage of the value of the time
			 * @return        result of the computation
			 *
			 * \~
			 */
			uint64_t actual(const type_t value, const type_t type, const actual_t actual, const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод смещения на указанное количество единиц времени
			 *
			 * @details Двигает дату на заданное количество единиц вперёд либо назад.
			 *          Смещение на единицы постоянной длительности (от недели и мельче) -
			 *          это сложение штампов, смещение на месяцы и годы требует разложения
			 *          даты и обратной её сборки.
			 *
			 * @note Смещение на месяцы необратимо: 31 января со смещением на месяц вперёд
			 *       даёт 28 февраля, а обратное смещение возвращает 28 января, а не 31-е.
			 *       Так же поступают все календарные библиотеки - иного корректного
			 *       определения у этой операции нет
			 * @note Смещение на микросекунды и наносекунды меняет размерность ответа:
			 *       довод принимается штампом времени в миллисекундах, а такой штамп
			 *       доли миллисекунды не представляет, поэтому результат выдаётся в
			 *       микросекундах либо наносекундах соответственно и в миллисекундных
			 *       расчётах напрямую не участвует
			 * @param date   дата относительно которой производится смещение, в миллисекундах
			 * @param value  значение на которое производится смещение
			 * @param type   тип единиц измерений даты
			 * @param offset направление смещения
			 * @return       результат вычисления в формате UnixTimestamp, в миллисекундах
			 *               для всех единиц крупнее микросекунды
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * const uint64_t jan31 = chrono.parse("2025-01-31", "%Y-%m-%d");
			 * // Число месяца ограничивается последним днём конечного месяца
			 * chrono.offset(jan31, 1, ch::type_t::MONTH, ch::offset_t::INCREMENT);   // 2025-02-28
			 * chrono.offset(jan31, 13, ch::type_t::MONTH, ch::offset_t::INCREMENT);  // 2026-02-28
			 * chrono.offset(date, 90, ch::type_t::DAY, ch::offset_t::INCREMENT);     // 2025-07-05
			 * @endcode
			 *
			 * @code{.cpp}
			 * chrono.offset(1743943021000, 1, ch::type_t::SECONDS, ch::offset_t::INCREMENT);
			 * // 1743943022000, миллисекунды
			 * chrono.offset(1743943021000, 1, ch::type_t::MICROSECONDS, ch::offset_t::INCREMENT);
			 * // 1743943021000001, микросекунды
			 * @endcode
			 *
			 * \~english
			 * @brief Method of the offset by the specified number of the units of time
			 * @details Moves a date by the given number of the units forward or backward.
			 *          An offset by the units of a constant length (from a week and finer) is
			 *          an addition of timestamps, an offset by months and years requires a decomposition
			 *          of the date and its reverse assembly.
			 * @note An offset by months is irreversible: the 31st of January with an offset by a month forward
			 *       gives the 28th of February, and the reverse offset returns the 28th of January, and not the 31st.
			 *       So do all the calendar libraries — there is no other correct
			 *       definition of this operation
			 * @note An offset by microseconds and nanoseconds changes the dimension of the answer:
			 *       the argument is taken as a timestamp in milliseconds, and such a timestamp
			 *       does not represent fractions of a millisecond, and therefore the result is yielded in
			 *       microseconds or nanoseconds respectively and does not participate in millisecond
			 *       computations directly
			 * @param date   date the offset is performed relative to, in milliseconds
			 * @param value  value the offset is performed by
			 * @param type   type of the units of the measurements of the date
			 * @param offset direction of the offset
			 * @return       result of the computation in the UnixTimestamp format, in milliseconds
			 *               for all the units coarser than a microsecond
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * const uint64_t jan31 = chrono.parse("2025-01-31", "%Y-%m-%d");
			 * // The day of the month is limited by the last day of the resulting month
			 * chrono.offset(jan31, 1, ch::type_t::MONTH, ch::offset_t::INCREMENT);   // 2025-02-28
			 * chrono.offset(jan31, 13, ch::type_t::MONTH, ch::offset_t::INCREMENT);  // 2026-02-28
			 * chrono.offset(date, 90, ch::type_t::DAY, ch::offset_t::INCREMENT);     // 2025-07-05
			 * @endcode
			 *
			 * @code{.cpp}
			 * chrono.offset(1743943021000, 1, ch::type_t::SECONDS, ch::offset_t::INCREMENT);
			 * // 1743943022000, milliseconds
			 * chrono.offset(1743943021000, 1, ch::type_t::MICROSECONDS, ch::offset_t::INCREMENT);
			 * // 1743943021000001, microseconds
			 * @endcode
			 *
			 */
			uint64_t offset(const uint64_t date, const uint64_t value, const type_t type, const offset_t offset) const noexcept;
			/**
			 * \~russian
			 * @brief Метод смещения текущей даты на указанное количество единиц времени
			 *
			 * @param value   значение на которое производится смещение
			 * @param type    тип единиц измерений даты
			 * @param offset  направление смещения
			 * @param storage хранение значение времени
			 * @return        результат вычисления в формате UnixTimestamp
			 *
			 * \~english
			 * @brief Method of the offset of the current date by the specified number of the units of time
			 * @param value   value the offset is performed by
			 * @param type    type of the units of the measurements of the date
			 * @param offset  direction of the offset
			 * @param storage storage of the value of the time
			 * @return        result of the computation in the UnixTimestamp format
			 *
			 * \~
			 */
			uint64_t offset(const uint64_t value, const type_t type, const offset_t offset, const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения текстового значения времени
			 *
			 * @details Записывает продолжительность наибольшей единицей, в которой она
			 *          выражается числом не меньше единицы. Единица обозначается одним
			 *          символом: s - секунды, m - минуты, h - часы, d - сутки, w - недели,
			 *          M - месяцы, y - годы. Обратное преобразование делает одноимённая
			 *          перегрузка, принимающая строку.
			 *
			 * @note Дробная часть округляется до сотых: обозначение предназначено для
			 *       чтения человеком, и полная разрядность двоичного числа давала бы для
			 *       тридцати суток 4.285714285714286w вместо 4.29w. Обратное чтение
			 *       округлённого обозначения точного исходного значения поэтому не даёт
			 * @note Отрицательная продолжительность выводится со знаком, как и читается:
			 * @param seconds количество секунд для конвертации
			 * @return        обозначение времени с указанием размерности
			 *
			 * @code{.cpp}
			 * chrono.seconds(45.0);       // 45s
			 * chrono.seconds(2592000.0);  // 4.29w
			 * chrono.seconds(90.0);       // 1.5m
			 * chrono.seconds(5400.0);     // 1.5h
			 * chrono.seconds(86400.0);    // 1d
			 * chrono.seconds(604800.0);   // 1w
			 * chrono.seconds(31536000.0); // 1y
			 * @endcode
			 *
			 * @code{.cpp}
			 * chrono.seconds(-7200.0);   // -2h
			 * chrono.seconds(-2592000.0); // -4.29w
			 * @endcode
			 *
			 * \~english
			 * @brief Method of getting the text value of a time
			 * @details Writes a duration by the largest unit in which it
			 *          is expressed by a number not less than one. The unit is designated by one
			 *          character: s is the seconds, m is the minutes, h is the hours, d is the days, w is the weeks,
			 *          M is the months, y is the years. The reverse conversion is done by the overload of the same name
			 *          taking a string.
			 * @note The fractional part is rounded to the hundredths: the designation is meant for
			 *       reading by a human, and the full width of a binary number would give for
			 *       thirty days 4.285714285714286w instead of 4.29w. The reverse reading of
			 *       a rounded designation therefore does not give the exact original value
			 * @note A negative duration is yielded with a sign, as it is read as well:
			 * @param seconds number of the seconds to convert
			 * @return        designation of the time with the dimension specified
			 *
			 * @code{.cpp}
			 * chrono.seconds(45.0);       // 45s
			 * chrono.seconds(2592000.0);  // 4.29w
			 * chrono.seconds(90.0);       // 1.5m
			 * chrono.seconds(5400.0);     // 1.5h
			 * chrono.seconds(86400.0);    // 1d
			 * chrono.seconds(604800.0);   // 1w
			 * chrono.seconds(31536000.0); // 1y
			 * @endcode
			 *
			 * @code{.cpp}
			 * chrono.seconds(-7200.0);   // -2h
			 * chrono.seconds(-2592000.0); // -4.29w
			 * @endcode
			 *
			 */
			string seconds(const double seconds) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получения размера в секундах из строки
			 *
			 * @details Переводит обозначение продолжительности в секунды. Им удобно задавать
			 *          сроки жизни соединений, таймауты и сроки хранения в настройках: «90m»
			 *          читается лучше, чем «5400».
			 *
			 * @note Обозначение занимает запись целиком: число обязано стоять в её начале,
			 *       а единица размерности - в её конце, и записи «42», «1,5h», «timeout=90m»
			 *       и « 90m» дают ноль. Пробел между числом и единицей допускается («90 m»).
			 *       Дробная часть отделяется точкой, запятая не принимается. Месяц и год
			 *       берутся средней длительности - 30.436875 и 365 суток соответственно
			 * @note Обозначение состоит из одного числа и одной единицы: составные записи
			 *       вида «1h30m» и «1w2d3h15m30s» не предусмотрены и дают ноль. Прежде
			 *       разбор искал число с любой позиции записи и брал у составной один лишь
			 *       её хвост, отбрасывая начало молча, а проверка такую запись одобряла
			 * @param value строка обозначения размерности (s, m, h, d, w, M, y)
			 * @return      размер в секундах
			 *
			 * @code{.cpp}
			 * chrono.seconds("45s");   // 45
			 * chrono.seconds("90m");   // 5400
			 * chrono.seconds("1.5h");  // 5400
			 * chrono.seconds("2d");    // 172800
			 * chrono.seconds("1w");    // 604800
			 * chrono.seconds("3M");    // 7889238, месяц равен 30.436875 суток
			 * chrono.seconds("1y");    // 31536000
			 * @endcode
			 *
			 * \~english
			 * @brief Method of getting the size in seconds from a string
			 * @details Converts a designation of a duration into seconds. It is convenient to set with it
			 *          the lifetimes of the connections, the timeouts and the storage times in the settings: «90m»
			 *          reads better than «5400».
			 * @note The designation occupies the record entirely: the number is obliged to stand at its beginning,
			 *       and the unit of the dimension — at its end, and the records «42», «1,5h», «timeout=90m»
			 *       and « 90m» give zero. A space between the number and the unit is allowed («90 m»).
			 *       The fractional part is separated by a dot, a comma is not accepted. The month and the year
			 *       are taken of the average length — 30.436875 and 365 days respectively
			 * @note The designation consists of one number and one unit: compound records
			 *       of the form «1h30m» and «1w2d3h15m30s» are not provided for and give zero. Formerly
			 *       the parsing searched for a number from any position of the record and took from a compound one only
			 *       its tail, discarding the beginning silently, and the check approved such a record
			 * @param value string of the designation of the dimension (s, m, h, d, w, M, y)
			 * @return      size in seconds
			 *
			 * @code{.cpp}
			 * chrono.seconds("45s");   // 45
			 * chrono.seconds("90m");   // 5400
			 * chrono.seconds("1.5h");  // 5400
			 * chrono.seconds("2d");    // 172800
			 * chrono.seconds("1w");    // 604800
			 * chrono.seconds("3M");    // 7889238, a month equals 30.436875 days
			 * chrono.seconds("1y");    // 31536000
			 * @endcode
			 *
			 */
			double seconds(string_view value) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения статуса 12-и часового формата времени
			 *
			 * @details Определяет половину суток, к которой относится дата: часы от 0 до 11
			 *          дают AM, от 12 до 23 - PM. Полночь относится к AM, полдень - к PM.
			 *
			 * @param date дата для проверки
			 * @return     половина суток, к которой относится дата
			 *
			 * \~english
			 * @brief Method of extracting the status of the 12-hour format of the time
			 * @details Determines the half of the day the date belongs to: the hours from 0 to 11
			 *          give AM, from 12 to 23 — PM. The midnight belongs to AM, the noon — to PM.
			 * @param date date to check
			 * @return     half of the day the date belongs to
			 *
			 * \~
			 */
			h12_t h12(const uint64_t date) const noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения текущего статуса 12-и часового формата времени
			 *
			 * @param storage хранение значение времени
			 * @return        текущее установленное значение статуса 12-и часового формата времени
			 *
			 * \~english
			 * @brief Method of extracting the current status of the 12-hour format of the time
			 * @param storage storage of the value of the time
			 * @return        current set value of the status of the 12-hour format of the time
			 *
			 * \~
			 */
			h12_t h12(const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод извлечения значения года
			 *
			 * @details Выводит полное обозначение года, к которому относится дата. Год
			 *          определяется в нулевой временной зоне, без учёта смещения зоны окружения.
			 *
			 * @note Штамп времени за пределом 9999 года выводит наибольший представимый
			 *       год - 9999: год записывается четырьмя разрядами во всех стандартах,
			 *       которым модуль служит, и хранится полем разрядностью в два октета
			 *
			 * @param date дата для извлечения года
			 * @return     полное обозначение года
			 *
			 * \~english
			 * @brief Method of extracting the value of the year
			 * @details Yields the full designation of the year the date belongs to. The year
			 *          is determined in the zero time zone, without the offset of the environment zone taken into account.
			 * @note A timestamp beyond the limit of the year 9999 yields the largest representable
			 *       year — 9999: the year is written with four digits in all the standards
			 *       the module serves, and is held by a field two octets wide
			 * @param date date to extract the year from
			 * @return     full designation of the year
			 *
			 * \~
			 */
			uint16_t year(const uint64_t date) const noexcept;
			/**
			 * \~russian
			 * @brief Метод получение текущего значения года
			 *
			 * @param storage хранение значение времени
			 * @return        текущее значение года
			 *
			 * \~english
			 * @brief Method of getting the current value of the year
			 * @param storage storage of the value of the time
			 * @return        current value of the year
			 *
			 * \~
			 */
			uint16_t year(const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки действует ли на дату летнее время (DST)
			 *
			 * @details Проверка ведётся по правилам США и Канады, действующим с 2007 года:
			 *          летнее время начинается во второе воскресенье марта в 02:00 и
			 *          заканчивается в первое воскресенье ноября в 02:00.
			 *
			 * @note Правила перехода у каждой страны свои, и к европейским, австралийским
			 *       или южноамериканским зонам эта проверка неприменима. Модуль базой
			 *       временных зон операционной системы не пользуется и потому чужих правил
			 *       не знает
			 *
			 * @param date дата для проверки
			 * @return     результат проверки
			 *
			 * \~english
			 * @brief Method of checking whether the daylight saving time (DST) is in force on a date
			 * @details The check is performed by the rules of the USA and Canada in force since the year 2007:
			 *          the daylight saving time begins on the second Sunday of March at 02:00 and
			 *          ends on the first Sunday of November at 02:00.
			 * @note The rules of the transition are their own for every country, and this check is inapplicable
			 *       to the European, Australian or South American zones. The module does not use the time zone
			 *       database of the operating system and therefore does not know foreign rules
			 * @param date date to check
			 * @return     result of the check
			 *
			 * \~
			 */
			bool dst(const uint64_t date) const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки действует ли летнее время (DST)
			 *
			 * @details Признак местного хранилища берётся у самого объекта и потому судит по
			 *          его временной зоне: у сводных зон Северной Америки - zone_t::AT,
			 *          zone_t::CT, zone_t::ET, zone_t::MT, zone_t::NT и zone_t::PT - он
			 *          выводится из разрешённого смещения и означает ровно то, что зона стоит
			 *          на летнем времени. Признак общего хранилища раскладывает штамп времени
			 *          в нулевой зоне.
			 *
			 * @note У зон с постоянным смещением - zone_t::UTC, zone_t::MSK, zone_t::EST и
			 *       прочих - признак остаётся ответом на вопрос «действует ли летнее время по
			 *       правилам США и Канады на эту гражданскую дату», а не свойством самой зоны:
			 *       смещение таких зон от времени года не зависит вовсе, и признак у них
			 *       летом истинен. Летнее время зоны выводится из её смещения, а не отсюда:
			 *       сличением getTimeZone с датой и без неё
			 *
			 * @param storage хранение значение времени
			 * @return        результат проверки
			 *
			 * @see getTimeZone(const zone_t, const uint64_t)
			 *
			 * \~english
			 * @brief Method of checking whether the daylight saving time (DST) is in force
			 * @details The sign of the local storage is taken from the very object and therefore judges by
			 *          its time zone: for the composite zones of North America — zone_t::AT,
			 *          zone_t::CT, zone_t::ET, zone_t::MT, zone_t::NT and zone_t::PT — it
			 *          is derived from the resolved offset and means exactly that the zone stands
			 *          on the daylight saving time. The sign of the global storage decomposes the timestamp
			 *          in the zero zone.
			 * @note For the zones with a constant offset — zone_t::UTC, zone_t::MSK, zone_t::EST and
			 *       the rest — the sign remains an answer to the question «is the daylight saving time in force by
			 *       the rules of the USA and Canada on this civil date», and not a property of the zone itself:
			 *       the offset of such zones does not depend on the season at all, and their sign
			 *       is true in the summer. The daylight saving time of a zone is derived from its offset, and not from here:
			 *       by matching getTimeZone with a date and without one
			 * @param storage storage of the value of the time
			 * @return        result of the check
			 * @see getTimeZone(const zone_t, const uint64_t)
			 *
			 * \~
			 */
			bool dst(const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки является ли год високосным
			 *
			 * @details Проверка ведётся по правилам григорианского календаря: год високосен,
			 *          если делится на четыре, но не на сто, либо делится на четыреста.
			 *          Поэтому 2000-й год високосен, а 1900-й и 2100-й - нет.
			 *
			 * @param year год для проверки
			 * @return     результат проверки
			 *
			 * \~english
			 * @brief Method of checking whether a year is a leap one
			 * @details The check is performed by the rules of the Gregorian calendar: a year is a leap one,
			 *          if it is divisible by four, but not by a hundred, or is divisible by four hundred.
			 *          Therefore the year 2000 is a leap one, and 1900 and 2100 are not.
			 * @param year year to check
			 * @return     result of the check
			 *
			 * \~
			 */
			bool leap(const uint16_t year) const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки является ли год високосным
			 *
			 * @details Проверяет год, к которому относится дата. Отличается от одноимённой
			 *          перегрузки доводом: та принимает номер года, эта - штамп времени.
			 *
			 * @note Обе перегрузки принимают целое число, поэтому целочисленный литерал
			 *       подходит им одинаково и делает вызов неоднозначным. Довод следует
			 *       передавать переменной либо приводить явно:
			 * @param date дата для проверки
			 * @return     результат проверки
			 *
			 * @code{.cpp}
			 * chrono.leap(2024);                              // не собирается
			 * chrono.leap(static_cast <uint16_t> (2024));     // проверка номера года
			 * chrono.leap(static_cast <uint64_t> (date));     // проверка года даты
			 * @endcode
			 *
			 * \~english
			 * @brief Method of checking whether a year is a leap one
			 * @details Checks the year the date belongs to. Differs from the overload of the same name
			 *          by the argument: that one takes the number of the year, this one — a timestamp.
			 * @note Both overloads take an integer, and therefore an integer literal
			 *       suits them equally and makes the call ambiguous. The argument should
			 *       be passed by a variable or cast explicitly:
			 * @param date date to check
			 * @return     result of the check
			 *
			 * @code{.cpp}
			 * chrono.leap(2024);                              // does not build
			 * chrono.leap(static_cast <uint16_t> (2024));     // the check of the number of the year
			 * chrono.leap(static_cast <uint64_t> (date));     // the check of the year of the date
			 * @endcode
			 *
			 */
			bool leap(const uint64_t date) const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки является ли текущий год високосным
			 *
			 * @param storage хранение значение времени
			 * @return        результат проверки
			 *
			 * \~english
			 * @brief Method of checking whether the current year is a leap one
			 * @param storage storage of the value of the time
			 * @return        result of the check
			 *
			 * \~
			 */
			bool leap(const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода установки данных даты и времени
			 *
			 * @tparam T тип данных в котором устанавливаются данные
			 *
			 * \~english
			 * @brief Template of the method of setting the data of a date and a time
			 * @tparam T type of the data the data is set in
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод установки данных даты и времени
			 *
			 * @details Изменяет отдельную составляющую внутреннего объекта даты - хранилища
			 *          storage_t::LOCAL. Системные часы (storage_t::GLOBAL) не затрагиваются.
			 *          Позволяет собрать дату по частям либо поправить одну составляющую
			 *          разобранной записи.
			 *
			 * @note Метод изменяет состояние объекта: согласование доступа при работе из
			 *       нескольких потоков лежит на том, кто объектом пользуется
			 * @param date дата для обработки
			 * @param unit элементы данных для установки
			 * @see timestamp
			 * @see clear
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // Кладём разобранную дату во внутренний объект
			 * chrono.timestamp(1743943021520, ch::type_t::MILLISECONDS);
			 * // Правим отдельные её составляющие
			 * chrono.set <uint16_t> (2030, ch::unit_t::YEAR);
			 * chrono.set <std::string> ("Dec", ch::unit_t::MONTH);
			 * // Читаем получившееся из локального хранилища
			 * chrono.format("%Y-%m-%d", ch::storage_t::LOCAL);  // 2030-12-06
			 * @endcode
			 *
			 * \~english
			 * @brief Method of setting the data of a date and a time
			 * @details Changes a separate part of the internal date object — of the storage_t::LOCAL
			 *          storage. The system clock (storage_t::GLOBAL) is not touched.
			 *          Allows a date to be assembled piece by piece or one part of
			 *          a parsed record to be corrected.
			 * @note The method changes the state of the object: the synchronization of the access when working from
			 *       several threads lies on the one who uses the object
			 * @param date date to process
			 * @param unit elements of the data to set
			 * @see timestamp
			 * @see clear
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // Putting the parsed date into the internal object
			 * chrono.timestamp(1743943021520, ch::type_t::MILLISECONDS);
			 * // Correcting its separate constituents
			 * chrono.set <uint16_t> (2030, ch::unit_t::YEAR);
			 * chrono.set <std::string> ("Dec", ch::unit_t::MONTH);
			 * // Reading what came out from the local storage
			 * chrono.format("%Y-%m-%d", ch::storage_t::LOCAL);  // 2030-12-06
			 * @endcode
			 *
			 */
			void set(const T date, const unit_t unit) noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод установки данных даты и времени
			 *
			 * @param buffer бинарный буфер данных
			 * @param size   размер бинарного буфера
			 * @param unit   элементы данных для установки
			 * @param text   данные переданы в виде текста
			 *
			 * \~english
			 * @brief Method of setting the data of a date and a time
			 * @param buffer binary buffer of the data
			 * @param size   size of the binary buffer
			 * @param unit   elements of the data to set
			 * @param text   the data is passed as a text
			 *
			 * \~
			 */
			void set(const void * buffer, const size_t size, const unit_t unit, const bool text) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода извлечения данных даты и времени
			 *
			 * @tparam T тип данных в котором извлекаются данные
			 *
			 * \~english
			 * @brief Template of the method of extracting the data of a date and a time
			 * @tparam T type of the data the data is extracted in
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод извлечения данных даты и времени
			 *
			 * @details Извлекает отдельную составляющую календарного разложения даты.
			 *          Составляющие DAY и MONTH выдаются и числом, и названием - смотря
			 *          какой тип задан шаблонным доводом.
			 *
			 * @note Извлечение любой составляющей требует полного разложения штампа
			 *       времени, поэтому несколько составляющих одной даты дешевле получить
			 *       разом через format, чем несколькими вызовами get
			 * @note Единица unit_t::WEEKS округляется до ближайшего целого, а не
			 *       отбрасывает неполную неделю: 95 прошедших дней дают 14, а не 13.
			 *       Величина эта отличается и от номера недели в году, который выдают
			 *       переменные формата \%U и \%W: те отсчитывают неделю от её первого
			 *       дня и дни до первого такого дня года относят к нулевой неделе
			 * @param date дата для обработки
			 * @param unit элементы данных для извлечения
			 * @return     значение данных даты и времени
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // 2025-04-06T12:37:01.520Z, воскресенье
			 * chrono.get <uint16_t> (date, ch::unit_t::YEAR);         // 2025
			 * chrono.get <uint8_t> (date, ch::unit_t::MONTH);         // 4
			 * chrono.get <std::string> (date, ch::unit_t::MONTH);     // April
			 * chrono.get <uint8_t> (date, ch::unit_t::DATE);          // 6
			 * chrono.get <uint8_t> (date, ch::unit_t::DAY);           // 7
			 * chrono.get <std::string> (date, ch::unit_t::DAY);       // Sunday
			 * chrono.get <uint16_t> (date, ch::unit_t::DAYS);         // 95
			 * chrono.get <uint8_t> (date, ch::unit_t::WEEKS);         // 14
			 * chrono.get <uint32_t> (date, ch::unit_t::MILLISECONDS); // 520
			 * @endcode
			 *
			 * \~english
			 * @brief Method of extracting the data of a date and a time
			 * @details Extracts a separate part of the calendar decomposition of a date.
			 *          The DAY and MONTH parts are yielded both as a number and as a name — depending on
			 *          which type is set by the template argument.
			 * @note Extracting any part requires a full decomposition of the timestamp,
			 *       and therefore several parts of one date are cheaper to obtain
			 *       all at once through format than by several get calls
			 * @note The unit_t::WEEKS unit is rounded to the nearest whole, and does not
			 *       discard an incomplete week: 95 elapsed days give 14, and not 13.
			 *       That quantity differs from the number of the week in the year as well, which is yielded by
			 *       the \%U and \%W format variables: those count the week from its first
			 *       day and attribute the days before the first such day of the year to the zeroth week
			 * @param date date to process
			 * @param unit elements of the data to extract
			 * @return     value of the data of the date and the time
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // 2025-04-06T12:37:01.520Z, a Sunday
			 * chrono.get <uint16_t> (date, ch::unit_t::YEAR);         // 2025
			 * chrono.get <uint8_t> (date, ch::unit_t::MONTH);         // 4
			 * chrono.get <std::string> (date, ch::unit_t::MONTH);     // April
			 * chrono.get <uint8_t> (date, ch::unit_t::DATE);          // 6
			 * chrono.get <uint8_t> (date, ch::unit_t::DAY);           // 7
			 * chrono.get <std::string> (date, ch::unit_t::DAY);       // Sunday
			 * chrono.get <uint16_t> (date, ch::unit_t::DAYS);         // 95
			 * chrono.get <uint8_t> (date, ch::unit_t::WEEKS);         // 14
			 * chrono.get <uint32_t> (date, ch::unit_t::MILLISECONDS); // 520
			 * @endcode
			 *
			 */
			T get(const uint64_t date, const unit_t unit) const noexcept;
			/**
			 * \~russian
			 * @brief Шаблон метода извлечения данных даты и времени
			 *
			 * @tparam T тип данных в котором извлекаются данные
			 *
			 * \~english
			 * @brief Template of the method of extracting the data of a date and a time
			 * @tparam T type of the data the data is extracted in
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод извлечения данных даты и времени
			 *
			 * @param unit элементы данных для извлечения
			 * @return     значение данных даты и времени
			 *
			 * \~english
			 * @brief Method of extracting the data of a date and a time
			 * @param unit elements of the data to extract
			 * @return     value of the data of the date and the time
			 *
			 * \~
			 */
			T get(const unit_t unit) const noexcept;
			/**
			 * \~russian
			 * @brief Шаблон метода извлечения данных даты и времени
			 *
			 * @tparam T тип данных в котором извлекаются данные
			 *
			 * \~english
			 * @brief Template of the method of extracting the data of a date and a time
			 * @tparam T type of the data the data is extracted in
			 *
			 * \~
			 */
			template <typename T>
			/**
			 * \~russian
			 * @brief Метод извлечения данных даты и времени
			 *
			 * @param unit    элементы данных для извлечения
			 * @param storage хранение значение времени
			 * @return        значение данных даты и времени
			 *
			 * \~english
			 * @brief Method of extracting the data of a date and a time
			 * @param unit    elements of the data to extract
			 * @param storage storage of the value of the time
			 * @return        value of the data of the date and the time
			 *
			 * \~
			 */
			T get(const unit_t unit, const storage_t storage) const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод извлечения данных даты и времени
			 *
			 * @param buffer бинарный буфер данных
			 * @param size   размер бинарного буфера
			 * @param date   дата для обработки
			 * @param unit   элементы данных для установки
			 * @param text   данные переданы в виде текста
			 *
			 * \~english
			 * @brief Method of extracting the data of a date and a time
			 * @param buffer binary buffer of the data
			 * @param size   size of the binary buffer
			 * @param date   date to process
			 * @param unit   elements of the data to set
			 * @param text   the data is passed as a text
			 *
			 * \~
			 */
			void get(void * buffer, const size_t size, const uint64_t date, const unit_t unit, const bool text) const noexcept;
			/**
			 * \~russian
			 * @brief Метод извлечения данных даты и времени
			 *
			 * @param buffer  бинарный буфер данных
			 * @param size    размер бинарного буфера
			 * @param unit    элементы данных для установки
			 * @param text    данные переданы в виде текста
			 * @param storage хранение значение времени
			 *
			 * \~english
			 * @brief Method of extracting the data of a date and a time
			 * @param buffer  binary buffer of the data
			 * @param size    size of the binary buffer
			 * @param unit    elements of the data to set
			 * @param text    the data is passed as a text
			 * @param storage storage of the value of the time
			 *
			 * \~
			 */
			void get(void * buffer, const size_t size, const unit_t unit, const bool text, const storage_t storage) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки временной зоны
			 *
			 * @param zone смещение временной зоны для установки (в секундах)
			 *
			 * \~english
			 * @brief Method of setting the time zone
			 * @param zone offset of the time zone to set (in seconds)
			 *
			 * \~
			 */
			void setTimeZone(const int32_t zone) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки временной зоны
			 *
			 * @param zone временная зона для установки
			 *
			 * \~english
			 * @brief Method of setting the time zone
			 * @param zone time zone to set
			 *
			 * \~
			 */
			void setTimeZone(const zone_t zone) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки временной зоны
			 *
			 * @details Обозначение задаёт временную зону целиком, а не поправку к
			 *          установленной: повторная установка одного и того же обозначения
			 *          смещения не меняет.
			 *
			 * @note Принимаются смещения от UTC-12 до UTC+14 - промежуток, которым
			 *       исчерпываются пояса Земли. Обозначение вне этого промежутка, равно как
			 *       и неизвестное, оставляет установленную зону нетронутой
			 * @note Установленное смещение читается методом getTimeZone с хранилищем
			 *       storage_t::LOCAL: без довода хранилища тот выдаёт зону операционной
			 *       системы, а не установленную объекту
			 * @param zone временная зона для установки
			 * @see getTimeZone
			 *
			 * @code{.cpp}
			 * chrono.setTimeZone("MSK");      // +3:00
			 * chrono.setTimeZone("+05:30");   // +5:30
			 * chrono.setTimeZone("GMT+0530"); // +5:30, название задаёт основание
			 * chrono.setTimeZone("3");        // +3:00, число означает часы
			 * @endcode
			 *
			 * \~english
			 * @brief Method of setting the time zone
			 * @details A designation sets the time zone entirely, and not a correction to
			 *          the set one: a repeated setting of one and the same designation does not change
			 *          the offset.
			 * @note Offsets from UTC-12 to UTC+14 are accepted — the range the belts of the Earth
			 *       are exhausted by. A designation outside this range, as well
			 *       as an unknown one, leaves the set zone untouched
			 * @note The set offset is read by the getTimeZone method with the storage_t::LOCAL
			 *       storage: without the argument of the storage that one yields the zone of the operating
			 *       system, and not the one set to the object
			 * @param zone time zone to set
			 * @see getTimeZone
			 *
			 * @code{.cpp}
			 * chrono.setTimeZone("MSK");      // +3:00
			 * chrono.setTimeZone("+05:30");   // +5:30
			 * chrono.setTimeZone("GMT+0530"); // +5:30, the name sets the basis
			 * chrono.setTimeZone("3");        // +3:00, the number means hours
			 * @endcode
			 *
			 */
			void setTimeZone(string_view zone) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод выполнения матчинга временной зоны
			 *
			 * @details Переводит текстовое обозначение зоны в элемент перечисления. Поиск
			 *          ведётся без учёта регистра.
			 *
			 * @note Смещение по названию зоны выдаёт getTimeZone, принимающий строку: он
			 *       понимает и название со смещением от него ("GMT+0530", "MSK+1"), чего
			 *       этот метод не умеет
			 * @param zone временная зона для конвертации
			 * @return     определённая временная зона
			 * @see getTimeZone
			 *
			 * @code{.cpp}
			 * chrono.matchTimeZone("MSK");   // zone_t::MSK
			 * chrono.matchTimeZone("msk");   // zone_t::MSK
			 * chrono.matchTimeZone("XXXX");  // zone_t::NONE
			 * @endcode
			 *
			 * \~english
			 * @brief Method of performing the matching of a time zone
			 * @details Converts a text designation of a zone into an element of the enumeration. The search
			 *          is performed without the case taken into account.
			 * @note The offset by the name of a zone is yielded by getTimeZone taking a string: it
			 *       understands a name with an offset from it ("GMT+0530", "MSK+1") as well, which
			 *       this method cannot do
			 * @param zone time zone to convert
			 * @return     the determined time zone
			 * @see getTimeZone
			 *
			 * @code{.cpp}
			 * chrono.matchTimeZone("MSK");   // zone_t::MSK
			 * chrono.matchTimeZone("msk");   // zone_t::MSK
			 * chrono.matchTimeZone("XXXX");  // zone_t::NONE
			 * @endcode
			 *
			 */
			zone_t matchTimeZone(string_view zone) const noexcept;
			/**
			 * \~russian
			 * @brief Метод выполнения матчинга временной зоны
			 *
			 * @param storage хранение значение времени
			 * @return        определённая временная зона
			 *
			 * \~english
			 * @brief Method of performing the matching of a time zone
			 * @param storage storage of the value of the time
			 * @return        the determined time zone
			 *
			 * \~
			 */
			zone_t matchTimeZone(const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод перевода временной зоны в смещение
			 *
			 * @details Выводит смещение зоны от UTC в секундах. Смещение положительно к
			 *          востоку от нулевого меридиана: московское время даёт 10800,
			 *          североамериканское восточное стандартное - минус 18000.
			 *
			 * @note Сводные зоны Северной Америки - zone_t::AT, zone_t::CT, zone_t::ET,
			 *       zone_t::MT, zone_t::NT и zone_t::PT - разрешаются здесь по текущему
			 *       моменту, и это назначение перегрузки, а не упущение: она отвечает на
			 *       вопрос «какое смещение у зоны сейчас». Смещение на заданную дату
			 *       выдаёт перегрузка, дату принимающую, и разбор с формированием записей
			 *       идут через неё. Смещение прочих зон от момента не зависит вовсе
			 *
			 * @param zone временная зона для конвертации
			 * @return     смещение временной зоны в секундах
			 *
			 * @see getTimeZone(const zone_t, const uint64_t)
			 *
			 * \~english
			 * @brief Method of converting a time zone into an offset
			 * @details Yields the offset of the zone from UTC in seconds. The offset is positive to
			 *          the east of the zero meridian: the Moscow time gives 10800,
			 *          the North American eastern standard one — minus 18000.
			 * @note The composite zones of North America — zone_t::AT, zone_t::CT, zone_t::ET,
			 *       zone_t::MT, zone_t::NT and zone_t::PT — are resolved here by the current
			 *       moment, and this is the purpose of the overload, and not an omission: it answers
			 *       the question «what offset does the zone have now». The offset on a given date
			 *       is yielded by the overload taking a date, and the parsing and the building of the records
			 *       go through it. The offset of the other zones does not depend on the moment at all
			 * @param zone time zone to convert
			 * @return     offset of the time zone in seconds
			 * @see getTimeZone(const zone_t, const uint64_t)
			 *
			 * \~
			 */
			int32_t getTimeZone(const zone_t zone) const noexcept;
			/**
			 * \~russian
			 * @brief Метод перевода временной зоны в смещение на указанный момент времени
			 *
			 * @details Сводные зоны Северной Америки - zone_t::AT, zone_t::CT, zone_t::ET,
			 *          zone_t::MT, zone_t::NT и zone_t::PT - обозначают то стандартное время,
			 *          то летнее, и выбор между ними зависит от самого момента. Эта
			 *          перегрузка отвечает по указанной дате и от текущего момента не зависит
			 *          вовсе - через неё идут разбор и формирование записей.
			 *
			 * @note Смещение зон, сводными не являющихся, от момента не зависит, и ответ
			 *       здесь равен ответу перегрузки, даты не принимающей
			 * @param zone временная зона для конвертации
			 * @param date штамп времени в миллисекундах
			 * @return     смещение временной зоны в секундах
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // Момент 15 января 2025 года даёт -18000 (EST)
			 * chrono.getTimeZone(ch::zone_t::ET, 1736942400000);
			 * // Момент 15 июля 2025 года даёт -14400 (EDT)
			 * chrono.getTimeZone(ch::zone_t::ET, 1752580800000);
			 * @endcode
			 *
			 * \~english
			 * @brief Method of converting a time zone into an offset at the specified moment of time
			 * @details The composite zones of North America — zone_t::AT, zone_t::CT, zone_t::ET,
			 *          zone_t::MT, zone_t::NT and zone_t::PT — designate now the standard time,
			 *          now the daylight saving one, and the choice between them depends on the moment itself. This
			 *          overload answers by the specified date and does not depend on the current moment
			 *          at all — the parsing and the building of the records go through it.
			 * @note The offset of the zones that are not composite ones does not depend on the moment, and the answer
			 *       here equals the answer of the overload that does not take a date
			 * @param zone time zone to convert
			 * @param date timestamp in milliseconds
			 * @return     offset of the time zone in seconds
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // The moment of the 15th of January 2025 gives -18000 (EST)
			 * chrono.getTimeZone(ch::zone_t::ET, 1736942400000);
			 * // The moment of the 15th of July 2025 gives -14400 (EDT)
			 * chrono.getTimeZone(ch::zone_t::ET, 1752580800000);
			 * @endcode
			 *
			 */
			int32_t getTimeZone(const zone_t zone, const uint64_t date) const noexcept;
			/**
			 * \~russian
			 * @brief Метод перевода временной зоны в смещение
			 *
			 * @details Разбирает текстовое обозначение зоны. Приём намеренно широк: зона
			 *          задаётся названием, смещением, либо названием со смещением от него.
			 *          Смещение читается одной, двумя, тремя и четырьмя цифрами, с двоеточием
			 *          и без.
			 *
			 * @note Нераспознанное обозначение даёт смещение установленной зоны объекта, а
			 *       не признак ошибки. Отличить его от честного нулевого смещения нельзя,
			 *       поэтому обозначения из недоверенного источника стоит проверять методом
			 *       validateTimeZone: он знает и встроенные зоны, и добавленные в реестр,
			 *       и запись одним лишь смещением, тогда как matchTimeZone переводит
			 *       обозначение в элемент перечисления и зоны реестра ему неизвестны
			 * @param zone временная зона для конвертации
			 * @return     смещение временной зоны в секундах
			 * @see matchTimeZone
			 *
			 * @code{.cpp}
			 * chrono.getTimeZone("MSK");       // 10800
			 * chrono.getTimeZone("+05:30");    // 19800
			 * chrono.getTimeZone("+0530");     // 19800
			 * chrono.getTimeZone("+530");      // 19800
			 * chrono.getTimeZone("GMT+0530");  // 19800
			 * chrono.getTimeZone("MSK+1");     // 14400
			 * chrono.getTimeZone("XXXX");      // смещение зоны объекта: не распознано
			 * @endcode
			 *
			 * \~english
			 * @brief Method of converting a time zone into an offset
			 * @details Parses a text designation of a zone. The acceptance is deliberately broad: a zone
			 *          is set by a name, by an offset, or by a name with an offset from it.
			 *          An offset is read with one, two, three and four digits, with a colon
			 *          and without.
			 * @note An unrecognized designation gives the offset of the set zone of the object, and
			 *       not a sign of an error. It is impossible to tell it from an honest zero offset,
			 *       and therefore the designations from an untrusted source are worth checking by the
			 *       validateTimeZone method: it knows both the built-in zones and the ones added to the registry,
			 *       and the record by an offset alone, while matchTimeZone converts
			 *       a designation into an element of the enumeration and the zones of the registry are unknown to it
			 * @param zone time zone to convert
			 * @return     offset of the time zone in seconds
			 * @see matchTimeZone
			 *
			 * @code{.cpp}
			 * chrono.getTimeZone("MSK");       // 10800
			 * chrono.getTimeZone("+05:30");    // 19800
			 * chrono.getTimeZone("+0530");     // 19800
			 * chrono.getTimeZone("+530");      // 19800
			 * chrono.getTimeZone("GMT+0530");  // 19800
			 * chrono.getTimeZone("MSK+1");     // 14400
			 * chrono.getTimeZone("XXXX");      // the offset of the zone of the object: not recognized
			 * @endcode
			 *
			 */
			int32_t getTimeZone(string_view zone) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод определения текущей временной зоны относительно летнего времени
			 *
			 * @details Выбирает между стандартным и летним временем зоны по текущей дате.
			 *          Нужен потому, что перечисление зон задаёт лишь смещение: стандартное и
			 *          летнее время каждой зоны - это два разных его элемента.
			 *
			 * @note Действие летнего времени определяется по правилам США и Канады, как и в
			 *       методе dst, поэтому к зонам других стран способ неприменим
			 * @note Выбор по текущей дате здесь намеренный: разбор и формирование записей
			 *       этот способ не зовут вовсе - они идут через перегрузки, дату принимающие
			 * @param std временная зона стандартного времени
			 * @param sum временная зона летнего времени
			 * @return    смещение временной зоны в секундах
			 * @see dst
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // Летом даёт -14400 (EDT), зимой -18000 (EST)
			 * chrono.getTimeZone(ch::zone_t::EST, ch::zone_t::EDT);
			 * @endcode
			 *
			 * \~english
			 * @brief Method of determining the current time zone relative to the daylight saving time
			 * @details Chooses between the standard and the daylight saving time of a zone by the current date.
			 *          Is needed because the enumeration of the zones sets only the offset: the standard and
			 *          the daylight saving time of every zone are two different elements of it.
			 * @note The force of the daylight saving time is determined by the rules of the USA and Canada, as in
			 *       the dst method, and therefore the way is inapplicable to the zones of other countries
			 * @note The choice by the current date here is a deliberate one: the parsing and the building of the records
			 *       do not call this way at all — they go through the overloads taking a date
			 * @param std time zone of the standard time
			 * @param sum time zone of the daylight saving time
			 * @return    offset of the time zone in seconds
			 * @see dst
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // In the summer it gives -14400 (EDT), in the winter -18000 (EST)
			 * chrono.getTimeZone(ch::zone_t::EST, ch::zone_t::EDT);
			 * @endcode
			 *
			 */
			int32_t getTimeZone(const zone_t std, const zone_t sum) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения установленной временной зоны
			 *
			 * @details Хранилище задаёт, чья зона выводится, и значения эти различны:
			 *          storage_t::GLOBAL выводит зону операционной системы, storage_t::LOCAL
			 *          - зону, установленную самому объекту методом setTimeZone. По
			 *          умолчанию берётся зона операционной системы.
			 *
			 * @param storage хранение значение времени
			 * @return        смещение временной зоны в секундах
			 * @see setTimeZone
			 *
			 * @code{.cpp}
			 * chrono.setTimeZone("+05:30");
			 * chrono.getTimeZone(awh::chrono_t::storage_t::LOCAL);  // 19800
			 * chrono.getTimeZone();                                 // зона системы
			 * @endcode
			 *
			 * \~english
			 * @brief Method of getting the set time zone
			 * @details The storage sets whose zone is yielded, and these values are different:
			 *          storage_t::GLOBAL yields the zone of the operating system, storage_t::LOCAL
			 *          — the zone set to the object itself by the setTimeZone method. By
			 *          default the zone of the operating system is taken.
			 * @param storage storage of the value of the time
			 * @return        offset of the time zone in seconds
			 * @see setTimeZone
			 *
			 * @code{.cpp}
			 * chrono.setTimeZone("+05:30");
			 * chrono.getTimeZone(awh::chrono_t::storage_t::LOCAL);  // 19800
			 * chrono.getTimeZone();                                 // the zone of the system
			 * @endcode
			 *
			 */
			int32_t getTimeZone(const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод очистки списка временных зон
			 *
			 * \~english
			 * @brief Method of clearing the list of the time zones
			 *
			 * \~
			 */
			void clearTimeZones() noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления собственной временной зоны
			 *
			 * @details Дополняет реестр зон собственным обозначением. Добавленное обозначение
			 *          распознаётся и методом getTimeZone, и разбором записей даты по
			 *          переменной формата \%Z.
			 *
			 * @note Обозначение, совпадающее с уже известным в добавленном реестре, заменяет
			 *       его. Встроенные обозначения лежат в отдельной неизменяемой таблице
			 *       модуля, которой ни addTimeZone, ни clearTimeZones не касаются: очистка
			 *       снимает лишь добавленные обозначения, а встроенные остаются доступны
			 * @param name   название временной зоны
			 * @param offset смещение времени в секундах
			 * @see setTimeZones
			 * @see clearTimeZones
			 *
			 * @code{.cpp}
			 * chrono.addTimeZone("ANYKS", 12345);
			 * chrono.getTimeZone("ANYKS");  // 12345
			 * @endcode
			 *
			 * \~english
			 * @brief Method of adding one's own time zone
			 * @details Supplements the registry of the zones with one's own designation. An added designation
			 *          is recognized both by the getTimeZone method and by the parsing of the records of a date by
			 *          the \%Z format variable.
			 * @note A designation coinciding with one already known in the added registry replaces
			 *       it. The built-in designations lie in a separate immutable table
			 *       of the module, which neither addTimeZone nor clearTimeZones touches: the clearing
			 *       removes only the added designations, and the built-in ones remain available
			 * @param name   name of the time zone
			 * @param offset offset of the time in seconds
			 * @see setTimeZones
			 * @see clearTimeZones
			 *
			 * @code{.cpp}
			 * chrono.addTimeZone("ANYKS", 12345);
			 * chrono.getTimeZone("ANYKS");  // 12345
			 * @endcode
			 *
			 */
			void addTimeZone(string_view name, const int32_t offset) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки своего списка временных зон
			 *
			 * @param zones список временных зон для установки
			 *
			 * \~english
			 * @brief Method of setting one's own list of the time zones
			 * @param zones list of the time zones to set
			 *
			 * \~
			 */
			void setTimeZones(const unordered_map <string, int32_t> & zones) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки штампа времени в указанных единицах измерения
			 *
			 * @details Кладёт указанный момент времени во внутренний объект даты - хранилище
			 *          storage_t::LOCAL. Довод type задаёт размерность передаваемого штампа, а
			 *          не единицу хранения: штамп приводится к миллисекундам.
			 *
			 * @note Метод изменяет состояние объекта: согласование доступа при работе из
			 *       нескольких потоков лежит на том, кто объектом пользуется
			 * @param date дата для установки
			 * @param type единицы измерения штампа времени
			 * @see set
			 * @see clear
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // Оба вызова кладут во внутренний объект один и тот же момент
			 * chrono.timestamp(1743943021520, ch::type_t::MILLISECONDS);
			 * chrono.timestamp(1743943021, ch::type_t::SECONDS);
			 * @endcode
			 *
			 * \~english
			 * @brief Method of setting a timestamp in the specified units of the measurement
			 * @details Puts the specified moment of time into the internal date object — the storage_t::LOCAL
			 *          storage. The type argument sets the dimension of the passed timestamp, and
			 *          not the unit of the storage: the timestamp is brought to milliseconds.
			 * @note The method changes the state of the object: the synchronization of the access when working from
			 *       several threads lies on the one who uses the object
			 * @param date date to set
			 * @param type units of the measurement of the timestamp
			 * @see set
			 * @see clear
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // Both calls put one and the same moment into the internal object
			 * chrono.timestamp(1743943021520, ch::type_t::MILLISECONDS);
			 * chrono.timestamp(1743943021, ch::type_t::SECONDS);
			 * @endcode
			 *
			 */
			void timestamp(const uint64_t date, const type_t type) noexcept;
			/**
			 * \~russian
			 * @brief Метод получения штампа времени в указанных единицах измерения
			 *
			 * @details Выводит момент времени в заданной размерности. Хранилище GLOBAL берёт
			 *          момент из системных часов заново при каждом вызове, LOCAL - из
			 *          внутреннего объекта даты.
			 *
			 * @note Наносекундная размерность выводится, но разрешающая способность
			 *       источника ниже: младшие разряды у неё нулевые. На macOS и BSD обращение
			 *       ко времени обслуживается через страницу общего доступа ядра и системным
			 *       вызовом не является, поэтому вызов дёшев - около 24 наносекунд
			 * @param type    единицы измерения штампа времени
			 * @param storage хранение значение времени
			 * @return        штамп времени в указанных единицах измерения
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * chrono.timestamp(ch::type_t::SECONDS);       // 1785332104
			 * chrono.timestamp(ch::type_t::MILLISECONDS);  // 1785332104852
			 * chrono.timestamp(ch::type_t::NANOSECONDS);   // 1785332104852033000
			 * @endcode
			 *
			 * \~english
			 * @brief Method of getting a timestamp in the specified units of the measurement
			 * @details Yields the moment of time in the given dimension. The GLOBAL storage takes the
			 *          moment from the system clock anew at every call, LOCAL — from
			 *          the internal date object.
			 * @note The nanosecond dimension is yielded, but the resolution of
			 *       the source is lower: its lower digits are zero. On macOS and BSD the address
			 *       to the time is served through a shared page of the kernel and is not a system
			 *       call, and therefore the call is cheap — about 24 nanoseconds
			 * @param type    units of the measurement of the timestamp
			 * @param storage storage of the value of the time
			 * @return        timestamp in the specified units of the measurement
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * chrono.timestamp(ch::type_t::SECONDS);       // 1785332104
			 * chrono.timestamp(ch::type_t::MILLISECONDS);  // 1785332104852
			 * chrono.timestamp(ch::type_t::NANOSECONDS);   // 1785332104852033000
			 * @endcode
			 *
			 */
			uint64_t timestamp(const type_t type, const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод парсинга строки даты и времени в UnixTimestamp
			 *
			 * @details Переводит запись даты в штамп времени по образцу формата. Разбор идёт
			 *          по образцу, а не по жёсткой грамматике: символы формата, переменными не
			 *          являющиеся, служат лишь разделителями, а поля ищутся по разрядности.
			 *          Поэтому один и тот же формат читает записи, слегка расходящиеся в
			 *          оформлении.
			 *
			 *          <b>Подстановка недостающих полей.</b> Поля, форматом не заданные,
			 *          подставляются по несимметричному правилу: поля крупнее самого крупного
			 *          заданного берутся текущими, поля мельче самого мелкого заданного
			 *          задаются наименьшим своим значением.
			 *          Первая половина правила введена ради устаревшего стандарта системного
			 *          журнала RFC 3164, года не записывающего: без неё все его записи
			 *          попадали бы в 1970-й год. Если подстановка текущего года уводит запись
			 *          вперёд дальше допуска, заданного методом yearRollback, берётся
			 *          предыдущий год. Вторая половина обеспечивает устойчивость: один и тот
			 *          же довод даёт один и тот же результат вне зависимости от момента вызова.
			 * @note Разбор не сообщает об ошибке. Пустая запись либо пустой формат дают ноль,
			 *       а запись, в которой не нашлось ни одного поля формата, даёт текущий
			 *       момент целиком - по тому же правилу подстановки. Если запись приходит из
			 *       недоверенного источника, результат следует проверять на осмысленность
			 *       самостоятельно
			 * @note Значения полей не проверяются на допустимость и переносятся в соседние
			 *       разряды: запись "2025-13-45" читается как 45-е число тринадцатого месяца
			 *       и даёт 14 февраля 2026 года. Пригодность такой записи отвергает метод
			 *       validate, а поля внутреннего объекта даты после разбора описывают
			 *       выданный штамп времени, а не сырую запись
			 * @note Разбор в хранилище storage_t::LOCAL не только читает внутренний объект
			 *       даты, но и записывает в него разобранный результат: следующий разбор
			 *       будет считать текущим моментом результат предыдущего. Если этого не
			 *       требуется, часы объекта следует переводить методом timestamp перед
			 *       каждым разбором либо брать хранилище storage_t::GLOBAL
			 * @note Временная зона, форматом не заданная, зависит от хранилища. В
			 *       хранилище storage_t::LOCAL она берётся у объекта, если выставлена
			 *       методом setTimeZone, и лишь при её отсутствии - из окружения процесса.
			 *       Хранилище storage_t::GLOBAL объект не читает вовсе и потому берёт зону
			 *       окружения всегда, каким бы ни был setTimeZone. Запись без обозначения
			 *       зоны поэтому читается по-разному на машинах с разными настройками -
			 *       если это нежелательно, зону следует задавать форматом явно
			 * @note Календарь модуля охватывает годы с 1970 по 9999: отсчёт штампа времени
			 *       ведётся от начала 1970 года, а год записывается четырьмя разрядами во
			 *       всех стандартах, которым модуль служит. Запись года вне этих пределов
			 *       разбор выводит началом эпохи, а проверка validate признаёт негодной.
			 *       Тем же началом эпохи выводится и местная полночь первого дня эпохи в
			 *       зоне восточнее UTC: момент этот приходится на время до её начала
			 * @param date    строка даты
			 * @param format  формат даты
			 * @param storage хранение значение времени
			 * @return        дата в UnixTimestamp
			 *
			 * @code{.cpp}
			 * // Записи стандартов
			 * chrono.parse("2025-04-06T12:37:01.520Z", "%Y-%m-%dT%H:%M:%S.%s%i");
			 * chrono.parse("Sun, 06 Apr 2025 12:37:01 +0000", "%a, %d %b %Y %H:%M:%S %z");
			 * chrono.parse("06/Apr/2025:12:37:01 +0000", "%d/%b/%Y:%H:%M:%S %z");
			 * // Запись устаревшего системного журнала, года не содержащая
			 * chrono.parse("Apr  6 12:37:01", "%b %e %H:%M:%S");
			 * @endcode
			 *
			 * @code{.cpp}
			 * chrono.parse("2025", "%Y");                  // 2025-01-01T00:00:00.000
			 * chrono.parse("2025-04", "%Y-%m");            // 2025-04-01T00:00:00.000
			 * chrono.parse("2025-04-06", "%Y-%m-%d");      // 2025-04-06T00:00:00.000
			 * chrono.parse("12:37:01", "%H:%M:%S");        // текущие сутки, 12:37:01.000
			 * @endcode
			 *
			 * \~english
			 * @brief Method of parsing a string of a date and a time into a UnixTimestamp
			 * @details Converts a record of a date into a timestamp by a pattern of a format. The parsing goes
			 *          by a pattern, and not by a strict grammar: the characters of the format that are not
			 *          variables serve only as separators, and the fields are searched for by their width.
			 *          Therefore one and the same format reads the records slightly diverging in
			 *          their arrangement.
			 *          <b>The substitution of the missing fields.</b> The fields not set by the format
			 *          are substituted by an asymmetric rule: the fields coarser than the coarsest
			 *          set one are taken as the current ones, the fields finer than the finest set one
			 *          are set to their smallest value.
			 *          The first half of the rule is introduced for the sake of the obsolete standard of the system
			 *          log RFC 3164, which does not write the year: without it all of its records
			 *          would fall into the year 1970. If the substitution of the current year takes the record
			 *          forward further than the tolerance set by the yearRollback method, the
			 *          previous year is taken. The second half provides the stability: one and the same
			 *          argument gives one and the same result regardless of the moment of the call.
			 * @note The parsing does not report an error. An empty record or an empty format give zero,
			 *       and a record in which not a single field of the format has been found gives the current
			 *       moment entirely — by the same rule of the substitution. If the record comes from
			 *       an untrusted source, the result should be checked for meaningfulness
			 *       on one's own
			 * @note The values of the fields are not checked for admissibility and are carried over into the neighbouring
			 *       digits: the record "2025-13-45" is read as the 45th day of the thirteenth month
			 *       and gives the 14th of February 2026. The fitness of such a record is rejected by the validate
			 *       method, and the fields of the internal date object after the parsing describe the
			 *       yielded timestamp, and not the raw record
			 * @note The parsing into the storage_t::LOCAL storage not only reads the internal date
			 *       object, but writes the parsed result into it as well: the next parsing
			 *       will consider the result of the previous one to be the current moment. If this is not
			 *       required, the clock of the object should be moved by the timestamp method before
			 *       every parsing, or the storage_t::GLOBAL storage should be taken
			 * @note A time zone not set by the format depends on the storage. In
			 *       the storage_t::LOCAL storage it is taken from the object, if it is set
			 *       by the setTimeZone method, and only in its absence — from the environment of the process.
			 *       The storage_t::GLOBAL storage does not read the object at all and therefore takes the zone
			 *       of the environment always, whatever setTimeZone may be. A record without a designation
			 *       of a zone is therefore read differently on the machines with different settings —
			 *       if this is undesirable, the zone should be set by the format explicitly
			 * @note The calendar of the module covers the years from 1970 to 9999: the count of the timestamp
			 *       is kept from the beginning of the year 1970, and the year is written with four digits in
			 *       all the standards the module serves. A record of a year outside these limits
			 *       the parsing yields as the beginning of the epoch, and the validate check recognizes it as unfit.
			 *       By the same beginning of the epoch the local midnight of the first day of the epoch in
			 *       a zone to the east of UTC is yielded as well: that moment falls on the time before its beginning
			 * @param date    string of the date
			 * @param format  format of the date
			 * @param storage storage of the value of the time
			 * @return        date as a UnixTimestamp
			 *
			 * @code{.cpp}
			 * // The records of the standards
			 * chrono.parse("2025-04-06T12:37:01.520Z", "%Y-%m-%dT%H:%M:%S.%s%i");
			 * chrono.parse("Sun, 06 Apr 2025 12:37:01 +0000", "%a, %d %b %Y %H:%M:%S %z");
			 * chrono.parse("06/Apr/2025:12:37:01 +0000", "%d/%b/%Y:%H:%M:%S %z");
			 * // The record of the obsolete system log, not containing the year
			 * chrono.parse("Apr  6 12:37:01", "%b %e %H:%M:%S");
			 * @endcode
			 *
			 * @code{.cpp}
			 * chrono.parse("2025", "%Y");                  // 2025-01-01T00:00:00.000
			 * chrono.parse("2025-04", "%Y-%m");            // 2025-04-01T00:00:00.000
			 * chrono.parse("2025-04-06", "%Y-%m-%d");      // 2025-04-06T00:00:00.000
			 * chrono.parse("12:37:01", "%H:%M:%S");        // the current day, 12:37:01.000
			 * @endcode
			 *
			 */
			uint64_t parse(string_view date, string_view format, const storage_t storage = storage_t::GLOBAL) noexcept;
			/**
			 * \~russian
			 * @brief Метод разбора записи даты по стандарту
			 *
			 * @details Разбирает запись, стандартом заданную, принимая все допустимые им
			 *          разновидности: RFC 5322 позволяет опустить день недели, RFC 3339 -
			 *          долю секунды, ISO 8601 знает основную и расширенную формы записи.
			 *          Разновидности перебираются по порядку, и берётся первая, дающая
			 *          пригодную запись.
			 *
			 * @param date     строка даты для парсинга
			 * @param standard стандарт записи даты
			 * @param storage  хранение значение времени
			 * @return         дата в UnixTimestamp
			 * @see format
			 * @see validate
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * chrono.parse("Sun, 06 Apr 2025 12:37:01 GMT", ch::standard_t::RFC1123);
			 * chrono.parse("2025-04-06T12:37:01Z", ch::standard_t::RFC3339);
			 * chrono.parse("20250406T123701Z", ch::standard_t::ISO8601);
			 * @endcode
			 *
			 * \~english
			 * @brief Method of parsing a record of a date by a standard
			 * @details Parses a record set by a standard, accepting all the varieties allowed by it:
			 *          RFC 5322 allows the day of the week to be omitted, RFC 3339 —
			 *          the fraction of a second, ISO 8601 knows the basic and the extended forms of the record.
			 *          The varieties are gone over in order, and the first one giving
			 *          a fit record is taken.
			 * @param date     string of the date to parse
			 * @param standard standard of the record of the date
			 * @param storage  storage of the value of the time
			 * @return         date as a UnixTimestamp
			 * @see format
			 * @see validate
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * chrono.parse("Sun, 06 Apr 2025 12:37:01 GMT", ch::standard_t::RFC1123);
			 * chrono.parse("2025-04-06T12:37:01Z", ch::standard_t::RFC3339);
			 * chrono.parse("20250406T123701Z", ch::standard_t::ISO8601);
			 * @endcode
			 *
			 */
			uint64_t parse(string_view date, const standard_t standard, const storage_t storage = storage_t::GLOBAL) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки пригодности записи даты для разбора
			 *
			 * @details Разбор об ошибке не сообщает: запись, в которой не нашлось ни одного
			 *          поля формата, возвращается текущим моментом и от честного результата
			 *          не отличается. Проверка закрывает этот пробел, отвечая на вопрос,
			 *          осмысленно ли разбирать запись данным форматом.
			 *
			 *          Пригодной считается запись, в которой нашлась каждая переменная
			 *          формата и разобранные поля которой лежат в допустимых пределах.
			 *
			 *          Записи из недоверенного источника стоит проверять до разбора:
			 * @note Проверка выполняется в глобальном хранилище и внутреннего объекта даты
			 *       не изменяет, каким бы ни было хранилище последующего разбора
			 * @note Проверка не выполняет разбора повторно: если запись пригодна, разбор
			 *       её всё равно придётся выполнить отдельным вызовом. Двойной проход по
			 *       записи - плата за неизменность подписи метода parse
			 * @note Пригодными признаются годы с 1970 по 9999 и смещения временной зоны от
			 *       UTC-12 до UTC+14 - промежуток, которым исчерпываются пояса Земли:
			 * @param date   строка даты
			 * @param format формат даты
			 * @return       признак пригодности записи для разбора
			 * @see parse
			 *
			 * @code{.cpp}
			 * chrono.validate("2025-04-06", "%Y-%m-%d");   // true
			 * chrono.validate("мусор", "%Y-%m-%d");        // false, полей не нашлось
			 * chrono.validate("2025-13-45", "%Y-%m-%d");   // false, месяц и число вне пределов
			 * chrono.validate("2025-02-30", "%Y-%m-%d");   // false, в феврале нет 30-го числа
			 * @endcode
			 *
			 * @code{.cpp}
			 * if(chrono.validate(record, format))
			 *     // Запись пригодна, разбираем
			 *     date = chrono.parse(record, format);
			 * @endcode
			 *
			 * @code{.cpp}
			 * chrono.validate("1969-12-31", "%Y-%m-%d");                  // false
			 * chrono.validate("2025-04-06T14:30:45+1400", "%Y-%m-%dT%H:%M:%S%z"); // true
			 * chrono.validate("2025-04-06T14:30:45+1500", "%Y-%m-%dT%H:%M:%S%z"); // false
			 * @endcode
			 *
			 * \~english
			 * @brief Method of checking the fitness of a record of a date for the parsing
			 * @details The parsing does not report an error: a record in which not a single
			 *          field of the format has been found is returned as the current moment and is indistinguishable from
			 *          an honest result. The check closes this gap, answering the question whether
			 *          it is meaningful to parse the record by the given format.
			 *          A record is considered fit if every variable of the format has been found in it
			 *          and the parsed fields of which lie within the admissible limits.
			 *          The records from an untrusted source are worth checking before the parsing:
			 * @note The check is performed in the global storage and does not change the internal date
			 *       object, whatever the storage of the subsequent parsing may be
			 * @note The check does not perform the parsing twice: if the record is fit, the parsing
			 *       will have to be performed anyway by a separate call. A double pass over
			 *       the record is the price for the immutability of the signature of the parse method
			 * @note Recognized as fit are the years from 1970 to 9999 and the offsets of a time zone from
			 *       UTC-12 to UTC+14 — the range the belts of the Earth are exhausted by:
			 * @param date   string of the date
			 * @param format format of the date
			 * @return       sign of the fitness of the record for the parsing
			 * @see parse
			 *
			 * @code{.cpp}
			 * chrono.validate("2025-04-06", "%Y-%m-%d");   // true
			 * chrono.validate("garbage", "%Y-%m-%d");      // false, no fields were found
			 * chrono.validate("2025-13-45", "%Y-%m-%d");   // false, the month and the day are out of the limits
			 * chrono.validate("2025-02-30", "%Y-%m-%d");   // false, there is no 30th day in February
			 * @endcode
			 *
			 * @code{.cpp}
			 * if(chrono.validate(record, format))
			 *     // The record is fit, parsing it
			 *     date = chrono.parse(record, format);
			 * @endcode
			 *
			 * @code{.cpp}
			 * chrono.validate("1969-12-31", "%Y-%m-%d");                  // false
			 * chrono.validate("2025-04-06T14:30:45+1400", "%Y-%m-%dT%H:%M:%S%z"); // true
			 * chrono.validate("2025-04-06T14:30:45+1500", "%Y-%m-%dT%H:%M:%S%z"); // false
			 * @endcode
			 *
			 */
			bool validate(string_view date, string_view format) noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки пригодности записи даты по стандарту
			 *
			 * @details Запись пригодна, если её принимает хотя бы одна из разновидностей,
			 *          стандартом допускаемых, и все поля её допустимы
			 *
			 * @param date     строка даты для проверки
			 * @param standard стандарт записи даты
			 * @return         признак пригодности записи
			 *
			 * @see parse
			 *
			 * \~english
			 * @brief Method of checking the fitness of a record of a date by a standard
			 * @details A record is fit if it is accepted by at least one of the varieties
			 *          allowed by the standard, and all of its fields are admissible
			 * @param date     string of the date to check
			 * @param standard standard of the record of the date
			 * @return         sign of the fitness of the record
			 * @see parse
			 *
			 * \~
			 */
			bool validate(string_view date, const standard_t standard) noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки пригодности обозначения временной зоны
			 *
			 * @details Перевод обозначения зоны об ошибке не сообщает: неизвестное
			 *          обозначение даёт смещение установленной зоны объекта и от честного
			 *          её смещения не отличается. Проверка закрывает этот пробел.
			 *
			 * @param zone обозначение временной зоны
			 * @return     признак пригодности обозначения
			 * @see getTimeZone
			 *
			 * @code{.cpp}
			 * chrono.validateTimeZone("MSK");      // true
			 * chrono.validateTimeZone("+05:30");   // true
			 * chrono.validateTimeZone("GMT+0530"); // true
			 * chrono.validateTimeZone("XXXX");     // false
			 * @endcode
			 *
			 * \~english
			 * @brief Method of checking the fitness of a designation of a time zone
			 * @details The conversion of a designation of a zone does not report an error: an unknown
			 *          designation gives the offset of the set zone of the object and is indistinguishable from
			 *          its honest offset. The check closes this gap.
			 * @param zone designation of the time zone
			 * @return     sign of the fitness of the designation
			 * @see getTimeZone
			 *
			 * @code{.cpp}
			 * chrono.validateTimeZone("MSK");      // true
			 * chrono.validateTimeZone("+05:30");   // true
			 * chrono.validateTimeZone("GMT+0530"); // true
			 * chrono.validateTimeZone("XXXX");     // false
			 * @endcode
			 *
			 */
			bool validateTimeZone(string_view zone) const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки пригодности обозначения размерности времени
			 *
			 * @details Перевод обозначения размерности об ошибке не сообщает: непригодная
			 *          запись даёт ноль секунд и от честного нуля не отличается.
			 *
			 * @param value строка обозначения размерности (s, m, h, d, w, M, y)
			 * @return      признак пригодности обозначения
			 * @see seconds
			 *
			 * @code{.cpp}
			 * chrono.validateSeconds("90m");   // true
			 * chrono.validateSeconds("1.5h");  // true
			 * chrono.validateSeconds("42");    // false, единица размерности не указана
			 * chrono.validateSeconds("1,5h");  // false, дробная часть отделяется точкой
			 * chrono.validateSeconds("1h30m"); // false, составные записи не предусмотрены
			 * @endcode
			 *
			 * \~english
			 * @brief Method of checking the fitness of a designation of a dimension of a time
			 * @details The conversion of a designation of a dimension does not report an error: an unfit
			 *          record gives zero seconds and is indistinguishable from an honest zero.
			 * @param value string of the designation of the dimension (s, m, h, d, w, M, y)
			 * @return      sign of the fitness of the designation
			 * @see seconds
			 *
			 * @code{.cpp}
			 * chrono.validateSeconds("90m");   // true
			 * chrono.validateSeconds("1.5h");  // true
			 * chrono.validateSeconds("42");    // false, the unit of the dimension is not given
			 * chrono.validateSeconds("1,5h");  // false, the fractional part is separated by a dot
			 * chrono.validateSeconds("1h30m"); // false, the composite records are not provided for
			 * @endcode
			 *
			 */
			bool validateSeconds(string_view value) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод форматирования временной зоны
			 *
			 * @details Записывает смещение зоны сокращённо, для чтения человеком: целые часы
			 *          одним числом, некратные часу - часами и минутами через двоеточие.
			 *          Нулевое смещение даёт UTC.
			 *
			 * @note Эта запись ни одному стандарту не соответствует: все они требуют ведущего
			 *       нуля у часов, а обозначения UTC грамматика RFC 3339 не содержит вовсе.
			 *       Для обмена данными следует брать переменные формата \%z, \%o либо \%i, а не
			 *       этот метод. Разбор её принимает наравне с прочими
			 * @param zone временная зона (в секундах) в которой нужно получить результат
			 * @return     строковое обозначение временной зоны
			 *
			 * @code{.cpp}
			 * chrono.format(0);       // UTC
			 * chrono.format(10800);   // UTC+3
			 * chrono.format(19800);   // UTC+5:30
			 * chrono.format(-12600);  // UTC-3:30
			 * @endcode
			 *
			 * \~english
			 * @brief Method of formatting a time zone
			 * @details Writes the offset of a zone abbreviated, for reading by a human: the whole hours
			 *          as a single number, the ones not multiple of an hour — as hours and minutes through a colon.
			 *          A zero offset gives UTC.
			 * @note This record corresponds to no standard: all of them require a leading
			 *       zero at the hours, and the grammar of RFC 3339 does not contain the designation UTC at all.
			 *       For the exchange of data one should take the \%z, \%o or \%i format variables, and not
			 *       this method. The parsing accepts it on a par with the rest
			 * @param zone time zone (in seconds) the result is needed to be obtained in
			 * @return     string designation of the time zone
			 *
			 * @code{.cpp}
			 * chrono.format(0);       // UTC
			 * chrono.format(10800);   // UTC+3
			 * chrono.format(19800);   // UTC+5:30
			 * chrono.format(-12600);  // UTC-3:30
			 * @endcode
			 *
			 */
			string format(const int32_t zone) const noexcept;
			/**
			 * \~russian
			 * @brief Метод форматирования временной зоны
			 *
			 * @details Записывает обозначение зоны её общепринятым сокращением. Для зон,
			 *          имеющих летнее время, сокращение выбирается по текущей дате: зона
			 *          zone_t::ET даёт EDT летом и EST зимой.
			 *
			 * @note Выбор по текущей дате - назначение перегрузки, а не упущение: она
			 *       отвечает на вопрос «как зона обозначается сейчас». Обозначение на
			 *       заданную дату выдаёт перегрузка, дату принимающую, и переменная формата
			 *       \%Z идёт через неё - обозначение сводной зоны разрешается по самой
			 *       записи, а не по мгновению, в которое она формируется
			 *
			 * @param zone временная зона в которой нужно получить результат
			 * @return     строковое обозначение временной зоны
			 *
			 * @see format(const zone_t, const uint64_t)
			 *
			 * \~english
			 * @brief Method of formatting a time zone
			 * @details Writes the designation of a zone by its commonly accepted abbreviation. For the zones
			 *          having a daylight saving time, the abbreviation is chosen by the current date: the zone
			 *          zone_t::ET gives EDT in the summer and EST in the winter.
			 * @note The choice by the current date is the purpose of the overload, and not an omission: it
			 *       answers the question «how is the zone designated now». The designation on
			 *       a given date is yielded by the overload taking a date, and the \%Z format variable
			 *       goes through it — the designation of a composite zone is resolved by the record
			 *       itself, and not by the instant it is built at
			 * @param zone time zone the result is needed to be obtained in
			 * @return     string designation of the time zone
			 * @see format(const zone_t, const uint64_t)
			 *
			 * \~
			 */
			string format(const zone_t zone) const noexcept;
			/**
			 * \~russian
			 * @brief Метод форматирования временной зоны на указанный момент времени
			 *
			 * @details Записывает обозначение зоны её общепринятым сокращением, выбирая
			 *          между стандартным и летним временем по указанной дате, а не по
			 *          текущей. Через эту перегрузку идёт переменная формата \%Z:
			 *          обозначение сводной зоны разрешается по самой записи.
			 *
			 * @note Обозначение зон, сводными не являющихся, от момента не зависит, и ответ
			 *       здесь равен ответу перегрузки, даты не принимающей
			 * @param zone временная зона в которой нужно получить результат
			 * @param date штамп времени в миллисекундах
			 * @return     строковое обозначение временной зоны
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // Момент 15 января 2025 года даёт EST
			 * chrono.format(ch::zone_t::ET, 1736942400000);
			 * // Момент 15 июля 2025 года даёт EDT
			 * chrono.format(ch::zone_t::ET, 1752580800000);
			 * @endcode
			 *
			 * \~english
			 * @brief Method of formatting a time zone at the specified moment of time
			 * @details Writes the designation of a zone by its commonly accepted abbreviation, choosing
			 *          between the standard and the daylight saving time by the specified date, and not by
			 *          the current one. Through this overload the \%Z format variable goes:
			 *          the designation of a composite zone is resolved by the record itself.
			 * @note The designation of the zones that are not composite ones does not depend on the moment, and the answer
			 *       here equals the answer of the overload that does not take a date
			 * @param zone time zone the result is needed to be obtained in
			 * @param date timestamp in milliseconds
			 * @return     string designation of the time zone
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * // The moment of the 15th of January 2025 gives EST
			 * chrono.format(ch::zone_t::ET, 1736942400000);
			 * // The moment of the 15th of July 2025 gives EDT
			 * chrono.format(ch::zone_t::ET, 1752580800000);
			 * @endcode
			 *
			 */
			string format(const zone_t zone, const uint64_t date) const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод формирования объекта даты и времени
			 *
			 * @param dt     объект даты и времени
			 * @param format формат даты
			 * @return       строка содержащая дату
			 *
			 * \~english
			 * @brief Method of building an object of a date and a time
			 * @param dt     object of the date and the time
			 * @param format format of the date
			 * @return       string containing the date
			 *
			 * \~
			 */
			string format(const dt_t & dt, string_view format) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод формирования записи даты во временной зоне окружения
			 *
			 * @details Формирует запись даты по образцу формата. Символы формата, переменными
			 *          не являющиеся, записываются как есть. Дата записывается во временной
			 *          зоне окружения, определяемой самостоятельно, а не в нулевой: журналы и
			 *          отчётность ведутся по местному времени сервера, а оно заранее известно
			 *          далеко не всегда - особенно когда сервер стоит в другой стране.
			 *
			 *          Записать дату в заданной зоне позволяют перегрузки, принимающие зону
			 *          вторым доводом, а в нулевой - перегрузка с нулевым смещением.
			 *
			 * @note Первый довод обязан быть шестидесятичетырёхразрядным. Целочисленный
			 *       литерал подходит и этой перегрузке, и перегрузке со смещением временной
			 *       зоны первым доводом, поэтому малое число молча уходит в неё, а большое
			 *       делает вызов неоднозначным. Дату следует передавать переменной либо
			 *       приводить явно:
			 * @note Пустой формат даёт пустую строку. Нулевая дата пустой строки не даёт:
			 *       это полночь первого января 1970 года, дата не менее действительная,
			 *       чем любая другая
			 * @note Штамп времени за пределом 9999 года формируется последним представимым
			 *       моментом - записью 9999-12-31 23:59:59.999. Раскладывать такие штампы
			 *       нечем: поле года их обрезает, и запись выходила с несуществующим
			 *       числом месяца вида 1934-12-200
			 * @param date   дата в UnixTimestamp
			 * @param format формат даты
			 * @return       строка содержащая дату
			 * @see parse
			 * @see strip
			 *
			 * @code{.cpp}
			 * const uint64_t date = 1743943021520;
			 * // Записи ниже приведены для зоны окружения UTC
			 * chrono.format(date, "%Y-%m-%dT%H:%M:%S.%s%i");   // 2025-04-06T12:37:01.520Z
			 * chrono.format(date, "%d/%b/%Y:%H:%M:%S %z");     // 06/Apr/2025:12:37:01 +0000
			 * chrono.format(date, "%b %e %H:%M:%S");           // Apr  6 12:37:01
			 * chrono.format(date, "Отчёт за %d %B %Y года");     // Отчёт за 06 April 2025 года
			 * @endcode
			 *
			 * @code{.cpp}
			 * chrono.format(1, "%Y-%m-%d");                        // не дата, а зона +1 секунда
			 * chrono.format(static_cast <uint64_t> (1), "%Y-%m-%d"); // 1970-01-01
			 * @endcode
			 *
			 * \~english
			 * @brief Method of building a record of a date in the time zone of the environment
			 * @details Builds a record of a date by a pattern of a format. The characters of the format that are not
			 *          variables are written as they are. The date is written in the time
			 *          zone of the environment, determined on its own, and not in the zero one: the logs and
			 *          the reporting are kept by the local time of the server, and it is known in advance
			 *          far from always — especially when the server stands in another country.
			 *          Writing a date in a given zone is made possible by the overloads taking a zone
			 *          as the second argument, and in the zero one — by the overload with a zero offset.
			 * @note The first argument is obliged to be a sixty four bit one. An integer
			 *       literal suits both this overload and the overload with the offset of the time
			 *       zone as the first argument, and therefore a small number goes silently into it, and a large one
			 *       makes the call ambiguous. The date should be passed by a variable or
			 *       cast explicitly:
			 * @note An empty format gives an empty string. A zero date does not give an empty string:
			 *       it is the midnight of the first of January 1970, a date no less real
			 *       than any other
			 * @note A timestamp beyond the limit of the year 9999 is built as the last representable
			 *       moment — the record 9999-12-31 23:59:59.999. There is nothing to decompose such timestamps by:
			 *       the field of the year truncates them, and the record came out with a non-existent
			 *       day of the month of the form 1934-12-200
			 * @param date   date as a UnixTimestamp
			 * @param format format of the date
			 * @return       string containing the date
			 * @see parse
			 * @see strip
			 *
			 * @code{.cpp}
			 * const uint64_t date = 1743943021520;
			 * // The records below are given for the zone of the environment UTC
			 * chrono.format(date, "%Y-%m-%dT%H:%M:%S.%s%i");   // 2025-04-06T12:37:01.520Z
			 * chrono.format(date, "%d/%b/%Y:%H:%M:%S %z");     // 06/Apr/2025:12:37:01 +0000
			 * chrono.format(date, "%b %e %H:%M:%S");           // Apr  6 12:37:01
			 * chrono.format(date, "Report for %d %B %Y");        // Report for 06 April 2025
			 * @endcode
			 *
			 * @code{.cpp}
			 * chrono.format(1, "%Y-%m-%d");                        // not a date, but the zone +1 second
			 * chrono.format(static_cast <uint64_t> (1), "%Y-%m-%d"); // 1970-01-01
			 * @endcode
			 *
			 */
			string format(const uint64_t date, string_view format) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод формирования записи даты по стандарту
			 *
			 * @details Формирует запись образцом, который задан стандартом. Стандарты
			 *          RFC 1123 и RFC 850 предписывают нулевую зону и обозначают её словом
			 *          GMT, поэтому запись по ним формируется в нулевой зоне независимо от
			 *          зоны объекта и окружения. Прочие записываются в зоне окружения
			 *          наравне с перегрузкой, принимающей строку формата.
			 *
			 * @note Запись RFC 3164 ни года, ни зоны не несёт: устаревший системный журнал
			 *       их не передаёт вовсе, и разбор такой записи берёт год у текущего момента
			 * @param date     дата в UnixTimestamp
			 * @param standard стандарт записи даты
			 * @return         строка содержащая дату
			 * @see parse
			 * @see validate
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * const uint64_t date = 1743943021520;
			 * chrono.format(date, ch::standard_t::RFC3339);  // 2025-04-06T12:37:01.520Z
			 * chrono.format(date, ch::standard_t::RFC1123);  // Sun, 06 Apr 2025 12:37:01 GMT
			 * chrono.format(date, ch::standard_t::CLF);      // 06/Apr/2025:12:37:01 +0000
			 * @endcode
			 *
			 * \~english
			 * @brief Method of building a record of a date by a standard
			 * @details Builds a record by the pattern that is set by a standard. The standards
			 *          RFC 1123 and RFC 850 prescribe the zero zone and designate it by the word
			 *          GMT, and therefore a record by them is built in the zero zone regardless of
			 *          the zone of the object and of the environment. The rest are written in the zone of the environment
			 *          on a par with the overload taking a format string.
			 * @note The RFC 3164 record carries neither a year nor a zone: the obsolete system log
			 *       does not transmit them at all, and the parsing of such a record takes the year from the current moment
			 * @param date     date as a UnixTimestamp
			 * @param standard standard of the record of the date
			 * @return         string containing the date
			 * @see parse
			 * @see validate
			 *
			 * @code{.cpp}
			 * using ch = awh::chrono_t;
			 * const uint64_t date = 1743943021520;
			 * chrono.format(date, ch::standard_t::RFC3339);  // 2025-04-06T12:37:01.520Z
			 * chrono.format(date, ch::standard_t::RFC1123);  // Sun, 06 Apr 2025 12:37:01 GMT
			 * chrono.format(date, ch::standard_t::CLF);      // 06/Apr/2025:12:37:01 +0000
			 * @endcode
			 *
			 */
			string format(const uint64_t date, const standard_t standard) const noexcept;
			/**
			 * \~russian
			 * @brief Метод формирования записи даты по стандарту в указанной зоне
			 *
			 * @note Стандарты RFC 1123 и RFC 850 предписывают нулевую зону, и указанное
			 *       смещение для них не применяется
			 *
			 * @param date     дата в UnixTimestamp
			 * @param zone     смещение временной зоны в секундах
			 * @param standard стандарт записи даты
			 * @return         строка содержащая дату
			 *
			 * \~english
			 * @brief Method of building a record of a date by a standard in the specified zone
			 * @note The standards RFC 1123 and RFC 850 prescribe the zero zone, and the specified
			 *       offset is not applied for them
			 * @param date     date as a UnixTimestamp
			 * @param zone     offset of the time zone in seconds
			 * @param standard standard of the record of the date
			 * @return         string containing the date
			 *
			 * \~
			 */
			string format(const uint64_t date, const int32_t zone, const standard_t standard) const noexcept;
			/**
			 * \~russian
			 * @brief Метод формирования UnixTimestamp с учётом временной зоны
			 *
			 * @details Формирует запись даты в указанной временной зоне. Момент времени при
			 *          этом не меняется - меняется лишь гражданское время его записи, и
			 *          переменные обозначения зоны выводят заданное смещение.
			 *
			 * @param date   дата в UnixTimestamp
			 * @param zone   временная зона в которой нужно получить дату (в секундах)
			 * @param format формат даты
			 * @return       строка содержащая дату
			 *
			 * @code{.cpp}
			 * const uint64_t date = 1743943021520;  // 2025-04-06T12:37:01.520Z
			 * chrono.format(date, 10800, "%Y-%m-%dT%H:%M:%S%i");  // 2025-04-06T15:37:01+03:00
			 * chrono.format(date, 19800, "%Y-%m-%dT%H:%M:%S%i");  // 2025-04-06T18:07:01+05:30
			 * @endcode
			 *
			 * \~english
			 * @brief Method of building a UnixTimestamp with the time zone taken into account
			 * @details Builds a record of a date in the specified time zone. The moment of time at
			 *          that does not change — only the civil time of its record changes, and
			 *          the variables of the designation of the zone yield the given offset.
			 * @param date   date as a UnixTimestamp
			 * @param zone   time zone the date is needed to be obtained in (in seconds)
			 * @param format format of the date
			 * @return       string containing the date
			 *
			 * @code{.cpp}
			 * const uint64_t date = 1743943021520;  // 2025-04-06T12:37:01.520Z
			 * chrono.format(date, 10800, "%Y-%m-%dT%H:%M:%S%i");  // 2025-04-06T15:37:01+03:00
			 * chrono.format(date, 19800, "%Y-%m-%dT%H:%M:%S%i");  // 2025-04-06T18:07:01+05:30
			 * @endcode
			 *
			 */
			string format(const uint64_t date, const int32_t zone, string_view format) const noexcept;
			/**
			 * \~russian
			 * @brief Метод формирования UnixTimestamp с учётом временной зоны
			 *
			 * @param date   дата в UnixTimestamp
			 * @param zone   временная зона в которой нужно получить дату
			 * @param format формат даты
			 * @return       строка содержащая дату
			 *
			 * \~english
			 * @brief Method of building a UnixTimestamp with the time zone taken into account
			 * @param date   date as a UnixTimestamp
			 * @param zone   time zone the date is needed to be obtained in
			 * @param format format of the date
			 * @return       string containing the date
			 *
			 * \~
			 */
			string format(const uint64_t date, const zone_t zone, string_view format) const noexcept;
			/**
			 * \~russian
			 * @brief Метод формирования UnixTimestamp с учётом временной зоны
			 *
			 * @param date   дата в UnixTimestamp
			 * @param zone   временная зона в которой нужно получить дату
			 * @param format формат даты
			 * @return       строка содержащая дату
			 *
			 * \~english
			 * @brief Method of building a UnixTimestamp with the time zone taken into account
			 * @param date   date as a UnixTimestamp
			 * @param zone   time zone the date is needed to be obtained in
			 * @param format format of the date
			 * @return       string containing the date
			 *
			 * \~
			 */
			string format(const uint64_t date, string_view zone, string_view format) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод формирования текущей даты без учёта временной зоны
			 *
			 * @details Формирует запись текущего момента. Хранилище GLOBAL берёт момент из
			 *          системных часов, LOCAL - из внутреннего объекта даты, заполняемого
			 *          методами set и timestamp.
			 *
			 * @param format  формат даты
			 * @param storage хранение значение времени
			 * @return        строка содержащая дату
			 * @see timestamp
			 * @see set
			 *
			 * @code{.cpp}
			 * // Отметка времени для записи журнала прямо сейчас
			 * chrono.format("%Y-%m-%dT%H:%M:%S.%s%i");
			 * @endcode
			 *
			 * \~english
			 * @brief Method of building the current date without the time zone taken into account
			 * @details Builds a record of the current moment. The GLOBAL storage takes the moment from
			 *          the system clock, LOCAL — from the internal date object, filled by
			 *          the set and timestamp methods.
			 * @param format  format of the date
			 * @param storage storage of the value of the time
			 * @return        string containing the date
			 * @see timestamp
			 * @see set
			 *
			 * @code{.cpp}
			 * // A timestamp for a record of a log right now
			 * chrono.format("%Y-%m-%dT%H:%M:%S.%s%i");
			 * @endcode
			 *
			 */
			string format(string_view format, const storage_t storage = storage_t::GLOBAL) const noexcept;
			/**
			 * \~russian
			 * @brief Метод формирования текущей даты с учётом временной зоны
			 *
			 * @param zone    временная зона в которой нужно получить дату (в секундах)
			 * @param format  формат даты
			 * @param storage хранение значение времени
			 * @return        строка содержащая дату
			 *
			 * \~english
			 * @brief Method of building the current date with the time zone taken into account
			 * @param zone    time zone the date is needed to be obtained in (in seconds)
			 * @param format  format of the date
			 * @param storage storage of the value of the time
			 * @return        string containing the date
			 *
			 * \~
			 */
			string format(const int32_t zone, string_view format, const storage_t storage = storage_t::GLOBAL) const noexcept;
			/**
			 * \~russian
			 * @brief Метод формирования текущей даты с учётом временной зоны
			 *
			 * @param zone    временная зона в которой нужно получить дату
			 * @param format  формат даты
			 * @param storage хранение значение времени
			 * @return        строка содержащая дату
			 *
			 * \~english
			 * @brief Method of building the current date with the time zone taken into account
			 * @param zone    time zone the date is needed to be obtained in
			 * @param format  format of the date
			 * @param storage storage of the value of the time
			 * @return        string containing the date
			 *
			 * \~
			 */
			string format(const zone_t zone, string_view format, const storage_t storage = storage_t::GLOBAL) const noexcept;
			/**
			 * \~russian
			 * @brief Метод формирования текущей даты с учётом временной зоны
			 *
			 * @param zone    временная зона в которой нужно получить дату
			 * @param format  формат даты
			 * @param storage хранение значение времени
			 * @return        строка содержащая дату
			 *
			 * \~english
			 * @brief Method of building the current date with the time zone taken into account
			 * @param zone    time zone the date is needed to be obtained in
			 * @param format  format of the date
			 * @param storage storage of the value of the time
			 * @return        string containing the date
			 *
			 * \~
			 */
			string format(string_view zone, string_view format, const storage_t storage = storage_t::GLOBAL) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод преобразования даты из оного формата в другой
			 *
			 * @details Разбирает запись одним форматом и тут же формирует её другим. Равнозначен
			 *          паре вызовов parse и format, но не требует промежуточного штампа времени.
			 *
			 * @note Правила подстановки недостающих полей те же, что и у parse: конечная
			 *       запись может содержать поля, которых не было в исходной, и они будут
			 *       заполнены по этим правилам
			 * @note Метод константным не является: с хранилищем storage_t::LOCAL разбор
			 *       записывает разобранную дату в объект, ровно как это делает parse
			 * @param date    строка даты для преобразования
			 * @param format1 формат даты из которой нужно получить дату
			 * @param format2 формат даты в который нужно перевести дату
			 * @param storage хранение значение времени
			 * @return        результат работы
			 * @see parse
			 * @see format
			 *
			 * @code{.cpp}
			 * // Запись журнала веб-сервера в запись действующего стандарта журнала
			 * chrono.strip("06/Apr/2025:12:37:01 +0000",
			 *              "%d/%b/%Y:%H:%M:%S %z",
			 *              "%Y-%m-%dT%H:%M:%S%i");  // 2025-04-06T12:37:01Z
			 * @endcode
			 *
			 * \~english
			 * @brief Method of converting a date from one format into another
			 * @details Parses a record by one format and immediately builds it by another. Is equivalent to
			 *          a pair of parse and format calls, but does not require an intermediate timestamp.
			 * @note The rules of the substitution of the missing fields are the same as for parse: the final
			 *       record may contain the fields that were absent in the original one, and they will be
			 *       filled by these rules
			 * @note The method is not a constant one: with the storage_t::LOCAL storage the parsing
			 *       writes the parsed date into the object, exactly as parse does
			 * @param date    string of the date to convert
			 * @param format1 format of the date the date is needed to be obtained from
			 * @param format2 format of the date the date is needed to be converted into
			 * @param storage storage of the value of the time
			 * @return        result of the work
			 * @see parse
			 * @see format
			 *
			 * @code{.cpp}
			 * // The record of the log of a web server into the record of the standard of the log in force
			 * chrono.strip("06/Apr/2025:12:37:01 +0000",
			 *              "%d/%b/%Y:%H:%M:%S %z",
			 *              "%Y-%m-%dT%H:%M:%S%i");  // 2025-04-06T12:37:01Z
			 * @endcode
			 *
			 */
			string strip(string_view date, string_view format1, string_view format2, const storage_t storage = storage_t::GLOBAL) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 * @param fmk framework object
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Chrono(const fmk_t * fmk, const Logging * log) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			~Chrono() noexcept;
	} chrono_t;
};

#endif // __AWH_CHRONO__
