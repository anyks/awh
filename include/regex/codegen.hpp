/**
 * @file codegen.hpp
 * @date 2026-08-02
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
 * @brief Заголовочный файл преобразования программы регулярного выражения в машинный код —
 *        класс Codegen, порождающий сопоставитель для подмножества программ, исполнимого
 *        без набора точек возврата произвольной глубины
 *
 * @section codegen_decisions Намеренные решения
 *
 * @details Перечисленное ниже выглядит несообразностью, но выбрано осознанно и
 *          правке не подлежит. Раздел заведён затем, чтобы разбор кода не начинался
 *          каждый раз с одних и тех же выводов.
 *
 *          <b>Всякое сопоставление одиночного символа порождается обращением
 *          к таблице принадлежности байтов.</b> Одиночный символ, класс символов
 *          и любой символ различаются лишь содержимым таблицы, отчего порождаемый
 *          код для них один и тот же, а различие уходит в обстановку исполнения.
 *          Порождение отдельного кода на каждый вид сопоставления дало бы выигрыш
 *          лишь на одиночном символе - одно сравнение взамен обращения к таблице, -
 *          но отняло бы у порождённого кода перемещаемость: значение символа
 *          пришлось бы размещать в самой команде.
 *
 *          <b>Ряд повторения одиночного символа проходится целиком, а отступление
 *          выполняется по одной позиции.</b> Такой порядок отвечает жадному
 *          повторению в точности и не требует набора точек возврата: положение
 *          отступления каждого ряда хранится в кадре вызова, а количество рядов
 *          известно при порождении. Выражения, требующие набора точек возврата
 *          произвольной глубины - вложенные повторения и рекурсия, -
 *          кодогенерации не получают вовсе и исполняются
 *          программой, как и прежде. Ветви же выбора точек возврата
 *          произвольной глубины не требуют: количество их известно при
 *          порождении наравне с количеством рядов, отчего кодогенерацию они
 *          получают. Не требуют их и атомарные группы, проверки окружения
 *          и обратные ссылки - см. разделы ниже.
 *
 *          <b>Предварительный отбор позиций выполняется вызовом подпрограммы,
 *          а не порождается в самом коде.</b> Отбор ищет в тексте ведущий литерал
 *          совпадения либо единственный допустимый начальный байт, а поиск
 *          последовательности в тексте выполняется набором команд процессора
 *          над несколькими байтами сразу. Порождение побайтного перебора взамен
 *          вызова обошлось бы дороже самого вызова на всяком тексте, где отбор
 *          пропускает участки целиком, - а иных участков отбор и не пропускает.
 *          Цена решения - сохранение затираемых вызовом регистров в кадре,
 *          выполняемое единожды на попытку сопоставления.
 *
 *          <b>Позиции начала попытки просеиваются в самом коде, если отбор
 *          их не порождён.</b> Набор допустимых начальных байтов бывает не описан
 *          ни ведущим литералом, ни единственным байтом - таков всякий выбор
 *          ветвей, литералы каких начинаются по-разному, - и отбора позиций такое
 *          выражение не получает вовсе, пробуя все ветви в каждой позиции.
 *          Просеивание пропускает позиции, байт в каких совпадению начинать не
 *          дан, ценою двух обращений к памяти взамен попытки сопоставления
 *          целиком.
 *
 *          Довод против набора допустимых байтов, приведённый выше, касается
 *          вызова подпрограммы, а не порождения в коде: подпрограмма ускоряет
 *          пропуском участков, а просеивание - дешевизной хода. Замер на
 *          выражении «alpha|bravo|charlie|delta|echo|foxtrot» по тексту
 *          в шестьдесят четыре килобайта показал долю к исполнению программы
 *          0,65 без просеивания и 2,99 с ним, а выражения с набором байтов узким
 *          выиграли заодно: «[0-9]{3,5}» - с 3,64 до 4,83.
 *
 *          <b>Границы захватывающих групп запоминаются лишь на цепочках ветвей
 *          выбора, а при отступлении ряда - нет.</b> Отступление возвращает
 *          исполнение к сопоставлению вслед за рядом, отчего сохранения,
 *          размещённые после него, выполняются заново, а размещённые прежде него
 *          отступлением не затронуты. Ветвь же, сопоставление какой прервано,
 *          оставила бы записанные ею границы установленными, а ветвь следующая
 *          их не перезапишет, - потому цепочка ветвей границы свои запоминает
 *          при входе и восстанавливает при переходе к ветви следующей и при
 *          исчерпании цепочки. Набор запоминаемых ячеек известен при порождении:
 *          он собирается обходом области инструкций цепочки по адресам, а не по
 *          путям исполнения. Ячейка лишняя в наборе стоит двух обращений
 *          к памяти, тогда как недостающая дала бы неверные границы.
 *
 *          <b>Проход ряда порождается по пределу его, а не разбором таблицы
 *          всегда.</b> Разбор таблицы принадлежности читает из памяти дважды
 *          на каждый байт - сам байт и запись таблицы, - тогда как ряды устройства
 *          простого проходятся дешевле. Ряд, всякое значение байта принимающий,
 *          упирается лишь в конец текста и порождается установкой позиции
 *          в размер его, без цикла вовсе. Ряд, единственным значением
 *          ограниченный, проходится поиском этого значения: поиск байта в тексте
 *          выполняется набором команд процессора над несколькими байтами сразу.
 *          Ряды прочие проходятся разбором таблицы, как и прежде.
 *
 *          Количество значений, ряду не принадлежащих, считается при порождении
 *          обходом той самой таблицы, какая для ряда и заведена: выбор пути
 *          опирается на построенное, а не на вид выражения. Замер на выражении
 *          «.*needle» по тексту в шестьдесят четыре килобайта показал долю
 *          к исполнению программы 0,68 при разборе таблицы и 1,90 при поиске
 *          предела.
 *
 *          Отличие от отступления ряда обратным поиском, замером отвергнутого,
 *          в размере заменяемого: там вызов подпрограммы заменял собою одно
 *          отступление, здесь - проход ряда целиком.
 *
 *          <b>Ленивое повторение порождается продвижением по отказу, а не
 *          проходом ряда.</b> Ленивый ряд ходу навстречу жадному: сопоставление
 *          продолжается сразу, а тело поглощает по символу лишь по отказу
 *          продолжения. Оттого прохода и отступления ленивый ряд не имеет вовсе,
 *          а место кадра занимает то же самое - положение своё. Пометка ленивых
 *          повторений ведётся набором отдельным от жадных: пометка в наборе
 *          общем заставила бы толкователей пройти ленивый ряд жадно.
 *
 *          <b>Продвижение ленивого ряда идёт до байта, продолжение начинающего,
 *          а не по одному символу.</b> Возврат в продолжение на каждом символе
 *          обходится дороже самого продвижения: он проходит через переход по
 *          адресу в регистре, а положение ряда читается и пишется в кадр всякий
 *          раз. Между тем продолжение, начинаемое сопоставлением одиночного
 *          символа, отвергает всякое положение, где текст несёт байт иной, и
 *          опробовать его там незачем. Оттого ряд, всякий байт принимающий, ищет
 *          этот байт набором команд процессора над несколькими байтами сразу, ряд
 *          ограниченный - разбором таблицы принадлежности с добавленным к нему
 *          сравнением, а продолжение без приметы начала оставляет продвижение
 *          посимвольным. Примета берётся у одной инструкции, непосредственно за
 *          рядом стоящей, и лишь у сопоставления одиночного символа ASCII без
 *          приведения регистра: обход инструкций, текста не поглощающих, дал бы
 *          примету и там, где между рядом и символом размещена запись границы
 *          захвата, а границу эту порождённый код пишет без журнала.
 *
 *          <b>Пропуск участка, пройденного рядом, порождается и ленивым рядом,
 *          а не одним жадным.</b> Ленивый ряд, исчерпавшись, опробовал продолжение
 *          в каждом положении участка - сам ли, продвижением до байта, или
 *          возвратом в продолжение, - а тело повторения дальше не пускает.
 *          Начало попытки более позднее того же участка не изменит: байт, ряду
 *          не принадлежащий, стоит на месте. Замер на выражении «\\w+?@\\w+?\\.»
 *          по длинному тексту дал троекратное ускорение, а на «.*?needle» -
 *          двукратное.
 *
 *          <b>Отказ попытки пропускает участок, первым рядом пройденный,
 *          а не сдвигает начало попытки на позицию.</b> Отказ попытки, ряд
 *          исчерпавшей, означает, что хвост выражения не сошёлся ни в одной
 *          позиции отступления. Начало попытки более позднее сдвигает начало
 *          ряда вперёд, а конец его оставляет прежним - байт, ряду не
 *          принадлежащий, стоит на месте, - отчего набор проверяемых позиций
 *          окажется подмножеством уже отвергнутого. Без пропуска перебор
 *          проходит участок повторно и даёт квадрат от длины ряда: замер на
 *          выражении «a.*z» без совпадения показал долю 0,004 к исполнению
 *          программы, а с пропуском - 44,7.
 *
 *          Положение пропуска - это конец ряда за вычетом длины участка, ряду
 *          предшествующего: попытка следующая должна начаться там, где ряд
 *          начнётся за концом нынешнего, а не за концом его самого. Длина эта
 *          известна при порождении, ибо до ряда первого размещаются лишь
 *          сопоставления одиночных символов. Без вычитания пропуск проходит
 *          мимо позиций, ряду ещё не открытых, - изъян этот был обнаружен
 *          порождаемыми образцами и закреплён порчей.
 *
 *          Правомерность держится на двух опорах, каждая из каких проверена
 *          порчей. Первая - постоянство длины участка, ряду предшествующего:
 *          ветви выбора разной длины открыли бы позиции, ещё не проверенные,
 *          отчего выражения с цепочками ветвей пропуска не получают вовсе -
 *          и цепочка, разбираемая по первому байту, здесь исключением не служит.
 *          Счёт цепочек ведётся двумя числами именно поэтому: цепочка
 *          разбираемая мест кадра не занимает и на ведение отказа не влияет,
 *          однако длину участка, ряду предшествующего, меняет наравне с прочими.
 *          Изъян этот был внесён при заведении разбора по байту, обнаружен
 *          зависанием переносимого стенда на выражении «(?:a|x)\\B0*?[a-z]+?(?<!b)»
 *          и закреплён образцом.
 *          Вторая - положение конца, запоминаемое рядом первым и сбрасываемое
 *          при каждой попытке: ряды последующие начинаются с положения,
 *          отступлением ряда первого изменяемого, а остаток от попытки прежней
 *          пропустил бы участок непройденный.
 *
 *          Хвост выражения сходится или нет по одной лишь позиции: привязки,
 *          от начала попытки зависящие, принимаемое подмножество не содержит.
 *          Проверки окружения ему не помеха по той же причине: длины они
 *          не имеют, а вердикт их зависит от одной лишь позиции. Помехой служит
 *          ряд повторения внутри тела проверки - позиции сопоставления он
 *          не двигает вовсе, отчего положение конца его пройденному тексту
 *          не отвечает, - и запрет ставится ровно на него, а не на всякую
 *          проверку окружения.
 *
 *          <b>Через вызов подпрограммы не переживает ни один регистр,
 *          сохранности какого соглашение о вызове не обещает.</b> Всё, чем
 *          сопоставление живёт, сохраняется в кадре вызова, хотя подпрограммы
 *          обстановки за иные из этих регистров и не берутся. Проверка показала,
 *          что берутся они за них по-разному при разной оптимизации: недостача
 *          сохранения проявляется при «-O0» и не проявляется при «-O2» и «-O3»,
 *          отчего выпускная сборка такую недостачу и не обнаруживает. Опорой
 *          верности служит правило, а не наблюдение за распределением регистров.
 *
 *          Проверить правило набором тестов нельзя: недостача сохранения
 *          наблюдаема лишь тогда, когда подпрограмма за этот регистр берётся,
 *          а взялась она или нет - дело распределителя регистров. Проверяется
 *          правило иначе: подпрограмме приписывается вставка на языке ассемблера,
 *          нарочно занимающая проверяемый регистр перед самым возвратом, после
 *          чего сличение с исполнением программы прогоняется дважды - с сохранением
 *          регистра в кадре и без него. Отсутствие расхождений в первом случае
 *          и наличие их во втором и означает, что сохранение своё дело делает.
 *
 *          <b>В режиме разбора UTF-8 кодогенерацию получают выражения,
 *          сопоставляющие одни лишь символы ASCII.</b> Порождаемый код
 *          сопоставляет байты, тогда как программа этого режима сопоставляет
 *          символы целиком, и сходятся два эти способа не всегда, но на символах
 *          ASCII - в точности: кодирование UTF-8 самосинхронизируется, байт ASCII
 *          внутри последовательности многобайтовой не встречается вовсе, отчего
 *          байты её все до единого получают отказ, каковой выдало бы и
 *          сопоставление символа целиком. Отказ же сплошной, всякому выражению
 *          этого режима, оставлял бы без ускорения разбор журналов на всяком
 *          языке, кроме английского, - тогда как выражения разбора этого
 *          сопоставляют разделители и поля записи, сплошь знаки ASCII, а символы
 *          иные лежат в значениях полей и сопоставляются классом отрицаемым либо
 *          любым символом, каковые подмножеством и отвергаются.
 *
 *          Отбор ведётся по сопоставляемым символам, а не по значениям байта:
 *          класс «[a-zф]» ни одного байта вне ASCII не сопоставляет, а символ
 *          сопоставляет. Отвергаются класс отрицаемый, ссылка на свойство
 *          Юникода, сопоставление любого символа и всякий диапазон, предел ASCII
 *          превысивший. Сопоставление без учёта регистра отвергается заодно
 *          и тогда, когда символ ASCII образует набор приведения регистра
 *          с символом вне ASCII: знак Кельвина «K» приводится к букве «k»,
 *          а долгое «ſ» - к букве «s».
 *
 *          Позиции начала попытки при этом перебираются по границам символов,
 *          а не по байтам: сопоставление хотя бы одного символа началом попытки
 *          посреди символа многобайтового не станет - байт продолжающий отказ
 *          получает, - но совпадение пустое сопоставления не несёт вовсе, и
 *          перебор побайтный выдал бы его посреди символа, тогда как программа
 *          сдвигает начало поиска на символ целиком. Пропуск байтов продолжающих
 *          обходится в три команды на позицию и порождается лишь этому режиму.
 *
 *          Замер на текстах, символы кириллицы несущих, показал долю
 *          к исполнению программы от 12,6 на чередовании ветвей до 21,1
 *          на выражении адреса сети.
 *
 *          <b>Кадр вызова разделён надвое: область сохранения регистров
 *          адресуется указателем стека, а места самого кадра - отдельным
 *          регистром записи.</b> Разделение это заведено заделом под вложенность:
 *          вызов рекурсивный и проход повторения над областью требуют записи
 *          своей на каждый уровень, а число уровней при порождении не известно.
 *          Смена адреса записи оставляет смещения мест прежними, отчего ни одно
 *          из обращений порождения при заведении вложенности не меняется.
 *
 *          Область сохранения при этом остаётся общей и на указателе стека
 *          намеренно, и довод тому не один. Первый - замкнутый круг: сам регистр
 *          записи вызовом подпрограммы затирается, и брать его назад из области,
 *          им же адресуемой, было бы нечем. Второй - область эта жива лишь
 *          на время вызова подпрограммы и отступления не переживает, отчего
 *          отдельной на каждую запись быть не должна.
 *
 *          <b>Цепочка ветвей, различаемых первым байтом, порождается разбором
 *          по нему, а не перебором ветвей.</b> Байт текста называет единственную
 *          ветвь, способную сойтись, а отказ её отказом всей цепочки и является.
 *          Оттого такая цепочка отказа не откладывает, мест кадра не занимает
 *          и - главное - в счёт цепочек не идёт: счёт этот переводит весь
 *          сопоставитель в ведение действующего отказа ячейкой кадра, а ведение
 *          это обходится переходом по адресу из памяти на всяком отказе,
 *          случившемся после выбора. Плата эта постоянная и от размера выбора
 *          не зависит: замер показал, что «^(GET|POST) x» обходился в 2,6 раза
 *          дороже «^GET x» - на цепочке из двух ветвей по три и четыре байта.
 *
 *          Границы групп цепочка эта всё же запоминает: ветвь, отказавшая
 *          после разбора, оставила бы записанные ею границы стоять, и попытка
 *          следующая приняла бы их за свои. Восстановление размещено на пути
 *          отказа, а не при переходе к ветви следующей: перехода этого здесь
 *          нет вовсе.
 *
 *          Разбору поддаётся не всякая цепочка. Ветвь обязана начинаться
 *          сопоставлением одиночного символа ASCII без приведения регистра,
 *          а байты ветвей - различаться. Запись границ захвата разбору
 *          не мешает и пропускается: текста она не поглощает и размещается
 *          прежде первого символа ветви. Приведение регистра разбор отменяет -
 *          под ним ветвь начинают два байта, а не один; цепочка же, ветви
 *          какой байтом не различаются, порождается перебором по-прежнему.
 *
 *          <b>Отбор позиций начала попытки ведётся началами строк, если
 *          выражение начинается привязкой «^» в режиме соответствия границам
 *          строк.</b> Выражение это совпадает лишь в начале текста да вслед
 *          за переводом строки, а позиции прочие отвергает первой же
 *          инструкцией: перебирать их незачем. Начало строки отыскивается
 *          поиском перевода строки набором команд процессора над несколькими
 *          байтами сразу, и текст проходится строками, а не байтами. Отбор
 *          по ведущему литералу разбору этому предпочитается: литерал
 *          разборчивее перевода строки. Привязка отыскивается обходом
 *          инструкций начальных с пропуском записи границ захвата, а инструкция
 *          иная отбор отменяет - привязка, за нею стоящая, обязательной быть
 *          перестаёт.
 *
 *          <b>Атомарные группы и просмотр вперёд порождаются подменой
 *          действующего отказа, а не набором точек возврата.</b> Порождаемый код
 *          ведёт отказ, действующий в очередной миг сопоставления, - меткой,
 *          известной при порождении, либо ячейкой кадра, если выражение несёт
 *          цепочки ветвей. Атомарность сводится к возвращению этого отказа
 *          к тому, каким он был при входе в группу: точки возврата, телом группы
 *          заведённые, после того недостижимы, а иного атомарность и не значит.
 *          Проверка же окружения подменяет действующий отказ своим и возвращает
 *          прежний на обоих исходах - и при сопоставлении тела, и при отказе
 *          его, - отчего выходит атомарной сама собою, чего эталон от неё
 *          и требует. Мера - одно место кадра на группу и четыре на проверку.
 *
 *          Тела их границ захватывающих групп нести не вправе. Границы
 *          порождённый код записывает прямо, без журнала, а восстанавливает их
 *          цепочка ветвей - при переходе к ветви следующей и при отказе своём.
 *          Атомарная группа отказ этот обходит стороной, а проверка
 *          отрицательная обязана границы, телом записанные, отменить - и
 *          отменить их порождённому коду нечем. Потому цепочка ветвей с захватом
 *          внутри атомарной группы и всякий захват внутри тела проверки
 *          кодогенерации не получают. Изъян этот был обнаружен сличением
 *          с разбором программы на порождаемых образцах, а не разбором
 *          устройства.
 *
 *          <b>Ретроспективная проверка перебирает длины возвратом к телу,
 *          порождённому единожды.</b> Сопоставляется она отступом назад на длину
 *          проверяемой последовательности, а длина эта заранее не одна: ветви
 *          тела бывают разной длины, и квантор ограниченный даёт их несколько.
 *          Порождать тело на каждую длину заново - значит множить код на разность
 *          длин, тогда как возврат к телу стоит перехода назад. Точек возврата
 *          произвольной глубины перебор не требует: всё его состояние - это
 *          положение начала отступа и количество отступов, ещё не испробованных,
 *          и оба они лежат местами кадра. Перебор прекращают два предела:
 *          исчерпание длин, наибольшей ограниченных, и начало текста, отступать
 *          за какое некуда. Длина неограниченная делает проверку несопоставимой
 *          вовсе - её отвергает и сборка выражения, и порождение, каковое на сборку
 *          не полагается: программа приходит и записью хранилища, а запись - извне.
 *
 *          Сопоставление тела обязано завершиться ровно в позиции сопоставления,
 *          и отказ этой проверки передаётся действующему отказу тела, а не
 *          невыполнению проверки: ряд повторения внутри тела обязан отступить
 *          и попробовать завершиться там, где надо, - и лишь исчерпав отступление,
 *          передать отказ проверке. Передача отказа прямо проверке оставляла бы
 *          без совпадения выражения вида «(?<=\w{1,5})x» на всяком тексте, где ряд
 *          перерастает отступ.
 *
 *          <b>Обратная ссылка сличается двумя ходячими положениями, а стека
 *          возврата не требует.</b> Ссылка либо сходится целиком, либо
 *          отказывает - отступать внутрь неё некуда. Отступление ведёт ряд
 *          повторения, ссылке предшествующий, и он же повторяет сличение заново,
 *          уже с иным концом захвата: ровно так и работает «(\w+) \1».
 *
 *          Сличение ведётся двумя положениями - по захваченному тексту и по
 *          тексту в позиции сопоставления, - а не длиною отрезка: вычитания
 *          положений набор команд порождателя не выражает вовсе, сложение
 *          и вычитание там принимают лишь постоянные. Конец захваченного
 *          отрезка лежит местом кадра и вычитывается на каждом ходе: держать
 *          его регистром нечем, а лишнее чтение из кадра дешевле нехватки
 *          регистра. Место отводится одно на весь сопоставитель: сличение идёт
 *          прямым ходом и вложенным быть не может.
 *
 *          Ссылка без учёта регистра кодогенерации не получает: приведение
 *          регистра изменяет длину символа в байтах, отчего сличаемые участки
 *          надлежит проходить посимвольно и независимо друг от друга, тогда как
 *          порождаемый код сличает байты. Пропуска же пройденного участка
 *          не получают выражения со ссылками вовсе - см. довод о пропуске выше:
 *          вердикт ссылки зависит от содержимого захвата, то есть от начала
 *          попытки, а не от одной лишь позиции.
 *
 *          <b>Отказ порождения изъяном не является.</b> Кодогенерация принимает
 *          подмножество программ, а не всякую программу: принятое исполняется
 *          быстрее, непринятое - как прежде. Расширение подмножества - работа
 *          последующая, а не условие пригодности.
 *
 * \~english
 * @brief Header file of the conversion of the program of a regular expression into machine code —
 *        the Codegen class, which generates a matcher for the subset of programs executable
 *        without a set of backtracking points of arbitrary depth
 * @section codegen_decisions Deliberate decisions
 * @details What is listed below looks like an incongruity, but was chosen deliberately and
 *          is not subject to correction. The section is introduced so that reading the code does not start
 *          every time from the same conclusions.
 *          <b>Every match of a single character is generated as a reference
 *          to the byte belonging table.</b> A single character, a character class
 *          and any character differ only by the content of the table, which is why the generated
 *          code is one and the same for them, while the difference goes into the execution context.
 *          Generating separate code for every kind of match would give a gain
 *          only on a single character — one comparison instead of a reference to the table —
 *          but would take relocatability away from the generated code: the value of the character
 *          would have to be placed in the instruction itself.
 *          <b>A run of a repetition of a single character is walked as a whole, while the retreat
 *          is performed one position at a time.</b> Such an order corresponds to a greedy
 *          repetition exactly and requires no set of backtracking points: the retreat position
 *          of every run is kept in the call frame, and the number of runs
 *          is known at generation time. Expressions requiring a set of backtracking points
 *          of arbitrary depth — nested repetitions, backreferences, lookarounds —
 *          receive no code generation at all and are executed by the program,
 *          as before. The alternation branches, on the other hand, require no backtracking points of arbitrary depth:
 *          their number is known at generation time on a par with the number
 *          of runs, which is why they do receive code generation.
 *          <b>The preliminary selection of positions is performed by a subroutine call
 *          rather than generated in the code itself.</b> The selection searches the text for the leading literal
 *          of a match or for the single admissible starting byte, and searching for a
 *          sequence in the text is performed by processor instructions
 *          over several bytes at once. Generating a byte-by-byte walk instead of
 *          a call would cost more than the call itself on any text where the selection
 *          skips whole stretches — and other stretches the selection does not skip at all.
 *          The price of the decision is saving the registers clobbered by the call in the frame,
 *          performed once per match attempt.
 *          <b>The positions where an attempt begins are sifted in the code itself if the selection
 *          for them was not generated.</b> The set of admissible starting bytes is sometimes described
 *          neither by a leading literal nor by a single byte — such is every alternation
 *          of branches whose literals begin differently — and such an expression receives no
 *          position selection at all, trying all the branches at every position.
 *          The sifting skips the positions whose byte is not given to begin a match,
 *          at the price of two memory references instead of a match attempt
 *          as a whole.
 *          The argument against the set of admissible bytes given above concerns
 *          a subroutine call rather than generation in the code: the subroutine speeds things up
 *          by skipping stretches, and the sifting by the cheapness of the move. Measurement on the
 *          expression «alpha|bravo|charlie|delta|echo|foxtrot» over a text
 *          of sixty-four kilobytes showed a ratio to executing the program
 *          of 0.65 without the sifting and 2.99 with it, and the expressions with a narrow set of bytes
 *          gained as well: «[0-9]{3,5}» — from 3.64 to 4.83.
 *          <b>The boundaries of the capturing groups are remembered only on the chains of alternation
 *          branches, and not on the retreat of a run.</b> The retreat returns
 *          the execution to the matching following the run, which is why the saves
 *          placed after it are performed anew, while those placed before it
 *          are not touched by the retreat. A branch whose matching has been broken, on the other hand,
 *          would leave the boundaries it wrote established, and the next branch
 *          will not overwrite them — that is why a chain of branches remembers its boundaries
 *          at the entry and restores them at the transition to the next branch and at
 *          the exhaustion of the chain. The set of remembered cells is known at generation time:
 *          it is collected by walking the instruction area of the chain by addresses rather than by
 *          execution paths. A superfluous cell in the set costs two memory
 *          references, whereas a missing one would give wrong boundaries.
 *          <b>The walk of a run is generated by its limit rather than by parsing the table
 *          always.</b> Parsing the belonging table reads from memory twice
 *          per every byte — the byte itself and the table record — whereas runs of a simple
 *          arrangement are walked cheaper. A run accepting every byte value
 *          runs only into the end of the text and is generated as setting the position
 *          to its size, with no loop at all. A run bounded by a single
 *          value is walked by searching for that value: searching for a byte in the text
 *          is performed by processor instructions over several bytes at once.
 *          The other runs are walked by parsing the table, as before.
 *          The number of values not belonging to the run is counted at generation time
 *          by walking that very table which was introduced for the run: the choice of the path
 *          rests on what has been built rather than on the kind of the expression. Measurement on the expression
 *          «.*needle» over a text of sixty-four kilobytes showed a ratio
 *          to executing the program of 0.68 with parsing the table and 1.90 with searching for
 *          the limit.
 *          The difference from the retreat of a run by a backward search, rejected by measurement,
 *          is in the size of what is being replaced: there a subroutine call replaced one
 *          retreat, here the walk of a run as a whole.
 *          <b>A lazy repetition is generated as an advance on failure rather than
 *          as a walk of a run.</b> A lazy run goes counter to a greedy one: the matching
 *          continues at once, and the body absorbs a character only on the failure
 *          of the continuation. That is why a lazy run has no walk and no retreat at all,
 *          while it occupies the same place in the frame — its own position. Marking lazy
 *          repetitions is done by a set separate from the greedy ones: a mark in a common
 *          set would make the interpreters walk a lazy run greedily.
 *          <b>The failure of an attempt skips the stretch walked by the first run
 *          rather than shifting the beginning of the attempt by one position.</b> The failure of an attempt that has
 *          exhausted a run means that the tail of the expression did not converge at any
 *          retreat position. A later beginning of the attempt shifts the beginning of the
 *          run forward and leaves its end the same — the byte not belonging
 *          to the run stands still — which is why the set of checked positions
 *          will turn out to be a subset of the already rejected one. Without the skip the enumeration
 *          walks the stretch again and gives the square of the length of the run: measurement on the
 *          expression «a.*z» without a match showed a ratio of 0.004 to executing the
 *          program, and with the skip — 44.7.
 *          The position of the skip is the end of the run minus the length of the stretch
 *          preceding the run: the next attempt must begin where the run
 *          will begin past the end of the current one, and not past its own end. That length
 *          is known at generation time, for before the first run only
 *          matches of single characters are placed. Without the subtraction the skip passes
 *          by the positions not yet opened to the run — that defect was found
 *          by generated samples and fixed by corruption.
 *          The legitimacy rests on two supports, each of which was checked
 *          by corruption. The first is the constancy of the length of the stretch preceding the run:
 *          alternation branches of different lengths would open positions not yet checked,
 *          which is why expressions with chains of branches receive no skip at all.
 *          The second is the position of the end, remembered by the first run and reset
 *          at every attempt: the subsequent runs begin from the position
 *          changed by the retreat of the first run, and a remainder from the previous attempt
 *          would skip an unwalked stretch.
 *          The tail of the expression converges or not by one position alone: the anchors
 *          depending on the beginning of the attempt are not held by the accepted subset.
 *          <b>Not a single register whose preservation the calling convention does not promise
 *          survives a subroutine call.</b> Everything the
 *          matching lives by is saved in the call frame, although the context subroutines
 *          do not even take some of those registers. A check showed
 *          that they take them differently under different optimisation: a missing
 *          save shows up at «-O0» and does not show up at «-O2» and «-O3»,
 *          which is why a release build does not reveal such a shortage. The support
 *          of correctness is the rule rather than watching the register allocation.
 *          The rule cannot be checked by a set of tests: a missing save
 *          is observable only when the subroutine does take that register,
 *          and whether it took it or not is the business of the register allocator. The rule is checked
 *          otherwise: an assembly-language insert is ascribed to the subroutine,
 *          deliberately occupying the checked register right before the return, after
 *          which the comparison with executing the program is run twice — with the register saved
 *          in the frame and without it. The absence of divergences in the first case
 *          and their presence in the second is what means that the save does its job.
 *          <b>In the UTF-8 parsing mode code generation is received by the expressions
 *          matching ASCII characters alone.</b> The generated code
 *          matches bytes, whereas the program of that mode matches
 *          whole characters, and those two ways do not always converge, but on ASCII
 *          characters they do exactly: the UTF-8 encoding is self-synchronising, an ASCII byte
 *          never occurs inside a multibyte sequence at all, which is why
 *          every single byte of it receives the refusal that matching the whole character
 *          would give as well. A blanket refusal for every expression
 *          of that mode, on the other hand, would leave log parsing without speed-up in every
 *          language except English — whereas the expressions of that parsing
 *          match the separators and the fields of a record, all ASCII signs, while other
 *          characters lie in the field values and are matched by a negated class or
 *          by any character, which are rejected by the subset.
 *          The selection is driven by the matched characters rather than by the byte values:
 *          the class «[a-zф]» matches no byte outside ASCII, while a character
 *          it does match. Rejected are a negated class, a reference to a Unicode
 *          property, matching any character and every range that exceeded the ASCII
 *          limit. Case-insensitive matching is rejected as well
 *          when an ASCII character forms a case conversion set
 *          with a character outside ASCII: the Kelvin sign «K» converts to the letter «k»,
 *          and the long «ſ» to the letter «s».
 *          The positions where an attempt begins are then enumerated by character boundaries
 *          rather than by bytes: matching at least one character will not begin an attempt
 *          in the middle of a multibyte character — a continuation byte receives a refusal —
 *          but an empty match carries no matching at all, and
 *          a byte-by-byte enumeration would yield it in the middle of a character, whereas the program
 *          shifts the beginning of the search by a whole character. Skipping the continuation bytes
 *          costs three instructions per position and is generated only for that mode.
 *          Measurement on texts carrying Cyrillic characters showed a ratio
 *          to executing the program from 12.6 on an alternation of branches to 21.1
 *          on a network address expression.
 *          <b>A refusal to generate is not a defect.</b> Code generation accepts
 *          a subset of programs rather than every program: what is accepted is executed
 *          faster, what is not accepted as before. Extending the subset is subsequent
 *          work rather than a condition of fitness.
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_REGEX_CODEGEN__
#define __AWH_REGEX_CODEGEN__

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cstddef>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "program.hpp"
#include "emitter.hpp"
#include "assembly.hpp"

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
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён модуля регулярных выражений
	 *
	 * \~english
	 * @brief Namespace of the regular expression module
	 *
	 * \~
	 */
	namespace regex {
		/**
		 * \~russian
		 * @brief Наибольшее допустимое количество рядов повторения в порождаемом коде
		 *
		 * @details Положение отступления каждого ряда занимает место в кадре вызова,
		 *          а кадр размещается при входе в сопоставитель, поэтому количество
		 *          рядов ограничивается. Выражения с большим количеством рядов
		 *          кодогенерации не получают.
		 *
		 * \~english
		 * @brief Largest admissible number of repetition runs in the generated code
		 * @details The retreat position of every run occupies a place in the call frame,
		 *          and the frame is allocated at the entry into the matcher, therefore the number
		 *          of runs is bounded. Expressions with a larger number of runs
		 *          receive no code generation.
		 *
		 * \~
		 */
		constexpr size_t MAX_RUNS = 0x20;

		/**
		 * \~russian
		 * @brief Наибольшее допустимое количество цепочек ветвей в порождаемом коде
		 *
		 * @details Позиция начала выбора и адрес возврата выбранной ветви занимают
		 *          места в кадре вызова, а кадр размещается при входе в сопоставитель,
		 *          поэтому количество цепочек ветвей ограничивается.
		 *
		 * \~english
		 * @brief Largest admissible number of branch chains in the generated code
		 * @details The position where an alternation begins and the return address of the chosen branch occupy
		 *          places in the call frame, and the frame is allocated at the entry into the matcher,
		 *          therefore the number of branch chains is bounded.
		 *
		 * \~
		 */
		constexpr size_t MAX_CHAINS = 0x20;

		/**
		 * \~russian
		 * @brief Наибольшая допустимая длина тела повторения без записи кадра
		 *
		 * @details Отдача прохода такого повторения есть вычитание длины тела
		 *          из позиции сопоставления, а вычитание порождается значением
		 *          непосредственным, разрядность какого у наборов команд
		 *          ограничена. Тело длиннее получает записи кадра на проход.
		 *
		 * \~english
		 * @brief Largest admissible length of a repetition body without a frame record
		 * @details Giving back a pass of such a repetition is a subtraction of the body length
		 *          from the matching position, and the subtraction is emitted with an immediate
		 *          value, whose width is bounded in the instruction sets. A longer body
		 *          receives frame records per pass.
		 *
		 * \~
		 */
		constexpr size_t MAX_STRETCH = 0x400;

		/**
		 * \~russian
		 * @brief Наибольшее допустимое количество атомарных групп в порождаемом коде
		 *
		 * @details Отказ сопоставления, действовавший при входе в атомарную группу,
		 *          занимает место в кадре вызова, а кадр размещается при входе
		 *          в сопоставитель, поэтому количество групп ограничивается.
		 *
		 * \~english
		 * @brief Largest admissible number of atomic groups in the generated code
		 * @details The failure in effect at the entry into an atomic group occupies a place
		 *          in the call frame, and the frame is allocated at the entry into the matcher,
		 *          therefore the number of groups is bounded.
		 *
		 * \~
		 */
		constexpr size_t MAX_ATOMICS = 0x20;

		/**
		 * \~russian
		 * @brief Количество мест кадра вызова, отводимых проверке окружения
		 *
		 * @details Мест четыре: позиция сопоставления, какую проверка обязана вернуть
		 *          после исполнения своего тела, отказ сопоставления, действовавший
		 *          при входе в проверку, положение начала очередного отступа назад
		 *          и количество оставшихся отступов. Два последних места нужны
		 *          проверке ретроспективной: длина проверяемой последовательности
		 *          перебирается от наименьшей к наибольшей. Проверке предваряющей
		 *          они не нужны, однако разметка кадра ведётся единой: место лишнее
		 *          стоит восьми байтов однажды на сопоставитель.
		 *
		 * \~english
		 * @brief Number of places of the call frame allotted to a lookaround check
		 * @details There are four places: the matching position, which the check is obliged to restore
		 *          after executing its body, the matching failure that was in effect
		 *          at the entry into the check, the position where the current backward
		 *          offset begins and the number of the remaining offsets. The last two places
		 *          are needed by a lookbehind check: the length of the checked sequence
		 *          is iterated from the smallest to the largest. A lookahead check
		 *          does not need them, but the frame layout is kept uniform: an extra place
		 *          costs eight bytes once per matcher.
		 *
		 * \~
		 */
		constexpr size_t SIGHTS = 4;

		/**
		 * \~russian
		 * @brief Наибольшее допустимое количество проверок окружения в порождаемом коде
		 *
		 * @details Всякая проверка занимает свои места в кадре вызова, а кадр
		 *          размещается при входе в сопоставитель, поэтому количество
		 *          проверок ограничивается.
		 *
		 * \~english
		 * @brief Largest admissible number of lookaround checks in the generated code
		 * @details Every check occupies its own places in the call frame, and the frame
		 *          is allocated at the entry into the matcher, therefore the number of
		 *          checks is bounded.
		 *
		 * \~
		 */
		constexpr size_t MAX_LOOKS = 0x20;

		/**
		 * \~russian
		 * @brief Класс преобразования программы регулярного выражения в машинный код
		 *
		 * @details Класс порождает сопоставитель для программы, исполнимой проходом
		 *          рядов повторения с отступлением по одной позиции, и удерживает
		 *          вместе с ним обстановку исполнения: таблицы принадлежности байтов
		 *          и набор их адресов. Владение порождённым кодом единоличное.
		 *
		 * \~english
		 * @brief Class of the conversion of the program of a regular expression into machine code
		 * @details The class generates a matcher for a program executable by walking
		 *          the repetition runs with a retreat one position at a time, and holds
		 *          the execution context together with it: the byte belonging tables
		 *          and the set of their addresses. Ownership of the generated code is exclusive.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Codegen {
			private:
				/**
				 * \~russian
				 * @brief Тип вызова порождённого сопоставителя
				 *
				 * @details Соглашение о вызове: адрес текста, размер текста, позиция
				 *          начала попытки, адрес набора границ и адрес таблицы адресов
				 *          обстановки исполнения.
				 *
				 * \~english
				 * @brief Call type of the generated matcher
				 * @details The calling convention: the address of the text, the size of the text, the position
				 *          where the attempt begins, the address of the set of boundaries and the address of the address table
				 *          of the execution context.
				 *
				 * \~
				 */
				typedef bool (* matcher_t) (const char *, size_t, size_t, size_t *, const void *);
			private:
				// Исполняемая память порождённого сопоставителя
				assembly_t _assembly;
			private:
				/**
				 * \~russian
				 * Хранилище значений, к каким обращается порождённый код
				 *
				 * @details Хранилище несёт таблицы принадлежности значений байта
				 *          сопоставляемым символам по двести пятьдесят шесть байт
				 *          на каждую и приметы привязок к позиции в тексте
				 *          по восьми байтов на каждую, размещённые одно за другим.
				 *          Единое хранилище взамен раздельных заведено затем, чтобы
				 *          набор адресов обстановки восстанавливался единообразно:
				 *          размещение значения перемещает хранилище в памяти,
				 *          отчего адреса собираются заново по завершении порождения.
				 *
				 * \~english
				 * Storage of the values the generated code refers to
				 * @details The storage carries the tables of the belonging of the byte values to
				 *          the matched characters, two hundred and fifty-six bytes
				 *          for each, and the descriptors of the anchors to a position in the text,
				 *          eight bytes for each, placed one after another.
				 *          A single storage instead of separate ones was introduced so that
				 *          the set of the context addresses is restored uniformly:
				 *          placing a value moves the storage in memory,
				 *          which is why the addresses are collected anew when the generation has finished.
				 *
				 * \~
				 */
				vector <uint8_t> _members;
			private:
				// Смещения значений хранилища, отвечающие местам обстановки
				vector <size_t> _offsets;
			private:
				/**
				 * \~russian
				 * Предварительный отбор позиций начала попытки сопоставления
				 *
				 * @details Отбор копируется из программы, а не берётся по ссылке
				 *          на неё: порождённый код обращается к нему при каждой
				 *          попытке сопоставления и переживает программу,
				 *          для какой порождён.
				 *
				 * \~english
				 * Preliminary selection of the positions where a match attempt begins
				 * @details The selection is copied from the program rather than taken by a reference
				 *          to it: the generated code refers to it at every
				 *          match attempt and outlives the program
				 *          it was generated for.
				 *
				 * \~
				 */
				prefilter_t _prefilter;
			private:
				// Набор адресов обстановки исполнения порождённого кода
				vector <const void *> _context;
			private:
				// Количество захватывающих групп выражения порождённого сопоставителя
				uint32_t _captures;
			private:
				/**
				 * \~russian
				 * Размер записи кадра порождённого сопоставителя в байтах
				 *
				 * @details Область записей отводится вызовом сопоставления,
				 *          а не кадром вызова: проходы повторения над областью
				 *          и вызовы рекурсивные требуют записи своей на каждый
				 *          уровень, и число уровней доходит до длины текста.
				 *          Отведение их на стеке машины снесло бы его.
				 *
				 * \~english
				 * Size of the record of the frame of the generated matcher in bytes
				 *
				 * \~
				 */
				uint32_t _frame;
			private:
				/**
				 * \~russian
				 * Наибольшее количество записей кадра порождённого сопоставителя
				 *
				 * @details Исчерпание их отказом сопоставления не является:
				 *          сопоставитель отвечает «не берусь», и выражение
				 *          доигрывает исполнение программы.
				 *
				 * \~english
				 * Largest number of records of the frame of the generated matcher
				 *
				 * \~
				 */
				uint32_t _levels;
			private:
				// Опознание программы, для какой порождён сопоставитель
				uint64_t _identity;
			private:
				// Вызов порождённого сопоставителя
				matcher_t _matcher;
			private:
				/**
				 * \~russian
				 * @brief Метод заведения таблицы принадлежности байтов сопоставления
				 *
				 * @param instruction сопоставляющая инструкция программы
				 * @param program     программа регулярного выражения
				 * @return            номер заведённой таблицы в обстановке исполнения
				 *
				 * \~english
				 * @brief Method of introducing a byte belonging table of a match
				 * @param instruction matching instruction of the program
				 * @param program     program of the regular expression
				 * @return            number of the introduced table in the execution context
				 *
				 * \~
				 */
				size_t table(const instruction_t & instruction, const program_t & program) noexcept;
				/**
				 * \~russian
				 * @brief Метод заведения таблицы объединения байтов сопоставления
				 *
				 * @details Таблица несёт байты, какие принимает хотя бы одна
				 *          из указанных сопоставляющих инструкций. Она даёт
				 *          отбор прохода повторения над областью: байт, ни одной
				 *          инструкции не подошедший, отвергает тело целиком,
				 *          и проход отменяется прежде отведения записи кадра.
				 *
				 * @param leaders набор адресов сопоставляющих инструкций
				 * @param program программа регулярного выражения
				 * @return        номер заведённой таблицы в обстановке исполнения
				 *
				 * \~english
				 * @brief Method of introducing a union byte belonging table of a match
				 * @param leaders set of addresses of the matching instructions
				 * @param program program of the regular expression
				 * @return        number of the introduced table in the execution context
				 *
				 * \~
				 */
				size_t masking(const std::vector <address_t> & leaders, const program_t & program) noexcept;
				/**
				 * \~russian
				 * @brief Метод заведения приметы привязки к позиции в тексте
				 *
				 * @details Примета несёт тип привязки и набор режимов компиляции
				 *          инструкции: проверка привязки выполняется подпрограммой
				 *          обстановки, а не порождённым кодом, если порождению
				 *          она не поддаётся.
				 *
				 * @param instruction инструкция привязки к позиции в тексте
				 * @return            номер заведённой приметы в обстановке исполнения
				 *
				 * \~english
				 * @brief Method of introducing a descriptor of an anchor to a position in the text
				 * @details The descriptor carries the type of the anchor and the set of compilation modes
				 *          of the instruction: the check of the anchor is performed by a context
				 *          subroutine rather than by the generated code, if it does not lend itself
				 *          to generation.
				 * @param instruction instruction of the anchor to a position in the text
				 * @return            number of the introduced descriptor in the execution context
				 *
				 * \~
				 */
				size_t guard(const instruction_t & instruction) noexcept;
				/**
				 * \~russian
				 * @brief Метод заведения значения байта, ряд повторения ограничивающего
				 *
				 * @param letter значение байта, ряд повторения ограничивающее
				 * @return       номер заведённого значения в обстановке исполнения
				 *
				 * \~english
				 * @brief Method of introducing the byte value bounding a repetition run
				 * @param letter byte value bounding a repetition run
				 * @return       number of the introduced value in the execution context
				 *
				 * \~
				 */
				size_t limiter(const uint8_t letter) noexcept;
				/**
				 * \~russian
				 * @brief Метод заведения таблицы допустимых начальных байтов совпадения
				 *
				 * @return номер заведённой таблицы в обстановке исполнения
				 *
				 * \~english
				 * @brief Method of introducing the table of the admissible starting bytes of a match
				 * @return number of the introduced table in the execution context
				 *
				 * \~
				 */
				size_t sieve() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки применимости кодогенерации к программе
				 *
				 * @details Метод проверяет принадлежность программы подмножеству,
				 *          получающему кодогенерацию, не порождая кода.
				 *
				 * @param program проверяемая программа регулярного выражения
				 * @return        результат проверки применимости кодогенерации
				 *
				 * \~english
				 * @brief Method of checking the applicability of code generation to a program
				 * @details The method checks the belonging of the program to the subset
				 *          receiving code generation, without generating any code.
				 * @param program program of the regular expression to check
				 * @return        result of checking the applicability of code generation
				 *
				 * \~
				 */
				static bool applicable(const program_t & program) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод порождения сопоставителя программы
				 *
				 * @details Порождение отказывается на программах, подмножеству
				 *          не принадлежащих, и на сборках, кодогенерации не получивших.
				 *
				 * @param program программа регулярного выражения
				 * @return        результат порождения сопоставителя
				 *
				 * \~english
				 * @brief Method of generating the matcher of a program
				 * @details The generation refuses on the programs not belonging to the subset
				 *          and on the builds that have received no code generation.
				 * @param program program of the regular expression
				 * @return        result of generating the matcher
				 *
				 * \~
				 */
				bool compile(const program_t & program) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод записи порождённого сопоставителя
				 *
				 * @param result запись порождённого сопоставителя
				 * @return       результат записи порождённого сопоставителя
				 *
				 * @details Записываются порождённый машинный код, хранилище
				 *          значений обстановки исполнения и смещения их. Прочее
				 *          обстановка восстановлением собирает сама: четыре адреса
				 *          подпрограмм и адрес предварительного отбора позиций
				 *          принадлежат исполняемому образу, а адреса значений
				 *          хранилища выводятся из смещений.
				 *
				 * \~english
				 * @brief Method of writing the generated matcher
				 * @param result record of the generated matcher
				 * @return       result of writing the generated matcher
				 * @details What is written is the generated machine code, the storage
				 *          of the execution context values and their offsets. The rest
				 *          the context collects itself on restoration: four subroutine addresses
				 *          and the address of the preliminary selection of positions
				 *          belong to the executable image, while the addresses of the storage values
				 *          are derived from the offsets.
				 *
				 * \~
				 */
				bool save(string & result) const noexcept;
				/**
				 * \~russian
				 * @brief Метод восстановления порождённого сопоставителя
				 *
				 * @param data    запись порождённого сопоставителя
				 * @param offset  позиция чтения записи
				 * @param program программа порождённого сопоставителя
				 * @return        результат восстановления сопоставителя
				 *
				 * @details Восстановление обходится без обхода программы и без
				 *          сборки команд: код размещается в исполняемой памяти
				 *          как есть. Годен он лишь набору команд, для какого
				 *          порождён, поэтому запись несёт его опознание, а
				 *          несовпадение оборачивается отказом - и порождением
				 *          заново, если вызывающая сторона того желает.
				 *
				 * \~english
				 * @brief Method of restoring the generated matcher
				 * @param data    record of the generated matcher
				 * @param offset  reading position in the record
				 * @param program program of the generated matcher
				 * @return        result of restoring the matcher
				 * @details The restoration does without walking the program and without
				 *          assembling instructions: the code is placed in executable memory
				 *          as it is. It is fit only for the instruction set it was
				 *          generated for, therefore the record carries its identification, and
				 *          a mismatch turns into a refusal — and into generating it
				 *          anew, if the calling side so wishes.
				 *
				 * \~
				 */
				bool restore(string_view data, size_t & offset, const program_t & program) noexcept;
				/**
				 * \~russian
				 * @brief Метод очистки порождённого сопоставителя
				 *
				 * \~english
				 * @brief Method of clearing the generated matcher
				 *
				 * \~
				 */
				void clear() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод сопоставления регулярного выражения порождённым кодом
				 *
				 * @param text     текст для сопоставления
				 * @param start    позиция начала поиска совпадения
				 * @param captures набор границ обнаруженного совпадения
				 * @return         результат поиска совпадения
				 *
				 * \~english
				 * @brief Method of matching a regular expression by the generated code
				 * @param text     text to match
				 * @param start    position to start the search for a match from
				 * @param captures set of the boundaries of the found match
				 * @return         result of searching for a match
				 *
				 * \~
				 */
				bool exec(string_view text, const size_t start, vector <pair <size_t, size_t>> & captures) const noexcept;
				/**
				 * \~russian
				 * @brief Метод поиска совпадения порождённым сопоставителем
				 *
				 * @param text     текст поиска совпадения
				 * @param start    позиция начала поиска совпадения
				 * @param captures набор границ совпадения и захваченных групп
				 * @param refused  признак отказа сопоставителя от сопоставления
				 * @return         результат поиска совпадения
				 *
				 * @details Отказ означает исчерпание области записей кадра
				 *          повторением над областью, а не отсутствие совпадения:
				 *          выражение надлежит доиграть исполнением программы.
				 *          Вердикт «нет совпадения» при отказе разошёлся бы
				 *          с исполнением программы.
				 *
				 * \~english
				 * @brief Method of searching for a match by the generated matcher
				 * @param text     text of the search for a match
				 * @param start    position of the start of the search for a match
				 * @param captures set of the bounds of the match and the captured groups
				 * @param refused  indication of the refusal of the matcher to match
				 * @return         result of the search for a match
				 * @details The refusal means the exhaustion of the area of the records of the frame
				 *          by a repetition over a region rather than the absence of a match.
				 *
				 * \~
				 */
				bool exec(string_view text, const size_t start, vector <pair <size_t, size_t>> & captures, bool & refused) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки готовности порождённого сопоставителя
				 *
				 * @return результат проверки готовности порождённого сопоставителя
				 *
				 * \~english
				 * @brief Method of checking the readiness of the generated matcher
				 * @return result of checking the readiness of the generated matcher
				 *
				 * \~
				 */
				bool ready() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения опознания программы порождённого сопоставителя
				 *
				 * @return опознание программы порождённого сопоставителя
				 *
				 * \~english
				 * @brief Method of getting the program identification of the generated matcher
				 * @return program identification of the generated matcher
				 *
				 * \~
				 */
				uint64_t identity() const noexcept;
				/**
				 * \~russian
				 * @brief Метод извлечения размера порождённого машинного кода
				 *
				 * @return размер порождённого машинного кода в байтах
				 *
				 * \~english
				 * @brief Method of getting the size of the generated machine code
				 * @return size of the generated machine code in bytes
				 *
				 * \~
				 */
				size_t length() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор присванивания
				 *
				 * \~english
				 * @brief Assignment operator
				 *
				 * \~
				 */
				Codegen & operator = (const Codegen &) noexcept = delete;
				/**
				 * \~russian
				 * @brief Конструктор копирования
				 *
				 *
				 * \~english
				 * @brief Copy constructor
				 *
				 * \~
				 */
				Codegen(const Codegen &) noexcept = delete;
			public:
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
				Codegen() noexcept;
		} codegen_t;
	};
};

#endif // __AWH_REGEX_CODEGEN__
